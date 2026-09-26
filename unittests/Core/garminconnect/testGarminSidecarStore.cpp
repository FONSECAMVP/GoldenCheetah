/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-045 / T-046 — REQ-008 Slice B (DES-002 / DEC-017 Option A): GarminSidecarStore.
// Pure-Qt per-account persistence layer, Python-free — carries the `garmin-fast`
// CTest label. Mirrors GarminTokenStore's load-side perm-enforcement + atomic
// 0600 write discipline, but for the TWO per-account sidecar files:
//
//   <athlete>/garminconnect/imported-<uid>.json        — Tier-1 dedup map (DES-010)
//       maps garmin_activity_id -> { startTimeGMT, local_filename }
//   <athlete>/garminconnect/backfill-state-<uid>.json  — sync resume cursor
//       { last_success_startTimeGMT, range_start, range_end }
//
// Load discipline (mirrors GarminTokenStore::loadChecked):
//   * owner-wider mode (any group/other bit) → typed SidecarPermissionsRejected,
//     path exposed, NO data (DES-008 %1); caller forces a re-fetch.
//   * absent            → typed NotFound.
//   * torn/parse-failure → typed Torn, treated by the caller as absent-and-refetch
//     (DES-002: parse failure is a soft fallback, NOT a hard crash).
// Writes go through AtomicFile::writeOver at 0600 (DES-006, REQ-NF-Reliab-002).

#include "GarminSidecarStore.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <memory>
#include <vector>

class TestGarminSidecarStore : public QObject
{
    Q_OBJECT

  private:
    static bool isWiderThanOwnerOnly(QFileDevice::Permissions p)
    {
        const QFileDevice::Permissions groupOther = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                    QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                    QFileDevice::WriteOther | QFileDevice::ExeOther;
        return (p & groupOther) != QFileDevice::Permissions();
    }

  private slots:

    // ================================================================
    // T-045 — imported-<uid>.json (the Tier-1 dedup map)
    // ================================================================

    // Acceptance: recording an imported activity creates garminconnect/ at 0700
    // (POSIX), writes imported-<uid>.json owner-only 0600 with no .tmp residue,
    // and a subsequent load round-trips the (activityId -> {startTimeGMT,
    // local_filename}) entry EXACTLY. An unknown id misses.
    void imported_recordCreates0700Dir_file0600_roundTrip_miss()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("3141592");

        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = QStringLiteral("2026-03-14T09:26:53.0");
        entry.localFilename = QStringLiteral("garmin-987654321.fit");

        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uid, QStringLiteral("987654321"), entry));

        const QString subdir = athlete.filePath("garminconnect");
        const QString dest = subdir + "/imported-3141592.json";

        // Path shape is exactly per DES-002 (imported-<uid>.json).
        QCOMPARE(GarminSidecarStore::importedFilePath(athlete.path(), uid), dest);

        // Parent dir created and owner-only 0700 (POSIX).
        QVERIFY2(QFileInfo(subdir).isDir(), "garminconnect/ parent dir must be created");
