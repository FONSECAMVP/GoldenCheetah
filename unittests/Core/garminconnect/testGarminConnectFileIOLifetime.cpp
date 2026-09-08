/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST garmin: T-173/T-174/T-175 — REQ-029 (findings B-R025-01 and A3-R021b-F2,
// the same defect raised twice; one build dispositions both), the FILE-IO layer
// Context use-after-free in RideFileFactory::openRideFile.
//
// Finding text (verbatim):
//   "The file-IO layer must not dereference a `Context*` it was handed after
//    the format reader it called may have suspended on its own nested event
//    loop and let that Context be torn down. `RideFileFactory::openRideFile`
//    (`src/FileIO/RideFile.cpp:847-1002`) reads `context` again at `:999` after
//    a suspending reader call (`.fit` runs a nested `QEventLoop`,
//    `FitRideFile.cpp:172-184`); the null check there guards the pointer value,
//    never the object's lifetime."
//
// ACCEPTANCE (verbatim): "an executed ASan test proving a Context torn down
// during the reader's nested loop no longer faults at the tail, AND that the
// Athlete tag is correctly set when the Context survives."
//
// CENSUS THAT SHAPED THE MECHANISM (re-derived 2026-09-08; DEC-041's recorded
// census — "exactly two context touches, :906 and :999" — was stale, and this
// build is what corrected it). openRideFile spans :847-:1133, and it touches
// the handed-in Context FOUR times:
//
//   :906   context->athlete->home->temp()      pre-suspension (gz/zip branch), safe
//   :928   result->context = context           pointer STORE, no deref; implants
//                                              the pointer the ride will carry —
//                                              latent hazard, out of REQ-029's letter
//   :999   if (context) setTag("Athlete",
//              context->athlete->cyclist)      the named hazard — HOISTED (part 1)
//   :1055  if (context) result->
//              recalculateDerivedSeries()      second post-suspension deref, via
//                                              the ride's own context member into
//                                              context->athlete at :2545 and
//                                              context->athlete->cyclist at :2557
//                                              — cannot be captured ahead of the
//                                              reader call, so it BAILS (part 2)
//
// recalculateDerivedSeries cannot early-return out of :2545: its only gate is
// `dstale` (:2496), which is initialised true, set true by every appendPoint
// and cleared only inside recalculateDerivedSeries itself.
//
// WHAT IS REAL HERE: the REAL RideFile.cpp, FitRideFile.cpp and TcxRideFile.cpp
// are compiled in (same seam as testGarminConnectUncompressLifetime), so the
// tail under test is the shipping one and the FIT reader really enters
// loadMetadata()'s nested QEventLoop (FitRideFile.cpp:172-184), which quits on
// the reply's finished() or a 5s timer. The teardown is armed as a 0ms single
// shot BEFORE openRideFile: no loop runs between arming and that exec, so the
// nested loop is the only place it can be delivered — T-173 asserts the flag
// that proves the delivery happened, so a slot whose loop never ran fails
// loudly instead of passing vacuously.
//
// ONE SUSPENSION PER PROCESS — loadMetadata() is gated by a file-static
// `loaded` flag, so only the FIRST .fit parse in this binary suspends. T-173
// owns it (the slot that must prove no-fault-under-teardown); T-174 and T-175
// therefore run the tail on routes that do not suspend. The untested
// conjunction — suspends AND survives — is the product of the two facts T-173
// and T-174 each prove separately. Disclosed, not hidden.

#include "Athlete.h"
#include "Context.h"
#include "RideFile.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>
#include <QtTest/QtTest>

#if !defined(__SANITIZE_ADDRESS__) && !defined(GC_TEST_WITH_ASAN)
#    error \
        "T-173/T-174/T-175 are lifetime tests and are only trustworthy under AddressSanitizer; build this target with -fsanitize=address."
#endif

// The athlete name every tag assertion is pinned to. Set AFTER Athlete's ctor
// so no config read can overwrite it before the capture at factory entry.
static const QString kRiderName = QStringLiteral("FileIOLifetimeRider");

// FIT payload for the two slots that drive the suspending reader; the TCX for
// the slot that must prove the tail is unchanged on a reader with no loop.
static const QString kFitSample = QStringLiteral("/Garmin830_with_Stages.fit");
static const QString kTcxSample = QStringLiteral("/2009_09_19_09_22_42.tcx");

// DEV-ONLY sample probe (removed once the sample is settled).
static QString devSample(const char* var, const QString& fallback)
{
    const QString env = qEnvironmentVariable("GC_IO_DEV_SAMPLE");
    if (var[0] != 'F')
        return fallback;
    if (env.isEmpty())
        return fallback;
    return env.startsWith('/') ? env : QStringLiteral("/") + env;
}

class TestGarminConnectFileIOLifetime : public QObject
{
    Q_OBJECT

  private:
    // Counts the samples that carry power. The derived-pass observable is
    // RideFilePoint::np, which recalculateDerivedSeries writes ONLY where
    // watts exist and which is zero-initialised everywhere else — so the
    // "recalc did / did not run" assertions below are only meaningful for a
    // ride that really has watts. Stated as a premise, never assumed.
    static int wattsSampleCount(const RideFile* ride)
    {
        int n = 0;
        const QVector<RideFilePoint*> points = ride->dataPoints();
        for (const RideFilePoint* p : points)
            if (p->watts > 0.0)
                ++n;
        return n;
    }

    static int nonZeroNpCount(const RideFile* ride)
    {
        int n = 0;
        const QVector<RideFilePoint*> points = ride->dataPoints();
        for (const RideFilePoint* p : points)
            if (p->np != 0.0)
                ++n;
        return n;
    }

  private slots:

    // -----------------------------------------------------------------------
    // T-173 — REQ-029 core: a Context freed inside the FIT reader's nested
    // loop must not fault at EITHER tail site, and must come out with the
    // right observable for each half of the mechanism:
    //   * the Athlete tag is written from the value captured at factory entry
    //     (the context was alive then, so the name is the real one), and
    //   * the derived pass is skipped, so no dataPoint carries an np value.
    //
    // RED (both parts absent): the first link of the tag chain, the `context`
    // field read at `context->athlete`, is a heap-use-after-free compiled into
    // RideFile.cpp:999 — this target halts on the first ASan report, so the
    // abrupt end of the binary IS the red verdict.
    // -----------------------------------------------------------------------
    void fitNestedLoopTeardownMustNotFaultAtEitherTailSite()
    {
        QTemporaryDir athleteRoot;
        Context* context = new Context(nullptr);
        Athlete* athlete = new Athlete(context, QDir(athleteRoot.path()));
        athlete->cyclist = kRiderName;
        context->athlete = athlete;

        QFile sample(QStringLiteral(GC_TEST_RIDES_DIR) + devSample("FIT", kFitSample));
        QVERIFY2(sample.exists(), "sample ride missing from test/rides");

        // The teardown under test, in the production order
        // (MainWindow::removeAthleteTab: `delete athlete; delete context;`),
        // delivered from inside loadMetadata()'s nested loop.
        bool tornDownDuringOpen = false;
        QTimer::singleShot(0, this, [&]() {
            tornDownDuringOpen = true;
            delete athlete;
            delete context;
        });

        QStringList errors;
        RideFile* ride = RideFileFactory::instance().openRideFile(context, sample, errors);

        // Premise, not outcome: if no nested loop ran, this delete was
        // delivered by the test's own loop afterwards and the slot proves
        // nothing about the tail.
        QVERIFY2(tornDownDuringOpen, "the teardown never fired inside openRideFile — the FIT reader's nested loop did "
                                     "not run, so this slot exercised no suspension at all");

        QVERIFY2(ride != nullptr, qPrintable(QStringLiteral("the tail must still return the parsed ride, errors: %1")
                                                 .arg(errors.join(QStringLiteral("; ")))));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed ride must have samples");

        // Part 1 (hoist-and-capture): the tag is written from the entry capture.
        QCOMPARE(ride->getTag(QStringLiteral("Athlete"), QStringLiteral("<absent>")), kRiderName);

        // Part 2 (liveness bail): the derived pass consults a guard that the
        // teardown nulled, so it never touches the dead Context — and the
        // watts-bearing samples stay at np 0, the value they were parsed with.
        QVERIFY2(wattsSampleCount(ride) > 0,
                 "premise broken: this FIT sample carries no power samples, so the np assertion "
                 "below is vacuous — drive a FIT ride that has watts");
        QCOMPARE(nonZeroNpCount(ride), 0);

        delete ride;
    }

    // -----------------------------------------------------------------------
    // T-174 — positive control, first half of the acceptance: with a LIVE
    // Context the FIT route is unchanged — the Athlete tag is the real cyclist
    // name AND the derived pass still runs. The np assertion is what stops the
    // mechanism from over-bailing: a fix that simply skipped the recalc would
    // keep T-173 green and fail here.
    // -----------------------------------------------------------------------
    void fitLiveContextMustTagAthleteAndRecalculateDerivedSeries()
    {
        QTemporaryDir athleteRoot;
        Context context(nullptr);
        Athlete athlete(&context, QDir(athleteRoot.path()));
        athlete.cyclist = kRiderName;
        context.athlete = &athlete;

        QFile sample(QStringLiteral(GC_TEST_RIDES_DIR) + kFitSample);
        QVERIFY2(sample.exists(), "sample ride missing from test/rides");

        QStringList errors;
        RideFile* ride = RideFileFactory::instance().openRideFile(&context, sample, errors);

        QVERIFY2(ride != nullptr, qPrintable(QStringLiteral("the live-Context route must still parse the ride, "
                                                            "errors: %1")
                                                 .arg(errors.join(QStringLiteral("; ")))));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed ride must have samples");

        QCOMPARE(ride->getTag(QStringLiteral("Athlete"), QStringLiteral("<absent>")), kRiderName);

        QVERIFY2(wattsSampleCount(ride) > 0,
                 "premise broken: this FIT sample carries no power samples, so the np assertion "
                 "below cannot distinguish a recalc from a skip");
        QVERIFY2(nonZeroNpCount(ride) > 0,
                 "a live Context must still get its derived series recalculated — the mechanism "
                 "bails only on a dead one");

        delete ride;
    }

    // -----------------------------------------------------------------------
    // T-175 — selectivity control on a reader with NO nested loop at all
    // (TcxRideFile.cpp / TcxParser.cpp contain no QEventLoop): the same live
    // Context, tag and recalc both correct. Guards the reading that the bail
    // is about liveness, not about which reader ran — and pins the Athlete tag
    // on a second route, so the hoist's capture cannot be tuned to whatever
    // the FIT parser happens to leave in the tags.
    // -----------------------------------------------------------------------
    void nonSuspendingReaderLiveContextMustStillTagAndRecalculate()
    {
        QTemporaryDir athleteRoot;
        Context context(nullptr);
        Athlete athlete(&context, QDir(athleteRoot.path()));
        athlete.cyclist = kRiderName;
        context.athlete = &athlete;

        QFile sample(QStringLiteral(GC_TEST_RIDES_DIR) + kTcxSample);
        QVERIFY2(sample.exists(), "sample ride missing from test/rides");

        QStringList errors;
        RideFile* ride = RideFileFactory::instance().openRideFile(&context, sample, errors);

        QVERIFY2(ride != nullptr, qPrintable(QStringLiteral("the TCX route must still parse the ride, errors: %1")
                                                 .arg(errors.join(QStringLiteral("; ")))));
        QVERIFY2(ride->dataPoints().count() > 0, "the parsed ride must have samples");

        QCOMPARE(ride->getTag(QStringLiteral("Athlete"), QStringLiteral("<absent>")), kRiderName);

        QVERIFY2(wattsSampleCount(ride) > 0,
                 "premise broken: this TCX sample carries no power samples, so the np assertion "
                 "below cannot distinguish a recalc from a skip");
        QVERIFY2(nonZeroNpCount(ride) > 0,
                 "a live Context must still get its derived series recalculated on a reader that "
                 "never suspends");

        delete ride;
    }
};

QTEST_MAIN(TestGarminConnectFileIOLifetime)
#include "testGarminConnectFileIOLifetime.moc"
