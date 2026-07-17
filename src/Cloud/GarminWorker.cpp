/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminWorker.h"

GarminWorker::GarminWorker(IGarminPyAdapter* py, QObject* parent) : QObject(parent), m_py(py)
{
    // REQ-007 — GarminDownloadFailure crosses the worker thread boundary via a
    // queued connection; register it so QVariant/QSignalSpy can carry it.
    // (GarminAuthSuccess/Failure are registered in WorkerAuthClient's ctor;
    // QByteArray/QUuid are built-in metatypes.)
    qRegisterMetaType<GarminDownloadFailure>("GarminDownloadFailure");
    // REQ-007 closure (Slice 1) — restoreFailed's payload crosses the worker
    // thread boundary via a queued connection; register it too.
    qRegisterMetaType<GarminRestoreFailure>("GarminRestoreFailure");
}

void GarminWorker::authenticate(const QString& email, const QString& password, QUuid requestId)
{
    const PyAuthOutcome outcome = m_py->authenticate(email, password);

    switch (outcome.kind) {
    case PyAuthOutcome::Success: {
        GarminAuthSuccess result;
        result.garmin_user_id = outcome.garmin_user_id;
        result.display_name = outcome.display_name;
        emit finished(requestId, result);
        return;
    }
    case PyAuthOutcome::AuthFailed: {
        GarminAuthFailure err;
        err.kind = GarminAuthFailure::Auth;
        err.translatedMessage = outcome.rawMessage;
        emit failed(requestId, err);
        return;
    }
    case PyAuthOutcome::Network: {
        GarminAuthFailure err;
        err.kind = GarminAuthFailure::Network;
        err.translatedMessage = outcome.rawMessage;
        emit failed(requestId, err);
        return;
    }
    case PyAuthOutcome::Unknown:
    default: {
        GarminAuthFailure err;
        err.kind = GarminAuthFailure::Unknown;
        err.translatedMessage = outcome.rawMessage;
        emit failed(requestId, err);
        return;
    }
    }
}

void GarminWorker::downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId)
{
    const PyDownloadOutcome outcome = m_py->downloadActivity(activityId, fmt);

    switch (outcome.kind) {
    case PyDownloadOutcome::Success:
        emit downloaded(requestId, outcome.data);
        return;
    case PyDownloadOutcome::Network: {
        GarminDownloadFailure err;
        err.kind = GarminDownloadFailure::Network;
        err.rawMessage = outcome.rawMessage;
        emit downloadFailed(requestId, err);
        return;
    }
    case PyDownloadOutcome::RateLimited: {
        GarminDownloadFailure err;
        err.kind = GarminDownloadFailure::RateLimit;
        err.rawMessage = outcome.rawMessage;
        emit downloadFailed(requestId, err);
        return;
    }
    case PyDownloadOutcome::Unknown:
    default: {
        GarminDownloadFailure err;
        err.kind = GarminDownloadFailure::Unknown;
        err.rawMessage = outcome.rawMessage;
        emit downloadFailed(requestId, err);
        return;
    }
    }
}

void GarminWorker::restoreSession(const QString& tokenBlob, QUuid requestId)
{
    // DEC-002 / DES-001: the worker is the SOLE caller of the adapter. The blob
    // is forwarded verbatim (REQ-006 tokens → REQ-NF-Compat-001(b) silent reauth
    // from TOKENS, never a password — REQ-005).
    const PyLoadTokensOutcome outcome = m_py->loadTokens(tokenBlob);

    switch (outcome.kind) {
    case PyLoadTokensOutcome::Success:
        emit sessionRestored(requestId);
        return;
    case PyLoadTokensOutcome::SessionExpired: {
        GarminRestoreFailure err;
        err.kind = GarminRestoreFailure::SessionExpired;
        err.rawMessage = outcome.rawMessage;
        emit restoreFailed(requestId, err);
        return;
    }
    case PyLoadTokensOutcome::Network: {
        GarminRestoreFailure err;
        err.kind = GarminRestoreFailure::Network;
        err.rawMessage = outcome.rawMessage;
        emit restoreFailed(requestId, err);
        return;
    }
    case PyLoadTokensOutcome::Unknown:
    default: {
        GarminRestoreFailure err;
        err.kind = GarminRestoreFailure::Unknown;
        err.rawMessage = outcome.rawMessage;
        emit restoreFailed(requestId, err);
        return;
    }
    }
}
