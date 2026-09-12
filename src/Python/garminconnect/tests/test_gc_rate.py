"""REQ-010 (DES-005 bundled) — gc_rate.py: rate-limit + retry decorators.

PRD REQ-010 acceptance references REQ-NF-Perf-002 (paced calls) and
REQ-NF-Reliab-001 (bounded retry). design.md DES-005 is the governing spec:
`rate_limited` paces calls to >=1s apart (module-level `_BUCKET`); `with_retry`
retries a transient failure up to 3 attempts with exponential backoff + jitter,
and does NOT retry a non-transient one.

Cites: REQ-010, REQ-NF-Perf-002, REQ-NF-Reliab-001; DEC-007 (decorators, not
inline sleeps); DES-005.

RED expectation: gc_rate.py does not exist yet — collection fails at the
`from gc_rate import ...` line (right-reason RED, ModuleNotFoundError).
"""

from __future__ import annotations

import random
import time
import types
from collections.abc import Callable

import pytest

from gc_rate import _BUCKET, rate_limited, with_retry


@pytest.fixture(autouse=True)  # type: ignore[misc]
def _reset_bucket() -> None:
    """Every test starts with a bucket that never delays the first call."""
    _BUCKET["last"] = 0.0
    _BUCKET["min_interval"] = 1.0


def test_rate_limited_paces_consecutive_calls(monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-NF-Perf-002 — a second call within the pacing window sleeps for the
    remainder; the first call (bucket empty) never sleeps."""
    clock = {"now": 1000.0}
    sleeps: list[float] = []

    monkeypatch.setattr(time, "monotonic", lambda: clock["now"])
    monkeypatch.setattr(time, "sleep", lambda s: sleeps.append(s))

    @rate_limited
    def call() -> str:
        return "ok"

    assert call() == "ok"
    assert sleeps == [], "first call must not wait (bucket starts empty)"

    clock["now"] += 0.4  # 0.4s later, well inside the 1s window
    assert call() == "ok"
    assert len(sleeps) == 1
    assert sleeps[0] == pytest.approx(0.6, abs=1e-9), "must wait the REMAINDER of the 1s window"


def test_rate_limited_no_wait_once_interval_elapsed(monkeypatch: pytest.MonkeyPatch) -> None:
    """A call arriving >= min_interval after the last one is not delayed."""
    clock = {"now": 2000.0}
    sleeps: list[float] = []
    monkeypatch.setattr(time, "monotonic", lambda: clock["now"])
    monkeypatch.setattr(time, "sleep", lambda s: sleeps.append(s))

    @rate_limited
    def call() -> None:
        return None

    call()
    clock["now"] += 1.5
    call()
    assert sleeps == [], "a call outside the pacing window must not sleep"


def test_rate_limited_updates_bucket_after_fn_raises(monkeypatch: pytest.MonkeyPatch) -> None:
    """The pacing clock advances even when the wrapped call fails, so a
    failing call still counts against the next call's wait (no free retry
    burst just because the underlying call raised)."""
    clock = {"now": 3000.0}
    monkeypatch.setattr(time, "monotonic", lambda: clock["now"])
    monkeypatch.setattr(time, "sleep", lambda _s: None)

    @rate_limited
    def boom() -> None:
        raise RuntimeError("network dropped")

    with pytest.raises(RuntimeError):
        boom()
    assert _BUCKET["last"] == clock["now"]


class _TransientError(Exception):
    pass


class _FatalError(Exception):
    pass


def _is_transient(e: Exception) -> bool:
    return isinstance(e, _TransientError)


def test_with_retry_retries_transient_then_succeeds(monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-NF-Reliab-001 — a transient failure is retried (with backoff) and a
    later success is returned; the caller never sees the transient exception."""
    monkeypatch.setattr(time, "sleep", lambda _s: None)
    attempts = {"n": 0}

    def flaky() -> str:
        attempts["n"] += 1
        if attempts["n"] < 3:
            raise _TransientError("stub: connection refused")
        return "recovered"

    wrapped = with_retry(flaky, is_transient=_is_transient, max_attempts=3, base=0.01, cap=0.05)
    assert wrapped() == "recovered"
    assert attempts["n"] == 3


