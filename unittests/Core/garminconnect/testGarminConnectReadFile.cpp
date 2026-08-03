/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-020..T-023 — REQ-007 closure (Slice 3/3): GarminConnect::readFile
// implements the DEC-016 retry table VERBATIM:
//
//   request ORIGINAL (FIT) via the worker → downloaded / downloadFailed(kind)
//     kind == RateLimited → FAIL fast (NO retry — anti retry-storm)
//     kind == Network / Unknown → retry once as TCX
//   on downloaded(bytes): unzip (ORIGINAL is ZIP-wrapped); sniff FIT magic
//     (".FIT" at byte offset 8)
//     is FIT  → stage as garmin-<id>.fit  (*data = unzipped FIT bytes)
//     not FIT → retry once as TCX
//   TCX retry → stage as garmin-<id>.tcx  (or surface failure if it also fails)
//
// The acceptance criterion this encodes: "GC calls download_activity(fmt=
// ORIGINAL->FIT) for each new activity. Bytes are written under the activity's
// natural filename; the standard FitRideFile parser produces a RideItem. TCX
// fallback only if FIT is not available." Here we lock the readFile byte-
// resolution + staging filename (garmin-<id>.fit / .tcx drives uncompressRide's
// FitRideFile-vs-TCX parser selection) and the fallback/no-fallback control flow.
//
// Python-free: a fake IGarminDownloadClient (DES-004 seam) stands in for the
// worker host, and stubs/ReadFileStubPreamble.h stubs CloudService + replaces
// PyEmbeddedAdapter with a Python-free fake → `garmin-fast` label, never
// garmin-py. Real ZipReader/ZipWriter (contrib/qzip + ZLIB) exercise the ZIP
// unwrap for real.

#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "zipreader.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QHash>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>
#include <QtTest/QtTest>

#include <new> // placement-new for the TEST-025 lifetime harness

