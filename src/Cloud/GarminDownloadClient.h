/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-004 (concrete adapter) — GarminDownloadClient: production
// IGarminDownloadClient implementation that bridges GarminConnect's CloudService
// seam to the worker-side dispatch (DEC-002 / DES-001), symmetric to
// WorkerAuthClient on the auth path. The constructor wires the worker's
// sessionRestored / restoreFailed / downloaded / downloadFailed signals back
// through this object's IGarminDownloadClient signals so GarminConnect sees a
// single coherent contract regardless of how many threads sit between it and
// the embedded Python.

#ifndef GC_GarminDownloadClient_h
#define GC_GarminDownloadClient_h

#include "GarminWorker.h"
#include "IGarminDownloadClient.h"

#include <QString>
#include <QUuid>

class GarminDownloadClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    explicit GarminDownloadClient(GarminWorker* worker, QObject* parent = nullptr);
    ~GarminDownloadClient() override = default;

    void restoreSession(const QString& tokenBlob, QUuid requestId) override;
    void downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId) override;

  signals:
    // Internal — connected to the worker's slots as queued dispatches so the
    // call runs on the worker thread regardless of which thread invoked us.
    void dispatchRestore(QString tokenBlob, QUuid requestId);
    void dispatchDownload(QString activityId, QString fmt, QUuid requestId);

  private:
    GarminWorker* m_worker; // not owned — caller's lifetime
};

#endif // GC_GarminDownloadClient_h
