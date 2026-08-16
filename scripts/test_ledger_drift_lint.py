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

    # ---------------------------------------------------------------------
    # ORCH-010 — assignment-shape discrimination. Added 2026-08-12 after the
    # lint flagged 27 lines for the phrase "deferred reaper" (a MECHANISM NAME)
    # and "deferred-deletion semantics". Both directions are asserted: prose
    # must pass AND every genuine assignment shape must still be caught.
    # ---------------------------------------------------------------------

    # 16. PROSE PASSES: a status word used adjectivally is not an assignment.
    def test_status_word_as_adjective_not_flagged(self):
        mkfile(self.root, "STATE.md",
               "DEC-031 ACCEPTED (Option B - frame-counted deferred reaper).\n"
               "T-089 measures Qt loop-level deferred-deletion semantics.\n"
               "DEC-030 chose the reparent; the drafted proposal is in decisions.md.\n")
        code, out = run(self.root)
        self.assertEqual(code, 0, "adjectival status words must not be flagged; got:\n" + out)
        self.assertEqual(out, "")

    # 17. ASSIGNMENT STILL CAUGHT: token followed by a function word.
    def test_status_followed_by_function_word_still_flagged(self):
        mkfile(self.root, "STATE.md", "REQ-019 DEFERRED to REQ-021 pending the harness.\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-019", out)
        self.assertIn("DEFERRED", out)

    # 18. ASSIGNMENT STILL CAUGHT: table-cell shape (token then a pipe).
    def test_status_in_table_cell_still_flagged(self):
        mkfile(self.root, "WIKI.md", "| REQ-007 | download chain | GREEN |\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-007", out)
        self.assertIn("GREEN", out)

    # 19. ASSIGNMENT STILL CAUGHT: token at end of line.
    def test_status_at_end_of_line_still_flagged(self):
        mkfile(self.root, "wiki/architecture.md", "DES-013 sidebar wiring - CLOSED\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DES-013", out)
        self.assertIn("CLOSED", out)

    # 20. MIXED LINE: one adjectival use and one real assignment on the same line
    #     must still be caught (the "any occurrence in assignment shape" rule).
    def test_adjectival_and_assignment_on_same_line_flagged(self):
        mkfile(self.root, "STATE.md",
               "DEC-031 deferred reaper landed; REQ-019 is DEFERRED.\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEFERRED", out)

    # 21. A capitalised word after the token is NOT treated as an adjectival use —
    #     the suppression is deliberately narrow, so this still flags.
    def test_capitalised_follower_still_flagged(self):
        mkfile(self.root, "STATE.md", "T-015 harness CLOSED Friday\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("T-015", out)

    # ---------------------------------------------------------------------
    # ORCH-015 — BINDING SCOPE. Added 2026-08-15 after the lint flagged four
    # naturally-occurring STATE.md lines where the status word is in ASSIGNMENT
    # SHAPE (so ORCH-010's adjectival test correctly does not suppress it) but its
    # SUBJECT is not the id on the line: the id is an incidental citation that
    # landed on the same wrapped line. Two scope boundaries are asserted:
    #   (a) SENTENCE — an id in one sentence does not bind a status in the next;
    #   (b) QUOTATION — a status inside a quoted/code span and an id outside it
    #       (or inside a DIFFERENT span) are not part of the same assertion.
    # The four fixture lines below are the real STATE.md lines, VERBATIM. They are
    # embedded as string literals on purpose: STATE.md is live and its line numbers
    # move, so nothing here may be keyed to a line number.
    # Both directions are asserted: each prose line PASSES, and a near-miss variant
    # of each — where the status genuinely DOES bind to the id — still FLAGS.
    # ---------------------------------------------------------------------

    # The four real false-positive lines, quoted verbatim from STATE.md (2026-08-15).
    FP_QUOTED_MENTION_A = (
        '   fails on `STATE.md:533`, where "GREEN" describes the TEST SUITE and '
        '`DEC-030` is an incidental citation that landed')
    FP_QUOTED_MENTION_B = (
        'shape. `:591` is the cleanest: `47/47 STILL GREEN` is an assignment ABOUT '
        'THE TEST SUITE while `DEC-030` is an')
    FP_QUOTED_MENTION_C = (
        'prose (`STATE.md:533`, where "GREEN" describes the suite and `DEC-030` is '
        'an incidental citation on the wrapped line).')
    FP_OTHER_SENTENCE_D = (
        'dialog (DEC-030) and a loop that keeps iterating on `this`. **I deleted '
        'that line myself: 47/47 STILL GREEN.** The')

    # 22. PROSE PASSES: all four real STATE.md lines, each in its own fixture tree.
    def test_real_state_md_false_positive_lines_pass(self):
        for label, text in (
                ("A", self.FP_QUOTED_MENTION_A),
                ("B", self.FP_QUOTED_MENTION_B),
                ("C", self.FP_QUOTED_MENTION_C),
                ("D", self.FP_OTHER_SENTENCE_D)):
            with self.subTest(line=label):
                with tempfile.TemporaryDirectory() as td:
                    mkfile(td, "STATE.md", text + "\n")
                    code, out = run(td)
                    self.assertEqual(code, 0, f"line {label} must not be flagged; got:\n{out}")
                    self.assertEqual(out, "")

    # 22b. And all four together in one file (they are all in STATE.md in reality).
    def test_real_state_md_false_positive_lines_pass_together(self):
        mkfile(self.root, "STATE.md", "\n".join((
            self.FP_QUOTED_MENTION_A,
            self.FP_QUOTED_MENTION_B,
            self.FP_QUOTED_MENTION_C,
            self.FP_OTHER_SENTENCE_D)) + "\n")
        code, out = run(self.root)
        self.assertEqual(code, 0, "real STATE.md prose must not be flagged; got:\n" + out)
        self.assertEqual(out, "")

    # 23. NEAR-MISS of A/C: strip the quotation marks so the status and the id sit in
    #     the SAME (unquoted) region of the SAME sentence -> genuine drift, still flagged.
    def test_near_miss_unquoted_status_binds_id_still_flagged(self):
        mkfile(self.root, "STATE.md",
               "the line where DEC-030 is recorded GREEN, an incidental citation\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-030", out)
        self.assertIn("GREEN", out)

    # 24. NEAR-MISS of B: id and status inside the SAME code span bind normally —
    #     the quotation rule is same-region matching, NOT a blanket backtick amnesty.
    def test_near_miss_id_and_status_in_same_quote_still_flagged(self):
        mkfile(self.root, "STATE.md",
               "the cleanest case is `DEC-030 STILL GREEN and unfixed` in the wrapped line\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-030", out)
        self.assertIn("GREEN", out)

    # 25. NEAR-MISS of D: same sentence instead of the next one -> still flagged.
    def test_near_miss_same_sentence_binding_still_flagged(self):
        mkfile(self.root, "STATE.md",
               "**I deleted that line myself: DEC-030 is still GREEN.** The rest stands.\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-030", out)
        self.assertIn("GREEN", out)

    # 26. Sentence scoping must not become a hiding place: a real assignment in the
    #     SECOND sentence of a multi-sentence line is still flagged.
    def test_assignment_in_second_sentence_still_flagged(self):
        mkfile(self.root, "STATE.md",
               "The reaper landed and the UAF is gone. REQ-021 is DEFERRED to REQ-022.\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-021", out)
        self.assertIn("DEFERRED", out)

    # 27. Explicit assignment shape for a DEC (the `DEC-030 = <status>` form).
    #     NOTE: "ACCEPTED" is deliberately NOT in STATUS_TOKENS (case 16 depends on
    #     that), so the same SHAPE is asserted with a vocabulary token.
    def test_dec_explicit_assignment_still_flagged(self):
        mkfile(self.root, "STATE.md", "DEC-030 = GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-030", out)
        self.assertIn("GREEN", out)
        self.assertIn("decisions.md", out)

    # 28. Real REQ / TEST / VAL status assignments are all still caught.
    def test_req_test_val_assignments_still_flagged(self):
        mkfile(self.root, "wiki/architecture.md",
               "REQ-042 | download chain | GREEN |\n"
               "TEST-017 harness - CLOSED\n"
               "VAL-009 cross-layer check DEFERRED to the next wave.\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        lines = [ln for ln in out.splitlines() if ln.strip()]
        self.assertEqual(len(lines), 3, out)
        self.assertIn("REQ-042", out)
        self.assertIn("TEST-017", out)
        self.assertIn("VAL-009", out)

    # 29. MIXED LINE: a quoted MENTION of a status plus a real unquoted assignment
    #     on the same line -> the real one is still flagged (exactly once).
    def test_quoted_mention_plus_real_assignment_flagged_once(self):
        mkfile(self.root, "STATE.md",
               'the word "GREEN" is vocabulary, but REQ-007 download chain GREEN\n')
        code, out = run(self.root)
        self.assertEqual(code, 1)
        lines = [ln for ln in out.splitlines() if ln.strip()]
        self.assertEqual(len(lines), 1, out)
        self.assertIn("REQ-007", out)

    # 30. A quoted span that contains BOTH id and status (a drift line being quoted
    #     verbatim in a double-quoted string) still flags — same-region rule again.
    def test_double_quoted_span_with_id_and_status_still_flagged(self):
        mkfile(self.root, "WIKI.md", 'the row reads "REQ-007 download chain GREEN" today\n')
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-007", out)

    # 31. An id in one quoted span and a status in a DIFFERENT quoted span do not
    #     bind (this is the shape of the real STATE.md lines, minimised).
    def test_id_and_status_in_different_quotes_not_flagged(self):
        mkfile(self.root, "STATE.md", 'the tokens `GREEN` and `DEC-030` are unrelated here\n')
        code, out = run(self.root)
        self.assertEqual(code, 0, out)
        self.assertEqual(out, "")

    # 32. TABLE ROW = ONE RECORD: scope boundaries must not apply inside a row. Modelled on
    #     a real traceability.md row — a description cell that ENDS A SENTENCE before the
    #     status cell. Without the table-row exception this drift would be missed.
    def test_table_row_with_sentence_break_before_status_still_flagged(self):
        mkfile(self.root, "STATE.md",
               "| TEST-003 credentials-page contract | REQ-002 wizard-side "
               "(in-flight disables Next). 9 tests, **GREEN**. |\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("TEST-003", out)
        self.assertIn("GREEN", out)

    # 33. TABLE ROW: a status cell written in backticks is still a status cell, not a
    #     quoted mention (this is the exact LSN-008 drift shape lessons.md records).
    def test_table_row_with_backticked_status_cell_still_flagged(self):
        mkfile(self.root, "wiki/architecture.md",
               "| REQ-004 | Per-athlete token storage | `_uncommitted_` |\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-004", out)
        self.assertIn("_uncommitted_", out)

    # 33b. Ambiguity must resolve toward FLAGGING: a sentence break INSIDE a quotation does
    #      not split the quotation, so a drift line quoted verbatim is still caught whole.
    def test_sentence_break_inside_quote_does_not_split_binding(self):
        mkfile(self.root, "STATE.md", 'the row reads "REQ-007 landed. GREEN" verbatim\n')
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-007", out)
        self.assertIn("GREEN", out)

    # 33c. Ambiguity must resolve toward FLAGGING: a quote character with no closer on the
    #      line (a wrapped code span) opens NO region, so it cannot shield a real assignment.
    def test_unbalanced_quote_opener_does_not_shield_assignment(self):
        mkfile(self.root, "STATE.md",
               "REQ-007 depends on `AtomicFile and the row still says GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-007", out)
        self.assertIn("GREEN", out)

    # 34. The table-row exception is keyed to the ROW shape, not to the presence of pipes:
    #     a PROSE line that merely quotes a pipe-separated vocabulary is still scoped.
    def test_prose_quoting_pipe_vocabulary_not_flagged(self):
        mkfile(self.root, "STATE.md",
               "> Status cells use the controlled vocabulary (DEC-015): "
               "`drafted | GREEN | committed | CLOSED`\n")
        code, out = run(self.root)
        self.assertEqual(code, 0, out)
        self.assertEqual(out, "")

    # ---------------------------------------------------------------------
    # ORCH-015(b) — PARENTHETICAL CITATION. Added 2026-08-15. ORCH-010 taught the
    # lint the ATTRIBUTIVE position (status word + noun = prose); it never handled
    # the PREDICATE position ("... is deferred."), where the status word ends the
    # phrase and so reads as an assignment. Binding scope (ORCH-015a) does not reach
    # it either: the id and the status are in the SAME sentence and the SAME
    # quotation region. The discriminator here is the ID's role, not the status's:
    # an id in PARENTHESES is a CITATION attached to the neighbouring noun phrase,
    # never the subject of the sentence.
    #
    # The four lines below are the coordinator's ORCH-015(b) corpus, VERBATIM, with
    # the required verdict for each. Line 2 is the trap: it is also a predicate
    # construction and it MUST keep firing.
    # ---------------------------------------------------------------------

    PRED_CORPUS = (
        # (label, line, must_fire)
        ("1 parenthetical citation, predicate status",
         "the reaper (DEC-031) is frame-counted and deferred.", False),
        ("2 bare id in subject position",
         "DEC-031 was ACCEPTED and its slice is GREEN", True),
        ("3 attributive status (ORCH-010's case)",
         "a frame-counted, deferred reaper (DEC-031) drains the loop", False),
        ("4 parenthetical citation, bare predicate",
         "the store leak (DEC-025) is deferred.", False),
    )

    # 35. The ORCH-015(b) corpus, each line in its own fixture tree.
    def test_predicate_position_corpus(self):
        for label, text, must_fire in self.PRED_CORPUS:
            with self.subTest(line=label):
                with tempfile.TemporaryDirectory() as td:
                    mkfile(td, "STATE.md", text + "\n")
                    code, out = run(td)
                    if must_fire:
                        self.assertEqual(code, 1, f"{label} MUST still fire: {text}")
                    else:
                        self.assertEqual(code, 0, f"{label} must not fire; got:\n{out}")

    # 35b. Line 2's finding is the real one: the id and the status are both named.
    def test_predicate_true_assignment_names_id_and_status(self):
        mkfile(self.root, "STATE.md", "DEC-031 was ACCEPTED and its slice is GREEN\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-031", out)
        self.assertIn("GREEN", out)

    # 36. WHERE THE BOUNDARY SITS: one line, two ids outside an aside and one inside.
    #     The BARE ids still bind; the PARENTHESISED citation does not. (REQ-021 is
    #     reported too — it is the TARGET of "DEFERRED to", not the subject. That
    #     over-report predates ORCH-015 and needs a different discriminator; it is
    #     asserted here so the next person sees it rather than rediscovers it.)
    def test_bare_id_binds_while_parenthesised_id_does_not(self):
        mkfile(self.root, "STATE.md", "REQ-019 (see DEC-024) is DEFERRED to REQ-021\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-019", out)
        self.assertNotIn("DEC-024", out)
        self.assertIn("REQ-021", out)      # known over-report, documented above

    # 37. NEAR-MISS: id and status INSIDE the same parenthetical bind normally — the
    #     rule is about crossing the aside boundary, not about parentheses per se.
    def test_id_and_status_inside_same_parenthetical_still_flagged(self):
        mkfile(self.root, "STATE.md", "the reaper (DEC-031 DEFERRED) drains the loop\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-031", out)
        self.assertIn("DEFERRED", out)

    # 38. THE ASYMMETRY, and why it matters: a STATUS in a parenthetical predicates
    #     about the phrase it is attached to, so "REQ-019 (DEFERRED)" — a real and
    #     common drift shape — must still fire even though the tokens are separated
    #     by the same parenthesis boundary as case 36.
    def test_status_in_parenthetical_still_binds_outside_id(self):
        mkfile(self.root, "wiki/architecture.md", "REQ-019 upload dialog (DEFERRED)\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("REQ-019", out)
        self.assertIn("DEFERRED", out)

    # 39. Ambiguity resolves toward FLAGGING: an unclosed '(' opens no aside, so a
    #     mistyped or line-wrapped parenthesis cannot shield a real assignment.
    def test_unbalanced_parenthesis_does_not_shield_assignment(self):
        mkfile(self.root, "STATE.md", "the reaper (DEC-031 is DEFERRED\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-031", out)

    # 40. The citation rule does not leak across a table row's cells: a row whose
    #     PRIMARY id cell is bare still flags even though a later cell cites another
    #     id parenthetically (modelled on a real traceability.md row).
    def test_table_row_bare_primary_id_still_flags_with_parenthetical_citation(self):
        mkfile(self.root, "STATE.md",
               "| DEC-029 slice | Upload-dialog UAF (REQ-019) | GREEN |\n")
        code, out = run(self.root)
        self.assertEqual(code, 1)
        self.assertIn("DEC-029", out)


if __name__ == "__main__":
    unittest.main()
