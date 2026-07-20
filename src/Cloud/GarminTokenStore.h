/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-002 — GarminTokenStore: per-athlete OAuth token storage (write side).
//
// DEC-014 Option B: the embedded-Python adapter exports the session as an
// opaque blob; THIS class owns the single atomic 0600 write of that blob to
// <athlete-config-dir>/garminconnect/tokens.json (DEC-003 — per-athlete dir,
// singular tokens.json, JSON in Phase 1). Pure Qt — no Python.h — so it stays
// off the `garmin-fast` label's Python-free invariant.
//
// This slice (REQ-004) is the WRITE path plus a minimal plain read. The
// refuse-on-bad-perms load check is REQ-006 and is deliberately NOT here.

#ifndef GC_GarminTokenStore_h
#define GC_GarminTokenStore_h

#include <QByteArray>
#include <QString>

class GarminTokenStore
{
  public:
    // REQ-006 / DES-002 — the three mutually-exclusive outcomes of a
    // permission-enforcing load. A bare bool cannot encode three states, so the
    // load-side guard reports through this enum + LoadResult.
    enum class LoadStatus {
        Ok,                      // file present, owner-only 0600, bytes returned
        NotFound,                // file absent or plain read error — "no session yet"
        TokenPermissionsRejected // present but mode/ACL WIDER than owner-only:
                                 // REFUSED (name matches the DES-008 %1 key);
                                 // the caller maps this to a forced fresh SSO.
    };

    struct LoadResult
    {
        LoadStatus status = LoadStatus::NotFound;
        QByteArray bytes; // valid only when status == Ok; empty otherwise
        QString path;     // the token file path (DES-008 %1 arg), set in every case
        bool isOk() const { return status == LoadStatus::Ok; }
        bool isRejected() const { return status == LoadStatus::TokenPermissionsRejected; }
    };

    // <athleteConfigDir>/garminconnect
    static QString directoryFor(const QString& athleteConfigDir);

    // <athleteConfigDir>/garminconnect/tokens.json
    static QString tokenFilePath(const QString& athleteConfigDir);

    // REQ-008 Slice D / DEC-garmin-018 (Option B) — the SEPARATE, account-agnostic
    // pointer file <athleteConfigDir>/garminconnect/active-account.json that
    // records which garmin_user_id is currently connected as {"garmin_user_id":
    // "<uid>"}. It is deliberately NOT inside tokens.json (whose schema is
    // security-locked by REQ-006/007 — it carries only the raw garth OAuth blob):
    // resolveGarminUserId() reads THIS file, tokens.json stays the sole home of
    // the opaque blob.
    static QString activeAccountFilePath(const QString& athleteConfigDir);

    // Ensures <athleteConfigDir>/garminconnect/ exists (created 0700 POSIX /
    // owner-only if ABSENT — an existing dir's perms are NOT tightened, per
    // DES-002), then writes `tokenBlob` to tokens.json via
    // AtomicFile::writeOver (owner-only 0600, tmp+rename). Returns false on any
    // failure without corrupting a pre-existing token file. Two distinct
    // athlete dirs yield fully independent files.
    static bool save(const QString& athleteConfigDir, const QByteArray& tokenBlob);

    // REQ-008 Slice D / DEC-garmin-018 — write active-account.json =
    // {"garmin_user_id": garminUserId} via AtomicFile (owner-only 0600, tmp+rename),
    // creating garminconnect/ 0700 if absent (same as save()). Returns false on any
    // failure without corrupting a pre-existing file.
    static bool saveActiveAccount(const QString& athleteConfigDir, const QString& garminUserId);

    // REQ-008 Slice D — read the connected garmin_user_id from active-account.json.
    // Tolerant: a missing / unreadable / torn / non-object file yields an EMPTY
    // string (the existing graceful "no connected account" behaviour — readdir then
    // no-ops rather than keying a mis-named sidecar). No perms enforcement (the file
    // is a non-secret account pointer, not the OAuth blob).
    static QString loadActiveAccountUserId(const QString& athleteConfigDir);

    // REQ-008 Slice D — the connect-success PRODUCER (closes A3-R008-01). Persists a
    // successful connect atomically: writes tokens.json FIRST (the opaque blob, 0600
    // via save()), THEN active-account.json (0600 via saveActiveAccount()). The
    // ordering is per DEC-garmin-018's crash-risk note: a crash between the two
    // leaves at worst a missing active-account.json (-> empty uid -> a safe no-op),
    // never a token file pointing at the WRONG active account. Returns true only if
    // BOTH writes succeed.
    static bool persistConnectSuccess(const QString& athleteConfigDir, const QString& garminUserId,
                                      const QByteArray& tokenBlob);

    // REQ-008 Slice D — Disconnect (DES-002): delete tokens.json + active-account.json
    // so the account is no longer connected, while PRESERVING the per-account
    // sidecars imported-<uid>.json / backfill-state-<uid>.json (REQ-012 — imported/
    // backfill history survives a disconnect). Absent files are not an error.
    // Returns false only if an existing target could not be removed.
    static bool clearAccount(const QString& athleteConfigDir);

    // Minimal plain read of tokens.json (no perms enforcement). Returns the
    // file bytes; on absence/read failure returns an empty QByteArray and sets
    // *ok (if provided) to false. Retained for REQ-004 callers that do not want
    // enforcement; REQ-006 callers use loadChecked() below.
    static QByteArray load(const QString& athleteConfigDir, bool* ok = nullptr);

    // REQ-006 (DES-002) — permission-enforcing load. Reads the ACTUAL on-disk
    // mode at load time (never a cached value — cf. A3-R004-M1) and:
    //   * POSIX: if the file mode is WIDER than owner-only 0600 (any group/other
    //     bit set), REFUSES to read it and returns TokenPermissionsRejected with
    //     no bytes and the offending path (the caller emits a DES-008 message
    //     and forces a fresh SSO). A conforming 0600 file returns Ok + bytes.
    //   * absent/unreadable file returns NotFound (distinct from rejected).
    //   * Windows: ACL "only the owning user" check is REQ-NF-Pkg-001 Phase-2 CI
    //     territory (finding A3-R004-09) — not implemented here; the file opens
    //     normally. See loadChecked() body for the marked TODO.
    static LoadResult loadChecked(const QString& athleteConfigDir);
};

#endif // GC_GarminTokenStore_h
