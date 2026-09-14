"""TEST — REQ-013 (DEC-050 first slice): profile auto-fill, adapter side.

PRD acceptance (REQ-013 / US-5): "Optional profile auto-fill (opt-in, fills
only missing Athlete fields)."

Scope of THIS slice (DEC-050): `get_profile()` returns only
`{dob, weight_kg, height_cm}`. `hr_max`/`ftp_w` are explicitly DEFERRED (see
DES-011's "Scope" paragraph and DEC-050) — this file does not exercise them,
and a future slice adding them should not need to touch these tests.

Unverified-schema risk (DEC-050): the real `garminconnect==0.3.13` wheel's
`get_userprofile_settings()` (`/userprofile-service/userprofile/settings`) is
untyped (no `typed.py` model) and this project has never tested against a
live Garmin account. The adapter therefore tries a short list of plausible
key-name candidates per field and silently omits anything absent, malformed,
or implausible — the single most important behaviour under test here is that
a response missing all three fields still yields a clean, fully-empty
Success outcome (not an error), since REQ-013's own acceptance criterion
already treats "Garmin didn't have this field" as normal.

Cites: REQ-013, DEC-050, DES-011, DES-012 (adapter as the single
library-knowledge seam), LSN-006 (classify by exception type, never
message text).
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
    """Minimal fake of garminconnect.Garmin exposing only what the profile
    path touches: get_userprofile_settings()."""

    def __init__(self, email: str, password: str, **_: Any) -> None:
        # **_ swallows B-STAGE9-10's return_on_mfa=True (unused on this path).
        self.email = email
        self.password = password

    def get_userprofile_settings(self) -> dict[str, Any]:
        raise NotImplementedError  # overridden per test


def _install_fake_gc(monkeypatch: pytest.MonkeyPatch, garmin_cls: type) -> None:
    """Replace garmin_client._gc with a fake module exposing the surface the
    profile path touches: the Garmin class + the two profile-relevant
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


def test_profile_missing_all_fields_yields_clean_empty_dict(monkeypatch: pytest.MonkeyPatch) -> None:
    """The single most important test in this slice (DEC-050): a response
    missing dob/weight/height entirely must NOT raise or crash — it must
    yield a plain, fully-empty dict. REQ-013's acceptance already treats an
    absent Garmin field as a normal no-op, not an error."""

    class _EmptyGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"someUnrelatedKey": "whatever"}

    _install_fake_gc(monkeypatch, _EmptyGarmin)

    client = GarminClient("u@x.com", "p")
    profile = client.get_profile()

    assert profile == {}, "a response with none of the 3 fields must yield a clean empty dict, not an error"


def test_profile_empty_response_dict_yields_clean_empty_dict(monkeypatch: pytest.MonkeyPatch) -> None:
    """An entirely empty {} response (plausible if Garmin returns a bare
    object for a brand-new account) must degrade the same way as one with
    unrelated keys — no crash, no KeyError."""

    class _BareGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {}

    _install_fake_gc(monkeypatch, _BareGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {}


def test_profile_non_dict_response_yields_clean_empty_dict(monkeypatch: pytest.MonkeyPatch) -> None:
    """If the library ever returns something other than a dict (e.g. None on
    some unexpected path), the adapter must not crash trying to .get() it."""

    class _WeirdGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> Any:
            return None

    _install_fake_gc(monkeypatch, _WeirdGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {}


def test_profile_finds_dob_via_birthdate_key(monkeypatch: pytest.MonkeyPatch) -> None:
    """A plausible 'birthDate' key with a valid ISO date must be picked up."""

    class _DobGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"birthDate": "1985-06-15"}

    _install_fake_gc(monkeypatch, _DobGarmin)

    client = GarminClient("u@x.com", "p")
    profile = client.get_profile()

    assert profile == {"dob": "1985-06-15"}


def test_profile_finds_dob_via_dateofbirth_fallback_key(monkeypatch: pytest.MonkeyPatch) -> None:
    """If 'birthDate' is absent, the second candidate key 'dateOfBirth' must
    still be tried (DEC-050's unverified-schema hedge)."""

    class _DobGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"dateOfBirth": "1990-01-02T00:00:00.0"}

    _install_fake_gc(monkeypatch, _DobGarmin)

    client = GarminClient("u@x.com", "p")
    profile = client.get_profile()

    assert profile == {"dob": "1990-01-02"}, "a datetime-shaped value must be truncated to the date portion"


