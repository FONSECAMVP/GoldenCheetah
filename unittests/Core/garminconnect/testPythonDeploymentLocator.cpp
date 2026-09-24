/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 */

// DEC-062 (B-STAGE9-39/B-STAGE9-40 remedy) — the defect this was written
// against: PythonEmbed and PyProcessBootstrap's shared bootstrap (main.cpp's
// GC_WANT_PYTHON-independent OR-bootstrap call) each ran their OWN copy of
// "how do I find Python", so they could compute DIFFERENT homes for the same
// process — and main.cpp's copy never even looked at the deployed payload,
// silently ignoring it whenever a host stdlib was visible on PATH. The
// extraction moves both initialisers onto ONE shared
// PythonDeploymentLocator::select() call; this test targets exactly the two
// properties that extraction is supposed to guarantee:
//
//   1. sameInputsYieldTheSameHome — select() is deterministic: two
//      independent calls with the same explicit input and the same
//      environment produce byte-identical Selections, and — modelling the
//      two real call sites — byte-identical PyProcessBootstrap::Config
//      fields. Two initialisers consuming the SAME function cannot then
//      diverge merely by being called from two different places, which is
//      the shape the original bug actually had (see PROVE IT below for the
//      mutation that reproduces it under this test).
//
//   2. inheritedPythonHomeIsNotIgnored — DEC-062's Amendment-2 premise
//      correction: the extraction preserves the pre-existing inherited-
//      PYTHONHOME fallback rather than erasing it. A locator that only
//      detects a deployed payload or a PATH interpreter, and drops the
//      inherited-environment candidate on the floor, regresses a case
//      PythonEmbed handled correctly before this decision.
//
// PROVE IT (both tests were run RED then GREEN against a real mutation,
// reverted before this file was committed): test 1 was made to FAIL by
// editing PythonDeploymentLocator::select()'s deployed-candidate branch to
// append QString::number(QCoreApplication::applicationPid()) to `home` only
// on odd-numbered calls (a static call counter) — modelling the original
// defect's shape, where the SAME call site can validly return two different
// answers depending on which caller reaches it first, or when. Test 2 was
// made to FAIL by deleting the inherited-PYTHONHOME branch of select()
// entirely (falling straight through to a bare PATH search) — modelling the
// literal regression DEC-062's amendment corrected against. Both mutations
// were isolated to select() in PythonDeploymentLocator.cpp; no other file
// was touched to produce either RED, and reverting that one function's body
// was sufficient to return both tests to GREEN.
//
// This target does not call ensureInitialized() and never brings a real
// interpreter up (PyProcessBootstrap::Config is used only as a plain struct
// here) — but PythonDeploymentLocator.cpp includes Python.h for
// PY_MAJOR_VERSION/PY_MINOR_VERSION (LSN-007: Python.h precedes any Qt
// header), so this links real CPython and carries the `garmin-py` label,
// never `garmin-fast`.

// clang-format off
#include <Python.h>
// clang-format on

#include "PyProcessBootstrap.h"
#include "PythonDeploymentLocator.h"
#include "Utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
#include <QtTest/QtTest>

// Link-seam for Utils::searchPath(), called by PythonDeploymentLocator's PATH-
// search branch. The real definition lives in src/Core/Utils.cpp, which pulls
// in GenericChart.h/RideMetric.h and their own dependency chain — far more
// than this lightweight, Utils.cpp-free test target needs. This is a faithful
// copy of that function's body (unchanged since it has no dependency on
// anything Utils.cpp otherwise provides): search each PATHSEP-separated
// directory in `path` for `binary`, requiring the executable bit when isexec.
namespace Utils {
QStringList searchPath(QString path, QString binary, bool isexec)
{
    QStringList returning, extend;
#ifdef Q_OS_WIN
    if (isexec) {
        extend << ".exe" << ".com";
    } else {
        extend << "";
    }
#else
    extend << "";
#endif
    foreach (QString dir, path.split(PATHSEP)) {
        foreach (QString ext, extend) {
            QString filename(dir + QDir::separator() + binary + ext);
            if (QFileInfo(filename).exists() && (!isexec || QFileInfo(filename).isExecutable()) &&
                !returning.contains(filename)) {
                returning << filename;
            }
        }
    }
    return returning;
}
} // namespace Utils

class TestPythonDeploymentLocator : public QObject
{
    Q_OBJECT

  private slots:

