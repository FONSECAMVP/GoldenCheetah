import unittest
from guard_exchange_paths import offending_paths


def event(tool, **tool_input):
    return {"tool_name": tool, "tool_input": tool_input}


class GuardTests(unittest.TestCase):
    def test_blocks_write_to_an_invented_exchange_directory(self):
        self.assertEqual(
            offending_paths(event("Write",
                                  file_path="/home/andy/gc-insp-exchange/"
                                            "reviewer_s925_round1_findings.md")),
            ["/home/andy/gc-insp-exchange/reviewer_s925_round1_findings.md"])

    def test_blocks_the_old_tmp_convention(self):
        self.assertTrue(offending_paths(event("Write", file_path="/tmp/s925_brief.md")))
        self.assertTrue(offending_paths(event("Edit", file_path="/tmp/s925_findings.md")))
        self.assertTrue(offending_paths(event("Write", file_path="/tmp/s925_review_brief.md")))

    def test_allows_the_sanctioned_spill_path(self):
        self.assertEqual(offending_paths(event("Write",
                                               file_path="/tmp/insp-exchange/s925_report.md")), [])

    def test_ignores_ordinary_project_files(self):
        for path in ("/tmp/notes.md", "src/Cloud/GarminConnect.cpp",
                     ".claude/workflow-garminconnect/findings.md", "STATE.md"):
            self.assertEqual(offending_paths(event("Write", file_path=path)), [], path)

    def test_ledger_findings_file_is_not_an_exchange_file(self):
        # `findings.md` has no `<unit>_` prefix; the canonical ledger must stay writable.
        self.assertEqual(offending_paths(event("Edit", file_path="/repo/findings.md")), [])

    def test_blocks_a_bash_redirect(self):
        self.assertTrue(offending_paths(
            event("Bash", command="cat <<EOF > /home/andy/gc-insp-exchange/b_report.md")))
        self.assertTrue(offending_paths(
            event("Bash", command="cp out.md ~/gc-insp-exchange/s925_findings.md")))

    def test_allows_reading_a_stray_file(self):
        # Reading one is how you clean it up; only writes are guarded.
        self.assertEqual(offending_paths(
            event("Bash", command="cat /home/andy/gc-insp-exchange/s925_findings.md")), [])

    def test_ignores_unrelated_tools(self):
        self.assertEqual(offending_paths(event("Read", file_path="/tmp/s925_brief.md")), [])


if __name__ == "__main__":
    unittest.main()
