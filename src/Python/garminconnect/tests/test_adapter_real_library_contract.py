"""B-STAGE9-09/10/11 — DEC-014 OQ1 close: fake-fidelity contract test.

The dev `.venv` this suite normally runs under does NOT have the real
`python-garminconnect` wheel installed (DES-007/Pkg bundles it only for
production); every other adapter test in this directory therefore runs
against a hand-written FAKE `_gc` module. A B-STAGE9-08 live-account
diagnostic caught the fake drifting from the real library: the fakes DEFINED
a `full_name_id` attribute the real, installed library never had, so
`garmin_client.py` read it, got a bare `builtins.AttributeError` on a live
account, and A3-REQ-002's mutation testing (M8/M9) "killed" the very mutants
that would have exposed the drift — because the fakes agreed with the bug.

This file closes that hole two ways:

  1. `test_adapter_contract_matches_real_garminconnect_library` imports the
     REAL `garminconnect` package (when present — see
     `_import_real_garminconnect` below) and pins both attribute EXISTENCE
     and, where an existence check is insufficient (B-STAGE9-10's
     argument-ORDER defect passes every hasattr check), `inspect.signature`
     parameter NAMES AND ORDER against the real wheel. Skips cleanly (not
     fails) wherever the real wheel is absent, so CI without it stays green;
     runs for real wherever the wheel IS installed (this project's system
     Python3 — the same interpreter `testGarminCurlCffiInterposition` already
     uses for its real-library regression, per its CMakeLists.txt comment).

  2. A second group of tests below (`_RealShape*` fakes) needs NO real wheel
     at all — they run unconditionally, using fakes deliberately built to
     mirror the REAL library's confirmed behavior (not the adapter's current,
     buggy assumptions about that behavior — see each fake's docstring for
     the exact real-library source line it mirrors). These are the RED tests
     for B-STAGE9-10 (REQ-003 two-step MFA is dead against the real library —
     three compounding defects) and B-STAGE9-11 (dump_tokens/load_tokens read
     the wrong object). They fail today against the current, held
     garmin_client.py and are expected to flip GREEN once B-STAGE9-10/11 land.

NOTE — self-shadow hazard: this adapter's OWN directory is named
`garminconnect` (pyproject.toml's mypy override already documents this exact
name collision) and carries an `__init__.py`. Under pytest, that makes
pytest's rootdir-climbing insert THIS DIRECTORY'S PARENT onto `sys.path`, so
a plain `import garminconnect` resolves right back to this adapter package
(empty `__init__.py`) instead of the real PyPI library — a self-shadow, not
an absence. `_import_real_garminconnect` below detects and strips exactly
that shadowing path entry before importing, so the test exercises the real
site-packages library whenever one is present. This shadow is a pytest
collection-time artifact only (confirmed: `garmin_client.py`'s own module
load in this same environment, and the live embedded-CPython repro that
originally found the `full_name_id` bug, both reached the real library, since
production's sys.path does not include this directory's parent).

Cites: DEC-014 OQ1, LSN-006 (classify by type, not message), DES-012, REQ-003.
"""

from __future__ import annotations

import importlib
import inspect
import os
import sys
import types
from types import ModuleType
from typing import Any

import pytest

_ADAPTER_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def _import_real_garminconnect() -> ModuleType:
    """Import the real, installed `garminconnect` package, bypassing this
    adapter directory's self-shadow (see module docstring). Raises
    ModuleNotFoundError, uncaught, when no real package is installed at all —
    callers translate that into a clean pytest skip."""
    sys.modules.pop("garminconnect", None)
    saved_path = list(sys.path)
    try:
        sys.path[:] = [
            p
            for p in sys.path
            if not (
                os.path.isdir(p)
                and os.path.normpath(os.path.join(p, "garminconnect")) == os.path.normpath(_ADAPTER_DIR)
            )
        ]
        return importlib.import_module("garminconnect")
    finally:
        sys.path[:] = saved_path


# Every attribute name garmin_client.py's _login_impl/_submit_mfa_impl reads
# directly off `self._garmin` while resolving the post-auth identity dict.
# Keep this in sync with garmin_client.py — that sync drifting silently is
# exactly the B-STAGE9-08 bug class this test exists to catch.
_GARMIN_IDENTITY_ATTRS = (
    "login",
    "resume_login",
    "display_name",
)

