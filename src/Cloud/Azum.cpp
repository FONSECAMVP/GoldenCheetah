#include "Azum.h"
#include "Settings.h"
#include "Secrets.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QWebEngineView>

#ifndef AZUM_DEBUG
#define AZUM_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (AZUM_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (AZUM_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif


Azum::Azum(Context *context, QNetworkAccessManager *injectedNam)
    : CloudService(context, injectedNam), context(context), root_(NULL)
{
    printd("Azum::Azum\n");

    // DEC-040 Stage 1 (S-1) - the manager is CloudService's now, and it is not
    // built until something actually asks for it (nam()). Nothing is created
    // here, so this constructor is inert when the factory runs it pre-main.
    //
    // The sslErrors connect that used to sit here has moved to wireNam(), which
    // the base calls exactly once, when the manager comes into being. It cannot
    // stay in a constructor: there is no manager to connect to yet.

    // how is data uploaded and downloaded
    downloadCompression = none;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_AZUM_ACCESS_TOKEN);
    settings.insert(Local1, GC_AZUM_REFRESH_TOKEN);
    settings.insert(URL, GC_AZUM_URL);
    settings.insert(DefaultURL, "https://training.azum.com");
    settings.insert(AthleteID, GC_AZUM_ATHLETE_ID);
    settings.insert(Key, GC_AZUM_USERKEY);
}

Azum::~Azum() {
    printd("Azum::~Azum\n");
    // DEC-040 Stage 1 (S-1) - `if (context) delete nam;` removed; CloudService is
    // the sole owner and sole deleter of the manager.
}

// DEC-040 Stage 1 (S-1) - called by CloudService::nam() EXACTLY ONCE, the first
// time a manager exists. This is the same connect that used to live in the
// constructor; only its timing changed, because with lazy creation the
// constructor no longer has a manager to connect to.
void
Azum::wireNam(QNetworkAccessManager *nam)
{
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void
Azum::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    printd("Azum::onSslErrors\n");
    sslErrors(context->mainWindow, reply, errors);
}

bool
Azum::open(QStringList &errors)
{
    printd("Azum::open\n");

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    if (token == "") {
        errors << tr("There is no token");
        return false;
    }

    QString athleteId = getSetting(GC_AZUM_ATHLETE_ID, "").toString();
    if (athleteId == "") {
        errors << tr("There is no selected athlete");
        return false;
    }

    printd("Get access token for this session.\n");
    QString clientSecret = getSetting(GC_AZUM_USERKEY, "").toString().trimmed();
    if (clientSecret == "")
        clientSecret = GC_AZUM_CLIENT_SECRET;

    // refresh endpoint
    QString baseUrl = QString("%1/oauth/token/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString());
    QNetworkRequest request(baseUrl);
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    // set params
    QString data;
    data += "client_id=" GC_AZUM_CLIENT_ID;
    data += "&client_secret=" + clientSecret;
    data += "&refresh_token=" + getSetting(GC_AZUM_REFRESH_TOKEN).toString();
    data += "&grant_type=refresh_token";

    // make request - DEC-040 Stage 1 (W2), bounded by the generic auth timeout
    const RequestResult result = blockingRequest(nam()->post(request, data.toLatin1()), kOpenTimeoutMs);

    printd("HTTP response code: %d\n", result.httpStatus);

    // Covers a network error AND a timeout: both mean we do not have a token, and
    // neither may be allowed to fall through to the parse below with an empty body.
    if (!result.ok()) {
        printd("Got error %s\n", result.errorString.toStdString().c_str());
        errors << result.errorString;
        return false;
    }

    QByteArray r = result.body;
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        errors << tr("JSON parser error") << parseError.errorString();
        return false;
    }

    QString access_token = document.object()["access_token"].toString();
    QString refresh_token = document.object()["refresh_token"].toString();

    // update our settings
    if (access_token != "") setSetting(GC_AZUM_ACCESS_TOKEN, access_token);
    if (refresh_token != "") setSetting(GC_AZUM_REFRESH_TOKEN, refresh_token);

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Azum::close()
{
    printd("Azum::close\n");
    return true;
}

// home dire
QString
Azum::home()
{
    printd("Azum::home\n");
    return "";
}

bool
Azum::createFolder(QString)
{
    printd("Azum::createFolder\n");
    return false;
}


QList<CloudServiceEntry*>
Azum::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Azum::readdir(%s)\n", path.toStdString().c_str());
    QList<CloudServiceEntry*> returning;
    QString athleteId = getSetting(GC_AZUM_ATHLETE_ID, "").toString();
    if (athleteId == "") {
        errors << tr("No selected athlete");
        return returning;
    }

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    qDebug() << "token" << token;
    QString baseUrl = QString("%1/api/goldencheetah/athletes/%2/activities/?")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString())
          .arg(athleteId);
    QUrlQuery params;
    params.addQueryItem("date_from", from.toString("yyyy-MM-dd"));
    params.addQueryItem("date_to", to.toString("yyyy-MM-dd"));
    QUrl next = QUrl(baseUrl + params.toString());

    do {
        QNetworkRequest request(next);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        // DEC-040 Stage 1 (W2) - bounded, once per page of this do/while.
        const RequestResult result = blockingRequest(nam()->get(request), kListTimeoutMs);

        // A page that failed or timed out ends the listing here. Following
        // `next` after a failure would silently skip a page and return a
        // short list that is indistinguishable from a complete one.
        if (!result.ok()) {
            errors << result.errorString;
            return returning;
        }

        // did we get a good response ?
        QByteArray r = result.body;
        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            next = document.object()["next"].toString();
            QJsonArray activities = document.object()["results"].toArray();
            if (activities.count() > 0) {
                for (int i=0;i<activities.count();i++) {
                    QJsonObject activity = activities.at(i).toObject();
                    QString export_name = activity["export_name"].toString();
                    if (export_name.isEmpty()) {
                        continue;
                    }
                    QString export_name_extension = QFileInfo(export_name).suffix();
                    qDebug() << "export_name_extension " << export_name_extension ;
                    CloudServiceEntry *add = newCloudServiceEntry();

                    add->id = QString("%1").arg(activity["id"].toString());
                    add->name = QDateTime::fromString(activity["start"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+"."+export_name_extension;
                    add->label = add->id;
                    add->distance = activity["distance"].toDouble() / 1000.0f; // m -> km
                    add->isDir = false;
                    // Duration return in the following format - 'P0DT00H25M57S' (https://en.wikipedia.org/wiki/ISO_8601#Durations)
                    const QRegularExpression rx(QLatin1String("[^0-9]+"));
                    const auto&& parts = activity["timer_time"].toString().split(rx, Qt::SkipEmptyParts);
                    int sum = 0;
                    sum+= parts[0].toInt() * 24 * 60 * 60; // D - days
                    sum+= parts[1].toInt() * 60 * 60 ; // H - hours
                    sum+= parts[2].toInt() * 60; // M - minutes
                    sum+= parts[3].toInt(); // S - seconds
                    qDebug() << activity["timer_time"].toString() << " - " << sum;
                    add->duration = sum;

                    printd("item: %s %s %s %f %ld\n",
                           add->id.toStdString().c_str(),
                           add->name.toStdString().c_str(),
                           add->label.toStdString().c_str(),
                           add->distance,
                           add->duration
                    );
                    returning << add;
                }
            }
        }

    } while (!next.isEmpty());

    return returning;
}

