/*
 * Copyright (c) 2017 Mark Liversedge <liversedge@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "Nolio.h"
#include "MainWindow.h"
#include "JsonRideFile.h"
#include "Athlete.h"
#include "Secrets.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef NOLIO_DEBUG
#define NOLIO_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (NOLIO_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (NOLIO_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Nolio::Nolio(Context *context, QNetworkAccessManager *injectedNam)
    : CloudService(context, injectedNam), context(context), root_(NULL) {
    // DEC-040 Stage 1 (S-1) - the manager is CloudService's now, and it is not
    // built until something actually asks for it (nam()). Nothing is created
    // here, so this constructor is inert when the factory runs it pre-main.
    //
    // The sslErrors connect that used to sit here has moved to wireNam(), which
    // the base calls exactly once, when the manager comes into being. It cannot
    // stay in a constructor: there is no manager to connect to yet.

    downloadCompression = none;
    filetype = uploadType::JSON;

    // config
    settings.insert(OAuthToken, GC_NOLIO_ACCESS_TOKEN);
    settings.insert(URL, GC_NOLIO_URL);
    settings.insert(DefaultURL, "https://www.nolio.io");
    settings.insert(Local1, GC_NOLIO_REFRESH_TOKEN);
    settings.insert(Local2, GC_NOLIO_LAST_REFRESH);
    settings.insert(AthleteID, GC_NOLIO_ATHLETE_ID);
    settings.insert(Local3, GC_NOLIO_ATHLETE_NAME);
}

Nolio::~Nolio() {
    // DEC-040 Stage 1 (S-1) - `if (context) delete nam;` removed. CloudService
    // owns the manager on both the default and the injected path and is its sole
    // deleter, so it is destroyed exactly once, with this service.
}

// DEC-040 Stage 1 (S-1) - called by CloudService::nam() EXACTLY ONCE, the first
// time a manager exists. This is the same connect that used to live in the
// constructor; only its timing changed, because with lazy creation the
// constructor no longer has a manager to connect to.
void
Nolio::wireNam(QNetworkAccessManager *nam)
{
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void Nolio::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors){
    sslErrors(context->mainWindow, reply, errors);
}

bool Nolio::open(QStringList &errors){
    printd("Nolio::open\n");

    QString refresh_token = appsettings->value(NULL, GC_NOLIO_REFRESH_TOKEN, "").toString();
    if (refresh_token == "") {
        return false;
    }

    QString last_refresh_str = appsettings->value(NULL, GC_NOLIO_LAST_REFRESH, "0").toString();
    QDateTime last_refresh = QDateTime::fromString(last_refresh_str);
    last_refresh = last_refresh.addSecs(86400); // nolio tokens are valid for one day
    QDateTime now = QDateTime::currentDateTime();
    // check if we need to refresh the access token
    if (now <= last_refresh) { // credentials are still valid
        printd("tokens still valid\n");
        return true;
    }

    // get new credentials using refresh_token
    QNetworkRequest request(QUrl("https://www.nolio.io/api/token/"));
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    QString authheader = QString("%1:%2").arg(GC_NOLIO_CLIENT_ID).arg(GC_NOLIO_SECRET);
    request.setRawHeader("Authorization", "Basic " + authheader.toLatin1().toBase64());

    QString data = QString("grant_type=refresh_token&refresh_token=").append(refresh_token);

    // DEC-040 Stage 1 (W2) - bounded by the generic auth timeout
    const RequestResult result = blockingRequest(nam()->post(request, data.toLatin1()), kOpenTimeoutMs);

    printd("HTTP response code: %d\n", result.httpStatus);

    // Covers a TIMEOUT as well as a network error: both mean we have no token,
    // and neither may fall through to the parse below with an empty body.
    if (!result.ok()) {
        printd("Got error %d\n", result.error);
        errors << result.errorString;
        return false;
    }
    QByteArray r = result.body;
    printd("Got response: %s\n", r.data());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        printd("Parse error!\n");
        return false;
    }

    QString new_access_token = document.object()["access_token"].toString();
    QString new_refresh_token = document.object()["refresh_token"].toString();
    if (new_access_token != "") appsettings->setValue(GC_NOLIO_ACCESS_TOKEN, new_access_token);
    if (new_refresh_token != "") appsettings->setValue(GC_NOLIO_REFRESH_TOKEN, new_refresh_token);
    appsettings->setValue(GC_NOLIO_LAST_REFRESH, now.toString());
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

QList<CloudServiceEntry*> Nolio::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to){
    printd("Nolio::readdir(%s)\n", path.toStdString().c_str());
    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString access_token = appsettings->value(NULL, GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        errors << "You must authorise with Nolio first";
        return returning;
    }

    QString user_id = getSetting(GC_NOLIO_ATHLETE_ID, "").toString();
    // prepare the request
    QString urlstr = "https://www.nolio.io/api/get/training/?";
    QUrlQuery params;
    params.addQueryItem("from", from.toString("yyyy-MM-dd"));
    params.addQueryItem("to", to.toString("yyyy-MM-dd"));
    if(user_id.length() > 0) params.addQueryItem("athlete_id", user_id);
    QUrl url = QUrl(urlstr + params.toString());
    QNetworkRequest request(url);
    // request using the bearer token
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
    // DEC-040 Stage 1 (W2) - bounded by the generic listing timeout. This listing
    // is a SINGLE request: Nolio's /api/get/training/ endpoint takes a from/to
    // range and answers in one hit, so there is one wait here, not one per page.
    const RequestResult result = blockingRequest(nam()->get(request), kListTimeoutMs);

    // A listing that failed OR timed out. The message keeps the original wording
    // and adds what actually went wrong - "Network Problem reading Nolio data"
    // alone cannot tell a refused connection from a server that went quiet.
    if (!result.ok()) {
        errors << QString("%1: %2").arg(tr("Network Problem reading Nolio data")).arg(result.errorString);
        return returning;
    }
    // did we get a good response ?
    QByteArray r = result.body;

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray results = document.array();

        if (results.size() > 0) {
            for (int i = 0; i < results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                CloudServiceEntry *add = newCloudServiceEntry();

                add->label = QFileInfo(each["name"].toString()).fileName();
                add->id = QString("%1").arg(each["nolio_id"].toVariant().toULongLong());
                add->isDir = false;
                add->distance = each["distance"].toDouble();
                add->duration = each["duration"].toInt();
                if(each["hour_start"].toString().size() > 0){
                    QString full_date = QString("%1T%2Z").arg(each["date_start"].toString(), each["hour_start"].toString());
                    add->name = QDateTime::fromString(full_date, Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";
                }
                else add->name = QDateTime::fromString(each["date_start"].toString(), Qt::ISODate).toString("yyyy_MM_dd_HH_mm_ss")+".json";
                returning << add;
            }
        }
    }
    return returning;
}

bool Nolio::readFile(QByteArray *data, QString remotename, QString remoteid, CloudService::ReadFileArmed *){
    printd("Nolio::readFile\n");

    // do we have a token
    QString access_token = appsettings->value(NULL, GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        return false;
    }

    // prepare the request
    QString urlstr = "https://www.nolio.io/api/get/training/info/?";
    QUrlQuery params;
    params.addQueryItem("id", remoteid);
    QUrl url = QUrl(urlstr + params.toString());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());

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

void Nolio::readyRead(){
    printd("Nolio::readyRead");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void Nolio::readFileCompleted(){
    printd("Nolio::readCompleted");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QByteArray* data = prepareResponse(buffers.value(reply));
    notifyReadComplete(data, replyName(reply), tr("Completed."));
}

QByteArray* Nolio::prepareResponse(QByteArray* data){
    printd("Nolio::prepareResponse()\n");
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(data->constData(), &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject activity = document.object();
        QDateTime start_time = QDateTime::fromString(activity["date_start"].toString(), Qt::ISODate);
        RideFile *ride = new RideFile(start_time.toUTC(), 1.0f);
        // The device type is unknown
        ride->setDeviceType("Nolio");
        // set nolio id in metadata
        if (!activity["nolio_id"].isNull()) ride->setTag("NolioID",  QString("%1").arg(activity["id"].toVariant().toULongLong()));
        // set sport and subsport
        if (!activity["sport_id"].isNull()) {
            double sport_id = activity["sport_id"].toDouble();
            if (sport_id == 14 || sport_id == 15 || sport_id == 18 || sport_id == 28 || sport_id == 35 || sport_id == 36) ride->setTag("Sport", "Bike");
            else if (sport_id == 23) ride->setTag("Sport", "Run");
            else if (sport_id == 19) ride->setTag("Sport", "Swim");
            else if (sport_id == 33 || sport_id == 69) ride->setTag("Sport", "Row");
            else if (sport_id == 3 || sport_id == 4) ride->setTag("Sport", "Ski");
            else ride->setTag("Sport", activity["sport"].toString());
            ride->setTag("SubSport", activity["sport"].toString());
        }

        // do we have a rpe?
        if (!activity["rpe"].isNull()) {
            QString rpe = QString("%1").arg(activity["rpe"].toDouble());
            ride->setTag("RPE", rpe);
        }

        if (!activity["name"].isNull()) ride->setTag("Title", activity["name"].toString());

        // description saved to Notes
        if (!activity["description"].isNull()) ride->setTag("Notes", activity["description"].toString());

        QString athlete_name = getSetting(GC_NOLIO_ATHLETE_NAME, "").toString();
        if (athlete_name.length() > 0) ride->setTag("Athlete", athlete_name);

        // Streams
        QJsonArray streams = activity["streams"].toArray();
        if (streams.size() > 0) {
            for (int i = 0; i < streams.size(); i++) {
                QJsonObject sample = streams.at(i).toObject();
                RideFilePoint add;

                add.setValue(RideFile::secs, sample["time"].toDouble());
                add.setValue(RideFile::km, sample["distance"].toDouble() * 0.001f);
                add.setValue(RideFile::kph, sample["pace"].toDouble() * 3.6f);
                add.setValue(RideFile::alt, sample["altitude"].toDouble());
                add.setValue(RideFile::watts, sample["watts"].toDouble());
                add.setValue(RideFile::cad, sample["cadence"].toDouble());
                add.setValue(RideFile::hr, sample["heartrate"].toDouble());
                if (sample.contains("torque")) add.setValue(RideFile::nm, sample["torque"].toDouble());
                if (sample.contains("gps")){
                    QJsonArray gps_points = sample["gps"].toArray();
                    add.setValue(RideFile::lat, gps_points[0].toDouble());
                    add.setValue(RideFile::lon, gps_points[1].toDouble());
                }
                if (sample.contains("lrbalance")) add.setValue(RideFile::lrbalance, sample["lrbalance"].toDouble());
                if (sample.contains("left_power_phase_start")) add.setValue(RideFile::lppb, sample["left_power_phase_start"].toDouble());
                if (sample.contains("left_power_phase_end")) add.setValue(RideFile::lppe, sample["left_power_phase_end"].toDouble());
                if (sample.contains("left_power_phase_peak_start")) add.setValue(RideFile::lpppb, sample["left_power_phase_peak_start"].toDouble());
                if (sample.contains("left_power_phase_peak_end")) add.setValue(RideFile::lpppe, sample["left_power_phase_peak_end"].toDouble());
                if (sample.contains("right_power_phase_start")) add.setValue(RideFile::rppb, sample["right_power_phase_start"].toDouble());
                if (sample.contains("right_power_phase_end")) add.setValue(RideFile::rppe, sample["right_power_phase_end"].toDouble());
                if (sample.contains("right_power_phase_peak_start")) add.setValue(RideFile::rpppb, sample["right_power_phase_peak_start"].toDouble());
                if (sample.contains("right_power_phase_peak_end")) add.setValue(RideFile::rpppe, sample["right_power_phase_peak_end"].toDouble());
                ride->appendPoint(add);
            }
        }
        // if stream is empty --> manual workout
        else{
            if (activity["distance"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(activity["distance"].toDouble()));
                ride->metricOverrides.insert("total_distance", map);
            }
            if (activity["duration"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(activity["duration"].toDouble()));
                ride->metricOverrides.insert("time_riding", map);
            }
            if (activity["elevation_gain"].toDouble()>0) {
                QMap<QString,QString> map;
                map.insert("value", QString("%1").arg(activity["elevation_gain"].toDouble()));
                ride->metricOverrides.insert("elevation_gain", map);
            }
        }

        // laps
        QJsonArray laps = activity["laps"].toArray();
        if (laps.size() > 0){
            double start = 0.0;
            double end = 0.0;
            for (const QJsonValue &value : laps) {
                QJsonObject lap = value.toObject();
                double duration = lap["duration"].toDouble();
                if (start > 0.0){
                    start = end;
                }
                end = start + duration;
                ride->addInterval(RideFileInterval::USER, start, end, lap["name"].toString());
            }

        }

        JsonFileReader reader;
        data->clear();
        data->append(reader.toByteArray(context, ride, true, true, true, true));

        delete ride; // delete useless temp ride
    }
    return data;
}

QList<CloudServiceAthlete> Nolio::listAthletes(){
    printd("Nolio::listAthletes\n");
    QList<CloudServiceAthlete> returning;

    QString access_token = appsettings->value(NULL, GC_NOLIO_ACCESS_TOKEN, "").toString();
    if (access_token == "") {
        return returning;
    }
    QString urlstr = "https://www.nolio.io/api/get/athletes/?";
    QUrlQuery params;
    params.addQueryItem("wants_coach", "true");
    QUrl url = QUrl(urlstr + params.toString());
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(access_token)).toLatin1());
    // DEC-040 Stage 1 (W2) - bounded. Called from the GUI thread by
    // AddCloudWizard, so the unbounded wait that used to be here froze the wizard
    // and the whole application with it.
    //
    // A SINGLE request, not a paginated loop: /api/get/athletes/ answers in one
    // hit and there is no `next`, offset or page parameter anywhere in this
    // function. Azum::listAthletes, which shares the name, IS paginated - the two
    // are not structurally parallel and were read, not assumed.
    const RequestResult result = blockingRequest(nam()->get(request), kOpenTimeoutMs);

    // listAthletes has no errors channel - it returns the list and nothing else -
    // so a failure can only be reported as "no athletes". That was already true
    // of a network error; what changes is that it is now also true of a timeout,
    // instead of the wizard hanging. Returning early keeps the empty list from
    // being produced by a parse of an empty body, which is the same outcome
    // reached for a different and less honest reason.
    if (!result.ok()) return returning;

    // did we get a good response ?
    QByteArray r = result.body;

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    if (parseError.error == QJsonParseError::NoError) {
        QJsonArray results = document.array();
        if (results.size() > 0) {
            for (int i = 0; i < results.size(); i++) {
                QJsonObject each = results.at(i).toObject();
                CloudServiceAthlete add;

                add.id = QString("%1").arg(each["nolio_id"].toInt());
                add.name = each["name"].toString();
                returning << add;
            }
        }
    }
    return returning;
}

bool Nolio::selectAthlete(CloudServiceAthlete athlete){
    printd("Nolio::selectAthlete\n");
    // extract athlete name and identifier from the selected athlete
    // TODO
    setSetting(GC_NOLIO_ATHLETE_ID, athlete.id.toInt());
    setSetting(GC_NOLIO_ATHLETE_NAME, athlete.name);
    return true;
}

bool Nolio::createFolder(QString){
    printd("Nolio::createFolder\n");
    return false;
}

bool Nolio::close(){
    return true;
}

QString Nolio::home() {
    return "";
}

static bool addNolio() {
    CloudServiceFactory::instance().addService(new Nolio(NULL));
    return true;
}

static bool add = addNolio();
