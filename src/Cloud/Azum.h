#ifndef AZUM_H
#define AZUM_H

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

class Azum : public CloudService {

    Q_OBJECT

    public:

        // DEC-040 Stage 1 (S-1) - injectedNam defaults to NULL, so every
        // existing production caller keeps writing Azum(context) unchanged and
        // gets the manager CloudService makes. Tests pass one in.
        Azum(Context *context, QNetworkAccessManager *injectedNam = NULL);
        CloudService *clone(Context *context) { return new Azum(context); }
        ~Azum();

        QString id() const { return "Azum"; }
        QString uiName() const { return tr("Azum"); }
        QString description() const { return (tr("Sync with new and unique coaching platform from Switzerland.")); }
        QImage logo() const { return QImage(":images/services/azum.png"); }

        int capabilities() const { return OAuth | Download | Query; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // home directory
        QString home();

        // create a folder
        bool createFolder(QString);

        // read a file
        bool readFile(QByteArray *data, QString remotename, QString remoteid);

        // athlete selection
        QList<CloudServiceAthlete> listAthletes();
        bool selectAthlete(CloudServiceAthlete);

        // dirent style api
        CloudServiceEntry *root() { return root_; }
        QList<CloudServiceEntry*> readdir(QString path, QStringList &errors, QDateTime from, QDateTime to);

    public slots:
        void readyRead();
        void readFileCompleted();

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
