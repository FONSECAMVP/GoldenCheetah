"""TEST-005 fixture — scriptable stand-in for src/Python/garminconnect/garmin_client.py.

Consumed by testGarminConnectPyAdapter (garmin-py label). Mirrors the real
adapter's DES-012 surface exactly as PyEmbeddedAdapter sees it:

  - ``GarminClient(email, password)`` ctor shape (AUTH-ONLY; DEC-014 Option B /
    A3-R004-M3 — NO tokenstore path is forwarded, so the library self-writes no
    token file)
  - ``login() -> {'garmin_user_id': ..., 'display_name': ...}``
  - ``GarminError`` with ``.kind`` / ``.message``

Behavior is switched via the module-level ``SCENARIO`` attribute, driven from
the C++ test through ``PyRun_SimpleString`` (deterministic; env vars do not
propagate into os.environ after Py_Initialize). Ctor args are recorded in
``LAST_EMAIL`` / ``LAST_PASSWORD`` so the test can assert marshalling verbatim;
there is deliberately NO ``LAST_TOKENSTORE`` — T-016 asserts the adapter
forwards no such path.

The REAL adapter's behavior stays covered by its own pytest suite
(src/Python/garminconnect/tests/) — two seams, tested on their own sides,
meeting at the DES-012 contract (see design.md DES-013 "Tests").
"""

SCENARIO = "success"

LAST_EMAIL = None
LAST_PASSWORD = None

# TEST-009 / REQ-007 — download marshalling. download_activity() records its
# args here so the C++ side can assert verbatim marshalling, and returns this
# exact binary payload on the "dl_success" scenario. The payload deliberately
# contains embedded NUL (0x00) and high bytes (0xff/0xfe) so a strlen-based
# (NUL-truncating) marshalling bug is caught — the C++ test hardcodes the same
# bytes and asserts full-length QByteArray equality.
LAST_ACTIVITY_ID = None
LAST_FMT = None
DL_PAYLOAD = b"\x00\x01\x02FIT\x00\xff\xfe\x0a"

# T-036 / REQ-003 (MFA) — submit_mfa() records the OTP it saw so the C++ side can
# assert verbatim marshalling through the REAL bridge (PyEmbeddedAdapter.submitMfa
# → m_client.submit_mfa). Disjoint mfa_* SCENARIO values drive login()'s
# mfa_required sentinel and submit_mfa()'s success / auth-error branches.
LAST_MFA_CODE = None

# T-043 / REQ-008 Slice A — list_activities_since() records the since-timestamp
# it saw so the C++ side can assert verbatim forwarding through the REAL bridge
# (PyEmbeddedAdapter.listActivitiesSince → m_client.list_activities_since). The
# canned summaries carry activityId + startTimeGMT (the two keys the C++ marshaller
# reads); an int activityId proves the marshaller str()-normalizes non-unicode.
LAST_SINCE_GMT = None
LIST_SUMMARIES = [
    {"activityId": 1001, "startTimeGMT": "2026-07-01 06:30:00"},
    {"activityId": 1002, "startTimeGMT": "2026-07-03 18:05:11"},
]


class GarminError(Exception):
    """Shape-compatible with the production GarminError (.kind / .message)."""

    def __init__(self, kind, message, original=None):
        self.kind = kind
        self.message = message
        self.original = original
        super().__init__(message)


# Reviewer delta-fix #1 regression fixture — a metaclass whose __getattribute__
# makes __module__/__qualname__ resolve to a non-str object with a suspicious
# __str__. Overriding __getattribute__ (rather than defining __module__/
# __qualname__ directly in the metaclass body) is required: the compiler
# injects its own __qualname__ entry into every class namespace, so a
# same-named class-body definition collides with it and CPython rejects the
# metaclass's own creation ("type __qualname__ must be a str, not property").
# Intercepting at __getattribute__ time sidesteps that collision entirely.
class NonStrAttrsMeta(type):
    def __getattribute__(cls, name):
        if name in ("__module__", "__qualname__"):
            class _Leaky:
                def __str__(self):
                    return "LEAKED-SECRET-VIA-%s" % name.strip("_").upper()
            return _Leaky()
        return type.__getattribute__(cls, name)


