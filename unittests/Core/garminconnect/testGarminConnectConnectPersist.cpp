/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-049/T-050 — REQ-008 Slice D: the connect->persist producer wiring
// (closes adversary finding A3-R008-01).
//
// Slices A+B+C made GarminConnect a syncing CloudService, but sync could never
// run in production because the connect-success producer writes were never wired:
//   (1) GarminTokenStore::save() was never called -> tokens.json never written ->
//       open() finds NotFound -> no session -> readdir has no worker session.
//   (2) garmin_user_id was never persisted anywhere resolveGarminUserId() reads.
// Every existing sync unit test passed only because it injects a ctor uid-override
// (m_garminUserIdOverride), masking the gap (LSN-024: verify the producer, not
// just the consumer).
//
// DEC-garmin-018 (Option B): the active garmin_user_id is persisted in a SEPARATE,
// account-agnostic file <athlete-config-dir>/garminconnect/active-account.json =
// {"garmin_user_id": "<uid>"}. tokens.json keeps carrying ONLY the raw garth OAuth
// blob (its schema is security-locked by REQ-006/007). resolveGarminUserId() reads
// active-account.json.
//
//   T-049 — producer unit: on auth-success (tokenBlob + garmin_user_id),
//     GarminTokenStore::persistConnectSuccess writes tokens.json (0600, == the
//     blob) THEN active-account.json (0600, {"garmin_user_id": uid}, no .tmp
//     residue); a FRESH GarminConnect constructed WITHOUT the uid-override then
//     resolves the account from active-account.json (proven via readdir behaviour,
//     NOT the override); a missing active-account.json -> empty uid (graceful
//     no-op); GarminTokenStore::clearAccount (Disconnect) deletes tokens.json +
//     active-account.json but PRESERVES imported-<uid>.json / backfill-state-<uid>.json
//     (REQ-012 / DES-002).
//
//   T-050 — end-to-end (the integration proof A3-R008-01 says is missing): drive a
//     full connect->persist->resolve->readdir round-trip with a REAL (non-override)
//     garmin_user_id. A fake IGarminPyAdapter yields a Success PyAuthOutcome
//     carrying uid + tokenBlob; the REAL GarminWorker maps it to a GarminAuthSuccess
//     that MUST now carry the blob forward; the producer persists both files; a
//     FRESH GarminConnect constructed with NO uid-override resolves the account from
//     active-account.json and readdir returns the scripted activity entries —
//     proving the producer->consumer chain closes end-to-end.
//
// Python-free (garmin-fast): stubs/ReadFileStubPreamble.h stubs CloudService +
// replaces PyEmbeddedAdapter with a Python-free fake; the REAL (pure-Qt)
// GarminTokenStore + GarminSidecarStore + AtomicFile back the disk I/O.

#include "AtomicFile.h"
#include "GarminConnect.h"
#include "GarminSidecarStore.h"
#include "GarminTokenStore.h"
#include "GarminWorker.h"
#include "IGarminAuthClient.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

#ifndef Q_OS_WIN
#    include <sys/stat.h> // T-055 (LSN-031): the inode half of "untouched"
#endif

namespace {

const QString kUid = QStringLiteral("123456789");
// A blob that does NOT contain a top-level "garmin_user_id" — so a reader that
// (wrongly) still parses tokens.json would resolve EMPTY, making the reader-repoint
// behaviourally observable (not accidentally satisfied by the blob's contents).
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

// True when a POSIX mode is owner-only (no group/other bit set) — 0600-family.
bool isOwnerOnly(const QString& path)
{
    const QFileDevice::Permissions perms = QFileInfo(path).permissions();
    const QFileDevice::Permissions groupOther = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                QFileDevice::WriteOther | QFileDevice::ExeOther;
    return (perms & groupOther) == QFileDevice::Permissions();
}

// The two accounts used by the T-055 account-switch test (DES-002: the uid is
// STRUCTURAL — it is embedded in each sidecar's filename).
const QString kUidA = QStringLiteral("111111111");
const QString kUidB = QStringLiteral("222222222");

// Raw bytes of a file, or a null QByteArray when it does not exist. Used to
// prove a prior account's sidecars are byte-for-byte UNTOUCHED (REQ-012).
QByteArray readAllBytes(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray b = f.readAll();
    f.close();
    return b;
}

// Write `bytes` to `path` verbatim (no atomic dance) — used by T-058 to plant the
// `.tmp` siblings AtomicFile::writeOver leaves behind when a crash lands between
// its write and its rename (AtomicFile.cpp: `dest + ".tmp"`).
bool writeFileVerbatim(const QString& path, const QByteArray& bytes)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const bool ok = f.write(bytes) == bytes.size();
    f.close();
    return ok;
}

// Identity of the file INSTANCE currently at `path`. LSN-031 — "untouched" is not
// a byte compare: a rewrite that reproduces the same bytes is still a write. On
// POSIX this is (inode, size, mtime, ctime) at NANOSECOND resolution:
//   * inode  — AtomicFile::writeOver replaces via tmp+rename, so any rewrite through
//              the production writer lands a brand-new inode;
//   * mtime  — catches an in-place rewrite that keeps the inode. Millisecond
//              resolution is NOT enough (a same-millisecond rewrite slips through —
//              measured), hence tv_nsec;
//   * ctime  — metadata change time, so a bare chmod (which touches neither the
//              bytes nor the mtime) is caught here as well as by the mode check.
// An empty return means the file could not be stat'd; the callers assert that as a
// pre-condition so this can never silently degrade into a tautology.
QString fileIdentity(const QString& path)
{
#ifndef Q_OS_WIN
    struct stat st;
    if (::stat(QFile::encodeName(path).constData(), &st) != 0)
        return QString();
#    if defined(Q_OS_MACOS)
    const struct timespec mtim = st.st_mtimespec;
    const struct timespec ctim = st.st_ctimespec;
#    else
    const struct timespec mtim = st.st_mtim;
    const struct timespec ctim = st.st_ctim;
#    endif
    return QStringLiteral("ino=%1 size=%2 mtime=%3.%4 ctime=%5.%6")
        .arg(quint64(st.st_ino))
        .arg(qint64(st.st_size))
        .arg(qint64(mtim.tv_sec))
        .arg(qint64(mtim.tv_nsec))
        .arg(qint64(ctim.tv_sec))
        .arg(qint64(ctim.tv_nsec));
#else
    // Windows: no inode; NTFS timestamps come through QFileInfo (100ns ticks
    // internally, ms via Qt). Weaker than the POSIX identity but still catches a
    // rewrite; the owner-only mode assertions are the ACL story's Phase-2 item.
    const QFileInfo info(path);
    if (!info.exists())
        return QString();
    return QStringLiteral("size=%1 mtime=%2").arg(info.size()).arg(info.lastModified().toMSecsSinceEpoch());
#endif
}

