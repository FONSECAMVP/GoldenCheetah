/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST-003 — REQ-002 (wizard-side acceptance): "Valid email+password produces
//   persisted OAuth tokens; invalid credentials produce a labeled error."
//                — and —
//                REQ-005 (wizard-side enforcement): "Password never persisted /
//                only held in dialog memory until SSO completes, then zeroed."
//
// Slice scope — C++ wizard wiring (sibling to the GREEN Python adapter slice
// covered by TEST-002). MFA (REQ-003), CAPTCHA (REQ-015), ToS notice (REQ-009),
// backfill (REQ-010), token persistence (REQ-004 / REQ-006), and the worker
// off-GUI-thread enforcement (REQ-NF-Threads-001) are explicitly deferred to
// their own slices; this file does not exercise them.
//
// Cites:
//   DES-003   — AddCloudWizard pages (credentials, MFA, CAPTCHA, ToS, backfill)
//   DES-003a  — IGarminAuthClient interface (DEC-012)
//   DEC-012   — auth-dispatcher seam shape (Option A — interface injection)
//   DEC-004   — credentials + MFA dialog UX shape
//   DEC-008   — testing toolchain (QTest+CTest under `garmin-fast` label)
//   DoD       — must-have negative path + REQ-005 hand-off
//
// RED expectation:
//   Both src/Cloud/GarminCredentialsPage.{h,cpp} and src/Cloud/IGarminAuthClient.h
//   do not exist yet. The build fails at the #include lines below — that is the
//   right-reason RED signal (missing contract under test, not a wiring artifact).
//   GREEN introduces those files and makes every test pass without altering the
//   public surface this test locks in.

#include "GarminCredentialsPage.h" // <-- intentionally missing in RED phase
#include "IGarminAuthClient.h"     // <-- intentionally missing in RED phase

#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QString>
#include <QUuid>
#include <QVector>
#include <QtTest/QtTest>

// FakeAuthClient — records each authenticate() call, lets a test synthesize
// finished/failed responses synchronously. The page's behaviour under each
// kind of response is what TEST-003 locks in. Concurrency is not exercised
// here (see DES-001 / REQ-NF-Threads-001 — separate slice).
class FakeAuthClient : public IGarminAuthClient
{
    Q_OBJECT
  public:
    struct Call
    {
        QString email;
        QString password;
        QUuid requestId;
    };
    QVector<Call> calls;

    explicit FakeAuthClient(QObject* parent = nullptr) : IGarminAuthClient(parent) {}

    void authenticate(const QString& email, const QString& password, QUuid requestId) override
    {
        calls.append(Call{email, password, requestId});
    }

    void synthFinished(QUuid id, GarminAuthSuccess r) { emit finished(id, r); }
    void synthFailed(QUuid id, GarminAuthFailure e) { emit failed(id, e); }
};

class TestGarminConnectCredentialsPage : public QObject
{
    Q_OBJECT

  private:
    static QLineEdit* emailField(GarminCredentialsPage& page)
    {
        QLineEdit* e = page.findChild<QLineEdit*>(QStringLiteral("garminEmail"));
        return e;
    }
    static QLineEdit* passwordField(GarminCredentialsPage& page)
    {
        QLineEdit* p = page.findChild<QLineEdit*>(QStringLiteral("garminPassword"));
        return p;
    }
    static QLabel* messageField(GarminCredentialsPage& page)
    {
        return page.findChild<QLabel*>(QStringLiteral("garminAuthMessage"));
    }
    static void populate(GarminCredentialsPage& page, const QString& email, const QString& password)
    {
        emailField(page)->setText(email);
        passwordField(page)->setText(password);
    }

  private slots:

