#!/usr/bin/env python3
"""B-STAGE9-12 RED test for the stderr-buffering defect behind
REQ-NF-Obs-001's gc_obs traces (see src/Core/main.cpp's nostderr(), lines
~173-214, and GarminConnect.cpp's gcObsTrace(), lines ~71-80).

Drives nostderr_mirror (a faithful POSIX mirror of nostderr()'s current,
unfixed freopen sequence) and asserts the two live symptoms reported from a
real Garmin sync session:

  1. liveness -- a gc_obs-shaped line must be visible in the redirected log
     file before the writing process exits, not only after, on stdio's
     atexit flush. Without this, a still-running app's log looks like a
     code path never executed when it in fact did.
  2. ordering  -- lines must appear in the file in the order they were
     chronologically written, even when interleaved with a raw write(2)
     that bypasses the C stdio buffer, as the embedded Python adapter does
     writing directly to fd 2. Without this, the log cannot be read as a
     timeline.

Requires the setvbuf(stderr, nullptr, _IOLBF, 0) fix immediately after
nostderr()'s freopen, mirrored in lockstep by nostderr_mirror.cpp;
test_main_cpp_contract.py pins that main.cpp itself carries the fix at the
right place so this test cannot go green against a mirror that drifted from
an unfixed production function.
"""

import subprocess
import sys
import tempfile
import time
from pathlib import Path


def run_liveness(mirror: Path, tmp: Path) -> bool:
    log = tmp / "liveness.log"
    proc = subprocess.Popen([str(mirror), str(log)])
    deadline = (
        time.monotonic() + 1.5
    )  # shorter than the mirror's sleep(2) before its 2nd write/exit
    mid_content = ""
    while time.monotonic() < deadline:
        if log.exists():
            mid_content = log.read_text()
            if "duration_ms=12" in mid_content:
                break
        time.sleep(0.05)

    exit_code = proc.wait(timeout=10)
    if exit_code != 0:
        print(
            f"FAIL liveness: nostderr_mirror exited with code {exit_code}, expected 0"
        )
        return False

    if "duration_ms=12" not in mid_content:
        print(
            f"FAIL liveness: gc_obs line not visible before process exit (mid-run content: {mid_content!r})"
        )
        return False
    print("PASS liveness")
    return True


_KNOWN_ORDER_LINES = (
    "A qt-startup-line",
    "B python-429-warning (direct fd2 write)",
    "C gc_obs op=auth outcome=fail error_code=unknown duration_ms=12",
)


def run_ordering(mirror: Path, tmp: Path) -> bool:
    log = tmp / "ordering.log"
    try:
        subprocess.run([str(mirror), str(log), "interleave"], check=True, timeout=10)
    except subprocess.CalledProcessError as exc:
        print(f"FAIL ordering: nostderr_mirror exited with code {exc.returncode}")
        return False

    if not log.exists():
        print(f"FAIL ordering: expected log file {log} was never created")
        return False

    lines = log.read_text().splitlines()
    order = [
        line[0]
        for line in lines
        if any(line.startswith(known) for known in _KNOWN_ORDER_LINES)
    ]
    if order != ["A", "B", "C"]:
        print(f"FAIL ordering: expected chronological [A, B, C], got {order}")
        return False
    print("PASS ordering")
    return True


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: test_stderr_buffering.py <path-to-nostderr_mirror>")
        return 2
    mirror = Path(sys.argv[1])
    if not mirror.exists():
        print(f"nostderr_mirror binary not found at {mirror}")
        return 2

    with tempfile.TemporaryDirectory() as d:
        tmp = Path(d)
        liveness_ok = run_liveness(mirror, tmp)
        ordering_ok = run_ordering(mirror, tmp)

    return 0 if (liveness_ok and ordering_ok) else 1


if __name__ == "__main__":
    sys.exit(main())
