/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-060 / TEST-061 / TEST-062 / TEST-063 — REQ-017 Slice A
// (DEC-garmin-021 Option B): a GarminConnect session is bound to the ACCOUNT it
// was opened against by an in-memory account EPOCH, not by a disk re-check.
//
// Why this exists on top of DEC-garmin-020's fail-closed guard (T-057):
// accountStillConnected() asks the DISK "is some account connected right now?".
// That is a different question from "is the account this live session was opened
// against still the connected one?". Delete-then-reconnect (or any future change
// that neutralises the disk predicate) answers the first question YES while the
// second is NO — and the A3-R012-F1 exploit rides straight through. DEC-021
// Option B binds the session with an int:
//
//   * GarminAccountEpoch::current(configDir) is latched at session open
//     (alongside the garmin_user_id);
//   * GarminConnect::disconnectService() bumps that config dir's epoch;
//   * readdir()/readFile() compare the latched epoch to the current one — an int
//     compare, ZERO disk I/O — IN ADDITION TO (never instead of) the DEC-020
//     token re-check.
//
//   TEST-060 — clause (a). Two GarminConnect instances over the SAME config dir
//     (the AddCloudWizard finish-with-sync dialog holding one open while
//     ConfigDialog -> Accounts -> Delete mints a second — DEC-garmin-019 C).
//     (a1) the plain exploit: the live instance's next readdir/readFile fails
//          with a Garmin-labelled error and issues ZERO list/download calls;
//     (a2) THE POINT OF THIS SLICE — the same proof with tokens.json left VALID
//          (the account is reconnected right after the disconnect), so
//          accountStillConnected() is TRUE and DEC-020's guard is effectively
//          neutralised. Only the epoch can refuse. A test that only passes
//          because the token file vanished proves nothing new (a1 is the pin,
//          a2 is the requirement).
//
//   TEST-061 — clause (d) / A3-R012-F10: the garmin_user_id is latched at session
//     open and used by readdir/readFile/recordImport for the life of the session.
//     A mid-sync rewrite of active-account.json to a DIFFERENT uid must not
//     redirect the dedup read or the import record.
//
//   TEST-062 — clause (c): a download whose result lands AFTER the disconnect has
//     that result discarded — neither staged nor recorded in any sidecar — via a
//     post-download, pre-stage recheck. The disconnect is driven from inside the
//     download callback, i.e. while GarminConnect's nested QEventLoop is pumping,
//     which is exactly how a GUI-thread disconnect lands mid-frame in production.
//     True mid-flight cancellation is explicitly OUT of scope (DEC-021): the HTTP
//     call runs to completion and only its RESULT is dropped — so these slots
//     assert downloadCalls == 1, not 0. The recheck also has to sit BEFORE the
//     DEC-016 TCX retry, which is a second network request that clause (a) says a
//     superseded session may not issue at all.
//
//   TEST-063 — epoch isolation: the epoch map is process-global and keyed by
//     config dir, so bumping athlete A's epoch must NOT invalidate a live session
//     on athlete B's config dir. Both halves are driven with the token files left
//     VALID, so neither the refusal (A) nor the survival (B) can be attributed to
//     DEC-020's disk predicate.
//
// Python-free (garmin-fast): stubs/ReadFileStubPreamble.h stubs CloudService and
// replaces PyEmbeddedAdapter with a Python-free fake; the REAL (pure-Qt)
// GarminTokenStore + GarminSidecarStore + AtomicFile back the disk I/O and real
// contrib/qzip unwraps the FIT ORIGINAL.
//
// This is a SEPARATE executable (not a slot in testGarminConnectConnectPersist)
// on purpose: the epoch map is process-global static state, so a dedicated
// process keeps these slots' expectations independent of every other suite's.

#include "GarminConnect.h"
#include "GarminSidecarStore.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

#include <functional>

namespace {

const QString kUid = QStringLiteral("123456789");
const QString kUidA = QStringLiteral("111111111");
const QString kUidB = QStringLiteral("222222222");

// A blob with no top-level "garmin_user_id", so a reader that (wrongly) still
// parses tokens.json for the account id resolves EMPTY rather than accidentally
// resolving the right thing.
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

QByteArray readAllBytes(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray b = f.readAll();
    f.close();
    return b;
}

bool writeFileVerbatim(const QString& path, const QByteArray& bytes)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    const bool ok = f.write(bytes) == bytes.size();
    f.close();
    return ok;
}

