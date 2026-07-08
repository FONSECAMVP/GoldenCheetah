"""TEST-005 fixture — scriptable stand-in for src/Python/garminconnect/garmin_client.py.

Consumed by testGarminConnectPyAdapter (garmin-py label). Mirrors the real
adapter's DES-012 surface exactly as PyEmbeddedAdapter sees it:

  - ``GarminClient(email, password, tokenstore_path)`` ctor shape
  - ``login() -> {'garmin_user_id': ..., 'display_name': ...}``
  - ``GarminError`` with ``.kind`` / ``.message``

Behavior is switched via the module-level ``SCENARIO`` attribute, driven from
the C++ test through ``PyRun_SimpleString`` (deterministic; env vars do not
propagate into os.environ after Py_Initialize). Ctor args are recorded in
``LAST_EMAIL`` / ``LAST_PASSWORD`` / ``LAST_TOKENSTORE`` so the test can
assert marshalling verbatim.

The REAL adapter's behavior stays covered by its own pytest suite
(src/Python/garminconnect/tests/) — two seams, tested on their own sides,
meeting at the DES-012 contract (see design.md DES-013 "Tests").
"""

SCENARIO = "success"

LAST_EMAIL = None
LAST_PASSWORD = None
LAST_TOKENSTORE = None

# TEST-009 / REQ-007 — download marshalling. download_activity() records its
# args here so the C++ side can assert verbatim marshalling, and returns this
# exact binary payload on the "dl_success" scenario. The payload deliberately
# contains embedded NUL (0x00) and high bytes (0xff/0xfe) so a strlen-based
# (NUL-truncating) marshalling bug is caught — the C++ test hardcodes the same
# bytes and asserts full-length QByteArray equality.
LAST_ACTIVITY_ID = None
LAST_FMT = None
DL_PAYLOAD = b"\x00\x01\x02FIT\x00\xff\xfe\x0a"


class GarminError(Exception):
    """Shape-compatible with the production GarminError (.kind / .message)."""

    def __init__(self, kind, message, original=None):
        self.kind = kind
        self.message = message
        self.original = original
        super().__init__(message)


class GarminClient:
    def __init__(self, email, password, tokenstore_path):
        global LAST_EMAIL, LAST_PASSWORD, LAST_TOKENSTORE
        LAST_EMAIL = email
        LAST_PASSWORD = password
        LAST_TOKENSTORE = tokenstore_path

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
        if SCENARIO == "auth_error":
            raise GarminError("auth", "stub: bad credentials")
        if SCENARIO == "connection_error":
            raise GarminError("connection", "stub: connection refused")
        if SCENARIO == "rate_limit_error":
            raise GarminError("rate_limit", "stub: too many requests")
        if SCENARIO == "value_error":
            raise ValueError("stub: not a garmin error")
        raise GarminError("unknown", "stub: unrecognized scenario %r" % (SCENARIO,))

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
