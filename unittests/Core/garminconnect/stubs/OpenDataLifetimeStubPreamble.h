/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the License as published by the Free Software Foundation.
 */

// REQ-022 / T-164..T-166 — force-included (-include) before every .cpp of
// the testGarminConnectOpenDataLifetime target, INCLUDING the
// AUTOMOC-generated moc files. Same guard-predefinition trick as
// stubs/WizardLifetimeStubPreamble.h (see the block comment there): the
// REAL src/Cloud/OpenData.cpp is compiled into the target unchanged, and
// the heavyweight headers it pulls (Context/Athlete/RideCache/Settings/
// Colors/CloudService/RideFile/RideItem) are short-circuited to the
// stand-ins below.
//
// The Q_OBJECT Context stand-in is NOT here — it lives in
// stubs/OpenDataLifetimeStubContext.h, listed as a target source so AUTOMOC
// mocs its metaobject (string-based connect needs one; this preamble must
// stay moc-free).
//
// Secrets.h is taken REAL (pure macros, guard _GC_SECRETS_H) so
// GC_CLOUD_OPENDATA_SECRET resolves exactly as the app build resolves it
// (the "__GC_CLOUD_OPENDATA_SECRET__" fallback when no secret is
// configured). contrib/qzip stays REAL (linked zip.cpp + ZLIB) so STEP
// THREE's ZipWriter work is the shipping code.

#ifndef _GC_OPENDATA_LIFETIME_STUB_PREAMBLE_H
#define _GC_OPENDATA_LIFETIME_STUB_PREAMBLE_H

#include <QDate>
#include <QDialog>
#include <QEventLoop>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QList>
#include <QRandomGenerator>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QTemporaryFile>
#include <QUuid>
#include <QVBoxLayout>
#include <QVariant>
#include <QVector>
#include <QWidget>
#include <QtGlobal>

class QNetworkReply;

// ===========================================================================
// Context.h (guard: _GC_Context_h) — see stubs/OpenDataLifetimeStubContext.h
// ===========================================================================
#ifndef _GC_Context_h
#    define _GC_Context_h 1
#    include "OpenDataLifetimeStubContext.h"
#endif

// ===========================================================================
// Athlete.h (guard: _GC_Athlete_h) — cyclist/id/rideCache are the only
// members run() derefs (OpenData.cpp:224/233/238/307/312/344-346).
// ===========================================================================
#ifndef _GC_Athlete_h
#    define _GC_Athlete_h 1
class RideCache;
class Athlete
{
  public:
    QString cyclist;
    QUuid id;
    RideCache* rideCache = nullptr;
};
#endif

// ===========================================================================
// RideCache.h (guard: _GC_RideCache_h) — save() counts AND COPIES its
// filename so the context->athlete->rideCache->save chain is provably
// evaluated (a no-op stub could let the optimizer elide the freed-memory
// read the RED run must surface; same trick as WizardLifetimeStubPreamble's
// setCValue).
// ===========================================================================
#ifndef _GC_RideCache_h
#    define _GC_RideCache_h 1
class RideItem;
inline int g_ridesSaveCalls = 0;
inline QString g_lastRidesSaveName;
class RideCache
{
  public:
    void save(bool, QString filename)
    {
        ++g_ridesSaveCalls;
        g_lastRidesSaveName = filename;
    }
    int count() const { return 0; }
    QVector<RideItem*> rides() { return {}; }
};
#endif

// ===========================================================================
// RideItem.h (guard: _GC_RideItem_h) — path/fileName, read in run()'s
// STEP THREE foreach (never entered: the stub rides() list is empty).
// ===========================================================================
#ifndef _GC_RideItem_h
#    define _GC_RideItem_h 1
class RideItem
{
  public:
    QString path;
    QString fileName;
};
#endif

// ===========================================================================
// RideFile.h (guard: _RideFile_h) — RideFileFactory/RideFile/RideFilePoint/
// RideFileDataPresent, only referenced inside that same never-entered
// foreach body, but they must compile and link.
// ===========================================================================
#ifndef _RideFile_h
#    define _RideFile_h 1
class QFile;
inline int g_openRideFileCalls = 0;
class RideFilePoint
{
  public:
    double secs = 0, km = 0;
    int watts = 0, hr = 0, cad = 0, alt = 0;
};
class RideFileDataPresent
{
  public:
    bool watts = false, hr = false, cad = false, alt = false;
};
class RideFile
{
  public:
    QList<RideFilePoint*> dataPoints() const { return {}; }
    RideFileDataPresent* areDataPresent() const
    {
        static RideFileDataPresent dp;
        return &dp;
    }
};
class RideFileFactory
{
  public:
    static RideFileFactory& instance()
    {
        static RideFileFactory f;
        return f;
    }
    RideFile* openRideFile(Context*, QFile&, QStringList& errors, QList<RideFile*>* = nullptr) const
    {
        ++g_openRideFileCalls;
        errors.clear();
        return nullptr;
    }
};
#endif

// ===========================================================================
// Settings.h (guard: _GC_Settings_h) — appsettings global + the five
// GC_OPENDATA_* key macros (real Settings.h:258-264). setCValue records
// its key so the STEP FIVE tail (:344-346) is observable as three writes.
// ===========================================================================
#ifndef _GC_Settings_h
#    define _GC_Settings_h 1
#    define GC_OPENDATA_GRANTED "<athlete-general>opendata/allowed"
#    define GC_OPENDATA_RUNCOUNT "<athlete-general>opendata/runcount"
#    define GC_OPENDATA_LASTPOSTED "<athlete-general>opendata/postingdate"
#    define GC_OPENDATA_LASTPOSTCOUNT "<athlete-general>opendata/count"
#    define GC_OPENDATA_LASTPOSTVERSION "<athlete-general>opendata/version"
inline int g_cvalueCalls = 0;
inline int g_setCValueCalls = 0;
inline QStringList g_setCValueKeys;
class Configuration
{
  public:
    QVariant cvalue(const QString&, const QString& key, QVariant def = QVariant())
    {
        Q_UNUSED(key)
        ++g_cvalueCalls;
        return def;
    }
    void setCValue(const QString&, const QString& key, const QVariant&)
    {
        ++g_setCValueCalls;
        g_setCValueKeys.append(key);
    }
};
inline Configuration* appsettings = new Configuration();
#endif

// ===========================================================================
// Colors.h (guard: _GC_Colors_h) — dpi scale factors used by OpenDataDialog
// (compiled in the same TU, never constructed by these tests).
// ===========================================================================
#ifndef _GC_Colors_h
#    define _GC_Colors_h 1
inline double dpiXFactor = 1.0;
inline double dpiYFactor = 1.0;
#endif

// ===========================================================================
// CloudService.h (guard: GC_CloudService_h) — onSslErrors forwards to this
// static helper (real CloudService.h:433); counted so a late queued call is
// observable.
// ===========================================================================
#ifndef GC_CloudService_h
#    define GC_CloudService_h 1
class QSslError;
inline int g_sslErrorsCalls = 0;
class CloudService
{
  public:
    static void sslErrors(QWidget*, QNetworkReply*, QList<QSslError>) { ++g_sslErrorsCalls; }
};
#endif

#include <QSslError>

#endif // _GC_OPENDATA_LIFETIME_STUB_PREAMBLE_H
