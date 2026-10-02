/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// AddCloudWizard MFA page.
// QWizardPage subclass, not a hand-rolled modal: the wizard's
// Back/Next/Cancel machinery supplies "Submit" = Next; the auth seam is
// injected as IGarminAuthClient.
//
// State machine (mirrors GarminCredentialsPage):
//   Idle    → InFlight  (validatePage() with a 6-digit code)
//   InFlight → Success  (onAuthFinished, matching requestId)
//   InFlight → Error    (onAuthFailed, matching requestId, attempts < 3) → re-prompt
//   InFlight → Aborted  (onAuthFailed, matching requestId, 3rd strike) → emit aborted()
//
// Acceptance (UI half): "6-digit numeric input + Submit/Cancel. Valid
// OTP completes auth. Invalid OTP shows error and re-prompts up to 3 attempts;
// after 3rd fail, the connect attempt aborts with a non-retry error."
//
// The OTP is NOT a password — no masking/zeroing requirement — but the
// field is cleared on re-prompt so the previous (wrong) code does not linger.

#ifndef GC_GarminMfaPage_h
#define GC_GarminMfaPage_h

#include "IGarminAuthClient.h"

#include <QUuid>
#include <QWizardPage>

class QLabel;
class QLineEdit;

class GarminMfaPage : public QWizardPage
{
    Q_OBJECT
  public:
    explicit GarminMfaPage(IGarminAuthClient* authClient, QWidget* parent = nullptr);

    bool isComplete() const override;
    bool validatePage() override;

    // MFA hardening — reset the async state machine on
    // (re-)entry so a Back-then-Next after a terminal state (Success/Aborted/
    // Error) re-reads the inputs rather than latching the stale outcome. QWizard
    // invokes this each time the page is (re-)shown.
    void initializePage() override;

    // Observability accessor: the 3-strikes non-retry terminal state.
    // The wizard connects aborted() to reject(); this lets a nextId() override or
    // a test observe the latched state directly.
    bool isAborted() const { return m_state == Aborted; }

    // Test-only observability the running count of
    // invalid-OTP attempts charged against the 3-strikes budget. Lets a test
    // assert the stale-reply / terminal-state guards did NOT advance the counter.
    // Not part of the production contract — read-only, no behaviour attached.
    int attemptCount() const { return m_attempts; }

  signals:
    // 3 invalid OTPs: the connect attempt aborts with a non-retry
    // error. The wizard connects this to reject() so the attempt ends.
    void aborted();

    // Emitted EXACTLY
    // when this page reaches its id-gated terminal Success (the post-MFA path).
    // Wired alongside GarminCredentialsPage::succeeded so BOTH auth paths persist;
    // gated by the same m_pendingId guard, so a stale/late reply cannot persist.
    void succeeded(GarminAuthSuccess result);

  private slots:
    void onAuthFinished(QUuid id, GarminAuthSuccess result);
    void onAuthFailed(QUuid id, GarminAuthFailure error);

  private:
    enum State { Idle, InFlight, Success, Error, Aborted };

    static const int kMaxAttempts = 3;

    IGarminAuthClient* m_auth;
    QLineEdit* m_code = nullptr;
    QLabel* m_message = nullptr;
    State m_state = Idle;
    QUuid m_pendingId;
    int m_attempts = 0;
};

#endif // GC_GarminMfaPage_h
