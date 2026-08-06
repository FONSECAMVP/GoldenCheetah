/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-068 — DEC-garmin-023: GarminConnect::readFile carries read
// FAILURE explicitly, on its own `readFailed` channel.
//
// Acceptance criterion (verbatim from the briefing):
//   "every silent `return false` site in `readFile` emits `readFailed` exactly
//    once with a distinguishable reason, delivery is QUEUED not synchronous,
//    `readFile` still returns false, and the caller's own buffer pointer comes
//    back. Cover all five sites; drive RateLimit and TCX-also-failed as ORDINARY
//    failures (no disconnect involved) since those are the everyday paths."
//
// THE DEFECT THIS CLOSES (finding B-R017-10, confirmed on disk)
// ------------------------------------------------------------
// DEC-garmin-022 fixed the TWO fail-closed ENTRY guards (TEST-065): they now post
// a labelled completion so the caller's loop advances and the caller's buffer is
// freed. FIVE further `return false` sites in the SAME function were left silent:
//
//   1. post-download, pre-stage discard on the FIT path   (REQ-017 clause (c))
//   2. RateLimit fast-fail                                (DEC-016, ORDINARY)
//   3. pre-TCX recheck                                    (REQ-017 clauses (a)+(c))
//   4. post-download, pre-stage discard on the TCX path    (REQ-017 clause (c))
//   5. final fall-through: TCX also failed                (DEC-016, ORDINARY)
//
// Every one of them has the exact shape TEST-065 closed: CloudServiceSyncDialog::
// syncNext/downloadNext and CloudServiceAutoDownload::run DISCARD readFile's bool
// and wait on a signal, so a silent refusal hangs the dialog on "Downloading n of
// N" forever and LEAKS the QByteArray they preallocated. Two of the five are not
// exotic disconnect races at all — RateLimit and "TCX also failed" are the
// everyday ways a download does not happen — which is why they are driven here as
// ORDINARY failures, with the account fully connected throughout.
//
// WHY A NEW SIGNAL AND NOT `message` (DEC-023's rationale, pinned here)
// --------------------------------------------------------------------
// DEC-022's shared half was BLOCKED because every sibling service passes
// tr("Completed.") as `message` on SUCCESS (Strava, Dropbox, SportTracks, Xert,
// Azum, PolarFlow, CyclingAnalytics, SixCycle, Nolio, LocalFileStore), so success
// and failure are indistinguishable on that channel. DEC-023 therefore adds an
// EXPLICIT one. The success control at the end of this file pins that the two
// channels are alternatives, never both.
//
// WHAT IS ASSERTED, AND WHY EACH ASSERTION IS LOAD-BEARING
// --------------------------------------------------------
//   * EXACTLY ONE delivery, counting BOTH channels. The consumers free the buffer
//     when either channel reports; two deliveries for one readFile() would be a
//     DOUBLE FREE at the caller — strictly worse than the leak being fixed. So
//     every slot asserts readFailedCount + readCompleteCount == 1.
//   * the failure carries THE CALLER'S OWN buffer pointer — otherwise the leak is
//     not actually closed, because that is the pointer the consumer deletes.
//   * that buffer is still EMPTY: a refusal stages nothing.
//   * the reason is Garmin-labelled and DISTINGUISHABLE per site. Collapsing
//     "rate limited" into "could not be downloaded" would tell a user to retry
//     immediately when the correct advice is to wait, so the reasons are compared
//     pairwise for inequality rather than merely for non-emptiness.
//   * delivery is QUEUED, not inside readFile's own call frame (TEST-024/065
//     precedent). readFile runs inside a nested QEventLoop in production and the
//     A3-R007-01 use-after-free guard depends on the post going through
//     m_completionContext, so a synchronous emit is a real hazard, not a style
//     choice.
//   * readFile still returns false — reporting is not permission.
//   * the number of wire calls is asserted at every site, so "refused" cannot
//     quietly become "retried" (RateLimit must make NO second request, and the
//     pre-TCX recheck must make no second request either).
//
// Python-free (garmin-fast): stubs/ReadFileStubPreamble.h stubs CloudService (its
// notifyReadComplete / notifyReadFailed recorders stand in for the two signals)
// and replaces PyEmbeddedAdapter with a Python-free fake; the REAL
// GarminTokenStore / GarminSidecarStore / AtomicFile / GarminAccountEpoch back
// the disk and epoch state and real contrib/qzip unwraps the FIT ORIGINAL.
//
// A SEPARATE executable, for the same reason as TEST-065: the account-epoch map
// is process-global static state.

#include "GarminAccountEpoch.h"
#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "GarminWorker.h"
#include "IGarminDownloadClient.h"
#include "IGarminPyAdapter.h"
#include "zipwriter.h"

