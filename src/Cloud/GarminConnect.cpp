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

#include "GarminConnect.h"

#include "GarminAccountEpoch.h"
#include "GarminDownloadChain.h"
#include "GarminSidecarStore.h"
#include "GarminTokenStore.h"
#include "IGarminDownloadClient.h"
#include "PyEmbeddedAdapter.h"
#include "zipreader.h"

#include <QBuffer>
#include <QColor>
#include <QDateTime>
#include <QEventLoop>
#include <QTimer>
#include <QUuid>

#include <memory>

namespace {
// Generous bounds so production never wedges the GUI thread; unit tests reply
// on the first event-loop turn and never approach these.
constexpr int kRestoreTimeoutMs = 30000;
constexpr int kDownloadTimeoutMs = 60000;
constexpr int kListTimeoutMs = 60000;

// DES-010 step 3 — default the sync window to the last 7 days when there is no
// backfill-state cursor (and no explicit `from`).
constexpr int kDefaultSinceDays = 7;

// Garmin's server-side startTimeGMT is stored/marshalled verbatim as a string in
// this format (DES-010). Parsing it to a QDateTime lets readdir carry it as the
// CloudServiceEntry timestamp; the raw string is what gets recorded in the sidecar.
const char* const kGarminTimeFormat = "yyyy-MM-dd HH:mm:ss";

QDateTime parseGarminTime(const QString& s)
{
    QDateTime dt = QDateTime::fromString(s, QString::fromLatin1(kGarminTimeFormat));
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}

// FIT signature: the ASCII bytes ".FIT" live at offset 8 in the FIT file header
// (DEC-016). A shorter buffer, an HTML error page, or a TCX/GPX payload all fail
// this and route to the TCX fallback.
bool looksLikeFit(const QByteArray& b)
{
    return b.size() >= 12 && b.mid(8, 4) == QByteArrayLiteral(".FIT");
}

// ORIGINAL downloads are ZIP-wrapped (DEC-016). Unwrap the first entry in
// memory (no temp file) via ZipReader's QIODevice ctor. Returns false for any
// non-ZIP / unreadable / empty input — the caller then treats it as "not FIT".
bool unzipFirstEntry(const QByteArray& zipped, QByteArray* out)
{
    QByteArray work = zipped; // ZipReader's QBuffer needs a non-const backing store
    auto buf = std::make_unique<QBuffer>(&work);
    if (!buf->open(QIODevice::ReadOnly))
        return false;
    ZipReader reader(std::move(buf));
    if (!reader.isReadable() || reader.count() < 1)
        return false;
    const ZipReader::FileInfo info = reader.entryInfoAt(0);
    if (!info.isValid() || !info.isFile)
        return false;
    *out = reader.fileData(info.filePath);
    return true;
}
} // namespace

// REQ-018 — readFile stages UNCOMPRESSED bytes: the ORIGINAL download is
// unzipped in memory before it is staged as garmin-<id>.fit, and the DEC-016 TCX
// fallback is raw XML staged as garmin-<id>.tcx. CloudService's ctor defaults
// downloadCompression to `zip`, and uncompressRide's FIRST guard rejects — with
// "expected compressed activity file." — any name that does not match that
// setting. Leaving the default therefore made every SUCCESSFUL Garmin download
// unimportable in both consumers. Both ctors must declare what we actually hand
// over, because either can be the one the service is created through.
GarminConnect::GarminConnect(Context* c) : CloudService(c)
{
    downloadCompression = none;
}

GarminConnect::GarminConnect(Context* c, IGarminDownloadClient* injectedClient, const QString& configDirOverride,
                             const QString& garminUserIdOverride)
    : CloudService(c), m_client(injectedClient), m_injectedClient(true), m_configDirOverride(configDirOverride),
      m_garminUserIdOverride(garminUserIdOverride)
{
    downloadCompression = none;
}

GarminConnect::~GarminConnect()
{
    // DES-001a destruction order: host (worker) before adapter. Injected client
    // (tests) is not owned — m_chain/m_adapter are null there, so this is safe.
    delete m_chain;
    delete m_adapter;
}

