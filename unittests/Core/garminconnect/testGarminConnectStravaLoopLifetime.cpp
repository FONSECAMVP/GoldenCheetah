/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 */

// TEST garmin: T-170/T-171/T-172 — REQ-024 (finding S-R031-01), the store freed
// under its OWN suspended frame, on the axis `blockingCallDepth` cannot see.
//
// Finding text (verbatim):
//   "Strava::readFileCompleted (Strava.cpp:520-531) is a QNetworkReply::finished
//    slot — it retrieves its reply via QObject::sender(), so it runs from network
//    event delivery, outside any BlockingCall (Strava::readFile is async). It
//    calls prepareResponse (:528 → def :869), which at :962 calls addSamples
//    (def :534) for every non-manual activity, and addSamples runs
//    QEventLoop loop; … loop.exec(); at :558-560. So during an ordinary Strava
//    sync a nested loop runs with blockingCallDepth == 0; an athlete-tab close
//    delivered there destroys the dialog, the destructor does NOT take DEC-025's
//    decline branch, and closeAndDeleteStore(store) deletes the store while
//    Strava::addSamples is executing on it."
//
// ACCEPTANCE (verbatim): "A cloud STORE must not be destroyed while one of its
// own frames is suspended in a nested event loop it entered itself — same UAF
// family as REQ-021, on an axis blockingCallDepth is structurally incapable of
// seeing." Proven here by an EXECUTED ASan test through the REAL Strava async
// chain: reply-finished delivery into readFileCompleted via sender(), the real
// prepareResponse -> addSamples chain, the athlete-tab teardown delivered INSIDE
// the nested loop, then resume.
//
// CENSUS THAT SHAPED THESE SLOTS (re-derived 2026-09-07; the finding's 2026-08-11
// line numbers have shifted — REQ-023's uncompress guard moved CloudService.cpp
// ~+4 and DEC-040 moved the loop itself). The current chain is:
//
//   Strava::readFile (Strava.cpp:281, ASYNC: arms a reply, returns true)
//     -> [network delivery] QNetworkReply::finished
//     -> Strava::readFileCompleted (Strava.cpp:551, slot via QObject::sender())
//     -> Strava::prepareResponse (Strava.cpp:938; non-manual activity at :1050)
//     -> Strava::addSamples (Strava.cpp:593)
//     -> CloudService::blockingRequest (CloudService.cpp:217)
//     -> QEventLoop::exec() (CloudService.cpp:332)   <-- the nested loop
//
// The loop no longer lives in addSamples: DEC-040 Stage 1 moved it into
// CloudService::blockingRequest, shared by every migrated provider. The store's
// own suspending FRAME, however, still begins and ends in Strava.cpp — it is the
// whole readFileCompleted slot invocation — and that frame boundary is where the
// busy-depth marker goes (its span strictly contains the nested loop, and it is
// the only unwind point at which a declined store could ever be reaped safely:
// reaping at the inner blockingRequest unwind would free the store under
// addSamples/prepareResponse/readFileCompleted, which are all still on the stack).
//
// The dialog's decline branch (CloudService.cpp:1695, ~CloudServiceSyncDialog)
// consults `blockingCallDepth > 0` — DIALOG frames only. During this chain that
// depth is 0: the BlockingCall that wrapped store->readFile (CloudService.cpp:2806)
// unwound when the ASYNC readFile returned, long before the completion was
// delivered. That is the structural gap REQ-024 names.
//
// WHAT IS REAL HERE: the REAL src/Cloud/Strava.cpp and REAL src/Cloud/CloudService.cpp
// are compiled in (same seam family as testGarminConnectUncompressLifetime —
// stubs/ImportSeamStubs.cpp + stubs/SyncDialogSeamStubs.cpp), so the chain under
// test is the shipping one, and the dialog is the real CloudServiceSyncDialog.
// The fake is only the NETWORK (a QNetworkAccessManager injected through the
// constructor seam DEC-040 S-1 provides, the same seam testCloudProviderWatchdog
// drives); its streams reply runs the athlete-tab teardown from inside its own
// deliver(), which the nested loop dispatches — reproducing "an athlete-tab
// close delivered there" through event delivery, not a synchronous call.
//
// The one piece of fake on the path is JsonFileReader::toByteArray (the staged
// bytes), stubbed below exactly as the watchdog target stubs it: inert about
// `context`, but honest about the point count, so "samples were parsed" stays
// observable without linking the generated JSON writer.
//
// RED expectations, stated before the fix:
//   T-170 — pre-fix, the dialog dtor takes closeAndDeleteStore inside the nested
//   loop and frees the Strava store under blockingRequest/addSamples/
//   prepareResponse/readFileCompleted. The first post-resume store-member read
//   (replyName, Strava.cpp:589's argument evaluation, then the emit itself) is a
//   heap-use-after-free; this target halts on the first ASan report, so the
//   abrupt end of the binary IS the red verdict — the same shape
//   testGarminConnectUncompressLifetime documented for T-167.
//   T-171/T-172 — green from the start: they pin the SELECTIVITY (idle and
//   post-completion teardowns still delete synchronously) and the positive
//   control, so the fix cannot pass T-170 merely by never deleting anything.

