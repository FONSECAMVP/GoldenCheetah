/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef GC_GarminConnect_h
#define GC_GarminConnect_h

#include "CloudService.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QHash>
#include <QImage>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

// REQ-008 Slice C — the Slice-A summary type carried by the worker list op.
#include "IGarminPyAdapter.h"

// REQ-007 closure — GarminConnect owns the embedded-Python adapter + the
// download/restore host and implements the CloudService open()/close()/readFile()
// pipeline (Slices 2 & 3). It is intentionally NOT its own Q_OBJECT: it adds no
// new signals/slots of its own. In production it inherits QObject via
// CloudService; open()/readFile() bridge the async worker to a synchronous bool
// with a local QEventLoop (Strava precedent) using the download client's signals
// via QObject::connect to that loop — so no per-class metaobject is required.
// This also keeps the `garmin-fast` tile test's Python-free non-QObject stub
// CloudService valid (see build report NOTES).
class IGarminDownloadClient;
class PyEmbeddedAdapter;
class GarminDownloadChain;
struct GarminDownloadFailure;

class GarminConnect : public CloudService
{
  public:
    // REQ-NF-i18n-001 (T-208) — pins tr() to THIS class's context. Without it,
    // unqualified tr() inside member functions resolves to the inherited
    // CloudService::tr (context "CloudService", DES-014 deliberately adds no
    // Q_OBJECT here), while lupdate extracts under "GarminConnect" — so every
    // extracted entry is dead at runtime. NOTE: the macro ends in a
    // `private:` section — `public:` is re-opened immediately below.
    Q_DECLARE_TR_FUNCTIONS(GarminConnect)

  public:
    GarminConnect(Context* context);

    // Test seam (garmin-fast) — inject a Python-free fake download/restore/list
    // client and override the athlete config dir (and, for the REQ-008 Slice C
    // sync tests, the active garmin_user_id that keys the per-account sidecar),
    // so open()/readFile()/readdir() are exercisable without the embedded-Python
    // host. The injected client is NOT owned. `garminUserIdOverride` defaults to
    // empty (production resolves the uid from tokens.json — see resolveGarminUserId).
    // B-STAGE9-19: `listTimeoutOverrideMs` bounds blockingList()'s wait so the
    // timeout exit is reachable in tests (0 — the default — means
    // kListTimeoutMs; production never passes it).
    GarminConnect(Context* context, IGarminDownloadClient* injectedClient, const QString& configDirOverride,
                  const QString& garminUserIdOverride = QString(), int listTimeoutOverrideMs = 0);

    ~GarminConnect();

    CloudService* clone(Context* context) { return new GarminConnect(context); }

    // Service identity key; uiName() below is the display name.
    QString id() const { return QStringLiteral("Garmin Connect"); } // T208-ALLOW:I18N-TR-WRAP
    QString uiName() const { return tr("Garmin Connect"); }
    QString description() const { return tr("Download activities from Garmin Connect."); }
    QImage logo() const;

    // Read-only: Query advertises the listing capability used by sync; Download
    // covers per-activity fetch. No Upload, no OAuth — DEC-005.
    int capabilities() const { return Query | Download; }
    int type() const { return Activities; }

    // REQ-007 closure Slice 2 — resolve the athlete config dir, load the stored
    // tokens (GarminTokenStore::loadChecked, REQ-006), and restore the session
    // via the host. On token absence / permission-refusal push a labelled error
    // and return false (does NOT attempt a download).
    bool open(QStringList& errors) override;

    // Bounded teardown of the owned host (quit()+wait() in the chain dtor, never
    // terminate() from here); tokens persist on disk.
    bool close() override;

    // REQ-008 (DEC-garmin-019 Option C) — the connect/disconnect persistence
    // callers that close A3-R008-01/D-R008-01. persistConnectSuccess() is invoked
    // by AddCloudWizard on a successful (id-gated) auth on BOTH the direct and the
    // post-MFA paths; it wraps the already-tested producer
    // GarminTokenStore::persistConnectSuccess (tokens.json + active-account.json).
    // disconnectService() is invoked by CredentialsPage::deleteClicked(); it wraps
    // GarminTokenStore::clearAccount (delete tokens.json + active-account.json,
    // PRESERVE the per-account sidecars — REQ-012). Both resolve the athlete
    // config dir via resolveConfigDir().
    void persistConnectSuccess(const QString& garminUserId, const QString& tokenBlob) override;
    void disconnectService() override;

