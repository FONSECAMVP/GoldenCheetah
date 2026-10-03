"""Bootstrap smoke test (pytest + coverage.py).

Proves pytest can collect and run a test in this directory.
"""


def test_toolchain_alive() -> None:
    assert len("garmin") == 6
