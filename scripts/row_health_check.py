#!/usr/bin/env python3
"""
row_health_check.py — deterministic "row/line bloat" health check for this project's
governance ledgers.

Purpose (the LSN-034 anti-pattern): `references/lessons-memory.md`'s own design rule #6
says a `lessons.md` index row is a FIXED-SHAPE record — "Target <=~200 chars" — and that
when a lesson recurs, the dated narrative belongs in the row's COLD ENTRY below, never
appended in place. The rule was not followed for a while: LSN-034's own index row grew, one
recurrence at a time, to 16k+ characters, and that exact incident is the cautionary example
`lessons-memory.md` cites for why the rule exists. This script is the deterministic backstop
that would have flagged it as it grew, rather than only after the fact.

Two DIFFERENT shapes get two DIFFERENT checks — applying one rule blindly to both silently
misses one of them:

  * ID-INDEXED LEDGERS (lessons.md, findings.md, decisions.md, traceability.md) are one
    entry per physical line. Growth shows up as ONE LINE getting long — this project's real
    files confirm it: every offending line found so far is a single unwrapped physical line,
    while legitimate multi-line prose (this project's older `## LSN-NNN` cold entries) stays
    well under any of the caps below because it is hand-wrapped at ordinary paragraph width.
    Check: PER-LINE length against a per-file cap.

  * STATE.md is REWRITTEN WHOLESALE each pass, not indexed by id — there is no stable row to
    anchor a per-line finding to (VAL-011's own history shows the same paragraph relocating
    repeatedly across passes). It still needs a heath signal, but as a WHOLE-FILE one: total
    bytes, total lines, and the single longest line found anywhere in the file — reported and
    capped as file-level summary statistics, not as N separate per-line findings.

Threshold source (do not invent a parallel concept — reuse this project's own): STATE.md's
own `BUDGETS` line already measures per-file byte size for the hot files (WIKI.md/STATE.md/
lessons.md) with `wc -c`, updated pass over pass — see STATE.md's `## BUDGETS` block. This
script parses that line for an INFORMATIONAL cross-check (does the file's current size match
what STATE.md last recorded?) and, for the actual pass/fail caps, uses the DEFAULTS below,
each one sourced from either an ALREADY-DOCUMENTED project target (lessons.md: 200 chars,
`references/lessons-memory.md:29`) or a bootstrap figure derived from this project's own
measured percentiles where no cap has been ratified yet (findings.md/decisions.md/
traceability.md/STATE.md's whole-file caps). None of the bootstrap figures is an authority —
they are marked DEFAULT below and are meant to be promoted into STATE.md's own BUDGETS
concept (as an extra `ROWMAX <file> <n>` clause, or similar) by whoever next owns that
decision; until then, `--config` overrides them per project without editing this file.

Known blind spot (by design, not yet closed): a per-line/per-file check cannot see bloat that
is spread across MANY separate SHORT lines under one entry instead of concentrated in one
long line or one oversized file — e.g. a lesson's cold entry accreting fifty short dated
sub-entries never trips either check, because no single line or file crosses its cap. Closing
that would need a per-ENTRY (id-to-next-id span) aggregate check, which this script does not
attempt.

CLI:
    python3 scripts/row_health_check.py [--root DIR] [--config CONFIG.json] [--json]
    exit 0 = no findings
    exit 1 = at least one finding (one line per finding on stdout, or a JSON report with --json)
    exit 2 = usage / IO error

Read-only: this script never writes to any file it scans.

Importable API:
    check_id_indexed_file(path, max_chars) -> list[Finding]
    check_whole_file(path, max_bytes, max_line_chars) -> list[Finding]
    load_config(path) -> dict
    main(argv=None) -> int

Stdlib only; no third-party dependencies (matches sibling anti_duplication_guard.py /
ledger_drift_lint.py).
"""

import argparse
import collections
import json
import os
import re
import sys

# ============================ EXTENSIBLE CONSTANTS ============================
# Everything a maintainer might tune lives here, as plain data, or via --config (same shape).

