/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:T-019 — REQ-007 closure (Slice 2/3): GarminConnect::open()/close()
// worker-in-CloudService lifecycle.
//
//   open() resolves the athlete config dir, loads the stored tokens via
//   GarminTokenStore::loadChecked (REQ-006), and restores the session through
//   the host from the returned blob. On a permission-refused LoadResult it
//   returns false with a labelled error and does NOT attempt a download
//   (REQ-006 forces a fresh SSO — never a download with an unsafe session).
//   close() performs bounded teardown; tokens persist on disk.
//
// Python-free: a fake IGarminDownloadClient (DES-004 seam) records the restore
// blob + any download attempt, and stubs/ReadFileStubPreamble.h stubs
// CloudService + replaces PyEmbeddedAdapter with a Python-free fake →
// `garmin-fast` label. GarminTokenStore + AtomicFile are the real (pure-Qt)
// units, driven against a real temp config dir with controlled file modes.

#include "GarminAccountEpoch.h"
#include "GarminConnect.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest/QtTest>

#ifdef Q_OS_UNIX
#    include <sys/stat.h>
#endif

// ---------------------------------------------------------------------------
// FakeRestoreClient — records the restore blob it received and whether any
// download was attempted, then emits sessionRestored / restoreFailed via a
// queued invocation so it arrives while open()'s blocking QEventLoop runs.
// ---------------------------------------------------------------------------
class FakeRestoreClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    bool restoreOk = true;
    GarminRestoreFailure::Kind restoreKind = GarminRestoreFailure::Unknown;
    QString lastRestoreBlob;
    int restoreCalls = 0;
    int downloadCalls = 0;

    void restoreSession(const QString& blob, QUuid id) override
    {
        lastRestoreBlob = blob;
        ++restoreCalls;
        const bool ok = restoreOk;
        const GarminRestoreFailure::Kind kind = restoreKind;
        QMetaObject::invokeMethod(
            this,
            [this, id, ok, kind]() {
                if (ok) {
                    emit sessionRestored(id);
                } else {
                    GarminRestoreFailure e;
                    e.kind = kind;
                    emit restoreFailed(id, e);
                }
            },
            Qt::QueuedConnection);
    }

    // REQ-008 Slice C seam extension (IGarminDownloadClient gained a pure-virtual
    // list op). The open()/close() tests never list; a no-op satisfies the interface.
    void listActivities(const QString&, QUuid) override {}

    void downloadActivity(const QString&, const QString& fmt, QUuid id) override
    {
        ++downloadCalls; // open() must NEVER reach here on the failure paths
        QMetaObject::invokeMethod(
            this, [this, id]() { emit downloadFailed(id, GarminDownloadFailure{}); }, Qt::QueuedConnection);
        Q_UNUSED(fmt);
    }
};

namespace {
const QByteArray kBlob = QByteArray("{\"oauth1\":\"OA1\",\"oauth2\":\"OA2.refresh\"}");

// Write <dir>/garminconnect/tokens.json with `bytes` and set its POSIX mode.
QString writeTokenFile(const QString& configDir, const QByteArray& bytes, uint mode)
{
    QDir().mkpath(configDir + QStringLiteral("/garminconnect"));
    const QString path = configDir + QStringLiteral("/garminconnect/tokens.json");
    QFile f(path);
    f.open(QIODevice::WriteOnly | QIODevice::Truncate);
    f.write(bytes);
    f.close();
#ifdef Q_OS_UNIX
    ::chmod(path.toLocal8Bit().constData(), mode);
#else
    Q_UNUSED(mode);
#endif
    return path;
}

// REQ-NF-Obs-001 (T-207) — installs a Qt message handler for its lifetime and
// records every qDebug line, so the structured gc_obs trace can be asserted
// literally (prd.md:112's verification method is log-format review). QtTest
// runs slots sequentially, so a single capture at a time is safe. Qt permits
// a message handler to be invoked concurrently from any thread (Qt's own
// logging docs require handlers to be reentrant), so the writer (hook) and
// the reader (snapshot) serialize on one mutex — same as testGarminConnectSync.cpp.
class ObsCapture
{
  public:
    ObsCapture()
    {
        QMutexLocker locker(&mutex_);
        prev_ = qInstallMessageHandler(&ObsCapture::hook);
        current = this;
    }
    ~ObsCapture()
    {
        QMutexLocker locker(&mutex_);
        current = nullptr;
        locker.unlock();
        qInstallMessageHandler(prev_);
    }
    ObsCapture(const ObsCapture&) = delete;
    ObsCapture& operator=(const ObsCapture&) = delete;