#ifndef Q_OS_WIN
        const QFileDevice::Permissions dperms = QFileInfo(subdir).permissions();
        QVERIFY2(dperms.testFlag(QFileDevice::ReadOwner) && dperms.testFlag(QFileDevice::WriteOwner) &&
                     dperms.testFlag(QFileDevice::ExeOwner),
                 "created garminconnect/ must be owner rwx (0700)");
        QVERIFY2(!isWiderThanOwnerOnly(dperms), "created garminconnect/ must be 0700 — no group/other access");
        // The sidecar file itself is owner-only 0600 (REQ-NF-Sec-002).
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "imported-<uid>.json must be owner-only 0600 — no group/other access");
#endif
        // Atomic tmp+rename left no residue.
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "atomic write must leave no .tmp residue");

        // Round-trip: load the map, look up the recorded id → exact fields.
        const GarminSidecarStore::ImportedMap m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::Ok);
        QVERIFY2(m.isOk(), "0600 imported file must load as Ok");
        QVERIFY2(m.contains(QStringLiteral("987654321")), "recorded activity id must be present (isImported)");
        const GarminSidecarStore::ImportedEntry got = m.value(QStringLiteral("987654321"));
        QCOMPARE(got.startTimeGMT, entry.startTimeGMT);
        QCOMPARE(got.localFilename, entry.localFilename);

        // Miss on an unknown id.
        QVERIFY2(!m.contains(QStringLiteral("111111111")), "unknown activity id must miss");
    }

    // Recording a second activity read-modify-writes: both entries survive.
    void imported_secondRecordPreservesFirst()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("42");

        GarminSidecarStore::ImportedEntry a;
        a.startTimeGMT = QStringLiteral("2026-01-01T00:00:00.0");
        a.localFilename = QStringLiteral("garmin-100.fit");
        GarminSidecarStore::ImportedEntry b;
        b.startTimeGMT = QStringLiteral("2026-02-02T12:00:00.0");
        b.localFilename = QStringLiteral("garmin-200.tcx");

        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uid, QStringLiteral("100"), a));
        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uid, QStringLiteral("200"), b));

        const GarminSidecarStore::ImportedMap m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::Ok);
        QVERIFY2(m.contains(QStringLiteral("100")), "first record must survive the second (read-modify-write)");
        QVERIFY2(m.contains(QStringLiteral("200")), "second record must be present");
        QCOMPARE(m.value(QStringLiteral("100")).localFilename, a.localFilename);
        QCOMPARE(m.value(QStringLiteral("200")).localFilename, b.localFilename);
    }

#ifndef Q_OS_WIN
    // An imported-<uid>.json chmod'd 0640 (group-readable) is REFUSED — typed
    // SidecarPermissionsRejected, NO entries returned, offending path exposed
    // (mirrors TEST-014's mask). Also 0644 (world-readable) and the write/exec
    // bit classes, so the refusal mask covers all group/other bits.
    void imported_ownerWiderFileRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("77");
        GarminSidecarStore::ImportedEntry e;
        e.startTimeGMT = QStringLiteral("2026-05-05T05:05:05.0");
        e.localFilename = QStringLiteral("garmin-1.fit");
        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uid, QStringLiteral("1"), e));

        const QString dest = GarminSidecarStore::importedFilePath(athlete.path(), uid);

        // 0640 — group-read widened.
        QVERIFY2(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup),
                 "test setup: widen imported-<uid>.json to 0640");
        GarminSidecarStore::ImportedMap m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::SidecarPermissionsRejected);
        QVERIFY2(m.isRejected(), "0640 imported file must be SidecarPermissionsRejected");
        QVERIFY2(m.entries.isEmpty(), "a rejected file must NOT return its entries");
        QCOMPARE(m.path, dest); // offending path exposed (DES-008 %1)

        // 0644 — world-read widened.
        QVERIFY(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup |
                                                QFileDevice::ReadOther));
        m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::SidecarPermissionsRejected);
        QVERIFY(m.entries.isEmpty());

        // 0620 — group-WRITE-only widened (kills a Read-only-narrowed mask mutant).
        QVERIFY(
            QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::WriteGroup));
        m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::SidecarPermissionsRejected);

        // 0601 — other-EXEC-only widened (kills a Read/Write-narrowed mask mutant).
        QVERIFY(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOther));
        m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::SidecarPermissionsRejected);
    }
#endif

    // Torn/garbage JSON is a typed Torn outcome, NOT a crash and NOT Ok — the
    // caller treats it as absent-and-refetch (DES-002: parse failure → fallback).
    void imported_tornJsonIsTornNotCrash()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("88");

        // Force the dir + a garbage file where the imported sidecar belongs.
        const QString dest = GarminSidecarStore::importedFilePath(athlete.path(), uid);
        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(athlete.path())));
        QFile f(dest);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("{ this is not <json> at all "));
        f.close();
