/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// REQ-002 / TEST-007 / A3-R002-TR-01 — AddCloudWizard Garmin routing + lifecycle.
//
// Closes finding A3-R002-TR-01: the Garmin routing / lifecycle logic in
// src/Cloud/AddCloudWizard.{h,cpp} was compiled by zero tests. This suite
// exercises the REAL AddCloudWizard code (the .cpp is compiled straight into
// this executable via the force-included stubs/WizardStubPreamble.h, which
// short-circuits the heavyweight GC GUI headers and fakes only the DEC-013
// embedded-Python adapter — so the target stays Python-free on `garmin-fast`).
//
// The three behaviours under test (each mapped to a mutation it must kill):
//
//   1. ROUTING — AddService::nextId() and AddConsent::nextId() must return
//      page id 21 (AddGarminAuth) iff cloudService->id() == "Garmin Connect",
//      and 20 otherwise. Kills: a typo in the "Garmin Connect" string literal
//      and an inverted/removed branch.
//
//   2. ensureGarminAuthPage() IDEMPOTENCY — the `if (garminChain) return;`
//      guard. A second entry must NOT build a second GarminAuthChain /
//      PyEmbeddedAdapter / QThread nor re-register page 21. Kills: removing the
//      guard (would leak a second worker stack).
//
//   3. DESTRUCTOR TEARDOWN ORDER — ~AddCloudWizard() must `delete garminChain`
//      (worker) BEFORE `delete garminAdapter` (DES-001a: adapter outlives the
//      worker that calls into it). Kills: swapping the two deletes.

#include "AddCloudWizard.h"
#include "GarminAuthChain.h"
#include "GarminCredentialsPage.h"
#include "GarminMfaPage.h"
#include "GarminTokenStore.h"
#include "IGarminAuthClient.h"
#include "IGarminPyAdapter.h"
#include "WorkerAuthClient.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QSignalSpy>
#include <QUuid>
#include <QtTest/QtTest>

// -----------------------------------------------------------------------------
// Minimal athlete/context fixture. Context/Athlete/AthleteDirectoryStructure are
// the stubs from WizardStubPreamble.h; the wizard only reads mainWindow (the
// QWizard parent, left null → top-level) and, on the Garmin path,
// athlete->home->config() for the token-store path string.
// -----------------------------------------------------------------------------
namespace {
struct WizardFixture
{
    AthleteDirectoryStructure home;
    Athlete athlete;
    Context ctx;

    WizardFixture()
    {
        athlete.cyclist = QStringLiteral("tester");
        athlete.home = &home;
        ctx.athlete = &athlete;
        ctx.mainWindow = nullptr;
    }
};
} // namespace

class TestGarminConnectWizardRouting : public QObject
{
    Q_OBJECT

  private slots:

    // --- Behaviour 1: routing --------------------------------------------

    // AddService::nextId() (AddCloudWizard.cpp ~L272-287) routes the Garmin
    // tile to page 21, every other service to 20, and loops to 10 when no
    // service is selected yet.
    void addServiceRoutesGarminToNativeAuthPage()
    {
        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        AddService* page = static_cast<AddService*>(wizard.page(10));
        QVERIFY2(page != nullptr, "wizard must register AddService as page 10");

        CloudService service;
        wizard.cloudService = &service;

        service.serviceId = QStringLiteral("Garmin Connect");
        QCOMPARE(page->nextId(), 21);

        // Exact-literal / inverted-branch kill: anything but "Garmin Connect"
        // must fall through to the generic auth page 20.
        service.serviceId = QStringLiteral("Strava");
        QCOMPARE(page->nextId(), 20);

        service.serviceId = QStringLiteral("Garmin"); // near-miss literal
        QCOMPARE(page->nextId(), 20);

        // No service selected yet — the page loops back to the service picker.
        wizard.cloudService = nullptr;
        QCOMPARE(page->nextId(), 10);
    }