_GARMIN_EXCEPTIONS = (
    "GarminConnectAuthenticationError",
    "GarminConnectConnectionError",
    "GarminConnectTooManyRequestsError",
)


def test_adapter_contract_matches_real_garminconnect_library() -> None:
    """DEC-014 OQ1 / B-STAGE9-08/10/11 regression guard.

    Construct the REAL `garminconnect.Garmin` (the constructor only stores
    credentials — no network call) and assert:

      (B-STAGE9-08/09) every attribute name the adapter's login/MFA identity
      resolution reads actually exists on it. A missing attribute here means
      garmin_client.py (or a fake in tests/test_adapter_*.py) has drifted
      from the real library's surface — the exact class of bug that put
      `full_name_id` (never a real attribute) into production and slipped
      past 49/49 fake-backed adapter tests.

      (B-STAGE9-10) `Garmin.__init__` accepts `return_on_mfa` (needed to ever
      reach the needs-MFA branch at all — see
      `test_mfa_required_account_must_surface_two_step_sentinel...` below),
      AND `Garmin.resume_login`'s parameters are, IN ORDER,
      `(client_state, mfa_code)`. An existence/hasattr check alone is
      INSUFFICIENT here — an argument-order defect (passing the user's OTP as
      `client_state` instead of `mfa_code`) passes every hasattr check, so
      this pins the parameter NAMES AND POSITIONS, not just their presence.

      (B-STAGE9-11) `dump_tokens()`/`load_tokens()` must read through the
      inner `.client` object — `Garmin` itself has no `dumps()`/`loads()` on
      the real library; `.client.dumps()`/`.client.loads()` do exist.

      (B-STAGE9-10) `_load_profile_and_settings` exists — a PRIVATE method
      (leading underscore, no public alternative in 0.3.15) the compensating
      identity-load design depends on, since `return_on_mfa=True` makes the
      real wrapper's `login()` skip profile loading unconditionally (see
      `test_login_after_return_on_mfa_must_still_populate_identity_on_success`
      below). Pinning a private API is a deliberate, documented risk — it may
      break silently on a future library upgrade with nothing else to fall
      back to; this test is exactly what would catch that.
    """
    try:
        real_gc = _import_real_garminconnect()
    except ModuleNotFoundError:
        pytest.skip(
            "python-garminconnect not installed in this environment (the dev "
            ".venv intentionally lacks it; production bundles it under "
            "DEC-011/REQ-NF-Pkg-001) — this contract only runs where the "
            "real wheel is present."
        )

    real = real_gc.Garmin("probe@example.invalid", "probe-password")

    missing = [name for name in _GARMIN_IDENTITY_ATTRS if not hasattr(real, name)]
    assert not missing, (
        f"garmin_client.py reads {missing!r} off garminconnect.Garmin, but the "
        "REAL installed library exposes no such attribute(s) — update "
        "garmin_client.py AND the fakes in tests/test_adapter_*.py to match "
        "the real surface"
    )

    missing_exc = [name for name in _GARMIN_EXCEPTIONS if not hasattr(real_gc.exceptions, name)]
    assert not missing_exc, (
        f"garminconnect.exceptions is missing {missing_exc!r} — garmin_client.py's "
        "exception-type classification (LSN-006) would fail to import these names"
    )

    # B-STAGE9-10 (1): the needs-MFA branch is provably unreachable without
    # return_on_mfa — pin that the constructor still accepts it.
    ctor_params = inspect.signature(real_gc.Garmin.__init__).parameters
    assert "return_on_mfa" in ctor_params, (
        "garminconnect.Garmin.__init__ no longer accepts return_on_mfa — the "
        "B-STAGE9-10 design (constructing with return_on_mfa=True so the "
        "needs-MFA sentinel can ever be produced) depends on this parameter"
    )

    # B-STAGE9-10 (2): argument ORDER, not just existence — a reversed-argument
    # defect (today's bug: garmin_client.py calls resume_login(code, pending))
    # passes any hasattr check, so pin the real parameter NAMES AND POSITIONS.
    resume_params = list(inspect.signature(real_gc.Garmin.resume_login).parameters)
    resume_params = [p for p in resume_params if p != "self"]
    assert resume_params == ["client_state", "mfa_code"], (
        f"garminconnect.Garmin.resume_login's parameters are {resume_params!r}, "
        "expected exactly ['client_state', 'mfa_code'] in that order — the "
        "adapter's fixed call must pass the user's OTP as the SECOND "
        "positional argument (mfa_code), never the first"
    )

    # Repair NON-BLOCKING: garmin_client.py passes a placeholder (None) as
    # resume_login's first positional arg rather than forwarding login()'s
    # retained client_state, because the INNER Client.resume_login's first
    # parameter is named with a leading underscore (`_client_state`) — the
    # library's own convention for "never read". Pin that naming here so a
    # future wheel that starts reading this argument (while keeping the same
    # outer signature checked above) fails this assertion loudly instead of
    # silently breaking MFA in production.
    inner_resume_params = list(inspect.signature(real.client.resume_login).parameters)
    assert inner_resume_params[0] == "_client_state", (
        f"garminconnect.Garmin.client.resume_login's first parameter is "
        f"{inner_resume_params[0]!r}, not '_client_state' — the leading "
        "underscore was this library's own signal that it is never read; if "
        "a new wheel renamed it (dropping the underscore) that may mean it is "
        "now READ, and garmin_client.py's placeholder-None call must be "
        "revisited to forward the real client_state instead"
    )

    # B-STAGE9-11: dump_tokens()/load_tokens() must go through .client.
    assert not hasattr(real, "dumps"), (
        "garminconnect.Garmin unexpectedly grew its own dumps() — if this "
        "assertion fails, the B-STAGE9-11 fix routing through .client.dumps() "
        "may need revisiting (though routing through .client would likely "
        "still work if .client.dumps() also still exists)"
    )
    assert hasattr(real, "client") and hasattr(real.client, "dumps") and hasattr(real.client, "loads"), (
        "garminconnect.Garmin.client (the inner session object) must expose "
        "dumps()/loads() — B-STAGE9-11's fix reads/writes tokens through here, "
        "since the outer Garmin object has neither"
    )

    # B-STAGE9-10 (3): the private compensating-load method the return_on_mfa
    # early-return trap requires must still exist.
    assert hasattr(real, "_load_profile_and_settings"), (
        "garminconnect.Garmin._load_profile_and_settings (private, no public "
        "alternative in 0.3.15) no longer exists — the B-STAGE9-10 design's "
        "compensation for return_on_mfa's unconditional profile-load skip on "
        "a plain success depends on this exact method"
    )