#ifndef Q_OS_WIN
        QVERIFY(f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
#endif

        const GarminSidecarStore::ImportedMap m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::Torn);
        QVERIFY2(!m.isOk(), "torn file must NOT be Ok");
        QVERIFY2(m.entries.isEmpty(), "torn file must return no entries (absent-and-refetch)");
    }

    // Per-account partition is STRUCTURAL: the filename embeds <uid>, so account
    // A's record is invisible under account B (record uidA → look up uidB → miss).
    void imported_twoAccountsIndependent()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uidA = QStringLiteral("1001");
        const QString uidB = QStringLiteral("2002");

        GarminSidecarStore::ImportedEntry e;
        e.startTimeGMT = QStringLiteral("2026-06-06T06:06:06.0");
        e.localFilename = QStringLiteral("garmin-555.fit");
        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uidA, QStringLiteral("555"), e));

        // The two accounts resolve to DIFFERENT files.
        QVERIFY(GarminSidecarStore::importedFilePath(athlete.path(), uidA) !=
                GarminSidecarStore::importedFilePath(athlete.path(), uidB));

        // Under A: present. Under B: absent (NotFound) and the id is invisible.
        const GarminSidecarStore::ImportedMap mA = GarminSidecarStore::loadImported(athlete.path(), uidA);
        QCOMPARE(mA.status, GarminSidecarStore::LoadStatus::Ok);
        QVERIFY(mA.contains(QStringLiteral("555")));

        const GarminSidecarStore::ImportedMap mB = GarminSidecarStore::loadImported(athlete.path(), uidB);
        QCOMPARE(mB.status, GarminSidecarStore::LoadStatus::NotFound);
        QVERIFY2(!mB.contains(QStringLiteral("555")), "account A's imported id must be invisible under account B");
    }

    // An absent imported-<uid>.json is a distinct NotFound (not Torn, not Rejected).
    void imported_absentIsNotFound()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("999");
        QVERIFY(!QFileInfo::exists(GarminSidecarStore::importedFilePath(athlete.path(), uid)));

        const GarminSidecarStore::ImportedMap m = GarminSidecarStore::loadImported(athlete.path(), uid);
        QCOMPARE(m.status, GarminSidecarStore::LoadStatus::NotFound);
        QVERIFY(!m.isOk() && !m.isRejected());
        QVERIFY(m.entries.isEmpty());
    }

#ifndef Q_OS_WIN
    // DES-002: an EXISTING garminconnect/ dir's perms are NOT tightened — the
    // store only sets 0700 when it CREATES the dir. The sidecar file is still 0600.
    void imported_existingDirNotTightened()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("70");
        const QString subdir = athlete.filePath("garminconnect");
        QVERIFY(QDir(athlete.path()).mkdir("garminconnect"));

        // Widen the dir to 0755 before the store ever touches it.
        QFile dirHandle(subdir);
        QVERIFY(dirHandle.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                         QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                         QFileDevice::ExeOther));

        GarminSidecarStore::ImportedEntry e;
        e.startTimeGMT = QStringLiteral("t");
        e.localFilename = QStringLiteral("garmin-9.fit");
        QVERIFY(GarminSidecarStore::recordImported(athlete.path(), uid, QStringLiteral("9"), e));

        const QFileDevice::Permissions dperms = QFileInfo(subdir).permissions();
        QVERIFY2(dperms.testFlag(QFileDevice::ReadGroup) && dperms.testFlag(QFileDevice::ReadOther),
                 "an EXISTING garminconnect/ dir must keep its (wider) perms — DES-002 does not tighten it");

        // …but the sidecar file itself is still forced owner-only 0600.
        QVERIFY(
            !isWiderThanOwnerOnly(QFileInfo(GarminSidecarStore::importedFilePath(athlete.path(), uid)).permissions()));
    }
#endif

    // ================================================================
    // T-046 — backfill-state-<uid>.json (the sync resume cursor)
    // ================================================================

    // Acceptance: save→load round-trips {last_success_startTimeGMT, range_start,
    // range_end} EXACTLY; the file is atomic 0600 with no .tmp residue.
    void backfill_saveLoadRoundTrip_file0600_noTmp()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("271828");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-04-01T08:00:00.0");
        st.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
        st.rangeEnd = QStringLiteral("2026-04-01T00:00:00.0");

        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        const QString dest = athlete.filePath("garminconnect") + "/backfill-state-271828.json";
        QCOMPARE(GarminSidecarStore::backfillStateFilePath(athlete.path(), uid), dest);

