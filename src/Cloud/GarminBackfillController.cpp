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

#include <algorithm>

namespace {
// Generous bound so production never wedges the caller's event loop; unit
// tests reply on the first loop turn and never approach this.
constexpr int kBlockingTimeoutMs = 60000;

// Same verbatim-string convention DES-010 uses (GarminConnect.cpp); parsed
// only for the hard-cap/range-validity check below - range FILTERING and
// cursor comparisons stay on the lexicographically-sortable string form.
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
} // namespace

GarminBackfillController::GarminBackfillController(IGarminDownloadClient* client, QString athleteConfigDir,
                                                   QString garminUserId)
    : m_client(client), m_configDir(std::move(athleteConfigDir)), m_uid(std::move(garminUserId))
{
}

QString GarminBackfillController::stagedFitPath(const QString& athleteConfigDir, const QString& activityId)
{
    return QDir(GarminSidecarStore::directoryFor(athleteConfigDir))
        .filePath(QStringLiteral("backfill/garmin-%1.fit").arg(activityId));
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
        result.message = QStringLiteral("Garmin Connect: invalid backfill range.");
        return result;
    }
    if (startDt.daysTo(endDt) > kHardCapDays) {
        result.outcome = Outcome::Rejected;
        result.pauseReason = PauseReason::InvalidRange;
        result.message = QStringLiteral("Garmin Connect: backfill range exceeds the %1-day cap.").arg(kHardCapDays);
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
            result.message = QStringLiteral("Garmin Connect: could not persist backfill state.");
            return result;
        }
    }

    // B-R010-05 — pre-request check, mirroring readdir()'s sessionSuperseded()/
    // accountStillConnected() gate before ITS listing call.
    if (sessionInvalidated()) {
        result.outcome = Outcome::Paused;
        result.pauseReason = PauseReason::SessionInvalidated;
        result.message = QStringLiteral("Garmin Connect: the account session is no longer valid; backfill paused.");
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
        result.message = QStringLiteral("Garmin Connect: could not list activities for backfill.");
        return result;
    }

    // Garmin's default listing order is newest-first; resumability requires
    // OLDEST-first processing so last_success_startTimeGMT only ever advances
    // past activities already landed on disk - sort here rather than trust
    // the library's (or a test double's) ordering.
    QVector<GarminActivitySummary> filtered;
    const GarminSidecarStore::ImportedMap imported = GarminSidecarStore::loadImported(m_configDir, m_uid);
    for (const GarminActivitySummary& s : listed.summaries) {
        if (s.startTimeGMT <= cursor || s.startTimeGMT > rangeEndGmt)
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
            result.message = QStringLiteral("Garmin Connect: the account session is no longer valid; backfill paused.");
            return result;
        }

        const DownloadResult dl = blockingDownload(s.activityId);
        if (!dl.ok) {
            // DES-005 already retried this at the Python layer; a failure
            // reaching here means retries are exhausted (design.md's table).
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TransientError;
            result.message = QStringLiteral("Garmin Connect: could not download activity %1.").arg(s.activityId);
            return result;
        }

        // B-R010-05 — post-download, pre-stage recheck (REQ-017 clause c
        // mirror): blockingDownload() ran a nested QEventLoop, so a disconnect
        // or reconnect-to-a-different-account can have landed while the
        // request was in flight. Nothing is staged/recorded past this point.
        if (sessionInvalidated()) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::SessionInvalidated;
            result.message = QStringLiteral(
                "Garmin Connect: the account session became invalid while this activity was downloading; "
                "it was discarded.");
            return result;
        }

        const QString fitPath = stagedFitPath(m_configDir, s.activityId);
        QDir().mkpath(QFileInfo(fitPath).absolutePath());
        if (!AtomicFile::writeOver(fitPath, dl.bytes)) {
            // DES-006 torn-write recovery: do NOT advance the cursor past an
            // activity that never made it to disk intact - the next start()
            // re-fetches it rather than silently skipping it forever.
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::TornWrite;
            result.message = QStringLiteral("Garmin Connect: could not stage activity %1.").arg(s.activityId);
            return result;
        }

        // A false return here means the dedup record / resume cursor never
        // made it to disk despite the FIT bytes landing intact; advancing
        // in-memory progress anyway would break DES-009's resumability
        // contract (a crash right after would resume from unsaved state).
        GarminSidecarStore::ImportedEntry entry;
        entry.startTimeGMT = s.startTimeGMT;
        entry.localFilename = QFileInfo(fitPath).fileName();
        if (!GarminSidecarStore::recordImported(m_configDir, m_uid, s.activityId, entry)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message =
                QStringLiteral("Garmin Connect: could not record activity %1 as imported.").arg(s.activityId);
            return result;
        }

        GarminSidecarStore::BackfillState advanced;
        advanced.lastSuccessStartTimeGMT = s.startTimeGMT;
        advanced.rangeStart = rangeStartGmt;
        advanced.rangeEnd = rangeEndGmt;
        if (!GarminSidecarStore::saveBackfillState(m_configDir, m_uid, advanced)) {
            result.outcome = Outcome::Paused;
            result.pauseReason = PauseReason::StatePersistFailed;
            result.message = QStringLiteral("Garmin Connect: could not persist backfill state.");
            return result;
        }

        ++result.importedCount;
        // B-R010-04: report AFTER the cursor/dedup writes above, so a caller
        // that reacts to this (e.g. queues the staged FIT for RideImportWizard)
        // never sees an activity this run has not yet durably recorded.
        if (onProgress)
            onProgress(s.activityId, result.importedCount);
    }

    result.outcome = Outcome::Done;
    return result;
}