#include "Athlete.h"
#include "CloudService.h"
#include "Context.h"
#include "JsonRideFile.h"
#include "Settings.h"
#include "Strava.h"

#include <QApplication>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QList>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QString>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>
#include <QtTest/QtTest>

#include <functional>

#if !defined(__SANITIZE_ADDRESS__) && !defined(GC_TEST_WITH_ASAN)
#    error \
        "T-170/T-171/T-172 are lifetime tests and are only trustworthy under AddressSanitizer; build this target with -fsanitize=address."
#endif

// ---------------------------------------------------------------------------
// The fake network. A cut-down mimic of testCloudProviderWatchdog's FakeReply/
// FakeNam (that harness's seam is explicitly fine to mimic): a QNetworkReply
// that delivers its body from a zero-timer, so the delivery is EVENT-DRIVEN by
// whatever loop is running when it comes due — the outer loop for the activity
// reply, blockingRequest's own nested loop for the streams reply. abort()
// synthesises a finished() exactly as a real reply does (row 6 of DEC-040's
// transition table depends on that).
//
// The one addition over the watchdog shape: a beforeDeliver hook, so the STREAMS
// reply can run the athlete-tab teardown from inside the nested loop that is
// waiting on it — the finding's exact delivery geometry.
// ---------------------------------------------------------------------------
namespace fakenet {

class Reply : public QNetworkReply
{
    Q_OBJECT

  public:
    Reply(const QNetworkRequest& request, const QByteArray& body, std::function<void()> beforeDeliver)
        : body_(body), beforeDeliver_(std::move(beforeDeliver))
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(QNetworkAccessManager::GetOperation);
        setOpenMode(QIODevice::ReadOnly);
        QTimer::singleShot(0, this, &Reply::deliver);
    }

    ~Reply() override {}

    void abort() override
    {
        if (aborted_ || finished_)
            return;
        aborted_ = true;
        setError(QNetworkReply::OperationCanceledError, QStringLiteral("aborted"));
        finish();
    }

    qint64 bytesAvailable() const override { return buffer_.bytesAvailable() + QNetworkReply::bytesAvailable(); }
    bool isSequential() const override { return true; }

  public slots:
    void deliver()
    {
        if (aborted_ || finished_)
            return;
        // The teardown hook runs BEFORE the finish: the dialog destructor fires
        // while blockingRequest's nested loop is still running, which is the
        // whole point of this fixture.
        if (beforeDeliver_)
            beforeDeliver_();
        buffer_.setData(body_);
        buffer_.open(QIODevice::ReadOnly);
        finish();
    }

  protected:
    qint64 readData(char* data, qint64 maxlen) override
    {
        const qint64 n = buffer_.read(data, maxlen);
        return n > 0 ? n : -1;
    }