#ifndef Q_OS_WIN
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "backfill-state-<uid>.json must be owner-only 0600 (REQ-NF-Sec-002)");
#endif
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "atomic write must leave no .tmp residue");

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::Ok);
        QVERIFY(r.isOk());
        QCOMPARE(r.state.lastSuccessStartTimeGMT, st.lastSuccessStartTimeGMT);
        QCOMPARE(r.state.rangeStart, st.rangeStart);
        QCOMPARE(r.state.rangeEnd, st.rangeEnd);
        QCOMPARE(r.path, dest);
    }

    // An absent backfill-state-<uid>.json is a distinct NotFound outcome.
    void backfill_absentIsNotFound()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("0");
        QVERIFY(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(athlete.path(), uid)));

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::NotFound);
        QVERIFY(!r.isOk() && !r.isRejected());
    }

#ifndef Q_OS_WIN
    // An owner-wider backfill-state-<uid>.json is REFUSED (SidecarPermissionsRejected,
    // path exposed, no state).
    void backfill_ownerWiderFileRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("55");
        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("x");
        st.rangeStart = QStringLiteral("y");
        st.rangeEnd = QStringLiteral("z");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
        QVERIFY(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup));

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::SidecarPermissionsRejected);
        QVERIFY2(r.isRejected(), "0640 backfill-state file must be SidecarPermissionsRejected");
        QCOMPARE(r.path, dest);
        // No state leaked on rejection.
        QVERIFY(r.state.lastSuccessStartTimeGMT.isEmpty());
    }
#endif

    // Torn/garbage JSON in the backfill sidecar is a typed Torn outcome (not crash).
    void backfill_tornJsonIsTornNotCrash()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("66");

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(athlete.path())));
        QFile f(dest);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("not json <<<"));
        f.close();
#ifndef Q_OS_WIN
        QVERIFY(f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
#endif

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::Torn);
        QVERIFY2(!r.isOk(), "torn backfill file must NOT be Ok");
    }

    // ================================================================
    // DEC-071 / B-STAGE9-79 slice 1 — the `pending` manifest
    // ================================================================

    // Acceptance: recordPendingBackfill then dropPendingBackfill both
    // read-modify-write the WHOLE cursor atomically at 0600 — the three
    // existing cursor fields set by an earlier saveBackfillState survive both
    // calls unchanged, and the pending entry itself round-trips then vanishes.
    void backfill_pendingRecordDropRoundTrip_cursorFieldsUnchanged_0600_noTmp()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("161803");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-05-01T00:00:00.0");
        st.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
        st.rangeEnd = QStringLiteral("2026-05-01T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        GarminSidecarStore::ImportedEntry pending;
        pending.startTimeGMT = QStringLiteral("2026-04-15T10:00:00.0");
        pending.localFilename = QStringLiteral("garmin-321.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("321"), pending));

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
#ifndef Q_OS_WIN
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "backfill-state-<uid>.json must stay owner-only 0600 after recording a pending entry");
#endif
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "atomic write must leave no .tmp residue after recording pending");

        const GarminSidecarStore::BackfillLoadResult afterRecord =
            GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QVERIFY(afterRecord.isOk());
        QVERIFY2(afterRecord.state.pending.contains(QStringLiteral("321")), "recorded pending id must be present");
        QCOMPARE(afterRecord.state.pending.value(QStringLiteral("321")).startTimeGMT, pending.startTimeGMT);
        QCOMPARE(afterRecord.state.pending.value(QStringLiteral("321")).localFilename, pending.localFilename);
        QCOMPARE(afterRecord.state.lastSuccessStartTimeGMT, st.lastSuccessStartTimeGMT);
        QCOMPARE(afterRecord.state.rangeStart, st.rangeStart);
        QCOMPARE(afterRecord.state.rangeEnd, st.rangeEnd);

        QVERIFY(GarminSidecarStore::dropPendingBackfill(athlete.path(), uid, QStringLiteral("321")));
