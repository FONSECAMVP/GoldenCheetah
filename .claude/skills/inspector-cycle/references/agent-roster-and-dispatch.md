# Agent roster and dispatch

## The 4 standing herdr-level agents (outer layer)

| Role | Runtime | Job | Typical name pattern |
|---|---|---|---|
| builder | Claude Code | TDD implementation, one REQ at a time | `garmin_builder_stageN` |
| reviewer | Codex | delta-check / re-review of new or fixed code | `garmin_codex_reviewer` |
| investigator | Codex | isolated, read-only, parallel problem-solving | ad hoc, e.g. `pch_investigator` |
| token-monitor | Claude Code | watches the other 3's context usage, reports to Inspector | `garmin_token_monitor` |

These are separate long-lived herdr panes/processes, not to be confused with QGDW's own
in-process Task-tool subagents (`qgdw-builder`, `qgdw-adversary`, `qgdw-scout`,
`qgdw-librarian`, `qgdw-validator` — defined in `.claude/skills/quality-gated-dev-workflow/
agents/`). Any of the 4 herdr-level agents may spawn QGDW subagents internally to scope a
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

For new C++ UI/lifetime-safety code (raw pointers, QPointer guards, nested-event-loop
reentrancy): one clean delta-check pass is NOT enough. Re-dispatch a tight, scoped
confirmation pass after EVERY fix round that touches pointer/lifetime logic — each pass has
independently found real, distinct bugs the previous one missed. Actually dispatch the
reviewer; don't let the Inspector's own re-verification (rebuild + rerun) substitute for a
second agent's independent read — they catch different classes of bug.

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

## Token-monitor specific notes

Its job is narrow and cheap: read the current context usage (not cumulative totals) of the
other 3 agents AND the Inspector's own pane, and report all 4 numbers to the Inspector at
each poll tick. Give it the Inspector's own pane id explicitly when briefing it — the
Inspector's self-refresh threshold (see `token-budget-and-soft-landing.md`) is otherwise
easy to miss, since nothing else in this cycle checks it externally. This is a raw numeric
read, not a work-correctness claim — delegating it does not conflict with "never trust a
self-report," which is about validating WORK, not reading a status line. The token-monitor
itself is still subject to the same 250k budget and soft-landing procedure as the other 3
(see `token-budget-and-soft-landing.md`) — and while IT is soft-landing, the Inspector reads
all other panes (including its own) directly for that one cycle rather than going without
coverage.

**Reading the actual number, per agent kind — do not estimate from a bare percentage:**
- **Claude Code panes** (builder, Inspector, token-monitor itself): the bottom status line
  already shows a raw figure directly (`tok Nk/Mk`) — use it as-is, no conversion needed.
- **Codex panes** (reviewer, investigator): the persistent bottom status line shows ONLY a
  percentage ("Context 76% used") with no visible denominator — estimating a token count
  from that percentage is unreliable (confirmed 2026-09-12: two Codex panes' real figures,
  read via `/status`, were 199K/258K and 170K/258K — noticeably different from an earlier
  estimate based on the bare percentage alone). Send `/status` to the pane instead
  (`herdr pane send-text <pane> "/status"` then `enter`) and read its own reported
  "Context window: X% left (Y used / Z total)" line — use Y (the actual used count)
  directly, never re-derive it from a percentage.
