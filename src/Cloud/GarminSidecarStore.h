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
//       { last_success_startTimeGMT, range_start, range_end, schema_version,
//         pending: { garmin_activity_id -> { startTimeGMT, local_filename } } }
//       `pending` (DEC-071) is the downloaded-not-yet-import-complete set —
//       separate from the imported-<uid>.json completion map above, so a
//       cancelled import is retried rather than skipped forever. A legacy
//       (pre-DEC-071) file has neither key and loads with pending empty and
//       schema_version 0.
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
    // unparseable file (a bare bool cannot encode these states).
    enum class LoadStatus {
        Ok,                         // present, owner-only 0600, parsed cleanly
        NotFound,                   // file absent — "nothing recorded yet"
        Torn,                       // present but unparseable JSON — caller treats
                                    // as absent-and-refetch (DES-002 soft fallback)
        SidecarPermissionsRejected, // present but mode WIDER than owner-only:
                                    // REFUSED (DES-008 %1 key), path exposed, no
                                    // data; caller forces a re-fetch.
        PendingManifestMalformed    // backfill-state only (DEC-075): parsed as an
                                    // object, but its `pending` key is present
                                    // and not itself an object, or one of
                                    // pending's entries is not an object.
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

        // DEC-071: activities staged (bytes landed) but not yet import-complete.
        // activityId -> {startTimeGMT, localFilename}. Empty for a legacy
        // (pre-DEC-071) file, which carries no `pending` key at all.
        QHash<QString, ImportedEntry> pending;

        // Explicit on-disk schema version (DEC-071). 0 means the file predates
        // versioning (no `schema_version` key present) — never written by this
        // store; saveBackfillState/recordPendingBackfill/dropPendingBackfill
        // always stamp the current version.
        int schemaVersion = 0;
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

    // Permission-enforcing load of backfill-state-<uid>.json (Ok / NotFound /
    // Torn / SidecarPermissionsRejected / PendingManifestMalformed — DEC-075's
    // fifth status, see the enum).
    static BackfillLoadResult loadBackfillState(const QString& athleteConfigDir, const QString& garminUserId);

    // Persist the three cursor fields (DEC-075: `pending` is single-writer —
    // this call IGNORES `state.pending` and preserves the on-disk `pending`
    // map verbatim; only recordPendingBackfill/dropPendingBackfill mutate it).
    // Refuses (returns false, writes nothing) when the current file is
    // SidecarPermissionsRejected or PendingManifestMalformed — DEC-075: a file
    // this store cannot model is never overwritten. NotFound/Torn still
    // self-heal by overwrite (nothing recoverable there, the recordImported
    // precedent). On success, writes via AtomicFile::writeOver at 0600.
    static bool saveBackfillState(const QString& athleteConfigDir, const QString& garminUserId,
                                  const BackfillState& state);

    // DEC-083/B-STAGE9-136 — the range-only sibling of saveBackfillState, for
    // callers (GarminBackfillController) that must persist rangeStart/rangeEnd
    // without ever writing lastSuccessStartTimeGMT: after clause 1/2, only
    // promotePendingBackfill may move the cursor. Read-modify-write against
    // the on-disk state under the same lock: `pending` AND the cursor both
    // survive untouched, so a cursor advanced by a concurrent promotion is
    // never overwritten. Same DEC-075 refusal as saveBackfillState.
    static bool saveBackfillRange(const QString& athleteConfigDir, const QString& garminUserId,
                                  const QString& rangeStart, const QString& rangeEnd);

    // Record one pending (bytes-landed, not-yet-import-complete) backfill entry
    // (DEC-071 split from recordImported's overloaded "downloaded" vs.
    // "imported" meaning). Read-modify-write against the on-disk cursor: the
    // three cursor fields and every other pending entry survive untouched.
    // Same DEC-075 refusal as saveBackfillState on SidecarPermissionsRejected/
    // PendingManifestMalformed; NotFound/Torn treated as empty (DES-002
    // fallback, the recordImported precedent).
    static bool recordPendingBackfill(const QString& athleteConfigDir, const QString& garminUserId,
                                      const QString& activityId, const ImportedEntry& entry);

    // Drop one pending entry that will never be promoted — an ABANDONED
    // download (cancelled/failed registration, or a missing staged payload).
    // DEC-083: promotion is its own operation (promotePendingBackfill below)
    // and removes its own pending row directly; this call is for the
    // abandonment path only. Same read-modify-write/refusal discipline as
    // recordPendingBackfill. Dropping an id that is not pending is a no-op
    // that still returns true.
    // B-STAGE9-112/-126: if the dropped entry's startTimeGMT is AT OR BEFORE
    // the persisted cursor (lastSuccessStartTimeGMT), the cursor is cleared
    // with it — kept as defence in depth under DEC-083 clause 3, since the
    // cursor can no longer legitimately sit past an unpromoted row, but the
    // clear costs only re-listing, never loss.
    static bool dropPendingBackfill(const QString& athleteConfigDir, const QString& garminUserId,
                                    const QString& activityId);

    // DEC-083 clause 2 — the cursor is a completeness watermark, advanced
    // ONLY here. Looks up `activityId` in the on-disk `pending` map (its
    // entry there already carries the startTimeGMT/localFilename an imported
    // record needs — no separate ImportedEntry argument), records it
    // imported (imported-write-first), removes the pending row, and advances
    // `lastSuccessStartTimeGMT` to that entry's own startTimeGMT ONLY when no
    // surviving pending row sits at or before it — an earlier (or
    // equal-second sibling) still-unpromoted row must stay reachable to a
    // later run. Takes the state-path lock (DEC-076) across the whole
    // operation; every other pending row is preserved untouched. Same
    // DEC-075 refusal as the other writers. Returns false, with nothing
    // written, when `activityId` is not currently pending, or when
    // recordImported() fails (leaves the row pending, never in neither
    // manifest).
    static bool promotePendingBackfill(const QString& athleteConfigDir, const QString& garminUserId,
                                       const QString& activityId);
};

#endif // GC_GarminSidecarStore_h
