/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// T-203 — REQ-NF-Sec-001 (prd.md:104) "Password never persisted; only tokens."
//
// The STATIC half of this guard lives in unittests/buildguard/
// garmin_sec_source_guard.py (testGarminSecSourceGuard — dod.md's "grep
// test"): it forbids a password identifier touching a persistence API in the
// Garmin source tree, line-locally. THIS executable is the RUNTIME half: it
// proves the authenticate() flow — the one frame where the password is
// genuinely in memory — writes the password's BYTES to NO file at all, even
// one a line-local scan could never attribute (a member captured now and
// flushed later, a composed buffer, a dependency-side dump).
//
// Harness: HOME / XDG_* / TMPDIR are redirected into a private QTemporaryDir
// BEFORE Py_Initialize (env does not propagate into os.environ afterwards),
// then a REAL embedded CPython runs the production PyEmbeddedAdapter against
// the pystubs/garmin_client.py stub. After authenticate() the ENTIRE sentinel
// tree is byte-scanned for the password (UTF-8 and UTF-16LE forms).
//
// The scan is prevented from passing vacuously by a canary file written under
// the redirected HOME: if the scan cannot find the canary's marker bytes, the
// redirection or the walker is broken and the test fails on that, not on the
// password assertion.
//
// Scope boundaries (deliberate): the stub module's own behaviour is trusted
// only insofar as the scan observes it — the stub records the password in
// MEMORY (LAST_PASSWORD, asserted verbatim to prove the marshalling under
// test really happened) but writes nothing; the REAL python-garminconnect
// dependency's own file writes are covered on the pytest side
// (tests/test_token_store.py::test_dumped_blob_never_contains_the_password)
// and by this test only insofar as garmin_client.py forwards to them.
// Bytecode caches (pystubs/__pycache__) live in-tree, outside the sentinel
// root, and cannot contain an argument value — not scanned, by design.
//
// Labels: garmin-sec-guard. Links real CPython (Python3::Python) like
// testGarminConnectPyAdapter, so it must NEVER carry `garmin-fast` (that
// label is Python-free by invariant).
//
// Cites:
//   REQ-NF-Sec-001 (prd.md:104) — password never persisted; only tokens
//   REQ-005 — password never retained after auth (the in-memory sibling rule)
//   REQ-NF-Sec-002 — token FILE MODE 0600; already covered by
//     testGarminTokenStore/testAtomicFile/testGarminSidecarStore — NOT this
//     test's subject; this test asserts CONTENT, never modes.
//   DES-012 / DES-013 — adapter speaks only to garmin_client via the
//     embedded interpreter

// clang-format off
// Python.h must precede any Qt header (Qt's `slots` macro vs object.h's
// `slots` field) — protected from include re-sorting; the test harness owns
// the interpreter lifecycle.
#include <Python.h>
// clang-format on

#include "IGarminPyAdapter.h"
#include "PyEmbeddedAdapter.h"

#include <QByteArray>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#ifndef GARMIN_PYSTUBS_DIR
#    error "GARMIN_PYSTUBS_DIR must be defined by the CMake target (path to pystubs/)"
#endif

namespace {

// Run a snippet of Python under a properly acquired GIL (same idiom as
// testGarminConnectPyAdapter).
void runPy(const char* code)
{
    PyGILState_STATE st = PyGILState_Ensure();
    PyRun_SimpleString(code);
    PyGILState_Release(st);
}

// Read a string attribute off the (already imported) stub module.
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

void setScenario(const char* scenario)
{
    const QString code =
        QStringLiteral("import sys\n"
                       "_d = %1\n"
                       "if _d not in sys.path:\n"
                       "    sys.path.insert(0, _d)\n"
                       "import gc_garmin_adapter.garmin_client\n"
                       "gc_garmin_adapter.garmin_client.SCENARIO = '%2'\n")
            .arg(QStringLiteral("r'''") + QString::fromUtf8(GARMIN_PYSTUBS_DIR) + QStringLiteral("'''"),
                 QString::fromUtf8(scenario));
    runPy(code.toUtf8().constData());
}

// The sentinel secret: ASCII plus a non-ASCII UTF-8 byte pair, so an encoding
// bug (Latin-1 truncation, QString narrowing) cannot silently dodge the scan.
const char kSentinelPasswordRaw[] = "T203-s3nt!nel-P4ss-"
                                    "\xCF"
                                    "\x80"
                                    "-never-persist";

const QString kSentinelPassword = QString::fromUtf8(kSentinelPasswordRaw);
const char kCanaryMarker[] = "T203-CANARY-MARKER-4815162342";

// Byte-wise search of `needle` across EVERY file under `root` (incl. hidden).
// Sets *filesScanned and, on a hit, *hitPath. Depth is bounded by the tree the
// redirected env vars produce — a handful of files.
bool treeContainsBytes(const QString& root, const QByteArray& needle, int* filesScanned, QString* hitPath)
{
    *filesScanned = 0;
    QDirIterator it(root, QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        const QByteArray bytes = f.readAll();
        ++*filesScanned;
        if (bytes.contains(needle)) {
            *hitPath = path;
            return true;
        }
    }
    return false;
}

// The same password as UTF-16LE bytes — what a QString::toUtf16-style raw
// flush would land on disk as.
QByteArray utf16LeBytes(const QString& s)
{
    const ushort* u = s.utf16();
    QByteArray out;
    out.reserve(int(s.size()) * 2);
    for (int i = 0; i < s.size(); ++i) {
        out.append(char(u[i] & 0xFF));
        out.append(char((u[i] >> 8) & 0xFF));
    }
    return out;
}

} // namespace

class TestGarminConnectPasswordPersistence : public QObject
{
    Q_OBJECT

