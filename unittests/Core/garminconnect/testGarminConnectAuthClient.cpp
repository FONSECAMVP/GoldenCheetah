/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST-004 — REQ-002 (end-to-end Authenticate): "Valid email+password produces
//   persisted OAuth tokens; invalid credentials produce a labeled error."
//
// Slice scope — C++ worker + WorkerAuthClient. Pairs with TEST-003's wizard-
// side acceptance: this slice closes the loop end-to-end by demonstrating that
// the wizard page's IGarminAuthClient seam, driven through the real
// WorkerAuthClient on a real QThread, hits a Python adapter on a worker
// thread (FakePyAdapter substitutes for PyEmbeddedAdapter — DEC-013) and
// re-emits the response back on the GUI thread.
//
// Token persistence (REQ-004 / REQ-006), AddCloudWizard tile routing,
// rate-limit / retry (DES-005 — Python-side), and the real embedded-Python
// sub-interpreter (PyEmbeddedAdapter — exercised by Python-side
// test_adapter_login.py) are explicitly deferred. MFA (REQ-003) and CAPTCHA
// (REQ-015) extend the same interface in their own slices.
//
// Cites:
//   DES-001   — GarminWorker: dedicated worker thread + mailbox
//   DES-001a  — IGarminPyAdapter interface (DEC-013)
//   DES-003a  — IGarminAuthClient interface (DEC-012; re-emitted by WAC)
//   DEC-013   — worker ↔ Python adapter seam (Option A — interface injection)
//   DEC-002   — Python integration mechanism (worker thread)
//   DEC-008   — testing toolchain (QTest+CTest under `garmin-fast` label)
//   REQ-NF-Threads-001 — adapter call must happen off the GUI thread
//
// RED expectation:
//   src/Cloud/IGarminPyAdapter.h, src/Cloud/GarminWorker.{h,cpp}, and
//   src/Cloud/WorkerAuthClient.{h,cpp} do not exist yet. The build fails at
//   the #include lines below — right-reason RED (missing contract under test,
//   not a wiring artifact). GREEN introduces those files; every assertion in
//   this file must then pass without altering the public surface locked in.

#include "GarminWorker.h" // <-- intentionally missing in RED
#include "IGarminAuthClient.h"
#include "IGarminPyAdapter.h" // <-- intentionally missing in RED
#include "WorkerAuthClient.h" // <-- intentionally missing in RED

#include <QCoreApplication>
#include <QSignalSpy>
#include <QString>
#include <QThread>
#include <QUuid>
#include <QtTest/QtTest>

#include <atomic>

// ---------------------------------------------------------------------------
// FakePyAdapter — records the (email, password) it was called with and the
// QThread it was invoked on; returns a scripted PyAuthOutcome. The recorded
// thread is the keystone of REQ-NF-Threads-001 — if the worker accidentally
// runs the adapter on the GUI thread the threadSeen field will equal the
// test thread and the assertion in adapterCalledOnWorkerThreadNotGuiThread
// will fail.
// ---------------------------------------------------------------------------
class FakePyAdapter : public IGarminPyAdapter
{
  public:
    PyAuthOutcome scriptedOutcome;
    QString lastEmail;
    QString lastPassword;
    std::atomic<int> callCount{0};
    QThread* threadSeen = nullptr;

    // REQ-003 (MFA) Slice A — scripted submitMfa() outcome + the code it saw,
    // so the MFA worker tests (T-029..T-031) can drive the resume-auth path.
    PyAuthOutcome scriptedSubmitMfaOutcome;
    QString lastMfaCode;
    std::atomic<int> submitMfaCallCount{0};

    PyAuthOutcome authenticate(const QString& email, const QString& password) override
    {
        lastEmail = email;
        lastPassword = password;
        threadSeen = QThread::currentThread();
        callCount.fetch_add(1);
        return scriptedOutcome;
    }