class NonStrAttrsError(Exception, metaclass=NonStrAttrsMeta):
    """pyExceptionTypeName() must treat these non-str attrs as absent, never
    stringify them — see the reviewer delta-fix #1 test in
    testGarminConnectPyAdapter.cpp."""


# Reviewer delta-fix #2 regression fixture — __qualname__ access raises
# (simulating "absent"), forcing the tp_name fallback branch. A normal `class`
# statement always gives a heap type a BARE (unqualified) raw tp_name — the
# "module.Class" look of repr()/type.__module__/type.__qualname__ is
# reconstructed on demand by CPython's generic getters, not baked into
# tp_name itself. A real extension type authored directly in C (e.g.
# curl_cffi's) CAN set tp_name to an already-dotted static string, so this
# fixture reproduces that shape explicitly: type()'s 3-arg form does not
# validate that `name` is a bare identifier, so passing a dotted string here
# makes the resulting type's raw tp_name literally
# "curl_cffi.requests.exceptions.ImpersonateError" — proving the old code's
# unconditional moduleName+"."+qualName concatenation double-prefixed it.
class QualnameRaisesMeta(type):
    def __getattribute__(cls, name):
        if name == "__qualname__":
            raise AttributeError("qualname intentionally unavailable")
        return type.__getattribute__(cls, name)


TpNameFallbackError = QualnameRaisesMeta(
    "curl_cffi.requests.exceptions.ImpersonateError", (Exception,), {}
)
"""pyExceptionTypeName() must fall back to tp_name directly here, without
doubling the module prefix — see the reviewer delta-fix #2 test in
testGarminConnectPyAdapter.cpp."""


# Reviewer delta-fix #3 regression fixture — __module__ resolves to a
# genuine `str` (passes PyUnicode_Check, so strictUnicodeAttr() accepts it),
# but its CONTENT is not a real module path: Python lets any ordinary class
# have __module__ reassigned to arbitrary string content, so a foreign or
# adversarial exception type could smuggle secret-shaped text through here.
# Unlike NonStrAttrsMeta above (wrong TYPE), this fixture is deliberately the
# wrong SHAPE of an otherwise-valid str, exercising the grammar check rather
# than the PyUnicode_Check gate.
class BadModuleShapeMeta(type):
    def __getattribute__(cls, name):
        if name == "__module__":
            return "sk-LEAKED-SECRET-abc123"
        return type.__getattribute__(cls, name)


class BadModuleShapeError(Exception, metaclass=BadModuleShapeMeta):
    """pyExceptionTypeName() must reject a non-identifier-shaped __module__
    string content, even though it passes PyUnicode_Check — see the reviewer
    delta-fix #3 test in testGarminConnectPyAdapter.cpp."""


# Reviewer delta-fix #4 regression fixtures — grammar-shaped is not the same
# as safe: plenty of real secrets (API keys/tokens) are themselves
# identifier-shaped (e.g. "hunter2", "sk_live_abc123") and would pass
# isSafeDottedName() cleanly, so the grammar check alone (delta-fix #3)
# cannot close this gap. __module__/__qualname__ are ordinary writable str
# attributes on any class — no metaclass trickery needed to reassign them,
# unlike delta-fix #1's non-str fixture.
class ForeignModuleError(Exception):
    """__module__ reassigned to a genuine, grammar-valid identifier
    ("hunter2") that is NOT one of this integration's actual dependency-
    chain module roots. pyExceptionTypeName() must reject it as foreign
    rather than surface it — see the reviewer delta-fix #4 negative-case test
    in testGarminConnectPyAdapter.cpp."""


ForeignModuleError.__module__ = "hunter2"


