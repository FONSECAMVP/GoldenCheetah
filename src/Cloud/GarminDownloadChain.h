/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// GarminDownloadChain — the RAII assembly GarminConnect uses to stand up the
// production download/restore stack: a dedicated QThread + GarminWorker (moved
// onto it) + GarminDownloadClient, around an injected IGarminPyAdapter* that the
// chain does NOT own. It is the download/restore-facing sibling of
// GarminAuthChain (which exposes only the auth client) and follows the IDENTICAL
// ownership + destruction discipline (DES-001 invariant 3, DES-001a lifecycle).
//
// DES-001a lifecycle: GarminConnect owns the adapter and the chain, and destroys
// the chain (worker) BEFORE the adapter — "GarminConnect > host(worker) >
// adapter". DES-001 invariant 3: the destructor quit()+wait()s the owned QThread
// from the caller's thread, bounded — it never hangs.
//
// DEC-012 / DEC-013: the adapter seam arrives as an interface pointer, so this
// header and its .cpp are Python-free by construction (no Python.h — LSN-007)
// and safe for the `garmin-fast` CTest label.

#ifndef GC_GarminDownloadChain_h
#define GC_GarminDownloadChain_h

#include "GarminDownloadClient.h"
#include "GarminWorker.h"

#include <QThread>

class GarminDownloadChain
{
  public:
    // adapter: NOT owned — per DES-001a the caller (GarminConnect) owns it and
    // must keep it alive for the chain's whole lifetime.
    explicit GarminDownloadChain(IGarminPyAdapter* adapter);

    // DES-001 invariant 3: quit()+wait() on the worker thread from the caller's
    // thread, bounded — never hangs.
    ~GarminDownloadChain();

    GarminDownloadChain(const GarminDownloadChain&) = delete;
    GarminDownloadChain& operator=(const GarminDownloadChain&) = delete;

    // The CloudService-facing seam (DES-004). Lives on the caller's thread;
    // signals are delivered there via queued connections.
    IGarminDownloadClient* client();

    // The dedicated worker thread (observability/testing).
    QThread* workerThread();

  private:
    // Declaration order matters for destruction (reverse order): the client and
    // worker are destroyed before the thread object, and the destructor body
    // stops the thread before any member is destroyed.
    QThread m_thread;
    GarminWorker m_worker;         // moved onto m_thread in the constructor
    GarminDownloadClient m_client; // stays on the caller's thread
};

#endif // GC_GarminDownloadChain_h
