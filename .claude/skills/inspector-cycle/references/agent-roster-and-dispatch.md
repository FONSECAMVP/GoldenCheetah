# Agent roster and dispatch

## The 3 standing herdr-level agents (outer layer)

| Role | Runtime | Job | Typical name pattern |
|---|---|---|---|
| builder | Claude Code | TDD implementation, one REQ at a time | `garmin_builder_stageN` |
| reviewer | Codex | delta-check / re-review of new or fixed code | `garmin_codex_reviewer` |
| investigator | Codex | isolated, read-only, parallel problem-solving | ad hoc, e.g. `pch_investigator` |

There is deliberately no fourth standing "token-monitor" agent. Context usage is a
deterministic file read (see "Context-usage reads (no monitor agent)" below), not work that
justifies spending a whole supervised LLM's own context just to repeat a number back to
the Inspector — the Inspector runs the read-only scripts itself.

These are separate long-lived herdr panes/processes, not to be confused with QGDW's own
in-process Task-tool subagents (`qgdw-builder`, `qgdw-adversary`, `qgdw-scout`,
`qgdw-librarian`, `qgdw-validator` — defined in `.claude/skills/quality-gated-dev-workflow/
agents/`). Any of the 3 herdr-level agents may spawn QGDW subagents internally to scope a
piece of technical work; that nesting belongs to the agent doing it, not the Inspector.
Only the outer pane's own report is the Inspector's concern.

Always re-verify actual identity/topology fresh each cycle — herdr agent names, panes, and
even the Inspector's own cross-session identity can drift or reset silently between turns.
Don't dispatch to a name you remember without confirming it still resolves to the agent you
think it is.

## Briefing rule

Before sending a NEW task to an existing pane, find that pane's own FIRST-ever prompt
(scroll back past all later recaps) and mirror its structure — task framing, pre-researched
context with file:line references flagged "verify before trusting," a numbered lessons list,
explicit governance/scope guardrails, "report back GREEN-or-blocked, don't commit." Don't
paraphrase from memory of what the shape "roughly" was. If the pane's scrollback genuinely
doesn't go back that far (a topology reset, lost history), say so explicitly in the new
briefing and build it from this same shape from scratch — don't stall waiting for
unrecoverable history.

## Reviewer-specific discipline

**Baseline, every REQ, no exceptions:** before the Inspector does its own rebuild/rerun
verification in step 5, dispatch the reviewer for a delta-check on the builder's actual diff.
This applies to every builder GREEN report — logging, i18n, docs, config, anything — not
just C++/UI code. The Inspector's own re-verification is a supplement to the reviewer's
independent read, never a substitute for it; skipping the dispatch because a REQ "looks
simple" is the exact failure mode this line exists to prevent.

**Escalation for lifetime-safety code:** for new C++ UI/lifetime-safety code (raw pointers,
QPointer guards, nested-event-loop reentrancy), one clean delta-check pass is NOT enough —
re-dispatch a tight, scoped confirmation pass after EVERY fix round that touches
pointer/lifetime logic. Each pass has independently found real, distinct bugs the previous
one missed.

## Investigator-specific discipline

Dispatch only when the builder is genuinely iterating/struggling on a non-trivial problem
(multiple fix attempts, not just "taking a while" on a routine long build). Isolation is
mandatory: it builds a minimal throwaway reproduction, never touches or runs a build in the
real working tree (a concurrent build there will race/corrupt). Give it the same problem
statement the builder has, plus instructions to verify against real generated artifacts
(a Makefile, a compiler invocation), not the builder's own code-comment claims. The two
agents don't know about each other — the Inspector must manually relay the investigator's
finding into the builder's pane, then have the builder adopt/verify it. Still independently
re-run the SPECIFIC targeted test/mutation for the adopted fix yourself — don't just trust
either agent's report. Don't also re-run the full gate suite here; that runs once per
commit-ready handoff (the project's own lean-evidence protocol), not once per agent that
touches the fix.

## Context-usage reads (no monitor agent)

Reading current context usage (not cumulative totals) of the 3 agents AND the Inspector's
own pane is narrow and cheap enough that it does not need a dedicated LLM agent — the
Inspector runs the read-only scripts itself, directly, every poll tick. This is a raw
numeric file read, not a work-correctness claim — doing it yourself does not conflict with
"never trust a self-report," which is about validating WORK, not reading a token count.

**Read-only measurements:** read `token-budget-and-soft-landing.md` → "Measuring
current context" before the first poll — that section is the measurement contract. Do not
send `/status`, `/context`, prompts, or Enter to any pane to collect telemetry. Resolve
each pane's live PID fresh on every tick (never from a remembered session), then run:
- `scripts/claude_context.py --pid <pid>` for Claude Code panes (builder, and the
  Inspector's own pane — pass `--threshold 210000` for the Inspector's own pane).
- `scripts/codex_context.py --pid <pid>` for Codex panes (reviewer, investigator).

For each of the 4 numbers (builder, reviewer, investigator, yourself), track role, pane,
session/PID when available, used tokens, sample time, threshold, and `below_threshold` /
`warn` / `unknown`. Missing readings remain explicit unknowns; `idle` says nothing about
context size. A worker at **>=250000** starts that agent's soft-landing procedure; your own
threshold is **>=210000**. Keep reading the other panes while one agent is being refreshed.
