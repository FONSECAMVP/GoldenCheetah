#!/usr/bin/env python3
"""PreToolUse guard: an exchange file may only ever exist in /tmp/insp-exchange/.

Prose rules get improvised around -- an invented `~/gc-insp-exchange/` is exactly
what that looks like. This makes the rule mechanical for the Inspector and for any
supervised Claude Code pane that inherits the project settings.

Blocks a Write/Edit/NotebookEdit whose target, or a Bash command whose write
operation, names a `*_brief.md` / `*_review_brief.md` / `*_report.md` /
`*_findings.md` path outside the one sanctioned spill directory.

Exit 2 blocks the call and feeds stderr back as the reason; exit 0 allows.
See references/message-transport.md -> "Enforcement".
"""
import json
import re
import sys

SPILL_DIR = "/tmp/insp-exchange"
EXCHANGE_BASENAME_RE = re.compile(r"[\w.-]+_(?:brief|review_brief|report|findings)\.md$")
EXCHANGE_PATH_IN_TEXT_RE = re.compile(
    r"(?<![\w/])((?:~|/|\./)[\w./-]*[\w.-]+_(?:brief|review_brief|report|findings)\.md)\b")
# Only writes are guarded; reading a legacy stray file is how you clean one up.
BASH_WRITE_RE = re.compile(r"(>>?|\btee\b|\bcp\b|\bmv\b|\bdd\b)")

REASON = (
    "Blocked: `{path}` is an exchange file outside {spill}.\n"
    "A brief is a message payload -- send it as the `herdr agent prompt` payload via "
    "scripts/dispatch.py, never as a file an agent is told to read.\n"
    "A reply spills to a file only when it exceeds its line cap, and only to the path "
    "the brief declared under {spill}.\n"
    "Do not relocate the path to satisfy this hook: fix the transport "
    "(references/message-transport.md)."
)


def allowed(path):
    expanded = path.replace("~", "/home/_user")  # cheap: ~ is never the spill dir
    return expanded.startswith(SPILL_DIR + "/")


def offending_paths(event):
    tool = event.get("tool_name", "")
    data = event.get("tool_input") or {}
    if tool in ("Write", "Edit", "NotebookEdit"):
        target = data.get("file_path") or data.get("notebook_path") or ""
        if EXCHANGE_BASENAME_RE.search(target) and not allowed(target):
            return [target]
        return []
    if tool == "Bash":
        command = data.get("command") or ""
        if not BASH_WRITE_RE.search(command):
            return []
        return [m.group(1) for m in EXCHANGE_PATH_IN_TEXT_RE.finditer(command)
                if not allowed(m.group(1))]
    return []


def main():
    try:
        event = json.load(sys.stdin)
    except ValueError:
        return 0  # Never fail a tool call over an unreadable hook payload.
    hits = offending_paths(event)
    if not hits:
        return 0
    print(REASON.format(path=hits[0], spill=SPILL_DIR), file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
