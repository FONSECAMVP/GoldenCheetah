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
// On a successful readFile, GarminConnect records the activity as PENDING in
// backfill-state (DES-010 step 5e, amended by DEC-080/B-STAGE9-111). The
// cursor itself is untouched here — DEC-083 makes it a completeness
// watermark, advanced only by promotion to imported-<uid>.json, a separate
// step tested in T-244/T-245 below.
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
#include <QRegularExpression>
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

// T-211 / DEC-056 — mirrors RideFile::parseRideFileName's own regex exactly
// (RideFile.cpp:2434-2451) rather than linking RideFile.cpp: this target has
// no QtWidgets (RideFile.h drags in GoldenCheetah.h's QMenu) and pulling that
// in for one gate check is out of this unit's scope. Kept byte-for-byte
// identical to the real pattern so a divergence there is caught by re-copying,
// not silently missed.
bool parseRideFileNameLikeTheRealGate(const QString& name, QDateTime* dt)
{
    static const QRegularExpression rx(QStringLiteral("^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)"
                                                      "_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$"));
    const QRegularExpressionMatch m = rx.match(name);
    if (!m.hasMatch())
        return false;
    const QDate date(m.captured(2).toInt(), m.captured(3).toInt(), m.captured(4).toInt());
    const QTime time(m.captured(5).toInt(), m.captured(6).toInt(), m.captured(7).toInt());
    if (!date.isValid() || !time.isValid())
        return false;
    *dt = QDateTime(date, time);
    return true;
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
    bool neverReply = false; // B-STAGE9-19: emit nothing — force blockingList()'s timeout
    QHash<QString, QByteArray> originalBytesById;

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        if (neverReply)
            return; // no emission at all — the caller's timeout is the only exit
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

// B-STAGE9-19 — the production mapping switch is on raw int and this build
// sets no -W flags, so the test TU re-arms -Wswitch over the enum itself:
// a Kind appended without a case here is a compile ERROR, forcing the table
// below to grow with the enum.
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
bool listKindRowExists(GarminListFailure::Kind kind)
{
    switch (kind) {
    case GarminListFailure::Network:
    case GarminListFailure::RateLimit:
    case GarminListFailure::Unknown:
    case GarminListFailure::NoClient:
    case GarminListFailure::Timeout:
        return true;
    }
    return false;
}
#pragma GCC diagnostic pop

class TestGarminConnectSync : public QObject
{
    Q_OBJECT

  private slots:

    // =====================================================================
    // T-047 — readdir listing + since-resolution + threading + concurrent guard
    // =====================================================================

    // readdir drives the worker list op (through the REAL chain, off the caller
    // thread) and returns one CloudServiceEntry per activity with
    // remoteid==activityId, the yyyy_MM_dd_HH_mm_ss name derived from
    // startTimeLocal (DEC-056), and the startTimeGMT carried as the entry
    // timestamp (DES-010 step 4).
    void readdirBuildsOneEntryPerActivityCarryingIdAndTimestampOffCallerThread()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        // DEC-056 — named from startTimeLocal (the activity's OWN local start
        // time), never the machine's current timezone: startTimeLocal is set
        // explicitly here so the expected name is deterministic regardless of
        // the timezone the test happens to run in.
        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("1001");
        a1.startTimeGMT = QStringLiteral("2026-07-10 08:30:00");
        a1.startTimeLocal = QStringLiteral("2026-07-10 10:30:00");
        GarminActivitySummary a2;
        a2.activityId = QStringLiteral("1002");
        a2.startTimeGMT = QStringLiteral("2026-07-11 18:05:11");
        a2.startTimeLocal = QStringLiteral("2026-07-11 20:05:11");
        adapter.scriptedListOutcome.activities = {a1, a2};

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 2);

        QCOMPARE(entries.at(0)->id, QStringLiteral("1001"));
        QCOMPARE(entries.at(0)->name, QStringLiteral("2026_07_10_10_30_00.fit"));
        QCOMPARE(entries.at(1)->id, QStringLiteral("1002"));
        QCOMPARE(entries.at(1)->name, QStringLiteral("2026_07_11_20_05_11.fit"));

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

    // T-211 / DEC-056 — the emitted name is not just SOME string: it must
    // actually satisfy RideFile::parseRideFileName (the real gate CloudService
    // enumeration loops apply, RideFile.cpp:2434-2451) and round-trip back to
    // the exact local datetime startTimeLocal encoded.
    void readdirEmittedNameSatisfiesParseRideFileNameAndRoundTripsLocalDatetime()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("4001");
        a1.startTimeGMT = QStringLiteral("2026-08-02 04:00:00");
        a1.startTimeLocal = QStringLiteral("2026-08-02 06:00:00");
        adapter.scriptedListOutcome.activities = {a1};

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);

        QDateTime parsed;
        QVERIFY2(parseRideFileNameLikeTheRealGate(entries.at(0)->name, &parsed),
                 qPrintable(QStringLiteral("emitted name %1 must satisfy parseRideFileName's "
                                           "leading yyyy_MM_dd_HH_mm_ss gate")
                                .arg(entries.at(0)->name)));
        QCOMPARE(parsed, QDateTime(QDate(2026, 8, 2), QTime(6, 0, 0)));
    }

    // T-211 / DEC-056 — when startTimeLocal is absent (the OPTIONAL wheel
    // field), the name falls back to startTimeGMT converted to local time,
    // never a raise and never the discarded garmin-<id>.fit shape.
    void readdirNameFallsBackToGmtConvertedToLocalWhenStartTimeLocalIsMissing()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Success;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-15 12:00:00");
        // startTimeLocal deliberately left default-constructed (empty).
        adapter.scriptedListOutcome.activities = {a1};

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);

        QDateTime expectedGmt = QDateTime::fromString(a1.startTimeGMT, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        expectedGmt.setTimeSpec(Qt::UTC);
        const QString expectedName =
            expectedGmt.toLocalTime().toString(QStringLiteral("yyyy_MM_dd_HH_mm_ss")) + QStringLiteral(".fit");
        QCOMPARE(entries.at(0)->name, expectedName);

        QDateTime parsed;
        QVERIFY2(parseRideFileNameLikeTheRealGate(entries.at(0)->name, &parsed),
                 "the GMT-fallback name must still satisfy parseRideFileName");
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

    // =====================================================================
    // B-STAGE9-19 — every failure exit of the list path carries its own
    // GC-stable error_code; only a genuine adapter-reported Unknown may still
    // say "unknown", and its raw library message reaches its own trace line.
    // =====================================================================

    // The `!client` exit of blockingList() is unreachable through readdir()
    // (readdir pre-guards a null client and emits no_session), so the distinct
    // kind is asserted at the blockingList + vocabulary level.
    void blockingListNullClientEmitsNoClientKindAndCode()
    {
        // 4-arg form: the 1-arg and defaulted 5-arg ctors would be ambiguous.
        GarminConnect gc(nullptr, nullptr, QString(), QString());

        auto res = gc.blockingList(QStringLiteral("2026-09-09 05:21:24"));
        QVERIFY2(!res.ok, "a null client must fail the list op");
        QVERIFY2(res.failureKind == static_cast<int>(GarminListFailure::NoClient),
                 qPrintable(QStringLiteral("expected failureKind NoClient(%1), got %2")
                                .arg(static_cast<int>(GarminListFailure::NoClient))
                                .arg(res.failureKind)));
        QVERIFY2(QStringLiteral("no_client") == QLatin1String(GarminConnect::garminListKindCode(res.failureKind)),
                 qPrintable(QStringLiteral("expected code no_client for kind %1, got %2")
                                .arg(res.failureKind)
                                .arg(QLatin1String(GarminConnect::garminListKindCode(res.failureKind)))));
    }

    // A client that never replies must hit the (test-shortened) timeout and
    // emit the distinct timeout code — not the :411 Unknown initialiser — with
    // no detail line (a timeout carries no library message).
    void blockingListTimeoutEmitsDistinctTimeoutCodeAndNoDetailLine()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeSyncClient client;
        client.neverReply = true;
        GarminConnect gc(nullptr, &client, tmp.path(), kUid, /*listTimeoutOverrideMs*/ 50);

        ObsCapture capture;
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);

        const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=sync_incremental outcome="));
        QVERIFY2(trace.size() == 1,
                 qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. Captured: [%2]")
                                .arg(trace.size())
                                .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        QVERIFY2(trace.first().contains(QStringLiteral("outcome=fail error_code=")),
                 qPrintable(QStringLiteral("expected exact field outcome=fail in: %1").arg(trace.first())));
        QVERIFY2(trace.first().contains(QStringLiteral("error_code=timeout duration_ms=")),
                 qPrintable(QStringLiteral("expected exact field error_code=timeout in: %1, got unknown fold?")
                                .arg(trace.first())));
        QVERIFY2(
            trace.first().endsWith(QStringLiteral(" activity_count=0")),
            qPrintable(
                QStringLiteral("expected line to END with exact field activity_count=0, got: %1").arg(trace.first())));

        QVERIFY2(capture.snapshot().filter(QStringLiteral("gc_obs_raw")).isEmpty(),
                 qPrintable(QStringLiteral("a timeout carries no library message, so no detail line may fire. "
                                           "Captured: [%1]")
                                .arg(capture.snapshot().join(QStringLiteral(" | ")))));
    }

    // The genuine adapter-reported failure keeps its own code ("unknown" for a
    // real Unknown; rate_limit/network are pinned by the existing cases above)
    // AND its untranslated library message reaches a SEPARATE detail line —
    // never the parsed gc_obs record's error_code field.
    void listFailureRawMessageReachesItsOwnDetailLineNotErrorCode()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Unknown;
        // The exact message attempt #5's fold would have surfaced (the wheel's
        // startdate validation, B-STAGE9-19 round-1 report).
        adapter.scriptedListOutcome.rawMessage =
            QStringLiteral("startdate must be in format 'YYYY-MM-DD', got: 2026-09-09 05:21:24");

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        ObsCapture capture;
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);

        const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=sync_incremental outcome="));
        QVERIFY2(trace.size() == 1,
                 qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. Captured: [%2]")
                                .arg(trace.size())
                                .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        // the genuine-unknown contract did not change (anchored per B-STAGE9-14)
        QVERIFY2(trace.first().contains(QStringLiteral("error_code=unknown duration_ms=")),
                 qPrintable(QStringLiteral("expected exact field error_code=unknown in: %1").arg(trace.first())));
        QVERIFY2(
            trace.first().endsWith(QStringLiteral(" activity_count=0")),
            qPrintable(
                QStringLiteral("expected line to END with exact field activity_count=0, got: %1").arg(trace.first())));

        // ...and the message must NOT be folded into the parsed record
        QVERIFY2(
            !trace.first().contains(QStringLiteral("YYYY-MM-DD")),
            qPrintable(
                QStringLiteral("error_code must stay a vocabulary token; message leaked into: %1").arg(trace.first())));

        const QStringList detail = capture.snapshot().filter(QStringLiteral("gc_obs_raw op=sync_incremental detail="));
        QVERIFY2(detail.size() == 1,
                 qPrintable(QStringLiteral("expected exactly ONE gc_obs_raw detail line, got %1. Captured: [%2]")
                                .arg(detail.size())
                                .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        QVERIFY2(
            detail.first().contains(QStringLiteral("startdate must be in format")),
            qPrintable(QStringLiteral("detail line must carry the library message verbatim: %1").arg(detail.first())));
    }

    // The detail line is ONE bounded line: embedded newlines are flattened (a
    // raw newline would forge log records) and the message is truncated.
    void listFailureRawMessageDetailIsSanitisedToOneBoundedLine()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        QString longMessage(500, QChar('x'));
        longMessage.insert(0, QStringLiteral("line1\nline2\tend "));
        longMessage += QStringLiteral("tail");

        FakeListPyAdapter adapter;
        adapter.scriptedListOutcome.kind = PyListOutcome::Unknown;
        adapter.scriptedListOutcome.rawMessage = longMessage;

        GarminDownloadChain chain(&adapter);
        GarminConnect gc(nullptr, chain.client(), tmp.path(), kUid);

        ObsCapture capture;
        QStringList errors;
        gc.readdir(QString(), errors, QDateTime(), QDateTime());

        const QStringList detail = capture.snapshot().filter(QStringLiteral("gc_obs_raw op=sync_incremental detail="));
        QVERIFY2(detail.size() == 1,
                 qPrintable(QStringLiteral("a newline in the message must not forge extra records; got %1 lines: [%2]")
                                .arg(detail.size())
                                .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        QVERIFY2(detail.first().size() <= 250, // truncation bound + fixed prefix
                 qPrintable(QStringLiteral("detail line must be bounded, got %1 chars").arg(detail.first().size())));
    }

    // No -Wswitch guards the production mapping (switch on raw int, no warning
    // flags — B-STAGE9-19 round 1), so this table pins every declared Kind's
    // code, and listKindRowExists() makes a new Kind a compile error until this
    // table grows with it. The individual codes are separately pinned by the
    // round-1 tests above and the existing network/rate_limit cases.
    void garminListKindCodePinsEveryDeclaredKindAndFallback()
    {
        const struct
        {
            GarminListFailure::Kind kind;
            const char* code;
        } rows[] = {
            {GarminListFailure::Network, "network"}, {GarminListFailure::RateLimit, "rate_limit"},
            {GarminListFailure::Unknown, "unknown"}, {GarminListFailure::NoClient, "no_client"},
            {GarminListFailure::Timeout, "timeout"},
        };
        for (const auto& row : rows) {
            QVERIFY2(listKindRowExists(row.kind), "table row out of step with the exhaustiveness switch");
            QVERIFY2(QLatin1String(GarminConnect::garminListKindCode(static_cast<int>(row.kind))) ==
                         QLatin1String(row.code),
                     qPrintable(QStringLiteral("kind %1 maps to %2, expected %3")
                                    .arg(static_cast<int>(row.kind))
                                    .arg(QLatin1String(GarminConnect::garminListKindCode(static_cast<int>(row.kind))))
                                    .arg(row.code)));
        }
        // An out-of-range raw int keeps the deliberate switch-on-int fallback.
        QCOMPARE(QLatin1String(GarminConnect::garminListKindCode(9999)), QLatin1String("unknown"));
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
        aBBB.startTimeLocal = QStringLiteral("2026-07-05 11:15:00"); // DEC-056 — deterministic name, tz-independent
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
        QCOMPARE(entries.at(0)->name, QStringLiteral("2026_07_05_11_15_00.fit"));

        // The base machinery now downloads BBB via the existing readFile (REQ-007).
        QByteArray data;
        const bool ok = gc.readFile(&data, entries.at(0)->name, entries.at(0)->id);
        QVERIFY2(ok, "readFile must stage the fresh activity");
        QCOMPARE(data, makeFitBytes());

        // DEC-080, B-STAGE9-111 — the download is recorded PENDING, not imported.
        GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.isOk());
        QVERIFY2(!imported.contains(QStringLiteral("BBB")), "T-048: an unpromoted download must not read as imported");
        GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.pending.contains(QStringLiteral("BBB")), "T-048: the fresh download must be pending");
        QCOMPARE(bf.state.pending.value(QStringLiteral("BBB")).startTimeGMT, QStringLiteral("2026-07-05 09:15:00"));
        QCOMPARE(bf.state.pending.value(QStringLiteral("BBB")).localFilename, QStringLiteral("garmin-BBB.fit"));
        // The pre-existing AAA record is preserved (read-modify-write merge).
        QVERIFY(imported.contains(QStringLiteral("AAA")));

        // DEC-083 (B-STAGE9-133): the cursor is a completeness watermark now
        // advanced ONLY by promotion, so this download-time write leaves it
        // untouched (no prior success existed, so it stays empty).
        QVERIFY2(bf.state.lastSuccessStartTimeGMT.isEmpty(), "T-048: download time must not advance the cursor");

        // DEC-080, B-STAGE9-111 — no registration consumer on this route (no
        // rideRegistrationCompleted caller exists here), so BBB is never
        // promoted and is RE-OFFERED on the next listing.
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(entries2.size(), 1);
        QCOMPARE(entries2.at(0)->id, QStringLiteral("BBB"));
    }

    // =====================================================================
    // T-244 / T-245 — DEC-080, B-STAGE9-111: promotion round-trip and
    // abandonment via GarminConnect::rideRegistrationCompleted().
    // =====================================================================

    // T-244 (a) — a successful registration promotes.
    void rideRegistrationCompletedPromotesPendingToImported()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("PPP");
        a1.startTimeGMT = QStringLiteral("2026-08-10 00:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("PPP")] = makeZip(QStringLiteral("PPP.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 1);

        QByteArray data;
        QVERIFY(gc.readFile(&data, entries.at(0)->name, entries.at(0)->id));

        GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.pending.contains(QStringLiteral("PPP")), "T-244: readFile must record PPP pending");

        gc.rideRegistrationCompleted(QStringLiteral("garmin-PPP.fit"));

        bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(!bf.state.pending.contains(QStringLiteral("PPP")), "T-244: promotion must drop the pending entry");

        GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.isOk());
        QVERIFY2(imported.contains(QStringLiteral("PPP")), "T-244: promotion must record the completion");
        QCOMPARE(imported.value(QStringLiteral("PPP")).startTimeGMT, QStringLiteral("2026-08-10 00:00:00"));
        QCOMPARE(imported.value(QStringLiteral("PPP")).localFilename, QStringLiteral("garmin-PPP.fit"));

        // T-254 — DEC-083 clause 2, promotion caller #1
        // (GarminConnect::rideRegistrationCompleted): the cursor must advance
        // to the promoted entry's own startTimeGMT.
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, QStringLiteral("2026-08-10 00:00:00"));

        // T-243 side effect: no second download for an id already in `imported`.
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(entries2.size(), 0);
    }

    // T-244 (b) — B-STAGE9-161: DEC-083 clause 1 makes download time write a
    // pending row unconditionally, on v0 exactly as on v1; DEC-087 b1 is what
    // keeps the resulting file's schema_version at 0.
    void v0BackfillStateRecordsPendingAndPreservesSchemaVersion()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(tmp.path())));
        const QString v0Path = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid);
        const QByteArray v0Json = QByteArray(
            "{\"last_success_startTimeGMT\":\"2026-07-01 00:00:00\",\"range_start\":\"\",\"range_end\":\"\"}");
        {
            QFile f(v0Path);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(f.write(v0Json), qint64(v0Json.size()));
        }
        QVERIFY(QFile::setPermissions(v0Path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("V0P");
        a1.startTimeGMT = QStringLiteral("2026-08-01 00:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("V0P")] = makeZip(QStringLiteral("V0P.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 1);

        QByteArray data;
        QVERIFY2(gc.readFile(&data, entries.at(0)->name, entries.at(0)->id),
                 "T-244(b): readFile against v0 must succeed");

        const GarminSidecarStore::BackfillLoadResult after = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(after.isOk());
        QCOMPARE(after.state.schemaVersion, 0);
        QVERIFY2(after.state.pending.contains(QStringLiteral("V0P")), "T-244(b): readFile must record V0P pending");
    }

    // T-247 — B-STAGE9-161: a v0 athlete's download-time write is pending-only,
    // same as v1; `imported` gains the entry only through promotion.
    void v0ReadFileRecordsPendingNotImported()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(tmp.path())));
        const QString v0Path = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid);
        const QByteArray v0Json = QByteArray(
            "{\"last_success_startTimeGMT\":\"2026-07-01 00:00:00\",\"range_start\":\"\",\"range_end\":\"\"}");
        {
            QFile f(v0Path);
            QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
            QCOMPARE(f.write(v0Json), qint64(v0Json.size()));
        }
        QVERIFY(QFile::setPermissions(v0Path, QFileDevice::ReadOwner | QFileDevice::WriteOwner));

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("V0Q");
        a1.startTimeGMT = QStringLiteral("2026-08-02 00:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("V0Q")] = makeZip(QStringLiteral("V0Q.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 1);

        QByteArray data;
        QVERIFY2(gc.readFile(&data, entries.at(0)->name, entries.at(0)->id), "T-247: readFile against v0 must succeed");

        const GarminSidecarStore::BackfillLoadResult after = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(after.isOk());
        QCOMPARE(after.state.schemaVersion, 0);
        QVERIFY2(after.state.pending.contains(QStringLiteral("V0Q")), "T-247: v0 must record pending at download time");

        GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY2(!imported.isOk() || !imported.contains(QStringLiteral("V0Q")),
                 "T-247: download time must not write imported directly, even on v0");
    }

    // T-245 — abandonment: readFile alone must never self-promote. Only an
    // explicit rideRegistrationCompleted() call (the registration-succeeded
    // signal) may move an entry out of pending; withholding it (an abandoned
    // or failed registration) must leave `imported` and `pending` exactly as
    // readFile left them.
    void abandonedRegistrationLeavesEntryPendingNotImported()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY2(connectAccount(tmp.path()), "pre-condition: the account must be connected (DEC-garmin-020)");

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("QQQ");
        a1.startTimeGMT = QStringLiteral("2026-08-11 00:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("QQQ")] = makeZip(QStringLiteral("QQQ.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path(), kUid);
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 1);

        QByteArray data;
        QVERIFY(gc.readFile(&data, entries.at(0)->name, entries.at(0)->id));

        // No rideRegistrationCompleted() call here — the registration is
        // treated as abandoned/failed.
        GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY2(!imported.isOk() || !imported.contains(QStringLiteral("QQQ")),
                 "T-245: an abandoned registration must not promote");
        GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.pending.contains(QStringLiteral("QQQ")), "T-245: the entry must remain pending");

        // Tier-1 dedup consults `imported` only, so it is RE-OFFERED, not lost.
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(entries2.size(), 1);
        QCOMPARE(entries2.at(0)->id, QStringLiteral("QQQ"));

        // A call naming a DIFFERENT staged file must not disturb QQQ either.
        gc.rideRegistrationCompleted(QStringLiteral("garmin-ZZZ.fit"));
        bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.pending.contains(QStringLiteral("QQQ")), "T-245: an unrelated id must not promote QQQ");
    }
};

QTEST_MAIN(TestGarminConnectSync)
#include "testGarminConnectSync.moc"
