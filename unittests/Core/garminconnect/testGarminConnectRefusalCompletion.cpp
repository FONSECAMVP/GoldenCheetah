/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-065 — REQ-017 / DEC-garmin-022 (Option A): a fail-closed
// readFile() REPORTS itself instead of stalling the sync loop.
//
// THE DEFECT THIS CLOSES (finding B-R017-06, confirmed on disk):
// CloudServiceSyncDialog::syncNext/downloadNext (src/Cloud/CloudService.cpp)
// preallocate `new QByteArray`, call store->readFile(...), DISCARD the returned
// bool, and then wait for the readComplete signal to advance the loop:
//
//     QByteArray *data = new QByteArray; // gets deleted when read completes
//     store->readFile(data, curr->text(1), curr->text(6));
//     QApplication::processEvents();
//     return true;
//
// GarminConnect::readFile's two fail-closed entry guards — the DEC-021 session
// epoch (sessionSuperseded) and the DEC-020 credential re-check
// (accountStillConnected) — used to `return false` and post NOTHING. Two real
// consequences: (1) the dialog sits on "Downloading n of N" forever, with no
// error and no progress; (2) the preallocated buffer LEAKS once per attempt,
// because completedRead's `delete data` never runs. CloudServiceAutoDownload has
// the same shape (it blocks its per-activity QEventLoop on readComplete with a
// 30s watchdog, and its readComplete slot is what frees the buffer).
//
// DEC-022 Option A: the refusal posts a LABELLED completion through the existing
// postReadComplete() queued self-post and still returns false. The loop then
// advances, the user sees the reason, and the caller's `delete data` runs exactly
// once.
//
// WHAT IS ASSERTED, AND WHY EACH ASSERTION IS LOAD-BEARING:
//   * exactly ONE completion is delivered — a second delivery would mean a
//     DOUBLE DELETE at the caller (completedRead / autodownload readComplete both
//     `delete data`), which is strictly worse than the leak being fixed;
//   * the completion carries THE SAME buffer pointer the caller passed, i.e. the
//     buffer the caller will free — otherwise the leak is not actually closed;
//   * that buffer is still EMPTY: nothing was staged, and an empty buffer is what
//     forces the shared dialog down its `ride == NULL` branch — the only branch
//     that may ever surface a message;
//   * the message is Garmin-labelled;
//   * readFile still returns false, and ZERO list/download calls are made — the
//     refusal must stay a refusal, not become a download;
//   * the completion is QUEUED, not delivered inside readFile's own call frame
//     (TEST-024's precedent). readFile runs inside a nested QEventLoop in
//     production and the A3-R007-01 use-after-free guard depends on the post
//     going through m_completionContext, so a synchronous emit here would be a
//     real hazard, not a style choice.
//
// BOTH fail-closed paths are covered, and each is ISOLATED so the other cannot be
// what refused:
//   * superseded session — driven through the real disconnectService() on a
//     second instance (which bumps the epoch);
//   * credential gone — the token file is removed DIRECTLY, so the epoch is
//     provably unchanged (asserted against GarminAccountEpoch::current()) and
//     only DEC-020's disk predicate can refuse.
//
// The last slot is the SUCCESS CONTROL: a successful read posts tr("Completed.")
// together with NON-EMPTY staged bytes. It pins the fact that a NON-EMPTY message
// does not mean failure — every CloudService in this tree (Strava, Dropbox, Xert,
// SportTracks, ...) passes tr("Completed.") on success too — which is why the
// SHARED half of DEC-garmin-022 (rendering `message` in
// CloudServiceSyncDialog::completedRead) is NOT implemented here: "message is
// non-empty" cannot be the failure discriminator without relabelling every other
// service's parse failure as "Completed.". That half is reported back for a
// decision; this file covers the half that is unambiguous — the refusal reports
// itself, so the loop advances and the caller's buffer is freed.
//
// Python-free (garmin-fast): stubs/ReadFileStubPreamble.h stubs CloudService (its
// notifyReadComplete recorder stands in for the readComplete signal) and replaces
// PyEmbeddedAdapter with a Python-free fake; the REAL GarminTokenStore /
// GarminSidecarStore / AtomicFile back the disk I/O and real contrib/qzip unwraps
// the FIT ORIGINAL.
//
// A SEPARATE executable: the account-epoch map is process-global static state, so
// a dedicated process keeps these slots independent of every other suite.

#include "GarminAccountEpoch.h"
#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QDateTime>
#include <QFile>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

namespace {

const QString kUid = QStringLiteral("123456789");
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}");

// DEC-022: the wording readdir() already emits for a SUPERSEDED session, reused
// verbatim so a refusal reads the same however the user provoked it.
const QString kSupersededMessage =
    QStringLiteral("Garmin Connect: this session's account was disconnected; please sign in again.");

