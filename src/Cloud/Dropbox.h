/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_Dropbox_h
#define GC_Dropbox_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QImage>

class Dropbox : public CloudService {

    Q_OBJECT

    public:

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every existing
        // production caller keeps writing Dropbox(context) and gets a manager the
        // base makes. Tests pass one in.
        Dropbox(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Dropbox(context); }
        ~Dropbox();

        QString id() const { return "Dropbox"; }
        QString uiName() const { return tr("Dropbox"); }
        QString description() const { return (tr("Sync activities via your cloud storage.")); }
        QImage logo() const { return QImage(":images/services/dropbox.png"); }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // write a file 
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride, quint64 operationId);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString);

        // create a folder
        bool createFolder(QString path);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors);

    public slots:

        // getting data
        void readyRead(); // a readFile operation has work to do
        void readFileCompleted();

        // sending data
        void writeFileCompleted();

    private:
        Context *context;
        // DEC-040 Stage 1 (S-1) - `nam` now lives in CloudService, which creates
        // it on every construction path and owns it. The member that used to be
        // here was left indeterminate whenever context was NULL.
        QNetworkReply *reply;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;
};
#endif
