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

// Same verbatim-string convention DES-010 uses (GarminConnect.cpp); parsed
// only for the hard-cap/range-validity check below - range FILTERING and
// cursor comparisons stay on the lexicographically-sortable string form.
const char* const kGarminTimeFormat = "yyyy-MM-dd HH:mm:ss";

// DEC-070/B-STAGE9-83 — the staged extension follows these signatures, not
// an assumption about which shape a given download takes.
bool startsWithZipSignature(const QByteArray& bytes)
{
    return bytes.startsWith(QByteArrayLiteral("PK\x03\x04"));
}

bool startsWithGzipSignature(const QByteArray& bytes)
{
    return bytes.startsWith(QByteArrayLiteral("\x1f\x8b"));
}

// DEC-073/B-STAGE9-92 — signature level only, same idiom as the ZIP check
// above: the FIT parser (not this controller) validates the rest of the
// file. FIT's 14-byte header carries ".FIT" at bytes 8-11.
bool startsWithFitSignature(const QByteArray& bytes)
{
    return bytes.size() >= 14 && bytes.mid(8, 4) == QByteArrayLiteral(".FIT");
}

// DEC-072/DEC-073 — mirrors CloudService.cpp's gUncompress idiom
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

QDateTime parseGarminTime(const QString& s)
{
    QDateTime dt = QDateTime::fromString(s, QString::fromLatin1(kGarminTimeFormat));
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    if (dt.isValid())
        dt.setTimeSpec(Qt::UTC);
    return dt;
}