// B-R017-11 (folded in by DEC-garmin-023): the DEC-020 credential guard is a
// DIFFERENT condition — "no account is connected at all" rather than "the account
// THIS session was opened against is gone" — and readdir() has always worded it
// this way. readFile() used to echo the superseded wording for it, so the same
// condition read differently depending on which entry point the user hit; it now
// matches readdir() verbatim. The two constants are deliberately kept apart: if a
// future change collapses the two guards into one message, one of these slots
// fails rather than the distinction silently disappearing.
const QString kNoAccountMessage = QStringLiteral("Garmin Connect: no connected account; please sign in again.");

QByteArray readAllBytes(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QByteArray();
    const QByteArray b = f.readAll();
    f.close();
    return b;
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

} // namespace

// ---------------------------------------------------------------------------
// FakeRefusalClient — same-thread, Python-free IGarminDownloadClient. Counts the
// network ops so "refused but still talked to Garmin" is detectable rather than
// invisible.
// ---------------------------------------------------------------------------
class FakeRefusalClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    QHash<QString, QByteArray> originalBytesById;

    int restoreCalls = 0;
    int listCalls = 0;
    int downloadCalls = 0;

    void restoreSession(const QString&, QUuid id) override
    {
        ++restoreCalls;
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        ++listCalls;
        QMetaObject::invokeMethod(
            this, [this, id]() { emit activitiesListed(id, QVector<GarminActivitySummary>()); }, Qt::QueuedConnection);
    }

    void downloadActivity(const QString& activityId, const QString& fmt, QUuid id) override
    {
        ++downloadCalls;
        const QByteArray bytes = originalBytesById.value(activityId);
        QMetaObject::invokeMethod(
            this,
            [this, id, fmt, bytes]() {
                if (fmt == QStringLiteral("ORIGINAL") && !bytes.isEmpty())
                    emit downloaded(id, bytes);
                else
                    emit downloadFailed(id, GarminDownloadFailure{});
            },
            Qt::QueuedConnection);
    }
};

class TestGarminConnectRefusalCompletion : public QObject
{
    Q_OBJECT

  private:
    static bool connectAccount(const QString& dir) { return GarminTokenStore::persistConnectSuccess(dir, kUid, kBlob); }

  private slots:

