"""Adapter token export/import.

Acceptance (Python half): the adapter exposes the opaque
OAuth session as a serializable blob via ``dump_tokens()`` and restores it via
``load_tokens()`` — the *pair* whose round-trip is the Python contract.
C++ owns the atomic 0600 write of the blob; the adapter
never writes the token file itself.

Coverage:
  - acceptance encoded (round-trip):     test_dump_then_load_round_trips_the_session_blob
  - password never persisted:            test_dumped_blob_never_contains_the_password
  - negative path (must-have):           test_load_tampered_or_expired_blob_raises_session_expired
    (a server-side-invalidated / tampered
     session surfaces as GarminError(kind='session_expired'), a kind DISTINCT
     from 'token_permissions' and from login's 'auth'.)

The exact underlying library export/import method name is unconfirmed against
the not-yet-bundled wheel; this suite pins the *adapter* contract against a
monkeypatched fake, so it is stable regardless — the adapter's NOTE marks the
real call site.
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


class _FakeGarmin:
    """Fake of garminconnect.Garmin exposing only the surface the adapter's
    dump/load path touches: an in-memory OAuth session it can serialize to an
    opaque string and restore from one. A ``password`` handed to the ctor is
    NOT part of the serialized session.

    Each instance is seeded with a DISTINCT session (a monotonic per-instance
    counter, independent of the ctor args, which are identical for the src and
    dst clients in the round-trip test). This is what makes the round-trip test
    able to KILL a no-op ``load_tokens()``: a fresh dst instance starts life
    holding a session that differs from the src's blob, so if ``loads()`` were
    never actually invoked (e.g. body replaced by ``pass``) the dst would still
    dump its OWN seeded session and the round-trip assertion would FAIL. With a
    real ``load_tokens()`` the dst's session is overwritten with the src blob and
    the two dumps match. (A hardcoded-identical seed made "restored" and
    "never-touched" indistinguishable.)
    """

    _instance_counter = 0

    class _InnerClient:
        """dumps()/loads() live on the inner `.client` object on
        the real library, not on Garmin itself — see garmin_client.py's
        dump_tokens()/load_tokens()."""

        def __init__(self, outer: _FakeGarmin) -> None:
            self._outer = outer

        def dumps(self) -> str:
            import json

            return json.dumps(self._outer._session)

        def loads(self, token_str: str) -> None:
            import json

            try:
                data = json.loads(token_str)
                if not isinstance(data, dict) or "oauth1_token" not in data:
                    raise ValueError("missing oauth1_token")
            except Exception as e:
                # Confirmed against the real,
                # installed python-garminconnect 0.3.15 wheel by direct probe:
                # Garmin().client.loads(x) for x in '', '}{ bad', '{}',
                # '{"real":"blob"}' all raise GarminConnectConnectionError,
                # never GarminConnectAuthenticationError). The real
                # Client.loads() wraps ANY structural failure — bad JSON,
                # missing keys, even its OWN internal AuthenticationError —
                # into GarminConnectConnectionError. Modeling this as an auth
                # error (the old fake here) is exactly the mistake to avoid:
                # match the real wheel's behavior, not the adapter's
                # assumption.
                raise _FakeConnError("Token extraction loads() structurally failed") from e
            self._outer._session = data

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # AUTH-ONLY construction — no tokenstore path.
        # **_ swallows return_on_mfa=True (unused on this path).
        self.email = email
        self._password = password  # library's own retention; never serialized
        # Per-instance monotonic serial → distinct seeded session per instance,
        # so no two freshly-constructed fakes share OAuth material.
        _FakeGarmin._instance_counter += 1
        serial = _FakeGarmin._instance_counter
        # The "session" is the OAuth material the real library holds post-login.
        self._session = {
            "oauth1_token": f"OA1-abc-{serial}",
            "oauth2_token": f"OA2-xyz-{serial}",
        }
        self.client = _FakeGarmin._InnerClient(self)


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthError,
        GarminConnectConnectionError=_FakeConnError,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_dump_then_load_round_trips_the_session_blob(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Acceptance (Python half) — dump_tokens()/load_tokens() form a
    lossless pair: a blob exported from one authenticated session restores the
    identical session on a fresh adapter instance.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    src = GarminClient("a@example.com", "pw")
    blob = src.dump_tokens()
    assert isinstance(blob, str) and blob, "dump_tokens() must return a non-empty opaque string blob"

    # A brand-new adapter (as a resume path would build). It is seeded
    # with its OWN, DISTINCT session — so before restore it dumps something other
    # than src's blob. This precondition is what lets the round-trip assertion
    # below detect a no-op load_tokens(): if loads() were never invoked the dst
    # would still hold (and dump) its own untouched session, not src's blob.
    dst = GarminClient("a@example.com", "pw")
    assert dst.dump_tokens() != blob, (
        "test precondition — a fresh dst must start with a session distinct from "
        "src's blob, otherwise a no-op load_tokens() would pass vacuously"
    )

    dst.load_tokens(blob)

    assert dst.dump_tokens() == blob, (
        "load_tokens() must restore exactly the session that dump_tokens() "
        "exported — the round-trip is the Python contract"
    )


def test_dumped_blob_never_contains_the_password(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Only OAuth bearer + refresh tokens are stored; the Garmin
    password must never appear in the serialized blob C++ writes to disk.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    secret = "s3cret-never-persisted"
    client = GarminClient("a@example.com", secret)
    blob = client.dump_tokens()

    assert secret not in blob, (
        "the password must never be serialized into the token blob " "that C++ persists to tokens.json"
    )


def test_load_tampered_or_expired_blob_raises_session_expired(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Restoring a tampered or
    server-side-invalidated session raises GarminError(kind='session_expired'),
    a kind DISTINCT from 'token_permissions' and from login's 'auth', so the
    resume path can route it to a re-login prompt.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    client = GarminClient("a@example.com", "pw")
    with pytest.raises(GarminError) as excinfo:
        client.load_tokens("}{ not valid json — tampered blob")

    assert excinfo.value.kind == "session_expired", (
        "A tampered/expired session must map to GC-stable kind='session_expired' "
        "(distinct from 'token_permissions'/'auth')"
    )
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"
