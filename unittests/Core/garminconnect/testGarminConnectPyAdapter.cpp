/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// TEST-005 — REQ-002 (production closure of the Authenticate slice) / DES-013:
//   PyEmbeddedAdapter — the production embedded-CPython implementation of
//   IGarminPyAdapter (DEC-013 Option A seam, production side).
//
// This executable links a real CPython (Python3::Python) and therefore
// carries the `garmin-py` CTest label — it must NEVER carry `garmin-fast`
// (that label is Python-free by invariant).
//
// Harness pattern: initTestCase() first asserts the Py_IsInitialized()==false
// fail-safe (spec step 1 of DES-013 — this MUST run before the interpreter
// exists, hence it lives in initTestCase, which QTest guarantees runs first),
// then calls Py_Initialize() followed by PyEval_SaveThread() so that
// PyGILState_Ensure() works from ANY thread afterwards. cleanupTestCase()
// restores the main thread state and finalizes.
//
// The adapter's modulePath is pointed at a scriptable stub module
// (pystubs/garmin_client.py, GARMIN_PYSTUBS_DIR compile definition). The stub
// records ctor args in module attributes and switches behavior on the
// module-level SCENARIO variable, driven from here via PyRun_SimpleString —
// deterministic, no env-var propagation issues after Py_Initialize.
//
// Marshalling matrix (DES-013 "Tests" section, guarded by LSN-006 /
// A3-R002-M6: classification is by exception TYPE then .kind, never by
// message content):
//   a. success dict  -> Success + garmin_user_id + display_name; ctor args
//                       (email, password, tokenstorePath) reach the stub verbatim
//   b. GarminError kind='auth'       -> AuthFailed (+ stub's message as rawMessage)
//   c. GarminError kind='connection' -> Network
//   d. GarminError kind='rate_limit' -> Unknown (explicitly NOT AuthFailed)
//   e. ValueError (non-GarminError)  -> Unknown (explicitly NOT AuthFailed)
//   f. modulePath at an empty dir    -> Unknown, no crash
//   g. call from a non-main std::thread -> same Success (PyGILState off-main proof)
//   h. >=3 repeated mixed-scenario calls stay GIL-balanced; includes a
//      unicode email/display_name UTF-8 round-trip
//
// Cites:
//   DES-013 — PyEmbeddedAdapter embedded-CPython bridge (this spec)
//   DES-001a / DEC-013 — IGarminPyAdapter seam being implemented
//   DES-012 — adapter calls only garmin_client.GarminClient, never garminconnect
//   DES-008 — raw messages forwarded untranslated
//   LSN-006 / A3-R002-M6 — never classify exceptions by message content
//
// RED expectation:
//   src/Cloud/PyEmbeddedAdapter.{h,cpp} do not exist yet. The build fails at
//   the #include line below — right-reason RED (missing contract under test).
//   GREEN adds those files and extends the CMake target with the .cpp.

// clang-format off
// Python.h must precede any Qt header (Qt's `slots` macro vs object.h's
// `slots` field) — protected from include re-sorting; the test harness owns
// the interpreter lifecycle.
#include <Python.h>
// clang-format on

#include "IGarminPyAdapter.h"
#include "PyEmbeddedAdapter.h" // <-- intentionally missing in RED

#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <thread>

#ifndef GARMIN_PYSTUBS_DIR
#    error "GARMIN_PYSTUBS_DIR must be defined by the CMake target (path to pystubs/)"
#endif