// A minimal but valid FIT header: ".FIT" ASCII at byte offset 8 (DEC-016).
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
// Garmin ORIGINAL download arrives.
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

GarminActivitySummary summary(const QString& id, const QString& startTimeGMT)
{
    GarminActivitySummary s;
    s.activityId = id;
    s.startTimeGMT = startTimeGMT;
    return s;
}

} // namespace

// ---------------------------------------------------------------------------
// FakeEpochClient — same-thread, Python-free IGarminDownloadClient. Counts the
// network ops (so "refused but still talked to Garmin" is detectable rather than
// invisible) and exposes a hook that runs INSIDE the queued download callback,
// immediately before the result is delivered — the seam TEST-062 uses to land a
// disconnect between "the HTTP call completed" and "the result is staged".
// ---------------------------------------------------------------------------
class FakeEpochClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    QVector<GarminActivitySummary> listResult;
    QHash<QString, QByteArray> originalBytesById;

    int restoreCalls = 0;
    int listCalls = 0;
    int downloadCalls = 0;

    // Runs on the caller's event loop, inside the download completion callback,
    // just before downloaded()/downloadFailed() is emitted.
    std::function<void()> onBeforeDownloadResult;

    void restoreSession(const QString&, QUuid id) override
    {
        ++restoreCalls;
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        ++listCalls;
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
                if (onBeforeDownloadResult)
                    onBeforeDownloadResult();
                if (fmt == QStringLiteral("ORIGINAL") && !bytes.isEmpty()) {
                    emit downloaded(id, bytes);
                } else {
                    emit downloadFailed(id, GarminDownloadFailure{});
                }
            },
            Qt::QueuedConnection);
    }
};

class TestGarminConnectAccountEpoch : public QObject
{
    Q_OBJECT

  private:
    // Every slot needs a connected account on disk before it can open() —
    // DEC-garmin-020's guard is still in force and is deliberately NOT removed.
    static bool connectAccount(const QString& dir, const QString& uid)
    {
        return GarminTokenStore::persistConnectSuccess(dir, uid, kBlob);
    }

  private slots:

    // =====================================================================
    // TEST-060 — clause (a): the A3-R012-F1 exploit downloads NOTHING
    // =====================================================================

