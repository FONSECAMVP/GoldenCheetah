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
#include <QImage>
#include <QObject>
#include <QString>
#include <QStringList>

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

    // Test seam (garmin-fast) — inject a Python-free fake download/restore client
    // and override the athlete config dir, so open()/readFile() are exercisable
    // without the embedded-Python host. The injected client is NOT owned.
    GarminConnect(Context* context, IGarminDownloadClient* injectedClient, const QString& configDirOverride);

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

    // REQ-007 closure Slice 3 — the DEC-016 retry table: ORIGINAL(FIT) →
    // unzip+sniff → stage garmin-<id>.fit; RateLimited fails fast (no retry);
    // Network/Unknown/not-FIT retry once as TCX → stage garmin-<id>.tcx.
    bool readFile(QByteArray* data, QString remotename, QString remoteid) override;

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
