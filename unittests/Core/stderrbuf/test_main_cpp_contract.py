#!/usr/bin/env python3
"""B-STAGE9-12/27 -- source contract check over src/Core/main.cpp's nostderr().

nostderr_mirror.cpp is a hand-copied mirror, not a link against the real
function (main.cpp owns main()/QApplication and cannot be linked into a
test binary). Only a comment previously guarded against that mirror
drifting from main.cpp; this makes it an executable check instead.

Reads main.cpp's own text and asserts, independently:
  1. ORDER -- inside nostderr(), freopen -> fileno(stderr) -> dup2 ->
     sync_with_stdio appear in that relative order. Independent of the fix,
     so it survives the fix moving.
  2. EXACTLY ONE setvbuf call site -- a #ifdef WIN32/#else mode-selection
     pair counts as one site; any other setvbuf occurrence elsewhere in the
     function is a second site (B-STAGE9-27's original defect: an
     unconditional call plus a separate late WIN32-only call).
  3. PLACEMENT -- that one call site sits between the freopen NULL-check
     block and the fileno(stderr) call.
  4. MODE SELECTION -- the site is `#ifdef WIN32`-guarded, _IONBF on the
     WIN32 side, _IOLBF on the #else side (C11 7.21.5.6p2: setvbuf may be
     refused once stream I/O has occurred, so a single unconditional call
     placed before any I/O is required -- not two calls at two positions).
  5. RESULT CHECKED -- the call's return value is captured into a variable
     that is then tested in an `if`, not discarded.
"""

import re
import sys
from pathlib import Path

_MAIN_CPP = Path(__file__).resolve().parents[3] / "src" / "Core" / "main.cpp"

_SETVBUF_CALL_RE = re.compile(
    r"(?:(\w+)\s*=\s*)?setvbuf\s*\(\s*stderr\s*,[^;]*?,\s*(?:_IOLBF|_IONBF|_IOFBF)\s*,[^;]*\)"
)
_WIN32_ELSE_BLOCK_RE = re.compile(
    r"#ifdef\s+WIN32\b(?P<win>.*?)#else\b(?P<posix>.*?)#endif\b", re.DOTALL
)


def _extract_nostderr(text: str) -> str:
    start = text.index("void nostderr(")
    brace_start = text.index("{", start)
    depth = 0
    i = brace_start
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start : i + 1]
        i += 1
    raise AssertionError("unbalanced braces scanning nostderr()")


def main() -> int:
    if not _MAIN_CPP.exists():
        print(f"FAIL: {_MAIN_CPP} not found")
        return 2
    text = _MAIN_CPP.read_text()
    body = _extract_nostderr(text)

    failures = []

    order_names = ["freopen(", "fileno(stderr)", "dup2(", "sync_with_stdio("]
    positions = [body.find(name) for name in order_names]
    for name, pos in zip(order_names, positions):
        if pos < 0:
            failures.append(f"expected call {name!r} not found in nostderr()")
    if all(p >= 0 for p in positions) and positions != sorted(positions):
        failures.append(
            "nostderr() call order is not freopen->fileno->dup2->sync_with_stdio: "
            f"positions {list(zip(order_names, positions))}"
        )

    null_check_idx = body.find("== NULL")
    freopen_null_check_end = (
        body.find("}", null_check_idx) if null_check_idx >= 0 else -1
    )
    fileno_call = body.find("fileno(stderr)")

    win_block = None
    for m in _WIN32_ELSE_BLOCK_RE.finditer(body):
        if "setvbuf" in m.group(0):
            win_block = m
            break

    all_calls = list(_SETVBUF_CALL_RE.finditer(body))
    if win_block is not None:
        block_calls = [
            c for c in all_calls if win_block.start() <= c.start() < win_block.end()
        ]
        outside_calls = [
            c
            for c in all_calls
            if not (win_block.start() <= c.start() < win_block.end())
        ]
        num_sites = (1 if block_calls else 0) + len(outside_calls)
    else:
        block_calls = []
        outside_calls = all_calls
        num_sites = len(outside_calls)

    if num_sites != 1:
        failures.append(
            f"expected exactly one setvbuf call site in nostderr(), found {num_sites}"
        )

    site_calls = block_calls if block_calls else all_calls
    if not site_calls:
        failures.append("no setvbuf(stderr, ...) call found in nostderr()")
    elif freopen_null_check_end < 0 or fileno_call < 0:
        failures.append(
            "could not locate freopen NULL-check block or fileno(stderr) call to place setvbuf against"
        )
    else:
        site_start = (
            win_block.start() if win_block is not None else site_calls[0].start()
        )
        site_end = max(c.end() for c in site_calls)
        if not (freopen_null_check_end < site_start and site_end < fileno_call):
            failures.append(
                "setvbuf must sit between the freopen NULL-check block and the fileno(stderr) call"
            )

    if win_block is None:
        failures.append(
            "no #ifdef WIN32 / #else mode-selected setvbuf block found in nostderr()"
        )
    else:
        if not re.search(
            r"setvbuf\s*\(\s*stderr\s*,[^;]*_IONBF", win_block.group("win")
        ):
            failures.append(
                "the #ifdef WIN32 branch of the mode-selected setvbuf block must use _IONBF"
            )
        if not re.search(
            r"setvbuf\s*\(\s*stderr\s*,[^;]*_IOLBF", win_block.group("posix")
        ):
            failures.append(
                "the #else branch of the mode-selected setvbuf block must use _IOLBF"
            )

    checked_ok = False
    for c in site_calls:
        var = c.group(1)
        if not var:
            continue
        tail = body[c.end() : c.end() + 300]
        if re.search(rf"\bif\s*\(\s*{re.escape(var)}\b", tail):
            checked_ok = True
            break
    if site_calls and not checked_ok:
        failures.append(
            "setvbuf's return value must be captured and checked (e.g. via an `if`), not discarded"
        )

    if failures:
        for f in failures:
            print(f"FAIL: {f}")
        return 1
    print(
        "PASS: main.cpp nostderr() contract (order, exactly-one setvbuf, placement, WIN32/_IOLBF mode, result checked)"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
