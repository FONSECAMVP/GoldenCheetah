/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "Selfloops.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef SELFLOOPS_DEBUG
#define SELFLOOPS_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (SELFLOOPS_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (SELFLOOPS_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Selfloops::Selfloops(Context *context, QNetworkAccessManager *injectedNam)
    : CloudService(context, injectedNam), context(context), root_(NULL) {

    // DEC-040 Stage 1 (S-1) - the manager is CloudService's now, and it is not
    // built until something actually asks for it (nam()). Nothing is created
    // here, so this constructor is inert when the factory runs it pre-main.
    //
    // The sslErrors connect that used to sit here has moved to wireNam(), which
    // the base calls exactly once, when the manager comes into being. It cannot
    // stay in a constructor: there is no manager to connect to yet.

    uploadCompression = gzip; // gzip
    filetype = CloudService::uploadType::TCX;
    useMetric = true; // distance and duration metadata

    //config
    settings.insert(Username, GC_SELUSER);
    settings.insert(Password, GC_SELPASS);
}

Selfloops::~Selfloops() {
    // DEC-040 Stage 1 (S-1) - `if (context) delete nam;` removed. CloudService
    // owns the manager on both the default and the injected path and is its sole
    // deleter, so it is destroyed exactly once, with this service.
}

// DEC-040 Stage 1 (S-1) - called by CloudService::nam() EXACTLY ONCE, the first
// time a manager exists. This is the same connect that used to live in the
// constructor; only its timing changed, because with lazy creation the
// constructor no longer has a manager to connect to.
void
Selfloops::wireNam(QNetworkAccessManager *nam)
{
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void
Selfloops::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

bool
Selfloops::open(QStringList &errors)
{
    printd("Selfloops::open\n");
    QString username = getSetting(GC_SELUSER).toString();
    if (username == "") {
        errors << tr("Account is not configured,");
        return false;
    }
    return true;
}

bool
Selfloops::close()
{
    printd("Selfloops::close\n");
    // nothing to do for now
    return true;
}

bool
Selfloops::writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId)
{
    Q_UNUSED(ride);

    printd("Selfloops::writeFile(%s)\n", remotename.toStdString().c_str());

    QUrl url = QUrl( "https://www.selfloops.com/restapi/public/activities/upload.json" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString()+QVariant(QRandomGenerator::global()->generate()).toString();

    QString username = getSetting(GC_SELUSER).toString();
    QString password = getSetting(GC_SELPASS).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::MixedType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart emailPart;
    emailPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"email\""));
    emailPart.setBody(username.toLatin1());

    QHttpPart passwordPart;
    passwordPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"pw\""));
    passwordPart.setBody(password.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"tcxfile\"; filename=\"myfile.tcx.gz\"; type=\"application/x-gzip\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-gzip");
    filePart.setBody(data);

    multiPart->append(emailPart);
    multiPart->append(passwordPart);
    multiPart->append(filePart);

    // this must be performed asyncronously and call made
    // to notifyWriteCompleted(QString remotename, QString message) when done
    reply = nam()->post(request, multiPart);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(writeFileCompleted()));

    // remember
    mapReply(reply,remotename,operationId);
    return true;
}

void
Selfloops::writeFileCompleted()
{
    printd("Selfloops::writeFileCompleted()\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", reply->readAll().toStdString().c_str());

    bool uploadSuccessful = false;
    int error;
    QString uploadError;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        error = jsonResponse.root->getFieldInt("error_code");
        uploadError = jsonResponse.root->getFieldString("message").c_str();
        //XXX selfloopsActivityId = jsonResponse.root->getFieldInt("activity_id");

    } catch(...) {

        // problem!
        error = 500;
        uploadError = "bad response or parser exception.";
        //XXX selfloopsActivityId = 0;
    }

    // set tag for upload id
    if (error>0 || reply->error() != QNetworkReply::NoError) uploadSuccessful=false;
    else {

        //qDebug() << "activity: " << selfloopsActivityId;

        //XXX ride->ride()->setTag("Selfloops activityId", QString("%1").arg(selfloopsActivityId));
        //XXX ride->setDirty(true);
        uploadSuccessful = true;
    }

    if (uploadSuccessful && reply->error() == QNetworkReply::NoError) {
        notifyWriteComplete(replyWriteOperationId(reply), replyName(reply), tr("Completed."));
    } else {
        notifyWriteComplete(replyWriteOperationId(reply), replyName(reply), tr("Network Error - Upload failed."));
    }
}

static bool addSelfloops() {
    CloudServiceFactory::instance().addService(new Selfloops(NULL));
    return true;
}

static bool add = addSelfloops();
