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
// backfill-state -> record pending -> check cancellation) -> Done (range
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

#include <QCoreApplication>
#include <QString>
#include <QVector>

#include <functional>

class GarminBackfillController
{
  public:
    // REQ-NF-i18n-001 (T-208): Result::message strings surface verbatim in
    // GarminBackfillDialog's progress label, so they are user-facing and must
    // be translatable even though this class is deliberately not a QObject
    // (DES-009 decoupling). The macro gives tr() the class's own context.
    // NOTE: it ends in a `private:` section — `public:` is re-opened below.
    Q_DECLARE_TR_FUNCTIONS(GarminBackfillController)

  public:
    enum class Outcome { Done, Paused, Rejected };

    // Why start() returned something other than Done.
    enum class PauseReason {
        None,
        UserCancelled,      // cancel() observed at a loop-head check
        TransientError,     // DES-005 retry exhausted; listFailed/downloadFailed reached us
        TornWrite,          // AtomicFile::writeOver failed on the staged file; cursor not advanced past it
        StatePersistFailed, // GarminSidecarStore::saveBackfillState/recordPendingBackfill returned false
        InvalidRange,       // Outcome::Rejected only: end < start, or span > the hard cap
        SessionInvalidated, // B-R010-05: `sessionStillValid` returned false (see SessionCheck)
        UndecodablePayload  // DEC-073: downloaded bytes did not resolve to a handled shape
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

    // B-R010-04 (UI wiring) — reported once per successfully-imported
    // activity, in PROCESSING (oldest-first) order, carrying its id, the
    // path start() actually staged its bytes under (DEC-070: extension
    // follows the sniffed payload, not a fixed assumption), and the running
    // Result::importedCount. A backfill dialog's progress label and its
    // post-run RideImportWizard file-list hand-off are both built on this;
    // the path must be taken verbatim, never re-derived on the read side.
    using ProgressCallback =
        std::function<void(const QString& activityId, const QString& stagedPath, int importedSoFar)>;

    // B-R010-05 — checked at the SAME two points GarminConnect::readFile()/
    // readdir() enforce their fail-closed pair (REQ-017 clause a + DEC-
    // garmin-020, clause c): once before EACH network request this run
    // issues, and again immediately after EACH nested-loop wait completes,
    // before anything is staged/recorded. Kept a std::function (not a
    // GarminConnect*) so this class stays decoupled from GarminConnect/
    // CloudService, per DES-009; a caller binds it to
    // GarminConnect::backfillSessionStillValid() at the call site instead.
    // Defaults to always-valid so every pre-existing caller (T-176..T-189) is
    // unaffected. false => Paused/SessionInvalidated, never Rejected: a
    // disconnect/reconnect mid-run is an interruption, not a bad request -
    // same class as cancel().
    using SessionCheck = std::function<bool()>;

    // REQ-010 — run (or resume) the backfill for [rangeStartGmt, rangeEndGmt]
    // (Garmin server-side timestamps, "yyyy-MM-dd HH:mm:ss", verbatim strings
    // - DES-010 format). If backfill-state-<uid>.json already carries a
    // lastSuccessStartTimeGMT inside this range, resumes from there instead of
    // rangeStartGmt (DES-009 "Resume behaviour"). An empty activity list is
    // Done, not an error (DES-009 "Empty result is success"). `onProgress`
    // defaults to a no-op so every pre-existing caller (T-176..T-187) is
    // unaffected.
    Result start(const QString& rangeStartGmt, const QString& rangeEndGmt,
                 const ProgressCallback& onProgress = ProgressCallback(),
                 const SessionCheck& sessionStillValid = SessionCheck());

    // Cooperative cancellation (DES-009) — see class comment. Safe to call
    // re-entrantly from within a slot invoked while start() is running (same
    // thread only; this class is not thread-safe across threads).
    void cancel();

    // Where a given activity's downloaded bytes are staged (test/diagnostic
    // seam). DEC-070/B-STAGE9-83: the extension follows `bytes`' own leading
    // signature. DEC-072/DEC-073: `start()` resolves any gzip member to a
    // complete inflate-or-refusal before this function ever sees the bytes.
    static QString stagedPayloadPath(const QString& athleteConfigDir, const QString& activityId,
                                     const QByteArray& bytes);

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
