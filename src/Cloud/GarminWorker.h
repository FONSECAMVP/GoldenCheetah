/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// GarminWorker: dedicated worker thread owner of the SSO call
// path. Auth-only to begin with; later slices (MFA, RefreshTokens,
// Activity ops, Disconnect, FetchProfile) extend this class additively with the corresponding
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

// The failure payload of GarminWorker::downloadFailed. Mirrors
// GarminAuthFailure one op down; `rawMessage` is the untranslated library
// message (translated at the consumer/ErrorBus layer). Registered as a
// metatype (below + qRegisterMetaType in the ctor) so it can cross the worker
// thread boundary via a queued connection.
struct GarminDownloadFailure
{
    enum Kind { Network, RateLimit, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminDownloadFailure)

// The failure payload of GarminWorker::restoreFailed.
// Mirrors GarminDownloadFailure one op sideways; `rawMessage` is the untranslated
// library message (translated at the consumer/ErrorBus layer).
// SessionExpired is a DISTINCT kind (not folded into Unknown) because
// Silent reauth routes a stale stored session to a fresh SSO prompt,
// separately from a transient network dip. Registered as a metatype (below +
// qRegisterMetaType in the ctor) so it can cross the worker thread boundary.
struct GarminRestoreFailure
{
    enum Kind { SessionExpired, Network, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminRestoreFailure)

// The failure payload of GarminWorker::listFailed. Mirrors
// GarminDownloadFailure one op sideways; `rawMessage` is the untranslated
// library message (translated at the consumer/ErrorBus layer). RateLimit
// is a DISTINCT kind (not folded into Unknown) because the UI has dedicated
// rate-limit copy and sync is paced off it. Registered as a metatype
// (below + qRegisterMetaType in the ctor) so it can cross the worker thread
// boundary via a queued connection.
struct GarminListFailure
{
    // NoClient/Timeout are synthesised by GarminConnect's
    // blockingList() local exits (no worker round trip); Unknown stays the
    // genuine adapter-reported last resort.
    enum Kind { Network, RateLimit, Unknown, NoClient, Timeout };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminListFailure)

// The success payload element of GarminWorker::activitiesListed
// crosses the worker thread boundary as a QVector<GarminActivitySummary>; both
// the element and the container are registered as metatypes (in the ctor) so a
// queued connection / QSignalSpy can carry them. GarminActivitySummary itself is
// defined in IGarminPyAdapter.h.
Q_DECLARE_METATYPE(GarminActivitySummary)
Q_DECLARE_METATYPE(QVector<GarminActivitySummary>)

// The success payload of
// GarminWorker::profileFetched. Mirrors PyProfileOutcome's field shape
// (Success case only, see IGarminPyAdapter.h): a field's has* flag being
// false is a NORMAL outcome (Garmin didn't have that field), not a failure.
// hr_max/ftp_w are explicitly OUT of scope and
// have no fields here. Registered as a metatype (below + qRegisterMetaType in
// the ctor) so it can cross the worker thread boundary via a queued
// connection.
struct GarminProfileResult
{
    bool hasDob = false;
    QString dob; // ISO "YYYY-MM-DD"
    bool hasWeightKg = false;
    double weightKg = 0.0;
    bool hasHeightCm = false;
    double heightCm = 0.0;
};
Q_DECLARE_METATYPE(GarminProfileResult)

// The failure payload of
// GarminWorker::profileFailed. Mirrors GarminRestoreFailure's shape one op
// sideways, but with no dedicated SessionExpired/RateLimit kind —
// PyProfileOutcome has none either (see IGarminPyAdapter.h). Registered as a
// metatype (below + qRegisterMetaType in the ctor) so it can cross the worker
// thread boundary via a queued connection.
struct GarminProfileFailure
{
    enum Kind { Network, Unknown };
    Kind kind = Unknown;
    QString rawMessage;
};
Q_DECLARE_METATYPE(GarminProfileFailure)

class GarminWorker : public QObject
{
    Q_OBJECT
  public:
    explicit GarminWorker(IGarminPyAdapter* py, QObject* parent = nullptr);
    ~GarminWorker() override = default;

