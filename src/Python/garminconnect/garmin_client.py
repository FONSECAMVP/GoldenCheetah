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
