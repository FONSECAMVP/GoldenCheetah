/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-001a — Pure-virtual interface seam between GarminWorker (C++) and the
// embedded-Python adapter. DEC-013 Option A (interface injection). Production
// code wires PyEmbeddedAdapter (calls garmin_client.GarminClient.login());
// tests inject FakePyAdapter.
//
// This header is intentionally lightweight: no Python.h, no embedded-
// interpreter symbols. It must compile with GC_WANT_GARMINCONNECT=OFF so
// the worker headers can be built independently of the embedded-Python
// stack — same Python-free invariant as IGarminAuthClient.h.

#ifndef GC_IGarminPyAdapter_h
#define GC_IGarminPyAdapter_h

#include <QString>

// ---------------------------------------------------------------------------
// PyAuthOutcome — value type returned by IGarminPyAdapter::authenticate().
// The worker maps each Kind to either GarminAuthSuccess (Success) or a
// GarminAuthFailure with a matching Kind (AuthFailed → Auth, Network →
// Network, Unknown → Unknown). MFA / CAPTCHA / RateLimit / TokenPermissions
// outcomes arrive in later slices (REQ-003, REQ-015) and extend this enum
// additively without re-shaping existing call sites.
// ---------------------------------------------------------------------------

struct PyAuthOutcome
{
    enum Kind { Success, AuthFailed, Network, Unknown };
    Kind kind = Unknown;

    // Populated only when kind == Success.
    QString garmin_user_id;
    QString display_name;

    // Populated for non-Success outcomes. Raw library message — DES-008
    // translates at the page layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// Interface — header-only, no QObject inheritance. Production
// PyEmbeddedAdapter holds the embedded-Python sub-interpreter reference and
// invokes garmin_client.GarminClient.login(); FakePyAdapter records the
// call and returns a scripted outcome.
//
// authenticate() is synchronous from the worker-thread caller's perspective.
// The worker is what makes the call on a non-GUI thread (DES-001 owns the
// threading model); the GUI thread never invokes this method.
// ---------------------------------------------------------------------------

class IGarminPyAdapter
{
  public:
    virtual ~IGarminPyAdapter() = default;
    virtual PyAuthOutcome authenticate(const QString& email, const QString& password) = 0;
};

#endif // GC_IGarminPyAdapter_h
