/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminBackfillController.h"

#include "AtomicFile.h"
#include "GarminSidecarStore.h"
#include "GarminTime.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>
#include <QUuid>

#ifdef Q_CC_MSVC
#    include <QtZlib/zlib.h>
#else
#    include <zlib.h>
#endif

#include <algorithm>

namespace {
// Generous bound so production never wedges the caller's event loop; unit
// tests reply on the first loop turn and never approach this.
constexpr int kBlockingTimeoutMs = 60000;

// The staged extension follows these signatures, not
// an assumption about which shape a given download takes.
bool startsWithZipSignature(const QByteArray& bytes)
{
    return bytes.startsWith(QByteArrayLiteral("PK\x03\x04"));
}

bool startsWithGzipSignature(const QByteArray& bytes)
{
    return bytes.startsWith(QByteArrayLiteral("\x1f\x8b"));
}

// Signature level only, same idiom as the ZIP check
// above: the FIT parser (not this controller) validates the rest of the
// file. FIT's 14-byte header carries ".FIT" at bytes 8-11.
bool startsWithFitSignature(const QByteArray& bytes)
{
    return bytes.size() >= 14 && bytes.mid(8, 4) == QByteArrayLiteral(".FIT");
}

// Mirrors CloudService.cpp's gUncompress idiom
// (inflateInit2(&strm, 15 + 16) selects gzip-member decoding). Returns
// non-empty ONLY when zlib reports the member fully consumed: `Z_STREAM_END`
// reached with `strm.avail_in == 0`. A truncated member (loop exits on
// Z_OK/Z_BUF_ERROR) or a complete member followed by trailing bytes
// (leftover avail_in) both return empty — the caller treats empty as a
// refusal, never as "fall back to the original bytes".
QByteArray inflateGzipMember(const QByteArray& data)
{
    if (data.size() <= 4)
        return QByteArray();

    QByteArray result;
    z_stream strm;
    static const int kChunkSize = 1024;
    char out[kChunkSize];

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    if (inflateInit2(&strm, 15 + 16) != Z_OK)
        return QByteArray();

    int ret;
    do {
        strm.avail_out = kChunkSize;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_NO_FLUSH);
        if (ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&strm);
            return QByteArray();
        }

        result.append(out, kChunkSize - strm.avail_out);
    } while (ret != Z_STREAM_END && strm.avail_out == 0);

    const bool complete = (ret == Z_STREAM_END && strm.avail_in == 0);
    inflateEnd(&strm);
    return complete ? result : QByteArray();
}

// Refusal is deliberately RECOVERABLE
// (SidecarPermissionsRejected: a chmod fixes it; PendingManifestMalformed: an
// edit fixes it), so the message must say which one and how, not collapse
// both into "could not persist backfill state". Re-loads the current status
// rather than threading it through every call site (a load-first fix AT the writers themselves is ruled out; this reads AFTER the writer has
// already refused, purely to describe the refusal already decided).
QString describeBackfillPersistFailure(const QString& configDir, const QString& uid)
{
    const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(configDir, uid);
    switch (bf.status) {
    case GarminSidecarStore::LoadStatus::SidecarPermissionsRejected:
        return GarminBackfillController::tr(
                   "Garmin Connect: %1 has group- or other-readable permissions, so backfill state "
                   "cannot be updated. Run \"chmod 600\" on that file, then restart the backfill.")
            .arg(bf.path);
    case GarminSidecarStore::LoadStatus::PendingManifestMalformed:
        return GarminBackfillController::tr(
                   "Garmin Connect: %1's pending-activity list is malformed, so backfill state cannot "
                   "be updated. Move that file aside (or delete it to start a fresh backfill), then try again.")
            .arg(bf.path);
    // Ok/NotFound/Torn share the one generic message below;
    // enumerated explicitly (no `default:`), so a LoadStatus value added
    // later reaches the trailing generic `return` below rather than a silent
    // `default:` branch — this repo's build does not enable `-Wswitch`, so
    // that omission is not itself a compile-time diagnostic here.
    case GarminSidecarStore::LoadStatus::Ok:
    case GarminSidecarStore::LoadStatus::NotFound:
    case GarminSidecarStore::LoadStatus::Torn:
        return GarminBackfillController::tr("Garmin Connect: could not persist backfill state to %1.").arg(bf.path);
    }
    return GarminBackfillController::tr("Garmin Connect: could not persist backfill state to %1.").arg(bf.path);
}
} // namespace

GarminBackfillController::GarminBackfillController(IGarminDownloadClient* client, QString athleteConfigDir,
                                                   QString garminUserId)
    : m_client(client), m_configDir(std::move(athleteConfigDir)), m_uid(std::move(garminUserId))
{
}

