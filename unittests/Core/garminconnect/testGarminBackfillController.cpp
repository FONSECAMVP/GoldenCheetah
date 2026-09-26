/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-176..T-183 (informal, orchestrator assigns real TEST-NNN) — REQ-010 / DES-009:
// GarminBackfillController, the bulk-backfill orchestration state machine.
// Pure-Qt (no Python.h) - a Python-free IGarminDownloadClient fake scripts
// list/download outcomes, mirroring testGarminConnectSync.cpp's FakeSyncClient.
// The REAL GarminSidecarStore + AtomicFile back the sidecar/FIT-file I/O so
// resumability and torn-write recovery are exercised for real, not mocked.
//
// Covers all four REQ-010 interruption classes: user cancel, hard-crash/resume
// (a fresh controller instance resuming from a prior run's persisted state -
// indistinguishable on disk from a clean pause, by design), transient HTTP
// error (DES-005 retry-exhausted), and torn FIT write (AtomicFile::writeOver
// failure via its injectable TmpWriter test seam).
//
// RED phase: src/Cloud/GarminBackfillController.{h,cpp} do not exist yet - the
// executable's compile fails at the #include line (right-reason RED).

#include "AtomicFile.h"
#include "GarminBackfillController.h"
#include "GarminSidecarStore.h"
#include "IGarminDownloadClient.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

#ifdef Q_CC_MSVC
#    include <QtZlib/zlib.h>
#else
#    include <zlib.h>
#endif

namespace {
const QString kUid = QStringLiteral("555000111");

// B-STAGE9-92 — a real 14-byte FIT header (byte 0 = header size, bytes 8-11 =
// ".FIT") ahead of the distinguishing payload, so `startsWithFitSignature`
// accepts this fixture the same way it accepts a genuine Garmin FIT download.
QByteArray fitBytesFor(const QString& activityId)
{
    QByteArray header(14, '\0');
    header[0] = char(14);
    header.replace(8, 4, QByteArrayLiteral(".FIT"));
    return header + QStringLiteral("FIT-bytes-for-%1").arg(activityId).toUtf8();
}

// DEC-070 fixture: a minimal payload carrying a real ZIP local-file-header
// signature, standing in for Garmin's `<activityId>_ACTIVITY.fit`-in-a-zip
// download shape.
QByteArray zipBytesFor(const QString& activityId)
{
    return QByteArray("PK\x03\x04") + QStringLiteral("-zip-bytes-for-%1").arg(activityId).toUtf8();
}

// B-STAGE9-83 fixture: a minimal payload carrying a real GZIP member header
// signature, standing in for a gzip-shaped Garmin download.
QByteArray gzipBytesFor(const QString& activityId)
{
    return QByteArray("\x1f\x8b") + QStringLiteral("-gzip-bytes-for-%1").arg(activityId).toUtf8();
}

// B-STAGE9-83 fixture (T-216/T-217): wraps `payload` in a REAL gzip member,
// so the controller's `inflateGzipMember` has genuine deflate output to
// invert, not just a spoofed two-byte signature (see gzipBytesFor above,
// which stays a signature-only fixture for T-222).
QByteArray realGzipMemberFor(const QByteArray& payload)
{
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    const int ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    Q_ASSERT(ret == Z_OK);

    QByteArray out;
    out.resize(payload.size() + 128);
    strm.avail_in = payload.size();
    strm.next_in = (Bytef*)(payload.constData());
    strm.avail_out = out.size();
    strm.next_out = (Bytef*)(out.data());

    deflate(&strm, Z_FINISH);
    out.resize(out.size() - strm.avail_out);
    deflateEnd(&strm);
    return out;
}

// B-STAGE9-89/-90 fixture: a payload long enough that chopping off a
// meaningful tail from its real gzip member still leaves a mid-stream
// truncation (not just a missing trailer) - `data.size() <= 4` cannot
// short-circuit inflateGzipMember, and the truncated deflate body can never
// reach Z_STREAM_END.
QByteArray longFitBytesFor(const QString& activityId)
{
    return QStringLiteral("FIT-bytes-for-%1-").arg(activityId).repeated(64).toUtf8();
}

// DEC-073 refusal-assertion helper (T-218..T-221): none of the four new
// mutation cases may leave ANY staged file for the activity, under any
// extension - the refusal happens before stagedPayloadPath is ever called.
bool noStagedFileExistsFor(const QString& athleteConfigDir, const QString& activityId)
{
    const QDir backfillDir(
        QDir(GarminSidecarStore::directoryFor(athleteConfigDir)).filePath(QStringLiteral("backfill")));
    const QStringList matches = backfillDir.entryList({QStringLiteral("garmin-%1.*").arg(activityId)}, QDir::Files);
    return matches.isEmpty();
}
} // namespace

