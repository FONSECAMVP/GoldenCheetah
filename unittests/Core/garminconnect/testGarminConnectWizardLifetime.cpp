/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the License as published by the Free Software Foundation.
 */

// REQ-020 / T-161..T-163 — the OAuth "Add Cloud Account" wizard must not
// use-after-free while the user completes browser OAuth.
//
// Closes finding S-R021-03 under DEC-030's accepted rider shape (per-site
// QPointer self-bails, inherited by REQ-020). The REAL
// src/Cloud/AddCloudWizard.cpp is compiled into this executable via the
// force-included stubs/WizardLifetimeStubPreamble.h (sibling of the routing
// target's preamble; see the block comment there). The three frames under
// test, and the axis each exercises:
//
//   AddAuth::doAuth()          own lifetime   — the wizard (and this page,
//                                               its child) deleted inside
//                                               oauthDialog->exec(); the
//                                               parentless oauthDialog
//                                               SURVIVES, so exec() returns
//                                               normally onto a freed frame.
//   AddSettings::browseFolder() own lifetime  — the wizard deleted inside
//                                               the REAL QMessageBox::exec()
//                                               (the err box at :797).
//                            collaborator     — the Context deleted inside
//                                               dialog.exec() (the folder
//                                               picker at :802) while the
//                                               window-parented, NON-MODAL
//                                               wizard survives.
//   AddFinish::validatePage()  collaborator   — the :887 chain
//                                               (wizard->context->athlete->
//                                               cyclist) after that Context
//                                               is gone.
//
// The wizard is QWizard(context->mainWindow) + WA_DeleteOnClose +
// Qt::NonModal (AddCloudWizard.cpp ctor), so the athlete tab can close
// (freeing tab, Athlete and Context synchronously, MainWindow.cpp:2183-2185)
// while the wizard lives on — that asymmetry is what the collaborator-axis
// runs reproduce, exactly as TEST-082/083 did for the sync/upload dialogs.
//
// VERDICTS ARE ASAN VERDICTS: the RED half of every teardown run is a
// heap-use-after-free abort, not a QCOMPARE. The target therefore fails to
// compile without AddressSanitizer rather than silently running uninstrumented.

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
// instrumented — ok
#else
#    error "testGarminConnectWizardLifetime (T-161..163) is a lifetime test and requires AddressSanitizer"
#endif

#include "AddCloudWizard.h"

#include <QApplication>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>
#include <QtTest/QtTest>

// --- the scriptable seams of WizardLifetimeStubPreamble (force-included) ---
// g_oauthLoop / g_oauthDialogsConstructed / g_oauthSslMissing,
// g_folderLoop / g_folderExecRunsLoop / g_scriptedFolderDialogResult,
// g_openFails / g_folderSelectedCalls / g_saveSettingsCalls /
// g_syncDialogsConstructed / g_setCValueCalls.

namespace {

// The only capability override doAuth() branches on — "live for every OAuth
// service" is represented by the generic OAuth page (20), which is exactly
// what this service selects.
class OAuthCapableService : public CloudService
{
  public:
    int capabilities() const override { return CloudService::OAuth; }
};

void resetLifeCounters()
{
    g_openFails = false;
    g_folderSelectedCalls = 0;
    g_saveSettingsCalls = 0;
    g_syncDialogsConstructed = 0;
    g_setCValueCalls = 0;
    g_lastCValueCyclist.clear();
    g_folderExecRunsLoop = false;
    g_scriptedFolderDialogResult = 0;
    g_scriptedFolderPath.clear();
    g_folderLoop = nullptr;
    g_oauthDialogsConstructed = 0;
    g_oauthSslMissing = false;
    g_oauthLoop = nullptr;
}

// The err box of browseFolder (:793-797) is a REAL parentless stack
// QMessageBox in exec(); close whatever modal is up so its loop returns.
void dismissActiveModalMessageBox()
{
    QWidget* modal = QApplication::activeModalWidget();
    if (!modal) {
        const auto tops = QApplication::topLevelWidgets();
        for (QWidget* t : tops) {
            if (auto* mb = qobject_cast<QMessageBox*>(t)) {
                modal = mb;
                break;
            }
        }
    }
    if (auto* mb = qobject_cast<QMessageBox*>(modal))
        mb->done(QMessageBox::Ok);
}

} // namespace

class TestGarminConnectWizardLifetime : public QObject
{
    Q_OBJECT

  private slots:

