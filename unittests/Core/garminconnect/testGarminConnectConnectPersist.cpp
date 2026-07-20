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

#include "GarminConnect.h"
#include "GarminSidecarStore.h"
#include "GarminTokenStore.h"
#include "GarminWorker.h"
#include "IGarminAuthClient.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

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

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }
    void listActivities(const QString&, QUuid id) override
    {
        const QVector<GarminActivitySummary> res = listResult;
        QMetaObject::invokeMethod(this, [this, id, res]() { emit activitiesListed(id, res); }, Qt::QueuedConnection);
    }
    void downloadActivity(const QString&, const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(
            this, [this, id]() { emit downloadFailed(id, GarminDownloadFailure{}); }, Qt::QueuedConnection);
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
