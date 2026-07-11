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
//   4. GarminClient(email, password) -> .login() (AUTH-ONLY; DEC-014 Option B).
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

// Reads (and clears) the currently-raised exception ONCE, extracting the
// pieces every classifier needs: whether it is a garmin_client.GarminError,
// its .kind, and a non-empty raw message (DES-008 translates later; here we
// only forward raw text). `module` may be null (e.g. the import itself failed)
// — then GarminError cannot exist and the exception is foreign by definition.
// Classification is by TYPE then .kind, never by message content (LSN-006 /
// A3-R002-M6); centralizing it keeps authenticate() and downloadActivity()
// from drifting apart.
struct RaisedExc
{
    bool isGarminError = false;
    QString kind;
    QString message;
};

RaisedExc takeRaisedException(PyObject* module)
{
    RaisedExc info;

    PyRef exc(PyErr_GetRaisedException()); // new ref; clears the indicator
    if (!exc) {
        info.message = QStringLiteral("unknown embedded Python error");
        return info;
    }

    if (module != nullptr) {
        PyRef geType(PyObject_GetAttrString(module, "GarminError"));
        if (geType)
            info.isGarminError = (PyObject_IsInstance(exc.get(), geType.get()) == 1);
        PyErr_Clear();
    }

    if (info.isGarminError) {
        PyRef kindObj(PyObject_GetAttrString(exc.get(), "kind"));
        PyErr_Clear();
        info.kind = toQString(kindObj.get());

        PyRef msgObj(PyObject_GetAttrString(exc.get(), "message"));
        PyErr_Clear();
        info.message = toQString(msgObj.get());
        if (info.message.isNull())
            info.message = toQString(exc.get()); // fallback: str(e)
    }
    else {
        // Foreign exception (ValueError, ImportError, …) — never a Garmin kind.
        info.message = toQString(exc.get());
    }

    if (info.message.isEmpty())
        info.message = QStringLiteral("embedded Python call failed");

    PyErr_Clear(); // spec: error state cleared before returning
    return info;
}

// authenticate() classification (spec step 6): auth -> AuthFailed,
// connection -> Network, anything else / foreign -> Unknown (NEVER AuthFailed).
PyAuthOutcome classifyPendingException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyAuthOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("auth"))
        out.kind = PyAuthOutcome::AuthFailed;
    else if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyAuthOutcome::Network;
    else
        out.kind = PyAuthOutcome::Unknown; // rate_limit / captcha / foreign …
    return out;
}

// downloadActivity() classification (REQ-007): connection -> Network,
// rate_limit -> RateLimited, anything else / foreign -> Unknown (NEVER a
// spurious Success — the caller only treats Success as real bytes).
PyDownloadOutcome classifyDownloadException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyDownloadOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyDownloadOutcome::Network;
    else if (e.isGarminError && e.kind == QStringLiteral("rate_limit"))
        out.kind = PyDownloadOutcome::RateLimited;
    else
        out.kind = PyDownloadOutcome::Unknown;
    return out;
}

} // namespace

PyEmbeddedAdapter::PyEmbeddedAdapter(const QString& modulePath)
    : modulePath(modulePath)
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

    // Step 4 — GarminClient(email, password).login(). AUTH-ONLY construction
    // (DEC-014 Option B / A3-R004-M3): NO tokenstore path is forwarded, so the
    // library self-writes no token file; the session stays in memory and is
    // exported below via dump_tokens() for C++ to persist 0600.
    PyRef clientClass(PyObject_GetAttrString(module.get(), "GarminClient"));
    if (!clientClass)
        return classifyPendingException(module.get());

    PyRef client(PyObject_CallFunction(clientClass.get(), "ss",
                                       email.toUtf8().constData(),
                                       password.toUtf8().constData()));
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

    // REQ-007 session model: retain the authenticated client so
    // downloadActivity() can reuse it. The password is NOT kept (REQ-005), so
    // a fresh per-download client is impossible — this retained handle is the
    // session. GIL is held here (step 2), so the refcount ops are safe.
    Py_XDECREF(m_client);       // drop any prior session (re-auth replaces it)
    m_client = client.get();
    Py_INCREF(m_client);        // keep a ref beyond this call; `client` drops its own

    out.kind = PyAuthOutcome::Success;
    out.garmin_user_id = toQString(uid);
    out.display_name = toQString(name);

    // REQ-004 / DEC-014 Option B — export the authenticated session as an
    // opaque blob so C++ (GarminTokenStore) can own the atomic 0600 write. A
    // dump failure must NOT fail an otherwise-successful auth: leave tokenBlob
    // empty and clear any pending error (the worker treats empty as "nothing
    // to persist"). GIL is held (step 2), so the call is safe.
    PyRef blob(PyObject_CallMethod(client.get(), "dump_tokens", nullptr));
    if (blob && PyUnicode_Check(blob.get()))
        out.tokenBlob = toQString(blob.get());
    else
        PyErr_Clear();

    return out;
}

PyDownloadOutcome PyEmbeddedAdapter::downloadActivity(const QString& activityId, const QString& fmt)
{
    PyDownloadOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyDownloadOutcome::Unknown;
        out.rawMessage = QStringLiteral("embedded Python unavailable");
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // No retained session — authenticate() must have succeeded first. The
    // password is not kept (REQ-005), so we cannot build a fresh client here.
    if (m_client == nullptr) {
        out.kind = PyDownloadOutcome::Unknown;
        out.rawMessage = QStringLiteral("not authenticated");
        return out;
    }

    // garmin_client is needed only to resolve the GarminError type for
    // classification; it is already imported/cached from authenticate().
    prependToSysPathIfAbsent(modulePath);
    PyRef module(PyImport_ImportModule("garmin_client"));

    PyRef result(PyObject_CallMethod(m_client, "download_activity", "ss",
                                     activityId.toUtf8().constData(),
                                     fmt.toUtf8().constData()));
    if (!result)
        return classifyDownloadException(module.get());

    // A non-bytes result is a DES-012 contract breach, not a valid download:
    // fold to Unknown rather than fabricate an empty Success.
    if (!PyBytes_Check(result.get())) {
        out.kind = PyDownloadOutcome::Unknown;
        out.rawMessage = QStringLiteral("garmin_client.download_activity() returned a non-bytes result");
        return out;
    }

    char* buf = nullptr;
    Py_ssize_t len = 0;
    if (PyBytes_AsStringAndSize(result.get(), &buf, &len) != 0) {
        PyErr_Clear();
        out.kind = PyDownloadOutcome::Unknown;
        out.rawMessage = QStringLiteral("failed to read download bytes from embedded Python");
        return out;
    }

    out.kind = PyDownloadOutcome::Success;
    out.data = QByteArray(buf, static_cast<int>(len)); // (ptr,len) copy — binary-safe, keeps NULs
    return out;
}

PyEmbeddedAdapter::~PyEmbeddedAdapter()
{
    // Release the retained client under the GIL. If the interpreter is already
    // finalized its objects are gone — DECREF would be a use-after-free — so we
    // only touch it while the interpreter is live (a leaked handle in a dead
    // interpreter is harmless; the process is tearing down).
    if (m_client != nullptr && Py_IsInitialized()) {
        GilGuard gil;
        Py_DECREF(m_client);
    }
    m_client = nullptr;
}
