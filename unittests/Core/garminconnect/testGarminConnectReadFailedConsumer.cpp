/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-069 — DEC-garmin-023, the CONSUMER half: what a service gains
// by emitting readFailed.
//
// Acceptance criterion (verbatim from the briefing):
//   "the consumer contract: on `readFailed` the loop ADVANCES and the buffer is
//    FREED (this is the actual defect — assert advancement, not merely that a
//    signal fired), and a positive control that the ~15 sibling services are
//    unaffected (a service that never emits `readFailed` behaves exactly as
//    before)."
//
// WHY A SIGNAL ON ITS OWN IS WORTH NOTHING (LSN-033)
// -------------------------------------------------
// The defect being closed is a STALL plus a LEAK, not a missing notification:
// CloudServiceSyncDialog::syncNext/downloadNext preallocate `new QByteArray`,
// call store->readFile(...), DISCARD the returned bool and then wait for a signal
// to advance. A service that refuses in silence leaves the dialog parked on
// "Downloading n of N" forever and leaks that buffer once per attempt. So this
// file asserts the two things that actually go wrong:
//   * ADVANCEMENT — the store is asked for activity 2 and activity 3 after
//     activity 1 was refused, and the dialog reaches its terminal progress text.
//     "readFailed was emitted" is deliberately NOT the assertion.
//   * THE FREE — the exact heap block the dialog allocated is released, exactly
//     once. See the bufferwatch note below for how that is made deterministic;
//     "exactly once" is load-bearing because completedRead already does
//     `delete data`, so a second delivery channel for the SAME buffer is
//     precisely how a fixed leak becomes a double free.
//
// WHAT IS REAL HERE
// -----------------
// The REAL src/Cloud/CloudService.cpp is compiled in, so the class under test is
// the REAL CloudServiceSyncDialog: its real ctor, its real refresh/select/
// download slots, its real syncNext loop, its real completedRead, and the real
// uncompressRide the positive control's payload is measured against. Nothing on
// the path under test is stubbed. The dialog is driven ONLY through its public
// slots and observed ONLY through its real widget tree, so the test cannot
// accidentally assert on a private detail it also set up.
//
// The application layers CloudService.cpp merely references are satisfied by the
// same link-level stand-ins TEST-067 built (stubs/ImportSeamStubs.cpp) plus
// stubs/SyncDialogSeamStubs.cpp for the two symbols the DIALOG additionally
// touches but the import boundary did not (RideCache's ctor/dtor). Python-free,
// so this stays on the `garmin-fast` label.
//
// NOT COVERED HERE — and honestly so:
//   CloudServiceAutoDownload's half of the contract (its readFailed slot and the
//   `connect(..., &loop, SLOT(quit()))` that releases its blocking per-activity
//   QEventLoop) is NOT driven, because reaching it means running
//   CloudServiceAutoDownload::run() — which enumerates the whole service factory
//   off appsettings, walks the athlete's RideCache, spawns a QThread and
//   sleep(3)s. That is a program, not a seam. Both halves of it are verified by
//   inspection and reported as a residual.

#include "Athlete.h"
#include "CloudService.h"
#include "Context.h"
#include "RideCache.h"
#include "RideFile.h"

#include <QApplication>
#include <QByteArray>
#include <QCheckBox>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QLabel>
#include <QList>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QtTest/QtTest>

#include <atomic>
#include <cstdlib>
#include <new>

