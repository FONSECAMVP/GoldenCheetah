/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the License as published by the Free Software Foundation.
 */

// REQ-020 / T-161..T-163 — force-included (-include) before every .cpp of the
// testGarminConnectWizardLifetime target, INCLUDING the AUTOMOC-generated moc
// files. Sibling of stubs/WizardStubPreamble.h (same guard-predefinition trick,
// same rationale — see there); it DIFFERS where the lifetime harness needs
// seams the routing harness does not:
//
//   - Context is a QObject (QPointer<Context> in the production code must
//     track the stub for real, exactly as it tracks the real Context.h:106
//     QObject in the application build).
//   - OAuthDialog is SCRIPTABLE: sslLibMissing() defaults to false so
//     AddAuth::doAuth() reaches exec(), and exec() parks in a real nested
//     QEventLoop the test can deliver a teardown inside (g_oauthLoop).
//   - CloudServiceDialog is scriptable the same way (g_folderLoop), or returns
//     a scripted result immediately when no loop is requested.
//   - the never-executed collaborators count their calls (folderSelected,
//     saveSettings, CloudServiceSyncDialog construction, setCValue) so a bail
//     is observable as "the call did not happen", not just "no crash".
//
// GC_WANT_GARMINCONNECT is deliberately NOT defined for this target: the three
// frames under test (AddAuth::doAuth, AddSettings::browseFolder,
// AddFinish::validatePage) are outside every #ifdef in AddCloudWizard.cpp, so
// the Garmin compile of this TU stays covered by testGarminConnectWizardRouting
// and this target links no Python-free Garmin worker stack at all.

#ifndef _GC_WIZARD_LIFETIME_STUB_PREAMBLE_H
#define _GC_WIZARD_LIFETIME_STUB_PREAMBLE_H

// clang-format off
// --- Qt surface the wizard normally gets transitively via the heavy headers ---
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDir>
#include <QGuiApplication>
#include <QEventLoop>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMessageBox>
#include <QPixmap>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalMapper>
#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QThread>
#include <QVariant>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>
// clang-format on

// ===========================================================================
// GoldenCheetah.h (guard: _GC_GoldenCheetah_h) — nothing consumed here
// ===========================================================================
#ifndef _GC_GoldenCheetah_h
#    define _GC_GoldenCheetah_h
#endif

// ===========================================================================
// Serial.h (guard: _GC_PT_Serial_h) — nothing consumed here
// ===========================================================================
#ifndef _GC_PT_Serial_h
#    define _GC_PT_Serial_h 1
#endif

// ===========================================================================
// MainWindow.h (guard: _GC_MainWindow_h) — Context::mainWindow is a QWidget*
// ===========================================================================
#ifndef _GC_MainWindow_h
#    define _GC_MainWindow_h
#endif

// ===========================================================================
// RideItem.h (guard: _GC_RideItem_h) — moc pulls this in defensively; no
// wizard signal/slot uses a RideItem, so an empty stand-in suffices.
// ===========================================================================
#ifndef _GC_RideItem_h
#    define _GC_RideItem_h 1
class RideItem;
#endif

// ===========================================================================
// Perspective.h (guard: _GC_HomeWindow_h) — moc pulls this in defensively.
// ===========================================================================
#ifndef _GC_HomeWindow_h
#    define _GC_HomeWindow_h 1
class Perspective;
#endif

// ===========================================================================
// Colors.h (guard: _GC_Colors_h) — dpi scale factors used by the page ctors
// ===========================================================================
#ifndef _GC_Colors_h
#    define _GC_Colors_h 1
inline double dpiXFactor = 1.0;
inline double dpiYFactor = 1.0;
#endif

