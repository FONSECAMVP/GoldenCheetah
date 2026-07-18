"""REQ-003 (MFA) Slice A — the adapter half of the two-step MFA flow.

A Garmin login that needs a 6-digit OTP becomes a two-call flow on ONE adapter
instance:

  1. login()          → detects the library's MFA-required signal, RETAINS the
                         pending client_state on `self`, and returns the stable
                         sentinel {"mfa_required": True} (it does NOT raise and
                         does NOT return a success identity).
  2. submit_mfa(code) → resumes that retained session with the code; on success
                        returns the SAME identity dict shape login() returns on a
                        no-MFA success; on a bad/expired code raises
                        GarminError(kind='auth') (classified by exception TYPE —
                        LSN-006 — so the Slice-B page can re-prompt up to 3x);
                        with no pending session raises GarminError(kind='unknown').

The no-MFA path is unchanged: a login() whose library returns None (or anything
that is not the needs-MFA sentinel) still yields the {"garmin_user_id",
"display_name"} success dict — test_adapter_login.py guards that byte-for-byte.

The real python-garminconnect/garth wheel is absent, so — exactly like
test_adapter_login.py — a fake `_gc` (monkeypatched) defines the contract:
login() returns ("needs_mfa", client_state) and resume_login(code, client_state)
completes or raises. The precise real-lib sentinel/resume signature is a
DEC-014 OQ1-class residual (see NOTE in garmin_client.py).

Cites: REQ-003, REQ-005; DEC-012 (page↔SSO seam), DEC-013 (worker↔adapter seam),
DES-012 (adapter as stable seam), LSN-006 (classify by type, not message).
"""

from __future__ import annotations

import types
from typing import Any

import pytest

from garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeGarminMfaBase:
    """Minimal fake of garminconnect.Garmin for the two-step MFA flow.

    login() returns the needs-MFA sentinel; resume_login() is overridden per
    test. On a successful resume the library populates .full_name_id /
    .display_name on the instance (as the adapter reads them).
    """

    display_name: str = ""
    full_name_id: str = ""
    MFA_STATE = {"client": "state-opaque"}

    def __init__(self, email: str, password: str) -> None:
        self.email = email
        self.password = password
        self.resume_calls: list[tuple[str, Any]] = []

    def login(self) -> Any:
        # garth returns a ("needs_mfa", client_state) sentinel when the account
        # needs an OTP (rather than raising). The adapter must retain the state.
        return ("needs_mfa", self.MFA_STATE)

    def resume_login(self, code: str, client_state: Any) -> None:  # overridden per test
        raise NotImplementedError


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake exposing the surface the adapter
    touches: Garmin class + exceptions.GarminConnectAuthenticationError.
    """
    import garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthError,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_submit_mfa_valid_code_returns_success_identity(monkeypatch: pytest.MonkeyPatch) -> None:
    """T-027 — submit_mfa(code) GREEN.

    With a fake `_gc` scripted to ACCEPT the code, submit_mfa() returns the
    success dict shape IDENTICAL to login()'s no-MFA success return, so the
    worker's PyAuthOutcome::Success mapping is the same for both paths.
    """

    class _MfaOkGarmin(_FakeGarminMfaBase):
        def resume_login(self, code: str, client_state: Any) -> None:
            self.resume_calls.append((code, client_state))
            # The library completes auth + populates the identity on success.
            self.full_name_id = "77"
            self.display_name = "MFA Athlete"

    _install_fake_gc(monkeypatch, _MfaOkGarmin)

    client = GarminClient("u@x.com", "p")

    # Step 1 — login() surfaces the MFA-required sentinel and retains state.
    assert client.login() == {"mfa_required": True}

    # Step 2 — a valid code completes auth on the SAME session.
    result = client.submit_mfa("123456")
    assert result == {"garmin_user_id": "77", "display_name": "MFA Athlete"}, (
        "submit_mfa() must return the SAME identity dict shape login() returns "
        "on a no-MFA success, so the worker's Success mapping is identical"
    )
    # The retained client_state was forwarded verbatim to resume_login(), and
    # the code reached the library unmangled.
    assert client._garmin.resume_calls == [("123456", _MfaOkGarmin.MFA_STATE)]


def test_login_mfa_path_retains_state_and_submit_mfa_error_kinds(monkeypatch: pytest.MonkeyPatch) -> None:
    """T-028 — login() MFA path + submit_mfa error classification.

    - login() with an MFA-required library signal returns the mfa_required
      sentinel and RETAINS pending state — it does NOT raise.
    - submit_mfa() with a bad code raises GarminError(kind='auth') (classified
      by exception TYPE — LSN-006 — so the Slice-B page can re-prompt).
    - submit_mfa() with NO pending MFA state raises GarminError(kind='unknown').
    """

    class _MfaBadCodeGarmin(_FakeGarminMfaBase):
        def resume_login(self, code: str, client_state: Any) -> None:
            raise _FakeAuthError("invalid one-time code")

    _install_fake_gc(monkeypatch, _MfaBadCodeGarmin)

    client = GarminClient("u@x.com", "p")

    # login() must NOT raise; it surfaces the sentinel and retains state.
    outcome = client.login()
    assert outcome == {"mfa_required": True}
    assert client._garmin is not None  # session object retained  # type: ignore[attr-defined]

    # A bad/expired OTP -> GarminError(kind='auth').
    with pytest.raises(GarminError) as bad:
        client.submit_mfa("000000")
    assert bad.value.kind == "auth", (
        "an invalid MFA code must map to GC-stable kind='auth' (classified by "
        "exception TYPE) so DES-008 routes re-prompt copy"
    )
    assert bad.value.message, "message must be non-empty for UI display"
    assert bad.value.original is not None, "original library exception must be retained for diagnostics"

    # submit_mfa() on a client that never entered the MFA flow -> kind='unknown'.
    fresh = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as none_pending:
        fresh.submit_mfa("123456")
    assert none_pending.value.kind == "unknown", (
        "submit_mfa() with no pending MFA session is a programming/contract "
        "error (kind='unknown'), NOT an authentication failure"
    )


def test_pending_mfa_retained_across_bad_code_then_retry_succeeds(monkeypatch: pytest.MonkeyPatch) -> None:
    """T-038 — B-R003-02: a bad OTP does NOT consume the pending-MFA session.

    login() → mfa_required retains the client_state; a first submit_mfa(bad)
    raises GarminError(kind='auth') but MUST leave `self._pending_mfa` intact so a
    second submit_mfa(good) on the SAME client resumes the SAME session and
    succeeds. resume_login() must therefore be called TWICE, both times with the
    retained client_state. This FAILS if the except-block cleared
    self._pending_mfa (the retry would raise kind='unknown' instead).
    """

    class _MfaRetryGarmin(_FakeGarminMfaBase):
        def resume_login(self, code: str, client_state: Any) -> None:
            self.resume_calls.append((code, client_state))
            if code == "000000":
                raise _FakeAuthError("invalid one-time code")
            # A subsequent good code completes auth on the SAME session.
            self.full_name_id = "88"
            self.display_name = "Retry Rider"

    _install_fake_gc(monkeypatch, _MfaRetryGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.login() == {"mfa_required": True}

    # First attempt: a bad code raises kind='auth' but must NOT clear the session.
    with pytest.raises(GarminError) as bad:
        client.submit_mfa("000000")
    assert bad.value.kind == "auth"

    # Second attempt on the SAME client: the retained pending state lets a good
    # code succeed (it is NOT consumed/cleared by the failure).
    result = client.submit_mfa("123456")
    assert result == {"garmin_user_id": "88", "display_name": "Retry Rider"}

    # resume_login was called TWICE, both on the retained client_state — proving
    # the failure path preserved self._pending_mfa.
    assert client._garmin.resume_calls == [
        ("000000", _MfaRetryGarmin.MFA_STATE),
        ("123456", _MfaRetryGarmin.MFA_STATE),
    ]
