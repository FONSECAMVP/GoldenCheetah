#!/usr/bin/env python3
"""Brief dispatch over herdr's message channel. Never writes a brief to disk.

Sends a brief as an `herdr agent prompt` payload and classifies the reply read back
off the pane. The brief is passed as an argv element with no shell in between, so
arbitrary markdown (backticks, quotes, `$`) is safe -- herdr 0.8.0 has no `--file`
flag on `agent prompt`, and hand-rolled shell quoting of a 50-line payload is what
pushed this package to file-drop in the first place.

Three modes, and the choice matters to supervision, not just ergonomics:

  --mode send      fire, return as soon as the pane starts working. THE ROUTINE PATH.
  --mode collect   read a settled pane and classify. No input sent.
  --mode roundtrip send and block until the pane settles. Default for compatibility,
                   but it stalls the Inspector's whole turn for as long as the agent
                   takes -- every other pane goes unsupervised meanwhile. Use it only
                   for a reply you expect in seconds.

The supervision loop is asynchronous by design: dispatch with `send`, arm
`insp_wake.sh`, end the turn, and `collect` on the wake that reports the agent
idle/done. See references/message-transport.md.

Exit 0 = classified (read `status`), exit 2 = rejected, send failed, or unreadable.
"""
import argparse
import json
import re
import subprocess
import sys
import time

# Brief line caps, per agent-roster-and-dispatch.md -> "Briefing rule", rule 4.
# Checked against the Inspector's own lines, before the contract is appended.
LINE_CAPS = {"builder": 50, "reviewer": 40, "adhoc": 30}

# Appended to every brief, never hand-written into a template: it is identical each
# dispatch, so making the Inspector retype it would spend the brief's line budget on
# boilerplate (rule 2) and leave the unit id free to drift out of sync with --unit.
# Both markers stay INLINE in this sentence; only the agent's emission is alone on a
# line, which is exactly what lets classify() tell the reply from the prompt echo.
CONTRACT = (
    "DELIVER: reply in this pane. Open with <<<BEGIN {unit}>>> alone on a line and "
    "close with <<<END {unit}>>> alone on a line. Keep it under {cap} lines -- verdict "
    "first, then file:line + mechanism per item; reasoning belongs in the ledgers, "
    "cited by id. If it will not fit, write the full text to {spill}/{unit}.md and "
    "reply with ONLY that path between the markers."
)

# The only directory a spill file may live in. Anything matching an exchange
# basename outside it means a brief or a reply was about to become a record.
SPILL_DIR = "/tmp/insp-exchange"
EXCHANGE_PATH_RE = re.compile(
    r"(?<![\w/])(/[\w./-]*?)([\w.-]+_(?:brief|review_brief|report|findings)\.md)\b"
)


class Rejected(ValueError):
    pass


def marker_line_re(unit, word):
    # Deliberately anchored to a WHOLE line. The brief's own DELIVER block names
    # both markers inline inside a sentence ("Open with <<<BEGIN u>>> and close
    # with <<<END u>>>"), so the echoed prompt never matches and only the agent's
    # real emission does. Never reformat a template to put a marker alone on its
    # own line: that silently makes the echo indistinguishable from the reply.
    # The optional list-marker prefix is required for Codex panes: that TUI renders
    # a reply as a bulleted list, emitting "- <<<BEGIN u>>>" while the closing
    # marker often renders bare -- which classified EVERY Codex reply as `truncated`
    # (ends-without-begins) and discarded real verdicts. The trailing `$` is what
    # keeps the echoed DELIVER sentence out, so the prefix costs nothing.
    return re.compile(
        rf"^[ \t]*(?:[-*•>]+[ \t]*)?<<<{word} {re.escape(unit)}>>>[ \t]*$", re.M
    )


def validate_brief(text, unit, role):
    lines = text.splitlines()
    cap = LINE_CAPS[role]
    if len(lines) > cap:
        raise Rejected(f"brief is {len(lines)} lines, cap for {role} is {cap}; "
                       "over the cap means history crept back in -- replace it with pointers")
    if not text.strip():
        raise Rejected("brief is empty")

    for match in EXCHANGE_PATH_RE.finditer(text):
        path = match.group(0)
        if not path.startswith(SPILL_DIR + "/"):
            raise Rejected(
                f"brief references an exchange file outside {SPILL_DIR}: {path}. "
                "A brief is a message payload, never a file, and a reply spills only "
                "to the declared path (references/message-transport.md)")

    # The contract is appended, not authored. A hand-written one duplicates it and can
    # carry a stale unit id; a sentinel alone on a line would make the prompt echo
    # indistinguishable from the reply.
    if "<<<BEGIN" in text or "<<<END" in text:
        raise Rejected("brief writes its own reply sentinels; remove them -- dispatch.py "
                       "appends the DELIVER contract with the --unit id itself")


def herdr(args, timeout_s):
    # argv list, shell=False: the payload is never parsed by a shell.
    done = subprocess.run(["herdr", *args], capture_output=True, text=True, timeout=timeout_s)
    return done.returncode, done.stdout, done.stderr


def agent_status(target, timeout_s):
    code, out, _ = herdr(["agent", "get", target], timeout_s)
    if code != 0:
        return "unknown"
    try:
        return json.loads(out)["result"]["agent"]["agent_status"]
    except (ValueError, KeyError, TypeError):
        return "unknown"


def classify(transcript, unit):
    begins = [m.end() for m in marker_line_re(unit, "BEGIN").finditer(transcript)]
    ends = [m.start() for m in marker_line_re(unit, "END").finditer(transcript)]
    if not begins and not ends:
        return "no_reply", ""
    if ends and not begins:
        # Head scrolled off the alternate screen; those rows never enter herdr's
        # host scrollback, so a larger --lines cannot recover them.
        return "truncated", ""
    if begins and not ends:
        return "unterminated", ""
    last_end = ends[-1]
    prior = [b for b in begins if b < last_end]
    if not prior:
        return "truncated", ""
    return "complete", transcript[prior[-1]:last_end].strip()


