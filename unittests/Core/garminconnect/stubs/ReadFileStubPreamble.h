// Force-included before every .cpp in the REQ-007-closure GarminConnect tests
// (TEST-019 open/close, TEST-020..023 readFile) via -include. Defines the REAL
// include guards for the heavyweight GC headers and provides lightweight
// stand-ins so the full GC class hierarchy is never loaded — and, crucially,
// replaces the embedded-Python PyEmbeddedAdapter with a Python-free fake so the
// target stays on the `garmin-fast` label (no Python.h, no interpreter).
//
// Superset of stubs/GCStubPreamble.h: the tile contract PLUS the CloudService
// open/close/readFile virtuals and a notifyReadComplete recorder (production
// CloudService emits readComplete — the framework then stages+parses via
// uncompressRide keyed on the passed filename; here the stub records the staged
// name+bytes so a test can assert the DEC-016 garmin-<id>.fit/.tcx staging).

#ifndef _GC_GARMIN_READFILE_STUB_PREAMBLE_H
#define _GC_GARMIN_READFILE_STUB_PREAMBLE_H

#include <QByteArray>
#include <QDir>
#include <QHash>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

// === Settings.h stub (guard matches src/Core/Settings.h) ===
#ifndef _GC_Settings_h
#    define _GC_Settings_h
#endif

// === Athlete.h stub (guard matches src/Core/Athlete.h) ===
#ifndef _GC_Athlete_h
#    define _GC_Athlete_h
// Minimal AthleteDirectoryStructure shape — GarminConnect's production
// resolveConfigDir() reads context->athlete->home->config(); the tests always
// pass a configDir override so this branch is never executed, but it must
// compile against the stub.
class AthleteDirectoryStructureStub
{
  public:
    explicit AthleteDirectoryStructureStub(const QString& root) : m_root(root) {}
    QDir config() { return QDir(m_root); }

  private:
    QString m_root;
};
class Athlete
{
  public:
    QString cyclist;
    AthleteDirectoryStructureStub* home = nullptr;
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

    // REQ-007 closure surface under test.
    virtual bool open(QStringList& errors)
    {
        Q_UNUSED(errors);
        return false;
    }
    virtual bool close() { return false; }
    virtual bool readFile(QByteArray* data, QString remotename, QString remoteid)
    {
        Q_UNUSED(data);
        Q_UNUSED(remotename);
        Q_UNUSED(remoteid);
        return false;
    }

    // Production CloudService::notifyReadComplete emits readComplete(); the
    // framework's handler then stages+parses via uncompressRide keyed on `name`.
    // The stub records what GarminConnect staged so tests can assert the DEC-016
    // garmin-<id>.fit / .tcx filename and the final bytes.
    void notifyReadComplete(QByteArray* data, QString name, QString message)
    {
        ++readCompleteCount;
        lastReadName = name;
        lastReadMessage = message;
        if (data)
            lastReadData = *data;
    }
    int readCompleteCount = 0;
    QString lastReadName;
    QString lastReadMessage;
    QByteArray lastReadData;

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

// === PyEmbeddedAdapter.h Python-free fake (guard: GC_PyEmbeddedAdapter_h) ===
// GarminConnect.cpp's lazy production path does `new PyEmbeddedAdapter(modulePath)`;
// this fake makes that compile+link with no interpreter. The readFile/open tests
// inject a fake IGarminDownloadClient, so this fake adapter is never actually
// driven — it only satisfies the symbol (identical intent to WizardStubPreamble).
#ifndef GC_PyEmbeddedAdapter_h
#    define GC_PyEmbeddedAdapter_h
#    include "IGarminPyAdapter.h"
class PyEmbeddedAdapter : public IGarminPyAdapter
{
  public:
    explicit PyEmbeddedAdapter(const QString& modulePath) : m_modulePath(modulePath) {}
    ~PyEmbeddedAdapter() override = default;
    PyAuthOutcome authenticate(const QString&, const QString&) override { return {}; }
    PyAuthOutcome submitMfa(const QString&) override { return {}; }
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }
    QString modulePath() const { return m_modulePath; }

  private:
    QString m_modulePath;
};
#endif // GC_PyEmbeddedAdapter_h

#endif // _GC_GARMIN_READFILE_STUB_PREAMBLE_H