  private:
    void finish()
    {
        finished_ = true;
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, 200);
        // readyRead BEFORE finished: Strava::readyRead accumulates the body into
        // the caller's buffer, and a fake that only emitted finished() would
        // hand readFileCompleted an empty buffer and measure the fake.
        if (!body_.isEmpty())
            emit readyRead();
        setFinished(true);
        emit finished();
    }

    QByteArray body_;
    QBuffer buffer_;
    std::function<void()> beforeDeliver_;
    bool aborted_ = false;
    bool finished_ = false;
};

class Nam : public QNetworkAccessManager
{
    Q_OBJECT

  public:
    QByteArray activityBody;                    // the GET /activities/{id} reply
    QByteArray streamsBody;                     // the GET /activities/{id}/streams/... reply
    std::function<void()> beforeStreamsDeliver; // runs inside the nested loop

    QList<QPointer<Reply>> issued; // every reply, for end-of-slot cleanup

  protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData) override
    {
        Q_UNUSED(op);
        Q_UNUSED(outgoingData);
        const bool streams = request.url().toString().contains(QStringLiteral("/streams/"));
        Reply* reply = new Reply(request, streams ? streamsBody : activityBody,
                                 streams ? beforeStreamsDeliver : std::function<void()>{});
        issued << QPointer<Reply>(reply);
        return reply;
    }
};

} // namespace fakenet

// The staged bytes. The real writer needs the generated JsonRideFile
// lexer/parser; the bytes it produces are not what this slice is about. Inert
// about `context` (the real one dereferences nothing on this path either —
// JsonRideFile.y:465 takes the parameter unnamed), but honest about the point
// count, so "the samples were parsed and staged" stays observable. Same stub
// shape as testCloudProviderWatchdog's ProviderSeamStubs.cpp, so the two
// targets' staged-bytes vocabulary matches.
QByteArray JsonFileReader::toByteArray(Context* context, const RideFile* ride, bool withAlt, bool withWatts,
                                       bool withHr, bool withCad) const
{
    Q_UNUSED(context);
    Q_UNUSED(withAlt);
    Q_UNUSED(withWatts);
    Q_UNUSED(withHr);
    Q_UNUSED(withCad);
    if (ride == nullptr)
        return QByteArray();
    return QStringLiteral("STAGED-RIDE points=%1").arg(ride->dataPoints().count()).toUtf8();
}

// The finding's bodies, verbatim from testCloudProviderWatchdog's fixtures: a
// non-manual activity (so prepareResponse calls addSamples) and a streams set
// whose time+watts arrays carry three samples.
static const char kActivityJson[] = "{\"id\":7,\"type\":\"Ride\",\"name\":\"n\",\"manual\":false,"
                                    "\"start_date_local\":\"2020-01-01T10:00:00\"}";
static const char kStreamsJson[] = "[{\"type\":\"time\",\"data\":[0,1,2]},{\"type\":\"watts\",\"data\":[100,110,120]}]";

// ---------------------------------------------------------------------------
// The athlete-tab fixture — the established sibling shape
// (testGarminConnectSyncDialogClose.cpp's FakeAthleteWindow, reduced to what
// this slice needs). The order in teardownTabFirst() is MainWindow::
// removeAthleteTab's order and the SYNCHRONY is the point: `delete tabWidget`
// runs the sync dialog's destructor from Qt child destruction, strictly before
// the Athlete and Context are freed.
// ---------------------------------------------------------------------------
struct TabFixture
{
    QTemporaryDir athleteRoot;
    QWidget* tabWidget = nullptr;
    Context* context = nullptr;
    Athlete* athlete = nullptr;

    void build()
    {
        tabWidget = new QWidget;
        context = new Context(nullptr);
        // AthleteTab::AthleteTab assigns this (AthleteTab.cpp:35); the sync
        // dialog parents itself to it (cloudDialogParent, CloudService.cpp:447),
        // so a fixture that left it unset would be testing nothing.
        context->tab = reinterpret_cast<AthleteTab*>(tabWidget);
        athlete = new Athlete(context, QDir(athleteRoot.path()));
        context->athlete = athlete;
    }

