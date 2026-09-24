import unittest
from dispatch import CONTRACT, Rejected, classify, spill_path_of, validate_brief

UNIT = "s925-r1"
APPENDED = CONTRACT.format(unit=UNIT, cap=80, spill="/tmp/insp-exchange")


def brief(*extra):
    return "\n".join(("ROLE: You are the reviewer.", *extra))


class ValidateTests(unittest.TestCase):
    def test_accepts_an_ordinary_brief(self):
        validate_brief(brief("AUTHORITY: DEC-046 (.claude/workflow-garminconnect/"
                             "decisions.md). Read it first."), UNIT, "reviewer")

    def test_ledger_paths_are_not_exchange_paths(self):
        # The brief legitimately cites ledgers and source files; only the exchange
        # basenames are forbidden, so a blanket path ban would be wrong.
        validate_brief(brief("REVIEW: git diff -- src/Cloud/GarminConnect.cpp",
                             "SETTLED: see findings.md and decisions.md"), UNIT, "reviewer")

    def test_rejects_the_old_file_drop_handoff(self):
        with self.assertRaises(Rejected) as caught:
            validate_brief(brief(f"Read /tmp/{UNIT}_brief.md and carry out exactly "
                                 "what it asks."), UNIT, "reviewer")
        self.assertIn("outside /tmp/insp-exchange", str(caught.exception))

    def test_rejects_an_invented_exchange_directory(self):
        with self.assertRaises(Rejected):
            validate_brief(brief("prior findings at "
                                 "/home/andy/gc-insp-exchange/reviewer_s925_round1_findings.md"),
                           UNIT, "reviewer")

    def test_allows_the_declared_spill_path(self):
        validate_brief(brief(f"prior round: /tmp/insp-exchange/{UNIT}_findings.md"),
                       UNIT, "reviewer")

    def test_rejects_over_cap(self):
        with self.assertRaises(Rejected) as caught:
            validate_brief(brief(*["FILLER"] * 45), UNIT, "reviewer")
        self.assertIn("cap for reviewer is 40", str(caught.exception))

    def test_cap_is_per_role(self):
        body = brief(*["FILLER"] * 45)
        validate_brief(body, UNIT, "builder")          # 46 lines, under the builder's 50
        self.assertRaises(Rejected, validate_brief, body, UNIT, "adhoc")

    def test_cap_excludes_the_appended_contract(self):
        # The contract is boilerplate dispatch.py adds; it must not eat the budget
        # rule 4 reserves for unit content.
        validate_brief(brief(*["FILLER"] * 38), UNIT, "reviewer")

    def test_rejects_a_hand_written_contract(self):
        with self.assertRaises(Rejected) as caught:
            validate_brief(brief(APPENDED), UNIT, "reviewer")
        self.assertIn("appends the DELIVER contract", str(caught.exception))


class ContractTests(unittest.TestCase):
    def test_appended_contract_keeps_markers_inline(self):
        # If a future edit puts either sentinel alone on a line here, every prompt
        # echo becomes indistinguishable from a real reply.
        self.assertEqual(classify(APPENDED, UNIT), ("no_reply", ""))


class ClassifyTests(unittest.TestCase):
    def transcript(self, *reply_lines):
        # The pane shows the echoed brief (markers inline) and then the reply.
        return "\n".join(["> " + APPENDED, "", "I'll review that now.", *reply_lines])

    def test_echoed_brief_alone_is_not_a_reply(self):
        self.assertEqual(classify(self.transcript(), UNIT), ("no_reply", ""))

    def test_complete_reply_is_extracted_past_the_echo(self):
        status, body = classify(self.transcript(
            f"<<<BEGIN {UNIT}>>>", "BLOCKING garmin_client.py:380 -- ts_gmt unvalidated",
            f"<<<END {UNIT}>>>"), UNIT)
        self.assertEqual(status, "complete")
        self.assertEqual(body, "BLOCKING garmin_client.py:380 -- ts_gmt unvalidated")

    def test_lost_head_is_truncated_not_complete(self):
        # The BEGIN scrolled off the alternate screen; only the echo's inline
        # mention survives, and that must not be mistaken for the reply's start.
        status, body = classify(self.transcript(
            "...rest of a long report...", f"<<<END {UNIT}>>>"), UNIT)
        self.assertEqual((status, body), ("truncated", ""))

    def test_unterminated_reply(self):
        status, _ = classify(self.transcript(f"<<<BEGIN {UNIT}>>>", "still writing"), UNIT)
        self.assertEqual(status, "unterminated")

    def test_other_units_do_not_match(self):
        status, _ = classify(self.transcript("<<<BEGIN s925-r2>>>", "x", "<<<END s925-r2>>>"),
                             UNIT)
        self.assertEqual(status, "no_reply")

    def test_spill_reply_is_recognised(self):
        _, body = classify(self.transcript(
            f"<<<BEGIN {UNIT}>>>", f"/tmp/insp-exchange/{UNIT}.md", f"<<<END {UNIT}>>>"), UNIT)
        self.assertEqual(spill_path_of(body), f"/tmp/insp-exchange/{UNIT}.md")

    def test_inline_reply_has_no_spill_path(self):
        self.assertIsNone(spill_path_of("BLOCKING foo.py:1 -- bad\nNON-BLOCKING bar.py:2"))


if __name__ == "__main__":
    unittest.main()