def test_with_retry_gives_up_after_max_attempts(monkeypatch: pytest.MonkeyPatch) -> None:
    """Exhausting max_attempts re-raises the transient exception to the caller
    (DES-009: this is what a C++ controller sees as retry-exhausted)."""
    monkeypatch.setattr(time, "sleep", lambda _s: None)
    attempts = {"n": 0}

    def always_fails() -> None:
        attempts["n"] += 1
        raise _TransientError("stub: still down")

    wrapped = with_retry(always_fails, is_transient=_is_transient, max_attempts=3, base=0.01, cap=0.05)
    with pytest.raises(_TransientError):
        wrapped()
    assert attempts["n"] == 3, "must attempt exactly max_attempts times, no more"


def test_with_retry_does_not_retry_non_transient(monkeypatch: pytest.MonkeyPatch) -> None:
    """A non-transient exception (e.g. auth failure) propagates on the FIRST
    attempt — never retried, never delayed."""
    sleeps: list[float] = []
    monkeypatch.setattr(time, "sleep", lambda s: sleeps.append(s))
    attempts = {"n": 0}

    def fatal() -> None:
        attempts["n"] += 1
        raise _FatalError("stub: bad credentials")

    wrapped = with_retry(fatal, is_transient=_is_transient, max_attempts=3, base=0.01, cap=0.05)
    with pytest.raises(_FatalError):
        wrapped()
    assert attempts["n"] == 1
    assert sleeps == []


def _decoration_depth(fn: object) -> int:
    """How many functools.wraps layers wrap `fn` (login/submit_mfa/list_
    activities_since/download_activity are each rate_limited THEN with_retry
    per DES-005's composition table -> depth 2 when both are applied)."""
    depth = 0
    while hasattr(fn, "__wrapped__"):
        depth += 1
        fn = fn.__wrapped__
    return depth


def test_every_network_calling_GarminClient_method_is_rate_limited_and_retried() -> None:
    """DES-005 — every public GarminClient method that hits the network must be
    paced (rate_limited) AND retried (with_retry), not just list_activities_since/
    download_activity. A method added/left without both decorators would bypass
    the single pacing chokepoint DES-005 exists to enforce."""
    from garmin_client import GarminClient

    for name in ("login", "submit_mfa", "list_activities_since", "download_activity"):
        fn = getattr(GarminClient, name)
        assert _decoration_depth(fn) == 2, (
            f"GarminClient.{name} must be wrapped by both rate_limited and "
            "with_retry (DES-005) - found decoration depth "
            f"{_decoration_depth(fn)} instead of 2"
        )


def test_with_retry_backoff_is_exponential_and_capped(monkeypatch: pytest.MonkeyPatch) -> None:
    """Backoff grows exponentially (base * 2**attempt) and never exceeds cap,
    before jitter is added."""
    delays: list[float] = []
    monkeypatch.setattr(time, "sleep", lambda s: delays.append(s))
    monkeypatch.setattr(random, "uniform", lambda _a, _b: 0.0)  # isolate the exponential term
    attempts = {"n": 0}

    def always_fails() -> None:
        attempts["n"] += 1
        raise _TransientError("stub")

    wrapped = with_retry(always_fails, is_transient=_is_transient, max_attempts=4, base=0.1, cap=0.3)
    with pytest.raises(_TransientError):
        wrapped()

    # attempts 1,2,3 each fail-then-sleep before attempt 4 fails and re-raises
    # (no sleep after the LAST attempt): base*2**0, base*2**1, min(cap, base*2**2)
    assert delays == pytest.approx([0.1, 0.2, 0.3])


# --- T-204 (REQ-NF-Reliab-001) — the PRODUCTION-bound retry schedule ----------
# test_every_network_calling_GarminClient_method_is_rate_limited_and_retried
# proves the four network methods carry BOTH decorators (depth 2); this test
# proves they bind the SPEC'D schedule — 250 ms base, 2 s cap, 3 attempts
# (with_retry's signature defaults, never overridden at any of the four call
# sites). A regression that overrode cap=10.0 would pass a depth-only check;
# it cannot pass this one.