class AllowlistedThirdPartyError(Exception):
    """__module__ reassigned to an ALLOWLISTED module path
    (curl_cffi.requests.exceptions) that this class is NOT actually an
    attribute of — a claimed-but-unbacked module string. Originally written
    (delta-fix #4) to prove an allowlisted module string still produces its
    full qualified name; delta-fix #6 found the allowlist-membership check
    alone doesn't verify genuine provenance, so THIS fixture is exactly that
    spoof and is now repurposed as the delta-fix #6 regression test. See
    testGarminConnectPyAdapter.cpp."""


AllowlistedThirdPartyError.__module__ = "curl_cffi.requests.exceptions"


# Reviewer delta-fix #6 genuine-positive-path fixture — the pystub cannot
# fabricate a class that is ACTUALLY an attribute of a real imported module
# without importing a real module, so this imports the real, already-
# installed `requests` package (transitively bundled with curl_cffi in the
# real adapter's dependency chain — see requirements.txt) and raises one of
# its genuine exception classes unmodified. Its __module__/__qualname__ are
# never touched — they are exactly what a real dependency-backed exception
# looks like, and isGenuineAllowlistedException()'s sys.modules walk must
# resolve `requests.exceptions.ConnectionError` to this exact, real type
# object. Guarded import: if `requests` isn't importable in some environment,
# the scenario reports that explicitly rather than silently skipping.
try:
    import requests.exceptions as _real_requests_exceptions
except ImportError:
    _real_requests_exceptions = None


# Reviewer delta-fix #5 regression fixtures — the delta-fix #4 allowlist
# check only fired when moduleName was non-empty, leaving two bypasses open
# whenever moduleName is empty (absent/non-str/invalid-shape): it fell
# through to the qualname-or-tp_name logic UNGUARDED, and both qualName and
# tp_name are independently attacker-settable (same as __module__ itself).
class ModuleAndQualnameRaiseMeta(type):
    def __getattribute__(cls, name):
        if name in ("__module__", "__qualname__"):
            raise AttributeError("%s intentionally unavailable" % name)
        return type.__getattribute__(cls, name)


# Bypass (a): BOTH __module__ AND __qualname__ are absent (access raises for
# each independently), so pyExceptionTypeName() has no signal left except
# tp_name — genuinely exercising the tp_name-fallback branch (unlike a
# fixture that leaves __qualname__ readable, which would resolve via the
# qualname-concatenation path instead, see bypass (b)). tp_name is crafted
# here to a grammar-valid, secret-shaped string via type()'s 3-arg form
# (same technique as delta-fix #2's TpNameFallbackError fixture).
ModuleAbsentTpNameBypassError = ModuleAndQualnameRaiseMeta("sk_live_secret_abc", (Exception,), {})
"""pyExceptionTypeName() must reject this as foreign — an absent __module__
must not let a crafted tp_name substitute for module validation. See the
reviewer delta-fix #5 bypass-(a) test in testGarminConnectPyAdapter.cpp."""


class ModuleTypeConfusionMeta(type):
    def __getattribute__(cls, name):
        if name == "__module__":
            class _Leaky:
                def __str__(self):
                    return "LEAKED-VIA-MODULE"
            return _Leaky()
        return type.__getattribute__(cls, name)


# Bypass (b): __module__ resolves to a non-str object (delta-fix #1 shape),
# so moduleName is empty, but __qualname__ is a genuine, grammar-valid str
# ("hunter2") — pyExceptionTypeName() must not default moduleName to
# "builtins" and emit "builtins.hunter2".
class ModuleNonStrQualnameBypassError(Exception, metaclass=ModuleTypeConfusionMeta):
    """pyExceptionTypeName() must reject this as foreign — an absent/non-str
    __module__ must not let a merely-valid-shaped __qualname__ default to a
    "builtins.<qualname>" name. See the reviewer delta-fix #5 bypass-(b) test
    in testGarminConnectPyAdapter.cpp."""


ModuleNonStrQualnameBypassError.__qualname__ = "hunter2"