    void teardownTabFirst()
    {
        delete tabWidget; // the dialog's destructor runs here, as a Qt child
        delete athlete;
        delete context;
        tabWidget = nullptr;
        athlete = nullptr;
        context = nullptr;
    }
};

// What the completion channel reported — the same observations the watchdog's
// StravaCompletionWatcher records, so cross-target verdicts stay comparable.
class CompletionWatcher : public QObject
{
    Q_OBJECT

  public:
    explicit CompletionWatcher(CloudService* service)
    {
        connect(service, &CloudService::readComplete, this, [this](QByteArray* data, QString, QString) {
            completes++;
            lastPointer = data;
            staged = data ? *data : QByteArray();
        });
        connect(service, &CloudService::readFailed, this, [this](QByteArray* data, QString, QString reason) {
            failures++;
            lastPointer = data;
            staged = data ? *data : QByteArray();
            lastReason = reason;
        });
    }

    int completes = 0;
    int failures = 0;
    QByteArray* lastPointer = nullptr;
    QByteArray staged;
    QString lastReason;
};

class TestGarminConnectStravaLoopLifetime : public QObject
{
    Q_OBJECT

  private slots:

    // -----------------------------------------------------------------------
    // T-170 — REQ-024 core. The REAL async chain runs: readFile arms the
    // activity reply; the OUTER loop's timer delivers it; readFileCompleted
    // (via sender()) -> prepareResponse -> addSamples -> blockingRequest enters
    // the nested loop; the NESTED loop's timer delivers the streams reply,
    // whose beforeDeliver hook performs the athlete-tab teardown; the dialog
    // destructor therefore runs with blockingCallDepth == 0 while the store's
    // own frame is suspended; the streams delivery then finishes the wait and
    // the frame resumes.
    //
    // GREEN asserts, in order:
    //   1. the store was NOT freed by the dialog destructor (the decline fired
    //      for the store's own suspended frame) — captured inside the hook,
    //      i.e. AFTER the destructor has run;
    //   2. the resumed frame ran to completion on the SURVIVING store: exactly
    //      one readComplete, with the samples parsed and staged, on the same
    //      buffer pointer the caller handed readFile;
    //   3. the declined store reaped itself once its last frame unwound — it
    //      must not become a per-close accumulating leak (the DEC-031 lesson),
    //      and the reap must be DEFERRED, not synchronous: at the marker's
    //      unwind the reply's finished() emission is still on the stack.
    // -----------------------------------------------------------------------
    void teardownInsideAddSamplesLoopMustNotFreeTheStore()
    {
        TabFixture fixture;
        fixture.build();

        fakenet::Nam* nam = new fakenet::Nam();
        nam->activityBody = QByteArray(kActivityJson);
        nam->streamsBody = QByteArray(kStreamsJson);

        Strava* store = new Strava(fixture.context, nam);
        store->setSetting(GC_STRAVA_TOKEN, "TOKEN");
        // Bounds a wedged wait from below; the happy path finishes in
        // milliseconds, far inside it.
        store->setRequestTimeoutOverrideMs(3000);
        QPointer<Strava> storeGuard(store);

        // Heap, NOT stack: the dialog parents itself to context->tab, so the
        // tab's deletion must be the thing that destroys it. start() is never
        // called — the constructor builds the shell only (DEC-garmin-026) and
        // the decline branch under test lives in the destructor, not in start().
        CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(fixture.context, store);
        Q_UNUSED(dialog);

        CompletionWatcher watcher(store);

        bool storeAliveAfterDialogDtor = false;
        nam->beforeStreamsDeliver = [&fixture, storeGuard, &storeAliveAfterDialogDtor]() {
            // MainWindow::removeAthleteTab's order: the tab (and its child
            // dialog) first, then the athlete, then the context — all
            // synchronously, inside the nested loop's event delivery.
            delete fixture.tabWidget;
            storeAliveAfterDialogDtor = !storeGuard.isNull();
            delete fixture.athlete;
            delete fixture.context;
            fixture.tabWidget = nullptr;
            fixture.athlete = nullptr;
            fixture.context = nullptr;
        };

        // The caller preallocates the buffer and is its sole owner and sole
        // deleter (DEC-garmin-036's buffer-identity contract).
        QByteArray buffer;
        QVERIFY(store->readFile(&buffer, QStringLiteral("2020_01_01_10_00_00.json"), QStringLiteral("7")));

        QEventLoop outer;
        QTimer::singleShot(5000, &outer, &QEventLoop::quit); // anti-wedge net
        QObject::connect(store, &CloudService::readComplete, &outer, &QEventLoop::quit);
        QObject::connect(store, &CloudService::readFailed, &outer, &QEventLoop::quit);
        outer.exec();

        QVERIFY2(storeAliveAfterDialogDtor,
                 "the dialog destructor must DECLINE to delete the store while the store's own frame is "
                 "suspended in addSamples' nested loop (REQ-024: blockingCallDepth cannot see that frame; "
                 "the store's own busy-depth must)");
        QCOMPARE(watcher.completes, 1);
        QCOMPARE(watcher.failures, 0);
        QVERIFY2(watcher.staged.startsWith(QByteArray("STAGED-RIDE points=3")),
                 qPrintable(QStringLiteral("the resumed frame must still parse the samples, staged: %1")
                                .arg(QString::fromUtf8(watcher.staged))));
        QCOMPARE(watcher.lastPointer, &buffer);

        // Flush the deferred deletes (the store's self-reap among them) the
        // documented way, so the assert below does not depend on loop
        // implementation details. The reply list is snapshotted first: the
        // manager is a CHILD of the store, so after the reap it is freed too.
        const QList<QPointer<fakenet::Reply>> issued = nam->issued;
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(storeGuard.isNull(),
                 "the declined store must reap itself once its last frame has unwound - a per-close leak is "
                 "the DEC-031 defect reborn at the store layer");

        for (const QPointer<fakenet::Reply>& reply : issued)
            if (reply)
                delete reply;
    }

