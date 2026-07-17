/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-006 — VAL-007 second half: GarminAuthChain, the RAII assembly
// that AddCloudWizard uses to stand up the production auth stack
// (QThread + GarminWorker + WorkerAuthClient) around an injected
// IGarminPyAdapter*.
//
// Acceptance encoded here (chain half of the AddCloudWizard tile-routing
// slice):
//   a. construction exposes a non-null client and a running thread.
//   b. authenticate round-trip through the chain with FakePyAdapter: success
//      outcome delivered via the IGarminAuthClient signals on the caller's
//      thread.
//   c. the FakePyAdapter call executes OFF the caller's thread (recorded via
//      QThread::currentThread in the fake).
//   d. failure outcome (AuthFailed) propagates with its failure kind intact.
//   e. clean teardown while idle — destructor returns promptly (bounded, no
//      infinite hang).
//   f. teardown after a completed request — no crash, no dangling delivery.
//
// Cites:
//   DES-001   — worker thread model; invariant 3: destructor quit()+wait()
//               from the caller's thread, bounded — never hang
//   DES-001a  — lifecycle: wizard outlives worker outlives adapter; the
//               chain owns thread+worker+client, NOT the adapter
//   DES-003a  — IGarminAuthClient contract exposed by client()
//   DES-013   — production adapter arrives as IGarminPyAdapter* (this test
//               substitutes FakePyAdapter; stays Python-free / garmin-fast)
//   DEC-002   — Python integration mechanism (worker thread)
//   DEC-008   — QTest+CTest under the `garmin-fast` label
//   DEC-012 / DEC-013 — interface-injection seams reused unchanged
//
// RED expectation:
//   src/Cloud/GarminAuthChain.{h,cpp} do not exist yet. The build fails at
//   the #include "GarminAuthChain.h" line below — right-reason RED (missing
//   unit under test, not a wiring artifact). GREEN adds those files and
//   extends the CMake target; every assertion here must then pass without
//   altering the locked-in public surface.

#include "GarminAuthChain.h" // <-- intentionally missing in RED
#include "IGarminAuthClient.h"
#include "IGarminPyAdapter.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointer>
#include <QSignalSpy>
#include <QString>
#include <QThread>
#include <QUuid>
#include <QtTest/QtTest>

#include <atomic>

// ---------------------------------------------------------------------------
// FakePyAdapter — same shape as TEST-004's fake: records credentials and the
// QThread it was invoked on, returns a scripted PyAuthOutcome. threadSeen is
// the keystone of slot (c): if the chain accidentally runs the adapter on the
// caller's thread the assertion fails.
// ---------------------------------------------------------------------------
class FakePyAdapter : public IGarminPyAdapter
{
  public:
    PyAuthOutcome scriptedOutcome;
    QString lastEmail;
    QString lastPassword;
    std::atomic<int> callCount{0};
    QThread* threadSeen = nullptr;

    PyAuthOutcome authenticate(const QString& email, const QString& password) override
    {
        lastEmail = email;
        lastPassword = password;
        threadSeen = QThread::currentThread();
        callCount.fetch_add(1);
        return scriptedOutcome;
    }

    // REQ-007 seam extension (DEC-013 compile-enforced) — chain teardown tests
    // never download; a default outcome satisfies the interface.
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }

    // REQ-007 closure (Slice 1) seam extension (DEC-013 compile-enforced) — never
    // restores a session here; a default outcome satisfies the interface.
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }
};

// ---------------------------------------------------------------------------
// REQ-002 / TEST-006 / A3-R002-TR-04 — BusyPyAdapter: a deterministic,
// Python-free (no CPython, no GIL) adapter whose authenticate() wedges the
// worker thread in a tight busy-loop. This never returns and never yields to
// the worker's event loop, so the chain's graceful ~GarminAuthChain path
// (quit() + wait(kQuitWaitMs)) is GUARANTEED to time out and fall through to
// the terminate() last-resort branch — giving that branch intentional
// coverage. `entered` lets the test observe that the worker is actually inside
// the busy-loop before it tears the chain down.
// ---------------------------------------------------------------------------
class BusyPyAdapter : public IGarminPyAdapter
{
  public:
    std::atomic<bool> entered{false};
    std::atomic<bool> stop{false}; // never set true by the test — terminate() is the only exit

