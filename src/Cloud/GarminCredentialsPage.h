/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// AddCloudWizard credentials page.
// QWizardPage subclass; the auth seam is injected.
//
// State machine:
//   Idle → InFlight (validatePage with fields populated)
//   InFlight → Success (onAuthFinished matching requestId)
//   InFlight → Error   (onAuthFailed  matching requestId)
//   Error   → re-dispatches on next validatePage() if fields are re-populated
//
// Password field is masked, carries IME-sensitivity hints, and is
// zeroed at dispatch time (not at response time — prevents the field
// retaining the value if the user navigates Back to retry).

#ifndef GC_GarminCredentialsPage_h
#define GC_GarminCredentialsPage_h

#include "IGarminAuthClient.h"

#include <QUuid>
#include <QWizardPage>

class QLabel;
class QLineEdit;

class GarminCredentialsPage : public QWizardPage
{
    Q_OBJECT
  public:
    explicit GarminCredentialsPage(IGarminAuthClient* authClient, QWidget* parent = nullptr);

    bool isComplete() const override;
    bool validatePage() override;

    // MFA hardening — reset the async state machine on
    // (re-)entry so a Back-then-Next after a terminal state (Success / Error /
    // MfaRequired) re-dispatches a FRESH authenticate on re-submit instead of
    // early-returning on the stale latched outcome. QWizard invokes this each
    // time the page is (re-)shown.
    void initializePage() override;

    // True once the auth client answered mfaRequired for
    // the in-flight request. The wizard's AddGarminAuth::nextId() reads this to
    // route to the MFA page (id 22) instead of the post-auth 25/30.
    bool mfaPending() const { return m_state == MfaRequired; }

  signals:
    // Emitted EXACTLY
    // when this page reaches its id-gated terminal Success (the direct auth path).
    // The wizard connects this to drive the connect-success persist. Because it is
    // fired only from the m_pendingId-gated onAuthFinished (m_state == InFlight),
    // a stale/superseded/duplicate finished() can never emit it,
    // so a late abandoned reply cannot persist.
    void succeeded(GarminAuthSuccess result);

  private slots:
    void onAuthFinished(QUuid id, GarminAuthSuccess result);
    void onAuthFailed(QUuid id, GarminAuthFailure error);

    // Garmin demands a 6-digit OTP for the in-flight
    // request: latch MfaRequired so Next is allowed to advance (the wizard's
    // nextId() then routes to the MFA page).
    void onMfaRequired(QUuid id);

  private:
    enum State { Idle, InFlight, Success, Error, MfaRequired };

    IGarminAuthClient* m_auth;
    QLineEdit* m_email = nullptr;
    QLineEdit* m_password = nullptr;
    QLabel* m_message = nullptr;
    State m_state = Idle;
    QUuid m_pendingId;
};

#endif // GC_GarminCredentialsPage_h