QString GarminBackfillController::stagedPayloadPath(const QString& athleteConfigDir, const QString& activityId,
                                                    const QByteArray& bytes)
{
    QString ext;
    if (startsWithZipSignature(bytes))
        ext = QStringLiteral("zip");
    else if (startsWithGzipSignature(bytes))
        ext = QStringLiteral("gzip");
    else
        ext = QStringLiteral("fit");
    return QDir(GarminSidecarStore::directoryFor(athleteConfigDir))
        .filePath(QStringLiteral("backfill/garmin-%1.%2").arg(activityId, ext));
}

void GarminBackfillController::cancel()
{
    m_cancelRequested = true;
}

GarminBackfillController::ListResult GarminBackfillController::blockingList(const QString& sinceGmt)
{
    ListResult res;
    if (!m_client)
        return res;

    const QUuid reqId = QUuid::createUuid();
    QEventLoop loop;
    bool done = false;

    const QMetaObject::Connection c1 = QObject::connect(m_client, &IGarminDownloadClient::activitiesListed, &loop,
                                                        [&](QUuid id, QVector<GarminActivitySummary> summaries) {
                                                            if (done || id != reqId)
                                                                return;
                                                            done = true;
                                                            res.ok = true;
                                                            res.summaries = summaries;
                                                            loop.quit();
                                                        });
    const QMetaObject::Connection c2 =
        QObject::connect(m_client, &IGarminDownloadClient::listFailed, &loop, [&](QUuid id, GarminListFailure) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = false;
            loop.quit();
        });
    QTimer::singleShot(kBlockingTimeoutMs, &loop, [&]() {
        if (!done) {
            done = true;
            loop.quit();
        }
    });

    m_client->listActivities(sinceGmt, reqId);
    loop.exec();

    QObject::disconnect(c1);
    QObject::disconnect(c2);
    return res;
}

GarminBackfillController::DownloadResult GarminBackfillController::blockingDownload(const QString& activityId)
{
    DownloadResult res;
    if (!m_client)
        return res;

    const QUuid reqId = QUuid::createUuid();
    QEventLoop loop;
    bool done = false;

    const QMetaObject::Connection c1 =
        QObject::connect(m_client, &IGarminDownloadClient::downloaded, &loop, [&](QUuid id, QByteArray data) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = true;
            res.bytes = data;
            loop.quit();
        });
    const QMetaObject::Connection c2 =
        QObject::connect(m_client, &IGarminDownloadClient::downloadFailed, &loop, [&](QUuid id, GarminDownloadFailure) {
            if (done || id != reqId)
                return;
            done = true;
            res.ok = false;
            loop.quit();
        });
    QTimer::singleShot(kBlockingTimeoutMs, &loop, [&]() {
        if (!done) {
            done = true;
            loop.quit();
        }
    });

    m_client->downloadActivity(activityId, QStringLiteral("ORIGINAL"), reqId);
    loop.exec();

    QObject::disconnect(c1);
    QObject::disconnect(c2);
    return res;
}

