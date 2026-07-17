/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-001 — GarminWorker: dedicated worker thread owner of the SSO call
// path. Auth-only slice for REQ-002 end-to-end; later slices (REQ-003 MFA,
// REQ-006 RefreshTokens, REQ-007/010 Activity ops, REQ-012 Disconnect,
// REQ-014 FetchProfile) extend this class additively with the corresponding
// slots + signals against the same IGarminPyAdapter interface.
//
// Threading: the worker is intended to be moveToThread()'d onto a dedicated
// QThread by its owner (WorkerAuthClient sets up the connection chain via
// Qt::AutoConnection so calls cross the thread boundary as queued
// invocations). The worker DOES NOT own its QThread — the owner does.

#ifndef GC_GarminWorker_h
#define GC_GarminWorker_h

#include "IGarminAuthClient.h"
#include "IGarminPyAdapter.h"

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QUuid>

// REQ-007 — the failure payload of GarminWorker::downloadFailed. Mirrors
// GarminAuthFailure one op down; `rawMessage` is the untranslated library
// message (DES-008 translates at the consumer/ErrorBus layer). Registered as a
// metatype (below + qRegisterMetaType in the ctor) so it can cross the worker
// thread boundary via a queued connection.
struct GarminDownloadFailure
{
    enum Kind { Network, RateLimit, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminDownloadFailure)

// REQ-007 closure (Slice 1) — the failure payload of GarminWorker::restoreFailed.
// Mirrors GarminDownloadFailure one op sideways; `rawMessage` is the untranslated
// library message (DES-008 translates at the consumer/ErrorBus layer).
// SessionExpired is a DISTINCT kind (not folded into Unknown) because
// REQ-NF-Compat-001(b) routes a stale stored session to a fresh SSO prompt,
// separately from a transient network dip. Registered as a metatype (below +
// qRegisterMetaType in the ctor) so it can cross the worker thread boundary.
struct GarminRestoreFailure
{
    enum Kind { SessionExpired, Network, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminRestoreFailure)

class GarminWorker : public QObject
{
    Q_OBJECT
  public:
    explicit GarminWorker(IGarminPyAdapter* py, QObject* parent = nullptr);
    ~GarminWorker() override = default;

  public slots:
    // Called from the GUI/WAC thread via Qt::AutoConnection — when the
    // worker has been moved to its own thread this becomes a queued
    // invocation, so the adapter runs on the worker thread (REQ-NF-Threads-001).
    void authenticate(const QString& email, const QString& password, QUuid requestId);

    // REQ-007 — download one activity's bytes in `fmt` ("ORIGINAL" for FIT,
    // "TCX" for the DES-004 fallback) via the retained adapter session, off the
    // GUI thread. Emits downloaded() on Success, else downloadFailed(). `fmt`
    // is forwarded verbatim so the future readFile fallback can drive it.
    void downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId);

    // REQ-007 closure (Slice 1) — restore an authenticated session from a stored
    // OAuth blob (REQ-006 tokens) via the adapter's loadTokens(), off the GUI
    // thread. Emits sessionRestored() on Success, else restoreFailed(). The blob
    // is forwarded verbatim (no re-encoding). This is the download-capable
    // session seam for a fresh CloudService open() with no password (REQ-005).
    void restoreSession(const QString& tokenBlob, QUuid requestId);

  signals:
    // Emitted on the worker thread; cross-thread queued connection delivers
    // them to slots on the GUI thread (e.g. WorkerAuthClient re-emits).
    void finished(QUuid id, GarminAuthSuccess result);
    void failed(QUuid id, GarminAuthFailure error);

    // REQ-007 — download results, same threading contract as above.
    void downloaded(QUuid id, QByteArray data);
    void downloadFailed(QUuid id, GarminDownloadFailure error);

    // REQ-007 closure (Slice 1) — restore results, same threading contract.
    void sessionRestored(QUuid id);
    void restoreFailed(QUuid id, GarminRestoreFailure error);

  private:
    IGarminPyAdapter* m_py; // not owned — caller's lifetime
};

#endif // GC_GarminWorker_h
