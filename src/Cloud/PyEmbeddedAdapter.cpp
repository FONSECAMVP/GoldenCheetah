/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

// embedded-CPython bridge. This is the ONLY Garmin Connect
// translation unit that includes Python.h (the header stays Python-free).
//
// authenticate() sequence (spec):
//   1. Py_IsInitialized() false -> Unknown / "embedded Python unavailable".
//   2. PyGILState_Ensure via RAII guard, released on every exit path.
//   3. Explicit override -> hoist to sys.path index 0, check any cached
//      sys.modules entry's identity against the C++-owned provenance ledger,
//      import, then re-check identity against the ledger again; no override
//      -> import only.
//   4. GarminClient(email, password) -> .login() (AUTH-ONLY).
//   5. Success dict -> PyAuthOutcome{Success, garmin_user_id, display_name}.
//   6. Exceptions classified by TYPE then .kind — never by message content
//      : GarminError kind 'auth' -> AuthFailed,
//      'connection' -> Network, anything else -> Unknown; any non-GarminError
//      exception -> Unknown, NEVER AuthFailed. Error state is cleared before
//      returning on every path.
//
// Interpreter topology: the main-interpreter path uses PyGILState_Ensure against the main
// interpreter; per-feature sub-interpreter isolation is a later follow-up
// and does not change this surface.

// clang-format off
// Python.h must precede any Qt header: Qt's `slots` keyword-macro collides
// with the `slots` struct field in CPython's object.h. Protected from
// include re-sorting (the pre-commit clang-format hook reordered the test
// TU's includes and broke the build —.
#include <Python.h>
// clang-format on

#include "PyEmbeddedAdapter.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QPair>
#include <QSet>
#include <QVector>

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

// find the FIRST exact (string-equality, uncanonicalized)
// occurrence of dir in sys.path. Already at index 0 -> leave it. Present
// later -> remove that one occurrence and insert at 0 (later duplicates, if
// any, are left alone). Absent -> insert at 0. Returns false, with sys.path
// left as-is, on any list-op failure or if sys.path is not a list; callers
// on the explicit-override path must fail closed on false, never proceed to
// import.
bool hoistOverrideDirToFront(const QString& dir)
{
    PyObject* sysPath = PySys_GetObject("path"); // borrowed
    if (sysPath == nullptr || !PyList_Check(sysPath)) {
        PyErr_Clear();
        return false;
    }
    PyRef pyDir(PyUnicode_FromString(dir.toUtf8().constData()));
    if (!pyDir) {
        PyErr_Clear();
        return false;
    }

    const Py_ssize_t len = PyList_Size(sysPath);
    if (len < 0) {
        PyErr_Clear();
        return false;
    }

    Py_ssize_t firstIndex = -1;
    for (Py_ssize_t i = 0; i < len; ++i) {
        PyObject* item = PyList_GetItem(sysPath, i); // borrowed
        if (item == nullptr) {
            PyErr_Clear();
            return false;
        }
        const int eq = PyObject_RichCompareBool(item, pyDir.get(), Py_EQ);
        if (eq < 0) {
            PyErr_Clear();
            return false;
        }
        if (eq == 1) {
            firstIndex = i;
            break;
        }
    }

    if (firstIndex == 0)
        return true;

    if (firstIndex > 0) {
        PyObject* existing = PyList_GetItem(sysPath, firstIndex); // borrowed
        if (existing == nullptr) {
            PyErr_Clear();
            return false;
        }
        Py_INCREF(existing);
        PyRef owned(existing); // kept alive across the erase below
        if (PySequence_DelItem(sysPath, firstIndex) < 0) {
            PyErr_Clear();
            return false;
        }
        if (PyList_Insert(sysPath, 0, owned.get()) < 0) {
            PyErr_Clear();
            return false;
        }
        return true;
    }

    // firstIndex == -1: absent entirely.
    if (PyList_Insert(sysPath, 0, pyDir.get()) < 0) {
        PyErr_Clear();
        return false;
    }
    return true;
}

// Provenance is a C++-owned identity ledger,
// not an attribute read off a cached module — every attribute CPython exposes
// on a module is writable by the code the guard is trying to judge. The
// investigator's repro: a genuine ledgered parent package's __file__ still
// names the real override dir, but its __path__ was retargeted at a decoy,
// and the decoy child imports successfully AFTER a pre-import-only check
// already accepted the (still-genuine-looking) parent. The ledger instead
// records, per (interpreter, override dir, dotted name), the exact PyObject*
// produced by the one clean import this file performs; a later call accepts
// a cached entry only if its identity is the one this file itself ledgered.
QHash<QString, PyObject*>& provenanceLedger()
{
    static QHash<QString, PyObject*> ledger;
    return ledger;
}

QString provenanceLedgerKey(PyInterpreterState* interp, const QString& dir, const QString& name)
{
    // '\x1f' (US) as a field separator: not valid in a filesystem path or a
    // Python dotted identifier, so it cannot be forged into a collision.
    return QString::number(reinterpret_cast<quintptr>(interp)) + QLatin1Char('\x1f') + dir + QLatin1Char('\x1f') + name;
}

