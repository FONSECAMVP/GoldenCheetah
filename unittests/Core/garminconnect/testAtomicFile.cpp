/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-011 — REQ-004 / REQ-NF-Reliab-002 / REQ-NF-Sec-002: AtomicFile::writeOver
// (DES-006). Pure-Qt helper, Python-free — carries the `garmin-fast` CTest
// label (see CMakeLists.txt). Encodes the acceptance criterion literally:
//
//   - tmp+rename: writes <dest>.tmp in the SAME dir, then atomically renames
//     over dest; a successful write leaves NO .tmp residue.
//   - owner-only perms are set on the tmp file BEFORE the rename, so dest is
//     never briefly group/other-readable (REQ-NF-Sec-002 — 0600 POSIX). The
//     observable invariant asserted here is that the destination is never
//     group/other readable or writable.
//   - a failed write returns false and NEVER corrupts/replaces a pre-existing
//     good dest (REQ-NF-Reliab-002 — the previous good file survives torn
//     writes), and cleans up any tmp it created.

#include "AtomicFile.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

class TestAtomicFile : public QObject
{
    Q_OBJECT

  private:
    // True iff any group/other read/write/exec bit is set — i.e. the file is
    // wider than owner-only. Owner-only (0600-style) must have all of these
    // clear regardless of Qt's ReadOwner/ReadUser bit duplication on Unix.
    static bool isWiderThanOwnerOnly(QFileDevice::Permissions p)
    {
        const QFileDevice::Permissions groupOther = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                    QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                    QFileDevice::WriteOther | QFileDevice::ExeOther;
        return (p & groupOther) != QFileDevice::Permissions();
    }

  private slots:

    // Acceptance: contents land at dest; no .tmp residue; owner-only perms.
    void writesContents_ownerOnly_noTmpResidue()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = dir.filePath("tokens.json");
        const QByteArray payload = QByteArrayLiteral("{\"oauth2\":\"blob\"}");

        QVERIFY(AtomicFile::writeOver(dest, payload));

        QFile f(dest);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), payload);
        f.close();

        // REQ-NF-Sec-002 — destination must be owner-only (0600), never wider.
        QVERIFY2(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()),
                 "tokens.json must be owner-only (0600) — no group/other access");

        // REQ-NF-Reliab-002 — tmp+rename left no <dest>.tmp behind.
        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "successful write must leave no .tmp residue");
    }

    // Atomic replace of a pre-existing dest: new content wins, still owner-only,
    // still no residue. (Proves rename-over-dest, not a separate-path write.)
    void overwritesExistingAtomically()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = dir.filePath("tokens.json");

        QVERIFY(AtomicFile::writeOver(dest, QByteArrayLiteral("OLD")));
        QVERIFY(AtomicFile::writeOver(dest, QByteArrayLiteral("NEWER-CONTENT")));

        QFile f(dest);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("NEWER-CONTENT"));
        f.close();

        QVERIFY(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()));
        QVERIFY(!QFileInfo::exists(dest + ".tmp"));
    }

    // The perms argument is honored when a caller asks for something other than
    // the default — proves writeOver actually applies `perms`, and that the
    // owner-only default is not hard-coded.
    void honorsRequestedPerms()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = dir.filePath("widish");
        const QFileDevice::Permissions want = QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ReadGroup;

        QVERIFY(AtomicFile::writeOver(dest, QByteArrayLiteral("x"), want));

#ifndef Q_OS_WIN
        // On POSIX the group-read bit must be observable; owner bits too.
        const QFileDevice::Permissions got = QFileInfo(dest).permissions();
        QVERIFY(got.testFlag(QFileDevice::ReadGroup));
        QVERIFY(got.testFlag(QFileDevice::ReadOwner));
        QVERIFY(!got.testFlag(QFileDevice::WriteGroup));
#endif
    }

    // REQ-NF-Sec-002 (A3-R004 M1 — order invariant) — perms MUST be tightened
    // to owner-only on the still-empty tmp BEFORE the first secret byte is
    // written, so `dest` is never even briefly group/other-readable. A post-hoc
    // check of the final perms/content CANNOT see this order (both orders end
    // identical — which is exactly why the perms/write swap regression survived
    // the assertions above). We instead inject a recording TmpWriter double
    // (AtomicFile's test seam) that captures the true runtime order of the two
    // steps and assert perms-set precedes the byte-write. This FAILS ("WP") if
    // setPermissions is moved after the write — killing the swap mutant.
    void setsPermsBeforeWritingBytes_orderInvariant()
    {
        struct RecordingWriter : AtomicFile::TmpWriter
        {
            QString order;
            bool setPermissions(QFileDevice& f, QFileDevice::Permissions p) override
            {
                order += QLatin1Char('P');
                return AtomicFile::TmpWriter::setPermissions(f, p);
            }
            qint64 write(QFileDevice& f, const QByteArray& bytes) override
            {
                order += QLatin1Char('W');
                return AtomicFile::TmpWriter::write(f, bytes);
            }
        } rec;

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = dir.filePath("tokens.json");

        AtomicFile::setTmpWriterForTest(&rec);
        const bool ok = AtomicFile::writeOver(dest, QByteArrayLiteral("secret-bytes"));
        AtomicFile::setTmpWriterForTest(nullptr); // always restore before asserting

        QVERIFY(ok);
        // Perms are set (P) before any secret byte is written (W): exactly "PW".
        // The swap regression records "WP" and fails here.
        QCOMPARE(rec.order, QStringLiteral("PW"));

        // Sanity: the write still landed correctly through the seam.
        QFile f(dest);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), QByteArray("secret-bytes"));
        f.close();
        QVERIFY(!isWiderThanOwnerOnly(QFileInfo(dest).permissions()));
    }

#ifndef Q_OS_WIN
    // REQ-NF-Reliab-002 — a failed write returns false and leaves the previous
    // good dest byte-for-byte intact (no torn/partial destination), with no tmp
    // residue. Simulated by revoking write on the containing dir so the tmp
    // cannot be created; the pre-existing dest must survive untouched.
    void failedWrite_returnsFalse_leavesDestIntact()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString dest = dir.filePath("tokens.json");
        const QByteArray good = QByteArrayLiteral("GOOD-PREVIOUS-TOKENS");

        QVERIFY(AtomicFile::writeOver(dest, good));

        // Revoke write+exec-less: strip owner write so a new tmp cannot be
        // created in this directory. (Running as root would bypass this; CI
        // and dev run unprivileged.)
        QFile dirHandle(dir.path());
        const QFileDevice::Permissions restore = QFileInfo(dir.path()).permissions();
        QVERIFY(dirHandle.setPermissions(QFileDevice::ReadOwner | QFileDevice::ExeOwner));

        const bool ok = AtomicFile::writeOver(dest, QByteArrayLiteral("SHOULD-NOT-LAND"));

        // Restore dir perms first so QTemporaryDir can clean up regardless.
        dirHandle.setPermissions(restore);

        QVERIFY2(!ok, "writeOver must return false when it cannot create its tmp file");

        QFile f(dest);
        QVERIFY(f.open(QIODevice::ReadOnly));
        QCOMPARE(f.readAll(), good); // previous good file byte-for-byte intact
        f.close();

        QVERIFY2(!QFileInfo::exists(dest + ".tmp"), "a failed write must not leave a .tmp residue");
    }
#endif
};

QTEST_APPLESS_MAIN(TestAtomicFile)
#include "testAtomicFile.moc"
