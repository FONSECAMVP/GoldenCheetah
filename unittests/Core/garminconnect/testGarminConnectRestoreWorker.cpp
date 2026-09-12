/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-018 — REQ-007 closure (Slice 1/3): GarminWorker session-restore
// seam. A fresh CloudService session (no password — REQ-005) becomes
// download-capable by restoring the stored OAuth blob (REQ-006 tokens →
// REQ-NF-Compat-001(b) silent reauth from TOKENS, never a password). The worker
// is the SOLE caller of IGarminPyAdapter (DEC-002 / DES-001), so the restore op
// is symmetric to authenticate (TEST-004) and downloadActivity (TEST-010):
//
//   restoreSession(tokenBlob, requestId) forwards the blob VERBATIM to
//   adapter.loadTokens(), maps PyLoadTokensOutcome::Kind to either
//   sessionRestored(id) (Success) or restoreFailed(id, GarminRestoreFailure)
//   (SessionExpired / Network / Unknown), and runs the adapter off the GUI
//   thread (REQ-NF-Threads-001) — the same requestId round-trips so the future
//   GarminConnect::open() can correlate the async reply.
//
// Python-free by construction (FakeRestorePyAdapter stands in for
// PyEmbeddedAdapter, DEC-013 Option A seam) → `garmin-fast` CTest label, never
// `garmin-py`.
//
// RED expectation: GarminWorker has no restoreSession slot / sessionRestored /
// restoreFailed signals and GarminRestoreFailure + PyLoadTokensOutcome do not
// exist yet — the compile fails at those references (right-reason RED). GREEN
// adds them to src/Cloud/IGarminPyAdapter.h + src/Cloud/GarminWorker.{h,cpp}.

#include "GarminWorker.h"
#include "IGarminPyAdapter.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QString>
#include <QThread>
#include <QUuid>
#include <QtTest/QtTest>

#include <atomic>

// ---------------------------------------------------------------------------
// FakeRestorePyAdapter — Python-free IGarminPyAdapter whose loadTokens()
// returns a scripted PyLoadTokensOutcome and records the blob it received + the
// QThread it ran on (so the threading-invariant slot can assert it ran off the
// caller thread). authenticate()/downloadActivity() are unused here.
// ---------------------------------------------------------------------------
class FakeRestorePyAdapter : public IGarminPyAdapter
{
  public:
    PyLoadTokensOutcome scriptedOutcome;
    QString lastTokenBlob;
    std::atomic<int> callCount{0};
    QThread* threadSeen = nullptr;

    PyAuthOutcome authenticate(const QString&, const QString&) override { return {}; }
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }

    // REQ-008 Slice A seam extension (DEC-013 compile-enforced) — this restore
    // test never lists activities; a default outcome satisfies the interface so
    // the target still compiles. No assertion added/changed.
    PyListOutcome listActivitiesSince(const QString&) override { return {}; }
    // REQ-003 (MFA) Slice A seam extension (DEC-013 compile-enforced) — this
    // restore test never submits an OTP; a default outcome satisfies the seam.
    PyAuthOutcome submitMfa(const QString&) override { return {}; }

    PyLoadTokensOutcome loadTokens(const QString& tokenBlob) override
    {
        lastTokenBlob = tokenBlob;
        threadSeen = QThread::currentThread();
        callCount.fetch_add(1);
        return scriptedOutcome;
    }

    // REQ-013 (DEC-050) seam extension (DEC-013 compile-enforced) — this
    // restore test never fetches a profile; a default outcome satisfies the
    // interface.
    PyProfileOutcome fetchProfile() override { return {}; }
};

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

// A representative opaque OAuth blob (JSON-ish); the worker must forward it
// byte-for-byte with no re-encoding.
const QString kTokenBlob = QStringLiteral("{\"oauth1\":\"AAA\",\"oauth2\":\"BBB.refresh\"}");
} // namespace

class TestGarminConnectRestoreWorker : public QObject
{
    Q_OBJECT

  private slots:

    // Success outcome maps to sessionRestored(id) with the SAME requestId and
    // NO restoreFailed emission; the blob reaches the adapter VERBATIM.
    void restoreSuccessEmitsSessionRestoredWithSameRequestId()
    {
        FakeRestorePyAdapter fake;
        fake.scriptedOutcome.kind = PyLoadTokensOutcome::Success;
        GarminWorker worker(&fake);

        QSignalSpy restoredSpy(&worker, &GarminWorker::sessionRestored);
        QSignalSpy failedSpy(&worker, &GarminWorker::restoreFailed);

        const QUuid id = QUuid::createUuid();
        worker.restoreSession(kTokenBlob, id);

        QCOMPARE(restoredSpy.size(), 1);
        QCOMPARE(failedSpy.size(), 0);
        QCOMPARE(restoredSpy.first().at(0).toUuid(), id);
        // blob forwarded verbatim — no mangling / re-encoding
        QCOMPARE(fake.lastTokenBlob, kTokenBlob);
        QCOMPARE(int(fake.callCount.load()), 1);
    }

