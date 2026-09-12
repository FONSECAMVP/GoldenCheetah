/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DEC-052 — see PyProcessBootstrap.h for the full contract this implements.

// clang-format off
// Python.h must precede any Qt header (Qt's `slots` keyword-macro collides
// with the `slots` field in CPython's object.h — same invariant as
// PyEmbeddedAdapter.cpp / LSN-007). Protected from include re-sorting.
#include <Python.h>
// clang-format on

#include "PyProcessBootstrap.h"

#include <QMutex>
#include <QMutexLocker>

namespace PyProcessBootstrap {

namespace {

QMutex g_mutex;
bool g_attempted = false; // true once the first ensureInitialized() call has run to completion
Result g_result;

} // namespace

Result ensureInitialized(const Config &cfg)
{
    QMutexLocker locker(&g_mutex);

    if (g_attempted)
        return g_result; // cached — including a prior failure; no retry storms

    g_attempted = true;

    if (Py_IsInitialized()) {
        // Something else (e.g. a test harness exercising DES-013's pre-init
        // fail-safe deliberately, like testGarminConnectPyAdapter.cpp's
        // initTestCase()) already brought the interpreter up outside this
        // module. This module doesn't own that lifecycle, so it never runs
        // preInitHook here (genuinely too late — the interpreter's inittab is
        // long since fixed).
        //
        // B-STAGE9-05 (supersedes the B-STAGE9-04 fix below, which was itself
        // still wrong): this module must not touch GIL state AT ALL on this
        // path, in either direction. PyGILState_Check() can only tell us
        // whether THIS thread currently holds the GIL — never WHETHER this
        // call is the one entitled to release it. A calling thread can hold
        // the GIL for its own, completely unrelated reasons (e.g. it is
        // already inside its own PyGILState_Ensure() scope for other work)
        // at the exact moment it happens to be the one that reaches this
        // branch; this module has no way to distinguish that from "the GIL
        // happens to be held incidentally because whatever externally
        // initialized Python also left it held". Guessing either way is
        // wrong, so we never guess: we leave the calling thread's GIL state
        // exactly as found, in both directions, and let the caller's own
        // PyGILState_Ensure()/Release() pairing (never PyEval_SaveThread(),
        // which requires the caller to already know and own the thread
        // state) be the only thing that ever touches it. See
        // PyProcessBootstrap.h's narrowed GIL postcondition.
        g_result.ok = true;
        g_result.error.clear();
        return g_result;
    }

    if (cfg.preInitHook != nullptr)
        cfg.preInitHook();

    PyConfig config;
    PyConfig_InitPythonConfig(&config); // non-isolated: user site-packages ON by default

    // DEC-052: disable signal-handler installation (matches the pre-existing
    // Py_InitializeEx(0) behavior PythonEmbed relied on) and explicitly
    // preserve the user site-packages directory (~/.local/lib/python3.13/
    // site-packages, where garminconnect/curl_cffi live) rather than
    // depending on PyConfig_InitPythonConfig's default staying non-isolated.
    config.install_signal_handlers = 0;
    config.user_site_directory = 1;

    PyStatus status = PyConfig_Read(&config);
    if (PyStatus_Exception(status)) {
        g_result.ok = false;
        g_result.error = QString::fromUtf8(status.err_msg != nullptr ? status.err_msg
                                                                       : "PyConfig_Read failed");
        PyConfig_Clear(&config);
        return g_result;
    }

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);

    if (PyStatus_Exception(status)) {
        g_result.ok = false;
        g_result.error = QString::fromUtf8(status.err_msg != nullptr ? status.err_msg
                                                                       : "Py_InitializeFromConfig failed");
        return g_result;
    }

    // This IS the initializing call (we hold g_mutex and just performed the
    // one-and-only Py_InitializeFromConfig above) — release the GIL from
    // this thread so PyGILState_Ensure() works from any thread afterward.
    // No other call path ever reaches this line (see the g_attempted guard
    // and the Py_IsInitialized() early-return above).
    PyEval_SaveThread();

    g_result.ok = true;
    g_result.error.clear();
    return g_result;
}

bool isInitialized()
{
    return Py_IsInitialized() != 0;
}

} // namespace PyProcessBootstrap
