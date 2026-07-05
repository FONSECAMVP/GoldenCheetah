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
};

QTEST_MAIN(TestGarminConnectWizardRouting)
#include "testGarminConnectWizardRouting.moc"