// Every sys.modules key equal to "gc_garmin_adapter" or prefixed
// "gc_garmin_adapter.", paired with its current (borrowed) object pointer.
// Never leaves a pending Python error.
QVector<QPair<QString, PyObject*>> matchingCacheEntries()
{
    QVector<QPair<QString, PyObject*>> out;
    PyObject* sysModules = PyImport_GetModuleDict(); // borrowed
    if (sysModules == nullptr || !PyDict_Check(sysModules))
        return out;

    PyObject* key = nullptr;
    PyObject* value = nullptr;
    Py_ssize_t pos = 0;
    while (PyDict_Next(sysModules, &pos, &key, &value)) {
        if (!PyUnicode_Check(key))
            continue;
        const char* keyUtf8 = PyUnicode_AsUTF8(key);
        if (keyUtf8 == nullptr) {
            PyErr_Clear();
            continue;
        }
        const QString name = QString::fromUtf8(keyUtf8);
        if (name != QStringLiteral("gc_garmin_adapter") && !name.startsWith(QStringLiteral("gc_garmin_adapter.")))
            continue;
        out.append(qMakePair(name, value));
    }
    return out;
}

// Pre-import half: every already-cached matching entry
// must already be ledgered under (interp, dir) with the exact same pointer
// identity; any matching entry absent from the ledger fails the whole call
// closed (a pre-existing foreign/unverifiable entry). No
// matching entries at all is the common first-import case and trivially
// passes. namesOut collects the entry names that were present (and just
// verified) before the import, so the post-import check below can tell
// "already there and vetted" apart from "appeared during this very import".
bool explicitOverridePreImportCacheIsSafe(PyInterpreterState* interp, const QString& dir, QSet<QString>* namesOut)
{
    QHash<QString, PyObject*>& ledger = provenanceLedger();
    for (const auto& entry : matchingCacheEntries()) {
        const auto it = ledger.constFind(provenanceLedgerKey(interp, dir, entry.first));
        if (it == ledger.constEnd() || it.value() != entry.second)
            return false;
        namesOut->insert(entry.first);
    }
    return true;
}

// Post-import half: a forged parent whose retargeted
// __path__ produces a decoy child DURING the import is invisible to the
// pre-import check above (the parent alone still matches the ledger) and
// only shows up here, because the child's identity changes between the two
// checks. presentBeforeImport is the pre-import check's verified name set:
//   - a name that WAS present before import must still resolve to the exact
//     pointer the pre-import check just verified (a static defense — normal
//     import machinery never actually reaches this branch, since it does not
//     replace an already-fully-cached entry).
//   - a name that was NOT present before import, while presentBeforeImport
//     is otherwise non-empty, means some of the package's identity survived
//     from before this call and some did not — exactly the forged-__path__
//     shape (a ledgered parent retained, its child dropped and recreated
//     from a decoy during this call) — and fails closed.
//   - a name that was NOT present before import, and NOTHING matching was
//     cached before this import at all (presentBeforeImport empty), was
//     produced by this one clean import and is recorded fresh. This is both
//     the ordinary first-use-of-this-dir case and the legitimate case where
//     a caller deliberately evicted every prior gc_garmin_adapter[.*] entry
//     before reimporting under the same override — the stale ledger record
//     any such name may still hold from a since-evicted cycle is replaced,
//     its old strong reference released. This is the only place entries are
//     added or replaced; nothing is ever evicted out from under a caller
//     that never asked for it (eviction leaves live objects
//     referencing a stale type universe; a blanket reject would break a
//     genuine second construction over the same override, constraint 3).
bool explicitOverridePostImportCacheIsSafeAndRecord(PyInterpreterState* interp, const QString& dir,
                                                     const QSet<QString>& presentBeforeImport)
{
    QHash<QString, PyObject*>& ledger = provenanceLedger();
    for (const auto& entry : matchingCacheEntries()) {
        const QString key = provenanceLedgerKey(interp, dir, entry.first);
        if (presentBeforeImport.contains(entry.first)) {
            const auto it = ledger.constFind(key);
            if (it == ledger.constEnd() || it.value() != entry.second)
                return false;
            continue;
        }
        if (!presentBeforeImport.isEmpty())
            return false; // mixed state: some identity survived this call, this name did not
        const auto existing = ledger.find(key);
        if (existing != ledger.end()) {
            Py_DECREF(existing.value());
            existing.value() = entry.second;
        } else {
            ledger.insert(key, entry.second);
        }
        Py_INCREF(entry.second);
    }
    return true;
}