QImage GarminConnect::logo() const
{
    QImage img(QStringLiteral(":images/services/garminconnect.png"));
    if (!img.isNull())
        return img;

    QImage fallback(64, 64, QImage::Format_ARGB32);
    fallback.fill(QColor(0, 122, 195)); // Garmin brand blue
    return fallback;
}

IGarminDownloadClient* GarminConnect::ensureClient()
{
    if (m_client)
        return m_client;

    // Production lazy construction (DES-013 modulePath resolution mirrors
    // AddCloudWizard::ensureGarminAuthPage): runtime env override first, else the
    // build-time dev default. DES-001a: GarminConnect owns adapter + chain.
    QString modulePath = QString::fromLocal8Bit(qgetenv("GC_GARMIN_PYPATH"));
#ifdef GARMIN_PY_MODULE_DIR
    if (modulePath.isEmpty())
        modulePath = QStringLiteral(GARMIN_PY_MODULE_DIR);
#endif
    m_adapter = new PyEmbeddedAdapter(modulePath);
    m_chain = new GarminDownloadChain(m_adapter);
    m_client = m_chain->client();
    return m_client;
}

QString GarminConnect::resolveConfigDir() const
{
    if (!m_configDirOverride.isEmpty())
        return m_configDirOverride;
    if (context && context->athlete && context->athlete->home)
        return context->athlete->home->config().absolutePath();
    return QString();
}

QString GarminConnect::resolveGarminUserId() const
{
    // Test override wins (deterministic, decoupled from token internals).
    if (!m_garminUserIdOverride.isEmpty())
        return m_garminUserIdOverride;

    // Production (REQ-008 Slice D / DEC-garmin-018 Option B): the active
    // garmin_user_id is persisted at connect-success into the SEPARATE,
    // account-agnostic active-account.json — NOT tokens.json, whose schema is
    // security-locked (REQ-006/007) and carries only the raw garth OAuth blob.
    // The read is tolerant: a missing/torn/absent active-account.json yields an
    // empty id (readdir/record then no-op rather than key a mis-named sidecar).
    const QString dir = resolveConfigDir();
    if (dir.isEmpty())
        return QString();
    return GarminTokenStore::loadActiveAccountUserId(dir);
}

bool GarminConnect::accountStillConnected() const
{
    // DEC-garmin-020 (Option C) — the fail-closed predicate, evaluated FRESH on
    // every consuming call (A3-R012-F1).
    //
    // Ground truth: disconnectService() deletes tokens.json + active-account.json
    // and clears NO in-memory state, and nothing shuts down GarminConnect
    // instances that are already open() (the wizard's finish-with-sync dialog holds
    // one open while Options -> Athlete -> Accounts -> Delete disconnects through a
    // SECOND, freshly-minted instance — DEC-garmin-019 C). A live instance
    // therefore cannot trust its own restored session: the on-disk credential is
    // the single source of truth for "this account is still connected", so it is
    // re-read at every call rather than cached.
    //
    // The predicate is deliberately the SAME one open() gates on
    // (GarminTokenStore::loadChecked): only an Ok result counts as connected. An
    // absent token file means disconnected; a REJECTED one (mode wider than
    // owner-only, REQ-006) already forces a fresh SSO in open(), so it must not be
    // downloaded against here either. No carve-out exists for the injected-client
    // test seam or for a missing athlete config dir — a service that cannot prove
    // it is connected does not talk to Garmin.
    return GarminTokenStore::loadChecked(resolveConfigDir()).isOk();
}

void GarminConnect::latchSession()
{
    // DEC-garmin-021 (Option B) — bind this instance to the account session that
    // is current RIGHT NOW. Both halves are captured together and from then on the
    // instance speaks only for this account: the epoch decides whether it may act
    // at all, the uid decides which per-account sidecar it acts on.
    m_openedEpoch = GarminAccountEpoch::current(resolveConfigDir());
    m_openedUserId = resolveGarminUserId();
    m_sessionLatched = true;
}

void GarminConnect::ensureSessionLatched()
{
    if (!m_sessionLatched)
        latchSession();
}

