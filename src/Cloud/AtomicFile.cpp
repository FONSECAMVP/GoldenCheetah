/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "AtomicFile.h"

#include <QFile>

#ifdef Q_OS_WIN
#include <io.h>
#include <windows.h>
#else
#include <cstdio>
#include <unistd.h>
#endif

namespace {

// Flush the OS write-back cache for this file's bytes to stable storage
// (REQ-NF-Reliab-002 step: "fsync, then atomic rename"). Best-effort per
// platform; a false return aborts the write so a torn tmp never renames over
// a good dest.
bool fsyncFile(QFile& f)
{
    const int fd = f.handle();
    if (fd < 0)
        return false;
#ifdef Q_OS_WIN
    const HANDLE h = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    if (h == INVALID_HANDLE_VALUE)
        return false;
    return FlushFileBuffers(h) != 0;
#else
    return ::fsync(fd) == 0;
#endif
}

// Atomic replace of `to` by `from`. QFile::rename cannot overwrite an existing
// target, so we go native: POSIX rename(2) and Win MoveFileEx with
// REPLACE_EXISTING|WRITE_THROUGH both replace atomically without a window where
// the destination is missing.
bool atomicRename(const QString& from, const QString& to)
{
#ifdef Q_OS_WIN
    return MoveFileExW(reinterpret_cast<const wchar_t*>(from.utf16()),
                       reinterpret_cast<const wchar_t*>(to.utf16()),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    const QByteArray f = QFile::encodeName(from);
    const QByteArray t = QFile::encodeName(to);
    return ::rename(f.constData(), t.constData()) == 0;
#endif
}

} // namespace

// Test-only injection point (see AtomicFile.h). Null in production.
AtomicFile::TmpWriter* AtomicFile::s_testWriter_ = nullptr;

void AtomicFile::setTmpWriterForTest(TmpWriter* writer)
{
    s_testWriter_ = writer;
}

bool AtomicFile::writeOver(const QString& dest, const QByteArray& contents, QFileDevice::Permissions perms)
{
    const QString tmp = dest + QStringLiteral(".tmp");

    // Clear any stale tmp from a previously-aborted write so we never rename an
    // unrelated leftover over dest.
    QFile::remove(tmp);

    QFile f(tmp);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    // Both order-sensitive steps go through the writer so a test can observe
    // their true runtime order (production: default writer, no observation).
    TmpWriter defaultWriter;
    TmpWriter* writer = s_testWriter_ ? s_testWriter_ : &defaultWriter;

    // Set the requested (owner-only by default) perms on the still-EMPTY tmp,
    // BEFORE any secret bytes are written and before the rename — so dest is
    // never briefly group/other-readable (REQ-NF-Sec-002, no world-readable
    // window). rename preserves perms, so dest inherits exactly these.
    if (!writer->setPermissions(f, perms)) {
        f.close();
        QFile::remove(tmp);
        return false;
    }

    if (writer->write(f, contents) != contents.size()) {
        f.close();
        QFile::remove(tmp);
        return false;
    }

    if (!f.flush() || !fsyncFile(f)) {
        f.close();
        QFile::remove(tmp);
        return false;
    }
    f.close();

    if (!atomicRename(tmp, dest)) {
        // dest (if it existed) is untouched — leave the previous good file
        // intact and clean up our tmp (REQ-NF-Reliab-002).
        QFile::remove(tmp);
        return false;
    }
    return true;
}
