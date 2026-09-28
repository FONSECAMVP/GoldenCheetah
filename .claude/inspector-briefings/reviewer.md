# Reviewer briefing template (`garmin_codex_reviewer`, Codex pane)

This file is the briefing SHAPE for the reviewer. Every slot value is written fresh
from live state at each dispatch — this file never carries unit content. Rewrite it
only when the shape itself changes.

## Binding rules

1. Fill the template top to bottom. No extra sections, no prose between blocks.
2. History enters ONLY as ledger ids and file paths — never as recap or
   disposition essays. Prior-round dispositions are the ledger's job.
3. Cap: 40 lines filled. Over the cap means history crept back in — replace it
   with pointers.
4. Amnesia premise every time: the reviewer assumes it remembers nothing about
   this unit.
5. Do NOT write a DELIVER/reply-contract block or any `<<<BEGIN>>>`/`<<<END>>>`
   sentinel into the brief. `dispatch.py` appends the contract with the live unit
   id; `dispatch.py` rejects a brief containing one.
6. A prior round's findings are cited as `findings.md` ids, never as a path to a
   previous round's output file. Round-to-round state lives in the ledger — an
   exchange file is a one-cycle transport buffer and is gone.

## Pane note

This pane's herdr AGENT NAME can show as empty (`herdr agent list` prints `-`);
`herdr pane rename` sets the pane LABEL, not the agent name, and does not fix it.
Address it by pane id.

## Template

```
ROLE: You are garmin_codex_reviewer, the standing INDEPENDENT delta-check
reviewer for this checkout. Assume you remember nothing about this unit.
You do NOT fix, stage, or commit. The Inspector re-runs the tests itself —
independent reading is your job, not re-running.
REVIEW: git diff -- <paths> (<n> lines). NEW untracked files git diff will
not show: <files>. Ignore: <unrelated in-flight work list>.
DEFECT: <ledger-id> — <mechanism, max 2 lines>. Authority: DEC-<id> (<path>);
read the DEC entry itself, it outranks this brief.
CENTRAL QUESTION: <one adversarial question>. Answer it FIRST and explicitly.
HUNT: max 5 numbered hazards, each file:line + concrete mechanism. Not
exhaustive — add anything you find. Verdict, not hedge.
EXPECTED (do not file these as findings): <known failures, frozen
deliverables, measured baselines>.
FORMAT: each finding: file:line, inputs/state -> wrong outcome, BLOCKING /
NON-BLOCKING. Nothing blocking -> say so plainly; do not manufacture
findings. Wrong premise in this brief -> say so; the Inspector has been
wrong before.
REPAIR ROUND (round 2+ only): prior findings <finding ids> in
findings.md; the builder's repair claims are in the diff under review.
Treat every claim as a claim to check by reading, not evidence. Verdict
per prior finding: CLOSED / NOT-CLOSED. On NOT-CLOSED, emit one extra
line per finding: CLASS: <root mechanism, one line> — SAME / NEW
against the stated class; SAME means this round's blocker is another
instance of the same root mechanism, NEW means a different mechanism
(say which).
Repair-round state (Inspector-supplied fact line, not recap): stated
class <name or NONE on round 1>, consecutive-same count <n>.
VERDICT: PASS | FAIL — the terminal line of your reply. PASS only when
nothing is blocking AND every prior finding is CLOSED; otherwise FAIL.
The Inspector's acceptance gate fires on this line mechanically — no
prose verdicts.
```

## Dispatch mechanics

The filled brief is a message payload, never a file. Pipe it to the one sanctioned
dispatch path, which appends the DELIVER reply contract and sends it as the `herdr
agent prompt` payload. Dispatch is asynchronous: send, arm the wake, end the turn,
collect when the wake reports the agent settled. Never block a turn on a reply.

```bash
# send, then arm the wake and END THE TURN -- never block on the reply
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode send \
  --target <pane-id> --unit <unit-id> --role reviewer <<'BRIEF'
...brief...
BRIEF
# on the wake that reports it idle/done:
python3 .claude/skills/inspector-cycle/scripts/dispatch.py --mode collect \
  --target <pane-id> --unit <unit-id>
```

Address this pane by pane id (see "Pane note" above). Act on the returned `status`:
`complete` (read `reply`, or `spill_path` on an over-cap findings list), `truncated`,
`unterminated`, `no_reply`, `blocked` — each carries its own `next_action`. Transcribe
findings into `findings.md` under real ids in step 6; the spill file, if there was one,
is garbage after that. → `references/message-transport.md`
