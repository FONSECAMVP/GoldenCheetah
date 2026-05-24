/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-003a — Pure-virtual interface seam between GarminCredentialsPage and the
// SSO layer. DEC-012 Option A (interface injection). Production code wires
// WorkerAuthClient (DES-001); tests inject FakeAuthClient.
//
// This header is intentionally lightweight: no Python headers, no worker
// headers. It must compile with GC_WANT_GARMINCONNECT=OFF so the wizard
// infrastructure can be built independently of the embedded-Python worker.

#ifndef GC_IGarminAuthClient_h
#define GC_IGarminAuthClient_h

#include <QObject>
#include <QString>
#include <QUuid>

// ---------------------------------------------------------------------------
// Value types — passed through signals; must be copyable and default-constructible.
// ---------------------------------------------------------------------------

struct GarminAuthSuccess
{
    QString garmin_user_id;
    QString display_name;
};

struct GarminAuthFailure
{
    enum Kind { Auth, Network, Unknown };
    Kind kind = Unknown;
    QString translatedMessage;
};

// ---------------------------------------------------------------------------
// Interface
// ---------------------------------------------------------------------------

class IGarminAuthClient : public QObject
{
    Q_OBJECT
  public:
    explicit IGarminAuthClient(QObject* parent = nullptr) : QObject(parent) {}
    ~IGarminAuthClient() override = default;

    // Dispatch an authentication attempt. The caller must supply a non-null
    // requestId so the page can correlate the async response and discard stale
    // replies (e.g. a slow previous attempt arriving after the user has retried).
    virtual void authenticate(const QString& email, const QString& password, QUuid requestId) = 0;

  signals:
    void finished(QUuid id, GarminAuthSuccess result);
    void failed(QUuid id, GarminAuthFailure error);
};

#endif // GC_IGarminAuthClient_h
