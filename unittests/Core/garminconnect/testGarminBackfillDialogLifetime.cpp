/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-194/T-195 (informal, orchestrator assigns real TEST-NNN) — B-R010-06
// residual: GarminBackfillDialog::context must be tracked CONTINUOUSLY (a
// QPointer<Context> member from construction) rather than re-wrapped locally
// inside individual methods. A QPointer freshly built from an already-
// dangling raw pointer does not retroactively detect that the object is
// gone — only a QPointer that was watching BEFORE destruction auto-nulls.
//
// The REAL src/Cloud/GarminBackfillDialog.cpp is compiled into this
// executable via the force-included stubs/BackfillDialogLifetimeStubPreamble.h
// (sibling of testGarminConnectWizardLifetime's own preamble). The REAL
// src/Cloud/GarminBackfillController.{h,cpp} is compiled too, against a
// Python-free IGarminDownloadClient fake (same shape as
// testGarminBackfillController.cpp's FakeBackfillClient), so the controller's
// own state machine runs for real.
//
// The pre-fix code constructs a fresh QPointer<Context> from the (by then
// already-freed) raw `context` member unconditionally inside startClicked(),
// regardless of which branch runs afterwards. That construction dereferences
// freed memory — genuinely undefined behaviour — but is NOT guaranteed to
// crash: empirically (this file's own RED run) the freed Context's chunk
// gets recycled by ordinary heap churn (QString allocations, etc.) before
// the dangling QPointer is built, so no exception is thrown; the pointer is
// silently treated as live instead, and `store` gets touched a second time —
// exactly the "gets silently propagated as if valid" hazard B-R010-06
// describes. RED is therefore captured as a deterministic QCOMPARE mismatch
// (see T-195), not an ASan abort. AddressSanitizer is still required here as
// defense-in-depth (a different allocator/compiler could turn the same UB
// into a hard crash instead), matching this suite's other lifetime targets.

#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
// instrumented — ok
#else
#    error "testGarminBackfillDialogLifetime (T-194/195) is a lifetime test and requires AddressSanitizer"
#endif

#include "GarminBackfillDialog.h"

#include <QDateTime>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <functional>

namespace {
const QString kUid = QStringLiteral("555000111");

// ---------------------------------------------------------------------------
// FakeBackfillClient — same-thread Python-free IGarminDownloadClient, scripted
// with an empty listing so every start() run this file drives completes as
// Outcome::Done with no staged files (RideImportWizard is never constructed;
// see the preamble comment on why that stand-in exists only to compile
// against). Mirrors testGarminBackfillController.cpp's fake of the same name.
// ---------------------------------------------------------------------------
class FakeBackfillClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString& sinceGmt, QUuid id) override
    {
        ++listCalls;
        Q_UNUSED(sinceGmt);
        QMetaObject::invokeMethod(
            this, [this, id]() { emit activitiesListed(id, QVector<GarminActivitySummary>()); }, Qt::QueuedConnection);
    }

    void downloadActivity(const QString&, const QString&, QUuid) override
    {
        // never reached — listActivities always reports an empty result.
    }

    int listCalls = 0;
};

// ---------------------------------------------------------------------------
// ContextDyingBackfillClient — B-R010-06 final piece (T-196): lists exactly
// one in-range activity, then simulates an ordinary athlete-tab close landing
// INSIDE the controller's nested download wait (downloadActivity() deletes
// the Context before replying) - the exact hazard the SessionCheck lambda's
// entry guard exists for.
// ---------------------------------------------------------------------------
class ContextDyingBackfillClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        GarminActivitySummary s;
        s.activityId = QStringLiteral("act1");
        s.startTimeGMT = QDateTime::currentDateTimeUtc().addDays(-10).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QMetaObject::invokeMethod(
            this, [this, id, s]() { emit activitiesListed(id, QVector<GarminActivitySummary>{s}); },
            Qt::QueuedConnection);
    }

    void downloadActivity(const QString&, const QString&, QUuid id) override
    {
        if (onDownloadActivity)
            onDownloadActivity();
        QMetaObject::invokeMethod(this, [this, id]() { emit downloaded(id, QByteArray("FIT")); }, Qt::QueuedConnection);
    }

    // Fires synchronously, before the (queued) `downloaded` reply - simulates
    // the tab close landing while this request is still in flight.
    std::function<void()> onDownloadActivity;
};

