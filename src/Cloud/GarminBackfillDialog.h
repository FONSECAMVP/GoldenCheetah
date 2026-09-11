/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// B-R010-04 — REQ-010 UI wiring: a lightweight dialog driving
// GarminBackfillController::start()/cancel() directly.
//
// NOT CloudServiceSyncDialog reuse. GarminBackfillController is deliberately
// decoupled from CloudService (DES-009's own header comment: DES-010's
// incremental sync owns the CloudService integration for REQ-008), and
// CloudServiceSyncDialog's ~1800 lines of DEC-garmin-024..038 hardening are
// keyed to a SHARED, externally-orphanable `CloudService *store` (StoreReaper,
// per-row QTreeWidgetItem tickets, batch/list generations) that this dialog
// has no equivalent of: the controller here is owned exclusively by this
// dialog, nobody else holds a pointer into it.
//
// PARENTING, DELIBERATELY DIFFERENT FROM DEC-garmin-030. CloudServiceSyncDialog
// is parented to context->tab so athlete-tab close destroys it promptly; this
// dialog is parented to context->mainWindow instead, BECAUSE its controller
// (and the store it drives) live as a value/owned-pointer scoped to a call
// running on THIS dialog's own stack frame (startClicked -> a local
// GarminBackfillController::start(), a nested QEventLoop) - unlike
// CloudService::store, nothing outside this dialog can reap a controller mid-
// call, so the only safe general answer is "the frame executing on it must not
// be freed out from under it". context->tab closing is an everyday action (the
// hazard DEC-garmin-030 exists for); parenting there would hand this dialog
// that same fate with none of StoreReaper's machinery to survive it. Parenting
// to mainWindow instead means only a full application teardown can destroy
// this dialog synchronously mid-call - narrower, and disclosed as a residual
// gap in the B-R010-04 build report rather than solved (that would need a
// StoreReaper-equivalent of its own).
//
// Consequence: context (and context->athlete etc) CAN still go stale mid-call
// on an ordinary tab close even though `this` survives it - the same
// DEC-garmin-030 "collaborator" shape, guarded the same way (QPointer<Context>).
#ifndef GC_GarminBackfillDialog_h
#define GC_GarminBackfillDialog_h

#include "GarminBackfillController.h"

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QStringList>

class Context;
class GarminConnect;
class QCloseEvent;
class QDateEdit;
class QLabel;
class QProgressBar;
class QPushButton;

class GarminBackfillDialog : public QDialog
{
    Q_OBJECT

  public:
    // REQ-017 (e)-equivalent contract (mirrors CloudServiceSyncDialog): this
    // dialog OWNS `store` and closes+deletes it in its destructor. Shell only -
    // no nested event loop runs here (DEC-garmin-026 two-phase init pattern);
    // start() does the rest.
    GarminBackfillDialog(Context* context, GarminConnect* store, QWidget* parent = nullptr);
    ~GarminBackfillDialog() override;

  public slots:
    // Phase two: opens `store`, builds the rest of the widget, and shows it.
    // Returns false on open failure (the dialog then closes itself via a
    // queued close(), same as CloudServiceSyncDialog's start()) or if `this`/
    // context died inside store->open()'s nested loop.
    bool start();

    void startClicked();
    void cancelClicked();
    void done(int result) override;

  protected:
    void closeEvent(QCloseEvent* e) override;

  private:
    // Veto a close while a backfill run is on the stack, asking it to cancel
    // first (mirrors CloudServiceSyncDialog's deferCloseIfBusy, minus
    // StoreReaper - see the class comment for why none is needed here).
    bool deferCloseIfBusy();

    // B-R010-06 — a QPointer, not a raw Context*, so staleness is tracked
    // CONTINUOUSLY from construction rather than re-checked locally inside
    // individual methods: a QPointer built fresh from an already-dangling raw
    // pointer does not retroactively detect that the object is gone (see
    // startClicked()'s entry guard).
    QPointer<Context> context;
    GarminConnect* store; // owned

    bool running = false;
    // Non-owning; valid only while `running` (set/cleared by startClicked()),
    // so cancelClicked() can reach the in-flight controller re-entrantly.
    GarminBackfillController* runningController = nullptr;

    QDateEdit* from = nullptr;
    QDateEdit* to = nullptr;
    QPushButton* startButton = nullptr;
    QPushButton* cancelButton = nullptr;
    QProgressBar* progressBar = nullptr;
    QLabel* progressLabel = nullptr;
};

#endif // GC_GarminBackfillDialog_h