# Reviewer delta-fix #7 regression fixture — isGenuineAllowlistedException()
# only proves tp_name resolves to a REAL, genuinely-registered object; "real"
# and "allowlisted" are not the same guarantee. This fixture makes __module__
# resolve to a genuinely ALLOWLISTED string ("builtins", passing delta-fix
# #5's gate) while __qualname__ is absent (forcing the tp_name-fallback
# branch), and tp_name is crafted to a module root ("os") that is NOT on
# kAllowedModuleRoots. Unlike a merely-claimed, unbacked tp_name (which the
# EXISTING provenance walk already rejects on its own, making a RED/GREEN
# distinction unobservable), this fixture is genuinely, really registered as
# an attribute of the real, already-imported `os` module — so
# isGenuineAllowlistedException()'s walk WOULD succeed without delta-fix #7's
# new module-root check, proving the fix closes a real, otherwise-leaking gap
# rather than one the provenance walk already caught for an unrelated reason.
import os as _os_for_delta_fix_7_fixture


class ModuleBuiltinsQualnameRaisesMeta(type):
    def __getattribute__(cls, name):
        if name == "__module__":
            return "builtins"
        if name == "__qualname__":
            raise AttributeError("qualname intentionally unavailable")
        return type.__getattribute__(cls, name)


TpNameNotAllowlistedError = ModuleBuiltinsQualnameRaisesMeta(
    "os.TpNameNotAllowlistedError", (Exception,), {}
)
_os_for_delta_fix_7_fixture.TpNameNotAllowlistedError = TpNameNotAllowlistedError
"""pyExceptionTypeName() must reject this as foreign — an allowlisted
__module__ claim must not let a genuinely-real (really registered on `os`)
but non-allowlisted tp_name substitute for module validation. See the
reviewer delta-fix #7 test in testGarminConnectPyAdapter.cpp."""


