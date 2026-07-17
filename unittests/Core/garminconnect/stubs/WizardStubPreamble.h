/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// REQ-002 / TEST-007 / A3-R002-TR-01 — force-included (-include) before every
// .cpp of the testGarminConnectWizardRouting target, INCLUDING the AUTOMOC-
// generated moc files. It pre-defines the include guards of the heavyweight
// GoldenCheetah headers that AddCloudWizard.{h,cpp} pull in (co-located ones in
// src/Cloud such as CloudService.h/OAuthDialog.h/PyEmbeddedAdapter.h cannot be
// shadowed on the include path, because a quote-include resolves the compiled
// file's own directory first — so the guard-predefinition trick is required),
// and substitutes minimal stand-ins. Mirrors stubs/GCStubPreamble.h.
//
// What stays REAL (linked from src/Cloud, all Python-free):
//   AddCloudWizard.cpp  — the routing/lifecycle code under test
//   GarminAuthChain / GarminWorker / WorkerAuthClient / GarminCredentialsPage
//   IGarminAuthClient.h / IGarminPyAdapter.h
//
// What is stubbed here:
//   - heavy GC headers (Context/Athlete/MainWindow/Settings/Colors/CloudService/
//     OAuthDialog/RideMetadata/GoldenCheetah/Serial, plus RideItem/Perspective
//     which moc pulls into moc_AddCloudWizard.cpp defensively)
//   - PyEmbeddedAdapter — replaced by a Python-free fake implementing
//     IGarminPyAdapter (the sanctioned DEC-013 seam), keeping the target on
//     `garmin-fast`.

#ifndef _GC_WIZARD_STUB_PREAMBLE_H
#define _GC_WIZARD_STUB_PREAMBLE_H

// clang-format off
// --- Qt surface the wizard normally gets transitively via the heavy headers ---
#include <QCheckBox>
#include <QComboBox>
#include <QCommandLinkButton>
#include <QDialog>
#include <QDir>
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
// Settings.h (guard: _GC_Settings_h) — appsettings global (never-executed path)
// ===========================================================================
#ifndef _GC_Settings_h
#    define _GC_Settings_h
class Configuration
{
  public:
    void setCValue(const QString&, const QString&, const QVariant&) {}
};
inline Configuration* appsettings = new Configuration();
#endif

// ===========================================================================
// Athlete.h (guard: _GC_Athlete_h)
// ===========================================================================
#ifndef _GC_Athlete_h
#    define _GC_Athlete_h
class AthleteDirectoryStructure
{
  public:
    QDir config() const { return QDir(QStringLiteral("/tmp/gc-garmin-test")); }
};
class Athlete
{
  public:
    QString cyclist;
    AthleteDirectoryStructure* home = nullptr;
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
// Context.h (guard: _GC_Context_h) — Context + GlobalContext singleton
// ===========================================================================
#ifndef _GC_Context_h
#    define _GC_Context_h
class Context
{
  public:
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
// CloudService.h (guard: GC_CloudService_h) — routing contract + the
// never-executed collaborators the wizard TU references.
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

    // Routing contract — id() is overridable via the serviceId seam so a test
    // can flip the value the wizard branches on without a subclass.
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
    virtual bool open(QStringList&) { return true; }
    virtual void folderSelected(QString) {}
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
    void saveSettings(CloudService*, Context*) {}
};

// Never executed by the tests (browseFolder / AddFinish paths) — must link.
class CloudServiceDialog : public QDialog
{
  public:
    CloudServiceDialog(QWidget*, CloudService*, QString, QString, bool) {}
    QString pathnameSelected() const { return QString(); }
};

class CloudServiceSyncDialog : public QDialog
{
  public:
    CloudServiceSyncDialog(Context*, CloudService*) {}
};

#endif // GC_CloudService_h

// ===========================================================================
// OAuthDialog.h (guard: OAUTHDIALOG_H) — never executed; must link
// ===========================================================================
#ifndef OAUTHDIALOG_H
#    define OAUTHDIALOG_H
class OAuthDialog : public QDialog
{
  public:
    enum OAuthSite { NONE };
    OAuthDialog(Context*, OAuthSite, CloudService*, QString = QString(), QString = QString()) {}
    bool sslLibMissing() { return true; }
};
#endif

// ===========================================================================
// PyEmbeddedAdapter.h (guard: GC_PyEmbeddedAdapter_h) — Python-free fake of the
// DEC-013 embedded-CPython adapter. Same public surface as the real header
// (ctor(modulePath) — AUTH-ONLY, DEC-014 Option B — + authenticate()), so AddCloudWizard.cpp's
// `new PyEmbeddedAdapter(...)` compiles and links with no interpreter. The
// `observedThread` seam is used only by the destructor-order test (behaviour 3):
// if this adapter is destroyed while the chain's worker thread is still running,
// ~AddCloudWizard violated DES-001a. QPointer auto-nulls when the observed
// QThread is destroyed, so a correct teardown (chain first) leaves it null.
// ===========================================================================
#ifndef GC_PyEmbeddedAdapter_h
#    define GC_PyEmbeddedAdapter_h
#    include "IGarminPyAdapter.h"

// Shared across the AddCloudWizard TU and the test TU (C++17 inline vars).
inline int g_pyAdapterLiveCount = 0;
inline bool g_pyAdapterDeletedWhileWorkerThreadRunning = false;

class PyEmbeddedAdapter : public IGarminPyAdapter
{
  public:
    explicit PyEmbeddedAdapter(const QString& modulePath) : m_modulePath(modulePath) { ++g_pyAdapterLiveCount; }

    ~PyEmbeddedAdapter() override
    {
        if (observedThread && observedThread->isRunning())
            g_pyAdapterDeletedWhileWorkerThreadRunning = true;
        --g_pyAdapterLiveCount;
    }

    PyAuthOutcome authenticate(const QString&, const QString&) override
    {
        PyAuthOutcome o;
        o.kind = PyAuthOutcome::Unknown;
        return o;
    }

    // REQ-007 seam extension (DEC-013 compile-enforced) — the wizard-routing
    // test drives auth lifecycle only; a default outcome satisfies the seam.
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }

    // REQ-007 closure (Slice 1) seam extension (DEC-013 compile-enforced) — the
    // wizard-routing test never restores a session; a default outcome suffices.
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }

    // Set by the destructor-order test to the chain's worker thread.
    QPointer<QThread> observedThread;

    QString modulePath() const { return m_modulePath; }

  private:
    QString m_modulePath;
};
#endif // GC_PyEmbeddedAdapter_h

#endif // _GC_WIZARD_STUB_PREAMBLE_H