// A minimal but valid FIT header: ".FIT" ASCII at byte offset 8 (DEC-016).
// Mirrors testGarminConnectSync's helper so T-055 can drive a REAL readFile →
// recordImport and observe WHICH per-account sidecar gets created.
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
    return readAllBytes(path);
}

// T-055 (A3-R012-F5) — the ACTION-observing half of "untouched", alongside the
// forensic (bytes/mode/instance) half. EVERY production writer of a token file or
// a sidecar goes through AtomicFile::writeOver, and its test seam sees the tmp
// file each write stages through — so this records the DESTINATION of every atomic
// write while it is installed. It catches what no post-hoc inspection can: a
// byte-identical rewrite of a prior account's sidecar (identical content, and on a
// coarse-grained filesystem clock even an identical mtime — measured on tmpfs).
class RecordingTmpWriter : public AtomicFile::TmpWriter
{
  public:
    QStringList destinations; // the `dest` of every write, tmp suffix stripped

    qint64 write(QFileDevice& f, const QByteArray& bytes) override
    {
        QString dest = f.fileName();
        if (dest.endsWith(QStringLiteral(".tmp")))
            dest.chop(4);
        destinations << dest;
        return AtomicFile::TmpWriter::write(f, bytes);
    }
};

// Uninstall the observer on EVERY exit path — a QVERIFY failure returns from the
// slot, and a globally-installed test writer must never leak into another slot.
struct TmpWriterInstall
{
    explicit TmpWriterInstall(AtomicFile::TmpWriter* w) { AtomicFile::setTmpWriterForTest(w); }
    ~TmpWriterInstall() { AtomicFile::setTmpWriterForTest(nullptr); }
};

} // namespace

// ---------------------------------------------------------------------------
// FakeAuthPyAdapter — Python-free IGarminPyAdapter returning a scripted
// PyAuthOutcome from authenticate(), so the REAL GarminWorker's Success mapping
// (which must now carry tokenBlob forward) is exercisable.
// ---------------------------------------------------------------------------
class FakeAuthPyAdapter : public IGarminPyAdapter
{
  public:
    PyAuthOutcome scripted;

    PyAuthOutcome authenticate(const QString&, const QString&) override { return scripted; }
    PyAuthOutcome submitMfa(const QString&) override { return scripted; }
    PyDownloadOutcome downloadActivity(const QString&, const QString&) override { return {}; }
    PyLoadTokensOutcome loadTokens(const QString&) override { return {}; }
    PyListOutcome listActivitiesSince(const QString&) override { return {}; }
    PyProfileOutcome fetchProfile() override { return {}; }
};

// ---------------------------------------------------------------------------
// FakeSyncClient — same-thread Python-free IGarminDownloadClient scripting a
// listing so readdir can be driven without the worker thread (mirrors
// testGarminConnectSync). Emissions are queued so they arrive while
// GarminConnect's blocking QEventLoop runs.
// ---------------------------------------------------------------------------
class FakeSyncClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    QVector<GarminActivitySummary> listResult;

    // T-054: a reconnect must go through a full SSO — so after a Disconnect the
    // service must not even ATTEMPT a silent session restore. Counting the calls
    // (rather than only checking open()'s bool) makes "returned false but still
    // tried to reauth from leftover state" a detectable defect.
    int restoreCalls = 0;

    // T-056: the "since" basis readdir resolved (verbatim), so the test can prove
    // the pre-disconnect backfill cursor is still what drives the resumed sync.
    QString lastSinceGmt;

    // T-055: per-activity ORIGINAL (ZIP-wrapped) payloads, so a REAL readFile can
    // succeed and drive recordImport into the ACTIVE account's sidecar. Ids with
    // no scripted bytes fail the download (the prior default behaviour).
    QHash<QString, QByteArray> originalBytesById;

    // T-057: "fail closed" means NO network work is issued at all — not merely
    // that the call returned false after talking to Garmin. Counting the ops makes
    // "refused but still downloaded from the disconnected account" detectable.
    int listCalls = 0;
    int downloadCalls = 0;

    void restoreSession(const QString&, QUuid id) override
    {
        ++restoreCalls;
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }
    void listActivities(const QString& sinceGmt, QUuid id) override
    {
        ++listCalls;
        lastSinceGmt = sinceGmt;
        const QVector<GarminActivitySummary> res = listResult;
        QMetaObject::invokeMethod(this, [this, id, res]() { emit activitiesListed(id, res); }, Qt::QueuedConnection);
    }
    void downloadActivity(const QString& activityId, const QString& fmt, QUuid id) override
    {
        ++downloadCalls;
        const QByteArray bytes = originalBytesById.value(activityId);
        QMetaObject::invokeMethod(
            this,
            [this, id, fmt, bytes]() {
                if (fmt == QStringLiteral("ORIGINAL") && !bytes.isEmpty()) {
                    emit downloaded(id, bytes);
                } else {
                    emit downloadFailed(id, GarminDownloadFailure{});
                }
            },
            Qt::QueuedConnection);
    }
};