#ifndef Q_OS_WIN
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "backfill-state-<uid>.json must stay owner-only 0600 after dropping a pending entry");
#endif
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "atomic write must leave no .tmp residue after dropping pending");

        const GarminSidecarStore::BackfillLoadResult afterDrop =
            GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QVERIFY(afterDrop.isOk());
        QVERIFY2(!afterDrop.state.pending.contains(QStringLiteral("321")), "dropped pending id must be gone");
        QCOMPARE(afterDrop.state.lastSuccessStartTimeGMT, st.lastSuccessStartTimeGMT);
        QCOMPARE(afterDrop.state.rangeStart, st.rangeStart);
        QCOMPARE(afterDrop.state.rangeEnd, st.rangeEnd);
    }

    // A pre-DEC-071 3-field file (no `pending`, no `schema_version`) MUST load
    // Ok with pending empty and schema_version 0 — never Torn, and the load
    // must not rewrite the file (no key injected on a read-only path).
    void backfill_legacyFileLoadsOk_pendingEmpty_versionZero_notRewritten()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("31415");

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(athlete.path())));
        QFile f(dest);
        QVERIFY(f.open(QIODevice::WriteOnly));
        const QByteArray legacyBytes = QByteArrayLiteral("{\"last_success_startTimeGMT\":\"2026-02-02T02:02:02.0\","
                                                         "\"range_start\":\"2026-01-01T00:00:00.0\","
                                                         "\"range_end\":\"2026-02-02T00:00:00.0\"}");
        f.write(legacyBytes);
        f.close();
#ifndef Q_OS_WIN
        QVERIFY(f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
#endif

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::Ok);
        QVERIFY2(r.isOk(), "a pre-DEC-071 3-field file (no pending, no schema_version) must load Ok");
        QVERIFY2(r.state.pending.isEmpty(), "a legacy file has no pending key: pending must be empty");
        QCOMPARE(r.state.schemaVersion, 0);
        QCOMPARE(r.state.lastSuccessStartTimeGMT, QStringLiteral("2026-02-02T02:02:02.0"));
        QCOMPARE(r.state.rangeStart, QStringLiteral("2026-01-01T00:00:00.0"));
        QCOMPARE(r.state.rangeEnd, QStringLiteral("2026-02-02T00:00:00.0"));

        QFile check(dest);
        QVERIFY(check.open(QIODevice::ReadOnly));
        QCOMPARE(check.readAll(), legacyBytes); // load must never rewrite the file
    }

    // ================================================================
    // DEC-075 — pending is single-writer; a file we cannot model is refused
    // ================================================================

    // saveBackfillState must never persist its own `state.pending` argument —
    // only recordPendingBackfill/dropPendingBackfill mutate the on-disk map.
    void backfill_saveIgnoresArgumentPending_preservesOnDiskManifest()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("271831");

        GarminSidecarStore::BackfillState seed;
        seed.lastSuccessStartTimeGMT = QStringLiteral("2026-05-01T00:00:00.0");
        seed.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
        seed.rangeEnd = QStringLiteral("2026-05-01T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, seed));

        GarminSidecarStore::ImportedEntry pendingEntry;
        pendingEntry.startTimeGMT = QStringLiteral("2026-04-20T00:00:00.0");
        pendingEntry.localFilename = QStringLiteral("garmin-321.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("321"), pendingEntry));

        // A FRESH state, as every caller today constructs, carrying a bogus
        // pending entry that must never reach disk.
        GarminSidecarStore::BackfillState fresh;
        fresh.lastSuccessStartTimeGMT = QStringLiteral("2026-06-01T00:00:00.0");
        fresh.rangeStart = QStringLiteral("2026-02-01T00:00:00.0");
        fresh.rangeEnd = QStringLiteral("2026-06-01T00:00:00.0");
        GarminSidecarStore::ImportedEntry bogus;
        bogus.startTimeGMT = QStringLiteral("bogus");
        bogus.localFilename = QStringLiteral("bogus.fit");
        fresh.pending.insert(QStringLiteral("999"), bogus);
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, fresh));

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QVERIFY(r.isOk());
        QCOMPARE(r.state.pending.size(), 1);
        QVERIFY2(r.state.pending.contains(QStringLiteral("321")),
                 "the pending manifest recorded before the fresh save must survive it");
        QVERIFY2(!r.state.pending.contains(QStringLiteral("999")),
                 "saveBackfillState must never persist its own argument's pending");
        QCOMPARE(r.state.lastSuccessStartTimeGMT, fresh.lastSuccessStartTimeGMT);
        QCOMPARE(r.state.rangeStart, fresh.rangeStart);
        QCOMPARE(r.state.rangeEnd, fresh.rangeEnd);
    }

