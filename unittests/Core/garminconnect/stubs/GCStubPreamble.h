// Force-included before every .cpp in testGarminConnectTile via -include.
// Defines the REAL include guards for the heavyweight GC headers and provides
// lightweight stand-ins so the full GC class hierarchy is never loaded.
//
// REQ-001 only needs the tile-contract surface of CloudService:
//   id(), uiName(), description(), logo(), capabilities(), type(), clone()
// plus CloudServiceFactory::addService / serviceNames / service.
// Signals/slots are intentionally omitted (no QObject inheritance) — REQ-001
// has no asynchronous behaviour. Q_OBJECT and signals get re-introduced when
// REQ-005+ adds the credentials dialog and download path.

#ifndef _GC_GARMIN_STUB_PREAMBLE_H
#define _GC_GARMIN_STUB_PREAMBLE_H

#include <QHash>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

// === Settings.h stub (guard matches src/Core/Settings.h) ===
#ifndef _GC_Settings_h
#    define _GC_Settings_h
// no symbols required by the tile path
#endif

// === Athlete.h stub (guard matches src/Core/Athlete.h) ===
#ifndef _GC_Athlete_h
#    define _GC_Athlete_h
class Athlete
{
  public:
    QString cyclist;
};
#endif

// === Context.h stub (guard matches src/Core/Context.h) ===
#ifndef _GC_Context_h
#    define _GC_Context_h
class Context
{
  public:
    Athlete* athlete = nullptr;
};
#endif

// === CloudService.h stub (guard matches src/Cloud/CloudService.h) ===
#ifndef GC_CloudService_h
#    define GC_CloudService_h

class CloudServiceEntry;
class RideItem;
class RideFile;

class CloudServiceAthlete
{
  public:
    CloudServiceAthlete() : local(nullptr) {}
    QString id;
    QString name;
    QString desc;
    void* local;
};

class CloudService
{
  public:
    enum Capability { OAuth = 0x01, UserPass = 0x02, Upload = 0x04, Download = 0x08, Query = 0x10 };
    enum ServiceType { Activities = 0x01, Measures = 0x02, Calendar = 0x04 };
    enum CloudServiceSetting {
        Username,
        Password,
        OAuthToken,
        Key,
        URL,
        DefaultURL,
        Folder,
        AthleteID,
        Local1,
        Local2,
        Local3,
        Local4,
        Local5,
        Local6,
        Combo1,
        Metadata1,
        Consent
    };

    CloudService(Context* c) : context(c) {}
    virtual ~CloudService() {}

    virtual CloudService* clone(Context*) = 0;
    virtual QString id() const { return QStringLiteral("NONE"); }
    virtual QString uiName() const { return QStringLiteral("None"); }
    virtual QString description() const { return QString(); }
    virtual QImage logo() const = 0;
    virtual int capabilities() const { return OAuth | Upload | Download | Query; }
    virtual int type() const { return Activities; }

    // tr() shim — production CloudService inherits QObject and uses real tr().
    // For the unit-test build the stub just returns the literal as a QString.
    static QString tr(const char* s) { return QString::fromUtf8(s); }

    QHash<CloudServiceSetting, QString> settings;
    QHash<QString, QVariant> configuration;
    Context* context;
};

class CloudServiceFactory
{
    QHash<QString, CloudService*> services_;
    QStringList names_;

  public:
    static CloudServiceFactory& instance()
    {
        static CloudServiceFactory factory;
        return factory;
    }

    int serviceCount() const { return names_.size(); }
    const QStringList serviceNames() const { return names_; }
    const CloudService* service(QString name) const { return services_.value(name, nullptr); }

    bool addService(CloudService* service)
    {
        if (names_.contains(service->id()))
            return false;
        services_.insert(service->id(), service);
        names_.append(service->id());
        return true;
    }
};

#endif // GC_CloudService_h

#endif // _GC_GARMIN_STUB_PREAMBLE_H
