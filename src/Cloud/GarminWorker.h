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
#include <QVector>

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

// REQ-008 Slice A — the failure payload of GarminWorker::listFailed. Mirrors
// GarminDownloadFailure one op sideways; `rawMessage` is the untranslated
// library message (DES-008 translates at the consumer/ErrorBus layer). RateLimit
// is a DISTINCT kind (not folded into Unknown) because DES-008 has dedicated
// rate-limit copy and DES-005/DES-010 pace sync off it. Registered as a metatype
// (below + qRegisterMetaType in the ctor) so it can cross the worker thread
// boundary via a queued connection.
struct GarminListFailure
{
    enum Kind { Network, RateLimit, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminListFailure)

// REQ-008 Slice A — the success payload element of GarminWorker::activitiesListed
// crosses the worker thread boundary as a QVector<GarminActivitySummary>; both
// the element and the container are registered as metatypes (in the ctor) so a
// queued connection / QSignalSpy can carry them. GarminActivitySummary itself is
// defined in IGarminPyAdapter.h.
Q_DECLARE_METATYPE(GarminActivitySummary)
Q_DECLARE_METATYPE(QVector<GarminActivitySummary>)

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

    // REQ-008 Slice A — list the activities whose Garmin server-side
    // startTimeGMT is newer than `sinceGmt` (DES-010 step 4) via the retained
    // adapter session, off the GUI thread. Emits activitiesListed() on Success
    // (with the summaries — possibly empty, a normal result), else listFailed().
    // `sinceGmt` is forwarded verbatim (DES-010 — Garmin's server-side timestamp,
    // never the local clock). Same requestId-forwarding discipline as
    // downloadActivity().
    void listActivities(const QString& sinceGmt, QUuid requestId);

    // REQ-007 closure (Slice 1) — restore an authenticated session from a stored
    // OAuth blob (REQ-006 tokens) via the adapter's loadTokens(), off the GUI
    // thread. Emits sessionRestored() on Success, else restoreFailed(). The blob
    // is forwarded verbatim (no re-encoding). This is the download-capable
    // session seam for a fresh CloudService open() with no password (REQ-005).
    void restoreSession(const QString& tokenBlob, QUuid requestId);

    // REQ-003 (MFA) Slice A — resume the pending MFA session established when a
    // prior authenticate() emitted mfaRequired(). Calls the adapter's
    // submitMfa() off the GUI thread and maps the outcome with the SAME
    // Success/failure mapping authenticate() uses: Success → finished(),
    // failure → failed(). `code` is the 6-digit OTP forwarded verbatim.
    void submitMfa(const QString& code, QUuid requestId);

  signals:
    // Emitted on the worker thread; cross-thread queued connection delivers
    // them to slots on the GUI thread (e.g. WorkerAuthClient re-emits).
    void finished(QUuid id, GarminAuthSuccess result);
    void failed(QUuid id, GarminAuthFailure error);

    // REQ-003 (MFA) Slice A — authenticate() emits this (INSTEAD of finished/
    // failed) when the adapter reports PyAuthOutcome::MfaRequired, so the
    // consumer opens the (Slice-B) MFA dialog and later calls submitMfa().
    void mfaRequired(QUuid id);

    // REQ-007 — download results, same threading contract as above.
    void downloaded(QUuid id, QByteArray data);
    void downloadFailed(QUuid id, GarminDownloadFailure error);

    // REQ-008 Slice A — listing results, same threading contract as above.
    // activitiesListed carries the summaries verbatim (SAME requestId); an empty
    // vector on activitiesListed is a normal "nothing newer" success, NOT a
    // failure (DES-009/DES-010).
    void activitiesListed(QUuid id, QVector<GarminActivitySummary> summaries);
    void listFailed(QUuid id, GarminListFailure error);

    // REQ-007 closure (Slice 1) — restore results, same threading contract.
    void sessionRestored(QUuid id);
    void restoreFailed(QUuid id, GarminRestoreFailure error);

  private:
    // Shared Success/failure mapping used by BOTH authenticate() (for its
    // non-MFA outcomes) and submitMfa(): Success → finished(), AuthFailed →
    // failed{Auth}, Network → failed{Network}, anything else → failed{Unknown}.
    // Centralizing it keeps the two entry points from drifting apart (REQ-003:
    // "the SAME mapping authenticate() already uses").
    void emitAuthOutcome(const PyAuthOutcome& outcome, QUuid requestId);

    IGarminPyAdapter* m_py; // not owned — caller's lifetime
};

#endif // GC_GarminWorker_h
