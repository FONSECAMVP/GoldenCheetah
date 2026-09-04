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

#ifndef NOLIO_H
#define NOLIO_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Nolio : public CloudService {

    Q_OBJECT

    public:

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing Nolio(context) unchanged and
        // gets the manager CloudService makes. Tests pass one in.
        Nolio(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Nolio(context); }
        ~Nolio();

        QString id() const { return "Nolio"; }
        QString uiName() const { return tr("Nolio"); }
        QString description() const { return (tr("Sync with your favorite training partner.")); }
        QImage logo() const { return QImage(":images/services/nolio.png"); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        virtual int capabilities() const { return OAuth | Download | Query; }
        // home directory
        QString home();

        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid, ReadFileArmed *armed = nullptr);
        QByteArray* prepareResponse(QByteArray* data);

        // create a folder
        bool createFolder(QString);

        // athlete selection
        QList<CloudServiceAthlete> listAthletes();
        bool selectAthlete(CloudServiceAthlete);

        // dirent style api
        CloudServiceEntry *root() { return root_; }

    public slots:
        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

    private:
        Context *context;
        // DEC-040 Stage 1 (S-1) - `nam` now lives in CloudService, which builds
        // it on every construction path and owns it. The member that used to be
        // declared here was left indeterminate whenever context was NULL.
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;


        // DEC-040 Stage 1 (S-1) - installs this service's sslErrors handling on
        // the lazily created manager, exactly once. See CloudService::wireNam.
        void wireNam(QNetworkAccessManager *nam) override;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

#endif // NOLIO_H