# --- ID-indexed ledgers: PER-LINE cap, in characters (len(line), newline excluded — matches
#     `wc -c` minus 1 per line, and matches how this project's own lessons-memory.md phrases
#     its target). Paths are relative to --root (project root).
ID_INDEXED_LEDGERS = {
    "lessons.md": {
        "max_chars": 4000,
        "source": "TWO NUMBERS, not one, and they differ on purpose. This project's OWN "
                   "documented target (references/lessons-memory.md:29) is \"Target <=~200 "
                   "chars\" per index row — but empirically, EVERY one of this file's 85 "
                   "current compact-format rows already exceeds that (shortest: 101 chars; "
                   "next shortest that clears 200: 236) — the project has not been holding "
                   "itself to that aspiration for a long time, on any row, so using 200 as "
                   "this script's FLAG threshold would report ~all of them and catch nothing "
                   "specific. The 4000 used here is a DEFAULT bootstrap chosen from this "
                   "file's own measured gap (2026-09-05): 82 of 85 rows fall at or under "
                   "3811 chars, then there is a real jump to 5340/5840/16351 — the three "
                   "rows that actually show the LSN-034 shape (a recurrence narrative kept "
                   "in place instead of moved to a cold entry). 4000 separates that small "
                   "outlier set from the file's own current normal range without relying on "
                   "a target this file does not currently meet anywhere.",
    },
    ".claude/workflow-garminconnect/findings.md": {
        "max_chars": 8000,
        "source": "DEFAULT bootstrap, NOT yet ratified — this project has no documented "
                   "per-row cap for findings.md (its own convention caps COLUMN COUNT at "
                   "six, not row length); 8000 sits above this file's measured p99 (~6.4k "
                   "chars over 502 rows, 2026-09-05) so it flags only the most extreme "
                   "outliers, not this file's normal verbose-but-intended rows.",
    },
    ".claude/workflow-garminconnect/decisions.md": {
        "max_chars": 2500,
        "source": "DEFAULT bootstrap, NOT yet ratified — measured p99 ~920 chars, max "
                   "~1700 chars over 2170 lines (2026-09-05); 2500 leaves headroom above "
                   "the current max without being anywhere near lessons.md's 200-char "
                   "target, since decisions.md's full DEC entries are not under that rule.",
    },
    ".claude/workflow-garminconnect/traceability.md": {
        "max_chars": 4000,
        "source": "DEFAULT bootstrap, NOT yet ratified — measured p99 ~3.6k chars over 737 "
                   "lines (2026-09-05). NOTE: this file's own header text calls its TEST(s) "
                   "cell an intentionally 'accreted, dated slice narrative kept for "
                   "provenance... HISTORICAL EVIDENCE' — unlike lessons.md's index row, "
                   "growth here may be BY DESIGN, not a defect, so a finding against this "
                   "file is a prompt to check that distinction, not an assumed violation.",
    },
}

# --- STATE.md: WHOLE-FILE caps (not per-line). `max_bytes` bounds the file's total size the
#     same way STATE.md's own BUDGETS line already measures it (`wc -c`); `max_line_chars`
#     bounds the single longest physical line found anywhere in the file, reported as ONE
#     whole-file finding rather than one per offending line, because STATE.md has no stable
#     per-row id to anchor a per-line finding to.
WHOLE_FILE_LEDGERS = {
    "STATE.md": {
        "max_bytes": 150000,
        "max_line_chars": 5000,
        "source": "DEFAULT bootstrap, NOT yet ratified — STATE.md's own BUDGETS line "
                   "measures size pass over pass (e.g. \"STATE.md 69,127 bytes\") but has "
                   "never recorded a cap, only raw sizes; max_bytes sits above every size "
                   "this project's STATE.md has recorded so far (max seen: ~108KB, "
                   "2026-09-05). max_line_chars=5000 sits well above this file's own "
                   "measured p99 (~1.1k chars over 800 lines) and well below its actual "
                   "outlier (9,924 chars), so it separates ordinary long paragraphs from "
                   "the LSN-034-shaped anti-pattern without needing a per-row id.",
    },
}

_BUDGETS_LINE_RE = re.compile(r"^BUDGETS\b.*(?:\n[ \t].*)*", re.MULTILINE)
_BUDGETS_FILE_SIZE_RE = re.compile(
    r"\*\*(?P<file>[\w.-]+\.md)\s+(?P<bytes>[\d,]+)\s+bytes\*\*"
)

Finding = collections.namedtuple(
    "Finding", ["path", "line", "kind", "length", "cap", "snippet"]
)


def _read_lines(path):
    with open(path, "r", encoding="utf-8", errors="surrogateescape") as fh:
        return fh.readlines()


def _snippet(line, width=100):
    s = line.rstrip("\n")
    return s if len(s) <= width else s[:width] + "…"


def _byte_len(line):
    """Byte length of a line (newline excluded), matching this project's own `wc -c`
    convention (STATE.md's BUDGETS line) rather than a Unicode-codepoint count — the two
    diverge on every multi-byte character (em dashes, curly quotes) this project's prose
    uses constantly, so a codepoint count would under-report against every cap already
    calibrated, in this project's own history, against `wc -c`."""
    return len(line.rstrip("\n").encode("utf-8", errors="surrogateescape"))


def check_id_indexed_file(path, max_chars):
    """PER-LINE check: one Finding per physical line whose BYTE length (newline excluded,
    `wc -c` convention) exceeds max_chars. Returns [] if the file is well within cap on
    every line."""
    findings = []
    for i, line in enumerate(_read_lines(path), start=1):
        length = _byte_len(line)
        if length > max_chars:
            findings.append(
                Finding(path, i, "id-indexed-row-too-long", length, max_chars, _snippet(line))
            )
    return findings