    // AddConsent::nextId() (AddCloudWizard.cpp ~L340-350) routes the Garmin
    // tile to page 21 and everything else to 20.
    void addConsentRoutesGarminToNativeAuthPage()
    {
        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        AddConsent* page = static_cast<AddConsent*>(wizard.page(15));
        QVERIFY2(page != nullptr, "wizard must register AddConsent as page 15");

        CloudService service;
        wizard.cloudService = &service;

        service.serviceId = QStringLiteral("Garmin Connect");
        QCOMPARE(page->nextId(), 21);

        service.serviceId = QStringLiteral("Strava");
        QCOMPARE(page->nextId(), 20);

        service.serviceId = QStringLiteral("Garmin"); // near-miss literal
        QCOMPARE(page->nextId(), 20);

        // Null service also keeps the historical hardcoded 20.
        wizard.cloudService = nullptr;
        QCOMPARE(page->nextId(), 20);

        // Leave nothing dangling for QWizard teardown.
        wizard.cloudService = nullptr;
    }

    // --- Behaviour 2: ensureGarminAuthPage() idempotency -----------------

    // Repeated entry to the Garmin path (Back/Next, service re-selection) must
    // reuse the same chain/adapter/page — the `if (garminChain) return;` guard.
    void ensureGarminAuthPageIsIdempotent()
    {
        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);

        wizard.ensureGarminAuthPage();
        GarminAuthChain* chain1 = wizard.garminChain;
        PyEmbeddedAdapter* adapter1 = wizard.garminAdapter;
        QWizardPage* page1 = wizard.page(21);
        QVERIFY2(chain1 != nullptr, "first entry must build the GarminAuthChain");
        QVERIFY2(adapter1 != nullptr, "first entry must build the PyEmbeddedAdapter");
        QVERIFY2(page1 != nullptr, "first entry must register page 21");

        const int liveAfterFirst = g_pyAdapterLiveCount;

        // Second entry — the guard must make this a no-op.
        wizard.ensureGarminAuthPage();

