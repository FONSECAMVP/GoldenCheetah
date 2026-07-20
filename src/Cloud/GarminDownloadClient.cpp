/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminDownloadClient.h"

#include <QMetaType>

GarminDownloadClient::GarminDownloadClient(GarminWorker* worker, QObject* parent)
    : IGarminDownloadClient(parent), m_worker(worker)
{
    // Cross-thread signal types must be registered before the first queued
    // emission carrying them (GarminWorker's ctor also registers them; doing it
    // here too is idempotent and keeps this bridge self-sufficient).
    qRegisterMetaType<GarminRestoreFailure>("GarminRestoreFailure");
    qRegisterMetaType<GarminDownloadFailure>("GarminDownloadFailure");
    qRegisterMetaType<GarminListFailure>("GarminListFailure");
    qRegisterMetaType<GarminActivitySummary>("GarminActivitySummary");
    qRegisterMetaType<QVector<GarminActivitySummary>>("QVector<GarminActivitySummary>");

    // Dispatch ↦ worker. Qt::AutoConnection picks DirectConnection when the
    // worker lives on the caller's thread (unit tests) and QueuedConnection when
    // it has been moveToThread()'d (production, via GarminDownloadChain).
    connect(this, &GarminDownloadClient::dispatchRestore, m_worker, &GarminWorker::restoreSession);
    connect(this, &GarminDownloadClient::dispatchDownload, m_worker, &GarminWorker::downloadActivity);
    connect(this, &GarminDownloadClient::dispatchList, m_worker, &GarminWorker::listActivities);

    // Worker outcomes ↦ interface signals. Same Auto/Queued semantics.
    connect(m_worker, &GarminWorker::sessionRestored, this, &IGarminDownloadClient::sessionRestored);
    connect(m_worker, &GarminWorker::restoreFailed, this, &IGarminDownloadClient::restoreFailed);
    connect(m_worker, &GarminWorker::downloaded, this, &IGarminDownloadClient::downloaded);
    connect(m_worker, &GarminWorker::downloadFailed, this, &IGarminDownloadClient::downloadFailed);
    connect(m_worker, &GarminWorker::activitiesListed, this, &IGarminDownloadClient::activitiesListed);
    connect(m_worker, &GarminWorker::listFailed, this, &IGarminDownloadClient::listFailed);
}

void GarminDownloadClient::restoreSession(const QString& tokenBlob, QUuid requestId)
{
    emit dispatchRestore(tokenBlob, requestId);
}

void GarminDownloadClient::downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId)
{
    emit dispatchDownload(activityId, fmt, requestId);
}

void GarminDownloadClient::listActivities(const QString& sinceGmt, QUuid requestId)
{
    emit dispatchList(sinceGmt, requestId);
}
