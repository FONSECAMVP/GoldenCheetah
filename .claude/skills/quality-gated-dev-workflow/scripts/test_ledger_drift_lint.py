#!/usr/bin/env python3
"""
test_ledger_drift_lint.py — hermetic unittest suite for ledger_drift_lint.py (garmin-T-017).

Each test builds an isolated fixture tree with tempfile.TemporaryDirectory so the suite is
fully self-contained. Tests exercise the CLI contract via main([root]) (capturing stdout and
the integer exit code) and the importable scan()/format_finding() API.

Run:  python3 -m unittest test_ledger_drift_lint -v
"""

import contextlib
import io
import os
import tempfile
import unittest

import ledger_drift_lint as lint


# ----------------------------- fixture helpers -----------------------------

def mkfile(root, relpath, content):
    """Create root/relpath (making parent dirs) with the given text content."""
    full = os.path.join(root, relpath)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, "w", encoding="utf-8") as fh:
        fh.write(content)
    return full


def run(root):
    """Invoke the CLI entrypoint; return (exit_code, stdout_text)."""
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        code = lint.main([root])
    return code, buf.getvalue()


class LedgerDriftLintTest(unittest.TestCase):

    def setUp(self):
        self._td = tempfile.TemporaryDirectory()
        self.root = self._td.name

    def tearDown(self):
        self._td.cleanup()

    # 1. Clean tree: ids referenced WITHOUT any status token.
    def test_clean_tree_ids_without_status(self):
        mkfile(self.root, "STATE.md", "See REQ-007 and DES-013 for the download chain.\n")
        mkfile(self.root, "WIKI.md", "Overview mentions VAL-011 and T-015.\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 2. STATE.md pairing REQ-007 with GREEN -> exit 1, finding cites STATE.md + REQ-007 + GREEN.
    def test_state_req_green_violation(self):
        mkfile(self.root, "STATE.md", "REQ-007 download chain GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("STATE.md", out)
        self.assertIn("REQ-007", out)
        self.assertIn("GREEN", out)
        self.assertIn("traceability.md", out)  # canonical home named in message

    # 3. workflow-x/design.md pairing DES-013 ... DEFERRED -> exit 1.
    def test_design_des_deferred_violation(self):
        mkfile(self.root, ".claude/workflow-x/design.md",
               "DES-013 sidebar wiring is DEFERRED for now\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("design.md", out)
        self.assertIn("DES-013", out)
        self.assertIn("DEFERRED", out)

    # 4. traceability.md is a canonical source-of-truth ledger and is NOT scanned at all,
    #    so REQ-004 ... GREEN there -> exit 0.
    def test_traceability_not_scanned(self):
        mkfile(self.root, ".claude/workflow-x/traceability.md",
               "REQ-004 | download chain | GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 5. traceability.md legitimately co-locates a REQ's governing DEC(s) with the REQ's
    #    status word; since traceability.md is not scanned, "DEC-012 ... GREEN" -> exit 0.
    def test_traceability_canonical_not_scanned_no_foreign_flag(self):
        mkfile(self.root, ".claude/workflow-x/traceability.md",
               "REQ-002 | governed by DEC-001, DEC-012 | GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 6. decisions.md line "DEC-014 ... accepted (B)" -> exit 0 (decisions.md not scanned).
    def test_decisions_not_scanned(self):
        mkfile(self.root, ".claude/workflow-x/decisions.md",
               "DEC-014 | pick queue backend | accepted (B)\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 6b. decisions.md cascade-notes reference the REQ/DES a decision affects, alongside a
    #     status word — legitimate, and decisions.md is not scanned -> exit 0.
    def test_decisions_cascade_prose_not_flagged(self):
        mkfile(self.root, ".claude/workflow-x/decisions.md",
               "DEC-020 makes DES-002 literally true; REQ-004 write path GREEN, DES-002 GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 7. Provenance block exempts an otherwise-violating pairing; the SAME pairing AFTER the
    #    next '## ' heading in a non-canonical file is still flagged.
    def test_provenance_block_exempt_then_flagged_after_heading(self):
        content = (
            "# Ledger\n"
            "<!-- provenance: dated; excluded from status-lint -->\n"
            "REQ-007 historical row GREEN\n"          # line 3: inside block -> exempt
            "DES-006 another historical row GREEN\n"  # line 4: inside block -> exempt
            "## Live section\n"                        # line 5: closes the block (processed)
            "REQ-007 live status GREEN\n"             # line 6: NON-canonical file -> flagged
        )
        mkfile(self.root, "STATE.md", content)
        code, out = run(self.root)
        self.assertEqual(code, 1)
        # Exactly one finding, and it is the line-6 pairing (not the exempt line-3/4 ones).
        lines = [ln for ln in out.splitlines() if ln.strip()]
        self.assertEqual(len(lines), 1)
        self.assertIn("STATE.md:6:", lines[0])
        self.assertNotIn(":3:", out)
        self.assertNotIn(":4:", out)

        # And a DES-006 GREEN pairing inside a provenance block in a SCANNED file (design.md)
        # is exempt (would otherwise violate) -> exit 0.
        with tempfile.TemporaryDirectory() as td2:
            mkfile(td2, ".claude/workflow-x/design.md",
                   "<!-- provenance: dated; artifacts appendix -->\n"
                   "DES-006 historical row GREEN\n")   # would violate if not exempt
            code2, out2 = run(td2)
            self.assertEqual(code2, 0)
            self.assertEqual(out2, "")

    # 8. archive/, cycles/, validations/ dirs and lessons.md/findings.md are excluded.
    def test_exclusions_never_flagged(self):
        mkfile(self.root, "archive/STATE.md", "REQ-006 CLOSED\n")
        mkfile(self.root, "cycles/WIKI.md", "REQ-006 CLOSED\n")
        mkfile(self.root, "validations/STATE.md", "REQ-006 CLOSED\n")
        mkfile(self.root, "wiki/lessons.md", "REQ-006 CLOSED\n")
        mkfile(self.root, "wiki/findings.md", "REQ-006 CLOSED\n")
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    # 9. WIKI.md line "T-015/16 ... in build" -> exit 1 (TEST class, WIKI not canonical).
    def test_wiki_test_in_build_violation(self):
        mkfile(self.root, "WIKI.md", "T-015/16 harness scaffolding in build\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("WIKI.md", out)
        self.assertIn("T-015", out)
        self.assertIn("in build", out)
        self.assertIn("TEST", out)  # id-class label

    # 10. Multiple violations across files -> exit 1, all reported, sorted by path then lineno.
    def test_multiple_violations_deterministic_order(self):
        mkfile(self.root, "STATE.md",
               "REQ-001 alpha GREEN\n"
               "REQ-002 beta GREEN\n")
        mkfile(self.root, "WIKI.md", "DES-003 gamma DEFERRED\n")
        mkfile(self.root, ".claude/workflow-x/design.md", "VAL-004 delta CLOSED\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        lines = [ln for ln in out.splitlines() if ln.strip()]
        self.assertEqual(len(lines), 4)
        # Deterministic order: sorted by path then lineno.
        paths_linenos = [ln.split(":")[0] + ":" + ln.split(":")[1] for ln in lines]
        self.assertEqual(paths_linenos, sorted(paths_linenos))
        # And a second run yields byte-identical output.
        _, out2 = run(self.root)
        self.assertEqual(out, out2)

    # API surface: scan() returns structured findings; usage/IO errors exit 2.
    def test_scan_api_returns_findings(self):
        mkfile(self.root, "STATE.md", "REQ-007 download chain GREEN\n")
        findings = lint.scan(self.root)
        self.assertEqual(len(findings), 1)
        f = findings[0]
        self.assertEqual(f.id_token, "REQ-007")
        self.assertEqual(f.status_token, "GREEN")
        self.assertEqual(f.id_class, "REQ")
        self.assertEqual(f.canonical, "traceability.md")

    # Precision: "in progress" is NOT a status token (DEC-015 dropped it — it collides with
    # the spec'd REQ-NF-Perf-002 rejection message quoted in design.md). Must NOT flag.
    def test_in_progress_domain_message_not_flagged(self):
        mkfile(self.root, ".claude/workflow-x/design.md",
               'reject with "sync already in progress" (REQ-NF-Perf-002)\n')
        code, out = run(self.root)
        self.assertEqual(code, 0)
        self.assertEqual(out, "")

    def test_bad_root_exits_2(self):
        code = lint.main([os.path.join(self.root, "does-not-exist")])
        self.assertEqual(code, 2)

    def test_too_many_args_exits_2(self):
        code = lint.main([self.root, "extra"])
        self.assertEqual(code, 2)


if __name__ == "__main__":
    unittest.main()