def check_whole_file(path, max_bytes, max_line_chars):
    """WHOLE-FILE check: at most two Findings — one if total byte size exceeds max_bytes,
    one if the single longest line exceeds max_line_chars (reported once, at that line,
    not once per offending line — this file has no stable per-row id to key on)."""
    findings = []
    size = os.path.getsize(path)
    if size > max_bytes:
        findings.append(Finding(path, None, "whole-file-too-large", size, max_bytes, ""))
    lines = _read_lines(path)
    if lines:
        lengths = [_byte_len(l) for l in lines]
        longest = max(lengths)
        if longest > max_line_chars:
            lineno = lengths.index(longest) + 1
            findings.append(
                Finding(
                    path, lineno, "whole-file-longest-line-too-long",
                    longest, max_line_chars, _snippet(lines[lineno - 1]),
                )
            )
    return findings


def budgets_crosscheck(root):
    """Informational only (never affects exit code): parse STATE.md's own most recent
    BUDGETS line and compare its last-recorded per-file byte counts against the files'
    CURRENT sizes on disk, so drift since that line was written is visible."""
    state_path = os.path.join(root, "STATE.md")
    if not os.path.isfile(state_path):
        return []
    with open(state_path, "r", encoding="utf-8", errors="surrogateescape") as fh:
        text = fh.read()
    lines_with_budgets = [m.group(0) for m in _BUDGETS_LINE_RE.finditer(text)]
    if not lines_with_budgets:
        return []
    last = lines_with_budgets[-1]
    out = []
    for m in _BUDGETS_FILE_SIZE_RE.finditer(last):
        fname = m.group("file")
        recorded = int(m.group("bytes").replace(",", ""))
        fpath = os.path.join(root, fname)
        if os.path.isfile(fpath):
            current = os.path.getsize(fpath)
            out.append((fname, recorded, current, current - recorded))
    return out


def load_config(path):
    with open(path, "r", encoding="utf-8") as fh:
        return json.load(fh)


def _fmt_finding(f):
    if f.line is None:
        return (f"{f.path}: {f.kind} — {f.length:,} bytes > cap {f.cap:,}")
    return (
        f"{f.path}:{f.line}: {f.kind} — {f.length:,} chars > cap {f.cap:,} :: {f.snippet}"
    )


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--root", default=".", help="project root (default: cwd)")
    ap.add_argument("--config", default=None, help="JSON overriding the built-in caps")
    ap.add_argument("--json", action="store_true", help="machine-readable report")
    args = ap.parse_args(argv)

    root = os.path.abspath(args.root)
    id_indexed = dict(ID_INDEXED_LEDGERS)
    whole_file = dict(WHOLE_FILE_LEDGERS)
    if args.config:
        try:
            cfg = load_config(args.config)
        except (OSError, json.JSONDecodeError) as e:
            print(f"row_health_check.py: bad --config: {e}", file=sys.stderr)
            return 2
        id_indexed.update(cfg.get("id_indexed_ledgers", {}))
        whole_file.update(cfg.get("whole_file_ledgers", {}))

    all_findings = []
    try:
        for rel, spec in id_indexed.items():
            fpath = os.path.join(root, rel)
            if not os.path.isfile(fpath):
                continue
            all_findings.extend(check_id_indexed_file(fpath, spec["max_chars"]))
        for rel, spec in whole_file.items():
            fpath = os.path.join(root, rel)
            if not os.path.isfile(fpath):
                continue
            all_findings.extend(
                check_whole_file(fpath, spec["max_bytes"], spec["max_line_chars"])
            )
    except OSError as e:
        print(f"row_health_check.py: IO error: {e}", file=sys.stderr)
        return 2

    crosscheck = budgets_crosscheck(root)

    if args.json:
        print(json.dumps({
            "findings": [f._asdict() for f in all_findings],
            "budgets_crosscheck": [
                {"file": f, "recorded_bytes": r, "current_bytes": c, "drift_bytes": d}
                for (f, r, c, d) in crosscheck
            ],
        }, indent=2))
    else:
        print(f"row_health_check — {len(all_findings)} finding(s)")
        for f in all_findings:
            print("  " + _fmt_finding(f))
        if crosscheck:
            print("\nSTATE.md BUDGETS cross-check (informational only, not gated):")
            for (fname, recorded, current, drift) in crosscheck:
                sign = "+" if drift >= 0 else ""
                print(f"  {fname}: STATE.md last recorded {recorded:,} bytes, "
                      f"currently {current:,} bytes ({sign}{drift:,})")

    return 1 if all_findings else 0


if __name__ == "__main__":
    sys.exit(main())
