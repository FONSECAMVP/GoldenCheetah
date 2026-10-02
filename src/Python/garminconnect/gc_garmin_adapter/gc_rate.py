"""Rate-limit + retry decorators, Python-side, single chokepoint at
the Python boundary: every call an adapter method makes to Garmin
passes through here, so nothing above the Python boundary can bypass pacing.

Serves 1 req/s pacing and bounded retry with backoff on transient failures.
Reused by garmin_client.py, so `with_retry` takes its transient-exception
predicate from the caller rather than hardcoding one library's exception types
(which would make this module single-library-only).
"""

from __future__ import annotations

import functools
import random
import time as time  # re-exported: tests monkeypatch gc_rate.time.sleep directly
from typing import Callable, TypeVar

F = TypeVar("F", bound=Callable[..., object])

# 1 request/sec, shared module-level state (single chokepoint).
_BUCKET = {"last": 0.0, "min_interval": 1.0}


def rate_limited(fn: F) -> F:
    """Pace calls to `fn` to at most one per `_BUCKET['min_interval']` seconds.

    time.sleep() releases the GIL, so a cooperative cancellation check on the
    caller's side stays responsive across the wait."""

    @functools.wraps(fn)
    def wrapped(*args: object, **kwargs: object) -> object:
        elapsed = time.monotonic() - _BUCKET["last"]
        wait = _BUCKET["min_interval"] - elapsed
        if wait > 0:
            time.sleep(wait)
        try:
            return fn(*args, **kwargs)
        finally:
            _BUCKET["last"] = time.monotonic()

    return wrapped  # type: ignore[return-value]


def with_retry(
    fn: F,
    *,
    max_attempts: int = 3,
    base: float = 0.25,
    cap: float = 2.0,
    is_transient: Callable[[Exception], bool],
) -> F:
    """Retry `fn` up to `max_attempts` times when `is_transient(exc)` is True.

    Exponential backoff (base * 2**attempt) capped at `cap`, plus jitter.
    A non-transient exception propagates immediately, so
    no attempt is wasted on e.g. an auth failure."""

    @functools.wraps(fn)
    def wrapped(*args: object, **kwargs: object) -> object:
        attempt = 0
        while True:
            attempt += 1
            try:
                return fn(*args, **kwargs)
            except Exception as e:  # noqa: BLE001 - filtered by is_transient below
                if not is_transient(e) or attempt >= max_attempts:
                    raise
                delay = min(cap, base * (2 ** (attempt - 1)))
                delay += random.uniform(0, base)
                time.sleep(delay)

    return wrapped  # type: ignore[return-value]
