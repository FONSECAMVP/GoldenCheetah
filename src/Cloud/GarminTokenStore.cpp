/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminTokenStore.h"

#include "AtomicFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

QString GarminTokenStore::directoryFor(const QString& athleteConfigDir)
{
    return QDir(athleteConfigDir).filePath(QStringLiteral("garminconnect"));
}

QString GarminTokenStore::tokenFilePath(const QString& athleteConfigDir)
{
    return QDir(directoryFor(athleteConfigDir)).filePath(QStringLiteral("tokens.json"));
}

bool GarminTokenStore::save(const QString& athleteConfigDir, const QByteArray& tokenBlob)
{
    const QString dirPath = directoryFor(athleteConfigDir);

    // Create garminconnect/ 0700 ONLY when it is absent. DES-002: an existing
    // dir's perms are left as the user set them — we do not tighten them.
    if (!QFileInfo(dirPath).isDir()) {
        if (!QDir().mkpath(dirPath))
            return false;
        QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

    // C++ owns the single atomic 0600 write of the opaque blob (DEC-014 B).
    return AtomicFile::writeOver(tokenFilePath(athleteConfigDir), tokenBlob);
}

QByteArray GarminTokenStore::load(const QString& athleteConfigDir, bool* ok)
{
    // Minimal plain read (REQ-004). The refuse-on-bad-perms check is REQ-006.
    QFile f(tokenFilePath(athleteConfigDir));
    if (!f.open(QIODevice::ReadOnly)) {
        if (ok != nullptr)
            *ok = false;
        return QByteArray();
    }
    const QByteArray data = f.readAll();
    if (ok != nullptr)
        *ok = true;
    return data;
}

GarminTokenStore::LoadResult GarminTokenStore::loadChecked(const QString& athleteConfigDir)
{
    LoadResult result;
    result.path = tokenFilePath(athleteConfigDir);

    // Read the ACTUAL on-disk state now, at load time (A3-R004-M1: never a
    // cached value). A freshly-constructed QFileInfo stats the file once here.
    const QFileInfo info(result.path);
    if (!info.exists()) {
        result.status = LoadStatus::NotFound;
        return result;
    }

#ifndef Q_OS_WIN
    // POSIX (DES-002): refuse if the mode is WIDER than owner-only 0600 — any
    // group- or other-class bit set. The refusal is decided from the mode bits
    // directly, so it holds even under a DAC-overriding user (root); an
    // open()-based check would not (finding A3-R004-08).
    const QFileDevice::Permissions perms = info.permissions();
    const QFileDevice::Permissions groupOther = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                QFileDevice::WriteOther | QFileDevice::ExeOther;
    if ((perms & groupOther) != QFileDevice::Permissions()) {
        // Do NOT read/return the bytes — the caller forces a fresh SSO.
        result.status = LoadStatus::TokenPermissionsRejected;
        return result;
    }
#else
    // TODO(REQ-NF-Pkg-001, A3-R004-09): Windows ACL check — verify the DACL
    // grants only the owning user before returning bytes; refuse (set
    // TokenPermissionsRejected) otherwise. Phase-2 CI territory, mirroring the
    // cross-platform split AtomicFile uses on the write side. Until then the
    // file opens normally on Windows (no regression to any existing path).
#endif

    QFile f(result.path);
    if (!f.open(QIODevice::ReadOnly)) {
        result.status = LoadStatus::NotFound;
        return result;
    }
    result.bytes = f.readAll();
    result.status = LoadStatus::Ok;
    return result;
}
