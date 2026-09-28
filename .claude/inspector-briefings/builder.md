# Builder briefing template (`garmin_builder_stage<N>_vN`, Claude Code pane)

This file is the briefing SHAPE for the builder. Every slot value is written fresh
from live state at each dispatch — this file never carries unit content. Rewrite it
only when the shape itself changes.

## Binding rules

1. Fill the template top to bottom. No extra sections, no prose between blocks.
2. History enters ONLY as ledger ids and file paths — never as recap, reasoning,
   or round-by-round narrative.
3. No lessons block. A lesson that must reach the builder is a lesson row in
   `findings.md`, reached through AUTHORITY or SETTLED ids.
4. Cap: 50 lines filled. Over the cap means history crept back in — replace it
   with pointers, don't compress the type.
5. The builder is usually COLD (soft-landing restarts it from zero). The brief
   must work standalone: no "as previously", no "as you know".
6. Do NOT write a DELIVER/reply-contract block or any `<<<BEGIN>>>`/`<<<END>>>`
   sentinel into the brief. `dispatch.py` appends the contract with the live unit
   id; a hand-written one duplicates it, can carry a stale id, and spends the
   50-line budget on boilerplate. `dispatch.py` rejects a brief containing one.

## Cold-start check (before the FIRST dispatch to a newly started pane)

`herdr pane read <pane> --source visible --lines 6` must show `auto mode on`
and Sonnet; `tok 0k/0k` confirms genuinely fresh. Verify BEFORE dispatching —
a pane one step short of auto stalls on its first Bash dialog with nobody to
click through.

## Pane note

This pane's herdr AGENT NAME can show as empty (`herdr agent list` prints `-`),
same as the reviewer pane. Address it by pane id if the name does not resolve.

## Template

```
ROLE: You are <name>, the builder. Cold start, no memory.
TASK: <ledger-id> — <defect as a mechanism, max 3 lines, stated so the
builder can judge the brief rather than follow it>.
AUTHORITY: DEC-<id> (<path to decisions.md>). Read it first; it outranks
this brief.
STATE (verify with git status / git diff — do not trust this list):
<builder's own dirty paths only>. Ignore all other in-flight work; never
`git add -A`.
GROUND TRUTH (Inspector-verified personally; re-verify before relying on
any of it): <max 3 facts — real command output, probe paths left on disk>.
DO: numbered items, each with file:line, required direction, and
BLOCKING / NON-BLOCKING.
PROVE IT: per DO item, the exact mutation that must go RED, and the
side-effect assertion that must hold.
SETTLED: <finding/DEC ids only> — do not redo, do not re-open.
PATHS: allowed <exhaustive list>. NOT <list>.
HARD HOLD: <frozen files + one-line reason>. CMake + ctest against ./build
ARE allowed unless stated otherwise here.
REPORT: run <test subsets only — the Inspector runs the full gate once, at
reviewer acceptance>. GREEN-or-blocked. List changed paths, untracked paths,
and diff size — the Inspector briefs the reviewer from this list and never
opens the diff itself. Do NOT stage, do NOT commit. Report and stop.
RULE: every claim in this brief is verify-don't-trust. Reasoning lives in
the ledgers — cite by id, never into source comments.
COMMENTS: the default is NONE. Write one only where code is non-obvious:
one line, why-only — a DEC/REQ id pointer, a subtle-invariant warning, or a
one-line why for a surprising choice. Never a function/file/class summary,
never restate what the next lines do, never history or round narrative,
never a checkable fact (that becomes an assertion or a test). If a comment
needs a second line, rename the code or delete the comment. On touched
lines, delete stale comments — never correct a comment.
```

## Dispatch mechanics

The filled brief is a message payload, never a file. Pipe it to the one sanctioned
dispatch path, which appends the DELIVER reply contract and sends it as the `herdr
agent prompt` payload. Dispatch is asynchronous: send, arm the wake, end the turn,
collect when the wake reports the agent settled. Never block a turn on a reply.

```bash
# send, then arm the wake and END THE TURN -- never block on the reply
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode send \
  --target <name-or-pane> --unit <unit-id> --role builder <<'BRIEF'
...brief...
BRIEF
# on the wake that reports it idle/done:
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode collect \
  --target <name-or-pane> --unit <unit-id>
```

Act on the returned `status`: `complete` (read `reply`, or `spill_path` if the builder
spilled), `truncated`, `unterminated`, `no_reply`, `blocked` — each carries its own
`next_action`. Do not write the brief to a path, and do not hand-roll `herdr agent
prompt` for it: there is no `--file` flag in 0.8.0, and shell-quoting a 50-line payload
is exactly the hazard `dispatch.py` exists to remove. → `references/message-transport.md`
