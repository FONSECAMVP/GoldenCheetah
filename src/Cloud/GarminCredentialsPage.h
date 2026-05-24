/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-003 — AddCloudWizard credentials page (REQ-002 wizard side).
// DEC-004 (B — QWizardPage subclass), DEC-012 (auth seam injected).
//
// State machine:
//   Idle → InFlight (validatePage with fields populated)
//   InFlight → Success (onAuthFinished matching requestId)
//   InFlight → Error   (onAuthFailed  matching requestId)
//   Error   → re-dispatches on next validatePage() if fields are re-populated
//
// REQ-005: password field is masked, carries IME-sensitivity hints, and is
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

  private slots:
    void onAuthFinished(QUuid id, GarminAuthSuccess result);
    void onAuthFailed(QUuid id, GarminAuthFailure error);

  private:
    enum State { Idle, InFlight, Success, Error };

    IGarminAuthClient* m_auth;
    QLineEdit* m_email = nullptr;
    QLineEdit* m_password = nullptr;
    QLabel* m_message = nullptr;
    State m_state = Idle;
    QUuid m_pendingId;
};

#endif // GC_GarminCredentialsPage_h