def test_adapter_listing_contract_matches_real_garminconnect_library() -> None:
    """B-STAGE9-25 close — this file's OWN stated purpose (module docstring:
    "pin ... both attribute EXISTENCE and ... signature ... against the real
    wheel") never covered get_activities_by_date's argument SHAPE, only
    login/MFA/token surfaces (_GARMIN_IDENTITY_ATTRS above). That gap is why
    this file did not catch B-STAGE9-25: a hasattr/signature check would not
    have caught it either, since get_activities_by_date's signature never
    changed — the defect is in the VALUE (datetime vs date-only) accepted by
    the library's first-statement validator, not the call shape. Pins that
    directly: the real `_validate_date_format` (__init__.py:63-74) rejects a
    full datetime and accepts date-only, confirmed interactively against the
    installed 0.3.15 wheel."""
    try:
        real_gc = _import_real_garminconnect()
    except ModuleNotFoundError:
        pytest.skip(
            "python-garminconnect not installed in this environment (the dev "
            ".venv intentionally lacks it; production bundles it under "
            "DEC-011/REQ-NF-Pkg-001) — this contract only runs where the "
            "real wheel is present."
        )

    assert hasattr(real_gc.Garmin, "get_activities_by_date"), (
        "garminconnect.Garmin.get_activities_by_date no longer exists — "
        "garmin_client.py's _list_activities_since_impl (DES-012) depends on it"
    )
    listing_params = [p for p in inspect.signature(real_gc.Garmin.get_activities_by_date).parameters if p != "self"]
    startdate_param = listing_params[0]
    assert startdate_param == "startdate", (
        f"get_activities_by_date's first parameter is {startdate_param!r}, not "
        "'startdate' — B-STAGE9-25's fix forwards the query positionally, "
        "assuming this is the startdate slot"
    )

    with pytest.raises(ValueError, match="startdate"):
        real_gc._validate_date_format("2026-09-09 05:21:24", "startdate")
    assert real_gc._validate_date_format("2026-09-09", "startdate") == "2026-09-09", (
        "the real library's own validator must accept the date-only shape "
        "B-STAGE9-25's fix now sends — if this fails, the truncation in "
        "garmin_client.py's _list_activities_since_impl no longer satisfies "
        "the real wheel's contract"
    )