// ===========================================================================
// Settings.h (guard: _GC_Settings_h) — appsettings global; setCValue counts
// AND COPIES its cyclist argument, so the :887 chain
// (wizard->context->athlete->cyclist) is provably RUN on the live path (the
// copied value is assertable) and materializes as a real freed-memory READ
// on the dead one — a no-op stub would let the optimizer elide the argument
// evaluation entirely and hide the hazard.
// ===========================================================================
#ifndef _GC_Settings_h
#    define _GC_Settings_h
inline int g_setCValueCalls = 0;
inline QString g_lastCValueCyclist;
class Configuration
{
  public:
    void setCValue(const QString& cyclist, const QString&, const QVariant&)
    {
        ++g_setCValueCalls;
        g_lastCValueCyclist = cyclist;
    }
};
inline Configuration* appsettings = new Configuration();
#endif

// ===========================================================================
// Athlete.h (guard: _GC_Athlete_h) — validatePage reads cyclist, nothing else
// ===========================================================================
#ifndef _GC_Athlete_h
#    define _GC_Athlete_h
class Athlete
{
  public:
    QString cyclist;
};
#endif

// ===========================================================================
// RideMetadata.h (guard: _GC_RideMetadata_h) — FieldDefinition + RideMetadata,
// consumed by AddSettings' ctor via GlobalContext (returns an empty field set)
// ===========================================================================
#ifndef _GC_RideMetadata_h
#    define _GC_RideMetadata_h
class FieldDefinition
{
  public:
    QString name;
    bool isTextField() const { return false; }
};
class RideMetadata
{
  public:
    QList<FieldDefinition> getFields() const { return {}; }
};
#endif

// ===========================================================================
// Context.h (guard: _GC_Context_h) — Context + GlobalContext singleton.
// A QObject (no Q_OBJECT of its own: nothing connects to it, and moc must not
// run on a force-included preamble) so the production QPointer<Context> tracks
// destruction through QObject, as it does against the real Context in the app.
// ===========================================================================
#ifndef _GC_Context_h
#    define _GC_Context_h
class Context : public QObject
{
  public:
    Context() = default;
    QWidget* mainWindow = nullptr;
    Athlete* athlete = nullptr;
};
class GlobalContext
{
  public:
    RideMetadata* rideMetadata = nullptr;
    static GlobalContext* context()
    {
        static RideMetadata rm;
        static GlobalContext gc;
        gc.rideMetadata = &rm;
        return &gc;
    }
};
#endif

// ===========================================================================
// CloudService.h (guard: GC_CloudService_h) — the auth/settings contract the
// three frames under test drive, with call counters for the bail observables.
// ===========================================================================
#ifndef GC_CloudService_h
#    define GC_CloudService_h

class CloudServiceAthlete
{
  public:
    CloudServiceAthlete() : local(nullptr) {}
    QString id;
    QString name;
    QString desc;
    void* local;
};

inline bool g_openFails = false;         // browseFolder: script open() failure
inline int g_folderSelectedCalls = 0;    // browseFolder: post-dialog-exec call
inline int g_saveSettingsCalls = 0;      // validatePage: factory save
inline int g_syncDialogsConstructed = 0; // validatePage: the :892 construction

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

    explicit CloudService(Context* c = nullptr) : context(c) {}
    virtual ~CloudService() {}

    virtual CloudService* clone(Context*) { return nullptr; }
    virtual QString id() const { return serviceId; }
    virtual QString uiName() const { return serviceId; }
    virtual QString description() const { return QString(); }
    virtual QImage logo() const { return QImage(); }
    virtual int capabilities() const { return Query | Download; }
    virtual int type() const { return Activities; }

    virtual QVariant getSetting(QString, QVariant def = QVariant()) const { return def; }
    virtual void setSetting(QString, QVariant) {}
    virtual QString authiconpath() const { return QString(); }
    virtual QList<CloudServiceAthlete> listAthletes() { return {}; }
    virtual void selectAthlete(CloudServiceAthlete) {}
    virtual bool open(QStringList& errors)
    {
        if (g_openFails)
            errors << QStringLiteral("scripted connection failure");
        return !g_openFails;
    }
    virtual void persistConnectSuccess(const QString&, const QString&) {}
    virtual void disconnectService() {}

    virtual void folderSelected(QString) { ++g_folderSelectedCalls; }
    virtual QString syncOnStartupSettingName() const { return QStringLiteral("syncstartup"); }
    virtual QString syncOnImportSettingName() const { return QStringLiteral("syncimport"); }
    virtual QString activeSettingName() const { return QStringLiteral("active"); }

    QString serviceId; // test seam — the id() the wizard branches on
    QString message;
    QHash<CloudServiceSetting, QString> settings;
    Context* context = nullptr;
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
    bool addService(CloudService* s)
    {
        if (names_.contains(s->id()))
            return false;
        services_.insert(s->id(), s);
        names_.append(s->id());
        return true;
    }
    CloudService* newService(QString, Context*) { return nullptr; }
    void saveSettings(CloudService*, Context*) { ++g_saveSettingsCalls; }
};

