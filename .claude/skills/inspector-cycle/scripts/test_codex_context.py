import json
from pathlib import Path
import tempfile
import unittest
from codex_context import Unavailable, read_usage, rollout_for_pid


def usage(used, cumulative=9_000_000):
    return {"type": "event_msg", "timestamp": "2026-09-12T10:00:00Z", "payload": {
        "type": "token_count", "info": {"last_token_usage": {"total_tokens": used},
        "total_token_usage": {"total_tokens": cumulative}, "model_context_window": 258400}}}


class ContextTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(dir=Path(__file__).parent)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.log = self.root / "rollout-session.jsonl"

    def write(self, *events):
        meta = {"type": "session_meta", "payload": {"id": "session"}}
        self.log.write_text("".join(json.dumps(e) + "\n" for e in (meta, *events)))

    def test_current_not_cumulative_and_threshold(self):
        for used, status in ((249999, "below_threshold"), (250000, "warn"), (250001, "warn")):
            with self.subTest(used=used):
                self.write(usage(used))
                result = read_usage(self.log)
                self.assertEqual((result["used_tokens"], result["status"]), (used, status))
                self.assertEqual(result["session_id"], "session")

    def test_compaction_replaces_old_high_usage(self):
        self.write(usage(260000), {"type": "compacted"}, usage(40000))
        self.assertEqual(read_usage(self.log)["used_tokens"], 40000)
        self.write(usage(260000), {"type": "compacted"})
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_missing_invalid_and_partial_data_are_unknown(self):
        for events in ((), (usage(None),), (usage(-1),), (usage(True),),
                       (usage(40000), {"type": "event_msg", "payload": {"type": "token_count", "info": None}})):
            self.write(*events)
            with self.assertRaises(Unavailable):
                read_usage(self.log)
        self.write(usage(40000))
        with self.log.open("a") as stream:
            stream.write('{"type":')
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_bounded_tail_uses_latest_sample_or_reports_unknown(self):
        self.write(usage(260000), {"type": "response_item", "payload": {"text": "x" * (3 * 1024 * 1024)}}, usage(50000))
        self.assertEqual(read_usage(self.log)["used_tokens"], 50000)
        self.write(usage(260000), {"type": "response_item", "payload": {"text": "x" * (3 * 1024 * 1024)}})
        with self.assertRaises(Unavailable):
            read_usage(self.log)

    def test_pid_mapping_does_not_choose_newest_session(self):
        process = self.root / "123"
        process.mkdir()
        (process / "comm").write_text("codex\n")
        (process / "fd").mkdir()
        self.write(usage(40000))
        with self.assertRaises(Unavailable):
            rollout_for_pid(123, self.root)
        (process / "fd/1").symlink_to(self.log)
        self.assertEqual(rollout_for_pid(123, self.root), self.log)
        other = self.root / "rollout-other.jsonl"
        other.write_text("")
        (process / "fd/2").symlink_to(other)
        with self.assertRaises(Unavailable):
            rollout_for_pid(123, self.root)


if __name__ == "__main__":
    unittest.main()
