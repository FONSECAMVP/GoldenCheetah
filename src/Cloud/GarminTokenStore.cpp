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