    // DEC-080, B-STAGE9-111.
    void rideRegistrationCompleted(const QString& remoteId);

    // REQ-007 closure Slice 3 — the DEC-016 retry table: ORIGINAL(FIT) →
    // unzip+sniff → stage garmin-<id>.fit; RateLimited fails fast (no retry);
    // Network/Unknown/not-FIT retry once as TCX → stage garmin-<id>.tcx.
    // REQ-008 Slice C — on a successful stage the download is recorded into
    // imported-<uid>.json and backfill-state is advanced (DES-010 steps 5e/6).
    bool readFile(QByteArray* data, QString remotename, QString remoteid, ReadFileArmed* armed = nullptr) override;

    // REQ-008 Slice C (DES-010) — the incremental-sync enumeration. Drives the
    // worker list op (Slice A) for activities newer than the "since" timestamp
    // (the passed `from`, else backfill-state's lastSuccessStartTimeGMT, else
    // now()-7d), builds one CloudServiceEntry per activity (remoteid==activityId,
    // name garmin-<id>.fit, startTimeGMT as the entry timestamp), and short-
    // circuits any activity already in imported-<uid>.json so the base machinery
    // never issues a redundant readFile (Tier-1 dedup). A second concurrent
    // invocation while one is in progress is rejected (REQ-NF-Perf-002). A
    // listFailed outcome surfaces via `errors` and yields an empty list.
    QList<CloudServiceEntry*> readdir(QString path, QStringList& errors, QDateTime from, QDateTime to) override;

    // B-R010-04 — REQ-010's bulk-backfill dialog drives GarminBackfillController
    // directly (DES-009 keeps it decoupled from CloudService), but it still needs
    // the SAME authenticated seam + per-account keys open() already latches for
    // readdir()/readFile(). These publish ensureClient()/resolveConfigDir() for a
    // caller that has already called open() on this instance; calling
    // backfillClient() before open() lazily constructs the production worker
    // unauthenticated, exactly as ensureClient() would.
    IGarminDownloadClient* backfillClient() { return ensureClient(); }
    QString backfillConfigDir() const { return resolveConfigDir(); }

    // B-R010-05 — the LATCHED uid (REQ-017 Slice A / DEC-garmin-021 B), not a
    // live re-resolve: mirrors readdir()/readFile()'s m_openedUserId usage so a
    // disconnect/reconnect to a DIFFERENT account mid-backfill cannot repoint
    // which per-account sidecar this run writes into. Non-const (latches the
    // session on first use via ensureSessionLatched() — a no-op on the normal
    // path, since open() already forced one).
    QString backfillGarminUserId();

    // B-R010-05 — the SAME fail-closed pair readFile()/readdir() gate on
    // (REQ-017 clause a + DEC-garmin-020), published for
    // GarminBackfillController's SessionCheck callback (see
    // GarminBackfillController.h) so a disconnect or reconnect-to-a-different-
    // account mid-backfill is caught the same way, both before a request is
    // issued and after a nested-loop wait completes. Non-const: latches the
    // session on first use, same as backfillGarminUserId().
    bool backfillSessionStillValid();

  private:
    // Lazily construct the production adapter+host on first open() (deferred so
    // the NULL-context factory template never spins a thread at static-init).
    IGarminDownloadClient* ensureClient();
    QString resolveConfigDir() const;

    // DEC-garmin-020 (A3-R012-F1) — the fail-closed re-check both readFile() and
    // readdir() gate on. True only when the stored credential is present AND
    // acceptable (GarminTokenStore::loadChecked == Ok — the very predicate open()
    // uses). Re-read from disk on EVERY call because disconnectService() deletes
    // the token files without touching live instances: an already-open()ed service
    // must not keep serving downloads for an account the user has disconnected.
    bool accountStillConnected() const;

