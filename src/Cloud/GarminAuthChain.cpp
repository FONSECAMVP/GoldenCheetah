/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminAuthChain.h"

#include "IGarminPyAdapter.h"

namespace {
// DES-001 invariant 3: teardown is bounded — never hang. The graceful
// quit()+wait() ceiling is generous for an idle/finished worker; terminate()
// is a last resort only reached if the worker is wedged inside a call (the
// DES-001 "sub-interpreter wedges" failure mode, whose Phase-1 disposition
// is degrade-and-restart, not hang the GUI shutdown path).
constexpr int kQuitWaitMs = 2000;
constexpr int kTerminateWaitMs = 500;
} // namespace

GarminAuthChain::GarminAuthChain(IGarminPyAdapter* adapter) : m_thread(), m_worker(adapter), m_client(&m_worker)
{
    // The worker must be parentless to cross threads; it is a member, so the
    // chain (not the thread) owns its storage — the thread never owns its
    // consumers' lifetimes (DES-001).
    m_thread.setObjectName(QStringLiteral("GarminAuthChain"));
    m_worker.moveToThread(&m_thread);
    m_thread.start();
}

GarminAuthChain::~GarminAuthChain()
{
    // DES-001 invariant 3: quit()+wait() from the caller's thread, bounded.
    // The thread is fully stopped before member destruction begins, so the
    // worker (living on m_thread) is never destroyed while its thread runs.
    m_thread.quit();
    if (!m_thread.wait(kQuitWaitMs)) {
        m_thread.terminate(); // last resort — bounded, never hang
        m_thread.wait(kTerminateWaitMs);
    }
}

IGarminAuthClient* GarminAuthChain::client()
{
    return &m_client;
}

QThread* GarminAuthChain::workerThread()
{
    return &m_thread;
}