bool GarminConnect::sessionSuperseded() const
{
    // The A3-R012-F1 close-out (REQ-017 clause a): TRUE once the account this
    // session was opened against has been disconnected — INDEPENDENTLY of what is
    // on disk now, so a disconnect-then-reconnect (which restores a perfectly
    // valid tokens.json and therefore satisfies DEC-garmin-020's predicate) still
    // stops this session dead. An unlatched instance is not superseded: it has not
    // claimed a session yet, and every consuming path latches before asking.
    return m_sessionLatched && GarminAccountEpoch::current(resolveConfigDir()) != m_openedEpoch;
}

bool GarminConnect::downloadResultStillWanted() const
{
    // Both layers, re-asked after the wire work finished (REQ-017 clause c).
    return !sessionSuperseded() && accountStillConnected();
}

bool GarminConnect::blockingRestore(const QString& tokenBlob)
{
    IGarminDownloadClient* client = m_client;
    if (!client)
        return false;

    const QUuid reqId = QUuid::createUuid();
    QEventLoop loop;
    bool done = false;
    bool ok = false;

    const QMetaObject::Connection c1 =
        QObject::connect(client, &IGarminDownloadClient::sessionRestored, &loop, [&](QUuid id) {
            if (done || id != reqId)
                return;
            done = true;
            ok = true;
            loop.quit();
        });
    const QMetaObject::Connection c2 =
        QObject::connect(client, &IGarminDownloadClient::restoreFailed, &loop, [&](QUuid id, GarminRestoreFailure) {
            if (done || id != reqId)
                return;
            done = true;
            ok = false;
            loop.quit();
        });
    QTimer::singleShot(kRestoreTimeoutMs, &loop, [&]() {
        if (!done) {
            done = true;
            loop.quit();
        }
    });

    client->restoreSession(tokenBlob, reqId);
    loop.exec();

    QObject::disconnect(c1);
    QObject::disconnect(c2);
    return ok;
}

GarminConnect::DownloadResult GarminConnect::blockingDownload(const QString& fmt, const QString& remoteid)
{
    DownloadResult res;
    IGarminDownloadClient* client = m_client;
    if (!client)
        return res;

    const QUuid reqId = QUuid::createUuid();
    QEventLoop loop;
    bool done = false;

    const QMetaObject::Connection c1 =
        QObject::connect(client, &IGarminDownloadClient::downloaded, &loop, [&](QUuid id, QByteArray data) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = true;
            res.bytes = data;
            loop.quit();
        });
    const QMetaObject::Connection c2 =
        QObject::connect(client, &IGarminDownloadClient::downloadFailed, &loop, [&](QUuid id, GarminDownloadFailure e) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = false;
            res.failureKind = e.kind;
            loop.quit();
        });
    QTimer::singleShot(kDownloadTimeoutMs, &loop, [&]() {
        if (!done) {
            done = true;
            loop.quit();
        }
    });

    client->downloadActivity(remoteid, fmt, reqId);
    loop.exec();

    QObject::disconnect(c1);
    QObject::disconnect(c2);
    return res;
}

GarminConnect::ListResult GarminConnect::blockingList(const QString& sinceGmt)
{
    ListResult res;
    IGarminDownloadClient* client = m_client;
    if (!client)
        return res;

    const QUuid reqId = QUuid::createUuid();
    QEventLoop loop;
    bool done = false;

    const QMetaObject::Connection c1 = QObject::connect(client, &IGarminDownloadClient::activitiesListed, &loop,
                                                        [&](QUuid id, QVector<GarminActivitySummary> summaries) {
                                                            if (done || id != reqId)
                                                                return;
                                                            done = true;
                                                            res.ok = true;
                                                            res.summaries = summaries;
                                                            loop.quit();
                                                        });
    const QMetaObject::Connection c2 =
        QObject::connect(client, &IGarminDownloadClient::listFailed, &loop, [&](QUuid id, GarminListFailure) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = false;
            loop.quit();
        });
    QTimer::singleShot(kListTimeoutMs, &loop, [&]() {
        if (!done) {
            done = true;
            loop.quit();
        }
    });

    client->listActivities(sinceGmt, reqId);
    loop.exec();

    QObject::disconnect(c1);
    QObject::disconnect(c2);
    return res;
}

