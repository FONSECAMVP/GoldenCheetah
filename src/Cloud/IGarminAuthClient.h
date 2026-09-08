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

    // REQ-008 Slice D / DEC-014 Option B — the opaque OAuth session blob exported
    // at auth-success (PyAuthOutcome::tokenBlob, forwarded verbatim by
    // GarminWorker::emitAuthOutcome). The connect-success producer hands this to
    // GarminTokenStore::save, which owns the atomic 0600 write to tokens.json.
    // Empty when the adapter could not export a blob (never fails the auth itself).
    // Adding this member is metatype-safe: GarminAuthSuccess is a copyable value
    // struct marshalled across the queued worker->page connection and adding a
    // QString field does not change its Q_DECLARE_METATYPE registration.
    QString tokenBlob;
};

struct GarminAuthFailure
{
    // REQ-014: RateLimit is additive (DEC-013 Option A pattern) — mirrors
    // PyAuthOutcome::RateLimit one seam up; GarminWorker maps it 1:1.
    enum Kind { Auth, Network, Unknown, RateLimit };
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

    // REQ-003 (MFA) Slice A — submit the 6-digit OTP for the pending MFA session
    // established when authenticate() answered with mfaRequired(). Correlated by
    // the same requestId. Completes (finished) or fails (failed) auth on the
    // SAME session; a bad code fails with GarminAuthFailure::Auth so the
    // (Slice-B) page can re-prompt. A production client that forgets this op is
    // a build break (DEC-012 Option A — compile-enforced seam).
    virtual void submitMfa(const QString& code, QUuid requestId) = 0;

  signals:
    void finished(QUuid id, GarminAuthSuccess result);
    void failed(QUuid id, GarminAuthFailure error);

    // REQ-003 (MFA) Slice A — emitted when authenticate() determines Garmin
    // needs a 6-digit OTP. The (Slice-B) GarminMfaPage opens its modal dialog in
    // response and then drives submitMfa(); until then no consumer subscribes
    // and the no-MFA flow never emits this.
    void mfaRequired(QUuid requestId);
};

// Cross-thread signal marshalling: GarminWorker (on a worker thread) emits
// these via finished/failed; queued connections to the GUI thread require
// both Q_DECLARE_METATYPE here and qRegisterMetaType at construction time
// (see WorkerAuthClient's ctor). Same-thread direct connections (TEST-003)
// did not need this, but the cross-thread path (TEST-004) does.
Q_DECLARE_METATYPE(GarminAuthSuccess)
Q_DECLARE_METATYPE(GarminAuthFailure)

#endif // GC_IGarminAuthClient_h
