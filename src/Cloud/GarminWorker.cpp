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
    // REQ-008 Slice A — listFailed's payload and the activitiesListed summaries
    // (both the element type and the QVector container) cross the worker thread
    // boundary via a queued connection; register them so QVariant/QSignalSpy can
    // carry them.
    qRegisterMetaType<GarminListFailure>("GarminListFailure");
    qRegisterMetaType<GarminActivitySummary>("GarminActivitySummary");
    qRegisterMetaType<QVector<GarminActivitySummary>>("QVector<GarminActivitySummary>");
}

void GarminWorker::authenticate(const QString& email, const QString& password, QUuid requestId)
{
    const PyAuthOutcome outcome = m_py->authenticate(email, password);

    // REQ-003 (MFA) Slice A — an MFA-required outcome is NOT a success and NOT a
    // failure: surface it as a distinct signal so the consumer opens the MFA
    // dialog and (Slice B) drives submitMfa(). Every other outcome flows through
    // the shared mapping below, byte-for-byte unchanged from the pre-MFA path.
    if (outcome.kind == PyAuthOutcome::MfaRequired) {
        emit mfaRequired(requestId);
        return;
    }
    emitAuthOutcome(outcome, requestId);
}

void GarminWorker::submitMfa(const QString& code, QUuid requestId)
{
    // REQ-003 (MFA) Slice A — resume the pending MFA session with the OTP. The
    // adapter (garmin_client.submit_mfa) resolves it to the SAME PyAuthOutcome
    // shape authenticate() returns; reuse the identical Success/failure mapping.
    const PyAuthOutcome outcome = m_py->submitMfa(code);
    emitAuthOutcome(outcome, requestId);
}

void GarminWorker::emitAuthOutcome(const PyAuthOutcome& outcome, QUuid requestId)
{
    switch (outcome.kind) {
    case PyAuthOutcome::Success: {
        GarminAuthSuccess result;
        result.garmin_user_id = outcome.garmin_user_id;
        result.display_name = outcome.display_name;
        // REQ-008 Slice D — carry the opaque OAuth blob forward so the
        // connect-success producer can hand it to GarminTokenStore::save
        // (previously DROPPED here — A3-R008-01 root cause).
        result.tokenBlob = outcome.tokenBlob;
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
    // REQ-014 — new branch, added without disturbing the Auth/Network/Unknown
    // mappings above (locked by TEST-004/testGarminConnectAuthClient.cpp).
    case PyAuthOutcome::RateLimit: {
        GarminAuthFailure err;
        err.kind = GarminAuthFailure::RateLimit;
        err.translatedMessage = outcome.rawMessage;
        emit failed(requestId, err);
        return;
    }
    case PyAuthOutcome::Unknown:
    case PyAuthOutcome::MfaRequired:
    default: {
        // MfaRequired is handled by authenticate() before it reaches here; if a
        // submitMfa() outcome ever carried it (contract breach) fold to Unknown
        // rather than silently drop it.
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

void GarminWorker::listActivities(const QString& sinceGmt, QUuid requestId)
{
    // DEC-002 / DES-001: the worker is the SOLE caller of the adapter. The
    // since-timestamp is forwarded verbatim (DES-010 — Garmin's server-side
    // timestamp, never the local clock). Mirrors downloadActivity()'s
    // outcome→signal mapping one op sideways.
    const PyListOutcome outcome = m_py->listActivitiesSince(sinceGmt);

    switch (outcome.kind) {
    case PyListOutcome::Success:
        // An empty listing is a normal success (DES-009/DES-010) — emit it as
        // activitiesListed with an empty vector, NOT a failure.
        emit activitiesListed(requestId, outcome.activities);
        return;
    case PyListOutcome::Network: {
        GarminListFailure err;
        err.kind = GarminListFailure::Network;
        err.rawMessage = outcome.rawMessage;
        emit listFailed(requestId, err);
        return;
    }
    case PyListOutcome::RateLimited: {
        GarminListFailure err;
        err.kind = GarminListFailure::RateLimit;
        err.rawMessage = outcome.rawMessage;
        emit listFailed(requestId, err);
        return;
    }
    case PyListOutcome::Unknown:
    default: {
        GarminListFailure err;
        err.kind = GarminListFailure::Unknown;
        err.rawMessage = outcome.rawMessage;
        emit listFailed(requestId, err);
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
