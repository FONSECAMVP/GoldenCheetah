/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-047/T-048 — REQ-008 Slice C: GarminConnect incremental-sync
// orchestration (DES-010) bound onto GC's CloudService sync machinery.
//
// The base CloudService drives sync by calling readdir(path, errors, from, to)
// to enumerate remote activities, then readFile(data, remotename, remoteid) per
// returned CloudServiceEntry. This slice makes GarminConnect a syncing
// CloudService by overriding readdir to:
//   - resolve the active garmin_user_id + config dir,
//   - determine the "since" timestamp (from, else backfill-state's
//     lastSuccessStartTimeGMT, else now()-7d — DES-010 steps 2/3),
//   - drive the worker list op (Slice A) off the caller thread and build one
//     CloudServiceEntry per activity (remoteid==activityId, name garmin-<id>.fit,
//     startTimeGMT carried as the entry timestamp — DES-010 step 4),
//   - short-circuit any activity already in imported-<uid>.json so the base
//     machinery never issues a redundant readFile (DES-010 step 5a — Tier-1),
//   - reject a concurrent sync while one is in progress (REQ-NF-Perf-002).
// On a successful readFile, GarminConnect records the activity into
// imported-<uid>.json and advances backfill-state (DES-010 steps 5e + 6).
//
// Python-free: T-047 uses the REAL GarminDownloadChain (worker on a dedicated
// thread) + a Python-free fake IGarminPyAdapter (records the thread + sinceGmt)
// to prove the list op runs off the caller thread (REQ-NF-Threads-001) exactly
// as testGarminConnectDownloadWorker does; the concurrent-guard + dedup/record
// slots use same-thread fake IGarminDownloadClients. The REAL (pure-Qt)
// GarminSidecarStore + AtomicFile back the sidecar reads/writes, and real
// contrib/qzip unwraps the FIT ORIGINAL. stubs/ReadFileStubPreamble.h stubs
// CloudService + replaces PyEmbeddedAdapter with a Python-free fake → the target
// stays on the `garmin-fast` label.

#include "GarminConnect.h"
#include "GarminDownloadChain.h"
#include "GarminSidecarStore.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"
#include "zipreader.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHash>
#include <QMetaObject>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

#include <atomic>

namespace {

// A minimal but valid FIT header: ".FIT" ASCII at byte offset 8 (DEC-016).
QByteArray makeFitBytes()
{
    QByteArray b(12, '\0');
    b[0] = 0x0C;
    b[1] = 0x10;
    b[8] = '.';
    b[9] = 'F';
    b[10] = 'I';
    b[11] = 'T';
    b.append("\x00\x01record-payload");
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

const QString kUid = QStringLiteral("123456789");
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

// DEC-garmin-020 (A3-R012-F1): readdir/readFile now FAIL CLOSED unless the
// athlete's stored Garmin credential is present and acceptable AT CALL TIME. The
// slots below all exercise sync for a CONNECTED account, so each fixture writes a
// real tokens.json (+ active-account.json) through the production producer first.
// The ctor uid-override these slots use keys the sidecars exactly as before; this
// only supplies the credential whose absence now (correctly) blocks any network
// work. Verifying the credential re-check itself is TEST-057's job.
bool connectAccount(const QString& athleteConfigDir)
{
    return GarminTokenStore::persistConnectSuccess(athleteConfigDir, kUid, kBlob);
}

// REQ-NF-Obs-001 (T-207) — installs a Qt message handler for its lifetime and
// records every qDebug line, so the structured gc_obs trace can be asserted
// literally (prd.md:112's verification method is log-format review). QtTest
// runs slots sequentially, so a single capture at a time is safe. Same helper
// as testGarminConnectOpen.cpp — kept file-local per this suite's fake style.
// Qt permits a message handler to be invoked concurrently from any thread
// (Qt's own logging docs require handlers to be reentrant), and this suite's
// sync trace slots run a real worker-thread chain while capture is installed
// — so the writer (hook) and the reader (snapshot) serialize on one mutex.
class ObsCapture
{
  public:
    ObsCapture()
    {
        QMutexLocker locker(&mutex_);
        prev_ = qInstallMessageHandler(&ObsCapture::hook);
        current = this;
    }
    ~ObsCapture()
    {
        QMutexLocker locker(&mutex_);
        current = nullptr;
        locker.unlock();
        qInstallMessageHandler(prev_);
    }
    ObsCapture(const ObsCapture&) = delete;
    ObsCapture& operator=(const ObsCapture&) = delete;

    QStringList snapshot() const
    {
        QMutexLocker locker(&mutex_);
        return lines_;
    }

  private:
    static void hook(QtMsgType, const QMessageLogContext&, const QString& msg)
    {
        QMutexLocker locker(&mutex_);
        if (current != nullptr)
            current->lines_ << msg;
    }
    static ObsCapture* current;
    static QMutex mutex_;
    QStringList lines_;
    QtMessageHandler prev_;
};
ObsCapture* ObsCapture::current = nullptr;
QMutex ObsCapture::mutex_;
} // namespace

// ---------------------------------------------------------------------------
// FakeListPyAdapter — Python-free IGarminPyAdapter that records the sinceGmt it
// received + the thread it ran on and returns a scripted PyListOutcome. Mirrors
// FakeDownloadPyAdapter (testGarminConnectDownloadWorker) so, driven through the
// REAL GarminDownloadChain, T-047 can prove listActivitiesSince runs off the
// caller thread (REQ-NF-Threads-001).
// ---------------------------------------------------------------------------
class FakeListPyAdapter : public IGarminPyAdapter
{
  public:
    PyListOutcome scriptedListOutcome;
    QString lastSinceGmt;
    std::atomic<int> listCallCount{0};
    QThread* listThreadSeen = nullptr;

