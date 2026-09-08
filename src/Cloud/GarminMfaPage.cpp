/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminMfaPage.h"

#include "GarminErrors.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace {
// A fully-entered 6-digit OTP. The field carries an input mask of "999999", so
// unfilled positions can surface as blanks in text(); count the digits directly
// rather than trusting the raw string length.
bool hasSixDigits(const QString& text)
{
    int digits = 0;
    for (const QChar c : text)
        if (c.isDigit())
            ++digits;
    return digits == 6;
}
} // namespace

GarminMfaPage::GarminMfaPage(IGarminAuthClient* authClient, QWidget* parent) : QWizardPage(parent), m_auth(authClient)
{
    m_code = new QLineEdit(this);
    m_code->setObjectName(QStringLiteral("garminMfaCode"));
    // REQ-003 — 6-digit numeric input. '9' == an optional ASCII digit position,
    // so the mask caps the field at 6 digits; validatePage()/isComplete() gate
    // on a full six via hasSixDigits().
    m_code->setInputMask(QStringLiteral("999999"));

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("garminMfaMessage"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_code);
    layout->addWidget(m_message);
    setLayout(layout);

    connect(m_code, &QLineEdit::textChanged, this, [this](const QString&) { emit completeChanged(); });
    connect(m_auth, &IGarminAuthClient::finished, this, &GarminMfaPage::onAuthFinished);
    connect(m_auth, &IGarminAuthClient::failed, this, &GarminMfaPage::onAuthFailed);
    // NB: mfaRequired is the credentials page's concern (it decides to route
    // here); this page only submits the OTP and reacts to finished/failed.
}

bool GarminMfaPage::isComplete() const
{
    if (m_state == InFlight)
        return false;
    if (m_state == Aborted)
        return false;
    if (m_state == Success)
        return true;
    // Idle or Error: require a full 6-digit code before Next enables.
    return hasSixDigits(m_code->text());
}

bool GarminMfaPage::validatePage()
{
    if (m_state == Success)
        return true;
    if (m_state == InFlight)
        return false;
    if (m_state == Aborted)
        return false; // 3-strikes: cannot proceed, no further dispatch.

    // Idle or Error: dispatch a new OTP submission if a full code is present and
    // the attempt budget is not exhausted (belt-and-braces — Aborted already
    // guards the exhausted case above).
    if (!hasSixDigits(m_code->text()) || m_attempts >= kMaxAttempts)
        return false;

    m_pendingId = QUuid::createUuid();
    const QString code = m_code->text();
    m_state = InFlight;
    emit completeChanged(); // disable Next while the OTP submission is in flight.
    m_auth->submitMfa(code, m_pendingId);
    return false;
}

void GarminMfaPage::initializePage()
{
    // A3-R003-05 — reset the async state on (re-)entry so a Back-then-Next after
    // a terminal state does not present the stale Success/Aborted/Error latch.
    // The wizard re-reads the code the user types afresh.
    m_state = Idle;
    m_attempts = 0;
    m_pendingId = QUuid();
    if (m_code != nullptr)
        m_code->clear();
    if (m_message != nullptr)
        m_message->clear();
    emit completeChanged();
}

void GarminMfaPage::onAuthFinished(QUuid id, GarminAuthSuccess result)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    if (m_state != InFlight)
        return; // A3-R003-06 — ignore a duplicate/late delivery once terminal.
    m_state = Success;
    emit completeChanged();
    // REQ-008 (DEC-garmin-019 C) — id-gated post-MFA success drives the wizard's
    // persist; a stale/late reply never reaches here so it can never persist.
    emit succeeded(result);
}

void GarminMfaPage::onAuthFailed(QUuid id, GarminAuthFailure error)
{
    if (id != m_pendingId)
        return; // stale reply guard — does not count against the attempt budget.
    if (m_state != InFlight)
        return; // A3-R003-06 — a duplicate/late failure must NOT re-run the
                // terminal transition (would re-emit aborted() / over-count).

    ++m_attempts;
    if (m_attempts >= kMaxAttempts) {
        // REQ-003 — after the 3rd invalid OTP the connect attempt aborts with a
        // non-retry error: latch Aborted, show the terminal message, and signal
        // the wizard to end the attempt. No further submits are accepted.
        m_state = Aborted;
        // Reviewer delta-fix (REQ-014): the wrong-OTP text is only correct when
        // the terminal failure really was Auth; Network/RateLimit/Unknown on the
        // 3rd attempt must still get the kind-translated message.
        if (error.kind == GarminAuthFailure::Auth) {
            m_message->setText(tr("Too many incorrect codes. Garmin Connect sign-in has been cancelled. "
                                  "Please start the connection again."));
        } else {
            m_message->setText(GarminErrors::translate(error.kind));
        }
        emit aborted();
        return;
    }

    // Attempts remain: show the error and re-prompt with a cleared field.
    m_state = Error;
    // REQ-014: translation happens HERE (page layer) — error.translatedMessage
    // carries the raw library text and must not be shown directly (GarminErrors.h).
    m_message->setText(GarminErrors::translate(error.kind));
    m_code->clear();
    emit completeChanged();
}
