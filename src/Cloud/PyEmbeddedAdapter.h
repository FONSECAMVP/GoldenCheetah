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

class PyEmbeddedAdapter : public IGarminPyAdapter
{
  public:
    // modulePath: directory prepended to sys.path so `garmin_client`
    //             resolves (C++ owns path policy, per DES-012).
    // tokenstorePath: forwarded verbatim to
    //             GarminClient(email, password, tokenstore_path).
    PyEmbeddedAdapter(const QString& modulePath, const QString& tokenstorePath);

    // Never throws; every failure is folded into PyAuthOutcome (DES-013
    // step 1: interpreter down -> Unknown / "embedded Python unavailable").
    PyAuthOutcome authenticate(const QString& email, const QString& password) override;

  private:
    QString modulePath;
    QString tokenstorePath;
};

#endif // GC_PyEmbeddedAdapter_h
