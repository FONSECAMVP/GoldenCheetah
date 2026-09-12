/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DEC-052 — shared, process-level CPython bootstrap. Fixes B-STAGE9-01: with
// GC_WANT_PYTHON off, nothing in the tree ever called Py_Initialize(), so
// PyEmbeddedAdapter's (DES-013) Py_IsInitialized() fail-safe was permanently
// true-for-"down", making Garmin Connect completely non-functional whenever
// Python scripting was disabled/unavailable — regardless of the codebase's
// own stated "GC_WANT_GARMINCONNECT is independent of GC_WANT_PYTHON" intent.
//
// This header is Python-free by invariant (same rule as PyEmbeddedAdapter.h /
// LSN-007) — no Python.h, no interpreter symbols — so main.cpp and any
// Python-agnostic caller can include it under GC_WANT_GARMINCONNECT alone.
// Only PyProcessBootstrap.cpp touches Python.h.
//
// Ownership: this module owns the ONE Py_InitializeFromConfig() call, and
// only ever releases the GIL on the path where IT performed that call (the
// initializing thread, exactly once). On the rarer already-initialized-
// externally path it touches no GIL state at all, in either direction
// (B-STAGE9-05) — see ensureInitialized()'s GIL postcondition below for why.
// It never finalizes the interpreter (no matching Py_FinalizeEx() call anywhere):
// DEC-052 explicitly keeps worker/wizard teardown (GarminAuthChain/
// GarminDownloadChain's QThread::terminate() fallback) from ever racing a
// mid-finalize interpreter, by there being no finalize path to race.
//
// Callers (PythonEmbed, PyEmbeddedAdapter's owning worker, or anything else
// that needs the interpreter up) call ensureInitialized() and, on success,
// use PyGILState_Ensure()/Release() around their own work — exactly like
// PyEmbeddedAdapter's existing GilGuard pattern — rather than assuming any
// particular GIL state: the GIL is only guaranteed released on return when
// THIS call performed the real initialization; on the already-initialized-
// externally path it is left untouched/unknown. PyGILState_Ensure() is safe
// either way, which is exactly why callers must always Ensure()/Release()
// around their own work instead of branching on which path was taken.

#ifndef GC_PYPROCESSBOOTSTRAP_H
#define GC_PYPROCESSBOOTSTRAP_H

#include <QString>

namespace PyProcessBootstrap {

// Invoked synchronously, exactly once ever, on the call that performs REAL
// first-time initialization — BEFORE Py_InitializeFromConfig() — never on a
// call that merely observes an already-initialized interpreter. PythonEmbed
// uses this to run PyImport_AppendInittab("goldencheetah", ...) ahead of the
// interpreter's first init (CPython's own inittab contract: the table must
// be populated before the first Py_Initialize call touches it). A build
// where GC_WANT_PYTHON is off (or Python scripting failed/declined before
// reaching this call) simply passes no hook.
using PreInitHook = void (*)();

struct Config
{
    PreInitHook preInitHook = nullptr;
};

struct Result
{
    bool ok = false;
    QString error; // populated only when !ok; untranslated PyStatus message —
                    // a developer diagnostic, never shown to the end user
                    // (mirrors DES-008's rawMessage convention).
};

// Idempotent and thread-safe. The FIRST call across the process — from
// whichever thread reaches it first — runs `cfg.preInitHook` (if any), then
// Py_InitializeFromConfig() with signal-handler installation disabled and
// the user site-packages directory explicitly preserved, then releases the
// GIL via PyEval_SaveThread(). Every subsequent call — concurrent or later,
// regardless of its own `cfg` — blocks on that first call if still running,
// then returns the SAME cached Result without touching the interpreter
// again; a second call's preInitHook is never invoked.
//
// GIL postcondition on success (B-STAGE9-05 — narrowed from an earlier,
// unsafe claim): the calling thread does NOT hold the GIL when this returns
// ONLY on the path where THIS call performed the real first-time
// Py_InitializeFromConfig() itself. On the already-initialized-externally
// path — Py_IsInitialized() was already true the first time this was ever
// reached (e.g. a test harness, as testGarminConnectPyAdapter.cpp's
// initTestCase() deliberately still does to exercise DES-013's pre-init
// fail-safe) — this call leaves the calling thread's GIL state COMPLETELY
// UNTOUCHED: it may or may not be held, unchanged either way. This module
// cannot tell "the GIL happens to be held incidentally because whatever
// externally initialized Python also left it held" apart from "the calling
// thread holds it deliberately for its own, unrelated reasons" — guessing
// either way and releasing a GIL scope that isn't this module's to release
// is undefined behavior for whichever caller actually owns it. So: callers
// must NEVER assume any particular GIL state after a call to
// ensureInitialized(), on either path, and must always wrap their own
// subsequent Python work in their own PyGILState_Ensure()/PyGILState_Release()
// pair regardless of which path was taken — PyGILState_Ensure() is itself
// safe to call whether or not the calling thread currently holds the GIL
// (unlike PyEval_SaveThread(), which requires the caller to already know and
// own the thread state). This module still never finalizes an interpreter it
// didn't create, and never touches GIL state on behalf of a DIFFERENT thread
// than the one calling it.
Result ensureInitialized(const Config &cfg = Config());

// Mirrors Py_IsInitialized() for callers that would otherwise need Python.h
// just to ask. Safe to call at any time, from any thread.
bool isInitialized();

} // namespace PyProcessBootstrap

#endif // GC_PYPROCESSBOOTSTRAP_H
