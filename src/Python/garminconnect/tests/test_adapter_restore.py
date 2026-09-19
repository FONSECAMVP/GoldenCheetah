"""REQ-007 closure (Slice 1) — session restore from a stored OAuth blob.

The C++ open() path (a fresh CloudService session with NO password — REQ-005)
becomes download-capable by restoring the stored tokens (REQ-006). The adapter
exposes this as GarminClient.from_tokens(blob): construct the library
password-free and load the blob. A tampered/expired blob surfaces as a GC-stable
GarminError(kind='session_expired') (classified by exception TYPE — LSN-006 —
so DES-008 / REQ-NF-Compat-001(b) can route it to a fresh-SSO prompt, distinct
from login's 'auth').

Cites: REQ-007, REQ-005, REQ-006, REQ-NF-Compat-001(b); DEC-014 Option B,
DES-012 (adapter as stable seam), LSN-006 (classify by type, not message).
"""

from __future__ import annotations

import types
from typing import Any

import pytest

from gc_garmin_adapter.garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeConnError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeGarminNoArg:
    """Fake of garminconnect.Garmin supporting the password-free construction
    from_tokens() uses. Records the blob handed to loads().

    B-STAGE9-11: loads() lives on the inner `.client` object on the real
    library, not on Garmin itself — see garmin_client.py's load_tokens().
    """

    last_loaded: str | None = None

    def __init__(self, *args: Any) -> None:
        # from_tokens must construct WITHOUT a password (REQ-005). A stray
        # positional would be caught here.
        assert args == (), "from_tokens must construct the library password-free (no positional args)"
        self.client = types.SimpleNamespace(loads=self._loads)

    def _loads(self, token_str: str) -> None:
        _FakeGarminNoArg.last_loaded = token_str


class _ExpiredGarminNoArg(_FakeGarminNoArg):
    def _loads(self, token_str: str) -> None:  # noqa: ARG002
        # B-STAGE9-10 repair BLOCKING-2 (confirmed against the real, installed
        # python-garminconnect 0.3.15 wheel by direct probe: Garmin().client.
        # loads(x) for any structurally-bad x raises GarminConnectConnectionError,
        # never GarminConnectAuthenticationError). Modeling this as an auth
        # error is the DEC-014 OQ1 lesson: match the real wheel, not the
        # adapter's assumption.
        raise _FakeConnError("Token extraction loads() structurally failed")


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthError,
        GarminConnectConnectionError=_FakeConnError,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_from_tokens_restores_session_password_free(monkeypatch: pytest.MonkeyPatch) -> None:
    _FakeGarminNoArg.last_loaded = None
    _install_fake_gc(monkeypatch, _FakeGarminNoArg)

    blob = '{"oauth1":"OA1","oauth2":"OA2.refresh"}'
    client = GarminClient.from_tokens(blob)

    assert isinstance(client, GarminClient), "from_tokens must return a restored GarminClient"
    assert _FakeGarminNoArg.last_loaded == blob, "the blob must be forwarded VERBATIM to the library loads()"


def test_from_tokens_expired_blob_maps_to_session_expired(monkeypatch: pytest.MonkeyPatch) -> None:
    _install_fake_gc(monkeypatch, _ExpiredGarminNoArg)

    with pytest.raises(GarminError) as excinfo:
        GarminClient.from_tokens('{"stale":"blob"}')

    assert excinfo.value.kind == "session_expired", (
        "a tampered/expired stored session must map to GC-stable kind='session_expired' "
        "(distinct from login's 'auth') so the resume path routes to a fresh-SSO prompt"
    )
    assert excinfo.value.message, "message must be non-empty for UI display"
