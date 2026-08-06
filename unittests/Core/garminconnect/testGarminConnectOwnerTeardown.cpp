/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin:TEST-064 — REQ-017 Slice B, clauses (b) + (e): an owner destroys
// what it opens.
//
// Slice A's account-epoch invalidation (DEC-garmin-021 Option B) is deliberately
// LAZY — bump() never reaches into another live GarminConnect instance, because a
// disconnect can land inside a live download frame (GarminConnect::blockingDownload
// runs a nested QEventLoop) and tearing that instance down from the outside would
// be a use-after-free. The consequence: an IDLE sync dialog keeps its worker
// thread and its embedded-Python session alive until its OWNER lets go of it. So
// owner teardown is the guaranteed trigger for clause (b), not tidy-up.
//
// WHAT THIS TEST DRIVES, AND WHAT IT DOES NOT (read before trusting it):
//
//   * DRIVEN, for real: the production teardown contract
//     `closeAndDeleteStore()` (src/Cloud/CloudService.h). Every assertion below
//     is about what that production template does to the store it is handed —
//     close() is called, the store is destroyed AFTER that close(), the caller's
//     pointer is cleared, and a second teardown is a no-op. Nothing here asserts
//     a value the fixture just set (LSN-022).
//
//   * NOT driven, and NOT faked to look driven: the one line inside
//     `CloudServiceSyncDialog::~CloudServiceSyncDialog()` (src/Cloud/CloudService.cpp)
//     that calls it. That dialog cannot be instantiated in a `garmin-fast`
//     target — its ctor needs a real Context/Athlete/RideCache/MainWindow and
//     CloudService.cpp pulls in RideCache/JsonRideFile/CsvRideFile/DataProcessor,
//     i.e. the whole application. That was already the reason DEC-garmin-021
//     rejected Option C. The dialog dtor, MainWindow::syncCloud and
//     AddCloudWizard's finish-with-sync deletion are verified BY INSPECTION plus
//     the GoldenCheetah link, and are reported as such rather than covered by a
//     dialog-shaped fake that would prove nothing (LSN-022).
//
// The spy store below deliberately owns a REAL running QThread, because that is
// the thing clause (b) is about: GarminConnect::close() is what performs the
// bounded worker teardown (quit()+wait(), DES-001 invariant 3 — never
// terminate()), and a store that is deleted without close() leaves that worker —
// and the interpreter session it hosts — alive. So "close() was called, and it
// was called before the destructor" is a load-bearing assertion, not ceremony.

#include <QPointer>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QWidget>
#include <QtTest>

// --- minimal stand-ins for the two heavyweight GC headers CloudService.h pulls
// --- in (Context.h, Athlete.h). Guard-predefinition, same trick as
// --- stubs/WizardStubPreamble.h: a quote-include resolves the compiled file's
// --- own directory first, so co-located headers cannot be shadowed on the
// --- include path. Settings.h and Utils.h are included FOR REAL (they are pure
// --- Qt), so the only fakes here are the two the contract never touches.
#include "Utils.h" // real, pure Qt — CloudService.h sorts service names with it

#ifndef _GC_Athlete_h
#    define _GC_Athlete_h
class Athlete
{
  public:
    QString cyclist;
};
#endif

#ifndef _GC_Context_h
#    define _GC_Context_h
class RideItem;
class RideFile;
class Context
{
  public:
    QWidget* mainWindow = nullptr;
    Athlete* athlete = nullptr;
};
#endif

// THE REAL PRODUCTION HEADER — closeAndDeleteStore() comes from here, it is not
// re-declared or copied into the test.
#include "CloudService.h"

// ---------------------------------------------------------------------------
// Spy store — same teardown shape as GarminConnect: close() stops the worker
// thread and releases the "session"; the destructor records whether it ran while
// the worker was still going (that is the leak clause (b) forbids).
// ---------------------------------------------------------------------------
class SpyStore
{
  public:
    SpyStore()
    {
        worker = new QThread;
        worker->start();
        ++liveCount;
    }

    ~SpyStore()
    {
        if (worker && worker->isRunning())
            destroyedWithWorkerRunning = true;
        if (worker) {
            // do not abort the test run on the leak path
            worker->quit();
            worker->wait();
            delete worker;
            worker = NULL;
        }
        events << "delete";
        --liveCount;
    }

