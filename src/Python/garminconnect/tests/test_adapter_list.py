"""T-042 — REQ-008 Slice A: list activities since a timestamp (adapter side).

PRD acceptance (REQ-008 / US-3, the *listing* half): "Activities newer than the
last-successful-sync timestamp are downloaded." Slice A delivers the listing:
``list_activities_since(ts_gmt)`` returns the activities whose Garmin server-side
``startTimeGMT`` is newer than ``ts_gmt``. ``startTimeGMT`` is Garmin's
server-side timestamp, NOT the local clock (DES-010). Dedup + download are later
slices (B, C) and are explicitly out of scope here.

Scope of THIS slice (the DES-012 adapter, Python side only):
  - list_activities_since truncates ``ts_gmt`` to a date-only string
    (YYYY-MM-DD) before forwarding it to the underlying library's
    ``get_activities_by_date`` (B-STAGE9-25 — the real library rejects a full
    datetime as its FIRST statement, before any HTTP I/O), and returns an
    iterator of summary dicts each carrying at least ``activityId`` and
    ``startTimeGMT``.
  - the cursor AND every response ``startTimeGMT`` go through one instant
    path and are compared as UTC instants with ``>=`` (B-STAGE9-25 round 3 —
    the response separator shape is the server's choice, not ours to assume:
    ``parseGarminTime`` already accepts both space- and T-separated forms);
    a missing, non-string, or unparseable response timestamp raises
    ``GarminError(kind='response_invalid')`` so Tier-1 activityId dedup, not
    a timestamp boundary, owns de-duplication.
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

import re
import types
from datetime import datetime
from typing import Any

import pytest

from garmin_client import GarminClient, GarminError


class _FakeConnErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeRateErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectTooManyRequestsError."""


class _FakeAuthErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


# Mirrors garminconnect._validate_date_format (installed wheel 0.3.15,
# __init__.py:63-74), which get_activities_by_date calls on `startdate` as its
# very first statement (B-STAGE9-25). Verified interactively against the real
# wheel: _validate_date_format('2026-09-09 05:21:24', 'startdate') raises.
_REAL_STARTDATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def _validate_startdate_like_real_library(value: Any) -> None:
    # Mirrors the real function's own three steps (strip, regex shape, then
    # real-calendar validation) — round 1's regex-only version accepted
    # impossible dates like 2026-02-30 (findings.md B-STAGE9-25 round 2).
    if not isinstance(value, str):
        raise ValueError("startdate must be a string")
    stripped = value.strip()
    if not _REAL_STARTDATE_RE.match(stripped):
        raise ValueError(f"startdate must be in format 'YYYY-MM-DD', got: {stripped}")
    try:
        datetime.strptime(stripped, "%Y-%m-%d")
    except ValueError as e:
        raise ValueError(f"invalid startdate: {e}") from e


class _FakeGarminBase:
    """Minimal fake of garminconnect.Garmin exposing only what the listing path
    touches: get_activities_by_date()."""

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # DEC-014 Option B: AUTH-ONLY construction — no tokenstore path.
        # **_ swallows B-STAGE9-10's return_on_mfa=True (unused on this path).
        self.email = email
        self.password = password

    def get_activities_by_date(self, *args: Any, **kwargs: Any) -> Any:
        # B-STAGE9-25 — validate the way the real wheel does BEFORE the
        # per-test behaviour below, so a fake can never accept a call shape
        # the real library rejects.
        _validate_startdate_like_real_library(args[0] if args else kwargs.get("startdate"))
        return self._get_activities_by_date_impl(*args, **kwargs)

    def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
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


