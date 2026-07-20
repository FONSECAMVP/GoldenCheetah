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
    GarminConnect(Context* context);

    // Test seam (garmin-fast) — inject a Python-free fake download/restore/list
    // client and override the athlete config dir (and, for the REQ-008 Slice C
    // sync tests, the active garmin_user_id that keys the per-account sidecar),
    // so open()/readFile()/readdir() are exercisable without the embedded-Python
    // host. The injected client is NOT owned. `garminUserIdOverride` defaults to
    // empty (production resolves the uid from tokens.json — see resolveGarminUserId).
    GarminConnect(Context* context, IGarminDownloadClient* injectedClient, const QString& configDirOverride,
                  const QString& garminUserIdOverride = QString());

    ~GarminConnect();

    CloudService* clone(Context* context) { return new GarminConnect(context); }

    QString id() const { return QStringLiteral("Garmin Connect"); }
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

    // REQ-007 closure Slice 3 — the DEC-016 retry table: ORIGINAL(FIT) →
    // unzip+sniff → stage garmin-<id>.fit; RateLimited fails fast (no retry);
    // Network/Unknown/not-FIT retry once as TCX → stage garmin-<id>.tcx.
    // REQ-008 Slice C — on a successful stage the download is recorded into
    // imported-<uid>.json and backfill-state is advanced (DES-010 steps 5e/6).
    bool readFile(QByteArray* data, QString remotename, QString remoteid) override;

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

  private:
    // Lazily construct the production adapter+host on first open() (deferred so
    // the NULL-context factory template never spins a thread at static-init).
    IGarminDownloadClient* ensureClient();
    QString resolveConfigDir() const;

    // One blocking download attempt in `fmt`; bridges the async client to a sync
    // result via a local QEventLoop keyed on a fresh requestId.
    struct DownloadResult
    {
        bool ok = false;
        QByteArray bytes;
        int failureKind = 0; // GarminDownloadFailure::Kind when !ok
    };
    DownloadResult blockingDownload(const QString& fmt, const QString& remoteid);
    bool blockingRestore(const QString& tokenBlob);

    // REQ-008 Slice C — one blocking list op (since→summaries); bridges the async
    // client to a sync result via a local QEventLoop keyed on a fresh requestId,
    // mirroring blockingDownload/blockingRestore.
    struct ListResult
    {
        bool ok = false;
        QVector<GarminActivitySummary> summaries;
    };
    ListResult blockingList(const QString& sinceGmt);

    // Resolve the active garmin_user_id that keys the per-account sidecar
    // (DES-002). The test override wins; otherwise it is read from tokens.json
    // (DES-002 records it there at first-connect). Empty when no account is
    // resolvable — readdir/record then no-op rather than touching a mis-keyed file.
    QString resolveGarminUserId() const;

    // REQ-008 Slice C (DES-010 steps 5e/6) — record a freshly-staged activity
    // into imported-<uid>.json and advance backfill-state's lastSuccessStartTimeGMT
    // to its startTimeGMT. No-op when the config dir / uid cannot be resolved.
    void recordImport(const QString& activityId, const QString& stagedFilename);

    // B-R007-01 / REQ-NF-Perf-003: post the readComplete notification as a QUEUED
    // self-post (deferred onto the event queue via m_client's event loop, on the
    // calling thread) so the auto-download caller's QEventLoop observes it promptly
    // instead of timing out its 30s watchdog. No Q_OBJECT is added (DES-014).
    void postReadComplete(QByteArray* data, const QString& name);

    IGarminDownloadClient* m_client = nullptr; // active seam (injected or host's)
    PyEmbeddedAdapter* m_adapter = nullptr;    // owned in production only
    GarminDownloadChain* m_chain = nullptr;    // owned in production only
    bool m_injectedClient = false;             // true → m_client not owned
    QString m_configDirOverride;               // test override; empty in production
    QString m_garminUserIdOverride;            // test override; empty in production

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
