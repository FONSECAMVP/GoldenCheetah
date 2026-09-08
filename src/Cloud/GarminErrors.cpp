/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminErrors.h"

#include <QObject>

namespace GarminErrors {

QString translate(GarminAuthFailure::Kind kind)
{
    switch (kind) {
    case GarminAuthFailure::Auth:
        return QObject::tr("Garmin Connect rejected your email or password. Please check and try again.");
    case GarminAuthFailure::Network:
        return QObject::tr("Couldn't reach Garmin Connect. Check your internet connection and try again.");
    case GarminAuthFailure::RateLimit:
        return QObject::tr("Garmin Connect is rate-limiting GoldenCheetah. Please wait a moment and try again.");
    case GarminAuthFailure::Unknown:
    default:
        // REQ-014 acceptance criterion's fallback: a generic message + a
        // "code". The rawMessage/translatedMessage fields carry the raw
        // library exception text (see IGarminPyAdapter.h) and must never
        // appear here, so the "code" is the stable Kind tag, not that text.
        return QObject::tr("Connection to Garmin Connect failed (code: unknown).");
    }
}

} // namespace GarminErrors
