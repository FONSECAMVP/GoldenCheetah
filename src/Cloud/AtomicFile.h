/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-006 — AtomicFile: a generic tmp-and-rename file writer.
//
// Pure Qt — no Python.h, no GC-specific headers — so it compiles independently
// of GC_WANT_GARMINCONNECT (it is a generic helper; GarminTokenStore is its
// first consumer). REQ-NF-Reliab-002 (crash-safe writes) and REQ-NF-Sec-002
// (owner-only perms with no world-readable window) live here.

#ifndef GC_AtomicFile_h
#define GC_AtomicFile_h

#include <QByteArray>
#include <QFileDevice>
#include <QString>

class AtomicFile
{
  public:
    // Test-only observation seam (A3-R004 M1 / REQ-NF-Sec-002). writeOver
    // performs two ORDER-SENSITIVE steps on the tmp file — (1) tighten perms to
    // owner-only, (2) write the secret bytes — and the security invariant is
    // that (1) precedes (2), so `dest` is never briefly group/other-readable.
    // A post-hoc check of the FINAL perms/content cannot distinguish the two
    // orders (both end identical), so the swap regression survives such tests.
    //
    // Both steps are routed through this TmpWriter so a unit test can inject a
    // recording double and assert the true RUNTIME order of the calls. The
    // default implementation performs exactly the plain QFileDevice operations,
    // so production behaviour is byte-for-byte unchanged (the injection pointer
    // is null in production). Not thread-safe; strictly a test seam.
    struct TmpWriter
    {
        virtual ~TmpWriter() = default;
        // Step 1: set `p` on the still-empty tmp (REQ-NF-Sec-002).
        virtual bool setPermissions(QFileDevice& f, QFileDevice::Permissions p) { return f.setPermissions(p); }
        // Step 2: write the secret bytes.
        virtual qint64 write(QFileDevice& f, const QByteArray& bytes) { return f.write(bytes); }
    };

    // Inject a recording TmpWriter for tests; pass nullptr to restore the
    // production default. Test-only — never called from production code.
    static void setTmpWriterForTest(TmpWriter* writer);

    // Writes `contents` over `dest` crash-safely (REQ-NF-Reliab-002):
    //   1. write to "<dest>.tmp" in the SAME directory;
    //   2. set `perms` on the tmp file BEFORE the rename, so `dest` is never
    //      briefly group/other-readable (REQ-NF-Sec-002 — default is
    //      owner-only 0600);
    //   3. flush + fsync the tmp file's bytes to stable storage;
    //   4. atomically rename the tmp over `dest` (POSIX rename(2) /
    //      Win MoveFileEx REPLACE_EXISTING|WRITE_THROUGH via QFile::rename).
    //
    // Returns false on ANY failure and never leaves a corrupted/partial `dest`:
    // a pre-existing good `dest` survives a failed write byte-for-byte, and any
    // tmp created is removed. The parent directory of `dest` must already exist
    // (callers own directory creation — e.g. GarminTokenStore).
    static bool writeOver(const QString& dest,
                          const QByteArray& contents,
                          QFileDevice::Permissions perms =
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner);

  private:
    // Injected test double, or nullptr in production (uses the default writer).
    static TmpWriter* s_testWriter_;
};

#endif // GC_AtomicFile_h
