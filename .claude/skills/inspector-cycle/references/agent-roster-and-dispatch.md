# Agent roster and dispatch

## The 3 standing herdr-level agents (outer layer)

| Role | Runtime | Job | Typical name pattern |
|---|---|---|---|
| builder | Claude Code (Sonnet) | TDD implementation, one REQ at a time | `garmin_builder_stageN` |
| reviewer | Codex | delta-check / re-review of new or fixed code | `garmin_codex_reviewer` |
| investigator | Codex | isolated, read-only, parallel problem-solving | ad hoc, e.g. `pch_investigator` |

There is deliberately no fourth standing "token-monitor" agent. Context usage is a
deterministic file read (see "Context-usage reads (no monitor agent)" below), not work that
justifies spending a whole supervised LLM's own context just to repeat a number back to
the Inspector — the Inspector runs the read-only scripts itself.

These are separate long-lived herdr panes/processes, not to be confused with QGDW's own
in-process Task-tool subagents (`qgdw-builder`, `qgdw-adversary`, `qgdw-scout`,
`qgdw-librarian`, `qgdw-validator` — defined in `.claude/skills/quality-gated-dev-workflow/
agents/`). Spawning those is a Claude Code capability, and the roster is mixed: the
builder (Claude Code) may spawn QGDW subagents internally; the reviewer and investigator
(Codex) cannot — each gets everything it needs in the brief itself, and the Inspector scopes any subagent-grade work before
dispatching to them. Any such nesting belongs to the agent doing it, not the Inspector.
Only the outer pane's own report is the Inspector's concern.

Always re-verify actual identity/topology fresh each cycle — herdr agent names, panes, and
even the Inspector's own cross-session identity can drift or reset silently between turns.
Don't dispatch to a name you remember without confirming it still resolves to the agent you
think it is.

## Pane launch modes (per runtime)

**Codex never writes the shared checkout.** The repo's `.codex/WORKFLOW.md` confines every
Codex write to its own worktree (B-STAGE9-156), so a Codex pane never builds: the reviewer
only reads, the investigator writes only under `/tmp`.

**Codex panes (reviewer, investigator):** launch with `herdr agent start <name> --kind codex
--pane <id>`. There is no `--permission-mode`/
`--model` flag for Codex: set the approval mode once, and it carries over every later
refresh because `/new` resets context without exiting the process
(`token-budget-and-soft-landing.md` → "EXIT and restart cleanly", Codex branch). A fresh
start may open on an "Update available" menu — send `escape` first. Verify
a fresh pane is alive and settled at an input prompt (`herdr pane read <pane> --source
visible`) before first dispatch — do not treat the start command's own success response
as proof.

**Claude Code pane (builder): auto mode + Sonnet, every launch.**

**Invariant: every Claude Code pane in the roster runs in auto mode on Sonnet — at first launch, at
soft-landing restart, at any from-zero refresh.** Not "accept edits on": a supervised
pane one step short of auto stalls on its first Bash permission dialog, and nobody is
piloting it to click through — the whole unit blocks on that dialog. Not the
harness default either: `~/.claude/settings.json` sets `opus` and a prior session's
`/model` switch dies with the process, so a relaunch without an explicit model flag
silently comes back Opus.

- **Launch it that way:** `herdr agent start <name> --kind claude --pane <id> --
  --permission-mode auto --model sonnet`. The launch flags are the reliable mechanism;
  `shift+tab` sent via `herdr agent send-keys` does NOT reliably change the mode
  (confirmed 2026-09-12) — don't rely on it to fix a pane that came up short.
- **Verify before first dispatch:** read the fresh pane's visible status line for
  `auto mode on` and Sonnet as the active model. Do not treat the start command's own
  success response as proof, and do not dispatch work to a pane whose mode and
  model you haven't confirmed.
- **Any restart/refresh, same flags.** A plain restart with no args comes back in
  "accept edits on" and on the `opus` default, and the custom name is lost — re-rename.
  Full restart procedure: `token-budget-and-soft-landing.md` → "EXIT and restart
  cleanly" owns the details; this section owns the standing rule.
