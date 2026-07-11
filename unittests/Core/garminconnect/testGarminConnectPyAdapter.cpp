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
//                       (email, password) reach the stub verbatim, and NO
//                       tokenstore path is forwarded (T-016 / DEC-014 Option B)
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

#include <QByteArray>
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

// TEST-009 / REQ-007 — byte-identical to pystubs/garmin_client.py DL_PAYLOAD.
// Embedded NUL (0x00) mid-buffer + high bytes (0xff/0xfe) prove the marshalling
// reads the Python bytes by (ptr,len) — a strlen-based copy would truncate at
// the first NUL and this comparison would fail.
const unsigned char kDlBytesRaw[] = {0x00, 0x01, 0x02, 'F', 'I', 'T', 0x00, 0xff, 0xfe, 0x0a};
const QByteArray kExpectedDownload(reinterpret_cast<const char*>(kDlBytesRaw), static_cast<int>(sizeof(kDlBytesRaw)));

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

        PyEmbeddedAdapter early(kStubsDir);
        const PyAuthOutcome out = early.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.rawMessage, QStringLiteral("embedded Python unavailable"));

        // TEST-009 — the same fail-safe for the download op: before the
        // interpreter is up, downloadActivity must fold to Unknown /
        // "embedded Python unavailable" without crashing (DES-013 step 1).
        const PyDownloadOutcome dOut = early.downloadActivity(QStringLiteral("123"), QStringLiteral("ORIGINAL"));
        QCOMPARE(dOut.kind, PyDownloadOutcome::Unknown);
        QCOMPARE(dOut.rawMessage, QStringLiteral("embedded Python unavailable"));

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

        PyEmbeddedAdapter adapter(emptyDir.path());
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "a missing module must never masquerade as bad credentials");
        QVERIFY2(!out.rawMessage.isEmpty(), "rawMessage should carry the import error");
    }

    // (a) success dict marshals to Success + both fields, and the exact
    // email / password strings reach the stub's ctor — while NO tokenstore
    // path is forwarded (T-016 / DEC-014 Option B: auth-only construction).
    void successMarshalsFieldsAndCtorArgs()
    {
        setScenario("success");

        PyEmbeddedAdapter adapter(kStubsDir);
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Success);
        QCOMPARE(out.garmin_user_id, QStringLiteral("uid-123"));
        QCOMPARE(out.display_name, QStringLiteral("Alice Rider"));

        // ctor-arg marshalling — recorded by the stub, read back verbatim.
        QCOMPARE(stubAttr("LAST_EMAIL"), QStringLiteral("rider@example.com"));
        QCOMPARE(stubAttr("LAST_PASSWORD"), QStringLiteral("p@$$w/rd with spaces"));
        // T-016 — REQ-006 / DEC-014 Option B / A3-R004-M3: the adapter must
        // construct the Python GarminClient AUTH-ONLY, forwarding NO tokenstore
        // path. The pystub no longer records LAST_TOKENSTORE, so the attribute
        // is absent/None here — proving no path was forwarded (the inverse of
        // the old assertion that required the forwarded path).
        QVERIFY2(stubAttr("LAST_TOKENSTORE").isNull(),
                 "adapter must forward NO tokenstore path to the Python client (auth-only)");
    }

    // (b) GarminError kind='auth' -> AuthFailed, rawMessage carries the
    // stub's message untranslated (DES-008: translation is the page's job).
    void authKindMapsToAuthFailed()
    {
        setScenario("auth_error");

        PyEmbeddedAdapter adapter(kStubsDir);
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("wrong"));

        QCOMPARE(out.kind, PyAuthOutcome::AuthFailed);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: bad credentials"));
    }

    // (c) GarminError kind='connection' -> Network. Catches a collapse-all-
    // GarminErrors-to-AuthFailed mutant.
    void connectionKindMapsToNetwork()
    {
        setScenario("connection_error");

        PyEmbeddedAdapter adapter(kStubsDir);
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

        PyEmbeddedAdapter adapter(kStubsDir);
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

        PyEmbeddedAdapter adapter(kStubsDir);
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

        PyEmbeddedAdapter adapter(kStubsDir);
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

        PyEmbeddedAdapter adapter(kStubsDir);
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

        PyEmbeddedAdapter adapter(kStubsDir);
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
        PyEmbeddedAdapter adapter(kStubsDir);

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

    // ==================================================================
    // TEST-009 / REQ-007 — downloadActivity marshalling (DES-013 extension,
    // same DEC-013 seam one op down). PyEmbeddedAdapter.downloadActivity()
    // forwards (activity_id, fmt) to the authenticated GarminClient's
    // download_activity(), marshals a Python `bytes` return into a QByteArray
    // binary-exact, and classifies failures by exception TYPE then .kind
    // (LSN-006): connection→Network, rate_limit→RateLimited, foreign /
    // non-bytes / unknown-kind → Unknown — NEVER a spurious Success.
    //
    // Session model (DES-013 refinement): download reuses the client that
    // authenticate() established and the adapter retains — REQ-005 forbids
    // keeping the password, so a fresh per-download client is impossible.
    // Each slot therefore authenticates (success) first.
    // ==================================================================

    // (a) success: Python `bytes` marshals to a binary-exact QByteArray
    // (embedded NUL survives), and activity_id + fmt reach the stub verbatim.
    void downloadSuccessMarshalsBinaryBytesAndRecordsArgs()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        QCOMPARE(adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw")).kind,
                 PyAuthOutcome::Success);

        setScenario("dl_success");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("987654321"), QStringLiteral("ORIGINAL"));

        QCOMPARE(out.kind, PyDownloadOutcome::Success);
        QCOMPARE(out.data, kExpectedDownload);
        QCOMPARE(out.data.size(), kExpectedDownload.size()); // guards NUL-truncation explicitly
        QCOMPARE(stubAttr("LAST_ACTIVITY_ID"), QStringLiteral("987654321"));
        QCOMPARE(stubAttr("LAST_FMT"), QStringLiteral("ORIGINAL"));
    }

    // (b) fmt is forwarded, not hard-coded — 'TCX' reaches the stub as 'TCX'
    // (kills a mutant that always requests ORIGINAL, breaking DES-004 fallback).
    void downloadForwardsTcxFmt()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        QCOMPARE(adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw")).kind,
                 PyAuthOutcome::Success);

        setScenario("dl_success");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("TCX"));

        QCOMPARE(out.kind, PyDownloadOutcome::Success);
        QCOMPARE(stubAttr("LAST_FMT"), QStringLiteral("TCX"));
    }

    // (c) GarminError kind='connection' → Network, raw message forwarded.
    void downloadConnectionErrorMapsToNetwork()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_connection");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("ORIGINAL"));

        QCOMPARE(out.kind, PyDownloadOutcome::Network);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: download connection refused"));
    }

    // (d) GarminError kind='rate_limit' → RateLimited — a DISTINCT kind, not
    // collapsed to Unknown or misrouted to Network (DES-008 rate-limit copy).
    void downloadRateLimitErrorMapsToRateLimited()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_rate_limit");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("ORIGINAL"));

        QVERIFY2(out.kind != PyDownloadOutcome::Unknown, "rate_limit must be its own kind, not Unknown");
        QVERIFY2(out.kind != PyDownloadOutcome::Network, "rate_limit must not be misrouted to Network");
        QCOMPARE(out.kind, PyDownloadOutcome::RateLimited);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: download rate-limited"));
    }

    // (e) a non-GarminError exception (ValueError) → Unknown, NEVER Success
    // (LSN-006: classify by type; a foreign exception is not a valid download).
    void downloadForeignExceptionMapsToUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_value_error");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("ORIGINAL"));

        QVERIFY2(out.kind != PyDownloadOutcome::Success, "a foreign exception must NOT be reported as a Success");
        QCOMPARE(out.kind, PyDownloadOutcome::Unknown);
        QVERIFY2(out.rawMessage.contains(QStringLiteral("not a garmin error (download)")),
                 "rawMessage should carry str(e) of the foreign exception");
    }

    // (f) a non-bytes return (contract breach of the DES-012 seam) → Unknown,
    // NEVER a Success with empty data. Kills a mutant that skips the type check.
    void downloadNonBytesResultYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_non_bytes");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("ORIGINAL"));

        QVERIFY2(out.kind != PyDownloadOutcome::Success, "a non-bytes result must NOT be reported as Success");
        QCOMPARE(out.kind, PyDownloadOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "a non-bytes result must carry an explanatory message");
    }

    // (g) download before any successful authenticate → Unknown (no retained
    // session), never a crash and never a Success. The password cannot be
    // reused (REQ-005), so there is no client to download through.
    void downloadWithoutAuthenticateYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(kStubsDir); // never authenticated
        setScenario("dl_success");
        const PyDownloadOutcome out = adapter.downloadActivity(QStringLiteral("111"), QStringLiteral("ORIGINAL"));

        QVERIFY2(out.kind != PyDownloadOutcome::Success, "download without a session must NOT succeed");
        QCOMPARE(out.kind, PyDownloadOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "must explain why the download could not run");
    }

    // (h) the production call pattern: authenticate on this thread, then
    // download from a non-main worker-like std::thread. PyGILState_Ensure must
    // acquire the GIL there and marshal the identical bytes.
    void downloadFromWorkerThreadMarshalsSameBytes()
    {
        PyEmbeddedAdapter adapter(kStubsDir);
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_success");
        PyDownloadOutcome out;
        std::thread worker([&] { out = adapter.downloadActivity(QStringLiteral("42"), QStringLiteral("ORIGINAL")); });
        worker.join();

        QCOMPARE(out.kind, PyDownloadOutcome::Success);
        QCOMPARE(out.data, kExpectedDownload);
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