namespace {

// Run a snippet of Python under a properly acquired GIL — used to drive the
// stub's SCENARIO switch and to scrub import state between cases.
void runPy(const char* code)
{
    PyGILState_STATE st = PyGILState_Ensure();
    PyRun_SimpleString(code);
    PyGILState_Release(st);
}

// Point the stub at a scenario. Ensures the pystubs dir is importable first
// (the missing-module test deliberately scrubs it from sys.path), then flips
// the module-level SCENARIO switch.
void setScenario(const char* scenario)
{
    const QString code =
        QStringLiteral("import sys\n"
                       "_d = %1\n"
                       "if _d not in sys.path:\n"
                       "    sys.path.insert(0, _d)\n"
                       "import garmin_client\n"
                       "garmin_client.SCENARIO = '%2'\n")
            .arg(QStringLiteral("r'''") + QString::fromUtf8(GARMIN_PYSTUBS_DIR) + QStringLiteral("'''"),
                 QString::fromUtf8(scenario));
    runPy(code.toUtf8().constData());
}

// Read a string attribute off the (already imported) stub module. Returns a
// null QString when unavailable. Used to assert ctor-arg marshalling.
QString stubAttr(const char* name)
{
    PyGILState_STATE st = PyGILState_Ensure();
    QString out;
    PyObject* mod = PyImport_ImportModule("garmin_client");
    if (mod) {
        PyObject* v = PyObject_GetAttrString(mod, name);
        if (v && PyUnicode_Check(v)) {
            const char* utf8 = PyUnicode_AsUTF8(v);
            if (utf8)
                out = QString::fromUtf8(utf8);
        }
        Py_XDECREF(v);
        Py_DECREF(mod);
    }
    PyErr_Clear();
    PyGILState_Release(st);
    return out;
}

const QString kStubsDir = QString::fromUtf8(GARMIN_PYSTUBS_DIR);
const QString kTokenstore = QStringLiteral("/tmp/gc-test-tokens/garmin");

} // namespace

class TestGarminConnectPyAdapter : public QObject
{
    Q_OBJECT

    PyThreadState* mainState = nullptr;

  private slots:

    // Spec step 1 (DES-013): Py_IsInitialized() false -> Unknown +
    // rawMessage "embedded Python unavailable"; never crash/throw. This
    // assertion must precede Py_Initialize(), so it lives here — QTest runs
    // initTestCase before every other slot.
    void initTestCase()
    {
        QVERIFY2(!Py_IsInitialized(), "harness precondition: interpreter must not be up yet");

        PyEmbeddedAdapter early(kStubsDir, kTokenstore);
        const PyAuthOutcome out = early.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.rawMessage, QStringLiteral("embedded Python unavailable"));

