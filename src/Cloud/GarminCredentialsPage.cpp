/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#include "GarminCredentialsPage.h"

#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

GarminCredentialsPage::GarminCredentialsPage(IGarminAuthClient* authClient, QWidget* parent)
    : QWizardPage(parent), m_auth(authClient)
{
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
}

bool GarminCredentialsPage::isComplete() const
{
    if (m_state == InFlight)
        return false;
    if (m_state == Success)
        return true;
    // Idle or Error: fields must both be populated.
    return !m_email->text().isEmpty() && !m_password->text().isEmpty();
}

bool GarminCredentialsPage::validatePage()
{
    if (m_state == Success)
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

void GarminCredentialsPage::onAuthFinished(QUuid id, GarminAuthSuccess)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    m_state = Success;
    emit completeChanged();
}

void GarminCredentialsPage::onAuthFailed(QUuid id, GarminAuthFailure error)
{
    if (id != m_pendingId)
        return; // stale reply guard.
    m_state = Error;
    m_message->setText(error.translatedMessage);
    emit completeChanged();
}
