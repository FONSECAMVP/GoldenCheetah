"""DES-012 — stable adapter over python-garminconnect.

The *only* module in the Garmin Connect feature that imports `garminconnect`
directly. The C++ worker (DES-001) calls this adapter; nothing else in the
codebase imports the underlying library. Swapping `python-garminconnect` for
a community fork is a one-file change here.

Phase 2.2 status: STUB. Each method raises NotImplementedError until its
owning REQ reaches GREEN. TEST-NNN files live under tests/.
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
        raise NotImplementedError("REQ-002 GREEN step not yet implemented")

    def login(self) -> dict[str, Any]:
        raise NotImplementedError("REQ-002 GREEN step not yet implemented")

    def submit_mfa(self, code: str) -> dict[str, Any]:
        raise NotImplementedError("REQ-003 GREEN step not yet implemented")

    def list_activities_since(self, ts_gmt: str) -> Iterator[dict[str, Any]]:
        raise NotImplementedError("REQ-008 GREEN step not yet implemented")

    def download_activity(self, activity_id: str, fmt: str = "ORIGINAL") -> bytes:
        raise NotImplementedError("REQ-007 GREEN step not yet implemented")

    def get_profile(self) -> dict[str, Any]:
        raise NotImplementedError("REQ-013 GREEN step not yet implemented")

    def disconnect(self) -> None:
        raise NotImplementedError("REQ-012 GREEN step not yet implemented")
