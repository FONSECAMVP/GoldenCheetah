"""B-STAGE9-09 — DEC-014 OQ1 close: fake-fidelity contract test.

The dev `.venv` this suite normally runs under does NOT have the real
`python-garminconnect` wheel installed (DES-007/Pkg bundles it only for
production); every other adapter test in this directory therefore runs
against a hand-written FAKE `_gc` module. A B-STAGE9-08 live-account
diagnostic caught the fake drifting from the real library: the fakes DEFINED
a `full_name_id` attribute the real, installed library never had, so
`garmin_client.py` read it, got a bare `builtins.AttributeError` on a live
account, and A3-REQ-002's mutation testing (M8/M9) "killed" the very mutants
that would have exposed the drift — because the fakes agreed with the bug.

This test closes that hole a different way: instead of another fake, it
imports the REAL `garminconnect` package (when present — see
`_import_real_garminconnect` below) and asserts every attribute
`garmin_client.py` reads directly off a `garminconnect.Garmin` instance
during login/MFA identity resolution actually exists on it. It is
intentionally scoped to that surface (not the whole adapter) —
`dumps()`/`loads()`/`resume_login()`'s exact argument order are separate,
already-flagged DEC-014 OQ1 residuals with their own findings, not fixed by
this atomic unit.

Skips cleanly (not fails) wherever the real wheel is absent, so CI without it
stays green; it runs for real wherever the wheel IS installed (this project's
system Python3 — the same interpreter `testGarminCurlCffiInterposition`
already uses for its real-library regression, per its CMakeLists.txt
comment).

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

Cites: DEC-014 OQ1, LSN-006 (classify by type, not message), DES-012.
"""

from __future__ import annotations

import importlib
import os
import sys
from types import ModuleType

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


def test_adapter_identity_surface_matches_real_garminconnect_library() -> None:
    """DEC-014 OQ1 / B-STAGE9-08 regression guard.

    Construct the REAL `garminconnect.Garmin` (the constructor only stores
    credentials — no network call) and assert every attribute name the
    adapter's login/MFA identity resolution reads actually exists on it. A
    missing attribute here means garmin_client.py (or a fake in
    tests/test_adapter_*.py) has drifted from the real library's surface —
    the exact class of bug that put `full_name_id` (never a real attribute)
    into production and slipped past 49/49 fake-backed adapter tests.
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
