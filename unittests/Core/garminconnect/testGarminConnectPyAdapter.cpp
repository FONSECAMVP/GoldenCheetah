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
#include <QDir>
#include <QFile>
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

// Point the stub at a scenario. DEC-066: the sys.modules cache may only ever
// be seeded by this adapter's OWN ledgered import — a raw Python `import`
// statement on a cold cache would create the first, unledgered entry for
// kStubsDir, and the provenance ledger's pre-import check has nothing to
// trust it against (it would then fail closed on every real construction
// that follows). A throwaway adapter construction goes first to seed the
// ledger through the real production path (also handles hoisting kStubsDir
// onto sys.path, same as the missing-module test's deliberate scrub needed
// before); its ctor args and outcome are irrelevant and immediately
// superseded by the caller's own real construction after this returns. Only
// then is the module-level SCENARIO switch flipped, on the now-ledgered (or
// already-cached-from-ambient-state) module.
void setScenario(const char* scenario)
{
    const QString stubsDir = QString::fromUtf8(GARMIN_PYSTUBS_DIR);
    PyEmbeddedAdapter bootstrap(GarminPyModulePath::explicitOverride(stubsDir));
    bootstrap.authenticate(QStringLiteral("bootstrap@example.com"), QStringLiteral("bootstrap"));

    const QString code = QStringLiteral("import gc_garmin_adapter.garmin_client\n"
                                        "gc_garmin_adapter.garmin_client.SCENARIO = '%1'\n")
                             .arg(QString::fromUtf8(scenario));
    runPy(code.toUtf8().constData());
}