// B-STAGE9-107 — DEC-075's refusal is deliberately RECOVERABLE
// (SidecarPermissionsRejected: a chmod fixes it; PendingManifestMalformed: an
// edit fixes it), so the message must say which one and how, not collapse
// both into "could not persist backfill state". Re-loads the current status
// rather than threading it through every call site (DEC-075 forbids a
// load-first fix AT the writers themselves; this reads AFTER the writer has
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
    // B-STAGE9-118 — Ok/NotFound/Torn share the one generic message below;
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

    // B-R010-05 — see SessionCheck's declaration; a Paused/SessionInvalidated
    // helper so the pre-request/post-download call sites below stay one-liners.
    auto sessionInvalidated = [&sessionStillValid]() { return sessionStillValid && !sessionStillValid(); };

    const QDateTime startDt = parseGarminTime(rangeStartGmt);
    const QDateTime endDt = parseGarminTime(rangeEndGmt);
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

    // DES-009 "Resume behaviour" - prior progress inside this range wins over
    // rangeStartGmt. A state file for a DIFFERENT account is never consulted:
    // m_uid keys the sidecar filename (DES-002 structural partition).
    const GarminSidecarStore::BackfillLoadResult bf = GarminSidecarStore::loadBackfillState(m_configDir, m_uid);
    QString cursor = rangeStartGmt;
    QString priorSuccess;
    if (bf.isOk() && !bf.state.lastSuccessStartTimeGMT.isEmpty() && bf.state.lastSuccessStartTimeGMT >= rangeStartGmt &&
        bf.state.lastSuccessStartTimeGMT <= rangeEndGmt) {
        cursor = bf.state.lastSuccessStartTimeGMT;
        priorSuccess = cursor;
    }

    // Persist the range BEFORE any network op so a hard crash before the
    // first success still leaves resumable evidence on disk (DES-009: next
    // launch sees the state file and offers resume). A write failure here
    // means that guarantee never held, so bail rather than proceed as if it
    // had (mirrors the TransientError/TornWrite Paused-with-message shape).
    {
        GarminSidecarStore::BackfillState initial;
        initial.lastSuccessStartTimeGMT = priorSuccess;
        initial.rangeStart = rangeStartGmt;
        initial.rangeEnd = rangeEndGmt;
        if (!GarminSidecarStore::saveBackfillState(m_configDir, m_uid, initial)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }
    }

    // B-R010-05 — pre-request check, mirroring readdir()'s sessionSuperseded()/
    // accountStillConnected() gate before ITS listing call.
    if (sessionInvalidated()) {
        result.outcome = Outcome::Paused;
        result.pauseReason = PauseReason::SessionInvalidated;
        result.message = tr("Garmin Connect: the account session is no longer valid; backfill paused.");
        return result;
    }

    // DES-009 "Paging" - a single listActivities(cursor) call; the real
    // garminconnect library pages 20-at-a-time INSIDE that call (verified
    // against the wheel - see the REQ-010 build report), so there is no
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
    for (const GarminActivitySummary& s : listed.summaries) {
        // DEC-078 — `cursor` above is ONLY the blockingList() paging argument;
        // the filter reads `priorSuccess` (exclusive, only when a prior
        // success is actually in range) and `rangeStartGmt` (inclusive)
        // separately, so an activity sitting exactly on `rangeStartGmt` with
        // no prior success is never mistaken for one already landed.
        if (!priorSuccess.isEmpty() && s.startTimeGMT <= priorSuccess)
            continue;
        if (s.startTimeGMT < rangeStartGmt || s.startTimeGMT > rangeEndGmt)
            continue;
        if (imported.isOk() && imported.contains(s.activityId))
            continue; // Tier-1 dedup (DES-010), defense in depth
        filtered << s;
    }
    std::sort(filtered.begin(), filtered.end(), [](const GarminActivitySummary& a, const GarminActivitySummary& b) {
        return a.startTimeGMT < b.startTimeGMT;
    });

    for (const GarminActivitySummary& s : filtered) {
        // DES-009 step 6 / cancellation-latency caveat (A2-007): checked at
        // the loop head only - a per-activity download+write already in
        // flight always finishes.
        if (m_cancelRequested) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::UserCancelled;
            return result;
        }
        // B-R010-05 — pre-request check, loop head (mirrors readFile()'s entry
        // guards): before EACH per-activity download this run issues.
        if (sessionInvalidated()) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::SessionInvalidated;
            result.message = tr("Garmin Connect: the account session is no longer valid; backfill paused.");
            return result;
        }

        const DownloadResult dl = blockingDownload(s.activityId);
        if (!dl.ok) {
            // DES-005 already retried this at the Python layer; a failure
            // reaching here means retries are exhausted (design.md's table).
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TransientError;
            result.message = tr("Garmin Connect: could not download activity %1.").arg(s.activityId);
            return result;
        }

        // B-R010-05 — post-download, pre-stage recheck (REQ-017 clause c
        // mirror): blockingDownload() ran a nested QEventLoop, so a disconnect
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

        // DEC-073/B-STAGE9-92 — the accept-set is exactly two shapes: a bare
        // FIT/ZIP, or a single complete gzip member whose inflated bytes are
        // themselves FIT-or-ZIP. One inflate, never two: inflateGzipMember
        // already refuses an incomplete/nested/empty member by returning
        // empty (never falls back to the original bytes), so ONE signature
        // check on `stageBytes` afterwards covers the bare payload AND the
        // post-inflate result identically - nothing here re-derives a shape.
        // Anything that doesn't resolve to ZIP-or-FIT pauses BEFORE staging,
        // BEFORE recordPendingBackfill and BEFORE the cursor advances.
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
            // DES-006 torn-write recovery: do NOT advance the cursor past an
            // activity that never made it to disk intact - the next start()
            // re-fetches it rather than silently skipping it forever.
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TornWrite;
            result.message = tr("Garmin Connect: could not stage activity %1.").arg(s.activityId);
            return result;
        }

        // DEC-071 — the stage-time write records PENDING (bytes landed, not
        // yet import-complete), never recordImported: recordImported means
        // "downloaded", and the skip predicate above reads it as "imported",
        // so writing it here is exactly the B-STAGE9-79 defect (a cancelled
        // hand-off to RideImportWizard would orphan this id forever).
        // GarminBackfillDialog promotes this entry to imported-<uid>.json
        // only after RideCache confirms the ride actually landed. A false
        // return here means neither the dedup record nor the resume cursor
        // made it to disk despite the payload bytes landing intact; advancing
        // in-memory progress anyway would break DES-009's resumability
        // contract (a crash right after would resume from unsaved state).
        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = s.startTimeGMT;
        entry.localFilename = QFileInfo(payloadPath).fileName();
        if (!GarminSidecarStore::recordPendingBackfill(m_configDir, m_uid, s.activityId, entry)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }

        // Pending-then-cursor, atomically, before progress is reported below
        // (DEC-071's crash order): a crash between this write and the next
        // leaves a `pending` entry with the cursor not yet past it, which is
        // exactly DES-009's existing "never record success past an unwritten
        // file" contract, now covering import-completion too.
        GarminSidecarStore::BackfillState advanced;
        advanced.lastSuccessStartTimeGMT = s.startTimeGMT;
        advanced.rangeStart = rangeStartGmt;
        advanced.rangeEnd = rangeEndGmt;
        if (!GarminSidecarStore::saveBackfillState(m_configDir, m_uid, advanced)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = describeBackfillPersistFailure(m_configDir, m_uid);
            return result;
        }

        ++result.importedCount;
        // B-R010-04: report AFTER the cursor/dedup writes above, so a caller
        // that reacts to this (e.g. queues the staged payload for RideImportWizard)
        // never sees an activity this run has not yet durably recorded.
        if (onProgress)
            onProgress(s.activityId, payloadPath, result.importedCount);
    }

    result.outcome = Outcome::Done;
    return result;
}