def test_adapter_listing_response_shape_passthrough_matches_real_garminconnect_library() -> None:
    """B-STAGE9-25 round 3 — regression evidence for the RESPONSE side.

    The installed wheel's get_activities_by_date validates only its
    startdate/enddate INPUTS, then pages connectapi() results with
    ``activities.extend(act)`` and returns them — no per-record transformation
    exists between the server JSON and what this adapter receives, so the
    ``startTimeGMT`` separator shape (space or 'T') is the SERVER's choice,
    pinned by nothing on our side. This pins the passthrough itself by serving
    a canned connectapi page (both separator shapes, int activityId, extra
    keys) to the real method and asserting the records come back verbatim.

    This is regression evidence, NOT the safety net: an authenticated server
    response cannot be made safe by a test. The runtime instant
    validation/comparison in _list_activities_since_impl is the actual guard
    (findings.md B-STAGE9-25 round 3)."""
    try:
        real_gc = _import_real_garminconnect()
    except ModuleNotFoundError:
        pytest.skip(
            "python-garminconnect not installed in this environment (the dev "
            ".venv intentionally lacks it; production bundles it under "
            "DEC-011/REQ-NF-Pkg-001) — this contract only runs where the "
            "real wheel is present."
        )

    real = real_gc.Garmin("probe@example.invalid", "probe-password")
    page = [
        {"activityId": 1001, "startTimeGMT": "2026-09-08 21:14:00", "activityName": "space-form"},
        {"activityId": 1002, "startTimeGMT": "2026-09-08T21:14:00.0", "activityName": "iso-form"},
    ]
    remaining: list[list[dict[str, Any]]] = [page, []]  # one page, then empty → pagination breaks

    def fake_connectapi(path: str, **kwargs: Any) -> Any:
        assert kwargs.get("params", {}).get("startDate") == "2026-09-08", (
            f"get_activities_by_date must forward the validated startdate as the "
            f"startDate query param, got {kwargs.get('params')!r}"
        )
        return remaining.pop(0)

    real.connectapi = fake_connectapi

    result = real.get_activities_by_date("2026-09-08")

    assert result == page, (
        "the wheel must return the server's activity records verbatim — a "
        "transformation step (key renaming, timestamp normalization) between "
        "connectapi and the return would change what garmin_client.py's "
        "startTimeGMT validation sees"
    )
    assert all(returned is original for returned, original in zip(result, page)), (
        "the returned records must be the same objects connectapi produced, "
        "not rebuilt copies — the passthrough is what makes the response shape "
        "the server's contract rather than the wheel's"
    )


# =============================================================================
# B-STAGE9-10 / B-STAGE9-11 — behavioral RED tests against real-shape fakes.
#
# These need NO real wheel (they run unconditionally, in the dev .venv too) —
# unlike the contract test above, they don't check the library's shape, they
# exercise garmin_client.py's ACTUAL CODE against fakes deliberately built to
# mirror the real library's CONFIRMED behavior (each fake's docstring cites
# the exact real-library source line it mirrors; verified interactively
# against the installed python-garminconnect 0.3.15 before writing these).
#
# All of these currently FAIL against the held garmin_client.py — that is the
# point: they encode the DESIRED (fixed) behavior, so a raw, unhandled
# GarminError (or a wrong-value assertion) below is the expected RED. They
# are expected to flip GREEN once B-STAGE9-10/11 land.
# =============================================================================


