/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-070 / garmin:TEST-071 — DEC-garmin-024 (A3-R017-F1 + F3).
// TEST garmin:TEST-072 / garmin:TEST-073 — DEC-garmin-025 (A3-R017b-F1 + F4).
//
// THE DEFECT THIS CHANGESET INTRODUCED
// ------------------------------------
// REQ-017 Slice B made the modeless post-connect sync dialog own its store and
// delete itself on close (AddCloudWizard.cpp: setAttribute(Qt::WA_DeleteOnClose)
// then open()). That closed a leak. It also opened a use-after-free, because
// GarminConnect::readFile/readdir run NESTED QEventLoops (blockingDownload /
// blockingList / blockingRestore, GarminConnect.cpp:236-320) and therefore
// process GUI events in the middle of a call the dialog made ON THE STORE:
//
//   syncNext() -> store->readFile(...) -> blockingDownload() -> loop.exec()
//       -> user closes the dialog -> WA_DeleteOnClose -> deleteLater()
//       -> ~CloudServiceSyncDialog -> closeAndDeleteStore(store) -> delete store
//   loop.exec() returns -> readFile resumes ON A FREED `this`.
//
// WHY THIS FILE EXISTS AT ALL, AND WHY IT IS BUILT WITH -fsanitize=address
// -----------------------------------------------------------------------
// The claim "WA_DeleteOnClose uses deleteLater(), and Qt delivers DeferredDelete
// only at or below the posting loop level, so the dialog is not destroyed inside
// the nested frame" was written into a design comment, sounds correct, and is
// FALSE. Qt compares the posted event's level against loopLevel + scopeLevel, and
// a close arriving through ordinary event DELIVERY is posted from inside a
// scopeLevel bump (QCoreApplication::notifyInternal2) — so its level is strictly
// GREATER than the nested loop's, and the nested loop delivers it. Measured on
// the Qt this tree builds against, not reasoned about.
//
// A lifetime test that "passes" because the freed bytes happened not to be reused
// proves nothing. So this target is compiled and linked with AddressSanitizer and
// the build fails outright if it is not — see the #error below. Before the fix the
// binary aborts with `heap-use-after-free ... in BlockingStore::readFile`.
//
// WHAT IS REAL HERE
// -----------------
// The REAL src/Cloud/CloudService.cpp is compiled in (the same seam TEST-067 and
// TEST-069 established), so the class under test is the REAL
// CloudServiceSyncDialog: its real constructor, real refresh/select/download
// slots, real syncNext, real completedRead and its real destructor — the one that
// calls closeAndDeleteStore(). The only stand-in is the SERVICE, and it stands in
// for GarminConnect by doing the one thing that matters: running a nested
// QEventLoop inside readFile and then continuing to touch itself afterwards,
// exactly as GarminConnect::readFile does around blockingDownload().
//
// The scenario is driven from inside a QEventLoop standing in for
// QApplication::exec(), and the close is delivered as a QUEUED call, because
// event-loop LEVEL and event-delivery SCOPE are the whole subject of the bug: a
// harness that closes the dialog with a direct call from loop level 0 does not
// reproduce it and would be worthless.
//
// THE SECOND ROUTE — PARENT TEARDOWN (TEST-072/073, DEC-garmin-025)
// -----------------------------------------------------------------
// Everything above is about a close the dialog INITIATES: close(), reject(),
// Escape — all of which funnel through closeEvent()/done(), which is where
// DEC-024 put its gate. Qt has a second way to destroy this dialog that goes
// through NEITHER: when a parent QObject is destroyed it destroys its children
// DIRECTLY from ~QObject — no close event, no done(), no virtual dispatch, and
// nothing a gate can veto. The dialog is parented to context->mainWindow
// (CloudService.cpp:709) and MainWindow itself carries WA_DeleteOnClose
// (MainWindow.cpp:143), so closing the athlete window mid-sync runs
// ~CloudServiceSyncDialog with a nested QEventLoop still on the stack.
//
// That frees TWO objects the suspended frames are still using:
//   * the STORE, via closeAndDeleteStore() — readFile is executing on it;
//   * the DIALOG itself — ~BlockingCall writes dialog->blockingCallDepth
//     (CloudService.cpp:1011) and syncNext resumes at (:1521), both on `this`.
// So guarding only the store leaves the identical trigger crashing, which is
// why TEST-072 asserts on both halves.

#include "Athlete.h"
#include "CloudService.h"
#include "Context.h"
#include "RideCache.h"
#include "RideItem.h"

#include <QApplication>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QKeyEvent>
#include <QList>
#include <QMessageBox>
#include <QMetaObject>
#include <QPointer>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>
#include <QtTest/QtTest>

#if !defined(__SANITIZE_ADDRESS__) && !defined(GC_TEST_WITH_ASAN)
#    error \
        "TEST-070/TEST-071 are lifetime tests and are only trustworthy under AddressSanitizer; build this target with -fsanitize=address."
#endif

#include <sanitizer/asan_interface.h>

#include <functional>

// ---------------------------------------------------------------------------
// Observations — everything the store records about its own call, kept OUTSIDE
// the store.
//
// The whole point of TEST-070 is that the store may be dead when readFile
// resumes. Recording into a member would itself be the use-after-free and would
// make the evidence circular, so the store writes here instead: a namespace-scope
// object whose address is a link-time constant and needs no `this`.
// ---------------------------------------------------------------------------
namespace obs {

int readFileCalls = 0;               // ADVANCEMENT / abort is measured here
bool resumedAfterNestedLoop = false; // did readFile get to run its own tail?
bool dialogAliveOnResume = false;    // ...and was the dialog still there?
bool storeDestroyed = false;
bool storeClosed = false;         // TEST-073 — close() BEFORE delete, or not at all
QByteArray* lastBuffer = nullptr; // the buffer syncNext preallocated

// TEST-075 (DEC-garmin-026) — the construction-time route. open() is the store
// call the CONSTRUCTOR used to make (GarminConnect::blockingRestore). openResumed
// records that open()'s own nested loop ran to completion; the store is leaked on
// the DEC-025 decline path, so open() is always safe to resume even when the
// dialog it was building has been destroyed inside that loop.
int openCalls = 0;
bool openResumed = false;

// TEST-079 / TEST-080 (DEC-garmin-029, REQ-019) — the UPLOAD dialog's own
// blocking call. CloudServiceUploadDialog is the subject; see the ORCH-008 note
// on those slots for why it lives in a file named after GarminConnect.
int writeFileCalls = 0;
bool writeResumed = false;              // writeFile ran its tail after the nested loop
const void* lastUploadBuffer = nullptr; // the dialog MEMBER QByteArray it was handed
bool teardownSawModal = false;          // the teardown landed while a modal box was up
bool teardownFired = false;             // ...and it fired at all
bool startReturned = false;             // has start() handed control back yet?
bool teardownAfterStart = false;        // ...had it, when the teardown landed?

void reset()
{
    readFileCalls = 0;
    resumedAfterNestedLoop = false;
    dialogAliveOnResume = false;
    storeDestroyed = false;
    storeClosed = false;
    lastBuffer = nullptr;
    openCalls = 0;
    openResumed = false;
    writeFileCalls = 0;
    writeResumed = false;
    lastUploadBuffer = nullptr;
    teardownSawModal = false;
    teardownFired = false;
    startReturned = false;
    teardownAfterStart = false;
}

} // namespace obs

// ---------------------------------------------------------------------------
// BlockingStore — GarminConnect's SHAPE, reduced to the part that bites.
//
// GarminConnect::readFile calls blockingDownload(), which builds a local
// QEventLoop, arms a timeout and calls loop.exec(); when that returns, readFile
// goes on to call downloadResultStillWanted(), recordImport() and
// postReadComplete() — all member calls on `this`. This class does exactly that:
// nested loop, then touch `this`.
// ---------------------------------------------------------------------------
class BlockingStore : public CloudService
{
    Q_OBJECT

  public:
    explicit BlockingStore(Context* context) : CloudService(context) {}
    ~BlockingStore() override { obs::storeDestroyed = true; }

    CloudService* clone(Context* context) override { return new BlockingStore(context); }
    QString id() const override { return QStringLiteral("BlockingStore"); }
    QString uiName() const override { return QStringLiteral("Blocking Store"); }
    QImage logo() const override { return QImage(); }
    // The stock capability set: the dialog only builds its Upload tab, and so
    // only has a valid tab index 2 (sync), for a service that advertises Upload.
    // GarminConnect and every sibling do.
    int capabilities() const override { return OAuth | Upload | Download | Query; }
    QString home() override { return QStringLiteral("/"); }

    // GarminConnect::open() is blockingRestore() — a real nested QEventLoop, and
    // the ONE such call the CONSTRUCTOR used to make (CloudService.cpp:725,
    // pre-DEC-026). With blockInOpen set this store reproduces that: it spins a
    // nested loop inside open() and fires the teardown from inside it, exactly as
    // readFile/readdir do for the post-construction routes. Off by default, so
    // TEST-070..073 and start()'s own open() simply return true.
    bool open(QStringList&) override
    {
        ++obs::openCalls;
        if (blockInOpen) {
            fireActionThenBlock();

            // open() RESUMES. This runs on the STORE, which is deliberately
            // LEAKED on the parent-teardown decline path (DEC-025), so it is safe
            // even when the half-built dialog was destroyed inside the loop above
            // — that is the whole point of moving open() out of the constructor.
            obs::openResumed = true;
            if (canary_ != kCanary)
                return false;
        }
        // TEST-077 (A3-R026-F2) — an open that FAILS. start()'s open-failure
        // branch (CloudService.cpp:766-787) runs TWO more nested loops after this
        // returns — QMessageBox::exec() (:777) and QApplication::processEvents()
        // (:781) — each with its own self.isNull() bail (:778, :782). Today's
        // fixture never reaches them because open() always succeeds. Off by
        // default so TEST-070..075 and start()'s own open() are unchanged.
        if (openFailMode)
            return false;
        return true;
    }
    bool close() override
    {
        obs::storeClosed = true;
        return true;
    }

