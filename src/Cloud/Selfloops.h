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

#ifndef GC_Selfloops_h
#define GC_Selfloops_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Selfloops : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Selfloops"; }
        QString uiName() const { return tr("Selfloops"); }
        QString description() const { return tr("Upload and track your training and progress."); }
        QImage logo() const { return QImage(":images/services/selfloops.png"); }

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing Selfloops(context)
        // unchanged and gets the manager CloudService makes lazily on
        // first use. Tests pass one in.
        Selfloops(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Selfloops(context); }
        ~Selfloops();

        // upload only and authenticates with a user and password
        int capabilities() const { return UserPass | Upload; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId);

    public slots:

        // sending data
        void writeFileCompleted();

    private:
        Context *context;
        // DEC-040 Stage 1 (S-1) - `nam` now lives in CloudService, which creates
        // it lazily on first use and owns it. The member that used to be declared
        // here was left indeterminate whenever context was NULL.
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