// ---------------------------------------------------------------------------
// DummyCloudService — a minimal, non-Garmin concrete CloudService used by T-052
// to prove the base CloudService::disconnectService() default is a safe no-op (it
// must NOT touch any token files: only GarminConnect overrides disconnectService()).
// ---------------------------------------------------------------------------
class DummyCloudService : public CloudService
{
  public:
    DummyCloudService() : CloudService(nullptr) {}
    CloudService* clone(Context*) override { return nullptr; }
    QImage logo() const override { return QImage(); }
    QString id() const override { return QStringLiteral("Dummy"); }
};

class TestGarminConnectConnectPersist : public QObject
{
    Q_OBJECT

  private slots:

    // =====================================================================
    // T-049 — producer unit
    // =====================================================================

    // persistConnectSuccess writes tokens.json (== the raw blob, 0600) AND
    // active-account.json ({"garmin_user_id": uid}, 0600, no .tmp residue).
    void persistWritesTokensAndActiveAccountBothOwnerOnly()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        // tokens.json — the raw OAuth blob, verbatim, owner-only.
        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        QVERIFY2(QFileInfo::exists(tokensPath), "tokens.json must be written by the producer");
        QFile tf(tokensPath);
        QVERIFY(tf.open(QIODevice::ReadOnly));
        QCOMPARE(tf.readAll(), kBlob);
        tf.close();
        QVERIFY2(isOwnerOnly(tokensPath), "tokens.json must be owner-only 0600");