bool GarminConnect::open(QStringList& errors)
{
    IGarminDownloadClient* client = ensureClient();
    if (!client) {
        errors << tr("Garmin Connect: no embedded session is available.");
        return false;
    }

    const QString dir = resolveConfigDir();
    const GarminTokenStore::LoadResult r = GarminTokenStore::loadChecked(dir);

    // REQ-006: a token file wider than owner-only 0600 is REFUSED — force a
    // fresh SSO (do NOT attempt a download with an unsafe session).
    if (r.isRejected()) {
        errors << tr("Garmin Connect: the stored session file '%1' has unsafe permissions; please sign in again.")
                      .arg(r.path);
        return false;
    }
    // No stored session yet — the caller must run the credentials wizard.
    if (!r.isOk()) {
        errors << tr("Garmin Connect: no stored session found; please sign in again.");
        return false;
    }

    // REQ-005 / REQ-NF-Compat-001(b): silent reauth from the stored TOKENS only.
    if (!blockingRestore(QString::fromUtf8(r.bytes))) {
        errors << tr("Garmin Connect: could not restore the stored session; please sign in again.");
        return false;
    }

    // REQ-017 clause (d) / DEC-garmin-021 — the session is now open: latch the
    // account epoch AND the garmin_user_id it belongs to. A FORCED latch (not
    // ensureSessionLatched) so a close()+open(), or a reconnect through this same
    // instance, correctly rebinds to the account that is current now. A FAILED
    // open() latches nothing — it never claimed a session.
    latchSession();
    return true;
}

bool GarminConnect::close()
{
    // Bounded teardown of the owned host (the chain dtor quit()+wait()s its
    // thread — never terminate() from here). Tokens persist on disk. An injected
    // client (tests) is NOT owned, so it is left intact.
    delete m_chain;
    m_chain = nullptr;
    delete m_adapter;
    m_adapter = nullptr;
    if (!m_injectedClient)
        m_client = nullptr; // production seam pointed into the (now-gone) chain
    return true;
}

void GarminConnect::persistConnectSuccess(const QString& garminUserId, const QString& tokenBlob)
{
    // REQ-008 (DEC-garmin-019 C) — the production caller of the connect-success
    // producer (closes A3-R008-01). The wizard hands us the id-gated auth result;
    // we resolve the athlete config dir and persist tokens.json + active-account.json
    // atomically (0600) via the already-tested static producer. The blob crosses the
    // page/wizard boundary as a QString (GarminAuthSuccess::tokenBlob); tokens.json
    // stores the raw UTF-8 bytes.
    GarminTokenStore::persistConnectSuccess(resolveConfigDir(), garminUserId, tokenBlob.toUtf8());
}

void GarminConnect::disconnectService()
{
    // REQ-008 (DEC-garmin-019 C) — the production caller of Disconnect (DES-002).
    // Deletes tokens.json + active-account.json (account no longer connected) while
    // PRESERVING imported-<uid>.json / backfill-state-<uid>.json (REQ-012). The
    // risky delete logic lives here (testable), NOT in the generic deleteClicked().
    const QString dir = resolveConfigDir();
    GarminTokenStore::clearAccount(dir);

    // REQ-017 Slice A (DEC-garmin-021 B) — and invalidate every session that was
    // opened against the account just removed, including sessions held by OTHER,
    // still-live GarminConnect instances over this athlete's config dir. Deliberately
    // done by bumping a shared counter rather than by reaching into those instances:
    // blockingDownload() runs a nested QEventLoop, so this call can execute inside
    // another instance's live download frame, where touching its state (or deleting
    // its client) would be a use-after-free. A bump mutates nothing but the map;
    // the affected instances notice on their own next call.
    GarminAccountEpoch::bump(dir);
}