// 5 / 17: an explicit override is the FIRST thing
// consulted, every time it is set — never a fallback tried after a failed
// plain import (that shape let a missing installed payload silently succeed
// anywhere the override/source-tree directory happened to exist). On the
// explicit-override path, the override dir is hoisted to sys.path index 0
// and any already-cached gc_garmin_adapter[.*] entry must be proven to
// originate from that same dir; either check failing fails the whole call
// closed (a synthesized exception, import never attempted) rather than
// clearing the error and importing anyway. With no override, exactly one
// plain import is attempted against sys.path as CPython already built it
// (site-packages included); there is no fallback. Returns a new reference on
// success, or nullptr with a pending exception set (same contract as
// PyImport_ImportModule itself) on failure.
PyObject* importAdapterModule(const GarminPyModulePath& modulePath)
{
    if (!modulePath.isExplicitOverride())
        return PyImport_ImportModule("gc_garmin_adapter.garmin_client");

    if (!hoistOverrideDirToFront(modulePath.dir())) {
        // T208-ALLOW:I18N-TR-WRAP
        PyErr_SetString(PyExc_RuntimeError, "GC_GARMIN_PYPATH: sys.path is not a list, or a list operation failed");
        return nullptr;
    }

    PyInterpreterState* interp = PyInterpreterState_Get();
    QSet<QString> presentBeforeImport;
    if (!explicitOverridePreImportCacheIsSafe(interp, modulePath.dir(), &presentBeforeImport)) {
        PyErr_SetString(PyExc_ImportError,
                         // T208-ALLOW:I18N-TR-WRAP
                         "gc_garmin_adapter is already cached from an origin other than GC_GARMIN_PYPATH");
        return nullptr;
    }

    PyObject* module = PyImport_ImportModule("gc_garmin_adapter.garmin_client");
    if (module == nullptr)
        return nullptr;

    // re-validate AFTER the import returns — a forged
    // parent's retargeted __path__ only produces its decoy child during this
    // very call, invisible to the pre-import check above.
    if (!explicitOverridePostImportCacheIsSafeAndRecord(interp, modulePath.dir(), presentBeforeImport)) {
        Py_DECREF(module);
        PyErr_SetString(PyExc_ImportError,
                         // T208-ALLOW:I18N-TR-WRAP
                         "gc_garmin_adapter cache changed identity during import (provenance check failed)");
        return nullptr;
    }

    // Redundant under CPython's current PyImport_Import semantics
    // (cpython-import-return-binding) — retained as a tripwire against a
    // future import-path change.
    const QHash<QString, PyObject*>& ledger = provenanceLedger();
    const QString childKey = provenanceLedgerKey(interp, modulePath.dir(), QStringLiteral("gc_garmin_adapter.garmin_client"));
    const auto ledgered = ledger.constFind(childKey);
    if (ledgered == ledger.constEnd() || ledgered.value() != module) {
        Py_DECREF(module);
        PyErr_SetString(PyExc_ImportError,
                         // T208-ALLOW:I18N-TR-WRAP
                         "gc_garmin_adapter.garmin_client: import returned an object absent from the provenance ledger");
        return nullptr;
    }
    return module;
}

// Reviewer delta-fix #1 (security): __module__/__qualname__ are NOT
// guaranteed to be plain str — a custom metaclass or descriptor can make
// either resolve to an arbitrary object. toQString()'s str(o) fallback would
// then execute that object's __str__/__repr__ and return whatever untrusted
// text it produces, defeating pyExceptionTypeName()'s "carries no leak
// surface" guarantee below. This helper accepts ONLY an actual PyUnicode
// value; anything else (including one that merely LOOKS stringifiable) is
// treated as absent, never stringified.
QString strictUnicodeAttr(PyObject* o)
{
    if (o == nullptr || !PyUnicode_Check(o))
        return QString();
    const char* utf8 = PyUnicode_AsUTF8(o);
    if (utf8 == nullptr) {
        PyErr_Clear();
        return QString();
    }
    return QString::fromUtf8(utf8);
}

// Reviewer delta-fix #3 (security): strictUnicodeAttr() only proves a value
// IS a str — Python allows a foreign/adversarial exception class to reassign
// __module__ or __qualname__ to any string content, which would sail through
// PyUnicode_Check() and be surfaced verbatim by the old code. A genuine
// module path or qualified name is always one or more ASCII identifier
// segments (first char letter/underscore, remaining chars letters/digits/
// underscore) joined by dots; anything else is untrusted content
// masquerading as a name and must never reach exceptionType.
bool isAsciiIdentifierChar(QChar c, bool first)
{
    const ushort u = c.unicode();
    if (u == '_')
        return true;
    if (u >= 'a' && u <= 'z')
        return true;
    if (u >= 'A' && u <= 'Z')
        return true;
    return !first && u >= '0' && u <= '9';
}

bool isSafeDottedName(const QString& s)
{
    if (s.isEmpty())
        return false;
    const QStringList segments = s.split(QLatin1Char('.'));
    for (const QString& seg : segments) {
        if (seg.isEmpty())
            return false;
        for (int i = 0; i < seg.length(); ++i) {
            if (!isAsciiIdentifierChar(seg.at(i), i == 0))
                return false;
        }
    }
    return true;
}

// Reviewer delta-fix #4 (security): a grammar-shaped module string is not
// necessarily a SAFE one — plenty of real secrets (API keys/tokens) are
// themselves identifier-shaped (e.g. "hunter2", "sk_live_abc123") and would
// pass isSafeDottedName() cleanly if a foreign/adversarial exception type's
// __module__ were reassigned to one (Python allows this on any ordinary
// class; no metaclass needed). No regex can distinguish "looks like an
// identifier" from "is a secret", so the only closure is to stop trusting an
// arbitrary module string at all: this is the fixed, small set of module
// roots this integration's actual dependency chain can raise exceptions
// from — confirmed against the real installed packages and this project's
// own Stage-9 live-account findings, not guessed:
//   - builtins        — CPython built-in exceptions (e.g. ValueError).
//   - garmin_client    — this project's own wrapper; home of GarminError.
//   - garminconnect    — the upstream library's exceptions module
//     (garminconnect.exceptions); GarminConnectInvalidFileFormatError is a
//     real exception from here that garmin_client.py does NOT catch/wrap,
//     so it must remain diagnosable rather than folding to foreign_exception.
//   - curl_cffi        — e.g. curl_cffi.requests.exceptions.ImpersonateError
//     (the exact type seen in a live-account failure).
//   - requests         — transitively bundled by curl_cffi; e.g.
//     requests.exceptions.Timeout, requests.cookies.RequestsCookieJar
//     (the AttributeError seen in a live-account failure raises as builtins.
//     AttributeError, but the object it names lives here).
//   - urllib3, socket, ssl, http, json — the underlying network/HTTP stack
//     requests/curl_cffi are built on; genuine transport-level failures can
//     surface directly from any of these.
// Injecting a hostile class under one of these roots would require
// compromising the dependency chain itself — a materially different, more
// severe threat than a logging leak, and outside what this function can or
// should defend against.
const char* const kAllowedModuleRoots[] = {
    "builtins", "gc_garmin_adapter", "garminconnect", "curl_cffi", "requests",
    "urllib3",  "socket",        "ssl",           "http",      "json",
};

