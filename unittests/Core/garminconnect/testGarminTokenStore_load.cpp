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
};

QTEST_APPLESS_MAIN(TestGarminTokenStoreLoad)
#include "testGarminTokenStore_load.moc"