class _FakeAuthErrorReal(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthErrorReal,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


class _RealShapeMfaRequiredGarmin:
    """Mirrors garminconnect/client.py's resolve_mfa() (client.py:532-547):
    the needs-MFA sentinel is ONLY produced when return_on_mfa=True was
    passed to the constructor. Otherwise — exactly today's adapter
    construction, `_gc.Garmin(email, password)` with no return_on_mfa/
    prompt_mfa — an MFA-required account raises
    GarminConnectAuthenticationError("MFA Required but no prompt_mfa
    mechanism supplied") (client.py:547), so the ("needs_mfa", ...) tuple
    garmin_client.py:133 branches on can NEVER be produced today.
    """

    display_name: str = ""

    def __init__(self, email: str, password: str, return_on_mfa: bool = False, **_: Any) -> None:
        self.email = email
        self.password = password
        self.return_on_mfa = return_on_mfa

    def login(self) -> tuple[str | None, str | None]:
        if self.return_on_mfa:
            return ("needs_mfa", None)
        raise _FakeAuthErrorReal("MFA Required but no prompt_mfa mechanism supplied")


def test_mfa_required_account_must_surface_two_step_sentinel_not_generic_auth_failure(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-10 defect (1) — RED today.

    An MFA-required account must surface REQ-003's two-step
    `{"mfa_required": True}` sentinel. Today's adapter constructs the real
    library with no `return_on_mfa` (and no `prompt_mfa`), so — per the real
    library's own MFA gate, mirrored exactly by `_RealShapeMfaRequiredGarmin`
    above — the real call raises `GarminConnectAuthenticationError` instead,
    which the adapter's existing exception-type classification (correctly,
    given today's construction) maps to `GarminError(kind='auth')`: an
    MFA-required account is misreported as a bad password, and REQ-003's
    two-step flow is currently UNREACHABLE against the real library.
    """
    from gc_garmin_adapter.garmin_client import GarminClient

    _install_fake_gc(monkeypatch, _RealShapeMfaRequiredGarmin)
    client = GarminClient("u@x.com", "p")

    result = client.login()

    assert result == {"mfa_required": True}, (
        "an MFA-required account must surface the two-step sentinel, not "
        "raise — today's adapter passes no return_on_mfa to the real "
        "library's constructor, so the real MFA gate (client.py:547) raises "
        "'MFA Required but no prompt_mfa mechanism supplied' instead"
    )


class _EarlyReturnGarmin:
    """Mirrors garminconnect/__init__.py's login() wrapper (lines ~738-743):
    with return_on_mfa=True, login() returns EARLY and UNCONDITIONALLY —
    even on an immediate, non-MFA success — skipping
    `_load_profile_and_settings()` entirely. `display_name` therefore stays
    unpopulated unless something calls the loader explicitly afterward.
    """

    display_name: str = ""

    def __init__(self, email: str, password: str, **_: Any) -> None:
        self.email = email
        self.password = password

    def login(self) -> tuple[None, None]:
        return (None, None)  # real shape: ALWAYS a 2-tuple, never bare None

    def _load_profile_and_settings(self) -> None:
        self.display_name = "Compensated Athlete"


def test_login_after_return_on_mfa_must_still_populate_identity_on_success(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-10's non-obvious trap — RED today.

    Whatever fix threads `return_on_mfa=True` through construction (required
    by the sibling test above, to ever reach the needs-MFA branch at all)
    must NOT regress the plain, non-MFA happy path B-STAGE9-09 just fixed and
    confirmed live: the real wrapper's `login()` returns early UNCONDITIONALLY
    whenever `return_on_mfa=True`, even on immediate success, skipping
    `_load_profile_and_settings()` — so `display_name` stays empty unless the
    adapter calls it explicitly afterward.

    This test doesn't need `return_on_mfa` plumbed into GarminClient at all to
    prove the gap: `_login_impl`'s CURRENT body has no call to
    `_load_profile_and_settings()` anywhere on the non-MFA return path, so
    against a fake whose `display_name` is only populated BY that call, the
    identity dict comes back empty today — exactly the silent breakage the
    eventual fix must not reintroduce.
    """
    from gc_garmin_adapter.garmin_client import GarminClient

    _install_fake_gc(monkeypatch, _EarlyReturnGarmin)
    client = GarminClient("u@x.com", "p")

    result = client.login()

    assert result.get("display_name"), (
        "login() returned an empty/unset display_name — this is the "
        "return_on_mfa early-return trap: the fix MUST call "
        "_load_profile_and_settings() itself on a plain (non-MFA) success, "
        "since the real library's login() skips it unconditionally whenever "
        "return_on_mfa=True"
    )


class _CompensationLoadFailsGarmin:
    """Mirrors the real garminconnect.Garmin._load_profile_and_settings source
    (read directly off the installed 0.3.15 wheel): "Raises
    GarminConnectAuthenticationError if either [social profile or user
    settings] cannot be retrieved (e.g. the token is rejected)." Valid
    credentials, a successful non-MFA login() — and then the COMPENSATING
    call B-STAGE9-10 added (required because return_on_mfa=True makes the
    real login() skip this call itself) fails.
    """

    display_name: str = ""

    def __init__(self, email: str, password: str, **_: Any) -> None:
        self.email = email
        self.password = password

    def login(self) -> tuple[None, None]:
        return (None, None)  # real shape: ALWAYS a 2-tuple, never bare None

    def _load_profile_and_settings(self) -> None:
        raise _FakeAuthErrorReal("Failed to retrieve user settings")


def test_login_when_compensation_profile_load_fails_must_raise_classified_error(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-10 repair BLOCKING-1 — RED today.

    garmin_client.py's compensating `self._garmin._load_profile_and_settings()`
    call (added to close the return_on_mfa early-return trap above) sits
    AFTER `_login_impl`'s try/except block that classifies login() itself —
    so when the REAL library's own `_load_profile_and_settings()` raises
    GarminConnectAuthenticationError (confirmed against the installed 0.3.15
    wheel: valid credentials, but the social-profile/settings fetch fails),
    it escapes this adapter raw instead of becoming a classified GarminError.
    This is the exact B-STAGE9-09 failure pattern (an unclassified exception
    reaching PyEmbeddedAdapter, folded to the generic "code: unknown" UI
    copy) reintroduced on a new line by this very fix.
    """
    from gc_garmin_adapter.garmin_client import GarminClient, GarminError

    _install_fake_gc(monkeypatch, _CompensationLoadFailsGarmin)
    client = GarminClient("u@x.com", "p")

    with pytest.raises(GarminError) as excinfo:
        client.login()

    assert excinfo.value.kind == "auth", (
        "a failed compensating profile/settings load after valid credentials "
        "must classify as kind='auth' (GarminConnectAuthenticationError, same "
        "as any other auth-stage failure) — never escape as a raw, "
        "unclassified exception"
    )
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"


class _RealShapeResumeLoginGarmin:
    """Isolates ONLY B-STAGE9-10 defect (2) — the resume_login argument-order/
    semantics defect — from defect (1) tested above: login() unconditionally
    offers the needs-MFA sentinel (matching the EXISTING test_adapter_mfa.py
    fakes) so this test can exercise submit_mfa() on its own.

    resume_login()'s REAL signature is `(client_state, mfa_code)`, and the
    real `client.py:1620` implementation NEVER reads its first parameter at
    all (`def resume_login(self, _client_state: Any, mfa_code: str)` — the
    leading underscore is the library's own signal that it's unused) — only
    `mfa_code` is checked. Mirrored exactly here.
    """

    display_name: str = ""
    MFA_STATE = {"client": "state-opaque"}

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # **_ swallows B-STAGE9-10's return_on_mfa=True (this fake's login()
        # is unconditional, so it doesn't need to branch on the flag itself).
        self.email = email
        self.password = password
        self.resume_calls: list[tuple[Any, Any]] = []

    def login(self) -> tuple[str, Any]:
        return ("needs_mfa", self.MFA_STATE)

    def resume_login(self, client_state: Any, mfa_code: str) -> tuple[None, None]:
        self.resume_calls.append((client_state, mfa_code))
        if mfa_code != "123456":
            raise _FakeAuthErrorReal("invalid one-time code")
        self.display_name = "Resume Athlete"
        return (None, None)


def test_submit_mfa_correct_code_must_succeed_against_real_argument_order(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-10 defect (2) — RED today.

    A CORRECT one-time code must succeed. Today's adapter calls
    `self._garmin.resume_login(code, pending)` — reversed from the real
    library's `resume_login(client_state, mfa_code)` — so the user's correct
    OTP lands in the discarded `client_state` slot and the retained
    pending-state placeholder lands in `mfa_code` instead; against a fake
    that verifies by `mfa_code` (matching the real library, see
    `_RealShapeResumeLoginGarmin` above), a correct code is wrongly rejected
    as invalid today.
    """
    from gc_garmin_adapter.garmin_client import GarminClient

    _install_fake_gc(monkeypatch, _RealShapeResumeLoginGarmin)
    client = GarminClient("u@x.com", "p")
    assert client.login() == {"mfa_required": True}

    result = client.submit_mfa("123456")

    assert result == {"garmin_user_id": "Resume Athlete", "display_name": "Resume Athlete"}, (
        "the correct OTP must succeed — today's adapter's reversed "
        "resume_login(code, pending) call submits the retained pending-state "
        "placeholder as mfa_code instead of the user's actual code"
    )


class _RealShapeTokenGarmin:
    """Mirrors the real garminconnect.Garmin: NO dumps()/loads() of its own
    (B-STAGE9-11, confirmed against the real installed 0.3.15 library) —
    session export/import lives one level in, on the inner `.client` object.
    """

    display_name: str = "Token Athlete"

    class _InnerClient:
        def __init__(self) -> None:
            self.loaded: str | None = None

        def dumps(self) -> str:
            return '{"real":"blob"}'

        def loads(self, blob: str) -> None:
            self.loaded = blob

    def __init__(self, *_args: Any, **_kwargs: Any) -> None:
        # *args, not (email, password): from_tokens() constructs password-free
        # (Garmin()), exactly like the real library's own optional-args ctor.
        # **_kwargs swallows B-STAGE9-10's return_on_mfa=True when this fake
        # is instead constructed via the normal GarminClient(email, password)
        # path (test_dump_tokens_must_use_the_real_inner_client_surface below).
        self.client = self._InnerClient()

    def login(self) -> None:
        pass

    def _load_profile_and_settings(self) -> None:
        # B-STAGE9-10: garmin_client.py calls this explicitly on a plain
        # success (return_on_mfa=True skips the real wrapper's own call to
        # it); display_name is already set as a class attribute above, so
        # this fake's compensation is a no-op.
        pass


def test_dump_tokens_must_use_the_real_inner_client_surface(monkeypatch: pytest.MonkeyPatch) -> None:
    """B-STAGE9-11 — RED today.

    dump_tokens() must read the session blob from garminconnect.Garmin's REAL
    inner `.client.dumps()` — the outer Garmin object has no dumps() of its
    own on the real installed library, so today's `self._garmin.dumps()`
    raises AttributeError against a faithful fake.
    """
    from gc_garmin_adapter.garmin_client import GarminClient

    _install_fake_gc(monkeypatch, _RealShapeTokenGarmin)
    client = GarminClient("u@x.com", "p")
    client.login()

    blob = client.dump_tokens()

    assert blob == '{"real":"blob"}', (
        "dump_tokens() must read through garminconnect.Garmin's REAL inner "
        "`.client.dumps()` — the outer Garmin object has none of its own"
    )


def test_load_tokens_must_use_the_real_inner_client_surface(monkeypatch: pytest.MonkeyPatch) -> None:
    """B-STAGE9-11 — RED today.

    load_tokens()/from_tokens() must write the restored blob through
    garminconnect.Garmin's REAL inner `.client.loads()` — the outer Garmin
    object has no loads() of its own on the real installed library, so
    today's `self._garmin.loads(...)` raises AttributeError against a
    faithful fake.
    """
    from gc_garmin_adapter.garmin_client import GarminClient

    _install_fake_gc(monkeypatch, _RealShapeTokenGarmin)

    client = GarminClient.from_tokens('{"real":"blob"}')

    assert client._garmin.client.loaded == '{"real":"blob"}', (
        "load_tokens() must write through garminconnect.Garmin's REAL inner "
        "`.client.loads()` — the outer Garmin object has none of its own"
    )
