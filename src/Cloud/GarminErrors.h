/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// REQ-014 — friendly error translation (design.md DES-008).
// Keys on GarminAuthFailure::Kind, not DES-008's sample (raw exception class
// name) — the class name doesn't survive the pipeline; see DES-008 addendum.
// Scope: Auth/Network/RateLimit/Unknown only (other Kind values -> REQ-003/A2-005).
// Translation happens only at the page layer, never worker/adapter -- see DES-008.

#ifndef GC_GarminErrors_h
#define GC_GarminErrors_h

#include "IGarminAuthClient.h"

#include <QString>

namespace GarminErrors {

// Maps a GarminAuthFailure::Kind to a short, factual, non-alarmist message
// (REQ-009 tone). NEVER echoes outcome.rawMessage / error.translatedMessage —
// that field carries the raw library/exception text, which must not leak to
// the user (REQ-014 acceptance: "never showing the raw exception name").
QString translate(GarminAuthFailure::Kind kind);

} // namespace GarminErrors

#endif // GC_GarminErrors_h