// The folder picker of AddSettings::browseFolder — scriptable exec: parks in a
// real nested QEventLoop when g_folderExecRunsLoop (so a teardown can land
// inside it), otherwise returns the scripted result immediately (keeps the
// unscripted RED path from hanging on a modal that nobody closes).
inline bool g_folderExecRunsLoop = false;
inline int g_scriptedFolderDialogResult = 0;
inline QString g_scriptedFolderPath;
inline QEventLoop* g_folderLoop = nullptr;

class CloudServiceDialog : public QDialog
{
  public:
    CloudServiceDialog(QWidget* parent, CloudService*, QString, QString, bool) : QDialog(parent) {}
    QString pathnameSelected() const { return g_scriptedFolderPath; }
    int exec() override
    {
        if (!g_folderExecRunsLoop)
            return g_scriptedFolderDialogResult;
        QEventLoop loop;
        g_folderLoop = &loop;
        int r = loop.exec();
        g_folderLoop = nullptr;
        return r;
    }
};

// The :892 construction — counted, never executed further.
class CloudServiceSyncDialog : public QDialog
{
  public:
    CloudServiceSyncDialog(Context*, CloudService*) { ++g_syncDialogsConstructed; }
    bool start() { return true; }
};

#endif // GC_CloudService_h

// ===========================================================================
// OAuthDialog.h (guard: OAUTHDIALOG_H) — the doAuth() suspension point.
// sslLibMissing() defaults FALSE so the OAuth branch runs; exec() parks in a
// real nested QEventLoop the test quits after delivering its teardown. The
// dialog is parentless in production and here, so a wizard teardown does NOT
// end the loop — the user finishes (or abandons) the browser OAuth, exactly
// the arbitrarily-long window REQ-020 is about.
// ===========================================================================
#ifndef OAUTHDIALOG_H
#    define OAUTHDIALOG_H
inline int g_oauthDialogsConstructed = 0;
inline bool g_oauthSslMissing = false;
inline QEventLoop* g_oauthLoop = nullptr;

class OAuthDialog : public QDialog
{
  public:
    enum OAuthSite { NONE };
    OAuthDialog(Context* context, OAuthSite, CloudService* service, QString = QString(), QString = QString())
        : context(context), service(service)
    {
        ++g_oauthDialogsConstructed;
    }
    bool sslLibMissing() { return g_oauthSslMissing; }
    int exec() override
    {
        QEventLoop loop;
        g_oauthLoop = &loop;
        int r = loop.exec();
        g_oauthLoop = nullptr;
        return r;
    }

    Context* context = nullptr;
    CloudService* service = nullptr;
};
#endif

// ===========================================================================
// CalDAVDiscovery.h (guard: _Gc_CalDAVDiscovery_h) — never executed; must link
// ===========================================================================
#ifndef _Gc_CalDAVDiscovery_h
#    define _Gc_CalDAVDiscovery_h
class CalDAVDiscovery
{
  public:
    struct CalendarInfo
    {
        QString url;
        QString displayName;
    };
    static bool discoverCalendars(CloudService*, QList<CalendarInfo>*, QString* = nullptr) { return false; }
};
#endif // _Gc_CalDAVDiscovery_h

#endif // _GC_WIZARD_LIFETIME_STUB_PREAMBLE_H
