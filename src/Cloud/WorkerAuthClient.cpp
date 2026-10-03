/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "WorkerAuthClient.h"

#include <QMetaType>

WorkerAuthClient::WorkerAuthClient(GarminWorker* worker, QObject* parent)
    : IGarminAuthClient(parent), m_worker(worker)
{
    // Cross-thread signal types must be registered before the first queued
    // emission carrying them (otherwise Qt logs a warning and drops the
    // queued call). QUuid is already a built-in meta-type; the auth value
    // types are project-local.
    qRegisterMetaType<GarminAuthSuccess>("GarminAuthSuccess");
    qRegisterMetaType<GarminAuthFailure>("GarminAuthFailure");

    // Dispatch ↦ worker. Qt::AutoConnection picks DirectConnection when the
    // worker lives on the caller's thread (tests in slice-A) and
    // QueuedConnection when it has been moveToThread()'d (production +
    // slice-B integration tests).
    connect(this, &WorkerAuthClient::dispatchAuthenticate, m_worker, &GarminWorker::authenticate);

    // OTP dispatch ↦ worker, same Auto/Queued semantics
    // as authenticate so the resume-auth call also runs on the worker thread.
    connect(this, &WorkerAuthClient::dispatchSubmitMfa, m_worker, &GarminWorker::submitMfa);

    // Worker outcomes ↦ interface signals. Same Auto/Queued semantics — the
    // GUI thread receives signals as queued events when the worker is on
    // its own thread.
    connect(m_worker, &GarminWorker::finished, this, &IGarminAuthClient::finished);
    connect(m_worker, &GarminWorker::failed, this, &IGarminAuthClient::failed);

    // re-emit the worker's mfaRequired alongside
    // finished/failed so the page-side seam sees one coherent contract.
    connect(m_worker, &GarminWorker::mfaRequired, this, &IGarminAuthClient::mfaRequired);
}

void WorkerAuthClient::authenticate(const QString& email, const QString& password, QUuid requestId)
{
    emit dispatchAuthenticate(email, password, requestId);
}

void WorkerAuthClient::submitMfa(const QString& code, QUuid requestId)
{
    emit dispatchSubmitMfa(code, requestId);
}
