/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminBackfillDialog.h"

#include "Athlete.h"
#include "Context.h"
#include "GarminConnect.h"
#include "GarminSidecarStore.h"
#include "MainWindow.h"
#include "RideCache.h"
#include "RideImportWizard.h"

#include <QCloseEvent>
#include <QDate>
#include <QDateEdit>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMetaObject>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
// GarminBackfillController's verbatim wire-format (DES-010); see
// GarminBackfillController.cpp's kGarminTimeFormat.
QString toGarminTime(const QDate& d, const QTime& t)
{
    return QDateTime(d, t).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

// DEC-071 — the pending manifest's startTimeGMT is Garmin's verbatim
// server-side string (DES-010's format); parsed here purely to reach the
// RideCache::getRide(QDateTime) seam the DEC blessed. No tolerance window,
// no fallback format: an unparseable string cannot be matched, so it is left
// pending rather than guessed at.
QDateTime parsePendingStartTimeUtc(const QString& startTimeGMT)
{
    QDateTime dt = QDateTime::fromString(startTimeGMT, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}

// The on-disk path a pending entry's localFilename resolves to (mirrors
// GarminBackfillController::stagedPayloadPath's "backfill/" subdirectory,
// the ONLY place this controller stages bytes).
QString pendingEntryPath(const QString& athleteConfigDir, const QString& localFilename)
{
    return QDir(GarminSidecarStore::directoryFor(athleteConfigDir))
        .filePath(QStringLiteral("backfill/") + localFilename);
}
} // namespace

// Shell only - see the class comment in the header for why this dialog is
// parented to mainWindow rather than context->tab.
GarminBackfillDialog::GarminBackfillDialog(Context* context, GarminConnect* store, QWidget* parent)
    : QDialog(parent ? parent : (context ? context->mainWindow : nullptr)), context(context), store(store)
{
    setWindowTitle(tr("Garmin Connect: Backfill History"));
    setAttribute(Qt::WA_DeleteOnClose);
}

GarminBackfillDialog::~GarminBackfillDialog()
{
    // Mirrors CloudServiceSyncDialog's REQ-017 (e) ownership contract. Unlike
    // that dialog there is no StoreReaper here (see the class comment): if
    // `running` is still true we are being torn down with a controller call on
    // the stack (the disclosed residual gap - full application teardown only),
    // and the least-bad choice is the ORIGINAL DEC-025 one - decline to delete
    // rather than free the client out from under the frame executing on it.
    if (!running && store) {
        store->close();
        delete store;
    }
}

bool GarminBackfillDialog::start()
{
    QPointer<GarminBackfillDialog> self(this);

    QStringList errors;
    const bool opened = store->open(errors);
    if (self.isNull())
        return false;

    // B-R010-08 — `this` (mainWindow-parented) survived store->open()'s nested
    // loop but `context` did not: don't leave a hidden dialog holding an open
    // Garmin session until full application teardown - queue it closed, same
    // as the "open failed" branch just below. `context` is a QPointer member
    // (B-R010-06) so this reflects the live state directly - no local re-wrap.
    if (context.isNull()) {
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return false;
    }

    if (!opened) {
        QWidget::hide();
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Garmin Connect: Backfill History"));
        msgBox.setText(tr("Unable to connect, check your configuration in preferences."));
        msgBox.setDetailedText(errors.join("\n"));
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        if (self.isNull())
            return false;
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return false;
    }

    from = new QDateEdit(this);
    from->setCalendarPopup(true);
    from->setDate(QDate::currentDate().addYears(-1));
    to = new QDateEdit(this);
    to->setCalendarPopup(true);
    to->setDate(QDate::currentDate());

    startButton = new QPushButton(tr("Start"), this);
    cancelButton = new QPushButton(tr("Cancel"), this);
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 0); // indeterminate: total activity count is unknown up front
    progressBar->setVisible(false);
    progressLabel = new QLabel(tr("Choose a date range and click Start."), this);
    progressLabel->setWordWrap(true);

    QHBoxLayout* range = new QHBoxLayout;
    range->addWidget(new QLabel(tr("From:"), this));
    range->addWidget(from);
    range->addWidget(new QLabel(tr("To:"), this));
    range->addWidget(to);
    range->addStretch();

    QHBoxLayout* buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(cancelButton);
    buttons->addWidget(startButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(range);
    mainLayout->addWidget(progressLabel);
    mainLayout->addWidget(progressBar);
    mainLayout->addLayout(buttons);

    connect(startButton, &QPushButton::clicked, this, &GarminBackfillDialog::startClicked);
    connect(cancelButton, &QPushButton::clicked, this, &GarminBackfillDialog::cancelClicked);

    QWidget::show();
    return true;
}

void GarminBackfillDialog::startClicked()
{
    // B-R010-06 — checked BEFORE anything else, including `running`: a second
    // Start click reaching this dialog after the athlete tab has already
    // closed (this dialog is mainWindow-parented and outlives the tab, see
    // the class comment) must bail before touching `store` at all. `context`
    // is a QPointer tracked continuously from construction, so this is
    // reliable even when the staleness happened before this call began -
    // unlike a QPointer that gets freshly (re-)built from an already-dangling
    // raw pointer, which cannot retroactively detect that the object is gone.
    if (context.isNull()) {
        QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
        return;
    }

    if (running)
        return;

    running = true;
    startButton->setEnabled(false);
    from->setEnabled(false);
    to->setEnabled(false);
    cancelButton->setText(tr("Cancel"));
    progressBar->setVisible(true);
    progressLabel->setText(tr("Starting..."));

    const QString configDir = store->backfillConfigDir();
    const QString uid = store->backfillGarminUserId();
    const QString rangeStart = toGarminTime(from->date(), QTime(0, 0, 0));
    const QString rangeEnd = toGarminTime(to->date(), QTime(23, 59, 59));

    // B-R010-05 — `store` is owned by this dialog and, unlike `this`/`context`,
    // is deliberately NOT deleted while `running` is true (see ~GarminBackfill-
    // Dialog below): capturing it directly is safe across the nested loop
    // controller.start() is about to run, even in the disclosed residual
    // "`this` gone" case below. Binds the controller to the SAME fail-closed
    // checks readdir()/readFile() already gate on (REQ-017 clause a +
    // DEC-garmin-020), instead of driving the authenticated client with no
    // recheck at all.
    GarminConnect* const backfillStore = store;

    GarminBackfillController controller(backfillStore->backfillClient(), configDir, uid);
    runningController = &controller;

    QPointer<GarminBackfillDialog> self(this);
    // B-R010-06 — `context` is the QPointer member itself (tracked
    // continuously from construction, see the header), so it reflects
    // whatever the athlete tab did during controller.start()'s nested
    // QEventLoop with no local re-wrap needed: the hazard a fresh local
    // QPointer could not catch was staleness that happened BEFORE it was
    // built, which does not apply to a member watching since construction.
    const GarminBackfillController::Result r = controller.start(
        rangeStart, rangeEnd,
        [&](const QString&, const QString&, int importedSoFar) {
            if (!self.isNull())
                progressLabel->setText(tr("Imported %1 so far...").arg(importedSoFar));
        },
        // B-R010-06 (final piece) — a QPointer<Context> COPY captured by
        // value, checked FIRST: a stale `context` (ordinary athlete-tab
        // close landing mid-run) is treated as "session no longer valid"
        // without ever calling backfillSessionStillValid() - the deeper
        // GarminConnect/CloudService context-handling issue that call would
        // otherwise reach into is deferred, not fixed here.
        [backfillStore, context = context]() {
            return !context.isNull() && backfillStore->backfillSessionStillValid();
        });

    // DEC-garmin-025-style self-bail: `this` (and therefore progressBar/
    // progressLabel/runningController) may be gone if a full application
    // teardown landed inside the nested loop controller.start() just ran -
    // see the class comment's disclosed residual gap.
    if (self.isNull())
        return;

    runningController = nullptr;
    progressBar->setVisible(false);

    switch (r.outcome) {
    case GarminBackfillController::Outcome::Done:
        progressLabel->setText(r.importedCount > 0 ? tr("Done. Imported %1 activities.").arg(r.importedCount)
                                                   : tr("Done. No new activities in this range."));
        break;
    case GarminBackfillController::Outcome::Paused:
        progressLabel->setText(r.message.isEmpty()
                                   ? tr("Paused after importing %1 activities.").arg(r.importedCount)
                                   : tr("Paused: %1 (imported %2 so far)").arg(r.message).arg(r.importedCount));
        break;
    case GarminBackfillController::Outcome::Rejected:
        progressLabel->setText(r.message);
        break;
    }

    startButton->setEnabled(true);
    from->setEnabled(true);
    to->setEnabled(true);
    cancelButton->setText(tr("Close"));

    // DEC-071 — hand off every currently PENDING entry, not just what this
    // run's onProgress reported: the manifest is exactly what makes a
    // previously-cancelled activity re-offered on a LATER run, since a
    // pending entry's bytes are already staged on disk (its own
    // localFilename) and do not need re-listing or re-downloading to reach
    // the wizard again. `pendingLoad` is read ONCE here and reused below to
    // evaluate completion, so the id/entry set the wizard actually saw and
    // the set this dialog checks against RideCache are the same set.
    const GarminSidecarStore::BackfillLoadResult pendingLoad = GarminSidecarStore::loadBackfillState(configDir, uid);
    QStringList stagedFiles;
    if (pendingLoad.isOk()) {
        for (auto it = pendingLoad.state.pending.constBegin(); it != pendingLoad.state.pending.constEnd(); ++it) {
            const QString path = pendingEntryPath(configDir, it.value().localFilename);
            if (QFile::exists(path)) {
                stagedFiles << path;
            } else {
                // B-STAGE9-112/-120 — explicit drop, not silent exclusion:
                // GarminSidecarStore::dropPendingBackfill clears the
                // persisted cursor when it drops the entry the cursor points
                // at; DEC-078's split filter then reads that as "no prior
                // success" and applies rangeStart inclusively, which is what
                // re-lists this activity on a later run.
                GarminSidecarStore::dropPendingBackfill(configDir, uid, it.key());
            }
        }
    }

    // Hand off whatever is pending to GC's existing FIT/TCX import pipeline
    // (parse -> DataProcessorFactory::autoProcess -> JSON save -> RideCache
    // registration) - GarminBackfillController's own job stops at "downloaded
    // and staged atomically" (DES-009), same separation of concerns DES-010
    // draws between GarminConnect::readFile (stage bytes) and
    // CloudServiceSyncDialog::saveRide (parse+register). See the B-R010-04
    // build report for why there is no reusable "drop it and GC picks it up"
    // staging directory to redirect into instead.
    //
    // B-R010-06 — gated on `context` itself (the QPointer member): a stale
    // `context` is detected here rather than handed to RideImportWizard's
    // constructor, and the SAME checked pointer is what gets passed below -
    // no separate unchecked raw copy to fall out of sync with the check.
    if (!stagedFiles.isEmpty() && !context.isNull()) {
        // B-R010-07 — `running` is still true here and stays true until this
        // call returns (cleared below): wizard->process() pumps its own
        // nested event loop, and the wizard is parented to `this`, so a
        // close of THIS dialog landing mid-process() would take the wizard
        // down while it is still on the stack. deferCloseIfBusy() must keep
        // vetoing closes for the whole hand-off, not just through
        // controller.start().
        //
        // RESIDUAL GAP (disclosed, not fixed here): RideImportWizard itself
        // holds `context` as a raw Context* internally (src/Gui/RideImportWizard.h)
        // with no QPointer of its own, so an athlete-tab close landing inside
        // wizard->process()'s OWN nested event pumping can still invalidate
        // the context it is already holding - this check only guarantees
        // `context` was alive at the moment the wizard was constructed.
        // Fixing that is out of scope here (RideImportWizard is pre-existing,
        // shared code - not to be modified for this dialog).
        // B-STAGE9-114 — QPointer guards, not raw-pointer re-reads: `context`
        // surviving wizard->process()'s nested loop does not mean
        // `context->athlete` does, and Athlete/RideCache are themselves
        // QObjects, so a locally-owned QPointer tracks their destruction
        // independently of Context's own (non-tracking) raw member.
        QPointer<Athlete> athleteGuard = context.isNull() ? nullptr : context->athlete;
        QPointer<RideCache> rideCacheGuard = athleteGuard.isNull() ? nullptr : athleteGuard->rideCache;

        RideImportWizard* wizard = new RideImportWizard(stagedFiles, context, this);
        wizard->process();

        // DEC-071 — evaluate EACH pending entry via RideCache, the seam the
        // wizard itself uses (RideImportWizard.cpp's own already-imported
        // test), settled by measurement to need no tolerance window and no
        // FIT pre-parse. A match means the wizard actually saved this ride:
        // promote it to imported-<uid>.json (completion) and drop it from
        // pending. No match (cancelled, skipped, or still mid-wizard) leaves
        // the entry pending - re-offered on the next run by this same block.
        // This evaluation is a best-effort completion sweep, not a
        // correctness-critical write.
        if (!context.isNull() && !athleteGuard.isNull() && !rideCacheGuard.isNull()) {
            RideCache* const rideCache = rideCacheGuard.data();
            for (auto it = pendingLoad.state.pending.constBegin(); it != pendingLoad.state.pending.constEnd(); ++it) {
                const QDateTime startUtc = parsePendingStartTimeUtc(it.value().startTimeGMT);
                if (!startUtc.isValid())
                    continue; // cannot be matched - stays pending rather than guessed at
                if (rideCache->getRide(startUtc) == nullptr)
                    continue; // no RideCache match yet - stays pending, re-offered
                // B-STAGE9-113 — recordImported()'s result gates the drop: a
                // failed completion write must leave the entry in `pending`,
                // never in neither manifest.
                if (GarminSidecarStore::recordImported(configDir, uid, it.key(), it.value()))
                    GarminSidecarStore::dropPendingBackfill(configDir, uid, it.key());
            }
        }
    }

    // B-R010-07 — cleared LAST, after the wizard hand-off above completes
    // (not immediately after controller.start() returned): see the class
    // comment and deferCloseIfBusy() for why the busy window has to cover
    // the whole hand-off.
    running = false;
}

void GarminBackfillDialog::cancelClicked()
{
    if (deferCloseIfBusy())
        return;
    reject();
}

void GarminBackfillDialog::done(int result)
{
    if (deferCloseIfBusy())
        return;
    QDialog::done(result);
}

void GarminBackfillDialog::closeEvent(QCloseEvent* e)
{
    if (deferCloseIfBusy()) {
        e->ignore();
        return;
    }
    QDialog::closeEvent(e);
}

bool GarminBackfillDialog::deferCloseIfBusy()
{
    if (!running)
        return false;

    // Ask the in-flight run to stop; startClicked()'s frame will finish
    // (Outcome::Paused) and re-enable the UI, at which point a repeat close
    // succeeds. Cooperative, same-thread cancel() - see GarminBackfillController.h.
    //
    // B-R010-07 — `running` also stays true through the post-run
    // RideImportWizard hand-off, where `runningController` is already null
    // (the controller has finished and there is nothing left to cancel): this
    // veto alone is what keeps the dialog alive while the wizard - parented
    // to `this` - is still on the stack in wizard->process().
    if (runningController)
        runningController->cancel();
    return true;
}
