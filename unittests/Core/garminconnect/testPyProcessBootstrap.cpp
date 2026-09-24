/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DEC-052 — B-STAGE9-01 fix: PyProcessBootstrap (shared, process-level
// CPython bootstrap). This executable links a real CPython (Python3::Python)
// and therefore carries the `garmin-py` CTest label — it must NEVER carry
// `garmin-fast` (that label is Python-free by invariant).
//
// Reproduces the ACTUAL production defect (B-STAGE9-01): with GC_WANT_PYTHON
// off, nothing in the tree used to call Py_Initialize() at all, so
// Cloud/PyEmbeddedAdapter.cpp's (DES-013) Py_IsInitialized() fail-safe was
// permanently true-for-"down" and every real Garmin auth attempt failed with
// the generic "Connection to Garmin Connect failed (code: unknown)",
// regardless of credentials/network/installed packages. This test brings the
// interpreter up via ONLY PyProcessBootstrap::ensureInitialized() — the same
// call main.cpp now makes — with NO preInitHook and NO PythonEmbed involved
// anywhere, i.e. the exact Garmin-ON/Python-OFF configuration that shipped
// broken, and proves PyEmbeddedAdapter now proceeds past its fail-safe.
//
// Coverage (DEC-052 builder-owned test plan, minimum bar):
//   1. Garmin-ON / Python-OFF: interpreter comes up via ensureInitialized()
//      alone, with NO test-side Py_Initialize() call (unlike
//      testGarminConnectPyAdapter.cpp's initTestCase(), which deliberately
//      still calls Py_Initialize() itself to test DES-013's OLD, narrower,
//      still-valid pre-init fail-safe contract — untouched by this file).
//   3. Concurrent first use: several threads race into ensureInitialized()
//      as the very first callers in the process; all must observe success
//      and the interpreter must come up exactly once (no crash/hang from a
//      double Py_InitializeFromConfig or an unbalanced PyEval_SaveThread()).
//   4. Token restoration (loadTokens()) as the very first Garmin op on a
//      fresh adapter — no prior authenticate() call — still works once the
//      interpreter is up.
//   5. Worker-teardown/recreation-shaped churn (construct/authenticate/
//      destroy a PyEmbeddedAdapter repeatedly, across real OS threads)
//      doesn't strand interpreter state — the shared bootstrap never
//      finalizes (no Py_FinalizeEx anywhere in PyProcessBootstrap), so there
//      is structurally no teardown path for GarminAuthChain/
//      GarminDownloadChain's QThread::terminate() fallback to race.
//   6. (Negative space) testGarminConnectPyAdapter.cpp is NOT modified by
//      this change and is exercised separately by CTest — its initTestCase()
//      fail-safe assertions keep passing unchanged.
//
// Cites:
//   DEC-052 — this bootstrap's accepted design
//   DES-013 — PyEmbeddedAdapter's Py_IsInitialized() fail-safe (unchanged)
//   LSN-007 — Python.h must precede any Qt header

// clang-format off
#include <Python.h>
// clang-format on

#include "IGarminPyAdapter.h"
#include "PyEmbeddedAdapter.h"
#include "PyProcessBootstrap.h"

#include <QCoreApplication>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QtTest/QtTest>

#include <cstring>
#include <thread>
#include <vector>

#ifndef GARMIN_PYSTUBS_DIR
#    error "GARMIN_PYSTUBS_DIR must be defined by the CMake target (path to pystubs/)"
#endif

namespace {

void runPy(const char* code)
{
    PyGILState_STATE st = PyGILState_Ensure();
    PyRun_SimpleString(code);
    PyGILState_Release(st);
}

const QString kStubsDir = QString::fromUtf8(GARMIN_PYSTUBS_DIR);

// DEC-066 / B-STAGE9-70: mirrors testGarminConnectPyAdapter.cpp:104-114.
void setScenario(const char* scenario)
{
    PyEmbeddedAdapter bootstrap(GarminPyModulePath::explicitOverride(kStubsDir));
    bootstrap.authenticate(QStringLiteral("bootstrap@example.com"), QStringLiteral("bootstrap"));

    const QString code = QStringLiteral("import gc_garmin_adapter.garmin_client\n"
                                        "gc_garmin_adapter.garmin_client.SCENARIO = '%1'\n")
                             .arg(QString::fromUtf8(scenario));
    runPy(code.toUtf8().constData());
}

// B-STAGE9-05 — re-exec escape hatch. externallyInitializedGilIsNotStolen()
// below needs a process where Py_IsInitialized() AND ensureInitialized()'s
// own internal "first call ever" state are BOTH still untouched, to exercise
// the already-initialized-externally branch as the process's first-ever
// ensureInitialized() call — impossible inside THIS process, since
// initTestCase() above already consumes that one first-call slot (its
// racers go through the real-init branch, with Py_IsInitialized() false).
// Mirrors testCloudProviderWatchdog.cpp's own child-mode re-exec launcher,
// pared down to the one scenario this file needs.
const char* kGilOwnershipChildFlag = "--gil-ownership-child";
const char* kGilOwnershipResultPrefix = "GIL_OWNERSHIP_CHILD_RESULT=";

// Runs ONLY in the re-exec'd child process, before QTest or any other test
// state exists. Simulates "Python already initialized by something outside
// this module" (Py_Initialize()) immediately followed by a caller thread
// holding the GIL for its OWN, unrelated reasons (PyGILState_Ensure()) at the
// exact moment ensureInitialized() is first called — the B-STAGE9-05
// scenario. Asserts the GIL is still held afterward (not stolen), then
// releases it itself, exactly as the real caller that acquired it would.
int runGilOwnershipChild()
{
    Py_Initialize(); // the "something else already brought it up" half

    PyGILState_STATE st = PyGILState_Ensure(); // the caller's own, unrelated scope
    const PyProcessBootstrap::Result result = PyProcessBootstrap::ensureInitialized();
    const bool stillHeld = PyGILState_Check();

    fprintf(stderr, "%s%s\n", kGilOwnershipResultPrefix, (result.ok && stillHeld) ? "held" : "stolen-or-failed");
    fflush(stderr);

    PyGILState_Release(st); // ours to release — the same scope that acquired it
    return (result.ok && stillHeld) ? 0 : 1;
}

// DEC-061 (B-STAGE9-40) — same re-exec escape hatch as
// runGilOwnershipChild(): ensureInitialized() caches its first-ever result
// for the rest of any process, and initTestCase() already forced a SUCCESS
// into that cache for THIS binary's normal test process, so a real failure
// can only be observed in a fresh one. The parent sets PYTHONHOME to a
// nonexistent directory on the CHILD's environment only; MEASURED (a
// standalone probe against this build's CPython 3.13) to make
// Py_InitializeFromConfig() return a PyStatus_Exception ("Failed to import
// encodings module") rather than crash, hang, or call Py_ExitStatusException
// itself — CPython's own documented non-aborting failure path, which
// PyProcessBootstrap.cpp already handles without invoking that call.
const char* kBootstrapFailureChildFlag = "--bootstrap-failure-child";
const char* kBootstrapFailureResultPrefix = "BOOTSTRAP_FAILURE_CHILD_RESULT=";

int runBootstrapFailureChild()
{
    const PyProcessBootstrap::Result result = PyProcessBootstrap::ensureInitialized();
    const bool cleanFailure = !result.ok && !result.error.isEmpty() && !PyProcessBootstrap::isInitialized();

    fprintf(stderr, "%s%s\n", kBootstrapFailureResultPrefix, cleanFailure ? "failed-as-expected" : "unexpected");
    fflush(stderr);

    return cleanFailure ? 0 : 1;
}

} // namespace

class TestPyProcessBootstrap : public QObject
{
    Q_OBJECT

  private slots:

    // Spec step 1 (DES-013, re-verified against THIS module): before
    // ensureInitialized() has ever been called, the interpreter must be down
    // and PyEmbeddedAdapter must fail safe. Then races several threads into
    // ensureInitialized() as the very first callers in the process (test #3)
    // with a default Config — no preInitHook, mirroring the real
    // Garmin-ON/Python-OFF production configuration where no PythonEmbed
    // ever runs. Runs first because QTest guarantees initTestCase() precedes
    // every other slot, and this assertion is only true pre-init.
    void initTestCase()
    {
        QVERIFY2(!Py_IsInitialized(), "harness precondition: interpreter must not be up yet");

        PyEmbeddedAdapter early(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = early.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.rawMessage, QStringLiteral("embedded Python unavailable"));

        // Test #3 — concurrent first use. Every thread calls the SAME
        // no-preInitHook Config PyProcessBootstrap sees in the real
        // Garmin-ON/Python-OFF app path (main.cpp's own call site). Results
        // are written to disjoint vector slots (no lock needed) so this
        // thread can assert every racer observed success afterward.
        constexpr int kRacers = 8;
        std::vector<PyProcessBootstrap::Result> results(kRacers);
        std::vector<std::thread> racers;
        racers.reserve(kRacers);
        for (int i = 0; i < kRacers; ++i) {
            racers.emplace_back([&results, i] { results[i] = PyProcessBootstrap::ensureInitialized(); });
        }
        for (auto& t : racers)
            t.join();

        QVERIFY2(Py_IsInitialized(), "ensureInitialized() must bring the interpreter up with NO preInitHook");
        for (int i = 0; i < kRacers; ++i) {
            QVERIFY2(results[i].ok, qPrintable(QStringLiteral("racer %1 must observe success").arg(i)));
            QVERIFY(results[i].error.isEmpty());
        }
    }

    // Test #1 (core regression test for B-STAGE9-01) — the exact
    // Garmin-ON/Python-OFF production configuration: no PythonEmbed, no
    // test-side Py_Initialize(), ONLY PyProcessBootstrap::ensureInitialized()
    // (already called in initTestCase(), racing 8 ways). authenticate() must
    // now proceed past DES-013's fail-safe and succeed.
    void garminOnlyBootstrapUnblocksAdapter()
    {
        QVERIFY(Py_IsInitialized());

        setScenario("success");
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.garmin_user_id, QStringLiteral("uid-123"));
        QCOMPARE(out.display_name, QStringLiteral("Alice Rider"));
    }

    // Test #4 — token restoration as the FIRST op on a fresh adapter, no
    // prior authenticate() call (REQ-005/REQ-NF-Compat-001(b) session-restore
    // path — the app's actual "reopen with a saved session" startup case).
    void tokenRestorationAsFirstOpSucceeds()
    {
        QVERIFY(Py_IsInitialized());

        setScenario("success"); // from_tokens() only special-cases load_* scenarios
        PyEmbeddedAdapter freshAdapter(
            GarminPyModulePath::explicitOverride(kStubsDir)); // never had authenticate() called
        const PyLoadTokensOutcome out = freshAdapter.loadTokens(QStringLiteral("{\"oauth1\":\"x\"}"));

        QCOMPARE(out.kind, PyLoadTokensOutcome::Success);
    }

    // Test #5 — worker-teardown/recreation-shaped churn. Sequential (not
    // concurrent, so garmin_client.SCENARIO stays deterministic) construct /
    // authenticate / destroy cycles across real OS threads, each mirroring
    // one GarminAuthChain lifetime. The shared bootstrap never finalizes the
    // interpreter (no Py_FinalizeEx anywhere in PyProcessBootstrap) — this
    // asserts that invariant holds under churn: Py_IsInitialized() must stay
    // true across and after every cycle, and no cycle may crash/hang.
    void workerTeardownRecreationDoesNotStrandInterpreter()
    {
        for (int cycle = 0; cycle < 4; ++cycle) {
            // QCOMPARE/QVERIFY are only safe to call from the test's own
            // thread (QTest internal state is not thread-safe — matches
            // testGarminConnectPyAdapter.cpp's workerThreadCallReturnsSameSuccess()
            // convention of asserting AFTER join(), never inside the thread
            // lambda), so the outcome is captured and checked out here.
            PyAuthOutcome out;
            std::thread worker([cycle, &out] {
                setScenario("success");
                PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
                out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw-%1").arg(cycle));
                // adapter destructs here (end of thread lambda scope), on
                // this worker thread — mirrors GarminAuthChain destroying its
                // GarminWorker (which owns the adapter) after quit()+wait()
                // or the QThread::terminate() fallback.
            });
            worker.join();

            QCOMPARE(out.kind, PyAuthOutcome::Success);
            QVERIFY2(Py_IsInitialized(),
                     qPrintable(QStringLiteral("interpreter must survive worker cycle %1").arg(cycle)));
        }

        // Interpreter still usable after all the churn above — not merely
        // "flagged initialized" but actually functional.
        setScenario("success");
        PyEmbeddedAdapter finalAdapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = finalAdapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Success);
    }

    // B-STAGE9-05 — regression for the GIL-ownership bug the reviewer found
    // in ensureInitialized()'s "already initialized externally" branch: it
    // detected whether the calling thread held the GIL via PyGILState_Check()
    // alone — which only reports THAT, never WHY — and then unconditionally
    // called PyEval_SaveThread() whenever it did, stealing a GIL scope that
    // might belong to a completely unrelated caller (e.g. some other code
    // already inside its own PyGILState_Ensure() scope for unrelated work
    // when Python happened to already be up and this was still the first
    // ensureInitialized() call in the process). Re-execs this same binary
    // with a special flag (runGilOwnershipChild() above) that skips QTest
    // entirely and runs just that one fresh-process scenario.
    void externallyInitializedGilIsNotStolen()
    {
        QProcess child;
        child.setProgram(QCoreApplication::applicationFilePath());
        child.setArguments(QStringList() << QString::fromLatin1(kGilOwnershipChildFlag));
        child.start();

        QVERIFY2(child.waitForStarted(10000), "child process failed to start");
        QVERIFY2(child.waitForFinished(10000), "child process timed out");

        const QString err = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(err.contains(QString::fromLatin1(kGilOwnershipResultPrefix) + QStringLiteral("held")),
                 qPrintable(QStringLiteral("child did not report the GIL as still held after "
                                           "ensureInitialized(); stderr=%1")
                                .arg(err)));
        QCOMPARE(child.exitCode(), 0);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    }

    // DEC-061 (B-STAGE9-40) — PROVE IT: a failed ensureInitialized() must
    // leave isInitialized() false and Result.error non-empty — the exact
    // predicate/diagnostic pair main.cpp:552 now consumes instead of
    // discarding. main.cpp itself is the app entry point and links into no
    // CTest target (ground truth #2: no app-process smoke exists yet), so
    // this pins the contract main.cpp's fix relies on, in a fresh re-exec'd
    // process (runBootstrapFailureChild() above) with an invalid PYTHONHOME
    // on the CHILD's environment only — this process's own successful
    // bootstrap (initTestCase()) is untouched.
    void bootstrapFailureLeavesUninitializedWithADiagnostic()
    {
        QProcess child;
        child.setProgram(QCoreApplication::applicationFilePath());
        child.setArguments(QStringList() << QString::fromLatin1(kBootstrapFailureChildFlag));
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("PYTHONHOME"), QStringLiteral("/definitely/does/not/exist-gc-test"));
        child.setProcessEnvironment(env);
        child.start();

        QVERIFY2(child.waitForStarted(10000), "child process failed to start");
        QVERIFY2(child.waitForFinished(10000), "child process timed out");

        const QString err = QString::fromUtf8(child.readAllStandardError());
        QVERIFY2(
            err.contains(QString::fromLatin1(kBootstrapFailureResultPrefix) + QStringLiteral("failed-as-expected")),
            qPrintable(QStringLiteral("child did not report a clean bootstrap failure; stderr=%1").arg(err)));
        QCOMPARE(child.exitCode(), 0);
        QCOMPARE(child.exitStatus(), QProcess::NormalExit);
    }
};

// Custom main (replaces QTEST_APPLESS_MAIN) so the B-STAGE9-05 and DEC-061
// child-mode flags can be intercepted and dispatched BEFORE QCoreApplication/
// QTest ever see them or touch Python — see runGilOwnershipChild() and
// runBootstrapFailureChild() above.
int main(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], kGilOwnershipChildFlag) == 0)
            return runGilOwnershipChild();
        if (std::strcmp(argv[i], kBootstrapFailureChildFlag) == 0)
            return runBootstrapFailureChild();
    }

    QCoreApplication app(argc, argv); // gives applicationFilePath() a real path to report
    TestPyProcessBootstrap tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
#include "testPyProcessBootstrap.moc"