def test_list_truncates_since_timestamp_to_date_only_one_day_early_for_the_library_call(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 acceptance — get_activities_by_date's real contract (pinned
    against the installed 0.3.15 wheel) rejects anything but a date-only
    YYYY-MM-DD string as its FIRST statement, before any HTTP I/O. A full
    datetime ts_gmt (our kGarminTimeFormat shape) must be truncated to a
    date-only string for the query, and queried ONE DAY EARLY than the cursor's
    date (round 2 — the real endpoint's startDate day-boundary timezone
    semantics are undocumented; the post-query GMT filter, proven separately
    below, is what keeps the result exact regardless of this margin). Each
    underlying activity still becomes a summary dict carrying at least
    activityId + startTimeGMT (LSN-022: a real scenario, int activityId, extra
    keys)."""
    captured: dict[str, Any] = {}
    library_activities = [
        {"activityId": 1001, "startTimeGMT": "2026-07-01 06:30:00", "activityName": "Morning Ride"},
        {"activityId": 1002, "startTimeGMT": "2026-07-03 18:05:11", "activityName": "Evening Run"},
    ]

    class _OkGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            captured["args"] = args
            captured["kwargs"] = kwargs
            return library_activities

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("u@x.com", "p")
    result = list(client.list_activities_since("2026-06-30 05:21:24"))

    forwarded = list(captured["args"]) + list(captured["kwargs"].values())
    assert forwarded == ["2026-06-29"], (
        "the query sent to get_activities_by_date must be date-only "
        "(YYYY-MM-DD) and one day earlier than the cursor's date"
    )

    # Each underlying activity becomes a summary carrying at least the two keys.
    assert len(result) == 2, "one summary per underlying activity"
    assert result[0]["activityId"] == "1001", "activityId must survive to the summary"
    assert result[0]["startTimeGMT"] == "2026-07-01 06:30:00", "startTimeGMT must survive to the summary"
    assert result[1]["activityId"] == "1002"
    assert result[1]["startTimeGMT"] == "2026-07-03 18:05:11"


def test_list_cursor_that_fails_to_parse_raises_typed_cursor_invalid_not_bare_valueerror(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 2, Item 1 — a torn/foreign sidecar can supply a
    non-empty but unparseable cursor (loadBackfillState reads it with
    QJsonValue::toString(), never validated — findings.md B-STAGE9-25 round 2).
    That must fail CLOSED as a typed GarminError(kind='cursor_invalid'), never
    as a bare ValueError (which would cross the embedded-Python bridge
    unclassified) and never by silently falling back to a different window."""

    class _UnusedGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            raise AssertionError("the library must never be called with an unparseable cursor")

    _install_fake_gc(monkeypatch, _UnusedGarmin)
    client = GarminClient("u@x.com", "p")

    for bad_cursor in ("not-a-date", "2026-02-30 12:00:00", "2026-09-09 junk"):
        with pytest.raises(GarminError) as excinfo:
            list(client.list_activities_since(bad_cursor))
        assert excinfo.value.kind == "cursor_invalid", (
            f"a malformed cursor ({bad_cursor!r}) must classify as kind='cursor_invalid', "
            f"got {excinfo.value.kind!r}"
        )
        assert bad_cursor in excinfo.value.message, "the offending cursor value must be in the message"


