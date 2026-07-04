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

        QVERIFY2(elapsed < 3000, qPrintable(QStringLiteral("destructor took %1 ms — must tear down promptly "
                                                           "(bounded quit()+wait())")
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
        QVERIFY2(t.elapsed() < 3000, "post-request teardown must also be bounded");

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
};

QTEST_MAIN(TestGarminConnectAuthChain)
#include "testGarminConnectAuthChain.moc"