- The model half of this invariant is roster-only. The Inspector's OWN pane stays
  Opus (`token-budget-and-soft-landing.md` → "Successor's first actions", step 0) and its
  mirror rule is unchanged — switch to auto mode as the very first action on rebirth.

## Briefing rule

Brief each standing agent by filling its role template in `.claude/inspector-briefings/
<role>.md` (builder, reviewer). The template is the shape; every slot value is written
fresh from live state at each dispatch. Binding rules, all roles:

1. **Labeled blocks only, in template order.** No prose between blocks, no added sections.
2. **History enters only as ledger ids and file paths** — never as recap, reasoning, or
   round-by-round narrative. A DEC is cited as `DEC-<id>` plus the path to read it, not
   summarized into the brief.
3. **No lessons block, any role.** A lesson that must reach a worker is a lesson row in
   `findings.md`, reached through AUTHORITY / SETTLED / EXPECTED ids. Numbered lesson
   lists replayed every dispatch are how a momentary workaround hardened into a standing
   rule (the claude-sr roster incident).
4. **Line caps: builder 50, reviewer 40, ad-hoc agents 30.** Over the cap means history
   crept back in — replace it with pointers, don't compress the type.
5. **Every brief works standalone.** Supervised panes are restarted cold; assume no
   memory. No "as previously", no "as you know".
6. **A brief is a message payload, never a file.** It goes out as the `herdr agent
   prompt` payload through `scripts/dispatch.py` — never written to a path an agent is
   told to go read. The line caps in rule 4 exist precisely so every brief fits in a
   prompt; a brief that "needs" a file is a brief that broke rule 4. Replies come back
   through the same channel, spilling to `/tmp/insp-exchange/<unit>.md` only on the
   declared over-cap exception. → `message-transport.md`
7. **Ledger-scale work products are staging files, never reply payloads.** A librarian
   compaction draft, archive append, or state-history move is written to its declared
   staging path in the checkout; the reply carries only that path plus a ≤10-line
   summary. The reply/spill channel moves verdicts and findings, not ledger content —
   a multi-thousand-line draft through it is the exact failure this rule exists to
   prevent.

Ad-hoc agent (investigator): fill its shape cache `.claude/inspector-briefings/
investigator.md` (ROLE, TASK/problem statement, verification target, SCOPE, DELIVER —
the isolation rules ship inside the template), dispatched with `--role adhoc` under the
30-line cap. The cache files are the canonical shapes; rewrite one only when the shape
itself changes, and never store unit content in them.

## Reviewer-specific discipline

**Baseline, every REQ, no exceptions:** on every builder GREEN report — logging, i18n, docs,
config, anything, not just C++/UI code — dispatch the reviewer for a delta-check on the
builder's actual diff. Skipping the dispatch because a REQ "looks simple" is the exact
failure mode this line exists to prevent. The reviewer works every round; the Inspector's
own rebuild/rerun verification runs ONCE per unit, at the reviewer's PASS (step 5 acceptance
gate) — never per round. The two checks supplement each other; neither substitutes for the
other.

**Escalation for lifetime-safety code:** for new C++ UI/lifetime-safety code (raw pointers,
QPointer guards, nested-event-loop reentrancy), one clean delta-check pass is NOT enough —
re-dispatch a tight, scoped confirmation pass after EVERY fix round that touches
pointer/lifetime logic. Each pass has independently found real, distinct bugs the previous
one missed.

## Investigator-specific discipline

Runtime facts: it is a Codex pane — launch and refresh it per "Pane launch modes"; it
does not inherit the project's `PreToolUse` hook and cannot spawn QGDW subagents.

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
- `scripts/codex_context.py --pid <pid>` for Codex panes (reviewer, investigator).
- `scripts/claude_context.py --pid <pid>` for Claude Code panes — the builder
  (default 250000 threshold) and the Inspector's own pane (pass `--threshold 210000`
  for it).

For each of the 4 numbers (builder, reviewer, investigator, yourself), track role, pane,
session/PID when available, used tokens, sample time, threshold, and `below_threshold` /
`warn` / `unknown`. Missing readings remain explicit unknowns; `idle` says nothing about
context size. A worker at **>=250000** starts that agent's soft-landing procedure; your own
threshold is **>=210000**. Keep reading the other panes while one agent is being refreshed.
