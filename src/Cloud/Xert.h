/*
 * Copyright (c) 2017 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef GC_Xert_h
#define GC_Xert_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Xert : public CloudService {

    Q_OBJECT

    public:

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing Xert(context) unchanged and
        // gets the manager CloudService makes. Tests pass one in.
        Xert(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Xert(context); }
        ~Xert();

        QString id() const { return "Xert"; }
        QString uiName() const { return tr("Xert"); }
        QString description() const { return(tr("Sync with the innovative site for fitness monitoring, tracking, and planning.")); }
        QImage logo() const { return QImage(":images/services/xert.png"); }

        // now upload only and authenticates with a user and password
        int capabilities() const { return UserPass | OAuth | Upload | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid, ReadFileArmed *armed = nullptr);

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId);

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

        // DEC-040 Stage 1 (S-1) - installs this service's sslErrors handling on
        // the lazily created manager, exactly once. See CloudService::wireNam.
        void wireNam(QNetworkAccessManager *nam) override;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);

        QString getRideName(RideFile *ride);
        // DEC-040 Stage 1 (W2) - `error` is the failure channel readdir needs.
        // An empty QJsonObject cannot serve as one: it is also what a successful
        // fetch of an activity with no summary returns, so a caller reading only
        // the return value cannot tell "no detail" from "no answer" and would
        // record distance=0 / duration=0 for an activity it never heard about.
        // Defaulted to NULL so the signature is source-compatible.
        QJsonObject readActivityDetail(QString path, bool withSessionData, QString *error = NULL);
};
#endif