bool
Azum::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Azum::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());
    QString url = QString("%1/api/goldencheetah/athletes/%2/activities/%3/export/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString())
          .arg(getSetting(GC_AZUM_ATHLETE_ID, "").toString())
          .arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // put the file
    QNetworkReply *reply = nam()->get(request);

    // remember
    mapReply(reply,remotename);
    buffers.insert(reply,data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    return true;
}


void
Azum::readyRead()
{
    printd("Azum::readRead\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
Azum::readFileCompleted()
{
    printd("Azum::readFileCompleted\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

QList<CloudServiceAthlete>
Azum::listAthletes()
{
    printd("Azum::listAthletes\n");
    QList<CloudServiceAthlete> returning;

    QString token = getSetting(GC_AZUM_ACCESS_TOKEN, "").toString();
    QString next = QString("%1/api/goldencheetah/athletes/")
          .arg(getSetting(GC_AZUM_URL, "https://training.azum.com").toString());

    qDebug() << "token: " << token;
    do {
        // request using csrf token + session id
        QNetworkRequest request(next);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        // DEC-040 Stage 1 (W2) - bounded. Called from the GUI thread by
        // AddCloudWizard, so an unbounded wait here froze the wizard and the
        // whole application with it.
        const RequestResult result = blockingRequest(nam()->get(request), kOpenTimeoutMs);

        // Stop on the first page that did not arrive rather than paging on.
        // The old code reached the parser with an empty body and bailed out of
        // the loop as a "parse error"; being explicit costs nothing and says
        // what actually happened.
        if (!result.ok()) return returning;

        // did we get a good response ?
        QByteArray r = result.body;

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

        if (parseError.error == QJsonParseError::NoError) {
            next = document.object()["next"].toString();
            QJsonArray athletes = document.object()["results"].toArray();
            if (athletes.count()>0) {
                for (int i=0;i<athletes.count();i++) {
                    CloudServiceAthlete add;
                    QJsonObject athlete = athletes[i].toObject();
                    add.id = QString("%1").arg(athlete["user"].toInt());
                    add.name = athlete["full_name"].toString();
                    returning << add;
                }
            }
        } else {
            return returning;
        }
    } while (!next.isEmpty());

    return returning;
}

bool
Azum::selectAthlete(CloudServiceAthlete athlete)
{
    printd("Azum::selectAthlete\n");
    setSetting(GC_AZUM_ATHLETE_ID, athlete.id.toInt());
    return true;
}

static bool addAzum() {
    CloudServiceFactory::instance().addService(new Azum(NULL));
    return true;
}

static bool add = addAzum();