        QCOMPARE(wizard.garminChain, chain1);
        QCOMPARE(wizard.garminAdapter, adapter1);
        QCOMPARE(wizard.page(21), page1);
        // Removing the guard would construct a second adapter here.
        QCOMPARE(g_pyAdapterLiveCount, liveAfterFirst);
    }

    // --- Behaviour 3: destructor teardown order (DES-001a) ---------------

    // ~AddCloudWizard() must delete the chain (stopping the worker thread)
    // BEFORE the adapter that thread calls into. We aim the adapter's
    // QPointer<QThread> observer at the chain's worker thread: a correct
    // teardown destroys the thread first, nulling the QPointer, so the adapter
    // sees no running thread when it dies. A reversed teardown deletes the
    // adapter while the thread is still running → the flag trips.
    void destructorTearsDownChainBeforeAdapter()
    {
        g_pyAdapterDeletedWhileWorkerThreadRunning = false;

        {
            WizardFixture fx;
            AddCloudWizard* wizard = new AddCloudWizard(&fx.ctx);
            wizard->ensureGarminAuthPage();
            QVERIFY(wizard->garminChain != nullptr);
            QVERIFY(wizard->garminAdapter != nullptr);

            QThread* worker = wizard->garminChain->workerThread();
            QVERIFY(worker != nullptr);
            wizard->garminAdapter->observedThread = worker;

            // Ensure the worker thread has actually started before teardown so
            // the reversed-order mutation reliably observes it running.
            QTRY_VERIFY(worker->isRunning());

            delete wizard; // runs ~AddCloudWizard — the code under test
        }

        QVERIFY2(!g_pyAdapterDeletedWhileWorkerThreadRunning,
                 "DES-001a violated: adapter destroyed while worker thread still running "
                 "(chain must be deleted before adapter)");
        // And nothing leaked.
        QCOMPARE(g_pyAdapterLiveCount, 0);
    }

    // --- Behaviour 4: REQ-003 (MFA) Slice B routing to page 22 -----------
    //
    // T-035 — DES-003 conditional MFA page: ensureGarminAuthPage() registers the
    // MFA page as id 22 (idempotently); when the credentials page (21) receives
    // mfaRequired(id) it reports mfaPending() and AddGarminAuth::nextId() routes
    // to 22 (instead of the post-auth 25/30); the MFA page's own nextId()
    // continues the flow (hasAthlete ? 25 : 30); and the MFA page's aborted()
    // (3-strikes non-retry) is connected so it closes the wizard.
    //
    // Mutation kills:
    //   M-MFA-1  removing the `mfaPending() ? 22 :` branch → nextId stays 25/30.
    //   M-MFA-2  not registering page 22 → wizard.page(22) null.
    //   M-MFA-3  dropping the page-22 idempotency guard → a 2nd ensure re-registers.
    //   M-MFA-4  not connecting aborted() → wizard never rejects on 3-strikes.
    void garminMfaRequiredRoutesToPage22()
    {
        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        wizard.ensureGarminAuthPage();

        QWizardPage* page21 = wizard.page(21);
        QVERIFY2(page21 != nullptr, "ensureGarminAuthPage must register the credentials page as id 21");
        QWizardPage* page22 = wizard.page(22);
        QVERIFY2(page22 != nullptr, "M-MFA-2: ensureGarminAuthPage must register the MFA page as id 22");

        // Idempotency: a second ensure must NOT re-register / duplicate page 22.
        wizard.ensureGarminAuthPage();
        QCOMPARE(wizard.page(22), page22); // M-MFA-3

        // Before any MFA challenge, the credentials page routes onward (no athlete
        // configured in the fixture → 30), NOT to the MFA page.
        QVERIFY2(!static_cast<GarminCredentialsPage*>(page21)->mfaPending(),
                 "credentials page must not be MFA-pending before any challenge");
        QCOMPARE(page21->nextId(), 30);

        // Drive the credentials page to MFA-required. B-R003-03 hardening — the
        // former version dispatched a REAL authenticate (queued onto the worker
        // thread) purely to seed the page's pending id, then emitted
        // mfaRequired(thatId). That left a live queued authenticate whose eventual
        // outcome (the fake adapter returns Unknown → failed) could be delivered
        // by any later processEvents and CLOBBER the latched MfaRequired — a
        // timing-dependent fragility. Here we drive onMfaRequired deterministically
        // with NO worker dispatch: a freshly-(re)entered credentials page has a
        // null default pending id, so emitting mfaRequired(QUuid()) on the same
        // client the page is wired to matches the page's onMfaRequired guard and
        // latches MfaRequired synchronously, with no worker thread in play. This
        // still reproduces the seam's re-emit (WorkerAuthClient re-emits
        // GarminWorker::mfaRequired) and keeps every routing assertion intact.
        auto* client = static_cast<WorkerAuthClient*>(wizard.garminChain->client());
        auto* creds = static_cast<GarminCredentialsPage*>(page21);
        QVERIFY2(!creds->mfaPending(), "precondition: no MFA challenge dispatched yet");

        emit client->mfaRequired(QUuid()); // matches the page's default (null) pending id

        QVERIFY2(creds->mfaPending(), "after mfaRequired, the credentials page must report mfaPending()");
        QCOMPARE(page21->nextId(), 22); // M-MFA-1

        // The MFA page continues the flow exactly as the post-auth path does:
        // hasAthlete ? 25 : 30. The fixture configures no athlete → 30.
        QCOMPARE(page22->nextId(), 30);

        // The MFA page's 3-strikes abort must close the wizard (reject()).
        QSignalSpy rejectedSpy(&wizard, &QDialog::rejected);
        emit static_cast<GarminMfaPage*>(page22)->aborted();
        QCOMPARE(rejectedSpy.count(), 1); // M-MFA-4
    }

    // --- Behaviour 5: REQ-008 (DEC-garmin-019 C) persist trigger — T-051 -----
    //
    // The connect-success producer (GarminTokenStore::persistConnectSuccess) had
    // NO production caller before this slice (A3-R008-01 / D-R008-01), so live sync
    // no-opped. The wizard now drives persist on a SUCCESSFUL (id-gated) auth on
    // BOTH the direct path (credentials page 21) and the post-MFA path (page 22),
    // and MUST NOT persist a stale/superseded reply.
    //
    // These tests drive a REAL auth-success end-to-end through the chain worker (a
    // scriptable Python-free fake adapter -> GarminWorker -> WorkerAuthClient::
    // finished -> the page's id-gated Success -> succeeded -> the wizard's persist
    // trigger -> cloudService->persistConnectSuccess -> the REAL GarminTokenStore),
    // so the assertion is on the ACTUAL tokens.json/active-account.json written to
    // the resolved config dir (LSN-024 — verify the producer, not a fake).

    // (i) direct auth-success -> exactly one persist with the right uid+blob.
    void persistTriggerFiresOnDirectAuthSuccess()
    {
        g_persistConnectSuccessCalls = 0;
        g_scriptedAuthenticateOutcome = successOutcome(kUid, kBlob);

        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        CloudService service;
        service.context = &fx.ctx; // resolves the config dir for persist
        wizard.cloudService = &service;
        wizard.ensureGarminAuthPage();

        auto* page21 = static_cast<GarminCredentialsPage*>(wizard.page(21));
        QVERIFY(page21 != nullptr);
        driveCredentials(page21, QStringLiteral("rider@example.com"), QStringLiteral("secret"));

        const QString cfg = fx.home.config().absolutePath();
        const QString tokensPath = GarminTokenStore::tokenFilePath(cfg);
        const QString aaPath = GarminTokenStore::activeAccountFilePath(cfg);

        QTRY_VERIFY2(QFileInfo::exists(tokensPath), "direct auth-success must persist tokens.json");
        QTRY_COMPARE(g_persistConnectSuccessCalls, 1); // exactly ONE persist

        assertPersisted(tokensPath, aaPath, kUid, kBlob);
    }

    // (ii) post-MFA success (page 22) -> exactly one persist with the right uid+blob.
    void persistTriggerFiresOnPostMfaSuccess()
    {
        g_persistConnectSuccessCalls = 0;
        g_scriptedSubmitMfaOutcome = successOutcome(kUid2, kBlob2);

        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        CloudService service;
        service.context = &fx.ctx;
        wizard.cloudService = &service;
        wizard.ensureGarminAuthPage();

        auto* page22 = static_cast<GarminMfaPage*>(wizard.page(22));
        QVERIFY(page22 != nullptr);
        driveMfa(page22, QStringLiteral("123456"));

        const QString cfg = fx.home.config().absolutePath();
        const QString tokensPath = GarminTokenStore::tokenFilePath(cfg);
        const QString aaPath = GarminTokenStore::activeAccountFilePath(cfg);

        QTRY_VERIFY2(QFileInfo::exists(tokensPath), "post-MFA success must persist tokens.json");
        QTRY_COMPARE(g_persistConnectSuccessCalls, 1);

        assertPersisted(tokensPath, aaPath, kUid2, kBlob2);
    }

    // (iii) a stale/superseded finished (an abandoned earlier attempt's id, arriving
    // AFTER a fresh success) must NOT overwrite the freshly persisted token.
    void stalePersistDoesNotOverwriteFreshToken()
    {
        g_persistConnectSuccessCalls = 0;
        g_scriptedAuthenticateOutcome = successOutcome(kUid, kBlob);

        WizardFixture fx;
        AddCloudWizard wizard(&fx.ctx);
        CloudService service;
        service.context = &fx.ctx;
        wizard.cloudService = &service;
        wizard.ensureGarminAuthPage();

        auto* page21 = static_cast<GarminCredentialsPage*>(wizard.page(21));
        driveCredentials(page21, QStringLiteral("rider@example.com"), QStringLiteral("secret"));

        const QString cfg = fx.home.config().absolutePath();
        const QString tokensPath = GarminTokenStore::tokenFilePath(cfg);
        const QString aaPath = GarminTokenStore::activeAccountFilePath(cfg);
        QTRY_COMPARE(g_persistConnectSuccessCalls, 1); // fresh success persisted uid B

        // Now an abandoned earlier attempt's late reply arrives on the SAME client
        // with a DIFFERENT (superseded) uid + blob and an id matching nothing the
        // pages are still awaiting. The page id-gates (option b) discard it, so the
        // wizard's persist must NOT fire again and the fresh token must survive.
        GarminAuthSuccess stale;
        stale.garmin_user_id = QStringLiteral("999999999");
        stale.tokenBlob = QStringLiteral("{\"oauth1\":\"STALE\"}");
        auto* client = wizard.garminChain->client();
        emit client->finished(QUuid::createUuid(), stale);
        QTest::qWait(50); // give any (erroneous) delivery a chance to fire

        QCOMPARE(g_persistConnectSuccessCalls, 1);        // still exactly one — no overwrite
        assertPersisted(tokensPath, aaPath, kUid, kBlob); // fresh uid B intact
    }

  private:
    // --- T-051 helpers ---------------------------------------------------

    static constexpr const char* kUid = "123456789";
    static constexpr const char* kUid2 = "987654321";
    // Blobs deliberately WITHOUT a top-level garmin_user_id, so any reader that
    // (wrongly) parsed the blob for the uid would resolve empty — the uid must come
    // from the GarminAuthSuccess payload, not the blob.
    static constexpr const char* kBlob = "{\"oauth1\":\"OA1-secret\",\"oauth2\":\"OA2.refresh\"}";
    static constexpr const char* kBlob2 = "{\"oauth1\":\"OA1-mfa\",\"oauth2\":\"OA2.mfa\"}";

    static PyAuthOutcome successOutcome(const QString& uid, const QString& blob)
    {
        PyAuthOutcome o;
        o.kind = PyAuthOutcome::Success;
        o.garmin_user_id = uid;
        o.display_name = QStringLiteral("Test Athlete");
        o.tokenBlob = blob;
        return o;
    }

    static void driveCredentials(GarminCredentialsPage* page, const QString& email, const QString& pw)
    {
        auto* emailEdit = page->findChild<QLineEdit*>(QStringLiteral("garminEmail"));
        auto* passEdit = page->findChild<QLineEdit*>(QStringLiteral("garminPassword"));
        QVERIFY(emailEdit != nullptr);
        QVERIFY(passEdit != nullptr);
        emailEdit->setText(email);
        passEdit->setText(pw);
        page->validatePage(); // dispatches authenticate() through the chain worker
    }

    static void driveMfa(GarminMfaPage* page, const QString& code)
    {
        auto* codeEdit = page->findChild<QLineEdit*>(QStringLiteral("garminMfaCode"));
        QVERIFY(codeEdit != nullptr);
        codeEdit->setText(code);
        page->validatePage(); // dispatches submitMfa() through the chain worker
    }

    static void assertPersisted(const QString& tokensPath, const QString& aaPath, const QString& uid,
                                const QString& blob)
    {
        QFile tf(tokensPath);
        QVERIFY(tf.open(QIODevice::ReadOnly));
        QCOMPARE(tf.readAll(), blob.toUtf8());
        tf.close();

        QFile af(aaPath);
        QVERIFY(af.open(QIODevice::ReadOnly));
        const QJsonDocument doc = QJsonDocument::fromJson(af.readAll());
        af.close();
        QVERIFY(doc.isObject());
        QCOMPARE(doc.object().value(QStringLiteral("garmin_user_id")).toString(), uid);
    }
};

QTEST_MAIN(TestGarminConnectWizardRouting)
#include "testGarminConnectWizardRouting.moc"