    // --- T-161 — REQ-020 acceptance: teardown DURING the OAuth exec --------
    //
    // (a) OWN LIFETIME: the wizard is deleted inside oauthDialog->exec()
    //     (:492). RED = heap-use-after-free on the post-exec member touches
    //     (token->setText / wizard->cloudService / wizard->raise(), :493-505).
    //     GREEN = the QPointer<AddAuth> self-bail returns before any of them;
    //     observable as the service message staying uncleared.
    // (b) COLLABORATOR-ALIVE CONTROL: only the Context dies inside exec().
    //     doAuth()'s post-exec block dereferences no Context (getSetting/
    //     setSetting are context-free, CloudService.h:407-408), so the
    //     self-only guard must LET the block run — this run pins that the
    //     guard has not over-blocked onto the live-wizard path.
    void t161_teardownDuringOauthExecMustNotUseAfterFree()
    {
        // ---- (a) wizard death inside oauthDialog->exec() ----
        {
            resetLifeCounters();
            Context* ctx = new Context;
            Athlete athlete;
            athlete.cyclist = QStringLiteral("tester");
            ctx->athlete = &athlete;
            OAuthCapableService* svc = new OAuthCapableService;
            svc->settings.insert(CloudService::OAuthToken, QStringLiteral("local_oauth_token"));
            svc->message = QStringLiteral("oauth-msg");

            AddCloudWizard* wizard = new AddCloudWizard(ctx);
            wizard->cloudService = svc;
            QPointer<AddCloudWizard> wizardGuard(wizard);
            QPointer<Context> ctxGuard(ctx);

            auto* page20 = static_cast<AddAuth*>(wizard->page(20));
            QVERIFY2(page20 != nullptr, "wizard must register AddAuth as page 20");

            // Fires inside oauthDialog->exec()'s nested loop: deliver the
            // teardown, then end the loop as the user finishing/abandoning
            // the browser OAuth would.
            QTimer::singleShot(0, qApp, [wizard]() {
                delete wizard; // the teardown under test — page 20 dies with it
                if (g_oauthLoop)
                    g_oauthLoop->quit();
            });

            page20->doAuth(); // RED: use-after-free resumes here

            // PREMISE (LSN-047/050): the teardown must have actually landed,
            // and the OAuth branch must actually have run — otherwise this
            // run passed vacuously.
            QVERIFY2(wizardGuard.isNull(), "premise: the wizard teardown landed");
            QVERIFY2(ctxGuard.isNull() == false, "premise: the Context outlived (a)'s wizard-only teardown");
            QCOMPARE(g_oauthDialogsConstructed, 1);

            // GREEN observable: the bail fired BEFORE the message block, so
            // the (test-owned, wizard-unrelated) service kept its message.
            QCOMPARE(svc->message, QStringLiteral("oauth-msg"));

            delete svc;
            delete ctx;
        }

        // ---- (b) Context death inside oauthDialog->exec(), wizard alive ----
        {
            resetLifeCounters();
            Context* ctx = new Context;
            Athlete athlete;
            athlete.cyclist = QStringLiteral("tester");
            ctx->athlete = &athlete;
            OAuthCapableService* svc = new OAuthCapableService; // outlives the run
            svc->settings.insert(CloudService::OAuthToken, QStringLiteral("local_oauth_token"));
            svc->message = QStringLiteral("oauth-msg");

            AddCloudWizard* wizard = new AddCloudWizard(ctx);
            wizard->cloudService = svc;
            QPointer<AddCloudWizard> wizardGuard(wizard);
            QPointer<Context> ctxGuard(ctx);

            auto* page20 = static_cast<AddAuth*>(wizard->page(20));
            QVERIFY(page20 != nullptr);

            QTimer::singleShot(0, qApp, [ctx]() {
                delete ctx; // the athlete tab closed mid-OAuth
                if (g_oauthLoop)
                    g_oauthLoop->quit();
            });

            page20->doAuth();

            QVERIFY2(ctxGuard.isNull(), "premise: the Context teardown landed");
            QVERIFY2(!wizardGuard.isNull(), "premise: the window-hosted NON-MODAL wizard survived the tab close");
            QCOMPARE(g_oauthDialogsConstructed, 1);
            // The self-only guard deliberately does NOT block this run: the
            // post-exec block touches no Context, so it completes.
            QCOMPARE(svc->message, QString());

            delete svc;
            delete wizard; // ctx already gone
        }

        // ---- (c) Context ALREADY dead when doAuth() is entered ----
        //
        // The ENTRY hazard (follow-up to the build report's NOTES #3): the
        // wizard is NON-MODAL and survives the tab close that frees the
        // Context, so doAuth() can be ENTERED with wizard->context dangling
        // and would hand that pointer to `new OAuthDialog(wizard->context,
        // ...)` — a dialog that derefs it inside its own exec(). The frame
        // must bail BEFORE constructing the dialog. The quit timer only
        // matters in RED (where the dialog's loop is reached); under the
        // guard it fires later as a harmless no-op.
        {
            resetLifeCounters();
            Context* ctx = new Context;
            Athlete athlete;
            athlete.cyclist = QStringLiteral("tester");
            ctx->athlete = &athlete;
            OAuthCapableService* svc = new OAuthCapableService;
            svc->settings.insert(CloudService::OAuthToken, QStringLiteral("local_oauth_token"));
            svc->message = QStringLiteral("oauth-msg");

            AddCloudWizard* wizard = new AddCloudWizard(ctx);
            wizard->cloudService = svc;
            QPointer<AddCloudWizard> wizardGuard(wizard);
            QPointer<Context> ctxGuard(ctx);

            auto* page20 = static_cast<AddAuth*>(wizard->page(20));
            QVERIFY(page20 != nullptr);

            delete ctx; // the tab closed BEFORE the user hit Authorise

            QVERIFY2(ctxGuard.isNull(), "premise: the Context is dead at doAuth() entry");
            QVERIFY2(!wizardGuard.isNull(), "premise: the window-hosted NON-MODAL wizard survived the tab close");

            QTimer::singleShot(0, qApp, []() {
                if (g_oauthLoop)
                    g_oauthLoop->quit(); // RED only: bail if the loop was reached
            });
            page20->doAuth(); // must bail before constructing the OAuthDialog

            QCOMPARE(g_oauthDialogsConstructed, 0);              // RED: 1 — dead pointer handed over
            QCOMPARE(svc->message, QStringLiteral("oauth-msg")); // post-exec body never ran

            delete svc;
            delete wizard;
        }
    }