        // Now bring the interpreter up and release the GIL from this (main)
        // thread so PyGILState_Ensure works from any thread afterwards.
        Py_Initialize();
        QVERIFY(Py_IsInitialized());
        mainState = PyEval_SaveThread();
    }

    // (f) modulePath pointing at an empty dir — import garmin_client fails —
    // must yield Unknown without crashing. Runs FIRST among the authenticate
    // slots so garmin_client is not yet cached in sys.modules; the scrub
    // below makes it deterministic even if slot order ever changes.
    void missingModuleYieldsUnknownWithoutCrash()
    {
        runPy("import sys\n"
              "sys.modules.pop('garmin_client', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        QTemporaryDir emptyDir;
        QVERIFY(emptyDir.isValid());

        PyEmbeddedAdapter adapter(emptyDir.path(), kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "a missing module must never masquerade as bad credentials");
        QVERIFY2(!out.rawMessage.isEmpty(), "rawMessage should carry the import error");
    }

    // (a) success dict marshals to Success + both fields, and the exact
    // email / password / tokenstorePath strings reach the stub's ctor.
    void successMarshalsFieldsAndCtorArgs()
    {
        setScenario("success");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.garmin_user_id, QStringLiteral("uid-123"));
        QCOMPARE(out.display_name, QStringLiteral("Alice Rider"));

        // ctor-arg marshalling — recorded by the stub, read back verbatim.
        QCOMPARE(stubAttr("LAST_EMAIL"), QStringLiteral("rider@example.com"));
        QCOMPARE(stubAttr("LAST_PASSWORD"), QStringLiteral("p@$$w/rd with spaces"));
        QCOMPARE(stubAttr("LAST_TOKENSTORE"), kTokenstore);
    }

    // (b) GarminError kind='auth' -> AuthFailed, rawMessage carries the
    // stub's message untranslated (DES-008: translation is the page's job).
    void authKindMapsToAuthFailed()
    {
        setScenario("auth_error");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("wrong"));

        QCOMPARE(out.kind, PyAuthOutcome::AuthFailed);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: bad credentials"));
    }

    // (c) GarminError kind='connection' -> Network. Catches a collapse-all-
    // GarminErrors-to-AuthFailed mutant.
    void connectionKindMapsToNetwork()
    {
        setScenario("connection_error");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Network);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: connection refused"));
    }

    // (d) GarminError with a kind this slice does not own (rate_limit) ->
    // Unknown — and asserted NOT AuthFailed (A3-R002-M6 heritage: an
    // unrecognized kind must never be reported as bad credentials).
    void rateLimitKindMapsToUnknownNotAuthFailed()
    {
        setScenario("rate_limit_error");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "kind='rate_limit' must NOT be classified as AuthFailed");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: too many requests"));
    }

    // (e) a non-GarminError Python exception (ValueError) -> Unknown, NEVER
    // AuthFailed (LSN-006: classify by type, not message).
    void valueErrorMapsToUnknownNotAuthFailed()
    {
        setScenario("value_error");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a non-GarminError exception must NOT be classified as AuthFailed");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.rawMessage.contains(QStringLiteral("stub: not a garmin error")),
                 "rawMessage should carry str(e) of the foreign exception");
    }

    // REQ-002 / TEST-005 / A3-R002-TR-05 — malformed login() result branches.
    // PyEmbeddedAdapter defends against a login() that returns a non-dict, and
    // against a dict missing garmin_user_id/display_name: both are DES-012
    // contract breaches, mapped to Unknown + an explanatory message — NEVER a
    // spurious Success. These two slots exercise those defensive branches
    // (untested before TR-05) so a mutant deleting either check is killed: with
    // the checks gone, a non-dict / missing-keys result falls through to a
    // Success with empty fields, which these asserts reject.

    // (i) login() returns a non-dict -> Unknown, non-empty message, NOT Success.
    void nonDictLoginResultYieldsUnknownNotSuccess()
    {
        setScenario("non_dict_result");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::Success, "a non-dict login() result must NOT be reported as Success");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "a non-dict login() result must carry an explanatory message");
    }

    // (j) login() returns a dict missing the required keys -> Unknown, non-empty
    // message, NOT Success (empty uid/display_name must never look like a login).
    void loginResultMissingKeysYieldsUnknownNotSuccess()
    {
        setScenario("missing_keys");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::Success,
                 "a login() dict missing required keys must NOT be reported as Success");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "a missing-keys login() result must carry an explanatory message");
    }

    // (g) the exact call pattern production uses: authenticate() invoked from
    // a non-main worker-like thread. PyGILState_Ensure must acquire the GIL
    // there and return the same Success marshalling as on the main thread.
    void workerThreadCallReturnsSameSuccess()
    {
        setScenario("success");

        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);
        PyAuthOutcome out;
        std::thread worker(
            [&] { out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("hunter2")); });
        worker.join();

        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.garmin_user_id, QStringLiteral("uid-123"));
        QCOMPARE(out.display_name, QStringLiteral("Alice Rider"));
    }

    // (h) repeated mixed-scenario calls stay GIL-balanced (an unbalanced
    // Ensure/Release pair deadlocks or aborts well before 4 iterations) and
    // results remain correct per scenario; the unicode leg round-trips a
    // non-ASCII email in and a non-ASCII display_name out (UTF-8 both ways).
    void repeatedMixedCallsStayGilBalancedWithUnicode()
    {
        PyEmbeddedAdapter adapter(kStubsDir, kTokenstore);

        // 1 — success
        setScenario("success");
        PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Success);

        // 2 — auth failure
        setScenario("auth_error");
        out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("bad"));
        QCOMPARE(out.kind, PyAuthOutcome::AuthFailed);

        // 3 — unicode round-trip: non-ASCII email in, non-ASCII fields out
        setScenario("success_unicode");
        const QString uniEmail = QString::fromUtf8("zo\xC3\xAB@ex\xC3\xA4mple.com"); // zoë@exämple.com
        out = adapter.authenticate(uniEmail, QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.garmin_user_id, QString::fromUtf8("uid-\xC3\xBC\xC3\xB1\xC3\xAE-123"));
        QCOMPARE(out.display_name, QString::fromUtf8("Zo\xC3\xAB \xC3\x85str\xC3\xB6m \xF0\x9F\x9A\xB4"));
        QCOMPARE(stubAttr("LAST_EMAIL"), uniEmail);

        // 4 — back to plain success: interpreter still healthy, GIL balanced
        setScenario("success");
        out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.display_name, QStringLiteral("Alice Rider"));
    }

    void cleanupTestCase()
    {
        // Re-acquire the main thread state saved in initTestCase, then
        // finalize. If this ever proves flaky on CI the finalize may be
        // skipped (process exit reclaims everything) — see briefing note.
        if (mainState) {
            PyEval_RestoreThread(mainState);
            Py_FinalizeEx();
        }
    }
};

QTEST_GUILESS_MAIN(TestGarminConnectPyAdapter)
#include "testGarminConnectPyAdapter.moc"