// Read a string attribute off the (already imported) stub module. Returns a
// null QString when unavailable. Used to assert ctor-arg marshalling.
QString stubAttr(const char* name)
{
    PyGILState_STATE st = PyGILState_Ensure();
    QString out;
    PyObject* mod = PyImport_ImportModule("gc_garmin_adapter.garmin_client");
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

// True iff name is a key in sys.modules. Read-only — unlike stubAttr(), which
// itself calls PyImport_ImportModule and so cannot be used to check whether
// an import was ever attempted without triggering one.
bool sysModulesContains(const char* name)
{
    PyGILState_STATE st = PyGILState_Ensure();
    bool present = false;
    PyObject* mods = PyImport_GetModuleDict(); // borrowed
    if (mods && PyDict_Check(mods)) {
        PyObject* key = PyUnicode_FromString(name);
        if (key) {
            present = PyDict_Contains(mods, key) == 1;
            Py_DECREF(key);
        }
    }
    PyErr_Clear();
    PyGILState_Release(st);
    return present;
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

        PyEmbeddedAdapter early(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = early.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.rawMessage, QStringLiteral("embedded Python unavailable"));

        // TEST-009 — the same fail-safe for the download op: before the
        // interpreter is up, downloadActivity must fold to Unknown /
        // "embedded Python unavailable" without crashing (DES-013 step 1).
        const PyDownloadOutcome dOut = early.downloadActivity(QStringLiteral("123"), QStringLiteral("ORIGINAL"));
        QCOMPARE(dOut.kind, PyDownloadOutcome::Unknown);
        QCOMPARE(dOut.rawMessage, QStringLiteral("embedded Python unavailable"));

        // TEST-043 — the same fail-safe for the listing op: before the
        // interpreter is up, listActivitiesSince must fold to Unknown /
        // "embedded Python unavailable" without crashing (DES-013 step 1).
        const PyListOutcome lOut = early.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));
        QCOMPARE(lOut.kind, PyListOutcome::Unknown);
        QCOMPARE(lOut.rawMessage, QStringLiteral("embedded Python unavailable"));

        // REQ-013 (DEC-050) — the same fail-safe for the profile-fetch op:
        // before the interpreter is up, fetchProfile must fold to Unknown /
        // "embedded Python unavailable" without crashing (DES-013 step 1).
        const PyProfileOutcome pOut = early.fetchProfile();
        QCOMPARE(pOut.kind, PyProfileOutcome::Unknown);
        QCOMPARE(pOut.rawMessage, QStringLiteral("embedded Python unavailable"));

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
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        QTemporaryDir emptyDir;
        QVERIFY(emptyDir.isValid());

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(emptyDir.path()));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "a missing module must never masquerade as bad credentials");
        QVERIFY2(!out.rawMessage.isEmpty(), "rawMessage should carry the import error");
    }

    // B-STAGE9-38 unit 3 round 4 (DEC-058 c17(a) / B-STAGE9-44): a prepend-if-
    // absent is not enough — the override dir must be HOISTED to sys.path
    // index 0 even when it is already present LATER. This test deliberately
    // ESTABLISHES that precondition itself (strips every existing occurrence
    // of kStubsDir, then re-adds it at the END, after an unrelated installed
    // stand-in at the FRONT) rather than relying on ambient state left by
    // earlier slots, so it cannot pass vacuously (B-STAGE9-46).
    // RED mutation: reverting to insert-only-if-absent leaves kStubsDir at
    // the tail, the installed stand-in resolves first, and both assertions
    // below fail.
    void explicitOverrideDirIsHoistedInFrontOfAnAlreadyLaterEntry()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n");

        QTemporaryDir installedDir;
        QVERIFY(installedDir.isValid());
        QVERIFY(QDir(installedDir.path()).mkpath(QStringLiteral("gc_garmin_adapter")));
        const QString installedPkg = installedDir.path() + QStringLiteral("/gc_garmin_adapter/");
        const QString stubPkg = kStubsDir + QStringLiteral("/gc_garmin_adapter/");
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("__init__.py"), installedPkg + QStringLiteral("__init__.py")));
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("garmin_client.py"),
                            installedPkg + QStringLiteral("garmin_client.py")));

        // Precondition, established here rather than assumed: strip every
        // exact occurrence of kStubsDir, put the installed stand-in FIRST,
        // then append kStubsDir at the very END — later, not merely absent.
        const QString setupCode = QStringLiteral("import sys\n"
                                                 "_stub = %1\n"
                                                 "_installed = %2\n"
                                                 "sys.path = [p for p in sys.path if p != _stub]\n"
                                                 "sys.path.insert(0, _installed)\n"
                                                 "sys.path.append(_stub)\n")
                                      .arg(QStringLiteral("r'''") + kStubsDir + QStringLiteral("'''"),
                                           QStringLiteral("r'''") + installedDir.path() + QStringLiteral("'''"));
        runPy(setupCode.toUtf8().constData());

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Success);

        const QString resolvedFile = stubAttr("__file__");
        QVERIFY2(resolvedFile.startsWith(kStubsDir),
                 "an explicit override present later in sys.path must still win over an earlier stand-in");

        PyGILState_STATE st = PyGILState_Ensure();
        PyObject* sysPath = PySys_GetObject("path"); // borrowed
        QString frontEntry;
        if (PyList_Check(sysPath) && PyList_Size(sysPath) > 0) {
            PyObject* first = PyList_GetItem(sysPath, 0); // borrowed
            if (first && PyUnicode_Check(first)) {
                const char* utf8 = PyUnicode_AsUTF8(first);
                if (utf8)
                    frontEntry = QString::fromUtf8(utf8);
            }
        }
        PyGILState_Release(st);
        QCOMPARE(frontEntry, kStubsDir);
    }

    // B-STAGE9-38 unit 3 round 4 (DEC-058 c17(b) / B-STAGE9-45): a
    // `gc_garmin_adapter` cached in sys.modules from an ORIGIN OTHER THAN the
    // explicit override must never be silently reused — that is exactly how
    // PyImport_ImportModule() defeated the override with no path lookup at
    // all. This test ESTABLISHES the precondition itself: a plain import
    // (not through the adapter) from a directory that is provably NOT
    // kStubsDir, asserted via __file__ before the adapter is ever
    // constructed. The explicit-override construction must then fail closed
    // (Unknown, never AuthFailed) and must NOT evict or mutate the existing
    // cache. RED mutation: dropping the origin guard and calling
    // PyImport_ImportModule() directly instead reuses the wrong-origin cache
    // and reports Success.
    void explicitOverrideFailsClosedWhenCacheIsFromAnotherOrigin()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        QTemporaryDir installedDir;
        QVERIFY(installedDir.isValid());
        QVERIFY(QDir(installedDir.path()).mkpath(QStringLiteral("gc_garmin_adapter")));
        const QString installedPkg = installedDir.path() + QStringLiteral("/gc_garmin_adapter/");
        const QString stubPkg = kStubsDir + QStringLiteral("/gc_garmin_adapter/");
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("__init__.py"), installedPkg + QStringLiteral("__init__.py")));
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("garmin_client.py"),
                            installedPkg + QStringLiteral("garmin_client.py")));

        const QString importCode = QStringLiteral("import sys\n"
                                                  "sys.path.insert(0, %1)\n"
                                                  "import gc_garmin_adapter.garmin_client\n")
                                       .arg(QStringLiteral("r'''") + installedDir.path() + QStringLiteral("'''"));
        runPy(importCode.toUtf8().constData());

        const QString cachedFile = stubAttr("__file__");
        QVERIFY2(cachedFile.startsWith(installedDir.path()),
                 "harness precondition: the cache must be proven to originate from installedDir, not kStubsDir");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a cache-origin mismatch must never masquerade as bad credentials");

        const QString stillCachedFile = stubAttr("__file__");
        QCOMPARE(stillCachedFile, cachedFile); // cache left intact — no eviction happened
    }

    // B-STAGE9-38 unit 3 round 5 (DEC-058 c17(b) / B-STAGE9-45): a cached
    // `gc_garmin_adapter` whose __file__ EQUALS the override directory itself
    // (a directory, not a module file) must never count as proof of origin.
    // This seeds exactly that shape and gives the fake parent a __path__
    // pointing at a DECOY package under an unrelated directory — standing in
    // for the foreign/unverifiable origin the defect let through. If the
    // equals-dir check is wrongly accepted, the subsequent submodule import
    // resolves the decoy and reports Success from the wrong location; the
    // correct fail-closed behaviour refuses before that submodule import is
    // ever attempted. RED mutation: restoring the old
    // `cleanFile == cleanDir || ...` predicate at moduleFileIsUnderDir.
    void explicitOverrideFailsClosedWhenCachedFileIsTheDirItself()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        QTemporaryDir decoyDir;
        QVERIFY(decoyDir.isValid());
        QVERIFY(QDir(decoyDir.path()).mkpath(QStringLiteral("gc_garmin_adapter")));
        const QString decoyPkg = decoyDir.path() + QStringLiteral("/gc_garmin_adapter/");
        const QString stubPkg = kStubsDir + QStringLiteral("/gc_garmin_adapter/");
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("__init__.py"), decoyPkg + QStringLiteral("__init__.py")));
        QVERIFY(
            QFile::copy(stubPkg + QStringLiteral("garmin_client.py"), decoyPkg + QStringLiteral("garmin_client.py")));

        const QString fabricateCode = QStringLiteral("import sys, types\n"
                                                     "m = types.ModuleType('gc_garmin_adapter')\n"
                                                     "m.__file__ = %1\n"
                                                     "m.__path__ = [%2]\n"
                                                     "sys.modules['gc_garmin_adapter'] = m\n")
                                          .arg(QStringLiteral("r'''") + kStubsDir + QStringLiteral("'''"),
                                               QStringLiteral("r'''") + decoyPkg + QStringLiteral("'''"));
        runPy(fabricateCode.toUtf8().constData());

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a __file__-is-the-directory cache entry must never masquerade as bad credentials");
        QVERIFY2(!sysModulesContains("gc_garmin_adapter.garmin_client"),
                 "fail-closed must refuse before the decoy submodule is ever imported");
    }

    // DEC-066 constraint 1: a genuinely ledgered parent package is retained,
    // its child dropped from sys.modules, and the parent's own __path__
    // retargeted at a decoy — the pre-import check sees only the (still
    // genuine) parent and would accept; the decoy child is created fresh by
    // PyImport_ImportModule() DURING this very call, so only a post-import
    // re-check catches it. RED mutation: removing the post-import
    // validateExplicitOverrideCacheAgainstLedger() call (checking identity
    // only before PyImport_ImportModule, never after) reports Success from
    // the decoy.
    void explicitOverrideFailsClosedWhenLedgeredParentsPathIsRetargetedAfterChildIsDropped()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        // Establish a genuine ledger entry: one real construction over kStubsDir.
        PyEmbeddedAdapter warm(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome warmOut = warm.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(warmOut.kind, PyAuthOutcome::Success);

        // Decoy package the retargeted __path__ will resolve the child from.
        QTemporaryDir decoyDir;
        QVERIFY(decoyDir.isValid());
        QVERIFY(QDir(decoyDir.path()).mkpath(QStringLiteral("gc_garmin_adapter")));
        const QString decoyPkg = decoyDir.path() + QStringLiteral("/gc_garmin_adapter/");
        const QString stubPkg = kStubsDir + QStringLiteral("/gc_garmin_adapter/");
        QVERIFY(QFile::copy(stubPkg + QStringLiteral("__init__.py"), decoyPkg + QStringLiteral("__init__.py")));
        QVERIFY(
            QFile::copy(stubPkg + QStringLiteral("garmin_client.py"), decoyPkg + QStringLiteral("garmin_client.py")));

        // Attacker step: drop only the child, keep the (still-ledgered)
        // parent, and retarget the parent's __path__ at the decoy.
        const QString attackCode = QStringLiteral("import sys\n"
                                                  "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
                                                  "sys.modules['gc_garmin_adapter'].__path__ = [%1]\n")
                                       .arg(QStringLiteral("r'''") + decoyPkg + QStringLiteral("'''"));
        runPy(attackCode.toUtf8().constData());

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a post-import identity mismatch must never masquerade as bad credentials");

        // The decoy DID load into sys.modules — proving the attack surface is
        // real and the rejection is a post-import check, not a pre-import
        // refusal that never let the attack run.
        const QString loadedFile = stubAttr("__file__");
        QVERIFY2(loadedFile.startsWith(decoyDir.path()),
                 "harness precondition: the decoy child must actually have been imported");
    }

    // DEC-066 Option A residual proof: a second, independent adapter
    // constructed over the SAME override must still succeed — the ledger
    // records identity, it does not block legitimate repeat use (DEC-058
    // c17(b) constraint carried into DEC-066 constraint 3). RED mutation: an
    // over-broad "reject any second construction over an already-ledgered
    // dir" rule would fail this outright.
    void explicitOverrideSecondConstructionOverSameOverrideStillSucceeds()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        PyEmbeddedAdapter first(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome firstOut = first.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(firstOut.kind, PyAuthOutcome::Success);

        PyEmbeddedAdapter second(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome secondOut = second.authenticate(QStringLiteral("c@d"), QStringLiteral("pw2"));
        QCOMPARE(secondOut.kind, PyAuthOutcome::Success);
    }

    // DEC-066 constraint 5: any gc_garmin_adapter[.*] entry in sys.modules
    // that was never produced by this adapter's own clean import must fail
    // the whole call closed, even when the entries this call itself imports
    // (parent + its own child) are untouched and still genuinely ledgered.
    // RED mutation: scanning only the two names this call itself imports
    // (instead of every gc_garmin_adapter[.*] key already in sys.modules)
    // would miss the rogue sibling entry and report Success.
    void explicitOverrideFailsClosedWhenAnUnledgeredSiblingSubmoduleAppears()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        PyEmbeddedAdapter warm(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome warmOut = warm.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(warmOut.kind, PyAuthOutcome::Success);

        // A sibling submodule never imported through this adapter — unledgered
        // by construction, regardless of how well-formed it looks.
        runPy("import sys, types\n"
              "m = types.ModuleType('gc_garmin_adapter.rogue')\n"
              "sys.modules['gc_garmin_adapter.rogue'] = m\n");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out =
            adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("p@$$w/rd with spaces"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "an unledgered sibling entry must never masquerade as bad credentials");

        // Cleanup: the injected rogue entry is this test's own fixture, not
        // ambient state any later test should have to account for.
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.rogue', None)\n");
    }

    // DEC-066 constraint 2 (isolated case): releaseModuleProvenanceLedgerFor-
    // CurrentInterpreter() is wired into cleanupTestCase() right before
    // Py_FinalizeEx(), but nothing exercises the call itself anywhere else.
    // This proves the mechanism actually erases this interpreter's ledger
    // entries rather than merely being reachable: sys.modules is deliberately
    // left populated (NOT scrubbed) after the release call, so the only way a
    // later construction over the same override can still fail closed is if
    // the ledger itself — not the sys.modules cache — changed underneath it.
    // RED mutation: reducing releaseModuleProvenanceLedgerForCurrentInterpreter()
    // to a no-op leaves the ledger's entries in place, so the second
    // construction below wrongly succeeds instead of failing closed.
    void releaseLedgerForCurrentInterpreterMakesALaterConstructionFailClosed()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        PyEmbeddedAdapter first(GarminPyModulePath::explicitOverride(kStubsDir));
        QCOMPARE(first.authenticate(QStringLiteral("a@b"), QStringLiteral("pw")).kind, PyAuthOutcome::Success);

        // The GIL is not held between authenticate() calls (each acquires
        // and releases its own) — releaseModuleProvenanceLedgerForCurrentInterpreter()
        // requires it held, same as its real call site in cleanupTestCase().
        const PyGILState_STATE gil = PyGILState_Ensure();
        PyEmbeddedAdapter::releaseModuleProvenanceLedgerForCurrentInterpreter();
        PyGILState_Release(gil);

        // sys.modules is untouched by the release call above — it still
        // holds the exact entries `first`'s import produced.
        PyEmbeddedAdapter second(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = second.authenticate(QStringLiteral("c@d"), QStringLiteral("pw2"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a cleared-ledger rejection must never masquerade as bad credentials");

        // Restore the invariant every other test relies on (ledger and
        // sys.modules both empty for gc_garmin_adapter[.*]) before returning
        // — this test intentionally leaves them mismatched above.
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n");
    }

    // DEC-068: exercises the nullptr guard at PyEmbeddedAdapter.cpp:333-334,
    // not the :353-362 compare — see DEC-068 for why that compare is
    // unreached by this shape.
    void explicitOverrideFailsClosedWhenHostileImportHookLeavesSysModulesUntouched()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        PyEmbeddedAdapter warm(GarminPyModulePath::explicitOverride(kStubsDir));
        QCOMPARE(warm.authenticate(QStringLiteral("a@b"), QStringLiteral("pw")).kind, PyAuthOutcome::Success);

        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n");

        runPy("import types, builtins\n"
              "_decoy = types.ModuleType('decoy_garmin_client')\n"
              "class _DecoyClient:\n"
              "    def __init__(self, *a, **k): pass\n"
              "    def login(self):\n"
              "        return {'garmin_user_id': 'forged', 'display_name': 'Forged'}\n"
              "_decoy.GarminClient = _DecoyClient\n"
              "_real_import = builtins.__import__\n"
              "def _hostile(name, *a, **k):\n"
              "    if name == 'gc_garmin_adapter.garmin_client':\n"
              "        return _decoy\n"
              "    return _real_import(name, *a, **k)\n"
              "builtins.__import__ = _hostile\n");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        runPy("import sys, builtins\n"
              "builtins.__import__ = _real_import\n"
              "del _real_import, _hostile, _decoy, _DecoyClient\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n");

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a hostile-import rejection must never masquerade as bad credentials");
    }

    // DEC-068: pins the invariant the :353-362 tripwire rests on — a hostile
    // builtins.__import__ that WRITES a decoy into sys.modules and RETURNS a
    // different object still hands the adapter the sys.modules object, never
    // the hook's own return value. Two phases: hook returns what it wrote
    // (trivial baseline), then a distinct object (the actual invariant).
    void moduleImportAlwaysObservesTheSysModulesObjectNotWhatImportReturns()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        runPy("import types, builtins\n"
              "def _make(marker):\n"
              "    m = types.ModuleType('decoy_' + marker)\n"
              "    class C:\n"
              "        def __init__(self, *a, **k): pass\n"
              "        def login(self):\n"
              "            return {'garmin_user_id': marker, 'display_name': marker}\n"
              "    m.GarminClient = C\n"
              "    return m\n"
              "_written1 = _make('phase1-written')\n"
              "_real_import = builtins.__import__\n"
              "def _hostile(name, *a, **k):\n"
              "    if name == 'gc_garmin_adapter.garmin_client':\n"
              "        import sys as _sys\n"
              "        _sys.modules['gc_garmin_adapter.garmin_client'] = _written1\n"
              "        return _written1\n"
              "    return _real_import(name, *a, **k)\n"
              "builtins.__import__ = _hostile\n");

        PyEmbeddedAdapter phase1(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out1 = phase1.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));

        runPy("import sys, builtins\n"
              "builtins.__import__ = _real_import\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n");

        QCOMPARE(out1.kind, PyAuthOutcome::Success);
        QCOMPARE(out1.garmin_user_id, QStringLiteral("phase1-written"));

        runPy("import builtins\n"
              "_written2 = _make('phase2-written')\n"
              "_returned2 = _make('phase2-returned')\n"
              "def _hostile2(name, *a, **k):\n"
              "    if name == 'gc_garmin_adapter.garmin_client':\n"
              "        import sys as _sys\n"
              "        _sys.modules['gc_garmin_adapter.garmin_client'] = _written2\n"
              "        return _returned2\n"
              "    return _real_import(name, *a, **k)\n"
              "builtins.__import__ = _hostile2\n");

        PyEmbeddedAdapter phase2(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out2 = phase2.authenticate(QStringLiteral("c@d"), QStringLiteral("pw2"));

        runPy("import sys, builtins\n"
              "builtins.__import__ = _real_import\n"
              "del _make, _written1, _real_import, _hostile, _written2, _returned2, _hostile2\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n");

        QCOMPARE(out2.kind, PyAuthOutcome::Success);
        QCOMPARE(out2.garmin_user_id, QStringLiteral("phase2-written"));
    }

    // B-STAGE9-38 unit 3 round 2 (DO-2 proof): the reported defect —
    // importAdapterModule() prepended modulePath and retried after ANY
    // failed plain import, so a missing installed payload silently
    // succeeded wherever the source tree happened to be checked out. With
    // no override, exactly one plain import is attempted and sys.path is
    // never touched — proven independent of whether any particular
    // directory happens to satisfy the import, by asserting sys.path's own
    // length is unchanged.
    void noOverrideNeverTouchesSysPathWhenPackageIsAbsent()
    {
        runPy("import sys\n"
              "sys.modules.pop('gc_garmin_adapter.garmin_client', None)\n"
              "sys.modules.pop('gc_garmin_adapter', None)\n"
              "sys.path = [p for p in sys.path if 'pystubs' not in p]\n");

        PyGILState_STATE st0 = PyGILState_Ensure();
        PyObject* sysPathBefore = PySys_GetObject("path"); // borrowed
        const Py_ssize_t before = PyList_Check(sysPathBefore) ? PyList_Size(sysPathBefore) : -1;
        PyGILState_Release(st0);

        PyEmbeddedAdapter adapter(GarminPyModulePath::none());
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("a@b"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "a missing module must never masquerade as bad credentials");

        PyGILState_STATE st1 = PyGILState_Ensure();
        PyObject* sysPathAfter = PySys_GetObject("path"); // borrowed
        const Py_ssize_t after = PyList_Check(sysPathAfter) ? PyList_Size(sysPathAfter) : -1;
        PyGILState_Release(st1);

        QCOMPARE(after, before);
    }

    // (a) success dict marshals to Success + both fields, and the exact
    // email / password strings reach the stub's ctor — while NO tokenstore
    // path is forwarded (T-016 / DEC-014 Option B: auth-only construction).
    void successMarshalsFieldsAndCtorArgs()
    {
        setScenario("success");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("wrong"));

        QCOMPARE(out.kind, PyAuthOutcome::AuthFailed);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: bad credentials"));
        // Stage 9 diagnostic: exceptionType is only meaningful on the Unknown
        // path (see PyAuthOutcome::exceptionType's doc comment) — a classified
        // GarminError kind must leave it empty.
        QVERIFY2(out.exceptionType.isEmpty(), "exceptionType must be empty for a classified GarminError kind (Auth)");
    }

    // (c) GarminError kind='connection' -> Network. Catches a collapse-all-
    // GarminErrors-to-AuthFailed mutant.
    void connectionKindMapsToNetwork()
    {
        setScenario("connection_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Network);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: connection refused"));
    }

    // (d) REQ-014 — GarminError kind='rate_limit' -> PyAuthOutcome::RateLimit,
    // a DISTINCT kind (no longer folded into Unknown — that was REQ-002's
    // narrower bar before this slice added the branch). Still asserted NOT
    // AuthFailed (A3-R002-M6 heritage: an unrecognized/transient kind must
    // never be reported as bad credentials).
    void rateLimitKindMapsToRateLimitNotAuthFailed()
    {
        setScenario("rate_limit_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed, "kind='rate_limit' must NOT be classified as AuthFailed");
        QVERIFY2(out.kind != PyAuthOutcome::Unknown, "REQ-014: rate_limit must be its own kind, not Unknown");
        QCOMPARE(out.kind, PyAuthOutcome::RateLimit);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: too many requests"));
    }

    // (e) a non-GarminError Python exception (ValueError) -> Unknown, NEVER
    // AuthFailed (LSN-006: classify by type, not message).
    void valueErrorMapsToUnknownNotAuthFailed()
    {
        setScenario("value_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QVERIFY2(out.kind != PyAuthOutcome::AuthFailed,
                 "a non-GarminError exception must NOT be classified as AuthFailed");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.rawMessage.contains(QStringLiteral("stub: not a garmin error")),
                 "rawMessage should carry str(e) of the foreign exception");
        // Stage 9 diagnostic (reviewer delta-fix, security): the raw message
        // text is untrusted and unsafe to persist to the on-disk developer
        // log verbatim (it can carry request payloads/headers/tokens, or a
        // pathological library bug could even embed the password). Instead
        // classifyPendingException() must surface the module-qualified
        // exception TYPE name only — a type name cannot carry interpolated
        // secret material by construction. The stub raises a bare
        // `ValueError`, a builtin, so __module__ is "builtins".
        QCOMPARE(out.exceptionType, QStringLiteral("builtins.ValueError"));
    }

    // Reviewer delta-fix #1 (security) — a custom metaclass makes the raised
    // exception's __module__ AND __qualname__ resolve to non-str objects
    // whose __str__ returns "LEAKED-SECRET-VIA-...". pyExceptionTypeName()
    // must treat a non-unicode attribute as absent (never stringify it via
    // toQString()'s str(o) fallback) — otherwise an adversarial/unusual
    // foreign exception type could leak arbitrary text through a field whose
    // entire contract is "carries no leak surface" (PyAuthOutcome::
    // exceptionType's doc comment).
    void nonStrModuleAndQualnameNeverLeakIntoExceptionType()
    {
        setScenario("type_confusion_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("LEAKED-SECRET")),
                 "a non-str __module__/__qualname__ must never be stringified into exceptionType");
        // Reviewer delta-fix #5: a non-str (hence empty) moduleName no longer
        // falls back to a "builtins.<class name>"-shaped name — the unified
        // gate now requires moduleName to be present+valid+allowlisted
        // before ANY derived name (qualname- or tp_name-based) is emitted,
        // so an empty moduleName folds uniformly to foreign_exception.
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
    }

    // Reviewer delta-fix #2 (correctness) — __qualname__ access raises,
    // forcing the tp_name fallback branch, AND the raised type's raw tp_name
    // is already a dotted, fully-qualified string (the stub constructs it via
    // type()'s 3-arg form with a dotted name — see the fixture's comment for
    // why this is the honest reproduction of a real C-extension type's shape,
    // e.g. curl_cffi.requests.exceptions.ImpersonateError). The OLD code
    // unconditionally prepended moduleName + "." on top of this, doubling the
    // prefix (e.g. "garmin_client.curl_cffi.requests.exceptions.
    // ImpersonateError"); the fix must return tp_name verbatim instead.
    // Reviewer delta-fix #5 note: this fixture's moduleName is "garmin_client"
    // (the real, unintercepted __module__ of the type()-constructed class —
    // only __qualname__ is overridden) which is present/valid/allowlisted,
    // so the delta-fix #5 gate passes it through unaffected.
    //
    // Reviewer delta-fix #6 UPDATE: this fixture's tp_name
    // ("curl_cffi.requests.exceptions.ImpersonateError") was ALWAYS a
    // synthetic claim — this pystub never imports real curl_cffi, so no such
    // object is genuinely registered in sys.modules. Delta-fix #6's
    // provenance walk now correctly rejects it as foreign, which is exactly
    // the class of gap delta-fix #6 closes: a crafted dotted tp_name can no
    // longer produce ANY name, doubled-prefix or otherwise. Noted limitation
    // (per the reviewer's own guidance): real curl_cffi exception classes are
    // plain Python `class` definitions with a BARE (undotted) tp_name
    // matching their __qualname__ (confirmed via runtime inspection of the
    // actual installed package), so a genuinely-backed reproduction of
    // "missing __qualname__ + dotted tp_name" does not exist anywhere in
    // this project's real dependency chain — there is nothing to construct
    // a still-passing positive-path variant of this specific scenario from.
    void qualnameFallbackToTpNameDoesNotDoubleModulePrefix()
    {
        setScenario("tp_name_fallback_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
    }

    // Reviewer delta-fix #3 (security) — __module__ resolves to a genuine
    // `str` (passes PyUnicode_Check, so delta-fix #1's strictUnicodeAttr()
    // gate alone does NOT catch this), but its content is not a real module
    // path — "sk-LEAKED-SECRET-abc123" rather than an ASCII dotted
    // identifier. Nothing stops a foreign/adversarial exception class from
    // reassigning __module__ to arbitrary string content, so
    // pyExceptionTypeName() must validate the SHAPE of an already-str value,
    // not just its type, before surfacing it.
    void nonIdentifierShapedModuleNeverLeaksIntoExceptionType()
    {
        setScenario("bad_module_shape_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("LEAKED-SECRET")),
                 "a str __module__ whose content is not name-shaped must never be surfaced verbatim");
        // Reviewer delta-fix #5: an invalid-shape (hence empty) moduleName no
        // longer falls back to a "builtins.<class name>"-shaped name — see
        // the delta-fix #5 comment on nonStrModuleAndQualnameNeverLeak... above.
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
    }

    // Reviewer delta-fix #4 (security) — grammar-shaped is not the same as
    // safe: __module__ is reassigned to "hunter2", a genuine, grammar-valid
    // ASCII identifier (would pass delta-fix #3's isSafeDottedName() cleanly)
    // that is nonetheless NOT one of this integration's actual
    // dependency-chain module roots. Real secrets (API keys/tokens) are
    // often identifier-shaped too, so pyExceptionTypeName() must reject any
    // module string outside the fixed allowlist rather than surface it.
    void nonAllowlistedModuleFoldsToForeignException()
    {
        setScenario("foreign_module_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("hunter2")),
                 "an identifier-shaped but non-allowlisted __module__ must never be surfaced verbatim");
    }

    // Reviewer delta-fix #6 (security) — REPURPOSED from delta-fix #4's
    // positive-case test. That round proved an allowlisted __module__
    // string produces a full qualified name; delta-fix #6 found this exact
    // shape IS the spoof — __module__ is a claimed-but-unbacked allowlisted
    // string ("curl_cffi.requests.exceptions") that this class is NOT
    // actually an attribute of. isAllowedModuleRoot()'s string check alone
    // can't tell claimed from genuine; isGenuineAllowlistedException()'s
    // sys.modules + attribute-chain walk (compared by raw pointer identity)
    // now correctly rejects it as foreign. See
    // genuineDependencyBackedExceptionStillProducesFullyQualifiedName()
    // below for the still-passing REAL positive-path replacement.
    void spoofedAllowlistedModuleClaimFoldsToForeignException()
    {
        setScenario("allowlisted_third_party_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("AllowlistedThirdPartyError")),
                 "a claimed-but-unbacked allowlisted __module__ must never be surfaced verbatim, "
                 "even though the string itself is on the allowlist");
    }

    // Reviewer delta-fix #6 genuine-positive-path test — a REAL, unmodified
    // exception raised from the real, already-installed `requests` package
    // (transitively bundled with curl_cffi in the actual adapter's
    // dependency chain). Its __module__/__qualname__ are never touched, so
    // isGenuineAllowlistedException()'s sys.modules walk must resolve
    // "requests.exceptions.ConnectionError" to this exact real type object
    // by pointer identity, proving the delta-fix #6 provenance check does
    // not regress genuine diagnostic value for a real dependency exception.
    void genuineDependencyBackedExceptionStillProducesFullyQualifiedName()
    {
        setScenario("genuine_allowlisted_module_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(out.rawMessage != QStringLiteral("stub: real requests package unavailable in this environment"),
                 "the real `requests` package must be importable in this test environment for this test to be "
                 "meaningful — see the pystub's guarded import");
        QCOMPARE(out.exceptionType, QStringLiteral("requests.exceptions.ConnectionError"));
    }

    // Reviewer delta-fix #5 bypass (a) — __module__ is entirely absent
    // (attribute access raises), so moduleName is empty; delta-fix #4's
    // allowlist check only fired for a NON-empty moduleName, so this fell
    // through unguarded to the tp_name fallback. The fixture crafts tp_name
    // to a grammar-valid, secret-shaped string ("sk_live_secret_abc") via
    // type()'s 3-arg form (same technique delta-fix #2's TpNameFallbackError
    // used legitimately) to prove tp_name is exactly as attacker-settable as
    // moduleName. Must fold to foreign_exception, not surface tp_name.
    void moduleAbsentWithCraftedTpNameFoldsToForeignException()
    {
        setScenario("module_absent_tp_name_bypass_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("sk_live_secret_abc")),
                 "an absent __module__ must not let a crafted tp_name substitute for module validation");
    }

    // Reviewer delta-fix #5 bypass (b) — __module__ resolves to a non-str
    // object (delta-fix #1 shape, so moduleName is empty), but __qualname__
    // is reassigned to a genuine, grammar-valid str ("hunter2"). The OLD
    // code defaulted an empty moduleName to "builtins" and concatenated
    // unconditionally, producing "builtins.hunter2" — falsely dressing an
    // adversarial name as a real builtins exception. Must fold to
    // foreign_exception instead.
    void moduleNonStrWithValidShapedQualnameFoldsToForeignException()
    {
        setScenario("module_non_str_qualname_bypass_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("hunter2")),
                 "an absent/non-str __module__ must not default to \"builtins\" and concatenate a "
                 "merely-valid-shaped __qualname__");
    }

    // Reviewer delta-fix #7 (security) — __module__ resolves to a genuinely
    // ALLOWLISTED string ("builtins", passing delta-fix #5's gate), but
    // __qualname__ is absent (forcing the tp_name-fallback branch), and the
    // type's tp_name is genuinely, really registered as an attribute of the
    // real, already-imported `os` module (a module root NOT on
    // kAllowedModuleRoots). isGenuineAllowlistedException()'s provenance
    // walk WOULD succeed here (the object really is there) — proving "real"
    // and "allowlisted" are different guarantees, and that delta-fix #7's
    // new module-root check on tp_name closes a genuinely-leaking gap, not
    // one the provenance walk already caught for an unrelated reason (an
    // unbacked/fabricated tp_name would have been rejected regardless,
    // making RED/GREEN unobservable — this fixture avoids that).
    void tpNameModuleRootNotAllowlistedFoldsToForeignException()
    {
        setScenario("tp_name_not_allowlisted_error");

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QCOMPARE(out.exceptionType, QStringLiteral("foreign_exception"));
        QVERIFY2(!out.exceptionType.contains(QStringLiteral("os.TpNameNotAllowlistedError")),
                 "an allowlisted __module__ claim must not let a non-allowlisted tp_name substitute for "
                 "module validation, even if that tp_name is genuinely registered");
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

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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

        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));

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
    // T-036 / REQ-003 (MFA) — real embedded-Python MFA bridge coverage
    // (A3-R003-01). Exercises the PRODUCTION PyEmbeddedAdapter MFA path against
    // the real CPython bridge (not the Python-free fake used on garmin-fast):
    //   - authenticate() where the stub login() returns the
    //     {"mfa_required": True} sentinel exercises the real
    //     PyDict_GetItemString(...,"mfa_required") detection + m_client retention
    //     (Py_INCREF/XDECREF) → PyAuthOutcome::MfaRequired.
    //   - submitMfa("…") on the SAME adapter exercises the real submit_mfa
    //     marshalling + refcounting → Success with the identity fields, or
    //     AuthFailed when the stub raises GarminError(kind='auth').
    //   - submitMfa with no retained session → the mapped Unknown failure.
    // These are coverage of already-shipped code: passing first run means the
    // bridge is sound; a failure would expose a latent refcount/dict-key bug.
    // ==================================================================

    // authenticate() MFA sentinel → MfaRequired, and submitMfa() success on the
    // retained client marshals the identity + records the code verbatim.
    void mfaRequiredSentinelThenSubmitMfaSuccessMarshalsIdentity()
    {
        setScenario("mfa_required");
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));
        QCOMPARE(out.kind, PyAuthOutcome::MfaRequired);
        QVERIFY2(out.garmin_user_id.isEmpty(), "MfaRequired carries no identity yet");

        setScenario("mfa_success");
        const PyAuthOutcome ok = adapter.submitMfa(QStringLiteral("246810"));
        QCOMPARE(ok.kind, PyAuthOutcome::Success);
        QCOMPARE(ok.garmin_user_id, QStringLiteral("uid-mfa-77"));
        QCOMPARE(ok.display_name, QStringLiteral("MFA Rider"));
        QCOMPARE(stubAttr("LAST_MFA_CODE"), QStringLiteral("246810"));
    }

    // submitMfa() where the stub raises GarminError(kind='auth') → AuthFailed
    // (classified by exception TYPE, LSN-006), raw message forwarded untranslated.
    void submitMfaAuthErrorMapsToAuthFailed()
    {
        setScenario("mfa_required");
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        QCOMPARE(adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw")).kind,
                 PyAuthOutcome::MfaRequired);

        setScenario("mfa_auth_error");
        const PyAuthOutcome out = adapter.submitMfa(QStringLiteral("000000"));
        QCOMPARE(out.kind, PyAuthOutcome::AuthFailed);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: invalid one-time code"));
    }

    // submitMfa() with NO retained session (authenticate never established one) →
    // Unknown (a contract/order error, NEVER a spurious Success), no crash.
    void submitMfaWithoutPendingSessionMapsToUnknownNotSuccess()
    {
        setScenario("mfa_success");
        PyEmbeddedAdapter adapter(
            GarminPyModulePath::explicitOverride(kStubsDir)); // never authenticated → no retained client
        const PyAuthOutcome out = adapter.submitMfa(QStringLiteral("123456"));
        QVERIFY2(out.kind != PyAuthOutcome::Success, "submitMfa without a pending session must NOT succeed");
        QCOMPARE(out.kind, PyAuthOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "must explain why the OTP could not be submitted");
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir)); // never authenticated
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
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("dl_success");
        PyDownloadOutcome out;
        std::thread worker([&] { out = adapter.downloadActivity(QStringLiteral("42"), QStringLiteral("ORIGINAL")); });
        worker.join();

        QCOMPARE(out.kind, PyDownloadOutcome::Success);
        QCOMPARE(out.data, kExpectedDownload);
    }

    // ==================================================================
    // TEST-043 / REQ-008 Slice A — listActivitiesSince marshalling (DES-013
    // extension, same DEC-013 seam one op sideways). PyEmbeddedAdapter
    // .listActivitiesSince() forwards `sinceGmt` VERBATIM (DES-010 — Garmin's
    // server-side timestamp, never the local clock) to the authenticated
    // GarminClient's list_activities_since(), marshals the returned iterator of
    // summary dicts into a QVector<GarminActivitySummary> (activityId +
    // startTimeGMT exact, str-normalized), and classifies failures by exception
    // TYPE then .kind (LSN-006): connection→Network, rate_limit→RateLimited,
    // foreign / non-iterable / unknown-kind → Unknown — NEVER a spurious Success.
    //
    // Session model (DES-013): listing reuses the client authenticate()
    // established and the adapter retains — REQ-005 forbids keeping the password.
    // Each slot therefore authenticates (success) first.
    // ==================================================================

    // (a) success: the iterator of dicts marshals to summaries with activityId +
    // startTimeGMT exact (int activityId str-normalized), and the since-timestamp
    // reaches the stub verbatim (DES-010).
    void listSuccessMarshalsSummariesAndForwardsTimestampVerbatim()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        QCOMPARE(adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw")).kind,
                 PyAuthOutcome::Success);

        setScenario("list_success");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QCOMPARE(out.kind, PyListOutcome::Success);
        QCOMPARE(out.activities.size(), 2);
        QCOMPARE(out.activities.at(0).activityId, QStringLiteral("1001"));
        QCOMPARE(out.activities.at(0).startTimeGMT, QStringLiteral("2026-07-01 06:30:00"));
        // T-210 / DEC-056 — startTimeLocal marshals through DISTINCT from
        // startTimeGMT, proving the adapter reads the right dict key.
        QCOMPARE(out.activities.at(0).startTimeLocal, QStringLiteral("2026-07-01 08:30:00"));
        QCOMPARE(out.activities.at(1).activityId, QStringLiteral("1002"));
        QCOMPARE(out.activities.at(1).startTimeGMT, QStringLiteral("2026-07-03 18:05:11"));
        QCOMPARE(out.activities.at(1).startTimeLocal, QStringLiteral("2026-07-03 20:05:11"));
        QCOMPARE(stubAttr("LAST_SINCE_GMT"), QStringLiteral("2026-06-30 00:00:00"));
    }

    // T-210 / DEC-056 — startTimeLocal is OPTIONAL: a summary dict that omits
    // the key entirely must marshal to an empty QString (toQString's borrowed-
    // null branch), never crash and never fail the listing.
    void listMissingStartTimeLocalMarshalsToEmptyStringNotCrash()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_missing_start_time_local");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QCOMPARE(out.kind, PyListOutcome::Success);
        QCOMPARE(out.activities.size(), 1);
        QCOMPARE(out.activities.at(0).activityId, QStringLiteral("1001"));
        QVERIFY2(out.activities.at(0).startTimeLocal.isEmpty(),
                 "a summary dict with no startTimeLocal key must marshal to an empty QString");
    }

    // (b) empty listing → Success with an empty vector (DES-009/DES-010: a normal
    // "nothing newer" result, NOT a failure).
    void listEmptyResultIsSuccessNotFailure()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_empty");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QCOMPARE(out.kind, PyListOutcome::Success);
        QVERIFY2(out.activities.isEmpty(), "an empty listing is a normal success, not a failure");
    }

    // (c) GarminError kind='connection' → Network, raw message forwarded.
    void listConnectionErrorMapsToNetwork()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_connection");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QCOMPARE(out.kind, PyListOutcome::Network);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: listing connection refused"));
    }

    // (d) GarminError kind='rate_limit' → RateLimited — a DISTINCT kind, not
    // collapsed to Unknown or misrouted to Network (DES-008 rate-limit copy).
    void listRateLimitErrorMapsToRateLimited()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_rate_limit");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QVERIFY2(out.kind != PyListOutcome::Unknown, "rate_limit must be its own kind, not Unknown");
        QVERIFY2(out.kind != PyListOutcome::Network, "rate_limit must not be misrouted to Network");
        QCOMPARE(out.kind, PyListOutcome::RateLimited);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: listing rate-limited"));
    }

    // (e) a non-GarminError exception (ValueError) → Unknown, NEVER Success
    // (LSN-006: classify by type; a foreign exception is not a valid listing).
    void listForeignExceptionMapsToUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_value_error");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QVERIFY2(out.kind != PyListOutcome::Success, "a foreign exception must NOT be reported as a Success");
        QCOMPARE(out.kind, PyListOutcome::Unknown);
        QVERIFY2(out.rawMessage.contains(QStringLiteral("not a garmin error (listing)")),
                 "rawMessage should carry str(e) of the foreign exception");
    }

    // (f) a non-iterable return (contract breach of the DES-012 seam) → Unknown,
    // NEVER a Success with empty summaries. Kills a mutant that skips the check.
    void listNonIterableResultYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_non_iterable");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QVERIFY2(out.kind != PyListOutcome::Success, "a non-iterable result must NOT be reported as Success");
        QCOMPARE(out.kind, PyListOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "a non-iterable result must carry an explanatory message");
    }

    // TEST-053 / F3 (A3-R008 adversary gap) — the list IS iterable but yields a
    // NON-dict item mid-stream. PyEmbeddedAdapter's per-item PyDict_Check guard
    // (PyEmbeddedAdapter.cpp) must fold the WHOLE listing to Unknown — NEVER a
    // Success carrying a phantom empty-id/empty-timestamp row synthesized from the
    // non-dict. Mutation-confirmed: deleting the guard makes this test see
    // Success (kind==0) with a phantom entry, so it FAILS; restoring it passes.
    void listBadItemFoldsToUnknown()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_bad_item");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QVERIFY2(out.kind != PyListOutcome::Success, "a list with a non-dict item must NOT be reported as Success");
        QCOMPARE(out.kind, PyListOutcome::Unknown); // enum value 3
        QVERIFY2(out.activities.isEmpty(), "a folded-to-Unknown listing must carry no (phantom) summaries");
        QVERIFY2(!out.rawMessage.isEmpty(), "a non-dict list item must carry an explanatory message");
    }

    // (g) listing before any successful authenticate → Unknown (no retained
    // session), never a crash and never a Success.
    void listWithoutAuthenticateYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir)); // never authenticated
        setScenario("list_success");
        const PyListOutcome out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00"));

        QVERIFY2(out.kind != PyListOutcome::Success, "listing without a session must NOT succeed");
        QCOMPARE(out.kind, PyListOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "must explain why the listing could not run");
    }

    // (h) the production call pattern: authenticate on this thread, then list
    // from a non-main worker-like std::thread. PyGILState_Ensure must acquire the
    // GIL there and marshal the identical summaries (REQ-NF-Threads-001 proof at
    // the real-bridge level).
    void listFromWorkerThreadMarshalsSameSummaries()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("list_success");
        PyListOutcome out;
        std::thread worker([&] { out = adapter.listActivitiesSince(QStringLiteral("2026-06-30 00:00:00")); });
        worker.join();

        QCOMPARE(out.kind, PyListOutcome::Success);
        QCOMPARE(out.activities.size(), 2);
        QCOMPARE(out.activities.at(0).activityId, QStringLiteral("1001"));
    }

    // ==================================================================
    // REQ-013 (DEC-050 first slice) — fetchProfile marshalling (DES-013
    // extension, same DEC-013 seam one op sideways). PyEmbeddedAdapter
    // .fetchProfile() calls the authenticated GarminClient's get_profile()
    // and marshals whichever of dob/weight_kg/height_cm are present in the
    // returned dict — a dict missing some or all of the 3 keys is a NORMAL
    // Success (DEC-050: Garmin not having a field is expected, not an
    // error), never folded into a failure. Classifies failures by exception
    // TYPE (LSN-006): connection→Network, anything else/foreign→Unknown
    // (this outcome has no dedicated RateLimit kind — see
    // IGarminPyAdapter.h). hr_max/ftp_w are explicitly OUT of this slice
    // (DES-011 Scope) and have no fields to marshal.
    //
    // Session model (DES-013): fetchProfile reuses the client authenticate()
    // established — REQ-005 forbids keeping the password. Each slot
    // therefore authenticates (success) first.
    // ==================================================================

    // (a) all 3 fields present and sane → Success with all has* flags true
    // and the exact values marshalled through.
    void profileSuccessMarshalsAllThreeFields()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        QCOMPARE(adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw")).kind,
                 PyAuthOutcome::Success);

        setScenario("profile_full");
        const PyProfileOutcome out = adapter.fetchProfile();

        QCOMPARE(out.kind, PyProfileOutcome::Success);
        QVERIFY(out.hasDob);
        QCOMPARE(out.dob, QStringLiteral("1985-06-15"));
        QVERIFY(out.hasWeightKg);
        QCOMPARE(out.weightKg, 72.5);
        QVERIFY(out.hasHeightCm);
        QCOMPARE(out.heightCm, 178.0);
    }

    // (b) only dob present → Success, with weight/height has* flags false —
    // a partial result is still a normal Success, not a failure.
    void profilePartialResultLeavesMissingFieldsUnset()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_partial");
        const PyProfileOutcome out = adapter.fetchProfile();

        QCOMPARE(out.kind, PyProfileOutcome::Success);
        QVERIFY(out.hasDob);
        QCOMPARE(out.dob, QStringLiteral("1990-01-02"));
        QVERIFY2(!out.hasWeightKg, "an absent field must leave has* false, not a fabricated 0.0");
        QVERIFY2(!out.hasHeightCm, "an absent field must leave has* false, not a fabricated 0.0");
    }

    // (c) DEC-050's single most important case: a dict with NONE of the 3
    // fields still marshals to a clean Success with every has* flag false —
    // never a crash, never a failure signal.
    void profileEmptyResultIsSuccessWithNoFieldsSet()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_empty");
        const PyProfileOutcome out = adapter.fetchProfile();

        QCOMPARE(out.kind, PyProfileOutcome::Success);
        QVERIFY(!out.hasDob);
        QVERIFY(!out.hasWeightKg);
        QVERIFY(!out.hasHeightCm);
    }

    // (d) GarminError kind='connection' → Network, raw message forwarded.
    void profileConnectionErrorMapsToNetwork()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_connection");
        const PyProfileOutcome out = adapter.fetchProfile();

        QCOMPARE(out.kind, PyProfileOutcome::Network);
        QCOMPARE(out.rawMessage, QStringLiteral("stub: profile connection refused"));
    }

    // (e) a non-GarminError exception (ValueError) → Unknown, NEVER Success
    // (LSN-006: classify by type; a foreign exception is not a valid profile).
    void profileForeignExceptionMapsToUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_value_error");
        const PyProfileOutcome out = adapter.fetchProfile();

        QVERIFY2(out.kind != PyProfileOutcome::Success, "a foreign exception must NOT be reported as a Success");
        QCOMPARE(out.kind, PyProfileOutcome::Unknown);
        QVERIFY2(out.rawMessage.contains(QStringLiteral("not a garmin error (profile)")),
                 "rawMessage should carry str(e) of the foreign exception");
    }

    // (f) a non-dict return (contract breach of the DES-012 seam) → Unknown,
    // NEVER a Success with fabricated fields.
    void profileNonDictResultYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_non_dict");
        const PyProfileOutcome out = adapter.fetchProfile();

        QVERIFY2(out.kind != PyProfileOutcome::Success, "a non-dict result must NOT be reported as Success");
        QCOMPARE(out.kind, PyProfileOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "a non-dict result must carry an explanatory message");
    }

    // (g) fetchProfile before any successful authenticate → Unknown (no
    // retained session), never a crash and never a Success.
    void profileWithoutAuthenticateYieldsUnknownNotSuccess()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir)); // never authenticated
        setScenario("profile_full");
        const PyProfileOutcome out = adapter.fetchProfile();

        QVERIFY2(out.kind != PyProfileOutcome::Success, "fetchProfile without a session must NOT succeed");
        QCOMPARE(out.kind, PyProfileOutcome::Unknown);
        QVERIFY2(!out.rawMessage.isEmpty(), "must explain why the profile fetch could not run");
    }

    // (h) the production call pattern: authenticate on this thread, then
    // fetch the profile from a non-main worker-like std::thread. PyGILState_Ensure
    // must acquire the GIL there and marshal the identical fields.
    void profileFromWorkerThreadMarshalsSameFields()
    {
        PyEmbeddedAdapter adapter(GarminPyModulePath::explicitOverride(kStubsDir));
        setScenario("success");
        adapter.authenticate(QStringLiteral("rider@example.com"), QStringLiteral("pw"));

        setScenario("profile_full");
        PyProfileOutcome out;
        std::thread worker([&] { out = adapter.fetchProfile(); });
        worker.join();

        QCOMPARE(out.kind, PyProfileOutcome::Success);
        QVERIFY(out.hasDob);
        QCOMPARE(out.dob, QStringLiteral("1985-06-15"));
    }

    void cleanupTestCase()
    {
        // Re-acquire the main thread state saved in initTestCase, then
        // finalize. If this ever proves flaky on CI the finalize may be
        // skipped (process exit reclaims everything) — see briefing note.
        if (mainState) {
            PyEval_RestoreThread(mainState);
            // DEC-066 constraint 2: the ledger's strong references must not
            // outlive this interpreter's finalization.
            PyEmbeddedAdapter::releaseModuleProvenanceLedgerForCurrentInterpreter();
            Py_FinalizeEx();
        }
    }
};

QTEST_GUILESS_MAIN(TestGarminConnectPyAdapter)
#include "testGarminConnectPyAdapter.moc"