def spill_path_of(body):
    lines = [ln for ln in body.splitlines() if ln.strip()]
    if len(lines) == 1 and lines[0].strip().startswith(SPILL_DIR + "/"):
        return lines[0].strip()
    return None


def collect_reply(args):
    """Read a settled pane and classify what the agent said. No input is sent."""
    settled = agent_status(args.target, 30)
    code, transcript, err = herdr(["agent", "read", args.target, "--source",
                                   "recent-unwrapped", "--lines", str(args.lines),
                                   "--format", "text"], 60)
    if code != 0:
        return {"status": "read_failed", "reason": err.strip()[:400],
                "settled": settled}, 2

    status, body = classify(transcript, args.unit)
    if settled == "working" and status != "complete":
        # Still mid-turn; an absent sentinel says nothing yet. Not a failure.
        return {"status": "still_working", "settled": settled,
                "next_action": "agent has not finished; re-collect on the next wake"}, 0
    if settled == "blocked" and status != "complete":
        # A dialog is not a reply. Resolve it before assuming anything was said.
        status = "blocked"
    result = {"status": status, "settled": settled}
    if status == "complete":
        spill = spill_path_of(body)
        result["spill_path"] = spill
        result["reply"] = body if spill is None else ""
    else:
        result["next_action"] = {
            "truncated": f"re-prompt: write the previous reply verbatim to "
                         f"{SPILL_DIR}/{args.unit}.md and reply with only that path",
            "unterminated": "reply started but never closed; check whether the agent is "
                            "still working before re-prompting",
            "no_reply": "no sentinel in the read; diagnose with `herdr agent get`, "
                        "do not re-send blind",
            "blocked": "read --source visible and resolve the dialog first",
        }[status]
    return result, 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True, help="herdr agent name or pane id")
    parser.add_argument("--unit", required=True, help="unit id; must match the brief's sentinels")
    parser.add_argument("--role", choices=sorted(LINE_CAPS),
                        help="required for send/roundtrip; unused by collect")
    parser.add_argument("--mode", choices=("send", "collect", "roundtrip"),
                        default="roundtrip",
                        help="send: fire and return once the pane starts working "
                             "(the routine path). collect: read a settled pane. "
                             "roundtrip: both, blocking until it settles.")
    parser.add_argument("--ack-timeout", type=int, default=10_000,
                        help="send mode: ms to wait for the pane to start working")
    parser.add_argument("--timeout", type=int, default=900_000,
                        help="roundtrip mode: ms to wait for the pane to settle")
    parser.add_argument("--lines", type=int, default=200, help="rows to read back")
    parser.add_argument("--reply-cap", type=int, default=80,
                        help="reply lines before the agent must spill to a file")
    parser.add_argument("--dry-run", action="store_true",
                        help="validate, print the exact payload, and exit without sending")
    args = parser.parse_args(argv)
    if args.mode != "collect" and not args.role:
        parser.error("--role is required for send/roundtrip")

    emit = lambda payload_: (json.dump(payload_, sys.stdout, indent=2), print())
    base = {"target": args.target, "unit": args.unit, "mode": args.mode,
            "sample_at": time.strftime("%Y-%m-%dT%H:%M:%S%z")}

    if args.mode == "collect":
        result, code = collect_reply(args)
        emit({**base, **result})
        return code

    brief = sys.stdin.read()
    try:
        validate_brief(brief, args.unit, args.role)
    except Rejected as exc:
        emit({"status": "rejected", "reason": str(exc)})
        return 2
    payload = brief.rstrip("\n") + "\n" + CONTRACT.format(
        unit=args.unit, cap=args.reply_cap, spill=SPILL_DIR)
    if args.dry_run:
        emit({**base, "status": "valid", "brief_lines": len(brief.splitlines()),
              "cap": LINE_CAPS[args.role], "payload": payload})
        return 0

    if args.mode == "send":
        # Return as soon as the pane STARTS working, not when it settles. The
        # Inspector's turn must end with a wake armed, not sit in a subprocess:
        # a blocking dispatch stalls supervision of every other pane for as long
        # as one agent takes. Collect on the wake that reports it idle/done.
        code, _, err = herdr(["agent", "prompt", args.target, payload, "--wait",
                              "--until", "working", "--timeout", str(args.ack_timeout)],
                             args.ack_timeout / 1000 + 30)
        if code != 0:
            emit({**base, "status": "send_failed", "reason": err.strip()[:400],
                  "observed": agent_status(args.target, 30)})
            return 2
        emit({**base, "status": "sent", "observed": agent_status(args.target, 30),
              "next_action": f"arm the wake, end the turn, then on idle/done run: "
                             f"dispatch.py --mode collect --target {args.target} "
                             f"--unit {args.unit}"})
        return 0

    # roundtrip: blocks until the pane settles. Only for a reply you cannot
    # proceed without and that you expect in seconds, never routine dispatch.
    code, _, err = herdr(["agent", "prompt", args.target, payload,
                          "--wait", "--timeout", str(args.timeout)],
                         args.timeout / 1000 + 30)
    if code != 0:
        # agent_prompt_stalled / timeout / unresolved target all land here.
        emit({**base, "status": "send_failed", "reason": err.strip()[:400],
              "observed": agent_status(args.target, 30)})
        return 2
    result, rc = collect_reply(args)
    emit({**base, **result})
    return rc


if __name__ == "__main__":
    sys.exit(main())