bool isAllowedModuleRoot(const QString& moduleName)
{
    for (const char* root : kAllowedModuleRoots) {
        const QString r = QString::fromLatin1(root);
        if (moduleName == r || moduleName.startsWith(r + QStringLiteral(".")))
            return true;
    }
    return false;
}

// Reviewer delta-fix #6 (security): __module__/__qualname__/tp_name are all
// ordinary, Python-writable metadata — a foreign/adversarial class can set
// __module__ to an ALLOWLISTED string (e.g. "curl_cffi.requests.exceptions")
// without ever touching curl_cffi, pair it with a secret-shaped
// __qualname__, and sail through isAllowedModuleRoot()'s string-membership
// check untouched. That check proves the CLAIM is on the allowlist; it
// proves nothing about whether the type genuinely lives there. This walks
// the real sys.modules + attribute chain for dottedPath and confirms the
// caller's `type` (the actual Py_TYPE(exc) of the raised exception) IS the
// same object Python itself has registered there — by raw POINTER IDENTITY
// only, never a Python-level comparison (a foreign class could override
// __eq__ and defeat a value-based check; pointer identity on a real CPython
// type object cannot be spoofed by attribute assignment alone). Never
// leaves a pending Python error: every attribute-chain step is individually
// guarded, same style as strictUnicodeAttr.
bool isGenuineAllowlistedException(PyObject* type, const QString& dottedPath)
{
    PyObject* sysModules = PyImport_GetModuleDict(); // borrowed, never null once initialized
    const QStringList segments = dottedPath.split(QLatin1Char('.'));
    if (segments.isEmpty())
        return false;

    // Longest leading prefix (rejoined with '.') that is an actual key in
    // sys.modules is the real, already-imported module object; if no prefix
    // matches at all, the whole claim is unbacked.
    PyObject* moduleObj = nullptr; // borrowed, owned by sys.modules
    int prefixLen = 0;
    for (int i = segments.size(); i >= 1; --i) {
        QStringList prefixSegments;
        for (int j = 0; j < i; ++j)
            prefixSegments << segments.at(j);
        const QString candidate = prefixSegments.join(QLatin1Char('.'));
        PyRef key(PyUnicode_FromString(candidate.toUtf8().constData()));
        if (!key) {
            PyErr_Clear();
            continue;
        }
        PyObject* found = PyDict_GetItem(sysModules, key.get()); // borrowed
        if (found != nullptr) {
            moduleObj = found;
            prefixLen = i;
            break;
        }
    }
    if (moduleObj == nullptr)
        return false;

    // Walk the remaining trailing segments as a real attribute chain.
    PyObject* current = moduleObj; // borrowed for the duration of the walk
    PyObject* prevOwned = nullptr; // most recently GetAttrString'd owned ref
    bool resolved = true;
    for (int i = prefixLen; i < segments.size(); ++i) {
        PyObject* next = PyObject_GetAttrString(current, segments.at(i).toUtf8().constData());
        PyErr_Clear();
        Py_XDECREF(prevOwned);
        prevOwned = next;
        if (next == nullptr) {
            resolved = false;
            break;
        }
        current = next;
    }

    const bool genuine = resolved && (current == type); // pointer identity, computed before decref
    Py_XDECREF(prevOwned);
    return genuine;
}