def test_profile_malformed_dob_is_silently_omitted(monkeypatch: pytest.MonkeyPatch) -> None:
    """A 'birthDate' value that does not parse as an ISO date must be
    silently dropped rather than raising or filling garbage."""

    class _BadDobGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"birthDate": "not-a-date"}

    _install_fake_gc(monkeypatch, _BadDobGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {}


def test_profile_finds_weight_already_in_kilograms(monkeypatch: pytest.MonkeyPatch) -> None:
    """A plausible-magnitude 'weight' value is taken as-is (kilograms)."""

    class _WeightGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"weight": 72.5}

    _install_fake_gc(monkeypatch, _WeightGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {"weight_kg": 72.5}


def test_profile_converts_weight_reported_in_grams(monkeypatch: pytest.MonkeyPatch) -> None:
    """Garmin has been observed to report weight in grams on some endpoints;
    an implausibly large 'weight' value must be converted (DEC-050's
    magnitude sanity check), not filled as a 72500 kg athlete."""

    class _GramsGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"weight": 72500}

    _install_fake_gc(monkeypatch, _GramsGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {"weight_kg": 72.5}


def test_profile_implausible_weight_is_silently_omitted(monkeypatch: pytest.MonkeyPatch) -> None:
    """A weight value that is not a sane human weight even after the
    grams-conversion attempt (e.g. 0, negative, or absurdly large) must be
    dropped rather than filling nonsense into the Athlete profile."""

    class _ZeroWeightGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"weight": 0}

    _install_fake_gc(monkeypatch, _ZeroWeightGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {}


def test_profile_finds_height_in_centimeters(monkeypatch: pytest.MonkeyPatch) -> None:
    class _HeightGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"height": 178.0}

    _install_fake_gc(monkeypatch, _HeightGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {"height_cm": 178.0}


def test_profile_implausible_height_is_silently_omitted(monkeypatch: pytest.MonkeyPatch) -> None:
    class _BadHeightGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"height": 4}  # nonsense for a human, even in cm

    _install_fake_gc(monkeypatch, _BadHeightGarmin)

    client = GarminClient("u@x.com", "p")
    assert client.get_profile() == {}


def test_profile_finds_all_three_fields_together(monkeypatch: pytest.MonkeyPatch) -> None:
    """A response with all three fields present and sane must return all
    three — the multi-field positive case, sibling to the missing-all case
    above."""

    class _FullGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {"birthDate": "1988-03-04", "weight": 68.2, "height": 165.0}

    _install_fake_gc(monkeypatch, _FullGarmin)

    client = GarminClient("u@x.com", "p")
    profile = client.get_profile()

    assert profile == {"dob": "1988-03-04", "weight_kg": 68.2, "height_cm": 165.0}


def test_profile_hr_max_and_ftp_are_never_returned(monkeypatch: pytest.MonkeyPatch) -> None:
    """DEC-050 scope guard: even if the raw response happens to contain
    hr_max/ftp-shaped keys, this first slice must never surface them — they
    are explicitly deferred to a future REQ/DEC (Zones/CP system design)."""

    class _WithExtraGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            return {
                "birthDate": "1988-03-04",
                "weight": 68.2,
                "height": 165.0,
                "vo2Max": 55,
                "maxHr": 190,
                "ftp": 250,
            }

    _install_fake_gc(monkeypatch, _WithExtraGarmin)

    client = GarminClient("u@x.com", "p")
    profile = client.get_profile()

    assert set(profile.keys()) == {
        "dob",
        "weight_kg",
        "height_cm",
    }, "DEC-050: only dob/weight_kg/height_cm may appear in this slice's return value"


def test_profile_connection_error_maps_to_kind_connection(monkeypatch: pytest.MonkeyPatch) -> None:
    """A real transport failure (not a schema surprise) must still surface as
    GC-stable kind='connection' — the defensive parsing above is for the
    *response shape*, not for genuine network/auth failures."""

    class _ConnGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            raise _FakeConnErr("stub: connection refused")

    _install_fake_gc(monkeypatch, _ConnGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.get_profile()

    assert excinfo.value.kind == "connection"
    assert excinfo.value.original is not None


def test_profile_rate_limit_error_maps_to_kind_rate_limit(monkeypatch: pytest.MonkeyPatch) -> None:
    class _RateGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            raise _FakeRateErr("stub: 429 too many requests")

    _install_fake_gc(monkeypatch, _RateGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(GarminError) as excinfo:
        client.get_profile()

    assert excinfo.value.kind == "rate_limit"
    assert excinfo.value.original is not None


def test_profile_non_library_exception_is_not_misclassified(monkeypatch: pytest.MonkeyPatch) -> None:
    """LSN-006 parallel to the download/list paths — a foreign exception must
    propagate unchanged rather than inheriting a Garmin kind."""

    class _BoomGarmin(_FakeGarminBase):
        def get_userprofile_settings(self) -> dict[str, Any]:
            raise RuntimeError("simulated downstream library bug")

    _install_fake_gc(monkeypatch, _BoomGarmin)

    client = GarminClient("u@x.com", "p")
    with pytest.raises(RuntimeError, match="simulated downstream library bug"):
        client.get_profile()