    // =====================================================================
    // TEST-065(a) — the DEC-021 path: a SUPERSEDED session reports itself
    // =====================================================================
    void supersededSessionPostsALabelledCompletionSoTheLoopAdvances()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeRefusalClient client;
        client.originalBytesById[QStringLiteral("5001")] = makeZip(QStringLiteral("5001.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);
        QCOMPARE(svc.readCompleteCount, 0);

        // A second instance (ConfigDialog -> Accounts -> Delete) disconnects out
        // from under the live session — the real production entry point.
        GarminConnect gcOther(nullptr, nullptr, tmp.path());
        gcOther.disconnectService();

        // The dialog's preallocated buffer, exactly as syncNext/downloadNext build
        // it: heap-allocated, freed by whoever handles the completion.
        QByteArray* data = new QByteArray;
        const bool ok = gc.readFile(data, QStringLiteral("garmin-5001.fit"), QStringLiteral("5001"));

        QVERIFY2(!ok, "a superseded session must still REFUSE — reporting is not permission");
        QCOMPARE(client.downloadCalls, 0);
        QCOMPARE(client.listCalls, 0);

        // Queued, not synchronous (TEST-024's precedent): nothing may be delivered
        // inside readFile's own call frame.
        QCOMPARE(svc.readCompleteCount, 0);

        // ...but once the event loop turns, the completion lands — promptly, so a
        // short safety timeout (1s, NOT the caller's 30s watchdog) is ample. On the
        // pre-fix code this NEVER arrives: that is the stall.
        QTRY_COMPARE_WITH_TIMEOUT(svc.readCompleteCount, 1, 1000);

        // It is the CALLER'S buffer that came back — the one the caller will
        // delete. Anything else leaves the leak open.
        QCOMPARE(svc.lastReadPtr, static_cast<const QByteArray*>(data));
        QVERIFY2(svc.lastReadData.isEmpty(), "a refusal must stage nothing — the empty buffer is what forces the "
                                             "shared dialog down its ride == NULL (failure) branch");
        QVERIFY2(data->isEmpty(), "nothing may be written into the caller's buffer by a refusal");

        // Labelled, so the cell says WHY rather than going blank.
        QVERIFY2(svc.lastReadMessage.contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the completion must be labelled as a Garmin Connect problem");
        QCOMPARE(svc.lastReadMessage, kSupersededMessage);
        QCOMPARE(svc.lastReadName, QStringLiteral("garmin-5001.fit"));

        // EXACTLY once. A second delivery would make the caller's `delete data`
        // run twice — a double free, strictly worse than the leak being fixed.
        QTest::qWait(50);
        QCOMPARE(svc.readCompleteCount, 1);

        delete data; // stands in for completedRead's `delete data`
    }

    // =====================================================================
    // TEST-065(b) — the DEC-020 path, ISOLATED: the credential is gone while
    // the session epoch is provably untouched
    // =====================================================================
    void refusalOnTheCredentialCheckAlonePostsALabelledCompletion()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeRefusalClient client;
        client.originalBytesById[QStringLiteral("6001")] = makeZip(QStringLiteral("6001.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        const quint64 epochAtOpen = GarminAccountEpoch::current(tmp.path());

        // Remove the credential WITHOUT going through disconnectService(), so the
        // epoch is not bumped: the DEC-021 guard cannot be what refuses below.
        QVERIFY(QFile::remove(GarminTokenStore::tokenFilePath(tmp.path())));
        QVERIFY2(!GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "pre-condition: DEC-020's disk predicate must now REFUSE");
        QCOMPARE(GarminAccountEpoch::current(tmp.path()), epochAtOpen);

        QByteArray* data = new QByteArray;
        const bool ok = gc.readFile(data, QStringLiteral("garmin-6001.fit"), QStringLiteral("6001"));

        QVERIFY2(!ok, "a session whose credential is gone must still REFUSE");
        QCOMPARE(client.downloadCalls, 0);
        QCOMPARE(client.listCalls, 0);

        QCOMPARE(svc.readCompleteCount, 0); // queued, not synchronous
        QTRY_COMPARE_WITH_TIMEOUT(svc.readCompleteCount, 1, 1000);

        QCOMPARE(svc.lastReadPtr, static_cast<const QByteArray*>(data));
        QVERIFY2(svc.lastReadData.isEmpty(), "a refusal must stage nothing");
        QVERIFY2(svc.lastReadMessage.contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the completion must be labelled as a Garmin Connect problem");
        // B-R017-11: readdir()'s wording for this same predicate, verbatim — and
        // NOT the superseded wording asserted by the slot above.
        QCOMPARE(svc.lastReadMessage, kNoAccountMessage);
        QVERIFY2(svc.lastReadMessage != kSupersededMessage,
                 "the DEC-020 credential guard and the DEC-021 superseded guard are different conditions and must "
                 "not report the same sentence");
        QCOMPARE(svc.lastReadName, QStringLiteral("garmin-6001.fit"));

        QTest::qWait(50);
        QCOMPARE(svc.readCompleteCount, 1);

        delete data;
    }

    // =====================================================================
    // SUCCESS CONTROL — a SUCCESS is not a failure
    // =====================================================================
    //
    // GarminConnect passes tr("Completed.") as the message on the SUCCESS path.
    // So "message is non-empty" does NOT mean failure, and any code that treats it
    // that way relabels a successful download as an error. This pins the
    // discriminator the shared dialog actually branches on: a success carries
    // NON-EMPTY staged bytes (uncompressRide of them yields a ride, so
    // completedRead takes its `ride != NULL` branch and never consults `message`),
    // whereas a refusal carries an EMPTY buffer (asserted above).
    void successfulReadPostsCompletedWithTheStagedBytes()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeRefusalClient client;
        client.originalBytesById[QStringLiteral("7001")] = makeZip(QStringLiteral("7001.fit"), makeFitBytes());

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray* data = new QByteArray;
        const bool ok = gc.readFile(data, QStringLiteral("garmin-7001.fit"), QStringLiteral("7001"));

        QVERIFY2(ok, "pre-condition: a live session must serve the download");
        QCOMPARE(client.downloadCalls, 1);
        QTRY_COMPARE_WITH_TIMEOUT(svc.readCompleteCount, 1, 1000);

        QCOMPARE(svc.lastReadMessage, QStringLiteral("Completed."));
        QVERIFY2(!svc.lastReadMessage.isEmpty(),
                 "a NON-EMPTY message on the SUCCESS path is exactly why the message may only be rendered on the "
                 "ride == NULL branch");
        QCOMPARE(svc.lastReadData, makeFitBytes());
        QVERIFY2(!svc.lastReadData.isEmpty(),
                 "the success discriminator: non-empty staged bytes are what make completedRead's uncompressRide "
                 "yield a ride, so the failure branch is unreachable for a success");
        QCOMPARE(svc.lastReadName, QStringLiteral("garmin-7001.fit"));

        delete data;
    }
};

QTEST_MAIN(TestGarminConnectRefusalCompletion)
#include "testGarminConnectRefusalCompletion.moc"
