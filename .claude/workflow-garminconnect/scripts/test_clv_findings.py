#!/usr/bin/env python3
"""Self-tests for the canonical CLV Check 5 implementation.

Runs under BOTH runners the project uses (ORCH-004 convention):
    python3 -m pytest scripts/test_clv_findings.py
    python3 -m unittest discover -s scripts -p 'test_*.py'

EVERY CASE IS A SYNTHETIC FIXTURE. Not one of them reads the real `findings.md`:
a suite that asserts against live data stops being a suite the day the data
changes, and it can never exercise a case the data does not happen to contain.

THE SUITE MUST DISCRIMINATE, NOT MERELY PASS ([[LSN-050]]). Six cases assert
exit 0 and twelve assert non-zero (ten exit 1 and two input failures exit 2),
covering both verdict directions so a validator that returned a constant would
fail this file immediately.
"""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
VALIDATOR = os.path.join(HERE, "clv_findings.py")

HEADER = (
    "# synthetic fixture\n"
    "\n"
    "| ID | cycle | severity | summary | disposition | resolved-by |\n"
    "|---|---|---|---|---|---|\n"
)


def count_of(out: str, bucket: str) -> int:
    """Pull one bucket's count out of the report, independent of column padding."""
    import re as _re
    m = _re.search(r"^\s*%s\s+(\d+)" % _re.escape(bucket), out, _re.M)
    if m is None:
        raise AssertionError("bucket %r not present in report:\n%s" % (bucket, out))
    return int(m.group(1))


def run_on(body: str) -> tuple[int, str]:
    """Write a synthetic register, validate it, return (exit code, stdout)."""
    with tempfile.NamedTemporaryFile("w", suffix=".md", delete=False, encoding="utf-8") as fh:
        fh.write(HEADER + body)
        path = fh.name
    try:
        p = subprocess.run([sys.executable, VALIDATOR, "--findings", path, "--limit", "0"],
                           capture_output=True, text=True)
        return p.returncode, p.stdout + p.stderr
    finally:
        os.unlink(path)