class GarminClient:
    def __init__(self, email, password):
        # AUTH-ONLY (DEC-014 Option B): exactly (email, password); no tokenstore
        # path is accepted — a 3rd positional would raise TypeError, catching any
        # adapter that still forwards a path.
        global LAST_EMAIL, LAST_PASSWORD
        LAST_EMAIL = email
        LAST_PASSWORD = password

    # T-013 / REQ-004 / DEC-014 Option B — the adapter exports the authenticated
    # session as an opaque blob (dump_tokens) which PyEmbeddedAdapter surfaces on
    # PyAuthOutcome.tokenBlob for C++ to persist (0600). load_tokens restores it.
    # Canned fixture so a garmin-py C++ test can exercise the blob surfacing.
    TOKEN_BLOB = '{"oauth1":"OA1-stub","oauth2":"OA2-stub"}'

    def dump_tokens(self):
        return self.TOKEN_BLOB

    def load_tokens(self, token_str):
        if SCENARIO == "load_session_expired":
            raise GarminError("session_expired", "stub: stored session expired")
        return None

    # REQ-007 closure (Slice 1) — password-free restore counterpart of the real
    # adapter's classmethod. PyEmbeddedAdapter.loadTokens() calls
    # GarminClient.from_tokens(blob); this stub records the blob and honours the
    # load_session_expired / connection scenarios so a garmin-py C++ test could
    # exercise the SessionExpired / Network / Success branches.
    LAST_TOKEN_BLOB = None

    @classmethod
    def from_tokens(cls, token_str):
        global LAST_TOKEN_BLOB
        LAST_TOKEN_BLOB = token_str
        if SCENARIO == "load_connection":
            raise GarminError("connection", "stub: restore connection refused")
        self = cls.__new__(cls)
        self.load_tokens(token_str)
        return self

    def login(self):
        if SCENARIO == "success":
            return {"garmin_user_id": "uid-123", "display_name": "Alice Rider"}
        if SCENARIO == "success_unicode":
            return {"garmin_user_id": "uid-üñî-123",
                    "display_name": "Zoë Åström \U0001f6b4"}
        # REQ-002 / TEST-005 / A3-R002-TR-05 — malformed-result contract
        # breaches of the DES-012 seam. login() must return a dict carrying
        # garmin_user_id + display_name; these two scenarios violate that so the
        # adapter's defensive branches (PyEmbeddedAdapter.cpp: non-dict result;
        # dict missing required keys) are exercised. Both must fold to Unknown
        # with an explanatory message — NEVER a spurious Success.
        if SCENARIO == "non_dict_result":
            return ["not", "a", "dict"]  # login() returned a non-dict
        if SCENARIO == "missing_keys":
            return {"session": "opaque-token"}  # dict, but no garmin_user_id/display_name
        # T-036 / REQ-003 — MFA-required sentinel: login() returns
        # {"mfa_required": True} (instead of an identity dict) so the real bridge
        # exercises PyEmbeddedAdapter's PyDict_GetItemString(...,"mfa_required")
        # detection + retained-client path. submit_mfa() (below) then resumes.
        if SCENARIO == "mfa_required":
            return {"mfa_required": True}
        if SCENARIO == "auth_error":
            raise GarminError("auth", "stub: bad credentials")
        if SCENARIO == "connection_error":
            raise GarminError("connection", "stub: connection refused")
        if SCENARIO == "rate_limit_error":
            raise GarminError("rate_limit", "stub: too many requests")
        if SCENARIO == "value_error":
            raise ValueError("stub: not a garmin error")
        if SCENARIO == "type_confusion_error":
            raise NonStrAttrsError("stub: non-str __module__/__qualname__")
        if SCENARIO == "tp_name_fallback_error":
            raise TpNameFallbackError("stub: __qualname__ raises, tp_name already module-qualified")
        if SCENARIO == "bad_module_shape_error":
            raise BadModuleShapeError("stub: __module__ is a str but not name-shaped")
        if SCENARIO == "foreign_module_error":
            raise ForeignModuleError("stub: __module__ is identifier-shaped but not allowlisted")
        if SCENARIO == "allowlisted_third_party_error":
            raise AllowlistedThirdPartyError("stub: __module__ is a real allowlisted third-party path")
        if SCENARIO == "module_absent_tp_name_bypass_error":
            raise ModuleAbsentTpNameBypassError("stub: __module__ absent, tp_name crafted")
        if SCENARIO == "module_non_str_qualname_bypass_error":
            raise ModuleNonStrQualnameBypassError("stub: __module__ non-str, __qualname__ = 'hunter2'")
        if SCENARIO == "genuine_allowlisted_module_error":
            if _real_requests_exceptions is None:
                raise GarminError("unknown", "stub: real requests package unavailable in this environment")
            raise _real_requests_exceptions.ConnectionError("stub: genuine dependency-backed exception")
        if SCENARIO == "tp_name_not_allowlisted_error":
            raise TpNameNotAllowlistedError("stub: __module__ allowlisted, tp_name module root is not")
        raise GarminError("unknown", "stub: unrecognized scenario %r" % (SCENARIO,))

    # T-036 / REQ-003 (MFA) — resume the pending MFA session on the SAME retained
    # client (PyEmbeddedAdapter.submitMfa calls m_client.submit_mfa(code)). Mirrors
    # the production adapter's success/auth-error surface; the code is recorded so
    # the C++ side can assert verbatim marshalling through the real bridge.
    def submit_mfa(self, code):
        global LAST_MFA_CODE
        LAST_MFA_CODE = code
        if SCENARIO == "mfa_success":
            return {"garmin_user_id": "uid-mfa-77", "display_name": "MFA Rider"}
        if SCENARIO == "mfa_auth_error":
            raise GarminError("auth", "stub: invalid one-time code")
        raise GarminError("unknown", "stub: unrecognized mfa scenario %r" % (SCENARIO,))

    # TEST-009 / REQ-007 — download_activity mirrors garmin_client.GarminClient
    # as PyEmbeddedAdapter.downloadActivity() calls it: (activity_id, fmt) ->
    # bytes, raising GarminError(.kind) for translated failures. Behaviour is
    # switched on dl_* SCENARIO values (disjoint from the login scenarios above,
    # so a single SCENARIO switch drives both without collision).
    def download_activity(self, activity_id, fmt="ORIGINAL"):
        global LAST_ACTIVITY_ID, LAST_FMT
        LAST_ACTIVITY_ID = activity_id
        LAST_FMT = fmt
        if SCENARIO == "dl_success":
            return DL_PAYLOAD
        if SCENARIO == "dl_non_bytes":
            return "i am a str, not bytes"  # contract breach → Unknown, never Success
        if SCENARIO == "dl_connection":
            raise GarminError("connection", "stub: download connection refused")
        if SCENARIO == "dl_rate_limit":
            raise GarminError("rate_limit", "stub: download rate-limited")
        if SCENARIO == "dl_value_error":
            raise ValueError("stub: not a garmin error (download)")
        raise GarminError("unknown", "stub: unrecognized dl scenario %r" % (SCENARIO,))

    # T-043 / REQ-008 Slice A — list_activities_since mirrors the production
    # garmin_client.GarminClient as PyEmbeddedAdapter.listActivitiesSince() calls
    # it: (ts_gmt) -> Iterator[dict], raising GarminError(.kind) for translated
    # failures. Behaviour is switched on list_* SCENARIO values (disjoint from the
    # login/dl scenarios above, so a single SCENARIO switch drives them all). The
    # since-timestamp is recorded verbatim; on success an ITERATOR (not a list) is
    # returned so the C++ side proves it marshals via the iterator protocol.
    def list_activities_since(self, ts_gmt):
        global LAST_SINCE_GMT
        LAST_SINCE_GMT = ts_gmt
        if SCENARIO == "list_success":
            return iter(LIST_SUMMARIES)
        if SCENARIO == "list_empty":
            return iter([])
        if SCENARIO == "list_non_iterable":
            return 42  # contract breach → Unknown, never Success
        if SCENARIO == "list_bad_item":
            # TEST-053 / F3 — the list IS iterable but yields a NON-dict item
            # (a valid dict first, then an int). PyEmbeddedAdapter's per-item
            # PyDict_Check guard must fold the WHOLE listing to Unknown, never a
            # partial Success carrying a phantom empty-id/empty-timestamp row.
            return iter([{"activityId": 1001, "startTimeGMT": "2026-07-01 06:30:00"}, 42])
        if SCENARIO == "list_connection":
            raise GarminError("connection", "stub: listing connection refused")
        if SCENARIO == "list_rate_limit":
            raise GarminError("rate_limit", "stub: listing rate-limited")
        if SCENARIO == "list_value_error":
            raise ValueError("stub: not a garmin error (listing)")
        raise GarminError("unknown", "stub: unrecognized list scenario %r" % (SCENARIO,))

    # REQ-013 (DEC-050 first slice) — get_profile mirrors garmin_client.GarminClient
    # as PyEmbeddedAdapter.fetchProfile() calls it: () -> dict carrying only the
    # keys it confidently found among dob/weight_kg/height_cm (mirroring the REAL
    # adapter's own defensive extraction — this stub returns the ALREADY-extracted
    # shape, since PyEmbeddedAdapter only marshals whichever of the 3 keys are
    # present, it never re-parses raw Garmin field names). Behaviour is switched on
    # profile_* SCENARIO values (disjoint from the other scenarios above).
    def get_profile(self):
        if SCENARIO == "profile_full":
            return {"dob": "1985-06-15", "weight_kg": 72.5, "height_cm": 178.0}
        if SCENARIO == "profile_partial":
            return {"dob": "1990-01-02"}  # weight/height absent — a normal Success
        if SCENARIO == "profile_empty":
            return {}  # Garmin had none of the 3 fields — still a normal Success
        if SCENARIO == "profile_non_dict":
            return "i am a str, not a dict"  # contract breach → Unknown, never Success
        if SCENARIO == "profile_connection":
            raise GarminError("connection", "stub: profile connection refused")
        if SCENARIO == "profile_value_error":
            raise ValueError("stub: not a garmin error (profile)")
        raise GarminError("unknown", "stub: unrecognized profile scenario %r" % (SCENARIO,))