// ---------------------------------------------------------------------------
// FakeDownloadClient — Python-free IGarminDownloadClient. Scripts a response per
// requested fmt ("ORIGINAL" / "TCX") and records the exact fmt-call sequence so
// a test can assert BOTH the fallback (ORIGINAL then TCX) and the no-fallback
// (exactly one ORIGINAL) paths. Emissions are posted via a queued invocation so
// they arrive while GarminConnect's blocking QEventLoop is running.
// ---------------------------------------------------------------------------
class FakeDownloadClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    struct Resp
    {
        bool ok = false;
        QByteArray bytes;
        GarminDownloadFailure::Kind kind = GarminDownloadFailure::Unknown;
    };

    QHash<QString, Resp> responses; // fmt → scripted response
    QStringList downloadFmts;       // recorded fmt-call sequence
    int downloadCalls = 0;

    void restoreSession(const QString&, QUuid id) override
    {
        // Not exercised by the readFile tests; restore always "succeeds" so a
        // misuse would surface loudly rather than hang.
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    // REQ-008 Slice C seam extension (IGarminDownloadClient gained a pure-virtual
    // list op). The readFile tests never list; a no-op satisfies the interface.
    void listActivities(const QString& /*sinceGmt*/, QUuid /*id*/) override {}

    void downloadActivity(const QString& /*activityId*/, const QString& fmt, QUuid id) override
    {
        downloadFmts << fmt;
        ++downloadCalls;
        const Resp r = responses.value(fmt);
        QMetaObject::invokeMethod(
            this,
            [this, id, r]() {
                if (r.ok) {
                    emit downloaded(id, r.bytes);
                } else {
                    GarminDownloadFailure e;
                    e.kind = r.kind;
                    e.rawMessage = QStringLiteral("scripted failure");
                    emit downloadFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }
};

namespace {
// A minimal but valid FIT header: ".FIT" ASCII at byte offset 8 (DEC-016).
QByteArray makeFitBytes()
{
    QByteArray b(12, '\0');
    b[0] = 0x0C; // header size
    b[1] = 0x10; // protocol version
    b[8] = '.';
    b[9] = 'F';
    b[10] = 'I';
    b[11] = 'T';
    b.append("\x00\x01record-payload"); // plausible body so it is not just a header
    return b;
}

// Build a real ZIP (contrib/qzip ZipWriter) wrapping one entry — mirrors how a
// Garmin ORIGINAL download arrives (ZIP-wrapped).
QByteArray makeZip(const QString& entryName, const QByteArray& content)
{
    QTemporaryFile tf;
    tf.open();
    const QString path = tf.fileName();
    tf.close();
    {
        ZipWriter w(path);
        w.addFile(entryName, content);
        w.close();
    }
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    const QByteArray z = f.readAll();
    f.close();
    return z;
}

const QByteArray kTcxBytes =
    QByteArray("<?xml version=\"1.0\"?><TrainingCenterDatabase>tcx-body</TrainingCenterDatabase>");

// DEC-garmin-020 (A3-R012-F1): readFile now FAILS CLOSED unless the athlete's
// stored Garmin credential is present and acceptable AT CALL TIME. Every slot
// below exercises the DEC-016 download decision table for a CONNECTED account, so
// the fixture must represent one — an athlete config dir carrying a real
// tokens.json + active-account.json written by the production producer, instead of
// the empty config-dir override these slots used before. (This makes the fixture
// MORE faithful to production, where a download only ever follows a connect.)
const QString kUid = QStringLiteral("123456789");
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

bool connectAccount(const QString& athleteConfigDir)
{
    return GarminTokenStore::persistConnectSuccess(athleteConfigDir, kUid, kBlob);
}
} // namespace

class TestGarminConnectReadFile : public QObject
{
    Q_OBJECT

  private slots:

    // TEST-020 — FIT happy path: ORIGINAL success, ZIP-wrapped FIT → unwrapped,
    // FIT-magic sniffed, *data == the FIT bytes, staged as garmin-<id>.fit, and
    // NO TCX retry (exactly one ORIGINAL download call).
    void fitHappyPathUnwrapsAndStagesFitNoTcxRetry()
    {
        const QByteArray fit = makeFitBytes();
        FakeDownloadClient fake;
        fake.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("123.fit"), fit), {}};
        // A TCX response is scripted but must NEVER be requested on the FIT path.
        fake.responses[QStringLiteral("TCX")] = {true, kTcxBytes, {}};

        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");
        GarminConnect gc(nullptr, &fake, cfg.path());
        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("123"));

        QVERIFY2(ok, "FIT happy path must return true");
        QCOMPARE(data, fit); // the UNZIPPED FIT bytes, not the ZIP
        QCOMPARE(fake.downloadCalls, 1);
        QCOMPARE(fake.downloadFmts, (QStringList{QStringLiteral("ORIGINAL")}));

        // DEC-016 staging: garmin-<id>.fit (drives the FitRideFile parser). The
        // completion is now a QUEUED self-post (B-R007-01), so the staged name/bytes
        // land once an event loop turns — QTRY_COMPARE pumps it (same expected value).
        QTRY_COMPARE(static_cast<CloudService&>(gc).lastReadName, QStringLiteral("garmin-123.fit"));
        QCOMPARE(static_cast<CloudService&>(gc).lastReadData, fit);
    }

    // TEST-021 — fallback: first ORIGINAL attempt fails with kind=Network (and a
    // separate case kind=Unknown) → readFile retries with fmt="TCX", *data ==
    // TCX bytes, staged garmin-<id>.tcx.
    void networkOrUnknownOnOriginalRetriesTcx_data()
    {
        QTest::addColumn<int>("kind");
        QTest::newRow("Network") << int(GarminDownloadFailure::Network);
        QTest::newRow("Unknown") << int(GarminDownloadFailure::Unknown);
    }

    void networkOrUnknownOnOriginalRetriesTcx()
    {
        QFETCH(int, kind);
        FakeDownloadClient fake;
        fake.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::Kind(kind)};
        fake.responses[QStringLiteral("TCX")] = {true, kTcxBytes, {}};

        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");
        GarminConnect gc(nullptr, &fake, cfg.path());
        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("77"));

        QVERIFY2(ok, "TCX fallback must return true");
        QCOMPARE(data, kTcxBytes);
        // ORIGINAL first, then exactly one TCX retry.
        QCOMPARE(fake.downloadFmts, (QStringList{QStringLiteral("ORIGINAL"), QStringLiteral("TCX")}));
        // Completion is a QUEUED self-post (B-R007-01) — pump the loop to observe it.
        QTRY_COMPARE(static_cast<CloudService&>(gc).lastReadName, QStringLiteral("garmin-77.tcx"));
        QCOMPARE(static_cast<CloudService&>(gc).lastReadData, kTcxBytes);
    }

    // TEST-022 — kind=RateLimited on ORIGINAL → readFile returns false and makes
    // NO second (TCX) request (assert the worker saw exactly one download call).
    void rateLimitedOnOriginalFailsFastNoTcxRetry()
    {
        FakeDownloadClient fake;
        fake.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::RateLimit};
        // Present but must NOT be requested — RateLimited is fail-fast (anti retry-storm).
        fake.responses[QStringLiteral("TCX")] = {true, kTcxBytes, {}};

        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");
        GarminConnect gc(nullptr, &fake, cfg.path());
        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("9"));

        QVERIFY2(!ok, "RateLimited must fail fast (readFile returns false)");
        QCOMPARE(fake.downloadCalls, 1); // exactly one — NO TCX retry
        QCOMPARE(fake.downloadFmts, (QStringList{QStringLiteral("ORIGINAL")}));
        // Nothing staged.
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
    }

    // TEST-023 — content-sniff backstop: ORIGINAL returns 200 but the bytes are
    // NOT FIT → readFile retries as TCX. Cases: a ZIP wrapping a .tcx entry;
    // empty bytes; an HTML error page.
    void nonFitTwoHundredRetriesTcx_data()
    {
        QTest::addColumn<QByteArray>("originalBytes");
        QTest::newRow("zip-wrapping-tcx")
            << makeZip(QStringLiteral("act.tcx"), QByteArray("<TrainingCenterDatabase/>"));
        QTest::newRow("empty-bytes") << QByteArray();
        QTest::newRow("html-error-page") << QByteArray("<!DOCTYPE html><html><body>429 or error</body></html>");
    }

    void nonFitTwoHundredRetriesTcx()
    {
        QFETCH(QByteArray, originalBytes);
        FakeDownloadClient fake;
        fake.responses[QStringLiteral("ORIGINAL")] = {true, originalBytes, {}};
        fake.responses[QStringLiteral("TCX")] = {true, kTcxBytes, {}};

        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");
        GarminConnect gc(nullptr, &fake, cfg.path());
        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("55"));

        QVERIFY2(ok, "a non-FIT 200 must fall back to TCX and return true");
        QCOMPARE(data, kTcxBytes);
        QCOMPARE(fake.downloadFmts, (QStringList{QStringLiteral("ORIGINAL"), QStringLiteral("TCX")}));
        QTRY_COMPARE(static_cast<CloudService&>(gc).lastReadName, QStringLiteral("garmin-55.tcx"));
    }

    // TEST-024 — B-R007-01 / REQ-NF-Perf-003 regression: the completion
    // (readComplete) MUST be delivered as a QUEUED/DEFERRED self-post, NOT
    // synchronously inside readFile()'s own call frame.
    //
    // Why this matters (ground truth): the CloudService auto-download caller does
    //   QEventLoop loop; connect(provider, readComplete, &loop, quit);
    //   QTimer::singleShot(30000, &loop, quit);
    //   provider->readFile(data, name, id);
    //   loop.exec();
    // If readFile() emits readComplete BEFORE loop.exec() runs, quit() on a
    // not-yet-running loop is a NO-OP, so the caller blocks the full 30s watchdog
    // per activity (3 × 30s = 90s, breaching the ≤5s NF-Perf-003 budget). The fix
    // is to defer the emit so the caller's event loop observes it promptly.
    //
    // The stub CloudService is not a QObject and exposes no readComplete signal;
    // the notifyReadComplete recorder (readCompleteCount / lastReadName) is the
    // faithful in-stub equivalent of "the caller's slot fired". So this mirrors the
    // caller's ordering with the recorder: assert NOT delivered during the
    // readFile() call frame, then delivered once an event loop turns — with a SHORT
    // safety timeout (encodes "prompt", keeps the suite fast). Covers BOTH emit
    // paths (FIT success + TCX fallback) and asserts exactly-once delivery.
    void completionIsQueuedNotSynchronous_data()
    {
        QTest::addColumn<bool>("fitPath");
        QTest::addColumn<QString>("expectedName");
        // FIT success path — ORIGINAL returns a ZIP-wrapped FIT, staged .fit.
        QTest::newRow("fit-success") << true << QStringLiteral("garmin-321.fit");
        // TCX fallback path — ORIGINAL fails Network, retry TCX, staged .tcx.
        QTest::newRow("tcx-fallback") << false << QStringLiteral("garmin-321.tcx");
    }

    void completionIsQueuedNotSynchronous()
    {
        QFETCH(bool, fitPath);
        QFETCH(QString, expectedName);

        FakeDownloadClient fake;
        if (fitPath) {
            fake.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("321.fit"), makeFitBytes()), {}};
        } else {
            fake.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::Network};
            fake.responses[QStringLiteral("TCX")] = {true, kTcxBytes, {}};
        }

        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");
        GarminConnect gc(nullptr, &fake, cfg.path());
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("321"));
        QVERIFY2(ok, "readFile must still return its success bool synchronously");

        // The crux: the completion must NOT have been delivered inside readFile()'s
        // call frame. On the pre-fix (synchronous) code this is already 1 and this
        // assertion FAILS — that is the intended RED.
        QCOMPARE(svc.readCompleteCount, 0);

        // Once an event loop turns, the deferred post is delivered — promptly, so a
        // short safety timeout (1s, NOT the caller's 30s watchdog) is ample.
        QTRY_COMPARE_WITH_TIMEOUT(svc.readCompleteCount, 1, 1000);
        QCOMPARE(svc.lastReadName, expectedName);

        // Exactly-once: a deferred post must not double-emit. Pump the loop again
        // and confirm the count is still 1.
        QTest::qWait(50);
        QCOMPARE(svc.readCompleteCount, 1);
    }

    // TEST-025 — A3-R007-01 / use-after-free on destroy: the queued completion
    // post MUST be bound to the GarminConnect's OWN lifetime, so that if the
    // GarminConnect is destroyed while a post is still pending — with its
    // injected, unowned download client OUTLIVING it — the post is CANCELLED
    // rather than dispatched into the freed/destructed GarminConnect.
    //
    // Ground truth of the defect: postReadComplete() queues the lambda through a
    // context QObject. If that context is `m_client` (the injected client), Qt
    // cancels the pending invoke only when *m_client* is destroyed — NOT when the
    // GarminConnect that the lambda captures (`this`) is destroyed. So a
    // GarminConnect destroyed before its client, with a post in flight, leaves a
    // lambda that will dereference the dead GarminConnect (call notifyReadComplete
    // on it) when the event loop next turns — a use-after-free. The fix binds the
    // post to a bare QObject *member* of GarminConnect, so member destruction
    // (part of ~GarminConnect, before the CloudService base is torn down) cancels
    // the pending post.
    //
    // Observation strategy (cross-build, ASan-clean in the PASS path, no reliance
    // on a sanitizer to catch the defect): GarminConnect is placement-constructed
    // into a caller-owned storage buffer that is NEVER freed for the duration of
    // the test. We end the GarminConnect's *object lifetime* explicitly (call the
    // destructor) while leaving the storage valid, then pump the event loop. The
    // recorder counter (CloudService::readCompleteCount) lives inside that still-
    // valid storage, so we read it through an address captured before destruction:
    //   - AFTER THE FIX: destroying the object destroys its QObject context member
    //     → the pending post is cancelled → the loop dispatches nothing → the
    //     counter stays 0. Nothing ever touches the destructed object → clean.
    //   - BEFORE THE FIX (m_client context): the client (declared first here, so
    //     destroyed last) is still alive, so the post is NOT cancelled → the loop
    //     dispatches the lambda, which calls notifyReadComplete on the destructed
    //     object → the counter becomes 1. QCOMPARE(counter, 0) then FAILS — the
    //     RED that pins the use-after-free.
    void completionContextCancelsPendingPostOnDestroy()
    {
        FakeDownloadClient fake; // declared FIRST → destroyed LAST (outlives gc)
        QByteArray data;         // caller-owned; outlives gc so the captured
                                 // data pointer stays valid when the loop turns
        fake.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("555.fit"), makeFitBytes()), {}};

        // Connected account (DEC-garmin-020 fail-closed) — declared before the
        // storage so it outlives the placement-constructed service.
        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        // Storage the test owns and never frees — only the *object lifetime* ends
        // below, so reads of the recorder counter after destruction stay in valid
        // (allocated, in-scope) memory in the PASS path.
        alignas(GarminConnect) unsigned char storage[sizeof(GarminConnect)];
        GarminConnect* gc = new (storage) GarminConnect(nullptr, &fake, cfg.path());

        // Address of the recorder counter, captured while the object is alive.
        int* readCompleteCount = &static_cast<CloudService*>(gc)->readCompleteCount;

        const bool ok = gc->readFile(&data, QStringLiteral("ignored-name"), QStringLiteral("555"));
        QVERIFY2(ok, "readFile must still return true synchronously");
        // The post is QUEUED, not yet delivered — nothing recorded yet.
        QCOMPARE(*readCompleteCount, 0);

        // End the GarminConnect's lifetime with the post still pending. The fix's
        // context member is destroyed here → the pending post must be cancelled.
        gc->~GarminConnect();

        // Pump the event loop. Pre-fix: the m_client-bound post fires into the
        // destructed object → *readCompleteCount becomes 1. Post-fix: cancelled.
        QTest::qWait(100);

        QCOMPARE(*readCompleteCount, 0); // post cancelled, not dispatched into dead object
    }

    // TEST-026 — A3-R007-03 / readFile null-guard coverage: the early return
    //   if (data == nullptr || m_client == nullptr) return false;
    // has zero test coverage. This characterization test pins BOTH branches:
    // readFile returns false and posts NO completion when (a) the data pointer is
    // null and (b) the injected client seam is null. (With the guard present this
    // passes immediately — it is a characterization/pinning test, not a
    // behaviour-changing one; removing either disjunct makes it fail/crash, which
    // is how it bites. See build NOTES.)
    void readFileNullGuardReturnsFalseNoCompletion()
    {
        // Both cases run against a CONNECTED account, so the DEC-garmin-020
        // fail-closed re-check cannot be what returns false here — only the null
        // guard can be, which is exactly what this slot pins.
        QTemporaryDir cfg;
        QVERIFY(cfg.isValid());
        QVERIFY2(connectAccount(cfg.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        // (a) null data pointer, valid client.
        {
            FakeDownloadClient fake;
            fake.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("1.fit"), makeFitBytes()), {}};
            GarminConnect gc(nullptr, &fake, cfg.path());
            const bool ok = gc.readFile(nullptr, QStringLiteral("n"), QStringLiteral("1"));
            QVERIFY2(!ok, "null data pointer must return false");
            QCOMPARE(fake.downloadCalls, 0); // no download attempted
            QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
        }
        // (b) null client seam (injected nullptr), valid data.
        {
            GarminConnect gc(nullptr, nullptr, cfg.path());
            QByteArray data;
            const bool ok = gc.readFile(&data, QStringLiteral("n"), QStringLiteral("1"));
            QVERIFY2(!ok, "null client seam must return false");
            QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
        }
    }
};

QTEST_MAIN(TestGarminConnectReadFile)
#include "testGarminConnectReadFile.moc"
