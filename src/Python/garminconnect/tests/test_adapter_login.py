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
from gc_garmin_adapter.garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeConnError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeRateError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectTooManyRequestsError."""


class _FakeGarminBase:
    """Minimal fake of garminconnect.Garmin. Tests subclass to inject behaviour.

    B-STAGE9-09: deliberately has NO `full_name_id` — the real, installed
    python-garminconnect library never exposed one (that drift is what put a
    bare `builtins.AttributeError` into production against a live account).
    Only `display_name` is real.
    """

    display_name: str = ""

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # DEC-014 Option B: AUTH-ONLY construction — no tokenstore path. The
        # library holds an in-memory session and self-writes no token file.
        # **_ swallows B-STAGE9-10's return_on_mfa=True.
        self.email = email
        self.password = password

    def login(self) -> None:  # library returns None on success; adapter reads .display_name
        raise NotImplementedError  # overridden per test

    def _load_profile_and_settings(self) -> None:
        # B-STAGE9-10: the real wrapper's return_on_mfa=True skips this call on
        # a plain success, so garmin_client.py calls it explicitly; a no-op
        # here preserves whatever display_name a test already set (mirrors the
        # real library's OWN _load_profile_and_settings, which populates
        # display_name — see test_adapter_real_library_contract.py's
        # _EarlyReturnGarmin for a fake that models the populating case).
        pass


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake module exposing the same surface
    the adapter touches: Garmin class + exceptions.GarminConnectAuthenticationError.
    """
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_exceptions = types.SimpleNamespace(
        GarminConnectAuthenticationError=_FakeAuthError,
        # REQ-014 — login() also classifies connection/rate_limit failures,
        # mirroring download_activity()/list_activities_since()'s existing pattern.
        GarminConnectConnectionError=_FakeConnError,
        GarminConnectTooManyRequestsError=_FakeRateError,
    )
    fake_mod.exceptions = fake_exceptions  # type: ignore[attr-defined]
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_login_happy_path_constructs_auth_only_and_exposes_blob(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """T-015 — REQ-006 / DEC-014 Option B / A3-R004-M3 security-close.

    The adapter must construct the underlying library AUTH-ONLY: EXACTLY
    (email, password), with NO third/tokenstore argument. The library therefore
    self-writes NO token file — persistence is instead exposed to C++ via
    dump_tokens() (the opaque blob C++/GarminTokenStore writes 0600). This
    supersedes the old `assert tokenstore.exists()` acceptance: nothing below
    the adapter persists any more (T-012/T-014 own and perm-check the C++ write).
    """
    ctor_args: dict[str, tuple[Any, ...]] = {}

    class _OkGarmin(_FakeGarminBase):
        display_name = "Test Athlete"

        # Capture EVERY positional the adapter forwards, so a lingering
        # tokenstore path (a 3rd arg) is caught, not silently swallowed.
        # return_on_mfa is forwarded as a KEYWORD (B-STAGE9-10), so it never
        # lands in `args` — the positional-only assertions below still hold.
        def __init__(self, *args: Any, **_: Any) -> None:
            ctor_args["forwarded"] = args
            super().__init__(*args[:2])
            # B-STAGE9-11: dumps() lives on the inner .client object on the
            # real library, not on Garmin itself.
            self.client = types.SimpleNamespace(dumps=lambda: '{"oauth1":"blob","oauth2":"blob"}')

        def login(self) -> None:
            # Auth-only: the library holds an in-memory session; it does NOT
            # write any token file (no tokenstore path was ever handed to it).
            pass

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("good@example.com", "goodpass")
    result = client.login()

    assert result == {"garmin_user_id": "Test Athlete", "display_name": "Test Athlete"}, (
        "login() must return the per-DES-012 identity dict so the worker can "
        "resolve per-account sidecar paths (DES-002)"
    )
    # (a) auth-only construction — exactly (email, password), NO tokenstore path.
    assert ctor_args["forwarded"] == ("good@example.com", "goodpass"), (
        "DEC-014 Option B: the adapter must construct the library AUTH-ONLY, "
        "forwarding exactly (email, password) and NO tokenstore path"
    )
    assert len(ctor_args["forwarded"]) == 2, (
        "a third (tokenstore) argument would let the library self-write an "
        "unaudited token file — A3-R004-M3 forbids this"
    )
    # (b) persistence is available via the exported blob, not a library-side
    # file write. C++ (T-012/T-014) owns the single atomic 0600 write.
    blob = client.dump_tokens()
    assert isinstance(blob, str) and blob, (
        "REQ-006: a successful login exposes its session via dump_tokens() for "
        "C++ to persist 0600 — the library itself writes no token file"
    )


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

    client = GarminClient("bad@example.com", "wrong")
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


def test_login_connection_error_maps_to_kind_connection(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-014 — a library connection error during login() must surface as
    GC-stable kind='connection' (mirrors download_activity()'s existing
    translation), so DES-008 routes the "couldn't reach Garmin" copy."""

    class _ConnGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise _FakeConnError("stub: connection refused")

    _install_fake_gc(monkeypatch, _ConnGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "connection"
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"


def test_login_rate_limit_error_maps_to_kind_rate_limit(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-014 — Garmin 429 during login() must surface as kind='rate_limit'
    (mirrors download_activity()'s existing translation), never as a generic
    or auth error."""

    class _RateGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise _FakeRateError("stub: 429 too many requests")

    _install_fake_gc(monkeypatch, _RateGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "rate_limit"
    assert excinfo.value.original is not None


def test_non_auth_exception_is_not_misclassified_as_auth(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """A3/REQ-002 — mutant M6 kill.

    A library exception that is NOT an authentication failure must NOT come
    out of the adapter as `GarminError(kind='auth')`. DES-008's switch table
    routes user-facing copy off `kind`; a `RuntimeError` (or any non-auth
    exception) tagged 'auth' would route the wizard to "wrong password" copy
    for what is actually a connection/library error.

    REQ-002/REQ-014's GREEN slices only spec the auth/connection/rate_limit
    branches — the wider `_EXCEPTION_MAP` (captcha, mfa_required,
    token_permissions, ...) lands with later slices (A2-005, REQ-003,
    REQ-015). Until then, non-auth/foreign exceptions propagate unchanged;
    what they must not do is silently inherit 'auth'.
    """

    class _BoomGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise RuntimeError("simulated downstream library bug")

    _install_fake_gc(monkeypatch, _BoomGarmin)

    client = GarminClient("u@x.com", "p")
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

    class _SilentBadGarmin(_FakeGarminBase):
        def login(self) -> None:
            raise _FakeAuthError()  # no message

    _install_fake_gc(monkeypatch, _SilentBadGarmin)

    client = GarminClient("u@x.com", "p")
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

        def login(self) -> None:
            tokenstore.write_text("{}")

    _install_fake_gc(monkeypatch, _OkGarmin)

    secret = "s3cret-not-retained"
    client = GarminClient("u@x.com", secret)
    client.login()

    for attr_name, value in vars(client).items():
        assert value != secret, (
            f"Adapter attribute {attr_name!r} retains the password after login(); "
            "REQ-005 forbids any post-login retention on the adapter instance"
        )


def test_login_identity_shape_mismatch_raises_classified_GarminError(
    tmp_path: Any, monkeypatch: pytest.MonkeyPatch
) -> None:
    """B-STAGE9-09 — live-account regression guard.

    A B-STAGE9-08 live diagnostic caught the real reason a live login showed
    the UI's generic "code: unknown" copy: `self._garmin.full_name_id` doesn't
    exist on the real, installed library, so a bare `builtins.AttributeError`
    escaped this adapter's classification boundary entirely and reached C++
    unclassified. Any future library-shape mismatch in the post-login identity
    read (a rename, a version bump) must instead raise a CLASSIFIED
    GarminError(kind='unknown') with a diagnosable message — never a raw
    exception.
    """

    class _ShapeMismatchGarmin(_FakeGarminBase):
        def login(self) -> None:
            pass

        @property
        def display_name(self) -> str:  # type: ignore[override]
            raise AttributeError("simulated: real library dropped this attribute")

    _install_fake_gc(monkeypatch, _ShapeMismatchGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "unknown", (
        "a post-login identity shape mismatch is a programming/library-compat "
        "error, not an authentication failure — must classify as kind='unknown', "
        "never escape as a raw exception"
    )
    assert "AttributeError" in excinfo.value.message, "message must be diagnosable"
    assert isinstance(excinfo.value.original, AttributeError)