// Stage 9 live-account diagnostic: the module-qualified exception TYPE name
// only (e.g. "builtins.ValueError") — deliberately NEVER the exception's
// message/arguments (PyAuthOutcome::exceptionType's doc comment explains why
// a type name, unlike str(exc), carries no leak surface). Never throws / never
// leaves a pending Python error: every introspection step is individually
// guarded and falls back to "unknown" rather than propagating a failure.
QString pyExceptionTypeName(PyObject* exc)
{
    if (exc == nullptr)
        return QStringLiteral("unknown");
    PyObject* type = reinterpret_cast<PyObject*>(Py_TYPE(exc));
    if (type == nullptr)
        return QStringLiteral("unknown");

    PyRef moduleObj(PyObject_GetAttrString(type, "__module__"));
    QString moduleName = strictUnicodeAttr(moduleObj.get());
    PyErr_Clear();
    if (!moduleName.isEmpty() && !isSafeDottedName(moduleName))
        moduleName.clear(); // delta-fix #3: str, but not name-shaped — treat as absent

    // Reviewer delta-fix #5: ONE gate, checked before consulting qualName or
    // tp_name at all. Earlier rounds let an empty moduleName (attribute
    // absent/non-str/invalid-shape) fall through to the qualname-or-tp_name
    // logic below unguarded — but both qualName and tp_name are exactly as
    // independently attacker-settable as moduleName itself (delta-fix #2's
    // own TpNameFallbackError fixture proves tp_name can be crafted to an
    // arbitrary dotted string via type()'s 3-arg form; __qualname__ is a
    // plain writable str attribute same as __module__). So moduleName must
    // be present, Unicode, grammar-valid, AND allowlisted before this
    // function will emit ANY derived name — an empty moduleName is now
    // foreign too, not just a present-but-disallowed one.
    if (moduleName.isEmpty() || !isAllowedModuleRoot(moduleName))
        return QStringLiteral("foreign_exception");

    PyRef qualObj(PyObject_GetAttrString(type, "__qualname__"));
    QString qualName = strictUnicodeAttr(qualObj.get());
    PyErr_Clear();
    if (!qualName.isEmpty() && !isSafeDottedName(qualName))
        qualName.clear(); // delta-fix #3: str, but not name-shaped — treat as absent

    if (qualName.isEmpty()) {
        // Reviewer delta-fix #2: __qualname__ absent/non-str/failed/invalid —
        // tp_name is the best remaining name, and for a heap type outside the
        // 'builtins' module it is ALREADY module-qualified (e.g.
        // "curl_cffi.requests.exceptions.ImpersonateError"). Return it
        // directly; do NOT fall through to the moduleName+"." concatenation
        // below, which would double the module prefix.
        const char* tpName = Py_TYPE(exc)->tp_name;
        if (tpName != nullptr && tpName[0] != '\0') {
            const QString tpNameStr = QString::fromUtf8(tpName);
            if (isSafeDottedName(tpNameStr)) { // delta-fix #3: grammar guard
                // Reviewer delta-fix #7: isGenuineAllowlistedException() only
                // proves tpNameStr resolves to a REAL, genuinely-registered
                // object — "real" and "allowlisted" are not the same
                // guarantee, and only the latter was ever supposed to gate
                // what gets logged. A foreign type can set __module__ to an
                // allowlisted string (passing the gate above), hide
                // __qualname__ (forcing this branch), and have tp_name
                // genuinely be some real object from an UNallowlisted module
                // reachable in this process (e.g. "evil.hunter2"). tp_name's
                // OWN module portion — everything before the LAST '.', or
                // moduleName itself for a bare/undotted tp_name (already
                // validated allowlisted above) — must independently pass
                // isAllowedModuleRoot() BEFORE the provenance walk even
                // runs. Deliberately NOT an exact-equality check against
                // moduleName: a genuine curl_cffi exception's tp_name is
                // legitimately ALREADY module-qualified and differs from
                // whatever module the raising code happens to live in (see
                // TpNameFallbackError's fixture comment).
                const int lastDot = tpNameStr.lastIndexOf(QLatin1Char('.'));
                const QString tpModulePortion = lastDot >= 0 ? tpNameStr.left(lastDot) : moduleName;
                // delta-fix #6: genuine-provenance guard — tp_name is
                // exactly as claimable as moduleName/qualName; a
                // grammar-valid but unbacked/spoofed tp_name is foreign,
                // same marker as the moduleName-provenance check below.
                if (isAllowedModuleRoot(tpModulePortion) && isGenuineAllowlistedException(type, tpNameStr))
                    return tpNameStr;
                return QStringLiteral("foreign_exception");
            }
        }
        return QStringLiteral("unknown"); // no tp_name at all, or grammar-invalid — unchanged from before
    }

    // moduleName is guaranteed non-empty here — the delta-fix #5 gate above
    // already returned "foreign_exception" for an empty/disallowed one.
    const QString qualifiedName = moduleName + QStringLiteral(".") + qualName;
    // delta-fix #6: confirm `type` genuinely IS the object sys.modules says
    // lives at qualifiedName, not merely a class claiming to via writable
    // __module__/__qualname__ attributes.
    if (!isGenuineAllowlistedException(type, qualifiedName))
        return QStringLiteral("foreign_exception");
    return qualifiedName;
}

// Reads (and clears) the currently-raised exception ONCE, extracting the
// pieces every classifier needs: whether it is a garmin_client.GarminError,
// its .kind, and a non-empty raw message (translated later; here we
// only forward raw text). `module` may be null (e.g. the import itself failed)
// — then GarminError cannot exist and the exception is foreign by definition.
// Classification is by TYPE then .kind, never by message content; centralizing it keeps authenticate() and downloadActivity()
// from drifting apart.
struct RaisedExc
{
    bool isGarminError = false;
    QString kind;
    QString message;
    // Stage 9 diagnostic — only classifyPendingException's PyAuthOutcome wires
    // this through today (PyDownloadOutcome/PyListOutcome/PyProfileOutcome do
    // not need it yet); populated here regardless since takeRaisedException
    // is shared by all four classify*Exception functions.
    QString exceptionType;
};

RaisedExc takeRaisedException(PyObject* module)
{
    RaisedExc info;

    // Shipped Windows/macOS installers embed Python 3.11 (no PyErr_GetRaisedException).
#if PY_VERSION_HEX >= 0x030C0000
    PyRef exc(PyErr_GetRaisedException()); // new ref; clears the indicator
#else
    PyObject* excType = nullptr;
    PyObject* excValue = nullptr;
    PyObject* excTraceback = nullptr;
    PyErr_Fetch(&excType, &excValue, &excTraceback); // new refs; clears the indicator
    PyErr_NormalizeException(&excType, &excValue, &excTraceback);
    if (excValue && excTraceback)
        PyException_SetTraceback(excValue, excTraceback); // does not steal excTraceback
    Py_XDECREF(excType);
    Py_XDECREF(excTraceback);
    PyRef exc(excValue); // new ref; ownership now held by PyRef
#endif
    if (!exc) {
        // Developer diagnostic: rawMessage is never displayed in the UI.
        info.message = QStringLiteral("unknown embedded Python error"); // T208-ALLOW:I18N-TR-WRAP
        info.exceptionType = QStringLiteral("unknown");
        return info;
    }

    info.exceptionType = pyExceptionTypeName(exc.get());

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
        // Developer diagnostic: rawMessage is never displayed in the UI.
        info.message = QStringLiteral("embedded Python call failed"); // T208-ALLOW:I18N-TR-WRAP

    PyErr_Clear(); // spec: error state cleared before returning
    return info;
}

