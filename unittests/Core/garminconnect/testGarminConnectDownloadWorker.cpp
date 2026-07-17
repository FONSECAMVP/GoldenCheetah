/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST-010 — REQ-007 (slice 3/3): GarminWorker DownloadActivity op.
//   The worker is the sole caller of IGarminPyAdapter (DES-001). This slice
//   adds the download op symmetric to authenticate (TEST-004): a
//   downloadActivity(activityId, fmt, requestId) slot that forwards to the
//   adapter off the GUI thread and maps PyDownloadOutcome::Kind to either
//   downloaded(id, bytes) (Success) or downloadFailed(id, GarminDownloadFailure)
//   (Network / RateLimit / Unknown). The same requestId round-trips so the
//   consumer (the future GarminConnect::readFile — DES-004, deferred) can
//   correlate the async reply.
//
// Python-free by construction (FakeDownloadPyAdapter stands in for
// PyEmbeddedAdapter, DEC-013 Option A seam), so this target carries the
// `garmin-fast` CTest label — it must NEVER carry `garmin-py`.
//
// NOT in this slice (deferred, see STATE/design):
//   - GarminConnect::readFile staging bytes as garmin-<id>.<ext> — needs the
//     worker-in-CloudService lifecycle + loaded tokens (REQ-004/006).
//   - FIT->TCX fallback orchestration (DES-004) — its trigger ("FIT not
//     available") depends on unvalidated library behaviour (PRD Assumption B).
//   The worker op here takes `fmt` verbatim so the future fallback can request
//   ORIGINAL then TCX without changing this signature.
//
// Cites: REQ-007, REQ-NF-Threads-001 (adapter runs off the GUI thread);
//   DEC-002/DES-001 (worker is sole adapter caller), DEC-013/DES-001a
//   (IGarminPyAdapter seam), DES-013 (PyDownloadOutcome shape being mapped).
//
// RED expectation: GarminWorker has no downloadActivity slot / downloaded /
// downloadFailed signals and GarminDownloadFailure does not exist yet — the
// executable fails to compile at those references (right-reason RED). GREEN
// adds them to src/Cloud/GarminWorker.{h,cpp}.

#include "GarminWorker.h"
#include "IGarminPyAdapter.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QSignalSpy>
#include <QThread>
#include <QUuid>
#include <QtTest/QtTest>

#include <atomic>

// ---------------------------------------------------------------------------
// FakeDownloadPyAdapter — Python-free IGarminPyAdapter whose downloadActivity()
// returns a scripted PyDownloadOutcome and records the args + the thread it ran
// on (so the threading-invariant slot can assert it ran off the caller thread).
// ---------------------------------------------------------------------------
class FakeDownloadPyAdapter : public IGarminPyAdapter
{
  public:
    PyDownloadOutcome scriptedOutcome;
    QString lastActivityId;
    QString lastFmt;
    std::atomic<int> callCount{0};
    QThread* threadSeen = nullptr;

    PyAuthOutcome authenticate(const QString&, const QString&) override { return {}; }

    // REQ-007 closure (Slice 1) seam extension (DEC-013 compile-enforced) — this
    // download-worker test never restores a session; a default outcome satisfies
    // the interface so the target still compiles. No assertion added/changed.
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }

    PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) override
    {
        lastActivityId = activityId;
        lastFmt = fmt;
        threadSeen = QThread::currentThread();
        callCount.fetch_add(1);
        return scriptedOutcome;
    }
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

const QByteArray kFitBytes = QByteArray::fromRawData("\x00\x01\x02FIT\x00\xff\xfe\x0a", 10);
} // namespace

class TestGarminConnectDownloadWorker : public QObject
{
    Q_OBJECT

  private slots:

    // Success outcome maps to downloaded(uuid, bytes) with the SAME requestId,
    // the exact bytes (binary — QByteArray equality guards NUL survival), and
    // NO downloadFailed emission.
    void successOutcomeEmitsDownloadedWithSameRequestId()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::Success;
        fake.scriptedOutcome.data = kFitBytes;
        GarminWorker worker(&fake);

        QSignalSpy downloadedSpy(&worker, &GarminWorker::downloaded);
        QSignalSpy failedSpy(&worker, &GarminWorker::downloadFailed);

        const QUuid id = QUuid::createUuid();
        worker.downloadActivity(QStringLiteral("987654321"), QStringLiteral("ORIGINAL"), id);

