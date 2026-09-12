#!/usr/bin/env python3
"""One-shot Linux Claude Code context read. Never sends input or restarts an agent."""
import argparse
import json
import re
from pathlib import Path


class Unavailable(ValueError):
    pass


# A live Claude Code process holds an fd open on its own
# ~/.claude/ide-lock-style task directory, whose path embeds both the
# project's sanitized-cwd segment and the session id verbatim, matching
# ~/.claude/projects/<project_dir>/<session_id>.jsonl exactly. No mtime/cwd
# guessing needed, same spirit as the Codex fd-based rollout lookup.
TASKS_FD_RE = re.compile(r"^/tmp/claude-\d+/([^/]+)/([0-9a-fA-F-]{36})/tasks$")


def transcript_for_pid(pid, proc_root=Path("/proc"), claude_root=Path.home() / ".claude"):
    process = proc_root / str(pid)
    if process.joinpath("comm").read_text().strip() != "claude":
        raise Unavailable("target PID is not Claude Code")
    matches = set()
    for fd in process.joinpath("fd").iterdir():
        try:
            target = fd.readlink()
        except (FileNotFoundError, OSError):
            continue
        m = TASKS_FD_RE.match(str(target))
        if m:
            matches.add((m.group(1), m.group(2)))
    if len(matches) != 1:
        raise Unavailable("no unique session-tasks fd; session may be new or changing")
    project_dir, session_id = matches.pop()
    path = claude_root / "projects" / project_dir / f"{session_id}.jsonl"
    if not path.is_file():
        raise Unavailable("resolved transcript file does not exist")
    return path


def read_usage(path, threshold=250_000):
    # Bound work per poll. A record outside this tail is unavailable, never guessed.
    with path.open("rb") as stream:
        stream.seek(0, 2)
        start = max(0, stream.tell() - 2 * 1024 * 1024)
        stream.seek(start)
        if start:
            stream.readline()  # discard a potentially partial first record
        tail = stream.read()
    if not tail.endswith(b"\n"):
        raise Unavailable("transcript write in progress; retry next tick")
    for line in reversed(tail.splitlines()):
        if not line.strip():
            continue
        event = json.loads(line)
        if event.get("type") == "system" and event.get("subtype") == "compact_boundary":
            raise Unavailable("compaction has no subsequent usage sample yet")
        if event.get("isSidechain") or event.get("type") != "assistant":
            continue
        message = event.get("message")
        usage = message.get("usage") if isinstance(message, dict) else None
        if not isinstance(usage, dict):
            continue
        parts = (usage.get("input_tokens"), usage.get("cache_creation_input_tokens"),
                 usage.get("cache_read_input_tokens"))
        if any(type(p) is not int or p < 0 for p in parts):
            raise Unavailable("latest context usage is unavailable")
        used = sum(parts)
        return {"status": "warn" if used >= threshold else "below_threshold",
                "used_tokens": used, "threshold": threshold,
                "session_id": event.get("sessionId"),
                "sample_at": event.get("timestamp"), "source": str(path)}
    raise Unavailable("no recent usage event")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True, help="live Claude Code PID from Herdr process-info")
    parser.add_argument("--threshold", type=int, default=250_000,
                         help="warn threshold (250000 for a supervised worker, 210000 for the Inspector's own pane)")
    args = parser.parse_args()
    try:
        path = transcript_for_pid(args.pid)
        result = read_usage(path, args.threshold)
        if transcript_for_pid(args.pid) != path:
            raise Unavailable("session changed during read")
    except (OSError, ValueError, TypeError, AttributeError) as exc:
        print(json.dumps({"status": "unknown", "used_tokens": None, "reason": str(exc)}))
        return 2
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
