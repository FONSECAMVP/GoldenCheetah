/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminSidecarStore.h"

#include "AtomicFile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace {

// JSON keys for the two sidecar schemas (DES-002).
const QString kStartTimeGMT = QStringLiteral("startTimeGMT");
const QString kLocalFilename = QStringLiteral("local_filename");
const QString kLastSuccess = QStringLiteral("last_success_startTimeGMT");
const QString kRangeStart = QStringLiteral("range_start");
const QString kRangeEnd = QStringLiteral("range_end");

// The result of the shared permission-enforcing read step: a load status and
// the raw bytes (valid only when status==Ok). Parse/decode is layered on top by
// the two public loaders, so the perm/absent discipline lives in ONE place.
struct RawLoad
{
    GarminSidecarStore::LoadStatus status = GarminSidecarStore::LoadStatus::NotFound;
    QByteArray bytes;
};

// Read `path` with the DES-002 owner-only guard, mirroring
// GarminTokenStore::loadChecked. Reads the ACTUAL on-disk mode NOW (A3-R004-M1:
// never a cached value). POSIX: any group/other bit set → PermissionsRejected,
// no bytes. Absent → NotFound. Otherwise Ok + bytes. Torn is decided by the
// caller after a parse attempt (this step does not interpret the bytes).
RawLoad readChecked(const QString& path)
{
    RawLoad r;

    const QFileInfo info(path);
    if (!info.exists()) {
        r.status = GarminSidecarStore::LoadStatus::NotFound;
        return r;
    }

#ifndef Q_OS_WIN
    // POSIX (DES-002): refuse a mode WIDER than owner-only 0600 — any group- or
    // other-class bit (read/write/exec). Decided from the mode bits directly so
    // the refusal holds even under a DAC-overriding user (root) — finding
    // A3-R004-08.
    const QFileDevice::Permissions perms = info.permissions();
    const QFileDevice::Permissions groupOther = QFileDevice::ReadGroup | QFileDevice::WriteGroup |
                                                QFileDevice::ExeGroup | QFileDevice::ReadOther |
                                                QFileDevice::WriteOther | QFileDevice::ExeOther;
    if ((perms & groupOther) != QFileDevice::Permissions()) {
        // Do NOT read/return the bytes — the caller forces a re-fetch.
        r.status = GarminSidecarStore::LoadStatus::SidecarPermissionsRejected;
        return r;
    }
#else
    // TODO(REQ-NF-Pkg-001, A3-R004-09): Windows ACL check — verify the DACL
    // grants only the owning user before returning bytes; refuse otherwise.
    // Phase-2 CI territory, mirroring GarminTokenStore::loadChecked and
    // AtomicFile's cross-platform split. Until then the file opens normally on
    // Windows (no regression to any existing path).
#endif

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        r.status = GarminSidecarStore::LoadStatus::NotFound;
        return r;
    }
    r.bytes = f.readAll();
    r.status = GarminSidecarStore::LoadStatus::Ok;
    return r;
}

// Parse `bytes` as a top-level JSON object. On any parse error or non-object
// document, returns false (the caller maps this to LoadStatus::Torn).
bool parseObject(const QByteArray& bytes, QJsonObject& out)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    out = doc.object();
    return true;
}

// Ensure <athleteConfigDir>/garminconnect/ exists. Created 0700 (owner-only)
// ONLY when ABSENT; DES-002 does NOT tighten an existing dir's perms.
bool ensureDir(const QString& dirPath)
{
    if (!QFileInfo(dirPath).isDir()) {
        if (!QDir().mkpath(dirPath))
            return false;
        QFile::setPermissions(dirPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    }
    return true;
}

} // namespace

QString GarminSidecarStore::directoryFor(const QString& athleteConfigDir)
{
    return QDir(athleteConfigDir).filePath(QStringLiteral("garminconnect"));
}

QString GarminSidecarStore::importedFilePath(const QString& athleteConfigDir, const QString& garminUserId)
{
    return QDir(directoryFor(athleteConfigDir)).filePath(QStringLiteral("imported-%1.json").arg(garminUserId));
}

