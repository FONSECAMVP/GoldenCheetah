/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// WorkerAuthClient: production IGarminAuthClient
// implementation that bridges the wizard's page-side seam to the
// worker-side dispatch. The constructor wires the
// worker's `finished` / `failed` signals back through this object's
// IGarminAuthClient signals so the page sees a single, coherent contract
// regardless of how many threads sit between it and the embedded Python.

#ifndef GC_WorkerAuthClient_h
#define GC_WorkerAuthClient_h

#include "GarminWorker.h"
#include "IGarminAuthClient.h"

#include <QString>
#include <QUuid>

class WorkerAuthClient : public IGarminAuthClient
{
    Q_OBJECT
  public:
    explicit WorkerAuthClient(GarminWorker* worker, QObject* parent = nullptr);
    ~WorkerAuthClient() override = default;

    void authenticate(const QString& email, const QString& password, QUuid requestId) override;

    // Dispatch the OTP onto the worker thread, mirroring
    // authenticate(): emit dispatchSubmitMfa() (queued → GarminWorker::submitMfa).
    void submitMfa(const QString& code, QUuid requestId) override;

  signals:
    // Internal — connected to GarminWorker::authenticate as a queued slot so
    // the call dispatches onto the worker thread regardless of which thread
    // invoked WorkerAuthClient::authenticate().
    void dispatchAuthenticate(QString email, QString password, QUuid requestId);

    // Internal, connected to GarminWorker::submitMfa
    // with the SAME Qt::AutoConnection semantics as dispatchAuthenticate.
    void dispatchSubmitMfa(QString code, QUuid requestId);

  private:
    GarminWorker* m_worker; // not owned — caller's lifetime
};

#endif // GC_WorkerAuthClient_h
