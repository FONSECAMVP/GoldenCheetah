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
// TEST garmin:TEST-091 — DEC-garmin-024 applied to the UNCOUNTED nested loops
//                        (REQ-025 / B-R031-01: openRideFile enclosing a counted
//                        frame let DEC-031's reaper fire underneath it).
// TEST garmin:TEST-092 — DEC-garmin-030's rider for CloudServiceUploadDialog's
//                        lazy `item->ride()` open (REQ-025).
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
#include "RideFile.h"
#include "RideItem.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
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
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>
#include <QtTest/QtTest>

#if !defined(__SANITIZE_ADDRESS__) && !defined(GC_TEST_WITH_ASAN)
#    error \
        "TEST-070/TEST-071 are lifetime tests and are only trustworthy under AddressSanitizer; build this target with -fsanitize=address."
#endif

#include <sanitizer/asan_interface.h>

#include <cstring>
#include <functional>
#include <new>

// TEST-082 — the member-touch observation channel defined in
// stubs/ImportSeamStubs.cpp. See the block comment there: the stand-ins for
// Context::metadataFlush, RideItem::notifyRideMetadataChanged and
// MainWindow::saveSilent each perform one REAL load through their object and
// publish it here, so this file can prove the loads happen rather than assume
// they do. Declared extern rather than put in a header because ImportSeamStubs
// has no header and is shared verbatim by three targets.
namespace gcstub {
extern volatile quintptr contextMemberTouch;
extern volatile quintptr rideItemMemberTouch;
extern volatile quintptr saveSilentThisTouch;
extern volatile quintptr saveSilentArgTouch;
} // namespace gcstub

// ---------------------------------------------------------------------------
// TEST-090 (DEC-garmin-031) — ONE STORE'S REAP, and why a bool cannot state it.
//
// DEC-031 replaced DEC-025's deliberate leak with a frame-counted reaper, so
// "was the store destroyed?" stopped being enough in two directions at once:
//
//   EXACTLY ONCE. A bool is equally true for one close+delete and for two, and
//   two is a DOUBLE FREE - precisely what a shared (rather than per-dialog,
//   refcounted) record produces when two sync dialogs have suspended frames on
//   the same stack. So they are COUNTED.
//
//   ONLY AFTER THE NESTED LOOP RETURNED. That is the property DEC-025 gave the
//   store up to protect, and the one a naive store->deleteLater() would break
//   (TEST-089 measures that a deferred delete posted from inside a nested
//   QEventLoop is delivered BY that loop). Only the store knows when it is
//   inside its own loop.exec(), so it raises a depth around it and SNAPSHOTS
//   that depth at the instant it is closed and at the instant it is destroyed -
//   taken then and there, because afterwards the answer is always zero.
//
// A struct rather than more namespace-scope bools because TEST-090's refcount
// case has TWO stores alive at once and the shared obs:: counters cannot tell
// them apart. Kept OUTSIDE the store for the same reason obs:: is: the store is
// gone by the time anything reads it.
// ---------------------------------------------------------------------------

// TEST-091 (REQ-025) — HOW DEEP INSIDE A RIDE FILE OPEN WE ARE.
//
// DEC-024's invariant, written into CloudService.cpp:1371, is "every store call
// that can run a nested QEventLoop is wrapped in one of these [BlockingCall]".
// REQ-025 is about the nested loops that are NOT store calls and were therefore
// never counted at all: RideFileFactory::openRideFile runs one on the FIT read
// path (FitRideFile.cpp:172-184 — a 5s-timeout wait on a network reply), and
// GarminConnect downloads .fit. Such a loop can ENCLOSE a counted one, and then
// DEC-031's reaper fires on the INNER frame's release and closes+deletes the
// store under the still-suspended outer frame.
//
// So "was the store reaped while a ride file open this dialog started was still
// on the stack?" has to be answered at the instant of the reap — afterwards the
// answer is always no. The blocking reader below raises this around its
// loop.exec() and the store snapshots it in close()/~BlockingStore, exactly as
// ReapLog already does for the store's OWN calls.
int rideFileOpenDepth = 0;

struct ReapLog
{
    int closed = 0;    // close() calls
    int destroyed = 0; // ~BlockingStore calls
    int callDepth = 0; // nested loops THIS store is inside right now
    int maxCallDepth = 0;
    bool resumed = false;                   // a blocking call ran its tail on a live store
    bool closedInsideItsOwnCall = false;    // ...was it closed while inside one?
    bool destroyedInsideItsOwnCall = false; // ...and destroyed while inside one?
    bool closedBeforeDestroyed = false;     // close() first, delete second (REQ-017 (e))

    // TEST-091 (REQ-025) — the same two questions asked of the UNCOUNTED
    // enclosing frame: a ride file open the dialog started and is still
    // suspended in. See the rideFileOpenDepth comment above.
    bool closedInsideARideFileOpen = false;
    bool destroyedInsideARideFileOpen = false;

    void enterCall()
    {
        ++callDepth;
        if (callDepth > maxCallDepth)
            maxCallDepth = callDepth;
    }
    void leaveCall() { --callDepth; }
    void noteClosed()
    {
        ++closed;
        if (callDepth > 0)
            closedInsideItsOwnCall = true;
        if (rideFileOpenDepth > 0)
            closedInsideARideFileOpen = true;
    }
    void noteDestroyed()
    {
        ++destroyed;
        if (callDepth > 0)
            destroyedInsideItsOwnCall = true;
        if (rideFileOpenDepth > 0)
            destroyedInsideARideFileOpen = true;
        closedBeforeDestroyed = (closed > 0);
    }
};

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

// TEST-090 (DEC-garmin-031) — the reaper's bar for every single-store run. See
// the ReapLog comment above for what it records and why the two bools above
// stopped being enough.
ReapLog storeReap;

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
    storeReap = ReapLog();
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
    ~BlockingStore() override
    {
        obs::storeDestroyed = true;
        // TEST-090 — snapshotted HERE, while the answer still exists.
        obs::storeReap.noteDestroyed();
        if (log)
            log->noteDestroyed();
    }

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
            if (log)
                log->resumed = true;
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
        obs::storeReap.noteClosed();
        if (log)
            log->noteClosed();
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
            // TEST-090 — a SECOND action, for the run that needs two of this
            // store's nested loops on the stack at once (a Refresh dispatched by
            // the processEvents() inside a download, which is the nesting the
            // blockingCallDepth comment in CloudService.h describes). Empty for
            // every other run, which leaves this exactly as it was.
            closeAction = nextAction;
            nextAction = nullptr;
            QObject* target = closeActionContext ? closeActionContext : static_cast<QObject*>(dialog);
            QMetaObject::invokeMethod(target, action, Qt::QueuedConnection);
        }

        // blockingDownload() / blockingList(): a nested event loop that keeps
        // running until the transfer finishes, i.e. well past the user's click.
        //
        // TEST-090 — the depth is raised around loop.exec() and nowhere else, so
        // "was the store inside its own call?" is answerable at the instant it is
        // closed or destroyed. A DEPTH rather than a bool because these calls can
        // nest (the QApplication::processEvents() inside one can dispatch a click
        // that starts another).
        obs::storeReap.enterCall();
        if (log)
            log->enterCall();
        QEventLoop loop;
        QTimer::singleShot(blockingMs, &loop, &QEventLoop::quit);
        loop.exec();
        obs::storeReap.leaveCall();
        if (log)
            log->leaveCall();
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
        if (log)
            log->resumed = true;

        // A member READ, so the ASan report names this function and this line
        // rather than something further downstream.
        if (canary_ != kCanary)
            return false;

        // TEST-087 - the completion is what puts the dialog INSIDE completedRead,
        // and that run drives the slot itself so that it can queue the teardown
        // directly behind it. Suppressing the automatic notification keeps the two
        // out of each other's way; every other run leaves it on.
        if (!completeRead)
            return true;

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
    std::function<void()> nextAction;      // ...and what happens inside the NEXT one (TEST-090)
    ReapLog* log = nullptr;                // per-store reap record, when one store is not enough
    QObject* closeActionContext = nullptr; // who it is delivered to (default: dialog)
    bool blockInReaddir = false;           // does readdir run a nested loop too?
    bool blockInOpen = false;              // does open() run a nested loop too? (TEST-075)
    bool openFailMode = false;             // does open() FAIL, taking start()'s open-failure branch? (TEST-077)
    bool blockInWrite = false;             // does writeFile run a nested loop? (TEST-079)
    bool writeSucceeds = true;             // ...and does it report the upload started?
    bool completeWrite = false;            // ...and does writeComplete ever arrive? (TEST-080)
    bool completeRead = true;              // ...and does readComplete? (TEST-087 drives it itself)
    QPointer<QDialog> dialogGuard;
    int blockingMs = 120;

  private:
    static const int kCanary = 0x5A5A5A;
    int canary_ = kCanary;
};

// ---------------------------------------------------------------------------
// TEST-091 / TEST-092 (REQ-025) — BlockingRideFileReader: FitRideFile's SHAPE,
// reduced to the part that bites.
//
// FitRideFile::openRideFile fetches FITmetadata.json from goldencheetah.org and
// waits for the reply in a local QEventLoop with a 5-second timeout
// (FitRideFile.cpp:172-184). That loop is entered from
// RideFileFactory::openRideFile, which the sync dialog calls at three places and
// RideItem::ride(bool open=true) calls lazily at a fourth — and NONE of them is
// inside a BlockingCall. So it is a nested event loop that the dialog's own
// blockingCallDepth cannot see, and it can enclose one that it can.
//
// A real .fit file would make that loop depend on the network and on the
// FITmetadata cache, i.e. on whether the machine running the suite is online.
// So the SUFFIX is stood in for instead: this reader is registered for
// ".gcblock" and does the one thing that matters — run a nested QEventLoop, then
// return a RideFile — through the REAL RideFileFactory::openRideFile dispatch
// that production goes through.
// ---------------------------------------------------------------------------
namespace rideopen {

std::function<void()> action; // what the user does INSIDE the open's nested loop
int blockingMs = 300;         // ...and how long that loop lasts
int opens = 0;                // was the reader reached at all?
bool resumed = false;         // ...and did its loop run to completion?

void reset()
{
    action = nullptr;
    blockingMs = 300;
    opens = 0;
    resumed = false;
}

} // namespace rideopen

class BlockingRideFileReader : public RideFileReader
{
  public:
    RideFile* openRideFile(QFile& file, QStringList& errors, QList<RideFile*>* list) const override
    {
        Q_UNUSED(file);
        Q_UNUSED(errors);
        Q_UNUSED(list);

        ++rideopen::opens;

        // The user's action, delivered from INSIDE this loop and QUEUED (so it
        // arrives through QCoreApplication::notifyInternal2, which is what lets
        // a DeferredDelete posted by it outrank the loop). Fires once.
        if (rideopen::action) {
            std::function<void()> a = rideopen::action;
            rideopen::action = nullptr;
            QMetaObject::invokeMethod(qApp, a, Qt::QueuedConnection);
        }

        // FitRideFile.cpp:172-184, verbatim in shape: a single-shot timer, a
        // local QEventLoop and exec(WaitForMoreEvents).
        ++rideFileOpenDepth;
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(rideopen::blockingMs);
        loop.exec(QEventLoop::WaitForMoreEvents);
        --rideFileOpenDepth;
        rideopen::resumed = true;

        // A RideFile with NO tags and NO intervals on purpose: the tag-rewriting
        // tail of RideFileFactory::openRideFile (RideFile.cpp:930-960) walks
        // GlobalContext::context()->rideMetadata, which is null in this target's
        // stubs.
        return new RideFile();
    }
};

// ---------------------------------------------------------------------------
// TEST-082 — OrderProbeDialog: a cloud dialog reduced to ONE question.
//
// REQ-021 is not about whether the dialog dies; DEC-024/025/026/027/029 settled
// that. It is about WHEN it dies RELATIVE TO the Context, Athlete and RideItem
// its suspended frames still hold raw pointers to. So this stand-in records, at
// the instant its destructor runs, whether the storage it was watching is still
// addressable. It NEVER dereferences that pointer — it asks ASan's shadow map
// instead, which is the only way to ask the question without committing the very
// use-after-free under investigation.
// ---------------------------------------------------------------------------
class OrderProbeDialog : public QDialog
{
  public:
    OrderProbeDialog(QWidget* parent, const void* watched, size_t watchedSize)
        : QDialog(parent, Qt::Dialog), watched_(watched), watchedSize_(watchedSize)
    {
    }

    ~OrderProbeDialog() override
    {
        destroyed = true;
        watchedAddressableAtMyDeath = __asan_region_is_poisoned(const_cast<void*>(watched_), watchedSize_) == nullptr;
    }

    // Kept outside the object for the same reason obs:: is: the object is gone by
    // the time anything reads them.
    static bool destroyed;
    static bool watchedAddressableAtMyDeath;

    static void reset()
    {
        destroyed = false;
        watchedAddressableAtMyDeath = false;
    }

  private:
    const void* watched_;
    size_t watchedSize_;
};

bool OrderProbeDialog::destroyed = false;
bool OrderProbeDialog::watchedAddressableAtMyDeath = false;

// ---------------------------------------------------------------------------
// TEST-082 — FakeAthleteWindow: production's OWNERSHIP ORDER, and nothing else.
//
// WHY THE EXISTING HELPER CANNOT BE MADE TO SERVE
// -----------------------------------------------
// runUploadTeardown's `killOwner` does `delete owner; delete runCtx;` back to
// back, and the dialog is a CHILD of `owner`. So the dialog is always the FIRST
// thing to die and every `QPointer<T> self(this)` bail is already true on
// resume: the collaborator axis is unobservable by construction. That helper is
// correct for what it tests (DEC-029, the dialog's own lifetime) and is left
// exactly as it is.
//
// WHAT PRODUCTION ACTUALLY DOES, WHICH THIS CLASS MODELS
// ------------------------------------------------------
// The Context is owned by the ATHLETE TAB, and the tab dies FIRST:
//
//   MainWindow::removeAthleteTab (MainWindow.cpp:2183-2185)
//       delete tab; delete athlete; delete context;      <- SYNCHRONOUS
//
// and the window-close route runs that for every tab and only THEN lets Qt
// collect the window itself:
//
//   MainWindow::closeEvent (MainWindow.cpp:1102-1103) -> removeAthleteTab(tab)
//       ... MainWindow carries WA_DeleteOnClose (MainWindow.cpp:143), so its own
//       destruction is DEFERRED to a posted DeferredDelete.
//
// The consequence, and the whole of REQ-021: a dialog parented to the WINDOW
// reliably OUTLIVES the Context it points at. Every self-bail stays false and the
// resumed frame dereferences freed memory. A dialog parented to the TAB does not
// — it is destroyed by ~QObject's deleteChildren() at `delete tab`, strictly
// before `delete context`. Both shapes are hosted by this one class, because
// which of them is correct is DEC-030's open question (TEST-081) and not this
// harness's to prejudge.
//
// Widgets, layouts and views are all absent: nothing here is about what an
// AthleteTab LOOKS like, only about what deletes what, in what order, on whose
// stack.
// ---------------------------------------------------------------------------
class FakeAthleteWindow : public QWidget
{
  public:
    // One athlete tab's worth of ownership.
    struct AthleteSlot
    {
        QWidget* tabWidget = nullptr; // stands in for AthleteTab (AthleteTab.h:32)
        Context* context = nullptr;   // owned by the TAB, not by Qt's parent chain
        Athlete* athlete = nullptr;
        RideCache* rideCache = nullptr; // owned by the ATHLETE, as in production
        RideItem* item = nullptr;
        RideFile* rideFile = nullptr;

        // The same addresses, kept as raw storage AFTER the objects are freed, so
        // a test can ask ASan whether they are poisoned without dereferencing
        // anything. Never cast these back to a live type.
        const void* contextAddr = nullptr;
        const void* athleteAddr = nullptr;
        const void* itemAddr = nullptr;
    };

    enum DialogHost {
        HostedByWindow, // DEC-030 option A shape: dialog parented to the window
        HostedByTab     // DEC-030 option B shape: dialog parented to context->tab
    };

    explicit FakeAthleteWindow(const QDir& athleteHome) : home_(athleteHome) {}

    ~FakeAthleteWindow() override
    {
        // A safety net for a window torn down with tabs still open: the tab
        // widgets would otherwise be collected as children AFTER this body runs,
        // leaking every Context. closeWindow() has normally emptied the list.
        while (!tabs_.isEmpty())
            closeAthleteTab(0);
        qDeleteAll(closedTabs_);
        closedTabs_.clear();
    }

    // MainWindow::openAthleteTab's shape: the tab widget is a CHILD of the
    // window, and the Context / Athlete / RideItem hang off the TAB rather than
    // off Qt's ownership tree.
    AthleteSlot* addAthleteTab()
    {
        AthleteSlot* slot = new AthleteSlot;
        slot->tabWidget = new QWidget(this);
        // Only the QWidget-level identity of this window is used through the
        // MainWindow* the Context is handed (the same reinterpret_cast the
        // upload harness already relies on, testGarminConnectSyncDialogClose.cpp
        // as it stood before this slice - saveSilent is the one call, and it is a
        // link stub).
        slot->context = new Context(reinterpret_cast<MainWindow*>(this));
        // AthleteTab::AthleteTab does exactly this, in its constructor body
        // (AthleteTab.cpp:35). REQ-021 turns that link into the dialogs' PARENT
        // (DEC-030 option B), so a harness that left it unset would be testing
        // nothing. Only the QWidget-level identity of the tab is ever used
        // through it - the same reinterpret_cast this file already makes for
        // MainWindow, and for the same reason (no AthleteTab is linked here).
        slot->context->tab = reinterpret_cast<AthleteTab*>(slot->tabWidget);
        slot->athlete = new Athlete(slot->context, home_);
        slot->context->athlete = slot->athlete;
        // The Athlete owns its RideCache, and the RideCache owns the RideItems.
        // The sync dialog walks context->athlete->rideCache->rides()
        // (CloudService.cpp:1147) and the upload dialog's Save branch reaches
        // the same objects, so both have to be real here.
        slot->rideCache = new RideCache(slot->context);
        slot->athlete->rideCache = slot->rideCache;
        slot->rideFile = new RideFile();
        slot->rideFile->context = slot->context;
        slot->item = new RideItem(slot->rideFile, slot->context);
        slot->item->fileName = QStringLiteral("2026_08_10_10_00_00.json");
        slot->rideCache->rides().push_back(slot->item);
        // Production hands the upload dialog context->ride (MainWindow.cpp:2556).
        slot->context->ride = slot->item;

        slot->contextAddr = slot->context;
        slot->athleteAddr = slot->athlete;
        slot->itemAddr = slot->item;

        tabs_.append(slot);
        return slot;
    }

    QWidget* hostFor(DialogHost host, int index)
    {
        return host == HostedByWindow ? static_cast<QWidget*>(this) : tabs_.at(index)->tabWidget;
    }

    int openTabCount() const { return int(tabs_.count()); }

    // MainWindow::removeAthleteTab (MainWindow.cpp:2183-2185) — the order and the
    // SYNCHRONY are the point. Nothing here is deferred, and the window survives.
    // Reached from the tab bar's X and from AthleteCard::clicked ->
    // closeAthleteTab(QString) (AthleteView.cpp:213 -> MainWindow.cpp:2103).
    void closeAthleteTab(int index)
    {
        AthleteSlot* slot = tabs_.takeAt(index);

        delete slot->tabWidget;
        // The RideItem lives in the Athlete's RideCache in production, so it goes
        // when the Athlete does — after the tab, before the Context.
        delete slot->item;
        delete slot->rideFile;
        delete slot->rideCache;
        delete slot->athlete;
        delete slot->context;

        // The live pointers are corpses now; keep only the address copies.
        slot->tabWidget = nullptr;
        slot->context = nullptr;
        slot->athlete = nullptr;
        slot->rideCache = nullptr;
        slot->item = nullptr;
        slot->rideFile = nullptr;
        closedTabs_.append(slot);
    }

    // MainWindow::closeEvent (MainWindow.cpp:1102-1103) + WA_DeleteOnClose
    // (MainWindow.cpp:143): every tab is torn down SYNCHRONOUSLY, and only the
    // window's own destruction is DEFERRED. That asymmetry is the defect.
    void closeWindow()
    {
        while (!tabs_.isEmpty())
            closeAthleteTab(0);
        deleteLater();
    }

  private:
    QDir home_;
    QList<AthleteSlot*> tabs_;
    QList<AthleteSlot*> closedTabs_;
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

                // A Context whose TAB is the owner, so the constructor parents the
                // dialog to it (DEC-garmin-030: CloudService.cpp:884 now reads
                // QDialog(context->tab, Qt::Dialog); before REQ-021 the same link
                // was made through context->mainWindow). Only QWidget-level
                // parenting is used before the teardown lands (see method
                // comment), so `owner` standing in for the athlete tab is exactly
                // as faithful as it was standing in for the window.
                Context ctorCtx(reinterpret_cast<MainWindow*>(owner));
                ctorCtx.tab = reinterpret_cast<AthleteTab*>(owner);
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