    // -----------------------------------------------------------------------
    // T-171 — selectivity, both directions the decline could corrupt.
    //   (a) IDLE: a teardown with no store frame anywhere must take the
    //       ordinary synchronous close+delete — the decline must not turn every
    //       teardown into a deferred one, or an idle dialog close would leak
    //       its store until some later event delivery.
    //   (b) AFTER THE CHAIN: run the full async chain to completion (so the
    //       marker has come and gone), then tear down — again the ordinary
    //       synchronous delete. This pins that the busy-depth is a property of
    //       LIVE frames, not a latch the chain leaves behind.
    // -----------------------------------------------------------------------
    void idleAndCompletedTeardownStillDeleteTheStore()
    {
        // (a) idle teardown
        {
            TabFixture fixture;
            fixture.build();

            fakenet::Nam* nam = new fakenet::Nam();
            nam->activityBody = QByteArray(kActivityJson);
            nam->streamsBody = QByteArray(kStreamsJson);

            Strava* store = new Strava(fixture.context, nam);
            store->setSetting(GC_STRAVA_TOKEN, "TOKEN");
            QPointer<Strava> storeGuard(store);
            CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(fixture.context, store);
            Q_UNUSED(dialog);

            delete fixture.tabWidget;
            QVERIFY2(storeGuard.isNull(),
                     "an idle teardown must still close+delete the store synchronously - the decline is for "
                     "suspended frames only");
            delete fixture.athlete;
            delete fixture.context;
        }

        // (b) teardown after every store frame has unwound
        {
            TabFixture fixture;
            fixture.build();

            fakenet::Nam* nam = new fakenet::Nam();
            nam->activityBody = QByteArray(kActivityJson);
            nam->streamsBody = QByteArray(kStreamsJson);

            Strava* store = new Strava(fixture.context, nam);
            store->setSetting(GC_STRAVA_TOKEN, "TOKEN");
            store->setRequestTimeoutOverrideMs(3000);
            QPointer<Strava> storeGuard(store);
            CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(fixture.context, store);
            Q_UNUSED(dialog);

            CompletionWatcher watcher(store);
            QVERIFY(!nam->beforeStreamsDeliver); // no teardown mid-chain here

            QByteArray buffer;
            QVERIFY(store->readFile(&buffer, QStringLiteral("2020_01_01_10_00_00.json"), QStringLiteral("7")));

            QEventLoop outer;
            QTimer::singleShot(5000, &outer, &QEventLoop::quit);
            QObject::connect(store, &CloudService::readComplete, &outer, &QEventLoop::quit);
            QObject::connect(store, &CloudService::readFailed, &outer, &QEventLoop::quit);
            outer.exec();

            QCOMPARE(watcher.completes, 1);
            QVERIFY2(!storeGuard.isNull(), "after an ordinary completed chain the dialog still owns the store");

            // Snapshot before the teardown: the manager dies with the store.
            const QList<QPointer<fakenet::Reply>> issued = nam->issued;
            delete fixture.tabWidget;
            QVERIFY2(storeGuard.isNull(),
                     "a teardown after every store frame unwound must take the ordinary synchronous delete, "
                     "not the decline - the busy-depth must not be a latch the chain leaves behind");
            delete fixture.athlete;
            delete fixture.context;

            for (const QPointer<fakenet::Reply>& reply : issued)
                if (reply)
                    delete reply;
        }
    }

