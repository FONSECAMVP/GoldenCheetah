"""Activity download via python-garminconnect (adapter side).

Acceptance: "GC calls download_activity(activity_id,
fmt='ORIGINAL'->'FIT') for each new activity. ... TCX fallback only if FIT is
not available for that activity."

Scope (the adapter, Python side only):
  - download_activity forwards the activity id verbatim and maps the GC-stable
    `fmt` string ('ORIGINAL' | 'TCX') to the library's ActivityDownloadFormat
    enum, returning the library's bytes unchanged.
  - download-time library errors are translated to GC-stable GarminError kinds
    ('connection', 'rate_limit') so callers can switch on .kind without knowing
    the library's exception class names.
  - a non-library exception is NOT misclassified (as on the login
    path): it propagates unchanged rather than inheriting a Garmin kind.
  - an unsupported `fmt` is rejected deterministically before any library call
    (never send garbage to the library).

Explicitly NOT covered here:
  - The FIT->TCX *fallback orchestration* — that lives in the C++
    GarminConnect::readFile, which calls this adapter once per format.
    Here download_activity is a thin, single-format fetch (`-> bytes`).
  - Bytes marshalling across the embedded-CPython seam (PyEmbeddedAdapter) —
    that is the C++ side of the seam.
"""

from __future__ import annotations

import types
from typing import Any

import pytest

from gc_garmin_adapter.garmin_client import GarminClient, GarminError


class _FakeConnErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectConnectionError."""


class _FakeRateErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectTooManyRequestsError."""


class _FakeAuthErr(Exception):
    """Stand-in for garminconnect.exceptions.GarminConnectAuthenticationError."""


class _FmtEnum:
    """Mirror of garminconnect.Garmin.ActivityDownloadFormat (identity values;
    the adapter must forward whichever enum member it maps `fmt` to)."""

    ORIGINAL = "download-format-original"
    TCX = "download-format-tcx"


class _FakeGarminBase:
    """Minimal fake of garminconnect.Garmin exposing only what the download
    path touches: the ActivityDownloadFormat enum + download_activity()."""

    ActivityDownloadFormat = _FmtEnum

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # AUTH-ONLY construction — no tokenstore path.
        # **_ swallows return_on_mfa=True (unused on this path).
        self.email = email
        self.password = password

    def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
        raise NotImplementedError  # overridden per test


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake module exposing the surface the
    adapter download path touches: the Garmin class + the three download-
    relevant exception types."""
    import gc_garmin_adapter.garmin_client as garmin_client

    fake_mod = types.ModuleType("garminconnect_fake")
    fake_mod.Garmin = garmin_cls  # type: ignore[attr-defined]
    fake_mod.exceptions = types.SimpleNamespace(  # type: ignore[attr-defined]
        GarminConnectAuthenticationError=_FakeAuthErr,
        GarminConnectConnectionError=_FakeConnErr,
        GarminConnectTooManyRequestsError=_FakeRateErr,
    )
    monkeypatch.setattr(garmin_client, "_gc", fake_mod)


def test_download_original_forwards_id_and_returns_bytes(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Acceptance — a FIT (ORIGINAL) download forwards the activity id
    verbatim, requests the ORIGINAL format, and returns the library's bytes
    unchanged (the C++ side stages them as garmin-<id>.fit)."""
    captured: dict[str, Any] = {}
    payload = b"\x0e\x10FIT-bytes\x00\xff"

    class _OkGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            captured["activity_id"] = activity_id
            captured["dl_fmt"] = dl_fmt
            return payload

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("u@x.com", "p")
    data = client.download_activity("987654321")

    assert data == payload, "adapter must return the library's bytes verbatim (no re-encoding)"
    assert captured["activity_id"] == "987654321", "activity id must reach the library unchanged"
    assert (
        captured["dl_fmt"] == _FmtEnum.ORIGINAL
    ), "default fmt 'ORIGINAL' must map to the library's ActivityDownloadFormat.ORIGINAL (FIT default)"


def test_download_tcx_maps_to_tcx_format(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """TCX fallback path — fmt='TCX' must select the library's TCX
    enum member (so the C++ readFile fallback actually fetches TCX, not FIT
    again). A change that ignores `fmt` and always sends ORIGINAL fails this."""
    captured: dict[str, Any] = {}

    class _OkGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            captured["dl_fmt"] = dl_fmt
            return b"<TrainingCenterDatabase/>"

    _install_fake_gc(monkeypatch, _OkGarmin)

    client = GarminClient("u@x.com", "p")
    data = client.download_activity("111", fmt="TCX")

    assert data == b"<TrainingCenterDatabase/>"
    assert captured["dl_fmt"] == _FmtEnum.TCX, "fmt='TCX' must map to ActivityDownloadFormat.TCX, not ORIGINAL"


def test_download_connection_error_maps_to_kind_connection(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """A library connection error must surface as GC-stable kind='connection'
    so the UI routes the "couldn't reach Garmin" copy — not a raw class name."""

    class _ConnGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            raise _FakeConnErr("stub: connection refused")

    _install_fake_gc(monkeypatch, _ConnGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.download_activity("111")

    assert excinfo.value.kind == "connection"
    assert excinfo.value.message, "message must be non-empty for UI display"
    assert excinfo.value.original is not None, "original library exception must be retained for diagnostics"


def test_download_rate_limit_error_maps_to_kind_rate_limit(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Garmin 429 during download must surface as kind='rate_limit'
    (driving the rate-limit message), never as a generic/auth error."""

    class _RateGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            raise _FakeRateErr("stub: 429 too many requests")

    _install_fake_gc(monkeypatch, _RateGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.download_activity("111")

    assert excinfo.value.kind == "rate_limit"
    assert excinfo.value.original is not None


def test_download_non_library_exception_is_not_misclassified(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """Like the login case, a foreign exception (RuntimeError)
    from the library must NOT be caught-and-relabelled with a Garmin kind. It
    propagates unchanged so a downstream bug is never displayed as a network or
    auth failure."""

    class _BoomGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            raise RuntimeError("simulated downstream library bug")

    _install_fake_gc(monkeypatch, _BoomGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(RuntimeError, match="simulated downstream library bug"):
        client.download_activity("111")


def test_download_unsupported_fmt_rejected_before_library_call(tmp_path: Any, monkeypatch: pytest.MonkeyPatch) -> None:
    """An unsupported `fmt` must be rejected deterministically BEFORE any
    library call — never silently forwarded as a garbage dl_fmt. Guards the
    format-map from a KeyError leaking as an opaque crash and proves no network
    call is attempted for a bad format."""
    called = {"download": False}

    class _TrackGarmin(_FakeGarminBase):
        def download_activity(self, activity_id: str, dl_fmt: Any = None) -> bytes:
            called["download"] = True
            return b""

    _install_fake_gc(monkeypatch, _TrackGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.download_activity("111", fmt="GPX")  # not offered (FIT + TCX only)

    assert excinfo.value.kind == "unknown"
    assert not called["download"], "no library download must be attempted for an unsupported format"