    PyAuthOutcome authenticate(const QString&, const QString&) override
    {
        entered.store(true);
        // Wedge the worker: this slot NEVER returns to the thread's exec()
        // event loop, so m_thread.quit() (which only takes effect once control
        // is back in exec()) is ignored and wait(kQuitWaitMs) is forced to time
        // out — driving ~GarminAuthChain into its terminate() last-resort
        // branch (the whole point of TR-04). No Python, no GIL: fully
        // deterministic.
        //
        // The 1ms sleep is deliberate and load-bearing: Qt6 (qthread_unix)
        // starts worker threads with the DEFAULT deferred pthread cancellation
        // type, so terminate() (pthread_cancel) can only take effect at a POSIX
        // cancellation point. nanosleep() is such a point; a pure atomic-read
        // spin-loop is NOT, and under this Qt/glibc terminate() then fails to
        // cancel it within kTerminateWaitMs, leaving ~QThread to abort with
        // "QThread: Destroyed while thread is still running". See the TR-04
        // finding in the build report. This sleep models the realistic wedge
        // (a blocking call with a cancellation point) so terminate() genuinely
        // unwinds the worker and the thread finishes.
        while (!stop.load(std::memory_order_relaxed)) {
            QThread::msleep(1);
        }
        return PyAuthOutcome{};
    }

    // REQ-007 seam extension (DEC-013 compile-enforced) — never invoked; the
    // wedge is in authenticate().
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }

    // REQ-007 closure (Slice 1) seam extension (DEC-013 compile-enforced) — never
    // invoked; the wedge is in authenticate().
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }
};

// ---------------------------------------------------------------------------
// Helper — drains the caller thread's event queue until a predicate is true
// or a timeout elapses (mirrors TEST-004's waitFor).
// ---------------------------------------------------------------------------
namespace {
template <typename Pred>
bool waitFor(Pred p, int timeoutMs = 2000)
{
    QElapsedTimer t;
    t.start();
    while (!p()) {
        if (t.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(5);
    }
    return true;
}

// REQ-002 / TEST-006 / A3-R002-TR-02 (LSN-009): graceful-teardown watchdog,
// deliberately WELL UNDER the chain's kQuitWaitMs (=2000ms). See the (e) slot
// comment for the full mutation-survivor rationale.
constexpr qint64 kGracefulTeardownBoundMs = 200;
} // namespace

class TestGarminConnectAuthChain : public QObject
{
    Q_OBJECT

  private slots:

    // (a) Construction exposes a non-null client and a running thread.
    void constructionExposesNonNullClientAndRunningThread()
    {
        FakePyAdapter fake;
        GarminAuthChain chain(&fake);

        QVERIFY2(chain.client() != nullptr, "GarminAuthChain::client() must expose the IGarminAuthClient seam");
        QVERIFY2(chain.workerThread() != nullptr, "the chain must own a dedicated worker QThread");
        QVERIFY2(chain.workerThread()->isRunning(),
                 "the worker thread must be started by the constructor (running immediately "
                 "after construction)");
        QVERIFY2(chain.workerThread() != QThread::currentThread(),
                 "the worker thread must be a dedicated thread, not the caller's");
    }

    // (b) Success round-trip: outcome delivered via IGarminAuthClient signals
    //     ON the caller's thread, with requestId + payload intact.
    void authenticateSuccessDeliveredOnCallerThread()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-42");
        fake.scriptedOutcome.display_name = QStringLiteral("Chain Rider");

        GarminAuthChain chain(&fake);

        QThread* deliveryThread = nullptr;
        int finishedCount = 0;
        QUuid deliveredId;
        GarminAuthSuccess delivered;
        connect(chain.client(), &IGarminAuthClient::finished, this, [&](QUuid id, GarminAuthSuccess result) {
            deliveryThread = QThread::currentThread();
            deliveredId = id;
            delivered = result;
            ++finishedCount;
        });
        QSignalSpy failedSpy(chain.client(), &IGarminAuthClient::failed);

        const QUuid id = QUuid::createUuid();
        chain.client()->authenticate(QStringLiteral("rider@example.com"), QStringLiteral("hunter2"), id);