#include <QByteArray>
#include <QFile>
#include <QHash>
#include <QMetaObject>
#include <QSet>
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
// FakeFailureClient — same-thread, Python-free IGarminDownloadClient.
//
// Scripts a response per requested fmt and records the exact fmt-call sequence,
// so "refused" versus "retried anyway" is visible rather than invisible.
//
// bumpEpochOnFmt reproduces the REQ-017 clause-(c) race for real: the epoch is
// bumped from INSIDE the queued response, i.e. while GarminConnect is parked in
// blockingDownload's nested QEventLoop — exactly where a production Disconnect
// through a second instance lands. It is the same primitive
// GarminConnect::disconnectService() uses (GarminAccountEpoch::bump), so the
// clause-(c) rechecks see precisely what they see in production.
// ---------------------------------------------------------------------------
class FakeFailureClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    struct Resp
    {
        bool ok = false;
        QByteArray bytes;
        GarminDownloadFailure::Kind kind = GarminDownloadFailure::Unknown;
    };

    QHash<QString, Resp> responses; // fmt -> scripted response
    QStringList downloadFmts;       // recorded fmt-call sequence
    QString bumpEpochOnFmt;         // fmt whose in-flight window bumps the epoch
    QString bumpEpochDir;

    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(
            this, [this, id]() { emit activitiesListed(id, QVector<GarminActivitySummary>()); }, Qt::QueuedConnection);
    }

    void downloadActivity(const QString&, const QString& fmt, QUuid id) override
    {
        downloadFmts << fmt;
        const Resp r = responses.value(fmt);
        const bool bump = (!bumpEpochOnFmt.isEmpty() && fmt == bumpEpochOnFmt);
        QMetaObject::invokeMethod(
            this,
            [this, id, r, bump]() {
                // Mid-flight disconnect: GarminConnect is inside blockingDownload's
                // nested loop right now.
                if (bump)
                    GarminAccountEpoch::bump(bumpEpochDir);
                if (r.ok) {
                    emit downloaded(id, r.bytes);
                } else {
                    GarminDownloadFailure e;
                    e.kind = r.kind;
                    e.rawMessage = QStringLiteral("scripted failure");
                    emit downloadFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }
};

class TestGarminConnectReadFailure : public QObject
{
    Q_OBJECT

  private:
    // Every slot reports its reason string out so the pairwise-distinctness
    // assertion at the end compares what the code ACTUALLY produced, not a
    // literal this test re-states.
    QStringList reasonsSeen;

    static bool connectAccount(const QString& dir) { return GarminTokenStore::persistConnectSuccess(dir, kUid, kBlob); }

    // The shared shape of all five sites. Returns the reason that was posted.
    // `data` is the caller-preallocated buffer, exactly as syncNext/downloadNext
    // and CloudServiceAutoDownload::run build it.
    QString driveRefusal(GarminConnect& gc, CloudService& svc, QByteArray* data, const QString& remotename,
                         const QString& remoteid)
    {
        const bool ok = gc.readFile(data, remotename, remoteid);

        // Reporting is not permission.
        if (ok) {
            qWarning("readFile returned TRUE where a refusal was required");
            return QString();
        }

        // QUEUED, not synchronous: nothing may be delivered inside readFile's own
        // call frame (TEST-024/065 precedent, A3-R007-01).
        if (svc.readFailedCount != 0 || svc.readCompleteCount != 0) {
            qWarning("a notification was delivered SYNCHRONOUSLY inside readFile's call frame");
            return QString();
        }

        // ...but promptly once the loop turns. On the pre-fix code this NEVER
        // arrives: that stall IS the defect.
        const bool arrived = QTest::qWaitFor([&]() { return svc.readFailedCount > 0; }, 1000);
        if (!arrived) {
            qWarning("no readFailed was ever posted — the caller's loop stalls and its buffer leaks");
            return QString();
        }
        return svc.lastFailedReason;
    }

    // Asserted after EVERY site: one delivery, on the failure channel, carrying
    // the caller's own still-empty buffer — and still exactly one after the loop
    // has been pumped again (a second delivery is a double free at the consumer).
    void verifyExactlyOneFailureFor(CloudService& svc, const QByteArray* data, const QString& name)
    {
        QCOMPARE(svc.readFailedCount, 1);
        QCOMPARE(svc.readCompleteCount, 0);
        QCOMPARE(svc.lastFailedPtr, data);
        QCOMPARE(svc.lastFailedName, name);
        QVERIFY2(svc.lastFailedData.isEmpty(), "a refusal must stage nothing into the caller's buffer");
        QVERIFY2(svc.lastFailedReason.contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the reason must be labelled as a Garmin Connect problem");
        QTest::qWait(50);
        QCOMPARE(svc.readFailedCount, 1);
        QCOMPARE(svc.readCompleteCount, 0);
    }

  private slots:

    // =====================================================================
    // SITE 2 — RateLimit fast-fail. An ORDINARY failure: the account is
    // connected throughout, nothing is disconnected, and this is simply what
    // Garmin says when the athlete has synced too much today.
    // =====================================================================
    void rateLimitedDownloadReportsAQueuedReadFailedAndMakesNoSecondRequest()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::RateLimit};

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        // The account is and stays fully connected — no disconnect is involved.
        const quint64 epochAtOpen = GarminAccountEpoch::current(tmp.path());

        QByteArray* data = new QByteArray;
        const QString reason = driveRefusal(gc, svc, data, QStringLiteral("garmin-8001.fit"), QStringLiteral("8001"));

        QVERIFY2(!reason.isEmpty(), "the RateLimit fast-fail must REPORT itself, not return false in silence");
        verifyExactlyOneFailureFor(svc, data, QStringLiteral("garmin-8001.fit"));

        // DEC-016 anti retry-storm: RateLimit makes NO second request.
        QCOMPARE(client.downloadFmts, QStringList() << QStringLiteral("ORIGINAL"));
        QCOMPARE(GarminAccountEpoch::current(tmp.path()), epochAtOpen);
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "this is an ORDINARY failure: the account must still be connected");

        reasonsSeen << reason;
        delete data; // stands in for the consumer's `delete data`
    }

    // =====================================================================
    // SITE 5 — the final fall-through: ORIGINAL failed and the TCX retry
    // failed too. Also ORDINARY (a 404/network pair, no disconnect).
    // =====================================================================
    void tcxAlsoFailedReportsAQueuedReadFailedAfterBothRequests()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::Network};
        client.responses[QStringLiteral("TCX")] = {false, {}, GarminDownloadFailure::Network};

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray* data = new QByteArray;
        const QString reason = driveRefusal(gc, svc, data, QStringLiteral("garmin-8002.fit"), QStringLiteral("8002"));

        QVERIFY2(!reason.isEmpty(), "an activity that is downloadable in NEITHER format must report itself");
        verifyExactlyOneFailureFor(svc, data, QStringLiteral("garmin-8002.fit"));

        // DEC-016: the fallback WAS attempted — exactly once.
        QCOMPARE(client.downloadFmts, QStringList() << QStringLiteral("ORIGINAL") << QStringLiteral("TCX"));
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "this is an ORDINARY failure: the account must still be connected");

        reasonsSeen << reason;
        delete data;
    }

    // =====================================================================
    // SITE 1 — REQ-017 clause (c): a Disconnect lands WHILE the ORIGINAL
    // download is in flight, so the FIT bytes are discarded, not staged.
    // =====================================================================
    void disconnectDuringTheOriginalDownloadReportsTheDiscardedFitActivity()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("8003.fit"), makeFitBytes()),
                                                        GarminDownloadFailure::Unknown};
        client.bumpEpochDir = tmp.path();
        client.bumpEpochOnFmt = QStringLiteral("ORIGINAL"); // superseded mid-flight

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);
        const quint64 epochAtOpen = GarminAccountEpoch::current(tmp.path());

        QByteArray* data = new QByteArray;
        const QString reason = driveRefusal(gc, svc, data, QStringLiteral("garmin-8003.fit"), QStringLiteral("8003"));

        QVERIFY2(!reason.isEmpty(), "a mid-flight disconnect must report the discard, not stall the loop");
        verifyExactlyOneFailureFor(svc, data, QStringLiteral("garmin-8003.fit"));

        // It really was the clause-(c) DISCARD: the download completed (so the
        // guard cannot have been an entry guard) and no TCX retry was issued.
        QCOMPARE(client.downloadFmts, QStringList() << QStringLiteral("ORIGINAL"));
        QVERIFY2(GarminAccountEpoch::current(tmp.path()) != epochAtOpen,
                 "pre-condition: the epoch must have been bumped mid-flight");

        reasonsSeen << reason;
        delete data;
    }

    // =====================================================================
    // SITE 3 — REQ-017 clauses (a)+(c): the Disconnect landed during a FAILED
    // ORIGINAL, so the TCX retry must not be ISSUED AT ALL (not merely
    // discarded afterwards).
    // =====================================================================
    void disconnectBeforeTheTcxRetryReportsItAndIssuesNoSecondRequest()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::Network};
        client.responses[QStringLiteral("TCX")] = {true, QByteArray("<TrainingCenterDatabase/>"),
                                                   GarminDownloadFailure::Unknown};
        client.bumpEpochDir = tmp.path();
        client.bumpEpochOnFmt = QStringLiteral("ORIGINAL");

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray* data = new QByteArray;
        const QString reason = driveRefusal(gc, svc, data, QStringLiteral("garmin-8004.fit"), QStringLiteral("8004"));

        QVERIFY2(!reason.isEmpty(), "a refusal to even attempt the retry must report itself");
        verifyExactlyOneFailureFor(svc, data, QStringLiteral("garmin-8004.fit"));

        // The whole point of this site: NO second wire call, even though the
        // scripted TCX would have succeeded.
        QCOMPARE(client.downloadFmts, QStringList() << QStringLiteral("ORIGINAL"));

        reasonsSeen << reason;
        delete data;
    }

    // =====================================================================
    // SITE 4 — REQ-017 clause (c) on the RETRY path: the Disconnect lands
    // while the TCX request is in flight, so the TCX bytes are discarded.
    // =====================================================================
    void disconnectDuringTheTcxRetryReportsTheDiscardedTcxActivity()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {false, {}, GarminDownloadFailure::Network};
        client.responses[QStringLiteral("TCX")] = {true, QByteArray("<TrainingCenterDatabase/>"),
                                                   GarminDownloadFailure::Unknown};
        client.bumpEpochDir = tmp.path();
        client.bumpEpochOnFmt = QStringLiteral("TCX"); // survives the ORIGINAL, dies on the retry

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray* data = new QByteArray;
        const QString reason = driveRefusal(gc, svc, data, QStringLiteral("garmin-8005.fit"), QStringLiteral("8005"));

        QVERIFY2(!reason.isEmpty(), "a mid-retry disconnect must report the discard, not stall the loop");
        verifyExactlyOneFailureFor(svc, data, QStringLiteral("garmin-8005.fit"));

        // Both requests were made — so this is the RETRY-path discard, not the
        // pre-TCX recheck above.
        QCOMPARE(client.downloadFmts, QStringList() << QStringLiteral("ORIGINAL") << QStringLiteral("TCX"));

        reasonsSeen << reason;
        delete data;
    }

    // =====================================================================
    // ALL FIVE REASONS ARE DISTINGUISHABLE
    // =====================================================================
    //
    // Compared PAIRWISE on the strings the production code actually produced
    // (collected by the five slots above), never against literals restated here.
    // "Rate limited, wait" and "not available in either format, this activity is
    // not coming" are different pieces of advice; collapsing them into one string
    // would make the sync log useless exactly when a user needs it.
    void everySiteIsDistinguishableFromEveryOther()
    {
        QCOMPARE(reasonsSeen.count(), 5);
        for (const QString& r : reasonsSeen)
            QVERIFY2(!r.isEmpty(), "every site must produce a reason");
        QCOMPARE(QSet<QString>(reasonsSeen.begin(), reasonsSeen.end()).count(), 5);
    }

    // =====================================================================
    // SUCCESS CONTROL — the two channels are ALTERNATIVES, never both
    // =====================================================================
    //
    // The consumers free the caller's buffer on WHICHEVER channel reports. If a
    // success ever posted both, `delete data` would run twice. This pins that a
    // successful read posts readComplete ONCE and readFailed NEVER.
    void aSuccessfulReadPostsCompletionOnlyAndNeverTheFailureChannel()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(connectAccount(tmp.path()));

        FakeFailureClient client;
        client.responses[QStringLiteral("ORIGINAL")] = {true, makeZip(QStringLiteral("8006.fit"), makeFitBytes()),
                                                        GarminDownloadFailure::Unknown};

        GarminConnect gc(nullptr, &client, tmp.path());
        QStringList openErrors;
        QVERIFY2(gc.open(openErrors), "pre-condition: a connected account must open()");
        CloudService& svc = static_cast<CloudService&>(gc);

        QByteArray* data = new QByteArray;
        QVERIFY2(gc.readFile(data, QStringLiteral("garmin-8006.fit"), QStringLiteral("8006")),
                 "pre-condition: a live session must serve the download");

        QTRY_COMPARE_WITH_TIMEOUT(svc.readCompleteCount, 1, 1000);
        QTest::qWait(50);
        QCOMPARE(svc.readCompleteCount, 1);
        QCOMPARE(svc.readFailedCount, 0);
        QCOMPARE(svc.lastReadMessage, QStringLiteral("Completed."));
        QCOMPARE(svc.lastReadData, makeFitBytes());

        delete data;
    }
};

QTEST_MAIN(TestGarminConnectReadFailure)
#include "testGarminConnectReadFailure.moc"
