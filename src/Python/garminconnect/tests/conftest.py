"""Session-wide guard so gc_rate.py's real pacing (now
wired into garmin_client.py's list_activities_since/download_activity) never
adds real wall-clock delay to the rest of this suite. Existing adapter
tests were written before pacing landed and do not expect a sleep.
"""

from __future__ import annotations

import time
from collections.abc import Iterator

import pytest

from gc_garmin_adapter import gc_rate


@pytest.fixture(autouse=True)  # type: ignore[misc]
def _no_real_rate_limit_sleep(monkeypatch: pytest.MonkeyPatch) -> Iterator[None]:
    monkeypatch.setattr(gc_rate.time, "sleep", lambda _s: None)
    monkeypatch.setattr(time, "sleep", lambda _s: None)
    yield
