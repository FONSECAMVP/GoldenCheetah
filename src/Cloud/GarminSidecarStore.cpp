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
const QString kPending = QStringLiteral("pending");              // DEC-071 pending manifest
const QString kSchemaVersion = QStringLiteral("schema_version"); // DEC-071

// The schema version this store writes. A file with no `schema_version` key
// (pre-DEC-071) loads as version 0 — never produced by a write in this file.
constexpr int kCurrentBackfillSchemaVersion = 1;

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

// Serialize the full backfill cursor, including the DEC-071 pending manifest
// and the current schema version. Shared by saveBackfillState and the two
// pending writers so all three stamp the identical on-disk shape.
QJsonObject serializeBackfillState(const GarminSidecarStore::BackfillState& state)
{
    QJsonObject obj;
    obj.insert(kLastSuccess, state.lastSuccessStartTimeGMT);
    obj.insert(kRangeStart, state.rangeStart);
    obj.insert(kRangeEnd, state.rangeEnd);
    obj.insert(kSchemaVersion, kCurrentBackfillSchemaVersion);

    QJsonObject pending;
    for (auto it = state.pending.constBegin(); it != state.pending.constEnd(); ++it) {
        QJsonObject e;
        e.insert(kStartTimeGMT, it.value().startTimeGMT);
        e.insert(kLocalFilename, it.value().localFilename);
        pending.insert(it.key(), e);
    }
    obj.insert(kPending, pending);
    return obj;
}

// DEC-075: a file whose current status is one of these two cannot be safely
// modelled (Rejected: bytes never read; PendingManifestMalformed: parsed but
// `pending` uninterpretable), so a cursor writer must refuse rather than
// overwrite it. NotFound/Torn have nothing recoverable and keep self-healing.
bool refuseCursorOverwrite(GarminSidecarStore::LoadStatus status)
{
    return status == GarminSidecarStore::LoadStatus::SidecarPermissionsRejected ||
           status == GarminSidecarStore::LoadStatus::PendingManifestMalformed;
}

// Ensures the dir and writes the WHOLE state atomically at 0600 (DES-006).
// Shared write path for saveBackfillState/recordPendingBackfill/
// dropPendingBackfill so all three match exactly.
bool writeBackfillState(const QString& athleteConfigDir, const QString& garminUserId,
                        const GarminSidecarStore::BackfillState& state)
{
    if (!ensureDir(GarminSidecarStore::directoryFor(athleteConfigDir)))
        return false;

    const QByteArray bytes = QJsonDocument(serializeBackfillState(state)).toJson(QJsonDocument::Compact);
    return AtomicFile::writeOver(GarminSidecarStore::backfillStateFilePath(athleteConfigDir, garminUserId), bytes);
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

    // DEC-075: a present `pending` key that is not an object, or an entry
    // within it that is not an object, is parsed JSON we still cannot model —
    // refuse the whole load rather than silently drop the offending entry (a
    // silent drop would become permanent on the next write).
    if (obj.contains(kPending)) {
        const QJsonValue pendingVal = obj.value(kPending);
        if (!pendingVal.isObject()) {
            result.status = LoadStatus::PendingManifestMalformed;
            return result;
        }
        const QJsonObject pendingObj = pendingVal.toObject();
        for (auto it = pendingObj.constBegin(); it != pendingObj.constEnd(); ++it) {
            if (!it.value().isObject()) {
                result.status = LoadStatus::PendingManifestMalformed;
                return result;
            }
        }
    }

    result.state.lastSuccessStartTimeGMT = obj.value(kLastSuccess).toString();
    result.state.rangeStart = obj.value(kRangeStart).toString();
    result.state.rangeEnd = obj.value(kRangeEnd).toString();
    // Missing key (pre-DEC-071 legacy file) -> version 0, NOT Torn.
    result.state.schemaVersion = obj.value(kSchemaVersion).toInt(0);

    // Missing key (legacy file) -> not present, pending stays empty. Present
    // means every entry validated as an object above.
    if (obj.contains(kPending)) {
        const QJsonObject pendingObj = obj.value(kPending).toObject();
        for (auto it = pendingObj.constBegin(); it != pendingObj.constEnd(); ++it) {
            const QJsonObject e = it.value().toObject();
            ImportedEntry entry;
            entry.startTimeGMT = e.value(kStartTimeGMT).toString();
            entry.localFilename = e.value(kLocalFilename).toString();
            result.state.pending.insert(it.key(), entry);
        }
    }
    result.status = LoadStatus::Ok;
    return result;
}

bool GarminSidecarStore::saveBackfillState(const QString& athleteConfigDir, const QString& garminUserId,
                                           const BackfillState& state)
{
    const BackfillLoadResult current = loadBackfillState(athleteConfigDir, garminUserId);
    if (refuseCursorOverwrite(current.status))
        return false;

    // DEC-075: pending is single-writer — this call never persists its own
    // `state.pending` argument, only the on-disk map (empty if there wasn't one).
    BackfillState toWrite = state;
    toWrite.pending = current.isOk() ? current.state.pending : QHash<QString, GarminSidecarStore::ImportedEntry>();
    return writeBackfillState(athleteConfigDir, garminUserId, toWrite);
}

bool GarminSidecarStore::recordPendingBackfill(const QString& athleteConfigDir, const QString& garminUserId,
                                               const QString& activityId, const ImportedEntry& entry)
{
    const BackfillLoadResult current = loadBackfillState(athleteConfigDir, garminUserId);
    if (refuseCursorOverwrite(current.status))
        return false;

    // NotFound/Torn treated as empty (DES-002 fallback), same precedent as
    // recordImported: recording never propagates a prior parse failure.
    BackfillState state = current.isOk() ? current.state : BackfillState();
    state.pending.insert(activityId, entry);
    return writeBackfillState(athleteConfigDir, garminUserId, state);
}

bool GarminSidecarStore::dropPendingBackfill(const QString& athleteConfigDir, const QString& garminUserId,
                                             const QString& activityId)
{
    const BackfillLoadResult current = loadBackfillState(athleteConfigDir, garminUserId);
    if (refuseCursorOverwrite(current.status))
        return false;

    BackfillState state = current.isOk() ? current.state : BackfillState();
    state.pending.remove(activityId);
    return writeBackfillState(athleteConfigDir, garminUserId, state);
}