#ifndef Q_OS_WIN
    // A SidecarPermissionsRejected cursor must never be overwritten: all three
    // writers refuse (return false), and the file's bytes AND mode are left
    // byte-identical.
    void backfill_rejectedFileNeverOverwritten_allThreeWritersRefuse()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("112358");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-03-03T00:00:00.0");
        st.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
        st.rangeEnd = QStringLiteral("2026-03-03T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = QStringLiteral("2026-02-15T00:00:00.0");
        entry.localFilename = QStringLiteral("garmin-42.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("42"), entry));

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
        QFile before(dest);
        QVERIFY(before.open(QIODevice::ReadOnly));
        const QByteArray bytesBefore = before.readAll();
        before.close();

        QVERIFY(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup));
        const QFileDevice::Permissions modeBefore = QFileInfo(dest).permissions();

        QVERIFY2(!GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("77"), entry),
                 "recordPendingBackfill must refuse a Rejected cursor");
        QVERIFY2(!GarminSidecarStore::dropPendingBackfill(athlete.path(), uid, QStringLiteral("42")),
                 "dropPendingBackfill must refuse a Rejected cursor");
        QVERIFY2(!GarminSidecarStore::saveBackfillState(athlete.path(), uid, st),
                 "saveBackfillState must refuse a Rejected cursor");

        QFile after(dest);
        QVERIFY(after.open(QIODevice::ReadOnly));
        QCOMPARE(after.readAll(), bytesBefore);
        after.close();
        QCOMPARE(QFileInfo(dest).permissions(), modeBefore);
    }
#endif

    // A `pending` entry that parses but is not itself an object is
    // PendingManifestMalformed, not Ok — and all three writers refuse, leaving
    // the file's bytes unchanged.
    void backfill_pendingManifestMalformed_refusesAllWriters_bytesUnchanged()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("503");

        const QString dest = GarminSidecarStore::backfillStateFilePath(athlete.path(), uid);
        QVERIFY(QDir().mkpath(GarminSidecarStore::directoryFor(athlete.path())));
        QFile f(dest);
        QVERIFY(f.open(QIODevice::WriteOnly));
        const QByteArray malformedBytes = QByteArrayLiteral("{\"last_success_startTimeGMT\":\"2026-01-01T00:00:00.0\","
                                                            "\"range_start\":\"2026-01-01T00:00:00.0\","
                                                            "\"range_end\":\"2026-01-02T00:00:00.0\","
                                                            "\"pending\":{\"55\":\"not-an-object\"}}");
        f.write(malformedBytes);
        f.close();
#ifndef Q_OS_WIN
        QVERIFY(f.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner));