    // --- T-162 — the WIZARD-ITSELF half of S-R021-03 -----------------------
    //
    // (a) wizard death inside the REAL QMessageBox err.exec() (:797): RED =
    //     use-after-free constructing the folder picker with the freed page
    //     as parent. GREEN = the post-exec self-bail returns first.
    // (b) Context death inside dialog.exec() (:802), wizard alive: the
    //     folder-browse frame itself dereferences no Context, but the flow
    //     must not carry on into it — and AddFinish::validatePage() must
    //     then refuse the :887 chain (wizard->context->athlete->cyclist)
    //     and the :892 sync-dialog construction. RED = folderSelected runs;
    //     mutation M3 (guard removed) = use-after-free at :887.
    void t162_wizardBlockingExecTeardownAndFinishChainBail()
    {
        // ---- (a) wizard death inside err.exec() ----
        {
            resetLifeCounters();
            g_openFails = true; // drive browseFolder into the QMessageBox branch

            Context* ctx = new Context;
            Athlete athlete;
            athlete.cyclist = QStringLiteral("tester");
            ctx->athlete = &athlete;
            CloudService* svc = new CloudService;

            AddCloudWizard* wizard = new AddCloudWizard(ctx);
            wizard->cloudService = svc;
            QPointer<AddCloudWizard> wizardGuard(wizard);
            QPointer<Context> ctxGuard(ctx);

            auto* page30 = static_cast<AddSettings*>(wizard->page(30));
            QVERIFY2(page30 != nullptr, "wizard must register AddSettings as page 30");

            // Fires inside the REAL QMessageBox::exec() loop.
            QTimer::singleShot(0, qApp, [wizard]() {
                delete wizard; // the teardown under test
                dismissActiveModalMessageBox();
            });

            page30->browseFolder(); // RED: use-after-free resumes here

            QVERIFY2(wizardGuard.isNull(), "premise: the wizard teardown landed");
            QVERIFY2(!ctxGuard.isNull(), "premise: only the wizard died in (a)");
            // GREEN observable: bailed BEFORE the folder picker was built on
            // the freed page.
            QCOMPARE(g_folderSelectedCalls, 0);

            delete svc;
            delete ctx;
        }

        // ---- (b) Context death inside dialog.exec(), then the :887 chain ----
        {
            resetLifeCounters();
            g_folderExecRunsLoop = true; // park the folder picker in a loop
            g_scriptedFolderDialogResult = QDialog::Accepted;

            Context* ctx = new Context;
            Athlete athlete;
            athlete.cyclist = QStringLiteral("tester");
            ctx->athlete = &athlete;
            CloudService* svc = new CloudService;

            AddCloudWizard* wizard = new AddCloudWizard(ctx, QString(), true /*fsync*/);
            wizard->cloudService = svc;
            QPointer<AddCloudWizard> wizardGuard(wizard);
            QPointer<Context> ctxGuard(ctx);

            auto* page30 = static_cast<AddSettings*>(wizard->page(30));
            QVERIFY(page30 != nullptr);
            auto* page90 = static_cast<AddFinish*>(wizard->page(90));
            QVERIFY2(page90 != nullptr, "wizard must register AddFinish as page 90");

            // Fires inside CloudServiceDialog::exec()'s nested loop: the
            // athlete tab closes; the folder picker then "accepts" so the
            // post-exec code would run if not bailed.
            QTimer::singleShot(0, qApp, [ctx]() {
                delete ctx; // the teardown under test — the wizard survives
                if (g_folderLoop)
                    g_folderLoop->exit(QDialog::Accepted);
            });

            page30->browseFolder(); // must bail after dialog.exec()

            QVERIFY2(ctxGuard.isNull(), "premise: the Context teardown landed");
            QVERIFY2(!wizardGuard.isNull(), "premise: the window-hosted NON-MODAL wizard survived the tab close");
            QCOMPARE(g_folderSelectedCalls, 0); // RED: the picker result was carried on

            // The :887 chain and the :892 construction must both be refused.
            QCOMPARE(page90->validatePage(), false); // M3 RED: use-after-free at :887
            QCOMPARE(g_saveSettingsCalls, 0);
            QCOMPARE(g_setCValueCalls, 0);
            QCOMPARE(g_syncDialogsConstructed, 0);

            // validatePage() bailed BEFORE its trailing `delete
            // wizard->cloudService`, so the service is still the test's.
            delete svc;
            delete wizard; // ctx already gone
        }
    }

