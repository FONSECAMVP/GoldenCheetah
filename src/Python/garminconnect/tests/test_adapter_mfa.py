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

from gc_garmin_adapter.garmin_client import GarminClient, GarminError


class _FakeAuthError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeConnError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeRateError(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectTooManyRequestsError."""


class _FakeGarminMfaBase:
    """Minimal fake of garminconnect.Garmin for the two-step MFA flow.

    login() returns the needs-MFA sentinel (only reachable because
    return_on_mfa=True — B-STAGE9-10 — is always passed to the constructor);
    resume_login() is overridden per test. On a successful resume the library
    populates .display_name on the instance (as the adapter reads it).

    B-STAGE9-10 (confirmed against the real, installed python-garminconnect
    0.3.15 wheel): resume_login's real parameters are, IN ORDER,
    (client_state, mfa_code) — and the inner Client.resume_login's first
    parameter is even named `_client_state` (leading underscore), confirming
    it is genuinely never read. garmin_client.py therefore retains no
    client_state payload at all (self._pending_mfa is a plain boolean
    sentinel) and calls resume_login(<placeholder>, code) — the OTP in the
    SECOND positional slot. This fake's MFA_STATE value is consequently never
    threaded through the adapter; it exists only so login()'s return shape
    matches the real 2-tuple.

    B-STAGE9-09: deliberately has NO `full_name_id` — the real, installed
    python-garminconnect library never exposed one.
    """

    display_name: str = ""
    MFA_STATE = {"client": "state-opaque"}

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # **_ swallows B-STAGE9-10's return_on_mfa=True.
        self.email = email
        self.password = password
        self.resume_calls: list[tuple[Any, str]] = []

    def login(self) -> Any:
        # garth returns a ("needs_mfa", client_state) sentinel when the account
        # needs an OTP (rather than raising).
        return ("needs_mfa", self.MFA_STATE)

    def resume_login(self, client_state: Any, mfa_code: str) -> None:  # overridden per test
        raise NotImplementedError


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake exposing the surface the adapter
    touches: Garmin class + the exception types resume_login() can raise
    (auth/connection/rate_limit — B-STAGE9-10 repair BLOCKING-3).
    """
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthError,
        GarminConnectConnectionError=_FakeConnError,
        GarminConnectTooManyRequestsError=_FakeRateError,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_submit_mfa_valid_code_returns_success_identity(monkeypatch: pytest.MonkeyPatch) -> None:
    """T-027 — submit_mfa(code) GREEN.

    With a fake `_gc` scripted to ACCEPT the code, submit_mfa() returns the
    success dict shape IDENTICAL to login()'s no-MFA success return, so the
    worker's PyAuthOutcome::Success mapping is the same for both paths.
    """

    class _MfaOkGarmin(_FakeGarminMfaBase):
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
            self.resume_calls.append((client_state, mfa_code))
            # The library completes auth + populates the identity on success.
            self.display_name = "MFA Athlete"

    _install_fake_gc(monkeypatch, _MfaOkGarmin)

    client = GarminClient("u@x.com", "p")

    # Step 1 — login() surfaces the MFA-required sentinel and retains state.
    assert client.login() == {"mfa_required": True}

    # Step 2 — a valid code completes auth on the SAME session.
    result = client.submit_mfa("123456")
    assert result == {"garmin_user_id": "MFA Athlete", "display_name": "MFA Athlete"}, (
        "submit_mfa() must return the SAME identity dict shape login() returns "
        "on a no-MFA success, so the worker's Success mapping is identical"
    )
    # The user's code reached the library unmangled, in the real library's
    # SECOND positional slot (mfa_code) — B-STAGE9-10's fixed argument order.
    assert client._garmin.resume_calls == [(None, "123456")]


def test_login_mfa_path_retains_state_and_submit_mfa_error_kinds(monkeypatch: pytest.MonkeyPatch) -> None:
    """T-028 — login() MFA path + submit_mfa error classification.

    - login() with an MFA-required library signal returns the mfa_required
      sentinel and RETAINS pending state — it does NOT raise.
    - submit_mfa() with a bad code raises GarminError(kind='auth') (classified
      by exception TYPE — LSN-006 — so the Slice-B page can re-prompt).
    - submit_mfa() with NO pending MFA state raises GarminError(kind='unknown').
    """

    class _MfaBadCodeGarmin(_FakeGarminMfaBase):
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
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
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
            self.resume_calls.append((client_state, mfa_code))
            if mfa_code == "000000":
                raise _FakeAuthError("invalid one-time code")
            # A subsequent good code completes auth on the SAME session.
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
    assert result == {"garmin_user_id": "Retry Rider", "display_name": "Retry Rider"}

    # resume_login was called TWICE, both times with the user's code in the
    # real library's SECOND positional slot (mfa_code) — proving the failure
    # path preserved self._pending_mfa rather than clearing it.
    assert client._garmin.resume_calls == [
        (None, "000000"),
        (None, "123456"),
    ]


def test_submit_mfa_identity_shape_mismatch_raises_classified_GarminError(monkeypatch: pytest.MonkeyPatch) -> None:
    """B-STAGE9-09 — MFA-path counterpart of the login() regression guard.

    A successful resume_login() must not let a post-success identity-shape
    mismatch (the B-STAGE9-08 live-account `full_name_id` root cause) escape
    as a raw exception; it must raise a classified GarminError(kind='unknown')
    with a diagnosable message instead. The pending-MFA state is still cleared
    (resume_login() already consumed the one-time code — REQ-003).
    """

    class _ShapeMismatchGarmin(_FakeGarminMfaBase):
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
            self.resume_calls.append((client_state, mfa_code))

        @property
        def display_name(self) -> str:  # type: ignore[override]
            raise AttributeError("simulated: real library dropped this attribute")

    _install_fake_gc(monkeypatch, _ShapeMismatchGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.login() == {"mfa_required": True}

    with pytest.raises(GarminError) as excinfo:
        client.submit_mfa("123456")

    assert excinfo.value.kind == "unknown", (
        "a post-MFA identity shape mismatch is a programming/library-compat "
        "error, not an authentication failure — must classify as "
        "kind='unknown', never escape as a raw exception"
    )
    assert "AttributeError" in excinfo.value.message, "message must be diagnosable"
    assert isinstance(excinfo.value.original, AttributeError)
    assert client._pending_mfa is None, (
        "resume_login() already consumed the one-time code — the pending "
        "state must still be cleared even though identity resolution failed"
    )


def test_submit_mfa_connection_error_after_correct_code_retries_and_classifies(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-10 repair BLOCKING-3 — RED today.

    Read directly off the real, installed python-garminconnect 0.3.15 wheel's
    inner Client.resume_login source: AFTER `_complete_mfa()` has already
    verified a CORRECT one-time code, the real library can still raise
    `GarminConnectConnectionError("token rejected by API tier after MFA")` —
    a transient failure distinct from a bad/expired OTP. Today's adapter
    catches only GarminConnectAuthenticationError from resume_login(), so this
    escapes raw instead of becoming a classified, RETRYABLE GarminError. That
    also means the existing kind='connection'-is-transient retry policy
    (is_transient below submit_mfa) never engages for this real failure mode.
    """

    class _MfaConnectionErrorGarmin(_FakeGarminMfaBase):
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
            self.resume_calls.append((client_state, mfa_code))
            # Mirrors the real library: _complete_mfa() already verified the
            # code; the failure happens AFTER, unconditionally here so the
            # test can assert the retry count deterministically.
            raise _FakeConnError("token rejected by API tier after MFA")

    _install_fake_gc(monkeypatch, _MfaConnectionErrorGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.login() == {"mfa_required": True}

    with pytest.raises(GarminError) as excinfo:
        client.submit_mfa("123456")

    assert excinfo.value.kind == "connection", (
        "a post-verification connection failure from resume_login() must "
        "classify as kind='connection' (GarminConnectConnectionError), never "
        "escape as a raw exception — this is the SAME failure pattern as "
        "B-STAGE9-09, reintroduced on a new line"
    )
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"
    # DES-005's retry policy (base=0.25s, cap=2.0s, max_attempts=3) must have
    # actually engaged now that the failure is classified as transient.
    assert len(client._garmin.resume_calls) == 3, (
        "kind='connection' is in submit_mfa's is_transient set — the with_retry "
        "wrapper must have retried resume_login up to the spec'd 3 attempts, "
        "proving the classification (not just its message) drives real retry "
        "behaviour"
    )
    assert client._pending_mfa is not None, (
        "a transient failure must NOT clear the pending MFA session (same "
        "precedent as a bad-OTP auth failure) so a subsequent submit_mfa "
        "retry can still resume the SAME session"
    )


def test_submit_mfa_rate_limit_error_is_classified(monkeypatch: pytest.MonkeyPatch) -> None:
    """B-STAGE9-10 repair BLOCKING-3 (rate-limit half) — RED today.

    Read directly off the real wheel's inner Client._complete_mfa source: if
    every MFA-verify endpoint is rate-limited, it raises
    GarminConnectTooManyRequestsError. Today's adapter does not catch this
    from resume_login() either, so it also escapes raw instead of classifying
    as the existing kind='rate_limit'.
    """

    class _MfaRateLimitGarmin(_FakeGarminMfaBase):
        def resume_login(self, client_state: Any, mfa_code: str) -> None:
            self.resume_calls.append((client_state, mfa_code))
            raise _FakeRateError("MFA verification rate limited on all endpoints")

    _install_fake_gc(monkeypatch, _MfaRateLimitGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.login() == {"mfa_required": True}

    with pytest.raises(GarminError) as excinfo:
        client.submit_mfa("123456")

    assert excinfo.value.kind == "rate_limit", (
        "a rate-limited MFA verification must classify as kind='rate_limit', " "never escape as a raw exception"
    )
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"