def test_list_filters_out_activities_older_than_the_full_precision_cursor(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 2, Item 2 — the date-only query widens the window to
    the whole day, so an activity the library returns can be OLDER than the
    real (full-precision) ts_gmt cursor. That must be filtered out here, using
    the untruncated ts_gmt this adapter still has in scope — never forwarded to
    the caller, and never allowed to regress the incremental-sync cursor
    downstream (recordImport() has no monotonic guard of its own — GarminConnect.cpp:909;
    findings.md B-STAGE9-25 round 2). Round 3 flips the boundary from strict
    ``>`` to ``>=``: an activity exactly AT the cursor now survives (no
    uniqueness invariant exists on startTimeGMT, so a same-second sibling of an
    already-imported activity must not be stranded — findings.md B-STAGE9-25
    round 3); an equal write cannot regress the cursor, and Tier-1 activityId
    dedup suppresses the already-imported one."""
    library_activities = [
        {"activityId": 1, "startTimeGMT": "2026-09-09 07:00:00"},  # older than cursor — must be dropped
        {"activityId": 2, "startTimeGMT": "2026-09-09 18:00:00"},  # exactly at cursor — must survive (>=)
        {"activityId": 3, "startTimeGMT": "2026-09-09 18:00:01"},  # one second newer — must survive
    ]

    class _WidenedGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _WidenedGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-09-09 18:00:00"))

    assert [r["activityId"] for r in result] == [
        "2",
        "3",
    ], "only activities at or newer than the full-precision cursor may survive the filter"


def test_list_end_to_end_cursor_regression_scenario_is_prevented(monkeypatch: pytest.MonkeyPatch) -> None:
    """B-STAGE9-25 round 2 — the reviewer's exact scenario, driven end-to-end:
    cursor at 2026-09-09 18:00:00, the widened date-only query returns an
    activity at 2026-09-09 07:00:00 (as it would if the imported-ID sidecar
    were torn and Tier-1 dedup could not catch it — GarminConnect.cpp:865-867
    only dedupes by activityId, not by this timestamp). That activity must
    never reach the caller at all, so it can never reach recordImport()
    (GarminConnect.cpp:909, which persists whatever startTimeGMT it is handed
    with no comparison against the existing cursor) and regress the cursor
    backwards."""
    library_activities = [
        {"activityId": "old-slipped-through", "startTimeGMT": "2026-09-09 07:00:00"},
    ]

    class _RegressionGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _RegressionGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-09-09 18:00:00"))

    assert result == [], (
        "an activity older than the cursor must never reach the caller — if it did, "
        "the C++ side would persist it as the new cursor and regress incremental sync"
    )


def test_list_t_separated_response_older_than_the_cursor_is_excluded(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 3, Item 1 — the round-2 filter compared raw strings:
    ``str(startTimeGMT) > ts_gmt``. ``'T'`` (0x54) sorts above ``' '`` (0x20),
    so a T-separated activity ELEVEN HOURS OLDER than the cursor passed the
    filter and would reach recordImport() (GarminConnect.cpp:909), silently
    reinstating the cursor regression the filter was added to prevent. The
    comparison must be between UTC instants through the same parsing path the
    cursor uses — parseGarminTime (GarminConnect.cpp:119-127) already accepts
    BOTH separator forms, so neither form is ours to assume away."""
    library_activities = [
        {"activityId": "t-old", "startTimeGMT": "2026-09-09T07:00:00"},  # 11h older, T-separated — must be dropped
        {"activityId": "space-new", "startTimeGMT": "2026-09-09 19:00:00"},  # newer — must survive
    ]

    class _MixedShapesGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _MixedShapesGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-09-09 18:00:00"))

    assert [r["activityId"] for r in result] == ["space-new"], (
        "a T-separated timestamp 11 hours older than the cursor passed the raw-string "
        "comparison ('T' sorts above ' ') — the filter must compare instants, not strings"
    )


def test_list_response_timestamp_breach_raises_typed_response_invalid(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 3, Item 1 — the round-2 filter COERCED before
    comparing: ``str(None) == 'None'`` sorts above any '2...' date, so a null
    timestamp passed the filter and would be persisted as the cursor; an int
    sorted below and was silently dropped; a missing key raised a bare
    KeyError. A response value that is missing, non-string, or unparseable is
    a contract breach and must raise GarminError(kind='response_invalid') —
    never be sorted, coerced, or silently skipped (findings.md B-STAGE9-25
    round 3). Distinct from kind='cursor_invalid' by design: the cursor is our
    persisted state, the response is the server's."""
    library_activities: list[dict[str, Any]] = [
        {"activityId": "null-ts", "startTimeGMT": None},
        {"activityId": "missing-key"},  # startTimeGMT absent entirely
        {"activityId": "int-ts", "startTimeGMT": 1757671200},
        {"activityId": "garbage-ts", "startTimeGMT": "not-a-timestamp"},
    ]

    served: list[dict[str, Any]] = []  # one breach at a time, so each is individually exercised

    class _BreachGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return served

    _install_fake_gc(monkeypatch, _BreachGarmin)
    client = GarminClient("u@x.com", "p")

    for bad_record in library_activities:
        served[:] = [bad_record]
        with pytest.raises(GarminError) as excinfo:
            list(client.list_activities_since("2026-09-09 18:00:00"))
        assert excinfo.value.kind == "response_invalid", (
            f"a malformed response timestamp ({bad_record!r}) must classify as "
            f"kind='response_invalid', got {excinfo.value.kind!r}"
        )
        assert "startTimeGMT" in excinfo.value.message, "the message must name the offending field"


def test_list_missing_activity_id_raises_typed_response_invalid_naming_the_key(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-26 (DEC-055) — activityId is validated by the SAME typed
    contract as its sibling startTimeGMT (round 3, above). A record carrying a
    valid startTimeGMT but no activityId key is a library-contract breach and
    must raise GarminError(kind='response_invalid') naming 'activityId' — NOT
    a bare KeyError, which with_retry cannot classify (it is not a
    GarminError) and which classifyListException then folds to Unknown
    (PyEmbeddedAdapter.cpp:485-497), the exact unclassified fold B-STAGE9-25
    exists to abolish."""
    library_activities: list[dict[str, Any]] = [{"startTimeGMT": "2026-09-09 19:00:00"}]

    class _MissingIdGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _MissingIdGarmin)
    client = GarminClient("u@x.com", "p")

    with pytest.raises(GarminError) as excinfo:
        list(client.list_activities_since("2026-09-09 18:00:00"))
    assert (
        excinfo.value.kind == "response_invalid"
    ), f"a record missing activityId must classify as kind='response_invalid', got {excinfo.value.kind!r}"
    assert "activityId" in excinfo.value.message, "the message must name the missing key"


def test_list_marshals_start_time_local_when_present(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """T-210 / DEC-056 — the summary carries a third key, startTimeLocal, so
    GarminConnect.cpp can name listing entries from the activity's own LOCAL
    start time (peer-service house pattern, Strava.cpp:258) instead of the
    server-side startTimeGMT, which parseGarminTime stamps UTC."""
    library_activities: list[dict[str, Any]] = [
        {"activityId": 1, "startTimeGMT": "2026-07-01 06:30:00", "startTimeLocal": "2026-07-01 08:30:00"},
    ]

    class _LocalGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _LocalGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-06-30 00:00:00"))

    assert result[0]["startTimeLocal"] == "2026-07-01 08:30:00"


def test_list_missing_or_none_start_time_local_marshals_to_empty_string_not_error(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """T-210 / DEC-056 — the installed wheel declares startTimeLocal as
    `str | None` (garminconnect/typed.py:396-407): OPTIONAL. Absence or None
    must fall back to an empty string and the listing must still succeed —
    raising here would reproduce the exact blank-dialog defect DEC-056 exists
    to fix, one key over from the one DEC-055 just closed."""
    library_activities: list[dict[str, Any]] = [
        {"activityId": 1, "startTimeGMT": "2026-07-01 06:30:00"},  # key absent entirely
        {"activityId": 2, "startTimeGMT": "2026-07-02 06:30:00", "startTimeLocal": None},
    ]

    class _NoLocalGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _NoLocalGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-06-30 00:00:00"))

    assert result[0]["startTimeLocal"] == "", "missing startTimeLocal must marshal to empty, not raise"
    assert result[1]["startTimeLocal"] == "", "a None startTimeLocal must marshal to empty, not raise"


def test_list_iso_form_cursor_does_not_drop_space_form_newer_activities(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 3, Item 1, mirror case — a cursor previously persisted
    in ISO form ('2026-09-09T18:00:00') made the raw-string comparison drop
    every VALID space-form activity newer than it (' ' sorts below 'T'), a
    silent miss. Instant comparison must keep the newer space-form activity,
    still drop the older one, and still derive the one-day-early date-only
    query from the cursor."""
    captured: dict[str, Any] = {}
    library_activities = [
        {"activityId": "keep", "startTimeGMT": "2026-09-09 19:00:00"},  # newer than cursor — must survive
        {"activityId": "drop", "startTimeGMT": "2026-09-09 07:00:00"},  # older — must be dropped
    ]

    class _IsoCursorGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            captured["args"] = args
            return library_activities

    _install_fake_gc(monkeypatch, _IsoCursorGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-09-09T18:00:00"))

    assert [r["activityId"] for r in result] == ["keep"], (
        "a space-form activity newer than an ISO-form cursor was dropped by the raw-string "
        "comparison (' ' sorts below 'T') — the filter must compare instants, not strings"
    )
    forwarded = list(captured["args"])
    assert forwarded == [
        "2026-09-08"
    ], "the one-day-early date-only query must still be derived correctly from an ISO-form cursor"


def test_list_same_second_activities_at_the_cursor_survive_for_tier1_id_dedup(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    """B-STAGE9-25 round 3, Item 2 — there is no uniqueness invariant on
    startTimeGMT and the cursor carries no activityId tie-breaker, so a second,
    genuinely-new activity sharing the exact cursor second is PERMANENTLY lost
    under a strict '>' filter. '>=' keeps it reachable; the already-imported
    same-second activity is suppressed where identity lives — Tier-1 activityId
    dedup (GarminConnect.cpp:862-867) — not by a timestamp boundary (findings.md
    B-STAGE9-25 round 3)."""
    library_activities = [
        {"activityId": "already-imported", "startTimeGMT": "2026-09-09 18:00:00"},  # at cursor
        {"activityId": "new-sibling", "startTimeGMT": "2026-09-09 18:00:00"},  # at cursor, distinct id
        {"activityId": "older", "startTimeGMT": "2026-09-09 17:59:59"},  # before cursor — dropped
    ]

    class _SameSecondGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return library_activities

    _install_fake_gc(monkeypatch, _SameSecondGarmin)
    client = GarminClient("u@x.com", "p")

    result = list(client.list_activities_since("2026-09-09 18:00:00"))

    assert [r["activityId"] for r in result] == ["already-imported", "new-sibling"], (
        "both same-second activities must reach the caller — the adapter must not "
        "pre-drop the new sibling, and Tier-1 activityId dedup (not this filter) "
        "suppresses the already-imported one"
    )


def test_list_connection_error_maps_to_kind_connection(monkeypatch: pytest.MonkeyPatch) -> None:
    """A library connection error during listing must surface as GC-stable
    kind='connection' so DES-008 routes the "couldn't reach Garmin" copy — not a
    raw library class name. Classified by exception TYPE (LSN-006)."""

    class _ConnGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
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
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
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
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
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
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return []

    _install_fake_gc(monkeypatch, _EmptyGarmin)

    client = GarminClient("u@x.com", "p")
    assert list(client.list_activities_since("2026-06-30 00:00:00")) == []


def test_fake_get_activities_by_date_rejects_a_non_date_only_call_like_the_real_wheel() -> None:
    """B-STAGE9-25 Item 2 — the fake itself must reject what the real installed
    wheel rejects (garminconnect._validate_date_format, __init__.py:63-74),
    so a regression that goes back to forwarding a full datetime is caught by
    the suite instead of coexisting with a green run (LSN: verify against the
    artifact that actually runs, 5th instance)."""

    class _AnyGarmin(_FakeGarminBase):
        def _get_activities_by_date_impl(self, *args: Any, **kwargs: Any) -> Any:
            return []

    fake = _AnyGarmin("u@x.com", "p")
    with pytest.raises(ValueError, match="startdate"):
        fake.get_activities_by_date("2026-06-30 05:21:24")
    # Round 2 (findings.md B-STAGE9-25) — calendar reality, not just regex shape:
    # 2026 is not a leap year and February never has 30 days.
    with pytest.raises(ValueError):
        fake.get_activities_by_date("2026-02-30")
    with pytest.raises(ValueError):
        fake.get_activities_by_date("2024-02-30")
    fake.get_activities_by_date("2026-06-30")  # date-only is accepted
    fake.get_activities_by_date("2024-02-29")  # 2024 IS a leap year — accepted