    QStringList snapshot() const
    {
        QMutexLocker locker(&mutex_);
        return lines_;
    }

  private:
    static void hook(QtMsgType, const QMessageLogContext&, const QString& msg)
    {
        QMutexLocker locker(&mutex_);
        if (current != nullptr)
            current->lines_ << msg;
    }
    static ObsCapture* current;
    static QMutex mutex_;
    QStringList lines_;
    QtMessageHandler prev_;
};
ObsCapture* ObsCapture::current = nullptr;
QMutex ObsCapture::mutex_;
} // namespace

class TestGarminConnectOpen : public QObject
{
    Q_OBJECT

  private slots:

    // open() with a conforming 0600 token file: loadChecked returns Ok, and the
    // session is restored from THAT blob (verbatim) through the injected client.
    void openRestoresSessionFromLoadedBlob()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        fake.restoreOk = true;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(ok, "open() must succeed when a conforming token file restores");
        QCOMPARE(errors.size(), 0);
        QCOMPARE(fake.restoreCalls, 1);
        // Restored from the blob loadChecked returned — VERBATIM.
        QCOMPARE(fake.lastRestoreBlob, QString::fromUtf8(kBlob));
    }

    // REQ-006: a token file WIDER than owner-only 0600 is refused. open() must
    // return false with a labelled error and must NOT attempt any restore or
    // download (forces a fresh SSO instead of using an unsafe session).
    void openRefusesOnPermissionRejectedAndDoesNotDownload()
    {
#ifndef Q_OS_UNIX
        QSKIP("POSIX permission-refusal path is UNIX-only");
#endif
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // 0644 — group/other-readable → loadChecked must reject.
        writeTokenFile(tmp.path(), kBlob, 0644);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail on a permission-rejected token file");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed on rejection");
        QVERIFY2(errors.first().contains(QStringLiteral("Garmin"), Qt::CaseInsensitive),
                 "the error must be labelled as a Garmin Connect problem");
        QCOMPARE(fake.restoreCalls, 0);  // did NOT attempt to restore an unsafe session
        QCOMPARE(fake.downloadCalls, 0); // and did NOT attempt any download
    }

    // No token file at all → NotFound: open() returns false with a labelled
    // error and attempts no download.
    void openFailsWhenNoStoredSession()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        // deliberately do not write any token file

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail when there is no stored session");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed when no session exists");
        QCOMPARE(fake.restoreCalls, 0);
        QCOMPARE(fake.downloadCalls, 0);
    }

    // B-STAGE9-13 — a PRESENT, conforming-0600 tokens.json with ZERO bytes
    // (the live-observed shape: a connect attempt that failed to persist a
    // real blob) must fail open() with an ACCURATE, DISTINCT label — not the
    // misleading "could not restore the stored session" text a real restore
    // ATTEMPT would produce — and must never attempt a restore or download
    // with an empty blob.
    void openFailsWithDistinctLabelOnEmptyTokenFile()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString dest = writeTokenFile(tmp.path(), QByteArray(), 0600);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        ObsCapture capture;
        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail on an empty token file");
        // Assert the EXACT user-facing message, not merely "isn't the restore
        // label": the reviewer's counter-implementation (map blank content to
        // TokenPermissionsRejected) also avoids the restore label, but shows
        // the WRONG ("unsafe permissions") text. This must be the specific
        // empty-file message and nothing else.
        QCOMPARE(errors.size(), 1);
        QCOMPARE(
            errors.first(),
            QStringLiteral("Garmin Connect: the stored session file '%1' is empty; please sign in again.").arg(dest));
        QCOMPARE(fake.restoreCalls, 0); // never attempted a restore with an empty blob
        QCOMPARE(fake.downloadCalls, 0);

        // Assert the EXACT gc_obs error_code, not merely "not not_found/unknown":
        // the same counter-implementation would emit error_code=token_permissions_rejected,
        // which is also neither not_found nor unknown, so only the literal code proves this.
        // Match the FIELD, not a prefix of it: gcObsTrace() always writes
        // "error_code=<code> duration_ms=<n>" (GarminConnect.cpp), so error_code is
        // never the last field and the following " duration_ms=" is a reliable
        // terminator. Without it, an implementation emitting error_code=empty_corrupt
        // would satisfy a bare contains("error_code=empty").
        const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=auth outcome="));
        QVERIFY2(trace.size() == 1, qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                                              "A wrong op name yields 0 here because the filter pins "
                                                              "the op field. Captured: [%2]")
                                                   .arg(trace.size())
                                                   .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        QVERIFY2(trace.first().contains(QStringLiteral("error_code=empty duration_ms=")),
                 qPrintable(QStringLiteral("expected exact field error_code=empty in: %1").arg(trace.first())));
    }

    // A restore that fails (e.g. session expired) → open() returns false with a
    // labelled error.
    void openFailsWhenRestoreFails()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        fake.restoreOk = false;
        fake.restoreKind = GarminRestoreFailure::SessionExpired;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        QVERIFY2(!ok, "open() must fail when the stored session cannot be restored");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed when restore fails");
        QCOMPARE(fake.restoreCalls, 1);
    }

    // T-205 / REQ-NF-Reliab-002 — torn-write detection on READ: tokens.json with
    // CORRECT 0600 permissions but corrupted (truncated, non-JSON) content
    // must still fail open() gracefully. Today's contract, pinned here
    // deliberately, has three halves:
    //   1. loadChecked does NO content validation — it returns Ok with
    //      whatever bytes are on disk (the exact seam where a
    //      LoadStatus::Torn case would appear if one were ever added);
    //   2. those bytes are delivered VERBATIM to the restore attempt — the
    //      component that parses them;
    //   3. open() returns false with the "sign in again" label — no crash,
    //      no download, forced fresh SSO.
    // Scope note: the failure SIGNAL still comes from FakeRestoreClient —
    // the real parser is Python load_tokens behind the production chain,
    // not part of this binary. The JSON-parse of garbage itself is covered
    // pytest-side (tests/test_adapter_restore.py::
    // test_load_tampered_or_expired_blob_raises_session_expired). What THIS
    // test adds is the missing link the existing openFailsWhenRestoreFails
    // does not cover: REAL torn on-disk state -> REAL loadChecked ->
    // verbatim delivery -> labelled failure, end to end through open().
    void openWithTornTokenContentFailsWithRestoreLabel()
    {
        // Literal truncated bytes — not valid JSON in any shape (cut mid
        // member, no closing quote or brace), i.e. exactly what a crash
        // mid-legacy-write would leave behind.
        const QByteArray kTorn("{\"oauth1\":\"OA1\",\"oauth2\":");

        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kTorn, 0600);

        // (1) the load seam hands torn CONTENT through as Ok at 0600.
        const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(tmp.path());
        QVERIFY2(r.isOk(), "a 0600 token file must pass the permission gate regardless of content");
        QCOMPARE(r.bytes, kTorn);

        FakeRestoreClient fake;
        fake.restoreOk = false;
        fake.restoreKind = GarminRestoreFailure::SessionExpired;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        const bool ok = gc.open(errors);

        // (2) the torn bytes reached the restore seam verbatim (the real
        // chain would json-parse them exactly here and fail session_expired).
        QCOMPARE(fake.restoreCalls, 1);
        QCOMPARE(fake.lastRestoreBlob, QString::fromUtf8(kTorn));

        // (3) labelled failure routing to a fresh sign-in; never a download.
        QVERIFY2(!ok, "open() must fail on torn token content");
        QVERIFY2(!errors.isEmpty(), "a labelled error must be pushed for torn token content");
        QVERIFY2(errors.join(QLatin1Char(' ')).contains(QStringLiteral("sign in again")),
                 "torn content must route the user to a fresh sign-in, not a crash");
        QCOMPARE(fake.downloadCalls, 0);
    }

    // REQ-NF-Obs-001 (T-207) — DES-008's structured qDebug developer-trace
    // (design.md:786: "Structured qDebug mirrors the same fields"): open()
    // (op "auth") emits ONE parseable gc_obs line per call — op, outcome,
    // error_code (empty on success, the GC-stable kind on failure), duration_ms.
    // prd.md:112's verification method is log-format review; this pins the
    // format as executable review.
    //
    // B-STAGE9-14 — EXACT-FIELD ANCHORING. A bare contains("k=v") is a PREFIX
    // match: an implementation emitting k=v_suffix satisfies it, so the
    // assertion cannot fail and is worse than no assertion. gcObsTrace()
    // (src/Cloud/GarminConnect.cpp) emits a fixed field order, verified by
    // capturing a real line rather than by reading the format string:
    //   gc_obs op=<op> outcome=<ok|fail> error_code=<code> duration_ms=<ms>[ activity_count=<n>]
    // Two anchoring rules follow from that shape:
    //   * a field with a field AFTER it is matched together with the next
    //     key, e.g. "error_code=rate_limit duration_ms=" — the following
    //     " <key>=" is the delimiter that pins the value's end.
    //   * activity_count is LAST when present, so it has no following key to
    //     anchor against and uses endsWith(" activity_count=<n>") instead.
    //     The captured QString was checked byte-wise (cat -A) and ends exactly
    //     at the digit — Qt hands the message handler no trailing newline or
    //     space. If a field is ever appended after activity_count, these
    //     endsWith assertions SHOULD break loudly; that is intended.
    // The op field is pinned by the snapshot().filter() selector carrying its
    // own " outcome=" delimiter, so a wrong op name yields zero matched lines
    // rather than silently validating a different op's trace.
    void openEmitsStructuredObsTraceOnSuccess()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        fake.restoreOk = true;
        GarminConnect gc(nullptr, &fake, tmp.path());

        ObsCapture capture;
        QStringList errors;
        QVERIFY(gc.open(errors));

        const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=auth outcome="));
        QVERIFY2(trace.size() == 1, qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                                              "A wrong op name yields 0 here because the filter pins "
                                                              "the op field. Captured: [%2]")
                                                   .arg(trace.size())
                                                   .arg(capture.snapshot().join(QStringLiteral(" | ")))));
        QVERIFY2(trace.first().contains(QStringLiteral("outcome=ok error_code=")),
                 qPrintable(QStringLiteral("expected exact field outcome=ok in: %1").arg(trace.first())));
        // error_code present but EMPTY on success — the field set is fixed so a
        // log parser can rely on the shape.
        QVERIFY2(trace.first().contains(QStringLiteral("error_code= ")),
                 qPrintable(QStringLiteral("expected empty error_code in: %1").arg(trace.first())));
        QVERIFY2(trace.first().contains(QStringLiteral("duration_ms=")),
                 qPrintable(QStringLiteral("expected duration_ms=<n> in: %1").arg(trace.first())));
    }

    // Every failure path carries the GC-stable code as its error_code field:
    // the loadChecked enum (not_found / token_permissions_rejected) and the
    // real GarminRestoreFailure::Kind (session_expired / network / unknown).
    void openEmitsStructuredObsTraceWithKindOnFailurePaths()
    {
        // (a) no stored session -> LoadStatus::NotFound -> not_found
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            FakeRestoreClient fake;
            GarminConnect gc(nullptr, &fake, tmp.path());

            ObsCapture capture;
            QStringList errors;
            QVERIFY(!gc.open(errors));

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=auth outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            QVERIFY2(trace.first().contains(QStringLiteral("outcome=fail error_code=")),
                     qPrintable(QStringLiteral("expected exact field outcome=fail in: %1").arg(trace.first())));
            QVERIFY2(trace.first().contains(QStringLiteral("error_code=not_found duration_ms=")),
                     qPrintable(QStringLiteral("expected exact field error_code=not_found in: %1").arg(trace.first())));
        }

        // (b) restore failure -> the actual GarminRestoreFailure::Kind
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            writeTokenFile(tmp.path(), kBlob, 0600);

            FakeRestoreClient fake;
            fake.restoreOk = false;
            fake.restoreKind = GarminRestoreFailure::SessionExpired;
            GarminConnect gc(nullptr, &fake, tmp.path());

            ObsCapture capture;
            QStringList errors;
            QVERIFY(!gc.open(errors));

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=auth outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            QVERIFY2(trace.first().contains(QStringLiteral("error_code=session_expired duration_ms=")),
                     qPrintable(
                         QStringLiteral("expected exact field error_code=session_expired in: %1").arg(trace.first())));
        }

        // (c) B-STAGE9-13 — present but EMPTY token file -> a SPECIFIC
        // error_code, neither folded into "not_found" (that would erase the
        // never-connected vs. connected-but-persistence-broke distinction)
        // nor left as "unknown" (getting a real kind instead of unknown is
        // half the point of this unit).
        {
            QTemporaryDir tmp;
            QVERIFY(tmp.isValid());
            writeTokenFile(tmp.path(), QByteArray(), 0600);

            FakeRestoreClient fake;
            GarminConnect gc(nullptr, &fake, tmp.path());

            ObsCapture capture;
            QStringList errors;
            QVERIFY(!gc.open(errors));

            const QStringList trace = capture.snapshot().filter(QStringLiteral("gc_obs op=auth outcome="));
            QVERIFY2(trace.size() == 1,
                     qPrintable(QStringLiteral("expected exactly ONE gc_obs line for this op, got %1. "
                                               "A wrong op name yields 0 here because the filter pins "
                                               "the op field. Captured: [%2]")
                                    .arg(trace.size())
                                    .arg(capture.snapshot().join(QStringLiteral(" | ")))));
            // The EXACT code, not merely "isn't not_found/unknown": a
            // counter-implementation emitting error_code=token_permissions_rejected
            // would also pass a merely-negative assertion here. The trailing
            // " duration_ms=" pins the field boundary (gcObsTrace() always emits
            // duration_ms right after error_code), so error_code=empty_corrupt fails.
            QVERIFY2(trace.first().contains(QStringLiteral("error_code=empty duration_ms=")),
                     qPrintable(QStringLiteral("expected exact field error_code=empty in: %1").arg(trace.first())));
        }
    }

    // B-R010-04 — the backfill dialog (REQ-010 UI wiring) is handed an
    // already-open()'d GarminConnect and needs the SAME authenticated client
    // + per-account keys ensureClient()/resolveConfigDir()/resolveGarminUserId()
    // already latch for readdir()/readFile(); these public wrappers publish
    // exactly that, once open() has run.
    void backfillAccessorsExposeTheOpenedClientAndKeys()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path(), QStringLiteral("9998887"));

        QStringList errors;
        QVERIFY(gc.open(errors));

        QCOMPARE(gc.backfillClient(), static_cast<IGarminDownloadClient*>(&fake));
        QCOMPARE(gc.backfillConfigDir(), tmp.path());
        QCOMPARE(gc.backfillGarminUserId(), QStringLiteral("9998887"));
    }

    // B-R010-05 — backfillGarminUserId() must answer from the uid LATCHED at
    // open() time, never a live re-resolve of active-account.json: a
    // disconnect/reconnect to a DIFFERENT account mid-backfill must not
    // repoint which per-account sidecar a running backfill writes into.
    void backfillGarminUserIdReturnsTheLatchedUidNotALiveReresolve()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), QStringLiteral("accountA"), kBlob));

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path()); // no uid override: reads active-account.json for real

        QStringList errors;
        QVERIFY(gc.open(errors));
        QCOMPARE(gc.backfillGarminUserId(), QStringLiteral("accountA"));

        // Simulate a reconnect to a DIFFERENT account landing on disk while
        // this instance's session stays latched to accountA (mirrors
        // AddCloudWizard's persist-success producer running through a
        // second, freshly-opened instance - DEC-garmin-019 C).
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), QStringLiteral("accountB"), kBlob));
        QCOMPARE(GarminTokenStore::loadActiveAccountUserId(tmp.path()), QStringLiteral("accountB"));

        QCOMPARE(gc.backfillGarminUserId(), QStringLiteral("accountA"));
    }

    // B-R010-05 — backfillSessionStillValid() must go false the moment this
    // session is superseded (REQ-017 clause a), even though the reconnect
    // that superseded it leaves a perfectly valid tokens.json on disk (which
    // would satisfy accountStillConnected() alone) - the same independence
    // sessionSuperseded() already guarantees readdir()/readFile().
    void backfillSessionStillValidGoesFalseOnReconnectToADifferentAccount()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), QStringLiteral("accountA"), kBlob));

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        QVERIFY(gc.open(errors));
        QVERIFY2(gc.backfillSessionStillValid(), "pre-condition: a freshly opened session must be valid");

        // A second, independent GarminConnect instance disconnects (bumps the
        // shared epoch for this config dir) and then reconnects to a
        // DIFFERENT account - GarminAccountEpoch::bump()'s documented "other
        // still-live instances" case.
        GarminAccountEpoch::bump(tmp.path());
        QVERIFY(GarminTokenStore::persistConnectSuccess(tmp.path(), QStringLiteral("accountB"), kBlob));
        QVERIFY2(GarminTokenStore::loadChecked(tmp.path()).isOk(),
                 "pre-condition: the reconnect leaves a perfectly valid tokens.json");

        QVERIFY2(!gc.backfillSessionStillValid(),
                 "a superseded session must refuse even though tokens.json now checks out");
    }

    // Positive control: a session that is never superseded and stays
    // connected reports valid for the whole run, and re-asking mutates
    // nothing (matches downloadResultStillWanted()'s zero-side-effect shape).
    void backfillSessionStillValidStaysTrueWhileConnected()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());

        QStringList errors;
        QVERIFY(gc.open(errors));
        QVERIFY(gc.backfillSessionStillValid());
        QVERIFY(gc.backfillSessionStillValid());
    }

    // close() performs bounded teardown and returns promptly. With an injected
    // client (not owned) there is no host thread to stop, so this must return
    // true well within any watchdog.
    void closeTearsDownBounded()
    {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        writeTokenFile(tmp.path(), kBlob, 0600);

        FakeRestoreClient fake;
        GarminConnect gc(nullptr, &fake, tmp.path());
        QStringList errors;
        QVERIFY(gc.open(errors));

        QElapsedTimer t;
        t.start();
        const bool closed = gc.close();
        QVERIFY2(closed, "close() must return true");
        QVERIFY2(t.elapsed() < 2000, "close() teardown must be bounded (never hang)");
    }
};

QTEST_MAIN(TestGarminConnectOpen)
#include "testGarminConnectOpen.moc"