GarminBackfillController::Result GarminBackfillController::start(const QString& rangeStartGmt,
                                                                 const QString& rangeEndGmt,
                                                                 const ProgressCallback& onProgress,
                                                                 const SessionCheck& sessionStillValid)
{
    Result result;
    m_cancelRequested = false;

    // See SessionCheck's declaration; a Paused/SessionInvalidated
    // helper so the pre-request/post-download call sites below stay one-liners.
    auto sessionInvalidated = [&sessionStillValid]() { return sessionStillValid && !sessionStillValid(); };

    const QDateTime startDt = garminInstantFromString(rangeStartGmt);
    const QDateTime endDt = garminInstantFromString(rangeEndGmt);
    if (!startDt.isValid() || !endDt.isValid() || endDt < startDt) {
        result.outcome = Outcome::Rejected;
        result.pauseReason = PauseReason::InvalidRange;
        result.message = tr("Garmin Connect: invalid backfill range.");
        return result;
    }
    if (startDt.daysTo(endDt) > kHardCapDays) {
        result.outcome = Outcome::Rejected;
        result.pauseReason = PauseReason::InvalidRange;
        result.message = tr("Garmin Connect: backfill range exceeds the %1-day cap.").arg(kHardCapDays);
        return result;
    }

    // prior progress inside this range wins over
    // rangeStartGmt. A state file for a DIFFERENT account is never consulted:
    // m_uid keys the sidecar filename.
    const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(m_configDir, m_uid);
    QString cursor = rangeStartGmt;
    QString priorSuccess;
    // CONTROLLER branch: an unparseable stored cursor must
    // never narrow the range, so it falls through to rangeStartGmt below —
    // the isValid() guards on both sides make that the case.
    const QDateTime storedCursorInstant = garminInstantFromString(bf.state.lastSuccessStartTimeGMT);
    if (bf.isOk() && !bf.state.lastSuccessStartTimeGMT.isEmpty() && storedCursorInstant.isValid() &&
        storedCursorInstant >= startDt && storedCursorInstant <= endDt) {
        cursor = bf.state.lastSuccessStartTimeGMT;
        priorSuccess = cursor;
    }

    // Persist the range BEFORE any network op so a hard crash before the
    // first success still leaves resumable evidence on disk (next
    // launch sees the state file and offers resume). A write failure here
    // means that guarantee never held, so bail rather than proceed as if it
    // had (mirrors the TransientError/TornWrite Paused-with-message shape).
    // range-only — the cursor is never written from
    // this controller's own in-memory `priorSuccess`; saveBackfillRange
    // preserves whatever cursor is on disk, so a promotion racing this save
    // (e.g. another session's rideRegistrationCompleted) is never clobbered.
    {
        if (!GarminSidecarStore::saveBackfillRange(m_configDir, m_uid, rangeStartGmt, rangeEndGmt)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }
    }

    // pre-request check, mirroring readdir()'s sessionSuperseded()/
    // accountStillConnected() gate before ITS listing call.
    if (sessionInvalidated()) {
        result.outcome = Outcome::Paused;
        result.pauseReason = PauseReason::SessionInvalidated;
        result.message = tr("Garmin Connect: the account session is no longer valid; backfill paused.");
        return result;
    }

    // a single listActivities(cursor) call; the real
    // garminconnect library pages 20-at-a-time INSIDE that call (verified
    // against the wheel), so there is no
    // second wire round-trip for this controller to pace between.
    const ListResult listed = blockingList(cursor);
    if (!listed.ok) {
        result.outcome = Outcome::Paused;
        result.pauseReason = PauseReason::TransientError;
        result.message = tr("Garmin Connect: could not list activities for backfill.");
        return result;
    }

    // Garmin's default listing order is newest-first; resumability requires
    // OLDEST-first processing so last_success_startTimeGMT only ever advances
    // past activities already landed on disk - sort here rather than trust
    // the library's (or a test double's) ordering.
    QVector<GarminActivitySummary> filtered;
    const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(m_configDir, m_uid);
    const QDateTime priorSuccessInstant = priorSuccess.isEmpty() ? QDateTime() : garminInstantFromString(priorSuccess);
    for (const GarminActivitySummary& s : listed.summaries) {
        // `cursor` above is ONLY the
        // blockingList() paging argument; the filter reads `priorSuccess`
        // (STRICTLY BEFORE, only when a prior success is actually in range)
        // and the range bound (inclusive) separately, so an activity sitting
        // exactly on the range start OR an equal-second sibling of
        // `priorSuccess` is never mistaken for one already landed — the
        // adapter's own listing bound is already inclusive and
        // Tier-1 imported-id dedup below handles the exact-cursor duplicate.
        // An entry whose startTimeGMT fails to parse is
        // never excluded by either bound — this filter protects reachability,
        // the opposite of the store's completeness guard.
        const QDateTime entryInstant = garminInstantFromString(s.startTimeGMT);
        if (entryInstant.isValid()) {
            if (priorSuccessInstant.isValid() && entryInstant < priorSuccessInstant)
                continue;
            if (entryInstant < startDt || entryInstant > endDt)
                continue;
        }
        if (imported.isOk() && imported.contains(s.activityId))
            continue; // Tier-1 dedup, defense in depth
        if (bf.isOk() && bf.state.pending.contains(s.activityId))
            continue; // Already staged, awaiting promotion; not re-downloaded
        filtered << s;
    }
    std::sort(filtered.begin(), filtered.end(), [](const GarminActivitySummary& a, const GarminActivitySummary& b) {
        // Order on the (isValid, instant) pair — comparing
        // raw instants when either side is invalid is not a strict weak
        // ordering and makes std::sort undefined behaviour.
        const QDateTime da = garminInstantFromString(a.startTimeGMT);
        const QDateTime db = garminInstantFromString(b.startTimeGMT);
        if (da.isValid() != db.isValid())
            return da.isValid();
        return da.isValid() && da < db;
    });

    for (const GarminActivitySummary& s : filtered) {
        // / cancellation-latency caveat: checked at
        // the loop head only - a per-activity download+write already in
        // flight always finishes.
        if (m_cancelRequested) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::UserCancelled;
            return result;
        }
        // pre-request check, loop head (mirrors readFile()'s entry
        // guards): before EACH per-activity download this run issues.
        if (sessionInvalidated()) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::SessionInvalidated;
            result.message = tr("Garmin Connect: the account session is no longer valid; backfill paused.");
            return result;
        }

        const DownloadResult dl = blockingDownload(s.activityId);
        if (!dl.ok) {
            // Already retried this at the Python layer; a failure
            // reaching here means retries are exhausted.
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TransientError;
            result.message = tr("Garmin Connect: could not download activity %1.").arg(s.activityId);
            return result;
        }

        // post-download, pre-stage recheck (mirrors
        // the entry checks): blockingDownload() ran a nested QEventLoop, so a disconnect
        // or reconnect-to-a-different-account can have landed while the
        // request was in flight. Nothing is staged/recorded past this point.
        if (sessionInvalidated()) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::SessionInvalidated;
            result.message =
                tr("Garmin Connect: the account session became invalid while this activity was downloading; "
                   "it was discarded.");
            return result;
        }

        // The accept-set is exactly two shapes: a bare
        // FIT/ZIP, or a single complete gzip member whose inflated bytes are
        // themselves FIT-or-ZIP. One inflate, never two: inflateGzipMember
        // already refuses an incomplete/nested/empty member by returning
        // empty (never falls back to the original bytes), so ONE signature
        // check on `stageBytes` afterwards covers the bare payload AND the
        // post-inflate result identically - nothing here re-derives a shape.
        // Anything that doesn't resolve to ZIP-or-FIT pauses BEFORE staging
        // and BEFORE recordPendingBackfill.
        QByteArray stageBytes = startsWithGzipSignature(dl.bytes) ? inflateGzipMember(dl.bytes) : dl.bytes;
        if (!startsWithZipSignature(stageBytes) && !startsWithFitSignature(stageBytes)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::UndecodablePayload;
            result.message =
                tr("Garmin Connect: activity %1's payload could not be decoded; it was discarded.").arg(s.activityId);
            return result;
        }

        const QString payloadPath = stagedPayloadPath(m_configDir, s.activityId, stageBytes);
        QDir().mkpath(QFileInfo(payloadPath).absolutePath());
        if (!AtomicFile::writeOver(payloadPath, stageBytes)) {
            // torn-write recovery: no pending row is written for an
            // activity that never made it to disk intact - the next start()
            // re-fetches it rather than silently skipping it forever.
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TornWrite;
            result.message = tr("Garmin Connect: could not stage activity %1.").arg(s.activityId);
            return result;
        }

        // The stage-time write records PENDING (bytes landed, not
        // yet import-complete), never recordImported: recordImported means
        // "downloaded", and the skip predicate above reads it as "imported",
        // so writing it here is exactly the defect (a cancelled
        // hand-off to RideImportWizard would orphan this id forever).
        // GarminBackfillDialog promotes this entry (GarminSidecarStore::
        // promotePendingBackfill) only after RideCache confirms the
        // ride actually landed. A false return here means the dedup record
        // never made it to disk despite the payload bytes landing intact;
        // advancing in-memory progress anyway would break the
        // resumability contract (a crash right after would resume from
        // unsaved state).
        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = s.startTimeGMT;
        entry.localFilename = QFileInfo(payloadPath).fileName();
        if (!GarminSidecarStore::recordPendingBackfill(m_configDir, m_uid, s.activityId, entry)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }

        // The cursor is a completeness watermark,
        // advanced ONLY by promotion; staging never moves it. This write
        // persists rangeStart/rangeEnd (so a fresh controller instance still
        // resumes the correct window). It no longer touches
        // lastSuccessStartTimeGMT at all — saveBackfillRange leaves whatever
        // cursor is on disk untouched, so a promotion from another session
        // landing during this run's blockingList()/download is never
        // overwritten by this controller's stale in-memory `priorSuccess`.
        // Resumability across a crash between this write and the next rests
        // on the pending-skip: a re-run sees `s` still in `pending` and does
        // not re-download it.
        if (!GarminSidecarStore::saveBackfillRange(m_configDir, m_uid, rangeStartGmt, rangeEndGmt)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }

        ++result.importedCount;
        // Report AFTER the cursor/dedup writes above, so a caller
        // that reacts to this (e.g. queues the staged payload for RideImportWizard)
        // never sees an activity this run has not yet durably recorded.
        if (onProgress)
            onProgress(s.activityId, payloadPath, result.importedCount);
    }

    result.outcome = Outcome::Done;
    return result;
}