        // active-account.json — {"garmin_user_id": uid}, owner-only, no .tmp left.
        const QString aaPath = GarminTokenStore::activeAccountFilePath(tmp.path());
        QVERIFY2(QFileInfo::exists(aaPath), "active-account.json must be written by the producer");
        QFile af(aaPath);
        QVERIFY(af.open(QIODevice::ReadOnly));
        const QJsonDocument doc = QJsonDocument::fromJson(af.readAll());
        af.close();
        QVERIFY2(doc.isObject(), "active-account.json must be a JSON object");
        QCOMPARE(doc.object().value(QStringLiteral("garmin_user_id")).toString(), kUid);
        QVERIFY2(isOwnerOnly(aaPath), "active-account.json must be owner-only 0600");
        QVERIFY2(!QFileInfo::exists(aaPath + QStringLiteral(".tmp")), "no .tmp residue may remain");
    }

    // The account-agnostic reader: loadActiveAccountUserId round-trips the uid.
    void loadActiveAccountUserIdRoundTrips()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), kUid);
    }

    // A FRESH GarminConnect constructed WITHOUT the ctor uid-override resolves the
    // account from active-account.json (LSN-024 — prove the disk read, not the
    // override). Proven behaviourally: readdir proceeds to list (uid resolved)
    // instead of erroring "no connected account".
    void freshGarminConnectResolvesUidFromActiveAccountFileNoOverride()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        client.listResult = {a1};

        // NO uid override (4th ctor arg omitted -> empty) — the uid MUST come from
        // active-account.json on disk.
        GarminConnect gc(nullptr, &client, tmp.path());

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0)->id, QStringLiteral("5001"));
    }

    // A missing active-account.json -> empty uid -> readdir gracefully no-ops with
    // a "no connected account" error (the existing graceful behaviour, DES-010).
    void missingActiveAccountYieldsEmptyUidGraceful()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // Nothing persisted -> no active-account.json.
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), QString());

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        client.listResult = {a1};

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(entries.size(), 0);
        QVERIFY2(!errors.isEmpty(), "a missing active account must surface a graceful error, not a crash");
    }

    // Disconnect (clearAccount) deletes tokens.json + active-account.json but
    // PRESERVES imported-<uid>.json / backfill-state-<uid>.json (REQ-012, DES-002).
    void disconnectDeletesTokensAndActiveAccountButPreservesSidecars()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        // Seed the two per-account sidecars (REQ-012 must not touch these).
        GarminSidecarStore::ImportedEntry seeded;
        seeded.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seeded.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUid, QStringLiteral("AAA"), seeded));
        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        const QString aaPath = GarminTokenStore::activeAccountFilePath(tmp.path());
        const QString importedPath = GarminSidecarStore::importedFilePath(tmp.path(), kUid);
        const QString backfillPath = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid);
        QVERIFY(QFileInfo::exists(tokensPath));
        QVERIFY(QFileInfo::exists(aaPath));
        QVERIFY(QFileInfo::exists(importedPath));
        QVERIFY(QFileInfo::exists(backfillPath));

        QVERIFY(GarminTokenStore::clearAccount(tmp.path()));

        QVERIFY2(!QFileInfo::exists(tokensPath), "Disconnect must delete tokens.json");
        QVERIFY2(!QFileInfo::exists(aaPath), "Disconnect must delete active-account.json");
        QVERIFY2(QFileInfo::exists(importedPath), "Disconnect must PRESERVE imported-<uid>.json (REQ-012)");
        QVERIFY2(QFileInfo::exists(backfillPath), "Disconnect must PRESERVE backfill-state-<uid>.json (REQ-012)");
    }

    // =====================================================================
    // T-052 — REQ-008 (DEC-garmin-019 C): GarminConnect::disconnect() is the
    // PRODUCTION caller of GarminTokenStore::clearAccount, and the base
    // CloudService::disconnect() default is a safe no-op.
    // =====================================================================

    // GarminConnect::disconnect() deletes tokens.json + active-account.json but
    // PRESERVES imported-<uid>.json / backfill-state-<uid>.json (REQ-012). This is
    // the production caller whose absence left the token files on disk after a
    // disconnect (deleteClicked only flipped appsettings flags before this slice).
    void garminConnectDisconnectClearsTokensButPreservesSidecars()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        // Seed the two per-account sidecars (REQ-012 must not touch these).
        GarminSidecarStore::ImportedEntry seeded;
        seeded.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seeded.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUid, QStringLiteral("AAA"), seeded));
        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        const QString aaPath = GarminTokenStore::activeAccountFilePath(tmp.path());
        const QString importedPath = GarminSidecarStore::importedFilePath(tmp.path(), kUid);
        const QString backfillPath = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid);
        QVERIFY(QFileInfo::exists(tokensPath));
        QVERIFY(QFileInfo::exists(aaPath));
        QVERIFY(QFileInfo::exists(importedPath));
        QVERIFY(QFileInfo::exists(backfillPath));

        // Drive the REAL production entry point: GarminConnect::disconnectService().
        // No client is needed (disconnect only touches the on-disk token files);
        // the config dir is the injected override.
        FakeSyncClient client;
        GarminConnect gc(nullptr, &client, tmp.path());
        gc.disconnectService();

        QVERIFY2(!QFileInfo::exists(tokensPath), "disconnect() must delete tokens.json");
        QVERIFY2(!QFileInfo::exists(aaPath), "disconnect() must delete active-account.json");
        QVERIFY2(QFileInfo::exists(importedPath), "disconnect() must PRESERVE imported-<uid>.json (REQ-012)");
        QVERIFY2(QFileInfo::exists(backfillPath), "disconnect() must PRESERVE backfill-state-<uid>.json (REQ-012)");
    }

    // The base CloudService::disconnectService() default is a safe no-op: a
    // non-Garmin service's disconnectService() touches nothing (it must not know or
    // delete any token files). Proven by seeding a tokens.json in a dir and
    // confirming a plain service's disconnectService() leaves it intact.
    void baseCloudServiceDisconnectIsSafeNoOp()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));
        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        QVERIFY(QFileInfo::exists(tokensPath));

        DummyCloudService dummy;
        dummy.disconnectService(); // base default — must do nothing

        QVERIFY2(QFileInfo::exists(tokensPath),
                 "a non-Garmin service's disconnect() default no-op must not delete any token file");
    }

    // =====================================================================
    // T-054 / T-055 / T-056 — REQ-garmin-012, the clauses of the acceptance
    // criterion that the T-049/T-052 slots above do NOT encode:
    //
    //   "Disconnect" deletes the token file (tokens.json) before clearing
    //   in-memory state. Reconnect must perform a full SSO. All per-account
    //   sidecars (imported-<uid>.json, backfill-state-<uid>.json) are preserved
    //   — reconnecting with the same Garmin account resumes its history;
    //   reconnecting with a different account creates/uses that account's own
    //   sidecar files. Prior-account sidecars sit on disk untouched and are not
    //   consulted by the active session.
    //
    //   T-054 — "Reconnect must perform a full SSO."
    //   T-055 — "Prior-account sidecars sit on disk untouched and are not
    //            consulted by the active session" (+ the different-account
    //            creates/uses ITS OWN sidecar clause).
    //   T-056 — "reconnecting with the same Garmin account resumes its history."
    // =====================================================================

    // T-054 — after Disconnect, a FRESH GarminConnect over the same config dir
    // cannot silently reauth: open() returns false with a labelled error AND the
    // client records ZERO restoreSession calls (a false return that still tried
    // to restore leftover state would be a defect). The pre-disconnect open() is
    // asserted first so the post-disconnect failure is attributable to the
    // disconnect, not to a broken fixture.
    void reconnectAfterDisconnectRequiresFullSsoNoSilentReauth()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        // Baseline: while connected, open() restores the stored session exactly once.
        FakeSyncClient connected;
        GarminConnect gcConnected(nullptr, &connected, tmp.path());
        QStringList connectedErrors;
        QVERIFY2(gcConnected.open(connectedErrors), "pre-condition: a connected account must open()");
        QCOMPARE(connectedErrors.size(), 0);
        QCOMPARE(connected.restoreCalls, 1);

        // Disconnect through the REAL production entry point (DEC-garmin-019 C).
        gcConnected.disconnectService();

        // A FRESH service over the SAME config dir — the reconnect attempt.
        FakeSyncClient after;
        GarminConnect gcAfter(nullptr, &after, tmp.path());

        QStringList errors;
        const bool ok = gcAfter.open(errors);

        QVERIFY2(!ok, "after Disconnect open() must fail — reconnect requires a full SSO");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed when there is no stored session");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");
        QVERIFY2(after.restoreCalls == 0, "no silent reauth: NO restoreSession may be attempted after Disconnect");

        // And no session survives: readdir finds no connected account (the active
        // account file went with the tokens) and lists nothing.
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        after.listResult = {a1};
        QStringList readdirErrors;
        QList<CloudServiceEntry*> entries = gcAfter.readdir(QString(), readdirErrors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QVERIFY2(!readdirErrors.isEmpty(), "a disconnected service must surface an error, not a silent session");
    }

    // T-055 — account switch: connect as A (with history), Disconnect, connect as
    // B. A's sidecars must sit on disk BYTE-IDENTICAL and must NOT be consulted by
    // B's session (A's imported id must not be short-circuited for B), and B must
    // create/use ITS OWN imported-<B>.json. Account identity comes through the REAL
    // producer path (persistConnectSuccess -> active-account.json ->
    // resolveGarminUserId): NO ctor uid-override is used (LSN-024).
    void priorAccountSidecarsUntouchedAndNotConsultedAfterAccountSwitch()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        // --- Account A: connected, with an imported activity AAA + a cursor ----
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUidA, kBlob));
        GarminSidecarStore::ImportedEntry seededA;
        seededA.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seededA.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUidA, QStringLiteral("AAA"), seededA));
        GarminSidecarStore::BackfillState stA;
        stA.lastSuccessStartTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUidA, stA));

        const QString importedA = GarminSidecarStore::importedFilePath(tmp.path(), kUidA);
        const QString backfillA = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUidA);
        const QString importedB = GarminSidecarStore::importedFilePath(tmp.path(), kUidB);
        const QByteArray importedABefore = readAllBytes(importedA);
        const QByteArray backfillABefore = readAllBytes(backfillA);
        QVERIFY2(!importedABefore.isEmpty(), "pre-condition: account A's imported sidecar must have content");
        QVERIFY2(!backfillABefore.isEmpty(), "pre-condition: account A's backfill sidecar must have content");

        // A3-R012-F5 / LSN-031 — "untouched" is more than the bytes: capture the
        // on-disk file INSTANCE (mtime + inode) and the mode too, so a rewrite that
        // reproduces the bytes while relaxing the mode or replacing the file is
        // still caught.
        const QString importedAIdBefore = fileIdentity(importedA);
        const QString backfillAIdBefore = fileIdentity(backfillA);
        QVERIFY2(!importedAIdBefore.isEmpty(), "pre-condition: account A's imported sidecar must be stat-able");
        QVERIFY2(!backfillAIdBefore.isEmpty(), "pre-condition: account A's backfill sidecar must be stat-able");
        QVERIFY2(isOwnerOnly(importedA), "pre-condition: account A's imported sidecar starts owner-only 0600");
        QVERIFY2(isOwnerOnly(backfillA), "pre-condition: account A's backfill sidecar starts owner-only 0600");

        // --- Disconnect A through the REAL production entry point --------------
        FakeSyncClient clientA;
        GarminConnect gcA(nullptr, &clientA, tmp.path());
        gcA.disconnectService();

        // A3-R012-F8 — the disconnect above must actually have DONE something; without
        // these the slot would pass unchanged if disconnectService() were a no-op.
        QVERIFY2(!QFileInfo::exists(GarminTokenStore::tokenFilePath(tmp.path())),
                 "Disconnect must have deleted account A's tokens.json");
        QVERIFY2(!QFileInfo::exists(GarminTokenStore::activeAccountFilePath(tmp.path())),
                 "Disconnect must have deleted account A's active-account.json");

        // --- Connect as a DIFFERENT account B ----------------------------------
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUidB, kBlob));
        QVERIFY2(!QFileInfo::exists(importedB), "pre-condition: account B has no sidecar yet");

        // B's listing includes A's already-imported AAA plus a fresh BBB.
        FakeSyncClient clientB;
        GarminActivitySummary aAAA;
        aAAA.activityId = QStringLiteral("AAA");
        aAAA.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        GarminActivitySummary aBBB;
        aBBB.activityId = QStringLiteral("BBB");
        aBBB.startTimeGMT = QStringLiteral("2026-07-05 09:15:00");
        clientB.listResult = {aAAA, aBBB};
        clientB.originalBytesById[QStringLiteral("BBB")] = makeZip(QStringLiteral("BBB.fit"), makeFitBytes());

        // Watch every atomic write B's session performs (A3-R012-F5).
        RecordingTmpWriter observer;
        TmpWriterInstall installed(&observer);

        GarminConnect gcB(nullptr, &clientB, tmp.path()); // NO uid override (LSN-024)
        QStringList errors;
        QList<CloudServiceEntry*> entries = gcB.readdir(QString(), errors, QDateTime(), QDateTime());

        // (a) A's dedup map is NOT consulted: AAA is not short-circuited for B.
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries.at(0)->id, QStringLiteral("AAA"));
        QCOMPARE(entries.at(1)->id, QStringLiteral("BBB"));

        // (c) B's OWN imported-<B>.json is what gets created/used: a real download
        // records into B's sidecar, and B's map holds only B's own history.
        QByteArray data;
        QVERIFY2(gcB.readFile(&data, entries.at(1)->name, entries.at(1)->id), "readFile must stage B's activity");
        QVERIFY2(QFileInfo::exists(importedB), "the active account B must create ITS OWN imported-<B>.json");
        const GarminSidecarStore::ImportedMap mapB = GarminSidecarStore::loadImported(tmp.path(), kUidB);
        QVERIFY(mapB.isOk());
        QVERIFY2(mapB.contains(QStringLiteral("BBB")), "B's download must be recorded in B's sidecar");
        QVERIFY2(!mapB.contains(QStringLiteral("AAA")), "A's history must not leak into B's sidecar");

        // A second sync proves the consulted file is B's own: BBB (in B's map) is
        // now short-circuited while AAA (only in A's map) still passes through.
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gcB.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(errors2.size(), 0);
        QCOMPARE(entries2.size(), 1);
        QCOMPARE(entries2.at(0)->id, QStringLiteral("AAA"));

        // (b) A's sidecars sit on disk UNTOUCHED — byte-identical, not merely present.
        QVERIFY2(QFileInfo::exists(importedA), "prior-account imported-<A>.json must remain on disk");
        QVERIFY2(QFileInfo::exists(backfillA), "prior-account backfill-state-<A>.json must remain on disk");
        QCOMPARE(readAllBytes(importedA), importedABefore);
        QCOMPARE(readAllBytes(backfillA), backfillABefore);

        // ...and UNTOUCHED covers the metadata too (A3-R012-F5 / LSN-031): the mode
        // must still be owner-only (a byte-preserving rewrite that widened it to
        // 0644 would leak A's history to other local users) and the file INSTANCE
        // must be the same one (mtime + inode — every writer here goes through
        // AtomicFile's tmp+rename, so any rewrite lands a NEW inode even when the
        // bytes and the mtime second are identical).
        QVERIFY2(isOwnerOnly(importedA), "prior-account imported-<A>.json must still be owner-only 0600");
        QVERIFY2(isOwnerOnly(backfillA), "prior-account backfill-state-<A>.json must still be owner-only 0600");
        QCOMPARE(fileIdentity(importedA), importedAIdBefore);
        QCOMPARE(fileIdentity(backfillA), backfillAIdBefore);

        // ...and no WRITE was ever aimed at them. The forensic checks above cannot
        // see a byte-identical rewrite; the atomic-writer observer can. The
        // positive control (B's own sidecar WAS seen) is what stops this from
        // passing vacuously if the observer were never invoked (LSN-022).
        QVERIFY2(observer.destinations.contains(importedB),
                 "pre-condition: the atomic-write observer must have seen B's own sidecar write");
        QVERIFY2(!observer.destinations.contains(importedA),
                 "no write may target the prior account's imported-<A>.json during B's session");
        QVERIFY2(!observer.destinations.contains(backfillA),
                 "no write may target the prior account's backfill-state-<A>.json during B's session");
    }

    // T-056 — same-account reconnect resumes that account's history: after a
    // Disconnect and a reconnect as the SAME uid, the already-imported AAA is
    // short-circuited (history resumed), a fresh BBB passes through, and the
    // backfill cursor written BEFORE the disconnect is still the resolved "since"
    // basis. Identity comes from active-account.json — NO ctor override (LSN-024).
    void reconnectSameAccountResumesImportedHistoryAndBackfillCursor()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        // Connected, with one imported activity and a resume cursor.
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));
        GarminSidecarStore::ImportedEntry seeded;
        seeded.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seeded.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUid, QStringLiteral("AAA"), seeded));
        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        // Disconnect, then reconnect with the SAME Garmin account.
        FakeSyncClient clientBefore;
        GarminConnect gcBefore(nullptr, &clientBefore, tmp.path());
        gcBefore.disconnectService();
        QVERIFY2(!QFileInfo::exists(GarminTokenStore::tokenFilePath(tmp.path())),
                 "pre-condition: Disconnect must have deleted tokens.json");
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        FakeSyncClient client;
        GarminActivitySummary aAAA;
        aAAA.activityId = QStringLiteral("AAA");
        aAAA.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        GarminActivitySummary aBBB;
        aBBB.activityId = QStringLiteral("BBB");
        aBBB.startTimeGMT = QStringLiteral("2026-07-05 09:15:00");
        client.listResult = {aAAA, aBBB};

        GarminConnect gc(nullptr, &client, tmp.path()); // NO uid override (LSN-024)
        QStringList errors;
        // An invalid `from` so the "since" basis must come from backfill-state.
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);
        QVERIFY2(entries.at(0)->id == QStringLiteral("BBB"),
                 "history resumed: the pre-disconnect AAA must still be short-circuited");
        // The cursor persisted BEFORE the disconnect still drives the resumed sync.
        QCOMPARE(client.lastSinceGmt, QStringLiteral("2026-07-01 00:00:00"));
    }

    // =====================================================================
    // T-057 / T-058 / T-059 — DEC-garmin-020 (Option C) hardening: the
    // CONSUMING side fails closed once the account is no longer connected,
    // Disconnect sweeps the atomic-writer `.tmp` siblings, and the empty-uid
    // sidecar write guard is pinned.
    //
    //   T-057 — A3-R012-F1 mitigation. disconnectService() deletes the token
    //     files and clears NO in-memory state, and nothing shuts down live
    //     GarminConnect instances (the AddCloudWizard finish-with-sync dialog
    //     holds one open while ConfigDialog -> Accounts -> Delete mints a
    //     SECOND instance and disconnects). readFile/readdir therefore have to
    //     re-check, on EVERY call, that the account is still connected — and
    //     refuse, performing NO network work, once it is not.
    //   T-058 — A3-R012-F2. AtomicFile::writeOver stages through `<dest>.tmp`;
    //     a crash between its write and its rename leaves the COMPLETE OAuth
    //     blob in tokens.json.tmp. Disconnect must sweep those siblings too
    //     (LSN-030: "the file is absent" is not "the secret is gone").
    //   T-059 — A3-R012-F4. Pin recordImport's `uid.isEmpty()` guard: no
    //     unkeyed imported-.json / backfill-state-.json may ever be written.
    // =====================================================================

    // T-057 (a) — the F1 scenario itself: a live, already-open()ed service is
    // disconnected out from under by a SECOND instance (deleteClicked's fresh
    // instance, DEC-garmin-019 C), then asked to download. It must fail closed:
    // readFile returns false, stages nothing and issues NO download; readdir
    // returns no entries, issues NO list and pushes a labelled error.
    void liveInstanceFailsClosedAfterAnotherInstanceDisconnectsTheAccount()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        FakeSyncClient live;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        live.listResult = {a1};
        live.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        // The live service — opened while connected, exactly like the sync dialog's
        // store. NO uid override: identity comes from the real producer (LSN-024).
        GarminConnect gcLive(nullptr, &live, tmp.path());
        QStringList openErrors;
        QVERIFY2(gcLive.open(openErrors), "pre-condition: a connected account must open()");

        // Pre-condition: while still connected THIS instance can enumerate — so the
        // refusal below is attributable to the disconnect, not to a dead fixture.
        QStringList preErrors;
        QList<CloudServiceEntry*> preEntries = gcLive.readdir(QString(), preErrors, QDateTime(), QDateTime());
        QCOMPARE(preErrors.size(), 0);
        QCOMPARE(preEntries.size(), 1);
        QCOMPARE(live.listCalls, 1);

        // A SECOND instance disconnects the account underneath the live one.
        FakeSyncClient other;
        GarminConnect gcOther(nullptr, &other, tmp.path());
        gcOther.disconnectService();
        QVERIFY2(!QFileInfo::exists(GarminTokenStore::tokenFilePath(tmp.path())),
                 "pre-condition: the second instance's Disconnect must have deleted tokens.json");

        // readFile on the STILL-LIVE instance: fail closed.
        const int downloadsBefore = live.downloadCalls;
        QByteArray data;
        QVERIFY2(!gcLive.readFile(&data, preEntries.at(0)->name, preEntries.at(0)->id),
                 "a disconnected account must not serve readFile");
        QCOMPARE(live.downloadCalls, downloadsBefore); // NO download was issued
        QVERIFY2(data.isEmpty(), "nothing may be staged from a disconnected account");
        QCOMPARE(static_cast<CloudService&>(gcLive).readCompleteCount, 0);

        // readdir on the STILL-LIVE instance: fail closed.
        const int listsBefore = live.listCalls;
        QStringList errors;
        QList<CloudServiceEntry*> entries = gcLive.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QCOMPARE(live.listCalls, listsBefore); // NO listing was issued
        QVERIFY2(!errors.isEmpty(), "a disconnected account must surface an error, not a silent empty sync");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");
    }

    // T-057 (b) — the refusal must come from re-checking the CREDENTIAL, not from
    // the pre-existing uid check: here active-account.json (and therefore the
    // resolved uid) is left fully intact and only tokens.json is gone. Without a
    // token re-check readdir would happily list and readFile would happily
    // download for an account whose credential no longer exists.
    void failsClosedWhenTokenFileIsGoneEvenThoughActiveAccountRemains()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        QVERIFY(QFile::remove(GarminTokenStore::tokenFilePath(tmp.path())));
        // The uid still resolves — so the "no connected account" uid check CANNOT
        // be what refuses below.
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), kUid);

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QCOMPARE(client.listCalls, 0);
        QVERIFY2(!errors.isEmpty(), "a missing token file must surface an error");

        QByteArray data;
        QVERIFY2(!gc.readFile(&data, QStringLiteral("garmin-5001.fit"), QStringLiteral("5001")),
                 "a missing token file must refuse readFile");
        QCOMPARE(client.downloadCalls, 0);
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
    }

    // T-057 (c) — REQ-006 alignment: a token file whose mode is WIDER than
    // owner-only is refused on load and forces a fresh SSO in open(); the
    // consuming side must treat that same state as "not connected" rather than
    // downloading with a session whose credential is now world-readable.
    void failsClosedWhenTokenFilePermissionsAreRejected()
    {
#ifdef Q_OS_WIN
        QSKIP("POSIX mode bits: the Windows ACL check is REQ-NF-Pkg-001 Phase-2 territory");
#else
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        QVERIFY2(QFile::setPermissions(tokensPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                                       QFileDevice::ReadGroup | QFileDevice::ReadOther),
                 "pre-condition: the token file must be widened to 0644");
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), kUid); // uid still resolves

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("5001");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QCOMPARE(client.listCalls, 0);
        QVERIFY2(!errors.isEmpty(), "a permission-rejected token file must surface an error");

        QByteArray data;
        QVERIFY2(!gc.readFile(&data, QStringLiteral("garmin-5001.fit"), QStringLiteral("5001")),
                 "a permission-rejected token file must refuse readFile");
        QCOMPARE(client.downloadCalls, 0);
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
#endif
    }

    // T-058 — A3-R012-F2 / LSN-030: Disconnect must also remove the `<path>.tmp`
    // siblings AtomicFile::writeOver stages through. A crash between its write and
    // its rename leaves tokens.json.tmp holding the COMPLETE OAuth blob; deleting
    // only tokens.json leaves that secret on disk forever. The per-account sidecars
    // stay preserved (REQ-012) throughout.
    void disconnectAlsoRemovesAtomicWriterTmpSiblings()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        GarminSidecarStore::ImportedEntry seeded;
        seeded.startTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        seeded.localFilename = QStringLiteral("garmin-AAA.fit");
        QVERIFY(GarminSidecarStore::recordImported(tmp.path(), kUid, QStringLiteral("AAA"), seeded));
        GarminSidecarStore::BackfillState st;
        st.lastSuccessStartTimeGMT = QStringLiteral("2026-07-01 00:00:00");
        QVERIFY(GarminSidecarStore::saveBackfillState(tmp.path(), kUid, st));

        const QString tokensPath = GarminTokenStore::tokenFilePath(tmp.path());
        const QString aaPath = GarminTokenStore::activeAccountFilePath(tmp.path());
        const QString tokensTmp = tokensPath + QStringLiteral(".tmp");
        const QString aaTmp = aaPath + QStringLiteral(".tmp");
        const QString importedPath = GarminSidecarStore::importedFilePath(tmp.path(), kUid);
        const QString backfillPath = GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid);

        // The interrupted-write residue: tokens.json.tmp holds the WHOLE blob.
        QVERIFY(writeFileVerbatim(tokensTmp, kBlob));
        QVERIFY(writeFileVerbatim(aaTmp, QByteArray("{\"garmin_user_id\":\"123456789\"}")));
        QVERIFY(QFileInfo::exists(tokensTmp));
        QVERIFY(QFileInfo::exists(aaTmp));

        FakeSyncClient client;
        GarminConnect gc(nullptr, &client, tmp.path());
        gc.disconnectService(); // the REAL production entry point

        QVERIFY2(!QFileInfo::exists(tokensPath), "Disconnect must delete tokens.json");
        QVERIFY2(!QFileInfo::exists(tokensTmp),
                 "Disconnect must also delete tokens.json.tmp — it holds the complete OAuth blob (LSN-030)");
        QVERIFY2(!QFileInfo::exists(aaPath), "Disconnect must delete active-account.json");
        QVERIFY2(!QFileInfo::exists(aaTmp), "Disconnect must also delete active-account.json.tmp");
        QVERIFY2(QFileInfo::exists(importedPath), "Disconnect must still PRESERVE imported-<uid>.json (REQ-012)");
        QVERIFY2(QFileInfo::exists(backfillPath), "Disconnect must still PRESERVE backfill-state-<uid>.json");
    }

    // T-059 (a) — A3-R012-F4: pin recordImport's `uid.isEmpty()` guard AT ITS OWN
    // LEVEL. The DEC-garmin-018 write ordering (tokens.json THEN
    // active-account.json) has a real crash window in which the credential is
    // valid but the uid is unresolvable; that is the one production state that
    // reaches recordImport with an EMPTY uid once readFile fails closed on the
    // credential. In it the download proceeds (the credential is fine) but NO
    // unkeyed imported-.json / backfill-state-.json may be created.
    void emptyUidNeverWritesAnUnkeyedSidecar()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        // The DEC-018 crash window: tokens.json landed, active-account.json did not.
        QVERIFY(QFile::remove(GarminTokenStore::activeAccountFilePath(tmp.path())));
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), QString());

        FakeSyncClient client;
        client.originalBytesById[QStringLiteral("CCC")] = makeZip(QStringLiteral("CCC.fit"), makeFitBytes());
        GarminConnect gc(nullptr, &client, tmp.path());

        QByteArray data;
        QVERIFY2(gc.readFile(&data, QStringLiteral("garmin-CCC.fit"), QStringLiteral("CCC")),
                 "pre-condition: the credential is still valid, so readFile must reach recordImport");
        QCOMPARE(client.downloadCalls, 1);

        const QString unkeyedImported = GarminSidecarStore::importedFilePath(tmp.path(), QString());
        const QString unkeyedBackfill = GarminSidecarStore::backfillStateFilePath(tmp.path(), QString());
        QVERIFY2(!QFileInfo::exists(unkeyedImported),
                 "an empty uid must never key a sidecar: no imported-.json may be created");
        QVERIFY2(!QFileInfo::exists(unkeyedBackfill),
                 "an empty uid must never key a sidecar: no backfill-state-.json may be created");
        // Nor may it fall back to some other account's file.
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUid)),
                 "an unresolvable uid must record nowhere at all");
    }

    // T-059 (b) — the F1-shaped variant the finding describes: Disconnect lands
    // BETWEEN a successful readdir and the base machinery's readFile. Belt and
    // braces over T-057: the download must be refused AND, either way, no unkeyed
    // sidecar may appear (pre-DEC-020 this path imported bytes that were recorded
    // in no sidecar at all — silently broken dedup).
    void disconnectBetweenReaddirAndReadFileWritesNoSidecarAndDownloadsNothing()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), kUid, kBlob));

        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("DDD");
        a1.startTimeGMT = QStringLiteral("2026-07-14 07:00:00");
        client.listResult = {a1};
        client.originalBytesById[QStringLiteral("DDD")] = makeZip(QStringLiteral("DDD.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);

        // Disconnect lands here — mid-sync.
        FakeSyncClient other;
        GarminConnect gcOther(nullptr, &other, tmp.path());
        gcOther.disconnectService();

        QByteArray data;
        QVERIFY2(!gc.readFile(&data, entries.at(0)->name, entries.at(0)->id),
                 "a mid-sync Disconnect must stop the pending download");
        QCOMPARE(client.downloadCalls, 0);
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), QString())),
                 "no unkeyed imported-.json may be created by a mid-sync Disconnect");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(tmp.path(), QString())),
                 "no unkeyed backfill-state-.json may be created by a mid-sync Disconnect");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUid)),
                 "nothing may be recorded for the disconnected account either");
    }

    // =====================================================================
    // T-050 — end-to-end connect->persist->resolve->readdir round-trip
    // =====================================================================
    void endToEndConnectPersistResolveReaddirWithRealUid()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        // 1) Simulate auth-success through the REAL worker: the fake adapter yields
        //    a Success outcome carrying uid + blob; the worker must carry the blob
        //    forward on GarminAuthSuccess (the field this slice adds).
        FakeAuthPyAdapter adapter;
        adapter.scripted.kind = PyAuthOutcome::Success;
        adapter.scripted.garmin_user_id = kUid;
        adapter.scripted.display_name = QStringLiteral("Test Athlete");
        adapter.scripted.tokenBlob = QString::fromUtf8(kBlob);

        GarminWorker worker(&adapter);
        QSignalSpy finishedSpy(&worker, &GarminWorker::finished);
        worker.authenticate(QStringLiteral("e@x.com"), QStringLiteral("pw"), QUuid::createUuid());
        QCOMPARE(finishedSpy.count(), 1);

        const GarminAuthSuccess success = qvariant_cast<GarminAuthSuccess>(finishedSpy.at(0).at(1));
        QCOMPARE(success.garmin_user_id, kUid);
        QCOMPARE(success.tokenBlob, QString::fromUtf8(kBlob)); // blob carried forward

        // 2) Producer persists both files from the auth-success payload.
        QVERIFY(
            GarminTokenStore::persistConnectSuccess(tmp.path(), success.garmin_user_id, success.tokenBlob.toUtf8()));

        // Prove the blob reached tokens.json verbatim (the consumer open() path).
        QFile tf(GarminTokenStore::tokenFilePath(tmp.path()));
        QVERIFY(tf.open(QIODevice::ReadOnly));
        QCOMPARE(tf.readAll(), kBlob);
        tf.close();

        // 3) A FRESH GarminConnect with NO uid-override resolves the account from
        //    active-account.json and readdir returns the scripted activity entries
        //    (producer -> consumer chain closes; this is the proof the ctor-override
        //    unit tests could not give — LSN-024 / A3-R008-01).
        FakeSyncClient client;
        GarminActivitySummary a1;
        a1.activityId = QStringLiteral("9001");
        a1.startTimeGMT = QStringLiteral("2026-07-15 06:30:00");
        GarminActivitySummary a2;
        a2.activityId = QStringLiteral("9002");
        a2.startTimeGMT = QStringLiteral("2026-07-16 12:00:00");
        client.listResult = {a1, a2};

        GarminConnect gc(nullptr, &client, tmp.path()); // NO override
        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());

        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 2);
        QCOMPARE(entries.at(0)->id, QStringLiteral("9001"));
        QCOMPARE(entries.at(1)->id, QStringLiteral("9002"));
    }
};

QTEST_MAIN(TestGarminConnectConnectPersist)
#include "testGarminConnectConnectPersist.moc"
