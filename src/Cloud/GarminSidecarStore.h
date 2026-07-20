/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-002 / DEC-017 (Option A) — GarminSidecarStore: per-account sidecar storage.
//
// A dedicated Qt-only (Python-free) persistence class that mirrors
// GarminTokenStore and owns the two per-account sidecar files that live beside
// tokens.json in <athlete-config-dir>/garminconnect/:
//
//   imported-<garmin_user_id>.json        — the Tier-1 dedup map (DES-010):
//       garmin_activity_id -> { startTimeGMT, local_filename }
//   backfill-state-<garmin_user_id>.json  — the sync/backfill resume cursor:
//       { last_success_startTimeGMT, range_start, range_end }
//
// <garmin_user_id> is the stable numeric Garmin account id embedded in the
// filename (DES-002), so the per-account partition is STRUCTURAL: account A's
// store never reads account B's file. startTimeGMT is Garmin's server-side
// timestamp, stored verbatim as a string.
//
// This slice (REQ-008 Slice B) is persistence ONLY: no sync orchestration, no
// listing, no download. Writes go through AtomicFile::writeOver at 0600
// (DES-006, REQ-NF-Reliab-002); loads enforce owner-only perms the same way
// GarminTokenStore::loadChecked does (DES-002 invariant), and a torn/unparseable
// file is a typed soft outcome (DES-002: parse failure → fallback/re-fetch,
// never a hard crash). Pure Qt — no Python.h — so it stays on the `garmin-fast`
// label's Python-free invariant.

#ifndef GC_GarminSidecarStore_h
#define GC_GarminSidecarStore_h

#include <QHash>
#include <QString>

class GarminSidecarStore
{
  public:
    // The mutually-exclusive outcomes of a permission-enforcing sidecar load.
    // Mirrors GarminTokenStore::LoadStatus, plus a Torn state for a present-but-
    // unparseable file (a bare bool cannot encode four states).
    enum class LoadStatus {
        Ok,                        // present, owner-only 0600, parsed cleanly
        NotFound,                  // file absent — "nothing recorded yet"
        Torn,                      // present but unparseable JSON — caller treats
                                   // as absent-and-refetch (DES-002 soft fallback)
        SidecarPermissionsRejected // present but mode WIDER than owner-only:
                                   // REFUSED (DES-008 %1 key), path exposed, no
                                   // data; caller forces a re-fetch.
    };

    // One imported-activity record: Garmin's verbatim server timestamp + the
    // local filename the activity was written to (DES-010 Tier-1 dedup fields).
    struct ImportedEntry
    {
        QString startTimeGMT;  // Garmin server-side timestamp, stored verbatim
        QString localFilename; // e.g. "garmin-987654321.fit"
    };

    // The result of loading imported-<uid>.json: a status + the decoded map
    // (activityId -> ImportedEntry). `entries` is non-empty only when status==Ok.
    struct ImportedMap
    {
        LoadStatus status = LoadStatus::NotFound;
        QString path; // the sidecar path (DES-008 %1 arg), set in every case
        QHash<QString, ImportedEntry> entries;

        bool isOk() const { return status == LoadStatus::Ok; }
        bool isRejected() const { return status == LoadStatus::SidecarPermissionsRejected; }
        bool isTorn() const { return status == LoadStatus::Torn; }

        // Tier-1 dedup short-circuit helpers (DES-010): has this activity been
        // imported for this account, and if so, what was recorded?
        bool contains(const QString& activityId) const { return entries.contains(activityId); }
        ImportedEntry value(const QString& activityId) const { return entries.value(activityId); }
    };

    // The backfill/sync resume cursor persisted in backfill-state-<uid>.json.
    struct BackfillState
    {
        QString lastSuccessStartTimeGMT; // last activity successfully imported
        QString rangeStart;              // window lower bound (verbatim string)
        QString rangeEnd;                // window upper bound (verbatim string)
    };

    // The result of loading backfill-state-<uid>.json: a status + the decoded
    // cursor. `state` carries data only when status==Ok.
    struct BackfillLoadResult
    {
        LoadStatus status = LoadStatus::NotFound;
        QString path; // the sidecar path (DES-008 %1 arg), set in every case
        BackfillState state;

        bool isOk() const { return status == LoadStatus::Ok; }
        bool isRejected() const { return status == LoadStatus::SidecarPermissionsRejected; }
        bool isTorn() const { return status == LoadStatus::Torn; }
    };

    // --- Path resolution (per-account partition is in the filename) ----------

    // <athleteConfigDir>/garminconnect
    static QString directoryFor(const QString& athleteConfigDir);

    // <athleteConfigDir>/garminconnect/imported-<garminUserId>.json
    static QString importedFilePath(const QString& athleteConfigDir, const QString& garminUserId);

    // <athleteConfigDir>/garminconnect/backfill-state-<garminUserId>.json
    static QString backfillStateFilePath(const QString& athleteConfigDir, const QString& garminUserId);

    // --- imported-<uid>.json (Tier-1 dedup map) ------------------------------

    // Permission-enforcing load of imported-<uid>.json. Reads the ACTUAL on-disk
    // mode at load time (never cached — cf. A3-R004-M1):
    //   * owner-wider mode (any group/other bit) → SidecarPermissionsRejected,
    //     no entries, offending path exposed.
    //   * absent/unreadable → NotFound.
    //   * present but unparseable → Torn, no entries (absent-and-refetch).
    //   * conforming 0600 + valid JSON object → Ok + decoded entries.
    static ImportedMap loadImported(const QString& athleteConfigDir, const QString& garminUserId);

    // Record one imported activity (read-modify-write). Ensures garminconnect/
    // exists (created 0700 if ABSENT — an existing dir is NOT tightened, DES-002),
    // merges (activityId -> entry) into the current map, and writes the whole
    // object back via AtomicFile::writeOver at owner-only 0600 (DES-006). A
    // pre-existing torn/unparseable file is treated as empty (DES-002 fallback),
    // so recording never propagates a parse failure. Returns false on write
    // failure without corrupting a pre-existing good file.
    static bool recordImported(const QString& athleteConfigDir, const QString& garminUserId, const QString& activityId,
                               const ImportedEntry& entry);

    // --- backfill-state-<uid>.json (sync resume cursor) ----------------------

    // Permission-enforcing load of backfill-state-<uid>.json, same discipline as
    // loadImported (Ok / NotFound / Torn / SidecarPermissionsRejected).
    static BackfillLoadResult loadBackfillState(const QString& athleteConfigDir, const QString& garminUserId);

    // Persist the backfill cursor. Ensures garminconnect/ exists (0700 if absent,
    // existing dir not tightened) and writes the record via AtomicFile::writeOver
    // at owner-only 0600. Returns false on write failure without corrupting a
    // pre-existing good file.
    static bool saveBackfillState(const QString& athleteConfigDir, const QString& garminUserId,
                                  const BackfillState& state);
};

#endif // GC_GarminSidecarStore_h
