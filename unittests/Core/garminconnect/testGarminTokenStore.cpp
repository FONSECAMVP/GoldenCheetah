/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-012 — REQ-004 write path (DES-002): GarminTokenStore. Pure-Qt storage
// layer, Python-free — carries the `garmin-fast` CTest label. Encodes REQ-004's
// acceptance criterion literally:
//
//   "Tokens written to <athlete-config-dir>/garminconnect/tokens.json (atomic
//    tmp+rename). If the parent directory does not exist, it is created with
//    mode 0700 (POSIX). Two athletes on one GC install have independent tokens."
//   + REQ-NF-Sec-002 (token file mode 0600 POSIX)
//   + REQ-NF-Reliab-002 (tmp-and-rename; no torn/partial dest)
//   + DES-002 (an EXISTING garminconnect/ dir's perms are NOT tightened).

#include "GarminTokenStore.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestGarminTokenStore : public QObject
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

    // Acceptance: file at <athlete>/garminconnect/tokens.json; parent dir
    // created 0700; file 0600; no .tmp residue (atomic tmp+rename).
    void writesTokensJson_creates0700Dir_file0600()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QByteArray blob = QByteArrayLiteral("{\"oauth2\":\"opaque-blob\"}");

        QVERIFY(GarminTokenStore::save(athlete.path(), blob));

        const QString subdir = athlete.filePath("garminconnect");
        const QString dest = subdir + "/tokens.json";

        // Path shape is exactly per DEC-003 / REQ-004 (singular tokens.json).
        QCOMPARE(GarminTokenStore::tokenFilePath(athlete.path()), dest);

        // Parent dir was created and is owner-only 0700 (POSIX).
        QVERIFY2(QFileInfo(subdir).isDir(), "garminconnect/ parent dir must be created");
#ifndef Q_OS_WIN
        const QFileDevice::Permissions dperms = QFileInfo(subdir).permissions();
        QVERIFY2(dperms.testFlag(QFileDevice::ReadOwner) && dperms.testFlag(QFileDevice::WriteOwner) &&
                     dperms.testFlag(QFileDevice::ExeOwner),
                 "created garminconnect/ must be owner rwx (0700)");
        QVERIFY2(!isWiderThanOwnerOnly(dperms), "created garminconnect/ must be 0700 — no group/other access");
#endif

        // File content and 0600 perms.
        QFile f(dest);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), blob);
        f.close();
#ifndef Q_OS_WIN
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "tokens.json must be owner-only 0600 — no group/other access (REQ-NF-Sec-002)");
#endif

        // Atomic tmp+rename left no residue.
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "atomic write must leave no .tmp residue");
    }

    // REQ-004: two athletes on one GC install have INDEPENDENT tokens. Writing
    // athlete B must not perturb athlete A's file, and vice-versa.
    void twoAthletesHaveIndependentTokens()
    {
        QTemporaryDir a;
        QTemporaryDir b;
        QVERIFY(a.isValid() && b.isValid());

        const QByteArray blobA = QByteArrayLiteral("AAAA-athlete-a-tokens");
        const QByteArray blobB = QByteArrayLiteral("BBBB-athlete-b-tokens-different-length");

        QVERIFY(GarminTokenStore::save(a.path(), blobA));
        QVERIFY(GarminTokenStore::save(b.path(), blobB));

        // Re-save A to ensure ordering does not cross-contaminate.
        QVERIFY(GarminTokenStore::save(a.path(), blobA));

        bool okA = false;
        bool okB = false;
        const QByteArray readA = GarminTokenStore::load(a.path(), &okA);
        const QByteArray readB = GarminTokenStore::load(b.path(), &okB);

        QVERIFY(okA && okB);
        QCOMPARE(readA, blobA);
        QCOMPARE(readB, blobB);
        QVERIFY2(readA != readB, "two athletes must have fully independent token files");
        QVERIFY(GarminTokenStore::tokenFilePath(a.path()) != GarminTokenStore::tokenFilePath(b.path()));
    }

#ifndef Q_OS_WIN
    // DES-002: if garminconnect/ ALREADY exists, its perms are NOT tightened —
    // the store only sets 0700 when it creates the dir. A user who deliberately
    // widened the dir keeps their choice; only the token file is forced 0600.
    void existingDirPermsNotTightened()
    {
        QTemporaryDir athlete;
        QVERIFY(athlete.isValid());
        const QString subdir = athlete.filePath("garminconnect");
        QVERIFY(QDir(athlete.path()).mkdir("garminconnect"));

        // Widen it to 0755 before the store ever touches it.
        QFile dirHandle(subdir);
        QVERIFY(dirHandle.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                         QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                         QFileDevice::ExeOther));

        QVERIFY(GarminTokenStore::save(athlete.path(), QByteArrayLiteral("x")));

        const QFileDevice::Permissions dperms = QFileInfo(subdir).permissions();
        QVERIFY2(dperms.testFlag(QFileDevice::ReadGroup) && dperms.testFlag(QFileDevice::ReadOther),
                 "an EXISTING garminconnect/ dir must keep its (wider) perms — DES-002 does not tighten it");

        // …but the token file itself is still forced owner-only 0600.
        const QString dest = subdir + "/tokens.json";
        QVERIFY(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()));
    }
#endif
};

QTEST_APPLESS_MAIN(TestGarminTokenStore)
#include "testGarminTokenStore.moc"
