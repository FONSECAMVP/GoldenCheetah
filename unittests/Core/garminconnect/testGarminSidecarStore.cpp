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
};

QTEST_APPLESS_MAIN(TestGarminSidecarStore)
#include "testGarminSidecarStore.moc"