    // mirrors CloudService::close() — bool-returning, bounded teardown
    bool close()
    {
        events << "close";
        ++closeCalls;
        if (worker) {
            worker->quit();
            worker->wait();
        }
        sessionReleased = true;
        return true;
    }

    bool workerRunning() const { return worker && worker->isRunning(); }

    QThread* worker = NULL;
    bool sessionReleased = false;
    int closeCalls = 0;

    // shared observation points
    static QStringList events;
    static int liveCount;
    static bool destroyedWithWorkerRunning;

    static void reset()
    {
        events.clear();
        liveCount = 0;
        destroyedWithWorkerRunning = false;
    }
};

QStringList SpyStore::events;
int SpyStore::liveCount = 0;
bool SpyStore::destroyedWithWorkerRunning = false;

// ---------------------------------------------------------------------------
// Fake owner — stands in for CloudServiceSyncDialog. Its destructor body is the
// SAME one line as the production dialog destructor; the behaviour under test is
// entirely inside the production template it calls.
// ---------------------------------------------------------------------------
class FakeStoreOwner
{
  public:
    FakeStoreOwner(SpyStore* store) : store(store) {}
    ~FakeStoreOwner() { closeAndDeleteStore(store); }

    SpyStore* store;
};

class TestGarminConnectOwnerTeardown : public QObject
{
    Q_OBJECT

  private slots:

    void init() { SpyStore::reset(); }

    // TEST-064(a) — the owner tears down what it opened: close() FIRST (bounded
    // worker/session teardown), destruction SECOND. A store that is merely
    // deleted, or merely dropped, leaves the worker thread and the session
    // alive to process exit — which is exactly the REQ-017(b)/(e) leak.
    void ownerTeardownClosesStoreThenDestroysIt()
    {
        SpyStore* store = new SpyStore;
        QVERIFY2(store->workerRunning(), "fixture: the spy store must start a worker to have something to tear down");

        FakeStoreOwner* owner = new FakeStoreOwner(store);
        QCOMPARE(SpyStore::liveCount, 1);

        // the owner goes away — nothing else touches the store
        delete owner;

        // close() ran, and it ran BEFORE the destructor
        QCOMPARE(SpyStore::events, QStringList() << "close" << "delete");

        // nothing survives the owner: no store, no worker thread
        QCOMPARE(SpyStore::liveCount, 0);
        QVERIFY2(!SpyStore::destroyedWithWorkerRunning,
                 "the store was destroyed while its worker thread was still running — close() did not run first");
    }

    // TEST-064(b) — the owner's pointer is cleared, so a second teardown (dialog
    // closed, then destroyed; or an owner that both closes and is destroyed)
    // cannot double-delete. A double delete would be strictly worse than the leak
    // this slice removes.
    void secondTeardownIsANoOpAndCannotDoubleDelete()
    {
        SpyStore* store = new SpyStore;
        SpyStore* raw = store;

        closeAndDeleteStore(store);

        QVERIFY2(store == NULL, "teardown must clear the caller's pointer");
        QCOMPARE(SpyStore::events, QStringList() << "close" << "delete");
        QCOMPARE(SpyStore::liveCount, 0);
        Q_UNUSED(raw);

        // second teardown of the same owner-held pointer
        closeAndDeleteStore(store);

        QCOMPARE(SpyStore::events, QStringList() << "close" << "delete");
        QCOMPARE(SpyStore::liveCount, 0);
    }

    // TEST-064(c) — an owner that never got a store (or already gave it up)
    // tears down cleanly. CloudServiceSyncDialog's ctor has an early-return
    // failure path (store->open() failed), so its destructor must survive being
    // handed nothing.
    void teardownOfANullStoreIsSafe()
    {
        SpyStore* store = NULL;

        closeAndDeleteStore(store);

        QVERIFY(store == NULL);
        QCOMPARE(SpyStore::events, QStringList());
    }
};

QTEST_MAIN(TestGarminConnectOwnerTeardown)
#include "testGarminConnectOwnerTeardown.moc"