bool GarminConnect::readFile(QByteArray* data, QString remotename, QString remoteid)
{
    if (data == nullptr || m_client == nullptr)
        return false;

    // REQ-017 (DEC-garmin-021 B) — bind to the account session before deciding
    // anything (a no-op on the normal path: open() already latched).
    ensureSessionLatched();

    // REQ-017 clause (a) — FAIL CLOSED on the in-memory binding FIRST: this
    // session's account was disconnected, so it may not download regardless of
    // what tokens.json says now (a reconnect would satisfy the disk predicate
    // below while leaving this session's account gone). Zero disk I/O.
    //
    // DEC-garmin-022 (B-R017-06) — REFUSE, AND SAY SO. The sync/auto-download
    // callers DISCARD this bool and wait on the readComplete signal to advance;
    // a silent refusal hung the dialog on "Downloading n of N" forever and leaked
    // the QByteArray they preallocated for us (their completion handler is what
    // frees it). So post a labelled completion carrying the still-EMPTY buffer and
    // still return false: the loop moves on, the buffer is freed exactly once, and
    // the empty bytes yield no ride, so the caller takes its FAILURE branch —
    // nothing here can be mistaken for a successful download.
    if (sessionSuperseded()) {
        postReadComplete(data, remotename,
                         tr("Garmin Connect: this session's account was disconnected; please sign in again."));
        return false;
    }

    // DEC-garmin-020 — FAIL CLOSED (A3-R012-F1). The SECOND layer, deliberately
    // KEPT: the stored credential must also still be present and acceptable at
    // call time. Refuse BEFORE any network work, so nothing is downloaded from —
    // or staged for — an account the user has already removed. Nothing is staged;
    // as above (DEC-garmin-022) the refusal itself IS reported, so the loop moves
    // on and the caller's buffer is freed.
    //
    // B-R017-11 — the wording is readdir()'s, VERBATIM, for this same predicate
    // (see the matching accountStillConnected() branch there). The two entry
    // points describe an identical condition, so a user who provokes it by
    // listing and a user who provokes it by downloading must read the same
    // sentence; the previous text here was the SUPERSEDED-session wording, which
    // is a different condition (this instance's account is gone versus no account
    // is connected at all) and is still used by the guard immediately above.
    if (!accountStillConnected()) {
        postReadComplete(data, remotename, tr("Garmin Connect: no connected account; please sign in again."));
        return false;
    }

    // DEC-016 attempt 1 — request ORIGINAL (FIT); the payload is ZIP-wrapped.
    const DownloadResult original = blockingDownload(QStringLiteral("ORIGINAL"), remoteid);
    if (original.ok) {
        QByteArray inner;
        if (unzipFirstEntry(original.bytes, &inner) && looksLikeFit(inner)) {
            // REQ-017 clause (c) — POST-DOWNLOAD, PRE-STAGE recheck. blockingDownload
            // ran a nested QEventLoop, so a Disconnect can have landed WHILE the
            // request was in flight; the entry gates above are stale by now. DEC-021
            // scopes this to discard-only (there is no cancel primitive in
            // GarminWorker/PyEmbeddedAdapter and none is added here): the request ran
            // to completion, and its result is dropped — not staged, no sidecar
            // record, and no TCX retry either.
            //
            // DEC-garmin-023 (B-R017-10): the discard is REPORTED on the explicit
            // failure channel. It used to `return false` in silence, which hung the
            // sync dialog on "Downloading n of N" and leaked the caller's buffer —
            // the very defect DEC-022 closed for the two entry guards. Its own
            // reason: this one means "we did download your activity and then threw
            // it away", which is not the same news as "we never asked".
            if (!downloadResultStillWanted()) {
                postReadFailed(data, remotename,
                               tr("Garmin Connect: the account was disconnected while this activity was "
                                  "downloading; it was discarded."));
                return false;
            }
            *data = inner; // stage the UNZIPPED FIT bytes
            const QString staged = QStringLiteral("garmin-%1.fit").arg(remoteid);
            recordImport(remoteid, staged); // DES-010 steps 5e/6 (before completion)
            postReadComplete(data, staged, tr("Completed."));
            return true;
        }
        // 200 but not FIT (ZIP wrapping tcx/gpx, empty bytes, HTML page): fall
        // through to the TCX retry (content-sniff backstop).
    } else if (original.failureKind == GarminDownloadFailure::RateLimit) {
        // DEC-016: RateLimited → FAIL fast. NO second request (anti retry-storm).
        //
        // DEC-garmin-023 (B-R017-10): an ORDINARY failure — no disconnect, nothing
        // wrong with the account — and one of the two everyday ways a download does
        // not happen. Its reason is kept separate from every other site because the
        // advice it implies is unique: wait, then retry. Reporting it does NOT
        // retry it (still exactly one request, still false).
        postReadFailed(data, remotename,
                       tr("Garmin Connect: rate limited by the server; this activity was not downloaded. "
                          "Please try again later."));
        return false;
    }
    // Network / Unknown (DEC-016 Assumption-B: Network legitimately conflates a
    // real network failure with a 404 "no FIT original"), or a non-FIT 200 →
    // retry once as TCX.

    // REQ-017 clauses (a)+(c) — the retry is a SECOND network request, so the
    // recheck has to happen BEFORE it is issued as well: a Disconnect that landed
    // during the ORIGINAL attempt must leave this session issuing no further
    // download calls at all, not merely discarding what they return.
    //
    // DEC-garmin-023 (B-R017-10): reported, with its own reason — nothing was
    // downloaded here, so telling the user it was "discarded" would be a lie.
    if (!downloadResultStillWanted()) {
        postReadFailed(data, remotename,
                       tr("Garmin Connect: the account was disconnected; the TCX retry for this activity was "
                          "not attempted."));
        return false;
    }

    const DownloadResult tcx = blockingDownload(QStringLiteral("TCX"), remoteid);
    if (tcx.ok) {
        // REQ-017 clause (c) — the same post-download, pre-stage recheck on the
        // retry path (the TCX request is a second nested event loop and a second
        // window for a Disconnect to land).
        //
        // DEC-garmin-023 (B-R017-10): reported, and distinct from the FIT-path
        // discard above so the log says WHICH request was thrown away.
        if (!downloadResultStillWanted()) {
            postReadFailed(data, remotename,
                           tr("Garmin Connect: the account was disconnected while the TCX retry for this "
                              "activity was downloading; it was discarded."));
            return false;
        }
        *data = tcx.bytes; // TCX is raw XML, not ZIP-wrapped
        const QString staged = QStringLiteral("garmin-%1.tcx").arg(remoteid);
        recordImport(remoteid, staged); // DES-010 steps 5e/6 (before completion)
        postReadComplete(data, staged, tr("Completed."));
        return true;
    }

    // DEC-016 exhausted: neither ORIGINAL nor TCX produced a usable activity.
    //
    // DEC-garmin-023 (B-R017-10): the second ORDINARY failure, and the everyday
    // one — a 404 for the original plus a failed TCX is simply an activity Garmin
    // will not hand over. Silence here left the sync dialog stuck on this row
    // forever; now the row says so and the loop moves to the next activity.
    postReadFailed(data, remotename, tr("Garmin Connect: this activity could not be downloaded as either FIT or TCX."));
    return false;
}

