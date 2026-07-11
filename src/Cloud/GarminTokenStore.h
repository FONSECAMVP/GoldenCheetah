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
    // <athleteConfigDir>/garminconnect
    static QString directoryFor(const QString& athleteConfigDir);

    // <athleteConfigDir>/garminconnect/tokens.json
    static QString tokenFilePath(const QString& athleteConfigDir);

    // Ensures <athleteConfigDir>/garminconnect/ exists (created 0700 POSIX /
    // owner-only if ABSENT — an existing dir's perms are NOT tightened, per
    // DES-002), then writes `tokenBlob` to tokens.json via
    // AtomicFile::writeOver (owner-only 0600, tmp+rename). Returns false on any
    // failure without corrupting a pre-existing token file. Two distinct
    // athlete dirs yield fully independent files.
    static bool save(const QString& athleteConfigDir, const QByteArray& tokenBlob);

    // Minimal plain read of tokens.json (no perms enforcement — that is
    // REQ-006). Returns the file bytes; on absence/read failure returns an
    // empty QByteArray and sets *ok (if provided) to false.
    static QByteArray load(const QString& athleteConfigDir, bool* ok = nullptr);
};

#endif // GC_GarminTokenStore_h