    // The user's action, delivered from INSIDE a nested loop, then the loop.
    //
    // The action is QUEUED, so it is delivered by that loop through
    // QCoreApplication::notifyInternal2 — which is what makes its DeferredDelete
    // outrank the loop. It fires once, on the first blocking call that reaches
    // here.
    //
    // The context object is the DIALOG for the self-close routes (TEST-070/071),
    // because that is who the click lands on. TEST-072 overrides it with qApp:
    // the athlete window's teardown is initiated from OUTSIDE the dialog, and
    // posting it to the dialog would be asking Qt to deliver an event to an
    // object that the event itself destroys.
    void fireActionThenBlock()
    {
        if (closeAction) {
            std::function<void()> action = closeAction;
            closeAction = nullptr;
            QObject* target = closeActionContext ? closeActionContext : static_cast<QObject*>(dialog);
            QMetaObject::invokeMethod(target, action, Qt::QueuedConnection);
        }

        // blockingDownload() / blockingList(): a nested event loop that keeps
        // running until the transfer finishes, i.e. well past the user's click.
        QEventLoop loop;
        QTimer::singleShot(blockingMs, &loop, &QEventLoop::quit);
        loop.exec();
    }

    QList<CloudServiceEntry*> readdir(QString, QStringList&, QDateTime, QDateTime) override
    {
        // GarminConnect::readdir runs blockingList — a nested QEventLoop, just as
        // readFile runs blockingDownload — so the SAME parent teardown lands here
        // too, and it is refreshClicked rather than syncNext that resumes onto a
        // destroyed dialog. Off by default: TEST-070/071, and the refresh the
        // dialog's own constructor performs, want a readdir that simply returns.
        if (blockInReaddir) {
            fireActionThenBlock();

            // readdir RESUMES. newCloudServiceEntry() below is a member call on
            // `this`; if the dialog was destroyed inside the loop and took this
            // store with it, that is the use-after-free.
            obs::resumedAfterNestedLoop = true;
            obs::dialogAliveOnResume = !dialogGuard.isNull();
            if (canary_ != kCanary)
                return QList<CloudServiceEntry*>();
        }

        QList<CloudServiceEntry*> out;
        for (const QString& name : entryNames) {
            CloudServiceEntry* e = newCloudServiceEntry();
            e->name = name;
            e->id = name;
            e->isDir = false;
            e->size = 0;
            e->modified = QDateTime::currentDateTime();
            out << e;
        }
        return out;
    }

    bool readFile(QByteArray* data, QString remotename, QString /*remoteid*/) override
    {
        ++obs::readFileCalls;
        obs::lastBuffer = data;

        // ---- the user's click, then blockingDownload().
        fireActionThenBlock();

        // ---- readFile RESUMES. From here on every line is a member access on
        //      `this`; if the dialog was destroyed inside loop.exec() it took
        //      this store with it (closeAndDeleteStore) and this is the
        //      use-after-free. ASan reports it here.
        obs::resumedAfterNestedLoop = true;
        obs::dialogAliveOnResume = !dialogGuard.isNull();

        // A member READ, so the ASan report names this function and this line
        // rather than something further downstream.
        if (canary_ != kCanary)
            return false;

        // GarminConnect posts its completion deferred (m_completionContext), so
        // do the same: the dialog observes it in the processEvents() syncNext
        // runs immediately after readFile returns.
        QMetaObject::invokeMethod(
            this, [this, data, remotename]() { notifyReadComplete(data, remotename, tr("Completed.")); },
            Qt::QueuedConnection);
        return true;
    }

    // REQ-019 / DEC-garmin-029 — the UPLOAD path's store call.
    //
    // CloudServiceUploadDialog::start() calls compressRide() and then writeFile()
    // (CloudService.cpp:386/:389) and lands the result in `status`. writeFile is
    // virtual and every service implements it over the network; DEC-029 names it
    // as one of the four suspension points a parent teardown can land in, so this
    // stub does what readFile already does for the download path: run a nested
    // QEventLoop, then resume and touch `this`.
    //
    // The base-class writeFile returns false, which would send the dialog down
    // its upload-failure branch, so this override also gives the happy path a
    // writeFile that SUCCEEDS and (optionally) notifies completion afterwards —
    // that notification is what the dialog's exec() sits waiting for.
    bool writeFile(QByteArray& data, QString remotename, RideFile* ride) override
    {
        Q_UNUSED(ride);
        ++obs::writeFileCalls;
        obs::lastUploadBuffer = &data;

        if (blockInWrite) {
            fireActionThenBlock();

            // writeFile RESUMES. The STORE is still alive here whatever happens —
            // MainWindow::uploadCloud owns it and closes+deletes it only after
            // upload() returns (REQ-017, MainWindow.cpp:2563) — so this tail is
            // safe. `data` is NOT: it is a MEMBER of the dialog the teardown may
            // just have destroyed, so nothing past this point may touch it.
            obs::writeResumed = true;
            if (canary_ != kCanary)
                return false;
        }

        if (!writeSucceeds)
            return false;

        if (completeWrite)
            QMetaObject::invokeMethod(
                this, [this, remotename]() { notifyWriteComplete(remotename, tr("Completed.")); },
                Qt::QueuedConnection);
        return true;
    }

    QStringList entryNames;
    QDialog* dialog = nullptr;             // where the close is sent
    std::function<void()> closeAction;     // what the user does, inside the loop
    QObject* closeActionContext = nullptr; // who it is delivered to (default: dialog)
    bool blockInReaddir = false;           // does readdir run a nested loop too?
    bool blockInOpen = false;              // does open() run a nested loop too? (TEST-075)
    bool openFailMode = false;             // does open() FAIL, taking start()'s open-failure branch? (TEST-077)
    bool blockInWrite = false;             // does writeFile run a nested loop? (TEST-079)
    bool writeSucceeds = true;             // ...and does it report the upload started?
    bool completeWrite = false;            // ...and does writeComplete ever arrive? (TEST-080)
    QPointer<QDialog> dialogGuard;
    int blockingMs = 120;

  private:
    static const int kCanary = 0x5A5A5A;
    int canary_ = kCanary;
};

// ---------------------------------------------------------------------------