QList<CloudServiceEntry*> GarminConnect::readdir(QString path, QStringList& errors, QDateTime from, QDateTime to)
{
    Q_UNUSED(path);
    Q_UNUSED(to);
    QList<CloudServiceEntry*> returning;

    // DES-010 step 1 / REQ-NF-Perf-002 — reject a concurrent sync; the running
    // one continues. FIRST check, before any I/O or worker op.
    if (m_syncInProgress) {
        errors << tr("Garmin Connect: sync already in progress.");
        return returning;
    }
    m_syncInProgress = true;
    // RAII reset so every early return clears the guard (never wedges).
    struct InProgressGuard
    {
        bool* flag;
        ~InProgressGuard() { *flag = false; }
    } guard{&m_syncInProgress};

    IGarminDownloadClient* client = m_client;
    if (client == nullptr) {
        errors << tr("Garmin Connect: no embedded session is available.");
        return returning;
    }

    // REQ-017 (DEC-garmin-021 B) — bind to the account session before deciding
    // anything (a no-op on the normal path: open() already latched).
    ensureSessionLatched();

    // REQ-017 clause (a) — FAIL CLOSED on the in-memory binding FIRST, before the
    // listing is issued. This session's account was disconnected, so it enumerates
    // nothing — even if the athlete has since reconnected and tokens.json is
    // perfectly valid again (which would satisfy the DEC-020 check below). Zero
    // disk I/O.
    if (sessionSuperseded()) {
        errors << tr("Garmin Connect: this session's account was disconnected; please sign in again.");
        return returning;
    }

    // DEC-garmin-020 — FAIL CLOSED (A3-R012-F1), before the listing is issued. The
    // SECOND layer, deliberately KEPT. Checked separately from (and ahead of) the
    // uid below: the uid lives in active-account.json, so it cannot speak for the
    // CREDENTIAL — a present-but-permission-rejected tokens.json, or a tokens.json
    // deleted on its own, leaves the uid resolvable while the account is not usable.
    if (!accountStillConnected()) {
        errors << tr("Garmin Connect: no connected account; please sign in again.");
        return returning;
    }

    // DES-010 step 2 — the active account + its per-account sidecar dir. REQ-017
    // clause (d) / A3-R012-F10: the uid is the one LATCHED at session open, not a
    // fresh read of active-account.json — a mid-sync rewrite of that file must not
    // be able to repoint this session's dedup map (nor, via recordImport, the file
    // its imports are written to).
    const QString dir = resolveConfigDir();
    const QString uid = m_openedUserId;
    if (uid.isEmpty()) {
        errors << tr("Garmin Connect: no connected account; please sign in again.");
        return returning;
    }

    // DES-010 step 3 — determine the "since" timestamp: the passed `from`, else
    // backfill-state's lastSuccessStartTimeGMT, else now()-7d. startTimeGMT is
    // Garmin's server-side clock — never the local clock.
    QString sinceGmt;
    if (from.isValid()) {
        sinceGmt = from.toUTC().toString(QString::fromLatin1(kGarminTimeFormat));
    } else {
        const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(dir, uid);
        if (bf.isOk() && !bf.state.lastSuccessStartTimeGMT.isEmpty())
            sinceGmt = bf.state.lastSuccessStartTimeGMT;
        else
            sinceGmt = QDateTime::currentDateTimeUtc()
                           .addDays(-kDefaultSinceDays)
                           .toString(QString::fromLatin1(kGarminTimeFormat));
    }

    // DES-010 step 4 — drive the worker list op (Slice A), off the caller thread.
    const ListResult listed = blockingList(sinceGmt);
    if (!listed.ok) {
        errors << tr("Garmin Connect: could not list activities; please try again.");
        return returning;
    }

    // DES-010 step 5a — Tier-1 dedup: filter OUT ids already in imported-<uid>.json
    // so the base machinery never issues a redundant readFile/download.
    const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(dir, uid);
    for (const GarminActivitySummary& s : listed.summaries) {
        if (imported.isOk() && imported.contains(s.activityId))
            continue; // found → short-circuit, no download

        CloudServiceEntry* e = newCloudServiceEntry();
        e->isDir = false;
        e->id = s.activityId;                                        // remoteid → readFile
        e->name = QStringLiteral("garmin-%1.fit").arg(s.activityId); // natural staging name
        e->modified = parseGarminTime(s.startTimeGMT);               // server-side timestamp
        returning << e;

        // Remember the server-side startTimeGMT so the subsequent readFile can
        // record it (the base machinery hands readFile only name + remoteid).
        m_pendingStartTimes.insert(s.activityId, s.startTimeGMT);
    }

    return returning;
}

