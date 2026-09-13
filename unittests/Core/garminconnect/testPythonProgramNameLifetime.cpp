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

// B-STAGE9-07 — regression for the pre-existing dangling-pointer defect at
// PythonEmbed.cpp's old inline call:
//
//     Py_SetProgramName((wchar_t*) pybin.toStdWString().c_str());
//
// The std::wstring returned by toStdWString() is a temporary; its buffer died
// at the end of the full expression, while the API's documented contract (and
// its actual implementation on Python <= 3.12 — including the 3.11
// GoldenCheetah builds against elsewhere, see commit e0a198a16) BORROWS that
// pointer for the interpreter's entire lifetime. Python 3.13 changed the
// function to copy its argument, so against THIS build's libpython the defect
// is no longer observable at runtime (verified empirically before this test
// was written: a freed+clobbered source buffer does not affect what 3.13's
// Py_GetProgramName returns).
//
// That is exactly why this test does NOT call the real Py_SetProgramName: it
// interposes a link-seam definition implementing the WORST-CASE (<= 3.12)
// contract — store the pointer at call time, dereference it only afterwards —
// so the storage obligation is EXECUTED, not assumed (A3-R017's standing
// rule: a lifetime claim must be judged by the memory model, and a test that
// passes because the freed bytes happened to survive proves nothing). Under
// the pre-fix wrapper the deferred read is an ASan heap-use-after-free and
// this binary aborts; under the fix it reads back intact.
//
// The unit under test is the REAL PyProcessBootstrap::setProgramName() that
// PythonEmbed's constructor now calls (the tree's only Py_SetProgramName call
// site). PythonEmbed.cpp itself cannot be linked into a unittest (it needs
// the appsettings/Gui application layers), which is why the wrapper lives in
// PyProcessBootstrap — see its header.
//
// This executable links a real CPython (Python3::Python) for
// PyProcessBootstrap's remaining symbol surface and therefore carries the
// `garmin-py` CTest label — it must NEVER carry `garmin-fast` (Python-free by
// invariant). LSN-007: Python.h precedes any Qt header.

// clang-format off
#include <Python.h>
// clang-format on

#include "PyProcessBootstrap.h"

#include <QString>
#include <QtTest/QtTest>

#include <cstddef>
#include <string>
#include <vector>

#ifndef __has_feature
#    define __has_feature(x) 0
#endif
#if !__has_feature(address_sanitizer) && !defined(__SANITIZE_ADDRESS__)
#    error \
        "testPythonProgramNameLifetime is a lifetime test: it must be compiled with -fsanitize=address (precedent: testGarminConnectSyncDialogClose)."
#endif

namespace {

// The link-seam stand-in for CPython's Py_SetProgramName(). The executable's
// own definition wins over the shared libpython's at link time, so the REAL
// PyProcessBootstrap.cpp (compiled unmodified into this target) calls THIS
// one. If the seam ever fails to interpose, slot 1's count assertion fails
// loudly rather than letting the suite pass vacuously.
std::vector<const wchar_t*> g_borrowedProgramNames;

// Deliberately longer than std::wstring's SSO capacity (~15 wchar_t) so the
// converted name is heap-allocated: a freed heap buffer is ASan-quarantined
// and poisoned, making the RED deterministic. Real pybin paths (e.g.
// /opt/python3.11/bin/python3.11) are likewise over SSO.
QString probeName(const char* suffix)
{
    QString name = QStringLiteral("/opt/python3.11/bin/python3.11-") + QString::fromLatin1(suffix);
    while (name.size() < 96)
        name += QLatin1Char('p');
    return name;
}

// Reads through a pointer CPython was handed, AFTER the call returned — the
// way <= 3.12 reads it at Py_Initialize()/Py_GetProgramName() time. Under the
// pre-fix wrapper this loop's first read is the heap-use-after-free ASan
// aborts on; every read here happens in instrumented test code on purpose.
void readBorrowedEquals(const wchar_t* borrowed, const QString& expected)
{
    const std::wstring w = expected.toStdWString();
    for (std::size_t i = 0; i < w.size(); ++i) {
        QVERIFY2(borrowed[i] == w[i],
                 qPrintable(QStringLiteral("borrowed storage diverged from the name at wchar %1").arg(i)));
    }
    QVERIFY2(borrowed[w.size()] == L'\0', "borrowed storage is not zero-terminated where the name ended");
}

} // namespace

// Must match Python.h's declaration exactly (Py_DEPRECATED(3.11) warns at
// call sites only — PythonEmbed.cpp has carried that warning for years).
extern "C" void Py_SetProgramName(const wchar_t* program_name)
{
    g_borrowedProgramNames.push_back(program_name);
}

class TestPythonProgramNameLifetime : public QObject
{
    Q_OBJECT

  private slots:

    // The original B-STAGE9-07 defect, verbatim shape: the name handed to
    // Py_SetProgramName must still read back intact AFTER the call has
    // returned and the caller's full expression is over.
    void programNameOutlivesTheCall()
    {
        const int recordedBefore = int(g_borrowedProgramNames.size());
        const QString name = probeName("lifetime");

        PyProcessBootstrap::setProgramName(name);

        QVERIFY2(int(g_borrowedProgramNames.size()) == recordedBefore + 1,
                 "link seam did not interpose — PyProcessBootstrap called the real "
                 "libpython Py_SetProgramName, this test is vacuous as-is");
        readBorrowedEquals(g_borrowedProgramNames.back(), name);
    }

    // The in-process restart shape: main.cpp's do{}while(restarting) loop can
    // construct a second PythonEmbed (same process, interpreter already up),
    // which calls setProgramName again. Storage handed over by an EARLIER
    // call must stay valid — a single retained buffer that reallocates on the
    // second call would re-create the same dangling pointer for a name
    // CPython already holds. Hence every handed-over buffer must be retained,
    // not just the latest.
    void repeatedCallsKeepEarlierStorageAlive()
    {
        const QString first = probeName("first");
        const QString second = probeName("second-name-of-a-different-length");

        PyProcessBootstrap::setProgramName(first);
        const wchar_t* firstBorrowed = g_borrowedProgramNames.back();
        PyProcessBootstrap::setProgramName(second);

        readBorrowedEquals(firstBorrowed, first); // earlier storage, still intact
        readBorrowedEquals(g_borrowedProgramNames.back(), second);
    }
};

QTEST_APPLESS_MAIN(TestPythonProgramNameLifetime)
#include "testPythonProgramNameLifetime.moc"