// ---------------------------------------------------------------------------
// SingleZipActivityBackfillClient — DEC-070 (T-214): lists exactly one
// in-range activity and returns ZIP-local-file-header-signed bytes on
// download, so the resulting RideImportWizard hand-off carries a real
// ".zip"-staged path the dialog must consume verbatim rather than re-derive.
// ---------------------------------------------------------------------------
class SingleZipActivityBackfillClient : public IGarminDownloadClient
{
    Q_OBJECT
  public:
    void restoreSession(const QString&, QUuid id) override
    {
        QMetaObject::invokeMethod(this, [this, id]() { emit sessionRestored(id); }, Qt::QueuedConnection);
    }

    void listActivities(const QString&, QUuid id) override
    {
        GarminActivitySummary s;
        s.activityId = QStringLiteral("act-zip-1");
        s.startTimeGMT = QDateTime::currentDateTimeUtc().addDays(-10).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        QMetaObject::invokeMethod(
            this, [this, id, s]() { emit activitiesListed(id, QVector<GarminActivitySummary>{s}); },
            Qt::QueuedConnection);
    }

    void downloadActivity(const QString&, const QString&, QUuid id) override
    {
        const QByteArray bytes = QByteArray("PK\x03\x04") + QByteArrayLiteral("-zip-payload");
        QMetaObject::invokeMethod(this, [this, id, bytes]() { emit downloaded(id, bytes); }, Qt::QueuedConnection);
    }
};

void resetCounters()
{
    g_backfillOpenCalls = 0;
    g_backfillCloseCalls = 0;
    g_backfillConfigDirCalls = 0;
    g_backfillUserIdCalls = 0;
    g_backfillClientCalls = 0;
    g_backfillSessionStillValidCalls = 0;
    g_backfillOpenSucceeds = true;
    g_rideImportWizardConstructions = 0;
    g_rideImportWizardPaths.clear();
}
} // namespace

class TestGarminBackfillDialogLifetime : public QObject
{
    Q_OBJECT

  private slots:

    // =====================================================================
    // T-194 — positive control: a single run with `context` alive throughout
    // completes normally and genuinely touches `store` (pins the
    // instrumentation itself, so T-195's "did not touch store again" is not
    // a vacuous pass against dead counters).
    // =====================================================================
    void normalRunWithContextAliveTouchesStoreAndCompletes()
    {
        resetCounters();
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        Context* ctx = new Context;
        Athlete athlete;
        ctx->athlete = &athlete;

        FakeBackfillClient client;
        GarminConnect* store = new GarminConnect;
        store->configDir = tmp.path();
        store->uid = kUid;
        store->client = &client;

        GarminBackfillDialog* dialog = new GarminBackfillDialog(ctx, store, nullptr);
        QVERIFY(dialog->start());

        dialog->startClicked();

        QCOMPARE(g_backfillConfigDirCalls, 1);
        QCOMPARE(g_backfillUserIdCalls, 1);
        QCOMPARE(g_backfillClientCalls, 1);
        QCOMPARE(client.listCalls, 1);

        delete dialog; // running == false here; ~GarminBackfillDialog closes+deletes store
        delete ctx;
    }

    // =====================================================================
    // T-195 — B-R010-06: a SECOND Start click reaching startClicked() after
    // `context` has already died (the athlete tab closed while this
    // mainWindow-parented dialog stayed open, see the class comment in
    // GarminBackfillDialog.h) must bail before touching `store` at all.
    //
    // RED (pre-fix): `context` is a raw Context* member, only re-wrapped in
    // a fresh local QPointer deep inside startClicked() — a QPointer built
    // from an already-dangling raw pointer cannot retroactively detect the
    // staleness (see the file header comment on why this surfaces as a
    // QCOMPARE mismatch here rather than an ASan abort). GREEN: the entry
    // guard (QPointer<Context> member, checked first) bails before
    // store->backfillConfigDir()/backfillGarminUserId()/backfillClient() are
    // reached a second time.
    // =====================================================================
    void secondStartClickAfterContextDiedBailsBeforeTouchingStore()
    {
        resetCounters();
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        Context* ctx = new Context;
        Athlete athlete;
        ctx->athlete = &athlete;
        QPointer<Context> ctxGuard(ctx);

        FakeBackfillClient client;
        GarminConnect* store = new GarminConnect;
        store->configDir = tmp.path();
        store->uid = kUid;
        store->client = &client;

        GarminBackfillDialog* dialog = new GarminBackfillDialog(ctx, store, nullptr);
        QVERIFY(dialog->start());

        dialog->startClicked(); // first run — completes normally, context still alive
        const int configDirCallsAfterFirstRun = g_backfillConfigDirCalls;
        const int userIdCallsAfterFirstRun = g_backfillUserIdCalls;
        const int clientCallsAfterFirstRun = g_backfillClientCalls;
        QVERIFY2(configDirCallsAfterFirstRun > 0, "premise: the first run genuinely touched store");

        delete ctx; // the athlete tab closing — `this` (mainWindow-parented) survives
        QVERIFY2(ctxGuard.isNull(), "premise: the Context teardown landed");

        dialog->startClicked(); // the second click under test — RED: ASan UAF abort here

        // GREEN observable: the guard fired before store was touched again.
        QCOMPARE(g_backfillConfigDirCalls, configDirCallsAfterFirstRun);
        QCOMPARE(g_backfillUserIdCalls, userIdCallsAfterFirstRun);
        QCOMPARE(g_backfillClientCalls, clientCallsAfterFirstRun);
        QCOMPARE(g_rideImportWizardConstructions, 0);

        delete dialog; // running == false; ~GarminBackfillDialog closes+deletes store
    }

