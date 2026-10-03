/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// Friendly error translation.
// Keys on GarminAuthFailure::Kind, not the raw exception class
// name — the class name doesn't survive the pipeline.
// Scope: Auth/Network/RateLimit/Unknown only (other Kind values are not translated here).
// Translation happens only at the page layer, never worker/adapter.

#ifndef GC_GarminErrors_h
#define GC_GarminErrors_h

#include "IGarminAuthClient.h"

#include <QString>

namespace GarminErrors {

// Maps a GarminAuthFailure::Kind to a short, factual, non-alarmist message
// NEVER echoes outcome.rawMessage / error.translatedMessage
// that field carries the raw library/exception text, which must not leak to
// the user ("never showing the raw exception name").
QString translate(GarminAuthFailure::Kind kind);

} // namespace GarminErrors

#endif // GC_GarminErrors_h
