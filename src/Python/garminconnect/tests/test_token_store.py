"""T-013 — REQ-004 (write path, Python side): adapter token export/import.

REQ-004 acceptance (Python half): the adapter (DES-012) exposes the opaque
OAuth session as a serializable blob via ``dump_tokens()`` and restores it via
``load_tokens()`` — the *pair* whose round-trip is REQ-004's Python contract.
C++ owns the atomic 0600 write of the blob (DEC-014 Option B); the adapter
never writes the token file itself.

DoD bar applied here (RIGOR: FULL):
  - acceptance encoded (round-trip):     test_dump_then_load_round_trips_the_session_blob
  - REQ-005 (password never persisted):  test_dumped_blob_never_contains_the_password
  - negative path (must-have):           test_load_tampered_or_expired_blob_raises_session_expired
    (DEC-014 OQ2 → REQ-NF-Compat-001(b): a server-side-invalidated / tampered
     session surfaces as GarminError(kind='session_expired'), a kind DISTINCT
     from 'token_permissions' and from login's 'auth'.)

The exact underlying library export/import method name is DEC-014 OQ1
(unconfirmed against the not-yet-bundled wheel); this suite pins the *adapter*
contract against a monkeypatched fake, so it is stable regardless of OQ1's
resolution — the adapter's NOTE(DEC-014 OQ1) marks the real call site.

Cites: REQ-004, REQ-005, REQ-NF-Compat-001(b); DEC-014 (Option B — adapter
exports blob, C++ persists), DEC-002 (worker is sole caller), DEC-008 (pytest),
DES-012 (adapter as stable seam).
"""

from __future__ import annotations

import types
from typing import Any

import pytest

# RED-for-the-right-reason: the import succeeds (garmin_client exists); the
# failure is the missing dump_tokens/load_tokens methods, not an ImportError.
from garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeGarmin:
    """Fake of garminconnect.Garmin exposing only the surface the adapter's
    dump/load path touches: an in-memory OAuth session it can serialize to an
    opaque string and restore from one. A ``password`` handed to the ctor is
    NOT part of the serialized session (REQ-005).

    Each instance is seeded with a DISTINCT session (a monotonic per-instance
    counter, independent of the ctor args, which are identical for the src and
    dst clients in the round-trip test). This is what makes the round-trip test
    able to KILL a no-op ``load_tokens()``: a fresh dst instance starts life
    holding a session that differs from the src's blob, so if ``loads()`` were
    never actually invoked (e.g. body replaced by ``pass``) the dst would still
    dump its OWN seeded session and the round-trip assertion would FAIL. With a
    real ``load_tokens()`` the dst's session is overwritten with the src blob and
    the two dumps match. (A hardcoded-identical seed made "restored" and
    "never-touched" indistinguishable — the A3-R004 M2 mutation survivor.)
    """

    _instance_counter = 0

    def __init__(self, email: str, password: str) -> None:
        # DEC-014 Option B: AUTH-ONLY construction — no tokenstore path.
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

    # DEC-014 OQ1 — the real library's in-memory export/import. The fake mirrors
    # the contract: dumps() -> opaque str; loads(str) restores the same session.
    def dumps(self) -> str:
        import json

        return json.dumps(self._session)

    def loads(self, token_str: str) -> None:
        import json

        try:
            data = json.loads(token_str)
        except (ValueError, TypeError) as e:
            # A tampered blob no longer parses as a valid session — the real
            # library surfaces an invalidated/expired session as an auth error.
            raise _FakeAuthError("session could not be restored") from e
        if not isinstance(data, dict) or "oauth1_token" not in data:
            raise _FakeAuthError("session is expired or invalid")
        self._session = data


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    import garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthError,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_dump_then_load_round_trips_the_session_blob(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-004 acceptance (Python half) — dump_tokens()/load_tokens() form a
    lossless pair: a blob exported from one authenticated session restores the
    identical session on a fresh adapter instance.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    src = GarminClient("a@example.com", "pw")
    blob = src.dump_tokens()
    assert isinstance(blob, str) and blob, "dump_tokens() must return a non-empty opaque string blob"

    # A brand-new adapter (as REQ-006's resume path will build). It is seeded
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
        "exported — the round-trip is REQ-004's Python contract"
    )


def test_dumped_blob_never_contains_the_password(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-005 — only OAuth bearer + refresh tokens are stored; the Garmin
    password must never appear in the serialized blob C++ writes to disk.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    secret = "s3cret-never-persisted"
    client = GarminClient("a@example.com", secret)
    blob = client.dump_tokens()

    assert secret not in blob, (
        "REQ-005: the password must never be serialized into the token blob " "that C++ persists to tokens.json"
    )


def test_load_tampered_or_expired_blob_raises_session_expired(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-NF-Compat-001(b) / DEC-014 OQ2 — restoring a tampered or
    server-side-invalidated session raises GarminError(kind='session_expired'),
    a kind DISTINCT from 'token_permissions' and from login's 'auth', so the
    resume path (REQ-006) can route it to a re-login prompt.
    """
    _install_fake_gc(monkeypatch, _FakeGarmin)

    client = GarminClient("a@example.com", "pw")
    with pytest.raises(GarminError) as excinfo:
        client.load_tokens("}{ not valid json — tampered blob")

    assert excinfo.value.kind == "session_expired", (
        "A tampered/expired session must map to GC-stable kind='session_expired' "
        "(distinct from 'token_permissions'/'auth') per DEC-014 OQ2"
    )
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"