class CheckFiveCases(unittest.TestCase):
    # ── the cases that MUST pass ─────────────────────────────────────────────

    def test_valid_resolved_finding_passes(self):
        rc, out = run_on("| X-01 | A3/REQ-001 | blocking | a real defect | fixed | TEST-001, commit abc1234 |\n")
        self.assertEqual(rc, 0, out)
        self.assertEqual(count_of(out, "OUTSTANDING"), 0, out)

    def test_escaped_pipe_inside_a_cell_passes(self):
        # The GFM escape. `capabilities == Query \| Download` is ONE cell, and the
        # row is well-formed. Getting this wrong is what produced 28 unreadable
        # rows in the real register.
        rc, out = run_on(
            "| X-02 | A3/REQ-011 | non-blocking | caps are `Query \\| Download` and `a \\|\\| b` | fixed | TEST-002 |\n")
        self.assertEqual(rc, 0, out)
        self.assertEqual(count_of(out, "MALFORMED"), 0, out)

    def test_superseded_provenance_row_passes(self):
        rc, out = run_on(
            "| X-03 | Builder/REQ-012 | blocking | superseded by a later row | superseded | see X-04 |\n")
        self.assertEqual(rc, 0, out)

    def test_accepted_with_rationale_passes(self):
        rc, out = run_on(
            "| X-04 | A3/REQ-008 | blocking | deferred by explicit user call | accepted-with-rationale | user 2026-07-20 |\n")
        self.assertEqual(rc, 0, out)

    def test_non_blocking_open_row_passes(self):
        # Open, but not blocking: it is owed work, not a gate.
        rc, out = run_on("| X-05 | A3/REQ-009 | non-blocking | cosmetic wording | open | — |\n")
        self.assertEqual(rc, 0, out)

    # ── the cases that MUST fail ─────────────────────────────────────────────

    def test_open_blocking_finding_fails(self):
        rc, out = run_on("| Y-01 | A3/REQ-021 | blocking | a live use-after-free | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("OUTSTANDING", out)
        self.assertIn("Y-01", out)

    def test_blocking_without_effect_set_is_reported_on_its_own_axis(self):
        rc, out = run_on("| Y-02 | A3/REQ-021 | blocking | no effect set anywhere | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertEqual(count_of(out, "MISSING-EFFECT"), 1, out)

    def test_classified_but_undispositioned_blocker_stays_outstanding(self):
        # THE ANTI-REGRESSION FOR THE IMPOSSIBLE GATE. This row carries a complete
        # effect set and is STILL outstanding, because classification is not
        # disposition. If this ever passes, someone has rewired OUTSTANDING to
        # consult the effect axis and the gate has become unsatisfiable again.
        rc, out = run_on(
            "| Y-03 | A3/REQ-021 | blocking | classified, not dispositioned. BLOCKS: {RELEASE}. | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertEqual(count_of(out, "OUTSTANDING"), 1, out)
        self.assertEqual(count_of(out, "MISSING-EFFECT"), 0, out)

    def test_malformed_row_fails(self):
        # An unescaped `|` inside a cell — the real register's defect shape.
        rc, out = run_on("| Y-04 | A3/REQ-007 | blocking | guard is `a || b` here | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("MALFORMED", out)
        self.assertIn("Y-04", out)

    def test_row_with_no_trailing_pipe_fails(self):
        # The shape that slipped past the awk predecessor: seven columns and no
        # terminating pipe split into exactly the field count a well-formed
        # six-column row produces, so it was read as valid and its severity was
        # taken from the wrong cell.
        rc, out = run_on("| Y-05 | A3/REQ-019 | blocking | seven cols | open | — | trailing note\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("MALFORMED", out)

    def test_missing_severity_fails(self):
        # The severity cell holds prose, not a severity. The predecessor skipped
        # these silently, which is a verdict of "harmless" that nobody made.
        rc, out = run_on(
            "| Y-06 | A3/REQ-028 | The three completion slots' tail can deliver a Refresh | detail | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("UNKNOWN-SEVERITY", out)

    def test_unknown_disposition_fails(self):
        rc, out = run_on(
            "| Y-07 | Builder/REQ-027 | blocking | real defect | SCOPE ITEM 3 IS PULLED OUT AND ROUTED | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("UNKNOWN-DISPOSITION", out)

    def test_needs_disposition_fails(self):
        # Declining to set a severity must be VISIBLE and must not pass the gate.
        rc, out = run_on(
            "| Y-08 | Builder/REQ-028 | needs-disposition | severity deliberately unset | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("NEEDS-DISPOSITION", out)

    def test_was_blocking_still_needs_a_closing_disposition(self):
        rc, out = run_on("| Y-09 | A3/REQ-004 | was-blocking-for-production | still open | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("OUTSTANDING", out)

    def test_blocking_with_qualifier_is_recognised_as_blocking(self):
        rc, out = run_on("| Y-10 | ORCH/build | blocking-for-clean-build | broke the build | open | — |\n")
        self.assertEqual(rc, 1, out)
        self.assertIn("OUTSTANDING", out)

    # ── input handling ───────────────────────────────────────────────────────

    def test_missing_required_input_exits_nonzero(self):
        p = subprocess.run(
            [sys.executable, VALIDATOR, "--findings", os.path.join(HERE, "no-such-file-xyz.md")],
            capture_output=True, text=True)
        self.assertEqual(p.returncode, 2, p.stdout + p.stderr)
        self.assertIn("missing or unreadable", p.stdout + p.stderr)

    def test_register_with_no_header_exits_nonzero(self):
        with tempfile.NamedTemporaryFile("w", suffix=".md", delete=False, encoding="utf-8") as fh:
            fh.write("# no table here\n\njust prose\n")
            path = fh.name
        try:
            p = subprocess.run([sys.executable, VALIDATOR, "--findings", path],
                               capture_output=True, text=True)
            self.assertEqual(p.returncode, 2, p.stdout + p.stderr)
        finally:
            os.unlink(path)

    def test_fenced_code_block_rows_are_not_parsed_as_findings(self):
        # findings.md documents the mechanism in a fenced block that contains
        # pipe characters. Those lines are documentation, not findings.
        body = ("| X-06 | A3/REQ-001 | blocking | fine | fixed | done |\n"
                "\n```sh\n| not | a | finding | row | at | all |\n```\n")
        rc, out = run_on(body)
        self.assertEqual(rc, 0, out)
        self.assertIn("1 data rows", out)


if __name__ == "__main__":
    unittest.main(verbosity=2)
