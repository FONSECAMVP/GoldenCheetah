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

namespace {
const QString kUid = QStringLiteral("555000111");

QByteArray fitBytesFor(const QString& activityId)
{
    return QStringLiteral("FIT-bytes-for-%1").arg(activityId).toUtf8();
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
        QCOMPARE(QFile(GarminBackfillController::stagedFitPath(tmp.path(), older.activityId)).size(),
                 qint64(fitBytesFor(older.activityId).size()));
        QFile f(GarminBackfillController::stagedFitPath(tmp.path(), newer.activityId));
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
};

QTEST_MAIN(TestGarminBackfillController)
#include "testGarminBackfillController.moc"