    // REQ-003 (MFA) Slice A seam extension (DEC-013 compile-enforced) — resumes
    // the pending MFA session established by a prior authenticate() that returned
    // MfaRequired. Records the code and returns a scripted PyAuthOutcome.
    PyAuthOutcome submitMfa(const QString& code) override
    {
        lastMfaCode = code;
        submitMfaCallCount.fetch_add(1);
        return scriptedSubmitMfaOutcome;
    }

    // REQ-007 seam extension (DEC-013 compile-enforced) — this auth test never
    // downloads; a default outcome satisfies the interface.
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }

    // REQ-007 closure (Slice 1) seam extension (DEC-013 compile-enforced) — this
    // auth-only test never restores a session; a default outcome satisfies the
    // interface so the target still compiles.
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }
};

// ---------------------------------------------------------------------------
// Helper — drains the worker thread's event queue until a predicate is true
// or a timeout elapses. Used in the WorkerAuthClient + real-QThread tests.
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

class TestGarminConnectAuthClient : public QObject
{
    Q_OBJECT

  private slots:

    // -----------------------------------------------------------------------
    // Slice A — GarminWorker behaviour against a synchronous FakePyAdapter.
    // The worker is exercised directly (its authenticate() slot called from
    // the test thread). Mapping from PyAuthOutcome::Kind to GarminAuthSuccess
    // vs GarminAuthFailure is the worker's responsibility — these tests lock
    // each kind's path.
    // -----------------------------------------------------------------------

    // REQ-002 positive path: a Success outcome from the adapter maps to a
    // finished(uuid, GarminAuthSuccess{garmin_user_id, display_name}) signal
    // on the worker, with the *same* requestId that was passed in (so the
    // page-side stale-reply guard can correlate).
    void successOutcomeEmitsFinishedWithSameRequestId()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-99");
        fake.scriptedOutcome.display_name = QStringLiteral("Rider");
        GarminWorker worker(&fake);