#endif

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QCOMPARE(r.status, GarminSidecarStore::LoadStatus::PendingManifestMalformed);
        QVERIFY2(!r.isOk(), "a malformed pending entry must not be Ok");

        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = QStringLiteral("x");
        entry.localFilename = QStringLiteral("y.fit");
        QVERIFY2(!GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("66"), entry),
                 "recordPendingBackfill must refuse a PendingManifestMalformed cursor");
        QVERIFY2(!GarminSidecarStore::dropPendingBackfill(athlete.path(), uid, QStringLiteral("55")),
                 "dropPendingBackfill must refuse a PendingManifestMalformed cursor");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-09-09T00:00:00.0");
        QVERIFY2(!GarminSidecarStore::saveBackfillState(athlete.path(), uid, st),
                 "saveBackfillState must refuse a PendingManifestMalformed cursor");

        QFile check(dest);
        QVERIFY(check.open(QIODevice::ReadOnly));
        QCOMPARE(check.readAll(), malformedBytes);
    }

    // ================================================================
    // B-STAGE9-112 — dropping the pending entry AT the cursor rewinds it
    // ================================================================

    // T-235 — mutation-must-go-RED: drop a pending entry whose startTimeGMT
    // equals the persisted cursor. Per DEC-075 the store owns this
    // invariant, not the caller; per DEC-078 clearing it to empty is what
    // GarminBackfillController::start() reads as "no prior success", the
    // exact value asserted below.
    void backfill_dropAtCursorEntry_rewindsCursorStrictlyBeforeIt_otherPendingSurvive_rangeUnchanged()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("235235");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-06-02T00:00:00.0");
        st.rangeStart = QStringLiteral("2026-06-01T00:00:00.0");
        st.rangeEnd = QStringLiteral("2026-06-10T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        GarminSidecarStore::ImportedEntry survivor;
        survivor.startTimeGMT = QStringLiteral("2026-06-01T12:00:00.0"); // strictly before cursor
        survivor.localFilename = QStringLiteral("garmin-111.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("111"), survivor));

        GarminSidecarStore::ImportedEntry atCursor;
        atCursor.startTimeGMT = st.lastSuccessStartTimeGMT; // exactly AT the cursor
        atCursor.localFilename = QStringLiteral("garmin-222.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("222"), atCursor));

        QVERIFY(GarminSidecarStore::dropPendingBackfill(athlete.path(), uid, QStringLiteral("222")));

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QVERIFY(r.isOk());
        QVERIFY2(!r.state.pending.contains(QStringLiteral("222")), "the dropped id must be gone");
        QCOMPARE(r.state.lastSuccessStartTimeGMT, QString());
        QVERIFY2(r.state.pending.contains(QStringLiteral("111")), "every OTHER pending entry must survive (DEC-075)");
        QCOMPARE(r.state.rangeStart, st.rangeStart);
        QCOMPARE(r.state.rangeEnd, st.rangeEnd);
    }

    // T-236 — side-effect invariant: dropping a pending entry STRICTLY BEFORE
    // the cursor must leave the cursor untouched. Only the entry the cursor
    // currently points AT is the resume anchor; an earlier still-pending
    // entry going missing does not invalidate later, already-confirmed
    // progress.
    void backfill_dropStrictlyBeforeCursorEntry_leavesCursorUntouched()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString uid = QStringLiteral("235236");

        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-03T00:00:00.0");
        st.rangeStart = QStringLiteral("2026-07-01T00:00:00.0");
        st.rangeEnd = QStringLiteral("2026-07-10T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athlete.path(), uid, st));

        GarminSidecarStore::ImportedEntry earlier;
        earlier.startTimeGMT = QStringLiteral("2026-07-02T00:00:00.0"); // strictly before cursor
        earlier.localFilename = QStringLiteral("garmin-333.fit");
        QVERIFY(GarminSidecarStore::recordPendingBackfill(athlete.path(), uid, QStringLiteral("333"), earlier));

        QVERIFY(GarminSidecarStore::dropPendingBackfill(athlete.path(), uid, QStringLiteral("333")));

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athlete.path(), uid);
        QVERIFY(r.isOk());
        QVERIFY2(!r.state.pending.contains(QStringLiteral("333")), "the dropped id must be gone");
        QCOMPARE(r.state.lastSuccessStartTimeGMT, st.lastSuccessStartTimeGMT);
    }

    // ================================================================
    // DEC-076 — the store serializes its own load-modify-write transactions
    // ================================================================

    // >=4 threads x many iterations, each thread's ids distinct from every
    // other thread's, all calling recordPendingBackfill for ONE uid. Without
    // the DEC-076 lock, two threads' load-modify-write cycles interleave and
    // one thread's insert is overwritten by the other's stale read — this
    // asserts every single id survives.
    void backfill_concurrentRecordPendingBackfill_distinctIds_allSurvive()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString athletePath = athlete.path();
        const QString uid = QStringLiteral("999001");

        constexpr int kThreads = 4;
        constexpr int kPerThread = 25;

        std::vector<std::unique_ptr<QThread>> threads;
        for (int t = 0; t < kThreads; ++t) {
            threads.push_back(std::unique_ptr<QThread>(QThread::create([athletePath, uid, t]() {
                for (int i = 0; i < kPerThread; ++i) {
                    GarminSidecarStore::ImportedEntry e;
                    e.startTimeGMT = QStringLiteral("2026-01-01T00:00:00.0");
                    e.localFilename = QStringLiteral("garmin-x.fit");
                    GarminSidecarStore::recordPendingBackfill(athletePath, uid, QStringLiteral("%1-%2").arg(t).arg(i),
                                                              e);
                }
            })));
        }
        for (auto& th : threads)
            th->start();
        for (auto& th : threads)
            th->wait();

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athletePath, uid);
        QVERIFY(r.isOk());
        QCOMPARE(r.state.pending.size(), kThreads * kPerThread);
        for (int t = 0; t < kThreads; ++t) {
            for (int i = 0; i < kPerThread; ++i) {
                QVERIFY2(r.state.pending.contains(QStringLiteral("%1-%2").arg(t).arg(i)),
                         "every concurrently-recorded pending id must survive (DEC-076)");
            }
        }
    }

    // One thread hammers saveBackfillState (cursor advance) while another
    // hammers recordPendingBackfill (pending insert) on the SAME uid. Without
    // the DEC-076 lock, saveBackfillState's own load-modify-write can capture
    // a pre-insert pending snapshot and overwrite the recorder's entry (the
    // classic lost-update race) — this asserts both survive.
    void backfill_concurrentSaveAndRecordPending_bothSurvive()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString athletePath = athlete.path();
        const QString uid = QStringLiteral("999002");

        GarminSidecarStore::BackfillState seed;
        seed.lastSuccessStartTimeGMT = QStringLiteral("2026-01-01T00:00:00.0");
        seed.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
        seed.rangeEnd = QStringLiteral("2026-01-02T00:00:00.0");
        QVERIFY(GarminSidecarStore::saveBackfillState(athletePath, uid, seed));

        constexpr int kIterations = 200;
        std::unique_ptr<QThread> saver(QThread::create([athletePath, uid]() {
            for (int i = 0; i < kIterations; ++i) {
                GarminSidecarStore::BackfillState st;
                st.lastSuccessStartTimeGMT = QStringLiteral("2026-03-15T00:00:00.0");
                st.rangeStart = QStringLiteral("2026-01-01T00:00:00.0");
                st.rangeEnd = QStringLiteral("2026-03-15T00:00:00.0");
                GarminSidecarStore::saveBackfillState(athletePath, uid, st);
            }
        }));
        std::unique_ptr<QThread> recorder(QThread::create([athletePath, uid]() {
            for (int i = 0; i < kIterations; ++i) {
                GarminSidecarStore::ImportedEntry e;
                e.startTimeGMT = QStringLiteral("2026-02-01T00:00:00.0");
                e.localFilename = QStringLiteral("garmin-555.fit");
                GarminSidecarStore::recordPendingBackfill(athletePath, uid, QStringLiteral("555"), e);
            }
        }));

        saver->start();
        recorder->start();
        saver->wait();
        recorder->wait();

        const GarminSidecarStore::BackfillLoadResult r = GarminSidecarStore::loadBackfillState(athletePath, uid);
        QVERIFY(r.isOk());
        QCOMPARE(r.state.lastSuccessStartTimeGMT, QStringLiteral("2026-03-15T00:00:00.0"));
        QVERIFY2(r.state.pending.contains(QStringLiteral("555")),
                 "a concurrent recordPendingBackfill entry must survive concurrent saveBackfillState calls (DEC-076)");
    }
};

QTEST_APPLESS_MAIN(TestGarminSidecarStore)
#include "testGarminSidecarStore.moc"