void GarminConnect::recordImport(const QString& activityId, const QString& stagedFilename)
{
    // REQ-017 clause (d) / A3-R012-F10 — record against the account this session
    // was OPENED against. Re-resolving active-account.json here (the previous
    // behaviour) meant a rewrite of that file between readdir and readFile silently
    // moved the import into another account's sidecar — or, if it had been deleted,
    // into no sidecar at all, breaking dedup without a trace.
    ensureSessionLatched();
    const QString dir = resolveConfigDir();
    const QString uid = m_openedUserId;
    if (dir.isEmpty() || uid.isEmpty())
        return; // nothing to key the per-account sidecar on (e.g. readFile unit tests)

    const QString startTimeGMT = m_pendingStartTimes.value(activityId);

    // DES-010 step 5e — record the import (read-modify-write, atomic 0600).
    GarminSidecarStore::ImportedEntry entry;
    entry.startTimeGMT = startTimeGMT;
    entry.localFilename = stagedFilename;
    GarminSidecarStore::recordImported(dir, uid, activityId, entry);

    // DES-010 step 6 — advance the resume cursor to this activity's server time.
    const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(dir, uid);
    GarminSidecarStore::BackfillState state = bf.isOk() ? bf.state : GarminSidecarStore::BackfillState{};
    state.lastSuccessStartTimeGMT = startTimeGMT;
    GarminSidecarStore::saveBackfillState(dir, uid, state);

    // DES-010 step 7 (OUT OF SCOPE this slice — REQ-NF-Obs-001): the ErrorBus
    // success event (count + duration) is a later item. TODO(REQ-NF-Obs-001).
}