QString GarminSidecarStore::backfillStateFilePath(const QString& athleteConfigDir, const QString& garminUserId)
{
    return QDir(directoryFor(athleteConfigDir)).filePath(QStringLiteral("backfill-state-%1.json").arg(garminUserId));
}

GarminSidecarStore::ImportedMap GarminSidecarStore::loadImported(const QString& athleteConfigDir,
                                                                 const QString& garminUserId)
{
    ImportedMap result;
    result.path = importedFilePath(athleteConfigDir, garminUserId);

    const RawLoad raw = readChecked(result.path);
    if (raw.status != LoadStatus::Ok) {
        // NotFound or SidecarPermissionsRejected — no data, propagate the status.
        result.status = raw.status;
        return result;
    }

    QJsonObject obj;
    if (!parseObject(raw.bytes, obj)) {
        // Present but unparseable — soft fallback, NOT a crash (DES-002).
        result.status = LoadStatus::Torn;
        return result;
    }

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        const QJsonValue v = it.value();
        if (!v.isObject())
            continue; // skip a malformed entry rather than fail the whole map
        const QJsonObject e = v.toObject();
        ImportedEntry entry;
        entry.startTimeGMT = e.value(kStartTimeGMT).toString();
        entry.localFilename = e.value(kLocalFilename).toString();
        result.entries.insert(it.key(), entry);
    }
    result.status = LoadStatus::Ok;
    return result;
}

bool GarminSidecarStore::recordImported(const QString& athleteConfigDir, const QString& garminUserId,
                                        const QString& activityId, const ImportedEntry& entry)
{
    if (!ensureDir(directoryFor(athleteConfigDir)))
        return false;

    const QString path = importedFilePath(athleteConfigDir, garminUserId);

    // Read-modify-write: start from the current object. A present-but-torn file
    // is treated as empty (DES-002 fallback) so recording never propagates a
    // parse failure. An owner-wider existing file is likewise not read for its
    // bytes (readChecked refuses it); recording rewrites it 0600.
    QJsonObject obj;
    const RawLoad raw = readChecked(path);
    if (raw.status == LoadStatus::Ok)
        parseObject(raw.bytes, obj); // best-effort; leaves obj empty on parse fail

    QJsonObject e;
    e.insert(kStartTimeGMT, entry.startTimeGMT);
    e.insert(kLocalFilename, entry.localFilename);
    obj.insert(activityId, e);

    const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return AtomicFile::writeOver(path, bytes);
}

GarminSidecarStore::BackfillLoadResult GarminSidecarStore::loadBackfillState(const QString& athleteConfigDir,
                                                                             const QString& garminUserId)
{
    BackfillLoadResult result;
    result.path = backfillStateFilePath(athleteConfigDir, garminUserId);

    const RawLoad raw = readChecked(result.path);
    if (raw.status != LoadStatus::Ok) {
        result.status = raw.status;
        return result;
    }

    QJsonObject obj;
    if (!parseObject(raw.bytes, obj)) {
        result.status = LoadStatus::Torn;
        return result;
    }

    result.state.lastSuccessStartTimeGMT = obj.value(kLastSuccess).toString();
    result.state.rangeStart = obj.value(kRangeStart).toString();
    result.state.rangeEnd = obj.value(kRangeEnd).toString();
    result.status = LoadStatus::Ok;
    return result;
}

bool GarminSidecarStore::saveBackfillState(const QString& athleteConfigDir, const QString& garminUserId,
                                           const BackfillState& state)
{
    if (!ensureDir(directoryFor(athleteConfigDir)))
        return false;

    QJsonObject obj;
    obj.insert(kLastSuccess, state.lastSuccessStartTimeGMT);
    obj.insert(kRangeStart, state.rangeStart);
    obj.insert(kRangeEnd, state.rangeEnd);

    const QByteArray bytes = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    return AtomicFile::writeOver(backfillStateFilePath(athleteConfigDir, garminUserId), bytes);
}