    // REQ-017 Slice A / DEC-garmin-021 (Option B) — the SESSION LATCH. Binds this
    // instance to the account it was opened against, in memory:
    //   * m_openedEpoch  — GarminAccountEpoch::current() at latch time;
    //     disconnectService() bumps that dir's epoch, so every session latched
    //     before it becomes SUPERSEDED (an int compare, zero disk I/O).
    //   * m_openedUserId — the garmin_user_id at latch time, consumed by
    //     readdir/readFile/recordImport for the life of the session instead of
    //     being re-resolved from active-account.json on every call (A3-R012-F10):
    //     a mid-sync rewrite of that file can then never record an import against
    //     a different-or-empty account.
    // latchSession() FORCES a (re-)latch and is called on every successful open()
    // — so a close()+open(), or a reconnect, correctly starts a NEW session.
    // ensureSessionLatched() is the lazy fallback for the (test-only today, but
    // reachable) case of a consuming call on an instance that was never open()ed:
    // it binds to what is current at first use rather than leaving the session
    // unbound. It never RE-latches, so it can never launder away a bump.
    void latchSession();
    void ensureSessionLatched();
    bool sessionSuperseded() const;

    // REQ-017 clause (c) — the POST-DOWNLOAD, PRE-STAGE recheck. blockingDownload()
    // runs a nested QEventLoop, so the GUI event queue keeps pumping and a
    // Disconnect can land while the request is in flight; the checks readFile made
    // on entry are stale by the time the bytes arrive. Re-asks BOTH questions (the
    // DEC-021 epoch binding and the DEC-020 credential predicate) immediately
    // before anything is staged or recorded. False => drop the result on the floor.
    // NOT cancellation: DEC-021 puts that explicitly out of scope (there is no
    // cancel primitive in GarminWorker/PyEmbeddedAdapter) — the request has already
    // run to completion; only its result is discarded.
    bool downloadResultStillWanted() const;

    // One blocking download attempt in `fmt`; bridges the async client to a sync
    // result via a local QEventLoop keyed on a fresh requestId.
    struct DownloadResult
    {
        bool ok = false;
        QByteArray bytes;
        int failureKind = 0; // GarminDownloadFailure::Kind when !ok
    };
    DownloadResult blockingDownload(const QString& fmt, const QString& remoteid);
    // REQ-NF-Obs-001 (T-207) — failureKindOut (may be null) receives the
    // GarminRestoreFailure::Kind when the restore fails (Unknown on timeout).
    bool blockingRestore(const QString& tokenBlob, int* failureKindOut = nullptr);

    // B-STAGE9-19 — the sync suite drives blockingList()'s !client exit, which
    // is unreachable through readdir(); everything else stays private.
    friend class TestGarminConnectSync;

    // REQ-008 Slice C — one blocking list op (since→summaries); bridges the async
    // client to a sync result via a local QEventLoop keyed on a fresh requestId,
    // mirroring blockingDownload/blockingRestore.
    struct ListResult
    {
        bool ok = false;
        QVector<GarminActivitySummary> summaries;
        int failureKind = 0; // GarminListFailure::Kind when !ok (mirrors DownloadResult)
        // B-STAGE9-19 — the untranslated library message when the failure came
        // from the adapter (empty on the local no-client/timeout exits).
        QString rawMessage;
    };
    ListResult blockingList(const QString& sinceGmt);

    // B-STAGE9-19 — GC-stable error_code vocabulary for a GarminListFailure::Kind
    // (REQ-NF-Obs-001); private static so the friend test can pin the mapping.
    static const char* garminListKindCode(int kind);

    // B-STAGE9-19 — test override for blockingList()'s timeout (ctor seam).
    // 0 means the production kListTimeoutMs applies.
    int m_listTimeoutOverrideMs = 0;