    // --- T-163 — positive control: nothing dies, everything completes ------
    //
    // Pins that the guards do not over-block: with NO teardown anywhere, the
    // OAuth post-exec body runs (message cleared), the folder pick is applied,
    // and validatePage() saves, walks the :887 chain for real and constructs
    // the :892 sync dialog.
    void t163_noTeardownNormalPathStillCompletes()
    {
        resetLifeCounters();
        g_folderExecRunsLoop = true;
        g_scriptedFolderDialogResult = QDialog::Accepted;
        g_scriptedFolderPath = QStringLiteral("/chosen");

        Context* ctx = new Context;
        Athlete athlete;
        athlete.cyclist = QStringLiteral("tester");
        ctx->athlete = &athlete;
        OAuthCapableService* svc = new OAuthCapableService;
        svc->settings.insert(CloudService::OAuthToken, QStringLiteral("local_oauth_token"));
        svc->message = QStringLiteral("oauth-msg");

        AddCloudWizard* wizard = new AddCloudWizard(ctx, QString(), true /*fsync*/);
        wizard->cloudService = svc;
        QPointer<AddCloudWizard> wizardGuard(wizard);
        QPointer<Context> ctxGuard(ctx);

        auto* page20 = static_cast<AddAuth*>(wizard->page(20));
        QVERIFY(page20 != nullptr);
        auto* page30 = static_cast<AddSettings*>(wizard->page(30));
        QVERIFY(page30 != nullptr);
        auto* page90 = static_cast<AddFinish*>(wizard->page(90));
        QVERIFY(page90 != nullptr);

        QTimer::singleShot(0, qApp, []() {
            if (g_oauthLoop)
                g_oauthLoop->quit(); // the user completes OAuth
        });
        page20->doAuth();
        QCOMPARE(g_oauthDialogsConstructed, 1);
        QCOMPARE(svc->message, QString()); // post-exec body ran

        QTimer::singleShot(0, qApp, []() {
            if (g_folderLoop)
                g_folderLoop->exit(QDialog::Accepted);
        });
        page30->browseFolder();
        QCOMPARE(g_folderSelectedCalls, 1); // the picked folder was applied

        QCOMPARE(page90->validatePage(), true);
        QCOMPARE(g_saveSettingsCalls, 1);
        QCOMPARE(g_setCValueCalls, 1); // the :887 chain ran with a live Context
        QCOMPARE(g_lastCValueCyclist, QStringLiteral("tester"));
        QCOMPARE(g_syncDialogsConstructed, 1); // :892 ran (fsync)

        QVERIFY2(!wizardGuard.isNull() && !ctxGuard.isNull(), "premise: nothing was torn down in the positive control");

        // validatePage() took ownership of the service (its trailing delete);
        // the wizard itself is still the test's.
        delete wizard;
        delete ctx;
    }
};

QTEST_MAIN(TestGarminConnectWizardLifetime)
#include "testGarminConnectWizardLifetime.moc"