    // SessionExpired maps to restoreFailed{SessionExpired} (NOT Network, NOT
    // Unknown) — REQ-NF-Compat-001(b): a stored session that no longer works
    // must route to a re-login prompt, distinctly from a transient network dip.
    void sessionExpiredEmitsRestoreFailedSessionExpired()
    {
        FakeRestorePyAdapter fake;
        fake.scriptedOutcome.kind = PyLoadTokensOutcome::SessionExpired;
        fake.scriptedOutcome.rawMessage = QStringLiteral("stored session expired");
        GarminWorker worker(&fake);

        QSignalSpy restoredSpy(&worker, &GarminWorker::sessionRestored);
        QSignalSpy failedSpy(&worker, &GarminWorker::restoreFailed);

        const QUuid id = QUuid::createUuid();
        worker.restoreSession(kTokenBlob, id);

        QCOMPARE(restoredSpy.size(), 0);
        QCOMPARE(failedSpy.size(), 1);
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminRestoreFailure emitted = failedSpy.first().at(1).value<GarminRestoreFailure>();
        QVERIFY2(emitted.kind != GarminRestoreFailure::Network, "session-expired must not be misrouted to Network");
        QCOMPARE(emitted.kind, GarminRestoreFailure::SessionExpired);
        QCOMPARE(emitted.rawMessage, QStringLiteral("stored session expired"));
    }

    // Network maps to restoreFailed{Network} — the adapter's failure kind is
    // preserved through the worker (mutation catch on the mapping).
    void networkEmitsRestoreFailedNetwork()
    {
        FakeRestorePyAdapter fake;
        fake.scriptedOutcome.kind = PyLoadTokensOutcome::Network;
        fake.scriptedOutcome.rawMessage = QStringLiteral("connection refused");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::restoreFailed);

        worker.restoreSession(kTokenBlob, QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminRestoreFailure emitted = failedSpy.first().at(1).value<GarminRestoreFailure>();
        QCOMPARE(emitted.kind, GarminRestoreFailure::Network);
        QCOMPARE(emitted.rawMessage, QStringLiteral("connection refused"));
    }

    // Unknown maps to restoreFailed{Unknown} — catches a default-case mutant
    // that collapses Unknown into another kind.
    void unknownEmitsRestoreFailedUnknown()
    {
        FakeRestorePyAdapter fake;
        fake.scriptedOutcome.kind = PyLoadTokensOutcome::Unknown;
        fake.scriptedOutcome.rawMessage = QStringLiteral("library exploded");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::restoreFailed);

        worker.restoreSession(kTokenBlob, QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminRestoreFailure emitted = failedSpy.first().at(1).value<GarminRestoreFailure>();
        QCOMPARE(emitted.kind, GarminRestoreFailure::Unknown);
        QCOMPARE(emitted.rawMessage, QStringLiteral("library exploded"));
    }

    // REQ-NF-Threads-001: when the worker is moved to a QThread and the slot is
    // invoked via a queued connection, the adapter's loadTokens() must run on
    // the worker thread, NOT the caller/test thread — and sessionRestored() must
    // still reach a test-thread QSignalSpy.
    void adapterRunsOnWorkerThreadNotCaller()
    {
        FakeRestorePyAdapter fake;
        fake.scriptedOutcome.kind = PyLoadTokensOutcome::Success;

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        QSignalSpy restoredSpy(&worker, &GarminWorker::sessionRestored);
        const QUuid id = QUuid::createUuid();
        const bool queued = QMetaObject::invokeMethod(&worker, "restoreSession", Qt::QueuedConnection,
                                                      Q_ARG(QString, kTokenBlob), Q_ARG(QUuid, id));
        QVERIFY2(queued, "restoreSession must be an invokable slot for the queued cross-thread dispatch");

        QVERIFY2(waitFor([&] { return restoredSpy.size() >= 1; }),
                 "sessionRestored() must reach the test-thread spy after the worker-thread restore");
        QCOMPARE(int(fake.callCount.load()), 1);
        QVERIFY2(fake.threadSeen != nullptr && fake.threadSeen != QThread::currentThread(),
                 "REQ-NF-Threads-001: adapter.loadTokens must run on the worker thread, not the test thread");
        QVERIFY(fake.threadSeen == &workerThread);

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }
};

QTEST_MAIN(TestGarminConnectRestoreWorker)
#include "testGarminConnectRestoreWorker.moc"