// authenticate() classification (spec step 6): auth -> AuthFailed,
// connection -> Network, rate_limit -> RateLimit, anything else /
// foreign -> Unknown (NEVER AuthFailed).
PyAuthOutcome classifyPendingException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyAuthOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("auth"))
        out.kind = PyAuthOutcome::AuthFailed;
    else if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyAuthOutcome::Network;
    else if (e.isGarminError && e.kind == QStringLiteral("rate_limit"))
        out.kind = PyAuthOutcome::RateLimit;
    else
        out.kind = PyAuthOutcome::Unknown; // captcha / foreign …
    // Stage 9 diagnostic: only meaningful on the Unknown path (see
    // PyAuthOutcome::exceptionType's doc comment); left empty for a classified
    // GarminError kind so a dev-log line never fires for an already-known kind.
    if (out.kind == PyAuthOutcome::Unknown)
        out.exceptionType = e.exceptionType;
    return out;
}

// downloadActivity() classification: connection -> Network,
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

// listActivitiesSince() classification: mirrors
// classifyDownloadException one op sideways — connection -> Network,
// rate_limit -> RateLimited, anything else / foreign -> Unknown (NEVER a
// spurious Success — the caller only treats Success as a real listing).
PyListOutcome classifyListException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyListOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyListOutcome::Network;
    else if (e.isGarminError && e.kind == QStringLiteral("rate_limit"))
        out.kind = PyListOutcome::RateLimited;
    else
        out.kind = PyListOutcome::Unknown;
    return out;
}

// loadTokens() classification:
// session_expired -> SessionExpired (route to fresh SSO, DISTINCT from a
// transient dip), connection -> Network, anything else / foreign -> Unknown.
PyLoadTokensOutcome classifyLoadTokensException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyLoadTokensOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("session_expired"))
        out.kind = PyLoadTokensOutcome::SessionExpired;
    else if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyLoadTokensOutcome::Network;
    else
        out.kind = PyLoadTokensOutcome::Unknown;
    return out;
}

// fetchProfile() classification: connection -> Network,
// anything else / foreign (including rate_limit — this outcome has no
// dedicated RateLimit kind, mirroring PyLoadTokensOutcome) -> Unknown.
PyProfileOutcome classifyProfileException(PyObject* module)
{
    const RaisedExc e = takeRaisedException(module);
    PyProfileOutcome out;
    out.rawMessage = e.message;
    if (e.isGarminError && e.kind == QStringLiteral("connection"))
        out.kind = PyProfileOutcome::Network;
    else
        out.kind = PyProfileOutcome::Unknown;
    return out;
}

} // namespace

PyEmbeddedAdapter::PyEmbeddedAdapter(const GarminPyModulePath& modulePath)
    : modulePath(modulePath)
{
}

