/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// B-R010-06 (T-194/T-195) — force-included (-include) before every .cpp of the
// testGarminBackfillDialogLifetime target, INCLUDING the AUTOMOC-generated moc
// files of the REAL src/Cloud/GarminBackfillDialog.cpp this target compiles.
// Sibling of stubs/WizardLifetimeStubPreamble.h (same guard-predefinition
// trick, same rationale — see there) but far narrower: GarminBackfillDialog's
// own dependency surface is Athlete/Context/GarminConnect/MainWindow/
// RideImportWizard, not the AddCloudWizard's CloudService/OAuthDialog stack.
//
//   - Context is a QObject (no Q_OBJECT of its own — nothing connects to it,
//     and moc must not run on a force-included preamble), exactly as in
//     WizardLifetimeStubPreamble: the production QPointer<Context> member
//     under test must track a REAL QObject destruction, not a plain struct.
//   - GarminConnect is a from-scratch stand-in (NOT the real CloudService-
//     derived class — GarminBackfillDialog.cpp only ever calls its own
//     concrete methods, never anything through the CloudService base), with a
//     call counter per method so a test can pin that a bailed startClicked()
//     never reached `store` at all.
//   - RideImportWizard is a from-scratch stand-in (not the real import
//     pipeline) — T-194/T-195 never populate stagedFiles, so it is never
//     actually constructed; it only needs to exist for GarminBackfillDialog.cpp
//     to compile against.
//
// GarminBackfillController itself is compiled REAL (same as
// testGarminBackfillController) against a Python-free IGarminDownloadClient
// fake, so the controller's own state machine is exercised for real, not
// stubbed away.

#ifndef _GC_BACKFILL_DIALOG_LIFETIME_STUB_PREAMBLE_H
#define _GC_BACKFILL_DIALOG_LIFETIME_STUB_PREAMBLE_H

#include <QDialog>
#include <QList>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>

// ===========================================================================
// MainWindow.h (guard: _GC_MainWindow_h) — Context::mainWindow is a QWidget*
// ===========================================================================
#ifndef _GC_MainWindow_h
#    define _GC_MainWindow_h 1
#endif

// ===========================================================================
// Athlete.h (guard: _GC_Athlete_h)
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
// Context.h (guard: _GC_Context_h) — a QObject so the production
// QPointer<Context> member tracks destruction for real, exactly as it does
// against the real Context in the app.
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
#endif

// ===========================================================================
// GarminConnect.h (guard: GC_GarminConnect_h) — a from-scratch stand-in (see
// block comment above): every method GarminBackfillDialog.cpp calls is
// call-counted so a bailed startClicked() can be pinned as having never
// reached `store`.
// ===========================================================================
#ifndef GC_GarminConnect_h
#    define GC_GarminConnect_h
#    include "IGarminDownloadClient.h"

inline int g_backfillOpenCalls = 0;
inline int g_backfillCloseCalls = 0;
inline int g_backfillConfigDirCalls = 0;
inline int g_backfillUserIdCalls = 0;
inline int g_backfillClientCalls = 0;
inline int g_backfillSessionStillValidCalls = 0;
inline bool g_backfillOpenSucceeds = true;

class GarminConnect
{
  public:
    bool open(QStringList& errors)
    {
        ++g_backfillOpenCalls;
        if (!g_backfillOpenSucceeds)
            errors << QStringLiteral("scripted open failure");
        return g_backfillOpenSucceeds;
    }
    bool close()
    {
        ++g_backfillCloseCalls;
        return true;
    }
    QString backfillConfigDir() const
    {
        ++g_backfillConfigDirCalls;
        return configDir;
    }
    QString backfillGarminUserId()
    {
        ++g_backfillUserIdCalls;
        return uid;
    }
    IGarminDownloadClient* backfillClient()
    {
        ++g_backfillClientCalls;
        return client;
    }
    bool backfillSessionStillValid()
    {
        ++g_backfillSessionStillValidCalls;
        return true;
    }

    IGarminDownloadClient* client = nullptr; // not owned
    QString configDir;
    QString uid;
};
#endif // GC_GarminConnect_h

// ===========================================================================
// RideImportWizard.h (guard: _RideImportWizard_h) — never actually
// constructed by T-194/T-195 (both runs stage no files); exists only so
// GarminBackfillDialog.cpp compiles against it. See src/Gui/RideImportWizard.h
// for the real (unmodified) production class this stands in for.
// ===========================================================================
#ifndef _RideImportWizard_h
#    define _RideImportWizard_h
inline int g_rideImportWizardConstructions = 0;
class RideImportWizard : public QDialog
{
  public:
    RideImportWizard(QList<QString>, Context*, QWidget* parent = nullptr) : QDialog(parent)
    {
        ++g_rideImportWizardConstructions;
    }
    int process() { return 0; }
};
#endif // _RideImportWizard_h

#endif // _GC_BACKFILL_DIALOG_LIFETIME_STUB_PREAMBLE_H
