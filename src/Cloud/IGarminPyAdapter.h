/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// Pure-virtual interface seam between GarminWorker (C++) and the
// embedded-Python adapter. Interface injection. Production
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
#include <QVector>

// ---------------------------------------------------------------------------
// PyAuthOutcome — value type returned by IGarminPyAdapter::authenticate() and
// IGarminPyAdapter::submitMfa(). The worker maps each Kind to either
// GarminAuthSuccess (Success) or a GarminAuthFailure with a matching Kind
// (AuthFailed → Auth, Network → Network, Unknown → Unknown, RateLimit →
// RateLimit). MfaRequired is the outcome: authenticate() returns it
// (instead of Success/AuthFailed) when Garmin needs a 6-digit OTP — the
// worker emits a distinct mfaRequired signal for it, and a later submitMfa()
// resumes the SAME session. CAPTCHA / TokenPermissions outcomes arrive in
// later slices and extend this enum additively without re-shaping
// existing call sites.
// ---------------------------------------------------------------------------

struct PyAuthOutcome
{
    // RateLimit is an additive Kind (mirrors
    // PyDownloadOutcome/PyListOutcome's existing RateLimited; the worker maps
    // it to GarminAuthFailure::RateLimit.
    enum Kind { Success, AuthFailed, Network, Unknown, MfaRequired, RateLimit };
    Kind kind = Unknown;

    // Populated only when kind == Success.
    QString garmin_user_id;
    QString display_name;

    // Populated only when kind == Success: the
    // opaque OAuth session blob exported by the adapter's dump_tokens(). The
    // worker hands this to GarminTokenStore, which owns the atomic 0600 write
    // to <athlete>/garminconnect/tokens.json. Empty if the adapter could not
    // export a blob (never fails the auth itself).
    QString tokenBlob;

    // Populated for non-Success outcomes. Raw library message
    // translates at the page layer; the adapter does NOT translate.
    QString rawMessage;

    // Stage 9 live-account diagnostic — populated only when kind == Unknown:
    // the module-qualified Python exception TYPE name (e.g.
    // "builtins.ValueError", "curl_cffi.requests.exceptions.ImpersonateError"),
    // NEVER the exception's message/arguments. A type name cannot carry
    // interpolated secret material, unlike rawMessage above, which is why this
    // field (not rawMessage) is safe for a developer-trace log line to persist
    // to disk. Empty for a classified GarminError kind (Auth/Network/RateLimit
    // already carry a known, safe kind).
    QString exceptionType;
};

// ---------------------------------------------------------------------------
// PyDownloadOutcome — value type returned by IGarminPyAdapter::downloadActivity()
// Mirrors PyAuthOutcome's shape one op down: the worker maps each
// Kind to the CloudService/ErrorBus surface. `data` carries
// the raw activity bytes (FIT or TCX — the caller knows which format it asked
// for) and is populated ONLY on Success; failures carry the raw library
// message for the page/ErrorBus layer to translate.
//
// RateLimited is a distinct Kind (not folded into Unknown) because the UI has
// dedicated rate-limit copy and backfill/sync is paced off it.
// ---------------------------------------------------------------------------

struct PyDownloadOutcome
{
    enum Kind { Success, Network, RateLimited, Unknown };
    Kind kind = Unknown;

    // Populated only when kind == Success. Binary-safe (may contain NUL).
    QByteArray data;

    // Populated for non-Success outcomes. Raw library message
    // translates at the page/ErrorBus layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// GarminActivitySummary — the minimal per-activity record carried by
// PyListOutcome. Each summary carries the fields the
// incremental-sync flow and entry naming key off:
// `activityId` (the stable Garmin activity id, used by the Tier-1
// imported-<uid>.json dedup in a later slice), `startTimeGMT` (Garmin's
// SERVER-SIDE timestamp — NOT the local clock — the "newer than"
// comparison basis), and `startTimeLocal` (the activity's own LOCAL start
// time, OPTIONAL per the installed wheel — empty when the library omitted it).
// All three are marshalled as strings across the
// seam. Later slices may widen this struct additively without re-shaping the
// listing op.
// ---------------------------------------------------------------------------

struct GarminActivitySummary
{
    QString activityId;
    QString startTimeGMT;
    QString startTimeLocal;
};

// ---------------------------------------------------------------------------
// PyListOutcome — value type returned by IGarminPyAdapter::listActivitiesSince()
// Mirrors PyDownloadOutcome's shape one op sideways: the
// worker maps each Kind to activitiesListed() (Success) or listFailed() with a
// matching kind. `activities` carries the summaries and is populated ONLY on
// Success; failures carry the raw library message for the page/ErrorBus layer
// to translate. An empty `activities` on Success is a NORMAL result
// (no activities newer than the timestamp), NOT an error.
//
// RateLimited is a distinct Kind (not folded into Unknown) — mirrors
// PyDownloadOutcome — because the UI has dedicated rate-limit copy and
// Pace sync off it.
// ---------------------------------------------------------------------------

struct PyListOutcome
{
    enum Kind { Success, Network, RateLimited, Unknown };
    Kind kind = Unknown;

    // Populated only when kind == Success (may be empty — an empty listing is a
    // normal success).
    QVector<GarminActivitySummary> activities;