// B-R007-01 / REQ-NF-Perf-003: deliver readComplete as a QUEUED self-post rather
// than synchronously inside readFile()'s call frame. The CloudService auto-download
// caller sets up a QEventLoop and only *then* calls loop.exec(); a synchronous emit
// arrives before the loop runs, where quit() is a no-op, so the caller blocks its
// full 30s watchdog per activity. Deferring the emit onto the event queue lets the
// caller's loop observe it promptly. DES-014: GarminConnect is intentionally NOT its
// own Q_OBJECT and this adds none.
//
// The post is queued through m_completionContext — a bare QObject member of this
// GarminConnect (A3-R007-01). The context object is what determines both (1) the
// event loop the post is delivered on and (2) when a still-pending post is
// cancelled. m_completionContext is constructed with GarminConnect on the CALLING
// thread and never moveToThread'd, so its thread affinity is the caller's event
// loop — the same loop m_client lives on (in production only the worker crosses to
// the download thread; the client and this stay on the caller). So the post lands
// on the same loop a `this`-context or m_client-context post would.
//
// Why NOT m_client as the context: Qt cancels a pending queued invoke only when its
// *context* object is destroyed, never when a merely-captured object (`this`) is.
// m_client is injected and unowned; a GarminConnect destroyed while m_client
// outlives it, with a post in flight, would leave a lambda that dereferences the
// freed GarminConnect (use-after-free). Binding the context to a member ties the
// pending post to GarminConnect's own lifetime: the member is destroyed during
// ~GarminConnect (before the CloudService base is torn down), so Qt cancels the
// post BEFORE `this` is gone. (`this` itself cannot be the context: the readFile
// unit test's stub CloudService is deliberately non-QObject, so `this` is not a
// QObject in that TU.) DES-014: GarminConnect is still NOT its own Q_OBJECT — a
// plain QObject member adds no signals/slots and no moc obligation.
//
// Lifetime: `data` is a raw QByteArray* owned by the caller and preallocated before
// readFile(); the caller does not free it before its loop.exec() runs the post, so
// the captured pointer stays valid. We capture the pointer (and the computed name)
// by value — never a reference to anything with a shorter lifetime than the post.
// Ownership is unchanged by DEC-garmin-022: this posts the caller's buffer back
// exactly once per readFile() call (success OR refusal) and never frees it — the
// caller's completion handler is the sole owner and the sole deleter.
void GarminConnect::postReadComplete(QByteArray* data, const QString& name, const QString& message)
{
    QMetaObject::invokeMethod(
        &m_completionContext, [this, data, name, message]() { notifyReadComplete(data, name, message); },
        Qt::QueuedConnection);
}

// DEC-garmin-023 (B-R017-10) — the failure twin of postReadComplete. Every word
// of the rationale above applies unchanged: the same m_completionContext, so the
// post lands on the same event loop and is cancelled by the same lifetime; the
// same captured-by-value pointer; the same ownership rule — this posts the
// CALLER'S buffer back and never frees it. The only difference is the channel:
// CloudService::readFailed, which says "this read did not happen" in a way
// tr("Completed.")-on-success cannot. readFile() calls exactly one of the two per
// invocation, so the consumers' `delete data` runs exactly once.
void GarminConnect::postReadFailed(QByteArray* data, const QString& name, const QString& reason)
{
    QMetaObject::invokeMethod(
        &m_completionContext, [this, data, name, reason]() { notifyReadFailed(data, name, reason); },
        Qt::QueuedConnection);
}

// Static-init registration, mirroring the precedent in Selfloops.cpp /
// Strava.cpp. Runs before main(); the factory rejects duplicates so this is
// safe even if the translation unit were somehow linked twice.
static bool addGarminConnect()
{
    CloudServiceFactory::instance().addService(new GarminConnect(NULL));
    return true;
}

static bool addedGarminConnect = addGarminConnect();