    // =====================================================================
    // T-196 — B-R010-06 final piece: the SessionCheck lambda passed to
    // controller.start() must check the dialog's `context` FIRST and bail
    // (false, without touching `store`) if it is gone - rather than reaching
    // backfillSessionStillValid() unconditionally on every checkpoint.
    //
    // RED (pre-fix): the lambda ignores `context` entirely and always calls
    // backfillSessionStillValid() at all three checkpoints this run reaches
    // (pre-list, pre-download, post-download), so g_backfillSessionStillValidCalls
    // reaches 3. GREEN: the first two checkpoints run normally (`context` is
    // still alive), but the post-download checkpoint - reached after
    // onDownloadActivity() deleted `context` - bails via context.isNull()
    // without touching `store` again, so g_backfillSessionStillValidCalls
    // stops at 2.
    // =====================================================================
    void sessionCheckAfterContextDiedMidDownloadBailsBeforeTouchingStoreAgain()
    {
        resetCounters();
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        Context* ctx = new Context;
        Athlete athlete;
        ctx->athlete = &athlete;

        ContextDyingBackfillClient client;
        client.onDownloadActivity = [&ctx]() {
            delete ctx;
            ctx = nullptr;
        };

        GarminConnect* store = new GarminConnect;
        store->configDir = tmp.path();
        store->uid = kUid;
        store->client = &client;

        GarminBackfillDialog* dialog = new GarminBackfillDialog(ctx, store, nullptr);
        QVERIFY(dialog->start());

        dialog->startClicked(); // RED: reaches g_backfillSessionStillValidCalls == 3

        QCOMPARE(g_backfillSessionStillValidCalls, 2);

        delete dialog; // running == false; ~GarminBackfillDialog closes+deletes store
    }

    // =====================================================================
    // T-214 — DEC-070: the RideImportWizard file-list hand-off must carry the
    // controller's OWN reported staged path (here, a real ".zip"-suffixed
    // file the real GarminBackfillController actually wrote), not a path the
    // dialog re-derives itself. A dialog that goes back to guessing the
    // extension on the read side would hand the wizard a ".fit" path that
    // does not exist on disk.
    // =====================================================================
    void wizardHandoffCarriesControllerReportedPathNotAGuessedOne()
    {
        resetCounters();
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());

        Context* ctx = new Context;
        Athlete athlete;
        ctx->athlete = &athlete;

        SingleZipActivityBackfillClient client;
        GarminConnect* store = new GarminConnect;
        store->configDir = tmp.path();
        store->uid = kUid;
        store->client = &client;

        GarminBackfillDialog* dialog = new GarminBackfillDialog(ctx, store, nullptr);
        QVERIFY(dialog->start());

        dialog->startClicked();

        QCOMPARE(g_rideImportWizardConstructions, 1);
        QCOMPARE(g_rideImportWizardPaths.size(), 1);
        const QString handedOffPath = g_rideImportWizardPaths.first();
        QVERIFY2(handedOffPath.endsWith(QStringLiteral(".zip")), "the zip payload must have been staged as .zip");
        QVERIFY2(QFile(handedOffPath).exists(), "the path handed to the wizard must be the one actually written");

        delete dialog; // running == false here; ~GarminBackfillDialog closes+deletes store
        delete ctx;
    }
};

QTEST_MAIN(TestGarminBackfillDialogLifetime)
#include "testGarminBackfillDialogLifetime.moc"