// ---------------------------------------------------------------------------
// FakeBackfillClient — same-thread Python-free IGarminDownloadClient. Scripts
// one listing result (or a listFailed) and per-activity download bytes (or a
// downloadFailed by id). Emissions are queued so they arrive while the
// controller's blocking QEventLoop runs (mirrors FakeSyncClient).
// ---------------------------------------------------------------------------
class FakeBackfillClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    QVector<GarminActivitySummary> listResult;
    bool listOk = true;
    QHash<QString, QByteArray> okBytesById;
    QStringList failDownloadIds;
    QStringList listCallsSeen;
    QStringList downloadCallsSeen;

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString& sinceGmt, QUuid id) override
    {
        listCallsSeen << sinceGmt;
        const bool ok = listOk;
        const QVector<GarminActivitySummary> res = listResult;
        QMetaObject::invokeMethod(
            this,
            [this, id, ok, res]() {
                if (ok) {
                    emit activitiesListed(id, res);
                } else {
                    GarminListFailure e;
                    e.kind = GarminListFailure::Network;
                    e.rawMessage = QStringLiteral("scripted list failure");
                    emit listFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }

    void downloadActivity(const QString& activityId, const QString&, QUuid id) override
    {
        downloadCallsSeen << activityId;
        const bool fail = failDownloadIds.contains(activityId);
        const QByteArray bytes = okBytesById.value(activityId);
        QMetaObject::invokeMethod(
            this,
            [this, id, fail, bytes]() {
                if (fail) {
                    GarminDownloadFailure e;
                    e.kind = GarminDownloadFailure::Network;
                    emit downloadFailed(id, e);
                } else {
                    emit downloaded(id, bytes);
                }
            },
            Qt::QueuedConnection);
    }
};

// ---------------------------------------------------------------------------
// A TmpWriter that fails only FIT-file writes (path contains "/backfill/") -
// the controller's OWN sidecar writes (backfill-state/imported json, via the
// real GarminSidecarStore) must still succeed so the test can observe the
// cursor NOT advancing, rather than nothing being persisted at all
// (torn-write recovery, T-182). QFileDevice::fileName() on the tmp file is
// "<dest>.tmp", so the destination path is inspectable here.
// ---------------------------------------------------------------------------
class FailingTmpWriter : public AtomicFile::TmpWriter
{
  public:
    qint64 write(QFileDevice& f, const QByteArray& bytes) override
    {
        if (f.fileName().contains(QStringLiteral("/backfill/")))
            return -1;
        return f.write(bytes);
    }
};

// ---------------------------------------------------------------------------
// A TmpWriter that fails EVERY write to backfill-state-<uid>.json (T-185),
// used to force GarminSidecarStore::saveBackfillState() itself to fail.
// ---------------------------------------------------------------------------
class FailingBackfillStateWriter : public AtomicFile::TmpWriter
{
  public:
    qint64 write(QFileDevice& f, const QByteArray& bytes) override
    {
        if (f.fileName().contains(QStringLiteral("backfill-state-")))
            return -1;
        return f.write(bytes);
    }
};

// ---------------------------------------------------------------------------
// A TmpWriter that fails EVERY write to imported-<uid>.json (T-186), used to
// force GarminSidecarStore::recordImported() itself to fail while leaving the
// FIT write and backfill-state write untouched.
// ---------------------------------------------------------------------------
class FailingImportedWriter : public AtomicFile::TmpWriter
{
  public:
    qint64 write(QFileDevice& f, const QByteArray& bytes) override
    {
        if (f.fileName().contains(QStringLiteral("imported-")))
            return -1;
        return f.write(bytes);
    }
};

// ---------------------------------------------------------------------------
// A TmpWriter that lets the FIRST write to backfill-state-<uid>.json (the
// pre-listing persist) succeed but fails every subsequent one (T-187), so the
// PER-ACTIVITY saveBackfillState() call can be isolated from the initial one.
// ---------------------------------------------------------------------------
class FailingBackfillStateWriterAfterFirst : public AtomicFile::TmpWriter
{
  public:
    int seen = 0;
    qint64 write(QFileDevice& f, const QByteArray& bytes) override
    {
        if (f.fileName().contains(QStringLiteral("backfill-state-"))) {
            ++seen;
            if (seen > 1)
                return -1;
        }
        return f.write(bytes);
    }
};

class TestGarminBackfillController : public QObject
{
    Q_OBJECT

  private slots:

    // =====================================================================
    // T-176 — happy path: full range, multiple activities, oldest-first
    // =====================================================================
    void backfillDownloadsAllActivitiesOldestFirstAndReportsDone()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary newer, older;
        newer.activityId = QStringLiteral("2002");
        newer.startTimeGMT = QStringLiteral("2026-01-05 09:00:00");
        older.activityId = QStringLiteral("2001");
        older.startTimeGMT = QStringLiteral("2026-01-02 09:00:00");
        // Library's default order is newest-first - script it that way to prove
        // the controller re-sorts before processing.
        client.listResult = {newer, older};
        client.okBytesById.insert(older.activityId, fitBytesFor(older.activityId));
        client.okBytesById.insert(newer.activityId, fitBytesFor(newer.activityId));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const GarminBackfillController::Result r =
            ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-01-10 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QCOMPARE(r.importedCount, 2);

        // Oldest processed first: 2001's download must have been requested
        // before 2002's.
        QCOMPARE(client.downloadCallsSeen, QStringList({older.activityId, newer.activityId}));

        // Both FIT files staged atomically with the right bytes.
        QCOMPARE(QFile(GarminBackfillController::stagedPayloadPath(tmp.path(), older.activityId,
                                                                   fitBytesFor(older.activityId)))
                     .size(),
                 qint64(fitBytesFor(older.activityId).size()));
        QFile f(
            GarminBackfillController::stagedPayloadPath(tmp.path(), newer.activityId, fitBytesFor(newer.activityId)));
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), fitBytesFor(newer.activityId));

        // Cursor advanced to the LAST (newest) successfully-imported activity.
        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, newer.startTimeGMT);

        // Both activities recorded into the Tier-1 dedup map.
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.contains(older.activityId));
        QVERIFY(imported.contains(newer.activityId));
    }

    // =====================================================================
    // T-177 — empty result is success, not an error (DES-009)
    // =====================================================================
    void emptyListingIsSuccessNotError()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        client.listResult = {};

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const GarminBackfillController::Result r =
            ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-01-10 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QCOMPARE(r.importedCount, 0);
    }

    // =====================================================================
    // T-178 — user cancel: preserved partial results, Paused (interruption class 1)
    // =====================================================================
    void userCancelPausesAndPreservesPartialProgress()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1, a2, a3;
        a1.activityId = QStringLiteral("3001");
        a1.startTimeGMT = QStringLiteral("2026-02-01 00:00:00");
        a2.activityId = QStringLiteral("3002");
        a2.startTimeGMT = QStringLiteral("2026-02-02 00:00:00");
        a3.activityId = QStringLiteral("3003");
        a3.startTimeGMT = QStringLiteral("2026-02-03 00:00:00");
        client.listResult = {a1, a2, a3};
        for (const auto& a : {a1, a2, a3})
            client.okBytesById.insert(a.activityId, fitBytesFor(a.activityId));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        // downloadActivity() is queued (Qt::QueuedConnection) so cancel() can
        // land between the first and second activity: cancel from inside the
        // fake's download handler itself, once a1 has been requested.
        QObject::connect(&client, &IGarminDownloadClient::downloaded, &client, [&](QUuid, QByteArray) {
            if (client.downloadCallsSeen.size() == 1)
                ctrl.cancel();
        });

        const GarminBackfillController::Result r =
            ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-03-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UserCancelled));
        QCOMPARE(r.importedCount, 1);
        QCOMPARE(client.downloadCallsSeen, QStringList({a1.activityId}));

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, a1.startTimeGMT);
    }

    // =====================================================================
    // T-179 — hard crash / restart: a FRESH controller resumes from the
    // state a PRIOR (interrupted) run left on disk. On-disk state cannot
    // distinguish a clean pause from a SIGKILL - both leave the same
    // backfill-state-<uid>.json, so resuming from it IS the crash-recovery
    // path (interruption class 2, design.md: "same as cancel - resume offered").
    // =====================================================================
    void freshControllerResumesFromPriorRunsPersistedCursor()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        GarminActivitySummary a1, a2;
        a1.activityId = QStringLiteral("4001");
        a1.startTimeGMT = QStringLiteral("2026-03-01 00:00:00");
        a2.activityId = QStringLiteral("4002");
        a2.startTimeGMT = QStringLiteral("2026-03-02 00:00:00");

        // Run 1: only a1 is offered; simulates a process that was killed
        // before a2 ever appeared in a listing.
        {
            FakeBackfillClient client;
            client.listResult = {a1};
            client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));
            GarminBackfillController ctrl(&client, tmp.path(), kUid);
            const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-04-01 00:00:00"));
            QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        }

        // Run 2 (fresh instance - "after restart"): now a2 is available too.
        // Must resume from a1's cursor, NOT re-download a1, and pick up a2.
        {
            FakeBackfillClient client;
            client.listResult = {a1, a2}; // server would return both since range_start
            client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));
            client.okBytesById.insert(a2.activityId, fitBytesFor(a2.activityId));
            GarminBackfillController ctrl(&client, tmp.path(), kUid);
            const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-04-01 00:00:00"));

            QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
            QCOMPARE(r.importedCount, 1);
            QCOMPARE(client.downloadCallsSeen, QStringList({a2.activityId}));
            QVERIFY2(!client.downloadCallsSeen.contains(a1.activityId),
                     "already-imported activity must not be re-downloaded");
        }

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, a2.startTimeGMT);
    }

    // =====================================================================
    // T-180 — transient HTTP error (list): DES-005 retry already exhausted at
    // the Python layer by the time listFailed reaches us -> Paused + message.
    // =====================================================================
    void listFailureExhaustedRetryPausesWithErrorMessage()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        client.listOk = false;

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-02-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::TransientError));
        QVERIFY(!r.message.isEmpty());
    }

    // =====================================================================
    // T-181 — transient HTTP error (download): same recovery, mid-run.
    // =====================================================================
    void downloadFailureExhaustedRetryPausesPreservingEarlierProgress()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1, a2;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-04-01 00:00:00");
        a2.activityId = QStringLiteral("5002");
        a2.startTimeGMT = QStringLiteral("2026-04-02 00:00:00");
        client.listResult = {a1, a2};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));
        client.failDownloadIds << a2.activityId;

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-05-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::TransientError));
        QCOMPARE(r.importedCount, 1);

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, a1.startTimeGMT);
    }

    // =====================================================================
    // T-182 — torn FIT write: AtomicFile::writeOver fails -> Paused, cursor
    // NOT advanced past the unwritten activity (interruption class 4).
    // =====================================================================
    void tornFitWritePausesWithoutAdvancingCursor()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("6001");
        a1.startTimeGMT = QStringLiteral("2026-05-01 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        FailingTmpWriter failing;
        AtomicFile::setTmpWriterForTest(&failing);
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-06-01 00:00:00"));
        AtomicFile::setTmpWriterForTest(nullptr);

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::TornWrite));
        QCOMPARE(r.importedCount, 0);

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.lastSuccessStartTimeGMT.isEmpty(), "cursor must not advance past an unwritten activity");
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-183 — range validation: end < start, and the hard 5-year cap.
    // =====================================================================
    void invalidRangeAndHardCapAreRejected()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        FakeBackfillClient client;

        {
            GarminBackfillController ctrl(&client, tmp.path(), kUid);
            const auto r = ctrl.start(QStringLiteral("2026-02-01 00:00:00"), QStringLiteral("2026-01-01 00:00:00"));
            QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Rejected));
            QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::InvalidRange));
        }
        {
            GarminBackfillController ctrl(&client, tmp.path(), kUid);
            const auto r = ctrl.start(QStringLiteral("2010-01-01 00:00:00"), QStringLiteral("2026-01-01 00:00:00"));
            QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Rejected));
            QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::InvalidRange));
        }
        // A request never reaches the network on rejection.
        QCOMPARE(client.listCallsSeen.size(), 0);
    }

    // =====================================================================
    // T-184 — per-account isolation: a DIFFERENT uid's state file is untouched.
    // =====================================================================
    void differentAccountsStateFilesAreIsolated()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString otherUid = QStringLiteral("999888777");

        GarminSidecarStore::BackfillState otherState;
        otherState.lastSuccessStartTimeGMT = QStringLiteral("2026-01-15 00:00:00");
        otherState.rangeStart = QStringLiteral("2020-01-01 00:00:00");
        otherState.rangeEnd = QStringLiteral("2030-01-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), otherUid, otherState));

        FakeBackfillClient client;
        client.listResult = {};
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-02-01 00:00:00"));

        // kUid's list call used ITS OWN range start, not otherUid's cursor.
        QCOMPARE(client.listCallsSeen.first(), QStringLiteral("2026-01-01 00:00:00"));

        const GarminSidecarStore::BackfillLoadResult otherAfter =
            GarminSidecarStore::loadBackfillState(tmp.path(), otherUid);
        QVERIFY(otherAfter.isOk());
        QCOMPARE(otherAfter.state.lastSuccessStartTimeGMT, otherState.lastSuccessStartTimeGMT);
    }

    // =====================================================================
    // T-185 — the INITIAL saveBackfillState() (before any network op) failing
    // must pause the run, not silently proceed as if the resume cursor had
    // been persisted. No network call is ever made in this case.
    // =====================================================================
    void initialStatePersistFailurePausesBeforeAnyNetworkCall()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        client.listResult = {};

        FailingBackfillStateWriter failing;
        AtomicFile::setTmpWriterForTest(&failing);
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-02-01 00:00:00"));
        AtomicFile::setTmpWriterForTest(nullptr);

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QVERIFY2(!r.message.isEmpty(), "a persist failure must carry a user-facing message");
        QCOMPARE(client.listCallsSeen.size(), 0);
    }

    // =====================================================================
    // T-186 — recordImported() failing mid-loop must pause rather than
    // silently advance: no importedCount credit, cursor stays unadvanced, and
    // the (unwritten) dedup entry is genuinely absent on disk.
    // =====================================================================
    void recordImportedFailurePausesWithoutAdvancing()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("7001");
        a1.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        FailingImportedWriter failing;
        AtomicFile::setTmpWriterForTest(&failing);
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-08-01 00:00:00"));
        AtomicFile::setTmpWriterForTest(nullptr);

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QVERIFY2(!r.message.isEmpty(), "a persist failure must carry a user-facing message");
        QCOMPARE(r.importedCount, 0);

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.lastSuccessStartTimeGMT.isEmpty(), "cursor must not advance past an unrecorded activity");
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY2(!imported.contains(a1.activityId), "the failed recordImported() write must leave no entry");
    }

    // =====================================================================
    // T-187 — the PER-ACTIVITY saveBackfillState() (after a successful
    // download+write+recordImported) failing must also pause without
    // crediting importedCount or advancing the persisted cursor.
    // =====================================================================
    void perActivityStatePersistFailurePausesWithoutAdvancing()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("7002");
        a1.startTimeGMT = QStringLiteral("2026-07-02 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        FailingBackfillStateWriterAfterFirst failing;
        AtomicFile::setTmpWriterForTest(&failing);
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-08-01 00:00:00"));
        AtomicFile::setTmpWriterForTest(nullptr);

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QVERIFY2(!r.message.isEmpty(), "a persist failure must carry a user-facing message");
        QCOMPARE(r.importedCount, 0);

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.lastSuccessStartTimeGMT.isEmpty(), "cursor must not advance when its own persist failed");
        // recordImported() DID succeed here (only backfill-state writes fail) -
        // resume will treat a1 as already imported via the Tier-1 dedup map,
        // so it is not silently lost even though the cursor didn't move.
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-188 — B-R010-04 UI wiring seam: start()'s optional progress callback
    // fires once per successfully-imported activity, in PROCESSING (oldest-
    // first) order, with a running total. This is what the backfill dialog's
    // progress label and RideImportWizard file-list hand-off are built on.
    // =====================================================================
    void progressCallbackFiresPerSuccessInOrderWithRunningCount()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1, a2;
        a1.activityId = QStringLiteral("8001");
        a1.startTimeGMT = QStringLiteral("2026-09-01 00:00:00");
        a2.activityId = QStringLiteral("8002");
        a2.startTimeGMT = QStringLiteral("2026-09-02 00:00:00");
        client.listResult = {a2, a1}; // newest-first, as the library returns
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));
        client.okBytesById.insert(a2.activityId, fitBytesFor(a2.activityId));

        QStringList seenIds;
        QStringList seenPaths;
        QVector<int> seenCounts;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-10-01 00:00:00"),
                                  [&](const QString& id, const QString& stagedPath, int importedSoFar) {
                                      seenIds << id;
                                      seenPaths << stagedPath;
                                      seenCounts << importedSoFar;
                                  });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QCOMPARE(seenIds, QStringList({a1.activityId, a2.activityId}));
        QCOMPARE(seenCounts, (QVector<int>{1, 2}));
        QCOMPARE(
            seenPaths,
            QStringList(
                {GarminBackfillController::stagedPayloadPath(tmp.path(), a1.activityId, fitBytesFor(a1.activityId)),
                 GarminBackfillController::stagedPayloadPath(tmp.path(), a2.activityId, fitBytesFor(a2.activityId))}));
    }

    // =====================================================================
    // T-189 — the callback must NOT fire for an activity whose FIT write
    // never landed (torn write): only genuinely-imported activities are
    // reported to the caller.
    // =====================================================================
    void progressCallbackNotFiredForTornWrite()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("8003");
        a1.startTimeGMT = QStringLiteral("2026-09-03 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        FailingTmpWriter failing;
        AtomicFile::setTmpWriterForTest(&failing);
        int calls = 0;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-10-01 00:00:00"),
                                  [&](const QString&, const QString&, int) { ++calls; });
        AtomicFile::setTmpWriterForTest(nullptr);

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(calls, 0);
    }

    // =====================================================================
    // T-190 — B-R010-05: a SessionCheck that is already invalid before the
    // FIRST network op (the listing) must pause with no network call at all
    // - mirrors readdir()'s pre-listing fail-closed gate.
    // =====================================================================
    void sessionCheckFalseBeforeListingPausesWithNoNetworkCall()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        client.listResult = {};

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-02-01 00:00:00"),
                                  GarminBackfillController::ProgressCallback(), [] { return false; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::SessionInvalidated));
        QVERIFY(!r.message.isEmpty());
        QCOMPARE(client.listCallsSeen.size(), 0);
    }

    // =====================================================================
    // T-191 — B-R010-05: a SessionCheck that flips false BETWEEN two
    // activities (a disconnect/reconnect landing mid-run) must pause at the
    // next loop-head check, preserving the first activity's progress -
    // exactly the shape T-178's user-cancel case exercises for cancel().
    // =====================================================================
    void sessionCheckFalseBetweenActivitiesPausesPreservingEarlierProgress()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1, a2;
        a1.activityId = QStringLiteral("9001");
        a1.startTimeGMT = QStringLiteral("2026-09-10 00:00:00");
        a2.activityId = QStringLiteral("9002");
        a2.startTimeGMT = QStringLiteral("2026-09-11 00:00:00");
        client.listResult = {a1, a2};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));
        client.okBytesById.insert(a2.activityId, fitBytesFor(a2.activityId));

        bool sessionValid = true;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        // Flip via the progress callback, which fires only AFTER a1 is fully
        // recorded (cursor + dedup writes already landed - see start()'s own
        // comment on ordering) - so a1's OWN post-download recheck still sees
        // a valid session, and only the NEXT loop-head check (before a2 is
        // ever requested) observes the flip.
        const auto r = ctrl.start(
            QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-10-01 00:00:00"),
            [&](const QString&, const QString&, int) { sessionValid = false; }, [&] { return sessionValid; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::SessionInvalidated));
        QCOMPARE(r.importedCount, 1);
        QCOMPARE(client.downloadCallsSeen, QStringList({a1.activityId}));

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QCOMPARE(bf.state.lastSuccessStartTimeGMT, a1.startTimeGMT);
    }

    // =====================================================================
    // T-192 — B-R010-05 (REQ-017 clause c mirror): a SessionCheck that flips
    // false WHILE an activity's own download is in flight (discovered right
    // after blockingDownload() returns) must discard THAT activity - not
    // stage it, not record it, not advance the cursor past it.
    // =====================================================================
    void sessionCheckFalseImmediatelyAfterADownloadDiscardsThatActivity()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("9003");
        a1.startTimeGMT = QStringLiteral("2026-09-12 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        bool sessionValid = true;
        // Fires while the controller's own blockingDownload() is still
        // parked in its nested QEventLoop, BEFORE that call returns - the
        // exact race REQ-017 clause (c) closes for GarminConnect::readFile.
        QObject::connect(&client, &IGarminDownloadClient::downloaded, &client,
                         [&](QUuid, QByteArray) { sessionValid = false; });

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-10-01 00:00:00"),
                                  GarminBackfillController::ProgressCallback(), [&] { return sessionValid; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::SessionInvalidated));
        QCOMPARE(r.importedCount, 0);
        QVERIFY2(
            !QFile(GarminBackfillController::stagedPayloadPath(tmp.path(), a1.activityId, fitBytesFor(a1.activityId)))
                 .exists(),
            "a discarded download must never be staged to disk");

        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(tmp.path(), kUid);
        QVERIFY(bf.isOk());
        QVERIFY2(bf.state.lastSuccessStartTimeGMT.isEmpty(), "cursor must not advance past a discarded activity");
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY2(!imported.contains(a1.activityId), "a discarded activity must not be recorded as imported");
    }

    // =====================================================================
    // T-193 — a SessionCheck that stays valid for the whole run must not
    // change behaviour at all relative to the no-SessionCheck default
    // (T-176), proving the new parameter is additive.
    // =====================================================================
    void sessionCheckAlwaysValidBehavesExactlyLikeNoSessionCheck()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("9004");
        a1.startTimeGMT = QStringLiteral("2026-09-13 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-10-01 00:00:00"),
                                  GarminBackfillController::ProgressCallback(), [] { return true; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QCOMPARE(r.importedCount, 1);
    }

    // =====================================================================
    // T-212 — DEC-070: a downloaded payload starting with the ZIP local-file-
    // header signature must be staged under a ".zip" filename, so
    // RideImportWizard's suffix-dispatching expandFiles() actually unpacks
    // it instead of receiving a mislabelled ".fit" blob.
    // =====================================================================
    void zipSignaturePayloadIsStagedWithZipExtension()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10001");
        a1.startTimeGMT = QStringLiteral("2026-10-01 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, zipBytesFor(a1.activityId));

        QString reportedPath;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"),
                                  [&](const QString&, const QString& stagedPath, int) { reportedPath = stagedPath; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QVERIFY2(reportedPath.endsWith(QStringLiteral(".zip")), "a zip-signature payload must stage as .zip");
        QFile f(reportedPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), zipBytesFor(a1.activityId));
    }

    // =====================================================================
    // T-213 — DEC-070: a bare FIT payload (no ZIP signature) must still be
    // staged under a ".fit" filename - sniffing must not misclassify the
    // common case.
    // =====================================================================
    void bareFitPayloadIsStagedWithFitExtension()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10002");
        a1.startTimeGMT = QStringLiteral("2026-10-02 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, fitBytesFor(a1.activityId));

        QString reportedPath;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"),
                                  [&](const QString&, const QString& stagedPath, int) { reportedPath = stagedPath; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QVERIFY2(reportedPath.endsWith(QStringLiteral(".fit")), "a bare FIT payload must stage as .fit");
        QFile f(reportedPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), fitBytesFor(a1.activityId));
    }

    // =====================================================================
    // T-222 (was T-215, inverted by DEC-073/B-STAGE9-92) — a downloaded
    // payload starting with the GZIP member-header signature but not
    // decodable to a complete member (this fixture's bytes fail to parse at
    // all) is a refusal, not a ".gzip"-suffixed guess: DEC-073's accept-set
    // is exactly ZIP-or-FIT, reached either bare or via a complete gzip
    // inflate, so undecodable gzip-signed bytes pause with
    // UndecodablePayload and stage nothing.
    // =====================================================================
    void gzipSignatureThatFailsToDecodePausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10003");
        a1.startTimeGMT = QStringLiteral("2026-10-03 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, gzipBytesFor(a1.activityId));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-216 — DEC-072: a REAL gzip member wrapping a bare-FIT payload must be
    // inflated before staging, so the staged bytes are the FIT payload
    // (never the still-compressed bytes) and the extension follows the
    // INFLATED content (".fit").
    // =====================================================================
    void realGzipOfFitPayloadIsInflatedAndStagedAsFit()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10004");
        a1.startTimeGMT = QStringLiteral("2026-10-04 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(fitBytesFor(a1.activityId)));

        QString reportedPath;
        GarminSidecarStore::ImportedEntry recordedEntry;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"),
                                  [&](const QString&, const QString& stagedPath, int) { reportedPath = stagedPath; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QVERIFY2(reportedPath.endsWith(QStringLiteral(".fit")), "an inflated bare-FIT payload must stage as .fit");
        QFile f(reportedPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), fitBytesFor(a1.activityId));

        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.contains(a1.activityId));
        QCOMPARE(imported.value(a1.activityId).localFilename, QFileInfo(reportedPath).fileName());
    }

    // =====================================================================
    // T-217 — DEC-072: a REAL gzip member wrapping a ZIP payload must also be
    // inflated first, then re-sniffed - the extension follows the inflated
    // bytes, so gzip-of-zip stages as ".zip", not ".gzip" or ".fit".
    // =====================================================================
    void realGzipOfZipPayloadIsInflatedAndStagedAsZip()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10005");
        a1.startTimeGMT = QStringLiteral("2026-10-05 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(zipBytesFor(a1.activityId)));

        QString reportedPath;
        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"),
                                  [&](const QString&, const QString& stagedPath, int) { reportedPath = stagedPath; });

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Done));
        QVERIFY2(reportedPath.endsWith(QStringLiteral(".zip")), "an inflated ZIP payload must stage as .zip");
        QFile f(reportedPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), zipBytesFor(a1.activityId));

        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(imported.contains(a1.activityId));
        QCOMPARE(imported.value(a1.activityId).localFilename, QFileInfo(reportedPath).fileName());
    }

    // =====================================================================
    // T-218 — DEC-073/B-STAGE9-89: a gzip member truncated mid-stream (never
    // reaches Z_STREAM_END) must PAUSE with UndecodablePayload, not stage the
    // partial bytes zlib already produced. RED mutation (a) - dropping the
    // Z_STREAM_END requirement in inflateGzipMember - must fail this test.
    // =====================================================================
    void truncatedGzipMemberPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10006");
        a1.startTimeGMT = QStringLiteral("2026-10-06 00:00:00");
        client.listResult = {a1};
        const QByteArray full = realGzipMemberFor(longFitBytesFor(a1.activityId));
        QVERIFY(full.size() > 20);
        client.okBytesById.insert(a1.activityId, full.left(full.size() / 2));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-219 — DEC-073/B-STAGE9-89: two concatenated gzip members. The first
    // member reaches Z_STREAM_END but leaves the second member's bytes in
    // `avail_in`, so the stream is not "complete" by DEC-073's definition -
    // must PAUSE with UndecodablePayload, never stage the first member alone.
    // =====================================================================
    void concatenatedGzipMembersPauseAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10007");
        a1.startTimeGMT = QStringLiteral("2026-10-07 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(fitBytesFor(a1.activityId)) +
                                                     realGzipMemberFor(fitBytesFor(a1.activityId)));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-220 — DEC-073/B-STAGE9-90: gzip-of-gzip, BOTH members individually
    // complete and valid. One inflate only: the inner gzip-signed result is a
    // refusal, never a second inflate pass. RED mutation (b) - allowing a
    // second inflate pass - must fail this test.
    // =====================================================================
    void gzipOfGzipBothValidPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10008");
        a1.startTimeGMT = QStringLiteral("2026-10-08 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(realGzipMemberFor(fitBytesFor(a1.activityId))));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-221 — DEC-073: a complete, valid gzip member whose inflated output is
    // empty (zero-byte payload) must PAUSE with UndecodablePayload rather than
    // stage a zero-byte file under any name.
    // =====================================================================
    void gzipMemberOfEmptyPayloadPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10009");
        a1.startTimeGMT = QStringLiteral("2026-10-09 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(QByteArray()));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-223 — DEC-073/B-STAGE9-92: a bare downloaded payload that is neither
    // ZIP- nor gzip- nor FIT-signed must PAUSE with UndecodablePayload - the
    // accept-set is exactly ZIP-or-FIT, not "else assume FIT".
    // =====================================================================
    void bareUnsignedPayloadPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10010");
        a1.startTimeGMT = QStringLiteral("2026-10-10 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, QByteArrayLiteral("not-a-recognised-payload-shape"));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-224 — DEC-073/B-STAGE9-92: a REAL complete gzip member whose inflated
    // bytes are neither ZIP- nor FIT-signed must PAUSE with
    // UndecodablePayload - a successful inflate is not itself acceptance,
    // the DECODED bytes still have to resolve to a handled shape.
    // =====================================================================
    void realGzipOfNonFitNonZipPayloadPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10011");
        a1.startTimeGMT = QStringLiteral("2026-10-11 00:00:00");
        client.listResult = {a1};
        client.okBytesById.insert(a1.activityId, realGzipMemberFor(QByteArrayLiteral("plain-text-not-fit-not-zip")));

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-225 — DEC-073/B-STAGE9-91: ".FIT" present but at byte offset 7, not
    // the required 8, must PAUSE with UndecodablePayload - `startsWithFitSignature`
    // checks the exact offset, not merely the substring's presence anywhere
    // in the header.
    // =====================================================================
    void fitSignatureAtWrongOffsetPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10012");
        a1.startTimeGMT = QStringLiteral("2026-10-12 00:00:00");
        client.listResult = {a1};
        QByteArray offsetLie(14, '\0');
        offsetLie.replace(7, 4, QByteArrayLiteral(".FIT"));
        client.okBytesById.insert(a1.activityId, offsetLie);

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-226 — DEC-073/B-STAGE9-91: a buffer whose bytes 8-11 are exactly
    // ".FIT" but whose total length is only 12 (short of the 14-byte header)
    // must PAUSE with UndecodablePayload - `startsWithFitSignature`'s length
    // gate applies even when the marker bytes themselves are intact.
    // =====================================================================
    void fitSignatureHeaderTooShortPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10013");
        a1.startTimeGMT = QStringLiteral("2026-10-13 00:00:00");
        client.listResult = {a1};
        QByteArray shortHeader(12, '\0');
        shortHeader.replace(8, 4, QByteArrayLiteral(".FIT"));
        QCOMPARE(shortHeader.size(), 12);
        client.okBytesById.insert(a1.activityId, shortHeader);

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }

    // =====================================================================
    // T-227 — DEC-073/B-STAGE9-91: a buffer whose bytes 8-11 are exactly
    // ".FIT" but whose total length is only 13 (one byte short of the
    // 14-byte header) must PAUSE with UndecodablePayload -
    // `startsWithFitSignature`'s length gate is `>= 14`, not `>= 13`.
    // =====================================================================
    void fitSignatureHeaderOneByteShortPausesAsUndecodable()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        FakeBackfillClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("10014");
        a1.startTimeGMT = QStringLiteral("2026-10-14 00:00:00");
        client.listResult = {a1};
        QByteArray oneByteShortHeader(13, '\0');
        oneByteShortHeader.replace(8, 4, QByteArrayLiteral(".FIT"));
        QCOMPARE(oneByteShortHeader.size(), 13);
        client.okBytesById.insert(a1.activityId, oneByteShortHeader);

        GarminBackfillController ctrl(&client, tmp.path(), kUid);
        const auto r = ctrl.start(QStringLiteral("2026-01-01 00:00:00"), QStringLiteral("2026-11-01 00:00:00"));

        QCOMPARE(int(r.outcome), int(GarminBackfillController::Outcome::Paused));
        QCOMPARE(int(r.pauseReason), int(GarminBackfillController::PauseReason::UndecodablePayload));
        QCOMPARE(r.importedCount, 0);
        QVERIFY(noStagedFileExistsFor(tmp.path(), a1.activityId));
        const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(tmp.path(), kUid);
        QVERIFY(!imported.contains(a1.activityId));
    }
};

QTEST_MAIN(TestGarminBackfillController)
#include "testGarminBackfillController.moc"
