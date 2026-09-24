"""End-to-end test of the dispatch round trip against a stub `herdr` on PATH.

Proves argv construction, the prompt -> get -> read sequence, and classification --
everything except the terminal itself. What this CANNOT prove is whether a live
Claude Code or Codex TUI renders a multi-line bracketed-paste payload faithfully;
only an isolated herdr session shows that (see MIGRATION.md).
"""
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

DISPATCH = str(Path(__file__).resolve().parent / "dispatch.py")
UNIT = "s925-r4"

# Records every invocation to $ARGV_LOG, then replays a canned reply for `read`.
STUB = r"""#!/bin/bash
printf '%s\0' "$@" >> "$ARGV_LOG"; printf -- '---\0' >> "$ARGV_LOG"
case "$2" in
  prompt) echo '{"result":{"ok":true}}' ;;
  get)    echo "{\"result\":{\"agent\":{\"agent_status\":\"${AGENT_STATUS:-idle}\"}}}" ;;
  read)   cat "$REPLY_FIXTURE" ;;
esac
"""


class DispatchCliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        root = Path(self.temp.name)
        stub = root / "herdr"
        stub.write_text(STUB)
        stub.chmod(0o755)
        self.argv_log = root / "argv.log"
        self.fixture = root / "reply.txt"
        self.env = {**os.environ,
                    "PATH": f"{root}{os.pathsep}{os.environ['PATH']}",
                    "ARGV_LOG": str(self.argv_log),
                    "REPLY_FIXTURE": str(self.fixture)}

    def run_dispatch(self, brief, reply, *extra, status="idle"):
        self.fixture.write_text(reply)
        self.env["AGENT_STATUS"] = status
        done = subprocess.run(
            [sys.executable, DISPATCH, "--target", "sandbox_reviewer", "--unit", UNIT,
             "--role", "reviewer", "--timeout", "1000", *extra],
            input=brief, capture_output=True, text=True, env=self.env)
        return json.loads(done.stdout), done.returncode

    def invocations(self):
        blobs = self.argv_log.read_bytes().split(b"---\x00")
        return [b.decode().split("\x00")[:-1] for b in blobs if b]

    def test_brief_reaches_herdr_verbatim_through_argv(self):
        # The payload the Inspector wrote must arrive byte-for-byte: no shell, so
        # backticks, quotes, $ and newlines are all inert.
        brief = ("ROLE: reviewer\n"
                 "REVIEW: `git diff -- src/*.py` — watch $HOME and \"quoted\" paths\n"
                 "CENTRAL QUESTION: can it raise?\n")
        reply = f"noise\n<<<BEGIN {UNIT}>>>\nBLOCKING sync_client.py:18\n<<<END {UNIT}>>>\n"
        result, code = self.run_dispatch(brief, reply)
        self.assertEqual(code, 0)
        prompt_call = self.invocations()[0]
        self.assertEqual(prompt_call[:3], ["agent", "prompt", "sandbox_reviewer"])
        self.assertTrue(prompt_call[3].startswith(brief))
        self.assertIn(f"<<<END {UNIT}>>>", prompt_call[3])   # contract appended
        self.assertEqual(prompt_call[4:], ["--wait", "--timeout", "1000"])

    def test_sequence_is_prompt_then_get_then_read(self):
        reply = f"<<<BEGIN {UNIT}>>>\nclean\n<<<END {UNIT}>>>\n"
        self.run_dispatch("ROLE: reviewer\n", reply)
        self.assertEqual([call[1] for call in self.invocations()], ["prompt", "get", "read"])

    def test_complete_reply_is_returned(self):
        reply = f"<<<BEGIN {UNIT}>>>\nBLOCKING sync_client.py:18 -- ValueError escapes\n<<<END {UNIT}>>>\n"
        result, code = self.run_dispatch("ROLE: reviewer\n", reply)
        self.assertEqual((result["status"], code), ("complete", 0))
        self.assertEqual(result["reply"], "BLOCKING sync_client.py:18 -- ValueError escapes")
        self.assertIsNone(result["spill_path"])

    def test_spill_reply_returns_the_path_not_the_body(self):
        reply = f"<<<BEGIN {UNIT}>>>\n/tmp/insp-exchange/{UNIT}.md\n<<<END {UNIT}>>>\n"
        result, _ = self.run_dispatch("ROLE: reviewer\n", reply)
        self.assertEqual(result["spill_path"], f"/tmp/insp-exchange/{UNIT}.md")
        self.assertEqual(result["reply"], "")

    def test_lost_head_reports_truncated_with_a_recovery_action(self):
        result, _ = self.run_dispatch("ROLE: reviewer\n", f"...tail...\n<<<END {UNIT}>>>\n")
        self.assertEqual(result["status"], "truncated")
        self.assertIn(f"/tmp/insp-exchange/{UNIT}.md", result["next_action"])

    def test_silent_pane_is_no_reply(self):
        result, _ = self.run_dispatch("ROLE: reviewer\n", "just a shell prompt\n")
        self.assertEqual(result["status"], "no_reply")

    def test_send_returns_without_waiting_for_the_reply(self):
        # The routine path: the Inspector must be able to end its turn with a wake
        # armed instead of blocking in a subprocess while one agent works.
        result, code = self.run_dispatch("ROLE: reviewer\n", "", "--mode", "send",
                                         status="working")
        self.assertEqual((result["status"], code), ("sent", 0))
        self.assertIn("--mode collect", result["next_action"])
        prompt_call = self.invocations()[0]
        self.assertEqual(prompt_call[4:], ["--wait", "--until", "working",
                                           "--timeout", "10000"])
        self.assertNotIn("read", [call[1] for call in self.invocations()])

    def test_send_still_appends_the_contract(self):
        self.run_dispatch("ROLE: reviewer\n", "", "--mode", "send", status="working")
        self.assertIn(f"<<<END {UNIT}>>>", self.invocations()[0][3])

    def test_send_validates_before_sending(self):
        result, code = self.run_dispatch(
            f"ROLE: reviewer\nRead /tmp/{UNIT}_brief.md and do it.\n", "",
            "--mode", "send")
        self.assertEqual((result["status"], code), ("rejected", 2))
        self.assertFalse(self.argv_log.exists())

    def test_collect_sends_no_input(self):
        reply = f"<<<BEGIN {UNIT}>>>\nBLOCKING sync_client.py:18\n<<<END {UNIT}>>>\n"
        self.fixture.write_text(reply)
        self.env["AGENT_STATUS"] = "idle"
        done = subprocess.run(
            [sys.executable, DISPATCH, "--mode", "collect",
             "--target", "sandbox_reviewer", "--unit", UNIT],
            capture_output=True, text=True, env=self.env)
        result = json.loads(done.stdout)
        self.assertEqual(result["status"], "complete")
        self.assertEqual(result["reply"], "BLOCKING sync_client.py:18")
        self.assertEqual([call[1] for call in self.invocations()], ["get", "read"])

    def test_collect_on_a_still_working_pane_is_not_a_failure(self):
        # An absent sentinel mid-turn says nothing yet; re-collect on the next wake.
        self.fixture.write_text("thinking...\n")
        self.env["AGENT_STATUS"] = "working"
        done = subprocess.run(
            [sys.executable, DISPATCH, "--mode", "collect",
             "--target", "sandbox_reviewer", "--unit", UNIT],
            capture_output=True, text=True, env=self.env)
        self.assertEqual(json.loads(done.stdout)["status"], "still_working")
        self.assertEqual(done.returncode, 0)

    def test_collect_does_not_require_a_role(self):
        self.fixture.write_text(f"<<<BEGIN {UNIT}>>>\nx\n<<<END {UNIT}>>>\n")
        done = subprocess.run(
            [sys.executable, DISPATCH, "--mode", "collect",
             "--target", "sandbox_reviewer", "--unit", UNIT],
            capture_output=True, text=True, env=self.env)
        self.assertEqual(done.returncode, 0, done.stderr)

    def test_rejected_brief_never_reaches_herdr(self):
        result, code = self.run_dispatch(
            f"ROLE: reviewer\nRead /tmp/{UNIT}_brief.md and do it.\n", "")
        self.assertEqual((result["status"], code), ("rejected", 2))
        self.assertFalse(self.argv_log.exists(), "herdr was invoked despite rejection")


if __name__ == "__main__":
    unittest.main()
