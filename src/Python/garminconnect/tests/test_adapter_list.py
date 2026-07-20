"""T-042 — REQ-008 Slice A: list activities since a timestamp (adapter side).

PRD acceptance (REQ-008 / US-3, the *listing* half): "Activities newer than the
last-successful-sync timestamp are downloaded." Slice A delivers the listing:
``list_activities_since(ts_gmt)`` returns the activities whose Garmin server-side
``startTimeGMT`` is newer than ``ts_gmt``. ``startTimeGMT`` is Garmin's
server-side timestamp, NOT the local clock (DES-010). Dedup + download are later
slices (B, C) and are explicitly out of scope here.

Scope of THIS slice (the DES-012 adapter, Python side only):
  - list_activities_since forwards the ``ts_gmt`` timestamp to the underlying
    library VERBATIM (no reformatting / no local-clock substitution — DES-010),
    and returns an iterator of summary dicts each carrying at least
    ``activityId`` and ``startTimeGMT``.
  - listing-time library errors are translated to GC-stable GarminError kinds
    ('connection', 'rate_limit') so DES-008 can switch on .kind without knowing
    the library's exception class names — classified by exception TYPE (LSN-006).
  - a non-library exception is NOT misclassified (LSN-006): it propagates
    unchanged rather than inheriting a Garmin kind (so a downstream bug is never
    displayed as a network/rate-limit failure).

Explicitly NOT in this slice:
  - Incremental-sync orchestration (DES-010 steps 5-7), the per-account sidecar
    dedup (Tier-1 imported-<uid>.json), and the actual downloads — those are the
    C++ GarminConnect sync path (slices B/C), which calls this adapter once to
    list, then download_activity() per new activity.

Cites: REQ-008, US-3; DEC-002 (worker is sole caller), DEC-013 (worker↔adapter
seam), DES-010 (server-side startTimeGMT, incremental flow), DES-012 (adapter as
the single library-knowledge seam), DES-008 (error switch keyed on .kind);
LSN-006 (classify by exception type, never misroute a foreign exception),
LSN-022 (a real scenario, not a tautology).

RED expectation: garmin_client.GarminClient.list_activities_since currently
raises NotImplementedError("REQ-008 ...") — every test below fails for that
reason (right-reason RED). GREEN implements the forward+translate contract.
"""

from __future__ import annotations

import types
from typing import Any

import pytest

from garmin_client import GarminClient, GarminError


class _FakeConnErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeRateErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectTooManyRequestsError."""


class _FakeAuthErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FakeGarminBase:
    """Minimal fake of garminconnect.Garmin exposing only what the listing path
    touches: get_activities_by_date()."""

    def __init__(self, email: str, password: str) -> None:
        # DEC-014 Option B: AUTH-ONLY construction — no tokenstore path.
        self.email = email
        self.password = password

    def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
        raise NotImplementedError  # overridden per test


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake module exposing the surface the
    adapter listing path touches: the Garmin class + the listing-relevant
    exception types."""
    import garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthErr,
        GarminConnectConnectionError=_FakeConnErr,
        GarminConnectTooManyRequestsError=_FakeRateErr,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_list_returns_summaries_and_forwards_timestamp_verbatim(monkeypatch: pytest.MonkeyPatch) -> None:
    """REQ-008/US-3 acceptance (listing half) — the ts_gmt is forwarded to the
    library VERBATIM (DES-010: it is Garmin's server-side timestamp, never the
    local clock), and each underlying activity becomes a summary dict carrying at
    least activityId + startTimeGMT. A REAL scenario (LSN-022): the library
    returns realistic activity records (int activityId, extra keys) and the
    adapter yields the GC-stable summaries the C++ side marshals."""
    captured: dict[str, Any] = {}
    library_activities = [
        {"activityId": 1001, "startTimeGMT": "2026-07-01 06:30:00", "activityName": "Morning Ride"},
        {"activityId": 1002, "startTimeGMT": "2026-07-03 18:05:11", "activityName": "Evening Run"},
    ]

    class _OkGarmin(_FakeGarminBase):
        def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
            captured["args"] = args
            captured["kwargs"] = kwargs
            return library_activities

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("u@x.com", "p")
    result = list(client.list_activities_since("2026-06-30 00:00:00"))

    # The since-timestamp reached the library verbatim (DES-010 — not reformatted,
    # not swapped for a local-clock value). It is the first forwarded argument.
    forwarded = list(captured["args"]) + list(captured["kwargs"].values())
    assert "2026-06-30 00:00:00" in forwarded, "the since-timestamp must reach the library verbatim (DES-010)"

    # Each underlying activity becomes a summary carrying at least the two keys.
    assert len(result) == 2, "one summary per underlying activity"
    assert result[0]["activityId"] == "1001", "activityId must survive to the summary"
    assert result[0]["startTimeGMT"] == "2026-07-01 06:30:00", "startTimeGMT must survive to the summary"
    assert result[1]["activityId"] == "1002"
    assert result[1]["startTimeGMT"] == "2026-07-03 18:05:11"


def test_list_connection_error_maps_to_kind_connection(monkeypatch: pytest.MonkeyPatch) -> None:
    """A library connection error during listing must surface as GC-stable
    kind='connection' so DES-008 routes the "couldn't reach Garmin" copy — not a
    raw library class name. Classified by exception TYPE (LSN-006)."""

    class _ConnGarmin(_FakeGarminBase):
        def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
            raise _FakeConnErr("stub: listing connection refused")

    _install_fake_gc(monkeypatch, _ConnGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        list(client.list_activities_since("2026-06-30 00:00:00"))

    assert excinfo.value.kind == "connection"
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"


def test_list_rate_limit_error_maps_to_kind_rate_limit(monkeypatch: pytest.MonkeyPatch) -> None:
    """Garmin 429 during listing must surface as kind='rate_limit' (DES-008
    rate-limit copy / DES-005 pacing), never a generic or auth error."""

    class _RateGarmin(_FakeGarminBase):
        def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
            raise _FakeRateErr("stub: 429 too many requests")

    _install_fake_gc(monkeypatch, _RateGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        list(client.list_activities_since("2026-06-30 00:00:00"))

    assert excinfo.value.kind == "rate_limit"
    assert excinfo.value.original is not None


def test_list_non_library_exception_is_not_misclassified(monkeypatch: pytest.MonkeyPatch) -> None:
    """LSN-006 — a foreign exception (RuntimeError) from the library must NOT be
    caught-and-relabelled with a Garmin kind. It propagates unchanged so a
    downstream bug is never displayed as a network / rate-limit failure. NOT a
    tautology (LSN-022): the adapter really runs and really re-raises the exact
    foreign exception."""

    class _BoomGarmin(_FakeGarminBase):
        def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
            raise RuntimeError("simulated downstream library bug")

    _install_fake_gc(monkeypatch, _BoomGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(RuntimeError, match="simulated downstream library bug"):
        list(client.list_activities_since("2026-06-30 00:00:00"))


def test_list_empty_result_is_success_not_error(monkeypatch: pytest.MonkeyPatch) -> None:
    """An empty listing (no activities newer than ts_gmt) is a NORMAL success
    state, not an error (DES-009 "Empty result is success" / DES-010) — the
    adapter returns an empty iterator, it does NOT raise."""

    class _EmptyGarmin(_FakeGarminBase):
        def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
            return []

    _install_fake_gc(monkeypatch, _EmptyGarmin)

    client = GarminClient("u@x.com", "p")
    assert list(client.list_activities_since("2026-06-30 00:00:00")) == []
