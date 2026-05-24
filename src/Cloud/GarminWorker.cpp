/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminWorker.h"

GarminWorker::GarminWorker(IGarminPyAdapter* py, QObject* parent) : QObject(parent), m_py(py) {}

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
