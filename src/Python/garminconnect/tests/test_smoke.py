"""Phase 2.1 bootstrap smoke test for DEC-008 (pytest + coverage.py).

Proves pytest can collect and run a test in this directory. Phase 2.2 replaces
this with real tests for gc_rate.py (DES-005) and garmin_client.py (DES-012).
"""


def test_toolchain_alive() -> None:
    assert len("garmin") == 6
