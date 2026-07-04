/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// DES-013 — embedded-CPython bridge. This is the ONLY Garmin Connect
// translation unit that includes Python.h (the header stays Python-free).
//
// authenticate() sequence (spec, DES-013):
//   1. Py_IsInitialized() false -> Unknown / "embedded Python unavailable".
//   2. PyGILState_Ensure via RAII guard, released on every exit path.
//   3. Prepend modulePath to sys.path if absent; import garmin_client.
//   4. GarminClient(email, password, tokenstorePath) -> .login().
//   5. Success dict -> PyAuthOutcome{Success, garmin_user_id, display_name}.
//   6. Exceptions classified by TYPE then .kind — never by message content
//      (LSN-006 / A3-R002-M6): GarminError kind 'auth' -> AuthFailed,
//      'connection' -> Network, anything else -> Unknown; any non-GarminError
//      exception -> Unknown, NEVER AuthFailed. Error state is cleared before
//      returning on every path.
//
// Interpreter topology: Phase-1 uses PyGILState_Ensure against the main
// interpreter; per-feature sub-interpreter isolation is the A2-001 /
// Phase-1.5 follow-up and does not change this surface.

// clang-format off
// Python.h must precede any Qt header: Qt's `slots` keyword-macro collides
// with the `slots` struct field in CPython's object.h. Protected from
// include re-sorting (the pre-commit clang-format hook reordered the test
// TU's includes and broke the build — see LSN-007).
#include <Python.h>
// clang-format on

#include "PyEmbeddedAdapter.h"

namespace {

// -- RAII: GIL acquisition released on every exit path (spec step 2) --------
class GilGuard
{
  public:
    GilGuard() : state(PyGILState_Ensure()) {}
    ~GilGuard() { PyGILState_Release(state); }
    GilGuard(const GilGuard&) = delete;
    GilGuard& operator=(const GilGuard&) = delete;

  private:
    PyGILState_STATE state;
};

// -- RAII: owned ("new") Python reference ------------------------------------
class PyRef
{
  public:
    explicit PyRef(PyObject* o = nullptr) : obj(o) {}
    ~PyRef() { Py_XDECREF(obj); }
    PyRef(const PyRef&) = delete;
    PyRef& operator=(const PyRef&) = delete;

    PyObject* get() const { return obj; }
    explicit operator bool() const { return obj != nullptr; }

