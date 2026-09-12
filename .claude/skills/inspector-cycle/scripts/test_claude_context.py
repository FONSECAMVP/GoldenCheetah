import json
from pathlib import Path
import tempfile
import unittest
from claude_context import Unavailable, read_usage, transcript_for_pid


def usage(used, cache_read=0, output=500, session_id="session"):
    input_tokens = 2
    cache_creation = max(used - input_tokens - cache_read, 0)
    return {"type": "assistant", "sessionId": session_id, "timestamp": "2026-09-12T10:00:00Z",
            "isSidechain": False, "message": {"role": "assistant", "usage": {
                "input_tokens": input_tokens, "cache_creation_input_tokens": cache_creation,
                "cache_read_input_tokens": cache_read, "output_tokens": output}}}


def sidechain_usage(used):
    event = usage(used)
    event["isSidechain"] = True
    return event


class ContextTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=Path(__file__).parent)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.log = self.root / "session.jsonl"

    def write(self, *events):
        self.log.write_text("".join(json.dumps(e) + "\n" for e in events))

    def test_current_not_cumulative_and_threshold(self):
        for used, status in ((249999, "below_threshold"), (250000, "warn"), (250001, "warn")):
            with self.subTest(used=used):
                self.write(usage(used, cache_read=used - 2))
                result = read_usage(self.log)
                self.assertEqual((result["used_tokens"], result["status"]), (used, status))
                self.assertEqual(result["session_id"], "session")

    def test_inspector_threshold_override(self):
        self.write(usage(215000, cache_read=214998))
        result = read_usage(self.log, threshold=210_000)
        self.assertEqual(result["status"], "warn")

    def test_sidechain_usage_is_ignored(self):
        self.write(sidechain_usage(999_000), usage(40_000, cache_read=39_998))
        result = read_usage(self.log)
        self.assertEqual(result["used_tokens"], 40_000)

    def test_compaction_replaces_old_high_usage(self):
        self.write(usage(260000, cache_read=259998),
                   {"type": "system", "subtype": "compact_boundary"},
                   usage(40000, cache_read=39998))
        self.assertEqual(read_usage(self.log)["used_tokens"], 40000)
        self.write(usage(260000, cache_read=259998), {"type": "system", "subtype": "compact_boundary"})
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_missing_invalid_and_partial_data_are_unknown(self):
        bad_usage = usage(40000, cache_read=39998)
        bad_usage["message"]["usage"]["input_tokens"] = None
        for events in ((), (bad_usage,),
                       ({"type": "assistant", "isSidechain": False, "message": {"role": "assistant"}},)):
            self.write(*events)
            with self.assertRaises(Unavailable):
                read_usage(self.log)
        self.write(usage(40000, cache_read=39998))
        with self.log.open("a") as stream:
            stream.write('{"type":')
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_bounded_tail_uses_latest_sample_or_reports_unknown(self):
        self.write(usage(260000, cache_read=259998),
                   {"type": "attachment", "content": "x" * (3 * 1024 * 1024)},
                   usage(50000, cache_read=49998))
        self.assertEqual(read_usage(self.log)["used_tokens"], 50000)
        self.write(usage(260000, cache_read=259998),
                   {"type": "attachment", "content": "x" * (3 * 1024 * 1024)})
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_pid_mapping_does_not_choose_newest_session(self):
        import os
        process = self.root / "123"
        process.mkdir()
        (process / "comm").write_text("claude\n")
        (process / "fd").mkdir()
        claude_root = self.root / "dotclaude"
        project_dir = claude_root / "projects" / "-my-project"
        project_dir.mkdir(parents=True)
        transcript = project_dir / "abcdefab-1234-1234-1234-1234567890ab.jsonl"
        transcript.write_text(json.dumps(usage(1000)) + "\n")
        with self.assertRaises(Unavailable):
            transcript_for_pid(123, self.root, claude_root)
        # fd target text is matched by shape alone; it need not resolve on disk.
        os.symlink("/tmp/claude-1000/-my-project/abcdefab-1234-1234-1234-1234567890ab/tasks",
                    process / "fd/3")
        self.assertEqual(transcript_for_pid(123, self.root, claude_root), transcript)
        os.symlink("/tmp/claude-1000/-other-project/11111111-1111-1111-1111-111111111111/tasks",
                    process / "fd/4")
        with self.assertRaises(Unavailable):
            transcript_for_pid(123, self.root, claude_root)


if __name__ == "__main__":
    unittest.main()
