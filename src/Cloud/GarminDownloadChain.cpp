/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminDownloadChain.h"

#include "IGarminPyAdapter.h"

namespace {
// Teardown is bounded — never hang. Mirrors GarminAuthChain.
constexpr int kQuitWaitMs = 2000;
constexpr int kTerminateWaitMs = 500;
} // namespace

GarminDownloadChain::GarminDownloadChain(IGarminPyAdapter* adapter) : m_thread(), m_worker(adapter), m_client(&m_worker)
{
    // The worker must be parentless to cross threads; it is a member, so the
    // chain (not the thread) owns its storage — the thread never owns its
    // consumers' lifetimes.
    m_thread.setObjectName(QStringLiteral("GarminDownloadChain"));
    m_worker.moveToThread(&m_thread);
    m_thread.start();
}

GarminDownloadChain::~GarminDownloadChain()
{
    // quit()+wait() from the caller's thread, bounded. The
    // thread is fully stopped before member destruction begins, so the worker
    // (living on m_thread) is never destroyed while its thread runs.
    m_thread.quit();
    if (!m_thread.wait(kQuitWaitMs)) {
        m_thread.terminate(); // last resort — bounded, never hang
        m_thread.wait(kTerminateWaitMs);
    }
}

IGarminDownloadClient* GarminDownloadChain::client()
{
    return &m_client;
}

QThread* GarminDownloadChain::workerThread()
{
    return &m_thread;
}