// ---------------------------------------------------------------------------
// bufferwatch — a deterministic "was THIS heap block released?" probe.
//
// The buffer under test is allocated by PRODUCTION code (`new QByteArray` inside
// syncNext), so the test cannot instrument it at the call site; the only place
// its release is observable is the global deallocation function. Replacing
// ::operator new/delete for this one test executable is therefore the seam.
//
// The subtle part: glibc's allocator is LIFO, so the next `new QByteArray` after
// a free reliably lands on the SAME address — naively counting frees per address
// would report the second buffer's release as the first buffer's double free.
// So a WATCHED block is never actually handed back to the allocator: its address
// is quarantined for the life of the process. That costs a few 24-byte leaks in
// a test binary and buys two exact properties:
//   * no later allocation can ever reuse a watched address, so a counted free is
//     unambiguously a free of THAT buffer;
//   * a second `delete` of the same buffer is likewise unambiguous and is
//     COUNTED rather than being undefined behaviour that may or may not abort.
// ---------------------------------------------------------------------------
namespace bufferwatch {

constexpr int kMaxWatched = 32;

// Constant-initialised, so these are live before any static ctor (and before any
// allocation the Qt libraries make on the way to main()).
std::atomic<void*> g_watched[kMaxWatched];
std::atomic<int> g_frees[kMaxWatched];
std::atomic<int> g_count{0};

void reset()
{
    // Watched addresses are quarantined forever, so slots are never recycled;
    // reset only rewinds the cursor for a fresh scenario.
    for (int i = 0; i < kMaxWatched; ++i) {
        g_watched[i].store(nullptr, std::memory_order_relaxed);
        g_frees[i].store(0, std::memory_order_relaxed);
    }
    g_count.store(0, std::memory_order_relaxed);
}

// Start watching `p`; returns its slot, or -1 if the (generous) table is full.
int watch(void* p)
{
    const int slot = g_count.fetch_add(1, std::memory_order_relaxed);
    if (slot < 0 || slot >= kMaxWatched)
        return -1;
    g_watched[slot].store(p, std::memory_order_relaxed);
    return slot;
}

int frees(int slot)
{
    if (slot < 0 || slot >= kMaxWatched)
        return -1;
    return g_frees[slot].load(std::memory_order_relaxed);
}

// Returns true when `p` is watched — in which case the block is QUARANTINED and
// must NOT be returned to the allocator.
bool noteFree(void* p)
{
    const int n = g_count.load(std::memory_order_relaxed);
    const int upto = n < kMaxWatched ? n : kMaxWatched;
    for (int i = 0; i < upto; ++i) {
        if (g_watched[i].load(std::memory_order_relaxed) == p) {
            g_frees[i].fetch_add(1, std::memory_order_relaxed);
            return true;
        }
    }
    return false;
}

} // namespace bufferwatch