    // -----------------------------------------------------------------------
    // T-172 — positive control. The real chain, no teardown: one readComplete,
    // the samples parsed and staged (time+watts arrays -> three points), the
    // caller's buffer pointer back unchanged, and the ordinary teardown at the
    // end still deletes the store. Guards against a "fix" that passes T-170 by
    // suppressing the completion channel itself.
    // -----------------------------------------------------------------------
    void ordinaryChainStillStagesSamples()
    {
        TabFixture fixture;
        fixture.build();

        fakenet::Nam* nam = new fakenet::Nam();
        nam->activityBody = QByteArray(kActivityJson);
        nam->streamsBody = QByteArray(kStreamsJson);

        Strava* store = new Strava(fixture.context, nam);
        store->setSetting(GC_STRAVA_TOKEN, "TOKEN");
        store->setRequestTimeoutOverrideMs(3000);
        QPointer<Strava> storeGuard(store);
        CloudServiceSyncDialog* dialog = new CloudServiceSyncDialog(fixture.context, store);
        Q_UNUSED(dialog);

        CompletionWatcher watcher(store);

        QByteArray buffer;
        QVERIFY(store->readFile(&buffer, QStringLiteral("2020_01_01_10_00_00.json"), QStringLiteral("7")));

        QEventLoop outer;
        QTimer::singleShot(5000, &outer, &QEventLoop::quit);
        QObject::connect(store, &CloudService::readComplete, &outer, &QEventLoop::quit);
        QObject::connect(store, &CloudService::readFailed, &outer, &QEventLoop::quit);
        outer.exec();

        QCOMPARE(watcher.completes, 1);
        QCOMPARE(watcher.failures, 0);
        QVERIFY2(watcher.staged.startsWith(QByteArray("STAGED-RIDE points=3")),
                 qPrintable(QStringLiteral("the ordinary chain must still parse and stage the samples, staged: %1")
                                .arg(QString::fromUtf8(watcher.staged))));
        QCOMPARE(watcher.lastPointer, &buffer);

        // Snapshot before the teardown: the manager dies with the store.
        const QList<QPointer<fakenet::Reply>> issued = nam->issued;
        delete fixture.tabWidget;
        QVERIFY2(storeGuard.isNull(), "the ordinary teardown still deletes the store exactly once");
        delete fixture.athlete;
        delete fixture.context;

        for (const QPointer<fakenet::Reply>& reply : issued)
            if (reply)
                delete reply;
    }
};

QTEST_MAIN(TestGarminConnectStravaLoopLifetime)
#include "testGarminConnectStravaLoopLifetime.moc"
