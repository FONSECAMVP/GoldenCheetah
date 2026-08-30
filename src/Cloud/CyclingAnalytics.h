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

#ifndef GC_CyclingAnalytics_h
#define GC_CyclingAnalytics_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class CyclingAnalytics : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Cycling Analytics"; }
        QString uiName() const { return tr("Cycling Analytics"); }
        QString description() const { return (tr("Sync with the power focused cycling site.")); }
        QImage logo() const { return QImage(":images/services/cyclinganalytics.png"); }

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing CyclingAnalytics(context) unchanged and
        // gets the manager CloudService makes. Tests pass one in.
        CyclingAnalytics(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new CyclingAnalytics(context); }
        ~CyclingAnalytics();

        // upload only and authenticates with OAuth tokens
        int capabilities() const { return OAuth | Upload | Download | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // get list of rides
        QList<CloudServiceEntry *> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);


    public slots:

        // fetching data
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
};
#endif
