/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// GarminAuthChain — small RAII assembly of the production Garmin auth stack:
// a dedicated QThread + GarminWorker (moved onto it) + WorkerAuthClient,
// around an injected IGarminPyAdapter* that the chain does NOT own.
//
// DES-001a lifecycle: "the wizard creates the production PyEmbeddedAdapter
// once and passes it to GarminWorker's constructor; destruction order:
// wizard outlives worker outlives adapter" — the wizard owns adapter and
// chain, and destroys the chain (worker) before the adapter.
//
// DES-001 invariant 3: the destructor calls quit() + wait() on the owned
// QThread from the caller's thread, bounded — it never hangs. The worker
// thread does not own its lifetime.
//
// DEC-012 / DEC-013: both seams arrive as interface pointers, so this header
// and its .cpp are Python-free by construction (no Python.h — LSN-007) and
// safe for the `garmin-fast` CTest label.

#ifndef GC_GarminAuthChain_h
#define GC_GarminAuthChain_h

#include "GarminWorker.h"
#include "WorkerAuthClient.h"

#include <QThread>

class GarminAuthChain
{
  public:
    // adapter: NOT owned — per DES-001a the caller (wizard) owns it and must
    // keep it alive for the chain's whole lifetime (chain outlives nothing;
    // wizard > chain(worker) > adapter).
    explicit GarminAuthChain(IGarminPyAdapter* adapter);

    // DES-001 invariant 3: quit()+wait() on the worker thread from the
    // caller's thread, bounded — never hangs.
    ~GarminAuthChain();

    GarminAuthChain(const GarminAuthChain&) = delete;
    GarminAuthChain& operator=(const GarminAuthChain&) = delete;

    // The page-facing seam (DES-003a). Lives on the caller's thread; signals
    // are delivered there via queued connections.
    IGarminAuthClient* client();

    // The dedicated worker thread (observability/testing — the GarminWorker
    // lives on it; the adapter is invoked there, never on the caller's).
    QThread* workerThread();

    // REQ-013 (DEC-050 first slice) — the underlying worker, so a caller
    // holding the auth chain (the wizard, post-persist) can also dispatch
    // fetchProfile() for the opt-in profile auto-fill offer. Callers MUST
    // invoke its slots via a queued call (e.g. QMetaObject::invokeMethod with
    // Qt::AutoConnection) rather than calling directly — the worker lives on
    // workerThread(), not the caller's thread.
    GarminWorker* worker();

  private:
    // Declaration order matters for destruction (reverse order): the client
    // and worker are destroyed before the thread object, and the destructor
    // body stops the thread before any member is destroyed.
    QThread m_thread;
    GarminWorker m_worker;     // moved onto m_thread in the constructor
    WorkerAuthClient m_client; // stays on the caller's thread
};

#endif // GC_GarminAuthChain_h