  private:
    PyObject* obj;
};

// UTF-8 conversion; falls back to str(o) for non-unicode objects. Never
// leaves a pending Python error behind.
QString toQString(PyObject* o)
{
    if (o == nullptr)
        return QString();
    if (PyUnicode_Check(o)) {
        const char* utf8 = PyUnicode_AsUTF8(o);
        if (utf8 != nullptr)
            return QString::fromUtf8(utf8);
        PyErr_Clear();
        return QString();
    }
    PyRef s(PyObject_Str(o));
    if (s && PyUnicode_Check(s.get())) {
        const char* utf8 = PyUnicode_AsUTF8(s.get());
        if (utf8 != nullptr)
            return QString::fromUtf8(utf8);
    }
    PyErr_Clear();
    return QString();
}

// Spec step 3 (first half): sys.path gets modulePath prepended exactly once.
void prependToSysPathIfAbsent(const QString& dir)
{
    PyObject* sysPath = PySys_GetObject("path"); // borrowed
    if (sysPath == nullptr || !PyList_Check(sysPath)) {
        PyErr_Clear();
        return;
    }
    PyRef pyDir(PyUnicode_FromString(dir.toUtf8().constData()));
    if (!pyDir) {
        PyErr_Clear();
        return;
    }
    const int present = PySequence_Contains(sysPath, pyDir.get());
    if (present == 0)
        PyList_Insert(sysPath, 0, pyDir.get());
    PyErr_Clear(); // swallow Contains/Insert errors — import will report
}

// Spec step 6 — classification by exception TYPE, then .kind. Takes the
// currently-raised exception (clearing the error indicator) and folds it
// into a non-Success PyAuthOutcome. `module` may be null (e.g. the import
// itself failed) — then GarminError cannot exist and the exception is
// foreign by definition.
PyAuthOutcome classifyPendingException(PyObject* module)
{
    PyAuthOutcome out;
    out.kind = PyAuthOutcome::Unknown;

    PyRef exc(PyErr_GetRaisedException()); // new ref; clears the indicator
    if (!exc) {
        out.rawMessage = QStringLiteral("unknown embedded Python error");
        return out;
    }

    bool isGarminError = false;
    if (module != nullptr) {
        PyRef geType(PyObject_GetAttrString(module, "GarminError"));
        if (geType)
            isGarminError = (PyObject_IsInstance(exc.get(), geType.get()) == 1);
        PyErr_Clear();
    }

    if (isGarminError) {
        PyRef kindObj(PyObject_GetAttrString(exc.get(), "kind"));
        PyErr_Clear();
        const QString kind = toQString(kindObj.get());
        if (kind == QStringLiteral("auth"))
            out.kind = PyAuthOutcome::AuthFailed;
        else if (kind == QStringLiteral("connection"))
            out.kind = PyAuthOutcome::Network;
        else
            out.kind = PyAuthOutcome::Unknown; // rate_limit / captcha / … —
                                               // later slices extend the enum

        PyRef msgObj(PyObject_GetAttrString(exc.get(), "message"));
        PyErr_Clear();
        out.rawMessage = toQString(msgObj.get());
        if (out.rawMessage.isNull())
            out.rawMessage = toQString(exc.get()); // fallback: str(e)
    }
    else {
        // Foreign exception (ValueError, ImportError, …) — NEVER AuthFailed.
        out.kind = PyAuthOutcome::Unknown;
        out.rawMessage = toQString(exc.get());
    }

    if (out.rawMessage.isEmpty())
        out.rawMessage = QStringLiteral("embedded Python call failed");

    PyErr_Clear(); // spec: error state cleared before returning
    return out;
}

} // namespace

PyEmbeddedAdapter::PyEmbeddedAdapter(const QString& modulePath, const QString& tokenstorePath)
    : modulePath(modulePath), tokenstorePath(tokenstorePath)
{
}

PyAuthOutcome PyEmbeddedAdapter::authenticate(const QString& email, const QString& password)
{
    PyAuthOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyAuthOutcome::Unknown;
        out.rawMessage = QStringLiteral("embedded Python unavailable");
        return out;
    }

    // Step 2 — GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // Step 3 — path policy + import.
    prependToSysPathIfAbsent(modulePath);
    PyRef module(PyImport_ImportModule("garmin_client"));
    if (!module)
        return classifyPendingException(nullptr);

    // Step 4 — GarminClient(email, password, tokenstorePath).login()
    PyRef clientClass(PyObject_GetAttrString(module.get(), "GarminClient"));
    if (!clientClass)
        return classifyPendingException(module.get());

    PyRef client(PyObject_CallFunction(clientClass.get(), "sss",
                                       email.toUtf8().constData(),
                                       password.toUtf8().constData(),
                                       tokenstorePath.toUtf8().constData()));
    if (!client)
        return classifyPendingException(module.get());

    PyRef result(PyObject_CallMethod(client.get(), "login", nullptr));
    if (!result)
        return classifyPendingException(module.get());

    // Step 5 — marshal the success dict. A malformed result is a contract
    // breach of the DES-012 seam, not bad credentials: fold to Unknown.
    if (!PyDict_Check(result.get())) {
        out.kind = PyAuthOutcome::Unknown;
        out.rawMessage = QStringLiteral("garmin_client.login() returned a non-dict result");
        return out;
    }
    PyObject* uid = PyDict_GetItemString(result.get(), "garmin_user_id"); // borrowed
    PyObject* name = PyDict_GetItemString(result.get(), "display_name"); // borrowed
    if (uid == nullptr || name == nullptr) {
        out.kind = PyAuthOutcome::Unknown;
        out.rawMessage = QStringLiteral("garmin_client.login() result missing required keys");
        PyErr_Clear();
        return out;
    }

    out.kind = PyAuthOutcome::Success;
    out.garmin_user_id = toQString(uid);
    out.display_name = toQString(name);
    return out;
}