    PyAuthOutcome authenticate(const QString&, const QString&) override { return {}; }
    PyAuthOutcome submitMfa(const QString&) override { return {}; }
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }

    PyListOutcome listActivitiesSince(const QString& sinceGmt) override
    {
        lastSinceGmt = sinceGmt;
        listThreadSeen = QThread::currentThread();
        listCallCount.fetch_add(1);
        return scriptedListOutcome;
    }

    // REQ-013 (DEC-050) seam extension (DEC-013 compile-enforced) — this sync
    // test never fetches a profile; a default outcome satisfies the interface.
    PyProfileOutcome fetchProfile() override { return {}; }
};

// ---------------------------------------------------------------------------
// FakeSyncClient — same-thread Python-free IGarminDownloadClient. Scripts the
// list result (or a listFailed) and per-activity ORIGINAL bytes so the dedup +
// record path (T-048) is exercisable without the worker thread. Emissions are
// queued so they arrive while GarminConnect's blocking QEventLoop runs.
// ---------------------------------------------------------------------------
class FakeSyncClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    QVector<GarminActivitySummary> listResult;
    bool listOk = true;
    GarminListFailure::Kind listKind = GarminListFailure::Network;
    QHash<QString, QByteArray> originalBytesById;

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        const bool ok = listOk;
        const GarminListFailure::Kind kind = listKind;
        const QVector<GarminActivitySummary> res = listResult;
        QMetaObject::invokeMethod(
            this,
            [this, id, ok, kind, res]() {
                if (ok) {
                    emit activitiesListed(id, res);
                } else {
                    GarminListFailure e;
                    e.kind = kind;
                    e.rawMessage = QStringLiteral("scripted list failure");
                    emit listFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }

    void downloadActivity(const QString& activityId, const QString& fmt, QUuid id) override
    {
        const QByteArray bytes = originalBytesById.value(activityId);
        QMetaObject::invokeMethod(
            this,
            [this, id, fmt, bytes]() {
                if (fmt == QStringLiteral("ORIGINAL") && !bytes.isEmpty()) {
                    emit downloaded(id, bytes);
                } else {
                    GarminDownloadFailure e;
                    e.kind = GarminDownloadFailure::Unknown;
                    emit downloadFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }
};

// ---------------------------------------------------------------------------
// ReentrantListClient — same-thread fake whose listActivities(), before it
// releases the first readdir's blocking loop, re-enters gc->readdir() ONCE. This
// second call runs while the first sync is still in progress, so it must be
// rejected (REQ-NF-Perf-002) — the guard is the FIRST thing readdir checks.
// ---------------------------------------------------------------------------
class ReentrantListClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    GarminConnect* gc = nullptr;
    QVector<GarminActivitySummary> listResult;

    QStringList reentrantErrors;
    int reentrantEntryCount = -1;
    bool reentrantDone = false;

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }
    void downloadActivity(const QString&, const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(
            this, [this, id]() { emit downloadFailed(id, GarminDownloadFailure{}); }, Qt::QueuedConnection);
    }
    void listActivities(const QString&, QUuid id) override
    {
        const QVector<GarminActivitySummary> res = listResult;
        QMetaObject::invokeMethod(
            this,
            [this, id, res]() {
                // Re-entrant sync while the first readdir is still in progress.
                QStringList errs;
                QList<CloudServiceEntry*> r = gc->readdir(QString(), errs, QDateTime(), QDateTime());
                reentrantErrors = errs;
                reentrantEntryCount = r.size();
                reentrantDone = true;
                // Release the first readdir's blocking loop.
                emit activitiesListed(id, res);
            },
            Qt::QueuedConnection);
    }
};

class TestGarminConnectSync : public QObject
{
    Q_OBJECT

  private slots:

    // =====================================================================
    // T-047 — readdir listing + since-resolution + threading + concurrent guard
    // =====================================================================

    // readdir drives the worker list op (through the REAL chain, off the caller
    // thread) and returns one CloudServiceEntry per activity with
    // remoteid==activityId, the natural garmin-<id>.fit name, and the
    // startTimeGMT carried as the entry timestamp (DES-010 step 4).
    void readdirBuildsOneEntryPerActivityCarryingIdAndTimestampOffCallerThread()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("1001");
        a1.startTimeGMT = QStringLiteral("2026-07-10 08:30:00");
        GarminActivitySummary a2;
        a2.activityId = QStringLiteral("1002");
        a2.startTimeGMT = QStringLiteral("2026-07-11 18:05:11");
        adapter.scriptedListOutcome.activities = {a1, a2};

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 2);

        QCOMPARE(entries.at(0)->id, QStringLiteral("1001"));
        QCOMPARE(entries.at(0)->name, QStringLiteral("garmin-1001.fit"));
        QCOMPARE(entries.at(1)->id, QStringLiteral("1002"));
        QCOMPARE(entries.at(1)->name, QStringLiteral("garmin-1002.fit"));

        // startTimeGMT carried as the entry's timestamp (Garmin's server-side
        // time, parsed to a UTC QDateTime).
        QDateTime expected0 = QDateTime::fromString(a1.startTimeGMT, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        expected0.setTimeSpec(Qt::UTC);
        QCOMPARE(entries.at(0)->modified, expected0);

        // REQ-NF-Threads-001: the adapter list call ran on the worker thread, not
        // the caller/test thread.
        QCOMPARE(int(adapter.listCallCount.load()), 1);
        QVERIFY2(adapter.listThreadSeen != nullptr && adapter.listThreadSeen != QThread::currentThread(),
                 "listActivitiesSince must run off the caller thread (REQ-NF-Threads-001)");
    }

    // When `from` is null, the "since" timestamp comes from backfill-state's
    // lastSuccessStartTimeGMT (DES-010 step 3).
    void sinceComesFromBackfillStateWhenFromIsNull()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-12 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(adapter.lastSinceGmt, QStringLiteral("2026-07-12 00:00:00"));
    }

    // When no backfill-state exists and `from` is null, the "since" defaults to
    // now()-7d (DES-010 step 3).
    void sinceDefaultsToSevenDaysAgoWhenNoBackfillState()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QDateTime seen = QDateTime::fromString(adapter.lastSinceGmt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        seen.setTimeSpec(Qt::UTC);
        QVERIFY2(seen.isValid(), "the default since must be a parseable server-time string");
        const QDateTime expected = QDateTime::currentDateTimeUtc().addDays(-7);
        QVERIFY2(qAbs(seen.secsTo(expected)) < 300, "the default since must be ~now()-7d (DES-010 step 3)");
    }

    // A passed `from` overrides both backfill-state and the default (DES-010).
    void passedFromOverridesBackfillAndDefault()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-12 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QDateTime from(QDate(2026, 1, 2), QTime(3, 4, 5), Qt::UTC);
        gc.readdir(QString(), errors, from, QDateTime());

        QCOMPARE(adapter.lastSinceGmt, QStringLiteral("2026-01-02 03:04:05"));
    }

    // A listFailed outcome surfaces via the errors out-param and yields an empty
    // list — never a crash.
    void listFailureSurfacesViaErrorsAndReturnsEmptyList()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Network;
        adapter.scriptedListOutcome.rawMessage = QStringLiteral("connection refused");

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(entries.size(), 0);
        QVERIFY2(!errors.isEmpty(), "a listFailed outcome must surface via the errors out-param");
    }

    // REQ-NF-Obs-001 (T-207) — DES-008's structured qDebug developer-trace
    // (design.md:786: "Structured qDebug mirrors the same fields"): readdir()
    // (op "sync_incremental") emits ONE parseable gc_obs line per call —
    // outcome, error_code (empty on success, the GC-stable kind on failure,
    // the stable guard label on local rejections), duration_ms, and
    // activity_count (entries returned on success, 0 on failure).
    //
    // B-STAGE9-14 — EXACT-FIELD ANCHORING. A bare contains("k=v") is a PREFIX
    // match: an implementation emitting k=v_suffix satisfies it, so the
    // assertion cannot fail and is worse than no assertion. gcObsTrace()
    // (src/Cloud/GarminConnect.cpp) emits a fixed field order, verified by
    // capturing a real line rather than by reading the format string:
    //   gc_obs op=<op> outcome=<ok|fail> error_code=<code> duration_ms=<ms>[ activity_count=<n>]
    // Two anchoring rules follow from that shape:
    //   * a field with a field AFTER it is matched together with the next
    //     key, e.g. "error_code=rate_limit duration_ms=" — the following
    //     " <key>=" is the delimiter that pins the value's end.
    //   * activity_count is LAST when present, so it has no following key to
    //     anchor against and uses endsWith(" activity_count=<n>") instead.
    //     The captured QString was checked byte-wise (cat -A) and ends exactly
    //     at the digit — Qt hands the message handler no trailing newline or
    //     space. If a field is ever appended after activity_count, these
    //     endsWith assertions SHOULD break loudly; that is intended.
    // The op field is pinned by the snapshot().filter() selector carrying its
    // own " outcome=" delimiter, so a wrong op name yields zero matched lines
    // rather than silently validating a different op's trace.
    void readdirEmitsStructuredObsTraceOnSuccessFailureAndGuard()
    {
        // (a) success — activity_count carries the returned entry count.
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

            FakeListPyAdapter adapter;
            adapter.scriptedListOutcome.kind = PyListOutcome::Success;
            GarminActivitySummary a1;
            a1.activityId = QStringLiteral("2001");
            a1.startTimeGMT = QStringLiteral("2026-07-10 08:30:00");
            GarminActivitySummary a2;
            a2.activityId = QStringLiteral("2002");
            a2.startTimeGMT = QStringLiteral("2026-07-11 18:05:11");
            adapter.scriptedListOutcome.activities = {a1, a2};

            GarminDownloadChain chain(&adapter);
            GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

            ObsCapture capture;
            QStringList errors;
            QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
            QCOMPARE(entries.size(), 2);

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=sync_incremental outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            QVERIFY2(trace.first().contains(QStringLiteral("outcome=ok error_code=")),
                     qPrintable(QStringLiteral("expected exact field outcome=ok in: %1").arg(trace.first())));
            QVERIFY2(trace.first().contains(QStringLiteral("error_code= ")),
                     qPrintable(QStringLiteral("expected empty error_code in: %1").arg(trace.first())));
            QVERIFY2(trace.first().endsWith(QStringLiteral(" activity_count=2")),
                     qPrintable(QStringLiteral("expected line to END with exact field activity_count=2, got: %1")
                                    .arg(trace.first())));
            QVERIFY2(trace.first().contains(QStringLiteral("duration_ms=")),
                     qPrintable(QStringLiteral("expected duration_ms=<n> in: %1").arg(trace.first())));
        }

        // (b) list failure — the real GarminListFailure::Kind survives into the
        // trace's error_code (RateLimited translates to rate_limit).
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

            FakeListPyAdapter adapter;
            adapter.scriptedListOutcome.kind = PyListOutcome::RateLimited;

            GarminDownloadChain chain(&adapter);
            GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

            ObsCapture capture;
            QStringList errors;
            QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
            QCOMPARE(entries.size(), 0);

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=sync_incremental outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            QVERIFY2(trace.first().contains(QStringLiteral("outcome=fail error_code=")),
                     qPrintable(QStringLiteral("expected exact field outcome=fail in: %1").arg(trace.first())));
            QVERIFY2(
                trace.first().contains(QStringLiteral("error_code=rate_limit duration_ms=")),
                qPrintable(QStringLiteral("expected exact field error_code=rate_limit in: %1").arg(trace.first())));
            QVERIFY2(trace.first().endsWith(QStringLiteral(" activity_count=0")),
                     qPrintable(QStringLiteral("expected line to END with exact field activity_count=0, got: %1")
                                    .arg(trace.first())));
        }

        // (c) local guard rejection — no connected account: carries its stable
        // trace label (the DEC-garmin-020 fail-closed path, before any I/O).
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            // deliberately NOT connected: no tokens.json

            FakeSyncClient fake;
            GarminConnect gc(nullptr, &fake, tmp.path(), kUid);

            ObsCapture capture;
            QStringList errors;
            QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
            QCOMPARE(entries.size(), 0);

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=sync_incremental outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            QVERIFY2(trace.first().contains(QStringLiteral("outcome=fail error_code=")),
                     qPrintable(QStringLiteral("expected exact field outcome=fail in: %1").arg(trace.first())));
            QVERIFY2(
                trace.first().contains(QStringLiteral("error_code=no_connected_account duration_ms=")),
                qPrintable(
                    QStringLiteral("expected exact field error_code=no_connected_account in: %1").arg(trace.first())));
            QVERIFY2(trace.first().endsWith(QStringLiteral(" activity_count=0")),
                     qPrintable(QStringLiteral("expected line to END with exact field activity_count=0, got: %1")
                                    .arg(trace.first())));
        }
    }

    // REQ-NF-Perf-002: a second sync/readdir while one is in progress is REJECTED
    // ("sync already in progress"); the running one continues to completion.
    void concurrentReaddirWhileOneInProgressIsRejected()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        ReentrantListClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("2001");
        a1.startTimeGMT = QStringLiteral("2026-07-13 10:00:00");
        client.listResult = {a1};

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);
        client.gc = &gc;

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        // The FIRST (outer) readdir completes normally.
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0)->id, QStringLiteral("2001"));

        // The concurrent (re-entrant) readdir was rejected.
        QVERIFY2(client.reentrantDone, "the re-entrant readdir must have executed");
        QCOMPARE(client.reentrantEntryCount, 0);
        QVERIFY2(!client.reentrantErrors.isEmpty(), "the rejected sync must push an error");
        QVERIFY2(client.reentrantErrors.join(QChar(' ')).contains(QStringLiteral("in progress"), Qt::CaseInsensitive),
                 "the rejection must say a sync is already in progress (REQ-NF-Perf-002)");

        // After the outer readdir returns, the guard is released — a fresh sync is
        // accepted again (not permanently wedged).
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(entries2.size(), 1);
    }

    // =====================================================================
    // T-048 — dedup short-circuit + record on successful download
    // =====================================================================

    void dedupFiltersAlreadyImportedAndRecordsFreshOnDownload()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        // Pre-seed imported-<uid>.json with one already-imported activity (AAA).
        GarminSidecarStore::ImportedEntry seeded;
        seeded.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seeded.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUid, QStringLiteral("AAA"), seeded));

        FakeSyncClient client;
        GarminActivitySummary aAAA;
        aAAA.activityId = QStringLiteral("AAA");
        aAAA.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        GarminActivitySummary aBBB;
        aBBB.activityId = QStringLiteral("BBB");
        aBBB.startTimeGMT = QStringLiteral("2026-07-05 09:15:00");
        client.listResult = {aAAA, aBBB};
        client.originalBytesById[QStringLiteral("BBB")] = makeZip(QStringLiteral("BBB.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);

        // readdir: the already-imported AAA is filtered OUT (Tier-1 short-circuit,
        // no entry → the base machinery never issues a readFile for it); the fresh
        // BBB passes through.
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0)->id, QStringLiteral("BBB"));
        QCOMPARE(entries.at(0)->name, QStringLiteral("garmin-BBB.fit"));

        // The base machinery now downloads BBB via the existing readFile (REQ-007).
        QByteArray data;
        const bool ok = gc.readFile(&data, entries.at(0)->name, entries.at(0)->id);
        QVERIFY2(ok, "readFile must stage the fresh activity");
        QCOMPARE(data, makeFitBytes());

        // The download is recorded into imported-<uid>.json with {startTimeGMT,
        // stagedFilename} (DES-010 step 5e), keyed on Garmin's server-side time.
        GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.isOk());
        QVERIFY2(imported.contains(QStringLiteral("BBB")), "the fresh download must be recorded");
        QCOMPARE(imported.value(QStringLiteral("BBB")).startTimeGMT, QStringLiteral("2026-07-05 09:15:00"));
        QCOMPARE(imported.value(QStringLiteral("BBB")).localFilename, QStringLiteral("garmin-BBB.fit"));
        // The pre-existing AAA record is preserved (read-modify-write merge).
        QVERIFY(imported.contains(QStringLiteral("AAA")));

        // backfill-state's lastSuccessStartTimeGMT advances to BBB's startTimeGMT
        // (DES-010 step 6).
        GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, QStringLiteral("2026-07-05 09:15:00"));

        // Second sync: BBB is now already-imported, so it too is short-circuited —
        // the whole point of the sidecar (no redundant download).
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(entries2.size(), 0);
    }
};

QTEST_MAIN(TestGarminConnectSync)
#include "testGarminConnectSync.moc"