def _transient_failer(counter: dict[str, int]) -> Callable[..., object]:
    """An impl that fails with a transient GarminError forever, counting tries."""
    from garmin_client import GarminError

    def fail(*_args: object, **_kwargs: object) -> object:
        counter["n"] += 1
        raise GarminError("connection", "stub: still down")

    return fail


_SPEC_CALL_ARGS: dict[str, tuple[object, ...]] = {
    "login": (),
    "submit_mfa": ("123456",),
    "list_activities_since": ("2026-01-01 00:00:00",),
    "download_activity": ("42", "ORIGINAL"),
}


def _closure_cells(fn: object) -> dict[str, object]:
    """Name-keyed closure cells of a decorator's wrapped function."""
    assert isinstance(fn, types.FunctionType), "expected a plain wrapped function"
    assert fn.__closure__ is not None, "expected a closure (decorator-captured args)"
    return dict(zip(fn.__code__.co_freevars, (cell.cell_contents for cell in fn.__closure__)))


def test_production_GarminClient_retry_binds_the_spec_schedule(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """The REAL wrapped GarminClient.{login,submit_mfa,list_activities_since,
    download_activity} retry with exactly the prd schedule (250 ms base, 2 s
    cap, max 3 attempts).

    Two halves, because no single mechanism observes all three parameters:

    - Behavioural (base + max_attempts): the wrapped method is driven with
      time.sleep recorded and jitter zeroed against an always-transient impl;
      the delays must be exactly [0.25, 0.5] over exactly 3 attempts.
    - Closure cells (cap): with max_attempts=3 the largest scheduled delay is
      0.5 s, so cap never binds and is behaviourally unobservable through the
      production binding — the functools.wraps-preserved closure cell is the
      only honest way to pin it.

    The transient failure is injected by writing a fake into the
    rate_limited layer's closure cell (CPython cells are writable) rather than
    by patching GarminClient._<name>_impl: the impl is captured in the
    decorator closures at class-body time, so a class-attr patch would never
    be reached and the test would exercise nothing. This way the REAL
    with_retry+rate_limited chain stays fully in the path under test.
    """
    from garmin_client import GarminClient, GarminError

    sleeps: list[float] = []
    monkeypatch.setattr(time, "sleep", lambda s: sleeps.append(s))
    monkeypatch.setattr(random, "uniform", lambda _a, _b: 0.0)  # isolate base/cap terms
    monkeypatch.setitem(_BUCKET, "min_interval", 0.0)  # pacing must not add sleeps

    for name, call_args in _SPEC_CALL_ARGS.items():
        fn = getattr(GarminClient, name)

        # (1) Closure cells — pins all three parameters, including cap, which
        # the behavioural half cannot reach within 3 attempts.
        cells = _closure_cells(fn)
        assert cells.get("base") == 0.25, f"{name}: base must stay the spec'd 0.25 s"
        assert cells.get("cap") == 2.0, f"{name}: cap must stay the spec'd 2.0 s"
        assert cells.get("max_attempts") == 3, f"{name}: max_attempts must stay 3"

        # (2) Behaviour — real with_retry+rate_limited chain, transient impl
        # failing forever: exactly 3 attempts, sleeps exactly [base, 2*base]
        # (cap not reached at these attempt indexes), then the transient
        # error surfaces to the caller.
        attempts = {"n": 0}
        always_transient = _transient_failer(attempts)

        inner = fn.__wrapped__
        inner_cells = _closure_cells(inner)
        assert set(inner_cells) == {"fn"}, f"{name}: unexpected rate_limited closure shape"
        original_impl = inner_cells["fn"]
        assert inner.__closure__ is not None and len(inner.__closure__) == 1
        cell = inner.__closure__[0]
        cell.cell_contents = always_transient
        try:
            sleeps.clear()
            with pytest.raises(GarminError) as excinfo:
                getattr(GarminClient.__new__(GarminClient), name)(*call_args)
            assert excinfo.value.kind == "connection", f"{name}: transient kind must surface"
            assert attempts["n"] == 3, f"{name}: must give up after exactly 3 attempts"
            assert sleeps == pytest.approx([0.25, 0.5]), (
                f"{name}: retry delays must be the spec'd 250 ms -> 500 ms schedule "
                "(250 ms base, exponential, cap 2.0 s, jitter zeroed)"
            )
        finally:
            cell.cell_contents = original_impl
