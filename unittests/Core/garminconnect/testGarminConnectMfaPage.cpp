/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// REQ-003 (MFA) Slice B — GarminMfaPage acceptance (UI half of the criterion):
//   "GC opens a modal Qt dialog with 6-digit numeric input + Submit/Cancel.
//    Valid OTP completes auth. Invalid OTP shows error and re-prompts up to 3
//    attempts; after 3rd fail, the connect attempt aborts with a non-retry
//    error."
//
// DEC-004 realised the "modal dialog" as a QWizardPage subclass driven by the
// AddCloudWizard Back/Next/Cancel machinery (the wizard supplies the Submit =
// Next button; a 3-strikes abort() closes the wizard). This suite unit-tests the
// page against a FAKE IGarminAuthClient (DEC-012 Option A) — Python-free under
// the `garmin-fast` CTest label, mirroring testGarminConnectCredentialsPage.cpp.
//
//   T-032 — 6-digit code + Next dispatches submitMfa(code,id); matching finished
//           drives Success (isComplete()); a non-6-digit code does NOT dispatch.
//   T-033 — the 3-attempts rule: two failures leave the page re-promptable
//           (Error, field cleared, gated on a fresh code, aborted() NOT emitted);
//           the 3rd failure emits aborted() EXACTLY ONCE, sets the non-retry
//           state, and a subsequent validatePage() does NOT dispatch again.
//   T-034 — stale-reply guard: a finished/failed carrying an id != the page's
//           pending id is IGNORED (no state change).
//
// RED expectation: src/Cloud/GarminMfaPage.{h,cpp} do not exist yet — the build
// fails at the #include "GarminMfaPage.h" line (right-reason RED: the missing
// contract under test, not a wiring artifact).

#include "GarminMfaPage.h" // <-- intentionally missing in RED phase
#include "IGarminAuthClient.h"

#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QString>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

// FakeAuthClient — records each submitMfa() call (exact code + requestId) and
// lets a test synthesize finished/failed responses synchronously. Mirrors the
// FakeAuthClient in testGarminConnectCredentialsPage.cpp, extended for the MFA
// flow: submitMfa records (code,id) like authenticate(), plus a synthMfaRequired
// emit helper for symmetry with the seam.
class FakeAuthClient : public IGarminAuthClient
{
    Q_OBJECT
  public:
    struct MfaCall
    {
        QString code;
        QUuid requestId;
    };
    QVector<MfaCall> mfaCalls;

    explicit FakeAuthClient(QObject* parent = nullptr) : IGarminAuthClient(parent) {}

    // The MFA page never calls authenticate(); the credentials page owns that
    // dispatch. A no-op override satisfies the compile-enforced seam.
    void authenticate(const QString&, const QString&, QUuid) override {}

    void submitMfa(const QString& code, QUuid requestId) override { mfaCalls.append(MfaCall{code, requestId}); }

    void synthFinished(QUuid id, GarminAuthSuccess r) { emit finished(id, r); }
    void synthFailed(QUuid id, GarminAuthFailure e) { emit failed(id, e); }
    void synthMfaRequired(QUuid id) { emit mfaRequired(id); }
};

class TestGarminConnectMfaPage : public QObject
{
    Q_OBJECT

  private:
    static QLineEdit* codeField(GarminMfaPage& page)
    {
        return page.findChild<QLineEdit*>(QStringLiteral("garminMfaCode"));
    }
    static QLabel* messageField(GarminMfaPage& page)
    {
        return page.findChild<QLabel*>(QStringLiteral("garminMfaMessage"));
    }
    // Enter a 6-digit code as if typed by the user. The field carries an input
    // mask of "999999", so a fully-entered code round-trips through text().
    static void enterCode(GarminMfaPage& page, const QString& code) { codeField(page)->setText(code); }

  private slots:

