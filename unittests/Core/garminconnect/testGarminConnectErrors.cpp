/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// REQ-014 — friendly error translation: "GC maps each [failure] to a
// user-friendly localized message via a fixed switch table, never showing
// the raw exception name. Unknown codes fall back to a generic 'Connection
// to Garmin Connect failed' + code."
//
// Scope: GarminErrors::translate(GarminAuthFailure::Kind) — the page-layer
// translation module (DES-008, adapted; see GarminErrors.h for why it keys
// on Kind rather than design.md's sample exception-class-name keying).
// GarminCredentialsPage / GarminMfaPage call sites are covered by their own
// suites (testGarminConnectCredentialsPage / testGarminConnectMfaPage).
//
// RED expectation: src/Cloud/GarminErrors.h does not exist yet — the build
// fails at the #include line below (right-reason RED).

#include "GarminErrors.h"

#include <QString>
#include <QtTest/QtTest>

class TestGarminConnectErrors : public QObject
{
    Q_OBJECT

  private slots:

    // Auth/Network/RateLimit each get their OWN non-empty message, and the
    // three are pairwise distinct (a mutant collapsing the switch to one
    // return statement is caught by the distinctness checks below).
    void eachKnownKindGetsADistinctNonEmptyMessage()
    {
        const QString auth = GarminErrors::translate(GarminAuthFailure::Auth);
        const QString network = GarminErrors::translate(GarminAuthFailure::Network);
        const QString rateLimit = GarminErrors::translate(GarminAuthFailure::RateLimit);

        QVERIFY2(!auth.isEmpty(), "Auth must have a non-empty translated message");
        QVERIFY2(!network.isEmpty(), "Network must have a non-empty translated message");
        QVERIFY2(!rateLimit.isEmpty(), "RateLimit must have a non-empty translated message");

        QVERIFY2(auth != network, "Auth and Network must have distinct copy");
        QVERIFY2(auth != rateLimit, "Auth and RateLimit must have distinct copy");
        QVERIFY2(network != rateLimit, "Network and RateLimit must have distinct copy");
    }

    // REQ-014 acceptance: "never showing the raw exception name." A hard-coded
    // sample of raw-looking library text must never appear as a SUBSTRING of
    // any translated message (catches a mutant that interpolates rawMessage).
    void translatedMessagesNeverContainRawLibraryText()
    {
        const QStringList rawSamples = {
            QStringLiteral("GarminConnectAuthenticationError"),
            QStringLiteral("GarminConnectConnectionError"),
            QStringLiteral("GarminConnectTooManyRequestsError"),
            QStringLiteral("Traceback (most recent call last)"),
        };
        const QList<GarminAuthFailure::Kind> kinds = {GarminAuthFailure::Auth, GarminAuthFailure::Network,
                                                      GarminAuthFailure::RateLimit, GarminAuthFailure::Unknown};

        for (const auto kind : kinds) {
            const QString msg = GarminErrors::translate(kind);
            for (const QString& raw : rawSamples) {
                QVERIFY2(!msg.contains(raw),
                         qPrintable(QStringLiteral("translated message must not contain raw text %1").arg(raw)));
            }
        }
    }

    // REQ-014 acceptance, Unknown fallback: "a generic 'Connection to Garmin
    // Connect failed' + code" — non-empty, and carries some code-like token
    // beyond the bare generic sentence (so a mutant that drops the "+ code"
    // half is caught).
    void unknownKindFallsBackToGenericMessageWithCode()
    {
        const QString msg = GarminErrors::translate(GarminAuthFailure::Unknown);

        QVERIFY2(!msg.isEmpty(), "Unknown must still produce a non-empty message");
        QVERIFY2(msg.contains(QStringLiteral("Garmin Connect"), Qt::CaseInsensitive),
                 "Unknown fallback must name Garmin Connect (per REQ-014's generic wording)");
        QVERIFY2(msg.length() > QStringLiteral("Connection to Garmin Connect failed").length(),
                 "REQ-014: the fallback must carry a generic message PLUS a code, not the bare sentence alone");
    }

    // translate() is a pure function of `kind` — same input, same output,
    // repeatable (kills a mutant that introduces hidden state/randomness).
    void translateIsPureAndRepeatable()
    {
        QCOMPARE(GarminErrors::translate(GarminAuthFailure::Auth), GarminErrors::translate(GarminAuthFailure::Auth));
        QCOMPARE(GarminErrors::translate(GarminAuthFailure::RateLimit),
                 GarminErrors::translate(GarminAuthFailure::RateLimit));
    }
};

QTEST_GUILESS_MAIN(TestGarminConnectErrors)
#include "testGarminConnectErrors.moc"
