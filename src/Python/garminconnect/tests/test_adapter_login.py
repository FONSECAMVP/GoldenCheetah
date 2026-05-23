"""TEST-002 — REQ-002: SSO authentication via python-garminconnect.

PRD acceptance (REQ-002): "Valid email+password produces persisted OAuth
tokens; invalid credentials produce a labeled error."

Scope: the adapter (DES-012) login contract only. The C++ wizard wiring
(DES-003 GarminCredentialsPage) and worker thread-id assertion
(DES-001 GarminWorker — DoD must-have when REQ touches DES-001) are
covered by TEST-003 when the C++ wiring RED lands. Splitting the slice
here keeps the Python-only TDD cycle reversible without dragging Qt build
state into the loop.

DoD bar applied here:
  - acceptance encoded:                test_login_happy_path_returns_user_identity
  - negative path (must-have):         test_login_bad_credentials_raises_GarminError_kind_auth
  - REQ-005 password-handling shim:    test_password_not_retained_on_adapter_instance
    (the on-disk half of REQ-005 — "never written" — is the library's contract,
     enforced indirectly by REQ-002 because we never pass the password to
     anything other than the library's login() call.)

Cites: REQ-002, REQ-005; DEC-001 (staged ship), DEC-002 (worker is sole caller),
DEC-008 (pytest+coverage.py), DES-008 (error switch keyed on .kind),
DES-012 (adapter as stable seam).
"""

from __future__ import annotations

import types
from typing import Any

import pytest

# Intentional: imports the stub adapter. RED-for-the-right-reason is the
# NotImplementedError raised from GarminClient.__init__/login, not an import
# failure — the stub exists precisely so the import succeeds.
from garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeGarminBase:
    """Minimal fake of garminconnect.Garmin. Tests subclass to inject behaviour."""

    display_name: str = ""
    full_name_id: str = ""

    def __init__(self, email: str, password: str, tokenstore: str = "") -> None:
        self.email = email
        self.password = password
        self.tokenstore = tokenstore

    def login(self) -> None:  # library returns None on success; adapter reads .display_name
        raise NotImplementedError  # overridden per test


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake module exposing the same surface
    the adapter touches: Garmin class + exceptions.GarminConnectAuthenticationError.
    """
    import garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_exceptions = types.SimpleNamespace(
        GarminConnectAuthenticationError=_FakeAuthError,
    )
    fake_mod.exceptions = fake_exceptions  # type: ignore[attr-defined]
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_login_happy_path_returns_user_identity(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-002 acceptance — valid creds yield a persisted token file and an
    identity dict the worker can hand back to the UI.
    """
    tokenstore = tmp_path / "tokens.json"
    captured: dict[str, tuple[str, str, str]] = {}

    class _OkGarmin(_FakeGarminBase):
        display_name = "Test Athlete"
        full_name_id = "1234567"

        def login(self) -> None:
            captured["called_with"] = (self.email, self.password, self.tokenstore)
            # The real library writes the tokenstore JSON on successful SSO.
            tokenstore.write_text('{"oauth1":"fake","oauth2":"fake"}')

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("good@example.com", "goodpass", str(tokenstore))
    result = client.login()

    assert result == {"garmin_user_id": "1234567", "display_name": "Test Athlete"}, (
        "login() must return the per-DES-012 identity dict so the worker can "
        "resolve per-account sidecar paths (DES-002)"
    )
    assert captured["called_with"] == (
        "good@example.com",
        "goodpass",
        str(tokenstore),
    ), "Adapter must forward credentials and the tokenstore path verbatim to the library"
    assert tokenstore.exists(), "REQ-002 acceptance: a successful login must leave a persisted token file"


def test_login_bad_credentials_raises_GarminError_kind_auth(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-002 acceptance — negative path (DoD must-have bar).

    The library's GarminConnectAuthenticationError must be translated to a
    GC-stable GarminError(kind='auth') so DES-008 can switch on it without
    knowing the library's exception class names.
    """
    tokenstore = tmp_path / "tokens.json"

    class _BadGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise _FakeAuthError("Invalid email or password")

    _install_fake_gc(monkeypatch, _BadGarmin)

    client = GarminClient("bad@example.com", "wrong", str(tokenstore))
    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "auth", (
        "Library auth error must map to GC-stable kind='auth' so DES-008's "
        "switch table (keyed on .kind, not on library class names) routes "
        "to the user-facing translation"
    )
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"
    assert not tokenstore.exists(), "REQ-002 acceptance: failed auth must not leave a token file on disk"


def test_non_auth_exception_is_not_misclassified_as_auth(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """A3/REQ-002 — mutant M6 kill.

    A library exception that is NOT an authentication failure must NOT come
    out of the adapter as `GarminError(kind='auth')`. DES-008's switch table
    routes user-facing copy off `kind`; a `RuntimeError` (or any non-auth
    exception) tagged 'auth' would route the wizard to "wrong password" copy
    for what is actually a connection/library error.

    REQ-002's GREEN slice only specs the `auth` branch — the wider
    `_EXCEPTION_MAP` (rate_limit, connection, captcha, ...) lands with
    REQ-014. Until then, non-auth exceptions propagate unchanged; what they
    must not do is silently inherit 'auth'.
    """
    tokenstore = tmp_path / "tokens.json"

    class _BoomGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise RuntimeError("simulated downstream library bug")

    _install_fake_gc(monkeypatch, _BoomGarmin)

    client = GarminClient("u@x.com", "p", str(tokenstore))
    with pytest.raises(RuntimeError, match="simulated downstream library bug"):
        client.login()


def test_auth_error_with_empty_message_still_yields_displayable_message(
    tmp_path: Any, monkeypatch: pytest.MonkeyPatch
) -> None:
    """A3/REQ-002 — mutant M11 kill.

    Some library exceptions carry no message (`raise GarminConnectAuthenticationError()`).
    The adapter must still produce a non-empty `.message` so DES-008's UI
    translation has something to display — never an empty dialog body.
    """
    tokenstore = tmp_path / "tokens.json"

    class _SilentBadGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise _FakeAuthError()  # no message

    _install_fake_gc(monkeypatch, _SilentBadGarmin)

    client = GarminClient("u@x.com", "p", str(tokenstore))
    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "auth"
    assert excinfo.value.message, (
        "Empty library-side message must still yield a non-empty adapter message " "for UI display"
    )


def test_password_not_retained_on_adapter_instance(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-005 (must) — the password must not survive on the adapter beyond
    its handoff to the library's login() call.

    REQ-005's on-disk half ("never written to disk") is structurally enforced
    by REQ-002 — the adapter never writes the password anywhere; only the
    library writes tokens. This test enforces the in-memory half at the
    adapter seam so a future refactor can't accidentally stash the password
    on `self` for "convenience".
    """
    tokenstore = tmp_path / "tokens.json"

    class _OkGarmin(_FakeGarminBase):
        display_name = "X"
        full_name_id = "1"

        def login(self) -> None:
            tokenstore.write_text("{}")

    _install_fake_gc(monkeypatch, _OkGarmin)

    secret = "s3cret-not-retained"
    client = GarminClient("u@x.com", secret, str(tokenstore))
    client.login()

    for attr_name, value in vars(client).items():
        assert value != secret, (
            f"Adapter attribute {attr_name!r} retains the password after login(); "
            "REQ-005 forbids any post-login retention on the adapter instance"
        )