void* operator new(std::size_t n)
{
    void* p = std::malloc(n ? n : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t n)
{
    void* p = std::malloc(n ? n : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept
{
    return std::malloc(n ? n : 1);
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept
{
    return std::malloc(n ? n : 1);
}

void operator delete(void* p) noexcept
{
    if (p && bufferwatch::noteFree(p))
        return; // quarantined - see the note above
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    ::operator delete(p);
}
void operator delete(void* p, std::size_t) noexcept
{
    ::operator delete(p);
}
void operator delete[](void* p, std::size_t) noexcept
{
    ::operator delete(p);
}
void operator delete(void* p, const std::nothrow_t&) noexcept
{
    ::operator delete(p);
}
void operator delete[](void* p, const std::nothrow_t&) noexcept
{
    ::operator delete(p);
}

// ---------------------------------------------------------------------------
// SpyStore — a CloudService that reports its reads on ONE of the two channels.
//
// `Channel::Failure` is what a DEC-023 service (GarminConnect) does; `Channel::
// CompletionOnly` is what all ~15 siblings do and have always done — it is the
// POSITIVE CONTROL, and it is the same class so the two runs differ in exactly
// one respect.
//
// Both post QUEUED, because that is the production shape: GarminConnect defers
// through m_completionContext (B-R007-01), and the dialog observes the post in
// the QApplication::processEvents() that syncNext runs right after readFile.
// ---------------------------------------------------------------------------
class SpyStore : public CloudService
{
  public:
    enum class Channel { Failure, CompletionOnly };

    SpyStore(Context* context, Channel channel) : CloudService(context), channel_(channel) {}

    CloudService* clone(Context* context) override { return new SpyStore(context, channel_); }
    QString id() const override { return QStringLiteral("SpyStore"); }
    QString uiName() const override { return QStringLiteral("Spy Store"); }
    QImage logo() const override { return QImage(); }
    // The stock capability set. It matters: the dialog only ADDS its Upload tab
    // for a service that advertises Upload, and it selects tab index 2 (sync) in
    // its ctor — a Download-only service would leave index 2 out of range and
    // downloadClicked would drive the UPLOAD path instead. GarminConnect and the
    // ~15 siblings all advertise Upload, so this is the production layout.
    int capabilities() const override { return OAuth | Upload | Download | Query; }
    QString home() override { return QStringLiteral("/"); }

    bool open(QStringList&) override { return true; }
    bool close() override { return true; }

    QList<CloudServiceEntry*> readdir(QString, QStringList&, QDateTime, QDateTime) override
    {
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

    // DEC-garmin-033 (REQ-027 (e)) — NOT a mechanical no-op edit: unlike the ten
    // production siblings this comment used to (wrongly) equate this fixture
    // with, Channel::Failure below is GarminConnect's OWN "return false after
    // arming" shape (queue readFailed, then still return false) — the exact
    // majority shape B-R027-03 is about. Leaving the out-param untouched here
    // would make syncNext's/downloadNext's new branch mistreat this armed
    // refusal as a silent one (labelling the row "Refused" instead of letting
    // failedRead's tail render `reason`), which is precisely the regression an
    // incomplete mechanical edit would have hidden. Caught by
    // readFailedAdvancesTheSyncLoopAndFreesTheCallersBuffer going red before
    // this line was added.
    bool readFile(QByteArray* data, QString remotename, QString remoteid,
                  CloudService::ReadFileArmed* armed = nullptr) override
    {
        requestedIds << remoteid;
        watchSlots << bufferwatch::watch(data);

        if (channel_ == Channel::Failure) {
            const QString why = reason;
            QMetaObject::invokeMethod(
                this, [this, data, remotename, why]() { notifyReadFailed(data, remotename, why); },
                Qt::QueuedConnection);
            if (armed)
                *armed = CloudService::ArmedCompletion;
            return false; // a refusal stays a refusal
        }

        // The sibling shape: readComplete carrying tr("Completed.") — which is
        // what Strava, Dropbox, SportTracks, Xert, Azum, PolarFlow,
        // CyclingAnalytics, SixCycle, Nolio and LocalFileStore all pass, on
        // SUCCESS. The payload stays empty so the dialog takes its own existing
        // ride == NULL branch; readFailed is never emitted.
        QMetaObject::invokeMethod(
            this, [this, data, remotename]() { notifyReadComplete(data, remotename, tr("Completed.")); },
            Qt::QueuedConnection);
        return true;
    }

    QStringList entryNames;
    QString reason = QStringLiteral("Spy Store: this activity could not be downloaded.");
    QStringList requestedIds; // ADVANCEMENT is measured here
    QList<int> watchSlots;    // THE FREE is measured here

  private:
    Channel channel_;
};

// ---------------------------------------------------------------------------

class TestGarminConnectReadFailedConsumer : public QObject
{
    Q_OBJECT

  private:
    QTemporaryDir athleteRoot;
    Context* context = nullptr;
    Athlete* athlete = nullptr;
    RideCache* rideCache = nullptr;

    // Three activities dated today, so they sit inside the dialog's default
    // from/to window (last month .. today) and parse as GC ride filenames.
    static QStringList threeActivities()
    {
        const QString day = QDate::currentDate().toString(QStringLiteral("yyyy_MM_dd"));
        return QStringList() << day + QStringLiteral("_10_00_00.fit") << day + QStringLiteral("_11_00_00.fit")
                             << day + QStringLiteral("_12_00_00.fit");
    }

    // Every string the dialog is actually SHOWING, harvested from its real
    // widget tree (no private member is touched).
    static QStringList visibleCellTexts(QDialog* dialog)
    {
        QStringList out;
        for (QTreeWidget* tree : dialog->findChildren<QTreeWidget*>()) {
            QTreeWidgetItem* root = tree->invisibleRootItem();
            for (int i = 0; i < root->childCount(); ++i)
                for (int c = 0; c < tree->columnCount(); ++c)
                    out << root->child(i)->text(c);
        }
        return out;
    }

    static QStringList visibleLabelTexts(QDialog* dialog)
    {
        QStringList out;
        for (QLabel* label : dialog->findChildren<QLabel*>())
            out << label->text();
        return out;
    }

    // Everything a slot needs to assert on, snapshotted while it is still valid.
    struct Outcome
    {
        QStringList requestedIds; // what the store was asked for, in order
        QList<int> watchSlots;    // bufferwatch slot per preallocated buffer
        QStringList cells;        // every string the dialog is showing
        QStringList labels;
    };

    // Drives ONE full sync of `store` through the dialog's PUBLIC slots only:
    // refresh the remote list, select everything, press Download.
    //
    // The dialog OWNS the store it was handed and closes+destroys it in its own
    // dtor (REQ-017 (e), closeAndDeleteStore). So the store is dead the moment
    // the dialog is - everything the caller wants from it is copied out FIRST,
    // and neither pointer escapes this function.
    Outcome runSyncThrough(SpyStore* store)
    {
        Outcome out;
        CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(context, store);
        // DEC-garmin-026 two-phase init: the constructor now builds only the
        // widget shell; start() runs store->open() and builds the tabs/lists these
        // slots drive. Without it refreshClicked() would dereference null widgets.
        dialog->start();
        dialog->refreshClicked();
        dialog->selectAllSyncChanged(Qt::Checked);
        dialog->downloadClicked();

        // On the FIXED code the run completes inside downloadClicked (each queued
        // notification is picked up by syncNext's own processEvents()); pumping
        // here is what gives the BROKEN code every chance to finish too, so a
        // failure is a genuine stall and not an impatient test.
        for (int i = 0; i < 200; ++i)
            QApplication::processEvents(QEventLoop::AllEvents, 5);

        out.requestedIds = store->requestedIds;
        out.watchSlots = store->watchSlots;
        out.cells = visibleCellTexts(dialog);
        out.labels = visibleLabelTexts(dialog);

        delete dialog; // takes the store with it
        return out;
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
    // TEST-069(a) — THE DEFECT: on readFailed the loop ADVANCES and the
    // caller's buffer is FREED.
    // =====================================================================
    void readFailedAdvancesTheSyncLoopAndFreesTheCallersBuffer()
    {
        bufferwatch::reset();

        SpyStore* store = new SpyStore(context, SpyStore::Channel::Failure);
        const QStringList wanted = threeActivities();
        const QString reason = store->reason;
        store->entryNames = wanted;

        const Outcome out = runSyncThrough(store); // `store` is dead from here on

        // --- ADVANCEMENT ---------------------------------------------------
        // The store was asked for activity 2 and activity 3 AFTER activity 1 was
        // refused. Before the fix this list holds exactly one id forever: the
        // dialog is parked on row 1 waiting for a signal that never comes.
        QCOMPARE(out.requestedIds, wanted);

        // ...and the loop RAN OUT, rather than merely getting further: the
        // dialog printed its terminal tally. 0 of 3 succeeded, which is correct —
        // every one of them was refused.
        QVERIFY2(out.labels.contains(QStringLiteral("Processed 0 of 3 successfully")),
                 qPrintable(QStringLiteral("the sync never reached its terminal state; labels were: ") +
                            out.labels.join(QStringLiteral(" | "))));

        // --- THE FREE ------------------------------------------------------
        // Every buffer the dialog preallocated was released, EXACTLY once. Once
        // says the leak is closed; exactly says a second delivery channel for the
        // same buffer has not turned that leak into a double free.
        QCOMPARE(out.watchSlots.count(), 3);
        for (int i = 0; i < out.watchSlots.count(); ++i)
            QCOMPARE(bufferwatch::frees(out.watchSlots.at(i)), 1);

        // --- THE REASON IS SHOWN -------------------------------------------
        // Not "a signal fired" but "the user can read why", in the row that
        // failed. Without this the row goes blank and the failure is invisible.
        QVERIFY2(out.cells.contains(reason), qPrintable(QStringLiteral("the reason was never rendered; cells were: ") +
                                                        out.cells.join(QStringLiteral(" | "))));
        QCOMPARE(out.cells.count(reason), 3);
    }

    // =====================================================================
    // TEST-069(b) — POSITIVE CONTROL: a service that never emits readFailed
    // behaves EXACTLY as before.
    // =====================================================================
    //
    // This is what protects the ~15 siblings that DEC-023 deliberately does not
    // touch. The same dialog, the same three activities, the same empty payload —
    // the only difference is the channel. It must still advance, still free every
    // buffer exactly once, and still show uncompressRide's OWN complaint rather
    // than anything DEC-023 introduced.
    void aServiceThatNeverEmitsReadFailedIsUnaffected()
    {
        bufferwatch::reset();

        SpyStore* store = new SpyStore(context, SpyStore::Channel::CompletionOnly);
        const QStringList wanted = threeActivities();
        store->entryNames = wanted;

        // Watch the failure channel directly: it must never fire for a sibling.
        int readFailedSeen = 0;
        connect(store, &CloudService::readFailed, this, [&readFailedSeen]() { ++readFailedSeen; });

        // What the PRE-EXISTING path produces for this payload, computed by the
        // production code itself (uncompressRide) rather than restated as a
        // literal here — that is the "exactly as before" yardstick.
        QStringList expectedErrors;
        QByteArray empty;
        RideFile* none = store->uncompressRide(&empty, wanted.first(), expectedErrors);
        QVERIFY2(none == nullptr, "fixture: an empty payload must not yield a ride");
        QVERIFY2(!expectedErrors.isEmpty(), "fixture: uncompressRide must say why it refused");
        const QString legacyText = expectedErrors.join(QStringLiteral(" "));

        const Outcome out = runSyncThrough(store); // `store` is dead from here on

        QCOMPARE(readFailedSeen, 0);
        QCOMPARE(out.requestedIds, wanted);
        QVERIFY2(out.labels.contains(QStringLiteral("Processed 0 of 3 successfully")),
                 qPrintable(QStringLiteral("the sibling's sync did not complete as before; labels were: ") +
                            out.labels.join(QStringLiteral(" | "))));

        QCOMPARE(out.watchSlots.count(), 3);
        for (int i = 0; i < out.watchSlots.count(); ++i)
            QCOMPARE(bufferwatch::frees(out.watchSlots.at(i)), 1);

        // The row still carries uncompressRide's own words, unchanged.
        QCOMPARE(out.cells.count(legacyText), 3);
    }
};

QTEST_MAIN(TestGarminConnectReadFailedConsumer)
#include "testGarminConnectReadFailedConsumer.moc"