        QCOMPARE(downloadedSpy.size(), 1);
        QCOMPARE(failedSpy.size(), 0);
        QCOMPARE(downloadedSpy.first().at(0).toUuid(), id);
        QCOMPARE(downloadedSpy.first().at(1).toByteArray(), kFitBytes);
        QCOMPARE(downloadedSpy.first().at(1).toByteArray().size(), kFitBytes.size());
    }

    // activityId + fmt reach the adapter verbatim — no mangling / no hard-coded
    // format. Catches an argument-swap or a "always ORIGINAL" mutant (which
    // would break the DES-004 TCX fallback once it lands).
    void activityIdAndFmtForwardedToAdapterVerbatim()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::Success;
        fake.scriptedOutcome.data = kFitBytes;
        GarminWorker worker(&fake);

        worker.downloadActivity(QStringLiteral("42-abc"), QStringLiteral("TCX"), QUuid::createUuid());

        QCOMPARE(fake.lastActivityId, QStringLiteral("42-abc"));
        QCOMPARE(fake.lastFmt, QStringLiteral("TCX"));
        QCOMPARE(int(fake.callCount.load()), 1);
    }

    // Network outcome maps to downloadFailed(uuid, {Network, rawMsg}) and NOT
    // downloaded. The consumer applies DES-008 translation; the worker forwards
    // the raw message + the kind.
    void networkOutcomeEmitsDownloadFailedNetwork()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::Network;
        fake.scriptedOutcome.rawMessage = QStringLiteral("connection refused");
        GarminWorker worker(&fake);

        QSignalSpy downloadedSpy(&worker, &GarminWorker::downloaded);
        QSignalSpy failedSpy(&worker, &GarminWorker::downloadFailed);

        const QUuid id = QUuid::createUuid();
        worker.downloadActivity(QStringLiteral("1"), QStringLiteral("ORIGINAL"), id);

        QCOMPARE(downloadedSpy.size(), 0);
        QCOMPARE(failedSpy.size(), 1);
        QCOMPARE(failedSpy.first().at(0).toUuid(), id);
        const GarminDownloadFailure emitted = failedSpy.first().at(1).value<GarminDownloadFailure>();
        QCOMPARE(emitted.kind, GarminDownloadFailure::Network);
        QCOMPARE(emitted.rawMessage, QStringLiteral("connection refused"));
    }

    // RateLimited maps to a DISTINCT downloadFailed kind (RateLimit), not
    // Network and not Unknown — DES-008 has dedicated rate-limit copy.
    void rateLimitOutcomeEmitsDownloadFailedRateLimit()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::RateLimited;
        fake.scriptedOutcome.rawMessage = QStringLiteral("429 slow down");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::downloadFailed);

        worker.downloadActivity(QStringLiteral("1"), QStringLiteral("ORIGINAL"), QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminDownloadFailure emitted = failedSpy.first().at(1).value<GarminDownloadFailure>();
        QVERIFY2(emitted.kind != GarminDownloadFailure::Network, "rate limit must not be misrouted to Network");
        QCOMPARE(emitted.kind, GarminDownloadFailure::RateLimit);
        QCOMPARE(emitted.rawMessage, QStringLiteral("429 slow down"));
    }

    // Unknown maps to downloadFailed{Unknown} — catches a default-case mutant
    // that collapses Unknown into another kind.
    void unknownOutcomeEmitsDownloadFailedUnknown()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::Unknown;
        fake.scriptedOutcome.rawMessage = QStringLiteral("library exploded");
        GarminWorker worker(&fake);
        QSignalSpy failedSpy(&worker, &GarminWorker::downloadFailed);

        worker.downloadActivity(QStringLiteral("1"), QStringLiteral("ORIGINAL"), QUuid::createUuid());

        QCOMPARE(failedSpy.size(), 1);
        const GarminDownloadFailure emitted = failedSpy.first().at(1).value<GarminDownloadFailure>();
        QCOMPARE(emitted.kind, GarminDownloadFailure::Unknown);
        QCOMPARE(emitted.rawMessage, QStringLiteral("library exploded"));
    }

    // REQ-NF-Threads-001: when the worker is moved to a QThread and the slot is
    // invoked via a queued connection, the adapter's downloadActivity() must run
    // on the worker thread, NOT the caller/test thread — and downloaded() must
    // still reach a test-thread QSignalSpy.
    void adapterRunsOnWorkerThreadNotCaller()
    {
        FakeDownloadPyAdapter fake;
        fake.scriptedOutcome.kind = PyDownloadOutcome::Success;
        fake.scriptedOutcome.data = kFitBytes;

        QThread workerThread;
        GarminWorker worker(&fake);
        worker.moveToThread(&workerThread);
        workerThread.start();

        QSignalSpy downloadedSpy(&worker, &GarminWorker::downloaded);
        const QUuid id = QUuid::createUuid();
        const bool queued = QMetaObject::invokeMethod(&worker, "downloadActivity", Qt::QueuedConnection,
                                                      Q_ARG(QString, QStringLiteral("1")),
                                                      Q_ARG(QString, QStringLiteral("ORIGINAL")), Q_ARG(QUuid, id));
        QVERIFY2(queued, "downloadActivity must be an invokable slot for the queued cross-thread dispatch");

        QVERIFY2(waitFor([&] { return downloadedSpy.size() >= 1; }),
                 "downloaded() must reach the test-thread spy after the worker-thread download");
        QCOMPARE(int(fake.callCount.load()), 1);
        QVERIFY2(fake.threadSeen != nullptr && fake.threadSeen != QThread::currentThread(),
                 "REQ-NF-Threads-001: adapter.downloadActivity must run on the worker thread, not the test thread");
        QVERIFY(fake.threadSeen == &workerThread);

        workerThread.quit();
        QVERIFY(workerThread.wait(2000));
    }
};

QTEST_MAIN(TestGarminConnectDownloadWorker)
#include "testGarminConnectDownloadWorker.moc"