    // The page must expose discoverable QLineEdits for email + password. The
    // wizard-tile contract test (TEST-001) locked the *service* surface; this
    // locks the *page* surface a future page-flow test or theming pass needs.
    void pageHasEmailAndPasswordFields()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        QLineEdit* email = emailField(page);
        QLineEdit* pass = passwordField(page);
        QVERIFY2(email != nullptr, "GarminCredentialsPage must expose a QLineEdit named 'garminEmail'");
        QVERIFY2(pass != nullptr, "GarminCredentialsPage must expose a QLineEdit named 'garminPassword'");
        QVERIFY(email->isEnabled());
        QVERIFY(pass->isEnabled());
    }

    // Password field must mask input (REQ-005 wizard-side surface hardening).
    void passwordFieldIsMasked()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        QLineEdit* pass = passwordField(page);
        QVERIFY(pass != nullptr);
        QCOMPARE(pass->echoMode(), QLineEdit::Password);
    }

    // Password field must hint to the OS/IME that this is sensitive data so
    // platform autocomplete, predictive-text caches, and clipboard-suggest
    // surfaces stay away from the password. (REQ-005 wizard-side defence-in-
    // depth — pairs with the post-submit clear below.)
    void passwordFieldDisablesAutocomplete()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        QLineEdit* pass = passwordField(page);
        QVERIFY(pass != nullptr);
        const Qt::InputMethodHints hints = pass->inputMethodHints();
        QVERIFY2((hints & Qt::ImhSensitiveData) != 0, "REQ-005: password QLineEdit must set Qt::ImhSensitiveData "
                                                      "(no OS/IME caching of the keystrokes)");
        QVERIFY2((hints & Qt::ImhHiddenText) != 0, "REQ-005: password QLineEdit must set Qt::ImhHiddenText");
        QVERIFY2((hints & Qt::ImhNoAutoUppercase) != 0, "REQ-005: password QLineEdit must set Qt::ImhNoAutoUppercase");
        QVERIFY2((hints & Qt::ImhNoPredictiveText) != 0,
                 "REQ-005: password QLineEdit must set Qt::ImhNoPredictiveText "
                 "(prevents the IME predictive-text cache from retaining the password)");
    }

    // QWizard reads isComplete() to enable/disable Next. With either field
    // empty the page must report not-complete so the wizard cannot advance
    // past a blank credentials state — DoD must-have negative path.
    void validationBlocksEmptyFields()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);

        QVERIFY2(!page.isComplete(), "Initial state (both fields empty) must not be complete");

        emailField(page)->setText(QStringLiteral("rider@example.com"));
        QVERIFY2(!page.isComplete(), "Email alone (no password) must not be complete");

        emailField(page)->clear();
        passwordField(page)->setText(QStringLiteral("hunter2"));
        QVERIFY2(!page.isComplete(), "Password alone (no email) must not be complete");

        emailField(page)->setText(QStringLiteral("rider@example.com"));
        QVERIFY2(page.isComplete(), "Both fields populated must be complete (Next becomes enabled)");
    }

    // Core REQ-002 wizard-side acceptance, positive half: clicking Next
    // dispatches an Authenticate request with the user-entered credentials.
    // The page itself does not return success synchronously — it defers the
    // wizard advance until the async response arrives. (See the next two
    // tests for the response-handling halves.)
    void submittingDispatchesAuthenticate()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        populate(page, QStringLiteral("rider@example.com"), QStringLiteral("hunter2"));

        const bool advancedSynchronously = page.validatePage();
        QVERIFY2(!advancedSynchronously, "validatePage() must defer advance until async auth completes");
        QCOMPARE(fake.calls.size(), 1);
        QCOMPARE(fake.calls.first().email, QStringLiteral("rider@example.com"));
        QCOMPARE(fake.calls.first().password, QStringLiteral("hunter2"));
        QVERIFY2(!fake.calls.first().requestId.isNull(),
                 "Each authenticate() must carry a non-null QUuid so the page can "
                 "correlate the response (defence against stale-response acceptance)");
    }

    // REQ-002 acceptance, success half. After Authenticate succeeds, the
    // next call to validatePage() (driven by QWizard re-evaluating Next)
    // must allow the wizard to advance.
    void successResponseAdvancesPage()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        populate(page, QStringLiteral("rider@example.com"), QStringLiteral("hunter2"));
        QVERIFY(!page.validatePage()); // dispatch
        QCOMPARE(fake.calls.size(), 1);

        GarminAuthSuccess result{QStringLiteral("uid-12345"), QStringLiteral("Rider")};
        fake.synthFinished(fake.calls.first().requestId, result);

        QVERIFY2(page.validatePage(), "After success response, validatePage() must return true so the "
                                      "wizard advances on the next Next click (or programmatic next())");
    }

    // REQ-002 acceptance, error half: "invalid credentials produce a
    // labeled error" — the user-facing label must be populated, non-empty,
    // and branded enough that the test can distinguish it from a default
    // fallback. The page must NOT advance on error.
    //
    // The error payload is already DES-008-translated by the time it
    // reaches the page (the adapter / worker translate at the seam). This
    // test asserts the page surfaces the translated string verbatim — it
    // does NOT re-translate, double-translate, or fall back to the raw
    // exception class name.
    void errorResponseShowsLabeledInlineMessage()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        populate(page, QStringLiteral("rider@example.com"), QStringLiteral("wrong"));
        QVERIFY(!page.validatePage());
        QCOMPARE(fake.calls.size(), 1);

        GarminAuthFailure err;
        err.kind = GarminAuthFailure::Auth;
        err.translatedMessage =
            QStringLiteral("Garmin Connect rejected your email or password. Please check and try again.");
        fake.synthFailed(fake.calls.first().requestId, err);

        QLabel* msg = messageField(page);
        QVERIFY2(msg != nullptr, "Page must expose a QLabel named 'garminAuthMessage' for inline errors");
        QVERIFY2(!msg->text().isEmpty(),
                 "REQ-002 negative path: an error response must populate the inline message label");
        QVERIFY2(msg->text() == err.translatedMessage,
                 "Page must surface the translated message verbatim — never raw exception names "
                 "and never a re-translated string (A3 mutant kill: catches a missing assignment "
                 "that falls back to a hard-coded placeholder).");
        QVERIFY2(!page.validatePage(), "After an error, validatePage() must remain false so the user is forced to "
                                       "edit the credentials and resubmit before the wizard advances");
    }

    // REQ-005 wizard-side enforcement: the password is consumed by the
    // dispatch and zeroed in the widget so it cannot be read back by a
    // later UI inspection, a wizard back-navigation, or a Qt accessibility
    // tree dump. The pair to the password-mask + IME-hint defences above.
    //
    // This must hold regardless of the eventual auth outcome — the field is
    // cleared at dispatch time, not at success/failure time. (A backup hold-
    // until-success approach would leak the password back into the widget if
    // the user navigates Back to retry; the at-dispatch clear closes that.)
    void passwordClearedAfterSubmit()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        populate(page, QStringLiteral("rider@example.com"), QStringLiteral("hunter2"));
        QLineEdit* pass = passwordField(page);

        QVERIFY(!page.validatePage());
        QVERIFY2(pass->text().isEmpty(), "REQ-005 wizard-side: password field must be cleared the moment the "
                                         "authenticate() request is dispatched — never retained in the widget");
    }

    // REQ-NF-Perf-002 page-side hardening: while an authenticate() is in
    // flight, isComplete() must report false so QWizard disables Next.
    // Prevents the user from triggering a second concurrent dispatch (which
    // the worker mailbox would also reject — but disabling Next at the page
    // layer gives the right UX, not a confusing error toast).
    void inFlightStateDisablesNextButton()
    {
        FakeAuthClient fake;
        GarminCredentialsPage page(&fake);
        populate(page, QStringLiteral("rider@example.com"), QStringLiteral("hunter2"));
        QVERIFY2(page.isComplete(), "Pre-dispatch (fields populated): isComplete() must be true so Next enables");

        QVERIFY(!page.validatePage()); // dispatch — in-flight state begins
        QVERIFY2(!page.isComplete(), "REQ-NF-Perf-002 page-side: isComplete() must be false during in-flight to "
                                     "disable Next and prevent a second concurrent dispatch");
    }
};

QTEST_MAIN(TestGarminConnectCredentialsPage)
#include "testGarminConnectCredentialsPage.moc"
