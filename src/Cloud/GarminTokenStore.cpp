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
#include <QJsonDocument>
#include <QJsonObject>

QString GarminTokenStore::directoryFor(const QString& athleteConfigDir)
{
    return QDir(athleteConfigDir).filePath(QStringLiteral("garminconnect"));
}

QString GarminTokenStore::tokenFilePath(const QString& athleteConfigDir)
{
    return QDir(directoryFor(athleteConfigDir)).filePath(QStringLiteral("tokens.json"));
}

QString GarminTokenStore::activeAccountFilePath(const QString& athleteConfigDir)
{
    return QDir(directoryFor(athleteConfigDir)).filePath(QStringLiteral("active-account.json"));
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

bool GarminTokenStore::saveActiveAccount(const QString& athleteConfigDir, const QString& garminUserId)
{
    const QString dirPath = directoryFor(athleteConfigDir);

    // Create garminconnect/ 0700 ONLY when absent (mirrors save(); DES-002 does not
    // tighten an existing dir's perms).
    if (!QFileInfo(dirPath).isDir()) {
        if (!QDir().mkpath(dirPath))
            return false;
        QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }

    // DEC-garmin-018 — {"garmin_user_id": "<uid>"}, atomic 0600 (AtomicFile default).
    QJsonObject obj;
    obj.insert(QStringLiteral("garmin_user_id"), garminUserId);
    const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return AtomicFile::writeOver(activeAccountFilePath(athleteConfigDir), bytes);
}

QString GarminTokenStore::loadActiveAccountUserId(const QString& athleteConfigDir)
{
    // Tolerant, non-secret read: any absence / read error / torn-or-foreign JSON
    // yields an empty id so the caller no-ops rather than keying a mis-named
    // sidecar (DEC-garmin-018 graceful-degradation note).
    QFile f(activeAccountFilePath(athleteConfigDir));
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    const QByteArray bytes = f.readAll();
    f.close();
    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject())
        return QString();
    return doc.object().value(QStringLiteral("garmin_user_id")).toString();
}

bool GarminTokenStore::persistConnectSuccess(const QString& athleteConfigDir, const QString& garminUserId,
                                             const QByteArray& tokenBlob)
{
    // DEC-garmin-018 ordering: tokens.json FIRST, then active-account.json — a crash
    // between the two degrades to a missing active-account.json (empty uid -> safe
    // no-op), never a token file bound to the wrong active account.
    if (!save(athleteConfigDir, tokenBlob))
        return false;
    return saveActiveAccount(athleteConfigDir, garminUserId);
}

bool GarminTokenStore::clearAccount(const QString& athleteConfigDir)
{
    // Disconnect (DES-002): remove the two connect-success artefacts. The
    // per-account sidecars (imported-<uid>.json / backfill-state-<uid>.json) are
    // deliberately left untouched — REQ-012 preserves imported/backfill history
    // across a disconnect. Absent files are not a failure.
    //
    // DEC-garmin-020 (A3-R012-F2): each artefact is written through
    // AtomicFile::writeOver, which stages the bytes in a `<path>.tmp` sibling
    // before renaming it over the destination. A crash in that window leaves the
    // COMPLETE OAuth blob sitting in tokens.json.tmp — deleting only tokens.json
    // would leave that secret on disk for good. Sweep the siblings too.
    bool ok = true;
    const QString tokens = tokenFilePath(athleteConfigDir);
    const QString activeAccount = activeAccountFilePath(athleteConfigDir);
    for (const QString& path :
         {tokens, tokens + QStringLiteral(".tmp"), activeAccount, activeAccount + QStringLiteral(".tmp")}) {
        if (QFileInfo::exists(path) && !QFile::remove(path))
            ok = false;
    }
    return ok;
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