class TestGarminConnectSyncDialogClose : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir athleteRoot;
    Context* context = nullptr;
    Athlete* athlete = nullptr;
    RideCache* rideCache = nullptr;

    // Three activities dated today, so they fall inside the dialog's default
    // from/to window and parse as GC ride filenames.
    static QStringList threeActivities()
    {
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        return QStringList() << day + QStringLiteral("_10_00_00.fit") << day + QStringLiteral("_11_00_00.fit")
                             << day + QStringLiteral("_12_00_00.fit");
    }

    // What one run of the scenario leaves behind, all of it observable after
    // both the dialog and the store are gone.
    struct Outcome
    {
        bool dialogSurvivedBlockingCall = false; // no UAF: it was still there on resume
        bool storeSurvivedBlockingCall = false;  // ditto, and readFile ran its tail
        bool dialogGoneAtEnd = false;            // no leak: it really did close
        bool storeGoneAtEnd = false;             // ...and took the store with it
        int readFileCalls = 0;                   // did the sync keep going?
        bool inFlightBufferFreed = false;        // A3-R017-F3
    };

    // Drives the real production sequence at realistic event-loop levels:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event DELIVERY, so scopeLevel is bumped, as it is when
    //        the user activates a menu item or the wizard finishes)
    //          -> new CloudServiceSyncDialog + WA_DeleteOnClose   [AddCloudWizard]
    //          -> refresh / select all / Download
    //               -> syncNext -> store->readFile -> NESTED loop
    //                    -> queued `closeAction` on the dialog
    //
    // Nothing here deletes the dialog: the dialog is supposed to delete itself.
    //
    // `duringBlockingCall` is what the user does WHILE the store is blocked; it
    // is handed the dialog so a scenario can drive any of the three ways out (the
    // window X, the Cancel button, the Escape key), which take three genuinely
    // different routes through Qt and are not interchangeable.
    //
    // `afterTheSync` is the CONTROL's user action: the same click, but once the
    // sync has finished and no guard is held.
    Outcome
    run(std::function<void(CloudServiceSyncDialog*)> duringBlockingCall,
        std::function<void(CloudServiceSyncDialog*)> afterTheSync = std::function<void(CloudServiceSyncDialog*)>(),
        int blockingMs = 120)
    {
        obs::reset();

        Outcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;

        QEventLoop appLoop; // the application's event loop

        QMetaObject::invokeMethod(
            this,
            [&]() {
                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                store->blockingMs = blockingMs;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                // VERBATIM the production line this slice must not undo
                // (AddCloudWizard.cpp:899) - the dialog is modeless and owns
                // itself.
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                // DEC-garmin-026 two-phase init: the constructor now builds only
                // the shell, so start() is what opens the store and builds the
                // widgets these routes then drive. blockInOpen is off here, so it
                // simply returns true - the DEC-024/025 routes under test are
                // unchanged (this is the production AddCloudWizard sequence:
                // construct, setAttribute, start(), open()).
                dialog->start();
                dialog->open();

                dialogGuard = dialog;
                storeGuard = store;
                store->dialog = dialog;
                store->dialogGuard = dialog;
                if (duringBlockingCall)
                    store->closeAction = [duringBlockingCall, dialog]() { duringBlockingCall(dialog); };

                dialog->refreshClicked();
                dialog->selectAllSyncChanged(Qt::Checked);
                dialog->downloadClicked();

                // The CONTROL's click, once every blocking frame has unwound.
                // `dialog` is the context object, so if the dialog somehow went
                // away first this simply does not fire (rather than crashing and
                // muddying the verdict).
                if (afterTheSync)
                    QTimer::singleShot(250, dialog, [afterTheSync, dialog]() { afterTheSync(dialog); });

                // Give the deferred close, the queued completions and the
                // DeferredDelete every chance to land before we look.
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        // Anything still queued at this point (the DeferredDelete is posted from
        // inside event delivery, so it is delivered once that scope unwinds).
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.dialogSurvivedBlockingCall = obs::dialogAliveOnResume;
        out.storeSurvivedBlockingCall = obs::resumedAfterNestedLoop;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeGoneAtEnd = storeGuard.isNull() && obs::storeDestroyed;
        out.readFileCalls = obs::readFileCalls;

        // A3-R017-F3 — was the buffer syncNext preallocated actually released?
        // Asked of the allocator itself: ASan poisons a block on free and holds
        // it in quarantine, so a poisoned address is a freed one and an
        // addressable address is a leaked one. (A double free would have aborted
        // the process long before this line.)
        out.inFlightBufferFreed =
            obs::lastBuffer != nullptr && __asan_region_is_poisoned(obs::lastBuffer, sizeof(QByteArray)) != nullptr;

        return out;
    }

    // =====================================================================
    // DEC-garmin-025 — what ONE parent-teardown run leaves behind.
    // =====================================================================
    struct TeardownOutcome
    {
        bool storeSurvivedBlockingCall = false; // readFile ran its tail on a live store
        bool dialogGoneOnResume = false;        // the teardown really did land mid-call
        bool dialogGoneAtEnd = false;           // ...and really did destroy the dialog
        bool storeDestroyed = false;            // was closeAndDeleteStore performed?
        bool storeClosed = false;               // ...and was close() called first?
        int readFileCalls = 0;                  // did the sync stop when its owner died?
    };

    // THE ROUTE DEC-024's GATE CANNOT SEE.
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped)
    //          -> owner QWidget                                   [MainWindow]
    //          -> CloudServiceSyncDialog, made a CHILD of that owner
    //          -> refresh / select all / Download
    //               -> syncNext -> store->readFile -> NESTED loop
    //                    -> queued `delete owner` ON qApp   [the athlete window
    //                       closing; WA_DeleteOnClose, MainWindow.cpp:143]
    //                         -> ~QWidget -> ~QObject destroys its children
    //                              -> ~CloudServiceSyncDialog, with a nested
    //                                 QEventLoop still on the stack
    //
    // No close(), no done(), no closeEvent() — nothing DEC-024 gates is on this
    // path at all. `whenBusy` is what makes the difference between TEST-072 (tear
    // the owner down mid-call) and TEST-073 (tear it down once the sync is over).
    //
    // `duringReaddir` picks WHICH blocking call the teardown lands in, because
    // the frame that resumes is a different one: readFile leaves syncNext
    // suspended, readdir leaves refreshClicked suspended — and refreshClicked
    // resumes into a member WRITE (`workouts = ...`), which syncNext does not.
    TeardownOutcome runParentTeardown(bool whenBusy, bool duringReaddir = false)
    {
        obs::reset();

        TeardownOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;

        QEventLoop appLoop; // the application's event loop

        QMetaObject::invokeMethod(
            this,
            [&]() {
                // The owning window. A plain QWidget, because the only property
                // of MainWindow that matters here is that Qt destroys its
                // children when it goes.
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                // Busy: each read blocks long past the teardown. Idle: the whole
                // sync is over long before it.
                store->blockingMs = whenBusy ? 120 : 5;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                // In production the dialog parents ITSELF, to context->mainWindow
                // (CloudService.cpp:709). This test Context has no MainWindow, so
                // the identical parent-child link is made explicitly here - that
                // link is the entire subject of TEST-072.
                dialog->setParent(owner, Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                // DEC-garmin-026 two-phase init - build the widgets these routes
                // drive. blockInOpen/blockInReaddir are still off here, so
                // start()'s own open() and initial refresh do not block.
                dialog->start();
                dialog->open();

                dialogGuard = dialog;
                storeGuard = store;
                store->dialog = dialog;
                store->dialogGuard = dialog;
                store->closeActionContext = qApp;

                if (whenBusy)
                    store->closeAction = [owner]() { delete owner; };
                // Armed only NOW, so that the refresh the dialog's constructor
                // runs (CloudService.cpp:972) is not the one that blocks: a
                // teardown landing inside the constructor would be destroying a
                // half-built object, which nothing at this layer could survive.
                store->blockInReaddir = duringReaddir;

                // The dialog can be GONE when refreshClicked returns - this
                // harness has to stand down for the same reason the production
                // code does.
                QPointer<CloudServiceSyncDialog> alive(dialog);
                dialog->refreshClicked();
                if (!alive.isNull()) {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    dialog->downloadClicked();
                }

                // TEST-073's teardown: same `delete owner`, but with every
                // blocking frame long since unwound.
                if (!whenBusy)
                    QTimer::singleShot(250, qApp, [owner]() { delete owner; });

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.storeSurvivedBlockingCall = obs::resumedAfterNestedLoop;
        out.dialogGoneOnResume = !obs::dialogAliveOnResume;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        out.storeClosed = obs::storeClosed;
        out.readFileCalls = obs::readFileCalls;
        return out;
    }

    // =====================================================================
    // DEC-garmin-026 (A3-R025-F1) — what ONE construction-time teardown run
    // leaves behind.
    // =====================================================================
    struct CtorTeardownOutcome
    {
        bool openRan = false;            // the (formerly constructor-time) open() was reached
        bool openResumed = false;        // ...and its nested loop ran to completion
        bool dialogGoneAtEnd = false;    // the teardown really did destroy the dialog
        bool startReturnedFalse = false; // start() bailed instead of building on a corpse
        bool storeDestroyed = false;     // (must be false: declined + leaked, DEC-025)
        bool storeClosed = false;        // (must be false: the call is still on the stack)
        int readFileCalls = 0;           // (must be 0: start() never reached the sync)
    };

    // THE ROUTE A3-R025-F1 FOUND — a teardown that lands inside the store->open()
    // the CONSTRUCTOR used to make (GarminConnect::blockingRestore).
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, scopeLevel bumped)
    //          -> owner QWidget                                   [MainWindow]
    //          -> CloudServiceSyncDialog(ctx-with-owner-as-mainWindow, store)
    //               [PHASE ONE: shell only, no nested loop, cannot be caught here]
    //          -> dialog->start()                                 [AddCloudWizard]
    //               -> store->open() -> NESTED loop
    //                    -> queued `delete owner` ON qApp
    //                         -> ~QWidget -> ~QObject destroys children
    //                              -> ~CloudServiceSyncDialog, with start()'s
    //                                 BlockingCall still on the stack
    //
    // Pre-DEC-026 that open() ran in the CONSTRUCTOR, so this teardown destroyed a
    // half-built dialog under its own constructor and the constructor resumed
    // writing members into freed memory: an ASan heap-use-after-free that NO gate
    // could veto (the constructor frame is unreachable by DEC-024's
    // done()/closeEvent(), and its BlockingCall could not carry a self-bail on a
    // not-yet-constructed `this`). After the split open() runs in start() on a
    // COMPLETE object, so DEC-025's destructor decline + BlockingCall QPointer
    // apply exactly as they do for readFile/readdir.
    //
    // The dialog must be a CHILD of `owner` AT CONSTRUCTION TIME — the teardown
    // lands inside a call the constructor makes, before any post-construction
    // setParent() could run — so this Context is handed `owner` as its mainWindow,
    // exactly as the production dialog parents itself to context->mainWindow
    // (CloudService.cpp:709). Only QWidget-level parenting is exercised on it.
    // This is also what lets the both-directions mutation (open() moved back into
    // the constructor) reproduce the original UAF: the child link is already in
    // place while the constructor blocks.
    CtorTeardownOutcome runCtorTeardown()
    {
        obs::reset();

        CtorTeardownOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;

        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                // Arm the construction-time route: open() spins a nested loop and
                // tears `owner` (and therefore its child, the dialog) down from
                // inside it. Set BEFORE construction, so it is live whether open()
                // runs in the constructor (mutated/RED) or in start() (GREEN).
                store->blockInOpen = true;
                store->closeActionContext = qApp;
                store->closeAction = [owner]() { delete owner; };

                // A Context whose mainWindow IS the owner, so the constructor
                // parents the dialog to it. Only its QWidget-level parenting is
                // used before the teardown lands (see method comment).
                Context ctorCtx(reinterpret_cast<MainWindow*>(owner));
                ctorCtx.athlete = athlete;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(&ctorCtx, store);
                dialog->setAttribute(Qt::WA_DeleteOnClose);

                dialogGuard = dialog;
                storeGuard = store;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // PHASE TWO. open()'s nested loop deletes owner -> destroys this
                // dialog mid-call; start() must bail via its QPointer instead of
                // building tabs on a corpse. Reading start()'s bool is safe even
                // though `dialog` is now dangling (the value is in a register);
                // dereferencing `dialog` past here would not be, so we do not.
                bool started = dialog->start();
                out.startReturnedFalse = (started == false);

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.openRan = obs::openCalls > 0;
        out.openResumed = obs::openResumed;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        out.storeClosed = obs::storeClosed;
        out.readFileCalls = obs::readFileCalls;
        return out;
    }

    // What the MODAL run leaves behind.
    struct ModalOutcome
    {
        bool timedOut = false;             // exec() never came back
        int readFileCalls = 0;             // did Cancel stop the sync?
        bool storeGoneAfterDelete = false; // the owner contract still holds
    };

    // THE OTHER PRODUCTION CALLER. MainWindow::syncCloud builds this dialog on
    // the STACK and runs it modally with exec(); it has no WA_DeleteOnClose and
    // so never had the use-after-free. But DEC-024 gates QDialog::done(), and
    // done() is precisely what exec() is waiting for — so a gate that failed to
    // reopen would HANG File > Sync instead of crashing it. That is the failure
    // mode this drives out.
    ModalOutcome runModalCancel()
    {
        obs::reset();

        ModalOutcome out;
        QPointer<CloudService> storeGuard;

        BlockingStore* store = new BlockingStore(context);
        store->entryNames = threeActivities();
        storeGuard = store;

        CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
        store->dialog = dialog;
        store->dialogGuard = dialog;
        store->closeAction = [dialog]() { dialog->cancelClicked(); };

        // DEC-garmin-026 two-phase init - build the widgets before exec() drives
        // them. blockInOpen is off, so this just opens and returns true.
        dialog->start();

        // Runs once exec()'s own event loop is up.
        QMetaObject::invokeMethod(
            dialog,
            [dialog]() {
                dialog->refreshClicked();
                dialog->selectAllSyncChanged(Qt::Checked);
                dialog->downloadClicked();
            },
            Qt::QueuedConnection);

        // If the gate never reopens, exec() never returns. Unwind every loop
        // rather than hanging the whole suite on a timeout, and record why.
        bool* timedOut = &out.timedOut;
        QTimer::singleShot(5000, dialog, [timedOut]() {
            *timedOut = true;
            QCoreApplication::exit(1);
        });

        dialog->exec();

        out.readFileCalls = obs::readFileCalls;

        delete dialog; // MainWindow's stack object going out of scope
        out.storeGoneAfterDelete = storeGuard.isNull() && obs::storeDestroyed;
        return out;
    }

    // The shared verdict for TEST-070's two routes. NOT a slot - QtTest would
    // run it as a test case of its own.
    void assertSurvivedThenClosed(const char* route, const Outcome& out)
    {
        QVERIFY2(out.storeSurvivedBlockingCall,
                 qPrintable(QStringLiteral("%1: readFile never resumed after its nested event loop - the "
                                           "store was destroyed mid-call")
                                .arg(QLatin1String(route))));
        QVERIFY2(out.dialogSurvivedBlockingCall,
                 qPrintable(QStringLiteral("%1: the dialog was destroyed inside the store's nested event loop")
                                .arg(QLatin1String(route))));

        // ...and the user's close was not simply swallowed: the dialog is gone,
        // and it took the store it owns with it.
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the deferred close was lost - the dialog is still alive, and "
                                           "leaked")
                                .arg(QLatin1String(route))));
        QVERIFY2(out.storeGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the dialog went away without closing and deleting the store "
                                           "it owns")
                                .arg(QLatin1String(route))));

        // The close is an abort: the sync must not carry on downloading the
        // remaining two activities behind a dialog the user has dismissed.
        QCOMPARE(out.readFileCalls, 1);

        // The buffer that was in flight when the user closed is not leaked.
        QVERIFY2(
            out.inFlightBufferFreed,
            qPrintable(
                QStringLiteral("%1: the in-flight QByteArray was never freed (A3-R017-F3)").arg(QLatin1String(route))));
    }

    // The shared verdict for TEST-072's two routes (a teardown inside readFile,
    // and one inside readdir). NOT a slot.
    void assertDeclinedRatherThanCrashed(const char* call, const TeardownOutcome& out)
    {
        // The route actually executed: the dialog was already gone by the time
        // the store call resumed. Without this, everything below could pass on a
        // run where the teardown never landed inside the nested loop at all.
        QVERIFY2(out.dialogGoneOnResume,
                 qPrintable(QStringLiteral("%1: the parent teardown did not land inside the nested event loop - "
                                           "this run proves nothing")
                                .arg(QLatin1String(call))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: deleting the parent did not destroy the dialog - the test's "
                                           "premise is wrong")
                                .arg(QLatin1String(call))));

        // Half one: the STORE outlived the call being made on it.
        QVERIFY2(out.storeSurvivedBlockingCall,
                 qPrintable(QStringLiteral("%1: the call never resumed after its nested event loop - the parent "
                                           "teardown deleted the store mid-call (A3-R017b-F1)")
                                .arg(QLatin1String(call))));

        // DEC-025: while a blocking call is in flight the deletion is DECLINED -
        // not deferred, not performed. The store is deliberately leaked, and
        // close() must not have been called on it either, since close() is what
        // tears down the worker thread the call is still using.
        QVERIFY2(!out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the destructor deleted the store while a blocking call was still "
                                           "on the stack (DEC-025 requires the deletion to be DECLINED)")
                                .arg(QLatin1String(call))));
        QVERIFY2(!out.storeClosed,
                 qPrintable(QStringLiteral("%1: the destructor closed the store while a blocking call was still "
                                           "executing on it")
                                .arg(QLatin1String(call))));
    }

  private slots:

    void initTestCase()
    {
        QVERIFY(athleteRoot.isValid());
        athlete = new Athlete(nullptr, QDir(athleteRoot.path()));
        context = new Context(nullptr);
        context->athlete = athlete;
        rideCache = new RideCache(context);
        athlete->rideCache = rideCache; // the dialog walks it; it stays empty
    }

    void cleanupTestCase()
    {
        athlete->rideCache = nullptr;
        delete rideCache;
        rideCache = nullptr;
        delete context;
        context = nullptr;
        delete athlete;
        athlete = nullptr;
    }

    // =====================================================================
    // TEST-070 — closing the dialog DURING a blocking store call must not
    // destroy the store (or the dialog) out from under that call, and must
    // still close the dialog once the call is done.
    // =====================================================================
    //
    // RED (before DEC-024): the process aborts inside this function with
    //   AddressSanitizer: heap-use-after-free ... in BlockingStore::readFile
    // because ~CloudServiceSyncDialog ran inside loop.exec() and its
    // closeAndDeleteStore() deleted the store readFile is still running on.
    //
    // The assertions below are the OUTCOME, not the flag: the store finished its
    // call, the dialog was still there while it did, and afterwards BOTH are
    // gone — i.e. the crash is closed without re-opening the leak that
    // WA_DeleteOnClose was added to close.
    void closingDuringABlockingStoreCallNeitherCrashesNorLeaks()
    {
        // The window X.
        assertSurvivedThenClosed("window close", run([](CloudServiceSyncDialog* d) { d->close(); }));
        if (QTest::currentTestFailed())
            return;

        // The Escape key. A DIFFERENT route with the same consequence: Qt turns
        // it into QDialog::reject() -> done(), which honours WA_DeleteOnClose
        // whether or not closeEvent() accepted the close. Gating closeEvent()
        // alone leaves this one lethal, which is why it is here and not assumed.
        assertSurvivedThenClosed("escape key", run([](CloudServiceSyncDialog* d) {
                                     QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
                                     QApplication::sendEvent(d, &escape);
                                 }));
        if (QTest::currentTestFailed())
            return;

        // THE WEDGE CONTROL. A guard that can be left raised - by an early
        // return, or by a bool that an inner frame cleared while an outer one was
        // still live - would make this dialog permanently un-closable, which is a
        // worse bug than the crash. So: nobody closes anything during a blocking
        // call, the whole sync runs to completion, and only THEN does the user
        // close. It must go FIRST TIME, and the sync must have run undisturbed
        // (nothing was spuriously aborted).
        const Outcome control = run(
            std::function<void(CloudServiceSyncDialog*)>(), [](CloudServiceSyncDialog* d) { d->close(); },
            /*blockingMs*/ 5);

        QCOMPARE(control.readFileCalls, 3);
        QVERIFY2(control.dialogGoneAtEnd,
                 "closing when NO store call is in flight was refused - the guard has wedged the dialog shut");
        QVERIFY2(control.storeGoneAtEnd, "the dialog closed without closing and deleting the store it owns");
    }

    // =====================================================================
    // TEST-071 — the Cancel button mid-blocking-call is DEFERRED, not
    // immediate.
    // =====================================================================
    //
    // cancelClicked() used to be an unconditional reject(), so the ordinary
    // Cancel button reached the same destruction path as the window X - and on
    // Qt, reject() destroys a WA_DeleteOnClose dialog even if closeEvent()
    // ignores the close, so gating closeEvent() alone does not cover this.
    //
    // Same three properties as TEST-070, plus the one that makes deferring
    // honest rather than a shrug: the click has to STOP the sync, otherwise the
    // user's Cancel looks like it did nothing while activity 2 and 3 keep
    // downloading.
    void cancelDuringABlockingStoreCallIsDeferredAndStopsTheSync()
    {
        const Outcome out = run([](CloudServiceSyncDialog* d) { d->cancelClicked(); });

        QVERIFY2(out.storeSurvivedBlockingCall,
                 "readFile never resumed after its nested event loop - the store was destroyed mid-call");
        QVERIFY2(out.dialogSurvivedBlockingCall, "Cancel destroyed the dialog inside the store's nested event loop");

        // THE SYNC STOPPED. Three activities were selected and only the first was
        // ever asked for: `aborted` took effect. If Cancel merely deferred the
        // close without aborting, this is 3.
        QCOMPARE(out.readFileCalls, 1);

        // ...and the deferred close was honoured once the frame unwound, taking
        // the store with it. No leak.
        QVERIFY2(out.dialogGoneAtEnd, "the deferred Cancel was lost - the dialog is still alive, and leaked");
        QVERIFY2(out.storeGoneAtEnd, "the dialog went away without closing and deleting the store it owns");

        // A3-R017-F3: completedRead's abort branch used to return without
        // deleting the caller's buffer, so aborting leaked it. Aborting is
        // exactly what this test does.
        QVERIFY2(out.inFlightBufferFreed, "aborting leaked the in-flight QByteArray (A3-R017-F3)");

        // The MODAL shape of the same click (MainWindow::syncCloud). Deferring
        // done() must delay exec()'s return, never prevent it.
        const ModalOutcome modal = runModalCancel();

        QVERIFY2(!modal.timedOut, "exec() never returned - the deferred Cancel hung the modal sync dialog");
        QCOMPARE(modal.readFileCalls, 1);
        QVERIFY2(modal.storeGoneAfterDelete, "the modal dialog did not close and delete the store it owns");
    }

    // =====================================================================
    // TEST-072 — DESTROYING THE PARENT during a blocking store call must not
    // free the store, or the dialog, out from under the frames suspended in
    // that call.
    // =====================================================================
    //
    // REQ-017 (e), verbatim: "an owner that opens a `CloudService` closes and
    // destroys it — `CloudServiceSyncDialog` (CloudService.h:336, no dtor) and
    // `MainWindow::syncCloud` (MainWindow.cpp:2561-2566) no longer leak their
    // store, worker thread and interpreter session past dialog close
    // [A3-R012-F12 fix]."
    //
    // A3-R017b-F1: the destructor that clause added is reached by a SECOND route
    // - parent teardown - on which DEC-024's done()/closeEvent() gate is not even
    // consulted, because Qt destroys children directly from ~QObject.
    //
    // RED, both halves, each reproduced by reverting one half of DEC-025:
    //   * restore the unconditional closeAndDeleteStore() in
    //     ~CloudServiceSyncDialog -> AddressSanitizer: heap-use-after-free
    //     READ in BlockingStore::readFile (the canary, after loop.exec()).
    //   * keep that fix but drop the QPointer half -> AddressSanitizer:
    //     heap-use-after-free WRITE in
    //     CloudServiceSyncDialog::BlockingCall::~BlockingCall
    //     (--dialog->blockingCallDepth, CloudService.cpp:1011).
    //
    // DEC-025's bar is deliberately NOT "the store is destroyed": on this path
    // the store is LEAKED on purpose. That is exactly the pre-REQ-017 behaviour
    // here, and it is the accepted price of not corrupting memory - so the leak
    // is ASSERTED, not tolerated, to keep it a decision rather than a drift.
    void destroyingTheParentDuringABlockingStoreCallDoesNotFreeWhatIsStillInUse()
    {
        // ROUTE 1 - the teardown lands in readFile (blockingDownload), leaving
        // syncNext suspended.
        const TeardownOutcome read = runParentTeardown(/*whenBusy*/ true);
        assertDeclinedRatherThanCrashed("readFile", read);
        if (QTest::currentTestFailed())
            return;

        // ...and the dialog's own suspended frames stood down instead of walking
        // members of a destroyed object. Had syncNext resumed regardless, it
        // would have gone on to activities 2 and 3.
        QCOMPARE(read.readFileCalls, 1);

        // ROUTE 2 - the teardown lands in readdir (blockingList), leaving
        // refreshClicked suspended. A DIFFERENT frame and a sharper one: it
        // resumes into `workouts = store->readdir(...)`, a member WRITE that
        // happens after the call returns but before any guard placed after the
        // BlockingCall scope could run.
        const TeardownOutcome list = runParentTeardown(/*whenBusy*/ true, /*duringReaddir*/ true);
        assertDeclinedRatherThanCrashed("readdir", list);
        if (QTest::currentTestFailed())
            return;

        // refreshClicked never got as far as populating the list, so no download
        // was ever attempted.
        QCOMPARE(list.readFileCalls, 0);
    }

    // =====================================================================
    // TEST-073 — THE POSITIVE CONTROL. Parent teardown with NO blocking call in
    // flight must still close and delete the store.
    // =====================================================================
    //
    // TEST-072 is satisfiable by a destructor that never deletes anything, which
    // would silently re-open the very leak REQ-017 (e) exists to close and
    // regress DEC-024/TEST-064. So: the identical `delete owner`, the identical
    // parent-child link, the only difference being that the sync has finished.
    // The store must be CLOSED and then DELETED, exactly as closeAndDeleteStore
    // has always done - that helper is untouched by DEC-025; only whether it is
    // CALLED is conditional.
    void destroyingTheParentWhenIdleStillClosesAndDeletesTheStore()
    {
        const TeardownOutcome out = runParentTeardown(/*whenBusy*/ false);

        // The sync ran undisturbed - nothing was spuriously aborted, and the
        // teardown really did happen after the last blocking frame unwound.
        QCOMPARE(out.readFileCalls, 3);
        QVERIFY2(out.dialogGoneAtEnd, "deleting the parent did not destroy the dialog");

        QVERIFY2(out.storeClosed,
                 "the store was not close()d on parent teardown - the worker thread and interpreter session "
                 "outlive the window again (REQ-017 (e))");
        QVERIFY2(out.storeDestroyed,
                 "the store was not deleted on parent teardown - the busy guard has swallowed the ordinary case "
                 "and re-opened the leak");
    }

    // =====================================================================
    // TEST-075 — DEC-garmin-026 (A3-R025-F1). The CONSTRUCTOR route.
    // =====================================================================
    //
    // The blocking finding: the dialog's constructor ran store->open() (a nested
    // QEventLoop) and then wrote `this` members, so tearing the parent window down
    // mid-construction destroyed the half-built dialog under its own constructor —
    // a heap-use-after-free that no gate could veto, since the constructor frame
    // is reachable by NEITHER DEC-024's done()/closeEvent() NOR a self-bail (there
    // is no fully-constructed `this` to guard).
    //
    // RED (proved by the both-directions mutation, not just asserted): moving
    // store->open()/refreshClicked() back into the constructor body makes this run
    // abort with
    //   AddressSanitizer: heap-use-after-free WRITE in
    //   CloudServiceSyncDialog::CloudServiceSyncDialog (a member store after the
    //   nested loop, on a dialog `delete owner` already destroyed).
    //
    // GREEN: the constructor builds only the shell and start() carries the open().
    // start() runs on a COMPLETE object, so when the teardown lands inside its
    // open() the destructor DECLINES to delete the store (DEC-025) and start()
    // bails via its QPointer instead of touching a member — no UAF, and the store
    // is deliberately leaked exactly as the readFile/readdir routes leak it.
    void tearingTheParentDownDuringConstructionTimeOpenDoesNotUseAFreedDialog()
    {
        const CtorTeardownOutcome out = runCtorTeardown();

        // The route actually executed: open()'s nested loop ran, and the teardown
        // inside it really did destroy the dialog. Without these, everything below
        // could pass on a run where the teardown never landed.
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(out.openResumed,
                 "open()'s nested event loop never resumed - the store was freed mid-open (the very UAF "
                 "DEC-026 removes)");
        QVERIFY2(out.dialogGoneAtEnd, "the parent teardown did not destroy the dialog - the test's premise is wrong");

        // start() stood down instead of building tabs/date-range/tree widgets onto
        // a destroyed `this`. If the split were absent (open() still in the
        // constructor) the process would have aborted under ASan long before here.
        QVERIFY2(out.startReturnedFalse,
                 "start() returned true after `this` was destroyed inside store->open() - it resumed onto a "
                 "freed dialog (A3-R025-F1)");

        // ...and it never got as far as the sync.
        QCOMPARE(out.readFileCalls, 0);

        // DEC-025 on this route too: a blocking call is on the stack, so the
        // destructor DECLINES to delete or close the store. It is leaked on
        // purpose (asserted, not tolerated), which is strictly better than freeing
        // the store the open() call is still running on.
        QVERIFY2(!out.storeDestroyed,
                 "the destructor deleted the store while store->open() was still on the stack (DEC-025 requires "
                 "the deletion to be DECLINED)");
        QVERIFY2(!out.storeClosed, "the destructor closed the store while store->open() was still executing on it");
    }

    // =====================================================================
    // TEST-077 (A3-R026-F2) — the FOUR self.isNull() bails DEC-026 added to
    // start() BEYOND the one after store->open() (:764, already load-bearing via
    // TEST-075). Each of :778, :782, :993 and :1023 guards a member access that
    // would otherwise run on a `this` a parent teardown freed inside a nested
    // event loop. They are NOT dead code — they were simply UNREACHED by the
    // fixture (open() always succeeded, the ride cache was empty, the tail
    // refresh never blocked). These helpers steer a teardown into each frame so
    // the guard becomes load-bearing: the mutation matrix in the accompanying
    // report neuters each guard (if(self.isNull())->if(false), snapshot-restore)
    // and confirms the matching slot then aborts under ASan.
    // =====================================================================

  private:
    // Which of start()'s open-failure / dirty-rides nested loops the teardown is
    // steered into.
    enum StartFrame {
        OpenFailExec,          // the open-failure QMessageBox::exec()   (:777) -> guard :778
        OpenFailProcessEvents, // the open-failure processEvents()       (:781) -> guard :782
        DirtyExec              // the unsaved-changes QMessageBox::exec() (:992) -> guard :993
    };

    struct StartBranchOutcome
    {
        bool openRan = false;            // start() reached store->open()
        bool startReturnedFalse = false; // it bailed rather than build on a corpse
        bool dialogGoneAtEnd = false;    // the teardown really destroyed the dialog
        bool storeDestroyed = false;     // (must be false: declined + leaked, DEC-025)
        bool storeClosed = false;        // (must be false: a blocking call is on the stack)
        int readFileCalls = 0;           // (must be 0: start() never reached the sync)
    };

    // Drives a parent teardown into ONE of start()'s open-failure / dirty-rides
    // nested loops.
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, scopeLevel bumped)
    //          -> owner QWidget                                   [MainWindow]
    //          -> CloudServiceSyncDialog, made a CHILD of that owner
    //          -> dialog->start()
    //               -> store->open()  [OpenFail*: returns false, no loop]
    //               -> QMessageBox::exec() / processEvents() -> NESTED loop
    //                    -> the queued `teardown` action lands HERE
    //
    // The teardown is posted to qApp BEFORE start() runs, so it is delivered by
    // the FIRST nested QEventLoop start() spins. With blockInOpen/blockInReaddir
    // off, open() runs no loop, so that first loop is exactly the QMessageBox /
    // processEvents this frame selects.
    StartBranchOutcome runStartBranchTeardown(StartFrame frame)
    {
        obs::reset();

        StartBranchOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;

        // DirtyExec needs one dirty ride so start() reaches its unsaved-changes
        // prompt; the cache is empty on every other route (removed again below so
        // no later slot trips the same prompt).
        RideItem* dirty = nullptr;
        if (frame == DirtyExec) {
            // The 2-arg ctor is the one ImportSeamStubs stubs (the default ctor is
            // not linked); it sets context but not isdirty, so mark it dirty
            // directly (setDirty() would notify through a null-ride context).
            dirty = new RideItem(nullptr, context);
            dirty->isdirty = true;
            rideCache->rides().push_back(dirty);
        }

        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                if (frame == OpenFailExec || frame == OpenFailProcessEvents)
                    store->openFailMode = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                // In production the dialog parents itself to context->mainWindow
                // (CloudService.cpp:727); this test Context has none, so the
                // identical child link is made explicitly (as runParentTeardown
                // does) — the teardown here lands post-construction, so a plain
                // setParent suffices.
                dialog->setParent(owner, Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);

                dialogGuard = dialog;
                storeGuard = store;

                // The teardown, delivered from inside start()'s first nested loop.
                //
                //  OpenFailExec / DirtyExec: delete owner (self -> null) INSIDE the
                //    QMessageBox::exec(), then close the box so exec() returns onto
                //    the guard (:778 / :993).
                //  OpenFailProcessEvents: close the box with the dialog still
                //    ALIVE (so :778 passes), and post the delete so it lands one
                //    frame later, in the QApplication::processEvents() at :781
                //    (guard :782). QEventLoop::exit() interrupts the modal loop the
                //    instant close() requests it, so the freshly-posted delete is
                //    NOT delivered inside exec() but survives to that processEvents.
                std::function<void()> teardown;
                if (frame == OpenFailProcessEvents) {
                    teardown = [owner]() {
                        if (QWidget* m = QApplication::activeModalWidget())
                            m->close();
                        QMetaObject::invokeMethod(qApp, [owner]() { delete owner; }, Qt::QueuedConnection);
                    };
                } else {
                    teardown = [owner]() {
                        delete owner;
                        if (QWidget* m = QApplication::activeModalWidget())
                            m->close();
                    };
                }
                QMetaObject::invokeMethod(qApp, teardown, Qt::QueuedConnection);

                // Reading start()'s bool is safe even after `this` is freed (the
                // value is in a register); dereferencing `dialog` past here is not,
                // so we do not.
                bool started = dialog->start();
                out.startReturnedFalse = (started == false);

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.openRan = obs::openCalls > 0;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        out.storeClosed = obs::storeClosed;
        out.readFileCalls = obs::readFileCalls;

        // The store is leaked on the DEC-025 decline path (asserted below), exactly
        // as TEST-072/075 leak it; detect_leaks=0 tolerates it.
        if (dirty) {
            rideCache->rides().removeAll(dirty);
            delete dirty;
        }
        return out;
    }

    // Shared verdict for the three msgBox/processEvents guard routes. NOT a slot.
    void assertStartBailedNotCrashed(const char* frame, const StartBranchOutcome& out)
    {
        // Reaching this line at all means the process did not abort under ASan:
        // the guard stopped start() before it touched a member of the freed
        // dialog. The mutation matrix (report) proves it is the guard, and not
        // luck, that did so.
        QVERIFY2(out.openRan,
                 qPrintable(QStringLiteral("%1: start() never reached store->open() - this run proves nothing")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the parent teardown did not destroy the dialog - the test's "
                                           "premise is wrong")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.startReturnedFalse,
                 qPrintable(QStringLiteral("%1: start() returned true after the dialog was destroyed inside this "
                                           "nested loop - it resumed onto a freed dialog (A3-R026-F2)")
                                .arg(QLatin1String(frame))));
        QCOMPARE(out.readFileCalls, 0);
        QVERIFY2(!out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the destructor deleted the store while a blocking call was on the "
                                           "stack (DEC-025 requires the deletion to be DECLINED)")
                                .arg(QLatin1String(frame))));
        QVERIFY2(!out.storeClosed,
                 qPrintable(QStringLiteral("%1: the destructor closed the store while a blocking call was still "
                                           "executing on it")
                                .arg(QLatin1String(frame))));
    }

  private slots:

    // guard :778 — teardown inside the open-failure QMessageBox::exec().
    // RED (neuter :778): start() resumes into QWidget::hide() (:780) on the freed
    // dialog -> ASan heap-use-after-free.
    void openFailure_teardownInsideMsgBoxExec_doesNotUseAFreedDialog()
    {
        assertStartBailedNotCrashed("open-failure QMessageBox::exec (:778)", runStartBranchTeardown(OpenFailExec));
    }

    // guard :782 — teardown inside the open-failure QApplication::processEvents().
    // RED (neuter :782): start() resumes into QMetaObject::invokeMethod(this,
    // "close", ...) (:784) on the freed dialog -> ASan heap-use-after-free.
    void openFailure_teardownInsideProcessEvents_doesNotUseAFreedDialog()
    {
        assertStartBailedNotCrashed("open-failure processEvents (:782)", runStartBranchTeardown(OpenFailProcessEvents));
    }

    // guard :993 — teardown inside the unsaved-changes QMessageBox::exec().
    // RED (neuter :993): start() resumes into switch(ret) -> a `this`-> access
    // (either the Cancel branch's queued close at :1010 or the tail
    // refreshClicked() at :1022) on the freed dialog -> ASan heap-use-after-free.
    void dirtyRides_teardownInsideMsgBoxExec_doesNotUseAFreedDialog()
    {
        assertStartBailedNotCrashed("unsaved-changes QMessageBox::exec (:993)", runStartBranchTeardown(DirtyExec));
    }

    // =====================================================================
    // TEST-078 (guard :1023) — the syncCloud ENTRY PATH.
    // =====================================================================
    //
    // Builds the dialog on the heap EXACTLY as MainWindow::syncCloud now does
    // (DEC-027 Option A: new + Qt::WA_DeleteOnClose + `if (sync->start())
    // sync->open()`), and tears the parent down inside start()'s tail
    // refreshClicked() -> store->readdir (blockingList) -> nested loop, leaving
    // start() suspended at its final frame (:1022).
    //
    // start()'s tail has NO member access of its own after refreshClicked()
    // returns (it just `return true`), so :1023's whole job is to hand the CALLER
    // the honest answer. This slot therefore proves the guard at the place it
    // actually matters — the call site: if start() wrongly returns true after the
    // dialog was destroyed, `dialog->open()` dereferences a freed dialog. That is
    // the precise use-after-free DEC-027 introduces by moving syncCloud from a
    // stack exec() to a modeless heap open(); this is the test that closes it.
    //
    // This is the syncCloud-entry-path geometry TEST-078 asks for. TEST-075
    // already exercises the OTHER half of the same path (teardown inside the
    // constructor-time open()), but does NOT call open() after start(); this slot
    // does, so it is not a duplicate.
    //
    // RED (neuter :1023): start() returns true, dialog->open() runs on the freed
    // dialog -> ASan heap-use-after-free at the syncCloud call site.
    void syncCloudEntryPath_teardownInsideTailReaddir_doesNotOpenAFreedDialog()
    {
        obs::reset();

        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;
        bool startedFalse = false;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                store->closeAction = [owner]() { delete owner; };
                // Armed BEFORE start(): with open() and the ride cache quiet, the
                // tail refreshClicked() (:1022) is the FIRST and only nested loop
                // start() spins, so the teardown lands there.
                store->blockInReaddir = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);

                dialogGuard = dialog;
                storeGuard = store;
                store->dialogGuard = dialog;

                // VERBATIM the production syncCloud call site (MainWindow.cpp,
                // DEC-027). Reading start()'s bool is safe after `this` is freed;
                // calling dialog->open() on that freed dialog is the UAF the guard
                // exists to prevent.
                if (dialog->start())
                    dialog->open();
                else
                    startedFalse = true;

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(obs::openCalls > 0, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::resumedAfterNestedLoop,
                 "the tail readdir never ran its nested loop - the teardown did not land where the test needs it");
        QVERIFY2(dialogGuard.isNull(), "the parent teardown did not destroy the dialog - the test's premise is wrong");
        QVERIFY2(startedFalse,
                 "start() returned true after the dialog was destroyed in its tail readdir - syncCloud would "
                 "open() a freed dialog (:1023 / A3-R026-F2)");
        QCOMPARE(obs::readFileCalls, 0);
        // DEC-025 decline: a blocking call (readdir) was on the stack, so the store
        // is leaked rather than freed/closed under the call still using it.
        QVERIFY2(!(storeGuard.isNull() && obs::storeDestroyed),
                 "the destructor deleted the store while readdir was on the stack (DEC-025 requires DECLINE)");
        QVERIFY2(!obs::storeClosed, "the destructor closed the store while readdir was still executing on it");
    }

  private:
    // =====================================================================
    // TEST-079 / TEST-080 (REQ-019, DEC-garmin-029) — THE UPLOAD DIALOG.
    // =====================================================================
    //
    // SUBJECT: CloudServiceUploadDialog (src/Cloud/CloudService.cpp) and its
    // single call site CloudService::upload(), reached from
    // MainWindow::uploadCloud. NOTHING BELOW IS ABOUT GARMINCONNECT — GarminConnect
    // advertises Query|Download only (GarminConnect.h:76) and can never reach this
    // dialog. The eleven services that CAN are RideWithGPS, CyclingAnalytics,
    // Selfloops, SportsPlusHealth, TrainingsTageBuch and Xert (explicit Upload
    // bit) plus Strava, Dropbox, SixCycle, SportTracks and LocalFileStore
    // (inheriting the CloudService.h:104 default). These slots live in a file
    // named after GarminConnect purely to reuse this target's ASan + offscreen +
    // blocking-store harness; the misnomer is knowingly accepted and tracked as
    // ORCH-008.
    //
    // THE DEFECT. The upload dialog's CONSTRUCTOR ran every blocking operation
    // while the object was a STACK dialog parented to the WA_DeleteOnClose
    // MainWindow (MainWindow.cpp:143): store->open(), the unsaved-changes
    // QMessageBox::exec(), compressRide/writeFile, and the upload-failure
    // QMessageBox::exec() — with no QPointer self-bail anywhere. A parent
    // teardown landing in any of those nested loops resumed the constructor on
    // freed memory and then wrote `status`, read `context` and called
    // QWidget::hide() on it. DEC-029 fixes it the way DEC-026/027 fixed the sync
    // dialog: two-phase init (shell-only ctor + start()) and a heap dialog with
    // WA_DeleteOnClose, modality PRESERVED.
    // =====================================================================
    enum UploadFrame {
        UploadOpen,           // teardown inside store->open()                        -> guard A
        UploadDirtyExec,      // teardown inside the unsaved-changes QMessageBox::exec() -> guard B
        UploadCancelPE,       // teardown inside the Cancel branch's processEvents()  -> guard B2
        UploadWrite,          // teardown inside compressRide/writeFile               -> guard C
        UploadFailExec,       // teardown inside the upload-failure QMessageBox::exec() -> guard D
        UploadFailPE,         // teardown inside the failure branch's processEvents() -> guard D2
        UploadExecWait,       // teardown inside the exec() wait for writeComplete    -> Qt's own guard
        UploadFailNoTeardown, // CONTROL: open() fails, nobody tears anything down
        UploadHappyPath       // CONTROL: no teardown at all
    };

    struct UploadOutcome
    {
        bool startReturnedFalse = false; // start() bailed instead of building on a corpse
        bool execReturned = false;       // exec() came back rather than hanging
        int execResult = -1;             // ...with what
        bool dialogGoneAtEnd = false;    // the teardown really did destroy the dialog
        bool storeClosed = false;        // the CALLER's closeAndDeleteStore ran close()
        bool storeDestroyed = false;     // ...and then deleted it, exactly once
        bool openRan = false;
        bool writeRan = false;
        bool timedOut = false;
        bool ownerSurvived = false;         // nothing tore the athlete window down
        bool dialogGoneBeforeOwner = false; // ...and it collected ITSELF anyway
    };

    // Drives ONE parent teardown into ONE of the upload dialog's suspension
    // points, through the production call sequence:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped, exactly as when
    //        the user picks a service from the Upload menu)
    //          -> owner QWidget + its Context           [MainWindow + its Context]
    //          -> new CloudServiceUploadDialog(owner, ...)   [CloudService::upload]
    //          -> setAttribute(Qt::WA_DeleteOnClose)
    //          -> if (start()) exec();
    //               -> store->open() / QMessageBox::exec() / writeFile / exec()
    //                    -> NESTED loop -> queued `delete owner; delete ctx` on qApp
    //                         -> ~QWidget -> ~QObject destroys its children
    //                              -> ~CloudServiceUploadDialog, mid-call
    //          -> closeAndDeleteStore(store)            [MainWindow.cpp:2563]
    //
    // The Context is destroyed WITH the window, because that is what makes the
    // criterion's "no use-after-free on context/context->mainWindow" testable at
    // all: in production the athlete window owns its Context, and the dialog's
    // `context` member is what the unsaved-changes Save branch dereferences.
    UploadOutcome runUploadTeardown(UploadFrame frame)
    {
        obs::reset();

        UploadOutcome out;
        QPointer<CloudServiceUploadDialog> dialogGuard;
        QPointer<CloudService> storeGuard;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                // The athlete window. WA_DeleteOnClose is the production
                // attribute (MainWindow.cpp:143) and the reason this teardown is
                // reachable at all.
                QWidget* owner = new QWidget;
                owner->setAttribute(Qt::WA_DeleteOnClose);

                // The Context that window owns. Only its QWidget-level identity
                // is used through the MainWindow* it is handed (saveSilent is the
                // one call, and it is a link stub here).
                Context* runCtx = new Context(reinterpret_cast<MainWindow*>(owner));
                runCtx->athlete = athlete;

                // The ride being uploaded. compressRide() dereferences
                // ride->context, so this needs a real RideFile, not a null one.
                RideFile* rideFile = new RideFile();
                rideFile->context = runCtx;
                RideItem* item = new RideItem(rideFile, runCtx);
                // ImportSeamStubs' RideItem ctor sets only context and ride_, so
                // isdirty is uninitialised garbage - and the dialog branches on
                // it. Say so explicitly rather than inherit whatever the heap
                // happened to hold (this cost one hung run to find).
                item->isdirty = false;
                item->fileName = QStringLiteral("2026_08_07_10_00_00.json");
                // Production hands the dialog context->ride (MainWindow.cpp:2556).
                runCtx->ride = item;

                BlockingStore* store = new BlockingStore(runCtx);
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                store->completeWrite = (frame == UploadHappyPath);

                switch (frame) {
                case UploadOpen:
                    store->blockInOpen = true;
                    break;
                case UploadWrite:
                    store->blockInWrite = true;
                    break;
                case UploadDirtyExec:
                case UploadCancelPE:
                    item->isdirty = true; // reaches the unsaved-changes prompt
                    break;
                case UploadFailExec:
                case UploadFailPE:
                case UploadFailNoTeardown:
                    store->openFailMode = true; // reaches the upload-failure prompt
                    break;
                default:
                    break;
                }

                bool ownerDead = false;
                auto killOwner = [owner, runCtx, &ownerDead]() {
                    obs::teardownFired = true;
                    obs::teardownSawModal = (QApplication::activeModalWidget() != nullptr);
                    obs::teardownAfterStart = obs::startReturned;
                    ownerDead = true;
                    delete owner;
                    delete runCtx;
                };

                if (frame == UploadOpen || frame == UploadWrite) {
                    // Fired from inside the store call's own nested loop.
                    store->closeAction = killOwner;
                } else if (frame == UploadFailNoTeardown) {
                    // No teardown at all - just a user dismissing the "unable to
                    // upload" box. What is under test is what happens NEXT: the
                    // dialog is a heap object with WA_DeleteOnClose that the
                    // caller will never exec(), so start()'s failure branch has
                    // to close it or it leaks (CloudService.cpp:501).
                    QTimer::singleShot(100, qApp, []() {
                        if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                            m->close();
                    });
                } else if (frame == UploadCancelPE || frame == UploadFailPE) {
                    // One frame LATER than the two above. The modal box is
                    // answered with the dialog still ALIVE (so the guard after
                    // exec() passes), and only THEN is the teardown posted - so
                    // it is not delivered inside exec() (done() exits that loop
                    // immediately) but by the QApplication::processEvents() the
                    // branch runs next (CloudService.cpp:450 / :493). Cancel is
                    // the answer that reaches the first of those.
                    const int answer = (frame == UploadCancelPE) ? int(QMessageBox::Cancel) : int(QMessageBox::Ok);
                    QMetaObject::invokeMethod(
                        qApp,
                        [killOwner, answer]() {
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(answer);
                            QMetaObject::invokeMethod(qApp, killOwner, Qt::QueuedConnection);
                        },
                        Qt::QueuedConnection);
                } else if (frame == UploadDirtyExec || frame == UploadFailExec) {
                    // Posted BEFORE the dialog runs, so the FIRST nested loop it
                    // spins — the QMessageBox — delivers it. Answering the box
                    // afterwards is what returns exec() onto the guard.
                    //
                    // DirtyExec answers SAVE deliberately: that branch is the one
                    // that dereferences context and context->mainWindow
                    // (CloudService.cpp:367-369), i.e. the exact objects the
                    // teardown just freed.
                    const int answer = (frame == UploadDirtyExec) ? int(QMessageBox::Save) : int(QMessageBox::Cancel);
                    QMetaObject::invokeMethod(
                        qApp,
                        [killOwner, answer]() {
                            killOwner();
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(answer);
                        },
                        Qt::QueuedConnection);
                }

                storeGuard = store;

                // If a guard ever failed to let exec() return, the whole suite
                // would hang; unwind instead and record why.
                bool* timedOut = &out.timedOut;
                QTimer::singleShot(5000, qApp, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                if (frame == UploadExecWait || frame == UploadHappyPath) {
                    // ---- THE REAL PRODUCTION ENTRY POINT.
                    //
                    // These two frames call CloudService::upload() itself - the
                    // function that owns the `new`, the WA_DeleteOnClose, the
                    // deliberate absence of an `else delete` and the exec(). They
                    // can, because they do not need to observe start()'s bool:
                    // start() succeeds on both. The dialog is reached through
                    // findChild, since upload() constructs it internally.
                    if (frame == UploadExecWait) {
                        // The teardown lands while exec() is waiting for
                        // writeComplete. Posted to qApp, not to the dialog: the
                        // dialog is what it destroys.
                        QTimer::singleShot(50, qApp, [owner, killOwner, &dialogGuard]() {
                            dialogGuard = owner->findChild<CloudServiceUploadDialog*>();
                            killOwner();
                        });
                    } else {
                        // The user clicking OK once the upload completed - the
                        // only route by which this dialog accepts.
                        QTimer::singleShot(200, qApp, [owner, &dialogGuard]() {
                            CloudServiceUploadDialog* d = owner->findChild<CloudServiceUploadDialog*>();
                            dialogGuard = d;
                            if (d && d->okcancel)
                                d->okcancel->click();
                        });
                    }

                    bool accepted = CloudService::upload(owner, runCtx, store, item);
                    obs::startReturned = true;
                    out.execReturned = true;
                    out.execResult = accepted ? int(QDialog::Accepted) : int(QDialog::Rejected);
                } else {
                    // ---- The same sequence CloudService::upload() runs, opened
                    // up so start()'s bool - the thing the guards actually
                    // produce - is observable. The two frames above are what
                    // prove this hand-copy still matches the real function.
                    CloudServiceUploadDialog* uploader = new CloudServiceUploadDialog(owner, runCtx, store, item);
                    uploader->setAttribute(Qt::WA_DeleteOnClose);
                    dialogGuard = uploader;

                    // PHASE TWO. Reading start()'s bool is safe even after `this`
                    // is freed (the value is in a register); dereferencing
                    // `uploader` past a false is not, so we do not.
                    bool started = uploader->start();
                    obs::startReturned = true;
                    out.startReturnedFalse = (started == false);

                    if (started) {
                        out.execResult = uploader->exec();
                        out.execReturned = true;
                    }
                }

                // ---- and the caller's REQ-017 obligation, unchanged by DEC-029:
                // MainWindow::uploadCloud closes and deletes the store it created,
                // exactly once, after upload() returns (MainWindow.cpp:2563).
                delete item;
                delete rideFile;
                closeAndDeleteStore(store);

                out.ownerSurvived = !ownerDead;
                if (!ownerDead) {
                    // Destroy the athlete window LATE, and record first whether
                    // the dialog had already collected itself. Deleting it here
                    // and now would destroy the dialog as a CHILD and hide the
                    // very thing UploadFailNoTeardown asks about: a dialog the
                    // caller never exec()s has to self-delete off the failure
                    // path (CloudService.cpp:501) rather than sit there leaking
                    // until its parent happens to go.
                    QTimer::singleShot(300, qApp, [owner, runCtx, &out, &dialogGuard]() {
                        out.dialogGoneBeforeOwner = dialogGuard.isNull();
                        delete owner;
                        delete runCtx;
                    });
                }

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeClosed = obs::storeClosed;
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        out.openRan = obs::openCalls > 0;
        out.writeRan = obs::writeFileCalls > 0;
        return out;
    }

    // The shared verdict for the four TEST-079 suspension points. NOT a slot.
    void assertUploadBailedNotCrashed(const char* frame, const UploadOutcome& out)
    {
        // Reaching this line at all means the process did not abort under ASan:
        // no heap-use-after-free and no bad-free on the dialog, on `context` or
        // on `context->mainWindow`. The mutation matrix (see the build report) is
        // what proves it is the guard, and not luck, that achieved that.
        QVERIFY2(
            out.timedOut == false,
            qPrintable(QStringLiteral("%1: the dialog never came back - a guard wedged it").arg(QLatin1String(frame))));
        QVERIFY2(obs::teardownFired,
                 qPrintable(QStringLiteral("%1: the parent teardown never ran - this run proves nothing")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the parent teardown did not destroy the dialog - the test's "
                                           "premise is wrong")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.startReturnedFalse,
                 qPrintable(QStringLiteral("%1: start() returned true after the dialog was destroyed inside this "
                                           "nested loop - upload() would exec() a freed dialog")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.execReturned == false,
                 qPrintable(QStringLiteral("%1: upload() exec()d a dialog that start() had already lost")
                                .arg(QLatin1String(frame))));

        // The store is the CALLER's (REQ-017, MainWindow.cpp:2563) - DEC-029
        // changes nothing about that - so unlike the sync dialog it is closed and
        // deleted on EVERY route, exactly once. A second free would have aborted
        // this process under ASan long before this line.
        QVERIFY2(out.storeClosed,
                 qPrintable(QStringLiteral("%1: the store was never close()d - the REQ-017 owner contract broke")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was never deleted - it leaked past the teardown")
                                .arg(QLatin1String(frame))));
    }

  private slots:

    // -- TEST-080 --------------------------------------------------------
    // THE ONE CLAIM DEC-029 RESTS ON AND HAD NOT EXECUTED.
    //
    // Option B keeps upload MODAL and leaves closeAndDeleteStore(db) unguarded at
    // MainWindow.cpp:2563. Both of those depend on QDialog::exec() SELF-PROTECTING
    // when `this` is destroyed inside its own event loop: if exec() touched a
    // member after the loop returned, the fix would be building on the same kind
    // of reasoned-but-unverified Qt claim that DEC-024's closeEvent() premise
    // turned out to be.
    //
    // So this tears the parent down while exec() is waiting for writeComplete —
    // the fourth suspension point — and asks ASan. It must report nothing, exec()
    // must come back rather than hang, and it must NOT report Accepted: a torn-down
    // upload is not a successful one.
    void uploadExecWait_parentTeardownMidExec_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadExecWait);

        QVERIFY2(out.timedOut == false, "exec() never returned after the dialog was destroyed inside it");
        QVERIFY2(obs::teardownFired, "the parent teardown never ran - this run proves nothing");
        QVERIFY2(out.writeRan, "the upload never reached writeFile, so exec() was not waiting for writeComplete");
        QVERIFY2(out.dialogGoneAtEnd, "the parent teardown did not destroy the dialog - the test's premise is wrong");
        QVERIFY2(out.execReturned, "exec() did not return at all");
        QVERIFY2(out.execResult != QDialog::Accepted,
                 "exec() reported Accepted for an upload whose dialog was destroyed mid-flight");
        QVERIFY2(out.storeClosed, "the store was never close()d - the REQ-017 owner contract broke");
        QVERIFY2(out.storeDestroyed, "the store was never deleted - it leaked past the teardown");
    }

    // -- TEST-079 --------------------------------------------------------
    // guard A - teardown inside store->open() (CloudService.cpp:346).
    // RED (neuter guard A): start() resumes into `status = opened` - a member
    // WRITE on the freed dialog -> ASan heap-use-after-free.
    void upload_teardownInsideStoreOpen_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadOpen);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::openResumed, "open() never resumed from its nested loop - the teardown did not land there");
        assertUploadBailedNotCrashed("store->open (:346)", out);
        QCOMPARE(obs::writeFileCalls, 0);
    }

    // guard B - teardown inside the unsaved-changes QMessageBox::exec() (:363).
    // RED (neuter guard B): start() resumes into the Save branch and dereferences
    // `context` and `context->mainWindow` (:367-369), both freed with the athlete
    // window -> ASan heap-use-after-free.
    void upload_teardownInsideUnsavedChangesPrompt_doesNotUseAFreedContext()
    {
        UploadOutcome out = runUploadTeardown(UploadDirtyExec);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::teardownSawModal,
                 "the teardown did not land inside a modal prompt - it missed the frame under test");
        assertUploadBailedNotCrashed("unsaved-changes QMessageBox::exec (:363)", out);
        QCOMPARE(obs::writeFileCalls, 0);
    }

    // guard B2 - teardown inside the Cancel branch's QApplication::processEvents().
    // RED (neuter guard B2): start() resumes into
    // QMetaObject::invokeMethod(this, "close", ...) on the freed dialog -> ASan
    // heap-use-after-free.
    void upload_teardownInsideCancelProcessEvents_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadCancelPE);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::teardownAfterStart == false,
                 "the teardown landed after start() had already returned - it missed the frame under test");
        QVERIFY2(obs::teardownSawModal == false,
                 "the teardown landed while the prompt was still up - that is guard B's frame, not this one");
        assertUploadBailedNotCrashed("unsaved-changes Cancel processEvents (:365)", out);
        QCOMPARE(obs::writeFileCalls, 0);
    }

    // guard C - teardown inside compressRide/writeFile (:386/:389).
    // RED (neuter guard C): start() resumes into `status = wrote` - a member WRITE
    // on the freed dialog -> ASan heap-use-after-free.
    void upload_teardownInsideWriteFile_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadWrite);
        QVERIFY2(out.writeRan, "start() never reached store->writeFile() - this run proves nothing");
        QVERIFY2(obs::writeResumed, "writeFile never resumed from its nested loop - the teardown did not land there");
        assertUploadBailedNotCrashed("compressRide/writeFile (:386/:389)", out);
    }

    // guard D - teardown inside the upload-failure QMessageBox::exec() (:401).
    // RED (neuter guard D): start() resumes into QWidget::hide() (:403) on the
    // freed dialog -> ASan heap-use-after-free.
    void upload_teardownInsideFailurePrompt_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadFailExec);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::teardownSawModal,
                 "the teardown did not land inside a modal prompt - it missed the frame under test");
        assertUploadBailedNotCrashed("upload-failure QMessageBox::exec (:401)", out);
        QCOMPARE(obs::writeFileCalls, 0);
    }

    // guard D2 - teardown inside the failure branch's QApplication::processEvents().
    // RED (neuter guard D2): start() resumes into the queued close() invoke on the
    // freed dialog -> ASan heap-use-after-free.
    void upload_teardownInsideFailureProcessEvents_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadFailPE);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(obs::teardownAfterStart == false,
                 "the teardown landed after start() had already returned - it missed the frame under test");
        QVERIFY2(obs::teardownSawModal == false,
                 "the teardown landed while the prompt was still up - that is guard D's frame, not this one");
        assertUploadBailedNotCrashed("upload-failure processEvents (:404)", out);
        QCOMPARE(obs::writeFileCalls, 0);
    }

    // CONTROL - the LEAK half of the heap conversion, with no teardown anywhere.
    //
    // Going from a stack dialog to `new` + WA_DeleteOnClose means the failure
    // path no longer has a scope to destroy it, and the caller deliberately does
    // NOT exec() (nor `else delete`) a dialog whose start() failed - so if that
    // path did not close itself, every failed upload would leak a dialog for the
    // life of the athlete window, on all eleven Upload-capable services.
    //
    // RED (delete the queued close() at CloudService.cpp:501): the dialog is
    // still alive when this looks, and only its parent's eventual destruction
    // collects it.
    void upload_openFailureWithNoTeardown_closesItselfRatherThanLeaking()
    {
        UploadOutcome out = runUploadTeardown(UploadFailNoTeardown);

        QVERIFY2(out.timedOut == false, "the failure prompt never came back");
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(out.ownerSurvived, "something destroyed the athlete window - this is the no-teardown control");
        QVERIFY2(out.startReturnedFalse, "start() reported success for a store that failed to open");
        QVERIFY2(out.execReturned == false, "the caller exec()d a dialog whose start() had failed");
        QVERIFY2(out.dialogGoneBeforeOwner,
                 "the failed upload dialog was still alive with its parent untouched - a heap dialog the caller "
                 "never exec()s and never deletes has leaked");
        QCOMPARE(obs::writeFileCalls, 0);
        QVERIFY2(out.storeClosed, "the store was never close()d");
        QVERIFY2(out.storeDestroyed, "the store was never deleted");
    }

    // CONTROL for all seven. The same heap + WA_DeleteOnClose + start()/exec()
    // sequence with NO teardown anywhere must still upload, still block modally,
    // still report Accepted when the user clicks OK, and still leave nothing
    // behind - otherwise DEC-029 has broken upload for eleven services in order
    // to fix a crash none of them had yet hit.
    void upload_withNoTeardown_stillCompletesModallyAndCleansUp()
    {
        UploadOutcome out = runUploadTeardown(UploadHappyPath);

        QVERIFY2(out.timedOut == false, "the upload dialog never returned from exec()");
        QVERIFY2(out.openRan, "the upload never opened the store");
        QVERIFY2(out.writeRan, "the upload never reached writeFile");
        QVERIFY2(out.startReturnedFalse == false, "start() failed on the happy path");
        QVERIFY2(out.execReturned, "exec() did not return");
        QCOMPARE(out.execResult, int(QDialog::Accepted));
        QVERIFY2(out.dialogGoneAtEnd, "the dialog leaked - WA_DeleteOnClose did not collect it");
        QVERIFY2(out.storeClosed, "the store was never close()d");
        QVERIFY2(out.storeDestroyed, "the store was never deleted");
    }
};

QTEST_MAIN(TestGarminConnectSyncDialogClose)
#include "testGarminConnectSyncDialogClose.moc"