        QSignalSpy finishedSpy(&worker, &GarminWorker::finished);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        const QUuid id = QUuid::createUuid();
        worker.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("hunter2"), id);

        QCOMPARE(finishedSpy.size(), 1);
        QCOMPARE(failedSpy.size(), 0);
        QCOMPARE(finishedSpy.first().at(0).toUuid(), id);
        const GarminAuthSuccess emitted = finishedSpy.first().at(1).value<GarminAuthSuccess>();
        QCOMPARE(emitted.garmin_user_id, QStringLiteral("uid-99"));
        QCOMPARE(emitted.display_name, QStringLiteral("Rider"));
    }

    // REQ-002 negative path: AuthFailed maps to failed(uuid, {Auth, rawMsg}).
    // The page applies DES-008 translation on top; the worker just maps the
    // kind enum and forwards the raw library message.
    void authFailedOutcomeMapsToFailedWithAuthKind()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::AuthFailed;
        fake.scriptedOutcome.rawMessage = QStringLiteral("bad credentials");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        const QUuid id = QUuid::createUuid();
        worker.authenticate(QStringLiteral("e@x"), QStringLiteral("wrong"), id);

        QCOMPARE(failedSpy.size(), 1);
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Auth);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("bad credentials"));
    }

    // Network outcome maps to GarminAuthFailure::Network (NOT Auth — mutation
    // catch: a copy/paste bug that always emits Auth would pass the AuthFailed
    // test above but fail here).
    void networkOutcomeMapsToFailedWithNetworkKind()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Network;
        fake.scriptedOutcome.rawMessage = QStringLiteral("DNS lookup failed");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        worker.authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Network);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("DNS lookup failed"));
    }

    // Unknown outcome maps to GarminAuthFailure::Unknown — catches a default-
    // case mutant that collapses Unknown into Auth (or vice versa).
    void unknownOutcomeMapsToFailedWithUnknownKind()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Unknown;
        fake.scriptedOutcome.rawMessage = QStringLiteral("library exploded");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        worker.authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Unknown);
    }

    // The exact email + password strings the worker was given must reach the
    // adapter verbatim — no mangling, no trimming, no logging substitution.
    // Catches argument-swap and string-rebind mutants.
    void credentialsForwardedToAdapterVerbatim()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        GarminWorker worker(&fake);

        worker.authenticate(QStringLiteral("strange+addr@sub.example.com"), QStringLiteral("p@$$w/rd with spaces"),
                            QUuid::createUuid());

        QCOMPARE(fake.lastEmail, QStringLiteral("strange+addr@sub.example.com"));
        QCOMPARE(fake.lastPassword, QStringLiteral("p@$$w/rd with spaces"));
        QCOMPARE(int(fake.callCount.load()), 1);
    }

    // -----------------------------------------------------------------------
    // Slice B — WorkerAuthClient on a real QThread.
    // The worker is moved to a QThread; WAC dispatches via QueuedConnection.
    // The interface signals (IGarminAuthClient::finished / ::failed) must
    // reach the GUI-thread test handler unchanged.
    // -----------------------------------------------------------------------

    // WAC.authenticate(...) must result in fakePy.authenticate(...) being
    // invoked on the worker thread (the thread the GarminWorker was moved
    // to), NOT on the GUI/test thread — REQ-NF-Threads-001 page-side hint.
    void workerAuthClientForwardsAndAdapterRunsOnWorkerThread()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-1");
        fake.scriptedOutcome.display_name = QStringLiteral("Rider");

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        WorkerAuthClient wac(&worker);
        const QUuid id = QUuid::createUuid();
        QSignalSpy finishedSpy(&wac, &IGarminAuthClient::finished);

        wac.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("hunter2"), id);

        QVERIFY2(waitFor([&] { return finishedSpy.size() >= 1; }),
                 "WorkerAuthClient.authenticate must dispatch through the worker thread and "
                 "re-emit IGarminAuthClient::finished on the GUI/test thread within timeout");
        QCOMPARE(int(fake.callCount.load()), 1);
        QCOMPARE(fake.lastEmail, QStringLiteral("rider@example.com"));
        QVERIFY2(fake.threadSeen != nullptr && fake.threadSeen != QThread::currentThread(),
                 "REQ-NF-Threads-001: FakePyAdapter.authenticate must run on the worker thread, "
                 "NOT the GUI/test thread (QThread::currentThread() in the adapter must differ "
                 "from the test thread)");
        QVERIFY(fake.threadSeen == &workerThread);

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }

    // WAC must re-emit GarminWorker::finished as IGarminAuthClient::finished
    // unchanged (same uuid, same payload). Catches mutants that drop the
    // re-emit connection.
    void workerAuthClientReemitsFinished()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-77");
        fake.scriptedOutcome.display_name = QStringLiteral("Bob");

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        WorkerAuthClient wac(&worker);
        QSignalSpy finishedSpy(&wac, &IGarminAuthClient::finished);

        const QUuid id = QUuid::createUuid();
        wac.authenticate(QStringLiteral("e@x"), QStringLiteral("p"), id);

        QVERIFY(waitFor([&] { return finishedSpy.size() >= 1; }));
        QCOMPARE(finishedSpy.first().at(0).toUuid(), id);
        const GarminAuthSuccess emitted = finishedSpy.first().at(1).value<GarminAuthSuccess>();
        QCOMPARE(emitted.garmin_user_id, QStringLiteral("uid-77"));
        QCOMPARE(emitted.display_name, QStringLiteral("Bob"));

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }

    // Symmetric to the previous test — WAC must re-emit GarminWorker::failed
    // as IGarminAuthClient::failed (so the page's onAuthFailed slot fires).
    void workerAuthClientReemitsFailed()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::AuthFailed;
        fake.scriptedOutcome.rawMessage = QStringLiteral("invalid creds");

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        WorkerAuthClient wac(&worker);
        QSignalSpy failedSpy(&wac, &IGarminAuthClient::failed);

        const QUuid id = QUuid::createUuid();
        wac.authenticate(QStringLiteral("e@x"), QStringLiteral("bad"), id);

        QVERIFY(waitFor([&] { return failedSpy.size() >= 1; }));
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Auth);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("invalid creds"));

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }

    // -----------------------------------------------------------------------
    // Slice C — REQ-003 (MFA) Slice A: the two-step MFA seam at the worker.
    // authenticate() surfaces an MFA-required outcome as a distinct signal
    // (mfaRequired) rather than finished/failed; a later submitMfa() completes
    // or fails auth on the SAME session with the SAME Success/failure mapping
    // authenticate() uses. The no-MFA path stays byte-for-byte unchanged
    // (T-030 regression guard).
    // -----------------------------------------------------------------------

    // T-029 — authenticate() with a fake adapter returning MfaRequired emits
    // mfaRequired(id) EXACTLY ONCE and emits NEITHER finished NOR failed. This
    // is what lets the (Slice-B) wizard open the MFA dialog instead of ending
    // the auth attempt.
    void authenticateMfaRequiredEmitsMfaRequiredOnlyOnce()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::MfaRequired;
        GarminWorker worker(&fake);

        QSignalSpy mfaSpy(&worker, &GarminWorker::mfaRequired);
        QSignalSpy finishedSpy(&worker, &GarminWorker::finished);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        const QUuid id = QUuid::createUuid();
        worker.authenticate(QStringLiteral("mfa@example.com"), QStringLiteral("pw"), id);

        QCOMPARE(mfaSpy.size(), 1);
        QCOMPARE(mfaSpy.first().at(0).toUuid(), id);
        QCOMPARE(finishedSpy.size(), 0);
        QCOMPARE(failedSpy.size(), 0);
    }

    // T-030 — submitMfa(code, id) with a fake adapter returning Success emits
    // finished(id, GarminAuthSuccess{...}) with the success payload; and the
    // no-MFA authenticate() path still emits finished/failed unchanged
    // (regression guard for the additive change).
    void submitMfaSuccessEmitsFinishedAndNoMfaPathUnchanged()
    {
        FakePyAdapter fake;
        fake.scriptedSubmitMfaOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedSubmitMfaOutcome.garmin_user_id = QStringLiteral("uid-mfa-1");
        fake.scriptedSubmitMfaOutcome.display_name = QStringLiteral("MFA Rider");
        GarminWorker worker(&fake);

        QSignalSpy finishedSpy(&worker, &GarminWorker::finished);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);
        QSignalSpy mfaSpy(&worker, &GarminWorker::mfaRequired);

        const QUuid id = QUuid::createUuid();
        worker.submitMfa(QStringLiteral("123456"), id);

        QCOMPARE(fake.lastMfaCode, QStringLiteral("123456"));
        QCOMPARE(int(fake.submitMfaCallCount.load()), 1);
        QCOMPARE(finishedSpy.size(), 1);
        QCOMPARE(failedSpy.size(), 0);
        QCOMPARE(mfaSpy.size(), 0);
        QCOMPARE(finishedSpy.first().at(0).toUuid(), id);
        const GarminAuthSuccess emitted = finishedSpy.first().at(1).value<GarminAuthSuccess>();
        QCOMPARE(emitted.garmin_user_id, QStringLiteral("uid-mfa-1"));
        QCOMPARE(emitted.display_name, QStringLiteral("MFA Rider"));

        // Regression guard: the no-MFA authenticate() path is unchanged — a
        // plain Success still emits finished, and an AuthFailed still emits
        // failed{Auth}, with NO spurious mfaRequired.
        FakePyAdapter okFake;
        okFake.scriptedOutcome.kind = PyAuthOutcome::Success;
        okFake.scriptedOutcome.garmin_user_id = QStringLiteral("uid-plain");
        okFake.scriptedOutcome.display_name = QStringLiteral("Plain Rider");
        GarminWorker okWorker(&okFake);
        QSignalSpy okFinished(&okWorker, &GarminWorker::finished);
        QSignalSpy okFailed(&okWorker, &GarminWorker::failed);
        QSignalSpy okMfa(&okWorker, &GarminWorker::mfaRequired);
        okWorker.authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());
        QCOMPARE(okFinished.size(), 1);
        QCOMPARE(okFailed.size(), 0);
        QCOMPARE(okMfa.size(), 0);

        FakePyAdapter badFake;
        badFake.scriptedOutcome.kind = PyAuthOutcome::AuthFailed;
        badFake.scriptedOutcome.rawMessage = QStringLiteral("bad creds");
        GarminWorker badWorker(&badFake);
        QSignalSpy badFinished(&badWorker, &GarminWorker::finished);
        QSignalSpy badFailed(&badWorker, &GarminWorker::failed);
        QSignalSpy badMfa(&badWorker, &GarminWorker::mfaRequired);
        badWorker.authenticate(QStringLiteral("e@x"), QStringLiteral("p"), QUuid::createUuid());
        QCOMPARE(badFinished.size(), 0);
        QCOMPARE(badFailed.size(), 1);
        QCOMPARE(badMfa.size(), 0);
        const GarminAuthFailure badErr = badFailed.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(badErr.kind, GarminAuthFailure::Auth);
    }

    // T-031 — submitMfa() with a fake adapter returning AuthFailed emits
    // failed(id, GarminAuthFailure{kind=Auth}) and NOT finished — so the
    // Slice-B page can re-prompt for the OTP.
    void submitMfaAuthFailedEmitsFailedAuthKind()
    {
        FakePyAdapter fake;
        fake.scriptedSubmitMfaOutcome.kind = PyAuthOutcome::AuthFailed;
        fake.scriptedSubmitMfaOutcome.rawMessage = QStringLiteral("invalid one-time code");
        GarminWorker worker(&fake);

        QSignalSpy finishedSpy(&worker, &GarminWorker::finished);
        QSignalSpy failedSpy(&worker, &GarminWorker::failed);

        const QUuid id = QUuid::createUuid();
        worker.submitMfa(QStringLiteral("000000"), id);

        QCOMPARE(finishedSpy.size(), 0);
        QCOMPARE(failedSpy.size(), 1);
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Auth);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("invalid one-time code"));
    }

    // -----------------------------------------------------------------------
    // T-037 — B-R003-01: WorkerAuthClient MFA wiring asserted end-to-end.
    // The two REQ-003 connections in WorkerAuthClient's ctor are exercised over
    // a real worker thread (Qt::AutoConnection → QueuedConnection):
    //   (b) GarminWorker::mfaRequired → IGarminAuthClient::mfaRequired re-emit;
    //   (a) WorkerAuthClient::dispatchSubmitMfa → GarminWorker::submitMfa, whose
    //       outcome re-emits finished/failed through the interface.
    // This test FAILS if EITHER connect() were dropped: dropping the mfaRequired
    // re-emit starves mfaSpy; dropping dispatchSubmitMfa starves the worker so
    // finishedSpy/failedSpy never fire (the waitFor times out).
    // -----------------------------------------------------------------------

    // (b) + (a-success): authenticate→MfaRequired re-emits interface mfaRequired;
    // submitMfa reaches the worker (records the code) and re-emits finished.
    void workerAuthClientReemitsMfaRequiredAndDispatchesSubmitMfa()
    {
        FakePyAdapter fake;
        fake.scriptedOutcome.kind = PyAuthOutcome::MfaRequired; // authenticate → mfaRequired
        fake.scriptedSubmitMfaOutcome.kind = PyAuthOutcome::Success;
        fake.scriptedSubmitMfaOutcome.garmin_user_id = QStringLiteral("uid-mfa-9");
        fake.scriptedSubmitMfaOutcome.display_name = QStringLiteral("MFA Rider");

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        WorkerAuthClient wac(&worker);
        QSignalSpy mfaSpy(&wac, &IGarminAuthClient::mfaRequired);
        QSignalSpy finishedSpy(&wac, &IGarminAuthClient::finished);
        QSignalSpy failedSpy(&wac, &IGarminAuthClient::failed);

        // (b) authenticate → MfaRequired → worker emits mfaRequired → WAC re-emits.
        const QUuid authId = QUuid::createUuid();
        wac.authenticate(QStringLiteral("mfa@example.com"), QStringLiteral("pw"), authId);
        QVERIFY2(waitFor([&] { return mfaSpy.size() >= 1; }),
                 "WorkerAuthClient must re-emit IGarminAuthClient::mfaRequired for an MfaRequired outcome "
                 "(the GarminWorker::mfaRequired → IGarminAuthClient::mfaRequired connect must exist)");
        QCOMPARE(mfaSpy.first().at(0).toUuid(), authId);
        QCOMPARE(finishedSpy.size(), 0);
        QCOMPARE(failedSpy.size(), 0);

        // (a) submitMfa → dispatchSubmitMfa → GarminWorker::submitMfa (records the
        // exact code) → Success → re-emitted through IGarminAuthClient::finished.
        const QUuid mfaId = QUuid::createUuid();
        wac.submitMfa(QStringLiteral("246810"), mfaId);
        QVERIFY2(waitFor([&] { return finishedSpy.size() >= 1; }),
                 "WorkerAuthClient::submitMfa must dispatch to GarminWorker::submitMfa "
                 "(the dispatchSubmitMfa → GarminWorker::submitMfa connect must exist)");
        QCOMPARE(fake.lastMfaCode, QStringLiteral("246810"));
        QCOMPARE(int(fake.submitMfaCallCount.load()), 1);
        QCOMPARE(finishedSpy.first().at(0).toUuid(), mfaId);
        const GarminAuthSuccess ok = finishedSpy.first().at(1).value<GarminAuthSuccess>();
        QCOMPARE(ok.garmin_user_id, QStringLiteral("uid-mfa-9"));
        QCOMPARE(ok.display_name, QStringLiteral("MFA Rider"));

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }

    // (a-failure): a submitMfa whose adapter outcome is AuthFailed re-emits
    // IGarminAuthClient::failed{Auth} through the interface (so the Slice-B page
    // can re-prompt). Complements the success leg above.
    void workerAuthClientReemitsSubmitMfaFailure()
    {
        FakePyAdapter fake;
        fake.scriptedSubmitMfaOutcome.kind = PyAuthOutcome::AuthFailed;
        fake.scriptedSubmitMfaOutcome.rawMessage = QStringLiteral("invalid one-time code");

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        WorkerAuthClient wac(&worker);
        QSignalSpy failedSpy(&wac, &IGarminAuthClient::failed);

        const QUuid mfaId = QUuid::createUuid();
        wac.submitMfa(QStringLiteral("000000"), mfaId);

        QVERIFY(waitFor([&] { return failedSpy.size() >= 1; }));
        QCOMPARE(fake.lastMfaCode, QStringLiteral("000000"));
        QCOMPARE(failedSpy.first().at(0).toUuid(), mfaId);
        const GarminAuthFailure emitted = failedSpy.first().at(1).value<GarminAuthFailure>();
        QCOMPARE(emitted.kind, GarminAuthFailure::Auth);
        QCOMPARE(emitted.translatedMessage, QStringLiteral("invalid one-time code"));

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }
};

QTEST_MAIN(TestGarminConnectAuthClient)
#include "testGarminConnectAuthClient.moc"