    // Property 1: determinism. Two independent calls to the ONE function
    // both real call sites now use must agree, given the same explicit
    // input and the same environment — otherwise the extraction has not
    // actually removed the possibility of the two initialisers disagreeing,
    // it has just moved the divergence risk into one function.
    void sameInputsYieldTheSameHome()
    {
        // Deliberately isolate from whatever PYTHONHOME the test-runner's own
        // environment happens to carry, so this test's environment-dependent
        // branch (candidate 3, inherited PYTHONHOME) does not leak in and
        // this test stays about determinism, not about what happens to be set.
        const bool hadEnv = qEnvironmentVariableIsSet("PYTHONHOME");
        const QByteArray savedEnv = qgetenv("PYTHONHOME");
        qunsetenv("PYTHONHOME");

        const PythonDeploymentLocator::Selection first = PythonDeploymentLocator::select(QString());
        const PythonDeploymentLocator::Selection second = PythonDeploymentLocator::select(QString());

        if (hadEnv)
            qputenv("PYTHONHOME", savedEnv);

        QCOMPARE(second.found, first.found);
        QCOMPARE(second.home, first.home);
        QCOMPARE(second.pybin, first.pybin);
        QCOMPARE(second.pypath, first.pypath);
        QCOMPARE(second.programName, first.programName);
        QCOMPARE(second.isDeployedPayload, first.isDeployedPayload);

        // Modelling the two real call sites: PythonEmbed's constructor and
        // main.cpp's OR-bootstrap call each build their OWN
        // PyProcessBootstrap::Config from the Selection they got. If those
        // two Configs can differ, PyProcessBootstrap::ensureInitialized()'s
        // cached-first-call semantics mean whichever caller loses the race
        // silently has no effect on the shared interpreter's actual home —
        // exactly DEC-062's "two initialisers can compute different homes"
        // defect.
        PyProcessBootstrap::Config fromFirst;
        fromFirst.home = first.home;
        fromFirst.programName = first.programName;

        PyProcessBootstrap::Config fromSecond;
        fromSecond.home = second.home;
        fromSecond.programName = second.programName;

        QCOMPARE(fromSecond.home, fromFirst.home);
        QCOMPARE(fromSecond.programName, fromFirst.programName);
    }

    // Property 2: DEC-062 Amendment 2's premise correction — "stops mutating
    // PYTHONHOME" must not become "ignores an inherited PYTHONHOME". The
    // fixture is the REAL system interpreter's own sys.prefix — a
    // self-contained home (stdlib and all) that PYTHONHOME can validly point
    // at, unlike a bare symlinked binary with no lib/ beside it — asked for
    // directly rather than this test binary's applicationDirPath() (which
    // has no opt/python3.<N> or Frameworks/Python.framework beside it, so a
    // found+correct result here can only have come from the
    // inherited-PYTHONHOME candidate, not the deployed one).
    void inheritedPythonHomeIsNotIgnored()
    {
        const QString systemPython3 = QStandardPaths::findExecutable(QStringLiteral("python3"));
        if (systemPython3.isEmpty())
            QSKIP("no system python3 on PATH to read sys.prefix from");

        QProcess prefixQuery;
        prefixQuery.setProgram(systemPython3);
        prefixQuery.setArguments({"-c", "import sys; print(sys.prefix)"});
        prefixQuery.start();
        QVERIFY2(prefixQuery.waitForFinished(4000), "timed out asking system python3 for sys.prefix");
        const QString sysPrefix = QString::fromUtf8(prefixQuery.readAllStandardOutput()).trimmed();
        if (sysPrefix.isEmpty())
            QSKIP("system python3 did not report a usable sys.prefix");

        const bool hadEnv = qEnvironmentVariableIsSet("PYTHONHOME");
        const QByteArray savedEnv = qgetenv("PYTHONHOME");
        qputenv("PYTHONHOME", sysPrefix.toUtf8());

        const PythonDeploymentLocator::Selection sel = PythonDeploymentLocator::select(QString());

        if (hadEnv)
            qputenv("PYTHONHOME", savedEnv);
        else
            qunsetenv("PYTHONHOME");

        QVERIFY2(sel.found, "inherited PYTHONHOME (the system interpreter's own sys.prefix) was not "
                            "found at all — select() ignored the environment variable entirely");
        QCOMPARE(sel.home, sysPrefix);
        QVERIFY2(!sel.isDeployedPayload, "sys.prefix home was misclassified as the deployed payload");
    }
};

// Custom main (replaces QTEST_APPLESS_MAIN): PythonDeploymentLocator::select()
// calls QCoreApplication::applicationDirPath() unconditionally (its deployed-
// candidate check), which needs a real QCoreApplication instance to answer
// correctly — same reasoning as testPyProcessBootstrap.cpp's own custom main.
int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    TestPythonDeploymentLocator tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}
#include "testPythonDeploymentLocator.moc"