PyAuthOutcome PyEmbeddedAdapter::authenticate(const QString& email, const QString& password)
{
    PyAuthOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyAuthOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // Step 2 — GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // Step 3 — path policy + import.
    PyRef module(importAdapterModule(modulePath));
    if (!module)
        return classifyPendingException(nullptr);

    // Step 4 — GarminClient(email, password).login(). AUTH-ONLY construction
    // NO tokenstore path is forwarded, so the
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
    // breach of the seam, not bad credentials: fold to Unknown.
    if (!PyDict_Check(result.get())) {
        out.kind = PyAuthOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.login() returned a non-dict result"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // MFA-required sentinel: garmin_client.login()
    // returns {"mfa_required": True} (instead of an identity dict) when Garmin
    // needs a 6-digit OTP. Retain the SAME client so submitMfa() can resume this
    // pending session, and surface MfaRequired (no identity, no failure). The
    // Python side classifies the needs-MFA signal by shape; here we
    // only read the stable sentinel key.
    PyObject* mfaFlag = PyDict_GetItemString(result.get(), "mfa_required"); // borrowed
    if (mfaFlag != nullptr && PyObject_IsTrue(mfaFlag) == 1) {
        Py_XDECREF(m_client); // drop any prior session
        m_client = client.get();
        Py_INCREF(m_client); // keep the pending-MFA client beyond this call
        out.kind = PyAuthOutcome::MfaRequired;
        PyErr_Clear();
        return out;
    }
    PyErr_Clear(); // PyObject_IsTrue may have set an error on a weird object

    PyObject* uid = PyDict_GetItemString(result.get(), "garmin_user_id"); // borrowed
    PyObject* name = PyDict_GetItemString(result.get(), "display_name"); // borrowed
    if (uid == nullptr || name == nullptr) {
        out.kind = PyAuthOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.login() result missing required keys");
        PyErr_Clear();
        return out;
    }

    // Session model: retain the authenticated client so
    // downloadActivity() can reuse it. The password is NOT kept, so
    // a fresh per-download client is impossible — this retained handle is the
    // session. GIL is held here (step 2), so the refcount ops are safe.
    Py_XDECREF(m_client);       // drop any prior session (re-auth replaces it)
    m_client = client.get();
    Py_INCREF(m_client);        // keep a ref beyond this call; `client` drops its own

    out.kind = PyAuthOutcome::Success;
    out.garmin_user_id = toQString(uid);
    out.display_name = toQString(name);

    // Export the authenticated session as an
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

PyAuthOutcome PyEmbeddedAdapter::submitMfa(const QString& code)
{
    PyAuthOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyAuthOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // No retained client -> authenticate() never established a pending MFA
    // session. This is a contract/order error, NOT bad credentials: Unknown.
    if (m_client == nullptr) {
        out.kind = PyAuthOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("no pending MFA session"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // garmin_client is needed only to resolve the GarminError type for
    // classification; it is already imported/cached from authenticate().
    PyRef module(importAdapterModule(modulePath));

    // Resume the pending MFA session on the SAME retained client. A bad/expired
    // code surfaces as GarminError kind 'auth' (classified by TYPE);
    // classifyPendingException maps auth -> AuthFailed, connection -> Network,
    // anything else / foreign -> Unknown (NEVER a spurious Success).
    PyRef result(PyObject_CallMethod(m_client, "submit_mfa", "s", code.toUtf8().constData()));
    if (!result)
        return classifyPendingException(module.get());

    // Marshal the success dict — mirrors authenticate() step 5.
    if (!PyDict_Check(result.get())) {
        out.kind = PyAuthOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.submit_mfa() returned a non-dict result");
        return out;
    }
    PyObject* uid = PyDict_GetItemString(result.get(), "garmin_user_id"); // borrowed
    PyObject* name = PyDict_GetItemString(result.get(), "display_name"); // borrowed
    if (uid == nullptr || name == nullptr) {
        out.kind = PyAuthOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.submit_mfa() result missing required keys");
        PyErr_Clear();
        return out;
    }

    out.kind = PyAuthOutcome::Success;
    out.garmin_user_id = toQString(uid);
    out.display_name = toQString(name);

    // a completed MFA auth yields a session to
    // persist, exactly like a no-MFA auth. Export the opaque blob; a dump
    // failure must NOT fail an otherwise-successful auth (leave tokenBlob empty).
    PyRef blob(PyObject_CallMethod(m_client, "dump_tokens", nullptr));
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
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // No retained session — authenticate() must have succeeded first. The
    // password is not kept, so we cannot build a fresh client here.
    if (m_client == nullptr) {
        out.kind = PyDownloadOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("not authenticated"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // garmin_client is needed only to resolve the GarminError type for
    // classification; it is already imported/cached from authenticate().
    PyRef module(importAdapterModule(modulePath));

    PyRef result(PyObject_CallMethod(m_client, "download_activity", "ss",
                                     activityId.toUtf8().constData(),
                                     fmt.toUtf8().constData()));
    if (!result)
        return classifyDownloadException(module.get());

    // A non-bytes result is a contract breach, not a valid download:
    // fold to Unknown rather than fabricate an empty Success.
    if (!PyBytes_Check(result.get())) {
        out.kind = PyDownloadOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.download_activity() returned a non-bytes result");
        return out;
    }

    char* buf = nullptr;
    Py_ssize_t len = 0;
    if (PyBytes_AsStringAndSize(result.get(), &buf, &len) != 0) {
        PyErr_Clear();
        out.kind = PyDownloadOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("failed to read download bytes from embedded Python");
        return out;
    }

    out.kind = PyDownloadOutcome::Success;
    out.data = QByteArray(buf, static_cast<int>(len)); // (ptr,len) copy — binary-safe, keeps NULs
    return out;
}

PyListOutcome PyEmbeddedAdapter::listActivitiesSince(const QString& sinceGmt)
{
    PyListOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyListOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // No retained session — authenticate()/loadTokens() must have succeeded
    // first. The password is not kept, so we cannot build a fresh
    // client here.
    if (m_client == nullptr) {
        out.kind = PyListOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("not authenticated"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // garmin_client is needed only to resolve the GarminError type for
    // classification; it is already imported/cached from authenticate().
    PyRef module(importAdapterModule(modulePath));

    // Forward the since-timestamp VERBATIM (Garmin's server-side
    // timestamp, never the local clock). The adapter returns an iterator of
    // summary dicts (list_activities_since -> Iterator[dict]).
    PyRef result(PyObject_CallMethod(m_client, "list_activities_since", "s", sinceGmt.toUtf8().constData()));
    if (!result)
        return classifyListException(module.get());

    // Iterate the returned iterable (list / generator / iterator) via the
    // iterator protocol so either a list or a lazy generator marshals the same.
    PyRef iter(PyObject_GetIter(result.get()));
    if (!iter) {
        // A non-iterable result is a contract breach, not a valid
        // listing: fold to Unknown rather than fabricate an empty Success.
        PyErr_Clear();
        out.kind = PyListOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.list_activities_since() returned a non-iterable result");
        return out;
    }

    QVector<GarminActivitySummary> summaries;
    while (true) {
        PyRef item(PyIter_Next(iter.get()));
        if (!item)
            break; // exhausted (or an error was set — checked below)

        // Each item must be a dict carrying at least activityId + startTimeGMT;
        // a non-dict item is a contract breach → Unknown (never a partial
        // Success with a bogus row).
        if (!PyDict_Check(item.get())) {
            out.kind = PyListOutcome::Unknown;
            // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
            out.rawMessage = QStringLiteral("garmin_client.list_activities_since() yielded a non-dict item");
            return out;
        }
        PyObject* aid = PyDict_GetItemString(item.get(), "activityId"); // borrowed
        PyObject* stg = PyDict_GetItemString(item.get(), "startTimeGMT"); // borrowed
        PyObject* stl = PyDict_GetItemString(item.get(), "startTimeLocal"); // borrowed; may be null (optional)
        GarminActivitySummary s;
        s.activityId = toQString(aid);
        s.startTimeGMT = toQString(stg);
        s.startTimeLocal = toQString(stl); // a borrowed null yields QString() — never a crash
        summaries.append(s);
    }

    // PyIter_Next returns null both at exhaustion and on an iteration error —
    // an error raised mid-iteration (e.g. a lazily-translated GarminError) must
    // be classified, not silently treated as a short listing.
    if (PyErr_Occurred())
        return classifyListException(module.get());

    out.kind = PyListOutcome::Success;
    out.activities = summaries;
    return out;
}

PyLoadTokensOutcome PyEmbeddedAdapter::loadTokens(const QString& tokenBlob)
{
    PyLoadTokensOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyLoadTokensOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // Path policy + import.
    PyRef module(importAdapterModule(modulePath));
    if (!module)
        return classifyLoadTokensException(nullptr);

    // Restore a session from the opaque OAuth
    // blob WITHOUT a password. GarminClient.from_tokens(blob) constructs the
    // library password-free and restores; a tampered/expired blob surfaces as
    // GarminError(kind='session_expired') (garmin_client classifies by exception
    // TYPE). Mirrors authenticate()'s class-then-call shape.
    PyRef clientClass(PyObject_GetAttrString(module.get(), "GarminClient"));
    if (!clientClass)
        return classifyLoadTokensException(module.get());

    PyRef client(PyObject_CallMethod(clientClass.get(), "from_tokens", "s", tokenBlob.toUtf8().constData()));
    if (!client)
        return classifyLoadTokensException(module.get());

    // Retain the restored client so downloadActivity() reuses the session
    // (same session model as authenticate()). GIL held (above), refcount safe.
    Py_XDECREF(m_client); // drop any prior session
    m_client = client.get();
    Py_INCREF(m_client);

    out.kind = PyLoadTokensOutcome::Success;
    return out;
}

PyProfileOutcome PyEmbeddedAdapter::fetchProfile()
{
    PyProfileOutcome out;

    // Step 1 — fail-safe before touching any interpreter API. Never throws.
    if (!Py_IsInitialized()) {
        out.kind = PyProfileOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("embedded Python unavailable"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // GIL held from here; the guard releases on every return below.
    GilGuard gil;

    // No retained session — authenticate()/loadTokens() must have succeeded
    // first. The password is not kept, so we cannot build a fresh
    // client here.
    if (m_client == nullptr) {
        out.kind = PyProfileOutcome::Unknown;
        // Developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("not authenticated"); // T208-ALLOW:I18N-TR-WRAP
        return out;
    }

    // garmin_client is needed only to resolve the GarminError type for
    // classification; it is already imported/cached from authenticate().
    PyRef module(importAdapterModule(modulePath));

    PyRef result(PyObject_CallMethod(m_client, "get_profile", nullptr));
    if (!result)
        return classifyProfileException(module.get());

    // A non-dict result is a contract breach, not a valid profile:
    // fold to Unknown rather than fabricate an empty Success.
    if (!PyDict_Check(result.get())) {
        out.kind = PyProfileOutcome::Unknown;
        // T208-ALLOW:I18N-TR-WRAP: dES-008 developer diagnostic: rawMessage is never displayed in the UI.
        out.rawMessage = QStringLiteral("garmin_client.get_profile() returned a non-dict result");
        return out;
    }

    // The Python adapter already did the defensive
    // key-name/plausibility extraction; this seam only marshals whichever of
    // the 3 keys are present. A missing key is a NORMAL Success outcome
    // (Garmin didn't have that field), never a failure.
    out.kind = PyProfileOutcome::Success;

    PyObject* dob = PyDict_GetItemString(result.get(), "dob"); // borrowed
    if (dob != nullptr && PyUnicode_Check(dob)) {
        out.hasDob = true;
        out.dob = toQString(dob);
    }

    PyObject* weight = PyDict_GetItemString(result.get(), "weight_kg"); // borrowed
    if (weight != nullptr && (PyFloat_Check(weight) || PyLong_Check(weight))) {
        out.hasWeightKg = true;
        out.weightKg = PyFloat_AsDouble(weight);
    }

    PyObject* height = PyDict_GetItemString(result.get(), "height_cm"); // borrowed
    if (height != nullptr && (PyFloat_Check(height) || PyLong_Check(height))) {
        out.hasHeightCm = true;
        out.heightCm = PyFloat_AsDouble(height);
    }

    PyErr_Clear(); // defensive: numeric conversion above never fails an otherwise-good Success
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

void PyEmbeddedAdapter::releaseModuleProvenanceLedgerForCurrentInterpreter()
{
    // interpreter-address reuse after Py_FinalizeEx()
    // is real (measured: INTERP_ADDR_REUSED=1), so a ledger key built from a
    // raw interpreter pointer must never survive finalization. A held
    // module's own address does NOT get reused while the ledger still owns
    // its strong reference (measured: HELD_MODULE_ADDR_REUSED=0 across a
    // million fresh allocations) — the hazard belongs only to an entry the
    // ledger has already dropped, so it is safe to walk the table and
    // release its references here, right before finalization.
    QHash<QString, PyObject*>& ledger = provenanceLedger();
    const QString prefix = QString::number(reinterpret_cast<quintptr>(PyInterpreterState_Get())) + QLatin1Char('\x1f');
    QHash<QString, PyObject*>::iterator it = ledger.begin();
    while (it != ledger.end()) {
        if (it.key().startsWith(prefix)) {
            Py_DECREF(it.value());
            it = ledger.erase(it);
        } else {
            ++it;
        }
    }
}