    // Populated for non-Success outcomes. Raw library message
    // translates at the page/ErrorBus layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// PyLoadTokensOutcome — value type returned by IGarminPyAdapter::loadTokens()
// session-restore seam). Mirrors PyAuthOutcome's
// shape: the worker maps each Kind onto the GarminWorker::restoreSession result
// (Success -> sessionRestored; every failure -> restoreFailed with a matching
// kind). The Python garmin_client.load_tokens() classifies a tampered/expired
// blob by exception TYPE as GarminError kind='session_expired'
// a stored session that no longer works forces a fresh
// SSO, NEVER a stored password). A coarse network failure is Network; anything
// else / foreign is Unknown. loadTokens() itself NEVER retains a password
// — it restores a session from the opaque OAuth blob only.
// ---------------------------------------------------------------------------

struct PyLoadTokensOutcome
{
    enum Kind { Success, SessionExpired, Network, Unknown };
    Kind kind = Unknown;

    // Populated for non-Success outcomes. Raw library message
    // translates at the page/ErrorBus layer; the adapter does NOT translate.
    QString rawMessage;
};

// ---------------------------------------------------------------------------
// PyProfileOutcome — value type returned by IGarminPyAdapter::fetchProfile()
// Mirrors PyLoadTokensOutcome's shape — a
// single-shot op with no extra params. Unlike the other outcomes, Success
// does NOT imply every field was found: the real
// library's profile/settings response is untyped and unverified against a
// live account, so garmin_client.get_profile() extracts each of
// dob/weight_kg/height_cm defensively and simply omits whatever it could not
// confidently find. A Success with zero `has*` flags set is therefore a
// VALID, expected outcome (Garmin had none of the three fields), not a
// failure — the same "only fill missing fields, absence is normal" contract
// Acceptance criterion already states. hr_max/ftp_w are explicitly
// OUT of scope and have no fields here.
// ---------------------------------------------------------------------------

struct PyProfileOutcome
{
    enum Kind { Success, Network, Unknown };
    Kind kind = Unknown;

    // Each field is populated only when kind == Success AND the adapter
    // found and could confidently parse that specific field. Absent/unset is
    // NOT an error — see the type-level comment above.
    bool hasDob = false;
    QString dob; // ISO "YYYY-MM-DD"
    bool hasWeightKg = false;
    double weightKg = 0.0;
    bool hasHeightCm = false;
    double heightCm = 0.0;

    // Populated for non-Success outcomes only. Raw library message
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
// The worker is what makes the call on a non-GUI thread (the
// worker owns the threading model); the GUI thread never invokes these methods.
//
// New ops extend this interface additively (compile-enforced
// seam): a production adapter that forgets to implement a new op is a build
// break, not a silent runtime no-op. downloadActivity() is the op.
// ---------------------------------------------------------------------------

class IGarminPyAdapter
{
  public:
    virtual ~IGarminPyAdapter() = default;
    virtual PyAuthOutcome authenticate(const QString& email, const QString& password) = 0;

    // Resume the pending MFA session established by a
    // prior authenticate() that returned PyAuthOutcome::MfaRequired. Same value
    // shape as authenticate(): on Success it populates garmin_user_id /
    // display_name / tokenBlob; a bad/expired code folds to AuthFailed, other
    // failures to Network / Unknown with rawMessage. The adapter retains the
    // pending-MFA state across the two calls (garmin_client.submit_mfa); a
    // production adapter that forgets this op is a build break (compile-enforced
    // seam).
    virtual PyAuthOutcome submitMfa(const QString& code) = 0;

    // Fetch one activity's bytes in the requested format ("ORIGINAL"
    // for FIT, "TCX" for the fallback). Reuses the session authenticate()
    // established (the password is never retained for a fresh client).
    virtual PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) = 0;

    // List the activities whose Garmin server-side
    // startTimeGMT is newer than `sinceGmt`. Reuses the session
    // authenticate()/loadTokens() established (the password is never retained
    // for a fresh client). On Success `activities` carries the summaries
    // (possibly empty — a normal result); failures fold into PyListOutcome
    // (Network / RateLimited / Unknown). Adding this pure-virtual is a
    // compile-enforced seam: a production adapter — or a test
    // double — that forgets to implement it is a build break, not a silent
    // runtime no-op. Never throws.
    virtual PyListOutcome listActivitiesSince(const QString& sinceGmt) = 0;

    // Restore an authenticated session from a
    // previously-exported opaque token blob (garmin_client.load_tokens). This is
    // the seam a fresh CloudService session uses to become download-capable
    // WITHOUT a password (silent reauth is from stored
    // TOKENS only; a password is never stored). On Success the adapter
    // holds a live session for downloadActivity(); failures fold into
    // PyLoadTokensOutcome (SessionExpired / Network / Unknown). Never throws.
    virtual PyLoadTokensOutcome loadTokens(const QString& tokenBlob) = 0;

    // Fetch dob/weight_kg/height_cm from
    // Garmin's profile (garmin_client.get_profile()) for the opt-in
    // post-connect auto-fill offer. Reuses the session authenticate()/
    // loadTokens() established (the password is never retained for a
    // fresh client). Success may have zero fields populated (Garmin had
    // none of them) — see PyProfileOutcome. Adding this pure-virtual is a
    // compile-enforced seam: a production adapter — or a
    // test double — that forgets to implement it is a build break, not a
    // silent runtime no-op. Never throws.
    virtual PyProfileOutcome fetchProfile() = 0;
};

#endif // GC_IGarminPyAdapter_h
