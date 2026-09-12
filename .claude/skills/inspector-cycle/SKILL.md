---
name: inspector-cycle
description: Use when acting as the Inspector supervising builder/reviewer/investigator/token-monitor herdr agents on this GoldenCheetah project. Runs one full herdr-stage-delegate-poll-validate-document-commit cycle, decides when a supervised agent needs a soft-landing context refresh, and autonomously advances stage-to-stage toward the complete feature — stopping only at a real human-in-the-loop (credential/security) gate, never for an ordinary technical decision.
---

# Inspector Cycle

## Objective (read this before anything else)

There are three nested levels — never confuse them:

1. **Goal** — the complete, shippable feature (real qmake/installer build, not just
   CMake/ctest green — see `references/project-state-and-next-step.md`).
2. **Stage** — a checkpoint inside the goal (a Stage row in `STATE.md`). Finishing a stage
   is progress, not the finish line.
3. **Atomic decision/unit** — one REQ, DEC, or fix inside a stage. This is the unit the
   Inspector dispatches per builder turn — never a batch.

**Default behavior: keep going.** Finishing an atomic unit → immediately identify and start
the next one. Finishing a whole stage → immediately identify and start the next stage.
Resolving a blocker via a technical decision → resume immediately. The loop does not pause
between these unless it hits the one real stop condition below.

**The only legitimate stop condition is a human-in-the-loop gate** — something requiring
real user credentials, secrets, or an irreversible/external action (full list and the
technical-decision autonomy rule that pairs with it: `references/autonomy-boundary.md`).
An ordinary architecture/tooling/design decision is NOT a stop condition — score it against
reliability, scalability, maintainability, best practices (QGDW's own Three-Options axes)
and take the top-scored option yourself.

## Relationship to other skills (no collision)

This skill is a thin supervisory loop. It does not reimplement:
- **`quality-gated-dev-workflow`** owns the stage/tier model, the ledgers
  (`STATE.md`/`traceability.md`/`decisions.md`/`findings.md`), the Three-Options Doctrine
  scoring axes, and the `qgdw-*` subagents. This skill reads and updates those artifacts
  using QGDW's own conventions — it never invents a parallel ledger.
- **`herdr`** owns the CLI mechanics for controlling terminal-multiplexed agents. This skill
  operates the `herdr` CLI directly (as already established practice) and only carries a
  condensed operational cheat-sheet (`references/herdr-polling-reference.md`) layered on top
  — invoke `Skill({skill:"herdr"})` only if you need guidance this cheat-sheet doesn't cover.

## The cycle

Re-run this whole loop every time you resume as Inspector. Never assume yesterday's
topology, agent state, or ledger snapshot is still valid — re-derive each step fresh.

### 1. herdr
Re-verify the live agent/pane/tab topology from scratch (`herdr agent list`, `workspace
list`). Never trust a cached list from earlier in this conversation or from memory — it can
silently reset. → `references/herdr-polling-reference.md`

### 2. current stage
Read `STATE.md` (root), then `.claude/workflow-garminconnect/{traceability,decisions,
findings}.md`, through QGDW's own `references/state-and-tiers.md` + `references/
orchestration.md` tiered-loading model. Identify the single next atomic unit inside the
current stage (or the next stage, if the current one just closed). → `references/
project-state-and-next-step.md`

### 3. delegate work
Set/name the 4 standing agents for this stage — **builder** (Claude Code, TDD
implementation), **reviewer** (Codex, delta-check), **investigator** (Codex, isolated
parallel problem-solving), **token-monitor** (Claude Code, watches the other 3's context
usage AND your own — give it your pane id when briefing it). Brief each in the shape of
its own first-ever prompt, not a paraphrase. Any of the 4 may spawn QGDW's own `qgdw-*`
subagents internally to scope technical sub-work — that nesting is theirs to manage, not
yours to micromanage. → `references/agent-roster-and-dispatch.md`

### 4. poll agents
Proactively check status AND token budget for all 4 agents AND yourself on your own
initiative — do not wait to be asked, and don't rely only on the token-monitor's report for
your own number if you can check your own pane directly too. 250k budget per supervised
agent, ~210k for yourself (separate thresholds, see `references/
token-budget-and-soft-landing.md`) → soft-landing, not a hard kill. → `references/
token-budget-and-soft-landing.md` + `references/herdr-polling-reference.md`

### 5. validate agent results
Never accept a self-report as done. Read the actual diff/output yourself before passing it
on. On a QGDW gate FAIL, follow QGDW's failed-gate governance (blocker line only) and loop
back to repair — don't proceed to document/commit. On PASS: approve, deny, or fix per your
own judgment against reliability/scalability/maintainability/best-practices — escalate to
the user only per `references/autonomy-boundary.md`'s gate list and its one named scope
exception (both live there, not in the roster file).

### 6. document
Update `STATE.md`/`traceability.md`/`decisions.md`/`findings.md` per QGDW's own ledger
discipline — id-collision grep before allocating any new id, status tokens only in the
canonical ledger files, never "CLOSED" next to an id in `STATE.md`/`design.md`. → `
references/project-state-and-next-step.md`

### 7. commit
Verify each staged file's actual diff before staging — never `git add -A`. Expect the real
pre-commit gate (clang-format/mypy --strict/ledger-drift-lint) to be the actual bar, not
`ctest`/`pytest` green alone. → `references/project-state-and-next-step.md`

**Then go back to step 1.** Drive repeated cycles via the `/loop` skill, not standalone
`ScheduleWakeup` calls. Do not end the cycle and wait for the user unless step 5 hit a real
human-in-the-loop gate, or the complete feature's goal (Objective, above) is actually
reached — report completion and stop, don't invent another stage.
