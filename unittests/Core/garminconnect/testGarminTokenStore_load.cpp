/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-014 — REQ-006 (Slice A): GarminTokenStore load-side permission enforcement.
// Pure-Qt storage layer, Python-free — carries the `garmin-fast` CTest label.
//
// Encodes REQ-006's acceptance criterion literally:
//
//   "Token files restricted to OS user; non-conforming files refused. POSIX:
//    file mode 0600. Windows: ACL grants only the owning user. On load, if the
//    file mode/ACL does not match, GC refuses to read the file, emits an error,
//    and prompts a fresh SSO. [F-M4 fix]"
//
//   + DES-002 invariant: on read, a mode/ACL WIDER than owner-only makes GC
//     REFUSE to load, raise a typed TokenPermissionsRejected (naming the path),
//     and trigger a forced re-login.
//
// This slice implements & tests the POSIX 0600 mode check. Windows ACL checking
// is REQ-NF-Pkg-001 Phase-2 CI territory (finding A3-R004-09) — not built here.

#include "GarminTokenStore.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestGarminTokenStoreLoad : public QObject
{
    Q_OBJECT

  private slots:

    // Case 1: a tokens.json written owner-only 0600 loads successfully and
    // returns the EXACT bytes, with the OK status.
    void conformingFileLoadsExactBytes()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("{\"oauth2\":\"conforming-0600-blob\"}");

        // save() writes tokens.json owner-only 0600 (REQ-004 / DES-006).
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::Ok);
        QVERIFY2(r.isOk(), "0600 file must load as Ok");
        QCOMPARE(r.bytes, blob);
        // The result exposes the file path (DES-008 %1 arg) even on success.
        QCOMPARE(r.path, GarminTokenStore::tokenFilePath(athlete.path()));
    }

#ifndef Q_OS_WIN
    // Case 2: a tokens.json chmod'd 0640 (group-readable) is REFUSED — the
    // typed TokenPermissionsRejected outcome, NO bytes returned, offending path
    // exposed (for DES-008's user-facing message + forced fresh SSO).
    void groupReadableFileRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("secret-group-readable");
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString dest = GarminTokenStore::tokenFilePath(athlete.path());
        QVERIFY2(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup),
                 "test setup: widen tokens.json to 0640");

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
        QVERIFY2(r.isRejected(), "0640 file must be TokenPermissionsRejected");
        QVERIFY2(r.bytes.isEmpty(), "a rejected file must NOT return its bytes");
        QCOMPARE(r.path, dest); // offending path exposed to the caller
    }

    // Case 3: a tokens.json chmod'd 0644 (world-readable) is likewise refused.
    void worldReadableFileRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("secret-world-readable");
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString dest = GarminTokenStore::tokenFilePath(athlete.path());
        QVERIFY2(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup |
                                                 QFileDevice::ReadOther),
                 "test setup: widen tokens.json to 0644");

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
        QVERIFY2(r.bytes.isEmpty(), "a world-readable file must NOT return its bytes");
        QCOMPARE(r.path, dest);
    }

    // Case 3a (A3-R006-01, mask completeness — WRITE bit class): a tokens.json
    // chmod'd 0620 (ReadOwner|WriteOwner|WriteGroup) has NO Read* group/other
    // bit — only a group-WRITE bit is widened. The refusal mask must still
    // reject it. This kills the M-A1 mutant that narrows the production mask to
    // Read-only bits (ReadGroup|ReadOther): under that mutant 0620 has no masked
    // bit set and would wrongly load as Ok — this assertion then FAILS, killing
    // the mutant.
    void groupWriteOnlyWidenedRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("secret-group-write-only");
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString dest = GarminTokenStore::tokenFilePath(athlete.path());
        QVERIFY2(
            QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::WriteGroup),
            "test setup: widen tokens.json to 0620 (group-write-only)");

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
        QVERIFY2(r.isRejected(), "0620 (group-write-only) file must be TokenPermissionsRejected");
        QVERIFY2(r.bytes.isEmpty(), "a rejected group-write-only file must NOT return its bytes");
        QCOMPARE(r.path, dest); // offending path exposed to the caller
    }

    // Case 3b (A3-R006-01, mask completeness — EXEC bit class): a tokens.json
    // chmod'd 0601 (ReadOwner|WriteOwner|ExeOther) has NO Read*/Write* group or
    // other bit — only an other-EXEC bit is widened. The refusal mask must still
    // reject it. Together with 3a this covers the two bit classes (Write, Exec)
    // that had ZERO coverage, so the Read-only-narrowed mutant cannot survive.
    void otherExecOnlyWidenedRefused()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("secret-other-exec-only");
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString dest = GarminTokenStore::tokenFilePath(athlete.path());
        QVERIFY2(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOther),
                 "test setup: widen tokens.json to 0601 (other-exec-only)");

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
        QVERIFY2(r.isRejected(), "0601 (other-exec-only) file must be TokenPermissionsRejected");
        QVERIFY2(r.bytes.isEmpty(), "a rejected other-exec-only file must NOT return its bytes");
    }

    // Case 6 (A3-R006-02, freshness — M-A4): perms are re-stat'd on EVERY
    // loadChecked() call, never cached across calls (code comment cites
    // A3-R004-M1). Save 0600 → first load is Ok; then widen the SAME file in
    // place to 0640 and load the SAME path AGAIN → it must now be
    // TokenPermissionsRejected. A mutant that caches the first (Ok) perm decision
    // would still return Ok on the second call — this assertion then FAILS,
    // killing the static-cache mutant.
    void permissionsReReadNotCachedAcrossCalls()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("secret-freshness-restat");
        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString dest = GarminTokenStore::tokenFilePath(athlete.path());

        // First call: owner-only 0600 → Ok, exact bytes.
        const GarminTokenStore::LoadResult first = GarminTokenStore::loadChecked(athlete.path());
        QCOMPARE(first.status, GarminTokenStore::LoadStatus::Ok);
        QCOMPARE(first.bytes, blob);

        // Widen the SAME file in place to 0640 (group-readable).
        QVERIFY2(QFile::setPermissions(dest, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup),
                 "test setup: widen SAME tokens.json to 0640 in place");

        // Second call on the SAME path: perms must be re-read → now rejected.
        const GarminTokenStore::LoadResult second = GarminTokenStore::loadChecked(athlete.path());
        QCOMPARE(second.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
        QVERIFY2(second.isRejected(), "widened-in-place file must be rejected on the next load (not cached)");
        QVERIFY2(second.bytes.isEmpty(), "a rejected (re-stat'd) file must NOT return its bytes");
        QCOMPARE(second.path, dest);
    }
