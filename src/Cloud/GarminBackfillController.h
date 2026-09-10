/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-009 — GarminBackfillController: bulk backfill orchestration (REQ-010).
//
// Standalone, decoupled from GarminConnect/CloudService (DES-010's incremental
// sync owns that integration for REQ-008): this class drives one bounded-range
// backfill run to completion (Done) or interruption (Paused), reusing the
// already-built DES-002/DES-006 primitives (GarminSidecarStore, AtomicFile)
// and the same IGarminDownloadClient seam GarminConnect bridges through
// (blocking-over-async via a nested QEventLoop — mirrors
// GarminConnect::blockingList/blockingDownload).
//
// State machine (design.md DES-009): Idle -> Paging (list once from the resume
// cursor; the real garminconnect library pages 20-at-a-time INTERNALLY inside
// that single call, invisible to this controller - see the REQ-010 build
// report) -> per-activity (download -> write FIT atomically -> advance
// backfill-state -> record imported -> check cancellation) -> Done (range
// exhausted / empty result) or Paused (user cancel / retry-exhausted transient
// error). Cancellation is cooperative: checked at the per-activity loop head,
// same-thread (this controller never leaves the caller's thread — the nested
// QEventLoop keeps it pumping between per-activity network ops, so a
// UI-triggered cancel() lands between iterations, A2-007's accepted latency).
//
// Resumability (crash/SIGKILL): backfill-state-<uid>.json is written BEFORE
// the first download (persisting range_start/range_end) and re-written after
// EVERY successful per-activity write, so a hard kill at any point leaves
// last_success_startTimeGMT at the most recent activity actually landed on
// disk; the next start() resumes from there. A per-activity FIT write that
// fails (AtomicFile::writeOver returns false) does NOT advance the cursor, so
// that activity is retried on the next run (DES-006's read-side "torn write"
// recovery, realized here as "never record success past an unwritten file").

#ifndef GC_GarminBackfillController_h
#define GC_GarminBackfillController_h

#include "IGarminDownloadClient.h"

#include <QString>
#include <QVector>

class GarminBackfillController
{
  public:
    enum class Outcome { Done, Paused, Rejected };

    // Why start() returned something other than Done.
    enum class PauseReason {
        None,
        UserCancelled,      // cancel() observed at a loop-head check
        TransientError,     // DES-005 retry exhausted; listFailed/downloadFailed reached us
        TornWrite,          // AtomicFile::writeOver failed on the FIT file; cursor not advanced past it
        StatePersistFailed, // GarminSidecarStore::saveBackfillState/recordImported returned false
        InvalidRange        // Outcome::Rejected only: end < start, or span > the hard cap
    };

    struct Result
    {
        Outcome outcome = Outcome::Done;
        PauseReason pauseReason = PauseReason::None;
        int importedCount = 0; // activities successfully written+recorded this run
        QString message;       // set for TransientError / InvalidRange
    };

    // `client` is not owned; must outlive start(). `athleteConfigDir` +
    // `garminUserId` key the per-account sidecar files (DES-002), same
    // convention as GarminSidecarStore's other callers.
    GarminBackfillController(IGarminDownloadClient* client, QString athleteConfigDir, QString garminUserId);

    // REQ-010 — run (or resume) the backfill for [rangeStartGmt, rangeEndGmt]
    // (Garmin server-side timestamps, "yyyy-MM-dd HH:mm:ss", verbatim strings
    // - DES-010 format). If backfill-state-<uid>.json already carries a
    // lastSuccessStartTimeGMT inside this range, resumes from there instead of
    // rangeStartGmt (DES-009 "Resume behaviour"). An empty activity list is
    // Done, not an error (DES-009 "Empty result is success").
    Result start(const QString& rangeStartGmt, const QString& rangeEndGmt);

    // Cooperative cancellation (DES-009) — see class comment. Safe to call
    // re-entrantly from within a slot invoked while start() is running (same
    // thread only; this class is not thread-safe across threads).
    void cancel();

    // Where a given activity's FIT bytes are staged (test/diagnostic seam).
    static QString stagedFitPath(const QString& athleteConfigDir, const QString& activityId);

    // DES-009 "Paging" — the checkpoint granularity referenced by design.md;
    // does not change per-activity cancellation/resume correctness (checked
    // every activity, not just every page) but documents the number backfill
    // was designed against. See the REQ-010 build report for how this
    // reconciles with the real garminconnect library's OWN internal paging.
    static constexpr int kPageSize = 20;

    // PRD REQ-010 - "hard cap 5 years (configurable via advanced setting)".
    // No settings UI exists yet to make this configurable; hardcoded here as
    // the enforced ceiling until one lands.
    static constexpr int kHardCapDays = 5 * 365;

  private:
    struct ListResult
    {
        bool ok = false;
        QVector<GarminActivitySummary> summaries;
    };
    struct DownloadResult
    {
        bool ok = false;
        QByteArray bytes;
    };

    ListResult blockingList(const QString& sinceGmt);
    DownloadResult blockingDownload(const QString& activityId);

    IGarminDownloadClient* m_client;
    QString m_configDir;
    QString m_uid;
    bool m_cancelRequested = false;
};

#endif // GC_GarminBackfillController_h