    // The page must expose a discoverable 6-digit code field + a message label.
    void pageHasSixDigitCodeAndMessageFields()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);
        QLineEdit* code = codeField(page);
        QLabel* msg = messageField(page);
        QVERIFY2(code != nullptr, "GarminMfaPage must expose a QLineEdit named 'garminMfaCode'");
        QVERIFY2(msg != nullptr, "GarminMfaPage must expose a QLabel named 'garminMfaMessage'");
        // A 7th digit must be rejected by the mask (max 6 numeric positions).
        code->setText(QStringLiteral("1234567"));
        QVERIFY2(code->text().length() <= 6, "MFA code field must accept at most 6 digits (input mask '999999')");
    }

    // T-032 — a 6-digit code + Next dispatches submitMfa(code,id); a matching
    // finished(id,…) drives the page to Success (isComplete() true). A non-6-digit
    // code does NOT dispatch.
    void sixDigitCodeDispatchesSubmitMfaAndSuccessCompletes()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);

        // Non-6-digit code must NOT dispatch and must not be complete.
        enterCode(page, QStringLiteral("123"));
        QVERIFY2(!page.isComplete(), "A partial (non-6-digit) code must leave the page not-complete");
        QVERIFY2(!page.validatePage(), "validatePage() with a partial code must defer (return false)");
        QCOMPARE(fake.mfaCalls.size(), 0);

        // A full 6-digit code dispatches submitMfa with the EXACT code.
        enterCode(page, QStringLiteral("246810"));
        QVERIFY2(page.isComplete(), "A full 6-digit code must make the page complete (Next enables)");
        const bool advancedSynchronously = page.validatePage();
        QVERIFY2(!advancedSynchronously, "validatePage() must defer the advance until the async MFA response arrives");
        QCOMPARE(fake.mfaCalls.size(), 1);
        QCOMPARE(fake.mfaCalls.first().code, QStringLiteral("246810"));
        QVERIFY2(!fake.mfaCalls.first().requestId.isNull(),
                 "Each submitMfa() must carry a non-null QUuid so the page can correlate the response");

        // While in-flight, Next must be disabled.
        QVERIFY2(!page.isComplete(), "In-flight MFA submit must disable Next (isComplete() false)");

        // A matching success drives the page to Success → complete / advances.
        GarminAuthSuccess ok{QStringLiteral("uid-1"), QStringLiteral("Rider")};
        fake.synthFinished(fake.mfaCalls.first().requestId, ok);
        QVERIFY2(page.isComplete(), "After a matching MFA success, isComplete() must be true (Next re-enables)");
        QVERIFY2(page.validatePage(), "After a matching MFA success, validatePage() must return true so the wizard "
                                      "advances past the MFA page");
    }

    // T-033 — the 3-attempts rule.
    void thirdInvalidCodeAbortsExactlyOnceAndStopsDispatching()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);
        QSignalSpy abortedSpy(&page, &GarminMfaPage::aborted);

        GarminAuthFailure bad;
        bad.kind = GarminAuthFailure::Auth;
        bad.translatedMessage = QStringLiteral("That code was not correct. Please try again.");

        // Attempt 1 — dispatch then fail: re-promptable Error, field cleared.
        enterCode(page, QStringLiteral("111111"));
        QVERIFY(!page.validatePage());
        QCOMPARE(fake.mfaCalls.size(), 1);
        fake.synthFailed(fake.mfaCalls.at(0).requestId, bad);
        QCOMPARE(abortedSpy.count(), 0);
        QVERIFY2(codeField(page)->text().isEmpty(), "After an invalid code the field must be cleared for re-entry");
        QVERIFY2(!page.isComplete(), "After a failure with the field cleared, the page must be gated on a fresh code");
        QVERIFY2(messageField(page)->text() == bad.translatedMessage,
                 "The re-prompt message must surface the translated failure verbatim");

        // Attempt 2 — dispatch then fail: still re-promptable, still no abort.
        enterCode(page, QStringLiteral("222222"));
        QVERIFY(!page.validatePage());
        QCOMPARE(fake.mfaCalls.size(), 2);
        fake.synthFailed(fake.mfaCalls.at(1).requestId, bad);
        QCOMPARE(abortedSpy.count(), 0);
        QVERIFY2(codeField(page)->text().isEmpty(), "Second failure must also clear the field");
        QVERIFY2(!page.isComplete(), "Second failure must leave the page gated on a fresh code (still re-promptable)");

        // Attempt 3 — dispatch then fail: aborts EXACTLY once, non-retry state.
        enterCode(page, QStringLiteral("333333"));
        QVERIFY(!page.validatePage());
        QCOMPARE(fake.mfaCalls.size(), 3);
        fake.synthFailed(fake.mfaCalls.at(2).requestId, bad);
        QCOMPARE(abortedSpy.count(), 1);
        QVERIFY2(!messageField(page)->text().isEmpty(),
                 "The 3rd failure must set a non-retry (abort) message on the page");

        // Post-abort: a further Next must NOT dispatch another submitMfa.
        enterCode(page, QStringLiteral("444444"));
        QVERIFY2(!page.validatePage(), "After the 3-strikes abort, validatePage() must return false (cannot proceed)");
        QVERIFY2(!page.isComplete(), "After the 3-strikes abort, the page must stay not-complete");
        QCOMPARE(fake.mfaCalls.size(), 3); // submit-call count must NOT increment
        QCOMPARE(abortedSpy.count(), 1);   // aborted() must not fire again
    }

    // T-034 — stale-reply guard: a finished/failed carrying an id != the page's
    // pending id is ignored (no state change).
    void staleReplyIsIgnored()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);
        QSignalSpy abortedSpy(&page, &GarminMfaPage::aborted);

        enterCode(page, QStringLiteral("555555"));
        QVERIFY(!page.validatePage()); // dispatch; m_pendingId = mfaCalls[0].requestId
        QCOMPARE(fake.mfaCalls.size(), 1);

        const QUuid staleId = QUuid::createUuid();
        QVERIFY(staleId != fake.mfaCalls.first().requestId);

        // Stale success — must NOT advance the page.
        GarminAuthSuccess ghost{QStringLiteral("uid-stale"), QStringLiteral("Ghost")};
        fake.synthFinished(staleId, ghost);
        QVERIFY2(!page.isComplete(), "A stale 'finished' (mismatched id) must not complete the page (still in-flight)");
        QVERIFY2(!page.validatePage(), "A stale 'finished' must not let validatePage() advance (still in-flight)");

        // Stale failure — must NOT count against the 3-attempt budget nor abort.
        GarminAuthFailure err;
        err.kind = GarminAuthFailure::Auth;
        err.translatedMessage = QStringLiteral("stale");
        fake.synthFailed(staleId, err);
        QCOMPARE(abortedSpy.count(), 0);
        // T-040 — A3-R003-07 / mutant M1: the stale failure must NOT charge the
        // attempt budget and must NOT change the page state (it is still
        // in-flight on the pending id). This replaces the former tautological
        // `... == false || true` placeholder. With the `id != m_pendingId` guard
        // removed from GarminMfaPage::onAuthFailed (mutant M1) the stale failure
        // would increment the counter to 1 — so this assertion KILLS M1.
        QCOMPARE(page.attemptCount(), 0);
        QVERIFY2(!page.isComplete(), "a stale failure (mismatched id) must leave the page in-flight — state unchanged");

        // The real reply on the pending id must now drive Success.
        GarminAuthSuccess ok{QStringLiteral("uid-real"), QStringLiteral("Rider")};
        fake.synthFinished(fake.mfaCalls.first().requestId, ok);
        QVERIFY2(page.validatePage(), "After the matching 'finished', validatePage() must return true");
    }

    // T-041 — A3-R003-06: a duplicate delivery on the SAME (still-pending) id
    // AFTER the 3-strikes abort must NOT re-run the terminal transition. Without
    // the `if (m_state != InFlight) return;` guard in onAuthFailed the duplicate
    // re-enters the ++m_attempts / abort branch, firing aborted() a second time
    // and over-counting the budget (RED). With the guard it is ignored.
    void duplicateFailureAfterAbortDoesNotReabort()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);
        QSignalSpy abortedSpy(&page, &GarminMfaPage::aborted);

        GarminAuthFailure bad;
        bad.kind = GarminAuthFailure::Auth;
        bad.translatedMessage = QStringLiteral("That code was not correct.");

        for (int i = 0; i < 3; ++i) {
            enterCode(page, QStringLiteral("111111"));
            QVERIFY(!page.validatePage());
            fake.synthFailed(fake.mfaCalls.at(i).requestId, bad);
        }
        QCOMPARE(abortedSpy.count(), 1);
        QCOMPARE(page.attemptCount(), 3);

        // Duplicate delivery of the SAME (still-pending) id after the abort.
        const QUuid pendingId = fake.mfaCalls.at(2).requestId;
        fake.synthFailed(pendingId, bad);

        QCOMPARE(abortedSpy.count(), 1);   // A3-R003-06: must NOT abort twice
        QCOMPARE(page.attemptCount(), 3);  // must NOT exceed the 3-strike budget
        QCOMPARE(fake.mfaCalls.size(), 3); // no fresh dispatch
    }

    // T-039 — A3-R003-05: initializePage() (Back-then-Next re-entry) resets the
    // async state so a page latched in a terminal state (Aborted) re-reads a
    // fresh code and dispatches a NEW submitMfa instead of staying latched.
    // Without the initializePage() override the base no-op leaves the stale
    // Aborted latch (RED at the first assertion).
    void reentryResetsTerminalStateAndAllowsFreshDispatch()
    {
        FakeAuthClient fake;
        GarminMfaPage page(&fake);

        GarminAuthFailure bad;
        bad.kind = GarminAuthFailure::Auth;
        bad.translatedMessage = QStringLiteral("nope");
        for (int i = 0; i < 3; ++i) {
            enterCode(page, QStringLiteral("111111"));
            QVERIFY(!page.validatePage());
            fake.synthFailed(fake.mfaCalls.at(i).requestId, bad);
        }
        QVERIFY(page.isAborted());
        QCOMPARE(page.attemptCount(), 3);

        // Re-entry (Back then Next) — must clear the latch, counter and fields.
        page.initializePage();
        QVERIFY2(!page.isAborted(), "re-entry must clear the 3-strikes Aborted latch");
        QCOMPARE(page.attemptCount(), 0);
        QVERIFY2(codeField(page)->text().isEmpty(), "re-entry must clear the code field");
        QVERIFY2(messageField(page)->text().isEmpty(), "re-entry must clear the inline message");

        // A fresh 6-digit code now dispatches a NEW submitMfa (not early-return).
        enterCode(page, QStringLiteral("246810"));
        QVERIFY2(page.isComplete(), "after re-entry a full code must re-enable Next");
        QVERIFY(!page.validatePage());
        QCOMPARE(fake.mfaCalls.size(), 4);
        QCOMPARE(fake.mfaCalls.at(3).code, QStringLiteral("246810"));
    }
};

QTEST_MAIN(TestGarminConnectMfaPage)
#include "testGarminConnectMfaPage.moc"