    // (a1) The plain two-instance exploit, as a pin. A live, already-open()ed
    // service is disconnected out from under by a SECOND instance over the same
    // config dir; its next readdir/readFile must refuse, with a Garmin-labelled
    // error and ZERO list/download calls. (DEC-020's guard alone already satisfies
    // this one — it is here so the scenario is pinned in this suite; (a2) is the
    // clause the epoch actually buys.)
    void liveInstanceDownloadsNothingAfterAnotherInstanceDisconnects()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUid));

        FakeEpochClient live;
        live.listResult = {summary(QStringLiteral("5001"), QStringLiteral("2026-07-14 07:00:00"))};
        live.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        GarminConnect gcLive(nullptr, &live, tmp.path()); // NO uid override (LSN-024)
        QStringList openErrors;
        QVERIFY2(gcLive.open(openErrors), "pre-condition: a connected account must open()");

        // Pre-condition: while connected THIS instance enumerates, so the refusal
        // below is attributable to the disconnect and not to a dead fixture.
        QStringList preErrors;
        QList<CloudServiceEntry*> preEntries = gcLive.readdir(QString(), preErrors, QDateTime(), QDateTime());
        QCOMPARE(preErrors.size(), 0);
        QCOMPARE(preEntries.size(), 1);
        QCOMPARE(live.listCalls, 1);

        // The second instance (deleteClicked's freshly-minted one) disconnects.
        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        gcOther.disconnectService();

        const int listsBefore = live.listCalls;
        const int downloadsBefore = live.downloadCalls;

        QStringList errors;
        QList<CloudServiceEntry*> entries = gcLive.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QCOMPARE(live.listCalls, listsBefore);
        QVERIFY2(!errors.isEmpty(), "a superseded session must surface an error, not a silent empty sync");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");

        QByteArray data;
        QVERIFY2(!gcLive.readFile(&data, QStringLiteral("garmin-5001.fit"), QStringLiteral("5001")),
                 "a superseded session must not serve readFile");
        QCOMPARE(live.downloadCalls, downloadsBefore);
        QVERIFY2(data.isEmpty(), "nothing may be staged by a superseded session");
        QCOMPARE(static_cast<CloudService&>(gcLive).readCompleteCount, 0);
    }

    // (a2) REQ-017(a) proper: the SAME exploit with DEC-garmin-020's guard
    // neutralised. The account is disconnected and immediately reconnected, so
    // tokens.json is present and acceptable again — accountStillConnected() is
    // TRUE and cannot be what refuses. The live instance was opened against the
    // PREVIOUS account session, so its epoch is stale and it must still download
    // nothing.
    void liveInstanceDownloadsNothingEvenWhenTheTokenFileIsValidAgain()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUid));

        FakeEpochClient live;
        live.listResult = {summary(QStringLiteral("5001"), QStringLiteral("2026-07-14 07:00:00"))};
        live.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        GarminConnect gcLive(nullptr, &live, tmp.path()); // NO uid override (LSN-024)
        QStringList openErrors;
        QVERIFY2(gcLive.open(openErrors), "pre-condition: a connected account must open()");

        QStringList preErrors;
        QList<CloudServiceEntry*> preEntries = gcLive.readdir(QString(), preErrors, QDateTime(), QDateTime());
        QCOMPARE(preErrors.size(), 0);
        QCOMPARE(preEntries.size(), 1);
        QCOMPARE(live.listCalls, 1);

        // Disconnect through the REAL production entry point on a SECOND instance...
        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        gcOther.disconnectService();
        // ...and then reconnect, so the credential is valid again. THIS is the
        // "accountStillConnected() neutralised" state REQ-017(a) demands: the disk
        // predicate now answers "connected", so only the in-memory binding is left
        // to refuse.
        QVERIFY(connectAccount(tmp.path(), kUid));
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "pre-condition: DEC-020's disk predicate is SATISFIED — it cannot be what refuses below");

        const int listsBefore = live.listCalls;
        const int downloadsBefore = live.downloadCalls;

        QStringList errors;
        QList<CloudServiceEntry*> entries = gcLive.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(entries.size(), 0);
        QCOMPARE(live.listCalls, listsBefore); // ZERO list calls
        QVERIFY2(!errors.isEmpty(), "a superseded session must surface an error, not a silent empty sync");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");

        QByteArray data;
        QVERIFY2(!gcLive.readFile(&data, QStringLiteral("garmin-5001.fit"), QStringLiteral("5001")),
                 "a superseded session must not serve readFile even when the account is connected again");
        QCOMPARE(live.downloadCalls, downloadsBefore); // ZERO download calls
        QVERIFY2(data.isEmpty(), "nothing may be staged by a superseded session");
        QCOMPARE(static_cast<CloudService&>(gcLive).readCompleteCount, 0);

        // ...and nothing was recorded against the reconnected account either.
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUid)),
                 "a superseded session must record nothing at all");
    }

    // =====================================================================
    // TEST-061 — clause (d) / A3-R012-F10: the uid is latched at session open
    // =====================================================================

    // active-account.json is rewritten to a DIFFERENT uid mid-sync while the
    // credential (tokens.json) is untouched — so the session stays valid and the
    // ONLY thing that moved is the account pointer. readdir's dedup read and
    // recordImport must both keep using the OPEN-TIME uid: the import lands in
    // imported-<A>.json and NOTHING is ever keyed on B.
    void uidLatchedAtOpenSurvivesMidSyncActiveAccountRewrite()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUidA));

        FakeEpochClient client;
        client.listResult = {summary(QStringLiteral("AAA"), QStringLiteral("2026-07-05 09:15:00"))};
        client.originalBytesById[QStringLiteral("AAA")] = makeZip(QStringLiteral("AAA.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path()); // NO uid override (LSN-024)
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");

        QStringList errors;
        QList<CloudServiceEntry*> entries = gc.readdir(QString(), errors, QDateTime(), QDateTime());
        QCOMPARE(errors.size(), 0);
        QCOMPARE(entries.size(), 1);

        // --- the mid-sync account change (nothing else is touched) -------------
        const QString aaPath = GarminTokenStore::activeAccountFilePath(tmp.path());
        QVERIFY(writeFileVerbatim(aaPath, QByteArray("{\"garmin_user_id\":\"") + kUidB.toUtf8() + "\"}"));
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), kUidB);
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "pre-condition: the credential is untouched, so the session stays usable");

        // readFile still succeeds (same credential, same session)...
        QByteArray data;
        QVERIFY2(gc.readFile(&data, entries.at(0)->name, entries.at(0)->id),
                 "the session is still valid — the download must proceed");
        QCOMPARE(client.downloadCalls, 1);
        QCOMPARE(data, makeFitBytes());

        // DEC-083 (B-STAGE9-133/-134): the cursor is a completeness watermark
        // now advanced ONLY by promotion, so recordImport's download-time
        // write leaves it untouched.
        const GarminSidecarStore::BackfillLoadResult bfPending =
            GarminSidecarStore::loadBackfillState(tmp.path(), kUidA);
        QVERIFY(bfPending.isOk());
        QVERIFY2(bfPending.state.lastSuccessStartTimeGMT.isEmpty(), "T-134: download time must not advance the cursor");

        // B-STAGE9-132/-134 — DEC-080/B-STAGE9-111: readFile alone leaves AAA
        // pending; promotion is what moves it into the imported map this test
        // asserts against.
        gc.rideRegistrationCompleted(QStringLiteral("garmin-AAA.fit"));

        // DEC-083 clause 2 — promotion is what advances the cursor, to the
        // promoted entry's own startTimeGMT.
        const GarminSidecarStore::BackfillLoadResult bfA = GarminSidecarStore::loadBackfillState(tmp.path(), kUidA);
        QVERIFY(bfA.isOk());
        QCOMPARE(bfA.state.lastSuccessStartTimeGMT, QStringLiteral("2026-07-05 09:15:00"));

        // ...and the import is recorded under the OPEN-TIME account.
        const GarminSidecarStore::ImportedMap mapA = GarminSidecarStore::loadImported(tmp.path(), kUidA);
        QVERIFY(mapA.isOk());
        QVERIFY2(mapA.contains(QStringLiteral("AAA")),
                 "the import must be recorded under the uid latched at session open");
        QCOMPARE(mapA.value(QStringLiteral("AAA")).localFilename, QStringLiteral("garmin-AAA.fit"));

        // NOTHING may be keyed on the account that appeared mid-sync.
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUidB)),
                 "no import may be recorded against an account that appeared mid-sync");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(tmp.path(), kUidB)),
                 "no resume cursor may be written against an account that appeared mid-sync");

        // readdir consumes the latched uid too: the second sync dedups against A's
        // map (AAA short-circuited), NOT against B's (which is empty and would let
        // AAA through for a second, redundant download).
        QStringList errors2;
        QList<CloudServiceEntry*> entries2 = gc.readdir(QString(), errors2, QDateTime(), QDateTime());
        QCOMPARE(errors2.size(), 0);
        QCOMPARE(entries2.size(), 0);
    }

    // =====================================================================
    // TEST-062 — clause (c): a result that lands after the disconnect is DROPPED
    // =====================================================================

    // The disconnect is executed inside the download callback, i.e. while
    // GarminConnect's nested QEventLoop is pumping — after the "HTTP call" has
    // completed and before its result reaches the stage/record step. The bytes
    // must be discarded: nothing staged, no completion posted, and no sidecar
    // written under ANY key. The download itself DID happen (downloadCalls == 1):
    // DEC-021 explicitly scopes this to discard-only, not cancellation.
    void downloadResultLandingAfterDisconnectIsDiscarded()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUid));

        FakeEpochClient client;
        client.originalBytesById[QStringLiteral("DDD")] = makeZip(QStringLiteral("DDD.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");

        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        bool disconnectRan = false;
        client.onBeforeDownloadResult = [&]() {
            gcOther.disconnectService();
            disconnectRan = true;
        };

        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("garmin-DDD.fit"), QStringLiteral("DDD"));

        QVERIFY2(disconnectRan, "pre-condition: the disconnect must have landed inside the download frame");
        QCOMPARE(client.downloadCalls, 1); // the in-flight call ran to completion (DEC-021: discard-only)
        QVERIFY2(!ok, "a download whose result lands after the disconnect must be discarded");
        QVERIFY2(data.isEmpty(), "the discarded result must not be staged");
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);

        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUid)),
                 "a discarded result must not be recorded in the account's sidecar");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid)),
                 "a discarded result must not advance the account's resume cursor");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), QString())),
                 "a discarded result must not be recorded in an unkeyed sidecar either");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(tmp.path(), QString())),
                 "a discarded result must not advance an unkeyed resume cursor either");
    }

    // The same ordering with DEC-garmin-020's guard neutralised: the mid-download
    // disconnect is immediately followed by a reconnect, so by the time the result
    // reaches the pre-stage recheck the credential is valid again. Only the epoch
    // can drop it.
    void downloadResultLandingAfterDisconnectReconnectIsDiscardedByEpochAlone()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUid));

        FakeEpochClient client;
        client.originalBytesById[QStringLiteral("EEE")] = makeZip(QStringLiteral("EEE.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");

        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        bool disconnectRan = false;
        bool reconnected = false;
        client.onBeforeDownloadResult = [&]() {
            gcOther.disconnectService();
            disconnectRan = true;
            reconnected = connectAccount(tmp.path(), kUid);
        };

        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("garmin-EEE.fit"), QStringLiteral("EEE"));

        QVERIFY2(disconnectRan, "pre-condition: the disconnect must have landed inside the download frame");
        QVERIFY2(reconnected, "pre-condition: the account must have been reconnected inside the same frame");
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "pre-condition: DEC-020's disk predicate is SATISFIED — only the epoch can discard below");

        QCOMPARE(client.downloadCalls, 1);
        QVERIFY2(!ok, "the result of a download that spanned a disconnect must be discarded");
        QVERIFY2(data.isEmpty(), "the discarded result must not be staged");
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(tmp.path(), kUid)),
                 "a discarded result must not be recorded in any sidecar");
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::backfillStateFilePath(tmp.path(), kUid)),
                 "a discarded result must not advance any resume cursor");
    }

    // The DEC-016 retry table gives readFile a SECOND network request: an
    // ORIGINAL that comes back 200-but-not-FIT falls through to a TCX retry. A
    // disconnect that landed during the ORIGINAL attempt must stop that retry —
    // clause (a)'s "issues zero download calls" has to hold for the requests
    // readFile would issue NEXT, not just the first one. So exactly ONE download
    // call is made and the call fails.
    void disconnectDuringTheOriginalAttemptBlocksTheTcxRetry()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path(), kUid));

        FakeEpochClient client;
        // A 200 that is neither a ZIP nor a FIT — the content-sniff backstop that
        // routes DEC-016 to its TCX retry.
        client.originalBytesById[QStringLiteral("FFF")] = QByteArray("<html>not an activity</html>");

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");

        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        bool disconnectRan = false;
        client.onBeforeDownloadResult = [&]() {
            if (disconnectRan)
                return; // only the FIRST (ORIGINAL) attempt spans the disconnect
            gcOther.disconnectService();
            disconnectRan = true;
        };

        QByteArray data;
        const bool ok = gc.readFile(&data, QStringLiteral("garmin-FFF.fit"), QStringLiteral("FFF"));

        QVERIFY2(disconnectRan, "pre-condition: the disconnect must have landed inside the download frame");
        QVERIFY2(!ok, "a superseded session must not stage anything");
        QVERIFY2(data.isEmpty(), "nothing may be staged");
        QCOMPARE(static_cast<CloudService&>(gc).readCompleteCount, 0);
        QCOMPARE(client.downloadCalls, 1); // the TCX retry must NOT be issued
    }

    // =====================================================================
    // TEST-063 — the epoch is keyed PER CONFIG DIR (athlete isolation)
    // =====================================================================

    // Athlete A disconnects (and reconnects, so the disk predicate stays TRUE for
    // both athletes throughout — neither half of this slot can be attributed to
    // DEC-020). A's live session must be superseded; athlete B's live session,
    // over a DIFFERENT config dir, must be completely unaffected: it still lists
    // and still downloads. A process-global epoch counter would kill B too.
    void bumpingOneAthletesEpochLeavesAnotherAthletesLiveSessionAlone()
    {
        QTemporaryDir dirA;
        QTemporaryDir dirB;
        QVERIFY(dirA.isValid());
        QVERIFY(dirB.isValid());
        QVERIFY(connectAccount(dirA.path(), kUidA));
        QVERIFY(connectAccount(dirB.path(), kUidB));

        FakeEpochClient clientA;
        clientA.listResult = {summary(QStringLiteral("A-1"), QStringLiteral("2026-07-01 06:00:00"))};
        FakeEpochClient clientB;
        clientB.listResult = {summary(QStringLiteral("B-1"), QStringLiteral("2026-07-02 06:00:00"))};
        clientB.originalBytesById[QStringLiteral("B-1")] = makeZip(QStringLiteral("B-1.fit"), makeFitBytes());

        GarminConnect gcA(nullptr, &clientA, dirA.path());
        GarminConnect gcB(nullptr, &clientB, dirB.path());
        QStringList openA;
        QStringList openB;
        QVERIFY2(gcA.open(openA), "pre-condition: athlete A must open()");
        QVERIFY2(gcB.open(openB), "pre-condition: athlete B must open()");

        // Pre-condition: both live sessions enumerate.
        QStringList preA;
        QStringList preB;
        QCOMPARE(gcA.readdir(QString(), preA, QDateTime(), QDateTime()).size(), 1);
        QCOMPARE(gcB.readdir(QString(), preB, QDateTime(), QDateTime()).size(), 1);
        QCOMPARE(preA.size(), 0);
        QCOMPARE(preB.size(), 0);

        // Athlete A disconnects and reconnects — A's tokens are valid again, so
        // ONLY the epoch can invalidate gcA, and NOTHING on disk changed for B.
        GarminConnect gcOtherA(nullptr, nullptr, dirA.path());
        gcOtherA.disconnectService();
        QVERIFY(connectAccount(dirA.path(), kUidA));
        QVERIFY(GarminTokenStore::loadChecked(dirA.path()).isOk());
        QVERIFY(GarminTokenStore::loadChecked(dirB.path()).isOk());

        // A's live session is superseded.
        const int listsA = clientA.listCalls;
        QStringList errA;
        QList<CloudServiceEntry*> entriesA = gcA.readdir(QString(), errA, QDateTime(), QDateTime());
        QCOMPARE(entriesA.size(), 0);
        QCOMPARE(clientA.listCalls, listsA);
        QVERIFY2(!errA.isEmpty(), "athlete A's superseded session must surface an error");
        QVERIFY2(errA.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");

        // B's live session is untouched — it still lists...
        const int listsB = clientB.listCalls;
        QStringList errB;
        QList<CloudServiceEntry*> entriesB = gcB.readdir(QString(), errB, QDateTime(), QDateTime());
        QCOMPARE(errB.size(), 0);
        QCOMPARE(entriesB.size(), 1);
        QCOMPARE(entriesB.at(0)->id, QStringLiteral("B-1"));
        QCOMPARE(clientB.listCalls, listsB + 1);

        // ...and still downloads + records into ITS OWN sidecar.
        QByteArray dataB;
        QVERIFY2(gcB.readFile(&dataB, entriesB.at(0)->name, entriesB.at(0)->id),
                 "athlete B's live session must keep working when athlete A disconnects");
        QCOMPARE(dataB, makeFitBytes());

        // B-STAGE9-132/-134 — DEC-080/B-STAGE9-111: readFile alone leaves B-1
        // pending; promotion is what moves it into the imported map below.
        gcB.rideRegistrationCompleted(QStringLiteral("garmin-B-1.fit"));

        const GarminSidecarStore::ImportedMap mapB = GarminSidecarStore::loadImported(dirB.path(), kUidB);
        QVERIFY(mapB.isOk());
        QVERIFY(mapB.contains(QStringLiteral("B-1")));

        // ...and athlete A recorded nothing.
        QVERIFY2(!QFileInfo::exists(GarminSidecarStore::importedFilePath(dirA.path(), kUidA)),
                 "athlete A's superseded session must record nothing");
    }
};

QTEST_MAIN(TestGarminConnectAccountEpoch)
#include "testGarminConnectAccountEpoch.moc"