#endif // Q_OS_WIN

    // Case 4: an ABSENT tokens.json yields the NotFound outcome — a distinct
    // state from perms-rejected (the caller maps only the latter to a forced
    // re-login; NotFound is the ordinary "no session yet" case).
    void absentFileIsNotFound()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        // Nothing saved — tokens.json does not exist.
        QVERIFY(!QFileInfo::exists(GarminTokenStore::tokenFilePath(athlete.path())));

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::NotFound);
        QVERIFY2(!r.isRejected(), "absent file must NOT be reported as perms-rejected");
        QVERIFY2(!r.isOk(), "absent file must NOT be reported as Ok");
        QVERIFY(r.bytes.isEmpty());
    }

    // Case 5 (anti-mutant guard): the three states are MUTUALLY EXCLUSIVE — a
    // rejected file is never OK and an OK file is never rejected. Kills the
    // trivial mutant that collapses two outcomes into one.
    void threeStatesMutuallyExclusive()
    {
        // OK path.
        QTemporaryDir okDir;
        QVERIFY(okDir.isValid());
        QVERIFY(GarminTokenStore::save(okDir.path(), QByteArrayLiteral("ok-blob")));
        const GarminTokenStore::LoadResult okR = GarminTokenStore::loadChecked(okDir.path());
        QVERIFY2(okR.isOk() && !okR.isRejected(), "OK outcome must not simultaneously be rejected");
        QCOMPARE(okR.status, GarminTokenStore::LoadStatus::Ok);

        // NotFound path.
        QTemporaryDir missingDir;
        QVERIFY(missingDir.isValid());
        const GarminTokenStore::LoadResult nfR = GarminTokenStore::loadChecked(missingDir.path());
        QVERIFY2(!nfR.isOk() && !nfR.isRejected(), "NotFound outcome is neither Ok nor Rejected");

#ifndef Q_OS_WIN
        // Rejected path.
        QTemporaryDir badDir;
        QVERIFY(badDir.isValid());
        QVERIFY(GarminTokenStore::save(badDir.path(), QByteArrayLiteral("bad-blob")));
        QVERIFY(QFile::setPermissions(GarminTokenStore::tokenFilePath(badDir.path()),
                                      QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadOther));
        const GarminTokenStore::LoadResult badR = GarminTokenStore::loadChecked(badDir.path());
        QVERIFY2(badR.isRejected() && !badR.isOk(), "Rejected outcome must not simultaneously be Ok");
        QCOMPARE(badR.status, GarminTokenStore::LoadStatus::TokenPermissionsRejected);
#endif
    }

    // B-STAGE9-13 — a PRESENT, conforming-0600 tokens.json with ZERO bytes must
    // NOT be reported Ok: an empty blob is not a restorable session (nothing
    // downstream can tell "healthy" from "empty" once status==Ok says so).
    // RED against today's loadChecked(), which only checks existence + perms,
    // never content: it returns Ok with empty bytes for exactly this file.
    void emptyConformingFileIsNotReportedOk()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        // save() with an empty blob writes a real, conforming 0600 tokens.json
        // with zero bytes — exactly the live-observed shape (0-byte, 0600).
        QVERIFY(GarminTokenStore::save(athlete.path(), QByteArray()));
        QCOMPARE(QFileInfo(GarminTokenStore::tokenFilePath(athlete.path())).size(), qint64(0));

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        // Assert the EXACT outcome, not merely "not Ok": a counter-fix that maps
        // blank content to TokenPermissionsRejected (wrong message, wrong
        // gc_obs code) would also satisfy "!isOk()" — it must not satisfy this.
        QCOMPARE(r.status, GarminTokenStore::LoadStatus::Empty);
        QVERIFY2(r.isEmpty(), "a present, 0-byte, 0600 token file must be reported Empty");
        QVERIFY2(!r.isOk(), "a present, 0-byte, 0600 token file must not be reported Ok");
        QVERIFY(r.bytes.isEmpty());
    }

    // B-STAGE9-13 — same defect, whitespace-only content: a file that is not
    // literally zero bytes but carries no usable blob (e.g. a truncated write
    // that left only a trailing newline) must be treated the same as empty.
    void whitespaceOnlyConformingFileIsNotReportedOk()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        QVERIFY(GarminTokenStore::save(athlete.path(), QByteArrayLiteral("\n \t\n")));

        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(athlete.path());

        QCOMPARE(r.status, GarminTokenStore::LoadStatus::Empty);
        QVERIFY2(r.isEmpty(), "a present, whitespace-only, 0600 token file must be reported Empty");
        QVERIFY2(!r.isOk(), "a present, whitespace-only, 0600 token file must not be reported Ok");
        QVERIFY(r.bytes.isEmpty());
    }

    // B-STAGE9-13 — the new outcome must be EXACTLY Empty, and distinct from
    // BOTH NotFound and TokenPermissionsRejected: "the file is empty", "there
    // is no file", and "the perms are unsafe" are three different diagnoses
    // (connected-but-persistence-broke / never-connected / unsafe-perms) that
    // must not collapse into each other. In particular this kills the
    // reviewer's counter-implementation (map blank content to
    // TokenPermissionsRejected instead of Empty — still "not Ok", still no
    // bytes, wrong user message and wrong gc_obs code): asserting the exact
    // enum value, not just inequality with NotFound, is what catches it.
    void emptyFileIsDistinctFromNotFoundAndRejected()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        QVERIFY(GarminTokenStore::save(athlete.path(), QByteArray()));

        const GarminTokenStore::LoadResult emptyR = GarminTokenStore::loadChecked(athlete.path());

        QTemporaryDir missingDir;
        QVERIFY(missingDir.isValid());
        const GarminTokenStore::LoadResult nfR = GarminTokenStore::loadChecked(missingDir.path());

        QCOMPARE(emptyR.status, GarminTokenStore::LoadStatus::Empty);
        QVERIFY2(emptyR.isEmpty() && !emptyR.isOk() && !emptyR.isRejected(),
                 "the empty-file outcome must be Empty, and neither Ok nor Rejected");
        QVERIFY2(emptyR.status != nfR.status,
                 "an empty-but-present file must be a DIFFERENT status than an absent one");
        QCOMPARE(nfR.status, GarminTokenStore::LoadStatus::NotFound);
    }
};

QTEST_APPLESS_MAIN(TestGarminTokenStoreLoad)
#include "testGarminTokenStore_load.moc"
