/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminCredentialsPage.h"

#include "GarminErrors.h"

#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

GarminCredentialsPage::GarminCredentialsPage(IGarminAuthClient* authClient, QWidget* parent)
    : QWizardPage(parent), m_auth(authClient)
{
    setTitle(tr("Connect to Garmin Connect"));
    // REQ-NF-Compat-001(b) — static, one-time notice: a Garmin-side session
    // invalidation (e.g. a password change) forces a full re-login rather than
    // a silent reauth. Set once here so a user hitting that mid-flow is not
    // surprised by it; the full limitations text lives in
    // docs/garminconnect-known-limits.md, not crammed into this page.
    // REQ-NF-Sec-004 — the same notice carries the file-based-token
    // disclosure: tokens are stored under the athlete folder with owner-only
    // file permissions rather than in the OS keychain, so same-user malware
    // could read and replay them (residual risk, detailed in the doc above).
    setSubTitle(tr("If Garmin ends your existing session (for example after a password change), "
                   "you may be asked to sign in again here. This is expected. Note that sign-in "
                   "tokens are stored in files under your GoldenCheetah athlete folder rather "
                   "than in your operating system keychain."));

    m_email = new QLineEdit(this);
    m_email->setObjectName(QStringLiteral("garminEmail"));

    m_password = new QLineEdit(this);
    m_password->setObjectName(QStringLiteral("garminPassword"));
    m_password->setEchoMode(QLineEdit::Password);
    // REQ-005: discourage OS/IME from caching the password keystrokes.
    m_password->setInputMethodHints(Qt::ImhSensitiveData | Qt::ImhHiddenText | Qt::ImhNoAutoUppercase |
                                    Qt::ImhNoPredictiveText);

    m_message = new QLabel(this);
    m_message->setObjectName(QStringLiteral("garminAuthMessage"));

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_email);
    layout->addWidget(m_password);
    layout->addWidget(m_message);
    setLayout(layout);

    connect(m_email, &QLineEdit::textChanged, this, [this](const QString&) { emit completeChanged(); });
    connect(m_password, &QLineEdit::textChanged, this, [this](const QString&) { emit completeChanged(); });
    connect(m_auth, &IGarminAuthClient::finished, this, &GarminCredentialsPage::onAuthFinished);
    connect(m_auth, &IGarminAuthClient::failed, this, &GarminCredentialsPage::onAuthFailed);
    // REQ-003 (MFA) Slice B — a mid-auth MFA challenge latches MfaRequired; the
    // wizard's nextId() then routes to the MFA page (id 22).
    connect(m_auth, &IGarminAuthClient::mfaRequired, this, &GarminCredentialsPage::onMfaRequired);
}

bool GarminCredentialsPage::isComplete() const
{
    if (m_state == InFlight)
        return false;
    // Success or MfaRequired both allow Next to advance (the wizard routes to the
    // MFA page for the latter via nextId()).
    if (m_state == Success || m_state == MfaRequired)
        return true;
    // Idle or Error: fields must both be populated.
    return !m_email->text().isEmpty() && !m_password->text().isEmpty();
}

bool GarminCredentialsPage::validatePage()
{
    // Success or MfaRequired: allow the wizard to advance (nextId() decides where).
    if (m_state == Success || m_state == MfaRequired)
        return true;
    if (m_state == InFlight)
        return false;

    // Idle or Error: attempt a new dispatch if fields are populated.
    if (m_email->text().isEmpty() || m_password->text().isEmpty())
        return false;

    m_pendingId = QUuid::createUuid();
    const QString email = m_email->text();
    const QString password = m_password->text();
    m_password->clear(); // REQ-005: zero at dispatch time, not at response time.
    m_state = InFlight;
    emit completeChanged(); // disable Next while in-flight (REQ-NF-Perf-002).
    m_auth->authenticate(email, password, m_pendingId);
    return false;
}

void GarminCredentialsPage::initializePage()
{
    // A3-R003-05 — reset the async state on (re-)entry so a Back-then-Next after
    // a terminal state (Success / Error / MfaRequired) starts a fresh dispatch
    // instead of early-returning on the stale latch. The email field is left
    // intact (Back navigation should preserve it); the password was already
    // zeroed at dispatch time (REQ-005), and the inline message is cleared.
    m_state = Idle;
    m_pendingId = QUuid();
    if (m_message != nullptr)
        m_message->clear();
    emit completeChanged();
}

void GarminCredentialsPage::onAuthFinished(QUuid id, GarminAuthSuccess result)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    if (m_state != InFlight)
        return; // A3-R003-06 — a duplicate/late finished must NOT clobber a
                // latched terminal state (e.g. MfaRequired → routed to page 22).
    m_state = Success;
    emit completeChanged();
    // REQ-008 (DEC-garmin-019 C) — only a FRESH, id-gated success reaches here, so
    // this drives the wizard's persist without ever acting on a stale/late reply.
    emit succeeded(result);
}

void GarminCredentialsPage::onAuthFailed(QUuid id, GarminAuthFailure error)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    if (m_state != InFlight)
        return; // A3-R003-06 — ignore a duplicate/late failure once terminal.
    m_state = Error;
    // Diagnostic-only (Stage 9 live-account triage): Unknown means
    // classifyPendingException() didn't recognize the raised exception as a
    // GarminError. Logs ONLY the exception's module-qualified TYPE name (e.g.
    // "builtins.ValueError") — NEVER translatedMessage/rawMessage, which can
    // carry arbitrary untrusted exception text (request payloads, headers,
    // tokens) unsafe to persist verbatim to a developer log file; a type name
    // cannot carry interpolated secret material. Developer-trace only, mirrors
    // the DES-008/gcObsTrace precedent (GarminConnect.cpp) — the UI never sees
    // this line; see GarminErrors.h's translate() contract enforced by
    // setText() below.
    if (error.kind == GarminAuthFailure::Unknown)
        qDebug().noquote() << QStringLiteral("garmin_auth_unknown exception_type=%1").arg(error.exceptionType);
    // REQ-014: translation happens HERE (page layer), not at the worker —
    // error.translatedMessage carries the raw library text and must not be
    // shown directly (see GarminErrors.h).
    m_message->setText(GarminErrors::translate(error.kind));
    emit completeChanged();
}

void GarminCredentialsPage::onMfaRequired(QUuid id)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    m_state = MfaRequired;
    emit completeChanged();
}
