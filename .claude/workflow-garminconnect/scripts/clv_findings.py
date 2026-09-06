#!/usr/bin/env python3
"""CLV Check 5 — THE ONE CANONICAL EXECUTABLE IMPLEMENTATION.

Before this file existed the mechanism lived TWICE: as an awk one-liner pasted into
`findings.md` under "CLV Check 5 — the mechanism", and as a second, hand-re-pasted
copy inside `clv-lite.sh`. Two copies of an algorithm are two algorithms, and the
header of the shell copy had to carry a note begging the next editor to re-sync
them. `findings.md` now DESCRIBES this file and `clv-lite.sh` RUNS it. Neither
contains an algorithm any more.

WHAT THE OLD MECHANISM GOT WRONG — all four are fail-OPEN, i.e. every one of them
could report a clean gate over a real blocker:

  1. A ROW IT COULD NOT PARSE WAS "REPORTED, NOT BLOCKING". 28 rows split into the
     wrong number of cells; the mechanism counted them as NEEDS-CLASSIFICATION and
     let the overall check PASS. Several of those rows literally begin
     `**BLOCKING — …**`. An unreadable row is an UNKNOWN row, and an unknown row
     must fail the gate, not decorate it.
  2. A ROW WHOSE SEVERITY CELL WAS PROSE WAS SILENTLY SKIPPED. The old predicate
     was `if severity does not start with "blocking": next`. A severity cell
     reading "The three completion slots' TAIL processEvents() …" does not start
     with "blocking", so the row vanished from the check entirely. That is not a
     judgement that the row is harmless; it is the absence of a judgement.
  3. A ROW WHOSE DISPOSITION WAS PROSE WAS TREATED AS OPEN *or* CLOSED BY ACCIDENT,
     depending only on which word it happened to start with.
  4. EFFECT CLASSIFICATION WAS CONFLATED WITH DISPOSITION. `BLOCKS: {RELEASE}` says
     WHAT a finding blocks. `fixed` / `deferred` / `accepted` says WHETHER anyone
     still owes work on it. They are independent axes, and writing an effect set
     onto a row closes NOTHING. STATE.md's previous NEXT_GATE was built on the
     opposite belief and was therefore not satisfiable by its own actions.

THE FIVE CONCEPTS, KEPT SEPARATE ON PURPOSE
  malformed row .... the row cannot be read at all -> verdict impossible -> FAIL
  severity ......... how bad it is                 -> unreadable -> FAIL
  disposition ...... whether work is still owed    -> unreadable -> FAIL
  effect set ....... WHAT it blocks (`BLOCKS: {…}`) -> an independent axis
  outstanding ...... blocking severity AND no closing disposition -> FAIL

`OUTSTANDING` is computed from severity + disposition ONLY. Adding, removing or
editing a `BLOCKS: {…}` set cannot move a row into or out of `OUTSTANDING`. That is
deliberate and it is the whole point: a gate whose actions cannot change its own
pass criteria is not a gate.

EXIT CODES
  0  every row readable, every severity and disposition in vocabulary, nothing
     outstanding, every outstanding row carrying an effect set (vacuously true)
  1  at least one blocking bucket is non-empty
  2  a required input is missing or unreadable  (fail-safe: never 0)
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys

# ── The controlled vocabularies ──────────────────────────────────────────────
# Deliberately SMALL. A token outside these sets is not silently tolerated: it is
# reported as unreadable and it fails the gate. Widening either set is a decision
# with consequences, so it has to be made here, in one place, on purpose.

# Severities that mean "this still blocks something until dispositioned".
# `was-blocking` stays in the blocking family: a row that WAS blocking still needs
# a closing disposition to prove it stopped being so.
BLOCKING_SEVERITIES = ("blocking", "was-blocking")

# Severities that are legible and are NOT blocking.
NON_BLOCKING_SEVERITIES = ("non-blocking", "informational", "advisory", "cosmetic", "refuted")

# Severity that means "nobody has decided how bad this is yet". Explicitly NOT the
# same as a missing severity, and explicitly NOT harmless: an undetermined severity
# is an undetermined gate, so it FAILS. It was invented (2026-08-23, B-R028-17) so
# that declining to classify would be VISIBLE; making it non-blocking would undo
# exactly the thing it was invented for.
UNDECIDED_SEVERITY = "needs-disposition"

# Dispositions that CLOSE a finding: no further work is owed.
# `fix-now` IS a closure in this register and stays one. Its convention is
# `fix-now` in the disposition cell + the actual resolution named in `resolved-by`
# (e.g. `| A1-002 | … | blocking | … | fix-now | REQ-007 (FIT default + TCX
# fallback) |`). VAL-018 ratified that reading, so re-reading it as "still open"
# here would silently re-open 36 historical rows on a vocabulary opinion rather
# than on evidence — the exact move this file exists to prevent.
CLOSING_DISPOSITIONS = (
    "fix-now", "fixed", "deferred", "defer", "accepted", "accept",
    "accept-with-note", "accept-with-rationale", "accepted-with-rationale",
    "accept-and-document", "superseded", "closed", "resolved", "resolved-by-mechanism",
    "ratified", "a3-ratified", "refuted", "withdrawn", "informational", "duplicate",
    "not-a-defect", "wontfix", "corrected", "no-action",
)

# Dispositions that are legible but leave work owed.
OPEN_DISPOSITIONS = ("open", "blocking", "confirmed", "scheduled", "pre-existing", "partially")

# A severity/disposition token may carry a trailing `-qualifier`
# (`blocking-for-clean-build`, `was-blocking-for-production`). The BASE token
# decides the class; the qualifier is free text and is preserved in the report.
_TOKEN = re.compile(r"[A-Za-z][A-Za-z0-9]*(?:-[A-Za-z0-9]+)*")

# `BLOCKS: {…}` — the effect set. `{}` is a real, explicit answer ("blocks nothing")
# and counts as classified; the ABSENCE of the marker is what does not.
_EFFECT = re.compile(r"BLOCKS:\s*\{[^}]*\}")


def split_cells(line: str) -> list[str]:
    """Split one GFM table row into cells, honouring backslash-escaped pipes.

    GFM is explicit that a code span does NOT protect a pipe: `a || b` inside
    backticks still splits the cell, and the only escape is `\\|`. So this reads
    `\\|` as one literal `|` character of content and every other `|` as a cell
    boundary — which is exactly what a Markdown renderer does, and therefore the
    only split that agrees with what a human sees in the rendered table.
    """
    cells: list[str] = []
    buf: list[str] = []
    i = 0
    while i < len(line):
        ch = line[i]
        if ch == "\\" and i + 1 < len(line) and line[i + 1] == "|":
            buf.append("|")
            i += 2
            continue
        if ch == "|":
            cells.append("".join(buf))
            buf = []
            i += 1
            continue
        buf.append(ch)
        i += 1
    cells.append("".join(buf))
    # A well-formed row is `| a | b | … |`, so the split yields an empty string at
    # each end. Drop exactly those two sentinels.
    if cells and cells[0].strip() == "":
        cells = cells[1:]
    if cells and cells[-1].strip() == "":
        cells = cells[:-1]
    return cells


def lead_token(cell: str) -> str:
    """The leading vocabulary token of a cell, with Markdown emphasis stripped."""
    text = cell.strip().lstrip("*`_ ").strip()
    m = _TOKEN.match(text)
    return m.group(0).lower() if m else ""


def base_token(tok: str, vocab: tuple[str, ...]) -> str | None:
    """Longest vocabulary entry that `tok` starts with, at a `-` boundary."""
    best = None
    for v in vocab:
        if tok == v or tok.startswith(v + "-"):
            if best is None or len(v) > len(best):
                best = v
    return best


HEADER_RE = re.compile(r"^\|\s*ID\s*\|\s*cycle\s*\|\s*severity\s*\|", re.I)


def parse_rows(path: str) -> tuple[list[dict], list[str]]:
    """Return (rows, errors). Rows are every data row of the findings register."""
    errors: list[str] = []
    try:
        with open(path, encoding="utf-8") as fh:
            lines = fh.read().split("\n")
    except OSError as exc:
        return [], ["cannot read %s: %s" % (path, exc)]

    rows: list[dict] = []
    in_fence = False
    seen_header = False
    for n, raw in enumerate(lines, 1):
        stripped = raw.strip()
        if stripped.startswith("```"):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        if HEADER_RE.match(raw):
            seen_header = True
            continue
        if not seen_header:
            continue
        if not raw.startswith("|"):
            continue
        if re.match(r"^\|[\s:-]+\|", raw):  # separator row
            continue
        cells = split_cells(raw)
        rows.append({"line": n, "cells": cells, "raw": raw})
    if not seen_header:
        errors.append("no findings-register header row (`| ID | cycle | severity | …`) in %s" % path)
    return rows, errors


def classify(rows: list[dict]) -> dict:
    """Sort every row into exactly one primary bucket, plus the effect axis."""
    out = {
        "MALFORMED": [], "UNKNOWN-SEVERITY": [], "UNKNOWN-DISPOSITION": [],
        "NEEDS-DISPOSITION": [], "OUTSTANDING": [], "OK": [],
    }
    missing_effect: list[dict] = []

    for row in rows:
        cells = row["cells"]
        line = row["line"]
        ident = cells[0].strip() if cells else "(no id)"
        rec = {"line": line, "id": ident, "cells": len(cells)}

        # 1. structure — an unreadable row cannot be adjudicated at all.
        if len(cells) != 6:
            rec["detail"] = "row has %d cells, the register schema has 6" % len(cells)
            out["MALFORMED"].append(rec)
            continue

        rec["cycle"] = cells[1].strip()
        sev_tok = lead_token(cells[2])
        dis_tok = lead_token(cells[4])
        rec["severity_token"] = sev_tok
        rec["disposition_token"] = dis_tok

        # 2. severity legibility.
        if sev_tok == UNDECIDED_SEVERITY:
            rec["detail"] = "severity is explicitly undecided; the gate cannot be judged"
            out["NEEDS-DISPOSITION"].append(rec)
            continue
        sev_base = base_token(sev_tok, BLOCKING_SEVERITIES + NON_BLOCKING_SEVERITIES)
        if sev_base is None:
            rec["detail"] = "severity cell does not begin with a vocabulary token: %r" % cells[2].strip()[:80]
            out["UNKNOWN-SEVERITY"].append(rec)
            continue
        rec["severity"] = sev_base

        # 3. disposition legibility — checked for EVERY legible row, because an
        #    unreadable disposition on a non-blocking row is still a ledger defect.
        dis_base = base_token(dis_tok, CLOSING_DISPOSITIONS + OPEN_DISPOSITIONS)
        if dis_base is None:
            rec["detail"] = "disposition cell does not begin with a vocabulary token: %r" % cells[4].strip()[:80]
            out["UNKNOWN-DISPOSITION"].append(rec)
            continue
        rec["disposition"] = dis_base

        # 4. the effect axis — recorded for every row, INDEPENDENT of disposition.
        rec["has_effect_set"] = bool(_EFFECT.search(row["raw"]))

        # 5. outstanding = blocking severity AND no closing disposition.
        #    Note what is NOT consulted here: `has_effect_set`. Classification is
        #    not disposition, so it cannot move a row out of this bucket.
        if sev_base in BLOCKING_SEVERITIES and dis_base not in CLOSING_DISPOSITIONS:
            out["OUTSTANDING"].append(rec)
            if not rec["has_effect_set"]:
                missing_effect.append(rec)
            continue

        out["OK"].append(rec)

    out["_missing_effect"] = missing_effect
    return out


# Buckets that make the gate fail. Every one of them is a case where the mechanism
# CANNOT prove the absence of a blocker — which is precisely when it must not pass.
BLOCKING_BUCKETS = ("MALFORMED", "UNKNOWN-SEVERITY", "UNKNOWN-DISPOSITION", "NEEDS-DISPOSITION", "OUTSTANDING")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="CLV Check 5 — findings-register closure.")
    default = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "findings.md")
    ap.add_argument("--findings", default=os.path.normpath(default))
    ap.add_argument("--json", action="store_true", help="machine-readable output")
    ap.add_argument("--quiet", action="store_true", help="counts only, no per-row listing")
    ap.add_argument("--limit", type=int, default=20, help="max rows listed per bucket (0 = all)")
    args = ap.parse_args(argv)

    if not os.path.isfile(args.findings) or not os.access(args.findings, os.R_OK):
        # Fail-safe: a missing input is exit 2, never exit 0. The predecessor of
        # this check passed for six weeks by reading a file that did not exist.
        msg = "FAIL — required input missing or unreadable: %s" % args.findings
        print(json.dumps({"error": msg}) if args.json else msg, file=sys.stderr)
        return 2

    rows, errors = parse_rows(args.findings)
    if errors:
        for e in errors:
            print("FAIL — %s" % e, file=sys.stderr)
        return 2

    buckets = classify(rows)
    missing_effect = buckets.pop("_missing_effect")
    counts = {k: len(v) for k, v in buckets.items()}
    counts["MISSING-EFFECT"] = len(missing_effect)
    counts["_rows"] = len(rows)

    failed = [b for b in BLOCKING_BUCKETS if buckets[b]]

    if args.json:
        print(json.dumps({
            "counts": counts,
            "failed_buckets": failed,
            "buckets": buckets,
            "missing_effect": missing_effect,
        }, indent=2))
        return 1 if failed else 0

    print("CLV Check 5 — findings register: %s" % args.findings)
    print("  %d data rows" % len(rows))
    for name in BLOCKING_BUCKETS:
        print("  %-20s %4d %s" % (name, counts[name], "" if counts[name] == 0 else "<- FAILS THE GATE"))
    print("  %-20s %4d" % ("OK", counts["OK"]))
    print()
    print("  MISSING-EFFECT       %4d  (of the %d OUTSTANDING rows, how many carry no `BLOCKS: {…}` set)"
          % (counts["MISSING-EFFECT"], counts["OUTSTANDING"]))
    print("       This is an INDEPENDENT axis. Writing an effect set on all %d of them"
          % counts["MISSING-EFFECT"])
    print("       leaves OUTSTANDING at %d: classification is not disposition." % counts["OUTSTANDING"])

    if not args.quiet:
        for name in BLOCKING_BUCKETS:
            items = buckets[name]
            if not items:
                continue
            print()
            print("  %s (%d):" % (name, len(items)))
            shown = items if args.limit == 0 else items[: args.limit]
            for rec in shown:
                extra = rec.get("detail", "")
                if name == "OUTSTANDING":
                    extra = "severity=%s disposition=%s effect_set=%s" % (
                        rec.get("severity"), rec.get("disposition"),
                        "yes" if rec.get("has_effect_set") else "NO")
                print("    L%-5d %-22s %s" % (rec["line"], rec["id"], extra))
            if len(items) > len(shown):
                print("    … and %d more (use --limit 0)" % (len(items) - len(shown)))

    print()
    print("RESULT: %s" % ("FAIL — " + ", ".join("%s=%d" % (b, counts[b]) for b in failed) if failed else "PASS"))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
