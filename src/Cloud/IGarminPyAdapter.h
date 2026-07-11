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

#include <QByteArray>
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

    // REQ-004 / DEC-014 Option B — populated only when kind == Success: the
    // opaque OAuth session blob exported by the adapter's dump_tokens(). The
    // worker hands this to GarminTokenStore, which owns the atomic 0600 write
    // to <athlete>/garminconnect/tokens.json. Empty if the adapter could not
    // export a blob (never fails the auth itself).
    QString tokenBlob;

    // Populated for non-Success outcomes. Raw library message — DES-008
    // translates at the page layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// PyDownloadOutcome — value type returned by IGarminPyAdapter::downloadActivity()
// (REQ-007). Mirrors PyAuthOutcome's shape one op down: the worker maps each
// Kind to the CloudService/ErrorBus surface (DES-004/DES-008). `data` carries
// the raw activity bytes (FIT or TCX — the caller knows which format it asked
// for) and is populated ONLY on Success; failures carry the raw library
// message for DES-008 to translate at the page/ErrorBus layer.
//
// RateLimited is a distinct Kind (not folded into Unknown) because DES-008 has
// dedicated rate-limit copy and DES-005/DES-010 pace backfill/sync off it.
// ---------------------------------------------------------------------------

struct PyDownloadOutcome
{
    enum Kind { Success, Network, RateLimited, Unknown };
    Kind kind = Unknown;

    // Populated only when kind == Success. Binary-safe (may contain NUL).
    QByteArray data;

    // Populated for non-Success outcomes. Raw library message — DES-008
    // translates at the page/ErrorBus layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// Interface — header-only, no QObject inheritance. Production
// PyEmbeddedAdapter holds the embedded-Python sub-interpreter reference and
// invokes garmin_client.GarminClient.login() / .download_activity();
// FakePyAdapter records the call and returns a scripted outcome.
//
// Every method is synchronous from the worker-thread caller's perspective.
// The worker is what makes the call on a non-GUI thread (DES-001 owns the
// threading model); the GUI thread never invokes these methods.
//
// New ops extend this interface additively (DEC-013 Option A — compile-enforced
// seam): a production adapter that forgets to implement a new op is a build
// break, not a silent runtime no-op. downloadActivity() is the REQ-007 op.
// ---------------------------------------------------------------------------

class IGarminPyAdapter
{
  public:
    virtual ~IGarminPyAdapter() = default;
    virtual PyAuthOutcome authenticate(const QString& email, const QString& password) = 0;

    // REQ-007 — fetch one activity's bytes in the requested format ("ORIGINAL"
    // for FIT, "TCX" for the fallback). Reuses the session authenticate()
    // established (REQ-005 forbids retaining the password for a fresh client).
    virtual PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) = 0;
};

#endif // GC_IGarminPyAdapter_h
