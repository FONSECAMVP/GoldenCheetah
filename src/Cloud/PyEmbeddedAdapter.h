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

class PyEmbeddedAdapter : public IGarminPyAdapter
{
  public:
    // modulePath: directory prepended to sys.path so `garmin_client`
    //             resolves (C++ owns path policy, per DES-012).
    // DEC-014 Option B (A3-R004-M3): the Python GarminClient is constructed
    // AUTH-ONLY — email+password only, NO tokenstore path. The library holds an
    // in-memory session and self-writes no token file; C++ (GarminTokenStore)
    // owns the single atomic 0600 write of the dump_tokens() blob.
    explicit PyEmbeddedAdapter(const QString& modulePath);

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

    // REQ-007 — download one activity via the retained authenticated client.
    // Never throws; interpreter-down / no-session / library errors all fold
    // into a non-Success PyDownloadOutcome (Network/RateLimited/Unknown).
    PyDownloadOutcome downloadActivity(const QString& activityId, const QString& fmt) override;

    // REQ-007 closure (Slice 1) — restore a session from a stored OAuth blob via
    // garmin_client.GarminClient.load_tokens(). Constructs a fresh client
    // WITHOUT a password (REQ-005 / REQ-NF-Compat-001(b)) and retains it for
    // downloadActivity() on Success. Never throws; interpreter-down / expired /
    // library errors fold into PyLoadTokensOutcome (SessionExpired/Network/
    // Unknown), classified by exception TYPE + .kind (LSN-006).
    PyLoadTokensOutcome loadTokens(const QString& tokenBlob) override;

  private:
    QString modulePath;
    PyObject* m_client = nullptr; // retained authenticated GarminClient; owned
};

#endif // GC_PyEmbeddedAdapter_h
