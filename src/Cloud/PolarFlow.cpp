/*
 * Copyright (c) 2016 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "PolarFlow.h"
#include "Athlete.h"
#include "Settings.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef POLARFLOW_DEBUG
// TODO(gille): This should be a command line flag.
#define POLARFLOW_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (POLARFLOW_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (POLARFLOW_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

PolarFlow::PolarFlow(Context *context, QNetworkAccessManager *injectedNam)
    : CloudService(context, injectedNam), context(context), root_(NULL) {

    // DEC-040 Stage 1 (S-1) - the manager is CloudService's now, and it is not
    // built until something actually asks for it (nam()). Nothing is created
    // here, so this constructor is inert when the factory runs it pre-main.
    //
    // The sslErrors connect that used to sit here has moved to wireNam(), which
    // the base calls exactly once, when the manager comes into being. It cannot
    // stay in a constructor: there is no manager to connect to yet.

    uploadCompression = gzip; // gzip
    downloadCompression = none;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_POLARFLOW_TOKEN);
    settings.insert(Local1, GC_POLARFLOW_USER_ID);
}

PolarFlow::~PolarFlow() {
    // DEC-040 Stage 1 (S-1) - `if (context) delete nam;` removed. CloudService
    // owns the manager on both the default and the injected path and is its sole
    // deleter, so it is destroyed exactly once, with this service.
}

// DEC-040 Stage 1 (S-1) - called by CloudService::nam() EXACTLY ONCE, the first
// time a manager exists. This is the same connect that used to live in the
// constructor; only its timing changed, because with lazy creation the
// constructor no longer has a manager to connect to.
void
PolarFlow::wireNam(QNetworkAccessManager *nam)
{
    connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
}

void
PolarFlow::onSslErrors(QNetworkReply *reply, const QList<QSslError>&errors)
{
    sslErrors(context->mainWindow, reply, errors);
}

// open by connecting and getting a basic list of folders available
bool
PolarFlow::open(QStringList &errors)
{
    printd("PolarFlow::open\n");

    // do we have a token
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Polar Flow first");
        return false;
    }

    // use the configed URL
    QString url = QString("%1/rest/users/delegates/users")
          .arg(getSetting(GC_POLARFLOW_URL, "https://whats.todaysplan.com.au").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    // DEC-040 Stage 1 (W2) - bounded by the generic auth timeout
    const RequestResult result = blockingRequest(nam()->get(request), kOpenTimeoutMs);

    // Covers a TIMEOUT as well as a network error. The message keeps the original
    // wording and adds what actually went wrong - "Network Problem reading Polar
    // Flow data" alone cannot tell a refused connection from a server that
    // accepted the connection and then went quiet.
    if (!result.ok()) {
        qDebug() << "error" << result.errorString;
        errors << QString("%1: %2").arg(tr("Network Problem reading Polar Flow data")).arg(result.errorString);
        return false;
    }
    // did we get a good response ?
    QByteArray r = result.body;
    printd("response: %s\n", r.toStdString().c_str());

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(r, &parseError);

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        printd("NoError");

        // we have a root
        root_ = newCloudServiceEntry();

        // path name
        root_->name = "/";
        root_->isDir = true;
        root_->size = 0;
    } else {
        errors << tr("problem parsing Polar Flow data");
    }

    // ok so far ?
    if (errors.count()) return false;
    return true;
}

bool
PolarFlow::close()
{
    printd("PolarFlow::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
PolarFlow::readdir(QString path, QStringList &errors, QDateTime, QDateTime)
{
    printd("PolarFlow::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Polar Flow first");
        return returning;
    }

    // all good ?
    return returning;
}

// read a file at location (relative to home) into passed array
bool
PolarFlow::readFile(QByteArray *data, QString remotename, QString remoteid, CloudService::ReadFileArmed *)
{
    printd("PolarFlow::readFile(%s)\n", remotename.toStdString().c_str());

    // this must be performed asyncronously and call made
    // to notifyReadComplete(QByteArray &data, QString remotename, QString message) when done

    // do we have a token ?
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") return false;

    // lets connect and get basic info on the root directory
    QString url = QString("%1/rest/files/download/%2")
          .arg(getSetting(GC_POLARFLOW_URL, "https://whats.todaysplan.com.au").toString())
          .arg(remoteid);

    printd("url:%s\n", url.toStdString().c_str());

    // request using the bearer token
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
PolarFlow::readyRead()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
}

void
PolarFlow::readFileCompleted()
{
    printd("PolarFlow::readFileCompleted\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    notifyReadComplete(buffers.value(reply), replyName(reply), tr("Completed."));
}

// development put on hold - AccessLink API compatibility issues with Desktop applications
#if 0
static bool addPolarFlow() {
    CloudServiceFactory::instance().addService(new PolarFlow(NULL));
    return true;
}

static bool add = addPolarFlow();
#endif