    PyThreadState* mainState = nullptr;
    QTemporaryDir sentinelRoot;
    QString homeDir;

  private slots:

    void initTestCase()
    {
        QVERIFY2(!Py_IsInitialized(), "harness precondition: interpreter must not be up yet");
        QVERIFY(sentinelRoot.isValid());

        // Redirect every location a persistence bug could plausibly hit BEFORE
        // the interpreter exists (env vars do not propagate into os.environ
        // after Py_Initialize). QTemporaryDir was created above, against the
        // ORIGINAL TMPDIR — the redirect for the interpreter comes after.
        homeDir = sentinelRoot.filePath(QStringLiteral("home"));
        QVERIFY(QDir().mkpath(homeDir));
        QVERIFY(qputenv("HOME", homeDir.toUtf8()));
        QVERIFY(qputenv("XDG_CONFIG_HOME", sentinelRoot.filePath(QStringLiteral("xdg-config")).toUtf8()));
        QVERIFY(qputenv("XDG_DATA_HOME", sentinelRoot.filePath(QStringLiteral("xdg-data")).toUtf8()));
        QVERIFY(qputenv("XDG_CACHE_HOME", sentinelRoot.filePath(QStringLiteral("xdg-cache")).toUtf8()));
        QVERIFY(qputenv("TMPDIR", sentinelRoot.filePath(QStringLiteral("tmp")).toUtf8()));

        Py_Initialize();
        QVERIFY(Py_IsInitialized());
        mainState = PyEval_SaveThread();

        setScenario("success");

        // Prove the redirect reached the interpreter's own environment view.
        // (After setScenario: the stub module is importable only once its dir
        // is on sys.path.)
        runPy("import os, gc_garmin_adapter.garmin_client\n"
              "gc_garmin_adapter.garmin_client.T203_PY_HOME = os.environ.get('HOME', '')\n");
        QCOMPARE(stubAttr("T203_PY_HOME"), homeDir);
    }

    void cleanupTestCase()
    {
        PyEval_RestoreThread(mainState);
        Py_Finalize();
    }

    // The core assertion: after a successful authenticate() through the REAL
    // production bridge, the sentinel tree contains the password's bytes in no
    // file, under any encoding a raw flush would produce.
    void authenticateWritesNoPasswordBytesToDisk()
    {
        PyEmbeddedAdapter adapter(QString::fromUtf8(GARMIN_PYSTUBS_DIR));

        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("t203@example.com"), kSentinelPassword);
        QCOMPARE(out.kind, PyAuthOutcome::Success);

        // The password genuinely crossed the bridge — without this, a silent
        // marshalling failure would make everything below pass vacuously.
        QCOMPARE(stubAttr("LAST_PASSWORD"), kSentinelPassword);
        QCOMPARE(stubAttr("LAST_EMAIL"), QStringLiteral("t203@example.com"));

        // Non-vacuous-scan proof: a canary under the redirected HOME must be
        // found by the same walker that is about to clear the password.
        const QString canaryDir = homeDir + QStringLiteral("/canary");
        QVERIFY(QDir().mkpath(canaryDir));
        const QString canaryPath = canaryDir + QStringLiteral("/t203-canary.txt");
        QFile canary(canaryPath);
        QVERIFY(canary.open(QIODevice::WriteOnly));
        QVERIFY(canary.write(kCanaryMarker) > 0);
        canary.close();

        int scanned = 0;
        QString hit;
        QVERIFY2(
            treeContainsBytes(sentinelRoot.filePath(QStringLiteral("home")), QByteArray(kCanaryMarker), &scanned, &hit),
            "scan must find the canary — walker or redirection is broken");
        QVERIFY2(scanned >= 1, "scan must have examined at least the canary file");

        // The invariant itself: no password bytes anywhere under the sentinel
        // root, as UTF-8 or UTF-16LE.
        QVERIFY2(!treeContainsBytes(sentinelRoot.path(), kSentinelPassword.toUtf8(), &scanned, &hit),
                 qPrintable(QStringLiteral("password persisted (UTF-8) to: %1").arg(hit)));
        QVERIFY2(!treeContainsBytes(sentinelRoot.path(), utf16LeBytes(kSentinelPassword), &scanned, &hit),
                 qPrintable(QStringLiteral("password persisted (UTF-16LE) to: %1").arg(hit)));
    }

    // The MFA window: login() returned mfa_required, so the password lingers
    // in scope while the user is mid-flow — the prime "cache it for the retry"
    // regression. No password bytes may land on disk here either.
    void mfaFlowWritesNoPasswordBytesToDisk()
    {
        setScenario("mfa_required");

        PyEmbeddedAdapter adapter(QString::fromUtf8(GARMIN_PYSTUBS_DIR));
        const PyAuthOutcome out = adapter.authenticate(QStringLiteral("t203@example.com"), kSentinelPassword);
        QCOMPARE(out.kind, PyAuthOutcome::MfaRequired);

        int scanned = 0;
        QString hit;
        QVERIFY2(!treeContainsBytes(sentinelRoot.path(), kSentinelPassword.toUtf8(), &scanned, &hit),
                 qPrintable(QStringLiteral("password persisted (UTF-8) during MFA flow: %1").arg(hit)));
        QVERIFY2(!treeContainsBytes(sentinelRoot.path(), utf16LeBytes(kSentinelPassword), &scanned, &hit),
                 qPrintable(QStringLiteral("password persisted (UTF-16LE) during MFA flow: %1").arg(hit)));

        setScenario("success");
    }
};

QTEST_MAIN(TestGarminConnectPasswordPersistence)
#include "testGarminConnectPasswordPersistence.moc"