        QVERIFY2(waitFor([&] { return finishedCount >= 1; }),
                 "success outcome must be delivered via IGarminAuthClient::finished within timeout");
        QCOMPARE(finishedCount, 1);
        QCOMPARE(failedSpy.size(), 0);
        QCOMPARE(deliveredId, id);
        QCOMPARE(delivered.garmin_user_id, QStringLiteral("uid-42"));
        QCOMPARE(delivered.display_name, QStringLiteral("Chain Rider"));
        QVERIFY2(deliveryThread == QThread::currentThread(),
                 "the finished signal must be delivered on the caller's (GUI/test) thread");
        QCOMPARE(fake.lastEmail, QStringLiteral("rider@example.com"));
        QCOMPARE(fake.lastPassword, QStringLiteral("hunter2"));
    }

    // (c) The adapter call executes OFF the caller's thread — on the chain's
    //     dedicated worker thread (REQ-NF-Threads-001 through the assembly).
    void adapterCallExecutesOffCallerThread()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;

        GarminAuthChain chain(&fake);
        QSignalSpy finishedSpy(chain.client(), &IGarminAuthClient::finished);

        chain.client()->authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());

        QVERIFY(waitFor([&] { return finishedSpy.size() >= 1; }));
        QCOMPARE(int(fake.callCount.load()), 1);
        QVERIFY2(fake.threadSeen != nullptr && fake.threadSeen != QThread::currentThread(),
                 "FakePyAdapter.authenticate must run OFF the caller's thread");
        QVERIFY2(fake.threadSeen == chain.workerThread(),
                 "FakePyAdapter.authenticate must run on the chain's own worker thread");
    }

    // (d) Failure outcome (AuthFailed) propagates with its failure kind
    //     intact — Auth, not Unknown/Network (mutation catch on the mapping
    //     surviving the extra assembly layer).
    void authFailedPropagatesWithKindIntact()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::AuthFailed;
        fake.scriptedOutcome.rawMessage = QStringLiteral("bad credentials");

        GarminAuthChain chain(&fake);
        QSignalSpy failedSpy(chain.client(), &IGarminAuthClient::failed);
        QSignalSpy finishedSpy(chain.client(), &IGarminAuthClient::finished);

        const QUuid id = QUuid::createUuid();
        chain.client()->authenticate(QStringLiteral("e@x"), QStringLiteral("wrong"), id);

        QVERIFY(waitFor([&] { return failedSpy.size() >= 1; }));
        QCOMPARE(failedSpy.size(), 1);
        QCOMPARE(finishedSpy.size(), 0);
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Auth);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("bad credentials"));
    }

    // (e) Clean teardown while idle — the destructor must return promptly
    //     (DES-001 invariant 3: quit()+wait() from the caller's thread,
    //     bounded — never hang). Watchdog: elapsed-time bound well below the
    //     chain's internal wait ceiling; a hang would also trip the QTest
    //     watchdog / ctest timeout rather than wedge CI forever.
    //
    // REQ-002 / TEST-006 / A3-R002-TR-02 (LSN-009): the graceful-teardown bound
    // is deliberately WELL UNDER the chain's kQuitWaitMs (=2000ms). An idle
    // worker's event loop returns from exec() the instant m_thread.quit() posts
    // its quit event, so wait() unblocks in single-digit milliseconds. The old
    // 3000ms bound was the SUM of both fallback ceilings and therefore survived
    // a mutant that deletes m_thread.quit(): without quit() the idle event loop
    // never returns, wait(kQuitWaitMs) times out and terminate()+wait fires,
    // yet elapsed (~2.5s) still cleared 3000ms. A bound of 200ms (< kQuitWaitMs/10)
    // forces that mutant to FAIL: skipping quit() pushes elapsed to ~2000ms+.
    void teardownWhileIdleReturnsPromptly()
    {
        FakePyAdapter fake;
        GarminAuthChain* chain = new GarminAuthChain(&fake);
        QVERIFY(chain->workerThread()->isRunning());
        QPointer<QThread> threadGuard(chain->workerThread());

        QElapsedTimer t;
        t.start();
        delete chain; // must quit()+wait() the thread, bounded
        const qint64 elapsed = t.elapsed();

        QVERIFY2(elapsed < kGracefulTeardownBoundMs,
                 qPrintable(QStringLiteral("destructor took %1 ms — graceful teardown must complete well under "
                                           "kQuitWaitMs (bounded quit()+wait()); a mutant that drops quit() would "
                                           "time out here")
                                .arg(elapsed)));
        QVERIFY2(threadGuard.isNull(), "the chain must destroy its owned QThread");
        QCOMPARE(int(fake.callCount.load()), 0); // idle — adapter never touched
    }

    // (f) Teardown after a completed request — no crash, and no dangling
    //     delivery: once the chain is destroyed, no further signal delivery
    //     may occur even if the event loop keeps spinning.
    void teardownAfterCompletedRequestNoDanglingDelivery()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-7");

        GarminAuthChain* chain = new GarminAuthChain(&fake);

        std::atomic<int> deliveries{0};
        connect(chain->client(), &IGarminAuthClient::finished, this,
                [&](QUuid, GarminAuthSuccess) { deliveries.fetch_add(1); });

        chain->client()->authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());
        QVERIFY(waitFor([&] { return deliveries.load() >= 1; }));
        QCOMPARE(deliveries.load(), 1);

        QElapsedTimer t;
        t.start();
        delete chain; // after a completed round-trip
        // REQ-002 / TEST-006 / A3-R002-TR-02: same tight graceful bound as (e).
        // After the round-trip the worker is back idle in exec(); quit() must
        // still unblock wait() in milliseconds. A dropped-quit() mutant times
        // out here too (~2s) and trips this bound.
        const qint64 postElapsed = t.elapsed();
        QVERIFY2(postElapsed < kGracefulTeardownBoundMs,
                 qPrintable(QStringLiteral("post-request teardown took %1 ms — must also be bounded well under "
                                           "kQuitWaitMs")
                                .arg(postElapsed)));

        // Spin the caller's event loop: nothing queued may still deliver.
        QElapsedTimer drain;
        drain.start();
        while (drain.elapsed() < 250) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(5);
        }
        QCOMPARE(deliveries.load(), 1); // no dangling delivery after teardown
        QCOMPARE(int(fake.callCount.load()), 1);
    }

    // (g) REQ-002 / TEST-006 / A3-R002-TR-04 — the terminate() last-resort
    //     teardown path (DES-001 invariant 3, second clause). With a worker
    //     wedged in BusyPyAdapter's busy-loop, quit()+wait(kQuitWaitMs) MUST
    //     time out and terminate()+wait(kTerminateWaitMs) MUST fire. The chain
    //     must STILL tear down within its bounded ceiling and must destroy its
    //     owned QThread cleanly — a QThread destroyed while still running would
    //     qFatal ("QThread: Destroyed while thread is still running"), so a
    //     normal return here is itself proof the thread had finished.
    void teardownWhileWorkerWedgedFallsBackToTerminate()
    {
        BusyPyAdapter fake;
        GarminAuthChain* chain = new GarminAuthChain(&fake);
        QVERIFY(chain->workerThread()->isRunning());
        QPointer<QThread> threadGuard(chain->workerThread());

        // Kick off an authenticate that will wedge the worker thread.
        chain->client()->authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());
        QVERIFY2(waitFor([&] { return fake.entered.load(); }),
                 "worker must enter the busy-loop before teardown so quit()+wait() is forced to time out");

        QElapsedTimer t;
        t.start();
        delete chain; // graceful wait times out -> terminate() fires
        const qint64 elapsed = t.elapsed();

        // Lower bound proves the graceful fast-path did NOT return quickly — the
        // worker was genuinely stuck and the terminate() branch was reached
        // (kQuitWaitMs=2000; an idle teardown returns in single-digit ms).
        QVERIFY2(elapsed >= 1900,
                 qPrintable(QStringLiteral("teardown returned in %1 ms — expected the graceful wait to time out "
                                           "(~kQuitWaitMs=2000ms) before terminate() fires; the wedged path was "
                                           "not exercised")
                                .arg(elapsed)));
        // Upper bound proves the terminate() path is still bounded — never a
        // hang (kQuitWaitMs + kTerminateWaitMs = 2500ms, plus scheduling slack).
        QVERIFY2(elapsed < 4000, qPrintable(QStringLiteral("terminate() teardown took %1 ms — must stay bounded "
                                                           "(kQuitWaitMs + kTerminateWaitMs)")
                                                .arg(elapsed)));
        // Thread actually finished: the chain destroyed its owned QThread, and
        // the destructor returned normally (no "Destroyed while running" qFatal).
        QVERIFY2(threadGuard.isNull(),
                 "the chain must destroy its owned QThread even on the terminate() path (thread finished)");
    }
};

QTEST_MAIN(TestGarminConnectAuthChain)
#include "testGarminConnectAuthChain.moc"