    // Resolve the active garmin_user_id that keys the per-account sidecar
    // (DES-002). The test override wins; otherwise it is read from tokens.json
    // (DES-002 records it there at first-connect). Empty when no account is
    // resolvable — readdir/record then no-op rather than touching a mis-keyed file.
    QString resolveGarminUserId() const;

    // REQ-008 Slice C (DES-010 steps 5e/6) — record a freshly-staged activity
    // into imported-<uid>.json and advance backfill-state's lastSuccessStartTimeGMT
    // to its startTimeGMT. No-op when the config dir / uid cannot be resolved.
    void recordImport(const QString& activityId, const QString& stagedFilename);

    // DEC-080, B-STAGE9-111.
    static QString activityIdFromStagedFilename(const QString& stagedFilename);

    // B-R007-01 / REQ-NF-Perf-003: post the readComplete notification as a QUEUED
    // self-post (deferred onto the event queue via m_client's event loop, on the
    // calling thread) so the auto-download caller's QEventLoop observes it promptly
    // instead of timing out its 30s watchdog. No Q_OBJECT is added (DES-014).
    //
    // DEC-garmin-022 (B-R017-06): `message` is carried through to the completion so
    // a FAIL-CLOSED refusal can report itself instead of stalling the caller's loop.
    // It is tr("Completed.") on the success paths — a non-empty message therefore
    // does NOT mean failure, which is exactly why DEC-garmin-023 stopped trying to
    // read failure off this channel and added postReadFailed below instead.
    void postReadComplete(QByteArray* data, const QString& name, const QString& message);

    // DEC-garmin-023 (B-R017-10): the same QUEUED self-post, on CloudService's
    // EXPLICIT failure channel (notifyReadFailed). Used by every readFile() path
    // that gives up: the consumers render `reason`, free the caller's buffer and
    // advance their loop, instead of hanging on a silent `return false`. Exactly
    // one of postReadComplete/postReadFailed runs per readFile() call — see the
    // ownership note above postReadComplete's definition.
    void postReadFailed(QByteArray* data, const QString& name, const QString& reason);

    IGarminDownloadClient* m_client = nullptr; // active seam (injected or host's)
    PyEmbeddedAdapter* m_adapter = nullptr;    // owned in production only
    GarminDownloadChain* m_chain = nullptr;    // owned in production only
    bool m_injectedClient = false;             // true → m_client not owned
    QString m_configDirOverride;               // test override; empty in production
    QString m_garminUserIdOverride;            // test override; empty in production

    // REQ-017 Slice A (DEC-garmin-021 B) — the latched session identity. See
    // latchSession() above. Never written by anything but latchSession().
    quint64 m_openedEpoch = 0;
    QString m_openedUserId;
    bool m_sessionLatched = false;

    // REQ-NF-Perf-002 — in-object in-progress guard: a second readdir/sync while
    // one is running is rejected; the running one continues. Single-threaded
    // per-object (readdir bridges async→sync on the caller thread), so a plain
    // bool set/cleared around the enumeration is sufficient.
    bool m_syncInProgress = false;

    // REQ-008 Slice C — startTimeGMT captured per activityId during readdir so the
    // subsequent readFile can record Garmin's server-side timestamp (the base
    // machinery only hands readFile the remotename + remoteid, not the summary).
    QHash<QString, QString> m_pendingStartTimes;

    // A3-R007-01: context object for postReadComplete()'s queued self-post. A
    // bare QObject *member* (NOT a Q_OBJECT on GarminConnect — DES-014 preserved;
    // adds no signals/slots and needs no moc). Using this member as the
    // QMetaObject::invokeMethod context binds the pending post to GarminConnect's
    // OWN lifetime: it is destroyed as part of ~GarminConnect (member destruction,
    // before the CloudService base subobject is torn down), so Qt cancels any
    // still-pending post BEFORE the object that the lambda captures (`this`) is
    // gone — closing the use-after-free that binding to the injected, unowned
    // m_client left open. Constructed on the calling thread and never
    // moveToThread'd, so its thread affinity == m_client's (the caller's event
    // loop) — delivery lands on the same loop as before.
    QObject m_completionContext;
};

#endif // GC_GarminConnect_h
