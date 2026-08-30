/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
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

#include "TrainingsTageBuch.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef TRAININGSTAGEBUCH_DEBUG
#define TRAININGSTAGEBUCH_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (TRAININGSTAGEBUCH_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (TRAININGSTAGEBUCH_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

const QString TTB_URL( "http://trainingstagebuch.org" );

TrainingsTageBuch::TrainingsTageBuch(Context *context, QNetworkAccessManager *injectedNam)
    : CloudService(context, injectedNam), context(context), root_(NULL) {

    // DEC-040 Stage 1 (S-1) - the manager is CloudService's now, and it is not
    // built until something actually asks for it (nam()). Nothing is created
    // here, so this constructor is inert when the factory runs it pre-main.
    //
    // The sslErrors connect that used to sit here has moved to wireNam(), which
    // the base calls exactly once, when the manager comes into being. It cannot
    // stay in a constructor: there is no manager to connect to yet.

    uploadCompression = none; // gzip
    filetype = CloudService::uploadType::PWX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_TTBUSER);
    settings.insert(Password, GC_TTBPASS);
}

TrainingsTageBuch::~TrainingsTageBuch() {
    // DEC-040 Stage 1 (S-1) - `if (context) delete nam;` removed. CloudService
    // owns the manager on both the default and the injected path and is its sole
    // deleter, so it is destroyed exactly once, with this service.
}

// DEC-040 Stage 1 (S-1) - called by CloudService::nam() EXACTLY ONCE, the first
// time a manager exists. This is the same connect that used to live in the
// constructor; only its timing changed, because with lazy creation the
// constructor no longer has a manager to connect to.
void
TrainingsTageBuch::wireNam(QNetworkAccessManager *nam)
{
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void
TrainingsTageBuch::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

bool
TrainingsTageBuch::open(QStringList &errors)
{
    // get a session token, then get the settings for the account
    printd("TrainingStageBuch::open\n");

    // GET ACCOUNT SETTINGS
    QString username = getSetting(GC_TTBUSER).toString();
    QString password = getSetting(GC_TTBPASS).toString();

    QUrlQuery urlquery;
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

    QUrl url (TTB_URL + "/settings/list");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    // DEC-040 Stage 1 (W2) - THE FIRST OF TWO INDEPENDENT BOUNDED REQUESTS.
    //
    // These two waits used to SHARE one QEventLoop declared here and re-exec'd
    // 40 lines further down, with a second connect stacked onto the same loop
    // object. Two requests sharing one loop is not one wait, it is two, and the
    // shared loop made that hard to see: the settings reply's finished() stayed
    // connected to the same loop across the second exec(), so a late finish from
    // request one could quit the wait for request two. They are decomposed here
    // into two separate bounded requests and are deliberately NOT collapsed into
    // one: they are different endpoints answering different questions, and the
    // first one can make the second unnecessary (see the early return below).
    const RequestResult settingsResult = blockingRequest(nam()->get(request), kOpenTimeoutMs);

    if (!settingsResult.ok()) {
        errors << (tr("failed to get settings: ") + settingsResult.errorString);
        return false;
    }

    TTBSettingsParser handler;
    // The body was snapshotted by blockingRequest and the reply is gone, so the
    // parser is fed the bytes rather than the QIODevice it used to read from.
    QXmlInputSource source;
    source.setData(settingsResult.body);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    if(! reader.parse(source) ){
        errors << (tr("failed to parse Settings response: ")+handler.errorString());
        return false;
    }

    if( handler.error.length() > 0 ){
        errors << (tr("failed to get settings: ") +handler.error);
        return false;
    }

    sessionId = handler.session;
    proMember = handler.pro;

    // if we got a session id, no need to go further.
    if(sessionId.length() > 0) return true;

    // GET SESSION TOKEN

    urlquery = QUrlQuery();
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

    url = QUrl(TTB_URL + "/login/sso");
    url.setQuery(urlquery.query());
    request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    // DEC-040 Stage 1 (W2) - THE SECOND INDEPENDENT BOUNDED REQUEST. Its own
    // wait, its own bound; see the note on the first one above.
    const RequestResult sessionResult = blockingRequest(nam()->get(request), kOpenTimeoutMs);

    if (!sessionResult.ok()) {
        errors << (tr("failed to get new session: ") + sessionResult.errorString);
        return false;
    }

    TTBSessionParser shandler;
    QXmlInputSource ssource;
    ssource.setData(sessionResult.body);

    reader.setContentHandler(&shandler);

    if(! reader.parse(ssource)) {
        errors << (tr("failed to parse Session response: ")+shandler.errorString());
        return false;
    }

    if(handler.error.length() > 0){
        errors << (tr("failed to get new session: ") +shandler.error );
        return false;
    }

    sessionId = shandler.session;

    if(sessionId.length() == 0){
        errors << (tr("got empty session"));
        return false;
    }

    // SUCCESS
    return true;
}

bool
TrainingsTageBuch::close()
{
    printd("TrainingStageBuch::close\n");
    // nothing to do for now
    return true;
}

bool
TrainingsTageBuch::writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId)
{
    Q_UNUSED(ride);

    printd("TrainingStageBuch::writeFile(%s)\n", remotename.toStdString().c_str());

    QHttpMultiPart *body = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"upload_submit\""));
    textPart.setBody("hrm");
    body->append(textPart);

    int limit = proMember ? 8 * 1024 * 1024 : 4 * 1024 * 1024;
    if(data.size() >= limit ){
        return false;
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
    QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"file\"; filename=\"gc-upload-ttb.pwx\""));
    filePart.setBody(data);
    body->append(filePart);

    QUrlQuery urlquery;
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "sso", sessionId );


    QUrl url (TTB_URL + "/file/upload");
    url.setQuery(urlquery.query());
    QNetworkRequest request = QNetworkRequest(url);

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done
    reply = nam()->post(request, body);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename,operationId);
    return true;
}

void
TrainingsTageBuch::writeFileCompleted()
{
    printd("TrainingStageBuch::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    TTBUploadParser handler;
    QXmlInputSource source(reply);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);

    bool success = true;
    if(! reader.parse(source)) {
        success = false;
    }

    if(success && handler.error.length() > 0){
        success = false;
    }

    if(success && handler.id.length() == 0 ){
        success = false;
    }

    if (success && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(replyWriteOperationId(reply), replyName(reply), tr("Completed."));
    } else {
        notifyWriteComplete(replyWriteOperationId(reply), replyName(reply), tr("Error - Upload failed."));
    }
}

static bool addTrainingStageBuch() {
    CloudServiceFactory::instance().addService(new TrainingsTageBuch(NULL));
    return true;
}

static bool add = addTrainingStageBuch();