    // =====================================================================
    // TEST-090 (REQ-022, DEC-garmin-031) — THE REAPER'S BAR.
    //
    // The shared verdict for every parent-teardown route that used to assert
    // DEC-025's deliberate leak. NOT a slot.
    // =====================================================================
    //
    // DEC-025 declined to close or delete the store when the destructor was
    // reached with a blocking call suspended, and leaked it, because "at
    // parent-teardown time the application is already tearing down, so there is
    // no live event loop left for a reaper to run on". REQ-021 falsified that:
    // the dialog is a child of context->tab, and closing one athlete tab of
    // several destroys it while the application runs on. So the leak stopped
    // being once-per-exit and became per-close ACCUMULATING - a live worker
    // thread and a resident interpreter session per occurrence for GarminConnect.
    //
    // DEC-031 replaces it with a frame-counted reaper, and this is its bar. Both
    // halves are load-bearing and both are asserted rather than implied:
    //
    //   EXACTLY ONCE, because the failure mode of getting the record shape wrong
    //   (one shared counter instead of one per dialog, refcounted) is a SECOND
    //   close and a second delete - a double free, which a bool cannot see.
    //
    //   ONLY AFTER THE NESTED LOOP RETURNED, because that is the property DEC-025
    //   gave the store up to protect, and the one the naive one-line reaper would
    //   break: TEST-089 measures that a store->deleteLater() posted from inside
    //   the store's own nested loop is delivered BY that loop.
    //
    // Takes the record by reference so the single-store slots can pass the shared
    // obs::storeReap - which still holds the run that has just finished, exactly
    // as assertSyncDiedWithItsTab already reads obs::teardownFired - and TEST-090
    // can pass one per store.
    //
    // RED, and the reason leak coverage no longer depends on detect_leaks (which
    // is OFF on this target, finding A3-R027-F4): delete the `record->release()`
    // in the dialog.isNull() branch of ~BlockingCall and every one of these fails
    // POSITIVELY - destroyed 0, not 1.
    void assertStoreReapedExactlyOnceAfterTheNestedLoop(const char* what, const ReapLog& reap)
    {
        QVERIFY2(reap.destroyed == 1,
                 qPrintable(QStringLiteral("%1: the store was deleted %2 times, not exactly once - 0 means the "
                                           "reaper never ran and the store leaked (DEC-031); more than 1 is a "
                                           "double free")
                                .arg(QLatin1String(what))
                                .arg(reap.destroyed)));
        QVERIFY2(reap.closed == 1,
                 qPrintable(QStringLiteral("%1: the store was close()d %2 times, not exactly once - 0 strands the "
                                           "worker thread and the interpreter session (REQ-017 (e)); more than 1 "
                                           "tears them down twice")
                                .arg(QLatin1String(what))
                                .arg(reap.closed)));

        QVERIFY2(reap.closedBeforeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was deleted without being close()d first - the reaper "
                                           "must perform closeAndDeleteStore, not a bare delete")
                                .arg(QLatin1String(what))));

        // ...and the ORDERING half. Not "the reap happened eventually" but "the
        // reap happened with no call of the store's own still on the stack".
        QVERIFY2(!reap.closedInsideItsOwnCall,
                 qPrintable(QStringLiteral("%1: close() was called on the store while it was still inside its own "
                                           "nested event loop - the reap fired too early and tore down the worker "
                                           "thread under the call using it (DEC-031)")
                                .arg(QLatin1String(what))));
        QVERIFY2(!reap.destroyedInsideItsOwnCall,
                 qPrintable(QStringLiteral("%1: the store was DELETED while it was still inside its own nested "
                                           "event loop - this is the use-after-free DEC-025 gave the store up to "
                                           "prevent, and exactly what a naive deleteLater() does (TEST-089)")
                                .arg(QLatin1String(what))));
    }

    // The single-store overload: every inverted slot measures the one store the
    // run created, so it reads the shared record.
    void assertStoreReapedExactlyOnceAfterTheNestedLoop(const char* what)
    {
        assertStoreReapedExactlyOnceAfterTheNestedLoop(what, obs::storeReap);
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

        // DEC-garmin-031 (REQ-022) — INVERTED. DEC-025 had the destructor DECLINE
        // the deletion and LEAK the store; this asserted that leak as required
        // behaviour, to keep it a decision rather than a drift. REQ-021 destroyed
        // its justification (see assertStoreReapedExactlyOnceAfterTheNestedLoop),
        // so the bar is now the opposite one: the store IS closed and deleted -
        // exactly once, and only once the nested loop it was executing in has
        // returned.
        QVERIFY2(out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was never deleted - DEC-025's leak is back and now "
                                           "accumulates once per athlete-tab close (DEC-031)")
                                .arg(QLatin1String(call))));
        QVERIFY2(out.storeClosed,
                 qPrintable(QStringLiteral("%1: the store was never close()d - its worker thread and interpreter "
                                           "session outlive the athlete tab")
                                .arg(QLatin1String(call))));
        assertStoreReapedExactlyOnceAfterTheNestedLoop(call);
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

        // TEST-091 / TEST-092 (REQ-025) — the ride file whose READER suspends.
        // Registered on the REAL factory singleton, so the dispatch under test
        // (RideFileFactory::openRideFile, called at CloudService.cpp:1994, :2261
        // and :363, and by RideItem::ride) is production's own.
        RideFileFactory::instance().registerReader(
            QStringLiteral("gcblock"), QStringLiteral("TEST-091 blocking reader"), new BlockingRideFileReader);
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

        // DEC-garmin-031 (REQ-022) — INVERTED on this route too. The destructor
        // still declines to delete the store WHERE IT STANDS (store->open() is on
        // the stack), but it no longer drops it: it hands it to the orphan record
        // and start()'s own BlockingCall, the last to unwind, closes and deletes
        // it once open() has returned.
        QVERIFY2(out.storeDestroyed,
                 "the store was never deleted after store->open() returned - DEC-025's leak is back (DEC-031)");
        QVERIFY2(out.storeClosed, "the store was never close()d - its worker thread and session were stranded");
        assertStoreReapedExactlyOnceAfterTheNestedLoop("construction-time store->open()");
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

        // DEC-garmin-031 (REQ-022) — INVERTED. See
        // assertStoreReapedExactlyOnceAfterTheNestedLoop: the deletion is
        // DEFERRED to the last unwinding frame now, not declined outright.
        QVERIFY2(out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was never deleted - DEC-025's leak is back (DEC-031)")
                                .arg(QLatin1String(frame))));
        QVERIFY2(out.storeClosed,
                 qPrintable(QStringLiteral("%1: the store was never close()d - its worker thread and interpreter "
                                           "session were stranded")
                                .arg(QLatin1String(frame))));
        assertStoreReapedExactlyOnceAfterTheNestedLoop(frame);
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
        // DEC-garmin-031 (REQ-022) — INVERTED. readdir was on the stack when the
        // destructor ran, so it declined THERE; start()'s BlockingCall - the
        // outer one, and the last to unwind - is what closes and deletes it.
        QVERIFY2(storeGuard.isNull() && obs::storeDestroyed,
                 "the store was never deleted after the tail readdir returned - DEC-025's leak is back (DEC-031)");
        QVERIFY2(obs::storeClosed, "the store was never close()d - its worker thread and session were stranded");
        assertStoreReapedExactlyOnceAfterTheNestedLoop("syncCloud entry path, tail readdir");
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
        UploadLazyOpen,       // teardown inside item->ride()'s LAZY OPEN (REQ-025)   -> guard C0
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
        rideopen::reset();

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
                // DEC-garmin-030 (REQ-021): CloudService::upload now parents the
                // dialog to context->tab rather than to the QWidget it is handed
                // (CloudService.cpp:119). TEST-079/080's subject is unchanged - a
                // teardown of the dialog's PARENT mid-call - so `owner` stands in
                // for the athlete tab here as it already stands in for the window;
                // killOwner destroys both together, exactly as before.
                runCtx->tab = reinterpret_cast<AthleteTab*>(owner);
                runCtx->athlete = athlete;

                // The ride being uploaded. compressRide() dereferences
                // ride->context, so this needs a real RideFile, not a null one.
                RideFile* rideFile = new RideFile();
                rideFile->context = runCtx;
                // TEST-092 (REQ-025) — except for the LAZY OPEN frame, whose whole
                // subject is a RideItem that does NOT have its ride in memory:
                // item->ride() then opens it through RideFileFactory
                // (RideItem.cpp:175-181) and suspends in the reader's nested loop.
                RideItem* item = new RideItem(frame == UploadLazyOpen ? nullptr : rideFile, runCtx);
                // The dialog BRANCHES on isdirty, so this frame states what it
                // wants rather than inheriting a default. TEST-082 closed the
                // other half of finding B-R019-04: ImportSeamStubs' RideItem ctor
                // used to leave isdirty uninitialised, and whatever the heap
                // happened to hold cost one hung run to diagnose.
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
                case UploadLazyOpen:
                    // Where the ride is read from, and with the suffix whose
                    // reader suspends (see BlockingRideFileReader).
                    item->path = context->athlete->home->activities().absolutePath();
                    item->fileName = localBlockingActivity();
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
                auto killOwner = [owner, runCtx, &ownerDead, frame]() {
                    obs::teardownFired = true;
                    obs::teardownSawModal = (QApplication::activeModalWidget() != nullptr);
                    obs::teardownAfterStart = obs::startReturned;
                    ownerDead = true;
                    delete owner;
                    // TEST-092 (REQ-025) — the LAZY OPEN frame LEAKS the Context
                    // on purpose (detect_leaks is off on this target). The tail of
                    // RideFileFactory::openRideFile dereferences the context it
                    // was handed AFTER the reader's nested loop returns
                    // (RideFile.cpp:999, `context->athlete->cyclist`), so a
                    // fixture that freed it here would abort inside RideFile.cpp
                    // and measure that out-of-scope defect instead of the one this
                    // frame is about. Reported as a finding rather than papered
                    // over. Every other frame is unchanged.
                    if (frame != UploadLazyOpen)
                        delete runCtx;
                };

                if (frame == UploadOpen || frame == UploadWrite) {
                    // Fired from inside the store call's own nested loop.
                    store->closeAction = killOwner;
                } else if (frame == UploadLazyOpen) {
                    // TEST-092 (REQ-025) — fired from inside the RIDE FILE
                    // reader's nested loop instead, which is where item->ride()
                    // at :558 suspends.
                    rideopen::blockingMs = 300;
                    rideopen::action = killOwner;
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
                // Scoped to `appLoop`, NOT to qApp: `timedOut` points into this
                // function's stack frame, and a watchdog left armed on qApp fires
                // five seconds later - inside whatever slot is running BY THEN -
                // and writes to a dead frame (ASan: stack-use-after-return). The
                // context object is destroyed when this run returns, so Qt drops
                // the pending call instead.
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
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
                //
                // TEST-092 — whatever the lazy open handed the item is ours to
                // release too; ride(false) does not re-open (RideItem.cpp:177).
                if (frame == UploadLazyOpen)
                    delete item->ride(false);
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

    // -- TEST-092 (REQ-025) ----------------------------------------------
    // guard C0 - teardown inside item->ride()'s LAZY OPEN, the suspension point
    // the comment above compressRide said did not exist.
    //
    // THE FALSE PREMISE. CloudService.cpp said: "compressRide() runs no nested
    // event loop - it is QTemporaryFile plus a RideFileReader::writeRideFile, and
    // the only QEventLoop anywhere in the file writers is on the FIT READ path
    // (FitRideFile.cpp:172, reached from openRideFile) - so a separate self-bail
    // between it and writeFile would be unreachable and untestable." The FIT READ
    // PATH IS REACHED FROM HERE: `item->ride()` is RideItem::ride(bool open =
    // true), which opens the ride file through RideFileFactory when it is not
    // already in memory (RideItem.cpp:175-181). An athlete tab closing inside
    // that loop destroys this dialog - it is a child of context->tab (DEC-030) -
    // and start() then resumes into `store->writeFile(...)`, a read of `store`
    // and of the `data` MEMBER off freed storage.
    //
    // The bail is `self`-only, and deliberately so (A3-R021-F5): the RideItem is
    // what the suspension is executing ON, and a collaborator test here would be
    // a guard no test could make fail - see the report's COMPLETENESS statement.
    //
    // RED (before the hoist + bail):
    //   AddressSanitizer: heap-use-after-free READ in
    //   CloudServiceUploadDialog::start(), CloudService.cpp:562.
    void upload_teardownInsideLazyRideOpen_doesNotUseAFreedDialog()
    {
        UploadOutcome out = runUploadTeardown(UploadLazyOpen);
        QVERIFY2(out.openRan, "start() never reached store->open() - this run proves nothing");
        QVERIFY2(rideopen::opens > 0,
                 "item->ride() never opened a ride file - the RideItem already had one in memory and this run "
                 "proves nothing");
        QVERIFY2(rideopen::resumed,
                 "the lazy open never resumed from its nested loop - the teardown did not land inside it");
        QVERIFY2(obs::teardownSawModal == false,
                 "the teardown landed inside a modal prompt - it missed the frame under test");
        assertUploadBailedNotCrashed("item->ride() lazy open (:558)", out);
        if (QTest::currentTestFailed())
            return;
        // ...and the upload did NOT proceed on a dialog that had already lost its
        // own storage.
        QCOMPARE(obs::writeFileCalls, 0);
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

  private:
    // "Has this storage been freed?" — asked of the ALLOCATOR, not of the
    // program. ASan poisons a block on free and holds it in quarantine, so a
    // poisoned region is a freed one. This is the only way to ask the question
    // without committing the very use-after-free under investigation.
    static bool isPoisoned(const void* addr, size_t size)
    {
        return addr != nullptr && __asan_region_is_poisoned(const_cast<void*>(addr), size) != nullptr;
    }

    // =====================================================================
    // TEST-082 (REQ-021) — driving a teardown at PRODUCTION EVENT-LOOP DEPTH.
    // =====================================================================
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event DELIVERY, so scopeLevel is bumped, exactly as
    //        when the user picks a service from a menu)
    //          -> `build`: the athlete window, its tabs and the dialog
    //          -> NESTED QEventLoop   [stands in for the store's blocking call:
    //             GarminConnect::blockingDownload, CloudService::readFile, a
    //             modal QMessageBox::exec(), ...]
    //               -> queued `teardown` on qApp, delivered INSIDE that loop
    //          -> `afterResume`: the suspended frame, running with whatever the
    //             teardown left of its collaborators
    //
    // A harness that tore things down from loop level 0 would not reproduce
    // anything: event-loop LEVEL and delivery SCOPE are the subject.
    void withNestedLoopTeardown(const std::function<void()>& build, const std::function<void()>& teardown,
                                const std::function<void()>& afterResume)
    {
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                build();

                QEventLoop blocking;
                QMetaObject::invokeMethod(
                    qApp,
                    [&]() {
                        teardown();
                        blocking.quit();
                    },
                    Qt::QueuedConnection);
                // If a teardown ever failed to land, the suite would hang; unwind
                // instead and let the assertions say what was missing.
                QTimer::singleShot(2000, &blocking, &QEventLoop::quit);
                blocking.exec();

                afterResume();

                QTimer::singleShot(100, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }

  private slots:

    // =====================================================================
    // TEST-082 — THE COLLABORATOR AXIS, MADE OBSERVABLE.
    //
    // Closing ONE athlete tab (the tab bar X, or AthleteCard::clicked ->
    // MainWindow::closeAthleteTab, AthleteView.cpp:213) runs
    //     delete tab; delete athlete; delete context;
    // synchronously and leaves the window standing. A dialog parented to the
    // WINDOW is therefore untouched — it is STILL ALIVE, with every one of its
    // `QPointer<T> self(this)` bails false, holding raw pointers into three
    // freed objects.
    //
    // This is the fact REQ-021 exists for, and no existing test can state it:
    // killOwner deletes the dialog's parent and the Context together, so the
    // dialog always dies first there.
    // =====================================================================
    void closingOneAthleteTabLeavesAWindowParentedDialogOnAFreedContext()
    {
        OrderProbeDialog::reset();

        FakeAthleteWindow* win = nullptr;
        FakeAthleteWindow::AthleteSlot* tab = nullptr;
        QPointer<OrderProbeDialog> dialogGuard;

        bool dialogAliveOnResume = false;
        bool contextFreedOnResume = false;
        bool athleteFreedOnResume = false;
        bool itemFreedOnResume = false;
        bool windowAliveOnResume = false;

        withNestedLoopTeardown(
            [&]() {
                win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                tab = win->addAthleteTab();
                OrderProbeDialog* dialog = new OrderProbeDialog(win->hostFor(FakeAthleteWindow::HostedByWindow, 0),
                                                                tab->context, sizeof(Context));
                dialog->open(); // modeless, as the sync dialog is
                dialogGuard = dialog;
            },
            [&]() { win->closeAthleteTab(0); },
            [&]() {
                dialogAliveOnResume = !dialogGuard.isNull();
                windowAliveOnResume = (win != nullptr && win->openTabCount() == 0);
                contextFreedOnResume = isPoisoned(tab->contextAddr, sizeof(Context));
                athleteFreedOnResume = isPoisoned(tab->athleteAddr, sizeof(Athlete));
                itemFreedOnResume = isPoisoned(tab->itemAddr, sizeof(RideItem));
            });

        // The premise: the teardown really did free all three collaborators.
        QVERIFY2(contextFreedOnResume, "the harness did not free the Context - it proves nothing about REQ-021");
        QVERIFY2(athleteFreedOnResume, "the harness did not free the Athlete");
        QVERIFY2(itemFreedOnResume, "the harness did not free the RideItem");

        // ...and the window, and therefore the dialog, sailed straight past it.
        QVERIFY2(windowAliveOnResume,
                 "closing one athlete tab destroyed the window - that is not what production does");
        QVERIFY2(dialogAliveOnResume,
                 "a window-parented dialog did NOT outlive its Context - the collaborator axis is unobservable "
                 "in this harness and REQ-021 cannot be tested with it");
        QVERIFY2(OrderProbeDialog::destroyed == false, "the dialog was destroyed by a tab close it is not a child of");

        delete win; // takes the dialog with it
    }

    // =====================================================================
    // TEST-082 — the SAME teardown against the DEC-030 option B shape.
    //
    // Parented to the TAB, the dialog is destroyed by ~QObject's
    // deleteChildren() at `delete tab` — the FIRST of the three deletes — so it
    // is gone STRICTLY BEFORE the Athlete and the Context it points at. Asserted
    // as an ORDERING, not as an end state: the dialog's destructor asks ASan
    // whether the Context storage is still addressable at the moment it runs.
    //
    // This slot does not endorse option B. It makes the property option B claims
    // measurable, so that TEST-081's verdict decides between the two on evidence.
    // =====================================================================
    void closingOneAthleteTabDestroysATabParentedDialogBeforeItsContext()
    {
        OrderProbeDialog::reset();

        FakeAthleteWindow* win = nullptr;
        FakeAthleteWindow::AthleteSlot* tab = nullptr;
        QPointer<OrderProbeDialog> dialogGuard;

        bool dialogGoneOnResume = false;
        bool contextFreedOnResume = false;

        withNestedLoopTeardown(
            [&]() {
                win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                tab = win->addAthleteTab();
                OrderProbeDialog* dialog = new OrderProbeDialog(win->hostFor(FakeAthleteWindow::HostedByTab, 0),
                                                                tab->context, sizeof(Context));
                dialog->open();
                dialogGuard = dialog;
            },
            [&]() { win->closeAthleteTab(0); },
            [&]() {
                dialogGoneOnResume = dialogGuard.isNull();
                contextFreedOnResume = isPoisoned(tab->contextAddr, sizeof(Context));
            });

        QVERIFY2(contextFreedOnResume, "the harness did not free the Context");
        QVERIFY2(OrderProbeDialog::destroyed, "the tab-parented dialog was not destroyed with its tab");
        QVERIFY2(dialogGoneOnResume, "the tab-parented dialog was still alive when the suspended frame resumed");
        QVERIFY2(OrderProbeDialog::watchedAddressableAtMyDeath,
                 "the tab-parented dialog was destroyed AFTER its Context was freed - option B's whole premise "
                 "(deleteChildren runs at `delete tab`, before `delete context`) is wrong");

        delete win;
    }

    // =====================================================================
    // TEST-082 — the WINDOW-close route, and the asymmetry that makes it bite.
    //
    // MainWindow::closeEvent tears every tab down SYNCHRONOUSLY
    // (MainWindow.cpp:1102-1103 -> :2183-2185) and then lets WA_DeleteOnClose
    // collect the window through a DEFERRED deleteLater(). So even the route that
    // does destroy the window leaves a window-parented dialog alive for the
    // remainder of the current event delivery — which is precisely where the
    // suspended frames resume.
    // =====================================================================
    void closingTheWindowFreesEveryContextBeforeDeferringItsOwnDeletion()
    {
        OrderProbeDialog::reset();

        FakeAthleteWindow* win = nullptr;
        QPointer<FakeAthleteWindow> windowGuard;
        QPointer<OrderProbeDialog> dialogGuard;
        const void* firstContextAddr = nullptr;
        const void* secondContextAddr = nullptr;

        bool windowAliveOnResume = false;
        bool dialogAliveOnResume = false;
        bool bothContextsFreedOnResume = false;

        withNestedLoopTeardown(
            [&]() {
                win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                windowGuard = win;
                FakeAthleteWindow::AthleteSlot* first = win->addAthleteTab();
                FakeAthleteWindow::AthleteSlot* second = win->addAthleteTab();
                firstContextAddr = first->contextAddr;
                secondContextAddr = second->contextAddr;

                OrderProbeDialog* dialog = new OrderProbeDialog(win->hostFor(FakeAthleteWindow::HostedByWindow, 0),
                                                                first->context, sizeof(Context));
                dialog->open();
                dialogGuard = dialog;
            },
            [&]() { win->closeWindow(); },
            [&]() {
                windowAliveOnResume = !windowGuard.isNull();
                dialogAliveOnResume = !dialogGuard.isNull();
                bothContextsFreedOnResume =
                    isPoisoned(firstContextAddr, sizeof(Context)) && isPoisoned(secondContextAddr, sizeof(Context));
            });

        QVERIFY2(bothContextsFreedOnResume, "closeWindow() did not free every tab's Context");
        QVERIFY2(windowAliveOnResume,
                 "the window was destroyed synchronously - WA_DeleteOnClose defers it, and that deferral is the "
                 "reason the dialog outlives its Context");
        QVERIFY2(dialogAliveOnResume, "the window-parented dialog did not outlive the Contexts");

        // ...and once the DeferredDelete is delivered, both really do go.
        QVERIFY2(windowGuard.isNull(), "the window leaked past its deleteLater()");
        QVERIFY2(dialogGuard.isNull(), "the dialog leaked - it was not collected with its parent");
        QVERIFY2(OrderProbeDialog::destroyed, "the dialog was never destroyed at all");
    }

    // =====================================================================
    // TEST-082 — the stand-ins that a freed collaborator has to FAULT through.
    //
    // Findings A3-R019-F3 and B-R019-05: three ImportSeamStubs bodies that
    // production reaches WITH a possibly-freed collaborator were inert `{}`, so
    // a test could call them on a corpse and see nothing. Two of them stand in
    // for SIGNALS (Context::metadataFlush, Context.h:341;
    // RideItem::notifyRideMetadataChanged, RideItem.h:89) whose real
    // implementation is QMetaObject::activate(this, ...) — an unconditional load
    // of `this`. The third, MainWindow::saveSilent, dereferences both arguments
    // for real (SaveDialogs.cpp:125+). All three sit on CloudService.cpp:444-445,
    // the upload dialog's Save branch.
    //
    // This asserts the loads HAPPEN. That they then fault on freed storage is the
    // mutation half of the evidence and is in the build report, because a test
    // that committed the use-after-free would abort the process rather than pass.
    //
    // B-R019-04 is here too: the RideItem stand-in left `isdirty` uninitialised
    // and the upload dialog BRANCHES on it (CloudService.cpp:436), which is why
    // that finding surfaced as a HANG - the worst diagnostic shape there is.
    // =====================================================================
    void stubbedCollaboratorCallsLoadThroughTheirObject()
    {
        QWidget fakeMainWindow;
        MainWindow* asMainWindow = reinterpret_cast<MainWindow*>(&fakeMainWindow);

        Context probeContext(asMainWindow);
        probeContext.athlete = athlete;

        RideFile probeRide;
        RideItem probeItem(&probeRide, &probeContext);

        // Context::metadataFlush - the real one is a signal; the stand-in must
        // dereference `this` the way QMetaObject::activate does.
        gcstub::contextMemberTouch = 0;
        probeContext.metadataFlush();
        QCOMPARE(quintptr(gcstub::contextMemberTouch), reinterpret_cast<quintptr>(athlete));

        // RideItem::notifyRideMetadataChanged - likewise (CloudService.cpp:444).
        gcstub::rideItemMemberTouch = 0;
        probeItem.notifyRideMetadataChanged();
        QCOMPARE(quintptr(gcstub::rideItemMemberTouch), reinterpret_cast<quintptr>(&probeContext));

        // MainWindow::saveSilent (CloudService.cpp:445). The `this` load is
        // QObject-level ON PURPOSE and is a PRAGMATIC close, not a strict one:
        // this target's "MainWindow" is a reinterpret_cast of a plain QWidget, so
        // reading a MainWindow-specific member would be undefined behaviour.
        // isWidgetType() reads d_ptr in the QObject subobject, which sits at
        // offset 0 of both types, and does fault on freed storage.
        gcstub::saveSilentThisTouch = 0;
        gcstub::saveSilentArgTouch = 0;
        asMainWindow->saveSilent(&probeContext, &probeItem);
        QCOMPARE(quintptr(gcstub::saveSilentThisTouch), quintptr(1)); // 1 == isWidgetType() was true
        QCOMPARE(quintptr(gcstub::saveSilentArgTouch),
                 reinterpret_cast<quintptr>(athlete) ^ reinterpret_cast<quintptr>(&probeContext));

        // B-R019-04. Constructed over deliberately non-zero storage, so "the heap
        // happened to hold a 0" cannot pass this.
        void* raw = ::operator new(sizeof(RideItem));
        std::memset(raw, 0xFF, sizeof(RideItem));
        RideItem* dirtyProbe = new (raw) RideItem(&probeRide, &probeContext);
        const bool isdirtyAfterConstruction = dirtyProbe->isdirty;
        dirtyProbe->~RideItem();
        ::operator delete(raw);
        QVERIFY2(isdirtyAfterConstruction == false,
                 "ImportSeamStubs' RideItem constructor leaves isdirty uninitialised; the upload dialog branches "
                 "on it (CloudService.cpp:436) and a garbage true hangs the test in an unasked-for modal prompt");
    }

    // =====================================================================
    // TEST-081 (REQ-021) — THE GATING PROBE FOR DEC-030.
    //
    // DEC-030 option B reparents both cloud dialogs to context->tab so that Qt
    // destroys them at `delete tab` (MainWindow.cpp:2183), before the Context
    // they point at. That is sound on lifetime grounds. It rests on ONE Qt
    // behaviour that no primary source settles:
    //
    //     does hiding the parent WIDGET also hide a child QDialog WINDOW?
    //
    // Because if it does, then every athlete switch hides the modeless sync
    // dialog: MainWindow::switchAthleteTab (MainWindow.cpp:2367) does
    // tabStack->setCurrentIndex (:2389), and QStackedLayout hides the outgoing
    // tab. Option B would then trade a use-after-free for a dialog that vanishes
    // mid-sync, and the fix has to fall back to option A instead.
    //
    // This project's standing rule: framework semantics are decided by
    // EXECUTION, not by reading docs or source. So this measures it, both for a
    // raw hide() and for the QStackedWidget switch production actually performs,
    // and asks the re-show question too — a STICKY hide is a worse failure than a
    // transient one.
    //
    // The recorded expectations below are MEASUREMENTS on Qt 6.8.2 with the
    // offscreen QPA plugin this target runs under. If a Qt upgrade changes them,
    // this slot is meant to fail loudly.
    // =====================================================================
    void hidingTheParentWidgetOfAChildQDialogWindow()
    {
        QWidget window; // MainWindow
        QStackedWidget* tabStack = new QStackedWidget(&window);
        QWidget* tabA = new QWidget; // the AthleteTab the dialog would parent to
        QWidget* tabB = new QWidget; // a second athlete
        tabStack->addWidget(tabA);
        tabStack->addWidget(tabB);
        tabStack->setCurrentIndex(0);
        window.show();
        QApplication::processEvents();
        QVERIFY2(tabA->isVisible(), "premise: the current athlete tab is visible");

        QDialog* dialog = new QDialog(tabA, Qt::Dialog);
        dialog->open(); // modeless, exactly as the sync dialog is opened
        QApplication::processEvents();
        QVERIFY2(dialog->isVisible(), "premise: the modeless child dialog came up");

        // THE SENSITIVITY CONTROL. A "still visible" verdict is worthless unless
        // this apparatus can see a hide at all — and under the offscreen QPA it
        // is fair to doubt that. So an ORDINARY (non-window) child of the same
        // parent rides along: it must go invisible at every step where the dialog
        // does not.
        QWidget* plainChild = new QWidget(tabA);
        plainChild->show();
        QApplication::processEvents();
        QVERIFY2(plainChild->isVisible(), "premise: the ordinary child widget is visible");

        // (a) a raw hide() of the parent widget.
        tabA->hide();
        QApplication::processEvents();
        const bool visibleAfterRawHide = dialog->isVisible();
        const bool controlVisibleAfterRawHide = plainChild->isVisible();
        tabA->show();
        QApplication::processEvents();
        const bool visibleAfterRawReshow = dialog->isVisible();

        // (b) what switchAthleteTab ACTUALLY does: QStackedWidget::setCurrentIndex,
        //     which hides the outgoing page for you.
        tabStack->setCurrentIndex(1);
        QApplication::processEvents();
        const bool visibleAfterTabSwitch = dialog->isVisible();
        const bool controlVisibleAfterTabSwitch = plainChild->isVisible();
        tabStack->setCurrentIndex(0);
        QApplication::processEvents();
        const bool visibleAfterSwitchBack = dialog->isVisible();

        qInfo("TEST-081 measurement (Qt %s, QPA %s): dialog rawHide=%d rawReshow=%d tabSwitch=%d switchBack=%d | "
              "control child rawHide=%d tabSwitch=%d",
              qVersion(), qPrintable(QApplication::platformName()), int(visibleAfterRawHide),
              int(visibleAfterRawReshow), int(visibleAfterTabSwitch), int(visibleAfterSwitchBack),
              int(controlVisibleAfterRawHide), int(controlVisibleAfterTabSwitch));

        // The control first: if these two ever pass, the four below mean nothing.
        QVERIFY2(controlVisibleAfterRawHide == false,
                 "the apparatus cannot see a hide at all - an ordinary child stayed visible when its parent was "
                 "hidden, so no verdict about the dialog can be drawn from this run");
        QVERIFY2(controlVisibleAfterTabSwitch == false,
                 "QStackedWidget::setCurrentIndex did not hide the outgoing page - the production mechanism this "
                 "probe models is not what is being exercised");

        // THE VERDICT, as measured: a child QDialog WINDOW does NOT follow its
        // parent widget's hide, on either route, and there is no sticky-hide
        // problem on the way back either.
        QCOMPARE(visibleAfterRawHide, true);
        QCOMPARE(visibleAfterRawReshow, true);
        QCOMPARE(visibleAfterTabSwitch, true);
        QCOMPARE(visibleAfterSwitchBack, true);
    }

    // =====================================================================
    // TEST-083 / TEST-084 / TEST-085 (REQ-021, DEC-garmin-030 option B) —
    // THE PRODUCTION FIX.
    //
    // TEST-082 established the fact: a dialog parented to the athlete WINDOW
    // outlives the Context, Athlete and RideItem it holds raw pointers to,
    // because MainWindow::removeAthleteTab frees all three SYNCHRONOUSLY
    // (MainWindow.cpp:2183-2185) and the window itself only ever goes through a
    // deferred deleteLater(). TEST-081 measured the one Qt behaviour option B
    // rests on and cleared it (a child QDialog WINDOW does not follow its parent
    // widget's hide, on either route).
    //
    // So the fix is in three parts and all three are tested here:
    //   PART 1  both dialogs are parented to context->tab instead of to the
    //           window, so Qt destroys them from ~QObject at `delete tab` -
    //           the FIRST of the three deletes - and every existing
    //           `QPointer<T> self(this)` bail becomes true again.
    //   PART 2  a rider of collaborator QPointers (ctx / ride) in both start()
    //           methods, for the case where the dialog is NOT a child of the
    //           tab that died and therefore survives it.
    //   PART 3  S-R021-01: the sync dialog's dirty-rides Cancel branch ran
    //           processEvents() and then invoked close() on `this` with no bail
    //           in between (the upload dialog's identical branch has one).
    // =====================================================================

  private:
    // Which of MainWindow's two teardown routes is driven. Both matter and they
    // are not the same shape: closing ONE tab leaves the window standing (so a
    // window-parented dialog survives outright), while closing the WINDOW tears
    // every tab down synchronously and only THEN defers its own deletion.
    enum CollabRoute {
        CloseOneTab,   // MainWindow::closeAthleteTab -> removeAthleteTab
        CloseTheWindow // MainWindow::closeEvent -> removeAthleteTab for every tab
    };

    // Where the teardown lands inside CloudServiceUploadDialog::start().
    enum UploadCollabFrame {
        UploadCollabOpen,      // store->open()                          (:463)
        UploadCollabDirtyExec, // the unsaved-changes QMessageBox::exec() (:485)
        UploadCollabCancelPE,  // the Cancel branch's processEvents()     (:498)
        UploadCollabWrite      // compressRide/writeFile              (:516/:520)
    };

    struct CollabOutcome
    {
        bool teardownFired = false;              // the route actually executed
        bool dialogFound = false;                // ...and there was a dialog to observe
        bool dialogParentWasTheTab = false;      // PART 1, as built by production code
        bool dialogDestroyed = false;            // the tab teardown really destroyed it
        bool dialogDiedBeforeItsContext = false; // ...and did so FIRST (the ordering)
        bool dialogGoneAtEnd = false;
        bool dialogAliveOnResume = false; // (the window-parented control's subject)
        bool contextFreed = false;        // the harness really did free them
        bool itemFreed = false;
        bool startReturnedFalse = false;
        bool callReturnedFalse = false; // upload()/start() reported failure
        bool timedOut = false;
        bool windowAliveAfterCall = false;
        int writeFileCalls = 0;
        int readFileCalls = 0;
        bool storeClosed = false;
        bool storeDestroyed = false;
    };

    // TEST-083 — a teardown of the athlete tab (or the whole window) delivered
    // into ONE of CloudServiceUploadDialog::start()'s suspension points, through
    // the REAL production entry point:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event DELIVERY, so scopeLevel is bumped, exactly as
    //        when the user picks a service from the Upload menu)
    //          -> FakeAthleteWindow + one athlete tab  [MainWindow + AthleteTab]
    //          -> CloudService::upload(window, ctx, store, item)
    //                                                  [MainWindow.cpp:2556]
    //               -> new CloudServiceUploadDialog(...)   <- PART 1 lives here
    //               -> start() -> store->open() / QMessageBox::exec() /
    //                  processEvents() / writeFile()
    //                    -> NESTED loop -> queued teardown on qApp
    //                         -> delete tab; delete item; delete athlete;
    //                            delete context;          [MainWindow.cpp:2183]
    //          -> closeAndDeleteStore(store)              [MainWindow.cpp:2563]
    //
    // upload() is called with the WINDOW as `parent`, exactly as
    // MainWindow::uploadCloud calls it, so the dialog's parent is whatever
    // production CHOOSES rather than whatever this fixture prefers.
    CollabOutcome runUploadCollabTeardown(UploadCollabFrame frame, CollabRoute route)
    {
        obs::reset();

        CollabOutcome out;
        QPointer<CloudServiceUploadDialog> dialogGuard;
        QPointer<FakeAthleteWindow> windowGuard;
        QPointer<CloudService> storeGuard;
        const void* ctxAddr = nullptr;
        const void* itemAddr = nullptr;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                windowGuard = win;
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                QWidget* const tabWidget = slot->tabWidget;
                ctxAddr = slot->contextAddr;
                itemAddr = slot->itemAddr;

                BlockingStore* store = new BlockingStore(slot->context);
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                storeGuard = store;

                switch (frame) {
                case UploadCollabOpen:
                    store->blockInOpen = true;
                    break;
                case UploadCollabWrite:
                    store->blockInWrite = true;
                    // So that the RED shape (a dialog that survived its tab and
                    // sailed on) finishes instead of sitting in exec() until the
                    // watchdog: it reports the wrong ANSWER rather than a timeout.
                    store->completeWrite = true;
                    break;
                case UploadCollabDirtyExec:
                case UploadCollabCancelPE:
                    slot->item->isdirty = true; // reaches the unsaved-changes prompt
                    break;
                }

                // The athlete tab going away, with the dialog observed on the way
                // past: who its parent is (PART 1), and - through its own
                // destroyed() signal - whether it dies while the Context it points
                // at is still addressable. That question is asked of ASan's shadow
                // map rather than by dereferencing anything.
                auto teardown = [&, win, tabWidget, route]() {
                    obs::teardownFired = true;
                    CloudServiceUploadDialog* d = win->findChild<CloudServiceUploadDialog*>();
                    out.dialogFound = (d != nullptr);
                    if (d) {
                        dialogGuard = d;
                        out.dialogParentWasTheTab = (d->parentWidget() == tabWidget);
                        QObject::connect(d, &QObject::destroyed, qApp, [&out, ctxAddr]() {
                            out.dialogDestroyed = true;
                            out.dialogDiedBeforeItsContext = !isPoisoned(ctxAddr, sizeof(Context));
                        });
                    }
                    if (route == CloseOneTab)
                        win->closeAthleteTab(0);
                    else
                        win->closeWindow();
                };

                if (frame == UploadCollabOpen || frame == UploadCollabWrite) {
                    // Delivered from inside the store call's own nested loop.
                    store->closeAction = teardown;
                } else if (frame == UploadCollabDirtyExec) {
                    // Posted before upload() runs, so the FIRST nested loop it
                    // spins - the unsaved-changes QMessageBox - delivers it.
                    // Answering SAVE deliberately: that is the branch which
                    // dereferences context, context->ride and context->mainWindow
                    // (CloudService.cpp:490-492), i.e. the exact objects the
                    // teardown just freed.
                    QMetaObject::invokeMethod(
                        qApp,
                        [teardown]() {
                            teardown();
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(QMessageBox::Save);
                        },
                        Qt::QueuedConnection);
                } else if (frame == UploadCollabCancelPE) {
                    // One frame LATER: the prompt is answered Cancel with the
                    // dialog still alive, and only then is the teardown posted -
                    // so it lands in the QApplication::processEvents() at :451.
                    QMetaObject::invokeMethod(
                        qApp,
                        [teardown]() {
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(QMessageBox::Cancel);
                            QMetaObject::invokeMethod(qApp, teardown, Qt::QueuedConnection);
                        },
                        Qt::QueuedConnection);
                }

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp: `timedOut` points into this
                // function's stack frame, and a watchdog left armed on qApp fires
                // five seconds later - inside whatever slot is running BY THEN -
                // and writes to a dead frame (ASan: stack-use-after-return). The
                // context object is destroyed when this run returns, so Qt drops
                // the pending call instead.
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                // THE PRODUCTION CALL SITE, verbatim (MainWindow.cpp:2556): the
                // window is handed in as `parent`, and everything about the
                // dialog's lifetime is upload()'s own choice from there.
                const bool accepted = CloudService::upload(win, slot->context, store, slot->item);
                out.callReturnedFalse = (accepted == false);
                out.windowAliveAfterCall = !windowGuard.isNull();

                // The caller's REQ-017 obligation, unchanged by REQ-021.
                closeAndDeleteStore(store);

                // Whatever is left of the window (the CloseOneTab route leaves it
                // standing) goes late, so nothing above is measured against a
                // window this fixture destroyed itself.
                QTimer::singleShot(300, &appLoop, [&windowGuard]() {
                    if (!windowGuard.isNull())
                        delete windowGuard.data();
                });

                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();

        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.contextFreed = isPoisoned(ctxAddr, sizeof(Context));
        out.itemFreed = isPoisoned(itemAddr, sizeof(RideItem));
        out.writeFileCalls = obs::writeFileCalls;
        out.storeClosed = obs::storeClosed;
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        return out;
    }

    // The shared verdict for TEST-083's eight (frame x route) runs. NOT a slot.
    void assertUploadDiedWithItsTab(const char* what, const CollabOutcome& out)
    {
        // Reaching this line at all means the process did not abort under ASan -
        // no use-after-free on the dialog, on `context`, on `context->ride` or on
        // the RideItem. The mutation matrix in the build report is what proves it
        // is the fix, and not luck, that achieved it.
        QVERIFY2(
            out.timedOut == false,
            qPrintable(QStringLiteral("%1: the upload never came back - a guard wedged it").arg(QLatin1String(what))));
        QVERIFY2(obs::teardownFired,
                 qPrintable(QStringLiteral("%1: the athlete teardown never ran - this run proves nothing")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogFound,
                 qPrintable(QStringLiteral("%1: CloudService::upload built no dialog under the athlete window - "
                                           "the fixture is not observing what it thinks it is")
                                .arg(QLatin1String(what))));

        // PART 1, stated as the production code's own choice of parent.
        QVERIFY2(out.dialogParentWasTheTab,
                 qPrintable(QStringLiteral("%1: CloudService::upload parented the dialog to the window it was "
                                           "handed instead of to context->tab - it will outlive the Context, "
                                           "Athlete and RideItem its suspended frames still point at (REQ-021)")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogDestroyed,
                 qPrintable(QStringLiteral("%1: tearing the athlete tab down did not destroy the dialog")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogDiedBeforeItsContext,
                 qPrintable(QStringLiteral("%1: the dialog was destroyed AFTER its Context was freed - option B's "
                                           "whole premise (deleteChildren runs at `delete tab`, before `delete "
                                           "context`) is wrong")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(
                     QStringLiteral("%1: the dialog is still alive at the end - it leaked").arg(QLatin1String(what))));

        // The premise: the teardown really did free the collaborators.
        QVERIFY2(out.contextFreed,
                 qPrintable(QStringLiteral("%1: the harness never freed the Context - it proves nothing about "
                                           "REQ-021")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.itemFreed,
                 qPrintable(QStringLiteral("%1: the harness never freed the RideItem").arg(QLatin1String(what))));

        // ...and the upload reported honestly rather than exec()ing a dialog that
        // start() had already lost.
        QVERIFY2(out.callReturnedFalse,
                 qPrintable(QStringLiteral("%1: upload() reported SUCCESS for an upload whose athlete went away "
                                           "mid-flight")
                                .arg(QLatin1String(what))));

        // The store is the CALLER's (REQ-017, MainWindow.cpp:2563), closed and
        // deleted exactly once on every route. A second free would have aborted
        // this process under ASan long before this line.
        QVERIFY2(out.storeClosed,
                 qPrintable(QStringLiteral("%1: the store was never close()d - the REQ-017 owner contract broke")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was never deleted - it leaked past the teardown")
                                .arg(QLatin1String(what))));
    }

  private slots:

    // -- TEST-083 --------------------------------------------------------
    // Every suspension point in CloudServiceUploadDialog::start(), driven from
    // both teardown routes.
    //
    // RED (before PART 1, i.e. `new CloudServiceUploadDialog(parent, ...)`):
    //   * store->open (:463)      AddressSanitizer: heap-use-after-free READ in
    //                             RideItem::isDirty (CloudService.cpp:471) - the
    //                             dialog outlived the RideItem it is uploading.
    //   * the other three         the dialog is a child of the WINDOW, so
    //                             dialogParentWasTheTab / dialogDestroyed /
    //                             dialogDiedBeforeItsContext are all false.
    void uploadDialogIsTornDownWithItsAthleteTabAtEverySuspensionPoint()
    {
        struct Case
        {
            UploadCollabFrame frame;
            CollabRoute route;
            const char* what;
            int expectedWrites;
        };
        const Case cases[] = {
            {UploadCollabOpen, CloseOneTab, "store->open (:463), one tab closed", 0},
            {UploadCollabOpen, CloseTheWindow, "store->open (:463), window closed", 0},
            {UploadCollabDirtyExec, CloseOneTab, "unsaved-changes exec (:485), one tab closed", 0},
            {UploadCollabDirtyExec, CloseTheWindow, "unsaved-changes exec (:485), window closed", 0},
            {UploadCollabCancelPE, CloseOneTab, "Cancel processEvents (:498), one tab closed", 0},
            {UploadCollabCancelPE, CloseTheWindow, "Cancel processEvents (:498), window closed", 0},
            {UploadCollabWrite, CloseOneTab, "compressRide/writeFile (:516/:520), one tab closed", 1},
            {UploadCollabWrite, CloseTheWindow, "compressRide/writeFile (:516/:520), window closed", 1},
        };

        for (const Case& c : cases) {
            const CollabOutcome out = runUploadCollabTeardown(c.frame, c.route);
            assertUploadDiedWithItsTab(c.what, out);
            if (QTest::currentTestFailed())
                return;
            QCOMPARE(out.writeFileCalls, c.expectedWrites);
            // Closing ONE tab must leave the window standing - otherwise this run
            // is the window route in disguise and proves nothing about the
            // asymmetry REQ-021 exists for.
            if (c.route == CloseOneTab)
                QVERIFY2(
                    out.windowAliveAfterCall,
                    qPrintable(
                        QStringLiteral("%1: closing one athlete tab destroyed the window").arg(QLatin1String(c.what))));
        }
    }

    // -- TEST-083 (PART 2, the rider) ------------------------------------
    // THE CASE THE REPARENT DOES NOT REACH.
    //
    // PART 1 fixes the ORDER for a dialog that is a child of the tab that died.
    // A dialog parented to anything longer-lived - the window, as it was before
    // this slice, or nothing at all when context->tab is not set - survives that
    // teardown with `self` non-null and every collaborator pointer dangling.
    // start() resumes from store->open() straight into item->isDirty()
    // (CloudService.cpp:471) and then into context / context->ride /
    // context->mainWindow (:490-492).
    //
    // So this run deliberately builds the WINDOW-parented shape and asserts the
    // dialog is STILL ALIVE on resume (otherwise it would be testing PART 1
    // again) and that start() stood down anyway.
    //
    // RED (widen nothing - `if (self.isNull()) return false;` at :464):
    //   AddressSanitizer: heap-use-after-free READ in RideItem::isDirty,
    //   CloudService.cpp:471.
    void uploadDialogThatOutlivesItsTabBailsInsteadOfUsingFreedCollaborators()
    {
        obs::reset();

        CollabOutcome out;
        QPointer<CloudServiceUploadDialog> dialogGuard;
        QPointer<CloudService> storeGuard;
        const void* ctxAddr = nullptr;
        const void* itemAddr = nullptr;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                ctxAddr = slot->contextAddr;
                itemAddr = slot->itemAddr;

                BlockingStore* store = new BlockingStore(slot->context);
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                store->blockInOpen = true;
                storeGuard = store;

                // The pre-REQ-021 shape, built on purpose: hosted by the WINDOW,
                // so the tab teardown cannot collect it.
                CloudServiceUploadDialog* uploader = new CloudServiceUploadDialog(
                    win->hostFor(FakeAthleteWindow::HostedByWindow, 0), slot->context, store, slot->item);
                uploader->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = uploader;

                store->closeAction = [&, win]() {
                    obs::teardownFired = true;
                    win->closeAthleteTab(0);
                };

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp: `timedOut` points into this
                // function's stack frame, and a watchdog left armed on qApp fires
                // five seconds later - inside whatever slot is running BY THEN -
                // and writes to a dead frame (ASan: stack-use-after-return). The
                // context object is destroyed when this run returns, so Qt drops
                // the pending call instead.
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                const bool started = uploader->start();
                out.startReturnedFalse = (started == false);
                out.callReturnedFalse = out.startReturnedFalse;
                out.dialogAliveOnResume = !dialogGuard.isNull();

                closeAndDeleteStore(store);

                QTimer::singleShot(300, qApp, [win]() { delete win; });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(out.timedOut == false, "start() never came back");
        QVERIFY2(obs::teardownFired, "the athlete teardown never ran - this run proves nothing");
        QVERIFY2(obs::openResumed, "store->open() never resumed from its nested loop - the teardown missed it");
        QVERIFY2(isPoisoned(ctxAddr, sizeof(Context)), "the harness never freed the Context");
        QVERIFY2(isPoisoned(itemAddr, sizeof(RideItem)), "the harness never freed the RideItem");

        // THE POINT: this dialog did NOT die with the tab. If it had, `self`
        // would have covered everything and the collaborator guards would be
        // untested by this run.
        QVERIFY2(out.dialogAliveOnResume,
                 "the window-parented dialog was destroyed by the tab teardown - this run is testing the "
                 "reparent, not the collaborator guards");
        QVERIFY2(out.startReturnedFalse,
                 "start() reported success after the Context and RideItem it holds were freed - it walked on "
                 "into item->isDirty() and the Save branch (CloudService.cpp:471, :490-492)");
        QCOMPARE(obs::writeFileCalls, 0);

        // A3-R021-F5 - the OTHER suspension point in this start() whose next
        // statements dereference a collaborator: the unsaved-changes prompt, and
        // the Save branch behind it. Without this run the ctx/ride half of the
        // bail at :524 is a guard no test can make fail.
        //
        // RED (revert :524 to `if (self.isNull()) return false;`):
        //   AddressSanitizer: heap-use-after-free READ in
        //   CloudServiceUploadDialog::start(), CloudService.cpp:490
        //   (context->notifyMetadataFlush() on the freed Context).
        const WindowHostedOutcome dirty = runWindowHostedDirtyPromptTeardown(/*syncDialog*/ false);
        assertDirtyPromptBailedNotCrashed("upload unsaved-changes prompt (:522), window-hosted", dirty);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(obs::writeFileCalls, 0);
    }

  private:
    // Where the teardown lands inside CloudServiceSyncDialog::start().
    enum SyncCollabFrame {
        SyncCollabOpen,       // store->open()                            (:938)
        SyncCollabDirtyExec,  // the unsaved-changes QMessageBox::exec()  (:1167)
        SyncCollabCancelPE,   // the Cancel branch's processEvents()      (:1184)
        SyncCollabTailReaddir // the tail refreshClicked() -> readdir    (:1203)
    };

    // TEST-084 — the same treatment for the SYNC dialog, whose parent is chosen
    // by its own constructor (CloudService.cpp:884) rather than by its caller,
    // and which is MODELESS and owns its store.
    //
    //   QEventLoop -> queued call -> FakeAthleteWindow + one athlete tab
    //     -> new CloudServiceSyncDialog(ctx, store)      <- PART 1 lives here
    //     -> WA_DeleteOnClose                            [AddCloudWizard.cpp:899]
    //     -> if (start()) open();                        [MainWindow::syncCloud]
    //          -> store->open() / QMessageBox::exec() / processEvents() /
    //             refreshClicked -> readdir
    //               -> NESTED loop -> queued teardown on qApp
    CollabOutcome runSyncCollabTeardown(SyncCollabFrame frame, CollabRoute route)
    {
        obs::reset();

        CollabOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<FakeAthleteWindow> windowGuard;
        QPointer<CloudService> storeGuard;
        const void* ctxAddr = nullptr;
        const void* itemAddr = nullptr;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                windowGuard = win;
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                QWidget* const tabWidget = slot->tabWidget;
                ctxAddr = slot->contextAddr;
                itemAddr = slot->itemAddr;

                BlockingStore* store = new BlockingStore(slot->context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                storeGuard = store;

                if (frame == SyncCollabOpen)
                    store->blockInOpen = true;
                if (frame == SyncCollabDirtyExec || frame == SyncCollabCancelPE)
                    slot->item->isdirty = true; // reaches the unsaved-changes prompt

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(slot->context, store);
                out.dialogFound = true;
                out.dialogParentWasTheTab = (dialog->parentWidget() == tabWidget);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = dialog;
                store->dialogGuard = dialog;
                QObject::connect(dialog, &QObject::destroyed, qApp, [&out, ctxAddr]() {
                    out.dialogDestroyed = true;
                    out.dialogDiedBeforeItsContext = !isPoisoned(ctxAddr, sizeof(Context));
                });

                auto teardown = [&, win, route]() {
                    obs::teardownFired = true;
                    if (route == CloseOneTab)
                        win->closeAthleteTab(0);
                    else
                        win->closeWindow();
                };

                if (frame == SyncCollabOpen) {
                    store->closeAction = teardown;
                } else if (frame == SyncCollabTailReaddir) {
                    store->closeAction = teardown;
                    // Armed BEFORE start(): with open() quiet, the tail
                    // refreshClicked() is the FIRST nested loop start() spins.
                    store->blockInReaddir = true;
                } else if (frame == SyncCollabDirtyExec) {
                    QMetaObject::invokeMethod(
                        qApp,
                        [teardown]() {
                            // TEST-088 — WHERE this call was delivered, recorded
                            // before it does anything. It was posted before
                            // start() was ever called, so whatever frame is on
                            // the stack now is the FIRST event-delivery
                            // opportunity start() offers. See the TEST-088 slot.
                            obs::teardownSawModal = (QApplication::activeModalWidget() != nullptr);
                            obs::teardownAfterStart = obs::startReturned;
                            teardown();
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(QMessageBox::SaveAll);
                        },
                        Qt::QueuedConnection);
                } else if (frame == SyncCollabCancelPE) {
                    // PART 3 (S-R021-01). The prompt is answered CANCEL with the
                    // dialog still alive, and the teardown is posted only then -
                    // so it is delivered by the QApplication::processEvents() at
                    // :1107, one statement before the queued close() on `this`.
                    QMetaObject::invokeMethod(
                        qApp,
                        [teardown]() {
                            if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                                m->done(QMessageBox::Cancel);
                            QMetaObject::invokeMethod(qApp, teardown, Qt::QueuedConnection);
                        },
                        Qt::QueuedConnection);
                }

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp: `timedOut` points into this
                // function's stack frame, and a watchdog left armed on qApp fires
                // five seconds later - inside whatever slot is running BY THEN -
                // and writes to a dead frame (ASan: stack-use-after-return). The
                // context object is destroyed when this run returns, so Qt drops
                // the pending call instead.
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                // VERBATIM the production syncCloud call site (MainWindow.cpp,
                // DEC-027). Reading start()'s bool is safe after `this` is freed;
                // touching the dialog past a false is not, so we do not.
                const bool started = dialog->start();
                obs::startReturned = true; // TEST-088's ordering channel
                out.startReturnedFalse = (started == false);
                out.callReturnedFalse = out.startReturnedFalse;
                if (started)
                    dialog->open();
                out.windowAliveAfterCall = !windowGuard.isNull();

                // NOTE: no closeAndDeleteStore here - this dialog OWNS its store
                // (REQ-017 (e)) and DEC-025 declines the deletion while a
                // blocking call is on the stack, so it is deliberately leaked.
                QTimer::singleShot(300, &appLoop, [&windowGuard]() {
                    if (!windowGuard.isNull())
                        delete windowGuard.data();
                });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.contextFreed = isPoisoned(ctxAddr, sizeof(Context));
        out.itemFreed = isPoisoned(itemAddr, sizeof(RideItem));
        out.readFileCalls = obs::readFileCalls;
        out.storeClosed = obs::storeClosed;
        out.storeDestroyed = storeGuard.isNull() && obs::storeDestroyed;
        return out;
    }

    // The shared verdict for TEST-084's runs. NOT a slot.
    void assertSyncDiedWithItsTab(const char* what, const CollabOutcome& out)
    {
        QVERIFY2(
            out.timedOut == false,
            qPrintable(QStringLiteral("%1: the sync never came back - a guard wedged it").arg(QLatin1String(what))));
        QVERIFY2(obs::teardownFired,
                 qPrintable(QStringLiteral("%1: the athlete teardown never ran - this run proves nothing")
                                .arg(QLatin1String(what))));

        QVERIFY2(out.dialogParentWasTheTab,
                 qPrintable(QStringLiteral("%1: CloudServiceSyncDialog parented itself to context->mainWindow "
                                           "instead of to context->tab - it will outlive the Context its "
                                           "modeless slots dereference (REQ-021)")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogDestroyed,
                 qPrintable(QStringLiteral("%1: tearing the athlete tab down did not destroy the dialog")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogDiedBeforeItsContext,
                 qPrintable(QStringLiteral("%1: the dialog was destroyed AFTER its Context was freed - option B's "
                                           "premise is wrong")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(
                     QStringLiteral("%1: the dialog is still alive at the end - it leaked").arg(QLatin1String(what))));
        QVERIFY2(out.contextFreed,
                 qPrintable(QStringLiteral("%1: the harness never freed the Context").arg(QLatin1String(what))));
        QVERIFY2(out.startReturnedFalse,
                 qPrintable(QStringLiteral("%1: start() returned true after the dialog was destroyed - syncCloud "
                                           "would open() a freed dialog")
                                .arg(QLatin1String(what))));

        // DEC-025 is untouched by REQ-021: a blocking call is on the stack for
        // the whole of start(), so the destructor DECLINES to close or delete the
        // store and it is leaked on purpose.
        // DEC-garmin-031 (REQ-022) — INVERTED, and this is the slot where the
        // inversion matters most: REQ-021 is what made these very runs (an
        // athlete TAB closing, with the application still running afterwards)
        // reachable, and so turned DEC-025's once-per-exit leak into a
        // per-close accumulating one.
        QVERIFY2(out.storeDestroyed,
                 qPrintable(QStringLiteral("%1: the store was never deleted - closing this athlete tab stranded "
                                           "it, and every subsequent close strands another (DEC-031)")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.storeClosed,
                 qPrintable(QStringLiteral("%1: the store was never close()d - for GarminConnect that is a live "
                                           "OS thread and a resident interpreter session left behind by an "
                                           "athlete-tab close")
                                .arg(QLatin1String(what))));
        assertStoreReapedExactlyOnceAfterTheNestedLoop(what);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(out.readFileCalls, 0);
    }

  private slots:

    // -- TEST-084 --------------------------------------------------------
    // Every suspension point in CloudServiceSyncDialog::start(), both routes.
    //
    // RED (before PART 1, i.e. `QDialog(context->mainWindow, Qt::Dialog)`): the
    // dialog is a child of the window, so dialogParentWasTheTab,
    // dialogDestroyed and dialogDiedBeforeItsContext are false; and on the
    // SyncCollabOpen frame start() also walks on into
    // `context->athlete->cyclist` (CloudService.cpp:987) on the freed Context ->
    // AddressSanitizer: heap-use-after-free.
    void syncDialogIsTornDownWithItsAthleteTabAtEverySuspensionPoint()
    {
        struct Case
        {
            SyncCollabFrame frame;
            CollabRoute route;
            const char* what;
        };
        const Case cases[] = {
            {SyncCollabOpen, CloseOneTab, "store->open (:938), one tab closed"},
            {SyncCollabOpen, CloseTheWindow, "store->open (:938), window closed"},
            {SyncCollabDirtyExec, CloseOneTab, "unsaved-changes exec (:1167), one tab closed"},
            {SyncCollabDirtyExec, CloseTheWindow, "unsaved-changes exec (:1167), window closed"},
            {SyncCollabTailReaddir, CloseOneTab, "tail refreshClicked (:1203), one tab closed"},
            {SyncCollabTailReaddir, CloseTheWindow, "tail refreshClicked (:1203), window closed"},
        };

        for (const Case& c : cases) {
            const CollabOutcome out = runSyncCollabTeardown(c.frame, c.route);
            assertSyncDiedWithItsTab(c.what, out);
            if (QTest::currentTestFailed())
                return;
            if (c.route == CloseOneTab)
                QVERIFY2(
                    out.windowAliveAfterCall,
                    qPrintable(
                        QStringLiteral("%1: closing one athlete tab destroyed the window").arg(QLatin1String(c.what))));
        }
    }

    // -- TEST-084 (PART 3, S-R021-01) ------------------------------------
    // THE ONE-LINE GAP THIS SLICE ALSO CLOSES.
    //
    // start()'s dirty-rides CANCEL branch ran
    //     QApplication::processEvents();                       (:1184)
    //     QMetaObject::invokeMethod(this, "close", ...);       (:1191)
    // with NO `if (self.isNull()) return false;` between them, while
    // CloudServiceUploadDialog::start()'s structurally identical branch has
    // carried one since DEC-029 (:498-499). processEvents() is an event-delivery
    // frame like any other: the athlete teardown lands there and destroys the
    // dialog, and the very next statement invokes a method on it.
    //
    // Kept as a slot of its own, not folded into the matrix above, because it is
    // shipped code (ae5a7a8ab) on the `this` axis and must be able to fail on its
    // own terms.
    //
    // RED (remove the new bail): AddressSanitizer: heap-use-after-free READ in
    // QMetaObject::invokeMethod / QObject::metaObject, CloudService.cpp:1191.
    void syncDialogCancelBranchDoesNotInvokeCloseOnAFreedDialog()
    {
        const CollabOutcome oneTab = runSyncCollabTeardown(SyncCollabCancelPE, CloseOneTab);
        assertSyncDiedWithItsTab("dirty-rides Cancel processEvents (:1184), one tab closed", oneTab);
        if (QTest::currentTestFailed())
            return;

        const CollabOutcome window = runSyncCollabTeardown(SyncCollabCancelPE, CloseTheWindow);
        assertSyncDiedWithItsTab("dirty-rides Cancel processEvents (:1184), window closed", window);
    }

    // -- TEST-084 (PART 2, the rider) ------------------------------------
    // The sync dialog's half of the collaborator guards, in the shape the
    // reparent does not reach: a dialog hosted by something longer-lived than
    // the tab whose Context it holds.
    //
    // RED (widen nothing - `if (self.isNull()) return false;` at :939): start()
    // resumes from store->open() and builds its widgets, reaching
    // `context->athlete->cyclist` (CloudService.cpp:987) on a freed Context ->
    // AddressSanitizer: heap-use-after-free.
    void syncDialogThatOutlivesItsTabBailsInsteadOfUsingAFreedContext()
    {
        obs::reset();

        CollabOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        const void* ctxAddr = nullptr;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                ctxAddr = slot->contextAddr;

                BlockingStore* store = new BlockingStore(slot->context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                store->blockInOpen = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(slot->context, store);
                // The pre-REQ-021 shape, built on purpose (the same explicit
                // re-parent runParentTeardown makes): hosted by the WINDOW, so the
                // tab teardown cannot collect it.
                dialog->setParent(win->hostFor(FakeAthleteWindow::HostedByWindow, 0), Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = dialog;
                store->dialogGuard = dialog;

                store->closeAction = [&, win]() {
                    obs::teardownFired = true;
                    win->closeAthleteTab(0);
                };

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp: `timedOut` points into this
                // function's stack frame, and a watchdog left armed on qApp fires
                // five seconds later - inside whatever slot is running BY THEN -
                // and writes to a dead frame (ASan: stack-use-after-return). The
                // context object is destroyed when this run returns, so Qt drops
                // the pending call instead.
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                const bool started = dialog->start();
                out.startReturnedFalse = (started == false);
                out.dialogAliveOnResume = !dialogGuard.isNull();
                if (started)
                    dialog->open();

                QTimer::singleShot(300, qApp, [win]() { delete win; });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(out.timedOut == false, "start() never came back");
        QVERIFY2(obs::teardownFired, "the athlete teardown never ran - this run proves nothing");
        QVERIFY2(obs::openResumed, "store->open() never resumed from its nested loop - the teardown missed it");
        QVERIFY2(isPoisoned(ctxAddr, sizeof(Context)), "the harness never freed the Context");
        QVERIFY2(out.dialogAliveOnResume,
                 "the window-parented dialog was destroyed by the tab teardown - this run is testing the "
                 "reparent, not the collaborator guard");
        QVERIFY2(out.startReturnedFalse,
                 "start() returned true after the Context it points at was freed - it went on to build its "
                 "widgets from context->athlete (CloudService.cpp:987, :1147)");
        QCOMPARE(obs::readFileCalls, 0);

        // A3-R021-F5 - the OTHER suspension point in this start() whose next
        // statements dereference a collaborator: the unsaved-changes prompt, and
        // the SaveAll branch behind it. Without this run the ctx half of the bail
        // at :1230 is a guard no test can make fail.
        //
        // RED (revert :1230 to `if (self.isNull()) return false;`):
        //   AddressSanitizer: heap-use-after-free READ in
        //   CloudServiceSyncDialog::start(), CloudService.cpp:1231
        //   (context->notifyMetadataFlush() on the freed Context).
        const WindowHostedOutcome dirty = runWindowHostedDirtyPromptTeardown(/*syncDialog*/ true);
        assertDirtyPromptBailedNotCrashed("sync unsaved-changes prompt (:1230), window-hosted", dirty);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(obs::readFileCalls, 0);
    }

    // -- TEST-086 (A3-R021-F2) -------------------------------------------
    // THE SECOND SUSPENSION POINT ON THE SAME AXIS - the one the rider missed.
    //
    // The slot above catches the teardown inside store->open(), where start()'s
    // OWN bail (:939) is the guard. start() has a second nested loop AFTER it, in
    // a function of its own: the tail refreshClicked() (:1203) runs
    // store->readdir (GarminConnect::blockingList). refreshClicked's post-readdir
    // guard was `self`-ONLY (:1392), and eleven lines later it walks
    //
    //     foreach(RideItem *item, context->athlete->rideCache->rides())   :1401
    //
    // So the window-parented shape - the very shape the collaborator rider exists
    // for - resumed with `self` alive and `context` freed and dereferenced it.
    // start()'s own `ctx` bail could not help: it sat AFTER the call (:1204), i.e.
    // after refreshClicked had already returned from walking a dead RideCache.
    //
    // This is the sibling slot with ONE variable changed - the suspension point
    // is readdir rather than open - and the premise assertion moved to the
    // observation readdir publishes (obs::resumedAfterNestedLoop; open() records
    // obs::openResumed instead).
    //
    // RED (before the fix, i.e. `if (self.isNull()) return;` at :1392):
    //   AddressSanitizer: heap-use-after-free READ of size 8
    //     #0 CloudServiceSyncDialog::refreshClicked() CloudService.cpp:1401
    //     #1 CloudServiceSyncDialog::start() CloudService.cpp:1203
    void syncDialogThatOutlivesItsTabBailsInsteadOfWalkingAFreedRideCache()
    {
        obs::reset();

        CollabOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        const void* ctxAddr = nullptr;
        // B-R021-12 — see the assertions at the end of this slot.
        bool visibleWhenStartBailed = false;
        bool aliveBeforeTheWindowWent = false;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                ctxAddr = slot->contextAddr;

                BlockingStore* store = new BlockingStore(slot->context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                store->closeActionContext = qApp;
                // THE ONE DIFFERENCE from the slot above: open() returns straight
                // away and it is readdir - reached from start()'s tail
                // refreshClicked() - that suspends. That is also what makes this
                // slot the natural home for B-R021-12 below: the tail
                // refreshClicked() is the LAST suspension point in start(), so
                // its ctx bail is the one whose `return false` reaches the caller
                // with the dialog fully built and already shown.
                store->blockInReaddir = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(slot->context, store);
                // The pre-REQ-021 shape, built on purpose: hosted by the WINDOW,
                // so the tab teardown cannot collect it.
                dialog->setParent(win->hostFor(FakeAthleteWindow::HostedByWindow, 0), Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = dialog;
                store->dialogGuard = dialog;

                store->closeAction = [&, win]() {
                    obs::teardownFired = true;
                    win->closeAthleteTab(0);
                };

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp - see the note on the slot
                // above (a watchdog left armed on qApp writes to a dead frame).
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                const bool started = dialog->start();
                out.startReturnedFalse = (started == false);
                out.dialogAliveOnResume = !dialogGuard.isNull();
                if (started)
                    dialog->open();

                // B-R021-12 — THE ORPHAN THIS BAIL USED TO LEAVE BEHIND. start()
                // show()s this dialog at :1204, well before the tail
                // refreshClicked() whose ctx bail has just fired, so what the
                // `return false` leaves standing is VISIBLE. Recorded here and
                // not asserted later, because with the fix in place the dialog is
                // gone a moment afterwards and the premise becomes unprovable.
                visibleWhenStartBailed = !dialogGuard.isNull() && dialogGuard->isVisible();

                // ...read again BEFORE `delete win`, which collects the dialog as
                // a child and would make the reading say "gone" either way.
                bool* alivePtr = &aliveBeforeTheWindowWent;
                QPointer<CloudServiceSyncDialog>* guardPtr = &dialogGuard;
                QTimer::singleShot(200, &appLoop, [alivePtr, guardPtr]() { *alivePtr = !guardPtr->isNull(); });

                QTimer::singleShot(300, qApp, [win]() { delete win; });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(out.timedOut == false, "start() never came back");
        QVERIFY2(obs::teardownFired, "the athlete teardown never ran - this run proves nothing");
        QVERIFY2(obs::resumedAfterNestedLoop,
                 "store->readdir() never resumed from its nested loop - the teardown missed it");
        QVERIFY2(isPoisoned(ctxAddr, sizeof(Context)), "the harness never freed the Context");

        // THE PREMISE, measured at the instant readdir resumed rather than
        // afterwards: this dialog did NOT die with the tab, so `self` covers
        // nothing here and only a collaborator guard can.
        QVERIFY2(obs::dialogAliveOnResume,
                 "the window-parented dialog was destroyed by the tab teardown - this run is testing the "
                 "reparent, not the collaborator guard");
        QVERIFY2(out.dialogAliveOnResume, "the dialog was gone by the time start() returned");

        // refreshClicked stood down instead of walking context->athlete->
        // rideCache->rides() (:1401), and start() reported that failure to
        // syncCloud rather than open()ing a dialog whose Context is freed.
        QVERIFY2(out.startReturnedFalse,
                 "start() returned true after the Context it points at was freed - refreshClicked walked "
                 "context->athlete->rideCache->rides() (CloudService.cpp:1401)");
        QCOMPARE(obs::readFileCalls, 0);

        // ---- B-R021-12 -------------------------------------------------
        // `self` survived and `ctx` did not, so start()'s tail bail is reached
        // with `this` ALIVE - the one exit from this function where that is
        // true and nothing used to be done about it. Both callers are
        // `if (start()) open();` with no else, so a bare `return false` leaves a
        // VISIBLE, modeless, WA_DeleteOnClose dialog holding a dangling
        // `context`, and the next click on its Refresh button walks freed
        // memory. Every other failure exit that can leave a live dialog behind
        // already posts a queued close (the open-failure branch at :1021, and
        // DEC-029's upload-failure branch at :594).
        //
        // RED (delete the queued close from start()'s tail): visible=1, and the
        // dialog is still alive 200ms later - the orphan.
        QVERIFY2(visibleWhenStartBailed,
                 "the dialog was not visible when start() bailed - the premise of B-R021-12 (start() show()s at "
                 ":1204, before this bail) is wrong and this assertion proves nothing");
        QVERIFY2(aliveBeforeTheWindowWent == false,
                 "start() bailed on a freed Context and left the dialog standing: a visible, modeless, "
                 "WA_DeleteOnClose sync dialog whose `context` is dangling, which a Refresh click then walks "
                 "(B-R021-12). It must post the same queued close() the other failure exits do");
    }

  private:
    // =====================================================================
    // TEST-087 (A3-R021-F1) — THE MODELESS COMPLETION SLOTS.
    //
    // Everything above is about start(): one frame, entered once, and guarded.
    // The sync dialog spends the REST of its life in four slots that each run
    //
    //     QApplication::processEvents();          <- an event-delivery frame
    //     <a call on `this`>                      <- with nothing in between
    //
    // and none of them held a QPointer. Before REQ-021 that was very nearly
    // unreachable: the dialog was a child of MainWindow, single-tab close does
    // not destroy MainWindow, and a whole-window close destroys it only through a
    // posted DeferredDelete, which processEvents() does not deliver at that loop
    // level. The reparent changed exactly that - the dialog is now a child of
    // context->tab, and MainWindow::removeAthleteTab runs `delete tab`
    // SYNCHRONOUSLY from inside mouse/menu event delivery, which is precisely
    // what processEvents() DOES deliver.
    //
    // So this is the reparent's own bill: a fix that shortens a lifetime enlarges
    // the set of frames that can be destroyed under themselves.
    // =====================================================================

    // Which completion slot the athlete teardown is delivered into.
    enum CompletionFrame {
        CompletedReadPE,  // completedRead's processEvents  (:2007) -> syncNext()  (:2009)
        FailedReadPE,     // failedRead's                   (:2054) -> syncNext()  (:2056)
        CompletedWritePE, // completedWrite's               (:2143) -> syncNext()  (:2145)
        UploadNextParsePE // uploadNext's parse-failure     (:2101) -> the loop's
                          //   own `rideListUp->invisibleRootItem()`  (:2065)
    };

    struct CompletionOutcome
    {
        bool teardownFired = false;      // the route actually executed
        bool teardownInsideSlot = false; // ...and landed while the slot was running
        bool dialogGoneAtEnd = false;    // the tab teardown really did destroy it
        bool timedOut = false;
        int upListCount = 0;   // (UploadNextParsePE premise: there was a row to fail on)
        int readFileCalls = 0; // did the sync carry on behind a destroyed dialog?
    };

    // The three tree widgets are private members reparented into their tab pages,
    // so they are identified the way a user would: by the column header the
    // dialog gave them (CloudService.cpp:1011 / :1038 / :1077).
    static QTreeWidget* rideListWithHeader(QWidget* dialog, const QString& column1)
    {
        const QList<QTreeWidget*> lists = dialog->findChildren<QTreeWidget*>();
        for (QTreeWidget* list : lists)
            if (list->headerItem() != nullptr && list->headerItem()->text(1) == column1)
                return list;
        return nullptr;
    }

    // One run: a REAL sync dialog, parented by its own constructor to the athlete
    // tab (the REQ-021 shape), driven into one completion slot with the athlete
    // teardown queued directly behind it so that the slot's own processEvents()
    // is what delivers it.
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call  [event delivery: scopeLevel bumped]
    //          -> FakeAthleteWindow + one athlete tab
    //          -> CloudServiceSyncDialog(ctx, store) + WA_DeleteOnClose
    //          -> start() / open() / select all / Download   [the user's sync]
    //          -> queued: the completion slot          }  posted back to back, so
    //          -> queued: the athlete teardown         }  the slot's processEvents
    //                                                     delivers the teardown
    //
    // The completion slot is invoked DIRECTLY rather than through
    // store->notifyReadComplete(). Both are public slots reached from event
    // dispatch, and the connection is a direct one, so the frames under test are
    // identical - but emitting from the store would put the STORE on the stack
    // too, and this dialog OWNS its store and deletes it in its destructor, which
    // would add a second, unrelated free to a run whose subject is `this`.
    CompletionOutcome runCompletionSlotTeardown(CompletionFrame frame)
    {
        obs::reset();

        CompletionOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QEventLoop appLoop;
        bool inSlot = false;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                // The Upload list is built from the athlete's RideCache and
                // filtered by the dialog's default one-month window
                // (CloudService.cpp:1526+), so the fixture ride needs a date
                // inside that window to appear there at all. Not "now to the
                // second" on a round hour, so it cannot collide with one of the
                // cloud entries and be marked as already uploaded.
                slot->item->dateTime = QDateTime::currentDateTime().addDays(-1);

                BlockingStore* store = new BlockingStore(slot->context);
                store->entryNames = threeActivities();
                store->blockingMs = 5;       // nothing suspends in THIS scenario
                store->completeRead = false; // ...and no completion arrives by itself
                store->closeActionContext = qApp;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(slot->context, store);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = dialog;
                store->dialogGuard = dialog;

                auto teardown = [&, win]() {
                    obs::teardownFired = true;
                    out.teardownInsideSlot = inSlot;
                    win->closeAthleteTab(0);
                };

                bool* timedOut = &out.timedOut;
                // Scoped to `appLoop`, NOT to qApp - see the note on
                // runUploadCollabTeardown (a watchdog left armed on qApp fires
                // inside a later slot and writes to a dead frame).
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                if (dialog->start())
                    dialog->open();

                if (frame == UploadNextParsePE) {
                    // The Upload tab, picked as the user picks it, then Select
                    // All and Upload. The fixture's ride file does not exist on
                    // disk, so openRideFile fails and uploadNext takes its
                    // parse-failure branch (CloudService.cpp:2099-2102).
                    if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                        tabs->setCurrentIndex(1);
                    dialog->selectAllUpChanged(Qt::Checked);
                    if (QTreeWidget* up = rideListWithHeader(dialog, QStringLiteral("File")))
                        out.upListCount = up->invisibleRootItem()->childCount();

                    QMetaObject::invokeMethod(qApp, teardown, Qt::QueuedConnection);
                    inSlot = true;
                    dialog->downloadClicked(); // -> uploadNext()
                    inSlot = false;
                } else {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    // -> syncNext(): listindex=1 and one store->readFile, which
                    // (completeRead off) reports nothing back.
                    dialog->downloadClicked();

                    QMetaObject::invokeMethod(
                        qApp,
                        [&, dialog, frame]() {
                            inSlot = true;
                            switch (frame) {
                            case CompletedReadPE:
                                dialog->completedRead(new QByteArray, threeActivities().at(0),
                                                      QStringLiteral("Completed."));
                                break;
                            case FailedReadPE:
                                dialog->failedRead(new QByteArray, threeActivities().at(0),
                                                   QStringLiteral("service refused"));
                                break;
                            case CompletedWritePE:
                                dialog->completedWrite(threeActivities().at(0), QStringLiteral("Completed."));
                                break;
                            case UploadNextParsePE:
                                break; // handled above
                            }
                            inSlot = false;
                        },
                        Qt::QueuedConnection);
                    QMetaObject::invokeMethod(qApp, teardown, Qt::QueuedConnection);
                }

                QTimer::singleShot(300, qApp, [win]() { delete win; });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.teardownFired = obs::teardownFired;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.readFileCalls = obs::readFileCalls;
        return out;
    }

    // The shared verdict for TEST-087's four runs. NOT a slot.
    void assertCompletionSlotStoodDown(const char* what, const CompletionOutcome& out)
    {
        QVERIFY2(out.timedOut == false,
                 qPrintable(QStringLiteral("%1: the run never came back").arg(QLatin1String(what))));
        QVERIFY2(out.teardownFired,
                 qPrintable(QStringLiteral("%1: the athlete teardown never ran - this run proves nothing")
                                .arg(QLatin1String(what))));
        // THE PREMISE. If the teardown landed anywhere but inside the slot, the
        // frame this run exists to test was never suspended and reaching the end
        // of it says nothing at all.
        QVERIFY2(out.teardownInsideSlot,
                 qPrintable(QStringLiteral("%1: the teardown was not delivered inside the slot's "
                                           "processEvents() - this run proves nothing")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: closing the athlete tab did not destroy the dialog - the "
                                           "REQ-021 reparent is not in force and this run proves nothing")
                                .arg(QLatin1String(what))));
    }

    // =====================================================================
    // A3-R021-F5 — THE SECOND COLLABORATOR-DEREFERENCING SUSPENSION POINT.
    //
    // The part-2 slots above catch a teardown inside store->open(), which is the
    // FIRST place either start() can be suspended while `context`/`item` are
    // freed under a dialog that survives. Each start() has a SECOND: the
    // unsaved-changes QMessageBox::exec(), whose Save/SaveAll branch is the only
    // other code in either function that dereferences a collaborator rather than
    // `this` (CloudService.cpp:490-492 and :1171-1177 - notifyMetadataFlush,
    // context->ride->notifyRideMetadataChanged, mainWindow->saveSilent).
    //
    // Without these two runs the `ctx`/`ride` half of those two bails is
    // unprovable, and REQ-021's criterion requires every guard to be shown
    // load-bearing. With them, every `ctx`/`ride` widening left in the tree is.
    //
    // The teardown is posted as a queued call BEFORE start() is entered, which
    // TEST-088 measured lands in the prompt's exec() and nowhere earlier.
    // =====================================================================
    struct WindowHostedOutcome
    {
        bool teardownFired = false;
        bool sawPrompt = false;           // it landed with the prompt up
        bool dialogAliveOnResume = false; // ...and the dialog outlived the tab
        bool startReturnedFalse = false;
        bool contextFreed = false;
        bool itemFreed = false;
        bool timedOut = false;
    };

    WindowHostedOutcome runWindowHostedDirtyPromptTeardown(bool syncDialog)
    {
        obs::reset();

        WindowHostedOutcome out;
        QPointer<QDialog> dialogGuard;
        const void* ctxAddr = nullptr;
        const void* itemAddr = nullptr;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                ctxAddr = slot->contextAddr;
                itemAddr = slot->itemAddr;
                slot->item->isdirty = true; // so the walk finds it and prompts

                BlockingStore* store = new BlockingStore(slot->context);
                store->entryNames = threeActivities();
                store->blockingMs = 5; // open() does not suspend in this run
                store->closeActionContext = qApp;

                QDialog* dialog = nullptr;
                if (syncDialog) {
                    CloudServiceSyncDialog* d = new CloudServiceSyncDialog(slot->context, store);
                    // The pre-REQ-021 shape, built on purpose: hosted by the
                    // WINDOW, so the tab teardown cannot collect it.
                    d->setParent(win->hostFor(FakeAthleteWindow::HostedByWindow, 0), Qt::Dialog);
                    store->dialogGuard = d;
                    dialog = d;
                } else {
                    dialog = new CloudServiceUploadDialog(win->hostFor(FakeAthleteWindow::HostedByWindow, 0),
                                                          slot->context, store, slot->item);
                }
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialogGuard = dialog;

                // Posted BEFORE start(): the first frame that can deliver it is
                // the unsaved-changes prompt (TEST-088).
                QMetaObject::invokeMethod(
                    qApp,
                    [&, win, syncDialog]() {
                        obs::teardownFired = true;
                        out.sawPrompt = (QApplication::activeModalWidget() != nullptr);
                        win->closeAthleteTab(0);
                        // SAVE deliberately - that is the branch which walks the
                        // collaborators the teardown has just freed.
                        if (QDialog* m = qobject_cast<QDialog*>(QApplication::activeModalWidget()))
                            m->done(syncDialog ? QMessageBox::SaveAll : QMessageBox::Save);
                    },
                    Qt::QueuedConnection);

                bool* timedOut = &out.timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOut]() {
                    *timedOut = true;
                    QCoreApplication::exit(1);
                });

                const bool started = syncDialog ? static_cast<CloudServiceSyncDialog*>(dialog)->start()
                                                : static_cast<CloudServiceUploadDialog*>(dialog)->start();
                out.startReturnedFalse = (started == false);
                out.dialogAliveOnResume = !dialogGuard.isNull();

                if (!syncDialog)
                    closeAndDeleteStore(store); // the REQ-017 owner contract

                QTimer::singleShot(300, qApp, [win]() { delete win; });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        out.teardownFired = obs::teardownFired;
        out.contextFreed = isPoisoned(ctxAddr, sizeof(Context));
        out.itemFreed = isPoisoned(itemAddr, sizeof(RideItem));
        return out;
    }

    void assertDirtyPromptBailedNotCrashed(const char* what, const WindowHostedOutcome& out)
    {
        QVERIFY2(out.timedOut == false,
                 qPrintable(QStringLiteral("%1: start() never came back").arg(QLatin1String(what))));
        QVERIFY2(out.teardownFired,
                 qPrintable(QStringLiteral("%1: the athlete teardown never ran - this run proves nothing")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.sawPrompt,
                 qPrintable(QStringLiteral("%1: the teardown was not delivered inside the unsaved-changes "
                                           "prompt - this run proves nothing")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.contextFreed,
                 qPrintable(QStringLiteral("%1: the harness never freed the Context").arg(QLatin1String(what))));
        QVERIFY2(out.itemFreed,
                 qPrintable(QStringLiteral("%1: the harness never freed the RideItem").arg(QLatin1String(what))));
        QVERIFY2(out.dialogAliveOnResume,
                 qPrintable(QStringLiteral("%1: the window-hosted dialog was destroyed by the tab teardown - "
                                           "this run is testing the reparent, not the collaborator guard")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.startReturnedFalse,
                 qPrintable(QStringLiteral("%1: start() reported success after answering Save on an athlete "
                                           "that no longer exists - it walked context->notifyMetadataFlush(), "
                                           "context->ride and context->mainWindow on freed memory")
                                .arg(QLatin1String(what))));
    }

  private slots:

    // -- TEST-087 (A3-R021-F1) -------------------------------------------
    // Reaching the end of each of these runs at all means the process did not
    // abort under ASan.
    //
    // RED (drop the new self-bail from the slot named):
    //   * completedRead   AddressSanitizer: heap-use-after-free READ in
    //                     CloudServiceSyncDialog::completedRead, CloudService.cpp
    //                     :2009 (`sync`, one statement after processEvents)
    //   * failedRead      ...the same at :2056
    //   * completedWrite  ...the same at :2145
    //   * uploadNext      ...at :2065, the for-loop's own
    //                     rideListUp->invisibleRootItem() on the next iteration
    void completionSlotsStandDownWhenTheAthleteTabDiesInTheirProcessEvents()
    {
        const CompletionOutcome read = runCompletionSlotTeardown(CompletedReadPE);
        assertCompletionSlotStoodDown("completedRead processEvents (:2007)", read);
        if (QTest::currentTestFailed())
            return;
        // The sync did not carry on downloading activity 2 behind a dialog that
        // no longer exists: exactly one read was ever asked for.
        QCOMPARE(read.readFileCalls, 1);

        const CompletionOutcome failed = runCompletionSlotTeardown(FailedReadPE);
        assertCompletionSlotStoodDown("failedRead processEvents (:2054)", failed);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(failed.readFileCalls, 1);

        const CompletionOutcome wrote = runCompletionSlotTeardown(CompletedWritePE);
        assertCompletionSlotStoodDown("completedWrite processEvents (:2143)", wrote);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(wrote.readFileCalls, 1);

        const CompletionOutcome upload = runCompletionSlotTeardown(UploadNextParsePE);
        assertCompletionSlotStoodDown("uploadNext parse-failure processEvents (:2101)", upload);
        if (QTest::currentTestFailed())
            return;
        // The premise for this one: there WAS a row in the upload list to fail
        // on, so uploadNext really did enter its loop body.
        QVERIFY2(upload.upListCount > 0,
                 "the upload list was empty - uploadNext never entered its loop and this run proves nothing");
    }

    // -- TEST-088 (A3-R021-F6) -------------------------------------------
    // THE FIFTH "SUSPENSION POINT" THAT IS NOT ONE.
    //
    // REQ-021's elaborated acceptance names five points in the sync path at which
    // an athlete teardown must be survivable. Four are driven by
    // syncDialogIsTornDownWithItsAthleteTabAtEverySuspensionPoint and
    // syncDialogCancelBranchDoesNotInvokeCloseOnAFreedDialog. The fifth - the
    // walk of context->athlete->rideCache->rides() at CloudService.cpp:1147 - has
    // no enumerator, because a teardown CANNOT be delivered into it:
    //
    //   * RideCache::rides() returns a reference to a member QVector
    //     (RideCache.h:93) and RideItem::isDirty() returns a bool member
    //     (RideItem.h:215). The loop body calls nothing that could run an event
    //     loop, so there is no frame there to suspend.
    //   * everything between the previous suspension point (store->open(), :938)
    //     and the walk is widget construction plus QWidget::show(), none of which
    //     drains the posted-event queue.
    //
    // :1147 is therefore a DEREFERENCE site, not a suspension point, and what it
    // needs is not a bail of its own but that the last bail before it holds -
    // which is what syncDialogThatOutlivesItsTabBailsInsteadOfUsingAFreedContext
    // proves by mutation (drop `ctx` from the guard at :939 and the run aborts in
    // this very neighbourhood).
    //
    // Rather than assert that in prose, this measures it. The DirtyExec run posts
    // its teardown as a QUEUED call BEFORE start() is entered, so the frame that
    // is on the stack when it finally runs IS the first event-delivery
    // opportunity start() offers after open(). If anything in the widget build or
    // in the walk could deliver it, it would have been delivered THERE, with no
    // modal window up. It is delivered with the unsaved-changes QMessageBox
    // (:1167) up instead - which also proves the walk RAN, since that prompt only
    // appears when the walk found the dirty ride.
    void nothingCanBeDeliveredIntoTheRideCacheWalkBeforeTheDirtyRidesPrompt()
    {
        const CollabOutcome out = runSyncCollabTeardown(SyncCollabDirtyExec, CloseOneTab);
        assertSyncDiedWithItsTab("ride-cache walk delivery probe (:1147)", out);
        if (QTest::currentTestFailed())
            return;

        QVERIFY2(obs::teardownAfterStart == false,
                 "the teardown was not delivered inside start() at all - this run measures nothing");
        QVERIFY2(obs::teardownSawModal,
                 "the teardown queued before start() was delivered with NO modal window up - something "
                 "between store->open() (:938) and the unsaved-changes prompt (:1167) drains the posted-event "
                 "queue, so the ride-cache walk at :1147 IS suspendable and needs a guard of its own "
                 "(A3-R021-F6)");
    }

    // =====================================================================
    // TEST-085 (a) — THE STRUCTURAL CONTROL DEC-030 REQUIRES: switching athlete
    // tabs must not hide the modeless sync dialog.
    //
    // TEST-081 measured this on a bare QDialog; this asserts it of the REAL
    // CloudServiceSyncDialog, parented by its REAL constructor, shown by the
    // REAL start()/open() sequence, and switched with the REAL mechanism
    // (QStackedWidget::setCurrentIndex, which is what MainWindow::switchAthleteTab
    // uses at MainWindow.cpp:2389).
    // =====================================================================
    void switchingAthleteTabsLeavesTheModelessSyncDialogVisible()
    {
        QWidget window; // MainWindow
        QStackedWidget* tabStack = new QStackedWidget(&window);
        QWidget* tabA = new QWidget; // the athlete tab the dialog parents to
        QWidget* tabB = new QWidget; // a second athlete
        tabStack->addWidget(tabA);
        tabStack->addWidget(tabB);
        tabStack->setCurrentIndex(0);
        window.show();
        QApplication::processEvents();
        QVERIFY2(tabA->isVisible(), "premise: the current athlete tab is visible");

        Context tabCtx(reinterpret_cast<MainWindow*>(&window));
        tabCtx.athlete = athlete; // the fixture athlete, with its empty RideCache
        tabCtx.tab = reinterpret_cast<AthleteTab*>(tabA);

        BlockingStore* store = new BlockingStore(&tabCtx);
        CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(&tabCtx, store);

        // PART 1, as the constructor chose it.
        QVERIFY2(dialog->parentWidget() == tabA,
                 "the sync dialog did not parent itself to context->tab - this control is not exercising the "
                 "shape REQ-021 ships");

        QVERIFY2(dialog->start(), "start() failed on a store that opens cleanly");
        dialog->open(); // modeless, exactly as syncCloud / AddCloudWizard do
        QApplication::processEvents();
        QVERIFY2(dialog->isVisible(), "premise: the modeless sync dialog came up");

        // THE SENSITIVITY CONTROL, as in TEST-081: an ordinary (non-window) child
        // of the same tab must go invisible where the dialog does not.
        QWidget* plainChild = new QWidget(tabA);
        plainChild->show();
        QApplication::processEvents();
        QVERIFY2(plainChild->isVisible(), "premise: the ordinary child widget is visible");

        tabStack->setCurrentIndex(1); // switchAthleteTab
        QApplication::processEvents();
        const bool dialogVisibleAfterSwitch = dialog->isVisible();
        const bool controlVisibleAfterSwitch = plainChild->isVisible();
        tabStack->setCurrentIndex(0);
        QApplication::processEvents();
        const bool dialogVisibleAfterSwitchBack = dialog->isVisible();

        QVERIFY2(controlVisibleAfterSwitch == false,
                 "QStackedWidget::setCurrentIndex did not hide the outgoing page - the production mechanism this "
                 "control models is not what is being exercised, so its verdict is worthless");
        QVERIFY2(dialogVisibleAfterSwitch,
                 "switching athlete tabs HID the modeless sync dialog - DEC-030 option B has traded a "
                 "use-after-free for a dialog that vanishes mid-sync");
        QVERIFY2(dialogVisibleAfterSwitchBack, "the sync dialog did not come back on switching back");

        delete dialog; // takes the store it owns with it
        QApplication::processEvents();

        // -- A3-R021-F3 -------------------------------------------------
        // THE OTHER HALF OF THE PARENT CHOICE. context->tab is NULL for the
        // whole of MainWindow::openAthleteTab (AthleteTab's constructor assigns
        // it later, AthleteTab.cpp:35), and a PARENTLESS top-level modeless
        // QDialog was measured to lose its window modality outright, to survive
        // the main window's close and to hold quitOnLastWindowClosed off. So the
        // constructor must fall back to the window rather than to nothing.
        Context noTabCtx(reinterpret_cast<MainWindow*>(&window));
        noTabCtx.athlete = athlete;
        QVERIFY2(noTabCtx.tab == nullptr, "premise: Context::Context leaves tab null until AthleteTab sets it");

        BlockingStore* fallbackStore = new BlockingStore(&noTabCtx);
        CloudServiceSyncDialog* fallback = new CloudServiceSyncDialog(&noTabCtx, fallbackStore);
        QVERIFY2(fallback->parentWidget() == &window,
                 "with context->tab unset the sync dialog took no parent at all - a parentless top-level "
                 "modeless QDialog blocks nothing and outlives the window (A3-R021-F3)");
        delete fallback;
        QApplication::processEvents();
    }

  private:
    // TEST-085 (b)'s APPARATUS CONTROL. Runs a BARE QDialog modally with the
    // given parent - no GoldenCheetah code anywhere in it - and reports how many
    // clicks on a button in ANOTHER TOP-LEVEL WINDOW were delivered while it was
    // up. Same Qt, same offscreen QPA, same click mechanism as the production
    // measurement, so it says whether this apparatus can see modal blocking AT
    // ALL, and whether the answer depends on the parent.
    // `counter` is the SHARED click count the target button increments; the
    // return value is the DELTA across this modal run, so the caller does not
    // have to unpick which click landed where.
    //
    // `click` MUST go through the platform path (QTest::mouseClick's QWindow
    // overload). The QWidget overload posts the event straight at the widget and
    // is delivered whatever is modal - measured, see the slot below - so a test
    // built on it would report "not blocked" for every dialog ever written.
    int clicksDeliveredDuringBareModalExec(QWidget* parent, const std::function<void()>& click, const int* counter)
    {
        const int before = *counter;
        QDialog probe(parent);
        QTimer::singleShot(50, &probe, [&probe, click]() {
            click();
            probe.accept();
        });
        QTimer::singleShot(3000, &probe, [&probe]() { probe.reject(); });
        probe.exec();
        QApplication::processEvents();
        return *counter - before;
    }

  private slots:

    // =====================================================================
    // TEST-085 (b) — THE OTHER STRUCTURAL CONTROL: the upload dialog still
    // blocks the ENTIRE APPLICATION, not merely the tab it now hangs off.
    //
    // DEC-029 kept upload MODAL on purpose (CloudService.cpp:94: going modeless
    // would race the store teardown at MainWindow.cpp:2563). Reparenting a modal
    // dialog is exactly the kind of change that can silently demote application
    // modality to window modality, which would leave every other top-level
    // window - Train, chart popups, the wizard - live while an upload is in
    // flight and let the user reach the very objects the upload is using.
    //
    // Measured four ways: what Qt reports, who Qt considers the active modal
    // widget, whether a click on ANOTHER TOP-LEVEL WINDOW is delivered while the
    // upload is up - and, as the apparatus control, the same click under a BARE
    // QDialog::exec() parented first to the window and then to the tab. That last
    // pair is what makes the verdict readable: if bare Qt blocks the click for
    // one parent and not the other, the parent is the cause; if it blocks for
    // neither, this apparatus cannot see modal blocking at all and only the two
    // reported-state assertions carry weight.
    // =====================================================================
    void uploadDialogExecStillBlocksTheWholeApplication()
    {
        obs::reset();

        int clickCount = 0; // every click on `otherButton`, whenever it lands
        int clicksDuringExec = -1;
        int clicksAfterExec = -1;
        bool dialogFound = false;
        bool parentWasTab = false;
        bool modalityWasApplicationModal = false;
        bool dialogWasTheActiveModalWidget = false;
        bool execReturned = false;
        int execResult = -1;
        bool timedOut = false;
        QEventLoop appLoop;

        // A SECOND TOP-LEVEL WINDOW. Application modality blocks it; window
        // modality (whose blocked set is the dialog's own parent chain) does not,
        // which is precisely the distinction this control has to make.
        QWidget other;
        QPushButton* otherButton = new QPushButton(QStringLiteral("elsewhere"), &other);
        connect(otherButton, &QPushButton::clicked, otherButton, [&clickCount]() { ++clickCount; });
        other.show();
        QApplication::processEvents();

        // THE CLICK, delivered through the PLATFORM path - QTest::mouseClick's
        // QWindow overload goes through QWindowSystemInterface, which is where Qt
        // consults blockedByModalWindow. Its QWidget overload does not, and posts
        // the event at the widget regardless of what is modal (measured: a bare
        // modal QDialog parented to the window let it through), which would make
        // every assertion below vacuous.
        const std::function<void()> clickOther = [&other, otherButton]() {
            QTest::mouseClick(other.windowHandle(), Qt::LeftButton, Qt::NoModifier, otherButton->geometry().center());
            QApplication::processEvents();
        };

        // ---- THE APPARATUS CONTROLS, in bare Qt, before any production code.
        FakeAthleteWindow* control = new FakeAthleteWindow(QDir(athleteRoot.path()));
        control->show();
        FakeAthleteWindow::AthleteSlot* controlSlot = control->addAthleteTab();
        controlSlot->tabWidget->show();
        QApplication::processEvents();
        const int clicksUnderWindowParentedModal = clicksDeliveredDuringBareModalExec(control, clickOther, &clickCount);
        const int clicksUnderTabParentedModal =
            clicksDeliveredDuringBareModalExec(controlSlot->tabWidget, clickOther, &clickCount);
        // ...and with NOTHING modal, so a zero above means blocked rather than
        // blind.
        const int before = clickCount;
        clickOther();
        const int clicksWithNothingModal = clickCount - before;
        delete control;
        QApplication::processEvents();

        QMetaObject::invokeMethod(
            this,
            [&]() {
                FakeAthleteWindow* win = new FakeAthleteWindow(QDir(athleteRoot.path()));
                win->show();
                FakeAthleteWindow::AthleteSlot* slot = win->addAthleteTab();
                slot->tabWidget->show();

                BlockingStore* store = new BlockingStore(slot->context);
                store->completeWrite = true; // so exec() has something to accept

                bool* timedOutp = &timedOut;
                // Scoped to `appLoop`, NOT to qApp - the same correction the two
                // watchdogs above carry, and for the same reason: `timedOut`
                // points into this test's stack frame, and a watchdog left armed
                // on qApp survives this run, fires five seconds later inside
                // whatever slot is running BY THEN and writes to a dead frame
                // (ASan: stack-use-after-return, reported in
                // TestGarminConnectSyncDialogClose.cpp:4521 as soon as a slot
                // added AFTER this one kept the suite alive long enough for it to
                // land). Scoping it to the loop makes Qt drop the pending call
                // when this run returns. Apparatus only - no assertion changes.
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });

                // Runs while exec() is up.
                QTimer::singleShot(200, qApp, [&, win]() {
                    CloudServiceUploadDialog* d = win->findChild<CloudServiceUploadDialog*>();
                    dialogFound = (d != nullptr);
                    if (!d)
                        return;
                    parentWasTab = (d->parentWidget() == win->hostFor(FakeAthleteWindow::HostedByTab, 0));
                    modalityWasApplicationModal = (d->windowModality() == Qt::ApplicationModal);
                    dialogWasTheActiveModalWidget = (QApplication::activeModalWidget() == d);

                    const int seen = clickCount;
                    clickOther();
                    clicksDuringExec = clickCount - seen;

                    if (d->okcancel)
                        d->okcancel->click(); // the user's OK
                });

                const bool accepted = CloudService::upload(win, slot->context, store, slot->item);
                execReturned = true;
                execResult = accepted ? int(QDialog::Accepted) : int(QDialog::Rejected);

                closeAndDeleteStore(store);

                // THE SENSITIVITY CONTROL: the identical click, with no modal
                // dialog in the way.
                const int seenAfter = clickCount;
                clickOther();
                clicksAfterExec = clickCount - seenAfter;

                delete win;
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        qInfo("TEST-085(b) measurement (Qt %s, QPA %s): upload dialog modality=%d activeModal=%d | clicks "
              "delivered to another top-level window: nothingModal=%d bareModalParentedToWindow=%d "
              "bareModalParentedToTab=%d duringUpload=%d afterUpload=%d",
              qVersion(), qPrintable(QApplication::platformName()), int(modalityWasApplicationModal),
              int(dialogWasTheActiveModalWidget), clicksWithNothingModal, clicksUnderWindowParentedModal,
              clicksUnderTabParentedModal, clicksDuringExec, clicksAfterExec);

        QVERIFY2(timedOut == false, "the modal upload never returned from exec()");
        QVERIFY2(dialogFound, "no upload dialog was found under the athlete window - this run proves nothing");
        QVERIFY2(parentWasTab, "the upload dialog was not parented to context->tab - this is not the shipped shape");
        QVERIFY2(execReturned, "exec() did not return");
        QCOMPARE(execResult, int(QDialog::Accepted));

        // Qt's own account of it: an application-modal dialog, and the one the
        // application is blocked on.
        QVERIFY2(modalityWasApplicationModal,
                 "the upload dialog's windowModality is no longer Qt::ApplicationModal - reparenting to the "
                 "athlete tab has demoted it to window-modal");
        QVERIFY2(dialogWasTheActiveModalWidget, "the upload dialog was not the application's active modal widget");

        // The apparatus can see a click at all, before and after.
        QCOMPARE(clicksWithNothingModal, 1);
        QCOMPARE(clicksAfterExec, 1);

        // ...and it can see modal BLOCKING: bare Qt, same QPA, same click, a
        // modal dialog parented to the window swallows it. Without this the four
        // zeroes below would be indistinguishable from an apparatus that simply
        // never delivers anything.
        QCOMPARE(clicksUnderWindowParentedModal, 0);
        // PARENTING IS NOT WHAT DECIDES IT: the same bare dialog parented to the
        // athlete TAB blocks the same click. A difference here would mean the
        // reparent alone changes who a modal dialog blocks.
        QCOMPARE(clicksUnderTabParentedModal, 0);
        // ...and the SHIPPED dialog blocks it too.
        QVERIFY2(clicksDuringExec == 0,
                 "a click on another top-level window was delivered while the upload dialog was exec()ing - "
                 "reparenting to the athlete tab has stopped it blocking the whole application");
    }

    // =====================================================================
    // TEST-089 (REQ-022, DEC-garmin-031) — THE GATING PROBE. Is a NAIVE
    // deferred deletion, posted from inside a nested QEventLoop, delivered by
    // THAT loop or only once control reaches the outer one?
    // =====================================================================
    //
    // DEC-031 replaces DEC-025's deliberate store leak with a FRAME-COUNTED
    // reaper, and the whole reason it is frame-counted rather than one line of
    // `store->deleteLater()` is a claim about Qt: deferred deletion is delivered
    // by the event loop the call was made FROM, and on this path that loop is the
    // STORE'S OWN suspended nested loop (blockingDownload / blockingList /
    // blockingRestore). If that claim holds, `deleteLater()` frees the store
    // under the very frame executing on it - the use-after-free DEC-025 exists to
    // prevent - and so does a Qt::QueuedConnection invokeMethod posted from the
    // same place.
    //
    // This project decides framework semantics by EXECUTION. The identical
    // deleteLater loop-level claim has already been written into a design comment
    // in this file (see the header block, "WHY THIS FILE EXISTS AT ALL") and
    // FALSIFIED once. So it is measured here rather than believed, on the Qt this
    // tree builds against and through the same posting geometry production uses:
    //
    //   appLoop                       [stands in for QApplication::exec()]
    //     -> queued call              [event delivery: notifyInternal2 scope bump]
    //          -> nested QEventLoop   [the store's blockingXxx()]
    //               -> queued call    [the athlete teardown]
    //                    -> the three deletions under test
    //
    // THE APPARATUS CONTROL, and why it is not optional. A "did not die" reading
    // is exactly what an apparatus that observes nothing produces, and this
    // project has been burned by that twice (an inert stub; a QTest::mouseClick
    // overload blind to modal blocking). So a THIRD victim is destroyed by a
    // plain `delete` in the same delivered event and read at the same instant
    // through the same QPointer: if that one reads "alive" the apparatus is
    // broken and every other reading here is void.
    //
    // ...and the SAME CONTROL IN THE OTHER DIRECTION, because every reading this
    // probe takes comes back "dead" and a reader that only ever says "dead" would
    // produce exactly that. A FOURTH victim is deleted on a timer that cannot
    // fire until the nested loop has returned, and is read at the same instant
    // through the same QPointer: it must read ALIVE there and dead at the end.
    void deferredDeletionInsideANestedLoopIsDeliveredByThatNestedLoop()
    {
        // Kept in one struct so the queued lambdas capture a POINTER to this
        // frame rather than a web of references: the innermost call may be
        // delivered after its enclosing lambda has returned.
        struct Probe
        {
            QPointer<QObject> controlVictim;  // plain `delete` - the apparatus control
            QPointer<QObject> deferredVictim; // deleteLater() - the naive reaper
            QPointer<QObject> queuedVictim;   // deleted by a queued invokeMethod
            QPointer<QObject> lateVictim;     // deleted only AFTER the nested loop - the inverse control

            bool teardownRan = false; // the deletions were issued at all
            bool nestedLoopReturned = false;
            bool queuedCallRan = false;
            bool queuedCallRanInsideNestedLoop = false;

            // Read the instant the nested loop returns - INSIDE the frame that
            // owns it, which is precisely where readFile/readdir/open resume and
            // go on to touch the store.
            bool controlGoneOnResume = false;
            bool deferredGoneOnResume = false;
            bool queuedGoneOnResume = false;
            bool lateGoneOnResume = false;

            // ...and again once control is back at the outer loop and every
            // posted event has been drained.
            bool controlGoneAtEnd = false;
            bool deferredGoneAtEnd = false;
            bool queuedGoneAtEnd = false;
            bool lateGoneAtEnd = false;
        };

        Probe p;
        Probe* const probe = &p;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [probe, &appLoop]() {
                probe->controlVictim = new QObject;
                probe->deferredVictim = new QObject;
                probe->queuedVictim = new QObject;
                probe->lateVictim = new QObject;

                QObject* const control = probe->controlVictim.data();
                QObject* const deferred = probe->deferredVictim.data();
                QObject* const queued = probe->queuedVictim.data();
                QObject* const late = probe->lateVictim.data();

                // THE INVERSE CONTROL. 250ms is well past the nested loop's own
                // 150ms lifetime, so this cannot fire until the loop has
                // returned; scoped to `appLoop`, which outlives it.
                QTimer::singleShot(250, &appLoop, [late]() { delete late; });

                // THE TEARDOWN. Posted queued on qApp immediately before the
                // nested loop is entered, which is verbatim how
                // BlockingStore::fireActionThenBlock delivers every teardown in
                // this file and how the real athlete close arrives - and it is
                // what puts the deletions below INSIDE the nested loop's
                // delivery, with a scopeLevel bump on top of it.
                QMetaObject::invokeMethod(
                    qApp,
                    [probe, control, deferred, queued]() {
                        probe->teardownRan = true;

                        // CONTROL: an unmistakable death, issued here, read at
                        // the same instant and through the same QPointer as the
                        // two measurements.
                        delete control;

                        // MEASUREMENT 1 - what a naive reaper would do.
                        deferred->deleteLater();

                        // MEASUREMENT 2 - and its invokeMethod equivalent.
                        QMetaObject::invokeMethod(
                            qApp,
                            [probe, queued]() {
                                probe->queuedCallRan = true;
                                probe->queuedCallRanInsideNestedLoop = !probe->nestedLoopReturned;
                                delete queued;
                            },
                            Qt::QueuedConnection);
                    },
                    Qt::QueuedConnection);

                // The store's own nested loop (GarminConnect::blockingDownload
                // arms a timeout and calls loop.exec(); so does this).
                QEventLoop nested;
                QTimer::singleShot(150, &nested, &QEventLoop::quit);
                nested.exec();
                probe->nestedLoopReturned = true;

                probe->controlGoneOnResume = probe->controlVictim.isNull();
                probe->deferredGoneOnResume = probe->deferredVictim.isNull();
                probe->queuedGoneOnResume = probe->queuedVictim.isNull();
                probe->lateGoneOnResume = probe->lateVictim.isNull();

                QTimer::singleShot(250, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        p.controlGoneAtEnd = p.controlVictim.isNull();
        p.deferredGoneAtEnd = p.deferredVictim.isNull();
        p.queuedGoneAtEnd = p.queuedVictim.isNull();
        p.lateGoneAtEnd = p.lateVictim.isNull();

        qInfo("TEST-089 measurement (Qt %s, QPA %s): posted from inside a nested QEventLoop, dead when that "
              "loop returned? control(delete)=%d deleteLater=%d queuedInvoke=%d late(inverse control)=%d | "
              "dead by the end? control=%d deleteLater=%d queuedInvoke=%d late=%d | queued call ran=%d "
              "insideNestedLoop=%d",
              qVersion(), qPrintable(QApplication::platformName()), int(p.controlGoneOnResume),
              int(p.deferredGoneOnResume), int(p.queuedGoneOnResume), int(p.lateGoneOnResume), int(p.controlGoneAtEnd),
              int(p.deferredGoneAtEnd), int(p.queuedGoneAtEnd), int(p.lateGoneAtEnd), int(p.queuedCallRan),
              int(p.queuedCallRanInsideNestedLoop));

        QVERIFY2(p.teardownRan, "the queued teardown never ran - no deletion was ever issued, this run proves nothing");

        // THE APPARATUS CONTROL. Same observation point, same QPointer, a death
        // that is not in doubt. If this is false, nothing else here is evidence.
        QVERIFY2(p.controlGoneOnResume,
                 "the apparatus did not observe a plain `delete` issued inside the nested loop - it cannot "
                 "observe anything, and the two measurements below are artefacts");
        QVERIFY2(p.lateGoneOnResume == false,
                 "the apparatus reported an object dead at the resume point that was not deleted until 100ms "
                 "later - it reports death unconditionally, and every reading here is an artefact");
        QVERIFY2(p.lateGoneAtEnd, "the inverse control was never deleted at all - its reading proves nothing");

        // THE MECHANISM CONTROL. Both deferred forms really were armed, so a
        // "still alive when the loop returned" reading would mean DEFERRED, not
        // NEVER ISSUED.
        QVERIFY2(p.deferredGoneAtEnd, "deleteLater() never took effect at all - the measurement below is void");
        QVERIFY2(p.queuedCallRan, "the queued invokeMethod never ran at all - the measurement below is void");
        QVERIFY2(p.queuedGoneAtEnd, "the queued invokeMethod never deleted its victim - the measurement below is void");

        // THE MEASUREMENT, and DEC-031's design premise. If either of these
        // fails, a naive `store->deleteLater()` in ~CloudServiceSyncDialog IS
        // safe, the frame-counted reaper is unnecessary machinery, and the
        // decision has to be revisited before any of it is written.
        QVERIFY2(p.deferredGoneOnResume,
                 "MEASURED: deleteLater() posted from inside a nested QEventLoop was NOT delivered by that loop. "
                 "A naive store->deleteLater() on the DEC-025 decline path would therefore be safe and DEC-031's "
                 "premise is wrong - STOP and revisit the decision");
        QVERIFY2(p.queuedGoneOnResume,
                 "MEASURED: a Qt::QueuedConnection invokeMethod posted from inside a nested QEventLoop was NOT "
                 "delivered by that loop - DEC-031's premise is wrong, revisit the decision");
        QVERIFY2(p.queuedCallRanInsideNestedLoop,
                 "the queued deletion ran only after the nested loop returned - DEC-031's premise is wrong");
    }

    // =====================================================================
    // TEST-090 (REQ-022, DEC-garmin-031) — THE REAPER MECHANISM ITSELF.
    //
    // PART 1: NESTED FRAMES. An INNER blocking frame unwinding must not reap
    // while an OUTER one is still suspended.
    // =====================================================================
    //
    // TEST-072/075/077/078/084 all drive ONE suspended frame, and every one of
    // them would still pass a reaper that fired on the FIRST release rather than
    // the last. That is the difference between a refcounted record and a flag,
    // and it is a use-after-free: the outer frame resumes onto a store that has
    // been close()d and deleted underneath it.
    //
    // The nesting is not invented for the test - it is the one the
    // blockingCallDepth comment in CloudService.h describes: "the
    // QApplication::processEvents() inside a blocking call can dispatch a click
    // on Refresh, putting a readdir frame inside a readFile frame".
    //
    //   syncNext -> store->readFile -> NESTED loop              [OUTER, depth 1]
    //     -> queued Refresh -> refreshClicked -> store->readdir
    //          -> NESTED loop                                   [INNER, depth 2]
    //               -> queued `delete owner` -> ~CloudServiceSyncDialog
    //                    -> depth 2, so the store is handed to the record
    //          <- readdir returns, refreshClicked bails, INNER frame releases
    //   <- readFile's own loop returns and readFile TOUCHES THE STORE (its
    //      canary read) - which is where an early reap becomes a heap-use-after-free
    //   <- readFile returns, OUTER frame releases: THIS is the reap
    //
    // RED, two ways, both proven by mutation (see the report):
    //   * reap on the first release instead of the last -> AddressSanitizer
    //     heap-use-after-free READ in BlockingStore::readFile, AND
    //     closedInsideItsOwnCall, so this fails even where ASan is blind.
    //   * drop the release() from ~BlockingCall's stand-down path -> the store is
    //     never reaped at all: destroyed 0, not 1.
    void anInnerBlockingFrameUnwindingDoesNotReapWhileAnOuterOneIsSuspended()
    {
        obs::reset();

        ReapLog reap;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;
        bool timedOut = false;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeActivities();
                store->blockingMs = 120;
                store->log = &reap;
                store->closeActionContext = qApp;
                storeGuard = store;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->start();
                dialog->open();

                dialogGuard = dialog;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // Delivered INSIDE readFile's loop: a Refresh, which is a readdir
                // with a BlockingCall of its own. Armed only now, so neither
                // start()'s open()/refresh nor the setup refresh below blocks.
                store->closeAction = [store, dialog]() {
                    store->blockInReaddir = true;
                    dialog->refreshClicked();
                };
                // ...and delivered inside THAT readdir's loop: the teardown.
                store->nextAction = [owner]() {
                    obs::teardownFired = true;
                    delete owner;
                };

                dialog->refreshClicked();
                dialog->selectAllSyncChanged(Qt::Checked);
                dialog->downloadClicked();

                bool* timedOutp = &timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
                QTimer::singleShot(600, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(timedOut == false, "the sync never came back - a guard wedged it");
        QVERIFY2(obs::teardownFired, "the parent teardown never ran - this run proves nothing");

        // THE PREMISE: two of this store's nested loops really were on the stack
        // at once. Without this the whole slot degenerates into a duplicate of
        // TEST-072 and proves nothing about refcounting.
        QCOMPARE(reap.maxCallDepth, 2);

        // The OUTER frame resumed - readFile ran its tail, which reads a member
        // of the store. Under ASan an early reap aborts the process here.
        QVERIFY2(reap.resumed,
                 "readFile never resumed after its nested loop - the outer frame was not suspended where this "
                 "test needs it");
        QVERIFY2(dialogGuard.isNull(), "the parent teardown did not destroy the dialog - the premise is wrong");
        QVERIFY2(storeGuard.isNull(), "the store outlived the run - the reaper never ran (DEC-031)");

        assertStoreReapedExactlyOnceAfterTheNestedLoop("nested readdir inside readFile", reap);
    }

    // =====================================================================
    // TEST-090 (PART 2) — THE REFCOUNT CASE: TWO DIALOGS, TWO STORES, ONE
    // STACK.
    // =====================================================================
    //
    // The record DEC-031 specifies is PER-DIALOG and REFCOUNTED. A single static
    // frame counter would satisfy every other test in this file and would be
    // silently wrong here: with two sync dialogs suspended on the same stack the
    // shared count never reaches zero when the INNER dialog's last frame unwinds,
    // so that dialog's store is never reaped - and a single shared record can
    // only hold one orphan anyway, so the other is dropped outright. The symptom
    // is one store leaked (destroyed == 0), which is invisible on this target
    // because detect_leaks is off (A3-R027-F4). So it is counted per store.
    //
    //   dialog A -> start() -> storeA->open() -> NESTED loop     [A's frame]
    //     -> queued: build dialog B, B->start() -> storeB->open()
    //          -> NESTED loop                                    [B's frame]
    //               -> queued `delete owner` -> both dialogs destroyed,
    //                  each handing ITS OWN store to ITS OWN record
    //          <- B's open() returns and touches storeB; B's frame releases
    //             LAST for B, so storeB is reaped HERE
    //     <- A's open() returns and touches storeA - storeA must still be alive;
    //        A's frame releases last for A, so storeA is reaped HERE
    //
    // No production route to two concurrent sync dialogs was found, so this is
    // the shape being pinned rather than a reachable defect - which is exactly
    // why it needs a test: nothing else would ever notice it going wrong.
    void twoSuspendedSyncDialogsEachReapTheirOwnStoreExactlyOnce()
    {
        obs::reset();

        ReapLog reapA;
        ReapLog reapB;
        QPointer<CloudServiceSyncDialog> guardA;
        QPointer<CloudServiceSyncDialog> guardB;
        QPointer<CloudService> storeGuardA;
        QPointer<CloudService> storeGuardB;
        bool secondDialogBuilt = false;
        bool startedA = true;
        bool startedB = true;
        bool timedOut = false;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* storeA = new BlockingStore(context);
                storeA->entryNames = threeActivities();
                // A's loop must outlast B's, so that B unwinds INSIDE it.
                storeA->blockingMs = 300;
                storeA->blockInOpen = true;
                storeA->log = &reapA;
                storeA->closeActionContext = qApp;
                storeGuardA = storeA;

                CloudServiceSyncDialog* dlgA = new CloudServiceSyncDialog(context, storeA);
                dlgA->setParent(owner, Qt::Dialog);
                dlgA->setAttribute(Qt::WA_DeleteOnClose);
                guardA = dlgA;
                storeA->dialogGuard = dlgA;

                // Delivered INSIDE A's open(): the second dialog, suspended in a
                // blocking call of its own.
                storeA->closeAction = [&, owner]() {
                    BlockingStore* storeB = new BlockingStore(context);
                    storeB->entryNames = threeActivities();
                    storeB->blockingMs = 120;
                    storeB->blockInOpen = true;
                    storeB->log = &reapB;
                    storeB->closeActionContext = qApp;
                    storeGuardB = storeB;

                    CloudServiceSyncDialog* dlgB = new CloudServiceSyncDialog(context, storeB);
                    dlgB->setParent(owner, Qt::Dialog);
                    dlgB->setAttribute(Qt::WA_DeleteOnClose);
                    guardB = dlgB;
                    storeB->dialogGuard = dlgB;
                    secondDialogBuilt = true;

                    // ...and inside B's open(): the teardown that destroys BOTH,
                    // each with one frame suspended.
                    storeB->closeAction = [&, owner]() {
                        obs::teardownFired = true;
                        delete owner;
                    };

                    // Only the returned bool survives this call.
                    startedB = dlgB->start();
                };

                bool* timedOutp = &timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });

                startedA = dlgA->start();

                QTimer::singleShot(600, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(timedOut == false, "a start() never came back");
        QVERIFY2(secondDialogBuilt, "the second sync dialog was never built - this run proves nothing");
        QVERIFY2(obs::teardownFired, "the parent teardown never ran - this run proves nothing");

        // Both dialogs really were suspended, and both really were destroyed by
        // the one teardown.
        QVERIFY2(reapA.maxCallDepth >= 1 && reapB.maxCallDepth >= 1,
                 "one of the two stores never ran a nested loop - the two frames were not on the stack together");
        QVERIFY2(reapA.resumed, "dialog A's store->open() never resumed - its frame was not suspended");
        QVERIFY2(reapB.resumed, "dialog B's store->open() never resumed - its frame was not suspended");
        QVERIFY2(startedA == false, "dialog A's start() returned true after the dialog was destroyed");
        QVERIFY2(startedB == false, "dialog B's start() returned true after the dialog was destroyed");
        QVERIFY2(guardA.isNull() && guardB.isNull(), "the teardown did not destroy both dialogs");

        // ...and EACH store was closed and deleted exactly once, by ITS OWN
        // dialog's last frame. A shared counter reaps one of them and drops the
        // other.
        QVERIFY2(storeGuardA.isNull(), "dialog A's store outlived the run - it was leaked (DEC-031)");
        QVERIFY2(storeGuardB.isNull(), "dialog B's store outlived the run - it was leaked (DEC-031)");
        assertStoreReapedExactlyOnceAfterTheNestedLoop("two dialogs: the OUTER dialog's store", reapA);
        if (QTest::currentTestFailed())
            return;
        assertStoreReapedExactlyOnceAfterTheNestedLoop("two dialogs: the INNER dialog's store", reapB);
    }

  private:
    // =====================================================================
    // TEST-091 (REQ-025, B-R031-01) — THE UNCOUNTED ENCLOSING FRAME.
    // =====================================================================
    //
    // DEC-031's reaper fires when the LAST COUNTED BlockingCall unwinds, on the
    // stated ground that that is "the point at which no store call this dialog
    // made is on the stack any more". That is only true if blockingCallDepth is
    // a COMPLETE predicate — i.e. if DEC-024's invariant ("every store call that
    // can run a nested QEventLoop is wrapped in one of these") actually holds.
    // It did not. Three of this dialog's own suspension points ran a nested
    // QEventLoop with no BlockingCall at all, because the loop belongs to the
    // RIDE FILE READER rather than to the store:
    //
    //   syncNext        openRideFile (:1994), then compressRide (:2000) and
    //                   writeFile (:2002) ON THE STORE;
    //   completedRead   store->uncompressRide (:2140) -> openRideFile (:363);
    //   uploadNext      openRideFile (:2261), then compressRide/writeFile.
    //
    // An UNCOUNTED loop can ENCLOSE a counted one, and then:
    //
    //   syncNext -> openRideFile -> NESTED loop            [OUTER, UNCOUNTED]
    //     -> queued Refresh -> refreshClicked -> BlockingCall (:1548)
    //          -> store->readdir -> NESTED loop            [INNER, depth 1]
    //               -> queued athlete-tab close -> ~CloudServiceSyncDialog sees
    //                  depth 1 and ADOPTS the store into the reaper
    //          <- readdir returns; refreshClicked bails; the INNER frame is the
    //             LAST counted one, so THIS is where the reap fires
    //   <- the reader's loop returns and syncNext resumes onto a store that has
    //      just been close()d and deleted: `store->compressRide` at :2000.
    //
    // Before DEC-031 the store was LEAKED on that path, so the resume was safe;
    // DEC-031 made it a use-after-free. .fit is exactly what GarminConnect
    // downloads, so this is the mainline path, not a corner.
    //
    // The fix is DEC-024's existing mechanism applied to the sites it always
    // intended to cover — a BlockingCall over the suspension AND the statements
    // that use its result on the store — plus the QPointer self-bail those sites
    // never had (the dialog is destroyed by the teardown, so `store`, `curr` and
    // `progressBar` are all reads on freed memory when the loop returns).
    enum RideOpenSite {
        SyncUploadBranch,        // syncNext's upload branch      (CloudService.cpp:1994)
        CompletedReadUncompress, // completedRead -> uncompressRide (:2140 -> :363)
        UploadNextBranch         // uploadNext                    (:2261)
    };

    struct RideOpenOutcome
    {
        bool rideOpenRan = false;     // the reader was reached
        bool rideOpenResumed = false; // ...and its nested loop ran to completion
        bool teardownFired = false;   // the athlete teardown landed inside it
        bool dialogGoneAtEnd = false; // ...and really did destroy the dialog
        bool storeGoneAtEnd = false;
        bool timedOut = false;
        ReapLog reap;
    };

    // The LOCAL activity the two upload sites read off disk, and the REMOTE one
    // the download site pulls. Both carry the suffix the blocking reader is
    // registered for — that is the whole point: a ride file whose reader runs a
    // nested QEventLoop, as FitRideFile's does.
    static QString localBlockingActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_09_00_00.gcblock");
    }
    static QString remoteBlockingActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_13_00_00.gcblock");
    }

    // Drives ONE athlete-tab teardown into a COUNTED frame that is itself
    // enclosed by an UNCOUNTED ride-file open, at one of the three sites.
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped)
    //          -> owner QWidget                          [the athlete tab]
    //          -> CloudServiceSyncDialog, made a CHILD of it (DEC-030)
    //          -> refresh / select all / Download
    //               -> syncNext|uploadNext|completedRead
    //                    -> openRideFile -> NESTED loop   [UNCOUNTED]
    //                         -> queued Refresh -> readdir -> NESTED loop
    //                              -> queued `delete owner`
    //
    // The Context is deliberately NOT destroyed with `owner`: the tail of
    // RideFileFactory::openRideFile dereferences the context it was handed
    // (RideFile.cpp:999, `context->athlete->cyclist`) AFTER the reader's nested
    // loop has returned, so a fixture that freed it would be measuring that
    // out-of-scope defect instead of this one. See the build report.
    RideOpenOutcome runUncountedRideOpen(RideOpenSite site)
    {
        obs::reset();
        rideopen::reset();

        RideOpenOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;
        QEventLoop appLoop;

        // The two upload sites need an activity in the ride cache: that is what
        // puts an "Upload" row in the sync list (CloudService.cpp:1763) and a row
        // in the upload list, and what syncNext/uploadNext then open off disk.
        RideItem* local = nullptr;
        if (site != CompletedReadUncompress) {
            local = new RideItem(nullptr, context);
            local->fileName = localBlockingActivity();
            local->path = context->athlete->home->activities().absolutePath();
            local->dateTime = QDateTime(QDate::currentDate(), QTime(9, 0, 0));
            // The upload list skips planned rides (:1696) and this target's
            // RideItem stand-in does not initialise the flag.
            local->planned = false;
            rideCache->rides().push_back(local);
        }

        QMetaObject::invokeMethod(
            this,
            [&]() {
                // The athlete tab. A plain QWidget: the only property that
                // matters is that Qt destroys its children when it goes.
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // Only the download site wants a remote listing; for the two
                // upload sites an EMPTY one keeps the local activity the only
                // row in the sync list, so the run reaches the upload branch
                // without downloading anything first.
                if (site == CompletedReadUncompress) {
                    store->entryNames = QStringList() << remoteBlockingActivity();
                    // GarminConnect's own setting (GarminConnect.cpp:102): it
                    // stages UNCOMPRESSED bytes, and uncompressRide's first guard
                    // (:320) rejects outright on the CloudService default.
                    store->downloadCompression = CloudService::none;
                }
                // The store's own loops are the INNER frames here, so they are
                // short: they must unwind well inside the reader's.
                store->blockingMs = 40;
                store->log = &out.reap;
                store->closeActionContext = qApp;
                storeGuard = store;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->start();
                dialog->open();

                dialogGuard = dialog;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // Delivered INSIDE the ride file open's loop: a Refresh, which is
                // a readdir with a BlockingCall of its own (:1548) — the COUNTED
                // frame this run needs INSIDE the uncounted one.
                rideopen::blockingMs = 300;
                rideopen::action = [store, dialog, owner]() {
                    store->blockInReaddir = true;
                    // ...and delivered inside THAT readdir's loop: the athlete
                    // tab closing, which destroys the dialog and hands the store
                    // to DEC-031's reaper.
                    store->closeAction = [owner]() {
                        obs::teardownFired = true;
                        delete owner;
                    };
                    dialog->refreshClicked();
                };

                dialog->refreshClicked();
                if (site == UploadNextBranch) {
                    // downloadClicked dispatches on the current tab (:1908), and
                    // the dialog opens on Synchronize (:1034).
                    QTabWidget* tabs = dialog->findChild<QTabWidget*>();
                    if (tabs)
                        tabs->setCurrentIndex(1);
                    dialog->selectAllUpChanged(Qt::Checked);
                } else {
                    dialog->selectAllSyncChanged(Qt::Checked);
                }
                dialog->downloadClicked();

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
                QTimer::singleShot(800, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        if (local) {
            rideCache->rides().removeAll(local);
            delete local->ride(false); // whatever the reader handed it
            delete local;
        }

        out.rideOpenRan = rideopen::opens > 0;
        out.rideOpenResumed = rideopen::resumed;
        out.teardownFired = obs::teardownFired;
        out.dialogGoneAtEnd = dialogGuard.isNull();
        out.storeGoneAtEnd = storeGuard.isNull();
        return out;
    }

    // The shared verdict for TEST-091's three sites. NOT a slot.
    void assertReapWaitedForTheRideFileOpen(const char* what, const RideOpenOutcome& out)
    {
        // Reaching this line at all means the process did not abort under ASan:
        // the resumed frame did not read `store`, `curr` or `progressBar` off a
        // destroyed dialog, and did not call into a reaped store.
        QVERIFY2(
            out.timedOut == false,
            qPrintable(QStringLiteral("%1: the sync never came back - a guard wedged it").arg(QLatin1String(what))));

        // The premises, all four of them, so that a run which quietly missed its
        // target cannot be mistaken for a pass.
        QVERIFY2(out.rideOpenRan,
                 qPrintable(
                     QStringLiteral("%1: no ride file was opened - this run proves nothing").arg(QLatin1String(what))));
        QVERIFY2(out.rideOpenResumed,
                 qPrintable(QStringLiteral("%1: the ride file open never resumed from its nested loop - the "
                                           "ENCLOSING frame was not suspended where this test needs it")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.teardownFired,
                 qPrintable(QStringLiteral("%1: the athlete teardown never ran - this run proves nothing")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.dialogGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the teardown did not destroy the dialog - the premise is wrong")
                                .arg(QLatin1String(what))));
        QVERIFY2(out.reap.maxCallDepth >= 1,
                 qPrintable(QStringLiteral("%1: the store never ran a nested loop of its own, so the COUNTED "
                                           "inner frame this run is about was never on the stack")
                                .arg(QLatin1String(what))));

        // THE POINT (B-R031-01). Not "the store was reaped" but "the store was
        // not reaped while a ride file open this dialog started was still
        // suspended underneath it".
        //
        // RED (remove the BlockingCall from the site, keep the self-bail): the
        // reap fires on the inner frame's release and BOTH of these are true.
        QVERIFY2(!out.reap.destroyedInsideARideFileOpen,
                 qPrintable(QStringLiteral("%1: the store was DELETED while an uncounted ride-file-open frame of "
                                           "this dialog's was still suspended - blockingCallDepth is not the "
                                           "complete predicate DEC-031's reaper assumes (REQ-025)")
                                .arg(QLatin1String(what))));
        QVERIFY2(!out.reap.closedInsideARideFileOpen,
                 qPrintable(QStringLiteral("%1: the store was close()d while an uncounted ride-file-open frame of "
                                           "this dialog's was still suspended - the worker thread and interpreter "
                                           "session went down under the frame about to use them (REQ-025)")
                                .arg(QLatin1String(what))));

        // ...and DEC-031's own bar still holds: exactly once, close before
        // delete, never inside one of the store's own calls.
        QVERIFY2(out.storeGoneAtEnd,
                 qPrintable(QStringLiteral("%1: the store outlived the run - the reaper never ran (DEC-031)")
                                .arg(QLatin1String(what))));
        assertStoreReapedExactlyOnceAfterTheNestedLoop(what, out.reap);
    }

  private slots:

    // -- TEST-091 (REQ-025, B-R031-01) -----------------------------------
    // The regression itself, at all three sync-dialog sites. See the block
    // comment on RideOpenSite for the sequence and for what RED looks like.
    void anUncountedRideFileOpenMustNotLetTheReaperFireUnderneathIt()
    {
        struct Case
        {
            RideOpenSite site;
            const char* what;
        };
        const Case cases[] = {
            {SyncUploadBranch, "syncNext upload branch: openRideFile (:1994) enclosing refreshClicked/readdir"},
            {CompletedReadUncompress,
             "completedRead: store->uncompressRide (:2140) -> openRideFile (:363) enclosing refreshClicked/readdir"},
            {UploadNextBranch, "uploadNext: openRideFile (:2261) enclosing refreshClicked/readdir"},
        };

        for (const Case& c : cases) {
            const RideOpenOutcome out = runUncountedRideOpen(c.site);
            assertReapWaitedForTheRideFileOpen(c.what, out);
            if (QTest::currentTestFailed())
                return;
        }
    }

  private:
    // =====================================================================
    // TEST-093 (REQ-026) — AN ABORT DELIVERED INSIDE uploadNext MUST STOP THE
    // LOOP. A QPointer SELF-BAIL DOES NOT DO THAT.
    // =====================================================================
    //
    // uploadNext (CloudService.cpp:2314-2411) does not read `aborted` ANYWHERE.
    // Every read of that member is in a DIFFERENT function - completedRead
    // (:2187), failedRead (:2292), completedWrite (:2421) - and the only mention
    // inside uploadNext is the `aborted=false` in its completion tail (:2399).
    //
    // That is survivable on the success path, because that path RETURNS after
    // every row (:2374) and re-entry comes through completedWrite, which does
    // check (:2421). It is NOT survivable on the parse-failure branch
    // (:2376-2387), which is the one branch in the function that SUSPENDS
    //
    //     QApplication::processEvents();      // :2378
    //     if (self.isNull()) return true;     // :2385  <- LIFETIME only
    //
    // and then carries on iterating. The abort can be set by the very event that
    // processEvents() delivered, and `self` is not null - the user aborted the
    // transfer, they did not close the dialog. So the loop advances to the next
    // checked row and, if that row's file parses, reaches store->compressRide and
    // store->writeFile (:2370-2371) and uploads an activity the user already
    // stopped.
    //
    // THE ROUTE THAT SETS THE FLAG. Not deferCloseIfBusy (:1471) - that needs
    // blockingCallDepth > 0 and a close. The dialog's own Abort BUTTON:
    // downloadClicked (:1908) sees downloading == true, sets `aborted=true`
    // (:1916), relabels itself "Download" (:1914), shows Cancel and returns
    // WITHOUT stopping uploadNext. It is labelled "Abort" for the whole batch
    // (:1924), so this is the primary user-facing abort control, and it is
    // synchronous - no deferral, no depth condition, no teardown.
    //
    // THE FIXTURE, and why it is not a copy of TEST-087's UploadNextParsePE.
    // That slot has ONE row, whose file is missing; it can only ever show that
    // the parse-failure branch was survived, never that the loop was stopped.
    // This one needs TWO checked rows with DIFFERENT file states:
    //
    //   row[0]  <today>_07_00_00.tcx   NOT on disk. The REAL TcxFileReader is
    //           compiled into this target and its openRideFile returns NULL the
    //           moment file.open() fails (TcxRideFile.cpp:41-42), so
    //           RideFileFactory hands uploadNext a null ride and the :2376
    //           parse-failure branch executes for real.
    //   row[1]  <today>_08_00_00.gcblock  ON DISK, and parseable: the suffix is
    //           the one BlockingRideFileReader is registered for, so it yields a
    //           RideFile and row[1] takes the `if (ride)` branch - the one that
    //           calls compressRide and writeFile.
    //
    // If row[1] were also missing the run could not tell "stopped correctly" from
    // "continued and failed again", and would pass for the wrong reason.
    //
    // The row ORDER matters and is not assumed: both the insertion order (the
    // ride cache walk at :1720) and every ascending sort on the file/date/time
    // columns agree, because 07:00 sorts before 08:00 in all of them. It is
    // asserted as a premise anyway.
    //
    // The abort is queued on qApp BEFORE downloadClicked(), exactly as TEST-087
    // does: row[0]'s file is missing, so nothing suspends in row[0] except the
    // parse-failure processEvents() itself, which is precisely the delivery point
    // this test needs.
    //
    // RED, before the fix:
    //   FAIL!  : ... uploadNext kept iterating after the user aborted: row[1]
    //            was uploaded anyway (store->writeFile was called 1 time(s))
    //            Actual (out.writeFileCalls): 1   Expected: 0
    struct AbortedUploadOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the situation it claims to test?
        int upListCount = 0;                  // two rows in the upload list...
        int checkedRows = 0;                  // ...and the user checked both
        QString row0File;                     // ...row[0] is the one that cannot be parsed
        QString row1File;                     // ...row[1] is the one that can
        bool sawAbortLabel = false;           // the button really said "Abort"
        bool abortDelivered = false;          // the queued abort ran at all
        bool abortTookTheAbortBranch = false; // ...and downloadClicked took :1910-1920
        QString row0Status;                   // "Parse failure" = the :2376 branch ran

        // -- the verdict, observed two independent ways
        int writeFileCalls = 0; // store->writeFile for row[1]
        int rideOpens = 0;      // row[1]'s ride file was OPENED at all
        QString row1Status;     // "Uploading" = the loop advanced onto it
        QString buttonTextAtEnd;
    };

    static QPushButton* pushButtonWithText(QWidget* dialog, const QString& text)
    {
        const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
        for (QPushButton* b : buttons)
            if (b->text() == text)
                return b;
        return nullptr;
    }

    static QString missingUploadActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_07_00_00.tcx");
    }
    static QString parseableUploadActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_08_00_00.gcblock");
    }

    // One run:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped)
    //          -> owner QWidget                       [stands in for the tab]
    //          -> CloudServiceSyncDialog, a child of it
    //          -> Upload tab / Select all / Upload
    //               -> uploadNext -> row[0] -> openRideFile fails -> :2376
    //                    -> processEvents() delivers the queued ABORT
    //                         -> downloadClicked() -> aborted = true
    //                    <- and then?  row[1], or nothing?
    //
    // Nothing here destroys the dialog: that is the point. `self` stays non-null
    // for the whole run, so a lifetime bail cannot be what stops the loop.
    AbortedUploadOutcome runAbortInsideUploadParseFailure()
    {
        obs::reset();
        rideopen::reset();
        rideopen::blockingMs = 5; // nothing in THIS run needs a long suspension

        AbortedUploadOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QEventLoop appLoop;

        // The two local activities, and their file states on disk.
        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());
        const QString missingPath = activities.absolutePath() + "/" + missingUploadActivity();
        const QString presentPath = activities.absolutePath() + "/" + parseableUploadActivity();
        QFile::remove(missingPath); // row[0]: NOT there
        QFile present(presentPath); // row[1]: there, and its reader succeeds
        present.open(QIODevice::WriteOnly);
        present.write("gcblock");
        present.close();

        RideItem* first = new RideItem(nullptr, context);
        first->fileName = missingUploadActivity();
        first->path = activities.absolutePath();
        first->dateTime = QDateTime(QDate::currentDate(), QTime(7, 0, 0));
        first->planned = false; // the upload list skips planned rides (:1723)
        RideItem* second = new RideItem(nullptr, context);
        second->fileName = parseableUploadActivity();
        second->path = activities.absolutePath();
        second->dateTime = QDateTime(QDate::currentDate(), QTime(8, 0, 0));
        second->planned = false;
        rideCache->rides().push_back(first);
        rideCache->rides().push_back(second);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // NOTHING remote: so neither row is marked as already existing
                // and the "File exists" skip (:2329) cannot swallow a row.
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                dialogGuard = dialog;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // The Upload tab, picked as the user picks it, then Select All.
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(1);
                dialog->selectAllUpChanged(Qt::Checked);

                QTreeWidget* up = rideListWithHeader(dialog, QStringLiteral("File"));
                if (up != nullptr) {
                    QTreeWidgetItem* root = up->invisibleRootItem();
                    out.upListCount = root->childCount();
                    for (int i = 0; i < out.upListCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(up->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.upListCount > 0)
                        out.row0File = root->child(0)->text(1);
                    if (out.upListCount > 1)
                        out.row1File = root->child(1)->text(1);
                }

                // Identified once, from inside the batch, and then WATCHED: this
                // is the dialog's download/abort button, and every later read is
                // of the same widget rather than of whatever happens to carry a
                // given label at the time.
                QPushButton* abortButton = nullptr;

                // THE ABORT. Queued, so that the parse-failure processEvents() at
                // :2378 is what delivers it - the same posting discipline
                // TEST-087 measured for this frame.
                QMetaObject::invokeMethod(
                    qApp,
                    [&, dialog]() {
                        out.abortDelivered = true;
                        // The batch is running, so the download button is the
                        // ABORT button (:1924). That IS the control under test.
                        abortButton = pushButtonWithText(dialog, QStringLiteral("Abort"));
                        out.sawAbortLabel = (abortButton != nullptr);
                        dialog->downloadClicked();
                        // downloadClicked's abort branch relabels that same button
                        // "Download" (:1914). So this is the observable proof that
                        // :1910-1920 ran and `aborted` is now true.
                        out.abortTookTheAbortBranch =
                            (abortButton != nullptr && abortButton->text() == QStringLiteral("Download"));
                    },
                    Qt::QueuedConnection);

                dialog->downloadClicked(); // -> uploadNext()

                // Read the verdict the instant uploadNext hands control back.
                out.writeFileCalls = obs::writeFileCalls;
                out.rideOpens = rideopen::opens;
                if (up != nullptr) {
                    QTreeWidgetItem* root = up->invisibleRootItem();
                    if (root->childCount() > 0)
                        out.row0Status = root->child(0)->text(7);
                    if (root->childCount() > 1)
                        out.row1Status = root->child(1)->text(7);
                }
                // Still what the abort left it, or has the completion tail
                // (:2397) relabelled it "Upload" behind the user's back?
                if (abortButton != nullptr)
                    out.buttonTextAtEnd = abortButton->text();

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        rideCache->rides().removeAll(first);
        rideCache->rides().removeAll(second);
        delete first->ride(false);
        delete first;
        delete second->ride(false);
        delete second;
        QFile::remove(presentPath);

        return out;
    }

  private slots:

    // -- TEST-093 (REQ-026) ----------------------------------------------
    // When the user aborts, NO FURTHER RIDE IS TRANSFERRED. See the block
    // comment on AbortedUploadOutcome for the defect, the route and the fixture.
    void anAbortInsideUploadNextMustStopTheLoopNotJustProveTheDialogIsAlive()
    {
        const AbortedUploadOutcome out = runAbortInsideUploadParseFailure();

        QVERIFY2(out.timedOut == false, "the upload never came back - a guard wedged it");

        // ---- THE PREMISES. Every one of them is a way this run could be green
        //      while seeing nothing at all (LSN-047, LSN-050).
        QCOMPARE(out.upListCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QCOMPARE(out.row0File, missingUploadActivity());
        QCOMPARE(out.row1File, parseableUploadActivity());
        QVERIFY2(out.abortDelivered, "the queued abort never ran - this run proves nothing");
        QVERIFY2(out.sawAbortLabel,
                 "the download button was not labelled \"Abort\" while the batch ran, so downloadClicked() could "
                 "not have been the abort control - this run proves nothing");
        QVERIFY2(out.abortTookTheAbortBranch,
                 "downloadClicked() did not take its abort branch (CloudService.cpp:1910-1920), so `aborted` was "
                 "never set - this run proves nothing");
        QCOMPARE(out.row0Status, QStringLiteral("Parse failure"));

        // ---- THE POINT. Two independent observations of the SAME claim: row[1]
        //      was never opened and was never written.
        QVERIFY2(out.writeFileCalls == 0,
                 qPrintable(QStringLiteral("uploadNext kept iterating after the user aborted: row[1] was uploaded "
                                           "anyway (store->writeFile was called %1 time(s))")
                                .arg(out.writeFileCalls)));
        QVERIFY2(out.rideOpens == 0,
                 qPrintable(QStringLiteral("uploadNext kept iterating after the user aborted: row[1]'s ride file "
                                           "was OPENED (%1 open(s)) - the abort did not stop the loop, it only "
                                           "failed to reach writeFile")
                                .arg(out.rideOpens)));
        QVERIFY2(out.row1Status.isEmpty(),
                 qPrintable(QStringLiteral("uploadNext advanced onto row[1] after the abort - its status cell reads "
                                           "\"%1\"")
                                .arg(out.row1Status)));

        // ...and it stopped by BAILING, not by running the completion tail
        // (:2391-2408), which would have relabelled the button "Upload" and
        // cleared `aborted` behind the user's back.
        QCOMPARE(out.buttonTextAtEnd, QStringLiteral("Download"));
    }

  private:
    // =====================================================================
    // TEST-094 (REQ-026, A3-R021b-F3) — THE SYMMETRIC RE-READ IN completedRead.
    // =====================================================================
    //
    // completedRead reads `aborted` ONCE, on ENTRY (CloudService.cpp:2187), and
    // never again. Between that read and its tail it SUSPENDS:
    //
    //     BlockingCall blocking(this);
    //     ride = store->uncompressRide(data, name, errors);   // :2215
    //
    // and uncompressRide stages the bytes and hands them to
    // RideFileFactory::openRideFile (:363), whose readers run nested QEventLoops
    // (FitRideFile.cpp:172-184). An abort delivered inside THAT loop - the Abort
    // button, downloadClicked's :1910-1920 branch, exactly as in TEST-093 - is
    // therefore set while this invocation is suspended, and the invocation then
    // resumes and calls saveRide() (:2233) anyway. The entry check cannot see it;
    // the QPointer bail at :2224 cannot either, because the dialog is alive - the
    // user aborted a transfer, they did not close a window.
    //
    // WHAT THIS RUN CAN AND CANNOT SEE, stated because it changes what the
    // assertions are allowed to claim. In THIS target saveRide's two real side
    // effects are absent: JsonFileReader::writeRideFile is a link stub that
    // writes nothing (ImportSeamStubs.cpp:491) and Athlete::addRide is a no-op
    // (:205). So the run observes that saveRide RAN - the row's own verdict cell
    // and the progress increment that precedes it - and not the bytes production
    // would have written. The CONTROL run below is what makes that observation
    // trustworthy rather than assumed: with no abort delivered, the same fixture
    // must reach saveRide and land "Saved" in that cell. If the apparatus could
    // not see saveRide happening, the control would fail (LSN-047, LSN-050).
    //
    // RED, before the fix:
    //   FAIL!  : ... completedRead finished saveRide() after the user aborted:
    //            the row reads "Saved"
    //            Actual   ("Saved")   Expected ("Aborted")
    struct AbortedSaveOutcome
    {
        bool timedOut = false;

        // -- premises
        int syncListCount = 0;
        bool rideOpenRan = false;     // uncompressRide really did reach a reader
        bool rideOpenResumed = false; // ...and that reader's nested loop ran
        bool abortDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;

        // -- the verdict, two independent observations
        QString rowStatus;      // "Saved" = saveRide ran; "Aborted" = it did not
        int progressValue = -1; // ++downloadcounter at :2229 is on saveRide's side of the guard
    };

    // One run. `deliverTheAbort` false is the CONTROL: identical in every other
    // respect, so a green run proves the apparatus sees saveRide when it happens.
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped)
    //          -> owner QWidget / CloudServiceSyncDialog child of it
    //          -> Select all / Synchronize
    //               -> syncNext -> store->readFile -> readComplete posted
    //                    -> syncNext's processEvents (:2012) delivers it
    //                         -> completedRead -> uncompressRide -> openRideFile
    //                              -> NESTED loop -> the ABORT
    //                         <- resume: saveRide, or a stand-down?
    AbortedSaveOutcome runAbortInsideCompletedReadUncompress(bool deliverTheAbort)
    {
        obs::reset();
        rideopen::reset();

        AbortedSaveOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QEventLoop appLoop;

        // uncompressRide stages the payload under the athlete's temp dir (:354)
        // and saveRide targets the activities dir (:2456); neither creates it.
        QDir().mkpath(context->athlete->home->activities().absolutePath());
        QDir().mkpath(context->athlete->home->temp().absolutePath());

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // ONE remote activity, carrying the suffix whose reader suspends.
                store->entryNames = QStringList() << remoteBlockingActivity();
                // GarminConnect's own setting (GarminConnect.cpp:102): it stages
                // UNCOMPRESSED bytes, and uncompressRide's first guard (:320)
                // rejects outright on the CloudService default.
                store->downloadCompression = CloudService::none;
                store->blockingMs = 5; // readFile is not the suspension under test
                store->closeActionContext = qApp;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                dialogGuard = dialog;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // THE ABORT, delivered from INSIDE the ride reader's nested loop -
                // i.e. from inside store->uncompressRide, which is where
                // completedRead is suspended.
                rideopen::blockingMs = 300;
                if (deliverTheAbort) {
                    rideopen::action = [&, dialog]() {
                        out.abortDelivered = true;
                        QPushButton* abort = pushButtonWithText(dialog, QStringLiteral("Abort"));
                        out.sawAbortLabel = (abort != nullptr);
                        dialog->downloadClicked();
                        out.abortTookTheAbortBranch = (abort != nullptr && abort->text() == QStringLiteral("Download"));
                    };
                }

                dialog->selectAllSyncChanged(Qt::Checked);
                QTreeWidget* syncList = rideListWithHeader(dialog, QStringLiteral("Source"));
                if (syncList != nullptr)
                    out.syncListCount = syncList->invisibleRootItem()->childCount();

                // The whole chain above runs INSIDE this call: readFile's loop,
                // the processEvents that delivers readComplete, completedRead and
                // the reader's 300ms loop.
                dialog->downloadClicked();

                if (syncList != nullptr && syncList->invisibleRootItem()->childCount() > 0)
                    out.rowStatus = syncList->invisibleRootItem()->child(0)->text(7);
                if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
                    out.progressValue = bar->value();
                out.rideOpenRan = rideopen::opens > 0;
                out.rideOpenResumed = rideopen::resumed;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(5000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        return out;
    }

  private slots:

    // -- TEST-094 (REQ-026, A3-R021b-F3) ---------------------------------
    // The same criterion as TEST-093, at completedRead's suspension point. See
    // the block comment on AbortedSaveOutcome for the route and for exactly what
    // this target can and cannot observe.
    void anAbortInsideUncompressRideMustStopCompletedReadFinishingSaveRide()
    {
        // ---- THE CONTROL FIRST. No abort: the run MUST reach saveRide, or the
        //      abort run below would prove nothing at all.
        const AbortedSaveOutcome control = runAbortInsideCompletedReadUncompress(false);
        QVERIFY2(control.timedOut == false, "control: the sync never came back");
        QCOMPARE(control.syncListCount, 1);
        QVERIFY2(control.rideOpenRan,
                 "control: uncompressRide never reached the blocking reader - the payload never got as far as "
                 "RideFileFactory and this apparatus cannot see saveRide at all");
        QVERIFY2(control.rideOpenResumed, "control: the reader's nested loop never ran to completion");
        QCOMPARE(control.rowStatus, QStringLiteral("Saved"));
        QCOMPARE(control.progressValue, 1);

        // ---- THE RUN. Same fixture, plus an abort delivered inside
        //      uncompressRide's nested loop.
        const AbortedSaveOutcome out = runAbortInsideCompletedReadUncompress(true);
        QVERIFY2(out.timedOut == false, "the sync never came back - a guard wedged it");
        QCOMPARE(out.syncListCount, 1);
        QVERIFY2(out.rideOpenRan, "uncompressRide never reached the blocking reader - this run proves nothing");
        QVERIFY2(out.rideOpenResumed,
                 "the reader's nested loop never resumed - completedRead was not suspended where this test needs "
                 "it and this run proves nothing");
        QVERIFY2(out.abortDelivered,
                 "the abort was never delivered inside the reader's loop - this run proves nothing");
        QVERIFY2(out.sawAbortLabel,
                 "the download button was not labelled \"Abort\" while the sync ran, so downloadClicked() could "
                 "not have been the abort control - this run proves nothing");
        QVERIFY2(out.abortTookTheAbortBranch,
                 "downloadClicked() did not take its abort branch (CloudService.cpp:1910-1920), so `aborted` was "
                 "never set - this run proves nothing");

        // ---- THE POINT.
        QVERIFY2(out.rowStatus == QStringLiteral("Aborted"),
                 qPrintable(QStringLiteral("completedRead finished saveRide() after the user aborted: the row "
                                           "reads \"%1\", not \"Aborted\"")
                                .arg(out.rowStatus)));
        QVERIFY2(out.progressValue == 0,
                 qPrintable(QStringLiteral("completedRead counted the aborted activity as processed "
                                           "(progressBar = %1) - it carried on past the abort")
                                .arg(out.progressValue)));
    }
};

QTEST_MAIN(TestGarminConnectSyncDialogClose)
#include "testGarminConnectSyncDialogClose.moc"
