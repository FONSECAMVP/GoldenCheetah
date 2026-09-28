# Investigator briefing template (ad-hoc name, e.g. `pch_investigator`, Codex pane)

This file is the briefing SHAPE for the investigator. Every slot value is written fresh
from live state at each dispatch — this file never carries unit content. Rewrite it
only when the shape itself changes.

## Binding rules

1. Fill the template top to bottom. No extra sections, no prose between blocks.
2. History enters ONLY as ledger ids and file paths — never as recap, reasoning, or
   round-by-round narrative.
3. No lessons block. A lesson that must reach the investigator is a lesson row in
   `findings.md`, reached through AUTHORITY or SETTLED ids.
4. Cap: 30 lines filled. Over the cap means history crept back in — replace it with
   pointers, don't compress the type.
5. The investigator is COLD by design (ad-hoc launch, soft-landing restarts it from
   zero). The brief must work standalone: no "as previously", no "as you know".
6. Do NOT write a DELIVER/reply-contract block or any `<<<BEGIN>>>`/`<<<END>>>`
   sentinel into the brief. `dispatch.py` appends the contract with the live unit
   id; a hand-written one duplicates it. `dispatch.py` rejects a brief containing one.

## Cold-start check (before the FIRST dispatch to a newly started pane)

`herdr pane read <pane> --source visible --lines 6` must show the pane settled at
an input prompt. Verify BEFORE dispatching, not after. Launch: `herdr agent start
<name> --kind codex --pane <id>`.

## Template

```
ROLE: You are <name>, the investigator. Cold start, no memory. You are NOT the
builder: you diagnose, you never fix the real tree.
TASK: <the same problem statement the builder has> — <ledger-id>. Authority:
DEC-<id> (<path to decisions.md>).
VERIFY AGAINST: real generated artifacts only (a Makefile, a compiler
invocation, generated output) — never the builder's code-comment claims.
SCOPE: build a minimal throwaway reproduction under /tmp/<scratch-dir>.
Never touch, build, or run builds in the real working tree — a concurrent
build there races/corrupts in-flight work. Read from the tree is fine.
DELIVER: the root mechanism (max 5 lines), the minimal repro that shows it,
and the evidence (command + real output) — not a fix. Cite paths, not prose.
```

## Dispatch mechanics

The filled brief is a message payload, never a file. Pipe it to the one sanctioned
dispatch path, which appends the DELIVER reply contract and sends it as the `herdr
agent prompt` payload. Dispatch is asynchronous: send, arm the wake, end the turn,
collect when the wake reports the agent settled. Never block a turn on a reply.

```bash
# send, then arm the wake and END THE TURN -- never block on the reply
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode send \
  --target <name-or-pane> --unit <unit-id> --role adhoc <<'BRIEF'
...brief...
BRIEF
# on the wake that reports it idle/done:
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode collect \
  --target <name-or-pane> --unit <unit-id>
```

Act on the returned `status`: `complete` (read `reply`, or `spill_path` if it
spilled), `truncated`, `unterminated`, `no_reply`, `blocked` — each carries its own
`next_action`. Do not write the brief to a path. → `references/message-transport.md`

## Restart note

A Codex pane refreshes with `/new` (same process and name); verify the fresh prompt
before the next dispatch.
