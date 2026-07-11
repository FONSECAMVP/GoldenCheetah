"""DES-012 — stable adapter over python-garminconnect.

The *only* module in the Garmin Connect feature that imports `garminconnect`
directly. The C++ worker (DES-001) calls this adapter; nothing else in the
codebase imports the underlying library. Swapping `python-garminconnect` for
a community fork is a one-file change here.

Phase 2.2 status (REQ-by-REQ):
  - REQ-002 login / GarminError(kind='auth') translation: GREEN.
  - REQ-007 download_activity (fmt map + connection/rate_limit translation): GREEN.
  - REQ-003 / REQ-008 / REQ-012 / REQ-013: still raise
    NotImplementedError until their owning slice reaches GREEN.

The wider `_EXCEPTION_MAP` (rate_limit, connection, captcha, mfa_required,
token_permissions) lands with REQ-014 — REQ-002's bar is the auth path only.
"""

from __future__ import annotations

from collections.abc import Iterator
from typing import Any

try:
    import garminconnect as _gc
except ImportError:
    # Library not present in dev/test environments without the runtime dep
    # (python-garminconnect + curl_cffi). Tests monkeypatch `_gc` to a fake;
    # production builds bundle the wheel under DEC-011 / REQ-NF-Pkg-001.
    _gc = None


class GarminError(Exception):
    """GC-stable error type emitted by the adapter.

    Callers (worker → wizard) switch on .kind, never on the underlying
    library's exception classes. See DES-008 error-translation table.

    kind ∈ {'auth', 'rate_limit', 'connection', 'captcha',
            'mfa_required', 'token_permissions', 'unknown'}
    """

    def __init__(self, kind: str, message: str, original: Exception | None = None) -> None:
        self.kind = kind
        self.message = message
        self.original = original
        super().__init__(message)


class GarminClient:
    """Adapter — see DES-012 for surface and translation responsibilities."""

    def __init__(self, email: str, password: str, tokenstore_path: str) -> None:
        if _gc is None:
            raise GarminError(
                "unknown",
                "python-garminconnect is not installed; cannot construct GarminClient",
            )
        # Forward `password` straight into the library constructor and drop
        # the local reference; REQ-005 forbids retaining it on this adapter.
        # The library's own retention is its contract — DES-012 isolates it.
        # tokenstore_path is forwarded only — the library owns the path; the
        # adapter has no reason to retain it on `self` until REQ-012 needs it.
        #
        # NOTE(DEC-014 Option B — reconciliation deferred): under Option B the
        # library must be constructed AUTH-ONLY (in-memory session, no
        # tokenstore path) and C++ owns the single atomic 0600 write of the blob
        # exported by dump_tokens(). Fully removing this path forwarding is a
        # cross-cutting change (PyEmbeddedAdapter ctor signature, its pystub,
        # GarminAuthChain/AddCloudWizard wiring, and REQ-002's login tests that
        # assert the forwarded path) — LARGER than REQ-004's write slice, so it
        # is left to the __init__ reconciliation slice. REQ-004 adds the
        # dump/load blob surface (the Option-B mechanism) without leaving the
        # adapter itself writing anywhere; the library's own on-login write via
        # this path is what that later slice removes.
        self._garmin = _gc.Garmin(email, password, tokenstore_path)

    def login(self) -> dict[str, Any]:
        try:
            self._garmin.login()
        except _gc.exceptions.GarminConnectAuthenticationError as e:
            raise GarminError("auth", str(e) or "Authentication failed", e) from e
        return {
            "garmin_user_id": str(self._garmin.full_name_id),
            "display_name": self._garmin.display_name,
        }

    def dump_tokens(self) -> str:
        # REQ-004 / DEC-014 Option B: export the authenticated in-memory OAuth
        # session as an opaque, serializable blob. The adapter does NOT write it
        # to disk — C++ owns the single atomic 0600 write (DES-002/DES-006).
        # REQ-005: only OAuth bearer + refresh tokens live in this blob; the
        # password was handed to the library and never retained here.
        #
        # NOTE(DEC-014 OQ1): the exact library export method is unconfirmed
        # against the not-yet-bundled python-garminconnect wheel (no version pin
        # in repo; lib absent from .venv). On the current native engine this is
        # `dumps()`; confirm when the wheel is bundled (DES-007/Pkg). The adapter
        # contract (return a non-empty str) is pinned by pytest against a fake.
        return str(self._garmin.dumps())

    def load_tokens(self, token_str: str) -> None:
        # REQ-004 counterpart of dump_tokens(): restore an authenticated session
        # from a previously-exported blob (the REQ-006 resume path feeds this).
        #
        # NOTE(DEC-014 OQ1): real library import method unconfirmed (see above);
        # `loads()` on the native engine.
        #
        # DEC-014 OQ2 → REQ-NF-Compat-001(b): a tampered or server-side-
        # invalidated session surfaces from the library as an authentication
        # error; translate it to GC-stable kind='session_expired' — DISTINCT
        # from login's 'auth' and from 'token_permissions' — so the resume path
        # can route it to a re-login prompt. Classify by exception TYPE, never
        # by message content (LSN-006).
        try:
            self._garmin.loads(token_str)
        except _gc.exceptions.GarminConnectAuthenticationError as e:
            raise GarminError(
                "session_expired",
                str(e) or "Stored Garmin session is expired; please sign in again",
                e,
            ) from e

    def submit_mfa(self, code: str) -> dict[str, Any]:
        raise NotImplementedError("REQ-003 GREEN step not yet implemented")

    def list_activities_since(self, ts_gmt: str) -> Iterator[dict[str, Any]]:
        raise NotImplementedError("REQ-008 GREEN step not yet implemented")

    def download_activity(self, activity_id: str, fmt: str = "ORIGINAL") -> bytes:
        # REQ-007 / DEC-006: FIT is the default (dl_fmt=ORIGINAL); TCX is the
        # fallback the C++ readFile requests when Garmin has no FIT original
        # (DES-004 owns the fallback orchestration — this adapter is a thin,
        # single-format fetch). Map the GC-stable `fmt` to the library's enum
        # here so nothing above this seam knows the library's format type.
        fmt_map = {
            "ORIGINAL": self._garmin.ActivityDownloadFormat.ORIGINAL,
            "TCX": self._garmin.ActivityDownloadFormat.TCX,
        }
        if fmt not in fmt_map:
            # Reject before any network call — never forward a garbage dl_fmt.
            raise GarminError("unknown", f"unsupported download format {fmt!r}")
        try:
            # Classify by exception TYPE, then translate (LSN-006): only the
            # known-transient download errors become GC-stable kinds; anything
            # else propagates unchanged rather than inheriting a Garmin kind.
            data: bytes = self._garmin.download_activity(activity_id, dl_fmt=fmt_map[fmt])
        except _gc.exceptions.GarminConnectConnectionError as e:
            raise GarminError("connection", str(e) or "Could not reach Garmin Connect", e) from e
        except _gc.exceptions.GarminConnectTooManyRequestsError as e:
            raise GarminError("rate_limit", str(e) or "Garmin Connect is rate-limiting downloads", e) from e
        return data

    def get_profile(self) -> dict[str, Any]:
        raise NotImplementedError("REQ-013 GREEN step not yet implemented")

    def disconnect(self) -> None:
        raise NotImplementedError("REQ-012 GREEN step not yet implemented")
