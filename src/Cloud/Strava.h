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

#ifndef GC_Strava_h
#define GC_Strava_h

#include "CloudService.h"

class QNetworkReply;
class QNetworkAccessManager;

class Strava : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Strava"; }
        QString uiName() const { return tr("Strava"); }
        QString description() const { return (tr("Sync with the social network for cyclists and runners.")); }
        QImage logo() const;

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing Strava(context) unchanged and
        // gets the manager CloudService makes. Tests pass one in.
        Strava(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Strava(context); }
        ~Strava();

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        //virtual int capabilities() const { return OAuth | Upload | Download | Query ; } // Default

        QString authiconpath() const { return QString(":images/services/strava_connect.png"); }

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void writeFileCompleted();

    private:
        Context *context;
        // DEC-040 Stage 1 (S-1) - `nam` now lives in CloudService, which builds
        // it on every construction path and owns it. The member that used to be
        // declared here was left indeterminate whenever context was NULL.
        QNetworkReply *reply;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

        // DEC-040 Stage 1 (W2) - `failure` is the reason channel readFileCompleted
        // needs in order to choose between readComplete and readFailed. Empty on
        // success. When it is non-empty NOTHING has been staged into `data`: a
        // ride whose samples request failed is not a shorter ride, it is a ride
        // we do not have, and staging it would import a header with no data and
        // report it as a completed download.
        QByteArray* prepareResponse(QByteArray* data, QString *failure = NULL);

        // DEC-040 Stage 1 (W2) - returns false when the (now bounded) streams
        // request did not produce a body. It used to return void and `return;`
        // on error, which is indistinguishable from "this activity genuinely has
        // no samples" and left the caller staging an empty ride.
        bool addSamples(RideFile* ret, QString remoteid);
        void fixLapSwim(RideFile* ret, QJsonArray laps);
        void fixSmartRecording(RideFile* ret);

        // DEC-040 Stage 1 (S-1) - installs this service's sslErrors handling on
        // the lazily created manager, exactly once. See CloudService::wireNam.
        void wireNam(QNetworkAccessManager *nam) override;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};
#endif
