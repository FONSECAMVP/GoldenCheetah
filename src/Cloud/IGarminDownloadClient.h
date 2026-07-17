/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-004 — Pure-virtual seam between GarminConnect (the CloudService) and the
// download/restore worker path. REQ-007 closure counterpart of IGarminAuthClient
// (DEC-012 Option A — interface injection): production wires GarminDownloadClient
// over a GarminWorker on a dedicated thread (via GarminDownloadChain); tests
// inject a Python-free fake so GarminConnect::open()/readFile() are exercisable
// under the `garmin-fast` label with no embedded Python.
//
// The restore + download failure payloads are defined once in GarminWorker.h
// (GarminRestoreFailure / GarminDownloadFailure) and reused here so the seam
// carries the worker's exact GC-stable kinds across the thread boundary.
//
// This header is Python-free by construction (it pulls only the Python-free
// worker header) — same invariant as IGarminAuthClient.h (LSN-007).

#ifndef GC_IGarminDownloadClient_h
#define GC_IGarminDownloadClient_h

#include "GarminWorker.h" // GarminRestoreFailure / GarminDownloadFailure payloads

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUuid>

class IGarminDownloadClient : public QObject
{
    Q_OBJECT
  public:
    explicit IGarminDownloadClient(QObject* parent = nullptr) : QObject(parent) {}
    ~IGarminDownloadClient() override = default;

    // Restore an authenticated session from a stored OAuth blob (REQ-006 tokens),
    // password-free (REQ-005 / REQ-NF-Compat-001(b)). The caller supplies a
    // requestId so it can correlate the async reply and discard stale ones.
    virtual void restoreSession(const QString& tokenBlob, QUuid requestId) = 0;

    // Fetch one activity's bytes in `fmt` ("ORIGINAL" for FIT, "TCX" for the
    // DEC-016 fallback). Same correlation contract as restoreSession.
    virtual void downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId) = 0;

  signals:
    void sessionRestored(QUuid id);
    void restoreFailed(QUuid id, GarminRestoreFailure error);
    void downloaded(QUuid id, QByteArray data);
    void downloadFailed(QUuid id, GarminDownloadFailure error);
};

#endif // GC_IGarminDownloadClient_h
