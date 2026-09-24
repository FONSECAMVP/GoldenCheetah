/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-013 — PyEmbeddedAdapter: the production embedded-CPython bridge
// implementing IGarminPyAdapter (DEC-013 Option A, production side).
//
// This header is Python-free by invariant — no Python.h, no interpreter
// symbols — so worker/wizard headers can include it with
// GC_WANT_GARMINCONNECT=OFF. Only PyEmbeddedAdapter.cpp touches Python.h.
//
// Threading: authenticate() is invoked by GarminWorker on the worker thread
// (DES-001 owns the threading model). The implementation acquires the GIL
// via PyGILState_Ensure per call, so any thread is safe; the GUI thread
// simply never calls it by design.

#ifndef GC_PyEmbeddedAdapter_h
#define GC_PyEmbeddedAdapter_h

#include "IGarminPyAdapter.h"

#include <QString>

// Opaque forward-decl so the header stays Python-free (no Python.h). The
// retained authenticated client is a PyObject*; only the .cpp knows its type.
struct _object;
using PyObject = _object;

// DEC-058 constraint 5 / B-STAGE9-38 unit 3: whether the caller supplied an
// EXPLICIT module-path override (GC_GARMIN_PYPATH) or none at all. A bare
// QString cannot carry that distinction — an unset override and an explicit
// override of "" collapse to the same value — so the two states are their
// own type, constructed via the named factories below or the public default
// ctor (equivalent to none()).
class GarminPyModulePath
{
  public:
    static GarminPyModulePath none() { return GarminPyModulePath(); }
    static GarminPyModulePath explicitOverride(const QString& dir) { return GarminPyModulePath(dir); }

    bool isExplicitOverride() const { return m_isOverride; }
    const QString& dir() const { return m_dir; }

    // Public per B-STAGE9-43. The 1-arg override ctor stays private below.
    GarminPyModulePath() = default;

  private:
    explicit GarminPyModulePath(const QString& dir) : m_isOverride(true), m_dir(dir) {}

    bool m_isOverride = false;
    QString m_dir;
};

class PyEmbeddedAdapter : public IGarminPyAdapter
{
  public:
    // modulePath: an explicit override is hoisted to sys.path index 0 and its
    //             sys.modules cache origin verified before import; with no
    //             override, the module is imported directly against sys.path
    //             as CPython built it (C++ owns path policy, per DES-012 —
    //             DEC-058 constraints 5, 17).
    // DEC-014 Option B (A3-R004-M3): the Python GarminClient is constructed
    // AUTH-ONLY — email+password only, NO tokenstore path. The library holds an
    // in-memory session and self-writes no token file; C++ (GarminTokenStore)
    // owns the single atomic 0600 write of the dump_tokens() blob.
    explicit PyEmbeddedAdapter(const GarminPyModulePath& modulePath);

    // Releases the retained authenticated client under the GIL (REQ-007
    // session model). Safe if the interpreter is already finalized.
    ~PyEmbeddedAdapter() override;

    // Owns a PyObject* (the retained client) — non-copyable.
    PyEmbeddedAdapter(const PyEmbeddedAdapter&) = delete;
    PyEmbeddedAdapter& operator=(const PyEmbeddedAdapter&) = delete;

    // Never throws; every failure is folded into PyAuthOutcome (DES-013
    // step 1: interpreter down -> Unknown / "embedded Python unavailable").
    // On Success the authenticated client is retained for downloadActivity().
    PyAuthOutcome authenticate(const QString& email, const QString& password) override;

    // REQ-003 (MFA) Slice A — resume the pending MFA session established by a
    // prior authenticate() that returned PyAuthOutcome::MfaRequired. Calls
    // garmin_client.GarminClient.submit_mfa() on the SAME retained client; on
    // Success populates garmin_user_id/display_name/tokenBlob exactly like
    // authenticate(). Never throws; a bad/expired code (GarminError kind 'auth')
    // folds to AuthFailed, other library errors to Network/Unknown, and a
    // missing pending session to Unknown.
    PyAuthOutcome submitMfa(const QString& code) override;

    // REQ-007 — download one activity via the retained authenticated client.
    // Never throws; interpreter-down / no-session / library errors all fold
    // into a non-Success PyDownloadOutcome (Network/RateLimited/Unknown).
    PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) override;

    // REQ-008 Slice A — list activities newer than `sinceGmt` via the retained
    // authenticated client (calls garmin_client.GarminClient.list_activities_since).
    // Never throws; interpreter-down / no-session / library errors all fold into
    // a non-Success PyListOutcome (Network/RateLimited/Unknown). Classified by
    // exception TYPE + .kind (LSN-006), same as downloadActivity().
    PyListOutcome listActivitiesSince(const QString& sinceGmt) override;

    // REQ-007 closure (Slice 1) — restore a session from a stored OAuth blob via
    // garmin_client.GarminClient.load_tokens(). Constructs a fresh client
    // WITHOUT a password (REQ-005 / REQ-NF-Compat-001(b)) and retains it for
    // downloadActivity() on Success. Never throws; interpreter-down / expired /
    // library errors fold into PyLoadTokensOutcome (SessionExpired/Network/
    // Unknown), classified by exception TYPE + .kind (LSN-006).
    PyLoadTokensOutcome loadTokens(const QString& tokenBlob) override;

    // REQ-013 (DEC-050 first slice) — fetch dob/weight_kg/height_cm via the
    // retained authenticated client (garmin_client.GarminClient.get_profile).
    // Never throws; interpreter-down / no-session / library errors fold into a
    // non-Success PyProfileOutcome (Network/Unknown). A Success with some or
    // all `has*` flags false is normal (Garmin didn't have that field) — the
    // Python adapter already did the defensive key-name/plausibility
    // extraction; this seam only marshals whichever keys are present.
    PyProfileOutcome fetchProfile() override;

    // DEC-066 constraint 2: releases the C++-owned module-provenance ledger's
    // strong references for the CURRENT interpreter. Must be called with the
    // GIL held, before any Py_FinalizeEx() of that interpreter — interpreter-
    // address reuse across a finalize/reinitialize cycle means the ledger's
    // pointer keys must never survive finalization. Production never
    // finalizes the interpreter (PyProcessBootstrap.h), so today's only
    // caller is interpreter-lifecycle test teardown.
    static void releaseModuleProvenanceLedgerForCurrentInterpreter();

  private:
    GarminPyModulePath modulePath;
    PyObject* m_client = nullptr; // retained authenticated GarminClient; owned
};

#endif // GC_PyEmbeddedAdapter_h