  public slots:
    // Called from the GUI/WAC thread via Qt::AutoConnection — when the
    // worker has been moved to its own thread this becomes a queued
    // invocation, so the adapter runs on the worker thread.
    void authenticate(const QString& email, const QString& password, QUuid requestId);

    // Download one activity's bytes in `fmt` ("ORIGINAL" for FIT,
    // "TCX" for the fallback) via the retained adapter session, off the
    // GUI thread. Emits downloaded() on Success, else downloadFailed(). `fmt`
    // is forwarded verbatim so the future readFile fallback can drive it.
    void downloadActivity(const QString& activityId, const QString& fmt, QUuid requestId);

    // List the activities whose Garmin server-side
    // startTimeGMT is newer than `sinceGmt` via the retained
    // adapter session, off the GUI thread. Emits activitiesListed() on Success
    // (with the summaries — possibly empty, a normal result), else listFailed().
    // `sinceGmt` is forwarded verbatim (Garmin's server-side timestamp,
    // never the local clock). Same requestId-forwarding discipline as
    // downloadActivity().
    void listActivities(const QString& sinceGmt, QUuid requestId);

    // Restore an authenticated session from a stored
    // OAuth blob via the adapter's loadTokens(), off the GUI
    // thread. Emits sessionRestored() on Success, else restoreFailed(). The blob
    // is forwarded verbatim (no re-encoding). This is the download-capable
    // session seam for a fresh CloudService open() with no password.
    void restoreSession(const QString& tokenBlob, QUuid requestId);

    // Resume the pending MFA session established when a
    // prior authenticate() emitted mfaRequired(). Calls the adapter's
    // submitMfa() off the GUI thread and maps the outcome with the SAME
    // Success/failure mapping authenticate() uses: Success → finished(),
    // failure → failed(). `code` is the 6-digit OTP forwarded verbatim.
    void submitMfa(const QString& code, QUuid requestId);

    // Fetch dob/weight_kg/height_cm via the
    // retained adapter session, off the GUI thread, for the opt-in
    // post-connect auto-fill offer. Emits profileFetched() on
    // Success (which may carry zero fields — Garmin not having a field is
    // normal, not a failure), else profileFailed(). hr_max/ftp_w are
    // explicitly deferred and are not part of this
    // op's payload.
    void fetchProfile(QUuid requestId);

  signals:
    // Emitted on the worker thread; cross-thread queued connection delivers
    // them to slots on the GUI thread (e.g. WorkerAuthClient re-emits).
    void finished(QUuid id, GarminAuthSuccess result);
    void failed(QUuid id, GarminAuthFailure error);

    // authenticate() emits this (INSTEAD of finished/
    // failed) when the adapter reports PyAuthOutcome::MfaRequired, so the
    // consumer opens the (Slice-B) MFA dialog and later calls submitMfa().
    void mfaRequired(QUuid id);

    // Download results, same threading contract as above.
    void downloaded(QUuid id, QByteArray data);
    void downloadFailed(QUuid id, GarminDownloadFailure error);

    // Listing results, same threading contract as above.
    // activitiesListed carries the summaries verbatim (SAME requestId); an empty
    // vector on activitiesListed is a normal "nothing newer" success, NOT a
    // failure.
    void activitiesListed(QUuid id, QVector<GarminActivitySummary> summaries);
    void listFailed(QUuid id, GarminListFailure error);

    // Restore results, same threading contract.
    void sessionRestored(QUuid id);
    void restoreFailed(QUuid id, GarminRestoreFailure error);

    // profile-fetch results, same threading
    // contract as above. A Success with no has* flags set is a NORMAL
    // outcome (Garmin had none of the 3 fields).
    void profileFetched(QUuid id, GarminProfileResult result);
    void profileFailed(QUuid id, GarminProfileFailure error);

  private:
    // Shared Success/failure mapping used by BOTH authenticate() (for its
    // non-MFA outcomes) and submitMfa(): Success → finished(), AuthFailed →
    // failed{Auth}, Network → failed{Network}, anything else → failed{Unknown}.
    // Centralizing it keeps the two entry points from drifting apart (the SAME mapping
    // authenticate() already uses).
    void emitAuthOutcome(const PyAuthOutcome& outcome, QUuid requestId);

    IGarminPyAdapter* m_py; // not owned — caller's lifetime
};

#endif // GC_GarminWorker_h
