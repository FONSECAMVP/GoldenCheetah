#!/usr/bin/env python3
"""One-shot Linux Codex context read. Never sends input or restarts an agent."""
import argparse
import json
from pathlib import Path


class Unavailable(ValueError):
    pass


def rollout_for_pid(pid, proc_root=Path("/proc")):
    process = proc_root / str(pid)
    if process.joinpath("comm").read_text().strip() != "codex":
        raise Unavailable("target PID is not Codex")
    paths = set()
    for fd in process.joinpath("fd").iterdir():
        try:
            path = fd.resolve(strict=True)
        except FileNotFoundError:
            continue
        if path.name.startswith("rollout-") and path.suffix == ".jsonl":
            paths.add(path)
    if len(paths) != 1:
        raise Unavailable("no unique open rollout; session may be new or changing")
    return paths.pop()


def read_usage(path, threshold=250_000):
    # Bound work per poll. A record outside this tail is unavailable, never guessed.
    with path.open("rb") as stream:
        meta = json.loads(stream.readline())
        if meta.get("type") != "session_meta" or not meta.get("payload", {}).get("id"):
            raise Unavailable("missing session identity")
        stream.seek(0, 2)
        start = max(0, stream.tell() - 2 * 1024 * 1024)
        stream.seek(start)
        if start:
            stream.readline()  # discard a potentially partial first record
        tail = stream.read()
    if not tail.endswith(b"\n"):
        raise Unavailable("rollout write in progress; retry next tick")
    for line in reversed(tail.splitlines()):
        event = json.loads(line)
        payload = event.get("payload") or {}
        if event.get("type") == "compacted" or payload.get("type") == "context_compacted":
            raise Unavailable("compaction has no subsequent usage sample yet")
        if event.get("type") != "event_msg" or payload.get("type") != "token_count":
            continue
        info = payload.get("info") or {}
        used = (info.get("last_token_usage") or {}).get("total_tokens")
        if type(used) is not int or used < 0:
            raise Unavailable("latest context usage is unavailable")
        return {"status": "warn" if used >= threshold else "below_threshold",
                "used_tokens": used, "threshold": threshold,
                "context_window": info.get("model_context_window"),
                "session_id": meta["payload"]["id"],
                "sample_at": event.get("timestamp"), "source": str(path)}
    raise Unavailable("no recent token_count event")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True, help="live Codex PID from Herdr process-info")
    args = parser.parse_args()
    try:
        path = rollout_for_pid(args.pid)
        result = read_usage(path)
        if rollout_for_pid(args.pid) != path:
            raise Unavailable("session changed during read")
    except (OSError, ValueError, TypeError, AttributeError) as exc:
        print(json.dumps({"status": "unknown", "used_tokens": None, "reason": str(exc)}))
        return 2
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
