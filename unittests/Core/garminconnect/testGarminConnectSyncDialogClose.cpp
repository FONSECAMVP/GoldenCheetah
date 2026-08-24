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

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QDate>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QKeyEvent>
#include <QLabel>
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
// TEST-126 — __sanitizer_print_stack_trace(), used only under GC_ORACLE_TRACE to
// name the frame that made a dispatch the oracle rejected.
#include <sanitizer/common_interface_defs.h>

#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <random>

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

// TEST-121 (A3-R028b-F1) — the autoProcess seam. saveRide's own suspension
// point: DataProcessorFactory::autoProcess runs the "Auto" processors, two of
// which enter a nested QEventLoop (FixElevation.cpp:288-300, no timeout;
// FixPyDataProcessor.cpp:39). The stand-in calls whatever is armed here from
// inside autoProcess, i.e. where postProcess would have suspended. Null unless
// a slot arms it, so the two other targets that compile ImportSeamStubs.cpp are
// unaffected (LSN-056).
extern std::function<void()> autoProcessAction;
extern int autoProcessCalls;
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

// TEST-107 (REQ-028 probe) — WHICH ROW that buffer belongs to. syncNext and
// downloadNext pass `curr->text(1)` as the remotename (CloudService.cpp:2074 /
// :2331), so this names the row the in-flight read was issued for. The exact
// counterpart of lastWriteName below, and needed for the same reason: a run that
// delivers a completion BY HAND has to be able to hand back the name the
// production loop actually asked for, rather than guess it from a row index.
QString lastReadName;

// TEST-114 (REQ-028 (c)) — EVERY row read, in order, for the same reason
// writeNames below exists on the write side: a count alone cannot tell "three
// rows, one read each" from "one row, read three times", and the positive
// control's whole claim is the former.
QStringList readNames;

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

// TEST-095 (REQ-027) — WHICH row was written, not merely how many. Criterion (a)
// says "store->writeFile called exactly once, FOR row[1]", and a count alone
// cannot tell a batch that skipped row[0] and wrote row[1] from one that somehow
// wrote row[0]. syncNext passes QFileInfo(curr->text(1)).baseName() +
// store->uploadExtension() (CloudService.cpp:2062), so this names the row.
QString lastWriteName;

// TEST-112 (S-R028-01) — EVERY row written, in order. A count and a "last" cannot
// tell "two rows, one each" from "one row, twice", and telling those apart is the
// whole of what the sort trace measures.
QStringList writeNames;
bool teardownSawModal = false;   // the teardown landed while a modal box was up
bool teardownFired = false;      // ...and it fired at all
bool startReturned = false;      // has start() handed control back yet?
bool teardownAfterStart = false; // ...had it, when the teardown landed?

void reset()
{
    readFileCalls = 0;
    resumedAfterNestedLoop = false;
    dialogAliveOnResume = false;
    storeDestroyed = false;
    storeClosed = false;
    storeReap = ReapLog();
    lastBuffer = nullptr;
    lastReadName.clear();
    readNames.clear();
    openCalls = 0;
    openResumed = false;
    writeFileCalls = 0;
    writeResumed = false;
    lastUploadBuffer = nullptr;
    lastWriteName.clear();
    writeNames.clear();
    teardownSawModal = false;
    teardownFired = false;
    startReturned = false;
    teardownAfterStart = false;
}

} // namespace obs

// ---------------------------------------------------------------------------
// TEST-126 — THE CONTINUOUS INVARIANT ORACLE.
//
// WHY THIS EXISTS. Six adversarial cycles on this dialog have each found a real
// defect behind a fully green suite, and the last three each found a defect
// created or missed by the fix for the previous one. There are now six
// interacting guard mechanisms in one dialog (listGeneration, the in-flight
// ticket, its invalidation sites, the driver-entry compares, the resumption
// compares, saveRide's three-part guard and the retired-write set). Each new
// mechanism MULTIPLIES the reachable state space rather than adding to it, and
// hand-enumerated click sequences - one slot per route somebody thought of -
// have stopped being able to keep up. So this is the other half: a small number
// of properties that must hold in EVERY run of this file, asserted from
// `cleanup()` after every test function, whatever that function was driving.
//
// WHAT IT CAN SEE. Dispatch/completion events come from the fake store and the
// dialog exposes the const diagnostic outstandingTransferCount(), so TEST-126
// compares its independent operation ledger directly with production's after
// dispatch, completion admission, completion removal and batch transitions.
// The remaining batch-scoping observations come from:
//
//   * the store calls this harness already intercepts (BlockingStore::readFile /
//     ::writeFile are the dispatches; CloudService's own readComplete /
//     readFailed / writeComplete signals are the completions), and
//   * the dialog's WIDGETS.
//
// One widget does more work here than the rest and the mechanism turns on it:
// `cancelButton` is shown and hidden at EXACTLY the four statements that write
// the private `downloading` flag - CloudService.cpp:1978/:1980 (abort),
// :2086/:2087 (start), :2570/:2573, :2707/:2709, :3304/:3306 (the three loop
// tails) - and nothing else in that file touches its visibility. So
// `cancelButton->isHidden()` IS `downloading`, exactly, and a QEvent::Show /
// QEvent::Hide filter on that button is a SYNCHRONOUS notification of every
// transition of a private member. That is what makes the invariant below
// scopeable to the LIVE batch without guessing.
//
// THE INVARIANTS, and the reason each is or is not asserted:
//
//   INV-1  LIVE OUTSTANDING <= 1.  Dispatches this dialog made for its CURRENT
//          batch, minus the completions delivered against them. DEC-036's own
//          comment states the property ("At most one transfer is outstanding per
//          LIVE batch: each of the four dispatch sites returns immediately after
//          its store call, and the only thing that re-drives a loop is a
//          completion slot's tail, which has consumed the ticket first") and
//          DEC-037's amendment states the exception ("per LIVE batch is a
//          statement about DISPATCH, not about the NETWORK"): a transfer the
//          dialog has ABANDONED is still on the wire and is not this dialog's
//          any more. So an abandonment - `downloading` going false, which is the
//          only way a batch loses a transfer - RETIRES whatever is outstanding
//          into a second bucket that is never counted again. Without that
//          scoping this invariant would fire on TEST-115/124/125, which park an
//          abandoned write across a restart ON PURPOSE.
//
//   INV-2  DISPATCHES IN ONE BATCH <= THAT BATCH'S OWN TOTAL.  downloadClicked
//          counts the checked rows into `downloadtotal` and publishes it as the
//          progress bar's maximum (CloudService.cpp:2098-2102), and a row is
//          transferred at most once, so a batch cannot legitimately issue more
//          store calls than it declared rows. This is the invariant "three writes
//          are issued for a two-row batch" violates.
//
//   INV-3  OPERATION CONSERVATION. Dispatches equal matched completions plus
//          outstanding operations. The latter is directly observed through
//          outstandingTransferCount(), including a synchronous resample after
//          the dialog's completion slot has removed the admitted record. This
//          catches both premature loss and failure to remove the final record.
//
//   NOT AN OBSERVER INVARIANT - RESTART IDEMPOTENCE.  "downloadClicked() twice
//          with nothing outstanding" is an ACTION, not an observation: asserting
//          it in every existing run would mean clicking in every existing run,
//          which changes what those runs drive. It is therefore checked by
//          TEST-127, which is already clicking, and only there. Its subject is
//          also only half-visible: `downloadcounter` is the progress bar's value
//          and `successful` reaches the progress label, but `listindex` is
//          private and has no widget - so the check is written against the two
//          that can be read and says so.
//
// WHAT IT DELIBERATELY DOES NOT ASSERT. A completion that matches NO dispatch is
// COUNTED but not failed: several slots in this file deliver one completion
// TWICE on purpose (that a second completion for the same row is swallowed is
// the property they measure), so a "no unmatched delivery" rule would be
// asserting the opposite of an existing criterion. The count is reported instead.
// ---------------------------------------------------------------------------
namespace oracle {

// One store call this harness watched the dialog make.
struct Dispatch
{
    bool isWrite = false;
    // The READ's identity: the buffer syncNext/downloadNext preallocated.
    // readComplete/readFailed hand back that same pointer (the contract DEC-036
    // states on notifyReadComplete), so it is a per-transfer token. COMPARED,
    // NEVER DEREFERENCED - by the time a completion arrives the dialog may
    // already have freed it.
    QByteArray* token = nullptr;
    // The WRITE's identity: the opaque operation id carried by writeComplete.
    // The name remains diagnostic metadata and is never used to match writes.
    quint64 operationId = 0;
    QString name;
    int serial = 0;
};

// --- the aggregate. Reset by init(), read by cleanup(), after EVERY test. ---
QStringList violations;      // one line per breach, in the order they happened
int dispatches = 0;          // readFile + writeFile calls the oracle scoped
int deliveries = 0;          // completion signals seen
int matchedLive = 0;         // ...that answered a LIVE batch's dispatch
int matchedAbandoned = 0;    // ...that answered an abandoned one
int unmatchedDeliveries = 0; // ...that answered nothing at all (counted only)
int maxLiveOutstanding = 0;
int maxDispatchesInOneBatch = 0;
int unattachedDispatches = 0;     // no sync dialog to scope them against
int abandonments = 0;             // `downloading` observed going false with work out
int batchStarts = 0;              // ...and going true
int conservationSamples = 0;      // direct outstandingTransferCount observations
int completionRemovalSamples = 0; // observations after production slots return
int operationOutstanding = 0;     // live plus abandoned, never capped or evicted
// ORCH-033 — deliveries during which the OBSERVER ITSELF was destroyed. The
// oracle lives inside the store; a completion slot that tears the dialog down
// deletes that store re-entrantly, so control returns into a freed object and
// the post-removal resample is not merely skipped, it is IMPOSSIBLE - there is
// no observer left to take it. This counter is namespace-scope precisely so it
// can be written when `this` is already gone, and it exists so the
// completionRemovalSamples meta-invariant can distinguish "the oracle stopped
// looking" (a real defect, still caught) from "the oracle was destroyed by the
// thing it was watching" (the run under test). It is NOT a general exemption:
// it only excuses a run where EVERY matched completion was terminal.
int observerDestroyedInDelivery = 0;

void reset()
{
    violations.clear();
    dispatches = 0;
    deliveries = 0;
    matchedLive = 0;
    matchedAbandoned = 0;
    unmatchedDeliveries = 0;
    maxLiveOutstanding = 0;
    maxDispatchesInOneBatch = 0;
    unattachedDispatches = 0;
    abandonments = 0;
    batchStarts = 0;
    conservationSamples = 0;
    completionRemovalSamples = 0;
    operationOutstanding = 0;
    observerDestroyedInDelivery = 0;
}

// Every event this oracle sees, in order, on demand (GC_ORACLE_TRACE=1). Off by
// default and silent: it exists so that a violation can be AUDITED - "was there
// really no abort between those two dispatches?" is the first question anyone
// will ask of an INV-1 report, and the answer has to be readable rather than
// re-derived.
bool tracing()
{
    static const bool on = qEnvironmentVariableIsSet("GC_ORACLE_TRACE");
    return on;
}

void trace(const QString& what)
{
    if (tracing())
        qInfo("  oracle| %s", qPrintable(what));
}

void note(const QString& what)
{
    const char* fn = QTest::currentTestFunction();
    violations << (QStringLiteral("[") + QString::fromLatin1(fn != nullptr ? fn : "?") + QStringLiteral("] ") + what);
}

QString summary()
{
    return QStringLiteral("dispatches=%1 deliveries=%2 (live %3 / abandoned %4 / unmatched %5) "
                          "maxLiveOutstanding=%6 maxDispatchesInOneBatch=%7 batchStarts=%8 abandonments=%9 "
                          "unattached=%10 conservationSamples=%11 postRemovalSamples=%12 "
                          "observerDestroyedInDelivery=%13")
        .arg(dispatches)
        .arg(deliveries)
        .arg(matchedLive)
        .arg(matchedAbandoned)
        .arg(unmatchedDeliveries)
        .arg(maxLiveOutstanding)
        .arg(maxDispatchesInOneBatch)
        .arg(batchStarts)
        .arg(abandonments)
        .arg(unattachedDispatches)
        .arg(conservationSamples)
        .arg(completionRemovalSamples)
        .arg(observerDestroyedInDelivery);
}

// The ONE sync dialog on screen, or null when there is none or more than one.
// Only ever consulted when a store was handed no `dialog` of its own; a run with
// two sync dialogs up (TEST-090's) always sets that member, so this returning
// null there costs nothing.
CloudServiceSyncDialog* soleSyncDialog()
{
    CloudServiceSyncDialog* found = nullptr;
    const QWidgetList all = QApplication::allWidgets();
    for (QWidget* w : all) {
        CloudServiceSyncDialog* d = qobject_cast<CloudServiceSyncDialog*>(w);
        if (d == nullptr)
            continue;
        if (found != nullptr)
            return nullptr; // ambiguous: say so rather than guess
        found = d;
    }
    return found;
}

QPushButton* buttonWithText(QWidget* dialog, const QString& text)
{
    const QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
    for (QPushButton* b : buttons)
        if (b->text() == text)
            return b;
    return nullptr;
}

// ONE DIALOG'S worth of accounting. Per STORE rather than global because two
// sync dialogs can be alive at once (TEST-090) and one shared counter would let
// either dialog's transfers be charged to the other.
class TransferOracle
{
  public:
    // Called at the first dispatch this store makes. `candidate` is the store's
    // own `dialog` member when the fixture set one.
    void attach(QWidget* candidate, QObject* filter)
    {
        if (attached_)
            return;
        CloudServiceSyncDialog* d = qobject_cast<CloudServiceSyncDialog*>(candidate);
        if (d == nullptr)
            d = soleSyncDialog();
        if (d == nullptr)
            return; // an upload dialog, or none: left unscoped, and counted
        dialog_ = d;
        cancel_ = buttonWithText(d, QStringLiteral("Close"));
        bar_ = d->findChild<QProgressBar*>();
        if (cancel_ == nullptr)
            return; // the one widget the scoping depends on is missing
        cancel_->installEventFilter(filter);
        // Attaching happens AT the first dispatch, so the batch that dispatch
        // belongs to began a moment ago and has issued nothing yet.
        downloading_ = cancel_->isHidden();
        dispatchesThisBatch_ = 0;
        attached_ = true;
    }

    bool attached() const { return attached_; }
    int liveOutstanding() const { return int(live_.count()); }

    // `downloading` may have changed. Called from the QEvent::Show/Hide filter on
    // cancelButton - i.e. synchronously, inside the very statement that wrote the
    // flag - and again at each dispatch as a belt-and-braces re-read.
    void sample()
    {
        if (!attached_ || cancel_.isNull())
            return;
        const bool now = cancel_->isHidden();
        if (now == downloading_)
            return;
        downloading_ = now;
        trace(now ? QStringLiteral("downloading -> TRUE (a batch started)")
                  : QStringLiteral("downloading -> FALSE (abort, or a loop reached its tail)"));
        if (now) {
            batchStarts++;
            dispatchesThisBatch_ = 0;
        } else {
            // THE BATCH LET GO OF WHATEVER IT HAD OUT. An abort does not cancel
            // the transfer (DEC-037); it stops being this batch's.
            if (!live_.isEmpty()) {
                abandonments++;
                trace(QStringLiteral("  ...retiring %1 outstanding transfer(s) as ABANDONED").arg(live_.count()));
                abandoned_ += live_;
                live_.clear();
            }
        }
    }

    void noteDispatch(QWidget* candidate, QObject* filter, bool isWrite, QByteArray* token, quint64 operationId,
                      const QString& name)
    {
        attach(candidate, filter);
        if (!attached_) {
            unattachedDispatches++;
            return;
        }
        sample();

        Dispatch d;
        d.isWrite = isWrite;
        d.token = token;
        d.operationId = operationId;
        d.name = name;
        d.serial = ++serial_;
        live_ << d;
        dispatches++;
        operationOutstanding++;
        dispatchesThisBatch_++;

        trace(QStringLiteral("DISPATCH %1#%2 (%3) -> live=%4 thisBatch=%5 barMax=%6")
                  .arg(isWrite ? QStringLiteral("write") : QStringLiteral("read"))
                  .arg(d.serial)
                  .arg(name)
                  .arg(live_.count())
                  .arg(dispatchesThisBatch_)
                  .arg(bar_.isNull() ? -1 : bar_->maximum()));

        if (live_.count() > maxLiveOutstanding)
            maxLiveOutstanding = int(live_.count());
        if (dispatchesThisBatch_ > maxDispatchesInOneBatch)
            maxDispatchesInOneBatch = dispatchesThisBatch_;

        checkConservation("dispatch");

        // ---- INV-1
        if (live_.count() > 1) {
            QStringList who;
            for (const Dispatch& o : live_)
                who << (o.isWrite ? QStringLiteral("write") : QStringLiteral("read")) + QStringLiteral("#") +
                           QString::number(o.serial) + QStringLiteral("(") + o.name + QStringLiteral(")");
            note(QStringLiteral("INV-1 outstanding<=1 violated: %1 transfers outstanding for the LIVE batch at once "
                                "[%2]")
                     .arg(live_.count())
                     .arg(who.join(QStringLiteral(", "))));
            // WHO ISSUED IT. The first question anyone asks of an INV-1 report is
            // which frame made the second dispatch, and under this target's
            // sanitizer that is answerable exactly rather than inferred from the
            // click trace. Only under GC_ORACLE_TRACE: it is diagnosis, not
            // verdict.
            if (tracing())
                __sanitizer_print_stack_trace();
        }

        // ---- INV-2. The bar's maximum is only meaningful once a batch with rows
        //      has set it (downloadClicked leaves it alone for an empty batch), so
        //      a maximum of zero is read as "not stated" rather than as "zero
        //      rows".
        const int total = bar_.isNull() ? 0 : bar_->maximum();
        if (total > 0 && dispatchesThisBatch_ > total) {
            note(QStringLiteral("INV-2 dispatches<=batch total violated: %1 store calls issued by a batch of %2 rows "
                                "(last: %3 %4)")
                     .arg(dispatchesThisBatch_)
                     .arg(total)
                     .arg(isWrite ? QStringLiteral("write") : QStringLiteral("read"))
                     .arg(name));
        }
    }

    // A completion signal left the store. Match its exact buffer/write identity
    // to the live batch first, then to an abandoned batch, then to nothing.
    void noteDelivery(bool isWrite, QByteArray* token, quint64 operationId, const QString& name)
    {
        if (!attached_)
            return;
        sample();
        // The oracle's emitter connection runs before the dialog's completion
        // slot, so both sides still include this operation at this point.
        checkConservation("completion admission");
        deliveries++;
        if (take(live_, isWrite, token, operationId)) {
            matchedLive++;
            operationOutstanding--;
            trace(QStringLiteral("DELIVERY %1 (%2) -> answered a LIVE dispatch, live=%3")
                      .arg(isWrite ? QStringLiteral("write") : QStringLiteral("read"))
                      .arg(name)
                      .arg(live_.count()));
            return;
        }
        if (take(abandoned_, isWrite, token, operationId)) {
            matchedAbandoned++;
            operationOutstanding--;
            trace(QStringLiteral("DELIVERY %1 (%2) -> answered an ABANDONED dispatch")
                      .arg(isWrite ? QStringLiteral("write") : QStringLiteral("read"))
                      .arg(name));
            return;
        }
        unmatchedDeliveries++;
        trace(QStringLiteral("DELIVERY %1 (%2) -> answered NOTHING this oracle had outstanding")
                  .arg(isWrite ? QStringLiteral("write") : QStringLiteral("read"))
                  .arg(name));
    }

    void checkConservation(const char* point)
    {
        if (!attached_ || dialog_.isNull())
            return;
        CloudServiceSyncDialog* const dialog = qobject_cast<CloudServiceSyncDialog*>(dialog_.data());
        if (dialog == nullptr)
            return;
        const int expected = int(live_.count() + abandoned_.count());
        const int actual = dialog->outstandingTransferCount();
        ++conservationSamples;
        if (dispatches != matchedLive + matchedAbandoned + operationOutstanding)
            note(QStringLiteral("operation conservation equation violated at %1: dispatches=%2 completions=%3 "
                                "outstanding=%4")
                     .arg(QString::fromLatin1(point))
                     .arg(dispatches)
                     .arg(matchedLive + matchedAbandoned)
                     .arg(operationOutstanding));
        if (actual != expected)
            note(QStringLiteral("operation conservation violated at %1: dialog=%2 oracle=%3")
                     .arg(QString::fromLatin1(point))
                     .arg(actual)
                     .arg(expected));
    }

    void checkCompletionRemoval()
    {
        checkConservation("completion removal");
        if (attached_ && !dialog_.isNull())
            ++completionRemovalSamples;
    }

  private:
    static bool take(QList<Dispatch>& from, bool isWrite, QByteArray* token, quint64 operationId)
    {
        for (int i = 0; i < from.count(); i++) {
            const Dispatch& d = from.at(i);
            if (d.isWrite != isWrite)
                continue;
            if (isWrite ? (d.operationId == operationId) : (d.token == token)) {
                from.removeAt(i);
                return true;
            }
        }
        return false;
    }

    QPointer<QWidget> dialog_;
    QPointer<QPushButton> cancel_;
    QPointer<QProgressBar> bar_;
    bool attached_ = false;
    bool downloading_ = false;
    int dispatchesThisBatch_ = 0;
    int serial_ = 0;
    QList<Dispatch> live_;
    QList<Dispatch> abandoned_;
};

} // namespace oracle

// ---------------------------------------------------------------------------
// insideframe — PUTTING THE USER'S ACTION INSIDE THE FRAME THAT IS UNDER TEST.
//
// THE MISTAKE THIS EXISTS TO REPLACE. Every slot in this file whose subject is a
// suspension - "the abort arrives inside completedRead's processEvents()", "the
// athlete tab dies inside failedRead's" - has to get an action to run at one
// exact point in a production frame: AFTER the slot's last read of the state it
// is about to be judged on, and BEFORE the guard reads it. The fixtures used to
// do that by QUEUEING the action (QMetaObject::invokeMethod ... QueuedConnection)
// behind the completion and letting the production QApplication::processEvents()
// inside the slot deliver it.
//
// That is not a guarantee, and the suite paid for assuming it was. Posting order
// fixes the order events are DELIVERED IN; it says nothing about which of them a
// NOMINATED processEvents() call delivers. QApplication::processEvents() is one
// non-blocking g_main_context_iteration, and whether GLib dispatches Qt's
// posted-event source in that single iteration does not follow from the queue
// having something in it. Measured, same machine, same binary, same dispatcher
// class (QPAEventDispatcherGlib): under QPA `offscreen` it was dispatched and the
// suite was green; under `minimal` and under an ambient wayland session it was
// not, the queued action was still pending when the guard ran, and the guard
// correctly read the un-aborted / not-yet-destroyed state. Forcing
// QCoreApplication::sendPostedEvents(nullptr, 0) immediately before the
// production processEvents() turned the same run green - the event was queued and
// reachable the whole time.
//
// SO NOTHING HERE WAITS TO BE NOTICED. Both stages are ordinary synchronous
// function calls made by code that is already running inside the frame:
//
//   stage 1  QProgressBar::valueChanged, a DIRECT connection, emitted by
//            production's own `progressBar->setValue(++downloadcounter)`. That
//            call sits inside every frame these slots test and, in all five of
//            them, strictly AFTER the last upstream read of `aborted`
//            (CloudService.cpp:2337 after the re-read at :2329; :2418 after the
//            entry check at :2412; :2610 after :2604; :2531 and :2123 in the two
//            parse-failure branches). It anchors us in the right invocation.
//
//   stage 2  QAbstractEventDispatcher::awake(), which QEventDispatcherGlib emits
//            as the FIRST statement of processEvents() whenever the caller did
//            not ask to wait - and QApplication::processEvents() never does. It
//            is emitted unconditionally, not as a consequence of dispatching
//            anything, so it does not care what is or is not in the queue.
//            Measured with a standalone probe under all three backends: a bare
//            nested processEvents() with NOTHING posted emits awake() exactly
//            once.
//
// Stage 2 is what makes a DESTRUCTIVE action safe. Deleting the dialog from
// stage 1 would be a use-after-free the fixture invented rather than one
// production is guilty of: QProgressBar::setValue touches its own d-pointer after
// emitting (repaintRequired/repaint), completedRead reads `listindex` and its
// tree two lines later (:2339), completedWrite writes `successful` at :2614. By
// the time awake() fires, every one of those has already happened and the very
// next thing production does is the guard.
//
// The action therefore runs INSIDE the processEvents() call the slot is being
// judged on, with the state it changes ALREADY APPLIED - `aborted` genuinely
// true, the tab genuinely destroyed - rather than merely pending.
// ---------------------------------------------------------------------------
namespace insideframe {

// Run `action` exactly once, inside the next QApplication::processEvents() on
// this thread. See stage 2 above.
inline void atNextProcessEvents(const std::function<void()>& action)
{
    QAbstractEventDispatcher* dispatcher = QAbstractEventDispatcher::instance();
    if (dispatcher == nullptr || !action)
        return;

    auto link = std::make_shared<QMetaObject::Connection>();
    *link = QObject::connect(
        dispatcher, &QAbstractEventDispatcher::awake, dispatcher,
        [link, action]() {
            QObject::disconnect(*link); // fires once
            action();
        },
        Qt::DirectConnection);
}

// Run `action` exactly once, inside the processEvents() of whichever completion
// slot (or parse-failure branch) next counts a row on `dialog`'s progress bar.
// See stages 1 and 2 above.
//
// `value <= 0` is ignored on purpose: downloadClicked() rewrites the bar's range
// and value at the start of every batch (CloudService.cpp:1960-1962), and only
// `setValue(++downloadcounter)` can produce a positive value.
inline void atTheNextCountedRow(QWidget* dialog, const std::function<void()>& action)
{
    if (dialog == nullptr || !action)
        return;
    QProgressBar* bar = dialog->findChild<QProgressBar*>();
    if (bar == nullptr)
        return;

    auto link = std::make_shared<QMetaObject::Connection>();
    *link = QObject::connect(
        bar, &QProgressBar::valueChanged, bar,
        [link, action](int value) {
            if (value <= 0)
                return;
            QObject::disconnect(*link); // arms once
            atNextProcessEvents(action);
        },
        Qt::DirectConnection);
}

} // namespace insideframe

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
    explicit BlockingStore(Context* context) : CloudService(context)
    {
        // TEST-126 — THE COMPLETION SIDE OF THE ORACLE, taken at the EMITTER.
        //
        // Connected here, in the store's own constructor, so these run BEFORE the
        // dialog's slots: CloudServiceSyncDialog connects to the same three
        // signals in ITS constructor, which cannot run until this one has
        // returned, and direct connections fire in connection order. So the
        // outstanding count is already decremented when the dialog's slot
        // re-drives its loop and dispatches the next row - which is the whole
        // point, since a healthy batch would otherwise read as two outstanding.
        //
        // At the SIGNAL rather than at notifyReadComplete/notifyWriteComplete
        // because several runs in this file deliver a completion BY HAND
        // (store->notifyWriteComplete(...) from the fixture) and a hook on this
        // class's own emit sites would not see those.
        connect(
            this, &CloudService::readComplete, this,
            [this](QByteArray* data, QString name, QString) { oracle_.noteDelivery(false, data, 0, name); },
            Qt::DirectConnection);
        connect(
            this, &CloudService::readFailed, this,
            [this](QByteArray* data, QString name, QString) { oracle_.noteDelivery(false, data, 0, name); },
            Qt::DirectConnection);
        connect(
            this, &CloudService::writeComplete, this,
            [this](quint64 operationId, QString name, QString) {
                oracle_.noteDelivery(true, nullptr, operationId, name);
            },
            Qt::DirectConnection);
        // There is no writeFailed signal on CloudService (CloudService.h:270-276):
        // the write channel reports only writeComplete, which is the same
        // asymmetry DEC-037 records as the reason the write side needed a set and
        // the read side did not.
    }
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
        obs::lastReadName = remotename; // TEST-107 — which row this buffer is for
        obs::readNames << remotename;   // TEST-114 — ...and every one of them, in order

        // TEST-126 — a DISPATCH, counted before anything else can happen.
        oracle_.noteDispatch(dialog, this, false, data, 0, remotename);
        parked << Parked{false, data, remotename, 0, false};

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

        // TEST-102 (A3-R027-F4) — the EXPLICIT failure channel, which
        // completedRead's sibling slot consumes. GarminConnect posts one or the
        // other per invocation (postReadComplete / postReadFailed), never both.
        if (failRead) {
            QMetaObject::invokeMethod(
                this, [this, data, remotename]() { notifyReadFailed(data, remotename, tr("service refused")); },
                Qt::QueuedConnection);
            armCompletionAction();
            return true;
        }

        // GarminConnect posts its completion deferred (m_completionContext), so
        // do the same: the dialog observes it in the processEvents() syncNext
        // runs immediately after readFile returns.
        QMetaObject::invokeMethod(
            this, [this, data, remotename]() { notifyReadComplete(data, remotename, tr("Completed.")); },
            Qt::QueuedConnection);
        armCompletionAction();
        return true;
    }

    // TEST-102 (A3-R027-F4) — the user's action, ARMED for the completion slot
    // this store has just queued.
    //
    // The abort has to land in a very specific place: after the slot's last read
    // of `aborted` - completedRead's REQ-026 re-read (CloudService.cpp:2329),
    // failedRead's entry check (:2412), completedWrite's (:2604) - and before the
    // guard that the abort must reach (:2372 / :2432 / :2626). The only
    // production statement between those two points is the slot's own
    // QApplication::processEvents() (:2354 / :2423 / :2615), so the action has to
    // run INSIDE that call.
    //
    // THIS DEPENDS ON TIMING AND SAYING OTHERWISE IS WHAT HID A REAL DEFECT FOR A
    // WHOLE WAVE. This used to queue the action behind the completion and rely on
    // that processEvents() to deliver it, with a comment claiming it worked "by
    // ordering rather than by timing". Queue order guarantees the order of
    // delivery; it does not guarantee delivery WITHIN a nominated processEvents()
    // call, and under QPA `minimal` and under wayland the action was still
    // pending when the guard ran. See the insideframe block comment for the
    // measurement and for the two synchronous seams used instead.
    //
    // THE CONSTRAINT THE MECHANISM RELIES ON: between production's
    // `progressBar->setValue(++downloadcounter)` and the processEvents() a few
    // lines below it, the slot must not itself pump events - otherwise the
    // one-shot lands in the wrong frame. It does not: what sits in that gap is a
    // QTreeWidgetItem::setText, a `successful++`, and (on the ride-bearing branch
    // of completedRead, which these runs do not take because their rows are
    // .gcfail) saveRide().
    void armCompletionAction()
    {
        if (!afterCompletionAction)
            return;
        std::function<void()> a = afterCompletionAction;
        afterCompletionAction = nullptr; // fires once
        insideframe::atTheNextCountedRow(dialog, a);
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
    bool writeFile(QByteArray& data, QString remotename, RideFile* ride, quint64 operationId) override
    {
        Q_UNUSED(ride);
        ++obs::writeFileCalls;
        obs::lastUploadBuffer = &data;
        obs::lastWriteName = remotename;
        obs::writeNames << remotename;

        // TEST-126 — a DISPATCH, counted before anything else can happen.
        oracle_.noteDispatch(dialog, this, true, nullptr, operationId, remotename);
        parked << Parked{true, nullptr, remotename, operationId, false};

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

        if (completeWrite) {
            QMetaObject::invokeMethod(
                this,
                [this, operationId, remotename]() { notifyWriteComplete(operationId, remotename, tr("Completed.")); },
                Qt::QueuedConnection);
            armCompletionAction(); // TEST-102: the completedWrite site (:2610/:2615)
        }
        return true;
    }

    // The base notify helpers emit direct signals. When they return, the
    // dialog's completion slot has finished removing the admitted record, so
    // this second observation catches a record retained past terminal delivery.
    // ORCH-033 — THE SELF-GUARD, AND WHY IT IS NOT PARANOIA.
    //
    // Each base notification below invokes the dialog's completion slot
    // DIRECTLY (a direct-connected signal, same stack). That slot runs
    // processEvents(), and a teardown delivered inside it destroys the dialog,
    // whose destructor deletes THIS STORE via closeAndDeleteStore
    // (CloudService.h:325, called from ~CloudServiceSyncDialog at
    // CloudService.cpp:1376). Control then returns HERE - into a member
    // function of an object that no longer exists - and the next statement
    // touches `oracle_`, a MEMBER, which died with us. That was the
    // heap-use-after-free at :712.
    //
    // The oracle already holds its SUBJECT as a QPointer and survives the
    // dialog's death; that is precisely what made this look guarded. The
    // missing guard was on the OBSERVER's own lifetime. QPointer is cleared by
    // ~QObject, so a null self-guard after the base call is not a heuristic -
    // it is the record that our own destructor ran while we were on the stack.
    //
    // Nothing may be touched through `this` once the guard is null: not
    // `oracle_`, not a member flag, not a trace call.
    void notifyReadComplete(QByteArray* data, const QString& name, const QString& message)
    {
        QPointer<CloudService> self(this);
        CloudService::notifyReadComplete(data, name, message);
        if (self.isNull()) {
            // We were deleted inside the base call. Nothing may be touched
            // through `this`; this counter is namespace-scope for exactly that.
            oracle::observerDestroyedInDelivery++;
            return;
        }
        oracle_.checkCompletionRemoval();
    }

    void notifyReadFailed(QByteArray* data, const QString& name, const QString& reason)
    {
        QPointer<CloudService> self(this);
        CloudService::notifyReadFailed(data, name, reason);
        if (self.isNull()) {
            // We were deleted inside the base call. Nothing may be touched
            // through `this`; this counter is namespace-scope for exactly that.
            oracle::observerDestroyedInDelivery++;
            return;
        }
        oracle_.checkCompletionRemoval();
    }

    void notifyWriteComplete(quint64 operationId, const QString& name, const QString& message)
    {
        QPointer<CloudService> self(this);
        CloudService::notifyWriteComplete(operationId, name, message);
        if (self.isNull()) {
            // We were deleted inside the base call. Nothing may be touched
            // through `this`; this counter is namespace-scope for exactly that.
            oracle::observerDestroyedInDelivery++;
            return;
        }
        oracle_.checkCompletionRemoval();
    }

    // TEST-126 — `downloading` CHANGED, observed at the statement that changed it.
    //
    // The filter is installed on the dialog's cancelButton (see the oracle's
    // block comment for why that button is `downloading` exactly). QEvent::Show
    // and QEvent::Hide are SENT, not posted - QWidgetPrivate::show_helper /
    // hide_helper call QCoreApplication::sendEvent - so this runs inside
    // downloadClicked's own frame rather than one event loop later, and the
    // abort-then-restart that happens entirely inside one processEvents() burst
    // is still seen as two transitions.
    //
    // The event is a TRIGGER, not the answer: what is read is isHidden(), so a
    // Show cascaded from the dialog itself being shown cannot be mistaken for an
    // abort (a button that is explicitly hidden stays hidden through its parent's
    // show, and isHidden() says so).
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (event != nullptr && (event->type() == QEvent::Show || event->type() == QEvent::Hide)) {
            oracle_.sample();
            oracle_.checkConservation("batch transition");
        }
        return CloudService::eventFilter(watched, event);
    }

    // TEST-127 — a completion this store has been asked to HOLD, so that the
    // fuzzer can choose when (and whether) it is delivered. Recorded at every
    // dispatch; only the fuzzer ever reads it. The buffer pointer is carried for
    // identity alone and is never dereferenced here.
    struct Parked
    {
        bool isWrite;
        QByteArray* token;
        QString name;
        quint64 operationId;
        bool delivered;
    };
    QList<Parked> parked;

    bool deliverParkedWrite(int index, const QString& result)
    {
        if (index < 0 || index >= parked.count() || !parked.at(index).isWrite)
            return false;
        const Parked p = parked.takeAt(index);
        notifyWriteComplete(p.operationId, p.name, result);
        return true;
    }

    // Compatibility spelling for older behavior fixtures. It now resolves the
    // oldest undelivered write record and emits its explicit operation id.
    void notifyWriteComplete(const QString& name, const QString& result)
    {
        for (Parked& p : parked) {
            if (!p.isWrite || p.delivered || p.name != name)
                continue;
            p.delivered = true;
            notifyWriteComplete(p.operationId, name, result);
            return;
        }
    }

    oracle::TransferOracle oracle_;

    QStringList entryNames;
    QDialog* dialog = nullptr;                   // where the close is sent
    std::function<void()> closeAction;           // what the user does, inside the loop
    std::function<void()> nextAction;            // ...and what happens inside the NEXT one (TEST-090)
    ReapLog* log = nullptr;                      // per-store reap record, when one store is not enough
    QObject* closeActionContext = nullptr;       // who it is delivered to (default: dialog)
    bool blockInReaddir = false;                 // does readdir run a nested loop too?
    bool blockInOpen = false;                    // does open() run a nested loop too? (TEST-075)
    bool openFailMode = false;                   // does open() FAIL, taking start()'s open-failure branch? (TEST-077)
    bool blockInWrite = false;                   // does writeFile run a nested loop? (TEST-079)
    bool writeSucceeds = true;                   // ...and does it report the upload started?
    bool completeWrite = false;                  // ...and does writeComplete ever arrive? (TEST-080)
    bool completeRead = true;                    // ...and does readComplete? (TEST-087 drives it itself)
    bool failRead = false;                       // ...or does the read REFUSE instead? (TEST-102)
    std::function<void()> afterCompletionAction; // what the user does behind that completion (TEST-102)
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

// TEST-103 (A3-R027-F5) — WAS THE PARSED RIDE FREED?
//
// syncNext/uploadNext own every RideFile RideFileFactory hands them, and the
// REQ-027 abort branches return with one in hand: without their `delete ride`
// the activity leaks once per aborted upload. The suite cannot see that with
// ASan's leak checker - the ctest entry runs with detect_leaks=0
// (unittests/Core/garminconnect/CMakeLists.txt:1457) precisely because this
// target leaks elsewhere - so the RIDE COUNTS ITS OWN DESTRUCTION instead.
//
// ~RideFile is virtual (RideFile.h:240), so a subclass destructor really does
// run on the production `delete ride` through a RideFile*. Nothing in
// ImportSeamStubs.cpp is touched to get this (LSN-056: that file is compiled
// into three targets).
int created = 0;
int destroyed = 0;

void reset()
{
    action = nullptr;
    blockingMs = 300;
    opens = 0;
    resumed = false;
    created = 0;
    destroyed = 0;
}

} // namespace rideopen

// The RideFile production is handed, and whose destruction is observable. No
// behaviour of its own: it exists to be counted.
class CountingRideFile : public RideFile
{
  public:
    CountingRideFile() { ++rideopen::created; }
    ~CountingRideFile() override { ++rideopen::destroyed; }
};

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
        // stubs. TEST-103: the subclass counts its own destruction and changes
        // nothing else.
        return new CountingRideFile();
    }
};

// ---------------------------------------------------------------------------
// TEST-096 (REQ-027) — FailingRideFileReader: the row that CANNOT be parsed,
// COUNTED.
//
// Criterion (b) turns on invoking the failing reader "exactly N times", because
// that single number separates the two failure modes REQ-027 sits between: a
// driver that re-enters over-counts, a driver that stalls under-counts. Neither
// is visible in the end state alone — a stalled batch and a correct one both
// leave N rows on screen.
//
// A MISSING file cannot supply that number. TEST-093's unparseable row is a
// .tcx that is not on disk, and TcxFileReader returns NULL the moment
// file.open() fails — true to production, but it counts nothing and it conflates
// "the reader refused" with "the reader was never asked". So this reader is
// registered for ".gcfail" and does the two things the criterion needs: it
// COUNTS, and it always returns NULL.
//
// It is reached through production's own dispatch: RideFileFactory::openRideFile
// selects on the SUFFIX alone (RideFile.cpp:899-900) and never stats the file, so
// exactly one invocation reaches exactly one call. The fixture writes the files
// anyway, so the situation on disk is a corrupt activity rather than an absent
// one — which is what "row[0] is an Upload row whose local file cannot be parsed"
// describes.
// ---------------------------------------------------------------------------
namespace ridefail {

int opens = 0; // how many times the unparseable rows were actually attempted

void reset()
{
    opens = 0;
}

} // namespace ridefail

class FailingRideFileReader : public RideFileReader
{
  public:
    RideFile* openRideFile(QFile& file, QStringList& errors, QList<RideFile*>* list) const override
    {
        Q_UNUSED(file);
        Q_UNUSED(list);
        ++ridefail::opens;
        errors << QStringLiteral("TEST-096 unparseable activity");
        return NULL;
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
        // A3-R028-F3 — a SECOND activity, present only for the runs that ask for
        // one (addSecondRide, below). Null everywhere else, and the teardown
        // skips it when it is.
        RideItem* item2 = nullptr;
        RideFile* rideFile2 = nullptr;

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

    // A SECOND activity in the same athlete's cache, on demand.
    //
    // A3-R028-F3: a fixture with exactly ONE activity makes "the batch did not
    // carry on" unfalsifiable - there was nothing to carry on TO - so a slot
    // that asserts a transfer count on it is asserting the fixture's shape and
    // not the production behaviour. Any run that needs the count to be capable
    // of failing asks for this; every other run is left exactly as it was, which
    // is why this is a call and not part of addAthleteTab().
    //
    // Owned on the same terms as the first: created here, freed by
    // closeAthleteTab in the same order (item, then its RideFile).
    RideItem* addSecondRide(AthleteSlot* slot)
    {
        slot->rideFile2 = new RideFile();
        slot->rideFile2->context = slot->context;
        slot->item2 = new RideItem(slot->rideFile2, slot->context);
        slot->item2->fileName = QStringLiteral("2026_08_09_10_00_00.json");
        slot->rideCache->rides().push_back(slot->item2);
        return slot->item2;
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
        // ...and the second one, for the runs that asked for it (A3-R028-F3).
        delete slot->item2;
        delete slot->rideFile2;
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
        slot->item2 = nullptr;
        slot->rideFile2 = nullptr;
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

        // TEST-096 (REQ-027) — the ride file whose reader COUNTS and then
        // REFUSES. Same real factory singleton, same production dispatch.
        RideFileFactory::instance().registerReader(QStringLiteral("gcfail"), QStringLiteral("TEST-096 failing reader"),
                                                   new FailingRideFileReader);
    }

    // TEST-126 — THE ORACLE IS EVALUATED IN EVERY RUN OF THIS FILE.
    //
    // init()/cleanup() are QTest's per-function hooks: they bracket EVERY private
    // slot in this class, including the seventy that were written before this
    // oracle existed and know nothing about it. That is deliberate and is the
    // whole mechanism - an invariant that is only checked by the run written to
    // check it is a scenario, not an invariant.
    void init() { oracle::reset(); }

    void cleanup()
    {
        if (oracle::dispatches != oracle::matchedLive + oracle::matchedAbandoned + oracle::operationOutstanding)
            oracle::note(QStringLiteral("operation conservation at cleanup: dispatches=%1 completions=%2 "
                                        "outstanding=%3")
                             .arg(oracle::dispatches)
                             .arg(oracle::matchedLive + oracle::matchedAbandoned)
                             .arg(oracle::operationOutstanding));
        if (oracle::dispatches > 0 && oracle::conservationSamples == 0)
            oracle::note(QStringLiteral("operation conservation was not sampled for this oracle-scoped run"));
        // ORCH-033 — the `observerDestroyedInDelivery` term is a NARROW
        // exemption, not a softening. A run where the oracle simply stopped
        // resampling is still caught: the exemption applies only when the
        // observer was destroyed by the very delivery it was observing, which
        // is a state the store records on its way out and cannot fake. If any
        // matched completion left an observer alive, that one owed a resample
        // and its absence still fails here.
        if (oracle::matchedLive + oracle::matchedAbandoned > 0 && oracle::completionRemovalSamples == 0 &&
            oracle::observerDestroyedInDelivery == 0)
            oracle::note(QStringLiteral("operation conservation was not resampled after terminal removal"));
        if (qEnvironmentVariableIsSet("GC_ORACLE_TRACE"))
            qInfo("ORACLE %s: %s", QTest::currentTestFunction(), qPrintable(oracle::summary()));
        if (oracle::violations.isEmpty())
            return;
        // Every breach, in the order they happened, plus the run's own numbers -
        // an oracle that says only "something was wrong" is not usable evidence.
        const QString detail = oracle::violations.join(QStringLiteral("\n  ")) + QStringLiteral("\n  (") +
                               oracle::summary() + QStringLiteral(")");
        QFAIL(qPrintable(QStringLiteral("TEST-126 invariant violated during this run:\n  ") + detail));
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
        CompletedReadPE,   // completedRead's processEvents  (:2007) -> syncNext()  (:2009)
        FailedReadPE,      // failedRead's                   (:2054) -> syncNext()  (:2056)
        CompletedWritePE,  // completedWrite's               (:2143) -> syncNext()  (:2145)
        UploadNextParsePE, // uploadNext's parse-failure     (:2101) -> the loop's
                           //   own `rideListUp->invisibleRootItem()`  (:2065)

        // TEST-101 (A3-R027-F1) — syncNext's parse-failure processEvents
        // (CloudService.cpp:2120) -> the `aborted` read at :2146 and the loop's
        // own `rideListSync->invisibleRootItem()` after the `continue`.
        //
        // THE GUARD THAT CHANGED STATUS WITHOUT APPEARING IN A DIFF (LSN-058).
        // The bail at :2121 is not new - it predates REQ-027. What DEC-032
        // changed is what stands behind it: before, this branch fell through to
        // an unconditional `return true` and touched nothing afterwards, so the
        // bail was dead code and mutating it changed nothing. Now it stands in
        // front of a loop that KEEPS ITERATING on `this`, and `this` is destroyed
        // synchronously when the athlete tab closes (DEC-030). Deleting :2121
        // left the whole suite green until this frame existed, because the four
        // frames above drive a readFile row on the sync tab and a parse-failure
        // row only on the UPLOAD tab - the one combination not covered was
        // sync-tab parse failure.
        SyncNextParsePE
    };

    struct CompletionOutcome
    {
        bool teardownFired = false;      // the route actually executed
        bool teardownInsideSlot = false; // ...and landed while the slot was running
        bool dialogGoneAtEnd = false;    // the tab teardown really did destroy it
        bool timedOut = false;
        int upListCount = 0;    // (UploadNextParsePE premise: there was a row to fail on)
        int readFileCalls = 0;  // did the sync carry on behind a destroyed dialog?
        int syncListCount = 0;  // (SyncNextParsePE premise: there was a row to fail on)
        int failOpens = 0;      // ...and the parse-failure branch really was reached
        int writeFileCalls = 0; // DEC-036: the WRITE frame's counterpart of readFileCalls
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
    // teardown ARMED to run inside that slot's own processEvents().
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call  [event delivery: scopeLevel bumped]
    //          -> FakeAthleteWindow + one athlete tab
    //          -> CloudServiceSyncDialog(ctx, store) + WA_DeleteOnClose
    //          -> start() / open() / select all / Download   [the user's sync]
    //          -> arm: the athlete teardown, on the next counted row
    //          -> queued: the completion slot
    //                 -> progressBar->setValue(++downloadcounter)  [stage 1]
    //                 -> processEvents()                          [stage 2: the
    //                                                              teardown runs]
    //                 -> the guard under test reads a DESTROYED `this`
    //
    // The teardown used to be POSTED behind the completion instead, on the
    // assumption that the slot's processEvents() would deliver it. It does not
    // reliably: under QPA `minimal` and under wayland it was still pending when
    // the guard ran, `self` was not null, and the slot handed control to the next
    // row - which is what this run exists to forbid. See the insideframe block
    // comment for the measurement and for why both seams used now are ordinary
    // synchronous calls.
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
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5; // nothing in these runs needs a long suspension

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

                if (frame == SyncNextParsePE || frame == CompletedWritePE) {
                    // TEST-101 — the athlete's one activity becomes the SYNC
                    // list's one UPLOAD row (CloudService.cpp:1774). `planned` is
                    // not initialised by this target's RideItem stand-in and the
                    // upload walk skips planned rides (:1723).
                    //
                    // THE SUFFIX IS THE DIFFERENCE BETWEEN THE TWO FRAMES:
                    //   .gcfail   FailingRideFileReader's, so RideFileFactory
                    //             hands syncNext a null ride and the parse-failure
                    //             branch executes for real (SyncNextParsePE).
                    //   .gcblock  BlockingRideFileReader's, so the ride OPENS and
                    //             syncNext goes on to store->writeFile - which is
                    //             what arms the write this run then completes
                    //             (CompletedWritePE, see the note on the delivery
                    //             below). That reader reads a real file, so one is
                    //             put on disk for it.
                    const bool parseable = (frame == CompletedWritePE);
                    slot->item->fileName = slot->item->dateTime.toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss")) +
                                           (parseable ? QStringLiteral(".gcblock") : QStringLiteral(".gcfail"));
                    slot->item->path = slot->context->athlete->home->activities().absolutePath();
                    slot->item->planned = false;

                    if (parseable) {
                        QDir().mkpath(slot->item->path);
                        QFile f(slot->item->path + "/" + slot->item->fileName);
                        f.open(QIODevice::WriteOnly);
                        f.write("gcblock");
                        f.close();

                        // A3-R028-F3 — A SECOND UPLOAD ROW, and it is what makes
                        // this frame's transfer count mean anything. With one row
                        // in the list, "the batch did not carry on uploading
                        // behind a destroyed dialog" holds whether the slot stands
                        // down or not: there is no second row to carry on TO, so
                        // writeFileCalls is 1 either way and the assertion is
                        // measuring the fixture. With two, the count is capable of
                        // reading 2, and the slot's claim becomes falsifiable.
                        RideItem* second = win->addSecondRide(slot);
                        second->dateTime = QDateTime::currentDateTime().addDays(-2);
                        second->fileName = second->dateTime.toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss")) +
                                           QStringLiteral(".gcblock");
                        second->path = slot->item->path;
                        second->planned = false;
                        QFile f2(second->path + "/" + second->fileName);
                        f2.open(QIODevice::WriteOnly);
                        f2.write("gcblock");
                        f2.close();
                    }
                }

                BlockingStore* store = new BlockingStore(slot->context);
                // TEST-101 — NOTHING remote for the sync-parse frame: a remote
                // entry would put DOWNLOAD rows in the sync list AHEAD of the
                // upload row (:1682), and syncNext would take its readFile branch
                // and return long before reaching the parse-failure branch.
                // Nothing remote for the two frames whose subject is the SYNC
                // list's upload row: a remote entry would put DOWNLOAD rows ahead
                // of it (:1682) and syncNext would take its readFile branch and
                // never reach the write.
                store->entryNames =
                    (frame == SyncNextParsePE || frame == CompletedWritePE) ? QStringList() : threeActivities();
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

                    // ARMED before the click: the first row uploadNext counts is
                    // the one it fails to parse (CloudService.cpp:2531), and the
                    // processEvents() one line below that (:2533) is the frame
                    // under test. Nothing earlier in this batch counts a row -
                    // the "File exists" skip (:2464) needs a matching cloud
                    // entry, and this run's ride is dated a day back precisely so
                    // that there is none.
                    insideframe::atTheNextCountedRow(dialog, teardown);
                    inSlot = true;
                    dialog->downloadClicked(); // -> uploadNext()
                    inSlot = false;
                } else if (frame == SyncNextParsePE) {
                    // The SYNC tab - index 2 only because this store advertises
                    // Upload (:1059-1061) - then Select All and Synchronize. As
                    // above: the parse-failure branch counts the row it failed on
                    // (:2123) and then suspends (:2125), and that suspension is
                    // the frame under test.
                    if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                        tabs->setCurrentIndex(2);
                    dialog->selectAllSyncChanged(Qt::Checked);
                    if (QTreeWidget* s = rideListWithHeader(dialog, QStringLiteral("Source")))
                        out.syncListCount = s->invisibleRootItem()->childCount();

                    insideframe::atTheNextCountedRow(dialog, teardown);
                    inSlot = true;
                    dialog->downloadClicked(); // -> syncNext()
                    inSlot = false;
                } else {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    // A3-R028-F3 — how many rows the batch HAD, so the transfer
                    // count asserted of it is read against the work that was
                    // available rather than against nothing.
                    if (QTreeWidget* s = rideListWithHeader(dialog, QStringLiteral("Source")))
                        out.syncListCount = s->invisibleRootItem()->childCount();
                    // -> syncNext(): listindex=1 and ONE outstanding transfer,
                    // which reports nothing back of its own accord (completeRead
                    // off for the two read frames, completeWrite off for the write
                    // one). For CompletedReadPE/FailedReadPE that transfer is
                    // store->readFile on the sync list's first DOWNLOAD row; for
                    // CompletedWritePE the list holds one UPLOAD row instead and
                    // the transfer is store->writeFile.
                    dialog->downloadClicked();

                    // AMENDED 2026-08-18 (DEC-garmin-036). These three deliveries
                    // used to be FABRICATED: a freshly `new`ed QByteArray and a
                    // remotename copied out of the fixture, neither of them the
                    // transfer the dialog had actually issued. Production now
                    // carries a per-transfer ticket, so a completion that does not
                    // match the outstanding transfer is swallowed - correctly -
                    // and a fabricated one never reaches the frame this run exists
                    // to suspend in. So each delivery is now the LIVE one:
                    // `obs::lastBuffer` is the exact buffer the driver preallocated
                    // and passed to readFile, and `obs::lastWriteName` is the exact
                    // remotename it passed to writeFile.
                    //
                    // This STRENGTHENS the run rather than weakening it: the frame
                    // under test is now reached the way production reaches it. The
                    // buffer is the dialog's to free on both read paths, which is
                    // also one fewer leak than the fabricated version had.
                    // ORCH-033, the SECOND lifetime window. This delivery is
                    // QUEUED, so it runs one event-loop turn later - and a
                    // teardown from an EARLIER frame in this same run can have
                    // destroyed the dialog (and with it the store) before it
                    // fires. Raw captures would then call into freed memory
                    // before the base notification is even reached, which the
                    // self-guard above cannot help with because it is never
                    // entered. Guarded handles make a late callback a no-op.
                    QPointer<CloudServiceSyncDialog> dialogHandle(dialog);
                    QPointer<BlockingStore> storeHandle(store);
                    QMetaObject::invokeMethod(
                        qApp,
                        [&, dialogHandle, storeHandle, frame]() {
                            if (dialogHandle.isNull() || storeHandle.isNull())
                                return;
                            inSlot = true;
                            switch (frame) {
                            case CompletedReadPE:
                                dialogHandle->completedRead(obs::lastBuffer, obs::lastReadName,
                                                            QStringLiteral("Completed."));
                                break;
                            case FailedReadPE:
                                dialogHandle->failedRead(obs::lastBuffer, obs::lastReadName,
                                                         QStringLiteral("service refused"));
                                break;
                            case CompletedWritePE:
                                storeHandle->deliverParkedWrite(0, QStringLiteral("Completed."));
                                break;
                            case UploadNextParsePE:
                            case SyncNextParsePE:
                                break; // handled above
                            }
                            inSlot = false;
                        },
                        Qt::QueuedConnection);
                    // The three completion slots all count the row they just
                    // finished and then suspend, and that suspension is the frame
                    // under test. Arming here rather than one line into the slot
                    // is safe because nothing between this statement and the slot
                    // counts a row: syncNext's download branch has no
                    // progressBar->setValue at all, and neither does the success
                    // path of its upload branch (only the parse-failure branch
                    // counts, and CompletedWritePE's row parses).
                    insideframe::atTheNextCountedRow(dialog, teardown);
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
        out.writeFileCalls = obs::writeFileCalls;
        out.failOpens = ridefail::opens;
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
        // AMENDED 2026-08-18 (DEC-garmin-036): this frame's run is now a WRITE
        // from end to end - upload rows in the sync list, nothing remote - so
        // the standing-down it proves is counted in writeFileCalls. It used to
        // assert one READ, which was true only because this frame borrowed the
        // read frames' fixture and then fabricated a write completion the dialog
        // had never issued. Same claim, now measured on the channel under test:
        // the batch did not carry on uploading behind a destroyed dialog.
        //
        // WIDENED 2026-08-18 (A3-R028-F3), because the count above was VACUOUS as
        // it stood. The fixture had exactly one upload row, so writeFileCalls was
        // 1 whether or not the slot stood down - there was nothing to carry on to
        // - and the sentence above claimed a discrimination the run could not
        // make. It now has TWO, asserted as a premise first: with two rows the
        // count is capable of reading 2, so "exactly one write" is a statement
        // about production and not about the fixture.
        //
        // WHAT IT STILL IS NOT, measured rather than glossed: removing the
        // completedWrite self-bail this frame exists to test does NOT make this
        // count read 2. The mutant aborts first - "SUMMARY: AddressSanitizer:
        // heap-use-after-free ... CloudService.cpp:3132 in
        // CloudServiceSyncDialog::completedWrite", which is the REQ-027 `aborted`
        // re-read sitting between the removed bail and the re-drive, not the
        // `sync` read the slot header names - so the KILLING OBSERVATION is the
        // abort, exactly as this slot's header says. The widening buys a real
        // PREMISE (there WAS a second upload for the batch to carry on to, and it
        // did not) and a count that could fail in a build without ASan; it does
        // not buy a second mutation-killer, and saying otherwise would repeat the
        // mistake the finding caught.
        QVERIFY2(wrote.syncListCount >= 2,
                 qPrintable(QStringLiteral("the sync list held %1 row(s) - with fewer than two there is no second "
                                           "upload for the batch to carry on to, and the count below cannot fail")
                                .arg(wrote.syncListCount)));
        QCOMPARE(wrote.readFileCalls, 0);
        QCOMPARE(wrote.writeFileCalls, 1);

        const CompletionOutcome upload = runCompletionSlotTeardown(UploadNextParsePE);
        assertCompletionSlotStoodDown("uploadNext parse-failure processEvents (:2101)", upload);
        if (QTest::currentTestFailed())
            return;
        // The premise for this one: there WAS a row in the upload list to fail
        // on, so uploadNext really did enter its loop body.
        QVERIFY2(upload.upListCount > 0,
                 "the upload list was empty - uploadNext never entered its loop and this run proves nothing");
    }

    // -- TEST-101 (A3-R027-F1) -------------------------------------------
    // syncNext's PARSE-FAILURE branch, which DEC-032 turned from a dead end into
    // a suspension the loop resumes from - and in doing so promoted the bail at
    // CloudService.cpp:2121 from dead code to the only thing standing between a
    // destroyed dialog and the `aborted` read at :2146.
    //
    // THE KILLING OBSERVATION IS THE ASan ABORT, not a QVERIFY: the target runs
    // with halt_on_error=1, so a use-after-free ends the process and the slot
    // never returns. The assertions below are the PREMISES that make that
    // meaningful - that the teardown fired, landed inside the branch, and really
    // did destroy the dialog. Delete :2121 and this run reports
    //   AddressSanitizer: heap-use-after-free ... syncNext() CloudService.cpp:2145
    //
    // See the SyncNextParsePE comment for why the other four frames could not
    // reach this branch, and LSN-058 for the class of gap it belongs to.
    void syncNextParseFailureStandsDownWhenTheAthleteTabDiesInItsProcessEvents()
    {
        const CompletionOutcome out = runCompletionSlotTeardown(SyncNextParsePE);
        assertCompletionSlotStoodDown("syncNext parse-failure processEvents (:2120)", out);
        if (QTest::currentTestFailed())
            return;

        // ---- THE PREMISES THAT ARE SPECIFIC TO THIS FRAME. Without both of
        //      these the run could be green having never entered the branch.
        QVERIFY2(out.syncListCount > 0,
                 "the sync list was empty - syncNext never entered its loop and this run proves nothing");
        QVERIFY2(out.failOpens == 1,
                 qPrintable(QStringLiteral("the failing reader was invoked %1 time(s), not once - syncNext did not "
                                           "reach its parse-failure branch (or re-entered it behind a destroyed "
                                           "dialog) and this run proves nothing")
                                .arg(out.failOpens)));

        // ...and no store call was ever made on this path, so nothing here can be
        // confused with the readFile branch the other frames drive.
        QCOMPARE(out.readFileCalls, 0);
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

  private:
    // =====================================================================
    // TEST-095..097, TEST-099, TEST-100 (REQ-027, DEC-garmin-032) — THE
    // BATCH THAT DIES SILENTLY, AND THE THREE THINGS THAT KEEP IT ALIVE.
    // =====================================================================
    //
    // THE DEFECT (B-R026-01). syncNext's parse-failure branch
    // (CloudService.cpp:2067-2071) labels the row, runs processEvents(), bails if
    // the dialog died - and then falls out of the else and hits the UNCONDITIONAL
    // `return true` at :2075. Returning is how every OTHER branch of this function
    // stands down, and it is correct for them because they have all armed
    // something first: readFile arms completedRead/failedRead (:2273/:2326),
    // writeFile arms completedWrite (:2472). This branch arms NOTHING. So the four
    // re-entry points syncNext has - :1962, :2273, :2326, :2472 - are all
    // unreachable from here, and the batch is over: no further row is attempted,
    // the completion tail (:2079-2102) never runs, and the progress bar freezes at
    // whatever the last transferred row left it. The user is shown a dialog that
    // is doing nothing and does not say so.
    //
    // DEC-garmin-032 Option A: `continue`, which is the shape uploadNext already
    // ships for the identical branch (:2394-2420, REQ-026, TEST-093).
    //
    // WHAT THESE FIVE SLOTS COVER, and why it takes five.
    //
    //   TEST-095 (a)+(c) PROGRESSION. One unparseable row followed by a parseable
    //            one: the second row must be ATTEMPTED, and the batch must reach
    //            its completion tail. This is the slot the defect fails.
    //   TEST-096 (b)     TERMINATION + SINGLE ATTEMPT. N unparseable rows, N=2 and
    //            a scale run at N=100. The failing reader must be invoked EXACTLY
    //            N times. A re-entering driver over-counts; a stalled one
    //            under-counts. Both failure modes, one number.
    //   TEST-097 (d)     THE ABORT IN THE FAILING BRANCH. Continuing means the
    //            processEvents() at :2069 is now a suspension the loop RESUMES
    //            from, so the abort it delivers has to be re-read. This is the
    //            hazard Option A creates and the reason uploadNext:2420 exists.
    //   TEST-099 (f)     S-R027-01, AND IT IS NOT NEW. Neither loop re-reads
    //            `aborted` between openRideFile (:2046 / :2374) and writeFile
    //            (:2062 / :2389), so an abort delivered during a PARSEABLE row's
    //            parse uploads it anyway. Live in committed code; RED without any
    //            of this slice.
    //   TEST-100          THE BATCH GENERATION. Two clicks in one processEvents()
    //            burst leave a STALE frame iterating alongside a NEW batch. A
    //            return-shaped branch stands the stale frame down; a
    //            continue-shaped one does not. Live in uploadNext today.
    //
    // WHAT THIS HARNESS CANNOT SEE, stated rather than faked. Criterion (b)'s
    // literal property is "the stack did not grow unboundedly"; nothing here
    // observes stack depth, and the invocation count is a PROXY for it (an
    // unbounded recursion would over-count long before it exhausted the stack, so
    // the proxy fails loudly in the same direction - but it is a proxy). RECORDED
    // RESIDUAL, not covered.
    //
    // ...and criterion (a) asks for `downloading == false`, which is PRIVATE
    // (CloudService.h:466) with no friend access from this target. The three side
    // effects asserted instead are the completion tail's: progressLabel
    // "Processed x of n successfully", downloadButton "Synchronize", and every
    // sync checkbox cleared.
    //
    // A3-R027-F7 — the button text is NOT set only by the tail: tabChanged
    // (CloudService.cpp:1828) sets it too, when the user switches tabs. So no ONE
    // of the three implies the tail on its own. Their CONJUNCTION does: only the
    // tail clears every checkbox AND writes the "...successfully" sentence, and
    // the fixture switches tabs once, before the batch starts, so a tabChanged
    // relabel cannot be what these read. `downloading=false` is one line inside
    // that same tail. It is a PROXY FOR THE PRIVATE FLAG, labelled as one
    // wherever it is asserted (B-R027-05).

    // Which loop is under test. Both are the SAME defect in two functions, and
    // scope item 2 and the batch-generation rider apply to both, so every fixture
    // below runs on either.
    enum BatchTab {
        SyncTabRun,  // tabs index 2 -> syncNext
        UploadTabRun // tabs index 1 -> uploadNext
    };

    enum BatchAbort {
        NoAbort,
        AbortInParseFailure, // delivered by the parse-failure processEvents (:2069 / :2396)
        AbortInRideOpen,     // delivered from INSIDE openRideFile (:2046 / :2374) - clause (f)
        AbortThenRestart     // two clicks in one burst - the batch-generation rider
    };

    struct BatchSpec
    {
        BatchTab tab = SyncTabRun;
        int failRows = 1;                 // rows whose reader counts and returns NULL
        bool trailingParseableRow = true; // ...plus one row that yields a RideFile
        BatchAbort abort = NoAbort;
        bool completeWrite = true;        // does writeComplete ever arrive?
        bool sampleBarInRideOpen = false; // read the bar from inside the parseable row's open
    };

    struct BatchOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the situation it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        QString row0File;
        QString lastRowFile;
        QString row0Action; // sync list column 6: "Upload" or "Download"
        QString lastRowAction;
        bool abortDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartDelivered = false;
        bool restartTookTheStartBranch = false;
        bool parseableOpenResumed = false;

        // -- the verdict
        int failOpens = 0;      // the counting always-fails reader
        int rideOpens = 0;      // the parseable reader
        int writeFileCalls = 0; // store->writeFile
        QString lastWriteName;  // ...and for WHICH row
        QStringList statuses;   // column 7 of every row, in row order
        int progressValue = -1;
        int progressMax = -1;
        int barAtParseableOpen = -1; // the bar, sampled mid-flight
        QString progressText;
        QString buttonTextAtEnd;
        int stillCheckedAtEnd = -1; // the tail clears every checkbox (:2092-2095)
        int ridesCreated = 0;       // TEST-103: RideFiles handed to the loop...
        int ridesDestroyed = 0;     // ...and RideFiles it freed
    };

    // The unparseable rows. Distinct times so the row order is well defined and
    // the dialog's own targetnosuffix bookkeeping (:1760-1769) cannot collide
    // them; the scale run needs 100, hence minutes rather than hours.
    static QString failActivity(int i)
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_%1_%2_00.gcfail")
                                                                                 .arg(i / 60, 2, 10, QLatin1Char('0'))
                                                                                 .arg(i % 60, 2, 10, QLatin1Char('0'));
    }

    // The row that CAN be parsed. 23:00 so it sorts last on every ascending
    // column the two lists offer, which is what makes it "row[1]" / "the last
    // row" rather than something the sort could move.
    static QString parseableActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_23_00_00.gcblock");
    }

    // One run:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call (event delivery, so scopeLevel is bumped)
    //          -> owner QWidget                       [stands in for the tab]
    //          -> CloudServiceSyncDialog, a child of it
    //          -> Sync|Upload tab / Select all / Synchronize
    //               -> syncNext|uploadNext
    //                    -> row[0..n-1]  openRideFile -> NULL -> parse failure
    //                    -> row[n]       openRideFile -> RideFile -> writeFile
    //
    // The store advertises NOTHING remote (entryNames empty), which matters twice:
    // no remote entry means no Download row can appear in the sync list ahead of
    // the Upload rows (:1682), and no local ride is marked as already existing, so
    // the "File exists" skip (:2114 / :2347) cannot swallow a row.
    //
    // Nothing here destroys the dialog: `self` stays non-null throughout, so a
    // lifetime bail cannot be what produces any of these verdicts.
    BatchOutcome runBatch(const BatchSpec& spec)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        BatchOutcome out;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QEventLoop appLoop;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        // The activities, on disk and in the ride cache. Written for real so that
        // the situation is a corrupt activity rather than a missing one - the
        // reader's refusal is then the reader's own verdict.
        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < spec.failRows; i++) {
            const QString name = failActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcfail");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(i / 60, i % 60, 0));
            item->planned = false; // the upload list skips planned rides (:1723)
            items << item;
        }
        if (spec.trailingParseableRow) {
            const QString name = parseableActivity();
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(23, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // Whether the upload is ever ACKNOWLEDGED. With it off the batch
                // parks after writeFile exactly as a real one waits on the
                // network - which is the state TEST-100 needs, because a batch
                // that has already run its completion tail has unchecked every
                // row and a stale frame would then find nothing left to transfer.
                store->completeWrite = spec.completeWrite;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                dialogGuard = dialog;
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // The tab, picked as the user picks it. Index 2 is the Sync tab
                // ONLY because this store advertises Upload (:1059-1061).
                QTreeWidget* list = nullptr;
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(spec.tab == SyncTabRun ? 2 : 1);
                if (spec.tab == SyncTabRun) {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Source"));
                } else {
                    dialog->selectAllUpChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("File"));
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.listCount > 0) {
                        out.row0File = root->child(0)->text(1);
                        out.row0Action = root->child(0)->text(6);
                        out.lastRowFile = root->child(out.listCount - 1)->text(1);
                        out.lastRowAction = root->child(out.listCount - 1)->text(6);
                    }
                }

                // Watched from here on, rather than looked up again by label:
                // this is the dialog's one download/abort button and its TEXT is
                // the observable.
                QPushButton* abortButton = pushButtonWithText(dialog, QStringLiteral("Synchronize"));
                if (abortButton == nullptr)
                    abortButton = pushButtonWithText(dialog, QStringLiteral("Upload"));

                // THE ACTION DELIVERED FROM INSIDE THE PARSEABLE ROW'S OPEN.
                // BlockingRideFileReader queues it and then runs its nested loop
                // (:596-610), so it lands while syncNext/uploadNext is suspended
                // between openRideFile and writeFile - the S-R027-01 window.
                if (spec.abort == AbortInRideOpen || spec.sampleBarInRideOpen) {
                    rideopen::blockingMs = 300;
                    rideopen::action = [&, dialog]() {
                        if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
                            out.barAtParseableOpen = bar->value();
                        if (spec.abort != AbortInRideOpen)
                            return;
                        out.abortDelivered = true;
                        out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                        dialog->downloadClicked();
                        out.abortTookTheAbortBranch =
                            (abortButton != nullptr && abortButton->text() == QStringLiteral("Download"));
                    };
                }

                // THE ABORT, and (for the generation rider) THE RESTART BEHIND
                // IT. Both queued on qApp BEFORE the batch starts, so ONE
                // processEvents() burst delivers them back to back - the posting
                // discipline TEST-087 and TEST-093 already use.
                if (spec.abort == AbortInParseFailure || spec.abort == AbortThenRestart) {
                    QMetaObject::invokeMethod(
                        qApp,
                        [&, dialog]() {
                            out.abortDelivered = true;
                            out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                            dialog->downloadClicked();
                            // The abort branch relabels that same button
                            // "Download" (:1914), so this is the observable proof
                            // that :1920-1928 ran and `aborted` is now true.
                            out.abortTookTheAbortBranch =
                                (abortButton != nullptr && abortButton->text() == QStringLiteral("Download"));
                        },
                        Qt::QueuedConnection);
                }
                if (spec.abort == AbortThenRestart) {
                    QMetaObject::invokeMethod(
                        qApp,
                        [&, dialog]() {
                            out.restartDelivered = true;
                            // downloading is false now, so THIS click takes the
                            // START branch (:1919-1926): aborted=false,
                            // listindex=0, counters zeroed, and syncNext/
                            // uploadNext re-driven from scratch - all of it from
                            // INSIDE the first batch's suspended frame.
                            dialog->downloadClicked();
                            out.restartTookTheStartBranch =
                                (abortButton != nullptr && abortButton->text() == QStringLiteral("Abort"));
                        },
                        Qt::QueuedConnection);
                }

                dialog->downloadClicked(); // -> syncNext() / uploadNext()

                // The verdict, read the instant the batch hands control back.
                out.writeFileCalls = obs::writeFileCalls;
                out.lastWriteName = obs::lastWriteName;
                out.failOpens = ridefail::opens;
                out.rideOpens = rideopen::opens;
                out.parseableOpenResumed = rideopen::resumed;
                // TEST-103 — read HERE, the instant the batch hands control back,
                // so nothing the fixture's own teardown frees can be mistaken for
                // the loop having freed it.
                out.ridesCreated = rideopen::created;
                out.ridesDestroyed = rideopen::destroyed;
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.stillCheckedAtEnd = 0;
                    for (int i = 0; i < root->childCount(); i++) {
                        out.statuses << root->child(i)->text(7);
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.stillCheckedAtEnd++;
                    }
                }
                if (QProgressBar* bar = dialog->findChild<QProgressBar*>()) {
                    out.progressValue = bar->value();
                    out.progressMax = bar->maximum();
                }
                out.progressText = progressLabelText(dialog);
                if (abortButton != nullptr)
                    out.buttonTextAtEnd = abortButton->text();

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // The progress LABEL, not the bar: the one widget carrying the completion
    // tail's sentence. Identified by content rather than by child order, and only
    // the tail's own wording is accepted, so an empty or "n of m selected" label
    // reads back empty and the assertions fail rather than silently matching.
    static QString progressLabelText(QWidget* dialog)
    {
        const QList<QLabel*> labels = dialog->findChildren<QLabel*>();
        for (QLabel* l : labels)
            if (l->text().contains(QStringLiteral("successfully")))
                return l->text();
        return QString();
    }

  private slots:

    // -- TEST-095 (REQ-027 (a)+(c)) --------------------------------------
    // A batch of two checked Sync Upload rows, row[0] unparseable and row[1]
    // parseable. row[1] must be ATTEMPTED and the batch must reach its completion
    // tail. See the block comment above for the defect.
    //
    // RED, before the fix:
    //   FAIL!  : ... the batch died on the unparseable row: store->writeFile was
    //            called 0 time(s), so row[1] was never attempted
    void aParseFailureInSyncNextMustNotKillTheRestOfTheBatch()
    {
        BatchSpec spec;
        spec.tab = SyncTabRun;
        spec.failRows = 1;
        spec.trailingParseableRow = true;
        spec.sampleBarInRideOpen = true; // (c): read the bar mid-flight
        const BatchOutcome out = runBatch(spec);

        QVERIFY2(out.timedOut == false, "the sync never came back - a guard wedged it");

        // ---- THE PREMISES. Every one is a way this run could be green while
        //      seeing nothing at all (LSN-047, LSN-050).
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QCOMPARE(out.row0File, failActivity(0));
        QCOMPARE(out.lastRowFile, parseableActivity());
        // Both must be UPLOAD-side sync rows, or the parse path at :2014-2074 is
        // never reached at all (syncNext:1981 branches on this cell).
        QCOMPARE(out.row0Action, QStringLiteral("Upload"));
        QCOMPARE(out.lastRowAction, QStringLiteral("Upload"));
        QCOMPARE(out.failOpens, 1);
        QCOMPARE(out.statuses.value(0), QStringLiteral("Parse failure"));

        // ---- (a) PROGRESSION. row[1] attempted, exactly once, and it is row[1]
        //      that was written.
        QVERIFY2(out.writeFileCalls == 1,
                 qPrintable(QStringLiteral("the batch died on the unparseable row: store->writeFile was called %1 "
                                           "time(s), so row[1] was never attempted")
                                .arg(out.writeFileCalls)));
        QCOMPARE(out.rideOpens, 1);
        QVERIFY2(
            out.lastWriteName.startsWith(QFileInfo(parseableActivity()).baseName()),
            qPrintable(QStringLiteral("writeFile was called for \"%1\", which is not row[1]").arg(out.lastWriteName)));

        // ---- (a) THE COMPLETION TAIL. `downloading == false` is PRIVATE
        //      (CloudService.h:466); these three are its TAIL-EXCLUSIVE side
        //      effects and stand in for it (B-R027-05).
        QCOMPARE(out.progressText, QStringLiteral("Processed 1 of 2 successfully"));
        QCOMPARE(out.buttonTextAtEnd, QStringLiteral("Synchronize"));
        QCOMPARE(out.stillCheckedAtEnd, 0);

        // ---- (c) VISIBILITY. The failed row NAMES its failure; the bar advanced
        //      FOR IT (sampled from inside row[1]'s open, i.e. after row[0] was
        //      counted and before row[1] could be); and the tail's `successful`
        //      is strictly less than `downloadtotal` - 1 of 2, read off the tail's
        //      own sentence, which is the only place either private counter is
        //      visible.
        QVERIFY2(!out.statuses.value(0).isEmpty(), "the failed row's status cell is empty - the user is told nothing");
        QVERIFY2(out.barAtParseableOpen == 1,
                 qPrintable(QStringLiteral("the progress bar did not advance for the failed row: it read %1 while "
                                           "row[1] was being opened, not 1")
                                .arg(out.barAtParseableOpen)));
        QCOMPARE(out.progressValue, 2);
        QCOMPARE(out.progressMax, 2);
    }

    // -- TEST-096 (REQ-027 (b)) ------------------------------------------
    // N checked all-unparseable Sync Upload rows. The batch must TERMINATE in its
    // completion tail with every row labelled and the bar at maximum, and the
    // failing reader must have been invoked EXACTLY N times - a re-entering driver
    // over-counts, a stalled one under-counts.
    //
    // RED, before the fix:
    //   FAIL!  : ... N=2: the failing reader was invoked 1 time(s), not 2 - the
    //            batch neither completed nor re-entered, it STALLED
    void anAllUnparseableSyncBatchTerminatesHavingAttemptedEachRowExactlyOnce()
    {
        assertAllUnparseableBatch(SyncTabRun, 2);
        if (QTest::currentTestFailed())
            return;

        // THE SCALE RUN. Two rows cannot distinguish "advanced once" from
        // "advanced correctly"; a hundred can, and it is also where an unbounded
        // re-entry would show up as a count in the thousands.
        assertAllUnparseableBatch(SyncTabRun, 100);
        if (QTest::currentTestFailed())
            return;

        // THE UPLOAD TAB, which criterion (b) does not name. It is here because
        // B-R027-04 - giving uploadNext's parse-failure branch the same
        // ++downloadcounter syncNext's now has - is otherwise an UNCOVERED change:
        // with only the sync runs above, deleting that line leaves the whole suite
        // green (measured: mutation M7 survived until this run existed).
        //
        // HONESTY ABOUT WHAT THIS RUN IS. uploadNext has iterated past parse
        // failures since REQ-026, so every assertion in the shared verdict except
        // the two about the BAR is a PIN on behaviour that already worked. The bar
        // assertions are the coverage; the rest is regression fencing.
        assertAllUnparseableBatch(UploadTabRun, 2);
    }

    // -- TEST-097 (REQ-027 (d)) ------------------------------------------
    // An abort delivered by the processEvents() INSIDE the parse-failure branch
    // must stop the batch. Option A turns that call from a dead end into a
    // suspension the loop resumes from, so this is the hazard the fix CREATES.
    //
    // THIS SLOT IS A PIN, NOT COVERAGE, AGAINST UNMODIFIED PRODUCTION (LSN-022
    // honesty). Before the fix the branch returns unconditionally at :2075, so
    // nothing further happens for the trivial reason that nothing further ever
    // happens - the slot passes while the defect it is aimed at is not even
    // reachable. It is load-bearing against the FIX: remove the `if (aborted ==
    // true) return true;` from syncNext's parse-failure branch and it fails with
    //   FAIL!  : ... syncNext kept iterating after the user aborted: row[1] was
    //            uploaded anyway (store->writeFile was called 1 time(s))
    void anAbortInsideSyncNextsParseFailureMustStopTheBatch()
    {
        BatchSpec spec;
        spec.tab = SyncTabRun;
        spec.failRows = 1;
        spec.trailingParseableRow = true;
        spec.abort = AbortInParseFailure;
        const BatchOutcome out = runBatch(spec);

        QVERIFY2(out.timedOut == false, "the sync never came back - a guard wedged it");

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QCOMPARE(out.row0File, failActivity(0));
        QCOMPARE(out.lastRowFile, parseableActivity());
        QVERIFY2(out.abortDelivered, "the queued abort never ran - this run proves nothing");
        QVERIFY2(out.sawAbortLabel,
                 "the download button was not labelled \"Abort\" while the batch ran, so downloadClicked() could "
                 "not have been the abort control - this run proves nothing");
        QVERIFY2(out.abortTookTheAbortBranch,
                 "downloadClicked() did not take its abort branch (CloudService.cpp:1920-1928), so `aborted` was "
                 "never set - this run proves nothing");
        QCOMPARE(out.statuses.value(0), QStringLiteral("Parse failure"));

        // ---- THE POINT, observed three independent ways: row[1] was never
        //      written, never opened, and never even labelled.
        //
        // A3-R027-F9 — WHICH OF THESE IS THE KILLING ASSERTION. `writeFileCalls`
        // encodes clause (d) verbatim, but it is NOT what kills the removal of
        // the abort re-read at CloudService.cpp:2146: with that guard gone the
        // loop still advances onto row[1], and the S-R027-01 guard (:2088) then
        // catches the write one step later, so this stays 0. `rideOpens` below is
        // the assertion that dies. DO NOT DROP IT as redundant - clause (d) goes
        // uncovered the moment it goes.
        QVERIFY2(out.writeFileCalls == 0,
                 qPrintable(QStringLiteral("syncNext kept iterating after the user aborted: row[1] was uploaded "
                                           "anyway (store->writeFile was called %1 time(s))")
                                .arg(out.writeFileCalls)));
        QVERIFY2(out.rideOpens == 0,
                 qPrintable(QStringLiteral("syncNext kept iterating after the user aborted: row[1]'s ride file was "
                                           "OPENED (%1 open(s)) - the abort did not stop the loop, it only failed "
                                           "to reach writeFile")
                                .arg(out.rideOpens)));
        QVERIFY2(out.statuses.value(1).isEmpty(),
                 qPrintable(QStringLiteral("syncNext advanced onto row[1] after the abort - its status cell reads "
                                           "\"%1\"")
                                .arg(out.statuses.value(1))));

        // ...and it stopped by BAILING, not by running the completion tail, which
        // would have relabelled the button and cleared `aborted` behind the user.
        QCOMPARE(out.buttonTextAtEnd, QStringLiteral("Download"));

        // ---- TEST-104 (A3-R027-F6) — WHERE THE COUNTER IS INCREMENTED, MEASURED.
        //      row[0] failed to parse and the loop then stood down on the abort.
        //      The row was still one of the two the user asked for and it is
        //      finished with, so it is counted: the bar reads 1, not 0. Move
        //      CloudService.cpp:2118 down to just before the `continue` and this
        //      is the assertion that dies - it is the only observation in the
        //      suite that can tell the two placements apart, because on every
        //      other path the increment happens either way.
        QVERIFY2(out.progressValue == 1,
                 qPrintable(QStringLiteral("the row that failed to parse was not counted before the loop stood "
                                           "down on the abort: the bar reads %1, not 1")
                                .arg(out.progressValue)));
    }

    // -- TEST-099 (REQ-027 (f), S-R027-01) -------------------------------
    // An abort delivered inside openRideFile of a PARSEABLE row must leave
    // writeFile uncalled - on BOTH loops. Neither re-reads `aborted` between
    // openRideFile (:2046 / :2374) and writeFile (:2062 / :2389) today, so an
    // abort during a parseable row's parse uploads it anyway. This is LIVE in
    // committed code and RED without any other part of this slice.
    //
    // RED, before the fix:
    //   FAIL!  : ... syncNext uploaded the row after the user aborted during its
    //            parse: store->writeFile was called 1 time(s)
    void anAbortInsideOpenRideFileMustLeaveTheRowUnwritten()
    {
        assertAbortInOpenStopsTheWrite(SyncTabRun, "syncNext");
        if (QTest::currentTestFailed())
            return;

        assertAbortInOpenStopsTheWrite(UploadTabRun, "uploadNext");
    }

    // -- TEST-100 (REQ-027, the batch-generation rider) ------------------
    // TWO CLICKS IN ONE processEvents() BURST.
    //
    // The batch is suspended in its parse-failure branch. That call delivers click
    // one, which downloadClicked (:1920-1928) turns into an ABORT; and then click
    // two, which the same slot (:1919-1926) turns into a fresh START - aborted
    // back to false, listindex back to 0, the counters zeroed and the loop
    // RE-DRIVEN, all from inside the first batch's own frame. When that returns,
    // the suspended frame resumes, finds `aborted == false` (the restart cleared
    // it, so the abort re-read cannot see this) and, being continue-shaped, KEEPS
    // GOING - a stale driver iterating the same list as the live one, addressing
    // rows by a `listindex` that now belongs to somebody else (:2193/:2242/:2249/
    // :2311/:2318/:2457/:2464).
    //
    // MEASURED, NOT ARGUED, and on the loop where it is LIVE TODAY: uploadNext
    // already ships the continue shape (REQ-026), so the upload run is RED against
    // unmodified production. The sync run is the same claim about the branch
    // Option A creates.
    //
    // The store does not acknowledge the upload (completeWrite off), so the
    // restarted batch PARKS after writeFile exactly as a real one waits on the
    // network. That matters: a batch that had already run its completion tail
    // would have unchecked every row (:2092-2095/:2437-2441) and the stale frame
    // would find nothing left to transfer - the hazard would hide, not vanish.
    //
    // RED, before the fix (upload run):
    //   FAIL!  : ... uploadNext: a STALE batch frame kept transferring alongside
    //            the restarted one - store->writeFile was called 2 time(s) for a
    //            single parseable row
    void aRestartInsideTheFailingBranchMustStandTheStaleFrameDown()
    {
        assertStaleFrameStandsDown(UploadTabRun, "uploadNext");
        if (QTest::currentTestFailed())
            return;

        assertStaleFrameStandsDown(SyncTabRun, "syncNext");
    }

    // -- TEST-102 (REQ-027, A3-R027-F4) ----------------------------------
    // An abort delivered by a COMPLETION SLOT's processEvents must stop the
    // batch. See the block comment on AbortAfterSite for the three sites, why
    // the entry checks cannot see this abort, and what each site uniquely proves.
    void anAbortInsideACompletionSlotMustStopTheNextTransfer()
    {
        assertAbortBehindCompletionStops(AfterCompletedRead, "completedRead");
        if (QTest::currentTestFailed())
            return;

        assertAbortBehindCompletionStops(AfterFailedRead, "failedRead");
        if (QTest::currentTestFailed())
            return;

        assertAbortBehindCompletionStops(AfterCompletedWrite, "completedWrite");
    }

    // -- TEST-129 (REQ-028, A3-R028c-F2 / ORACLE-F1) --------------------
    // A completion admits one generation, then its processEvents() delivers an
    // abort and an immediate restart. The restarted batch dispatches row[0].
    // When the old completion frame resumes it must not re-drive row[1] as a
    // second driver. Run independently through all three completion channels.
    void aRestartInsideEveryCompletionTailMustStandTheOldFrameDown_data()
    {
        QTest::addColumn<int>("siteValue");
        QTest::addColumn<QString>("channel");
        QTest::newRow("completedRead") << int(AfterCompletedRead) << QStringLiteral("completedRead");
        QTest::newRow("failedRead") << int(AfterFailedRead) << QStringLiteral("failedRead");
        QTest::newRow("completedWrite") << int(AfterCompletedWrite) << QStringLiteral("completedWrite");
    }

    void aRestartInsideEveryCompletionTailMustStandTheOldFrameDown()
    {
        QFETCH(int, siteValue);
        QFETCH(QString, channel);
        const AbortAfterSite site = AbortAfterSite(siteValue);
        const AbortAfterOutcome out = runAbortBehindCompletion(site, true, true);
        const QString where = channel + QStringLiteral(": ");
        QVERIFY2(!out.timedOut, qPrintable(where + QStringLiteral("the run never came back")));
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QVERIFY2(out.abortDelivered && out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("the completion tail did not deliver its abort")));
        QVERIFY2(out.restartDelivered, qPrintable(where + QStringLiteral("the abort was not immediately restarted")));

        if (site == AfterCompletedWrite) {
            QVERIFY2(out.writeFileCalls == 2,
                     qPrintable(where + QStringLiteral("the old completion frame became a second driver: %1 "
                                                       "writes, expected the original plus restarted row[0]")
                                            .arg(out.writeFileCalls)));
            QCOMPARE(out.rideOpens, 2);
        } else {
            QVERIFY2(out.readFileCalls == 2,
                     qPrintable(where + QStringLiteral("the old completion frame became a second driver: %1 "
                                                       "reads, expected the original plus restarted row[0]")
                                            .arg(out.readFileCalls)));
        }
        QVERIFY2(out.row1Status != QStringLiteral("Downloading") && out.row1Status != QStringLiteral("Uploading"),
                 qPrintable(where + QStringLiteral("the stale tail started row[1]: '%1'").arg(out.row1Status)));
    }

    // -- TEST-105 (REQ-027 (f), DEC-garmin-035) --------------------------
    // syncNext's DOWNLOAD branch. See the block comment on
    // BeforeTransferLoop for the asymmetry this exists to close and for what
    // it deliberately does NOT claim.
    //
    // RED, before the fix:
    //   FAIL!  : ... syncNext:2058: the transfer was ISSUED for a row the user
    //            had already aborted: store->readFile was called 1 time(s),
    //            expected 0
    //
    // The label names the GUARD's line, not the old comment's. A3-R027c-F3:
    // these strings used to read ":2011" / ":2225", carried over from the
    // pre-DEC-035 comment - and ":2225" is not even inside downloadNext (it is
    // rideCache->save() in syncNext's tail; downloadNext starts at :2231). A
    // stale file:NNN in an ASSERTION MESSAGE is read only when a test has gone
    // red, i.e. exactly when the reader is least able to doubt it, so it sent
    // whoever followed it into the wrong function. Re-derive these from the
    // guard whenever it moves.
    void anAbortAlreadySetMustStopSyncNextIssuingTheDownload()
    {
        assertAbortBeforeTransferStopsIt(InSyncNext, "syncNext:2058");
    }

    // -- TEST-106 (REQ-027 (f), DEC-garmin-035) --------------------------
    // downloadNext, the function that contained NO abort read anywhere.
    void anAbortAlreadySetMustStopDownloadNextIssuingTheDownload()
    {
        assertAbortBeforeTransferStopsIt(InDownloadNext, "downloadNext:2317");
    }

  private:
    // The shared verdict for TEST-096's runs. NOT a slot.
    void assertAllUnparseableBatch(BatchTab tab, int n)
    {
        BatchSpec spec;
        spec.tab = tab;
        spec.failRows = n;
        spec.trailingParseableRow = false;
        const BatchOutcome out = runBatch(spec);

        // The two loops' completion tails word the same sentence differently and
        // reset the button to their own tab's verb (:2182/:2171 and :2593/:2583).
        // Criterion (c) forbids touching either string - ~16 services share them
        // and changing one reopens translations - so the expectation follows the
        // code rather than the other way round.
        const bool isSync = (tab == SyncTabRun);
        const QString tailVerb = isSync ? QStringLiteral("Processed") : QStringLiteral("Uploaded");
        const QString idleButton = isSync ? QStringLiteral("Synchronize") : QStringLiteral("Upload");

        const QString where = QStringLiteral("%1 N=%2: ").arg(isSync ? "sync" : "upload").arg(n);
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the sync never came back")));

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, n);
        QCOMPARE(out.checkedRows, n);

        // ---- SINGLE ATTEMPT. The one number that catches both failure modes.
        QVERIFY2(out.failOpens == n,
                 qPrintable(where + QStringLiteral("the failing reader was invoked %1 time(s), not %2 - the batch "
                                                   "either re-entered (over-count) or STALLED (under-count)")
                                        .arg(out.failOpens)
                                        .arg(n)));
        QCOMPARE(out.writeFileCalls, 0);

        // ---- TERMINATION. The completion tail, reached - and `downloading ==
        //      false` with it (B-R027-05: PROXY, the flag is private).
        QCOMPARE(out.progressText, QStringLiteral("%1 0 of %2 successfully").arg(tailVerb).arg(n));
        QCOMPARE(out.buttonTextAtEnd, idleButton);
        QCOMPARE(out.stillCheckedAtEnd, 0);

        // ---- EVERY ROW LABELLED, and the bar at maximum.
        QCOMPARE(out.statuses.count(), n);
        QCOMPARE(out.statuses.count(QStringLiteral("Parse failure")), n);
        QCOMPARE(out.progressMax, n);
        QVERIFY2(out.progressValue == n,
                 qPrintable(where + QStringLiteral("the batch finished with the progress bar at %1 of %2 - the "
                                                   "failed rows were never counted, so the bar ends short of "
                                                   "maximum by exactly the number of unparseable activities")
                                        .arg(out.progressValue)
                                        .arg(n)));
    }

    // The shared verdict for TEST-099's two loops. NOT a slot.
    //
    // The CONTROL comes first and is what makes the abort run mean anything: the
    // same single parseable row with NO abort delivered must reach writeFile. If
    // this apparatus could not see the upload happening, the run below would pass
    // for the wrong reason (LSN-047, LSN-050).
    void assertAbortInOpenStopsTheWrite(BatchTab tab, const char* loop)
    {
        const QString where = QString::fromLatin1(loop) + QStringLiteral(": ");

        BatchSpec control;
        control.tab = tab;
        control.failRows = 0;
        control.trailingParseableRow = true;
        control.sampleBarInRideOpen = true; // arms the reader's action WITHOUT aborting
        const BatchOutcome base = runBatch(control);
        QVERIFY2(base.timedOut == false, qPrintable(where + QStringLiteral("control: never came back")));
        QCOMPARE(base.listCount, 1);
        QVERIFY2(base.rideOpens == 1, qPrintable(where + QStringLiteral("control: the row was never opened")));
        QVERIFY2(base.parseableOpenResumed,
                 qPrintable(where + QStringLiteral("control: the reader's nested loop never ran to completion")));
        QVERIFY2(base.writeFileCalls == 1,
                 qPrintable(where + QStringLiteral("control: writeFile was called %1 time(s), not once - this "
                                                   "apparatus cannot see an upload happening at all")
                                        .arg(base.writeFileCalls)));

        BatchSpec spec;
        spec.tab = tab;
        spec.failRows = 0;
        spec.trailingParseableRow = true;
        spec.abort = AbortInRideOpen;
        const BatchOutcome out = runBatch(spec);

        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the batch never came back")));

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, 1);
        QVERIFY2(out.rideOpens == 1, qPrintable(where + QStringLiteral("the row was never opened")));
        QVERIFY2(out.parseableOpenResumed,
                 qPrintable(where + QStringLiteral("the reader's nested loop never resumed - the loop was not "
                                                   "suspended where this test needs it")));
        QVERIFY2(out.abortDelivered,
                 qPrintable(where + QStringLiteral("the abort was never delivered inside openRideFile")));
        QVERIFY2(out.sawAbortLabel,
                 qPrintable(where + QStringLiteral("the download button was not labelled \"Abort\" while the batch "
                                                   "ran, so downloadClicked() could not have been the abort "
                                                   "control")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch "
                                                   "(CloudService.cpp:1920-1928), so `aborted` was never set")));

        // ---- THE POINT.
        QVERIFY2(out.writeFileCalls == 0,
                 qPrintable(where + QStringLiteral("uploaded the row after the user aborted during its parse: "
                                                   "store->writeFile was called %1 time(s)")
                                        .arg(out.writeFileCalls)));

        // ...and the row SAYS so, in the vocabulary completedWrite already uses
        // for this outcome (:2458).
        QCOMPARE(out.statuses.value(0), QStringLiteral("Aborted"));

        // ---- TEST-103 (A3-R027-F5) — AND THE RIDE IS FREED ON THE WAY OUT.
        //      The abort branch returns holding a RideFile that RideFileFactory
        //      made for it and that nothing else owns, so without its `delete
        //      ride` the activity leaks once per aborted upload. ASan's leak
        //      checker cannot say so - the ctest entry runs detect_leaks=0
        //      (CMakeLists.txt:1457) - so the ride counts its own destruction
        //      instead (~RideFile is virtual, RideFile.h:240).
        //
        //      The CONTROL above establishes the counter works on the ordinary
        //      path, where the SAME number of rides is created and freed at
        //      :2100. Here the freeing is the abort branch's own.
        QVERIFY2(base.ridesCreated == 1 && base.ridesDestroyed == 1,
                 qPrintable(where + QStringLiteral("control: %1 ride(s) created and %2 freed on the ORDINARY "
                                                   "path - this counter cannot see a ride being freed at all")
                                        .arg(base.ridesCreated)
                                        .arg(base.ridesDestroyed)));
        QVERIFY2(out.ridesCreated == 1,
                 qPrintable(where + QStringLiteral("the reader produced %1 ride(s), not 1 - nothing was in hand "
                                                   "when the abort branch ran and this proves nothing")
                                        .arg(out.ridesCreated)));
        QVERIFY2(out.ridesDestroyed == 1,
                 qPrintable(where + QStringLiteral("the abort branch returned WITHOUT freeing the ride it was "
                                                   "holding: %1 created, %2 freed - one leaked activity per "
                                                   "aborted upload")
                                        .arg(out.ridesCreated)
                                        .arg(out.ridesDestroyed)));
    }

    // =====================================================================
    // TEST-102 (REQ-027, A3-R027-F4) — THE ABORT IS NOT HONOURED ON THE READ
    // SIDE.
    // =====================================================================
    //
    // Every completion slot checks `aborted` on ENTRY and then SUSPENDS in a
    // processEvents() before handing control to the next row:
    //
    //   completedRead   :2354  ->  guard :2372  ->  syncNext() / downloadNext()
    //   failedRead      :2423  ->  guard :2432  ->  syncNext() / downloadNext()
    //   completedWrite  :2615  ->  guard :2626  ->  syncNext() / uploadNext()
    //
    // The abort the user presses is delivered BY that processEvents, so the entry
    // check cannot see it and the QPointer bail beside it answers a different
    // question (is this dialog alive, not is this transfer still wanted). Without
    // these guards the batch issues ANOTHER download after the user stopped it.
    // For GarminConnect that is a worker thread and an interpreter session
    // started for an activity the user already abandoned.
    //
    // AMENDED 2026-08-15 (DEC-garmin-035). This paragraph used to continue
    // "Neither downloadNext nor syncNext's download branch reads `aborted` at all
    // before issuing the next store->readFile (:2011 / :2225)". True when written,
    // FALSE now: DEC-035 added a read immediately ahead of both transfers
    // (TEST-105/106) exactly because the guards tested here - in a DIFFERENT
    // function, across a return - were the only thing covering them. These guards
    // remain load-bearing and individually mutation-proven; they are now the
    // OUTER of two layers. Left as an amendment rather than a silent rewrite,
    // because a comment that quietly stops being true is what this wave spent
    // three cycles learning to distrust.
    //
    // EACH SITE VERIFIED INDIVIDUALLY, not assumed identical - see the build
    // report for what each one uniquely contributes. In particular the
    // completedWrite site's WRITE is already stopped by this slice's S-R027-01
    // guard (:2088/:2507), so what the guard there uniquely prevents is the next
    // row being OPENED and relabelled; its killing assertion is rideOpens, not
    // writeFileCalls, and the slot says so.
    //
    // RED, before the fix:
    //   FAIL!  : ... completedRead: a further store->readFile was issued after
    //            the user aborted (2 read(s) in total, expected 1)
    enum AbortAfterSite {
        AfterCompletedRead, // Download tab -> completedRead:2265 -> downloadNext
        AfterFailedRead,    // Download tab -> failedRead:2399    -> downloadNext
        AfterCompletedWrite // Upload tab   -> completedWrite:2595 -> uploadNext
    };

    struct AbortAfterOutcome
    {
        bool timedOut = false;

        // -- premises
        int listCount = 0;
        int checkedRows = 0;
        bool abortDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartDelivered = false;
        bool restartTookTheStartBranch = false;
        bool slotRan = false; // the completion slot really did execute

        // -- the verdict
        int readFileCalls = 0;
        int rideOpens = 0;
        int writeFileCalls = 0;
        QString row1Status;
    };

    // The two remote activities the Download tab lists.
    //
    // THE SUFFIX IS .gcfail AND THAT IS LOAD-BEARING, not incidental. With
    // .gcblock the abort would be consumed by the NESTED EVENT LOOP inside
    // uncompressRide's reader (:2305 -> BlockingRideFileReader's loop), i.e.
    // BEFORE completedRead's REQ-026 re-read at :2329 - so TEST-094's guard
    // catches it, the slot never reaches :2354, and this run passes while proving
    // nothing about the site it names. (Measured: with .gcblock the completedRead
    // site went green before any guard existed, while failedRead - which never
    // calls uncompressRide - went RED.)
    //
    // FailingRideFileReader runs NO nested loop, so nothing between the entry
    // check and :2354 can consume the abort, and the abort is armed for the
    // counted row at :2337 - downstream of BOTH upstream reads of `aborted`.
    // completedRead then takes its no-ride branch (:2350-2352), which still
    // reaches :2354 - the suspension under test.
    static QStringList twoRemoteActivities()
    {
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        return QStringList() << (day + QStringLiteral("_13_00_00.gcfail"))
                             << (day + QStringLiteral("_14_00_00.gcfail"));
    }

    // One run. `deliverTheAbort` false is the CONTROL: identical in every other
    // respect, so a green run proves this apparatus can see the SECOND row being
    // started when nothing stops it.
    AbortAfterOutcome runAbortBehindCompletion(AbortAfterSite site, bool deliverTheAbort,
                                               bool restartImmediately = false)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        AbortAfterOutcome out;
        QEventLoop appLoop;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());
        QDir().mkpath(context->athlete->home->temp().absolutePath());

        // The upload site needs two LOCAL parseable activities; the two read
        // sites need none (their rows come from the store's listing, and a local
        // ride carrying the same timestamp would mark the row as already present
        // and skip it).
        QList<RideItem*> items;
        QStringList paths;
        if (site == AfterCompletedWrite) {
            for (int i = 0; i < 2; i++) {
                const QString name = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) +
                                     QStringLiteral("_1%1_00_00.gcblock").arg(i + 5);
                QFile f(activities.absolutePath() + "/" + name);
                f.open(QIODevice::WriteOnly);
                f.write("gcblock");
                f.close();
                paths << f.fileName();

                RideItem* item = new RideItem(nullptr, context);
                item->fileName = name;
                item->path = activities.absolutePath();
                item->dateTime = QDateTime(QDate::currentDate(), QTime(15 + i, 0, 0));
                item->planned = false;
                items << item;
                rideCache->rides().push_back(item);
            }
        }

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = (site == AfterCompletedWrite) ? QStringList() : twoRemoteActivities();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102): it stages
                // UNCOMPRESSED bytes, and uncompressRide's first guard (:320)
                // rejects outright on the CloudService default.
                store->downloadCompression = CloudService::none;
                store->failRead = (site == AfterFailedRead);
                store->completeWrite = (site == AfterCompletedWrite);

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                QTreeWidget* list = nullptr;
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(site == AfterCompletedWrite ? 1 : 0);
                if (site == AfterCompletedWrite) {
                    dialog->selectAllUpChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("File"));
                } else {
                    // The DOWNLOAD tab. No slot in this tree had ever driven it
                    // before TEST-102 (there is no other caller of
                    // selectAllChanged), so every premise below is load-bearing.
                    //
                    // Its column-1 header is "Workout Name" (CloudService.cpp:1100),
                    // NOT "File" - the download list is the one built from the
                    // SERVICE's listing rather than from local files, and only the
                    // upload list calls column 1 "File" (:1127). Asking for "File"
                    // here silently returns the (empty) upload list and every
                    // count reads zero.
                    dialog->selectAllChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                }

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Download"));
                if (watched == nullptr)
                    watched = pushButtonWithText(dialog, QStringLiteral("Upload"));

                if (deliverTheAbort) {
                    store->afterCompletionAction = [&, dialog, watched]() {
                        out.abortDelivered = true;
                        out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                        dialog->downloadClicked();
                        out.abortTookTheAbortBranch =
                            (watched != nullptr && watched->text() == QStringLiteral("Download"));
                        if (restartImmediately) {
                            out.restartDelivered = true;
                            if (site == AfterCompletedWrite)
                                store->completeWrite = false;
                            else
                                store->completeRead = false;
                            dialog->downloadClicked();
                            out.restartTookTheStartBranch =
                                (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                        }
                    };
                }

                dialog->downloadClicked();

                out.readFileCalls = obs::readFileCalls;
                out.rideOpens = rideopen::opens;
                out.writeFileCalls = obs::writeFileCalls;
                out.slotRan = (obs::readFileCalls > 0 || obs::writeFileCalls > 0);
                if (list != nullptr && list->invisibleRootItem()->childCount() > 1)
                    out.row1Status = list->invisibleRootItem()->child(1)->text(site == AfterCompletedWrite ? 7 : 5);

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // The shared verdict for TEST-102's three sites. NOT a slot.
    void assertAbortBehindCompletionStops(AbortAfterSite site, const char* what)
    {
        const QString where = QString::fromLatin1(what) + QStringLiteral(": ");
        const bool isWrite = (site == AfterCompletedWrite);

        // ---- THE CONTROL FIRST. With no abort the batch MUST start the second
        //      row, or the run below proves nothing at all (LSN-047, LSN-050).
        const AbortAfterOutcome base = runAbortBehindCompletion(site, false);
        QVERIFY2(base.timedOut == false, qPrintable(where + QStringLiteral("control: never came back")));
        QCOMPARE(base.listCount, 2);
        QCOMPARE(base.checkedRows, 2);
        if (isWrite)
            QVERIFY2(base.rideOpens == 2,
                     qPrintable(where + QStringLiteral("control: only %1 row(s) were opened - this apparatus "
                                                       "cannot see the second row being started")
                                            .arg(base.rideOpens)));
        else
            QVERIFY2(base.readFileCalls == 2,
                     qPrintable(where + QStringLiteral("control: only %1 read(s) were issued - this apparatus "
                                                       "cannot see the second row being started")
                                            .arg(base.readFileCalls)));

        // ---- THE RUN.
        const AbortAfterOutcome out = runAbortBehindCompletion(site, true);
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the batch never came back")));
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QVERIFY2(out.slotRan, qPrintable(where + QStringLiteral("no transfer was ever started")));
        QVERIFY2(out.abortDelivered,
                 qPrintable(where + QStringLiteral("the abort queued behind the completion never ran")));
        QVERIFY2(out.sawAbortLabel,
                 qPrintable(where + QStringLiteral("the download button was not labelled \"Abort\" while the batch "
                                                   "ran, so downloadClicked() could not have been the abort "
                                                   "control")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch "
                                                   "(CloudService.cpp:1920-1928), so `aborted` was never set")));

        if (isWrite) {
            // THE KILLING ASSERTION FOR THIS SITE. The WRITE is already stopped
            // by S-R027-01 (:2497) even without the completedWrite guard, so
            // writeFileCalls cannot be what proves this one. What the guard
            // uniquely prevents is row[1] being OPENED at all - a nested event
            // loop and a full file parse - and then relabelled.
            QVERIFY2(out.rideOpens == 1,
                     qPrintable(where + QStringLiteral("row[1]'s ride file was OPENED after the user aborted (%1 "
                                                       "open(s), expected 1) - the completion slot handed control "
                                                       "to the next row anyway")
                                            .arg(out.rideOpens)));
            QCOMPARE(out.writeFileCalls, 1);
        } else {
            // THE KILLING ASSERTION FOR THE TWO READ SITES, and the criterion's
            // own: no further store->readFile after the abort. For GarminConnect
            // each one is a worker thread and an interpreter session.
            QVERIFY2(out.readFileCalls == 1,
                     qPrintable(where + QStringLiteral("a further store->readFile was issued after the user "
                                                       "aborted (%1 read(s) in total, expected 1)")
                                            .arg(out.readFileCalls)));
        }

        // ...and row[1] was never even labelled as started.
        QVERIFY2(out.row1Status != QStringLiteral("Downloading") && out.row1Status != QStringLiteral("Uploading"),
                 qPrintable(where +
                            QStringLiteral("row[1] was labelled \"%1\" after the user aborted").arg(out.row1Status)));
    }

    // The shared verdict for TEST-100's two loops. NOT a slot.
    void assertStaleFrameStandsDown(BatchTab tab, const char* loop)
    {
        const QString where = QString::fromLatin1(loop) + QStringLiteral(": ");

        BatchSpec spec;
        spec.tab = tab;
        spec.failRows = 1;
        spec.trailingParseableRow = true;
        spec.abort = AbortThenRestart;
        spec.completeWrite = false; // the restarted batch PARKS, rows stay checked
        const BatchOutcome out = runBatch(spec);

        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the batch never came back")));

        // ---- THE PREMISES. Both clicks delivered, and each took the branch this
        //      test needs it to take.
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QCOMPARE(out.row0File, failActivity(0));
        QCOMPARE(out.lastRowFile, parseableActivity());
        QVERIFY2(out.abortDelivered, qPrintable(where + QStringLiteral("the queued abort never ran")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("click one did not take downloadClicked's abort branch")));
        QVERIFY2(out.restartDelivered, qPrintable(where + QStringLiteral("the queued restart never ran")));
        QVERIFY2(out.restartTookTheStartBranch,
                 qPrintable(where + QStringLiteral("click two did not take downloadClicked's START branch "
                                                   "(:1919-1926) - the button did not go back to \"Abort\", so no "
                                                   "second batch was started and this run proves nothing")));

        // ---- THE POINT. ONE parseable row exists, and exactly ONE batch is
        //      entitled to transfer it. Two transfers means the stale frame kept
        //      driving alongside the restarted one.
        QVERIFY2(out.writeFileCalls == 1,
                 qPrintable(where + QStringLiteral("a STALE batch frame kept transferring alongside the restarted "
                                                   "one - store->writeFile was called %1 time(s) for a single "
                                                   "parseable row")
                                        .arg(out.writeFileCalls)));
        QVERIFY2(out.rideOpens == 1,
                 qPrintable(where + QStringLiteral("the parseable row was opened %1 time(s) - the stale frame "
                                                   "re-opened it")
                                        .arg(out.rideOpens)));

        // The unparseable row is attempted once per batch, and there really were
        // TWO batches (the original and the restart), so twice - by the live
        // driver, not by a third.
        QVERIFY2(out.failOpens == 2,
                 qPrintable(where + QStringLiteral("the unparseable row was attempted %1 time(s); two batches ran, "
                                                   "so it should be exactly 2")
                                        .arg(out.failOpens)));
    }

    // =====================================================================
    // TEST-105 / TEST-106 (REQ-027 (f), DEC-garmin-035) — THE TRANSFER
    // ITSELF IS GUARDED.
    // =====================================================================
    //
    // THE MEASURED ASYMMETRY. `aborted` reads per function, counted on the tree
    // this slice started from:
    //
    //   uploadNext    2 — one IMMEDIATELY before its irreversible writeFile
    //                     (S-R027-01)
    //   syncNext      2 — the parse-failure branch and the pre-`continue`
    //                     re-check, but NEITHER before its readFile
    //   downloadNext  0 — none at all; it issues a third-party download with no
    //                     abort read anywhere in the function
    //
    // (Counted by symbol, not by line: this block cited ":2507/:2088/:2151/
    // :2372/:2432/:2626" until 2026-08-15 and not one of them still pointed at
    // an abort read — see ORCH-020. Production line numbers do not survive
    // edits to production, and this comment cannot be re-verified by anyone
    // reading it in isolation. Name the symbol.)
    //
    // The only guard that protected downloadNext's readFile sat in a DIFFERENT
    // function — the completion slots in completedRead / failedRead /
    // completedWrite — across a return and
    // a call. That is TEST-102's subject and it remains true and useful; what it
    // is not is a statement of the invariant AT the transfer. These two slots
    // make the invariant local: whatever route control took to get here, if
    // `aborted` is set when the loop reaches its transfer statement, the transfer
    // is not issued.
    //
    // WHAT THESE SLOTS DELIBERATELY DO NOT CLAIM, stated here rather than left to
    // be inferred (this ledger has already spent a wave on a test comment that
    // claimed more than its mechanism delivered):
    //
    //   · They do NOT stop an abort pressed AFTER the guard has run. Nothing at
    //     this layer can: between the guard and store->readFile sit the buffer
    //     allocation, a QPointer construction and a BlockingCall construction,
    //     none of which pumps events — so the window is empty, but "empty" is a
    //     property of today's statements, not a mechanism.
    //   · They do NOT close the processEvents() delivery gap. The abort still has
    //     to have been DELIVERED to be visible, and which processEvents() call
    //     delivers a given posted event is not something ordering guarantees (see
    //     the insideframe block comment). These runs therefore do not rely on a
    //     posted abort being noticed: they set the state SYNCHRONOUSLY.
    //
    // HOW THE ABORT IS DELIVERED, and why it is not a queued click.
    // `rideListSync->setCurrentItem(curr)` in syncNext's Download branch and
    // `rideListDown->setCurrentItem(curr)` in downloadNext are production's own
    // statements, they sit between the row's "Downloading" label and the
    // transfer, and QTreeWidget::currentItemChanged is emitted from them
    // SYNCHRONOUSLY. A one-shot direct connection to it therefore runs
    // downloadClicked() — the real abort control, taking its real abort branch —
    // at a point in the real frame that is strictly after the loop has committed
    // to this row and strictly before it issues the transfer. No event is posted,
    // nothing waits to be noticed, and the result does not vary by QPA backend.
    //
    // That seam is a FIXTURE DEVICE and is labelled as one: a real user's click
    // cannot be delivered there, because no event loop runs at that point. What
    // it establishes is exactly the criterion's premise — "an abort that is
    // ALREADY delivered when the loop reaches its transfer statement" — and
    // nothing about how such an abort comes to be set.
    //
    // WHAT REACHES THESE GUARDS IN PRODUCTION TODAY: NOTHING. Corrected here
    // after A3-R027c-F1/F2, because the sentence that used to sit at this spot
    // ("the realistic routes are TEST-093/094/097/099/102's subject, not this
    // one's") credited five tests with covering a route none of them takes —
    // each is intercepted earlier by an OLDER guard (TEST-093 and TEST-099 by
    // the two S-R027-01 reads in uploadNext and syncNext; TEST-094 by
    // completedRead's abort re-read; TEST-097 by syncNext's parse-failure
    // branch; TEST-102 by the completion slots) and control never
    // returns to syncNext/downloadNext with `aborted` still set. A reader was
    // being told the realistic case was covered somewhere. It is covered
    // nowhere, because it does not currently exist.
    //
    // WHY THE GUARDS ARE STILL WORTH HAVING, and why this is NOT the same as the
    // "UNTESTED-BY-DESIGN" `self.isNull()` dead code two lines below each guard.
    // Those are unreachable for a LOCAL reason — nothing in their window can
    // destroy the dialog — which any reader confirms in fifteen lines. These are
    // unreachable only because all four callers happen to re-establish
    // `aborted == false` in the same statement block before calling
    // (completedRead, failedRead and completedWrite each read it immediately
    // before dispatching; downloadClicked assigns it false when it starts a
    // batch), and because syncNext's one suspension-crossing
    // `continue` re-checks both `aborted` and the batch generation. That is a DISTRIBUTED
    // invariant: no reader of syncNext can verify it, and a fifth caller added
    // without the re-check breaks it silently. The guards convert it into a
    // local one. So these two slots are REGRESSION LOCKS on a local invariant,
    // not proof of a live fix — and the guards must not be deleted as dead code
    // on the strength of a mutation run, which measures coverage and says
    // nothing about reachability. See LSN-063.
    //
    // WHICH ASSERTION KILLS THE MUTANT, and which one does NOT.
    // The killing assertion is the criterion's own: readFileCalls == 0. The row
    // label is CORROBORATING ONLY and must not be mistaken for the verdict —
    // without these guards the read is issued, the store queues its completion,
    // and completedRead's ENTRY check labels that same row "Aborted" all
    // by itself. A test that asserted only the label would go green against a
    // tree with no guard at either site. Post-fix the label is set synchronously
    // by the guard before it returns, so reading it the instant downloadClicked()
    // hands back is deterministic; pre-fix it depends on whether the completion
    // was delivered yet, which is why it is ordered after the read count and
    // never asserted on the control.
    enum BeforeTransferLoop {
        InSyncNext,    // Sync tab (index 2)     -> syncNext's Download branch -> :2058
        InDownloadNext // Download tab (index 0) -> downloadNext               -> :2317
    };

    struct BeforeTransferOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the situation it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        QString rowAction; // sync list column 6 — must be "Download"
        bool seamFired = false;
        bool abortDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;

        // -- the verdict
        int readFileCalls = 0;
        QString rowStatus;
    };

    // ONE remote activity and no local ride, so: the download list has exactly
    // one row and its "Exists" box is clear (the "File exists" skip at :2204
    // cannot swallow it), and the sync list has exactly one row whose Action is
    // "Download" (:1708) with no Upload rows to reach first.
    //
    // .gcfail rather than .gcblock, for the same reason TEST-102 gives: if the
    // read IS issued (the pre-fix and mutant runs), the completion that follows
    // must not run a nested event loop of its own inside uncompressRide, or the
    // run's own bookkeeping stops being synchronous.
    static QString oneRemoteActivity()
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) + QStringLiteral("_12_00_00.gcfail");
    }

    // One run. `deliverTheAbort` false is the CONTROL: identical in every other
    // respect — the same seam armed, the same row, the same store — so a green
    // control proves this apparatus can see the transfer being ISSUED, and that
    // arming the seam is not itself what stops it (LSN-047, LSN-050).
    BeforeTransferOutcome runAbortBeforeTransfer(BeforeTransferLoop loop, bool deliverTheAbort)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        BeforeTransferOutcome out;
        QEventLoop appLoop;

        QDir().mkpath(context->athlete->home->activities().absolutePath());
        QDir().mkpath(context->athlete->home->temp().absolutePath());

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList() << oneRemoteActivity();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102).
                store->downloadCompression = CloudService::none;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                const int statusColumn = (loop == InSyncNext) ? 7 : 5; // :1172 / :1104

                QTreeWidget* list = nullptr;
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(loop == InSyncNext ? 2 : 0);
                if (loop == InSyncNext) {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Source"));
                } else {
                    // The download list's column-1 header is "Workout Name"
                    // (CloudService.cpp:1100), NOT "File" — see TEST-102.
                    dialog->selectAllChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.listCount > 0)
                        out.rowAction = root->child(0)->text(6);
                }

                // The one download/abort button, watched by IDENTITY from here
                // on: its text is what says which branch downloadClicked took.
                // "Download" on the download tab, "Synchronize" on the sync tab
                // (tabChanged, :1828).
                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Download"));
                if (watched == nullptr)
                    watched = pushButtonWithText(dialog, QStringLiteral("Synchronize"));

                // THE SEAM. Cleared first so that production's setCurrentItem is
                // guaranteed to be a CHANGE: if the view had already made this row
                // current (showing a tree can), Qt would emit nothing and the run
                // would prove nothing — `seamFired` would then be false and the
                // premise below fails loudly rather than passing quietly.
                QTreeWidgetItem* transferRow = nullptr;
                if (list != nullptr) {
                    list->setCurrentItem(nullptr);
                    auto link = std::make_shared<QMetaObject::Connection>();
                    *link = QObject::connect(
                        list, &QTreeWidget::currentItemChanged, list,
                        [&, link, dialog, watched](QTreeWidgetItem* current, QTreeWidgetItem*) {
                            if (current == nullptr)
                                return;
                            QObject::disconnect(*link); // fires once
                            out.seamFired = true;
                            transferRow = current;
                            if (!deliverTheAbort)
                                return;
                            out.abortDelivered = true;
                            out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                            dialog->downloadClicked();
                            // The abort branch relabels that same button
                            // "Download" (:1914), so this is the observable proof
                            // that :1920-1928 ran and `aborted` is now true.
                            out.abortTookTheAbortBranch =
                                (watched != nullptr && watched->text() == QStringLiteral("Download"));
                        },
                        Qt::DirectConnection);
                }

                dialog->downloadClicked(); // -> syncNext() / downloadNext()

                // The verdict, read the instant the batch hands control back —
                // everything the guard does is synchronous, so nothing here waits
                // on an event being delivered.
                out.readFileCalls = obs::readFileCalls;
                if (transferRow != nullptr)
                    out.rowStatus = transferRow->text(statusColumn);

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
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

    // The shared verdict for TEST-105 / TEST-106. NOT a slot.
    void assertAbortBeforeTransferStopsIt(BeforeTransferLoop loop, const char* what)
    {
        const QString where = QString::fromLatin1(what) + QStringLiteral(": ");

        // ---- THE CONTROL FIRST. With the seam armed but no abort delivered the
        //      row MUST be transferred, or the run below proves nothing at all.
        const BeforeTransferOutcome base = runAbortBeforeTransfer(loop, false);
        QVERIFY2(base.timedOut == false, qPrintable(where + QStringLiteral("control: never came back")));
        QCOMPARE(base.listCount, 1);
        QCOMPARE(base.checkedRows, 1);
        QVERIFY2(base.seamFired,
                 qPrintable(where + QStringLiteral("control: production's setCurrentItem never reached the "
                                                   "fixture, so the abort in the run below could not have been "
                                                   "delivered inside the frame under test")));
        QVERIFY2(base.readFileCalls == 1,
                 qPrintable(where + QStringLiteral("control: store->readFile was called %1 time(s), not once - "
                                                   "this apparatus cannot see a transfer being issued at all")
                                        .arg(base.readFileCalls)));

        // ---- THE RUN.
        const BeforeTransferOutcome out = runAbortBeforeTransfer(loop, true);
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the batch never came back")));

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, 1);
        QCOMPARE(out.checkedRows, 1);
        if (loop == InSyncNext) {
            // ...and it really is syncNext's DOWNLOAD branch (:1996), not the
            // upload branch that has carried S-R027-01 since :2088.
            QCOMPARE(out.rowAction, QStringLiteral("Download"));
        }
        QVERIFY2(out.seamFired,
                 qPrintable(where + QStringLiteral("production's setCurrentItem never reached the fixture")));
        QVERIFY2(out.abortDelivered, qPrintable(where + QStringLiteral("the abort was never delivered")));
        QVERIFY2(out.sawAbortLabel,
                 qPrintable(where + QStringLiteral("the download button was not labelled \"Abort\" while the batch "
                                                   "ran, so downloadClicked() could not have been the abort "
                                                   "control")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch "
                                                   "(CloudService.cpp:1920-1928), so `aborted` was never set")));

        // ---- THE POINT, and THE KILLING ASSERTION. The criterion's own number:
        //      with abort set, store->readFile is called 0 further times from
        //      this site. For GarminConnect each one is a worker thread and an
        //      interpreter session started for an activity the user has already
        //      stopped.
        QVERIFY2(out.readFileCalls == 0,
                 qPrintable(where + QStringLiteral("the transfer was ISSUED for a row the user had already "
                                                   "aborted: store->readFile was called %1 time(s), expected 0")
                                        .arg(out.readFileCalls)));

        // ...and the row SAYS so. CORROBORATING ONLY - see the block comment:
        // completedRead's entry check labels this same row "Aborted" when the
        // read is allowed to happen, so this assertion cannot kill the mutant and
        // is not what this test rests on.
        QCOMPARE(out.rowStatus, QStringLiteral("Aborted"));
    }

  private slots:
    // =====================================================================
    // TEST-107 (REQ-028) — A MEASUREMENT PROBE, NOT A GUARD.
    //
    // WHAT THIS SLOT IS. It asserts what unmodified production DOES today, so
    // that the design that follows is built on measured numbers rather than on
    // an argument. It is deliberately NOT the shape of the rest of this file:
    // every other slot here states a rule and would go red if production broke
    // it, and this one states an OBSERVATION and would go red if production were
    // FIXED. Whoever fixes A3-R027-F8 must change the two blocks marked
    // "MEASURED, NOT DESIRED" below and should expect to - that is the point of
    // recording them. Do not read a green run here as production being correct.
    //
    // THE TWO QUESTIONS.
    //
    // P1  What does QTreeWidgetItem::child(int) return out of range? Qt does not
    //     document it, and it decides what A3-R027-F8 costs. After a Refresh
    //     empties a list (CloudService.cpp:1527-1545 - takeChildren() then
    //     delete), the three completion slots still address rows as
    //     `child(listindex-1)` and dereference the result WITHOUT a null check
    //     (completedRead :2388/:2437/:2444, failedRead :2525/:2532,
    //     completedWrite :2717/:2724). Null means those sites are a crash; a
    //     non-null result would mean a silently wrong row. Different defects,
    //     different guards. Measured with no dialog at all - it is a fact about
    //     Qt, and mixing it into a dialog run would only make it harder to read.
    //
    // P2  Is the STALE-COMPLETION route reachable, and if so what does it hit?
    //     Start a batch, abort it mid-flight, restart it - downloadClicked bumps
    //     batchGeneration (:1931) and resets listindex (:1941) - and then let the
    //     OLD batch's completion arrive. `listindex` now belongs to the NEW
    //     batch, so the question is which row the old completion lands on.
    //
    // HOW THE LATE COMPLETION IS MADE, and it needs no new fixture. BlockingStore
    // already has `completeRead` (:707): with it false, readFile returns without
    // notifying anyone, so the batch parks exactly as a real one waits on the
    // network. The run then delivers each completion BY HAND through
    // CloudService::notifyReadComplete (CloudService.h:145), which is the same
    // `emit readComplete(...)` the store would have made, on the same direct
    // connection - so completedRead runs in the same frame shape it always does.
    // What changes is only WHEN, which is the whole subject.
    //
    // THE CONTROL IS LOAD-BEARING (LSN-047, LSN-050). The identical sequence
    // WITHOUT the abort+restart delivers the same row's completion at the same
    // point and must label THAT row. Without it a "the wrong row was labelled"
    // reading could just as well be an apparatus that cannot tell rows apart.
    //
    // THE ROWS ARE .gcfail, for TEST-102's reason: FailingRideFileReader runs no
    // nested event loop, so completedRead's uncompressRide cannot itself pump
    // events and re-order anything this run depends on. It also means `ride` is
    // always NULL, so `successful` (:2448) is never reached - this run measures
    // `downloadcounter` (:2442), and says nothing about `successful`.
    //
    // DISPOSITION AFTER REQ-028 (c) / DEC-garmin-036 (2026-08-18) - REWRITTEN,
    // AND ITS P2 HALF HANDED TO TEST-113.
    //
    // This slot was written expecting to go RED when its subject was fixed, and
    // its own header said so ("whoever fixes A3-R027-F8 must change the two
    // blocks marked MEASURED, NOT DESIRED"). DEC-036's in-flight ticket is that
    // fix, so the change is made here rather than argued about:
    //
    //   P1 STAYS, UNCHANGED. It is a fact about Qt - what
    //   QTreeWidgetItem::child(int) returns out of range - not a fact about this
    //   dialog, and no fix to this dialog can alter it. It is still what decides
    //   what a positional dereference COSTS, and it is still the reason DEC-034's
    //   guards had to be the kind of guard they are. Note that after DEC-036 the
    //   three completion slots no longer index positionally at all: they label
    //   through the ticket's stored row pointer. So P1 now records why that
    //   change was worth making rather than what a live defect costs.
    //
    //   P2 IS GONE FROM HERE. Its geometry (abort, restart, late completion) and
    //   its apparatus (runLateCompletion, below - still shared) now belong to
    //   TEST-113, which asserts the DESIRED numbers instead of the measured ones:
    //   3 reads for 3 checked rows, no row relabelled, the bar unmoved. Keeping a
    //   second copy of those numbers here in their "MEASURED, NOT DESIRED" form
    //   would mean this file asserted BOTH that production still mislabels and
    //   that it no longer does, and the first of those is now false. A green
    //   TEST-107 after DEC-036 must not be readable as "the defect is still
    //   there", which is exactly what leaving the block in would have made it.
    //
    // The line numbers in the prose above were re-derived 2026-08-17 and are
    // stale AGAIN wherever DEC-036 moved code; the seven `child(listindex-1)`
    // sites they name no longer exist at all. Prefer the SYMBOL over the number
    // (ORCH-020, LSN-034).
    void probeWhichRowALateCompletionLabelsAfterARestart()
    {
        // ---- P1. Out-of-range indexing on a QTreeWidget's invisible root,
        //      including a list emptied the way refreshClicked empties one.
        bool populatedPastEndIsNull = false;
        bool populatedNegativeIsNull = false;
        bool emptiedChild0IsNull = false;
        bool emptiedChildFarIsNull = false;
        {
            QTreeWidget tree;
            tree.setColumnCount(2);
            for (int i = 0; i < 3; i++)
                new QTreeWidgetItem(tree.invisibleRootItem(), QStringList() << QString::number(i));
            QTreeWidgetItem* root = tree.invisibleRootItem();
            QCOMPARE(root->childCount(), 3);

            populatedPastEndIsNull = (root->child(3) == nullptr);
            populatedNegativeIsNull = (root->child(-1) == nullptr);

            // refreshClicked's own idiom (CloudService.cpp:1527-1545).
            foreach (QTreeWidgetItem* curr, root->takeChildren())
                delete curr;
            QCOMPARE(root->childCount(), 0);

            emptiedChild0IsNull = (root->child(0) == nullptr); // the F8 index
            emptiedChildFarIsNull = (root->child(7) == nullptr);
        }

        // ---- P1, MEASURED, NOT DESIRED. Qt bounds-checks and returns null, so
        //      `curr->setText(...)` at the six sites listed above is a NULL
        //      DEREFERENCE whenever listindex-1 is out of range - a crash, not a
        //      mislabel. Recorded here so the guard that gets written is the
        //      right kind of guard.
        QVERIFY2(populatedPastEndIsNull, "child(childCount()) on a POPULATED tree was not null");
        QVERIFY2(populatedNegativeIsNull, "child(-1) on a POPULATED tree was not null");
        QVERIFY2(emptiedChild0IsNull, "child(0) on a tree emptied by takeChildren()+delete was not null");
        QVERIFY2(emptiedChildFarIsNull, "child(7) on an emptied tree was not null");

        // ---- P2 LIVES IN TEST-113 NOW. The abort+restart geometry and the
        //      apparatus that drives it (runLateCompletion, below) moved there
        //      with DEC-garmin-036, which fixed what they used to measure. See
        //      the DISPOSITION paragraph in this slot's header.
    }

    // =====================================================================
    // TEST-113 (REQ-028 (c), DEC-garmin-036) — THE STALE READ RESOLVES ONLY TO
    // ITS OWN STILL-LIVE ROW.
    // =====================================================================
    //
    // THE CRITERION, VERBATIM (REQ-028 (c)):
    //
    //   "(c) NO ROW IS LABELLED THAT THE FRAME DID NOT TRANSFER, AND NO ROW IS
    //   TRANSFERRED TWICE. Across a batch that is aborted and immediately
    //   restarted with the aborted batch's completion arriving LATE, every
    //   non-empty status cell names the outcome of a transfer that actually
    //   happened to that row, and the reader is invoked exactly once per checked
    //   row - an over-count catches a second driver, an under-count catches a
    //   stale one."
    //
    // THE GEOMETRY is TEST-107's, unchanged, and deliberately so: the numbers
    // this slot asserts are the SAME measurement that slot used to record as
    // "MEASURED, NOT DESIRED", read now as a rule. Batch one is parked on row 1
    // when the user aborts and immediately restarts; the restarted batch is
    // parked on row 0; and THEN row 1's completion - batch one's - arrives.
    //
    // WHAT DEC-036 PUTS IN ITS WAY. Each dispatch records the buffer, row, column
    // and admitted generation independently. Restart does not overwrite the old
    // operation. Its late completion therefore resolves to its own still-live
    // row, labels that old-generation transfer Aborted, and returns without
    // advancing the live batch's bar or re-driving it.
    //
    // THE THREE NUMBERS, and what each one catches:
    //
    //   readFileCallsAfter == 3   for 3 checked rows. This is the criterion's own
    //                             count. It was 4 before DEC-036 (row 0 and row 1
    //                             each transferred twice, row 2 never reached),
    //                             and the fourth read is the SECOND DRIVER the
    //                             criterion says an over-count catches.
    //   rowsRelabelled == {1}     only the old operation's own row changes. Row 0,
    //                             which the restarted batch is transferring, is
    //                             never given row 1's outcome.
    //   barAfter == 0             the restarted batch's progress bar does not
    //                             count a row that batch never issued.
    //
    // THE CONTROL IS PART OF THIS SLOT, not a neighbour's (LSN-050). The same
    // apparatus, the same hand-delivered completion, the same row - with no abort
    // and no restart in between - must still label row 1, still advance the bar
    // and still re-drive onto row 2. Without it, all three numbers above are
    // equally satisfied by a completedRead that does nothing at all.
    void aLateCompletionOfAnAbandonedBatchMustNotLabelTheLiveBatchesRow()
    {
        // The status cell a completed-but-unparseable row ends up with. NOT the
        // literal "Parse failure" the sync/upload loops write: completedRead's
        // no-ride branch puts `errors.join(" ")` in the cell, so the string is
        // FailingRideFileReader's own (:875). Named once here so the comparisons
        // below cannot drift apart from the fixture.
        const QString verdict = QStringLiteral("TEST-096 unparseable activity");

        // ---- THE CONTROL. Same delivery, no restart in between.
        const LateCompletionOutcome base = runLateCompletion(false);
        QVERIFY2(base.timedOut == false, "control: the run never came back");
        QCOMPARE(base.listCount, 3);
        QCOMPARE(base.checkedRows, 3);
        QVERIFY2(base.firstCompletionDelivered, "control: row 0's read was never issued, so nothing could complete");
        QVERIFY2(base.lateCompletionDelivered, "control: row 1's read was never issued");
        QCOMPARE(base.readFileCallsBefore, 2);

        // The batch is parked on row 1 and row 1 is what completes: the verdict
        // lands on ROW 1. This is the apparatus proving it can tell rows apart.
        //
        // Row 2 changes too, and that is not the completion: completedRead's tail
        // calls downloadNext, which dispatches the next row and labels it
        // "Downloading" inside this same delivery. The two are told apart by the
        // WORD, which is why the full lists are compared.
        QCOMPARE(base.statusesBefore, QStringList() << verdict << "Downloading" << "");
        QCOMPARE(base.statusesAfter, QStringList() << verdict << verdict << "Downloading");
        QCOMPARE(base.rowsRelabelled, QList<int>() << 1 << 2);
        QCOMPARE(base.barBefore, 1);
        QCOMPARE(base.barAfter, 2);
        // ...and the loop really did drive on, which is the half of the control
        // that stops "swallow everything" from passing this slot.
        QCOMPARE(base.readFileCallsAfter, 3);

        // ---- THE RUN. Abort, restart, and THEN the old batch's completion.
        const LateCompletionOutcome out = runLateCompletion(true);
        QVERIFY2(out.timedOut == false, "the run never came back");

        // ---- THE PREMISES. Every one of these is what makes the numbers below
        //      mean what they are read as.
        QCOMPARE(out.listCount, 3);
        QCOMPARE(out.checkedRows, 3);
        QVERIFY2(out.firstCompletionDelivered, "row 0's read was never issued");
        QVERIFY2(out.lateCompletionDelivered, "row 1's read was never issued, so there was nothing to deliver late");
        QVERIFY2(out.sawAbortLabel, "the button was not labelled \"Abort\" while the batch ran, so downloadClicked() "
                                    "could not have been the abort control");
        QVERIFY2(out.abortTookTheAbortBranch, "downloadClicked() did not take its abort branch");
        QVERIFY2(out.restartTookTheStartBranch, "the second click did not take downloadClicked's START branch");
        // The old batch was suspended on row 1; the restarted batch is on row 0.
        // If these were the same row the run would prove nothing.
        QCOMPARE(out.oldBatchRowName, out.rowNames.value(1));
        QCOMPARE(out.newBatchRowName, out.rowNames.value(0));
        // The restarted batch really is mid-transfer when the stale completion
        // lands: three reads have been issued (batch one's two, batch two's one)
        // and batch two's is still outstanding.
        QCOMPARE(out.readFileCallsBefore, 3);

        // ---- THE CRITERION.
        //
        // "the reader is invoked exactly once per checked row" - three checked
        // rows, three reads, and NOT the fourth that the stale completion's
        // re-drive used to issue.
        QCOMPARE(out.readFileCallsAfter, 3);
        QCOMPARE(out.readNames, QStringList()
                                    << out.rowNames.value(0) << out.rowNames.value(1) << out.rowNames.value(0));

        // "every non-empty status cell names the outcome of a transfer that
        // actually happened to that row": the late completion labels its own
        // row 1 Aborted, never the restarted batch's row 0.
        QCOMPARE(out.rowsRelabelled, QList<int>() << 1);
        QCOMPARE(out.statusesBefore, QStringList() << "Downloading" << "Downloading" << "");
        QCOMPARE(out.statusesAfter, QStringList() << "Downloading" << "Aborted" << "");

        // ...and the abandoned batch does not move the live batch's bar.
        QCOMPARE(out.barBefore, 0);
        QCOMPARE(out.barAfter, 0);
        QCOMPARE(out.barMax, 3);

        // ---- THE SAME RUN ON THE OTHER READ CHANNEL. failedRead is a second
        //      completion slot for the same readFile call (DEC-023), with its own
        //      copy of the ticket check, so the criterion has to be asserted of it
        //      too rather than inferred from its sibling. Same abandoned transfer,
        //      same three numbers.
        const LateCompletionOutcome fail = runLateCompletion(true, true);
        QVERIFY2(fail.timedOut == false, "failure channel: the run never came back");
        QVERIFY2(fail.lateCompletionDelivered, "failure channel: nothing was delivered late");
        QCOMPARE(fail.readFileCallsBefore, 3);
        QCOMPARE(fail.readFileCallsAfter, 3);
        QCOMPARE(fail.rowsRelabelled, QList<int>() << 1);
        QCOMPARE(fail.statusesAfter, QStringList() << "Downloading" << "service refused" << "");
        QCOMPARE(fail.barBefore, 0);
        QCOMPARE(fail.barAfter, 0);
    }

    // =====================================================================
    // TEST-116 (REQ-028 (c), DEC-garmin-036) — THE SWALLOWED BUFFER IS FREED
    // EXACTLY ONCE.
    // =====================================================================
    //
    // A guard that returns early from a completion slot is a LEAK unless it frees
    // first: the buffer was `new`ed before readFile and this dialog is its only
    // owner (A3-R017-F3, and the contract now written at CloudService.h on
    // notifyReadComplete). DEC-036 adds a fourth early return to completedRead
    // and a third to failedRead, so this asks the allocator directly whether the
    // swallowed buffer went back.
    //
    // "EXACTLY ONCE" is TWO measurements, not one:
    //   - freed at least once: ASan poisons a block on free and holds it in
    //     quarantine, so a poisoned region is a freed one.
    //   - freed at most once: a DOUBLE free aborts the process under ASan, so
    //     reaching this assertion at all is the other half.
    // And the discrimination that stops "poisoned" from being vacuous: the LIVE
    // batch's own in-flight buffer, allocated by the restarted batch and still
    // outstanding, must NOT be poisoned. If the slot freed indiscriminately, or
    // if the two runs shared one address, that assertion fails.
    void theSwallowedBuffersAreReleasedRatherThanLeaked()
    {
        const LateCompletionOutcome out = runLateCompletion(true);
        QVERIFY2(out.timedOut == false, "the run never came back");
        QVERIFY2(out.lateCompletionDelivered, "no stale completion was delivered, so nothing could be swallowed");
        QVERIFY2(out.staleAndLiveBuffersDiffer,
                 "the abandoned batch's buffer and the restarted batch's buffer are the SAME address, so neither "
                 "assertion below can tell them apart");

        QVERIFY2(out.staleBufferFreed, "the swallowed read buffer was not released - the guard leaks it once per "
                                       "abandoned transfer");
        QVERIFY2(out.liveBufferStillHeld, "the LIVE batch's outstanding buffer was released by the stale completion's "
                                          "delivery, which is a free of a buffer still in flight");

        // ...and the same of failedRead's copy of the guard, which owns the buffer
        // on exactly the same terms (DEC-023: one channel or the other per
        // readFile call, and the consumer frees whichever arrives).
        const LateCompletionOutcome fail = runLateCompletion(true, true);
        QVERIFY2(fail.timedOut == false, "failure channel: the run never came back");
        QVERIFY2(fail.lateCompletionDelivered, "failure channel: nothing was delivered late");
        QVERIFY2(fail.staleAndLiveBuffersDiffer, "failure channel: the two buffers are the same address");
        QVERIFY2(fail.staleBufferFreed, "failedRead's swallow did not release the buffer - it leaks it once per "
                                        "abandoned transfer");
        QVERIFY2(fail.liveBufferStillHeld, "failedRead's swallow released the LIVE batch's outstanding buffer");
    }

    // =====================================================================
    // TEST-114 (REQ-028 (c), DEC-garmin-036) — THE POSITIVE CONTROL: A HEALTHY
    // BATCH STILL DRIVES ITSELF TO ITS TAIL.
    // =====================================================================
    //
    // WHY THIS SLOT EXISTS (LSN-050). Every other criterion in this wave is about
    // something NOT happening - no label, no bar, no re-drive - and every one of
    // them is satisfied by a completedRead that swallows everything. This is the
    // slot that refuses that: an ordinary three-row download, no abort, no
    // restart, no Refresh, must issue three reads and reach its completion tail.
    //
    // AND IT IS WHAT MAKES EACH ARM SITE LOAD-BEARING INDIVIDUALLY. The ticket is
    // armed at four dispatch sites. Mutation of a swallow guard proves the guard
    // is load-bearing; nothing proves an ARM is, because a ticket that is never
    // armed simply swallows more. Drop the arming at ONE dispatch site and the
    // live completion that follows it arrives to `armed == false`, is swallowed,
    // and the batch stalls where it stood - which is this slot's assertions, and
    // only this slot's.
    //
    // THE TAIL IS OBSERVED AS ITS LAST SENTENCE, not its first. downloadNext's
    // completion tail sets progressLabel to "Downloads complete" and then, eleven
    // lines later in the same straight-line block, overwrites it with
    // "Downloaded %1 of %2 successfully". Nothing pumps events in between, so the
    // first string is never observable from outside; the second is the observable
    // proof that the same block ran. The rows are .gcfail (TEST-102's reason:
    // FailingRideFileReader runs no nested loop of its own), so `successful` is 0
    // and the sentence reads "Downloaded 0 of 3 successfully" - which still says
    // all three rows were attempted and the tail was reached.
    void aHealthyBatchStillDrivesItselfToTheCompletionTail()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        bool timedOut = false;
        int listCount = 0, checkedRows = 0, readFileCalls = 0;
        int barValue = -1, barMax = -1;
        QString progressText;
        QStringList statuses, readNames;

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeUnparseableRemoteActivities();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                // The DIFFERENCE from TEST-113's apparatus: completions arrive by
                // themselves, exactly as a service delivers them. Nothing in this
                // run is hand-delivered and nothing is abandoned.
                store->completeRead = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(0);
                dialog->selectAllChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    listCount = root->childCount();
                    for (int i = 0; i < listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            checkedRows++;
                    }
                }

                dialog->downloadClicked();

                // The batch drives itself through queued completions; let every
                // one of them land, and stop as soon as the tail has spoken.
                for (int i = 0; i < 200 && progressText.isEmpty(); ++i) {
                    QApplication::processEvents(QEventLoop::AllEvents, 5);
                    progressText = progressLabelText(dialog);
                }

                readFileCalls = obs::readFileCalls;
                readNames = obs::readNames;
                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        statuses << list->invisibleRootItem()->child(i)->text(5);
                if (QProgressBar* bar = dialog->findChild<QProgressBar*>()) {
                    barValue = bar->value();
                    barMax = bar->maximum();
                }

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
                QTimer::singleShot(10000, &appLoop, [&timedOut]() {
                    timedOut = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        QVERIFY2(timedOut == false, "the run never came back");
        QCOMPARE(listCount, 3);
        QCOMPARE(checkedRows, 3);

        // THE CRITERION FOR THIS SLOT. Three checked rows, three reads, one per
        // row and in row order - and the completion tail reached.
        QCOMPARE(readFileCalls, 3);
        QCOMPARE(readNames.count(), 3);
        QCOMPARE(progressText, QStringLiteral("Downloaded 0 of 3 successfully"));

        // Every row was told what happened to IT. The verdict is
        // FailingRideFileReader's own, as in TEST-113.
        const QString verdict = QStringLiteral("TEST-096 unparseable activity");
        QCOMPARE(statuses, QStringList() << verdict << verdict << verdict);

        // ...and the bar counted every one of them exactly once.
        QCOMPARE(barValue, 3);
        QCOMPARE(barMax, 3);

        // ---- AND THE SAME CLAIM ON THE WRITE SIDE, because the download batch
        //      above reaches exactly ONE of the four arm sites (downloadNext's
        //      read). Two local rides, the Upload tab, completions arriving by
        //      themselves: uploadNext must dispatch both rows, label both, and
        //      reach its own tail. Drop the arming at uploadNext and the first
        //      completion is swallowed, the loop is never re-driven, and the
        //      second row is never uploaded - which is these three numbers.
        const HealthyUploadOutcome up = runHealthyUpload();
        QVERIFY2(up.timedOut == false, "healthy upload: the run never came back");
        QCOMPARE(up.upListCount, 2);
        QCOMPARE(up.checkedRows, 2);
        QCOMPARE(up.writeFileCalls, 2);
        QVERIFY2(up.writeNames.count() == 2 && up.writeNames.value(0) != up.writeNames.value(1),
                 qPrintable(QStringLiteral("healthy upload: the two writes were not one per row: [%1]")
                                .arg(up.writeNames.join(QStringLiteral("|")))));
        QCOMPARE(up.statuses, QStringList() << "Completed." << "Completed.");
        QCOMPARE(up.progressText, QStringLiteral("Uploaded 2 of 2 successfully"));
    }

    // =====================================================================
    // TEST-115 (REQ-028 (c), DEC-garmin-036, RE-ARGUED FOR DEC-garmin-037) — A
    // STALE WRITE NEVER REACHES THE LIVE BATCH, INCLUDING WHEN THE RESTART
    // RE-DISPATCHES THE SAME ROW.
    // =====================================================================
    //
    // THE WRITE HALF of criterion (c). REQ-027 widened this defect from a read to
    // a WRITE, and completedWrite has the same two harms as its sibling - it
    // labels a row and it re-drives the loop - so the ticket has to cover it too.
    // What it CANNOT use is completedRead's discriminator: writeComplete carries
    // no buffer, only an id, so the ticket compares the exact `remotename` string
    // uploadNext/syncNext passed to writeFile. Services that pass an EMPTY id are
    // not rejected by that compare (nothing to compare), which is why the second
    // half of this slot matters.
    //
    // RE-ARGUED 2026-08-19 FOR DEC-garmin-037, AND BOTH HALVES CHANGED - said
    // here rather than left for a reader to notice that the numbers moved. This
    // slot used to claim "the stale write is SWALLOWED", and its exculpation of
    // the same-row half rested on the one-shot `armed` bit being the SOLE
    // separator on the write channel. Neither is true any more, because DEC-037
    // put a second separator in front of it: downloadClicked's START branch
    // RETIRES an abandoned write's ticket instead of merely voiding it, and
    // completedWrite answers a retired ticket from the row it names.
    //
    //   WHAT DID NOT CHANGE, and it is the whole of what this slot exists to
    //   protect: a stale write still never touches the LIVE batch. It does not
    //   label the live row, does not advance the bar, does not count a success and
    //   does not re-drive the loop. Every assertion below that says so is
    //   unchanged.
    //
    //   WHAT CHANGED is where the stale completion GOES. It used to go nowhere;
    //   now it goes to the row it was actually issued for, and that row's cell
    //   then names the outcome of a transfer that really happened to it - which is
    //   what criterion (c) asks of every non-empty cell, and is strictly more than
    //   silence delivered. So "swallowed" was never the criterion; it was one way
    //   of satisfying it, and DEC-037 replaced it with a better one on the routes
    //   where a name compare alone cannot decide.
    //
    // TWO RUNS, and they now exercise DIFFERENT separators, which is why they stay
    // in one slot:
    //
    //   DIFFERENT ROW - THE NAME COMPARE. Batch one parks on row 1; abort;
    //   restart; batch two parks on row 0; and THEN row 1's write completes. Its
    //   name is not row 0's, so the LIVE compare rejects it exactly as it did
    //   before DEC-037. What now catches it first is its own retired ticket, so it
    //   labels ROW 1 - its own row - and returns. Row 0, the live batch's row,
    //   keeps "Uploading", the bar stays at 0 and no third write is issued: the
    //   before-DEC-036 harm (the completion labelled row 0 and re-drove the loop)
    //   is still what this run is watching for.
    //
    //   SAME ROW - AND THIS IS THE HALF WHOSE ARGUMENT IS NEW. One row only. Batch
    //   one parks on it; abort; restart; batch two dispatches THE IDENTICAL ROW
    //   with the IDENTICAL name; and then BOTH completions arrive. Before DEC-037
    //   the two were separated by ONE mechanism - the one-shot `armed` bit, which
    //   consumed the first and swallowed the second. They are now separated by
    //   TWO, in order: the FIRST arrival is taken by the RETIRED ticket (it labels
    //   the row and returns, driving nothing), and the SECOND by the live ticket
    //   (it labels, counts once and drives once). `armed` is still one-shot and
    //   still load-bearing - it is what stops a THIRD completion - but it is no
    //   longer the sole separator, and the observable difference is exactly that
    //   the FIRST arrival no longer re-drives the loop: progressTextAfter is empty
    //   where it used to carry the completion tail's sentence, and that sentence
    //   now appears only after the SECOND. The killer assertion is unchanged and
    //   is still the tail's own sentence: "Uploaded 1 of 1 successfully" against
    //   "Uploaded 2 of 1 successfully", because a QProgressBar clamps setValue()
    //   to its maximum and would hide the double count.
    void aLateWriteCompletionOfAnAbandonedBatchMustNotLabelTheLiveBatchesRow()
    {
        // ---- RUN ONE: the restarted batch is on a DIFFERENT row.
        const StaleWriteOutcome out = runStaleWrite(false);
        QVERIFY2(out.timedOut == false, "different-row: the run never came back");
        QCOMPARE(out.upListCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QVERIFY2(out.sawAbortLabel, "different-row: the button was never labelled \"Abort\", so downloadClicked() "
                                    "could not have been the abort control");
        QVERIFY2(out.abortTookTheAbortBranch, "different-row: downloadClicked() did not take its abort branch");
        QVERIFY2(out.restartTookTheStartBranch, "different-row: the second click did not take the START branch");
        // The premise the whole run rests on: the abandoned transfer and the live
        // one are for DIFFERENT rows.
        QVERIFY2(out.staleName != out.liveName,
                 "different-row: the abandoned write and the live one carry the same name");
        QCOMPARE(out.staleName, out.writeNames.value(1));
        QCOMPARE(out.liveName, out.writeNames.value(2));
        QCOMPARE(out.writeCallsBefore, 3);

        // THE CRITERION. The late completion never reaches the LIVE batch: row 0
        // is the row batch two is uploading and it keeps "Uploading", the bar does
        // not move and no third write is issued.
        QCOMPARE(out.statusesBefore, QStringList() << "Uploading" << "Uploading");
        QCOMPARE(out.statusesAfter.value(0), QStringLiteral("Uploading"));
        QCOMPARE(out.writeCallsAfter, 3);
        QCOMPARE(out.barBefore, 0);
        QCOMPARE(out.barAfter, 0);

        // ...AND IT GOES TO ITS OWN ROW (DEC-037). Row 1 is the row batch one
        // really was writing when the user aborted, so its cell now names the
        // outcome of a transfer that really happened to it. Before DEC-037 this
        // read `rowsRelabelled == {}` - the completion went nowhere - and the
        // change is deliberate: see the RE-ARGUED paragraph above.
        QCOMPARE(out.rowsRelabelled, QList<int>() << 1);
        QCOMPARE(out.statusesAfter.value(1), QStringLiteral("Completed."));

        // ---- RUN TWO: the restart re-dispatches the SAME row, so the name
        //      compare cannot separate the two transfers at all - the retired
        //      ticket takes the first and the one-shot `armed` bit the second.
        const StaleWriteOutcome same = runStaleWrite(true);
        QVERIFY2(same.timedOut == false, "same-row: the run never came back");
        QCOMPARE(same.upListCount, 1);
        QCOMPARE(same.checkedRows, 1);
        QVERIFY2(same.abortTookTheAbortBranch, "same-row: downloadClicked() did not take its abort branch");
        QVERIFY2(same.restartTookTheStartBranch, "same-row: the second click did not take the START branch");
        // The premise: the two outstanding transfers are for the same row and
        // carry the same name, so a name compare cannot tell them apart.
        QCOMPARE(same.staleName, same.liveName);
        QCOMPARE(same.writeCallsBefore, 2);

        // The FIRST of the two completions is taken by the RETIRED ticket - it is
        // indistinguishable from the live one, and the cell it writes is true of
        // that row either way. It DRIVES NOTHING, and the empty label is the
        // observable proof of that: progressLabelText reports only a completion
        // TAIL's own sentence, and before DEC-037 this read "Uploaded 1 of 1
        // successfully" because the first arrival was consumed by the LIVE ticket
        // and re-drove the loop into its tail. That is the one behavioural change
        // in this half, and it is the right one: the batch that issued the first
        // completion no longer exists and has nothing to drive.
        QCOMPARE(same.rowsRelabelled, QList<int>() << 0);
        QCOMPARE(same.statusesAfter, QStringList() << "Completed.");
        QCOMPARE(same.progressTextAfter, QString());

        // ...AND THE SECOND IS THE LIVE BATCH'S OWN, counted exactly once. The
        // `armed` bit is still one-shot and still load-bearing here - it is what
        // would stop a THIRD completion - and the tail's sentence is still the
        // killer: "Uploaded 2 of 1 successfully" is what a double count reads,
        // because a QProgressBar clamps setValue() to its maximum and would hide
        // it.
        QCOMPARE(same.statusesFinal, same.statusesAfter);
        QCOMPARE(same.writeCallsFinal, 2);
        QCOMPARE(same.progressTextFinal, QStringLiteral("Uploaded 1 of 1 successfully"));
    }

  private:
    // TEST-114 (REQ-028 (c)) — what one HEALTHY upload batch leaves behind.
    // NOT a slot. The write-side twin of the download run in that slot's body,
    // and the run that makes uploadNext's arm site individually load-bearing.
    struct HealthyUploadOutcome
    {
        bool timedOut = false;
        int upListCount = 0;
        int checkedRows = 0;
        int writeFileCalls = 0;
        QStringList writeNames;
        QStringList statuses;
        QString progressText;
    };

    HealthyUploadOutcome runHealthyUpload()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        HealthyUploadOutcome out;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList(); // nothing remote: no "File exists" skip
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->writeSucceeds = true;
                // THE DIFFERENCE from runStaleWrite: completions arrive by
                // themselves, exactly as a service delivers them.
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(1);
                dialog->selectAllUpChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("File"));
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.upListCount = root->childCount();
                    for (int i = 0; i < out.upListCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                }

                dialog->downloadClicked();

                for (int i = 0; i < 200 && out.progressText.isEmpty(); ++i) {
                    QApplication::processEvents(QEventLoop::AllEvents, 5);
                    out.progressText = progressLabelText(dialog);
                }

                out.writeFileCalls = obs::writeFileCalls;
                out.writeNames = obs::writeNames;
                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statuses << list->invisibleRootItem()->child(i)->text(7);

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }

        return out;
    }

    // =====================================================================
    // TEST-115 (REQ-028 (c), DEC-garmin-036) — the WRITE apparatus.
    // =====================================================================
    // Everything one stale-write run leaves behind. NOT a slot.
    struct StaleWriteOutcome
    {
        bool timedOut = false;
        int upListCount = 0;
        int checkedRows = 0;
        QStringList rowNames; // column 1 of every upload row, in row order

        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartTookTheStartBranch = false;

        QString staleName; // the abandoned batch's outstanding write
        QString liveName;  // ...and the restarted batch's
        QStringList writeNames;

        // Read either side of the late delivery, so "which row changed" is an
        // observation rather than an inference. `Final` is read after the SECOND
        // delivery of the same-row run, and is a copy of `After` otherwise.
        QStringList statusesBefore;
        QStringList statusesAfter;
        QStringList statusesFinal;
        QList<int> rowsRelabelled;

        int writeCallsBefore = 0;
        int writeCallsAfter = 0;
        int writeCallsFinal = 0;
        int barBefore = 0;
        int barAfter = 0;
        int barMax = 0;
        QString progressTextAfter;
        QString progressTextFinal;
    };

    // One run of the stale-write geometry. `sameRow` puts a SINGLE row in the
    // upload list, so that the restarted batch re-dispatches the very row the
    // abandoned batch is still waiting on - and then delivers BOTH completions.
    StaleWriteOutcome runStaleWrite(bool sameRow)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        StaleWriteOutcome out;
        const int rows = sameRow ? 1 : 2;

        // The upload list is built from context->athlete->rideCache->rides(), and
        // the rows must exist on disk because uploadNext opens each one.
        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        for (int i = 0; i < rows; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false; // the upload list skips planned rides
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // NOTHING remote, so no row is marked as already existing and the
                // "File exists" skip cannot swallow one.
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->writeSucceeds = true;
                // THE WHOLE MECHANISM, and the twin of runLateCompletion's
                // completeRead=false: writeFile reports the upload started and
                // notifies nobody, so the batch parks exactly as a real one waits
                // on the network and this run owns the timing of every completion.
                store->completeWrite = false;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // The UPLOAD tab: its column-1 header is "File" and its status
                // column is 7.
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(1);
                dialog->selectAllUpChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("File"));

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.upListCount = root->childCount();
                    for (int i = 0; i < out.upListCount; i++) {
                        out.rowNames << root->child(i)->text(1);
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                }

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Upload"));
                QProgressBar* bar = dialog->findChild<QProgressBar*>();

                // ---- BATCH ONE. uploadNext writes row 0 and returns.
                dialog->downloadClicked();
                QString parked = obs::lastWriteName;

                if (!sameRow) {
                    // Row 0 completes normally. completedWrite's tail re-drives
                    // uploadNext, so the batch moves onto row 1 and parks there.
                    store->notifyWriteComplete(parked, QStringLiteral("Completed."));
                    parked = obs::lastWriteName;
                }
                out.staleName = parked;

                // ---- THE ABORT, AND THE RESTART BEHIND IT.
                out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                dialog->downloadClicked(); // the abort branch
                // The abort branch relabels that same button "Download", which is
                // the observable proof it ran and `aborted` is now true.
                out.abortTookTheAbortBranch = (watched != nullptr && watched->text() == QStringLiteral("Download"));

                dialog->downloadClicked(); // a NEW batch, from row 0
                out.restartTookTheStartBranch = (watched != nullptr && watched->text() == QStringLiteral("Abort"));
                out.liveName = obs::lastWriteName;

                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statusesBefore << list->invisibleRootItem()->child(i)->text(7);
                out.writeCallsBefore = obs::writeFileCalls;
                if (bar != nullptr) {
                    out.barBefore = bar->value();
                    out.barMax = bar->maximum();
                }

                // ---- THE LATE COMPLETION: batch one's write, arriving now.
                store->notifyWriteComplete(out.staleName, QStringLiteral("Completed."));

                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statusesAfter << list->invisibleRootItem()->child(i)->text(7);
                for (int i = 0; i < out.statusesAfter.count() && i < out.statusesBefore.count(); i++)
                    if (out.statusesAfter.at(i) != out.statusesBefore.at(i))
                        out.rowsRelabelled << i;
                out.writeCallsAfter = obs::writeFileCalls;
                if (bar != nullptr)
                    out.barAfter = bar->value();
                out.progressTextAfter = progressLabelText(dialog);

                // ---- ...AND THE LIVE ONE, for the SAME row under the SAME name.
                //      Only the ticket's one-shot `armed` bit is left to tell it
                //      from the completion just consumed.
                if (sameRow)
                    store->notifyWriteComplete(out.liveName, QStringLiteral("Completed."));

                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statusesFinal << list->invisibleRootItem()->child(i)->text(7);
                out.writeCallsFinal = obs::writeFileCalls;
                out.progressTextFinal = progressLabelText(dialog);
                out.writeNames = obs::writeNames;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }

        return out;
    }
    // Everything one run of the probe leaves behind. NOT a slot.
    struct LateCompletionOutcome
    {
        bool timedOut = false;
        int listCount = 0;
        int checkedRows = 0;
        QStringList rowNames; // column 1 of every row, in row order

        bool firstCompletionDelivered = false;
        bool lateCompletionDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartTookTheStartBranch = false;

        QString oldBatchRowName; // the row whose completion is delivered late
        QString newBatchRowName; // the row in flight when it is delivered

        // Read either side of that delivery, so "which row changed" is an
        // observation rather than an inference.
        QStringList statusesBefore;
        QStringList statusesAfter;
        QList<int> rowsRelabelled;
        int readFileCallsBefore = 0;
        int readFileCallsAfter = 0;
        int barBefore = 0;
        int barAfter = 0;
        int barMax = 0;
        QStringList readNames; // every row readFile was called for, in order

        // TEST-116 — buffer accounting, asked of the ALLOCATOR either side of the
        // late delivery. `staleAndLiveBuffersDiffer` is the premise that makes the
        // two below discriminating at all.
        bool staleAndLiveBuffersDiffer = false;
        bool staleBufferFreed = false;    // the swallowed one went back
        bool liveBufferStillHeld = false; // ...and the one still in flight did not
    };

    // Three remote activities the Download tab lists and cannot parse. Today's
    // date because refreshClicked filters on DATE only (:1613), and .gcfail for
    // the reason the slot comment gives.
    static QStringList threeUnparseableRemoteActivities()
    {
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        return QStringList() << (day + QStringLiteral("_19_00_00.gcfail")) << (day + QStringLiteral("_20_00_00.gcfail"))
                             << (day + QStringLiteral("_21_00_00.gcfail"));
    }

    // One run of the probe. `restartBetween` false is the CONTROL.
    //
    // `lateOnFailureChannel` delivers the LATE completion on DEC-023's explicit
    // failure channel (readFailed) instead of readComplete. It is the same
    // transfer, the same buffer and the same abandoned batch - only the slot that
    // consumes it differs - and it exists because failedRead carries its own copy
    // of the DEC-036 ticket check and its own free, and a guard no run reaches is
    // a guard nothing can measure (LSN-059). The FIRST completion stays a success
    // whichever channel is chosen, because it is what walks the batch onto row 1.
    LateCompletionOutcome runLateCompletion(bool restartBetween, bool lateOnFailureChannel = false)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        LateCompletionOutcome out;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeUnparseableRemoteActivities();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102):
                // uncompressRide's first guard rejects outright on the default.
                store->downloadCompression = CloudService::none;
                // THE WHOLE MECHANISM. readFile returns without notifying, so the
                // batch parks and this run owns the timing of every completion.
                store->completeRead = false;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // The DOWNLOAD tab: its column-1 header is "Workout Name"
                // (CloudService.cpp:1100) and its status column is 5, not 7.
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(0);
                dialog->selectAllChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        out.rowNames << root->child(i)->text(1);
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                }

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Download"));
                QProgressBar* bar = dialog->findChild<QProgressBar*>();

                // ---- BATCH ONE. downloadNext dispatches row 0 and returns.
                dialog->downloadClicked();
                QByteArray* row0Buffer = obs::lastBuffer;
                const QString row0Name = obs::lastReadName;
                out.readNames << obs::lastReadName;

                // Row 0 completes normally. completedRead's tail re-drives
                // downloadNext, so the batch moves onto row 1 and parks there.
                if (row0Buffer != nullptr) {
                    out.firstCompletionDelivered = true;
                    store->notifyReadComplete(row0Buffer, row0Name, QStringLiteral("Completed."));
                }
                QByteArray* row1Buffer = (obs::lastBuffer != row0Buffer) ? obs::lastBuffer : nullptr;
                const QString row1Name = obs::lastReadName;
                if (row1Buffer != nullptr)
                    out.readNames << row1Name;
                out.oldBatchRowName = row1Name;
                out.newBatchRowName = row1Name; // ...unless a restart moves it

                // ---- THE ABORT, AND THE RESTART BEHIND IT.
                if (restartBetween) {
                    out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                    dialog->downloadClicked(); // :1920-1928, aborted = true
                    out.abortTookTheAbortBranch = (watched != nullptr && watched->text() == QStringLiteral("Download"));

                    dialog->downloadClicked(); // :1919-1935, a NEW batch from row 0
                    out.restartTookTheStartBranch = (watched != nullptr && watched->text() == QStringLiteral("Abort"));
                    out.newBatchRowName = obs::lastReadName;
                    out.readNames << obs::lastReadName;
                }

                // TEST-116 — the restarted batch's OWN buffer, the one that is
                // still outstanding when the stale completion lands.
                QByteArray* liveBuffer = obs::lastBuffer;
                out.staleAndLiveBuffersDiffer = (row1Buffer != nullptr && liveBuffer != nullptr &&
                                                 (restartBetween == false || row1Buffer != liveBuffer));

                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statusesBefore << list->invisibleRootItem()->child(i)->text(5);
                out.readFileCallsBefore = obs::readFileCalls;
                if (bar != nullptr) {
                    out.barBefore = bar->value();
                    out.barMax = bar->maximum();
                }

                // ---- THE LATE COMPLETION: batch one's row-1 read, arriving now,
                //      on whichever of DEC-023's two channels this run chose.
                if (row1Buffer != nullptr) {
                    out.lateCompletionDelivered = true;
                    if (lateOnFailureChannel)
                        store->notifyReadFailed(row1Buffer, row1Name, QStringLiteral("service refused"));
                    else
                        store->notifyReadComplete(row1Buffer, row1Name, QStringLiteral("Completed."));
                }

                if (list != nullptr)
                    for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                        out.statusesAfter << list->invisibleRootItem()->child(i)->text(5);
                for (int i = 0; i < out.statusesAfter.count() && i < out.statusesBefore.count(); i++)
                    if (out.statusesAfter.at(i) != out.statusesBefore.at(i))
                        out.rowsRelabelled << i;
                out.readFileCallsAfter = obs::readFileCalls;
                if (out.readFileCallsAfter > out.readFileCallsBefore)
                    out.readNames << obs::lastReadName;
                if (bar != nullptr)
                    out.barAfter = bar->value();

                // TEST-116 — ...and what the allocator says about the two buffers.
                // Read HERE, before anything else can free either of them.
                out.staleBufferFreed = isPoisoned(row1Buffer, sizeof(QByteArray));
                out.liveBufferStillHeld = (liveBuffer != nullptr && !isPoisoned(liveBuffer, sizeof(QByteArray)));

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
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

    // =====================================================================
    // TEST-117 / TEST-118 (A3-R028-F1 / A3-R028-F2, REQ-028 (c),
    // DEC-garmin-036 AS AMENDED) — A RESTART THAT ARMS NOTHING.
    // =====================================================================
    //
    // THE CRITERION, VERBATIM (REQ-028 (c)):
    //
    //   "(c) NO ROW IS LABELLED THAT THE FRAME DID NOT TRANSFER, AND NO ROW IS
    //   TRANSFERRED TWICE. Across a batch that is aborted and immediately
    //   restarted with the aborted batch's completion arriving LATE, every
    //   non-empty status cell names the outcome of a transfer that actually
    //   happened to that row, and the reader is invoked exactly once per checked
    //   row - an over-count catches a second driver, an under-count catches a
    //   stale one."
    //
    // WHAT DEC-036 LEFT OPEN, AND THE AMENDMENT CLOSES. The ticket is ARMED at
    // four dispatch sites and CONSUMED at three completion sites; before the
    // amendment nothing INVALIDATED it. TEST-113/115 only ever exercised a
    // restart that DISPATCHES - and a dispatch overwrites the ticket, which is
    // what made the stale completion's token compare fail there. A restart that
    // dispatches NOTHING overwrites nothing, so the abandoned batch's ticket
    // survives the restart intact - and `batchListGeneration = listGeneration`
    // (CloudService.cpp:1951) hands that stale ticket a freshly VALID generation
    // stamp on the way past. Every guard then agrees the completion is welcome.
    //
    // TWO SHAPES OF "ARMS NOTHING", and they cost different things:
    //
    //   TEST-117 (F1)  the restart arms nothing because a REFRESH emptied the
    //                  checkboxes - and the same Refresh FREED the row the ticket
    //                  is holding. The completion labels through a dangling
    //                  QTreeWidgetItem*: a heap-use-after-free, not a mislabel.
    //   TEST-118 (F2)  the restart arms nothing because the user simply cleared
    //                  Select All. Nothing is freed, so this is not a memory
    //                  defect - it is the second-driver harm the criterion names:
    //                  a row relabelled by a transfer the live batch never made,
    //                  and a progress bar advanced past a total of zero.
    //
    // BOTH RUN THE SAME FOUR ORDINARY CLICKS in the same apparatus, and the
    // CONTROL is shared and load-bearing (LSN-050): the identical delivery with
    // no abort and no restart in between MUST still label row 0, still advance
    // the bar and still re-drive onto row 1. Without it every assertion below is
    // equally satisfied by a completedRead that swallows everything.
    enum RestartShape {
        NoRestartControl,      // the control: batch one's own completion, on time
        RestartAfterRefresh,   // F1 - the rows are FREED before the restart
        RestartAfterUnchecking // F2 - nothing is freed; the batch is just empty
    };

    struct RestartArmsNothingOutcome
    {
        bool timedOut = false;

        // -- premises about the fixture
        int listCount = 0;
        int checkedRows = 0;
        QStringList rowNames;

        // -- premises about batch one
        bool firstDispatchIssued = false; // row 0's read was really issued...
        QString dispatchedRowName;        // ...and it was THIS row
        int readFileCallsAfterDispatch = 0;

        // -- premises about the abort and the restart
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        int checkedRowsAtRestart = -1;
        QString progressTextAfterRestart; // only downloadNext's TAIL writes this
        int readFileCallsAfterRestart = 0;

        // -- F1's premise: the row the ticket holds is really dead. Asked of the
        //    ALLOCATOR, never by dereferencing it.
        bool rowFreedByRefresh = false;

        // -- the delivery, and what it did
        bool lateCompletionDelivered = false;
        QStringList statusesBefore;
        QStringList statusesAfter;
        QList<int> rowsRelabelled;
        int barBefore = 0;
        int barAfter = 0;
        int barMax = 0;
        int readFileCallsAfterLate = 0;
        QString progressTextAfterLate;

        // Set on the last line of the run: under ASan with halt_on_error=1 a
        // use-after-free ends the PROCESS, so a run that gets this far is a run
        // that committed none.
        bool survivedTheLateCompletion = false;
    };

    // One run of the geometry. NOT a slot (it takes an argument).
    //
    // HOW THE LATE COMPLETION IS MADE is runLateCompletion's mechanism exactly:
    // `store->completeRead = false` means readFile parks without notifying, and
    // the run delivers the completion by hand through notifyReadComplete - the
    // same `emit readComplete(...)` on the same direct connection, so completedRead
    // runs in the frame shape it always does. What changes is only WHEN.
    RestartArmsNothingOutcome runRestartThatArmsNothing(RestartShape shape)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        RestartArmsNothingOutcome out;
        QEventLoop appLoop;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = threeUnparseableRemoteActivities();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                store->completeRead = false; // this run owns the timing

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(0);
                dialog->selectAllChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));

                auto countChecked = [&]() {
                    int n = 0;
                    if (list == nullptr)
                        return n;
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            n++;
                    }
                    return n;
                };
                auto statuses = [&]() {
                    QStringList s;
                    if (list != nullptr)
                        for (int i = 0; i < list->invisibleRootItem()->childCount(); i++)
                            s << list->invisibleRootItem()->child(i)->text(5);
                    return s;
                };

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++)
                        out.rowNames << root->child(i)->text(1);
                }
                out.checkedRows = countChecked();

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Download"));
                QProgressBar* bar = dialog->findChild<QProgressBar*>();

                // ---- CLICK 1. Select all, Download. downloadNext arms the ticket
                //      for row 0 and parks on it.
                dialog->downloadClicked();
                QByteArray* row0Buffer = obs::lastBuffer;
                const QString row0Name = obs::lastReadName;
                out.firstDispatchIssued = (row0Buffer != nullptr);
                out.dispatchedRowName = row0Name;
                out.readFileCallsAfterDispatch = obs::readFileCalls;

                // The ADDRESS of the row the ticket is holding, kept as raw
                // storage. Never cast back, never dereferenced - asking ASan
                // whether it is poisoned is the only way to ask "is it dead?"
                // without committing the use-after-free under investigation.
                const void* row0Addr = (list != nullptr && list->invisibleRootItem()->childCount() > 0)
                                           ? static_cast<const void*>(list->invisibleRootItem()->child(0))
                                           : nullptr;

                if (shape != NoRestartControl) {
                    out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);

                    // ---- CLICK 2 (F1 only). Refresh. This is the ONE place a row
                    //      in this dialog dies, and the row it kills is the one the
                    //      ticket is holding.
                    if (shape == RestartAfterRefresh) {
                        dialog->refreshClicked();
                        out.rowFreedByRefresh = isPoisoned(row0Addr, sizeof(QTreeWidgetItem));
                    }

                    // ---- CLICK 3. The button reads "Abort", so this click takes
                    //      downloadClicked's abort branch: downloading=false,
                    //      aborted=true, and the ticket is left ARMED.
                    dialog->downloadClicked();
                    out.abortTookTheAbortBranch = (watched != nullptr && watched->text() == QStringLiteral("Download"));

                    // F2's way of emptying the batch: the user clears Select All.
                    // Nothing is freed - which is what makes F2 the same root
                    // cause without the memory unsafety.
                    if (shape == RestartAfterUnchecking)
                        dialog->selectAllChanged(Qt::Unchecked);

                    out.checkedRowsAtRestart = countChecked();

                    // ---- CLICK 4. The button reads "Download" again, so this
                    //      takes the START branch: aborted=false and
                    //      batchListGeneration = listGeneration. Nothing is
                    //      checked, so downloadNext dispatches NOTHING and falls
                    //      straight to its completion tail.
                    dialog->downloadClicked();
                    out.readFileCallsAfterRestart = obs::readFileCalls;
                    // Only downloadNext's TAIL writes this sentence
                    // (CloudService.cpp:2535) - the abort branch returns long
                    // before it - so this one string is the observable proof that
                    // the START branch ran AND that its batch was empty. It is
                    // also where `downloadtotal` is observable at all: it is a
                    // private member, and this is production's own report of it.
                    out.progressTextAfterRestart = progressLabelText(dialog);
                }

                out.statusesBefore = statuses();
                if (bar != nullptr) {
                    out.barBefore = bar->value();
                    out.barMax = bar->maximum();
                }

                // ---- THE LATE COMPLETION: batch one's row-0 read, arriving now.
                if (row0Buffer != nullptr) {
                    out.lateCompletionDelivered = true;
                    store->notifyReadComplete(row0Buffer, row0Name, QStringLiteral("Completed."));
                }

                out.statusesAfter = statuses();
                for (int i = 0; i < out.statusesAfter.count() && i < out.statusesBefore.count(); i++)
                    if (out.statusesAfter.at(i) != out.statusesBefore.at(i))
                        out.rowsRelabelled << i;
                out.readFileCallsAfterLate = obs::readFileCalls;
                if (bar != nullptr)
                    out.barAfter = bar->value();
                out.progressTextAfterLate = progressLabelText(dialog);
                out.survivedTheLateCompletion = true;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
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

    // The premises shared by TEST-117 and TEST-118, and the CONTROL that stops
    // either of them passing on a completedRead that does nothing at all. NOT a
    // slot.
    void assertRestartArmsNothingControl(const RestartArmsNothingOutcome& base, const QString& verdict)
    {
        QVERIFY2(base.timedOut == false, "control: the run never came back");
        QCOMPARE(base.listCount, 3);
        QCOMPARE(base.checkedRows, 3);
        QVERIFY2(base.firstDispatchIssued, "control: row 0's read was never issued");
        QVERIFY2(base.lateCompletionDelivered, "control: nothing was delivered");
        QCOMPARE(base.readFileCallsAfterDispatch, 1);

        // The apparatus CAN see a label, CAN see the bar move and CAN see the
        // loop drive on - so the run's "none of these happened" means something.
        QCOMPARE(base.statusesBefore, QStringList() << "Downloading" << "" << "");
        QCOMPARE(base.statusesAfter, QStringList() << verdict << "Downloading" << "");
        QCOMPARE(base.rowsRelabelled, QList<int>() << 0 << 1);
        QCOMPARE(base.barBefore, 0);
        QCOMPARE(base.barAfter, 1);
        QCOMPARE(base.readFileCallsAfterLate, 2);
    }

  private slots:

    // -- TEST-117 (A3-R028-F1) -------------------------------------------
    //
    // FOUR ORDINARY CLICKS ON THE DOWNLOAD TAB REACH A USE-AFTER-FREE. Select
    // all + Download (the ticket is armed for row 0); Refresh (row 0 is FREED,
    // and the rebuilt list is entirely unchecked); Abort; Download (the START
    // branch re-stamps batchListGeneration but dispatches nothing, so the stale
    // ticket survives with a valid stamp). Row 0's completion then passes the
    // DEC-034 compare, passes the ticket compare - it really IS the transfer the
    // ticket describes - passes the abort read, and labels through the freed row.
    //
    // THE KILLING OBSERVATION IS THE ASan ABORT, not a QVERIFY. This target runs
    // with halt_on_error=1, so a use-after-free ends the process and this slot
    // never returns; reaching its final assertion at all is the assertion. What
    // the QVERIFYs below do is stop that from being VACUOUS (LSN-050): without
    // the poison premise a slot like this passes just as happily when production
    // never reaches the row at all, and without the readFileCalls premise it
    // passes when the restart quietly dispatched something and overwrote the
    // ticket the ordinary way.
    //
    // RED, against the tree before the DEC-036 amendment (measured, offscreen;
    // the offsets are that tree's, and the amendment's own comments have moved
    // them since - prefer the symbols, ORCH-020/LSN-034):
    //   AddressSanitizer: heap-use-after-free READ of size 8
    //     #0 QTreeWidgetItem::setText ... in CloudServiceSyncDialog::completedRead
    //        CloudService.cpp:2654   [completedRead's no-ride row->setText]
    //     freed by refreshClicked() CloudService.cpp:1542
    //                               [the download list's `delete curr`]
    //
    // WHAT THIS SLOT KILLS, MEASURED, so nobody reads more coverage into it than
    // it has (LSN-059/LSN-063). The amendment has TWO invalidation sites and this
    // slot is a run on the PAIR: with only the downloadClicked one removed it
    // stays GREEN, because the Refresh in its own route has already voided the
    // ticket through the refreshClicked one. It goes red only with both gone -
    // which is the state the finding was raised against, and is the geometry
    // production shipped. TEST-118, whose route contains no Refresh, is the slot
    // that kills the downloadClicked site on its own.
    void aRestartThatArmsNothingMustNotLetAStaleCompletionLabelAFreedRow()
    {
        const QString verdict = QStringLiteral("TEST-096 unparseable activity");

        // ---- THE CONTROL, in this slot rather than a neighbour's (LSN-050).
        assertRestartArmsNothingControl(runRestartThatArmsNothing(NoRestartControl), verdict);
        if (QTest::currentTestFailed())
            return;

        const RestartArmsNothingOutcome out = runRestartThatArmsNothing(RestartAfterRefresh);
        QVERIFY2(out.timedOut == false, "the run never came back");

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, 3);
        QCOMPARE(out.checkedRows, 3);
        QVERIFY2(out.firstDispatchIssued, "row 0's read was never issued, so no ticket was ever armed");
        QCOMPARE(out.dispatchedRowName, out.rowNames.value(0));
        QCOMPARE(out.readFileCallsAfterDispatch, 1);
        QVERIFY2(out.sawAbortLabel, "the button was not labelled \"Abort\" while the batch ran, so downloadClicked() "
                                    "could not have been the abort control");
        QVERIFY2(out.abortTookTheAbortBranch, "downloadClicked() did not take its abort branch");

        // THE PREMISE THIS SLOT RESTS ON. The row the ticket is holding is dead:
        // the Refresh really did free it, so a completion that labels through the
        // ticket's pointer really is a use-after-free and not a mislabel.
        QVERIFY2(out.rowFreedByRefresh,
                 "the Refresh did NOT free row 0 - the ticket's pointer is still live, so this run cannot "
                 "distinguish a guard from a production path that simply never reaches the row");

        // ...AND THE RESTART REALLY ARMED NOTHING, which is the whole of what
        // separates this route from TEST-113's. A restart that dispatched would
        // have overwritten the ticket and closed the route by accident.
        QCOMPARE(out.checkedRowsAtRestart, 0);
        QCOMPARE(out.readFileCallsAfterRestart, out.readFileCallsAfterDispatch);
        QCOMPARE(out.progressTextAfterRestart, QStringLiteral("Downloaded 0 of 0 successfully"));
        QVERIFY2(out.lateCompletionDelivered, "row 0's completion was never delivered, so nothing could go stale");

        // ---- THE CRITERION. Reaching here is the ASan verdict; the rebuilt
        //      list's cells are the second half - a completion belonging to a
        //      list that no longer exists labels nothing in the one that replaced
        //      it either.
        QVERIFY2(out.survivedTheLateCompletion, "the run did not reach the end of the delivery");
        QCOMPARE(out.statusesBefore, QStringList() << "" << "" << "");
        QCOMPARE(out.statusesAfter, out.statusesBefore);
        QCOMPARE(out.rowsRelabelled, QList<int>());
        QCOMPARE(out.readFileCallsAfterLate, out.readFileCallsAfterRestart);
    }

    // -- TEST-118 (A3-R028-F2) -------------------------------------------
    //
    // THE SAME ROOT CAUSE WITH NO REFRESH IN THE ROUTE, so nothing is freed and
    // nothing is unsafe: this is the SECOND-DRIVER harm the criterion names.
    // Select all + Download; Abort; clear Select All; Download - the START branch
    // dispatches nothing and its tail reports "Downloaded 0 of 0 successfully".
    // The abandoned batch's completion still owns row 0, so it may label that
    // transferred row Aborted. It must not advance the empty live batch's bar or
    // re-drive it.
    //
    // THE BAR INVARIANT, and why it is spelled this way: `downloadtotal` is a
    // private member, so the total is read from production's OWN report of it -
    // the completion tail's "Downloaded %1 of %2 successfully" - and the bar's
    // value is compared against that. A QProgressBar clamps to its maximum, and
    // the maximum here is still 3 from batch one (the empty batch never resets
    // it), so nothing but this comparison would notice.
    //
    // RED, against the tree before the DEC-036 amendment (measured, offscreen):
    //   FAIL!  : ...the bar advanced to 1 for a batch whose total is 0
    void aRestartThatArmsNothingMustNotLetAStaleCompletionDriveTheEmptyBatch()
    {
        const QString verdict = QStringLiteral("TEST-096 unparseable activity");

        assertRestartArmsNothingControl(runRestartThatArmsNothing(NoRestartControl), verdict);
        if (QTest::currentTestFailed())
            return;

        const RestartArmsNothingOutcome out = runRestartThatArmsNothing(RestartAfterUnchecking);
        QVERIFY2(out.timedOut == false, "the run never came back");

        // ---- THE PREMISES.
        QCOMPARE(out.listCount, 3);
        QCOMPARE(out.checkedRows, 3);
        QVERIFY2(out.firstDispatchIssued, "row 0's read was never issued, so no ticket was ever armed");
        QCOMPARE(out.readFileCallsAfterDispatch, 1);
        QVERIFY2(out.sawAbortLabel, "the button was not labelled \"Abort\" while the batch ran");
        QVERIFY2(out.abortTookTheAbortBranch, "downloadClicked() did not take its abort branch");
        // Nothing was freed on this route - said explicitly, because it is the
        // difference between this slot and TEST-117.
        QVERIFY2(out.rowFreedByRefresh == false, "a row was freed on a route with no Refresh in it");
        QCOMPARE(out.checkedRowsAtRestart, 0);
        QCOMPARE(out.readFileCallsAfterRestart, out.readFileCallsAfterDispatch);
        QVERIFY2(out.lateCompletionDelivered, "row 0's completion was never delivered, so nothing could go stale");

        // The empty batch reached its tail and reported a total of ZERO. This is
        // both the proof that the START branch ran and the value the invariant
        // below is measured against.
        QCOMPARE(out.progressTextAfterRestart, QStringLiteral("Downloaded 0 of 0 successfully"));
        const int downloadtotal = 0;

        // ---- THE CRITERION, half one: the bar invariant.
        QCOMPARE(out.barBefore, 0);
        QVERIFY2(out.barAfter <= downloadtotal,
                 qPrintable(QStringLiteral("the bar advanced to %1 for a batch whose total is %2 - the abandoned "
                                           "batch's completion counted work the live batch never did")
                                .arg(out.barAfter)
                                .arg(downloadtotal)));

        // ---- half two: no row is labelled that the frame did not transfer. The
        //      preserved operation labels its own row 0 Aborted; the empty live
        //      batch transferred and labels nothing.
        QCOMPARE(out.statusesBefore, QStringList() << "Downloading" << "" << "");
        QCOMPARE(out.statusesAfter, QStringList() << "Aborted" << "" << "");
        QCOMPARE(out.rowsRelabelled, QList<int>() << 0);

        // ...and no second driver: the swallowed completion does not re-drive the
        // loop over the empty batch.
        QCOMPARE(out.readFileCallsAfterLate, out.readFileCallsAfterRestart);
    }

  private:
    // =====================================================================
    // REQ-028 / DEC-garmin-034 — WHO OWNS A ROW ACROSS A SUSPENSION.
    // =====================================================================
    //
    // THE DEFECT. refreshClicked (CloudService.cpp:1516) deletes EVERY
    // QTreeWidgetItem in all three lists (:1542/:1549/:1554 — takeChildren() then
    // delete) and rebuilds them. Every frame that is suspended at that moment is
    // holding either a raw `QTreeWidgetItem *curr` captured before the suspension
    // (syncNext:2014, uploadNext:2682) or an INDEX into the list it no longer owns
    // (`child(listindex-1)` in completedRead/failedRead/completedWrite). The
    // Refresh button is never disabled — four references in the whole file, not
    // one of them a setEnabled — and the code's own comment at :2126-2134 already says
    // a Refresh is deliverable from inside openRideFile's nested loop.
    //
    // THE DELIVERY POINTS BELOW ARE THE ONES THIS FIXTURE DRIVES, one per guard,
    // so that each guard has a run that dies when it is removed. They are NOT
    // several spellings of one scenario: each names a different production frame,
    // reached by a different route.
    //
    // WHAT THIS LIST IS NOT (A3-R028b-F8). It is NOT the closed set of places a
    // Refresh can be delivered into, and it never was - it was written as one and
    // that claim was false when it was written. A Refresh is deliverable from any
    // nested event loop and from any QApplication::processEvents() in a frame that
    // then carries on, and this file knows of these OTHER ones:
    //
    //   * the three completion slots' own TAIL processEvents(), below every guard
    //     they have, which re-drives the loop - driven by TEST-119/TEST-120 in a
    //     separate fixture (runRefreshInCompletionTail) because the delivery is
    //     synchronous rather than inside a nested loop;
    //   * the two DRIVER parse-failure branches' processEvents()
    //     (CloudService.cpp:2428, :3184), which `continue` past it onto the
    //     rebuilt list - TEST-122 and TEST-123, same reason, same other fixture;
    //   * saveRide's own autoProcess (CloudService.cpp:3396 -> FixElevation's
    //     untimed QEventLoop), reached from completedRead's ride-bearing branch -
    //     InSaveRideAutoProcess below, which IS in this fixture;
    //   * RESIDUAL, not driven anywhere in this suite: every other
    //     QApplication::processEvents() in the file that is not followed by a
    //     return (they are enumerated nowhere and nobody has counted them), and
    //     Athlete::addRide / rideCache->save (CloudService.cpp:3407/:2527), which
    //     this target stubs flat and whose production bodies have not been walked
    //     for suspensions. A future wave that finds one must add it here.
    //
    // WHAT MAKES THE ROWS REALLY DIE. Nothing is faked: the run calls the
    // dialog's own refreshClicked() slot from inside the nested loop, exactly as
    // the button's connect (:1225) would. `rowsWereFreed` records the row-0 item
    // POINTER either side of that call as an integer and never dereferences it,
    // so "the items were replaced" is an observation rather than an assumption.
    enum RefreshWhere {
        InSyncNextOpen,       // syncNext's openRideFile (:2150), an UPLOAD sync row
        InUploadNextOpen,     // uploadNext's openRideFile (:2714), the upload tab's twin
        InUncompressOpen,     // completedRead's uncompressRide -> openRideFile (:363)
        InReadFileLoop,       // store->readFile's own nested loop, ahead of failedRead
        InWriteFileLoop,      // store->writeFile's own nested loop, ahead of completedWrite
        InSaveRideAutoProcess // completedRead's saveRide -> autoProcess (:3267), TEST-121
    };

    struct RebuildSpec
    {
        RefreshWhere where = InSyncNextOpen;
        bool abortFirst = false;              // the burst sets `aborted` BEFORE the Refresh
        bool failTheRead = false;             // ...and does the read REFUSE, so failedRead is the slot?
        bool restartInsteadOfRefresh = false; // abort + immediate restart inside a driver's parse
    };

    struct RebuildOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the situation it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        QString row0Action; // sync list column 6: "Upload" or "Download"
        bool refreshDelivered = false;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartDelivered = false;
        int rowsBeforeRefresh = -1;
        int rowsAfterRefresh = -1;
        bool rowsWereFreed = false;

        // -- the verdict
        int writeFileCalls = 0;
        int readFileCalls = 0;
        int rideOpens = 0;
        QStringList statuses;
        int labelledRows = 0; // rows of the REBUILT list carrying any status text
        QString progressText; // the completion tail's "...successfully" sentence
        QString buttonTextAtEnd;
        int progressValue = -1;
        int stillCheckedAtEnd = -1;

        // TEST-121 only: was saveRide's autoProcess seam actually reached?
        int autoProcessCalls = 0;
    };

    // Two local activities, both PARSEABLE (.gcblock), so that row[0]'s own
    // openRideFile is the delivery point criterion (a) names. Distinct times so
    // the row order is well defined.
    static QString rebuildLocalActivity(int i)
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) +
               QStringLiteral("_1%1_00_00.gcblock").arg(i);
    }

    // Two REMOTE activities, for the runs that need sync-list DOWNLOAD rows
    // (:1691-1722). Parseable too: completedRead's uncompressRide hands the bytes
    // to the same RideFileFactory dispatch on the suffix.
    static QString rebuildRemoteActivity(int i)
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) +
               QStringLiteral("_2%1_00_00.gcblock").arg(i);
    }

    // One run:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call [event delivery]
    //          -> owner QWidget                    [stands in for the tab]
    //          -> CloudServiceSyncDialog, a child of it
    //          -> Sync|Upload tab / Select all / Synchronize
    //               -> syncNext|uploadNext -> ... -> A NESTED EVENT LOOP
    //                    -> refreshClicked()   [the rows are deleted and rebuilt]
    //                    -> the suspended frame RESUMES onto them
    //
    // Nothing here destroys the dialog or the store: every `self.isNull()` bail in
    // the file stays false throughout, so no lifetime guard can be what produces
    // any of these verdicts.
    RebuildOutcome runListRebuild(const RebuildSpec& spec)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;
        gcstub::autoProcessAction = nullptr;
        gcstub::autoProcessCalls = 0;

        // Which side of the sync list the batch runs on. The completion slots are
        // reached through a DOWNLOAD row (readFile -> completedRead/failedRead);
        // the two drivers through an UPLOAD row (openRideFile -> writeFile).
        const bool remoteRows =
            (spec.where == InUncompressOpen || spec.where == InReadFileLoop || spec.where == InSaveRideAutoProcess);

        RebuildOutcome out;
        QEventLoop appLoop;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        if (!remoteRows) {
            for (int i = 0; i < 2; i++) {
                const QString name = rebuildLocalActivity(i);
                QFile f(activities.absolutePath() + "/" + name);
                f.open(QIODevice::WriteOnly);
                f.write("gcblock");
                f.close();
                paths << f.fileName();

                RideItem* item = new RideItem(nullptr, context);
                item->fileName = name;
                item->path = activities.absolutePath();
                item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
                item->planned = false; // the upload list skips planned rides (:1723)
                items << item;
            }
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = remoteRows ? (QStringList() << rebuildRemoteActivity(0) << rebuildRemoteActivity(1))
                                               : QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102):
                // uncompressRide's first guard rejects outright on the default.
                store->downloadCompression = CloudService::none;
                store->failRead = spec.failTheRead;
                store->blockInWrite = (spec.where == InWriteFileLoop);
                // A restart-inside-open run must leave the restarted row parked:
                // its subject is whether the suspended OLD driver dispatches the
                // same row as well, not the completion tail walking row[1].
                store->completeWrite = !spec.restartInsteadOfRefresh;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                QTreeWidget* list = nullptr;
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(spec.where == InUploadNextOpen ? 1 : 2);
                if (spec.where == InUploadNextOpen) {
                    dialog->selectAllUpChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("File"));
                } else {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Source"));
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.listCount > 0)
                        out.row0Action = root->child(0)->text(6);
                }

                QPushButton* button = pushButtonWithText(dialog, QStringLiteral("Synchronize"));
                if (button == nullptr)
                    button = pushButtonWithText(dialog, QStringLiteral("Upload"));

                // THE USER'S REFRESH, delivered from inside whichever nested loop
                // this run is about. `abortFirst` puts the Abort click in the same
                // burst, ahead of it: that is the window REQ-027 widened from a
                // read of the row to a WRITE (`curr->setText(7, "Aborted")`).
                const auto refresh = [&, dialog, list]() {
                    if (spec.restartInsteadOfRefresh) {
                        out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                        dialog->downloadClicked(); // abort the suspended frame
                        out.abortTookTheAbortBranch = (button != nullptr && button->text() != QStringLiteral("Abort"));
                        dialog->downloadClicked(); // immediately start a new generation
                        out.restartDelivered = (button != nullptr && button->text() == QStringLiteral("Abort"));
                        return;
                    }
                    out.refreshDelivered = true;
                    quintptr row0Before = 0;
                    if (list != nullptr) {
                        out.rowsBeforeRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsBeforeRefresh > 0)
                            row0Before = reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0));
                    }
                    if (spec.abortFirst) {
                        out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                        dialog->downloadClicked(); // :1920-1928, aborted = true
                        out.abortTookTheAbortBranch = (button != nullptr && button->text() != QStringLiteral("Abort"));
                    }

                    dialog->refreshClicked(); // :1516 — every row deleted, then rebuilt

                    if (list != nullptr) {
                        out.rowsAfterRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsAfterRefresh > 0)
                            out.rowsWereFreed =
                                (reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0)) != row0Before);
                    }
                };

                switch (spec.where) {
                case InSyncNextOpen:
                case InUploadNextOpen:
                case InUncompressOpen:
                    // BlockingRideFileReader queues the action and then runs its
                    // nested loop, so it lands while the frame is suspended
                    // between openRideFile and everything that uses the row.
                    rideopen::blockingMs = 300;
                    rideopen::action = refresh;
                    break;
                case InReadFileLoop:
                case InWriteFileLoop:
                    // BlockingStore::fireActionThenBlock does the same for the
                    // store's own blockingDownload/blockingUpload loop.
                    store->blockingMs = 300;
                    store->closeAction = refresh;
                    break;
                case InSaveRideAutoProcess:
                    // TEST-121 (A3-R028b-F1) — INSIDE saveRide, which the fixture
                    // reaches through completedRead's RIDE-BEARING branch: the row
                    // is remote and .gcblock, so uncompressRide really does yield a
                    // RideFile and CloudService.cpp:2811 really does call saveRide.
                    //
                    // The suspension is supplied where production has one, at
                    // DataProcessorFactory::autoProcess (:3396): FixElevation runs
                    // an UNTIMED QEventLoop on an HTTPS round trip from inside
                    // postProcess (FixElevation.cpp:288-300), gated on that
                    // processor's configKeyAutomation being "Auto". The loop below
                    // is that loop's SHAPE - queue the click, then exec() - and the
                    // click is delivered BY the loop, exactly as in
                    // BlockingRideFileReader, so nothing here relies on a
                    // processEvents() noticing something.
                    //
                    // The reader must NOT block for this run: the delivery has to
                    // land inside saveRide, not inside the uncompressRide above it.
                    rideopen::blockingMs = 5;
                    gcstub::autoProcessAction = [refresh]() {
                        QMetaObject::invokeMethod(qApp, refresh, Qt::QueuedConnection);
                        QEventLoop loop;
                        QTimer timer;
                        timer.setSingleShot(true);
                        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
                        timer.start(300);
                        loop.exec(QEventLoop::WaitForMoreEvents);
                    };

                    // saveRide REFUSES before it ever reaches autoProcess when the
                    // target .json already exists and this box is clear
                    // (CloudService.cpp:3386-3390) - and it does exist, because
                    // TEST-110 runs the same branch earlier in this process and
                    // CountingRideFile has no start time, so every run of it names
                    // the same file. Ticked through the real widget, by its label,
                    // so the run reaches the seam it is about. The premise below
                    // MEASURES that rather than trusting it.
                    for (QCheckBox* box : dialog->findChildren<QCheckBox*>())
                        if (box->text().contains(QStringLiteral("Overwrite")))
                            box->setChecked(true);
                    break;
                }

                dialog->downloadClicked(); // -> syncNext() / uploadNext()

                out.writeFileCalls = obs::writeFileCalls;
                out.readFileCalls = obs::readFileCalls;
                out.rideOpens = rideopen::opens;
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.stillCheckedAtEnd = 0;
                    for (int i = 0; i < root->childCount(); i++) {
                        const QString status = root->child(i)->text(7);
                        out.statuses << status;
                        if (!status.isEmpty())
                            out.labelledRows++;
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.stillCheckedAtEnd++;
                    }
                }
                if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
                    out.progressValue = bar->value();
                out.progressText = progressLabelText(dialog);
                if (button != nullptr)
                    out.buttonTextAtEnd = button->text();
                out.autoProcessCalls = gcstub::autoProcessCalls;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        gcstub::autoProcessAction = nullptr;

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // The shared premises for every run of the fixture above. A run that never
    // reached the situation must fail LOUDLY rather than pass on nothing
    // (LSN-047, LSN-050).
    void assertRebuildPremises(const QString& where, const RebuildOutcome& out)
    {
        QVERIFY2(out.timedOut == false,
                 qPrintable(where + QStringLiteral("the run never came back - a guard wedged it")));
        QVERIFY2(out.listCount == 2,
                 qPrintable(where + QStringLiteral("the list held %1 rows, not 2").arg(out.listCount)));
        QVERIFY2(out.checkedRows == 2,
                 qPrintable(where + QStringLiteral("%1 rows were checked, not 2").arg(out.checkedRows)));
        QVERIFY2(out.refreshDelivered,
                 qPrintable(where + QStringLiteral("the Refresh was never delivered into the nested loop - this run "
                                                   "proves nothing")));
        QVERIFY2(out.rowsBeforeRefresh == 2,
                 qPrintable(where + QStringLiteral("the list held %1 rows when the Refresh arrived, not 2")
                                        .arg(out.rowsBeforeRefresh)));
        QVERIFY2(out.rowsAfterRefresh == 2,
                 qPrintable(where + QStringLiteral("the Refresh rebuilt %1 rows, not 2 - the run would then be about "
                                                   "an empty list rather than a REPLACED one")
                                        .arg(out.rowsAfterRefresh)));
        QVERIFY2(out.rowsWereFreed,
                 qPrintable(where + QStringLiteral("row 0 is the SAME QTreeWidgetItem after the Refresh as before it, "
                                                   "so nothing was freed and this run proves nothing")));
    }

  private slots:
    // -- TEST-108 (REQ-028 (a), DEC-garmin-034) --------------------------
    // NO ROW IS TOUCHED AFTER IT IS FREED.
    //
    // A Sync batch of two checked rows with a Refresh delivered from inside
    // row[0]'s openRideFile nested loop - TEST-091's own delivery point - must
    // complete with ZERO AddressSanitizer reports, and the same must hold when the
    // burst ALSO sets `aborted` first.
    //
    // WHAT ASSERTS THE CRITERION. Not a QVERIFY: this target is built with
    // -fsanitize=address and runs with halt_on_error=1, so the criterion's own
    // measurement is the PROCESS - a report here aborts the binary and no later
    // slot runs at all. The QVERIFYs below establish that the run reached the
    // window (premises) and record what the guard did instead (verdict).
    //
    // RED, before the fix (PROBE-A's signature, re-measured by this slot):
    //   ==NNNN==ERROR: AddressSanitizer: heap-use-after-free READ of size 8
    //     #0 QTreeWidgetItem::text(int) const
    //     #1 CloudServiceSyncDialog::syncNext() CloudService.cpp:2159
    //    freed by thread T0 here: ... CloudServiceSyncDialog::refreshClicked()
    //   ...and with abortFirst, a WRITE of size 8 at :2153 instead - the window
    //   REQ-027 (DEC-032/035) widened from a read to a write.
    void aRefreshInsideARowsOpenMustNotLeaveTheLoopHoldingThatRow()
    {
        // ---- (a) THE CRITERION'S OWN SCENARIO: syncNext, the sync tab.
        {
            RebuildSpec spec;
            spec.where = InSyncNextOpen;
            const RebuildOutcome out = runListRebuild(spec);
            const QString where = QStringLiteral("syncNext: ");
            assertRebuildPremises(where, out);
            if (QTest::currentTestFailed())
                return;
            QCOMPARE(out.row0Action, QStringLiteral("Upload"));
            QVERIFY2(out.rideOpens >= 1,
                     qPrintable(where + QStringLiteral("row[0]'s ride file was never opened, so "
                                                       "the Refresh had no nested loop to arrive in")));

            // THE VERDICT. The frame stood down instead of resuming onto the row
            // it no longer owns: it did not compress it, did not upload it, and
            // did not label it. Reading the freed row's text(1) is what
            // compressRide/writeFile (:2159/:2161) do, and it is the READ in
            // PROBE-A's report.
            QVERIFY2(out.writeFileCalls == 0,
                     qPrintable(where + QStringLiteral("the loop resumed onto the freed row and uploaded it: "
                                                       "store->writeFile was called %1 time(s)")
                                            .arg(out.writeFileCalls)));
        }

        // ---- (a) THE SAME, WITH `aborted` SET FIRST. Two clicks in one burst:
        //      Abort, then Refresh. Without the guard the resumed frame takes the
        //      REQ-027 abort branch and WRITES "Aborted" into the freed row
        //      (:2153) - so here the ASan report is the whole of the verdict, and
        //      writeFileCalls would be 0 either way. Said plainly because an
        //      assertion that cannot fail must not be read as coverage.
        {
            RebuildSpec spec;
            spec.where = InSyncNextOpen;
            spec.abortFirst = true;
            const RebuildOutcome out = runListRebuild(spec);
            const QString where = QStringLiteral("syncNext (abort first): ");
            assertRebuildPremises(where, out);
            if (QTest::currentTestFailed())
                return;
            QVERIFY2(out.sawAbortLabel,
                     qPrintable(where + QStringLiteral("the button was not labelled \"Abort\" while the batch ran, so "
                                                       "downloadClicked() could not have been the abort control")));
            QVERIFY2(out.abortTookTheAbortBranch,
                     qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch "
                                                       "(CloudService.cpp:1920-1928), so `aborted` was never set")));
            QVERIFY2(out.writeFileCalls == 0,
                     qPrintable(where + QStringLiteral("store->writeFile was called %1 time(s) after an abort")
                                            .arg(out.writeFileCalls)));
        }

        // ---- (a) THE UPLOAD TAB'S TWIN. uploadNext (:2553) captures `curr`
        //      before the same openRideFile and uses it after, so it carries the
        //      identical defect in a second function; without this run the guard
        //      there is an UNCOVERED change.
        {
            RebuildSpec spec;
            spec.where = InUploadNextOpen;
            const RebuildOutcome out = runListRebuild(spec);
            const QString where = QStringLiteral("uploadNext: ");
            assertRebuildPremises(where, out);
            if (QTest::currentTestFailed())
                return;
            QVERIFY2(out.rideOpens >= 1, qPrintable(where + QStringLiteral("row[0]'s ride file was never opened")));
            QVERIFY2(out.writeFileCalls == 0,
                     qPrintable(where + QStringLiteral("the loop resumed onto the freed row and uploaded it: "
                                                       "store->writeFile was called %1 time(s)")
                                            .arg(out.writeFileCalls)));
        }
    }

    // -- TEST-129 extension (REQ-028, A3-R028c-F2) ----------------------
    // The driver-side twin of the completion-tail rows: openRideFile admits one
    // batch generation, then its nested loop aborts and immediately restarts.
    // The restarted frame owns the one parked write. The old frame must compare
    // its admitted generation before dispatching, or both frames upload row[0].
    void aRestartInsideAParseableOpenMustStandTheOldDriverDown_data()
    {
        QTest::addColumn<int>("whereValue");
        QTest::addColumn<QString>("driver");
        QTest::newRow("syncNext") << int(InSyncNextOpen) << QStringLiteral("syncNext");
        QTest::newRow("uploadNext") << int(InUploadNextOpen) << QStringLiteral("uploadNext");
    }

    void aRestartInsideAParseableOpenMustStandTheOldDriverDown()
    {
        QFETCH(int, whereValue);
        QFETCH(QString, driver);
        RebuildSpec spec;
        spec.where = RefreshWhere(whereValue);
        spec.restartInsteadOfRefresh = true;
        const RebuildOutcome out = runListRebuild(spec);
        const QString where = driver + QStringLiteral(": ");
        QVERIFY2(!out.timedOut, qPrintable(where + QStringLiteral("the run never came back")));
        QCOMPARE(out.listCount, 2);
        QCOMPARE(out.checkedRows, 2);
        QVERIFY2(out.rideOpens >= 2,
                 qPrintable(where + QStringLiteral("the old and restarted frames did not both reach openRideFile")));
        QVERIFY2(out.sawAbortLabel && out.abortTookTheAbortBranch && out.restartDelivered,
                 qPrintable(where + QStringLiteral("the nested loop did not deliver Abort -> immediate restart")));
        QVERIFY2(out.writeFileCalls == 1,
                 qPrintable(where + QStringLiteral("the stale driver dispatched alongside the restarted one: "
                                                   "%1 writes for one restarted row")
                                        .arg(out.writeFileCalls)));
    }

    // -- TEST-109 (REQ-028 (b), DEC-garmin-034) --------------------------
    // A DRIVER WHOSE LIST WAS REPLACED DOES NOT REPORT THE BATCH IT WAS RUNNING.
    //
    // After the same delivery, no completion tail is produced for the destroyed
    // batch: progressLabel must not read "Processed 0 of 2 successfully", and
    // context->athlete->rideCache->save() must be called ZERO times on that
    // frame's behalf.
    //
    // HOW THE SECOND HALF IS ASSERTED, AND WHAT THAT IS WORTH. RideCache::save is
    // a no-op stub in this target (stubs/ImportSeamStubs.cpp:394) and is NOT
    // virtual (RideCache.h:161), so it can be neither counted nor overridden from
    // here; counting it would mean instrumenting a stub file shared by three
    // targets. It is asserted through its ONLY two call sites instead:
    // CloudService.cpp calls it at exactly two places, syncNext's completion tail
    // and downloadNext's, both UNCONDITIONAL and both one line after that tail
    // writes its "...successfully" sentence (grep-verified: two hits in the file).
    // So "no tail ran" and "save was not called by this frame" are the same
    // statement here, and the tail is what is measured. Recorded as a PROXY,
    // labelled as one (B-R027-05's discipline).
    //
    // The second run is not a spelling of the first: it is the OTHER frame that
    // reports a batch, completedRead's tail (:2588), which re-drives the driver
    // over the rebuilt list. Every rebuilt row is UNCHECKED, so the driver finds
    // nothing to do and falls straight through to the completion tail - which is
    // exactly how a Refresh mid-batch ends up announcing a batch that no longer
    // exists (A3-R027-F3, PROBE-B).
    //
    // RED, before the fix:
    //   syncNext run: the process aborts first, on TEST-108's use-after-free.
    //   completedRead run:
    //     FAIL!  : ... completedRead: the destroyed batch still reported itself:
    //              progressLabel reads "Processed 1 of 2 successfully"
    void aDriverWhoseListWasReplacedMustNotReportItsBatch()
    {
        struct Run
        {
            RefreshWhere where;
            const char* what;
        };
        const Run runs[] = {{InSyncNextOpen, "syncNext"}, {InUncompressOpen, "completedRead"}};

        for (const Run& run : runs) {
            RebuildSpec spec;
            spec.where = run.where;
            const RebuildOutcome out = runListRebuild(spec);
            const QString where = QStringLiteral("%1: ").arg(run.what);
            assertRebuildPremises(where, out);
            if (QTest::currentTestFailed())
                return;

            // ---- (b) THE CRITERION'S OWN ASSERTION. The tail's sentence is the
            //      one string only the tail writes, and progressLabelText() reads
            //      back empty unless it is there. Asserted EMPTY rather than
            //      merely "not that string": the dead batch's `successful` is not
            //      always 0 - on the completedRead run the stale slot has already
            //      counted a save - so pinning only the literal wording would let
            //      "Processed 1 of 2 successfully" through, which is the same
            //      defect with a different number in it.
            QVERIFY2(
                out.progressText.isEmpty(),
                qPrintable(where + QStringLiteral("the destroyed batch still reported itself: progressLabel reads "
                                                  "\"%1\" (the criterion names \"Processed 0 of 2 successfully\"; any "
                                                  "tail sentence at all is one batch too many, and one line below it "
                                                  "the tail calls rideCache->save())")
                                       .arg(out.progressText)));

            // ...and the tail's other side effect, which cannot be produced any
            // other way while a batch is running: tabChanged (:1825) returns early
            // while the button reads "Abort", so only the completion tail can have
            // relabelled it.
            QVERIFY2(
                out.buttonTextAtEnd == QStringLiteral("Abort"),
                qPrintable(where + QStringLiteral("the completion tail ran for the destroyed batch: the button reads "
                                                  "\"%1\" rather than \"Abort\"")
                                       .arg(out.buttonTextAtEnd)));

            // ...and no row of the list that REPLACED them was labelled by a
            // transfer that was never about it.
            QVERIFY2(
                out.labelledRows == 0,
                qPrintable(where + QStringLiteral("%1 row(s) of the REBUILT list carry a status the batch that owned "
                                                  "them never gave them: [%2]")
                                       .arg(out.labelledRows)
                                       .arg(out.statuses.join(QStringLiteral("|")))));
        }
    }

    // -- TEST-111 (REQ-028 (c), DEC-garmin-034) --------------------------
    // NO ROW IS LABELLED THAT THE FRAME DID NOT TRANSFER.
    //
    // WHAT THIS SLOT COVERS, AND WHAT IT DOES NOT — stated first, because the
    // criterion names a scenario this slot does NOT drive.
    //
    // Criterion (c) reads: "Across a batch that is aborted and immediately
    // restarted with the aborted batch's completion arriving LATE, every non-empty
    // status cell names the outcome of a transfer that actually happened to that
    // row, and the reader is invoked exactly once per checked row."
    //
    // THAT SCENARIO IS NOT DRIVEN HERE - and, AMENDED 2026-08-18, it is no longer
    // OPEN either. What this paragraph used to say was: DEC-034 cannot close it,
    // because on the abort+restart route TWO transfers are outstanding at once -
    // the aborted batch's, still in flight, and the restarted batch's, dispatched
    // before the first one completes - while the completion carries no identity of
    // its own (completedWrite carries no payload at all; completedRead's `name` is
    // documented as possibly not the name that was asked for). A single shared
    // snapshot, of listGeneration or of batchGeneration, is overwritten by the
    // restarted batch's own dispatch before the stale completion arrives, so it
    // reads EQUAL and no counter of that shape can tell the two apart. All of that
    // was true and remains the reason DEC-034 alone was not enough.
    //
    // DEC-garmin-036 supplied what it named as the requirement - per-transfer
    // identity - as an in-flight ticket armed at each dispatch and consumed at
    // each completion slot. That half of (c) is now CLOSED and is asserted by
    // TEST-113 (reads, both channels), TEST-115 (writes, including the same-row
    // case) and TEST-116 (the swallowed buffer), with TEST-114 as the positive
    // control. TEST-107 no longer measures it: its P2 half moved into TEST-113
    // when the numbers stopped being a defect and became a rule.
    //
    // WHAT IS DRIVEN HERE is the OTHER way a completion slot labels a row it never
    // transferred, which is DEC-034's alone to close: the user ABORTS and then hits
    // REFRESH - the obvious pair of clicks, in the obvious order - while a transfer
    // is still in flight. The list is rebuilt underneath the slot. Before DEC-036
    // that cost a MISLABEL through `child(listindex-1)` or a null dereference when
    // the rebuild came back shorter; since DEC-036 the slot holds a POINTER to an
    // item the rebuild has FREED, so it would cost a use-after-free instead. The
    // guard is the same guard and this run is still the only thing that kills it -
    // what it prevents just got worse. All THREE completion slots carry that entry
    // branch, so all three are run.
    //
    // THE HAZARD THIS FIXTURE BUYS AND THE ONE IT GIVES UP (B-R027-07). The rows
    // are .gcblock, not .gcfail: the reader runs a nested QEventLoop, which is
    // what makes a Refresh deliverable from inside a parse at all and what lets
    // the download half reach saveRide with a real RideFile. The price is that
    // this fixture cannot claim anything about event ORDERING inside that loop -
    // a .gcfail row pumps no events and would be the safer choice for a timing
    // claim, and TEST-107 uses it for exactly that reason. No assertion here rests
    // on when something was delivered relative to something else; each rests on
    // the end state of a list.
    //
    // RED, before the fix:
    //   FAIL!  : ... completedRead: a completion labelled 1 row(s) of a list that
    //            was rebuilt under it: [Aborted|]
    void noRowIsLabelledByACompletionThatWasNeverAboutIt()
    {
        struct Run
        {
            RefreshWhere where;
            bool failTheRead;
            const char* what;
        };
        const Run runs[] = {{InReadFileLoop, false, "completedRead"},
                            {InReadFileLoop, true, "failedRead"},
                            {InWriteFileLoop, false, "completedWrite"}};

        for (const Run& run : runs) {
            RebuildSpec spec;
            spec.where = run.where;
            spec.failTheRead = run.failTheRead;
            spec.abortFirst = true; // abort, THEN refresh, both mid-transfer
            const RebuildOutcome out = runListRebuild(spec);
            const QString where = QStringLiteral("%1: ").arg(run.what);
            assertRebuildPremises(where, out);
            if (QTest::currentTestFailed())
                return;
            QVERIFY2(out.sawAbortLabel,
                     qPrintable(where + QStringLiteral("the button was not labelled \"Abort\" while the transfer was "
                                                       "in flight, so downloadClicked() could not have been the abort "
                                                       "control")));
            QVERIFY2(out.abortTookTheAbortBranch,
                     qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch "
                                                       "(CloudService.cpp:1920-1928), so the slot below would not "
                                                       "have taken its `aborted` entry branch at all")));

            // ---- THE POINT. The rebuilt rows belong to nothing: no transfer has
            //      happened to any of them, so ANY status text on any of them is a
            //      verdict about a row the frame never transferred.
            QVERIFY2(
                out.labelledRows == 0,
                qPrintable(where + QStringLiteral("a completion labelled %1 row(s) of a list that was rebuilt under "
                                                  "it: [%2]")
                                       .arg(out.labelledRows)
                                       .arg(out.statuses.join(QStringLiteral("|")))));
        }
    }

    // -- TEST-121 (A3-R028b-F1, REQ-028 (a), DEC-garmin-034) -------------
    // NO ROW IS TOUCHED AFTER IT IS FREED — INCLUDING ACROSS saveRide.
    //
    // THE WINDOW, AND WHY IT IS NOT ONE OF THE OTHERS. completedRead proves the
    // row is still live at CloudService.cpp:2820 and then calls saveRide at
    // :2910 - and saveRide SUSPENDS. Its second statement of substance is
    // DataProcessorFactory::autoProcess(ride, "Auto", "Import") (:3396), which
    // runs every processor whose configKeyAutomation is "Auto"
    // (DataProcessor.cpp:220-221); FixElevation's postProcess then posts to
    // api.open-elevation.com and waits in a local QEventLoop with NO TIMEOUT
    // (FixElevation.cpp:288-300), and FixPyDataProcessor takes a second route
    // into a nested loop through FixPyRunner (FixPyDataProcessor.cpp:39 ->
    // FixPyRunner.cpp:40-48). A Refresh delivered there FREES `row`, and the very
    // next thing completedRead did was `row->setText(col, ...)` - THREE WRITES
    // ABOVE the first lifetime re-check, which used to be the self.isNull()
    // below the tail's processEvents(). Post-fix those writes are at
    // :2921/:2924, below the two bails this wave added at :2917/:2918.
    //
    // "Auto" is not the default ("Manual" is), so this is a SUPPORTED
    // CONFIGURATION rather than the out-of-the-box one. Criterion (a) - "no row
    // is touched after it is freed" - admits no configuration exemption, so the
    // window counts.
    //
    // WHY THE SUITE WAS GREEN WITHOUT THIS SLOT, said plainly. This target stubs
    // the entire downstream of saveRide flat: autoProcess, setLinkedDefaults,
    // RideCache::save and Athlete::addRide are all no-ops in
    // stubs/ImportSeamStubs.cpp. The .gcblock fixture DID drive the ride-bearing
    // branch, so the coverage was real - it covered a saveRide with the
    // suspension surgically removed. This slot puts the suspension back, at the
    // one seam it can be put back at, and nowhere else (LSN-056: that stub file is
    // compiled into testGarminConnectImport, testGarminConnectReadFailedConsumer
    // and this target; the seam is an empty std::function unless armed here, and
    // all three targets are built and run).
    //
    // WHAT ASSERTS THE CRITERION. Not a QVERIFY: this target is built with
    // -fsanitize=address and runs with halt_on_error=1, so the criterion's own
    // measurement is the PROCESS - a report here aborts the binary and no later
    // slot runs at all. The QVERIFYs below establish that the run reached the
    // window and record what the guard did instead.
    //
    // RED, before the fix:
    //   ==NNNN==ERROR: AddressSanitizer: heap-use-after-free WRITE of size 8
    //     #0 QTreeWidgetItem::setText(int, QString const&)
    //     #1 CloudServiceSyncDialog::completedRead() CloudService.cpp:2812
    //     (pre-fix numbering; measured verbatim under BOTH QPA backends)
    //    freed by thread T0 here: ... CloudServiceSyncDialog::refreshClicked()
    void aRefreshInsideSaveRideMustNotLeaveTheSlotHoldingThatRow()
    {
        RebuildSpec spec;
        spec.where = InSaveRideAutoProcess;
        const RebuildOutcome out = runListRebuild(spec);
        const QString where = QStringLiteral("completedRead/saveRide: ");
        assertRebuildPremises(where, out);
        if (QTest::currentTestFailed())
            return;

        // ---- THE PREMISE THAT MAKES THIS SLOT ABOUT saveRide AT ALL. Without
        //      this the run could have refreshed from anywhere and proved nothing
        //      about :2811. The seam is only reached from inside saveRide, and
        //      saveRide is only reached from completedRead's ride-bearing branch.
        QVERIFY2(
            out.autoProcessCalls >= 1,
            qPrintable(where + QStringLiteral("autoProcess was called %1 time(s) - saveRide never got past its "
                                              "\"File exists\" refusal (CloudService.cpp:3386-3390) or the "
                                              "ride-bearing branch at :2839 was never taken, so the Refresh was not "
                                              "delivered inside saveRide and this run proves nothing")
                                   .arg(out.autoProcessCalls)));
        QVERIFY2(out.rideOpens >= 1,
                 qPrintable(where + QStringLiteral("uncompressRide never parsed a ride, so :2811 was never reached")));

        // ---- THE VERDICT the guard leaves behind. The row the slot was about is
        //      gone; no row of the list that REPLACED it wears a status, and the
        //      slot's tail did not re-drive the loop into the completion sentence
        //      (one line below which, in syncNext's tail, is rideCache->save()).
        QVERIFY2(
            out.labelledRows == 0,
            qPrintable(where + QStringLiteral("%1 row(s) of the REBUILT list carry a status the batch that owned them "
                                              "never gave them: [%2]")
                                   .arg(out.labelledRows)
                                   .arg(out.statuses.join(QStringLiteral("|")))));
        QVERIFY2(
            out.progressText.isEmpty(),
            qPrintable(where + QStringLiteral("the destroyed batch still reported itself: progressLabel reads \"%1\"")
                                   .arg(out.progressText)));
        QVERIFY2(out.buttonTextAtEnd == QStringLiteral("Abort"),
                 qPrintable(where + QStringLiteral("the completion tail ran for the destroyed batch: the button reads "
                                                   "\"%1\" rather than \"Abort\"")
                                        .arg(out.buttonTextAtEnd)));
    }

    // -- TEST-110 (REQ-028 (d), DEC-garmin-034) --------------------------
    // THE DIALOG REMAINS USABLE — THE POSITIVE CONTROL.
    //
    // A Refresh performed while NO batch is running must repopulate all three
    // lists normally, and a subsequent batch must reach its completion tail.
    // Without this clause every criterion above is satisfiable by refusing to do
    // anything at all (LSN-050): a guard that stood every driver down
    // unconditionally, or a refreshClicked that returned at the top, would leave
    // TEST-108/109/111 green.
    //
    // It is also the ONE run that proves the counter is compared rather than
    // merely bumped in the right direction: the idle Refresh bumps
    // listGeneration, downloadClicked snapshots it AFTER that, and the batch then
    // runs to its tail through all four guarded frames - syncNext, the two
    // completion slots the sync tab uses, and the tail itself.
    void anIdleRefreshLeavesTheDialogFullyUsable()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        bool timedOut = false;
        int downRows = 0, upRows = 0, syncRows = 0;
        int checkedRows = 0, writeFileCalls = 0, readFileCalls = 0;
        QString progressText, buttonTextAtEnd;
        int progressValue = -1, progressMax = -1, stillCheckedAtEnd = -1;
        int labelledRows = 0;
        QStringList statuses;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // Both halves populated: two REMOTE activities (the download list,
                // and sync "Download" rows) and two LOCAL ones (the upload list,
                // and sync "Upload" rows), so "all three lists" is a real claim.
                store->entryNames = QStringList() << rebuildRemoteActivity(0) << rebuildRemoteActivity(1);
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // ---- THE IDLE REFRESH. No batch is running: `downloading` is
                //      false and nothing is suspended anywhere.
                dialog->refreshClicked();

                QTreeWidget* down = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                QTreeWidget* up = rideListWithHeader(dialog, QStringLiteral("File"));
                QTreeWidget* sync = rideListWithHeader(dialog, QStringLiteral("Source"));
                if (down != nullptr)
                    downRows = down->invisibleRootItem()->childCount();
                if (up != nullptr)
                    upRows = up->invisibleRootItem()->childCount();
                if (sync != nullptr)
                    syncRows = sync->invisibleRootItem()->childCount();

                // ---- ...AND A BATCH BEHIND IT, on the rebuilt rows.
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                if (sync != nullptr)
                    for (int i = 0; i < sync->invisibleRootItem()->childCount(); i++) {
                        QCheckBox* check =
                            qobject_cast<QCheckBox*>(sync->itemWidget(sync->invisibleRootItem()->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            checkedRows++;
                    }

                QPushButton* button = pushButtonWithText(dialog, QStringLiteral("Synchronize"));

                dialog->downloadClicked();

                // The batch drives itself to completion through queued
                // completions, so let them all be delivered.
                for (int i = 0; i < 200 && (progressText.isEmpty()); ++i) {
                    QApplication::processEvents(QEventLoop::AllEvents, 5);
                    progressText = progressLabelText(dialog);
                }

                writeFileCalls = obs::writeFileCalls;
                readFileCalls = obs::readFileCalls;
                if (sync != nullptr) {
                    QTreeWidgetItem* root = sync->invisibleRootItem();
                    stillCheckedAtEnd = 0;
                    for (int i = 0; i < root->childCount(); i++) {
                        const QString status = root->child(i)->text(7);
                        statuses << status;
                        if (!status.isEmpty())
                            labelledRows++;
                        QCheckBox* check = qobject_cast<QCheckBox*>(sync->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            stillCheckedAtEnd++;
                    }
                }
                if (QProgressBar* bar = dialog->findChild<QProgressBar*>()) {
                    progressValue = bar->value();
                    progressMax = bar->maximum();
                }
                if (button != nullptr)
                    buttonTextAtEnd = button->text();

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
                QTimer::singleShot(20000, &appLoop, [&timedOut]() {
                    timedOut = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        QVERIFY2(timedOut == false, "the run never came back");

        // ---- (d) ALL THREE LISTS REPOPULATED. Two remote, two local, and the
        //      sync list is the union - the shape refreshClicked builds at
        //      :1691 (Download rows) and :1783 (Upload rows).
        QVERIFY2(downRows == 2, qPrintable(QStringLiteral("the download list holds %1 rows after an idle Refresh, "
                                                          "not 2")
                                               .arg(downRows)));
        QVERIFY2(upRows == 2,
                 qPrintable(QStringLiteral("the upload list holds %1 rows after an idle Refresh, not 2").arg(upRows)));
        QVERIFY2(syncRows == 4,
                 qPrintable(QStringLiteral("the sync list holds %1 rows after an idle Refresh, not 4").arg(syncRows)));
        QCOMPARE(checkedRows, 4);

        // ---- (d) ...AND THE BATCH BEHIND IT REACHES ITS COMPLETION TAIL. Every
        //      row was transferred (two downloads, two uploads), every row is
        //      labelled, the bar is at maximum and the tail relabelled the button
        //      and cleared every checkbox.
        QCOMPARE(readFileCalls, 2);
        QCOMPARE(writeFileCalls, 2);
        QVERIFY2(labelledRows == 4, qPrintable(QStringLiteral("%1 of 4 rows were labelled: [%2]")
                                                   .arg(labelledRows)
                                                   .arg(statuses.join(QStringLiteral("|")))));
        QVERIFY2(progressText == QStringLiteral("Processed 4 of 4 successfully"),
                 qPrintable(QStringLiteral("the batch after an idle Refresh did not reach its completion tail: "
                                           "progressLabel reads \"%1\"")
                                .arg(progressText)));
        QCOMPARE(buttonTextAtEnd, QStringLiteral("Synchronize"));
        QCOMPARE(stillCheckedAtEnd, 0);
        QCOMPARE(progressValue, 4);
        QCOMPARE(progressMax, 4);
    }

  private:
    // =====================================================================
    // TEST-119 / TEST-120 (A3-R028-F5, REQ-028 (b), DEC-garmin-034) —
    // THE REFRESH DELIVERED BY THE COMPLETION TAIL'S OWN processEvents().
    // =====================================================================
    //
    // THE WINDOW, AND WHY IT IS NOT ANY OF THE FIVE ABOVE. Each of the three
    // completion slots ends the same way:
    //
    //     progressBar->setValue(++downloadcounter);   // ...the row is labelled
    //     QApplication::processEvents();              // <-- THE DELIVERY
    //     if (self.isNull()) return;
    //     if (aborted == true) return;
    //     if (sync) syncNext(); else downloadNext();  // <-- THE RE-DRIVE
    //
    // Every guard REQ-028 installed sits UPSTREAM of that processEvents(): the
    // three DEC-034 compares are at the slots' entries (and, in completedRead,
    // after uncompressRide), and the DEC-036 ticket is CONSUMED - `inflight.armed
    // = false` - above them too, so the amendment's invalidation is already a
    // no-op by the time the tail runs. NOTHING re-reads listGeneration between the
    // delivery and the re-drive. This is therefore a SIXTH delivery point, not a
    // sixth spelling of the five in runListRebuild: those all land while a frame
    // is SUSPENDED inside a nested loop it entered; this one lands in the frame's
    // own last statement, after every guard it has.
    //
    // WHAT THE RE-DRIVE THEN DOES. downloadNext carries no listGeneration guard of
    // any kind; syncNext and uploadNext snapshot `listGeneration` into a local at
    // LOOP ENTRY, which on this route is AFTER the Refresh, so the snapshot reads
    // the NEW generation and every compare below it is equal. All three therefore
    // walk the REBUILT list, whose rows are all unchecked (refreshClicked builds
    // each row's QCheckBox fresh and never checks it), find nothing to do, and
    // fall straight through to the completion tail - which writes the
    // "...successfully" sentence and calls context->athlete->rideCache->save() for
    // a batch that no longer exists. That is A3-R027-F3 / PROBE-B's harm reached
    // by a different route.
    //
    // HOW THE DELIVERY IS MADE, AND WHY IT IS NOT A TIMING BET. insideframe (see
    // its block comment) is used exactly as TEST-102 uses it: stage 1 is
    // production's own `progressBar->setValue(++downloadcounter)`, a direct
    // connection, which anchors us INSIDE the completion slot and strictly below
    // its last DEC-034 compare; stage 2 is the dispatcher's awake(), emitted as
    // the first statement of the very next processEvents() on this thread. Between
    // those two points production runs a QTreeWidgetItem::setText and (on
    // completedWrite) a `successful++` and nothing that pumps events, so "the next
    // processEvents()" is the tail's own. The two premises below MEASURE that
    // rather than assert it: the bar has already counted row 0 when the Refresh
    // runs (so we are past the compares) and no second transfer has been issued
    // yet (so we are above the re-drive).
    //
    // HOW rideCache->save() IS ASSERTED - A PROXY, LABELLED AS ONE, and it is
    // TEST-109's proxy rather than a second apparatus. RideCache::save is a no-op
    // stub in this target (stubs/ImportSeamStubs.cpp:394) and is NOT virtual, so
    // it can be neither counted nor overridden from here without instrumenting a
    // stub file shared by three targets (LSN-056). CloudService.cpp calls it at
    // exactly two places - syncNext's completion tail and downloadNext's - both
    // unconditional and both one line after that tail writes its "...successfully"
    // sentence. So "no tail ran" and "save was not called on this frame's behalf"
    // are the same statement here, and the sentence is what is measured.
    // uploadNext's tail calls no save at all, which is why the upload arm's
    // verdict is the tail SENTENCE and the button, not save.
    enum TailRoute {
        TailDownloadTab, // completedRead's tail -> downloadNext()  (no guard at all)
        TailSyncTab,     // completedRead's tail -> syncNext()      (post-Refresh snapshot)
        TailUploadTab    // completedWrite's tail -> uploadNext()   (post-Refresh snapshot)
    };

    struct TailRefreshOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the window it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        bool refreshDelivered = false;
        int transfersAtRefresh = -1; // readFile/writeFile calls when the Refresh ran
        int barValueAtRefresh = -1;  // ...and what the bar had counted by then
        int rowsBeforeRefresh = -1;
        int rowsAfterRefresh = -1;
        bool rowsWereFreed = false;

        // -- the verdict
        int transfersAtEnd = 0;
        QString progressText; // the completion tail's "...successfully" sentence
        QString buttonTextAtEnd;
        int labelledRows = 0;
        QStringList statuses;
    };

    TailRefreshOutcome runRefreshInCompletionTail(TailRoute route)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        TailRefreshOutcome out;
        QEventLoop appLoop;

        // The download and sync arms run on REMOTE rows that cannot be parsed
        // (.gcfail, TEST-102's reason: FailingRideFileReader runs no nested loop of
        // its own, so nothing between setValue and the tail's processEvents pumps
        // events). The upload arm runs on LOCAL rides that CAN be parsed, because
        // uploadNext has to reach writeFile for completedWrite to exist at all.
        const bool localRides = (route == TailUploadTab);

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        if (localRides) {
            for (int i = 0; i < 2; i++) {
                const QString name = rebuildLocalActivity(i);
                QFile f(activities.absolutePath() + "/" + name);
                f.open(QIODevice::WriteOnly);
                f.write("gcblock");
                f.close();
                paths << f.fileName();

                RideItem* item = new RideItem(nullptr, context);
                item->fileName = name;
                item->path = activities.absolutePath();
                item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
                item->planned = false;
                items << item;
            }
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = localRides ? QStringList() : threeUnparseableRemoteActivities();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102):
                // uncompressRide's first guard rejects outright on the default.
                store->downloadCompression = CloudService::none;
                store->completeRead = true;
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                const int tabIndex = (route == TailDownloadTab) ? 0 : (route == TailUploadTab ? 1 : 2);
                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(tabIndex);

                QTreeWidget* list = nullptr;
                int statusColumn = 5;
                QString buttonLabel;
                switch (route) {
                case TailDownloadTab:
                    dialog->selectAllChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                    statusColumn = 5; // the download list's Status header (:1104-1106)
                    buttonLabel = QStringLiteral("Download");
                    break;
                case TailUploadTab:
                    dialog->selectAllUpChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("File"));
                    statusColumn = 7;
                    buttonLabel = QStringLiteral("Upload");
                    break;
                case TailSyncTab:
                    dialog->selectAllSyncChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Source"));
                    statusColumn = 7; // the sync list's Status header (:1172)
                    buttonLabel = QStringLiteral("Synchronize");
                    break;
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                }

                QPushButton* button = pushButtonWithText(dialog, buttonLabel);

                // THE USER'S REFRESH, delivered INSIDE the completion tail's own
                // QApplication::processEvents(). Nothing is faked: this calls the
                // dialog's own refreshClicked() slot, exactly as the button's
                // connect (:1225) would, and records the row-0 item POINTER either
                // side of it as an integer without ever dereferencing it.
                const auto refresh = [&, dialog, list, route]() {
                    out.refreshDelivered = true;
                    out.transfersAtRefresh = (route == TailUploadTab) ? obs::writeFileCalls : obs::readFileCalls;
                    if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
                        out.barValueAtRefresh = bar->value();

                    quintptr row0Before = 0;
                    if (list != nullptr) {
                        out.rowsBeforeRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsBeforeRefresh > 0)
                            row0Before = reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0));
                    }

                    dialog->refreshClicked(); // :1516 — every row deleted, then rebuilt

                    if (list != nullptr) {
                        out.rowsAfterRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsAfterRefresh > 0)
                            out.rowsWereFreed =
                                (reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0)) != row0Before);
                    }
                };

                // Armed at the FIRST dispatch, fired inside the completion slot
                // that dispatch produces. See the block comment above.
                store->afterCompletionAction = refresh;

                dialog->downloadClicked();

                // The batch drives itself through queued completions. Pump until
                // the Refresh has been delivered, then let whatever the tail does
                // next run to a standstill.
                for (int i = 0; i < 300 && !out.refreshDelivered; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);
                for (int i = 0; i < 60; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);

                out.transfersAtEnd = (route == TailUploadTab) ? obs::writeFileCalls : obs::readFileCalls;
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        const QString status = root->child(i)->text(statusColumn);
                        out.statuses << status;
                        if (!status.isEmpty())
                            out.labelledRows++;
                    }
                }
                out.progressText = progressLabelText(dialog);
                if (button != nullptr)
                    out.buttonTextAtEnd = button->text();

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // The shared premises for one arm of the two slots below. A run that never
    // reached the window must fail LOUDLY rather than pass on nothing (LSN-047,
    // LSN-050) - and, uniquely for this fixture, it must prove WHERE in the
    // completion slot the Refresh landed.
    void assertTailRefreshPremises(const QString& where, const TailRefreshOutcome& out, int expectedRows)
    {
        QVERIFY2(out.timedOut == false,
                 qPrintable(where + QStringLiteral("the run never came back - a guard wedged it")));
        QVERIFY2(
            out.listCount == expectedRows,
            qPrintable(where + QStringLiteral("the list held %1 rows, not %2").arg(out.listCount).arg(expectedRows)));
        QVERIFY2(
            out.checkedRows == expectedRows,
            qPrintable(where + QStringLiteral("%1 rows were checked, not %2").arg(out.checkedRows).arg(expectedRows)));
        QVERIFY2(out.refreshDelivered,
                 qPrintable(where + QStringLiteral("the Refresh was never delivered into the completion tail - this "
                                                   "run proves nothing")));

        // WHERE it landed, measured. Below the slot's last DEC-034 compare...
        QVERIFY2(
            out.barValueAtRefresh == 1,
            qPrintable(where + QStringLiteral("the bar read %1 when the Refresh ran, not 1 - the Refresh was not "
                                              "inside the completion slot that had just counted row 0, so this run is "
                                              "about some other window")
                                   .arg(out.barValueAtRefresh)));
        // ...and above the re-drive, which is the only thing that can issue a
        // second transfer.
        QVERIFY2(
            out.transfersAtRefresh == 1,
            qPrintable(where + QStringLiteral("%1 transfer(s) had been issued when the Refresh ran, not 1 - the loop "
                                              "had already been re-driven, so the Refresh was not inside the tail's "
                                              "processEvents()")
                                   .arg(out.transfersAtRefresh)));

        QVERIFY2(out.rowsBeforeRefresh == expectedRows,
                 qPrintable(where + QStringLiteral("the list held %1 rows when the Refresh arrived, not %2")
                                        .arg(out.rowsBeforeRefresh)
                                        .arg(expectedRows)));
        QVERIFY2(out.rowsAfterRefresh == expectedRows,
                 qPrintable(where + QStringLiteral("the Refresh rebuilt %1 rows, not %2 - the run would then be about "
                                                   "an empty list rather than a REPLACED one")
                                        .arg(out.rowsAfterRefresh)
                                        .arg(expectedRows)));
        QVERIFY2(out.rowsWereFreed,
                 qPrintable(where + QStringLiteral("row 0 is the SAME QTreeWidgetItem after the Refresh as before it, "
                                                   "so nothing was freed and this run proves nothing")));
    }

    // The shared verdict for TEST-119 and TEST-120. NOT a slot.
    void assertNoTailForTheDestroyedBatch(const QString& where, const TailRefreshOutcome& out)
    {
        // ---- (b) THE CRITERION'S OWN ASSERTION. The tail's sentence is the one
        //      string only the tail writes, and progressLabelText() reads back
        //      empty unless it is there. Asserted EMPTY rather than merely "not
        //      that string" for TEST-109's reason: pinning one wording would let
        //      the same defect through with a different number in it. One line
        //      below that sentence, on two of these three routes, is
        //      context->athlete->rideCache->save().
        QVERIFY2(
            out.progressText.isEmpty(),
            qPrintable(where + QStringLiteral("the destroyed batch still reported itself: progressLabel reads \"%1\" "
                                              "(the criterion names \"Processed 0 of 2 successfully\"; any tail "
                                              "sentence at all is one batch too many, and in syncNext's tail and "
                                              "downloadNext's - the file's only two - the line below that sentence is "
                                              "rideCache->save())")
                                   .arg(out.progressText)));

        // ...and the tail's other side effect, which cannot be produced any other
        // way while a batch is running: tabChanged (:1856) returns early while the
        // button reads "Abort", so only the completion tail can have relabelled it.
        QVERIFY2(out.buttonTextAtEnd == QStringLiteral("Abort"),
                 qPrintable(where + QStringLiteral("the completion tail ran for the destroyed batch: the button reads "
                                                   "\"%1\" rather than \"Abort\"")
                                        .arg(out.buttonTextAtEnd)));

        // ...and no row of the list that REPLACED them was labelled, and no
        // further transfer was issued against it.
        QVERIFY2(
            out.labelledRows == 0,
            qPrintable(where + QStringLiteral("%1 row(s) of the REBUILT list carry a status the batch that owned them "
                                              "never gave them: [%2]")
                                   .arg(out.labelledRows)
                                   .arg(out.statuses.join(QStringLiteral("|")))));
        QVERIFY2(out.transfersAtEnd == 1,
                 qPrintable(where + QStringLiteral("%1 transfer(s) were issued in total, not 1 - the re-driven loop "
                                                   "dispatched against a list its batch never owned")
                                        .arg(out.transfersAtEnd)));
    }

  private slots:
    // -- TEST-119 (A3-R028-F5, REQ-028 (b)) ------------------------------
    // THE DOWNLOAD TAB: THE RE-DRIVE WITH NO GUARD AT ALL.
    //
    // A three-row Download batch. The Refresh is delivered inside completedRead's
    // tail processEvents() - after every guard that slot has - and the tail then
    // re-drives downloadNext(), which carries no listGeneration compare anywhere.
    //
    // After the same delivery, no completion tail is produced for the destroyed
    // batch: progressLabel must not read "Processed 0 of 2 successfully", and
    // context->athlete->rideCache->save() must be called zero times on that
    // frame's behalf (asserted through the proxy the block comment above names).
    //
    // RED, before the fix:
    //   FAIL!  : ... download tab: the destroyed batch still reported itself:
    //            progressLabel reads "Downloaded 0 of 3 successfully"
    void aRefreshInsideACompletionTailMustNotReDriveTheDownloadLoop()
    {
        const TailRefreshOutcome out = runRefreshInCompletionTail(TailDownloadTab);
        const QString where = QStringLiteral("download tab: ");
        assertTailRefreshPremises(where, out, 3);
        if (QTest::currentTestFailed())
            return;
        assertNoTailForTheDestroyedBatch(where, out);
    }

    // -- TEST-120 (A3-R028-F5, REQ-028 (b)) ------------------------------
    // THE SYNC TAB'S TWIN, AND THE UPLOAD TAB'S — ONE SLOT EACH.
    //
    // syncNext and uploadNext DO snapshot listGeneration - but at LOOP ENTRY, and
    // on this route loop entry is AFTER the Refresh, so the snapshot reads the new
    // generation and every compare below it is equal. The snapshot that exists is
    // therefore worth exactly as much as downloadNext's, which does not exist:
    // both walk the rebuilt list to their completion tails.
    //
    // Two production frames: completedRead's tail -> syncNext() on the sync tab,
    // and completedWrite's tail -> uploadNext() on the upload tab. The second is
    // not a spelling of the first - it is the third completion slot and the third
    // driver, reached through writeFile rather than readFile.
    //
    // TWO SLOTS, NOT TWO ARMS (A3-R028b-F5). These used to be one slot with the
    // upload arm second, behind `if (QTest::currentTestFailed()) return;`. That
    // early return is required INSIDE an arm - a premise that failed must not be
    // followed by verdicts read off a run that never happened - but between arms
    // it silently WITHDREW the third driver's coverage whenever the sync arm
    // failed, which is exactly the state a mutation run puts the suite in. A
    // mutation record taken in that shape cannot distinguish "the upload guard is
    // covered" from "the upload arm never ran". Split so that each driver's
    // coverage stands or falls on its own.
    //
    // RED, before the fix:
    //   FAIL!  : ... sync tab: the destroyed batch still reported itself:
    //            progressLabel reads "Processed 0 of 3 successfully"
    void aRefreshInsideACompletionTailMustNotReDriveTheSyncLoop()
    {
        const TailRefreshOutcome out = runRefreshInCompletionTail(TailSyncTab);
        const QString where = QStringLiteral("sync tab: ");
        assertTailRefreshPremises(where, out, 3);
        if (QTest::currentTestFailed())
            return;
        assertNoTailForTheDestroyedBatch(where, out);
    }

    // -- TEST-120, the upload half — see the block above for the window ---
    //
    // RED, before the fix:
    //   FAIL!  : ... upload tab: the destroyed batch still reported itself:
    //            progressLabel reads "Uploaded 0 of 2 successfully"
    void aRefreshInsideACompletionTailMustNotReDriveTheUploadLoop()
    {
        const TailRefreshOutcome out = runRefreshInCompletionTail(TailUploadTab);
        const QString where = QStringLiteral("upload tab: ");
        assertTailRefreshPremises(where, out, 2);
        if (QTest::currentTestFailed())
            return;
        assertNoTailForTheDestroyedBatch(where, out);
    }

    // -- TEST-112 (S-R028-01) — REWRITTEN 2026-08-23 UNDER DEC-garmin-038 -
    // THE SORT ROUTE: WHAT A COLUMN-HEADER CLICK DOES TO A RUNNING BATCH.
    //
    // WHAT THIS SLOT WAS, AND WHY IT CHANGED. It was a TRACE: DEC-034 validates
    // the CONTAINER, a SORT frees nothing and rebuilds nothing - it PERMUTES the
    // same items - so it bumped no counter and passed every guard REQ-028
    // installed, and this slot MEASURED what that cost rather than fixing it.
    // Its "MEASURED, NOT DESIRED" block pinned the damage: the delivered sort
    // moved the rows, the loop then transferred the SAME row twice and the other
    // never, and the slot said in as many words that whoever closed S-R028-01
    // should expect this block to change. DEC-garmin-038 closed it, so it has:
    // the pins below are now the RULE, and they are the desired values, not the
    // measured ones. What the old block measured is kept in prose at the foot of
    // this slot, because that measurement is the whole reason the fix exists.
    //
    // WHY THE ROUTE WAS OPEN, in the file as it stood: all three lists are built
    // setSortingEnabled(true) (:1116/:1146/:1186); downloadClicked disabled
    // sorting on rideListDown ONLY and explicitly RE-ENABLED rideListUp;
    // rideListSync was never disabled anywhere. Since DEC-038 downloadClicked
    // calls suspendListSorting(), which takes all three lists out of sorting for
    // the batch's duration and restores them - column and order intact - at every
    // termination path.
    //
    // THE DELIVERY IS A REAL CLICK, and the run RECORDS which mechanism moved the
    // rows - QTest::mouseClick on the header viewport, or the setSortIndicator
    // call that a click makes internally (QHeaderView::mouseReleaseEvent ->
    // setSortIndicator -> sortIndicatorChanged -> QTreeView::sortByColumn). Under
    // DEC-038 the answer must be NEITHER, and the slot samples the list's own
    // sorting flag inside the same frame so that the negative is attributed to the
    // guard rather than to a synthetic-delivery artefact (LSN-062/ORCH-017): a
    // `false` here means "the batch had sorting off", not "this backend does not
    // deliver clicks".
    void probeWhatAColumnSortDoesToARunningBatch()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 300;

        bool timedOut = false;
        bool sortingEnabledOnSyncList = false, sectionsClickable = false, sortIndicatorShown = false;
        bool clickDelivered = false, reorderedByRealClick = false, reorderedBySortIndicator = false;
        bool sameItemSetAfter = false;
        // DEC-garmin-038 — the same three properties, re-read INSIDE the running
        // batch. This is what separates "the guard shut the vector" from "this
        // backend does not deliver synthetic clicks".
        int sortingDuringBatch = -1, clickableDuringBatch = -1, indicatorShownDuringBatch = -1;
        QStringList namesBefore, namesAfter, statuses, namesAtFirstCompletion, statusesAtFirstCompletion;
        int writeFileCalls = 0, rideOpens = 0;
        QString labelledAtFirstCompletion, transferredRow, writeNameAtFirstCompletion;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("Source"));

                if (list != nullptr) {
                    sortingEnabledOnSyncList = list->isSortingEnabled();
                    if (QHeaderView* h = list->header()) {
                        sectionsClickable = h->sectionsClickable();
                        sortIndicatorShown = h->isSortIndicatorShown();
                    }
                }

                // THE USER'S CLICK ON THE "File" COLUMN HEADER, delivered from
                // inside row[0]'s openRideFile - the same nested loop TEST-108
                // delivers its Refresh into, and the same frame is suspended.
                rideopen::action = [&, list]() {
                    if (list == nullptr)
                        return;
                    sortingDuringBatch = int(list->isSortingEnabled());
                    if (QHeaderView* hv = list->header()) {
                        clickableDuringBatch = int(hv->sectionsClickable());
                        indicatorShownDuringBatch = int(hv->isSortIndicatorShown());
                    }
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    QSet<quintptr> before;
                    for (int i = 0; i < root->childCount(); i++) {
                        namesBefore << root->child(i)->text(1);
                        before.insert(reinterpret_cast<quintptr>(root->child(i)));
                    }

                    QHeaderView* h = list->header();
                    if (h != nullptr) {
                        const int section = 1;
                        const int x = h->sectionViewportPosition(section) + h->sectionSize(section) / 2;
                        QTest::mouseClick(h->viewport(), Qt::LeftButton, Qt::NoModifier,
                                          QPoint(x, h->viewport()->height() / 2));
                        clickDelivered = true;
                        QStringList afterClick;
                        for (int i = 0; i < root->childCount(); i++)
                            afterClick << root->child(i)->text(1);
                        reorderedByRealClick = (afterClick != namesBefore);

                        if (!reorderedByRealClick) {
                            // The call the click makes internally, so that the
                            // QUESTION (what a reorder does to the batch) is still
                            // answered even where the synthetic click is not.
                            h->setSortIndicator(section, Qt::DescendingOrder);
                        }
                    }

                    QSet<quintptr> after;
                    for (int i = 0; i < root->childCount(); i++) {
                        namesAfter << root->child(i)->text(1);
                        after.insert(reinterpret_cast<quintptr>(root->child(i)));
                    }
                    reorderedBySortIndicator = (!reorderedByRealClick && namesAfter != namesBefore);
                    sameItemSetAfter = (before == after);
                };

                // THE LIST, READ INSIDE THE FIRST COMPLETION'S OWN FRAME - after
                // completedWrite has labelled a row (:2856) and before its tail
                // re-drives the loop (:2872). Read at the END instead and the
                // measurement is worthless: the re-driven batch goes on to
                // transfer and label the other row too, so every row reads
                // "Completed." whether or not the right one was labelled first.
                store->afterCompletionAction = [&, list]() {
                    if (list == nullptr)
                        return;
                    writeNameAtFirstCompletion = obs::lastWriteName;
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        namesAtFirstCompletion << root->child(i)->text(1);
                        statusesAtFirstCompletion << root->child(i)->text(7);
                        if (root->child(i)->text(7) == QStringLiteral("Completed."))
                            labelledAtFirstCompletion = root->child(i)->text(1);
                    }
                };

                dialog->downloadClicked();

                // Let the write completion (and whatever it re-drives) arrive.
                for (int i = 0; i < 100; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);

                writeFileCalls = obs::writeFileCalls;
                rideOpens = rideopen::opens;
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++)
                        statuses << root->child(i)->text(7);
                }

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
                QTimer::singleShot(20000, &appLoop, [&timedOut]() {
                    timedOut = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        QVERIFY2(timedOut == false, "the run never came back");

        // ---- THE PREMISES: the route is open, and the click landed in the
        //      suspension.
        //
        // "the header is clickable" was, until this run, REASONED from Qt's API
        // contract: nothing calls setSectionsClickable(false) and
        // setSortingEnabled(true) is documented to make sections clickable. These
        // two are that claim, executed.
        QVERIFY2(sortingEnabledOnSyncList, "the sync list is not sortable, so this whole slot is about nothing");
        QVERIFY2(sectionsClickable, "the sync list's header sections are not clickable");
        QVERIFY2(sortIndicatorShown, "the sync list's header shows no sort indicator");
        QVERIFY2(clickDelivered, "the header click was never delivered into the nested loop");
        QVERIFY2(rideOpens >= 1, "row[0]'s ride file was never opened, so there was no nested loop to click into");
        QCOMPARE(namesBefore.count(), 2);

        // WHY NOTHING MOVED, established BEFORE the negative below is read, so
        // that the negative cannot be an artefact of synthetic mouse delivery -
        // the backend-dependent thing LSN-062 / ORCH-017 spent two cycles on. The
        // batch had sorting OFF on this list while the click was delivered, and
        // with it the header's own clickability and indicator, which is what
        // TEST-131 Q4 measured setSortingEnabled(false) to do.
        QCOMPARE(sortingDuringBatch, 0);
        QCOMPARE(clickableDuringBatch, 0);
        QCOMPARE(indicatorShownDuringBatch, 0);

        // ---- THE RULE (DEC-garmin-038). Was "MEASURED, NOT DESIRED"; the values
        //      below are now the desired ones.
        //
        // (i)  THE SORT DELIVERED INTO THE RUNNING BATCH MOVES NOTHING - by
        //      EITHER mechanism. The synthetic click is attempted first and the
        //      setSortIndicator fallback (the call QHeaderView makes for itself on
        //      a real mouse release) is attempted when the click moved nothing, so
        //      both vectors are exercised and both must be inert. The rows are
        //      also still the same two items: nothing is freed here either, which
        //      is why no lifetime guard was ever going to be what closed this
        //      route.
        QVERIFY2(!reorderedByRealClick,
                 qPrintable(QStringLiteral("a header click reordered a list whose batch had sorting disabled: [%1] -> "
                                           "[%2]")
                                .arg(namesBefore.join(QStringLiteral("|")))
                                .arg(namesAfter.join(QStringLiteral("|")))));
        QVERIFY2(!reorderedBySortIndicator,
                 qPrintable(QStringLiteral("setSortIndicator reordered a list whose batch had sorting disabled: [%1] "
                                           "-> [%2]")
                                .arg(namesBefore.join(QStringLiteral("|")))
                                .arg(namesAfter.join(QStringLiteral("|")))));
        QVERIFY2(namesAfter == namesBefore,
                 qPrintable(QStringLiteral("the list reordered under the running batch: [%1] -> [%2]")
                                .arg(namesBefore.join(QStringLiteral("|")))
                                .arg(namesAfter.join(QStringLiteral("|")))));
        QVERIFY2(sameItemSetAfter, "the rows were REPLACED, not permuted - this slot measures the wrong thing");

        // (ii) THE BATCH KEPT GOING, which is the point: no guard fired. The
        //      driver resumed onto `curr`, a pointer that is still valid and still
        //      names the right activity, and uploaded it.
        QVERIFY2(writeFileCalls >= 1,
                 qPrintable(QStringLiteral("no transfer happened at all after the sort (writeFile called %1 time(s)), "
                                           "so this run says nothing about what a sort does to a batch")
                                .arg(writeFileCalls)));

        // (iii) THE LABELLING HALF, WHICH DEC-garmin-036 CLOSED. AMENDED
        //       2026-08-18; what this block said before is kept below, because
        //       what it USED to measure is the whole reason the fix exists.
        //
        //       WAS: completedWrite addressed rows POSITIONALLY, as
        //       child(listindex-1), and the sort moved a different item under that
        //       index - so the verdict went on whatever the permutation put at
        //       index 0 while the row that had actually been transferred kept
        //       reading "Uploading". Worse, QTreeWidgetItem::child(int)
        //       bounds-checks and THEN calls executePendingSort()
        //       (qtreewidget.h:145-150, inline and readable on this machine), so a
        //       pending re-sort executed INSIDE the `child(listindex-1)`
        //       expression itself: no guard wrapped around that expression could
        //       have observed the pre-sort order.
        //
        //       IS: the completion labels through the in-flight ticket's stored
        //       row POINTER, which a reorder does not move because a reorder frees
        //       nothing and permutes nothing but positions - point (i) above. The
        //       index is gone from the slot, and with it that whole failure mode.
        //
        //       AND NOW BOTH HALVES ARE CLOSED (2026-08-23). The ticket kept the
        //       LABEL right THROUGH a permutation; DEC-garmin-038 stops the
        //       permutation happening at all while a batch is running, which is
        //       what the DRIVERS' positional walk needed. The two are independent:
        //       the ticket is still what carries a row through a sort made BETWEEN
        //       batches, and point (iv) below is no longer a measurement of damage
        //       but the assertion that the damage is gone.
        transferredRow = namesBefore.value(0);
        QVERIFY2(!writeNameAtFirstCompletion.isEmpty(),
                 "the first completion's frame was never sampled, so nothing here is a measurement");
        QVERIFY2(writeNameAtFirstCompletion.startsWith(QFileInfo(transferredRow).baseName()),
                 qPrintable(QStringLiteral("the row that was UPLOADED was \"%1\", not row[0] (\"%2\") - the premise "
                                           "of the comparison below does not hold")
                                .arg(writeNameAtFirstCompletion)
                                .arg(transferredRow)));
        QVERIFY2(!labelledAtFirstCompletion.isEmpty(),
                 qPrintable(QStringLiteral("no row was labelled \"Completed.\" in the completion's own frame: [%1]")
                                .arg(statusesAtFirstCompletion.join(QStringLiteral("|")))));
        // THE CRITERION, now that this half is a rule rather than a measurement
        // (REQ-028 (c): "every non-empty status cell names the outcome of a
        // transfer that actually happened TO THAT ROW"). The row that was
        // uploaded is the row that is labelled, reorder or no reorder.
        QVERIFY2(labelledAtFirstCompletion == transferredRow,
                 qPrintable(QStringLiteral("the completion labelled \"%1\", but the row it transferred was \"%2\" - "
                                           "DEC-036's ticket is not carrying the row through the sort")
                                .arg(labelledAtFirstCompletion)
                                .arg(transferredRow)));

        // ...spelled out as VALUES, before and after the fix, so that what changed
        // is on the record rather than only in a decision entry. Both columns
        // measured identically under offscreen and minimal:
        //
        //                          BEFORE DEC-038        NOW
        //   before                 [10:00, 11:00]        [10:00, 11:00]
        //   after the sort         [11:00, 10:00]        [10:00, 11:00]  (inert)
        //   uploaded first         10:00                 10:00
        //   labelled "Completed."  10:00                 10:00   (DEC-036's ticket
        //                                                         - it used to be
        //                                                         11:00 before that)
        //   statuses at that point ["", "Completed."]    ["Completed.", ""]
        //   uploads in total       10:00 TWICE           10:00 then 11:00
        //   11:00                  never transferred     transferred, once
        //
        // The status list flips because the ROWS no longer move: the labelled row
        // is row[0] both times, and before the fix row[0] was the one the
        // permutation had put there.
        QCOMPARE(namesAfter, namesBefore);
        QCOMPARE(labelledAtFirstCompletion, namesBefore.value(0));
        QCOMPARE(statusesAtFirstCompletion, QStringList() << "Completed." << "");

        // (iv) ...AND THE LOOP TRANSFERS EACH ROW EXACTLY ONCE. `listindex` is a
        //      POSITION, and that is only safe while nothing permutes the list
        //      under it: before DEC-038 the completion's tail re-drove the loop
        //      from index 1, which the permutation had made the row just uploaded,
        //      while the row now at index 0 - never transferred - sat behind the
        //      bookmark forever. Two writeFile calls for two checked rows, and
        //      they must be DIFFERENT rows: the count alone read correct even when
        //      the batch was uploading the same activity twice, which is why the
        //      names are compared.
        QCOMPARE(writeFileCalls, 2);
        QCOMPARE(obs::writeNames.count(), 2);
        QVERIFY2(obs::writeNames.value(0) != obs::writeNames.value(1),
                 qPrintable(QStringLiteral("the two uploads were of the SAME row (%1, %2) - the sort route's double "
                                           "transfer is back")
                                .arg(obs::writeNames.value(0))
                                .arg(obs::writeNames.value(1))));
        // ...and it is the two rows the user checked, not one row twice under two
        // names: row[0]'s and row[1]'s, in the list's own order.
        QVERIFY2(obs::writeNames.value(0).startsWith(QFileInfo(namesBefore.value(0)).baseName()) &&
                     obs::writeNames.value(1).startsWith(QFileInfo(namesBefore.value(1)).baseName()),
                 qPrintable(QStringLiteral("the batch uploaded [%1] for the rows [%2]")
                                .arg(obs::writeNames.join(QStringLiteral("|")))
                                .arg(namesBefore.join(QStringLiteral("|")))));
    }

    // -- TEST-131 (a) (S-R028-01, probe for the HELD decision) -----------
    // DOES AN ITEM DATA WRITE SCHEDULE A RESORT BY ITSELF? Qt only, no dialog.
    //
    // WHY. The recommendation that wants "disable sorting for the batch" rests
    // on one claim its author labelled REASONED-not-EXECUTED: that with
    // sortingEnabled == true, ANY change to an item's data in the SORT COLUMN
    // schedules a resort, applied at the next child()/index() call - so the
    // three drivers, which unconditionally write curr->setText(<status col>, ...)
    // on every dispatch, would trigger the reorder THEMSELVES with no click
    // during the batch at all. This project has had exactly that class of
    // reasoned framework claim falsified by measurement twice (TEST-081,
    // TEST-089), so the claim is measured here before anything is decided on it.
    //
    // WHAT THIS SLOT IS NOT. It asserts nothing about production and fixes
    // nothing. It is a TRACE of Qt's behaviour on this build, pinned so that a
    // Qt upgrade that changes the answer fails here rather than silently
    // invalidating the decision that was taken on it.
    //
    // THE SUBJECT IS A PLAIN QTreeWidget, deliberately: the dialog, the store and
    // the drivers are all absent, so nothing in this slot's answer can be an
    // artefact of GoldenCheetah code. Q5 (TEST-131 (b)/(c)) is where the same
    // question is asked of the real lists.
    //
    // READING THE ORDER IS PART OF THE MEASUREMENT. Every snapshot below goes
    // through invisibleRootItem()->child(i), which bounds-checks and THEN calls
    // executePendingSort() (qtreewidget.h:145-150 on this machine) - i.e. the
    // same addressing call the drivers use at CloudService.cpp:2171 / :2548 /
    // :3094. The layoutChanged counter beside it is what separates "the resort
    // ran inside setText" from "the resort was PENDING and ran inside child()".
    void probeWhetherADataWriteSelfTriggersAResort()
    {
        // THE SNAPSHOT KEEPS THE TWO APART ON PURPOSE. `addresses` is what every
        // reorder comparison below uses; `shown` is name@address, for the log
        // only. Comparing the names would be a bug in the instrument: a run that
        // WRITES to the column it is reading would see the text change and call
        // it a reorder. (It was written that way first, and Q1a caught it.)
        struct Snapshot
        {
            QStringList addresses;
            QStringList shown;
        };
        auto snapshot = [](QTreeWidget& w, int labelColumn) {
            Snapshot s;
            QTreeWidgetItem* root = w.invisibleRootItem();
            for (int i = 0; i < root->childCount(); i++) {
                QTreeWidgetItem* it = root->child(i); // <- executePendingSort() happens HERE
                const QString address = QStringLiteral("0x%1").arg(reinterpret_cast<quintptr>(it), 0, 16);
                s.addresses << address;
                s.shown << QStringLiteral("%1@%2").arg(it->text(labelColumn), address);
            }
            return s;
        };
        auto addRows = [](QTreeWidget& w, int statusColumn, const QStringList& names, const QStringList& statuses) {
            for (int i = 0; i < names.count(); i++) {
                QTreeWidgetItem* it = new QTreeWidgetItem(&w);
                it->setText(0, names.at(i));
                it->setText(statusColumn, statuses.value(i));
            }
        };

        // ---- Q1a. THE SORT NOBODY ASKED FOR: setSortingEnabled(true) alone
        //      sorts on the default indicator section, with no click, no
        //      setSortIndicator and no sortItems anywhere in this block. The
        //      section and order are READ rather than assumed, and the text
        //      written is chosen to be extreme in whichever direction that is,
        //      so the run cannot pass by writing a value that could not move.
        QTreeWidget q1a;
        q1a.setColumnCount(3);
        addRows(q1a, 2, QStringList() << "A" << "B" << "C", QStringList());
        q1a.setSortingEnabled(true);
        const int q1aSection = q1a.header()->sortIndicatorSection();
        const Qt::SortOrder q1aOrder = q1a.header()->sortIndicatorOrder();
        const QString q1aExtreme = (q1aOrder == Qt::AscendingOrder) ? QStringLiteral("zzz") : QStringLiteral("000");
        const Snapshot q1aBefore = snapshot(q1a, 0);
        int q1aLayoutChanges = 0;
        QObject::connect(q1a.model(), &QAbstractItemModel::layoutChanged, &q1a, [&]() { q1aLayoutChanges++; });
        q1a.invisibleRootItem()->child(0)->setText(q1aSection, q1aExtreme); // the ONLY event
        const int q1aChangesAtWrite = q1aLayoutChanges;
        const Snapshot q1aAfter = snapshot(q1a, 0);
        const int q1aChangesAfterRead = q1aLayoutChanges;
        const bool q1aReordered = (q1aAfter.addresses != q1aBefore.addresses);
        const int q1aIndexAfter = q1aAfter.addresses.indexOf(q1aBefore.addresses.value(0));

        // ---- Q1b. THE PRODUCTION SHAPE: eight columns, the status column is 7,
        //      and the sort on it was established EARLIER (the user's header
        //      click, stood in for by sortItems - a click's own model call).
        //      Between that and the write there is no further interaction.
        QTreeWidget q1b;
        q1b.setColumnCount(8);
        addRows(q1b, 7, QStringList() << "row0" << "row1" << "row2", QStringList() << "" << "" << "");
        q1b.setSortingEnabled(true);
        q1b.sortItems(7, Qt::AscendingOrder);
        const Snapshot q1bBefore = snapshot(q1b, 0); // any pending sort is executed HERE, not later
        int q1bLayoutChanges = 0;
        QObject::connect(q1b.model(), &QAbstractItemModel::layoutChanged, &q1b, [&]() { q1bLayoutChanges++; });
        q1b.invisibleRootItem()->child(0)->setText(7, QStringLiteral("Uploading")); // CloudService.cpp:2283, verbatim
        const int q1bChangesAtWrite = q1bLayoutChanges;
        const Snapshot q1bAfter = snapshot(q1b, 0);
        const int q1bChangesAfterRead = q1bLayoutChanges;
        const bool q1bReordered = (q1bAfter.addresses != q1bBefore.addresses);
        const int q1bIndexAfter = q1bAfter.addresses.indexOf(q1bBefore.addresses.value(0));

        // ---- Q1c. THE SPECIFICITY CONTROL, on the same widget: a write to a
        //      column that is NOT the sort column. If this moved rows too, the
        //      counter would be measuring writes rather than sorts, and "the
        //      drivers write the STATUS column" would stop being the point.
        q1b.invisibleRootItem()->child(0)->setText(2, QStringLiteral("not the sort column"));
        const Snapshot q1cAfter = snapshot(q1b, 0);
        const bool q1cReordered = (q1cAfter.addresses != q1bAfter.addresses);
        const int q1cChangesAfterWrite = q1bLayoutChanges;

        // ---- Q2. Does setSortingEnabled(false) suppress it? Same construction,
        //      one variable changed: sorting is off when the write happens.
        QTreeWidget q2;
        q2.setColumnCount(8);
        addRows(q2, 7, QStringList() << "row0" << "row1" << "row2", QStringList() << "" << "" << "");
        q2.setSortingEnabled(true);
        q2.sortItems(7, Qt::AscendingOrder);
        q2.setSortingEnabled(false);
        const Snapshot q2Before = snapshot(q2, 0);
        int q2LayoutChanges = 0;
        QObject::connect(q2.model(), &QAbstractItemModel::layoutChanged, &q2, [&]() { q2LayoutChanges++; });
        q2.invisibleRootItem()->child(0)->setText(7, QStringLiteral("Uploading"));
        const Snapshot q2After = snapshot(q2, 0);
        const bool q2Suppressed = (q2After.addresses == q2Before.addresses);

        // ---- Q3. THE RESIDUAL: is there a PENDING-SORT WINDOW for a later
        //      setSortingEnabled(false) to arrive too late for?
        //
        //      Two separate things are measured, because the answer to the
        //      second only means something given the first. (q3a) WHEN does the
        //      resort a data write causes actually run - inside setText, or
        //      lazily at the next child()? The layoutChanged counter sampled
        //      between the write and the read is the discriminator. (q3b) If the
        //      write is followed by setSortingEnabled(false) BEFORE anything
        //      reads the list, is the order the reader then sees the sorted one
        //      or the original one?
        QTreeWidget q3;
        q3.setColumnCount(8);
        addRows(q3, 7, QStringList() << "row0" << "row1" << "row2", QStringList() << "" << "" << "");
        q3.setSortingEnabled(true);
        q3.sortItems(7, Qt::AscendingOrder);
        const Snapshot q3Before = snapshot(q3, 0);
        int q3LayoutChanges = 0;
        QObject::connect(q3.model(), &QAbstractItemModel::layoutChanged, &q3, [&]() { q3LayoutChanges++; });
        q3.invisibleRootItem()->child(0)->setText(7, QStringLiteral("Uploading"));
        const int q3ChangesAtWrite = q3LayoutChanges; // 0 => a sort was left PENDING; 1 => it already ran
        q3.setSortingEnabled(false);                  // the guard, arriving after the write
        const int q3ChangesAtDisable = q3LayoutChanges;
        const Snapshot q3After = snapshot(q3, 0);
        const bool q3ReorderSurvivedTheDisable = (q3After.addresses != q3Before.addresses);

        // ---- Q3c. ...AND WHETHER ANY LAZY WINDOW EXISTS AT ALL on this build,
        //      from the OTHER direction: the counter is connected BEFORE sorting
        //      is turned on, so an enable-path sort that were deferred to the
        //      first child() call would show up as changes-at-enable == 0 and
        //      changes-after-read > 0.
        QTreeWidget q3c;
        q3c.setColumnCount(8);
        addRows(q3c, 7, QStringList() << "row0" << "row1" << "row2", QStringList() << "c" << "a" << "b");
        int q3cLayoutChanges = 0;
        QObject::connect(q3c.model(), &QAbstractItemModel::layoutChanged, &q3c, [&]() { q3cLayoutChanges++; });
        q3c.setSortingEnabled(true);
        q3c.sortItems(7, Qt::AscendingOrder);
        const int q3cChangesAtEnable = q3cLayoutChanges;
        const Snapshot q3cAfter = snapshot(q3c, 0);
        const int q3cChangesAfterRead = q3cLayoutChanges;

        // ---- Q4. WHAT DISABLING SORTING ACTUALLY SEVERS. Two separate
        //      sub-questions: (a) is the header still section-clickable, and
        //      (b) does setSortIndicator - the call QHeaderView makes for itself
        //      on a real mouse release - still reorder?
        QTreeWidget q4;
        q4.setColumnCount(8);
        addRows(q4, 7, QStringList() << "row0" << "row1" << "row2", QStringList() << "" << "Completed." << "Uploading");
        q4.setSortingEnabled(true);
        q4.sortItems(7, Qt::AscendingOrder);
        const bool q4ClickableWhileEnabled = q4.header()->sectionsClickable();
        const bool q4IndicatorShownWhileEnabled = q4.header()->isSortIndicatorShown();
        const Snapshot q4Ascending = snapshot(q4, 0);
        q4.header()->setSortIndicator(7, Qt::DescendingOrder); // the control: it works when enabled
        const Snapshot q4DescendingWhileEnabled = snapshot(q4, 0);
        const bool q4IndicatorReordersWhileEnabled = (q4DescendingWhileEnabled.addresses != q4Ascending.addresses);
        q4.setSortingEnabled(false);
        const bool q4ClickableWhileDisabled = q4.header()->sectionsClickable();
        const bool q4IndicatorShownWhileDisabled = q4.header()->isSortIndicatorShown();
        const Snapshot q4BeforeIndicatorWhileDisabled = snapshot(q4, 0);
        // Turning sorting off is not itself a reorder: the rows stay where the
        // last sort left them.
        const bool q4DisableAloneReordered =
            (q4BeforeIndicatorWhileDisabled.addresses != q4DescendingWhileEnabled.addresses);
        q4.header()->setSortIndicator(7, Qt::AscendingOrder);
        const Snapshot q4AfterIndicatorWhileDisabled = snapshot(q4, 0);
        const bool q4IndicatorReordersWhileDisabled =
            (q4AfterIndicatorWhileDisabled.addresses != q4BeforeIndicatorWhileDisabled.addresses);
        // ...and one more write, to show that the SUPPRESSION SURVIVES a later
        // indicator change while sorting is still off (Option A's window is the
        // whole batch, not one call).
        const Snapshot q4BeforeLateWrite = snapshot(q4, 0);
        q4.invisibleRootItem()->child(0)->setText(7, QStringLiteral("Uploading"));
        const Snapshot q4AfterLateWrite = snapshot(q4, 0);
        const bool q4LateWriteReordered = (q4AfterLateWrite.addresses != q4BeforeLateWrite.addresses);

        // The raw observation, in the run log, per QPA backend - so the numbers
        // behind the pins below are evidence and not a recollection.
        qInfo("TEST-131 Q1a [%s] sortingEnabled(true) only (section=%d order=%s), write \"%s\" to that section: "
              "before=[%s] after=[%s] reordered=%d writtenRowNowAtIndex=%d layoutChanged(at write)=%d "
              "(after read)=%d",
              qPrintable(QString::fromLatin1(qgetenv("QT_QPA_PLATFORM"))), q1aSection,
              q1aOrder == Qt::AscendingOrder ? "asc" : "desc", qPrintable(q1aExtreme),
              qPrintable(q1aBefore.shown.join(QChar('|'))), qPrintable(q1aAfter.shown.join(QChar('|'))),
              int(q1aReordered), q1aIndexAfter, q1aChangesAtWrite, q1aChangesAfterRead);
        qInfo("TEST-131 Q1b sorted on col 7 then setText(7,\"Uploading\") on index 0: before=[%s] after=[%s] "
              "reordered=%d writtenRowNowAtIndex=%d layoutChanged(at write)=%d (after read)=%d || Q1c write to a "
              "NON-sort column: reordered=%d layoutChanged=%d",
              qPrintable(q1bBefore.shown.join(QChar('|'))), qPrintable(q1bAfter.shown.join(QChar('|'))),
              int(q1bReordered), q1bIndexAfter, q1bChangesAtWrite, q1bChangesAfterRead, int(q1cReordered),
              q1cChangesAfterWrite);
        qInfo("TEST-131 Q2 sortingEnabled(false) before the write: before=[%s] after=[%s] suppressed=%d "
              "layoutChanged=%d",
              qPrintable(q2Before.shown.join(QChar('|'))), qPrintable(q2After.shown.join(QChar('|'))),
              int(q2Suppressed), q2LayoutChanges);
        qInfo("TEST-131 Q3 write THEN disable: before=[%s] after=[%s] reorderSurvivedTheDisable=%d "
              "layoutChanged(at write)=%d (at disable)=%d (after read)=%d || Q3c enable path: "
              "layoutChanged(at enable)=%d (after read)=%d order=[%s]",
              qPrintable(q3Before.shown.join(QChar('|'))), qPrintable(q3After.shown.join(QChar('|'))),
              int(q3ReorderSurvivedTheDisable), q3ChangesAtWrite, q3ChangesAtDisable, q3LayoutChanges,
              q3cChangesAtEnable, q3cChangesAfterRead, qPrintable(q3cAfter.shown.join(QChar('|'))));
        qInfo("TEST-131 Q4 enabled: clickable=%d indicatorShown=%d indicatorReorders=%d || disabled: clickable=%d "
              "indicatorShown=%d indicatorReorders=%d disableAloneReordered=%d lateWriteReordered=%d before=[%s] "
              "after=[%s]",
              int(q4ClickableWhileEnabled), int(q4IndicatorShownWhileEnabled), int(q4IndicatorReordersWhileEnabled),
              int(q4ClickableWhileDisabled), int(q4IndicatorShownWhileDisabled), int(q4IndicatorReordersWhileDisabled),
              int(q4DisableAloneReordered), int(q4LateWriteReordered),
              qPrintable(q4BeforeIndicatorWhileDisabled.shown.join(QChar('|'))),
              qPrintable(q4AfterIndicatorWhileDisabled.shown.join(QChar('|'))));

        // ---- THE PREMISES. Without these the answers above are about nothing.
        QCOMPARE(q1aBefore.addresses.count(), 3);
        QCOMPARE(q1bBefore.addresses.count(), 3);
        QCOMPARE(q2Before.addresses.count(), 3);
        QCOMPARE(q3Before.addresses.count(), 3);
        QCOMPARE(q4Ascending.addresses.count(), 3);
        QVERIFY2(q1b.isSortingEnabled(), "q1b was not sorting, so its answer is about nothing");
        QVERIFY2(!q2.isSortingEnabled(), "q2's sorting was never turned off, so its answer is about nothing");
        // The positive control for Q4's negative half: an indicator change DOES
        // reorder this data set while sorting is on, so a `false` when it is off
        // is a real difference and not a data set that cannot move.
        QVERIFY2(q4IndicatorReordersWhileEnabled,
                 "setSortIndicator did not reorder even with sorting ENABLED - Q4's negative half would be vacuous");

        // ---- THE ANSWERS, PINNED. Measured 2026-08-22, IDENTICAL under
        //      QT_QPA_PLATFORM=offscreen and =minimal, Qt 6.8.2.
        //
        // Q1: A DATA WRITE TO THE SORT COLUMN IS ENOUGH. No click, no
        //     setSortIndicator, no sortItems between the write and the read - the
        //     item that was written moves, and the move is there at the very next
        //     child() call. The claim the recommendation could not source is TRUE
        //     on this build, on both the default sort column and on column 7.
        QVERIFY2(q1aReordered,
                 qPrintable(QStringLiteral("MEASURED — a write to the DEFAULT sort column did NOT reorder: [%1] -> "
                                           "[%2]")
                                .arg(q1aBefore.shown.join(QChar('|')))
                                .arg(q1aAfter.shown.join(QChar('|')))));
        QCOMPARE(q1aIndexAfter, 2);
        QVERIFY2(q1bReordered,
                 qPrintable(QStringLiteral("MEASURED, NOT DESIRED — setText(7,\"Uploading\") did NOT reorder: "
                                           "[%1] -> [%2]")
                                .arg(q1bBefore.shown.join(QChar('|')))
                                .arg(q1bAfter.shown.join(QChar('|')))));
        // ...and the row that was written is the row that moved, to the END of an
        // ascending sort ("" < "Uploading"), which is exactly the direction that
        // hurts a loop walking forwards from a stored index.
        QCOMPARE(q1bIndexAfter, 2);
        //
        // Q1, THE PART THE RECOMMENDATION GOT WRONG IN DETAIL, and it is worse
        // rather than better: the resort is NOT lazy. layoutChanged is emitted
        // INSIDE setText (changes-at-write == 1, and reading afterwards adds
        // none), so the rows have already moved by the time anything addresses
        // them - there is no "not yet applied" window between the write and the
        // read at all, and the child()/executePendingSort() route is not what
        // delivers this one.
        QCOMPARE(q1bChangesAtWrite, 1);
        QCOMPARE(q1bChangesAfterRead, 1);
        QCOMPARE(q1aChangesAtWrite, 1);
        // Q1c, the specificity control: the trigger is the SORT COLUMN, not any
        // write. A write to another column of the same item moves nothing and
        // emits no layout change - which is exactly why "the drivers write the
        // status column, and the status column is one users sort by" is the
        // load-bearing sentence rather than "the drivers write".
        QVERIFY2(!q1cReordered, "MEASURED — a write to a NON-sort column reordered the list");
        QCOMPARE(q1cChangesAfterWrite, 1);
        // Q3c: nor is the ENABLE path lazy on this build - it too sorts inside
        // the call, not at the first read.
        QVERIFY2(q3cChangesAtEnable > 0, "the enable-path sort was deferred - re-read Q3");
        QCOMPARE(q3cChangesAfterRead, q3cChangesAtEnable);
        //
        // Q2: turning sorting off DOES suppress it - not one layout change, not
        //     one moved row.
        QVERIFY2(q2Suppressed,
                 qPrintable(QStringLiteral("MEASURED — setSortingEnabled(false) did NOT suppress the self-trigger: "
                                           "[%1] -> [%2]")
                                .arg(q2Before.shown.join(QChar('|')))
                                .arg(q2After.shown.join(QChar('|')))));
        QCOMPARE(q2LayoutChanges, 0);
        //
        // Q3: THE RESIDUAL IS NOT A PENDING SORT - IT IS AN ALREADY-EXECUTED ONE.
        //     Because the write sorts immediately (changes-at-write == 1, before
        //     the disable), a setSortingEnabled(false) that arrives after a write
        //     cannot cancel anything: the rows have already moved, and the
        //     disable neither undoes that nor adds to it. So "does disabling
        //     cancel a pending sort" does not arise on this build; what matters
        //     for a guard is only that it is in place BEFORE the first write.
        QCOMPARE(q3ChangesAtWrite, 1);
        QCOMPARE(q3ChangesAtDisable, 1);
        QCOMPARE(q3LayoutChanges, 1);
        QVERIFY2(q3ReorderSurvivedTheDisable,
                 "the reorder that had already executed was undone by setSortingEnabled(false) - re-read this slot");
        //
        // Q4 (a): the drafted clause is RIGHT, not wrong. setSortingEnabled(false)
        //         turns the header's own clickability OFF (QTreeView::
        //         setSortingEnabled calls setSectionsClickable(enable) and
        //         setSortIndicatorShown(enable)), so with sorting disabled there
        //         is no click-to-sort left to close.
        QVERIFY2(q4ClickableWhileEnabled, "premise: sorting-enabled headers are clickable");
        QVERIFY2(q4IndicatorShownWhileEnabled, "premise: sorting-enabled headers show the indicator");
        QVERIFY2(!q4ClickableWhileDisabled, "MEASURED — the header stayed clickable with sorting disabled");
        QVERIFY2(!q4IndicatorShownWhileDisabled, "MEASURED — the indicator stayed shown with sorting disabled");
        // Q4 (b): and the connection IS what gets severed - setSortIndicator, the
        //         call a real mouse release makes, no longer reorders anything
        //         once sorting is off. Both halves of the click vector are shut,
        //         not just one.
        QVERIFY2(!q4IndicatorReordersWhileDisabled,
                 qPrintable(QStringLiteral("MEASURED — setSortIndicator STILL reordered with sorting disabled: "
                                           "[%1] -> [%2]")
                                .arg(q4BeforeIndicatorWhileDisabled.shown.join(QChar('|')))
                                .arg(q4AfterIndicatorWhileDisabled.shown.join(QChar('|')))));
        QVERIFY2(!q4DisableAloneReordered, "MEASURED — setSortingEnabled(false) reordered the rows by itself");
        QVERIFY2(!q4LateWriteReordered, "MEASURED — a write after a disabled-mode indicator change still reordered");
    }

    // -- TEST-131 (b) (S-R028-01) ----------------------------------------
    // Q5, HALF ONE: DOES THE Q1 MECHANISM EVEN APPLY TO THESE THREE LISTS?
    //
    // A self-trigger that cannot move a row is a curiosity, not a defect. Three
    // things have to be true of the REAL dialog for Q1 to reach it: the Status
    // column has to exist within the list's columnCount, the user has to be able
    // to sort by it, and the strings the drivers write to it have to be able to
    // change a row's position relative to its siblings. This slot measures all
    // three on all three lists, using the drivers' own column indices and their
    // own literal strings.
    //
    // NO BATCH RUNS HERE. The status writes are made by the slot, not by
    // production - what is being measured is the WIDGET's response to them.
    // TEST-131 (c) is the end-to-end half, where production makes the write.
    void probeWhetherAStatusWriteCanMoveARowInTheRealLists()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        struct ListFacts
        {
            int columns = -1;
            int statusColumn = -1;
            int rows = 0;
            bool sortingEnabled = false;
            bool sectionsClickable = false;
            bool indicatorShown = false;
            bool statusColumnHidden = true;
            bool statusColumnWithinCount = false;
            QStringList afterUserSort;
            QStringList afterStatusWrite;
            bool movedByStatusWrite = false;
        };
        ListFacts down, up, sync;
        bool timedOut = false;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // Both halves populated so that ALL THREE lists have >= 2 rows:
                // two remote (download rows) and two local (upload rows).
                store->entryNames = QStringList() << rebuildRemoteActivity(0) << rebuildRemoteActivity(1);
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                // Names for the log; ADDRESSES for every comparison (a run that
                // writes the column it reads must never compare texts).
                auto fingerprints = [](QTreeWidget* w) {
                    QStringList out;
                    QTreeWidgetItem* root = w->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        QTreeWidgetItem* it = root->child(i);
                        out << QStringLiteral("%1@0x%2").arg(it->text(1)).arg(reinterpret_cast<quintptr>(it), 0, 16);
                    }
                    return out;
                };
                auto addressesOf = [](QTreeWidget* w) {
                    QStringList out;
                    QTreeWidgetItem* root = w->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++)
                        out << QStringLiteral("0x%1").arg(reinterpret_cast<quintptr>(root->child(i)), 0, 16);
                    return out;
                };

                auto probe = [&](QTreeWidget* w, ListFacts& f, const QString& driverText) {
                    if (w == nullptr)
                        return;
                    f.columns = w->columnCount();
                    f.rows = w->invisibleRootItem()->childCount();
                    f.sortingEnabled = w->isSortingEnabled();
                    if (QHeaderView* h = w->header()) {
                        f.sectionsClickable = h->sectionsClickable();
                        f.indicatorShown = h->isSortIndicatorShown();
                    }
                    // The Status column is found the way a user finds it: by its
                    // header text, over the columns the list actually HAS.
                    for (int c = 0; c < w->columnCount(); c++)
                        if (w->headerItem() != nullptr && w->headerItem()->text(c) == QStringLiteral("Status")) {
                            f.statusColumn = c;
                            break;
                        }
                    f.statusColumnWithinCount = (f.statusColumn >= 0 && f.statusColumn < w->columnCount());
                    if (f.statusColumn >= 0)
                        f.statusColumnHidden = w->isColumnHidden(f.statusColumn);
                    if (f.statusColumn < 0 || f.rows < 2)
                        return;

                    // THE USER SORTS BY STATUS - once, before anything else. The
                    // delivery is setSortIndicator because TEST-112 measured that
                    // a synthetic header click does not reorder under either of
                    // this target's QPA backends while setSortIndicator does; it
                    // is the call QHeaderView makes for itself on a real release.
                    w->header()->setSortIndicator(f.statusColumn, Qt::AscendingOrder);
                    f.afterUserSort = fingerprints(w);
                    const QStringList sortedAddresses = addressesOf(w);

                    // ...and then ONE status write, the driver's own literal.
                    w->invisibleRootItem()->child(0)->setText(f.statusColumn, driverText);
                    f.afterStatusWrite = fingerprints(w);
                    f.movedByStatusWrite = (addressesOf(w) != sortedAddresses);
                };

                probe(rideListWithHeader(dialog, QStringLiteral("Workout Name")), down, QStringLiteral("Downloading"));
                probe(rideListWithHeader(dialog, QStringLiteral("File")), up, QStringLiteral("Uploading"));
                probe(rideListWithHeader(dialog, QStringLiteral("Source")), sync, QStringLiteral("Uploading"));

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
                QTimer::singleShot(20000, &appLoop, [&timedOut]() {
                    timedOut = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        QVERIFY2(timedOut == false, "the run never came back");

        const ListFacts* all[3] = {&down, &up, &sync};
        const char* names[3] = {"rideListDown", "rideListUp", "rideListSync"};
        for (int i = 0; i < 3; i++)
            qInfo("TEST-131 Q5 [%s] %s: columns=%d statusColumn=%d withinCount=%d hidden=%d rows=%d "
                  "sortingEnabled=%d clickable=%d indicatorShown=%d movedByStatusWrite=%d afterUserSort=[%s] "
                  "afterStatusWrite=[%s]",
                  qPrintable(QString::fromLatin1(qgetenv("QT_QPA_PLATFORM"))), names[i], all[i]->columns,
                  all[i]->statusColumn, int(all[i]->statusColumnWithinCount), int(all[i]->statusColumnHidden),
                  all[i]->rows, int(all[i]->sortingEnabled), int(all[i]->sectionsClickable),
                  int(all[i]->indicatorShown), int(all[i]->movedByStatusWrite),
                  qPrintable(all[i]->afterUserSort.join(QChar('|'))),
                  qPrintable(all[i]->afterStatusWrite.join(QChar('|'))));

        // ---- THE PREMISES: three populated, sortable lists.
        QVERIFY2(down.rows >= 2, "the download list has fewer than two rows, so nothing can be moved relative to it");
        QVERIFY2(up.rows >= 2, "the upload list has fewer than two rows");
        QVERIFY2(sync.rows >= 2, "the sync list has fewer than two rows");

        // ---- THE ANSWERS, PINNED. Measured 2026-08-22, identical under both
        //      QPA backends.
        //
        // (i) WHERE THE STATUS COLUMN IS, read off the live widgets rather than
        //     off the constructor: 5 on the download list (whose columnCount is
        //     6, so "Workout Id" at index 6 is TRUNCATED AWAY and Status is the
        //     LAST column), 7 on the upload list, 7 on the sync list (whose
        //     columnCount is 8, so its "Workout Id" at index 8 is truncated too).
        //     These are the same indices the drivers write to: :2563 / :3109 /
        //     :2283.
        QCOMPARE(down.statusColumn, 5);
        QCOMPARE(up.statusColumn, 7);
        QCOMPARE(sync.statusColumn, 7);
        for (int i = 0; i < 3; i++) {
            QVERIFY2(all[i]->statusColumnWithinCount, names[i]);
            QVERIFY2(!all[i]->statusColumnHidden, names[i]);
            // (ii) ...AND THE USER CAN SORT BY IT: sorting is on and the header
            //      sections are clickable, on all three, out of the constructor.
            QVERIFY2(all[i]->sortingEnabled, names[i]);
            QVERIFY2(all[i]->sectionsClickable, names[i]);
            QVERIFY2(all[i]->indicatorShown, names[i]);
            // (iii) AND THE DRIVER'S OWN STRING MOVES THE ROW. One setText with
            //       the literal the driver uses, on a list sorted by Status, and
            //       the row leaves index 0. MEASURED, NOT DESIRED.
            QVERIFY2(all[i]->movedByStatusWrite,
                     qPrintable(QStringLiteral("%1: the driver's status write did NOT move the row: [%2] -> [%3]")
                                    .arg(QString::fromLatin1(names[i]))
                                    .arg(all[i]->afterUserSort.join(QChar('|')))
                                    .arg(all[i]->afterStatusWrite.join(QChar('|')))));
        }
    }

    // -- TEST-131 (c) (S-R028-01) ----------------------------------------
    // Q5, HALF TWO: THE ORDINARY ROUTE, END TO END, WITH NO CLICK DURING THE
    // BATCH AT ALL.
    //
    // TEST-112 needed a reorder DELIVERED into the running batch (a header click,
    // in practice a setSortIndicator, inside row[0]'s nested loop). This run
    // delivers NOTHING. The user sorts by Status BEFORE pressing Synchronize -
    // an action with no relationship to the batch, taken at any earlier time -
    // and then only production runs. If the batch still damages itself, the sort
    // route does not need an untraced click: the drivers trip it themselves,
    // because every dispatch writes the sort column.
    //
    // A NEGATIVE HERE WOULD ALSO BE A RESULT and would be reported as one: it
    // would mean Q1's mechanism, real as it is, does not reach the drivers.
    void probeWhetherAPreBatchStatusSortSelfTriggersDuringTheBatch()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        bool timedOut = false;
        bool sortingEnabledOnSyncList = false;
        int statusColumn = -1, rows = 0, checkedRows = 0, writeFileCalls = 0;
        bool userSortChangedTheOrder = false, reorderedDuringTheBatch = false;
        QStringList before, afterUserSort, atFirstCompletion, atEnd, statusesAtEnd;
        QStringList reorderLog; // every reorder the BATCH caused, and when
        QString writeNameAtFirstCompletion;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false;
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList(); // nothing remote: two UPLOAD rows on the sync tab
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                QTreeWidget* list = rideListWithHeader(dialog, QStringLiteral("Source"));

                auto fingerprints = [](QTreeWidget* w) {
                    QStringList out;
                    QTreeWidgetItem* root = w->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        QTreeWidgetItem* it = root->child(i);
                        out << QStringLiteral("%1@0x%2").arg(it->text(1)).arg(reinterpret_cast<quintptr>(it), 0, 16);
                    }
                    return out;
                };

                if (list != nullptr) {
                    sortingEnabledOnSyncList = list->isSortingEnabled();
                    rows = list->invisibleRootItem()->childCount();
                    for (int c = 0; c < list->columnCount(); c++)
                        if (list->headerItem()->text(c) == QStringLiteral("Status")) {
                            statusColumn = c;
                            break;
                        }
                    for (int i = 0; i < rows; i++) {
                        QCheckBox* check =
                            qobject_cast<QCheckBox*>(list->itemWidget(list->invisibleRootItem()->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            checkedRows++;
                    }
                    before = fingerprints(list);

                    // ---- THE USER'S ONLY ACTION, and it happens BEFORE the
                    //      batch: sort the list by its Status column. Every cell
                    //      in it is empty at this point, so this is invisible to
                    //      the user - it changes no row's position.
                    if (statusColumn >= 0)
                        list->header()->setSortIndicator(statusColumn, Qt::AscendingOrder);
                    afterUserSort = fingerprints(list);
                    userSortChangedTheOrder = (afterUserSort != before);

                    // WHEN the rows move, and what production had done by then.
                    // A reorder logged at writeFileCalls == 0 happened BEFORE the
                    // first upload was even dispatched - i.e. it can only be the
                    // curr->setText(7, "Uploading") at CloudService.cpp:2283 that
                    // caused it, not a completion, not a tail, and not this slot.
                    // Captured BY VALUE where the capture could outlive this
                    // frame: the connection can still fire while the dialog is
                    // being destroyed, after the enclosing lambda has returned.
                    // `reorderLog` is a local of the test function itself, which
                    // is blocked in appLoop.exec() throughout.
                    QObject::connect(list->model(), &QAbstractItemModel::layoutChanged, list,
                                     [&reorderLog, list, fingerprints]() {
                                         reorderLog << QStringLiteral("writeFileCalls=%1 order=%2")
                                                           .arg(obs::writeFileCalls)
                                                           .arg(fingerprints(list).join(QChar(',')));
                                     });
                }

                // The first completion's OWN frame, for the same reason TEST-112
                // samples there: read at the end and the re-driven batch has
                // already overwritten the evidence.
                store->afterCompletionAction = [&, list]() {
                    if (list == nullptr || !atFirstCompletion.isEmpty())
                        return;
                    writeNameAtFirstCompletion = obs::lastWriteName;
                    atFirstCompletion = fingerprints(list);
                };

                // ---- ...AND NOW ONLY PRODUCTION RUNS. No click, no
                //      setSortIndicator, no setText from this slot: whatever
                //      reorders the list from here is the dialog's own doing.
                dialog->downloadClicked();

                for (int i = 0; i < 200; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);

                writeFileCalls = obs::writeFileCalls;
                if (list != nullptr) {
                    atEnd = fingerprints(list);
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++)
                        statusesAtEnd << root->child(i)->text(statusColumn < 0 ? 7 : statusColumn);
                }

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);
                QTimer::singleShot(20000, &appLoop, [&timedOut]() {
                    timedOut = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        reorderedDuringTheBatch = (atFirstCompletion != afterUserSort || atEnd != afterUserSort);

        qInfo("TEST-131 Q5c [%s] statusColumn=%d rows=%d checked=%d userSortChangedOrder=%d before=[%s] "
              "afterUserSort=[%s] atFirstCompletion=[%s] atEnd=[%s] statuses=[%s] writeFileCalls=%d writeNames=[%s] "
              "firstCompletionWrote=%s reorders=[%s]",
              qPrintable(QString::fromLatin1(qgetenv("QT_QPA_PLATFORM"))), statusColumn, rows, checkedRows,
              int(userSortChangedTheOrder), qPrintable(before.join(QChar('|'))),
              qPrintable(afterUserSort.join(QChar('|'))), qPrintable(atFirstCompletion.join(QChar('|'))),
              qPrintable(atEnd.join(QChar('|'))), qPrintable(statusesAtEnd.join(QChar('|'))), writeFileCalls,
              qPrintable(obs::writeNames.join(QChar('|'))), qPrintable(writeNameAtFirstCompletion),
              qPrintable(reorderLog.join(QStringLiteral(" ;; "))));

        // ---- THE PREMISES.
        QVERIFY2(timedOut == false, "the run never came back");
        QVERIFY2(sortingEnabledOnSyncList, "the sync list is not sortable, so this whole slot is about nothing");
        QCOMPARE(statusColumn, 7);
        QCOMPARE(rows, 2);
        QCOMPARE(checkedRows, 2);
        // The user's sort was INVISIBLE: it moved nothing, because every Status
        // cell was empty. Nobody could tell from the screen that this list is
        // now sorted on a column production is about to write to.
        QVERIFY2(!userSortChangedTheOrder,
                 qPrintable(QStringLiteral("the pre-batch sort itself reordered the rows ([%1] -> [%2]), so what "
                                           "follows cannot be attributed to the drivers")
                                .arg(before.join(QChar('|')))
                                .arg(afterUserSort.join(QChar('|')))));
        QVERIFY2(writeFileCalls >= 1, "no transfer happened at all, so this run says nothing about the batch");

        // ---- THE ANSWER, PINNED. REWRITTEN 2026-08-23 UNDER DEC-garmin-038.
        //
        // WHAT THIS BLOCK USED TO SAY, because it is the whole reason the fix
        // exists and deleting it would delete the evidence. Measured 2026-08-22
        // under both QPA backends, with NO interaction during the batch at all:
        //
        //   afterUserSort          [10:00, 11:00]   (the sort itself moved nothing)
        //   atFirstCompletion      [11:00, 10:00]   <- the BATCH reordered itself
        //   first layout change    at writeFileCalls == 0, i.e. inside syncNext's
        //                          own curr->setText(7, "Uploading"), before the
        //                          first dispatch
        //   writeNames             [10_00.json.zip, 10_00.json.zip]  (twice!)
        //   statuses               ["", "Completed."]  (11:00 never transferred)
        //
        // That block was labelled MEASURED, NOT DESIRED and said in as many words
        // that whoever closed S-R028-01 should expect it to change. DEC-038 closed
        // it: sorting is suspended for the batch's duration, so the driver's own
        // status write moves nothing and the positional walk stays valid. The pins
        // below are the DESIRED values - a green here is now a statement about
        // production being right, not about it being broken.
        //
        // (i) THE BATCH DOES NOT REORDER ITS OWN LIST. Same order at the first
        //     completion and at the end as the user's pre-batch sort left it.
        QVERIFY2(!reorderedDuringTheBatch,
                 qPrintable(QStringLiteral("the list reordered under the running batch ([%1] -> [%2] -> [%3]) - the "
                                           "self-trigger is back")
                                .arg(afterUserSort.join(QChar('|')))
                                .arg(atFirstCompletion.join(QChar('|')))
                                .arg(atEnd.join(QChar('|')))));
        // (ii) ...AND SPECIFICALLY NOT AT THE STATUS WRITE. The layout-change log
        //      is kept rather than replaced by the inequality above, because it is
        //      what LOCATES a regression: an entry at writeFileCalls == 0 is
        //      syncNext's own curr->setText(7, "Uploading") sorting the list
        //      before it has dispatched anything, which is the defect itself. The
        //      log is NOT required to be empty - restoring the user's sorting at
        //      the completion tail is a layout change too, and it happens after
        //      the last transfer (writeFileCalls == 2).
        for (const QString& entry : reorderLog)
            QVERIFY2(!entry.startsWith(QStringLiteral("writeFileCalls=0 ")),
                     qPrintable(
                         QStringLiteral("a status write reordered the list before the first dispatch: %1").arg(entry)));
        // (iii) AND EACH CHECKED ROW IS TRANSFERRED EXACTLY ONCE. Two writeFile
        //       calls for two checked rows, of two DIFFERENT rows: the count alone
        //       read correct even when the batch was uploading one activity twice.
        QCOMPARE(writeFileCalls, 2);
        QCOMPARE(obs::writeNames.count(), 2);
        QVERIFY2(obs::writeNames.value(0) != obs::writeNames.value(1),
                 qPrintable(QStringLiteral("the two uploads were of the SAME row (%1, %2) - a pre-batch sort still "
                                           "costs a double transfer")
                                .arg(obs::writeNames.value(0))
                                .arg(obs::writeNames.value(1))));
        // ...and the row that was never transferred before now carries a verdict.
        QCOMPARE(statusesAtEnd, QStringList() << "Completed." << "Completed.");
    }

  private:
    // =====================================================================
    // TEST-132 … TEST-139 (S-R028-01 / S-R028-02, DEC-garmin-038 =
    // DEC-garmin-034 AMENDMENT) — SORTING IS OFF FOR THE BATCH'S DURATION.
    // =====================================================================
    //
    // WHAT IS BEING TESTED, AND WHY THE PROBES ABOVE ARE NOT IT. TEST-131
    // MEASURED the route: with sortingEnabled == true a setText on the SORT
    // COLUMN reorders the list INSIDE the write (Q1b, layoutChanged at write == 1,
    // no lazy window), all three lists are sortable by their Status column out of
    // the constructor (Q5), and a user who sorts by Status BEFORE pressing
    // Synchronize therefore makes the batch reorder its own list underneath the
    // positional `for (int i=listindex; …) child(i)` walk in all three drivers -
    // with NO click during the batch at all (Q5c: the same row uploaded twice, the
    // other never transferred). DEC-garmin-038 chose Option A: disable sorting on
    // all three lists for the batch's duration and restore it on every
    // termination path, preserving the user's column and order.
    //
    // THESE EIGHT SLOTS ARE THE RULE, not a trace. Each one asserts what
    // production must DO, and the two probes above (TEST-112, TEST-131 (c)) had
    // their MEASURED-NOT-DESIRED blocks rewritten to the desired values in the
    // same commit, for the reason those blocks themselves gave.
    //
    // THE FIVE THINGS THE DECISION MAKES CONDITIONS, and where each is measured:
    //
    //   1. the disable is in place BEFORE the batch's first status write
    //      -> reordersAtDispatch == 0 and sortingAtDispatch == [0,0,0]
    //         (TEST-132/133/134). Placement, not existence: a disable arriving
    //         after a write cannot undo the move that write already made (Q3).
    //   2. sorting is restored on EVERY termination path
    //      -> TEST-136, one run per termination class.
    //   3. all three drivers can neither duplicate nor skip a row
    //      -> TEST-132 (syncNext), TEST-133 (uploadNext), TEST-134
    //         (downloadNext), each asserting that the set of rows TRANSFERRED is
    //         exactly the set of rows CHECKED, with no repeats. Three slots and
    //         not three arms of one, because this ledger has repeatedly shipped a
    //         fix on one of three twins (A3-R028b-F5).
    //   4. the user's sort column AND order survive the round trip
    //      -> TEST-137, with a DIFFERENT (section, order) pair pinned on each of
    //         the three lists so that a restore which copies one list's state onto
    //         another, or falls back on Qt's default section 0 / DESCENDING
    //         (measured at Q1a), cannot pass.
    //   5. the positive control, without which every criterion above is
    //      satisfiable by never turning sorting back on
    //      -> TEST-138 (LSN-050).
    //
    // ...and TEST-139 is the corollary the decision names as the obvious wrong
    // answer: a batch is NOT one stack frame, so an RAII guard around a driver
    // CALL would re-enable sorting BETWEEN ROWS - looking correct, and
    // reinstating the defect. It samples inside the first completion, one
    // transfer in and before the tail re-drives.
    //
    // ON THE `self.isNull()` EXITS (TEST-136's fourth case): the dialog is
    // already gone there, so there is nothing to restore and the restore must be
    // structured so that it cannot touch a dead dialog. What that case asserts is
    // therefore the ABSENCE of a touch - the run comes back, and this target
    // aborts on any ASan report (halt_on_error=1), so surviving IS the assertion -
    // plus the premise that the dialog really did die inside the batch.
    enum SortTab { SortDownloadTab = 0, SortUploadTab = 1, SortSyncTab = 2 };

    // What, if anything, the user does from INSIDE row[0]'s nested loop - i.e.
    // while the batch is live and one transfer is on the stack.
    enum SortDisturbance {
        NoDisturbance,        // TEST-132/133/134/137/138/139: production alone
        IndicatorInsideBatch, // TEST-135: the header-click vector (setSortIndicator)
        AbortInsideBatch,     // TEST-136: downloadClicked's abort branch
        RefreshInsideBatch,   // TEST-136: refreshClicked -> the stale-generation exits
        TeardownInsideBatch   // TEST-136: the dialog dies -> the self.isNull() exits
    };

    struct SortGuardSpec
    {
        SortTab tab = SortSyncTab;
        SortDisturbance disturb = NoDisturbance;

        // Does the user sort BEFORE pressing the button, and by what? The
        // defaults are the defect's own geometry: every list sorted by its Status
        // column, ascending - which is INVISIBLE at that moment, because every
        // Status cell is empty (TEST-131 Q5c).
        bool preBatchSort = true;
        int downSection = 5, upSection = 7, syncSection = 7;
        Qt::SortOrder downOrder = Qt::AscendingOrder;
        Qt::SortOrder upOrder = Qt::AscendingOrder;
        Qt::SortOrder syncOrder = Qt::AscendingOrder;

        // TEST-138: after the batch, does an indicator change actually MOVE rows
        // again - or is the header merely wearing the flags?
        bool probeSortingAfterTheBatch = false;

        // TEST-142 (A3-R038-F2) — ONE LIST IS ALREADY OUT OF SORTING WHEN THE
        // BATCH STARTS. Index into lists[] (0 down, 1 up, 2 sync); -1, the
        // default, leaves every run above byte-for-byte what it was.
        //
        // Why the fixture needs this at all: suspendListSorting's own early
        // return (CloudService.cpp:1559) means production can never RECORD an
        // already-disabled list of its own accord, so every other run in this
        // block starts from [1,1,1] and ends at [1,1,1] - and a restore that
        // REPLAYS what it recorded and one that unconditionally force-enables
        // produce the identical answer. This is the one dimension that tells
        // them apart, and it can only be created from outside production.
        int preDisableList = -1;
    };

    struct SortGuardOutcome
    {
        bool timedOut = false;

        // -- premises: was this a two-row batch on a sortable list at all?
        int rows = 0, checkedRows = 0, statusColumn = -1;
        QList<int> sortingBefore;           // 3 entries: down, up, sync
        QList<int> sortingAtBatchStart;     // ...and again AFTER spec.preDisableList
        bool preBatchSortMovedRows = false; // did the user's sort change the order?
        QStringList orderAfterUserSort;     // the driven list, name@address
        QStringList rowKeys;                // the identities the batch must transfer, one each

        // -- inside row[0]'s dispatch frame
        bool sampledAtDispatch = false;
        QList<int> sortingAtDispatch;
        int reordersAtDispatch = -1; // layout changes caused by the batch so far
        bool disturbanceDelivered = false;
        QList<int> sortingAfterDisturbance;
        bool indicatorMovedRowsInsideBatch = false;

        // -- inside the FIRST completion, before its tail re-drives the loop
        bool sampledBetweenRows = false;
        QList<int> sortingBetweenRows;
        int reordersBetweenRows = -1;
        int transfersAtBetweenRows = -1;
        QStringList orderBetweenRows;

        // -- the end state
        bool dialogDestroyed = false;
        QList<int> sortingAtEnd, clickableAtEnd, indicatorShownAtEnd, sectionAtEnd, orderAtEnd;
        QStringList orderAtEndFingerprints, namesAtEnd;
        bool sortingWorksAfterTheBatch = false;
        QStringList transferKeys, statusesAtEnd;
        int reordersInTotal = -1;
        QString progressText, buttonTextAtEnd;
    };

    // Names for the log, ADDRESSES for every comparison: a run that writes the
    // column it reads must never compare texts (TEST-131's instrument bug).
    static QStringList sortFingerprints(QTreeWidget* w)
    {
        QStringList out;
        if (w == nullptr)
            return out;
        QTreeWidgetItem* root = w->invisibleRootItem();
        for (int i = 0; i < root->childCount(); i++) {
            QTreeWidgetItem* it = root->child(i);
            out << QStringLiteral("%1@0x%2").arg(it->text(1)).arg(reinterpret_cast<quintptr>(it), 0, 16);
        }
        return out;
    }

    // TWO remote activities the Download tab lists and cannot parse, so that the
    // download arm is a two-row batch like the other two. .gcfail for TEST-119's
    // reason: FailingRideFileReader runs no nested loop of its own.
    static QStringList twoUnparseableRemoteActivities()
    {
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        return QStringList() << (day + QStringLiteral("_19_00_00.gcfail"))
                             << (day + QStringLiteral("_20_00_00.gcfail"));
    }

    // One run: a REAL sync dialog, a REAL two-row batch on the tab named by the
    // spec, with the user's sort applied BEFORE the button is pressed and nothing
    // touching the lists afterwards except (optionally) one disturbance delivered
    // from inside row[0]'s own nested loop.
    SortGuardOutcome runSortGuard(const SortGuardSpec& spec)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        SortGuardOutcome out;
        QEventLoop appLoop;

        // The download arm runs on REMOTE rows (readFile -> completedRead); the
        // upload and sync arms on LOCAL parseable rides (openRideFile ->
        // writeFile -> completedWrite).
        const bool remoteRows = (spec.tab == SortDownloadTab);

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        if (!remoteRows) {
            for (int i = 0; i < 2; i++) {
                const QString name = rebuildLocalActivity(i);
                QFile f(activities.absolutePath() + "/" + name);
                f.open(QIODevice::WriteOnly);
                f.write("gcblock");
                f.close();
                paths << f.fileName();

                RideItem* item = new RideItem(nullptr, context);
                item->fileName = name;
                item->path = activities.absolutePath();
                item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
                item->planned = false;
                items << item;
            }
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;
                QPointer<QWidget> ownerGuard(owner);

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = remoteRows ? twoUnparseableRemoteActivities() : QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;
                store->completeRead = true;
                store->completeWrite = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;
                QPointer<CloudServiceSyncDialog> dialogGuard(dialog);

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(int(spec.tab));

                // QPointers throughout: the teardown case destroys the dialog and
                // every one of these widgets from inside the batch.
                QPointer<QTreeWidget> lists[3];
                lists[SortDownloadTab] = rideListWithHeader(dialog, QStringLiteral("Workout Name"));
                lists[SortUploadTab] = rideListWithHeader(dialog, QStringLiteral("File"));
                lists[SortSyncTab] = rideListWithHeader(dialog, QStringLiteral("Source"));

                QString buttonLabel;
                switch (spec.tab) {
                case SortDownloadTab:
                    dialog->selectAllChanged(Qt::Checked);
                    buttonLabel = QStringLiteral("Download");
                    break;
                case SortUploadTab:
                    dialog->selectAllUpChanged(Qt::Checked);
                    buttonLabel = QStringLiteral("Upload");
                    break;
                case SortSyncTab:
                    dialog->selectAllSyncChanged(Qt::Checked);
                    buttonLabel = QStringLiteral("Synchronize");
                    break;
                }
                QPushButton* button = pushButtonWithText(dialog, buttonLabel);
                QTreeWidget* driven = lists[spec.tab].data();

                auto sortingStates = [&lists]() {
                    QList<int> s;
                    for (int i = 0; i < 3; i++)
                        s << (lists[i].isNull() ? -1 : int(lists[i]->isSortingEnabled()));
                    return s;
                };

                out.sortingBefore = sortingStates();

                if (driven != nullptr) {
                    QTreeWidgetItem* root = driven->invisibleRootItem();
                    out.rows = root->childCount();
                    for (int i = 0; i < out.rows; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(driven->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                        // The identity a transfer of this row will report: the
                        // download path passes curr->text(1) to readFile, the two
                        // upload paths pass its baseName + uploadExtension().
                        out.rowKeys << QFileInfo(root->child(i)->text(1)).baseName();
                    }
                    // The Status column is found the way a user finds it, by its
                    // header text over the columns the list actually HAS.
                    for (int c = 0; c < driven->columnCount(); c++)
                        if (driven->headerItem() != nullptr &&
                            driven->headerItem()->text(c) == QStringLiteral("Status")) {
                            out.statusColumn = c;
                            break;
                        }
                }

                // ---- THE USER'S ONLY ACTION, and it happens BEFORE the batch.
                const QStringList orderBeforeUserSort = sortFingerprints(driven);
                if (spec.preBatchSort) {
                    const int sections[3] = {spec.downSection, spec.upSection, spec.syncSection};
                    const Qt::SortOrder orders[3] = {spec.downOrder, spec.upOrder, spec.syncOrder};
                    for (int i = 0; i < 3; i++)
                        if (!lists[i].isNull() && lists[i]->header() != nullptr)
                            lists[i]->header()->setSortIndicator(sections[i], orders[i]);
                }
                // TEST-142 — ...and, for the one run that asks it, ONE list is
                // put out of sorting before the button is pressed. AFTER the sort
                // above, so the list carries a real (section, order) as well as a
                // real `enabled` for suspendListSorting to record. Synthetic by
                // necessity: this dialog offers the user no way to reach it, and
                // production's own suspend can never create it (:1559).
                if (spec.preDisableList >= 0 && spec.preDisableList < 3 && !lists[spec.preDisableList].isNull())
                    lists[spec.preDisableList]->setSortingEnabled(false);
                out.sortingAtBatchStart = sortingStates();

                out.orderAfterUserSort = sortFingerprints(driven);
                out.preBatchSortMovedRows = (out.orderAfterUserSort != orderBeforeUserSort);

                // Every layout change the BATCH causes, counted. Connected after
                // the user's own sort, so it counts nothing this fixture did. Held
                // by value because the connection can outlive this frame.
                auto reorders = std::make_shared<int>(0);
                if (driven != nullptr)
                    QObject::connect(driven->model(), &QAbstractItemModel::layoutChanged, driven,
                                     [reorders]() { (*reorders)++; });

                // ---- INSIDE ROW[0]'s DISPATCH FRAME: the status write has been
                //      made and the transfer is on the stack.
                const auto insideRow0 = [&, owner]() {
                    if (out.sampledAtDispatch)
                        return;
                    out.sampledAtDispatch = true;
                    out.sortingAtDispatch = sortingStates();
                    out.reordersAtDispatch = *reorders;

                    switch (spec.disturb) {
                    case IndicatorInsideBatch: {
                        QTreeWidget* w = lists[spec.tab].data();
                        if (w == nullptr || w->header() == nullptr)
                            break;
                        // The call QHeaderView makes for itself on a real mouse
                        // release, which TEST-112 measured as the one delivery
                        // that reorders under both of this target's QPA backends.
                        const QStringList before = sortFingerprints(w);
                        w->header()->setSortIndicator(out.statusColumn < 0 ? 7 : out.statusColumn, Qt::DescendingOrder);
                        out.indicatorMovedRowsInsideBatch = (sortFingerprints(w) != before);
                        out.disturbanceDelivered = true;
                        break;
                    }
                    case AbortInsideBatch:
                        if (!dialogGuard.isNull()) {
                            dialogGuard->downloadClicked(); // the abort branch
                            out.disturbanceDelivered = true;
                        }
                        break;
                    case RefreshInsideBatch:
                        if (!dialogGuard.isNull()) {
                            dialogGuard->refreshClicked(); // every row deleted, then rebuilt
                            out.disturbanceDelivered = true;
                        }
                        break;
                    case TeardownInsideBatch:
                        delete owner; // the dialog dies under the suspended driver
                        out.disturbanceDelivered = true;
                        break;
                    case NoDisturbance:
                        break;
                    }

                    out.sortingAfterDisturbance = sortingStates();
                };

                // ---- INSIDE THE FIRST COMPLETION, after it has labelled its row
                //      and BEFORE its tail re-drives the loop. This is the
                //      between-rows point: the driver frame that dispatched row[0]
                //      has RETURNED and the one for row[1] does not exist yet.
                store->afterCompletionAction = [&]() {
                    if (out.sampledBetweenRows)
                        return;
                    out.sampledBetweenRows = true;
                    out.sortingBetweenRows = sortingStates();
                    out.reordersBetweenRows = *reorders;
                    out.transfersAtBetweenRows = remoteRows ? obs::readFileCalls : obs::writeFileCalls;
                    out.orderBetweenRows = sortFingerprints(lists[spec.tab].data());
                };

                if (remoteRows) {
                    store->blockingMs = 200;
                    store->closeAction = insideRow0;
                } else {
                    rideopen::blockingMs = 200;
                    rideopen::action = insideRow0;
                }

                // ---- ...AND NOW ONLY PRODUCTION RUNS.
                dialog->downloadClicked();

                for (int i = 0; i < 200; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);

                out.reordersInTotal = *reorders;
                for (const QString& name : (remoteRows ? obs::readNames : obs::writeNames))
                    out.transferKeys << QFileInfo(name).baseName();

                out.dialogDestroyed = dialogGuard.isNull();
                if (!out.dialogDestroyed) {
                    out.sortingAtEnd = sortingStates();
                    for (int i = 0; i < 3; i++) {
                        QHeaderView* h = lists[i].isNull() ? nullptr : lists[i]->header();
                        out.clickableAtEnd << (h == nullptr ? -1 : int(h->sectionsClickable()));
                        out.indicatorShownAtEnd << (h == nullptr ? -1 : int(h->isSortIndicatorShown()));
                        out.sectionAtEnd << (h == nullptr ? -99 : h->sortIndicatorSection());
                        out.orderAtEnd << (h == nullptr ? -1 : int(h->sortIndicatorOrder()));
                    }
                    QTreeWidget* w = lists[spec.tab].data();
                    out.orderAtEndFingerprints = sortFingerprints(w);
                    if (w != nullptr) {
                        QTreeWidgetItem* root = w->invisibleRootItem();
                        for (int i = 0; i < root->childCount(); i++) {
                            out.namesAtEnd << root->child(i)->text(1);
                            out.statusesAtEnd << root->child(i)->text(out.statusColumn < 0 ? 7 : out.statusColumn);
                        }
                    }
                    out.progressText = progressLabelText(dialog);
                    if (button != nullptr)
                        out.buttonTextAtEnd = button->text();

                    // TEST-138: is the header WORKING again, or merely wearing the
                    // flags? An indicator change on a column whose values differ
                    // must move rows.
                    if (spec.probeSortingAfterTheBatch && w != nullptr && w->header() != nullptr) {
                        const QStringList before = sortFingerprints(w);
                        w->header()->setSortIndicator(1, Qt::DescendingOrder);
                        const QStringList afterDesc = sortFingerprints(w);
                        w->header()->setSortIndicator(1, Qt::AscendingOrder);
                        const QStringList afterAsc = sortFingerprints(w);
                        out.sortingWorksAfterTheBatch = (afterDesc != before || afterAsc != afterDesc);
                    }
                }

                QTimer::singleShot(100, qApp, [ownerGuard]() {
                    if (!ownerGuard.isNull())
                        delete ownerGuard.data();
                });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    static QString sortStates(const QList<int>& v)
    {
        QStringList out;
        for (int i : v)
            out << QString::number(i);
        return out.join(QChar(','));
    }

    void logSortGuard(const QString& where, const SortGuardOutcome& out)
    {
        qInfo("%s [%s] rows=%d checked=%d statusCol=%d before=[%s] atBatchStart=[%s] userSortMoved=%d atDispatch=[%s] "
              "reordersAtDispatch=%d "
              "betweenRows=[%s] reordersBetween=%d transfersAtBetween=%d afterDisturbance=[%s] atEnd=[%s] "
              "clickable=[%s] indicator=[%s] section=[%s] order=[%s] reorders=%d transfers=[%s] statuses=[%s] "
              "names=[%s] progress=\"%s\" button=\"%s\" destroyed=%d",
              qPrintable(where), qPrintable(QString::fromLatin1(qgetenv("QT_QPA_PLATFORM"))), out.rows, out.checkedRows,
              out.statusColumn, qPrintable(sortStates(out.sortingBefore)),
              qPrintable(sortStates(out.sortingAtBatchStart)), int(out.preBatchSortMovedRows),
              qPrintable(sortStates(out.sortingAtDispatch)), out.reordersAtDispatch,
              qPrintable(sortStates(out.sortingBetweenRows)), out.reordersBetweenRows, out.transfersAtBetweenRows,
              qPrintable(sortStates(out.sortingAfterDisturbance)), qPrintable(sortStates(out.sortingAtEnd)),
              qPrintable(sortStates(out.clickableAtEnd)), qPrintable(sortStates(out.indicatorShownAtEnd)),
              qPrintable(sortStates(out.sectionAtEnd)), qPrintable(sortStates(out.orderAtEnd)), out.reordersInTotal,
              qPrintable(out.transferKeys.join(QChar('|'))), qPrintable(out.statusesAtEnd.join(QChar('|'))),
              qPrintable(out.namesAtEnd.join(QChar('|'))), qPrintable(out.progressText),
              qPrintable(out.buttonTextAtEnd), int(out.dialogDestroyed));
    }

    // The premises every run of the fixture shares. A run that never reached the
    // situation must fail LOUDLY rather than pass on nothing (LSN-047, LSN-050).
    void assertSortGuardPremises(const QString& where, const SortGuardOutcome& out, int expectedStatusColumn)
    {
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the run never came back")));
        QVERIFY2(out.rows == 2, qPrintable(where + QStringLiteral("the list held %1 rows, not 2").arg(out.rows)));
        QVERIFY2(out.checkedRows == 2,
                 qPrintable(where + QStringLiteral("%1 rows were checked, not 2").arg(out.checkedRows)));
        QVERIFY2(out.statusColumn == expectedStatusColumn,
                 qPrintable(where + QStringLiteral("the Status column is %1, not %2 - this run is about some other "
                                                   "column than the one the drivers write")
                                        .arg(out.statusColumn)
                                        .arg(expectedStatusColumn)));
        QVERIFY2(out.rowKeys.count() == 2 && out.rowKeys.value(0) != out.rowKeys.value(1),
                 qPrintable(where + QStringLiteral("the two rows are not distinguishable ([%1]), so \"the same row "
                                                   "twice\" and \"each row once\" would read alike")
                                        .arg(out.rowKeys.join(QChar('|')))));
        // The route exists at all: all three lists come out of the constructor
        // sortable (:1116/:1146/:1186), which is what makes the pre-batch sort a
        // thing an ordinary user can do.
        QVERIFY2(out.sortingBefore == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where + QStringLiteral("the three lists were not all sortable before the batch ([%1]), so "
                                                   "this run is about nothing")
                                        .arg(sortStates(out.sortingBefore))));
        QVERIFY2(out.sampledAtDispatch,
                 qPrintable(where + QStringLiteral("row[0]'s dispatch frame was never sampled - the batch never "
                                                   "reached a transfer, so this run proves nothing")));
    }

    // Condition 3, for one driver: the set of rows TRANSFERRED is exactly the set
    // of rows CHECKED, each of them once. A count alone cannot tell "two rows, one
    // each" from "one row, twice" - which is precisely the damage TEST-131 Q5c
    // measured - so the names are compared, sorted, as multisets.
    void assertEveryCheckedRowTransferredExactlyOnce(const QString& where, const SortGuardOutcome& out)
    {
        QStringList transferred = out.transferKeys;
        QStringList expected = out.rowKeys;
        transferred.sort();
        expected.sort();
        QVERIFY2(transferred == expected,
                 qPrintable(where + QStringLiteral("the batch transferred [%1] for the checked rows [%2] - a row was "
                                                   "duplicated or skipped, which is the sort route's own damage")
                                        .arg(out.transferKeys.join(QChar('|')))
                                        .arg(out.rowKeys.join(QChar('|')))));
        QVERIFY2(out.statusesAtEnd.count() == 2 && !out.statusesAtEnd.value(0).isEmpty() &&
                     !out.statusesAtEnd.value(1).isEmpty(),
                 qPrintable(where + QStringLiteral("a checked row ended the batch with no status at all: [%1] - it was "
                                                   "never transferred")
                                        .arg(out.statusesAtEnd.join(QChar('|')))));
    }

    // Conditions 1 and the mechanism behind it: sorting is OFF on ALL THREE lists
    // inside the dispatch frame, and the batch's first status write moved nothing.
    void assertSortingWasOffAtTheFirstWrite(const QString& where, const SortGuardOutcome& out)
    {
        QVERIFY2(out.sortingAtDispatch == (QList<int>() << 0 << 0 << 0),
                 qPrintable(where + QStringLiteral("sorting was still enabled inside row[0]'s dispatch: [%1] (down, "
                                                   "up, sync) - the guard is missing or is not on all three lists")
                                        .arg(sortStates(out.sortingAtDispatch))));
        QVERIFY2(out.reordersAtDispatch == 0,
                 qPrintable(where + QStringLiteral("the batch's first status write reordered the list (%1 layout "
                                                   "change(s) before the transfer) - a disable that arrives after the "
                                                   "write cannot undo the move it already made (TEST-131 Q3)")
                                        .arg(out.reordersAtDispatch)));
    }

  private slots:
    // -- TEST-132 (S-R028-01, DEC-garmin-038) ----------------------------
    // syncNext: THE SELF-TRIGGER ROUTE, WITH NO CLICK AT ANY POINT.
    //
    // TEST-131 Q5c's geometry exactly: the user sorts the Sync list by Status
    // before pressing Synchronize - invisible, because every Status cell is empty
    // - and then only production runs. Before DEC-038 this produced
    // writeNames=[…10_00.json.zip, …10_00.json.zip]: the same row uploaded twice
    // and the other never transferred.
    //
    // RED, before the fix:
    //   FAIL!  : ... syncNext: sorting was still enabled inside row[0]'s dispatch:
    //            [0,1,1] (down, up, sync)
    void aPreBatchStatusSortMustNotMakeSyncNextTransferARowTwice()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-132 syncNext: ");
        logSortGuard(QStringLiteral("TEST-132"), out);

        assertSortGuardPremises(where, out, 7);
        // The user's sort was INVISIBLE: it moved nothing, because every Status
        // cell was empty. Nobody could tell from the screen that this list is now
        // sorted on a column production is about to write to.
        QVERIFY2(!out.preBatchSortMovedRows,
                 qPrintable(where + QStringLiteral("the pre-batch sort itself reordered the rows, so what follows "
                                                   "cannot be attributed to the drivers")));

        assertSortingWasOffAtTheFirstWrite(where, out);
        QVERIFY2(out.reordersInTotal == 0 || out.sampledBetweenRows,
                 qPrintable(where + QStringLiteral("the batch never reached a completion")));
        QVERIFY2(out.orderBetweenRows == out.orderAfterUserSort,
                 qPrintable(where + QStringLiteral("the list reordered under the running batch: [%1] -> [%2]")
                                        .arg(out.orderAfterUserSort.join(QChar('|')))
                                        .arg(out.orderBetweenRows.join(QChar('|')))));
        assertEveryCheckedRowTransferredExactlyOnce(where, out);
    }

    // -- TEST-133 (S-R028-01, DEC-garmin-038) ----------------------------
    // uploadNext: THE UPLOAD TWIN. Same geometry, same criterion, its own run -
    // three drivers, three slots (A3-R028b-F5).
    //
    // RED, before the fix:
    //   FAIL!  : ... uploadNext: sorting was still enabled inside row[0]'s
    //            dispatch: [0,1,1] (down, up, sync)
    void aPreBatchStatusSortMustNotMakeUploadNextTransferARowTwice()
    {
        SortGuardSpec spec;
        spec.tab = SortUploadTab;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-133 uploadNext: ");
        logSortGuard(QStringLiteral("TEST-133"), out);

        assertSortGuardPremises(where, out, 7);
        QVERIFY2(!out.preBatchSortMovedRows,
                 qPrintable(where + QStringLiteral("the pre-batch sort itself reordered the rows")));
        assertSortingWasOffAtTheFirstWrite(where, out);
        QVERIFY2(out.orderBetweenRows == out.orderAfterUserSort,
                 qPrintable(where + QStringLiteral("the list reordered under the running batch: [%1] -> [%2]")
                                        .arg(out.orderAfterUserSort.join(QChar('|')))
                                        .arg(out.orderBetweenRows.join(QChar('|')))));
        assertEveryCheckedRowTransferredExactlyOnce(where, out);
    }

    // -- TEST-134 (S-R028-01, DEC-garmin-038) ----------------------------
    // downloadNext: THE DOWNLOAD TWIN, whose status column is 5 rather than 7 and
    // whose transfer is a readFile rather than a writeFile. Its rows are the
    // unparseable remote activities TEST-119 uses, so the batch runs to its tail
    // without a saveRide in the way.
    //
    // RED, before the fix:
    //   FAIL!  : ... downloadNext: sorting was still enabled inside row[0]'s
    //            dispatch: [0,1,1] - and downloadClicked disables the DOWNLOAD
    //            list only.
    void aPreBatchStatusSortMustNotMakeDownloadNextTransferARowTwice()
    {
        SortGuardSpec spec;
        spec.tab = SortDownloadTab;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-134 downloadNext: ");
        logSortGuard(QStringLiteral("TEST-134"), out);

        assertSortGuardPremises(where, out, 5);
        QVERIFY2(!out.preBatchSortMovedRows,
                 qPrintable(where + QStringLiteral("the pre-batch sort itself reordered the rows")));
        assertSortingWasOffAtTheFirstWrite(where, out);
        QVERIFY2(out.orderBetweenRows == out.orderAfterUserSort,
                 qPrintable(where + QStringLiteral("the list reordered under the running batch: [%1] -> [%2]")
                                        .arg(out.orderAfterUserSort.join(QChar('|')))
                                        .arg(out.orderBetweenRows.join(QChar('|')))));
        assertEveryCheckedRowTransferredExactlyOnce(where, out);
    }

    // -- TEST-135 (S-R028-01, DEC-garmin-038) ----------------------------
    // THE ORIGINAL SHAPE: A SORT DELIVERED INTO THE RUNNING BATCH.
    //
    // This is TEST-112's route - setSortIndicator, the call QHeaderView makes for
    // itself on a real mouse release, delivered from inside row[0]'s openRideFile
    // - asserted as a RULE rather than traced. TEST-131 Q4 measured that
    // setSortingEnabled(false) severs BOTH vectors (clickable=0, indicatorShown=0,
    // indicatorReorders=0), so with the guard in place the delivered sort must
    // move nothing and the batch must still transfer each row exactly once.
    //
    // RED, before the fix:
    //   FAIL!  : ... the sort delivered into the running batch MOVED rows
    void aSortDeliveredIntoARunningBatchMustNotMoveARow()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        spec.disturb = IndicatorInsideBatch;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-135 header vector: ");
        logSortGuard(QStringLiteral("TEST-135"), out);

        assertSortGuardPremises(where, out, 7);
        QVERIFY2(out.disturbanceDelivered,
                 qPrintable(where + QStringLiteral("the sort was never delivered into the batch")));
        assertSortingWasOffAtTheFirstWrite(where, out);
        QVERIFY2(!out.indicatorMovedRowsInsideBatch,
                 qPrintable(where + QStringLiteral("the sort delivered into the running batch MOVED rows - the header "
                                                   "vector is still open")));
        assertEveryCheckedRowTransferredExactlyOnce(where, out);
        // ...and the vector being shut is not permanent: the header is live again
        // once the batch is over.
        QVERIFY2(out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where + QStringLiteral("the lists were left unsortable after the batch: [%1]")
                                        .arg(sortStates(out.sortingAtEnd))));
    }

    // -- TEST-136 (S-R028-02, DEC-garmin-038) ----------------------------
    // THE RESTORE-ON-EVERY-EXIT MATRIX: one case per TERMINATION CLASS.
    //
    // "A path that exits without restoring leaves the user's lists permanently
    // unsortable until the dialog is reopened - a silent, shipped UX regression
    // that no memory-safety test would ever catch." Four classes:
    //
    //   normal tail        the three completion tails
    //   abort              downloadClicked's abort branch
    //   stale generation   a Refresh delivered mid-batch: every driver and every
    //                      completion then stands down, and NOTHING re-drives, so
    //                      the restore cannot be left to a tail that never runs
    //   self.isNull()      the dialog is already gone - there is nothing to
    //                      restore, and the restore must not touch a dead dialog
    //
    // RED, before the fix (the stale-generation case, on the DOWNLOAD tab, which
    // is the one list downloadClicked already disabled):
    //   FAIL!  : ... refresh: the lists were left unsortable: [0,1,1]
    void sortingMustBeRestoredOnEveryTerminationPath()
    {
        // ---- (1) THE NORMAL TAIL - ALL THREE OF THEM. syncNext's, downloadNext's
        //      and uploadNext's are three separate copies of the release in three
        //      separate functions, so they are three separate runs: this ledger
        //      has repeatedly shipped a fix on one of three twins, and a matrix
        //      that exercised one tail would leave the other two unmeasured.
        {
            const SortTab tabs[3] = {SortSyncTab, SortUploadTab, SortDownloadTab};
            const char* names[3] = {"syncNext", "uploadNext", "downloadNext"};
            for (int t = 0; t < 3; t++) {
                SortGuardSpec spec;
                spec.tab = tabs[t];
                SortGuardOutcome out = runSortGuard(spec);
                const QString where = QStringLiteral("TEST-136 normal tail (%1): ").arg(QString::fromLatin1(names[t]));
                logSortGuard(QStringLiteral("TEST-136 normal tail %1").arg(QString::fromLatin1(names[t])), out);
                assertSortGuardPremises(where, out, tabs[t] == SortDownloadTab ? 5 : 7);
                assertSortingWasOffAtTheFirstWrite(where, out);
                // The tail's own sentence: it is the one string only the
                // completion tail writes, so this is how the run proves it took
                // THIS exit rather than standing down somewhere earlier.
                QVERIFY2(out.progressText.contains(QStringLiteral("successfully")),
                         qPrintable(where + QStringLiteral("the batch never reached its completion tail (progress "
                                                           "reads \"%1\"), so this case is about some other exit")
                                                .arg(out.progressText)));
                QVERIFY2(out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                         qPrintable(
                             where +
                             QStringLiteral("the lists were left unsortable: [%1]").arg(sortStates(out.sortingAtEnd))));
            }
        }

        // ---- (2) THE ABORT BRANCH, delivered from inside row[0]'s open. The
        //      restore must be in place the moment downloadClicked returns, not
        //      at some later tail - there is no later tail on this route.
        {
            SortGuardSpec spec;
            spec.tab = SortSyncTab;
            spec.disturb = AbortInsideBatch;
            SortGuardOutcome out = runSortGuard(spec);
            const QString where = QStringLiteral("TEST-136 abort: ");
            logSortGuard(QStringLiteral("TEST-136 abort"), out);
            assertSortGuardPremises(where, out, 7);
            QVERIFY2(out.disturbanceDelivered,
                     qPrintable(where + QStringLiteral("the abort was never delivered into the batch")));
            assertSortingWasOffAtTheFirstWrite(where, out);
            QVERIFY2(out.sortingAfterDisturbance == (QList<int>() << 1 << 1 << 1),
                     qPrintable(where + QStringLiteral("the abort branch did not restore sorting: [%1]")
                                            .arg(sortStates(out.sortingAfterDisturbance))));
            QVERIFY2(
                out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                qPrintable(where +
                           QStringLiteral("the lists were left unsortable: [%1]").arg(sortStates(out.sortingAtEnd))));
        }

        // ---- (3) THE STALE-GENERATION EXITS. A Refresh delivered mid-batch
        //      rebuilds every list; the suspended driver then stands down at its
        //      listGeneration compare and NO completion tail runs for this batch,
        //      so nothing downstream can restore. Run on the DOWNLOAD tab because
        //      that is the list downloadClicked already turned sorting off on
        //      before DEC-038 - the pre-existing half of this defect.
        {
            SortGuardSpec spec;
            spec.tab = SortDownloadTab;
            spec.disturb = RefreshInsideBatch;
            SortGuardOutcome out = runSortGuard(spec);
            const QString where = QStringLiteral("TEST-136 refresh: ");
            logSortGuard(QStringLiteral("TEST-136 refresh"), out);
            assertSortGuardPremises(where, out, 5);
            QVERIFY2(out.disturbanceDelivered,
                     qPrintable(where + QStringLiteral("the Refresh was never delivered into the batch")));
            assertSortingWasOffAtTheFirstWrite(where, out);
            QVERIFY2(out.sortingAfterDisturbance == (QList<int>() << 1 << 1 << 1),
                     qPrintable(where + QStringLiteral("the Refresh did not restore sorting: [%1]")
                                            .arg(sortStates(out.sortingAfterDisturbance))));
            QVERIFY2(
                out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                qPrintable(where +
                           QStringLiteral("the lists were left unsortable: [%1]").arg(sortStates(out.sortingAtEnd))));
        }

        // ---- (4) THE self.isNull() EXITS. The dialog is destroyed from inside
        //      row[0]'s own nested loop. There is nothing to restore - the lists
        //      died with the dialog - and the restore must be structured so that
        //      it cannot touch any of them. This target runs under ASan with
        //      halt_on_error=1, so coming back at all IS the assertion.
        {
            SortGuardSpec spec;
            spec.tab = SortSyncTab;
            spec.disturb = TeardownInsideBatch;
            SortGuardOutcome out = runSortGuard(spec);
            const QString where = QStringLiteral("TEST-136 teardown: ");
            logSortGuard(QStringLiteral("TEST-136 teardown"), out);
            QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the run never came back")));
            QVERIFY2(out.disturbanceDelivered,
                     qPrintable(where + QStringLiteral("the teardown was never delivered into the batch")));
            QVERIFY2(out.sampledAtDispatch,
                     qPrintable(where + QStringLiteral("row[0]'s dispatch frame was never sampled")));
            QVERIFY2(out.sortingAtDispatch == (QList<int>() << 0 << 0 << 0),
                     qPrintable(where + QStringLiteral("sorting was not suspended when the dialog died ([%1]), so this "
                                                       "case never exercised a restore-on-a-dead-dialog at all")
                                            .arg(sortStates(out.sortingAtDispatch))));
            QVERIFY2(out.dialogDestroyed,
                     qPrintable(where + QStringLiteral("the dialog survived the teardown, so no self.isNull() exit was "
                                                       "taken and this case proves nothing")));
        }
    }

    // -- TEST-137 (DEC-garmin-038, the user's condition 4) ---------------
    // THE USER'S SORT COLUMN AND ORDER SURVIVE THE ROUND TRIP.
    //
    // setSortingEnabled(true) re-applies whatever indicator the header is
    // carrying, and TEST-131 Q1a measured that enabling ALONE establishes section
    // 0 / DESCENDING - so a restore that simply turns sorting back on can silently
    // reset the user's chosen sort. A DIFFERENT (section, order) pair is pinned on
    // each of the three lists, none of them (0, Descending) and no two alike, so
    // that a restore which resets to Qt's default, or copies one list's state onto
    // another, cannot pass.
    void theUsersSortColumnAndOrderMustSurviveTheBatch()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        spec.downSection = 2;
        spec.downOrder = Qt::DescendingOrder;
        spec.upSection = 3;
        spec.upOrder = Qt::AscendingOrder;
        spec.syncSection = 1;
        spec.syncOrder = Qt::DescendingOrder;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-137 round trip: ");
        logSortGuard(QStringLiteral("TEST-137"), out);

        assertSortGuardPremises(where, out, 7);
        // The user's choice was VISIBLE this time: sorting the Sync list by its
        // Source column, descending, really does move the rows. A run in which it
        // did not would say nothing about an order being preserved.
        QVERIFY2(out.preBatchSortMovedRows,
                 qPrintable(where + QStringLiteral("the user's descending sort on column 1 moved nothing, so there is "
                                                   "no order here to preserve")));
        assertSortingWasOffAtTheFirstWrite(where, out);

        QVERIFY2(out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where +
                            QStringLiteral("the lists were left unsortable: [%1]").arg(sortStates(out.sortingAtEnd))));
        QVERIFY2(out.sectionAtEnd == (QList<int>() << 2 << 3 << 1),
                 qPrintable(where + QStringLiteral("the sort COLUMN was not preserved: [%1], expected [2,3,1] (down, "
                                                   "up, sync)")
                                        .arg(sortStates(out.sectionAtEnd))));
        QVERIFY2(out.orderAtEnd ==
                     (QList<int>() << int(Qt::DescendingOrder) << int(Qt::AscendingOrder) << int(Qt::DescendingOrder)),
                 qPrintable(where + QStringLiteral("the sort ORDER was not preserved: [%1], expected [1,0,1] (1 = "
                                                   "descending)")
                                        .arg(sortStates(out.orderAtEnd))));
        // ...and the indicator is not merely a decoration: the rows really are
        // still in the user's descending order on column 1.
        QStringList descending = out.namesAtEnd;
        std::sort(descending.begin(), descending.end());
        std::reverse(descending.begin(), descending.end());
        QVERIFY2(out.namesAtEnd == descending,
                 qPrintable(where + QStringLiteral("the rows are not in the user's descending order at the end: [%1]")
                                        .arg(out.namesAtEnd.join(QChar('|')))));
    }

    // -- TEST-138 (DEC-garmin-038, LSN-050) ------------------------------
    // THE POSITIVE CONTROL: THE LISTS ARE FULLY USABLE AGAIN AFTERWARDS.
    //
    // Without this slot every criterion above is satisfiable by disabling sorting
    // permanently and never turning it back on. A batch runs with NO pre-batch
    // sort and nothing delivered into it, and afterwards all three lists must be
    // sortable, their headers clickable, their indicators shown - and the header
    // must actually WORK: an indicator change on a column whose values differ has
    // to move rows, which "isSortingEnabled() == true" alone does not prove.
    //
    // The mid-batch premise is deliberate, and it makes this slot RED before the
    // fix as well: a control that passes on a build where nothing was ever
    // disabled is measuring nothing.
    void aBatchThatReordersNothingLeavesTheListsFullyUsable()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        spec.preBatchSort = false;
        spec.probeSortingAfterTheBatch = true;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-138 positive control: ");
        logSortGuard(QStringLiteral("TEST-138"), out);

        assertSortGuardPremises(where, out, 7);
        assertSortingWasOffAtTheFirstWrite(where, out);
        assertEveryCheckedRowTransferredExactlyOnce(where, out);

        QVERIFY2(out.sortingAtEnd == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where + QStringLiteral("the lists are not sortable after the batch: [%1]")
                                        .arg(sortStates(out.sortingAtEnd))));
        QVERIFY2(out.clickableAtEnd == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where + QStringLiteral("the headers are not clickable after the batch: [%1]")
                                        .arg(sortStates(out.clickableAtEnd))));
        QVERIFY2(out.indicatorShownAtEnd == (QList<int>() << 1 << 1 << 1),
                 qPrintable(where + QStringLiteral("the sort indicators are not shown after the batch: [%1]")
                                        .arg(sortStates(out.indicatorShownAtEnd))));
        QVERIFY2(out.sortingWorksAfterTheBatch,
                 qPrintable(where + QStringLiteral("an indicator change after the batch moved no rows - the header is "
                                                   "wearing the flags but the connection is severed")));
    }

    // -- TEST-139 (DEC-garmin-038, the corollary) ------------------------
    // THE DISABLE PERSISTS ACROSS THE CALLBACK-DRIVEN LOOP.
    //
    // "A batch is NOT one stack frame. These loops are process one row, return,
    // let the completion re-drive me - so an RAII scope guard around a driver call
    // would re-enable sorting BETWEEN ROWS and reinstate the defect while looking
    // correct." This slot samples at exactly that point: inside the FIRST
    // completion, after it has labelled its row and before its tail re-drives the
    // loop. The driver frame that dispatched row[0] has returned; the frame for
    // row[1] does not exist yet. An RAII-per-call design reads [1,1,1] here and
    // passes every other slot in this block.
    //
    // RED, before the fix:
    //   FAIL!  : ... sorting was enabled between rows: [0,1,1]
    void theDisableMustPersistBetweenRowsNotJustWithinADriverCall()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-139 between rows: ");
        logSortGuard(QStringLiteral("TEST-139"), out);

        assertSortGuardPremises(where, out, 7);
        QVERIFY2(out.sampledBetweenRows,
                 qPrintable(where + QStringLiteral("the first completion was never sampled, so nothing here is a "
                                                   "measurement")));
        // WHERE the sample was taken, measured rather than asserted: exactly one
        // transfer had been issued, so this is after row[0]'s and before row[1]'s.
        QVERIFY2(out.transfersAtBetweenRows == 1,
                 qPrintable(where + QStringLiteral("%1 transfer(s) had been issued at the sample, not 1 - the sample "
                                                   "is not between rows")
                                        .arg(out.transfersAtBetweenRows)));
        QVERIFY2(out.sortingBetweenRows == (QList<int>() << 0 << 0 << 0),
                 qPrintable(where + QStringLiteral("sorting was enabled between rows: [%1] (down, up, sync) - the "
                                                   "guard is per driver CALL rather than per BATCH")
                                        .arg(sortStates(out.sortingBetweenRows))));
        QVERIFY2(out.reordersBetweenRows == 0,
                 qPrintable(where + QStringLiteral("%1 layout change(s) had happened by the first completion")
                                        .arg(out.reordersBetweenRows)));
    }

    // -- TEST-142 (A3-R038-F2, DEC-garmin-038, REQ-028 (e)) --------------
    // THE RESTORE REPLAYS WHAT WAS RECORDED - IT DOES NOT FORCE-ENABLE.
    //
    // WHAT THE OTHER EIGHT SLOTS CANNOT SEE. suspendListSorting records each
    // list's own `enabled` bit (CloudService.cpp:1567) and restoreListSorting
    // plays that bit back (:1636). Every run above starts with all three lists
    // sorting - which is what the dialog's constructor leaves behind
    // (:1116/:1146/:1186) - and production's own suspend can never manufacture
    // any other starting state, because its early return at :1559 makes a second
    // suspension a no-op. So on this suite's whole 89-slot geometry the recorded
    // vector is [1,1,1], and
    //
    //     lists[i]->setSortingEnabled(states[i]->enabled);   // what is written
    //     lists[i]->setSortingEnabled(true);                 // a force-enable
    //
    // are INDISTINGUISHABLE: the second survives all 89 slots under both QPA
    // backends (measured, 2026-08-23). The field `ListSortState::enabled`
    // (CloudService.h:842) is therefore written, read, and never once observed
    // to carry anything but `true`.
    //
    // WHAT THIS RUN CHANGES. Exactly one thing: the UPLOAD list is put out of
    // sorting before the button is pressed, so the vector production records is
    // [1,0,1] rather than [1,1,1]. Nothing else about the run differs from
    // TEST-132's - same tab, same two-row sync batch, same pre-batch sort, no
    // disturbance. The disable is SYNTHETIC and openly so (no widget in this
    // dialog offers it); the criterion it makes measurable is not.
    //
    // BOTH HALVES ARE ASSERTED, AND THE SECOND IS WHAT MAKES THE FIRST MEAN
    // ANYTHING (LSN-050). "The disabled list comes back disabled" on its own is
    // satisfied by a restore that force-DISABLES all three - the exact bug
    // DEC-038's own positive control (TEST-138) exists to forbid - so the two
    // lists that were sorting must come back sorting in the same breath.
    //
    // RED, by mutation (2026-08-23, both backends) - AND WHAT IS, AND IS NOT,
    // EXCLUSIVE TO THIS SLOT. Both directions of :1636 were driven. This slot
    // dies on both, but only ONE of them is evidence FOR IT (A3-R028e-F3).
    //
    //   :1636 -> lists[i]->setSortingEnabled(true);      EXCLUSIVE TO THIS SLOT.
    //     Measured: 90 passed, 1 failed, EXIT=1 - the other 89 slots stay green
    //     and this one alone dies on
    //       FAIL!  : ... TEST-142 recorded state: the batch did not restore what
    //                it recorded: [1,1,1], expected [1,0,1] ...
    //     This IS the slot's unique contribution, and it is the whole reason the
    //     slot exists: this is the only run in the suite whose recorded vector is
    //     anything other than [1,1,1], so it is the only run that can tell a
    //     REPLAY of the recorded bit from a FORCE-ENABLE.
    //
    //   :1636 -> lists[i]->setSortingEnabled(false);     NOT EXCLUSIVE.
    //     Measured: 86 passed, 5 FAILED, EXIT=5. Four of those five PRE-DATE this
    //     slice and already catch it, each on [0,0,0]:
    //       TEST-135 aSortDeliveredIntoARunningBatchMustNotMoveARow
    //       TEST-136 sortingMustBeRestoredOnEveryTerminationPath
    //       TEST-137 theUsersSortColumnAndOrderMustSurviveTheBatch
    //       TEST-138 aBatchThatReordersNothingLeavesTheListsFullyUsable
    //     ...and TEST-142 with them. Removing this slot would not let that
    //     mutant live, so the kill says nothing about this slot in particular.
    //
    // Both measurements are kept above because both were run; what must not be
    // read off them is "killed twice, independently". Only the ->true direction
    // is exclusive evidence for TEST-142.
    void theRestoreMustReplayEachListsOwnRecordedSortingState()
    {
        SortGuardSpec spec;
        spec.tab = SortSyncTab;
        spec.preDisableList = SortUploadTab; // the list that is NOT driven
        SortGuardOutcome out = runSortGuard(spec);
        const QString where = QStringLiteral("TEST-142 recorded state: ");
        logSortGuard(QStringLiteral("TEST-142"), out);

        // The shared premises still hold, INCLUDING sortingBefore == [1,1,1]:
        // that is what makes the [1,0,1] below this fixture's doing and not the
        // dialog's.
        assertSortGuardPremises(where, out, 7);
        QVERIFY2(out.sortingAtBatchStart == (QList<int>() << 1 << 0 << 1),
                 qPrintable(where + QStringLiteral("the batch did not start from the state this run is about: [%1], "
                                                   "expected [1,0,1] (down, up, sync) - the synthetic disable did not "
                                                   "take, so there is nothing here to record")
                                        .arg(sortStates(out.sortingAtBatchStart))));

        // The batch really ran, and the suspension covered the list that was
        // ALREADY off as well as the two that were on.
        assertSortingWasOffAtTheFirstWrite(where, out);
        assertEveryCheckedRowTransferredExactlyOnce(where, out);
        QVERIFY2(!out.dialogDestroyed,
                 qPrintable(where + QStringLiteral("the dialog did not survive the batch, so no restore ran")));

        // ---- THE CRITERION, both halves in one comparison so that neither can
        //      be satisfied without the other: the list that was not sorting
        //      comes back NOT SORTING, and the two that were come back SORTING.
        QVERIFY2(out.sortingAtEnd == (QList<int>() << 1 << 0 << 1),
                 qPrintable(where + QStringLiteral("the batch did not restore what it recorded: [%1], expected [1,0,1] "
                                                   "(down, up, sync). [1,1,1] means the restore FORCE-ENABLES and the "
                                                   "recorded bit is dead; [0,0,0] means it left the lists disabled")
                                        .arg(sortStates(out.sortingAtEnd))));

        // ...and the header agrees with the widget, so this is a real state
        // rather than a flag nobody acted on: TEST-131 Q4 measured that a list
        // out of sorting has a header that is neither clickable nor carrying a
        // visible indicator, and the two lists that ARE sorting have both.
        QVERIFY2(out.clickableAtEnd == (QList<int>() << 1 << 0 << 1),
                 qPrintable(where + QStringLiteral("the headers do not match the restored sorting states: clickable "
                                                   "[%1], expected [1,0,1]")
                                        .arg(sortStates(out.clickableAtEnd))));
        QVERIFY2(out.indicatorShownAtEnd == (QList<int>() << 1 << 0 << 1),
                 qPrintable(where + QStringLiteral("the sort indicators do not match the restored sorting states: "
                                                   "[%1], expected [1,0,1]")
                                        .arg(sortStates(out.indicatorShownAtEnd))));
    }

  private:
    // =====================================================================
    // TEST-122 / TEST-123 (A3-R028b-F2, REQ-028 (b), DEC-garmin-034) —
    // THE REFRESH DELIVERED BY A DRIVER'S OWN PARSE-FAILURE processEvents().
    // =====================================================================
    //
    // THE WINDOW. Both drivers have exactly one branch that suspends and then
    // KEEPS ITERATING - the row whose local file will not parse:
    //
    //   syncNext                            uploadNext
    //   2426  progressBar->setValue(++dc)   3182  progressBar->setValue(++dc)
    //   2428  QApplication::processEvents() 3184  QApplication::processEvents()
    //   2429  if (self.isNull())  return    3191  if (self.isNull())  return
    //   2454  if (aborted)        return    3208  if (aborted)        return
    //   2461  if (batchGen != gen) return   3219  if (batchGen != gen) return
    //   2495  if (listGen != listgen) ret.  3235  if (listGen != listgen) ret. <- ADDED
    //   2497  continue;  -> loop head       3236  }  -> loop head
    //
    // Three questions are asked after that processEvents() and the fourth is not:
    // is this dialog alive, does the user still want this, is this still the live
    // BATCH - but not IS THIS STILL THE LIVE LIST. The frame's own snapshot of
    // listGeneration exists (`listgen`, :2167 / :3082) and is compared one branch
    // away (:2356 / :3133); on this route it is simply never read.
    //
    // WHAT THAT COSTS. The rebuilt rows are all UNCHECKED (refreshClicked builds
    // each row's QCheckBox fresh and never checks it), so the resumed loop finds
    // nothing to do and falls straight through to the completion tail - which
    // writes "Processed 0 of N successfully" and, in syncNext, calls
    // context->athlete->rideCache->save() one line later (:2524/:2527) for a batch
    // that no longer exists. That is A3-R027-F3 / PROBE-B's harm, which was
    // BLOCKING when it was first found, reached by a route the entry guards miss.
    //
    // THIS IS DELIBERATELY *NOT* A MEMORY-SAFETY DEFECT and must not be reported
    // as one: `curr` is re-fetched from `child(i)` on every iteration, so every
    // pointer the resumed loop touches is live. What is wrong is the VERDICT, not
    // the addressing - which is why the assertions below are the TEST-119/120
    // verdict shape (no tail sentence, no row of the rebuilt list labelled) and
    // not an ASan expectation.
    //
    // TWO SLOTS, NOT TWO ARMS IN ONE SLOT (A3-R028b-F5). TEST-120 put its upload
    // arm behind `if (QTest::currentTestFailed()) return;`, so a sync-arm failure
    // silently withdrew the third driver's coverage. These are separate slots so
    // that neither driver's coverage can be hidden by the other's failure.
    //
    // HOW THE DELIVERY IS MADE, AND WHY IT IS NOT A TIMING BET. insideframe, used
    // exactly as TEST-102 and TEST-119/120 use it: stage 1 is production's own
    // `progressBar->setValue(++downloadcounter)` on the line above, a DIRECT
    // connection, which anchors us inside the parse-failure branch of the right
    // iteration; stage 2 is the dispatcher's awake(), emitted as the first
    // statement of the very next processEvents() on this thread. NOTHING runs
    // between those two production statements. The rows are .gcfail on purpose
    // (TEST-102's reason): FailingRideFileReader runs no nested loop, so nothing
    // else on this path pumps events and "the next processEvents()" is the
    // branch's own. Both premises MEASURE that: the bar has counted exactly row 0
    // when the Refresh runs, and exactly one row has been offered to the reader.
    struct DriverRefreshOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the window it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        QString row0Action; // sync list column 6 — must be "Upload" to reach the branch
        bool refreshDelivered = false;
        int barValueAtRefresh = -1;      // production had counted this many rows
        int parseAttemptsAtRefresh = -1; // ...and offered this many to the reader
        int rowsBeforeRefresh = -1;
        int rowsAfterRefresh = -1;
        bool rowsWereFreed = false;

        // -- the verdict
        int parseAttemptsAtEnd = 0;
        int transfersAtEnd = 0;
        QString progressText; // the completion tail's "...successfully" sentence
        QString buttonTextAtEnd;
        int labelledRows = 0;
        QStringList statuses;
    };

    // Two LOCAL activities whose suffix FailingRideFileReader is registered for,
    // so RideFileFactory's dispatch on the suffix (RideFile.cpp:899-900) returns
    // NULL and the driver takes its parse-failure branch. Distinct times so the
    // row order is well defined; distinct from every other fixture's names so no
    // slot can inherit another's files.
    static QString driverRefreshActivity(int i)
    {
        return QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd")) +
               QStringLiteral("_0%1_30_00.gcfail").arg(i + 3);
    }

    DriverRefreshOutcome runRefreshInDriverParseFailure(bool uploadTab)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        DriverRefreshOutcome out;
        QEventLoop appLoop;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = driverRefreshActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcfail");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(3 + i, 30, 0));
            item->planned = false; // the upload list skips planned rides (:1723)
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                // No remote entries at all: every local ride is therefore an
                // UPLOAD row on the sync tab and a normal row on the upload tab.
                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->downloadCompression = CloudService::none;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(uploadTab ? 1 : 2);

                QTreeWidget* list = nullptr;
                if (uploadTab) {
                    dialog->selectAllUpChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("File"));
                } else {
                    dialog->selectAllSyncChanged(Qt::Checked);
                    list = rideListWithHeader(dialog, QStringLiteral("Source"));
                }

                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.listCount > 0 && !uploadTab)
                        out.row0Action = root->child(0)->text(6);
                }

                QPushButton* button =
                    pushButtonWithText(dialog, uploadTab ? QStringLiteral("Upload") : QStringLiteral("Synchronize"));

                // THE USER'S REFRESH, delivered inside the parse-failure branch's
                // own QApplication::processEvents(). Nothing is faked: this calls
                // the dialog's own refreshClicked() slot, exactly as the button's
                // connect (:1225) would, and records the row-0 item POINTER either
                // side of it as an integer without ever dereferencing it.
                const auto refresh = [&, dialog, list]() {
                    out.refreshDelivered = true;
                    out.parseAttemptsAtRefresh = ridefail::opens;
                    if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
                        out.barValueAtRefresh = bar->value();

                    quintptr row0Before = 0;
                    if (list != nullptr) {
                        out.rowsBeforeRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsBeforeRefresh > 0)
                            row0Before = reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0));
                    }

                    dialog->refreshClicked(); // :1516 — every row deleted, then rebuilt

                    if (list != nullptr) {
                        out.rowsAfterRefresh = list->invisibleRootItem()->childCount();
                        if (out.rowsAfterRefresh > 0)
                            out.rowsWereFreed =
                                (reinterpret_cast<quintptr>(list->invisibleRootItem()->child(0)) != row0Before);
                    }
                };

                // Armed BEFORE the batch starts; fires in the first parse-failure
                // branch the driver reaches. See the block comment above.
                insideframe::atTheNextCountedRow(dialog, refresh);

                dialog->downloadClicked(); // -> syncNext() / uploadNext()

                for (int i = 0; i < 60; ++i)
                    QApplication::processEvents(QEventLoop::AllEvents, 5);

                out.parseAttemptsAtEnd = ridefail::opens;
                out.transfersAtEnd = obs::writeFileCalls + obs::readFileCalls;
                if (list != nullptr) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        const QString status = root->child(i)->text(7);
                        out.statuses << status;
                        if (!status.isEmpty())
                            out.labelledRows++;
                    }
                }
                out.progressText = progressLabelText(dialog);
                if (button != nullptr)
                    out.buttonTextAtEnd = button->text();

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // A run that never reached the window must fail LOUDLY rather than pass on
    // nothing (LSN-047, LSN-050) - and, as in TEST-119/120, it must prove WHERE
    // in the driver the Refresh landed.
    void assertDriverRefreshPremises(const QString& where, const DriverRefreshOutcome& out)
    {
        QVERIFY2(out.timedOut == false,
                 qPrintable(where + QStringLiteral("the run never came back - a guard wedged it")));
        QVERIFY2(out.listCount == 2,
                 qPrintable(where + QStringLiteral("the list held %1 rows, not 2").arg(out.listCount)));
        QVERIFY2(out.checkedRows == 2,
                 qPrintable(where + QStringLiteral("%1 rows were checked, not 2").arg(out.checkedRows)));
        QVERIFY2(out.refreshDelivered,
                 qPrintable(where + QStringLiteral("the Refresh was never delivered into the parse-failure branch - "
                                                   "this run proves nothing")));

        // WHERE it landed, measured. Below the branch's own setValue...
        QVERIFY2(
            out.barValueAtRefresh == 1,
            qPrintable(where + QStringLiteral("the bar read %1 when the Refresh ran, not 1 - the Refresh was not "
                                              "inside the parse-failure branch that had just counted row 0, so this "
                                              "run is about some other window")
                                   .arg(out.barValueAtRefresh)));
        // ...and above the `continue`, which is the only thing that can offer a
        // second row to the reader.
        QVERIFY2(
            out.parseAttemptsAtRefresh == 1,
            qPrintable(where + QStringLiteral("%1 row(s) had been offered to the reader when the Refresh ran, not 1 - "
                                              "the loop had already iterated, so the Refresh was not inside the "
                                              "branch's own processEvents()")
                                   .arg(out.parseAttemptsAtRefresh)));

        QVERIFY2(out.rowsBeforeRefresh == 2,
                 qPrintable(where + QStringLiteral("the list held %1 rows when the Refresh arrived, not 2")
                                        .arg(out.rowsBeforeRefresh)));
        QVERIFY2(out.rowsAfterRefresh == 2,
                 qPrintable(where + QStringLiteral("the Refresh rebuilt %1 rows, not 2 - the run would then be about "
                                                   "an empty list rather than a REPLACED one")
                                        .arg(out.rowsAfterRefresh)));
        QVERIFY2(out.rowsWereFreed,
                 qPrintable(where + QStringLiteral("row 0 is the SAME QTreeWidgetItem after the Refresh as before it, "
                                                   "so nothing was freed and this run proves nothing")));
    }

    // The shared verdict for TEST-122 and TEST-123. NOT a slot.
    //
    // What is and is NOT load-bearing here, said plainly so that no assertion is
    // read as coverage it does not provide: `labelledRows` and `transfersAtEnd`
    // are 0 whether or not the guard exists - the rebuilt rows are unchecked, so
    // the re-driven loop labels nothing and dispatches nothing. They are the
    // criterion's own wording and they pin the shape; the two that KILL are the
    // tail sentence and the button.
    void assertDriverStoodDown(const QString& where, const DriverRefreshOutcome& out)
    {
        QVERIFY2(
            out.progressText.isEmpty(),
            qPrintable(where + QStringLiteral("the destroyed batch still reported itself: progressLabel reads \"%1\" "
                                              "(any tail sentence at all is one batch too many, and in syncNext's "
                                              "tail the line below it is context->athlete->rideCache->save())")
                                   .arg(out.progressText)));

        QVERIFY2(out.buttonTextAtEnd == QStringLiteral("Abort"),
                 qPrintable(where + QStringLiteral("the completion tail ran for the destroyed batch: the button reads "
                                                   "\"%1\" rather than \"Abort\"")
                                        .arg(out.buttonTextAtEnd)));

        QVERIFY2(
            out.labelledRows == 0,
            qPrintable(where + QStringLiteral("%1 row(s) of the REBUILT list carry a status the batch that owned them "
                                              "never gave them: [%2]")
                                   .arg(out.labelledRows)
                                   .arg(out.statuses.join(QStringLiteral("|")))));
        QVERIFY2(
            out.transfersAtEnd == 0,
            qPrintable(where + QStringLiteral("%1 transfer(s) were issued - the re-driven loop dispatched against a "
                                              "list its batch never owned")
                                   .arg(out.transfersAtEnd)));
        QVERIFY2(out.parseAttemptsAtEnd == 1,
                 qPrintable(where + QStringLiteral("%1 row(s) were offered to the reader in total, not 1 - the loop "
                                                   "carried on over a list its batch never owned")
                                        .arg(out.parseAttemptsAtEnd)));
    }

  private slots:
    // -- TEST-122 (A3-R028b-F2, REQ-028 (b)) -----------------------------
    // THE SYNC TAB'S DRIVER: THE RESUMPTION THE ENTRY GUARD CANNOT SEE.
    //
    // A Sync batch of two checked Upload rows, row[0] unparseable. The Refresh is
    // delivered inside syncNext's parse-failure processEvents() (:2428) - after
    // the self-bail, the abort re-read and the batch-generation compare, and
    // before the `continue` at :2497 that walks the rebuilt list. The frame must
    // stand down instead: no completion tail for the destroyed batch, and no row
    // of the list that replaced it labelled.
    //
    // RED, before the fix:
    //   FAIL!  : ... sync tab: the destroyed batch still reported itself:
    //            progressLabel reads "Processed 0 of 2 successfully"
    void aRefreshInADriverParseFailureMustNotReDriveTheSyncLoop()
    {
        const DriverRefreshOutcome out = runRefreshInDriverParseFailure(false);
        const QString where = QStringLiteral("sync tab: ");
        assertDriverRefreshPremises(where, out);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(out.row0Action, QStringLiteral("Upload"));
        assertDriverStoodDown(where, out);
    }

    // -- TEST-123 (A3-R028b-F2, REQ-028 (b)) -----------------------------
    // THE UPLOAD TAB'S TWIN — ITS OWN SLOT, NOT AN ARM OF THE ONE ABOVE.
    //
    // uploadNext carries the identical branch at :3182-3219 and falls through to
    // its own completion tail rather than `continue`-ing, so it is a different
    // production frame reached by a different route. A3-R028b-F5 found that
    // TEST-120's upload arm was gated behind the sync arm's success; this is a
    // separate slot so the third driver's coverage cannot be withdrawn by another
    // slot failing.
    //
    // RED, before the fix:
    //   FAIL!  : ... upload tab: the destroyed batch still reported itself:
    //            progressLabel reads "Uploaded 0 of 2 successfully"
    void aRefreshInADriverParseFailureMustNotReDriveTheUploadLoop()
    {
        const DriverRefreshOutcome out = runRefreshInDriverParseFailure(true);
        const QString where = QStringLiteral("upload tab: ");
        assertDriverRefreshPremises(where, out);
        if (QTest::currentTestFailed())
            return;
        assertDriverStoodDown(where, out);
    }

  private:
    // =====================================================================
    // TEST-124 / TEST-125 (A3-R028b-F3, REQ-028 (c), DEC-garmin-036) —
    // AN ABANDONED WRITE'S COMPLETION CARRIES ITS OPERATION IDENTITY.
    // =====================================================================
    //
    // THE CRITERION, VERBATIM (REQ-028 (c)):
    //
    //   "(c) NO ROW IS LABELLED THAT THE FRAME DID NOT TRANSFER, AND NO ROW IS
    //   TRANSFERRED TWICE. Across a batch that is aborted and immediately
    //   restarted with the aborted batch's completion arriving LATE, every
    //   non-empty status cell names the outcome of a transfer that actually
    //   happened to that row, and the reader is invoked exactly once per checked
    //   row - an over-count catches a second driver, an under-count catches a
    //   stale one."
    //
    // THE AMBIGUITY OPTION C CLOSES. TWO LISTS CAN CARRY THE SAME remote name: the Upload
    // list's row and the Sync list's Upload row for one activity both hold
    // `text(1) == ride->fileName` (CloudService.cpp:1777 / :1823), and both arm
    // sites compute `QFileInfo(text(1)).baseName() + uploadExtension()` from it.
    // So a Sync batch abandoned mid-upload and an Upload batch restarted behind it
    // arm different operation ids even though their user-visible names match.
    //
    // THE FOUR CLICKS: Sync -> Synchronize -> Abort -> Upload tab, Select all,
    // Upload. No Refresh anywhere. Then the abandoned SYNC write completes.
    //
    // `completeWrite = false` lets the run deliver the two parked operation ids
    // in either order. The messages deliberately differ so every assertion can
    // prove that each result lands on the row for the operation that produced it.
    struct AbandonedWriteOutcome
    {
        bool timedOut = false;

        // -- premises about the fixture
        int syncListCount = 0;
        int upListCount = 0;
        int syncChecked = 0;
        int upChecked = 0;

        // -- premises about the two batches
        QString staleName; // the SYNC batch's outstanding write...
        QString liveName;  // ...and the restarted UPLOAD batch's
        int writeCallsAfterSyncDispatch = 0;
        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        bool restartTookTheStartBranch = false;
        int writeCallsAfterRestart = 0;

        // -- read either side of EACH of the two deliveries, so "which row
        //    changed, and when" is an observation rather than an inference.
        QStringList syncStatusesBefore, upStatusesBefore;
        QStringList syncStatusesMid, upStatusesMid;
        QStringList syncStatusesAfter, upStatusesAfter;
        int writeCallsBefore = 0, writeCallsMid = 0, writeCallsAfter = 0;
        int barBefore = 0, barMid = 0, barAfter = 0, barMax = 0;
        QString progressTextAfter;
        QStringList writeNames;

        // Set on the last line of the run: under ASan with halt_on_error=1 a
        // use-after-free ends the PROCESS, so a run that gets this far committed
        // none.
        bool survivedBothCompletions = false;
    };

    // The live upload's verdict, and the abandoned sync write's. DELIBERATELY
    // DIFFERENT, and only the first counts as a success (completedWrite compares
    // against tr("Completed.")), so "which verdict landed where" and "was anything
    // counted" are both observable.
    static QString liveWriteResult() { return QStringLiteral("Completed."); }
    static QString staleWriteResult() { return QStringLiteral("Upload failed."); }

    // One run of the F3 geometry. `staleFirst` chooses the ARRIVAL ORDER. NOT a
    // slot (it takes an argument).
    AbandonedWriteOutcome runAbandonedSyncWriteThenUpload(bool staleFirst)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        AbandonedWriteOutcome out;

        // Two local activities. The rows must exist on disk because both drivers
        // open each one before writing it.
        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false; // the upload list skips planned rides
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // NOTHING remote, so every local ride is "not on the service yet"
                // and gets a sync list UPLOAD row as well as an upload list row -
                // which is the pair of same-named rows this finding is about.
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->writeSucceeds = true;
                // This run owns the timing AND the order of both completions.
                store->completeWrite = false;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                QTreeWidget* syncList = rideListWithHeader(dialog, QStringLiteral("Source"));
                QTreeWidget* upList = rideListWithHeader(dialog, QStringLiteral("File"));
                QTabWidget* tabs = dialog->findChild<QTabWidget*>();
                QProgressBar* bar = dialog->findChild<QProgressBar*>();

                auto statuses = [](QTreeWidget* w) {
                    QStringList s;
                    if (w != nullptr)
                        for (int i = 0; i < w->invisibleRootItem()->childCount(); i++)
                            s << w->invisibleRootItem()->child(i)->text(7);
                    return s;
                };
                auto countChecked = [](QTreeWidget* w) {
                    int n = 0;
                    if (w == nullptr)
                        return n;
                    QTreeWidgetItem* root = w->invisibleRootItem();
                    for (int i = 0; i < root->childCount(); i++) {
                        QCheckBox* c = qobject_cast<QCheckBox*>(w->itemWidget(root->child(i), 0));
                        if (c != nullptr && c->isChecked())
                            n++;
                    }
                    return n;
                };

                // ---- CLICKS 1+2. The SYNC tab, Select all, Synchronize. syncNext
                //      takes its UPLOAD branch on sync row 0 and parks there.
                if (tabs != nullptr)
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                if (syncList != nullptr)
                    out.syncListCount = syncList->invisibleRootItem()->childCount();
                out.syncChecked = countChecked(syncList);

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Synchronize"));

                dialog->downloadClicked();
                out.staleName = obs::lastWriteName;
                out.writeCallsAfterSyncDispatch = obs::writeFileCalls;

                // ---- CLICK 3. Abort. The abort branch deliberately does NOT
                //      disarm, so the sync batch's write ticket is still armed.
                out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                dialog->downloadClicked();
                out.abortTookTheAbortBranch = (watched != nullptr && watched->text() == QStringLiteral("Download"));

                // ---- CLICK 4+5. The UPLOAD tab, Select all, Upload. The START
                //      branch is where the abandoned ticket must be RETIRED, and
                //      uploadNext then re-arms the IDENTICAL name for a different
                //      row of a different list.
                if (tabs != nullptr)
                    tabs->setCurrentIndex(1);
                dialog->selectAllUpChanged(Qt::Checked);
                if (upList != nullptr)
                    out.upListCount = upList->invisibleRootItem()->childCount();
                out.upChecked = countChecked(upList);

                dialog->downloadClicked();
                out.restartTookTheStartBranch = (watched != nullptr && watched->text() == QStringLiteral("Abort"));
                out.liveName = obs::lastWriteName;
                out.writeCallsAfterRestart = obs::writeFileCalls;

                out.syncStatusesBefore = statuses(syncList);
                out.upStatusesBefore = statuses(upList);
                out.writeCallsBefore = obs::writeFileCalls;
                if (bar != nullptr) {
                    out.barBefore = bar->value();
                    out.barMax = bar->maximum();
                }

                // ---- THE TWO COMPLETIONS. Their remote names match but their
                //      opaque operation ids do not; choose the id order directly.
                store->deliverParkedWrite(staleFirst ? 0 : 1, staleFirst ? staleWriteResult() : liveWriteResult());

                out.syncStatusesMid = statuses(syncList);
                out.upStatusesMid = statuses(upList);
                out.writeCallsMid = obs::writeFileCalls;
                if (bar != nullptr)
                    out.barMid = bar->value();

                store->deliverParkedWrite(0, staleFirst ? liveWriteResult() : staleWriteResult());

                out.syncStatusesAfter = statuses(syncList);
                out.upStatusesAfter = statuses(upList);
                out.writeCallsAfter = obs::writeFileCalls;
                if (bar != nullptr)
                    out.barAfter = bar->value();
                out.progressTextAfter = progressLabelText(dialog);
                out.writeNames = obs::writeNames;
                out.survivedBothCompletions = true;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);

        return out;
    }

    // The premises every arrival order shares. Without these the run below is
    // satisfied by a completedWrite that does nothing at all (LSN-050). NOT a
    // slot.
    void assertAbandonedWritePremises(const QString& where, const AbandonedWriteOutcome& out)
    {
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the run never came back")));
        QVERIFY2(out.survivedBothCompletions,
                 qPrintable(where + QStringLiteral("the run did not reach its last line")));
        QCOMPARE(out.syncListCount, 2);
        QCOMPARE(out.syncChecked, 2);
        QCOMPARE(out.upListCount, 2);
        QCOMPARE(out.upChecked, 2);

        // The SYNC batch really did dispatch a write, and the abort really was an
        // abort.
        QCOMPARE(out.writeCallsAfterSyncDispatch, 1);
        QVERIFY2(
            out.sawAbortLabel,
            qPrintable(where + QStringLiteral("the button was never labelled \"Abort\", so downloadClicked() could "
                                              "not have been the abort control")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch")));

        // ...and the UPLOAD batch behind it really did start and really did
        // dispatch its own write.
        QVERIFY2(out.restartTookTheStartBranch,
                 qPrintable(where + QStringLiteral("the third click did not take the START branch")));
        QCOMPARE(out.writeCallsAfterRestart, 2);

        // The collision premise: names match. Correct association therefore has
        // to come from the operation ids carried by the completion channel.
        QVERIFY2(out.staleName == out.liveName,
                 qPrintable(where + QStringLiteral("the abandoned sync write and the live upload carry DIFFERENT names "
                                                   "(\"%1\" vs \"%2\"), so this run is not the F3 route")
                                        .arg(out.staleName)
                                        .arg(out.liveName)));
        QCOMPARE(out.writeNames.value(0), out.staleName);
        QCOMPARE(out.writeNames.value(1), out.liveName);

        // Both lists are parked on their row 0 and nothing has been counted yet.
        QCOMPARE(out.syncStatusesBefore, QStringList() << "Uploading" << "");
        QCOMPARE(out.upStatusesBefore, QStringList() << "Uploading" << "");
        QCOMPARE(out.barBefore, 0);
        QCOMPARE(out.barMax, 2);
    }

    // What BOTH orders must end at, whichever verdict landed where. This is the
    // exhaustion argument as a measurement: exactly one re-drive, exactly one
    // count, and no completion swallowed. NOT a slot.
    void assertAbandonedWriteEndState(const QString& where, const AbandonedWriteOutcome& out)
    {
        // THE FIRST ARRIVAL DRIVES NOTHING. It is the abandoned batch's, whatever
        // it says, so it may not dispatch the live batch's next row.
        QVERIFY2(
            out.writeCallsMid == 2,
            qPrintable(where + QStringLiteral("the FIRST completion re-drove the loop: %1 writes have been issued, "
                                              "not 2")
                                   .arg(out.writeCallsMid)));
        QCOMPARE(out.barMid, 0);
        QCOMPARE(out.upStatusesMid, QStringList() << "Uploading" << "");

        // THE SECOND ARRIVAL IS NOT SWALLOWED: it is the live batch's ticket, so
        // it counts once and re-drives once - onto row 1, which is the row that
        // was never dispatched under the defect.
        QCOMPARE(out.barAfter, 1);
        QVERIFY2(out.writeCallsAfter == 3,
                 qPrintable(where + QStringLiteral("the loop was re-driven %1 times in total, not once (%2 writes)")
                                        .arg(out.writeCallsAfter - 2)
                                        .arg(out.writeCallsAfter)));
        QCOMPARE(out.upStatusesAfter.value(1), QStringLiteral("Uploading"));
        QVERIFY2(out.writeNames.count() == 3 && out.writeNames.value(2) != out.liveName,
                 qPrintable(where + QStringLiteral("the re-drive did not dispatch a NEW row: writes were [%1]")
                                        .arg(out.writeNames.join(QStringLiteral("|")))));

        // ...AND THE LIVE BATCH HAS NOT FINISHED. progressLabelText reports only a
        // completion TAIL's own sentence (it matches on "successfully"), so an
        // empty read here is the claim that neither batch announced itself: the
        // live one is still waiting on row 1's write, which is exactly the state a
        // batch of two rows should be in after one of them has landed.
        QCOMPARE(out.progressTextAfter, QString());

        // The sync list's row 1 was never dispatched by anybody.
        QCOMPARE(out.syncStatusesAfter.value(1), QString());
    }

    // =====================================================================
    // TEST-125 (A3-R028b-F3, REQ-028 (c), DEC-garmin-037) — a retired ticket
    // holds a RAW ROW POINTER.
    // =====================================================================
    //
    // The mechanism above keeps an abandoned write's ticket alive past the batch
    // that armed it, and that ticket holds a `QTreeWidgetItem*`. Refresh is the
    // one place in this dialog a row dies (:1570/:1577/:1584), so without a clear
    // beside the existing disarm the fix MANUFACTURES a use-after-free of exactly
    // the A3-R028-F1 shape - a new defect created by the repair of an old one.
    //
    // TWO RUNS, and they measure different things:
    //
    //   NO REFRESH   the criterion. The abandoned sync write's completion arrives
    //                after a restart that dispatched NOTHING (the download tab is
    //                empty), and must still label the row it was issued for. RED
    //                before DEC-037: the START branch disarms, so today the
    //                completion is swallowed and the row keeps "Uploading".
    //   WITH REFRESH the memory-safety rider on the same mechanism. The Refresh
    //                between the retire and the delivery FREES that row; a second
    //                START then re-stamps batchListGeneration so the DEC-034
    //                compare cannot be what saves the run. Nothing may be
    //                labelled and the process must survive. This run is green
    //                before DEC-037 too - there is nothing retained to dangle -
    //                and it goes red only against DEC-037 WITHOUT its clear, which
    //                is what makes that line mutation-provable rather than
    //                asserted.
    struct RetiredWriteOutcome
    {
        bool timedOut = false;

        int syncListCount = 0;
        int syncChecked = 0;
        QString staleName;
        int writeCallsAfterDispatch = 0;

        bool sawAbortLabel = false;
        bool abortTookTheAbortBranch = false;
        QString progressTextAfterRestart;
        int writeCallsAfterRestart = 0;

        bool refreshed = false;
        bool rowFreedByRefresh = false;
        QString progressTextAfterSecondRestart;

        QStringList syncStatusesBefore;
        QStringList syncStatusesAfter;
        QList<int> rowsRelabelled;
        int barBefore = 0, barAfter = 0;
        int writeCallsAfterLate = 0;

        bool restartedWithWrite = false;
        int parkedBeforeLate = 0;
        QStringList upStatusesBeforeLate;
        QStringList upStatusesAfterStale;
        QStringList upStatusesAfterLive;
        int barAfterStale = 0;
        int barAfterLive = 0;
        bool deliveredStale = false;
        bool deliveredLive = false;

        bool survivedTheLateCompletion = false;
    };

    RetiredWriteOutcome runRetiredWriteAcrossRefresh(bool withRefresh, bool restartWriteAfterRefresh = false)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5;

        RetiredWriteOutcome out;

        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        const QString name = rebuildLocalActivity(0);
        QFile f(activities.absolutePath() + "/" + name);
        f.open(QIODevice::WriteOnly);
        f.write("gcblock");
        f.close();
        const QString path = f.fileName();

        RideItem* item = new RideItem(nullptr, context);
        item->fileName = name;
        item->path = activities.absolutePath();
        item->dateTime = QDateTime(QDate::currentDate(), QTime(10, 0, 0));
        item->planned = false;
        rideCache->rides().push_back(item);

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                // Nothing remote: one sync UPLOAD row, one upload row, and an
                // EMPTY download list - which is how the restart below arms
                // nothing at all.
                store->entryNames = QStringList();
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                store->writeSucceeds = true;
                store->completeWrite = false;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                QTabWidget* tabs = dialog->findChild<QTabWidget*>();
                QProgressBar* bar = dialog->findChild<QProgressBar*>();

                // The sync list is looked up FRESH either side of the Refresh:
                // refreshClicked deletes its rows and builds new ones, and holding
                // a row pointer across it is the very defect under test.
                auto syncStatuses = [&]() {
                    QStringList s;
                    if (QTreeWidget* w = rideListWithHeader(dialog, QStringLiteral("Source")))
                        for (int i = 0; i < w->invisibleRootItem()->childCount(); i++)
                            s << w->invisibleRootItem()->child(i)->text(7);
                    return s;
                };

                // ---- CLICKS 1+2. Sync, Select all, Synchronize.
                if (tabs != nullptr)
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                QTreeWidget* syncList = rideListWithHeader(dialog, QStringLiteral("Source"));
                if (syncList != nullptr) {
                    out.syncListCount = syncList->invisibleRootItem()->childCount();
                    for (int i = 0; i < out.syncListCount; i++) {
                        QCheckBox* c =
                            qobject_cast<QCheckBox*>(syncList->itemWidget(syncList->invisibleRootItem()->child(i), 0));
                        if (c != nullptr && c->isChecked())
                            out.syncChecked++;
                    }
                }

                QPushButton* watched = pushButtonWithText(dialog, QStringLiteral("Synchronize"));

                dialog->downloadClicked();
                out.staleName = obs::lastWriteName;
                out.writeCallsAfterDispatch = obs::writeFileCalls;

                // The ADDRESS of the row the ticket is holding, kept as raw
                // storage. Never cast back, never dereferenced - asking ASan
                // whether it is poisoned is the only way to ask "is it dead?"
                // without committing the use-after-free under investigation.
                const void* syncRow0Addr = (syncList != nullptr && syncList->invisibleRootItem()->childCount() > 0)
                                               ? static_cast<const void*>(syncList->invisibleRootItem()->child(0))
                                               : nullptr;

                // ---- CLICK 3. Abort, which leaves the ticket armed.
                out.sawAbortLabel = (pushButtonWithText(dialog, QStringLiteral("Abort")) != nullptr);
                dialog->downloadClicked();
                out.abortTookTheAbortBranch = (watched != nullptr && watched->text() == QStringLiteral("Download"));

                // ---- CLICK 4. The DOWNLOAD tab, which holds no rows at all, so
                //      this START branch retires the abandoned write ticket and
                //      then dispatches NOTHING. Only downloadNext's tail writes
                //      the sentence below, so it is production's own proof that
                //      the START branch ran and that its batch was empty.
                if (tabs != nullptr)
                    tabs->setCurrentIndex(0);
                dialog->downloadClicked();
                out.progressTextAfterRestart = progressLabelText(dialog);
                out.writeCallsAfterRestart = obs::writeFileCalls;

                if (withRefresh) {
                    // ---- CLICK 5. Refresh: the one place a row dies.
                    dialog->refreshClicked();
                    out.refreshed = true;
                    out.rowFreedByRefresh = isPoisoned(syncRow0Addr, sizeof(QTreeWidgetItem));

                    // ---- CLICK 6. ...and a START behind it, so that the DEC-034
                    //      generation compare at completedWrite's entry reads EQUAL
                    //      and cannot be what stands the delivery down. Without
                    //      this click the run proves nothing about the retired set.
                    if (restartWriteAfterRefresh) {
                        if (tabs != nullptr)
                            tabs->setCurrentIndex(1);
                        dialog->selectAllUpChanged(Qt::Checked);
                        dialog->downloadClicked();
                        out.restartedWithWrite = true;
                    } else {
                        dialog->downloadClicked();
                        out.progressTextAfterSecondRestart = progressLabelText(dialog);
                    }
                }

                auto upStatuses = [&]() {
                    QStringList s;
                    if (QTreeWidget* w = rideListWithHeader(dialog, QStringLiteral("File")))
                        for (int i = 0; i < w->invisibleRootItem()->childCount(); i++)
                            s << w->invisibleRootItem()->child(i)->text(7);
                    return s;
                };

                out.syncStatusesBefore = syncStatuses();
                if (bar != nullptr)
                    out.barBefore = bar->value();

                // ---- THE LATE COMPLETION: the sync batch's write, arriving now.
                out.parkedBeforeLate = store->parked.count();
                if (restartWriteAfterRefresh) {
                    out.upStatusesBeforeLate = upStatuses();
                    out.deliveredStale = store->deliverParkedWrite(0, staleWriteResult());
                    out.upStatusesAfterStale = upStatuses();
                    if (bar != nullptr)
                        out.barAfterStale = bar->value();
                    out.deliveredLive = store->deliverParkedWrite(0, liveWriteResult());
                    out.upStatusesAfterLive = upStatuses();
                    if (bar != nullptr)
                        out.barAfterLive = bar->value();
                } else {
                    store->notifyWriteComplete(out.staleName, liveWriteResult());
                }

                out.syncStatusesAfter = syncStatuses();
                for (int i = 0; i < out.syncStatusesAfter.count() && i < out.syncStatusesBefore.count(); i++)
                    if (out.syncStatusesAfter.at(i) != out.syncStatusesBefore.at(i))
                        out.rowsRelabelled << i;
                out.writeCallsAfterLate = obs::writeFileCalls;
                if (bar != nullptr)
                    out.barAfter = bar->value();
                out.survivedTheLateCompletion = true;

                QTimer::singleShot(100, qApp, [owner]() { delete owner; });
                QTimer::singleShot(200, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(10000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        rideCache->rides().removeAll(item);
        delete item->ride(false);
        delete item;
        QFile::remove(path);

        return out;
    }

    void assertRetiredWritePremises(const QString& where, const RetiredWriteOutcome& out)
    {
        QVERIFY2(out.timedOut == false, qPrintable(where + QStringLiteral("the run never came back")));
        QVERIFY2(out.survivedTheLateCompletion,
                 qPrintable(where + QStringLiteral("the run did not reach its last line")));
        QCOMPARE(out.syncListCount, 1);
        QCOMPARE(out.syncChecked, 1);
        QCOMPARE(out.writeCallsAfterDispatch, 1);
        QVERIFY2(out.sawAbortLabel, qPrintable(where + QStringLiteral("the button was never labelled \"Abort\"")));
        QVERIFY2(out.abortTookTheAbortBranch,
                 qPrintable(where + QStringLiteral("downloadClicked() did not take its abort branch")));
        QCOMPARE(out.progressTextAfterRestart, QStringLiteral("Downloaded 0 of 0 successfully"));
        QCOMPARE(out.writeCallsAfterRestart, 1);
        // The late completion may not dispatch anything, in either run.
        QCOMPARE(out.writeCallsAfterLate, 1);
        QCOMPARE(out.barAfter, out.barBefore);
    }

  private slots:
    // -- TEST-124 (A3-R028b-F3, REQ-028 (c), DEC-garmin-036) -------------
    // AN ABANDONED SYNC WRITE MUST NOT BE TAKEN FOR THE LIVE UPLOAD — IN
    // EITHER ARRIVAL ORDER.
    //
    // RED, before DEC-037 (QPA offscreen and minimal both):
    //   FAIL!  : ...[stale, live]: the FIRST completion re-drove the loop: 3
    //            writes have been issued, not 2
    //
    // Both orders are run so operation/result association is independent of
    // completion order even when the remote names collide.
    void anAbandonedSyncWritesCompletionMustNotBeTakenForTheLiveUploadsInEitherOrder()
    {
        // ---- ORDER ONE: [stale, live]. The abandoned sync write lands first.
        const AbandonedWriteOutcome stale = runAbandonedSyncWriteThenUpload(true);
        const QString first = QStringLiteral("[stale, live]: ");
        assertAbandonedWritePremises(first, stale);
        if (QTest::currentTestFailed())
            return;

        // The first arrival is the abandoned SYNC batch's, and it labels the SYNC
        // row it was issued for - not the upload row that happens to share its
        // name.
        QCOMPARE(stale.syncStatusesMid, QStringList() << staleWriteResult() << "");
        // ...and the second is the live upload's, on the upload row.
        QCOMPARE(stale.upStatusesAfter.value(0), liveWriteResult());
        QCOMPARE(stale.syncStatusesAfter.value(0), staleWriteResult());
        assertAbandonedWriteEndState(first, stale);
        if (QTest::currentTestFailed())
            return;

        // ---- ORDER TWO: [live, stale]. The live operation lands first and must
        //      still resolve to the upload row by its own id.
        const AbandonedWriteOutcome live = runAbandonedSyncWriteThenUpload(false);
        const QString second = QStringLiteral("[live, stale]: ");
        assertAbandonedWritePremises(second, live);
        if (QTest::currentTestFailed())
            return;

        // The live result belongs to the upload row immediately; the later stale
        // result belongs to the abandoned sync row. No result swap is accepted.
        QCOMPARE(live.syncStatusesMid, QStringList() << "Uploading" << "");
        QCOMPARE(live.upStatusesMid.value(0), liveWriteResult());
        QCOMPARE(live.upStatusesAfter.value(0), liveWriteResult());
        QCOMPARE(live.syncStatusesAfter.value(0), staleWriteResult());
        QCOMPARE(live.writeCallsMid, 3);
        QCOMPARE(live.writeCallsAfter, 3);
        QCOMPARE(live.barMid, 1);
        QCOMPARE(live.barAfter, 1);
        QCOMPARE(live.upStatusesAfter.value(1), QStringLiteral("Uploading"));
        QCOMPARE(live.progressTextAfter, QString());
    }

    // -- TEST-125 (A3-R028b-F3, REQ-028 (c), DEC-garmin-037) -------------
    // A RETIRED TICKET MUST NOT OUTLIVE THE ROW IT NAMES.
    //
    // RED, before DEC-037 (QPA offscreen and minimal both):
    //   FAIL!  : ...Compared values are not the same
    //            Actual   (out.syncStatusesAfter): "Uploading"
    //            Expected (...liveWriteResult()) : "Completed."
    void aRetiredWriteTicketMustNotOutliveTheRowsItNames()
    {
        // ---- RUN ONE: no Refresh. The retained ticket labels its own row.
        const RetiredWriteOutcome kept = runRetiredWriteAcrossRefresh(false);
        const QString first = QStringLiteral("no refresh: ");
        assertRetiredWritePremises(first, kept);
        if (QTest::currentTestFailed())
            return;
        QCOMPARE(kept.syncStatusesBefore, QStringList() << "Uploading");
        QCOMPARE(kept.syncStatusesAfter.value(0), liveWriteResult());
        QCOMPARE(kept.rowsRelabelled, QList<int>() << 0);

        // ---- RUN TWO: a Refresh between the retire and the delivery. The row the
        //      ticket names is FREED, so the ticket must be gone with it.
        const RetiredWriteOutcome dropped = runRetiredWriteAcrossRefresh(true);
        const QString second = QStringLiteral("refresh: ");
        assertRetiredWritePremises(second, dropped);
        if (QTest::currentTestFailed())
            return;

        // The premise: the row really is dead. Asked of the ALLOCATOR, never by
        // dereferencing it.
        QVERIFY2(dropped.refreshed, qPrintable(second + QStringLiteral("no Refresh was issued")));
        QVERIFY2(dropped.rowFreedByRefresh,
                 qPrintable(second + QStringLiteral("the Refresh did not free the row the ticket names, so this run "
                                                    "cannot show the use-after-free it exists for")));
        // ...and the second START really did re-stamp the generation, so the
        // DEC-034 compare at completedWrite's entry is not what stands this
        // delivery down.
        QCOMPARE(dropped.progressTextAfterSecondRestart, QStringLiteral("Downloaded 0 of 0 successfully"));

        // THE CRITERION: nothing labelled, and - the assertion this run exists for
        // - the process is still alive to say so. Under ASan with halt_on_error=1
        // a dangling `row->setText` ends it here instead.
        QCOMPARE(dropped.syncStatusesBefore, QStringList() << "");
        QCOMPARE(dropped.syncStatusesAfter, QStringList() << "");
        QCOMPARE(dropped.rowsRelabelled, QList<int>());
    }

    // -- TEST-128 (REQ-028 (a)/(c), DEC-garmin-036 Option C) ------------
    // Preserve both operation identities across Abort -> Refresh -> restart,
    // while making the abandoned operation row-free. The stale completion must
    // consume only its own record; the live completion must remain able to label
    // and count the restarted row exactly once.
    void refreshMustMakeOutstandingWritesRowFreeWithoutLosingTheirIdentity()
    {
        const RetiredWriteOutcome out = runRetiredWriteAcrossRefresh(true, true);
        QVERIFY2(!out.timedOut, "TEST-128 run never came back");
        QVERIFY2(out.survivedTheLateCompletion, "TEST-128 did not reach its last observation");
        QCOMPARE(out.syncListCount, 1);
        QCOMPARE(out.syncChecked, 1);
        QCOMPARE(out.writeCallsAfterDispatch, 1);
        QVERIFY2(out.refreshed && out.rowFreedByRefresh, "Refresh did not free the abandoned operation's original row");
        QVERIFY2(out.restartedWithWrite, "the post-Refresh batch did not dispatch a live write");
        QCOMPARE(out.parkedBeforeLate, 2);
        QVERIFY2(out.deliveredStale && out.deliveredLive, "both parked completions were not delivered");

        QCOMPARE(out.syncStatusesBefore, QStringList() << "");
        QCOMPARE(out.syncStatusesAfter, QStringList() << "");
        QCOMPARE(out.upStatusesBeforeLate, QStringList() << "Uploading");
        QVERIFY2(out.upStatusesAfterStale == (QStringList() << "Uploading"),
                 qPrintable(QStringLiteral("the row-free stale operation consumed or labelled the LIVE row: [%1]")
                                .arg(out.upStatusesAfterStale.join(QStringLiteral("|")))));
        QCOMPARE(out.barAfterStale, 0);
        QCOMPARE(out.upStatusesAfterLive, QStringList() << liveWriteResult());
        QCOMPARE(out.barAfterLive, 1);
        QCOMPARE(out.writeCallsAfterLate, 2);
    }

  private:
    // =====================================================================
    // TEST-127 — THE RANDOMIZED CLICK-SEQUENCE DRIVER.
    // =====================================================================
    //
    // WHY. Every other slot in this file is one route somebody thought of. Six
    // guard mechanisms now interact inside this dialog and each new one
    // MULTIPLIES the reachable state space instead of adding to it, so the set of
    // routes nobody has thought of is growing faster than the set that has a slot.
    // This drives the same alphabet of user actions the hand-written slots draw
    // from - Synchronize/Upload/Download, Abort, Refresh, a tab switch and the
    // delivery of a completion the network has been holding - in orders NOBODY
    // CHOSE, and judges the result with the two oracles that need no route
    // knowledge:
    //
    //   1. TEST-126's invariants, which are already being evaluated in every run
    //      of this file and are evaluated in these too.
    //   2. THE PROCESS ITSELF. This target is built with -fsanitize=address and
    //      its ctest entries pin halt_on_error=1
    //      (unittests/Core/garminconnect/CMakeLists.txt:1457/:1486), so a
    //      use-after-free is not a failed comparison, it is the end of the
    //      process. "The run came back and printed its trace" is therefore an
    //      assertion, and the strongest one here.
    //
    // SEEDED, AND THE SEED IS PRINTED WITH THE WHOLE ACTION TRACE ON FAILURE. An
    // unreproducible fuzz failure is a rumour: the next person has to be able to
    // re-drive the exact sequence. The SCRIPT is a pure function of the seed - it
    // is generated in full before the dialog is built, so no decision in it
    // depends on what the dialog does - and the trace records what each step
    // actually did, including the steps that were skipped because their
    // precondition did not hold.
    //
    // WHERE the click lands is fuzzed as well as WHICH click it is, because that
    // is the dimension every defect this dialog has had lived in: a click at the
    // top level and the same click delivered inside a suspended frame's
    // processEvents() reach completely different code. The three placements use
    // the file's existing insideframe primitives, which are synchronous seams and
    // not timing bets (see that block comment).
    //
    // NO WALL CLOCK. The store's nested loop is set to its shortest, completions
    // are PARKED (completeRead/completeWrite off) and delivered only when the
    // script says so, and nothing waits for a duration to decide anything. The
    // fixture's teardown timers are bounds, not sequencing.
    //
    // THE THIRD TEST-126 INVARIANT LIVES HERE, for the reason given in that block
    // comment: restart idempotence is an ACTION, and only a run that is already
    // clicking may perform it.
    enum FuzzAction {
        FzStart = 0,   // select the current tab's rows and press the transfer button
        FzAbort,       // ...press it again while it says Abort
        FzRefresh,     // Refresh List - the one place a row dies
        FzTab,         // switch tabs (Download / Upload / Sync)
        FzDeliver,     // hand back one completion the store has been holding
        FzIdempotence, // the restart-idempotence probe
        FzActionCount
    };

    enum FuzzPlacement {
        FzNow = 0,             // at the top level, between store calls
        FzInNextProcessEvents, // inside the next processEvents() anywhere -
                               // production's own, inside a completion slot or a
                               // parse-failure branch, when there is one
        FzAtNextCountedRow,    // inside the processEvents() of the completion slot
                               // that next counts a row on the progress bar
        FzPlacementCount
    };

    struct FuzzStep
    {
        int action = 0;
        int placement = 0;
        int arg = 0;
    };

    struct FuzzOutcome
    {
        bool completed = false; // the run reached its last line
        bool timedOut = false;
        QStringList trace;      // every step, in the order it ran
        QStringList violations; // TEST-126's, if any
        QStringList idempotence;
        QString summary;
    };

    static void fuzzSelectAll(CloudServiceSyncDialog* d, int tab, Qt::CheckState state)
    {
        switch (tab) {
        case 0:
            d->selectAllChanged(state);
            break;
        case 1:
            d->selectAllUpChanged(state);
            break;
        default:
            d->selectAllSyncChanged(state);
            break;
        }
    }

    // Everything about the batch state that CAN be read from outside, in one
    // string. `listindex` is NOT in it and cannot be: it is a private member of
    // CloudServiceSyncDialog with no widget behind it and this target has no
    // friend access. So the idempotence probe below is written against
    // `downloadcounter` (the progress bar's value) and `successful` (which reaches
    // the progress label) and says so rather than implying it covers all three.
    static QString fuzzStateSnapshot(QWidget* dialog)
    {
        QStringList parts;
        if (QProgressBar* bar = dialog->findChild<QProgressBar*>())
            parts << QStringLiteral("bar=%1/%2").arg(bar->value()).arg(bar->maximum());
        parts << QStringLiteral("label='%1'").arg(progressLabelText(dialog));
        const QList<QTreeWidget*> lists = dialog->findChildren<QTreeWidget*>();
        for (QTreeWidget* w : lists) {
            // The download list labels its status in column 5 (:1104); the upload
            // and sync lists in column 7 (:1133/:1172).
            const int col =
                (w->headerItem() != nullptr && w->headerItem()->text(1) == QStringLiteral("Workout Name")) ? 5 : 7;
            QStringList rows;
            for (int i = 0; i < w->invisibleRootItem()->childCount(); i++)
                rows << w->invisibleRootItem()->child(i)->text(col);
            parts << QStringLiteral("[%1]").arg(rows.join(QStringLiteral("|")));
        }
        return parts.join(QStringLiteral(" "));
    }

    FuzzOutcome runFuzz(quint32 seed, int steps)
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        oracle::reset();
        rideopen::blockingMs = 1;

        FuzzOutcome out;

        // ---- THE SCRIPT. Generated in full, here, before anything is built: it
        //      is a pure function of the seed and of nothing else.
        std::mt19937 rng(seed);
        // UNIFORM OVER THE ALPHABET, AND THAT WAS MEASURED RATHER THAN ASSUMED.
        //
        // A weighted table was tried first, on the reasonable-sounding ground that
        // Start and Deliver are the two actions that put work in flight and the
        // idempotence probe by construction leaves the dialog idle - so weighting
        // 4/2/2/1/4/1 raises the density of transfers per step. It does: over 512
        // seeds it found INV-1 violations in 40 of them against the uniform draw's
        // 10, and over 4096 seeds in 323 (7.9%).
        //
        // IT ALSO STOPPED FINDING THE OTHER DEFECT. The uniform draw reaches a
        // heap-use-after-free (completedRead's abort branch writing a row that
        // refreshClicked freed) at seed 447; the weighted one did not reach it in
        // 4096 seeds. Tuning for the density of the failure you have already seen
        // is how a fuzzer stops finding the ones you have not, and a mechanism
        // that only rediscovers the cheap defect is worth much less than one that
        // reaches both. So the weights are gone and this paragraph is what is left
        // of them: if the density is ever raised again, re-check that seed 447's
        // CLASS of finding is still reachable, not just that the count went up.
        QList<FuzzStep> script;
        for (int i = 0; i < steps; i++) {
            FuzzStep s;
            s.action = int(rng() % unsigned(FzActionCount));
            s.placement = int(rng() % unsigned(FzPlacementCount));
            s.arg = int(rng() % 9973u);
            script << s;
        }

        // ---- THE FIXTURE: two local activities (Upload rows on the Upload and
        //      Sync lists) and two remote ones (Download rows on the Download and
        //      Sync lists), so every driver and every completion slot is
        //      reachable and a batch is capable of having a SECOND row to carry on
        //      to.
        const QDir activities = context->athlete->home->activities();
        QDir().mkpath(activities.absolutePath());

        QList<RideItem*> items;
        QStringList paths;
        for (int i = 0; i < 2; i++) {
            const QString name = rebuildLocalActivity(i);
            QFile f(activities.absolutePath() + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("gcblock");
            f.close();
            paths << f.fileName();

            RideItem* item = new RideItem(nullptr, context);
            item->fileName = name;
            item->path = activities.absolutePath();
            item->dateTime = QDateTime(QDate::currentDate(), QTime(10 + i, 0, 0));
            item->planned = false; // the upload list skips planned rides (:1723)
            items << item;
        }
        for (RideItem* item : items)
            rideCache->rides().push_back(item);

        // Shared with the deferred actions, which can outlive the frame that armed
        // them, so heap-held rather than captured by reference.
        auto trace = std::make_shared<QStringList>();
        auto idem = std::make_shared<QStringList>();

        QEventLoop appLoop;
        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList() << rebuildRemoteActivity(0) << rebuildRemoteActivity(1);
                store->blockingMs = 1;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102):
                // uncompressRide's first guard rejects outright on the default.
                store->downloadCompression = CloudService::none;
                // EVERY completion is PARKED. The script decides which of them is
                // ever handed back, and when - which is what makes "two transfers
                // outstanding" a state this driver can reach at all.
                store->completeRead = false;
                store->completeWrite = false;
                store->writeSucceeds = true;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;

                const QPointer<CloudServiceSyncDialog> alive(dialog);
                const QPointer<CloudService> storeGuard(store);

                // ONE STEP. Captured BY VALUE throughout: a deferred placement can
                // fire after the frame that armed it has gone, and both subjects
                // are QPointers so a step that arrives after the dialog has been
                // destroyed is a no-op instead of a crash the fixture invented.
                auto perform = std::make_shared<std::function<void(FuzzStep)>>();
                *perform = [alive, storeGuard, trace, idem](FuzzStep s) {
                    CloudServiceSyncDialog* d = alive.data();
                    BlockingStore* st = qobject_cast<BlockingStore*>(storeGuard.data());
                    // The step's own record, appended AND (when GC_ORACLE_TRACE is
                    // set) interleaved with the oracle's events, so a violation can
                    // be read as one sequence rather than two.
                    const auto say = [trace](const QString& s) {
                        *trace << s;
                        oracle::trace(QStringLiteral("STEP ") + s);
                    };
                    if (d == nullptr || st == nullptr) {
                        say(QStringLiteral("(dialog gone)"));
                        return;
                    }
                    QTabWidget* tabs = d->findChild<QTabWidget*>();
                    QPushButton* close = oracle::buttonWithText(d, QStringLiteral("Close"));
                    // cancelButton hidden <=> `downloading` (see the oracle's
                    // block comment): this is how the driver knows whether the
                    // transfer button currently says Abort.
                    const bool busy = (close != nullptr && close->isHidden());
                    const int tab = tabs != nullptr ? tabs->currentIndex() : 2;

                    switch (s.action) {
                    case FzStart:
                        if (busy) {
                            say(QStringLiteral("start(skipped: already running)"));
                            break;
                        }
                        // A user selects and then presses; a Refresh leaves every
                        // box clear, so a driver that did not re-select would run
                        // empty batches for the rest of the script.
                        fuzzSelectAll(d, tab, Qt::Checked);
                        say(QStringLiteral("start(tab %1)").arg(tab));
                        d->downloadClicked();
                        break;

                    case FzAbort:
                        if (!busy) {
                            say(QStringLiteral("abort(skipped: idle)"));
                            break;
                        }
                        say(QStringLiteral("abort"));
                        d->downloadClicked();
                        break;

                    case FzRefresh:
                        say(QStringLiteral("refresh"));
                        d->refreshClicked();
                        break;

                    case FzTab: {
                        const int k = s.arg % 3;
                        say(QStringLiteral("tab(%1)").arg(k));
                        if (tabs != nullptr)
                            tabs->setCurrentIndex(k);
                        break;
                    }

                    case FzDeliver: {
                        if (st->parked.isEmpty()) {
                            say(QStringLiteral("deliver(skipped: nothing parked)"));
                            break;
                        }
                        // Removed as it is delivered: each parked completion is
                        // handed back AT MOST ONCE, so this driver can never
                        // double-free a read buffer the dialog has already
                        // released and blame production for it.
                        const BlockingStore::Parked p = st->parked.takeAt(s.arg % st->parked.count());
                        if (p.isWrite) {
                            say(QStringLiteral("deliver(write %1)").arg(p.name));
                            st->notifyWriteComplete(p.operationId, p.name, QStringLiteral("Completed."));
                        } else if (s.arg % 5 == 0) {
                            say(QStringLiteral("deliver(read-failed %1)").arg(p.name));
                            st->notifyReadFailed(p.token, p.name, QStringLiteral("service refused"));
                        } else {
                            say(QStringLiteral("deliver(read %1)").arg(p.name));
                            st->notifyReadComplete(p.token, p.name, QStringLiteral("Completed."));
                        }
                        break;
                    }

                    case FzIdempotence: {
                        // TEST-126's third invariant, checked by the run that is
                        // already clicking. PRECONDITION: idle, with nothing
                        // outstanding - which is the state the invariant is stated
                        // for - and with the current tab's rows CLEARED, so that
                        // neither pair of clicks may transfer anything and the
                        // only thing that can differ between them is bookkeeping.
                        if (busy || st->oracle_.liveOutstanding() != 0) {
                            say(QStringLiteral("idempotence(skipped: not idle)"));
                            break;
                        }
                        fuzzSelectAll(d, tab, Qt::Unchecked);
                        const int dispatchesBefore = oracle::dispatches;

                        d->downloadClicked();
                        d->downloadClicked();
                        const QString afterOne = fuzzStateSnapshot(d);

                        d->downloadClicked();
                        d->downloadClicked();
                        const QString afterTwo = fuzzStateSnapshot(d);

                        say(QStringLiteral("idempotence(tab %1)").arg(tab));
                        if (afterOne != afterTwo)
                            *idem << QStringLiteral("two no-op restart pairs did not leave what one did: "
                                                    "after one [%1] after two [%2]")
                                         .arg(afterOne)
                                         .arg(afterTwo);
                        if (oracle::dispatches != dispatchesBefore)
                            *idem << QStringLiteral("a batch with nothing selected issued %1 store call(s)")
                                         .arg(oracle::dispatches - dispatchesBefore);
                        break;
                    }

                    default:
                        break;
                    }
                };

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(60000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });

                for (int i = 0; i < script.count(); i++) {
                    const FuzzStep s = script.at(i);
                    if (alive.isNull())
                        break;
                    switch (s.placement) {
                    case FzInNextProcessEvents:
                        *trace << QStringLiteral("-> armed for the next processEvents()");
                        oracle::trace(QStringLiteral("ARM -> next processEvents()"));
                        insideframe::atNextProcessEvents([perform, s]() { (*perform)(s); });
                        break;
                    case FzAtNextCountedRow:
                        *trace << QStringLiteral("-> armed for the next counted row");
                        oracle::trace(QStringLiteral("ARM -> next counted row"));
                        insideframe::atTheNextCountedRow(dialog, [perform, s]() { (*perform)(s); });
                        break;
                    default:
                        (*perform)(s);
                        break;
                    }
                    // Let whatever that step started (and anything armed for the
                    // next processEvents) actually run before the next one.
                    //
                    // A FIXED NUMBER OF PASSES, not a millisecond budget. The
                    // obvious spelling is processEvents(AllEvents, 2), and it
                    // makes how much of the queue a step drains depend on how
                    // busy the machine is - which would make a seed's outcome
                    // depend on the load rather than on the seed, and an
                    // irreproducible fuzz finding is a rumour. Eight passes is
                    // enough for a completion slot's own processEvents to be
                    // reached and for the one-shots armed inside it to fire.
                    for (int pass = 0; pass < 8; pass++)
                        QApplication::processEvents(QEventLoop::AllEvents);
                }

                out.completed = true;
                delete owner; // takes the dialog, which closes and deletes the store
                QTimer::singleShot(0, &appLoop, &QEventLoop::quit);
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        for (RideItem* item : items) {
            rideCache->rides().removeAll(item);
            delete item->ride(false);
            delete item;
        }
        for (const QString& p : paths)
            QFile::remove(p);
        // ...AND ANYTHING A COMPLETED DOWNLOAD SAVED. saveRide writes the parsed
        // activity into the athlete's folder (CloudService.cpp:2946), and
        // refreshClicked reads that folder to decide whether a remote row already
        // "exists" locally - so a run that left one behind would hand the NEXT
        // seed a different starting fixture and every seed after the first would
        // be running a batch that skips its rows. One seed, one clean fixture, is
        // the whole of what makes a seed reproducible.
        for (int i = 0; i < 2; i++)
            QFile::remove(activities.absolutePath() + "/" + rebuildRemoteActivity(i));

        out.trace = *trace;
        out.violations = oracle::violations;
        out.idempotence = *idem;
        out.summary = oracle::summary();
        return out;
    }

    // The failure report: the seed first, then every step in order. Anyone can
    // re-drive exactly this run with GC_FUZZ_SEED.
    static QString fuzzReport(quint32 seed, const FuzzOutcome& out)
    {
        return QStringLiteral("\n  SEED %1 (re-run just this one with GC_FUZZ_SEED=%1)\n  %2\n  TRACE:\n    %3\n")
            .arg(seed)
            .arg(out.summary)
            .arg(out.trace.join(QStringLiteral("\n    ")));
    }

  private slots:
    // -- TEST-127 (REQ-028) ----------------------------------------------
    // RANDOM CLICK SEQUENCES, JUDGED BY TEST-126 AND BY THE PROCESS.
    void randomClickSequencesMustNotBreakTheTransferInvariants()
    {
        // THE BUDGET. Every seed is an independent dialog, driven through `steps`
        // actions; both numbers are printed by the failure report so a run that
        // finds nothing says exactly what it covered.
        //
        // 512 x 24 CHOSEN FOR A REASON THAT IS NOT "IT FEELS LIKE ENOUGH": it is
        // the smallest round range that contains BOTH classes of finding this
        // driver made on the tree it was written against - the invariant
        // violation (first at seed 32, then 77, 128, 244, 271, 312, 336, 395, 402,
        // 410) and the heap-use-after-free (seed 447). No seed is pinned or listed
        // in the code: the RANGE is stated and the driver re-derives them, which
        // is the difference between a fuzzer and a hand-written slot wearing one's
        // clothes. Measured cost, seeds 1-446 under QPA offscreen: 17.0s, i.e.
        // ~38ms per seed, so ~20s per backend on top of this target's ~49s.
        //
        // GC_FUZZ_SEEDS widens it and GC_FUZZ_SURVEY reports every failing seed in
        // the range instead of stopping at the first.
        int seeds = 512;
        int steps = 24;
        quint32 first = 1;

        // One seed, on demand, for re-driving a discovery.
        if (qEnvironmentVariableIsSet("GC_FUZZ_SEED")) {
            first = quint32(qEnvironmentVariableIntValue("GC_FUZZ_SEED"));
            seeds = 1;
        }
        if (qEnvironmentVariableIsSet("GC_FUZZ_SEEDS"))
            seeds = qEnvironmentVariableIntValue("GC_FUZZ_SEEDS");
        if (qEnvironmentVariableIsSet("GC_FUZZ_STEPS"))
            steps = qEnvironmentVariableIntValue("GC_FUZZ_STEPS");

        // SURVEY MODE, off by default. The verdict below stops at the FIRST
        // failing seed, which is what a gate should do; a survey wants to know how
        // MANY of a range fail and which, and an ASan abort ends the process
        // without a report - so this mode names each seed before it runs it and
        // carries on past a violation instead of stopping. Diagnosis, not verdict:
        // it still fails at the end if anything failed.
        const bool survey = qEnvironmentVariableIsSet("GC_FUZZ_SURVEY");
        QStringList surveyFailures;

        for (int i = 0; i < seeds; i++) {
            const quint32 seed = first + quint32(i);
            if (survey)
                qInfo("FUZZSEED %u", seed);
            const FuzzOutcome out = runFuzz(seed, steps);

            if (survey) {
                if (!out.violations.isEmpty() || !out.idempotence.isEmpty() || out.timedOut || !out.completed)
                    surveyFailures << QStringLiteral("seed %1: %2")
                                          .arg(seed)
                                          .arg((out.violations + out.idempotence).join(QStringLiteral(" / ")));
                continue;
            }

            // THE ASAN ORACLE. Under halt_on_error=1 a use-after-free ends the
            // process, so reaching this line at all is the assertion; `completed`
            // is the guard against a run that came back for some other reason.
            QVERIFY2(!out.timedOut, qPrintable(QStringLiteral("the run never came back.") + fuzzReport(seed, out)));
            QVERIFY2(out.completed,
                     qPrintable(QStringLiteral("the run did not reach its last line.") + fuzzReport(seed, out)));

            // THE TEST-126 ORACLE.
            QVERIFY2(out.violations.isEmpty(),
                     qPrintable(QStringLiteral("TEST-126 invariant violated:\n  ") +
                                out.violations.join(QStringLiteral("\n  ")) + fuzzReport(seed, out)));
            QVERIFY2(out.idempotence.isEmpty(),
                     qPrintable(QStringLiteral("restart idempotence broken:\n  ") +
                                out.idempotence.join(QStringLiteral("\n  ")) + fuzzReport(seed, out)));
        }

        if (survey && !surveyFailures.isEmpty())
            QFAIL(qPrintable(QStringLiteral("%1 of %2 seeds failed:\n  ").arg(surveyFailures.count()).arg(seeds) +
                             surveyFailures.join(QStringLiteral("\n  "))));
    }

  private:
    // =====================================================================
    // TEST-140 (A3-R028c-F7, REQ-028 (e), DEC-garmin-034) —
    // A CLOSE DELIVERED FROM INSIDE saveRide.
    // =====================================================================
    //
    // THE GUARD. completedRead wraps its saveRide call in a BlockingCall
    // (CloudService.cpp:3078). saveRide is not a store call, but it SUSPENDS -
    // DataProcessorFactory::autoProcess (:3569) runs every processor whose
    // configKeyAutomation is "Auto", and FixElevation's postProcess waits on an
    // HTTPS round trip in an untimed QEventLoop (FixElevation.cpp:288-300). A
    // window X delivered into that loop reaches closeEvent -> deferCloseIfBusy
    // (:1494), and it is the DEPTH THIS FRAME RAISES that turns the close into a
    // deferral. Without it deferCloseIfBusy returns false, QDialog's
    // WA_DeleteOnClose path posts the DeferredDelete that same loop then
    // delivers, and saveRide resumes on freed memory: `context` at :3576 is a
    // member READ and `rideFiles` at :3579 a member WRITE, both BELOW the
    // suspension and both ABOVE any guard completedRead has.
    //
    // WHY THE 89-SLOT SUITE COULD NOT SEE IT (B-R028-13). Removing that one line
    // left the whole suite green: no run in this file had ever delivered a close
    // into saveRide, so the frame it protects was never entered. Every other
    // BlockingCall in the file wraps a STORE call, and the store stub's
    // fireActionThenBlock is the only suspension the close routes were ever
    // driven into. The guard shipped labelled "reasoned, not traced".
    //
    // WHY THIS RUN CAN. The autoProcess seam (stubs/ImportSeamStubs.cpp:136-137,
    // fired at :452-465) puts a real nested QEventLoop back where production has
    // one. It is TEST-121's seam and TEST-121's arming shape, aimed at a
    // different question: TEST-121 delivers a REFRESH there and is about the row,
    // this one delivers the WINDOW X and is about the dialog.
    //
    // THE CLOSE IS QUEUED AND THE LOOP DELIVERS IT, which is not a detail: an
    // event delivered through QCoreApplication::notifyInternal2 runs one
    // scopeLevel deeper than the loop that dispatched it, so a DeferredDelete
    // posted from inside it OUTRANKS that loop and is delivered by it. Called
    // synchronously instead, the deleteLater would sit undelivered until the
    // stack unwound and the run would prove nothing. This is exactly why
    // BlockingStore::fireActionThenBlock queues its own action.
    //
    // THE WINDOW X RATHER THAN THE Close BUTTON: cancelClicked's button is
    // HIDDEN for the duration of a batch (CloudService.cpp:2198), so during a
    // sync the routes a user actually has are the title-bar X (closeEvent) and
    // Escape (done()). Both funnel through deferCloseIfBusy; this run drives the
    // first, as TEST-070 does.
    //
    // THE SECOND GUARD IN THAT BLOCK IS NOT DRIVABLE HERE, AND THE MEASUREMENT
    // THAT DECIDED IT IS THIS FIXTURE (2026-08-23, run T-141 probe, offscreen).
    // A3-R028c-F7 proposed a companion slot for the `self.isNull()` bail at
    // CloudService.cpp:3086, on the reading that the athlete-tab teardown - which
    // the deferral above CANNOT intercept, because Qt destroys child widgets
    // straight from ~QObject - reaches it. IT DOES NOT REACH IT. Driven into
    // this same seam (`delete owner` queued into the nested loop, TEST-136's
    // TeardownInsideBatch shape), with :3086 fully present and unmutated, the
    // run dies BEFORE completedRead resumes at all:
    //
    //   ==NNNN==ERROR: AddressSanitizer: heap-use-after-free READ of size 8
    //     #0 CloudServiceSyncDialog::saveRide(...)      CloudService.cpp:3576
    //     #1 CloudServiceSyncDialog::completedRead(...) CloudService.cpp:3079
    //    freed by thread T0 here:
    //     #0 operator delete(void*, unsigned long)
    //     #1 QObjectPrivate::deleteChildren()
    //
    // The first half of the reason is structural, not a fixture artefact:
    // saveRide touches `this` TWICE below its own suspension point - `context`
    // at :3576 and `rideFiles` at :3579 - so any destruction the BlockingCall
    // cannot defer has already been dereferenced three statements before :3086
    // can run.
    //
    // THE SECOND HALF IS WEAKER THAN AN EARLIER DRAFT OF THIS BLOCK CLAIMED, AND
    // THE CORRECTION MATTERS. The only statement between saveRide's return and
    // :3086 is `delete ride` (CloudService.cpp:3084), and it is tempting - and
    // FALSE - to say that a delete cannot pump events. ~RideFile() OPENS with
    // `emit deleted();` (src/FileIO/RideFile.cpp:118-120), and a directly
    // connected slot would run synchronously inside that delete, which is exactly
    // the kind of code that reaches an event loop. What actually holds is
    // narrower and CONNECTION-DEPENDENT: at HEAD the only two connectors of
    // RideFile::deleted() in the whole tree are XDataTableModel
    // (XDataTableModel.cpp:48) and RideFileTableModel (RideFileTableModel.cpp:48),
    // both Ride Editor table models that attach themselves to a ride they are
    // DISPLAYING - and neither is attached to the freshly-parsed, never-displayed
    // temporary ride completedRead hands to saveRide and then deletes here. For
    // THIS instance the emission therefore has no receivers and is a no-op, and
    // only because of that does no route exist on which `self` is null at :3086
    // and was not null INSIDE saveRide.
    //
    // SO THE GUARANTEE IS FRAGILE, NOT STRUCTURAL. It rests on a fact about the
    // current connection graph rather than on anything the code enforces. The day
    // any future code connects a live observer to that ride instance before
    // `delete ride`, the emission becomes a call into arbitrary code, the route to
    // :3086 reopens SILENTLY, and nothing in this suite stands guard over it.
    //
    // :3086 therefore stays reasoned-not-traced, with this run as the record of
    // why; a slot claiming otherwise would have to fabricate a destruction route
    // production does not have TODAY. This is REPORTED, not fixed: changing
    // saveRide is outside this slice.
    //
    // WHAT ASSERTS THE CRITERION. Half of it is the PROCESS: this target is built
    // with -fsanitize=address and run with halt_on_error=1, so a use-after-free
    // in saveRide ends the binary and no later slot runs at all. The other half
    // is behavioural and is asserted below, because "it did not crash" is also
    // satisfied by a dialog that wedged, by a batch that never reached saveRide,
    // and by a deferral that swallowed the user's close for good.
    struct SaveRideCloseOutcome
    {
        bool timedOut = false;

        // -- premises: did this run reach the situation it claims to test?
        int listCount = 0;
        int checkedRows = 0;
        QString row0Action; // sync list column 6: "Download" for a remote row
        int autoProcessCalls = 0;
        int rideOpens = 0;
        int readFileCalls = 0;
        bool closeDeliveredInsideTheSeam = false;

        // -- the verdict
        int dialogAliveAtSeamTail = -1;            // was `this` still there when saveRide resumed?
        int dialogAliveAfterTheBatch = -1;         // ...and when the batch's frame returned
        bool dialogGoneBeforeOwnerDeleted = false; // the deferral was REPLAYED, not swallowed
        bool storeGoneAtEnd = false;               // ...and DEC-031 reaped the store
        QStringList statuses;
        QString progressText;
        QString buttonTextAtEnd;
    };

    // One run:
    //
    //   QEventLoop (stands in for QApplication::exec())
    //     -> queued call [event delivery]
    //          -> owner QWidget, and a WA_DeleteOnClose dialog inside it
    //          -> Sync tab / Select all / Synchronize
    //               -> syncNext -> store->readFile -> queued completion
    //                    -> completedRead -> saveRide -> autoProcess
    //                         -> THE SEAM: queue the window X, then a nested loop
    //                              -> dialog->close()
    //                         -> saveRide RESUMES on `this`
    //
    // Nothing in this fixture deletes the dialog: the dialog is supposed to
    // outlive saveRide and then delete itself.
    SaveRideCloseOutcome runCloseInsideSaveRide()
    {
        obs::reset();
        rideopen::reset();
        ridefail::reset();
        rideopen::blockingMs = 5; // the READER must not block: the delivery has to
                                  // land inside saveRide, not inside uncompressRide
        gcstub::autoProcessAction = nullptr;
        gcstub::autoProcessCalls = 0;

        SaveRideCloseOutcome out;
        QEventLoop appLoop;
        QPointer<CloudServiceSyncDialog> dialogGuard;
        QPointer<CloudService> storeGuard;

        QMetaObject::invokeMethod(
            this,
            [&]() {
                QWidget* owner = new QWidget;
                QPointer<QWidget> ownerGuard(owner);

                BlockingStore* store = new BlockingStore(context);
                store->entryNames = QStringList() << rebuildRemoteActivity(0) << rebuildRemoteActivity(1);
                store->blockingMs = 5;
                store->closeActionContext = qApp;
                // GarminConnect's own setting (GarminConnect.cpp:102):
                // uncompressRide's first guard rejects outright on the default.
                store->downloadCompression = CloudService::none;

                CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
                dialog->setParent(owner, Qt::Dialog);
                // VERBATIM the production line (MainWindow.cpp:2605,
                // AddCloudWizard.cpp:899): the dialog owns itself, and a close is
                // what FREES it. Without this attribute a close is a hide and
                // this run would be about nothing.
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->start();
                dialog->open();
                store->dialog = dialog;
                store->dialogGuard = dialog;
                dialogGuard = dialog;
                storeGuard = store;

                if (QTabWidget* tabs = dialog->findChild<QTabWidget*>())
                    tabs->setCurrentIndex(2);
                dialog->selectAllSyncChanged(Qt::Checked);
                QPointer<QTreeWidget> list(rideListWithHeader(dialog, QStringLiteral("Source")));

                if (!list.isNull()) {
                    QTreeWidgetItem* root = list->invisibleRootItem();
                    out.listCount = root->childCount();
                    for (int i = 0; i < out.listCount; i++) {
                        QCheckBox* check = qobject_cast<QCheckBox*>(list->itemWidget(root->child(i), 0));
                        if (check != nullptr && check->isChecked())
                            out.checkedRows++;
                    }
                    if (out.listCount > 0)
                        out.row0Action = root->child(0)->text(6);
                }

                QPushButton* button = pushButtonWithText(dialog, QStringLiteral("Synchronize"));

                // saveRide REFUSES before it ever reaches autoProcess when the
                // target .json already exists and this box is clear
                // (CloudService.cpp:3559-3563), and by the time this slot runs
                // TEST-121 has already written that file. Ticked through the real
                // widget, by its label; the autoProcessCalls premise MEASURES the
                // result rather than trusting it.
                for (QCheckBox* box : dialog->findChildren<QCheckBox*>())
                    if (box->text().contains(QStringLiteral("Overwrite")))
                        box->setChecked(true);

                gcstub::autoProcessAction = [&, dialogGuard]() {
                    // THE PREMISE THIS RUN STANDS ON, recorded where it actually
                    // happens. What has to be true is not "the seam fired" - that
                    // is out.autoProcessCalls, and a flag set here would say
                    // nothing more - but "the queued close was delivered WHILE
                    // THE NESTED LOOP BELOW WAS LIVE". Only an event dispatched
                    // by that loop runs a scopeLevel deeper than it, and only
                    // then does the DeferredDelete a successful close posts
                    // outrank it. Delivered instead by the OUTER appLoop, or by
                    // the drain after it, the whole BlockingCall frame has
                    // already unwound: the close would take the ordinary
                    // undeferred path, the dialog would still be gone before the
                    // owner, and every other assertion below would pass on
                    // nothing. So the flag is written by the QUEUED LAMBDA
                    // ITSELF and read back only once the loop has closed.
                    //
                    // A shared bool rather than a capture of a stack local: the
                    // very failure this flag exists to catch is a delivery that
                    // lands after this action has returned, and a pointer or
                    // reference into that dead frame would be written through at
                    // exactly that moment. The flag outlives the loop and the
                    // queued call alike, whichever of them wins.
                    const std::shared_ptr<bool> closeCallbackRan = std::make_shared<bool>(false);

                    // QUEUED, so the loop below delivers it one scopeLevel deeper
                    // and the DeferredDelete a successful close would post
                    // outranks that loop. See the block comment above.
                    QMetaObject::invokeMethod(
                        qApp,
                        [dialogGuard, closeCallbackRan]() {
                            *closeCallbackRan = true;
                            if (!dialogGuard.isNull())
                                dialogGuard->close(); // the window X
                        },
                        Qt::QueuedConnection);

                    // FixElevation's loop, in shape: untimed there, bounded here.
                    QEventLoop loop;
                    QTimer timer;
                    timer.setSingleShot(true);
                    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
                    timer.start(300);
                    loop.exec(QEventLoop::WaitForMoreEvents);

                    // Read the instant the loop closes and not one statement
                    // later: anything after this point could be a delivery from
                    // a shallower frame, which is the case this flag exists to
                    // reject.
                    out.closeDeliveredInsideTheSeam = *closeCallbackRan;

                    // saveRide is about to resume and touch `this` twice. Whether
                    // `this` is still there is the whole of the criterion, and it
                    // is read from a QPointer that never dereferences anything.
                    out.dialogAliveAtSeamTail = int(!dialogGuard.isNull());
                };

                dialog->downloadClicked(); // -> syncNext()

                out.dialogAliveAfterTheBatch = int(!dialogGuard.isNull());
                out.autoProcessCalls = gcstub::autoProcessCalls;
                out.rideOpens = rideopen::opens;
                out.readFileCalls = obs::readFileCalls;
                if (!dialogGuard.isNull()) {
                    if (!list.isNull()) {
                        QTreeWidgetItem* root = list->invisibleRootItem();
                        for (int i = 0; i < root->childCount(); i++)
                            out.statuses << root->child(i)->text(7);
                    }
                    out.progressText = progressLabelText(dialog);
                    if (button != nullptr)
                        out.buttonTextAtEnd = button->text();
                }

                // WHERE "the close was replayed" IS SEPARATED FROM "the owner
                // took it with it": sampled BEFORE the owner goes.
                QTimer::singleShot(300, qApp, [&, ownerGuard]() {
                    out.dialogGoneBeforeOwnerDeleted = dialogGuard.isNull();
                    if (!ownerGuard.isNull())
                        delete ownerGuard.data();
                });
                QTimer::singleShot(400, &appLoop, &QEventLoop::quit);

                bool* timedOutp = &out.timedOut;
                QTimer::singleShot(20000, &appLoop, [timedOutp]() {
                    *timedOutp = true;
                    QCoreApplication::exit(1);
                });
            },
            Qt::QueuedConnection);

        appLoop.exec();
        for (int i = 0; i < 50; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

        gcstub::autoProcessAction = nullptr;
        out.storeGoneAtEnd = storeGuard.isNull();
        return out;
    }

  private slots:
    // -- TEST-140 (A3-R028c-F7, REQ-028 (e), DEC-garmin-034) -------------
    // THE USER'S CLOSE IS DEFERRED ACROSS saveRide, NOT HONOURED UNDER IT.
    //
    // RED, by mutation (2026-08-23, both QPA backends): with the BlockingCall at
    // CloudService.cpp:3078 removed and its braces left in place, the run dies -
    // measured VERBATIM, under `offscreen` and under `minimal` alike:
    //   ==NNNN==ERROR: AddressSanitizer: heap-use-after-free
    //                  READ of size 8 ... thread T0
    //     #0 CloudServiceSyncDialog::saveRide(...)     CloudService.cpp:3576
    //     #1 CloudServiceSyncDialog::completedRead(...) CloudService.cpp:3079
    //     ...
    //     #22 CloudServiceSyncDialog::syncNext()        CloudService.cpp:2444
    //    freed by thread T0 here:
    //     #0 operator delete(void*, unsigned long)
    //     #1 QObject::event(QEvent*)            <- the DeferredDelete this run's
    //                                             own nested loop delivered
    //   SUMMARY: AddressSanitizer: heap-use-after-free CloudService.cpp:3576 in
    //            CloudServiceSyncDialog::saveRide(RideFile*, QList<QString>&)
    void aCloseDeliveredInsideSaveRideMustNotFreeTheDialogUnderIt()
    {
        const SaveRideCloseOutcome out = runCloseInsideSaveRide();
        const QString where = QStringLiteral("TEST-140 close in saveRide: ");
        qInfo("TEST-140 [%s] rows=%d checked=%d row0=\"%s\" autoProcess=%d opens=%d reads=%d closeInSeam=%d "
              "aliveAtSeamTail=%d aliveAfterBatch=%d goneBeforeOwner=%d storeGone=%d statuses=[%s] progress=\"%s\" "
              "button=\"%s\"",
              qPrintable(QString::fromLatin1(qgetenv("QT_QPA_PLATFORM"))), out.listCount, out.checkedRows,
              qPrintable(out.row0Action), out.autoProcessCalls, out.rideOpens, out.readFileCalls,
              int(out.closeDeliveredInsideTheSeam), out.dialogAliveAtSeamTail, out.dialogAliveAfterTheBatch,
              int(out.dialogGoneBeforeOwnerDeleted), int(out.storeGoneAtEnd), qPrintable(out.statuses.join(QChar('|'))),
              qPrintable(out.progressText), qPrintable(out.buttonTextAtEnd));

        // ---- PREMISES. A run that never reached the window must fail loudly
        //      rather than pass on nothing (LSN-047, LSN-050).
        QVERIFY2(!out.timedOut,
                 qPrintable(where + QStringLiteral("the run never came back - the deferral wedged the dialog")));
        QVERIFY2(out.listCount == 2,
                 qPrintable(where + QStringLiteral("the sync list held %1 rows, not 2").arg(out.listCount)));
        QVERIFY2(out.checkedRows == 2,
                 qPrintable(where + QStringLiteral("%1 rows were checked, not 2").arg(out.checkedRows)));
        QVERIFY2(out.row0Action == QStringLiteral("Download"),
                 qPrintable(where + QStringLiteral("row[0] is a \"%1\" row, not a Download - completedRead is only "
                                                   "reached through the download side")
                                        .arg(out.row0Action)));
        QVERIFY2(out.rideOpens >= 1,
                 qPrintable(where + QStringLiteral("uncompressRide never parsed a ride, so completedRead's "
                                                   "ride-bearing branch and its saveRide were never reached")));
        QVERIFY2(out.closeDeliveredInsideTheSeam,
                 qPrintable(where + QStringLiteral("the queued close was NOT delivered while the seam's own nested "
                                                   "loop was live - it was left to a shallower frame (the outer loop "
                                                   "or the drain behind it), by which time the BlockingCall had "
                                                   "unwound, so nothing below this line says anything about a close "
                                                   "arriving one scopeLevel deeper than a suspended saveRide")));

        // ---- THE CRITERION, first half: `this` outlived the call it was
        //      suspended in. saveRide reads `context` and writes `rideFiles`
        //      after the seam returns; under ASan with halt_on_error=1 the
        //      process would already be gone if it had not.
        QVERIFY2(out.dialogAliveAtSeamTail == 1,
                 qPrintable(where + QStringLiteral("the dialog was destroyed while saveRide was still on the stack "
                                                   "(aliveAtSeamTail=%1) - the close was honoured under a suspended "
                                                   "frame instead of being deferred")
                                        .arg(out.dialogAliveAtSeamTail)));

        // ---- ...and saveRide RAN TO COMPLETION through the suspension. It makes
        //      exactly two autoProcess calls ("Auto"/Import, then "Save"/ADD) and
        //      the seam is one-shot on the first, so 2 is "one row saved, all the
        //      way past the close"; 1 would be a saveRide that never came back
        //      from the seam, and 4 a batch that carried on to row[1].
        QVERIFY2(out.autoProcessCalls == 2,
                 qPrintable(where + QStringLiteral("autoProcess was called %1 time(s), not 2 - saveRide either did not "
                                                   "finish the row it was on or the batch carried on past the user's "
                                                   "close")
                                        .arg(out.autoProcessCalls)));

        // ---- THE CLOSE WAS HONOURED AS AN ABORT. deferCloseIfBusy sets
        //      `aborted` as well as `closeDeferred` (:1498-1499), which is what
        //      makes deferring honest rather than a shrug: completedRead's tail
        //      reads it (:3125) and the batch stands down, so the second checked
        //      row is never fetched.
        QVERIFY2(out.readFileCalls == 1,
                 qPrintable(where + QStringLiteral("%1 store->readFile call(s) were issued - the batch carried on "
                                                   "downloading behind a dialog the user had already dismissed")
                                        .arg(out.readFileCalls)));

        // ---- THE WEDGE CONTROL (LSN-050). Everything above is satisfiable by a
        //      guard that can never be lowered again - which would leave the
        //      dialog permanently un-closable, a worse bug than the crash. The
        //      deferred close must be REPLAYED from ~BlockingCall, and it must be
        //      the replay that frees the dialog rather than the owner's teardown.
        QVERIFY2(out.dialogGoneBeforeOwnerDeleted,
                 qPrintable(where + QStringLiteral("the dialog was still alive when the batch was over - the deferral "
                                                   "swallowed the user's close instead of replaying it")));
        // ...and DEC-031's half of the same unwinding: the store the dialog owns
        // goes with it, closed and deleted once no frame is executing on it.
        QVERIFY2(out.storeGoneAtEnd,
                 qPrintable(where + QStringLiteral("the store outlived the dialog - the reaper never released it")));
    }
};

QTEST_MAIN(TestGarminConnectSyncDialogClose)
#include "testGarminConnectSyncDialogClose.moc"
