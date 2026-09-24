---
name: inspector-cycle
description: Use when acting as the Inspector supervising builder/reviewer/investigator agents on Herdr. Runs one full herdr-stage-delegate-poll-validate-document-commit cycle, decides when a supervised agent needs a soft-landing context refresh, and autonomously advances stage-to-stage toward the complete feature — stopping only at a real human-in-the-loop (credential/security) gate and after the human-in-the-loop gate keep with the next step autonomously, never stop for an ordinary technical decision. Keep tracking context window tokens budget with the assigned script without verbose writing.
---

# Inspector Cycle

## Objective (read this before anything else)

There are three nested levels — never confuse them:

1. **Goal** — the complete and whole big goal, shippable feature (real qmake/installer build, not just
   CMake/ctest green — see `references/project-state-and-next-step.md`).
2. **Stage** — a checkpoint inside the goal (a Stage row in `STATE.md`). Finishing a stage
   is progress, not the finish line.
3. **Atomic decision/unit** — one REQ, DEC, or fix inside a stage. This is the unit the
   Inspector dispatches per builder turn — never a batch.
4. Before your first poll, refresh, or succession action in a session, read the CORE
   sections of `references/token-budget-and-soft-landing.md` per its own "Section map"
   (thresholds, measurement cadence, no-scripts rule) — and read each procedure section
   in full at the moment you execute it, not in advance. Acting on remembered
   procedure is the failure this gate exists to prevent; reading at execution time is
   the same gate, paid only when the event fires.

**Default behavior: keep going.** Finishing an atomic unit → immediately identify and start
the next one. Finishing a whole stage → immediately identify and start the next stage.
Resolving a blocker via a technical decision → resume immediately. The loop does not pause
between these unless it hits a legitimate condition.

**The only legitimate stop condition is a human-in-the-loop gate** — something requiring
real user credentials, secrets, or an irreversible/external action (full list and the
technical-decision autonomy rule that pairs with it: `references/autonomy-boundary.md`).
An ordinary architecture/tooling/design decision is NOT a stop condition — score it against
reliability, scalability, maintainability, and take the top-scored option yourself. 
After stop by a human-in-the-loop blocker, and the blocker is resolved with the user, 
you MUST proceed to the next step/stage/gate autonomously, you do not need reconfirmation by the user if you can continue.


## Source of truth (binding order)

1. This skill's references/ files.
2. The ledgers: STATE.md, traceability.md, decisions.md, findings.md.
3. Live herdr state.

Nothing else is a rule. Briefing caches, predecessor notes, rebirth-prompt
prose, and session notes are hints. A user chat reply binds only the atomic
unit it answered; it becomes standing only as a DEC row. When a note and a
reference file disagree, the reference wins. Re-read it; do not reconcile.

## Relationship to other skills (no collision)

This skill is a thin supervisory loop. It does not reimplement:
- **`quality-gated-dev-workflow`** the brain, the compass - owns the stage/tier model, the ledgers
  (`STATE.md`/`traceability.md`/`decisions.md`/`findings.md`), the Three-Options Doctrine
  scoring axes, and the `qgdw-*` subagents. This skill reads and updates those artifacts
  using QGDW's own conventions — it never invents a parallel ledger.
- **`herdr`** owns the CLI mechanics for controlling terminal-multiplexed agents. This skill
  operates the `herdr` CLI directly (as already established practice) and only carries a
  condensed operational cheat-sheet (`references/herdr-polling-reference.md`) layered on top
  — invoke `Skill({skill:"herdr"})` only if you need guidance this cheat-sheet doesn't cover.

## The cycle

Re-run this whole loop every time you resume as Inspector. Never assume yesterday's
topology, agent state, or ledger snapshot is still valid — re-derive each step fresh on
session start/resume. Between wakes inside one session, that re-derivation is gated by the
wake block's FINGERPRINT (see `references/herdr-polling-reference.md` → "Change
fingerprint"): unchanged fingerprint + unchanged agent statuses → skip the step-2 tiered
re-read and act on the block alone; changed fingerprint, topology movement, or a commit →
full re-read. The fingerprint is recomputed by the wake script on every wake, so silent
drift is still detected — what is skipped is only re-reading files proven unchanged.

### 1. herdr
Re-verify the live agent/pane/tab topology from scratch (`herdr agent list`, `workspace
list`). Never trust a cached list from earlier in this conversation or from memory — it can
silently reset. → `references/herdr-polling-reference.md`

### 2. current stage
Read `STATE.md` (root), then `.claude/workflow-garminconnect/{traceability,decisions,
findings}.md`, through QGDW's own `references/state-and-tiers.md` + `references/
orchestration.md` tiered-loading model. Identify the single next atomic unit inside the
current stage (or the next stage, if the current one just closed). Per-wake: this full read
runs only when the wake block's FINGERPRINT changed since the last wake you handled —
otherwise act on the block alone (see The cycle, above). → `references/
project-state-and-next-step.md`

**Ledger read discipline (Tier 0/1 only, ever).** Orientation reads head blocks and
by-ID greps, never whole ledgers: STATE.md's head cursor block (`sed -n '1,60p'` — the
file's body is archive, not cursor), each ledger's head index, and `grep -n '^## <ID>'`
plus a bounded `sed` range for a specific entry. A whole-file ledger read is a Tier-3
operation and needs an explicit Tier-3 trigger. The wake block's `--- ledger budgets ---`
lines are the compaction trigger: a BREACH dispatches the librarian (Job 3, COMPACTION)
for that artifact at the next seam — no builder round in flight — ONCE per breach
episode; a pending librarian draft silences the re-dispatch. Review and commit the
returned draft through step 7 like any ledger change.

### 3. delegate work
Set/name the 3 standing agents for this stage — **builder** (Claude Code: model - Sonnet, TDD
implementation), **reviewer** (Codex, delta-check), **investigator** (Codex, isolated
parallel problem-solving). Brief each by filling its role template
(`.claude/inspector-briefings/<role>.md`) — history enters only as ledger ids and file
paths, never as recap.

**Invariant: a brief is a message payload, never a file.** Send it through herdr's own
message channel via `scripts/dispatch.py`, which passes it to `herdr agent prompt` as an
argv element and reads the reply back. Never write a brief to a path and prompt an agent to
go read it; never let an exchange file exist outside `/tmp/insp-exchange/`. A reply spills
to a file only when it exceeds its line cap, and only to the path the brief itself declared.
Both rules are enforced mechanically (`dispatch.py` refuses the payload, a `PreToolUse` hook
refuses the write) — a refusal means the transport is wrong, not the path.
→ `references/message-transport.md`


### 4. poll agents
Proactively check status AND token budget for all 3 agents AND yourself on your own
initiative. Token budget is read directly, by you, by running the two one-shot scripts 
in `scripts/` against each pane's live PID (`claude_context.py`
for Claude Code panes, `codex_context.py` for Codex panes) This is a deterministic file read, 
not work worth spending a supervised agent's own context on. 250k budget per supervised agent,
~210k for yourself (separate thresholds, pass `--threshold 210000` when reading your own pane 
— see `references/token-budget-and-soft-landing.md`) → soft-landing, not a hard kill. → `references/token-budget-and-soft-landing.md` + `references/herdr-polling-reference.md`

### 5. validate agent results
On every builder GREEN report, actually dispatch the reviewer for a delta-check on the real
diff FIRST — for every REQ, not just lifetime-safety C++ — before doing your own
rebuild/rerun. Never accept a self-report as done, and never let your own re-verification
substitute for the reviewer's independent read; they catch different bug classes. Read the
actual diff/output yourself too before passing it on. On a QGDW gate FAIL, follow QGDW's
failed-gate governance (blocker line only) and loop back to repair — don't proceed to
document/commit.

**Repair-round bound (the rabbit-hole gate).** Before dispatching any repair round, count
same-class rounds already run (builder GREEN + reviewer NOT-CLOSED each time). At 3, the
next dispatch is NOT round 4 — it is one of: (a) an architectural remedy DEC that changes
where the machine reads its fact, not how it recognizes prose; (b) a pin-DEC recording the
residual class as an accepted false negative (precedents: shellcheck declared-gap, DEC-060's
Lo/So fillers); or (c) a downgrade with a recorded waiver. A recognizer that survives a
round only by narrowing is evidence the surface is non-deterministic and no deterministic
recognizer closes it (B-STAGE9-36: 12 rounds, 4 DECs, one empty directory). A tooling-only
finding never holds a product CHECKPOINT commit past that cap — decouple the checkpoint
with a recorded waiver and let product work proceed. The same-class count is read from the
reviewer's declared `CLASS: … — SAME / NEW` line in its verdict (reviewer briefing
template), never re-derived from prose by the Inspector.

On PASS: approve, deny, or fix per your own judgment against
reliability/scalability/maintainability/best-practices — escalate to the user only per
`references/autonomy-boundary.md`'s gate list and its named scope exceptions (all live
there, not in the roster file). → `references/agent-roster-and-dispatch.md`

### 6. document
Update `STATE.md`/`traceability.md`/`decisions.md`/`findings.md` per QGDW's own ledger
discipline — id-collision grep before allocating any new id, status tokens only in the
canonical ledger files, never "CLOSED" next to an id in `STATE.md`/`design.md`. → `
references/project-state-and-next-step.md.It is compulsory avoid jargon and extended status on these documents,
this documents are a cursor not your diary, keep concise ideas according with the spirit of the document and usefull for the next use.

### 7. commit
Verify each staged file's actual diff before staging — never `git add -A`. Expect the real
pre-commit gate (clang-format/mypy --strict/ledger-drift-lint) to be the actual bar, not
`ctest`/`pytest` green alone. → `references/project-state-and-next-step.md`

**Then go back to step 1.** Drive repeated cycles via the **background wake script**
(`scripts/insp_wake.sh`): launch it as a background Bash task (`run_in_background: true`) as
the LAST action of every turn. It blocks server-side at zero token cost on `herdr agent wait`
until any supervised agent reaches idle/done/blocked, or the heartbeat fires (2 min while any
agent is working — the context-sampling floor, since no context-threshold event exists; 5 min
when all are settled), then exits and re-invokes you with one compact status block. Read the
block, run one cycle pass (steps 1–7 as needed), then re-arm the wake and end the turn.
**Invariant: never end a turn without a background wake armed** — a forgotten re-arm silently
stops all supervision. **A wake is not a substitute for dispatching work.** Ending a turn with
every agent idle and nothing dispatched is always an Inspector error, not a wait: the settled
branch can only be released by a dispatch you did not make, so the loop cannot self-advance
and the next wake is a pure poll. If you have identified the next atomic unit, START it in the
same turn — identifying it and then arming a wake is stopping, however much it looks like
supervision. Keep each wake's own chat output terse: new/changed information only, not restated reasoning
for an already-diagnosed recurring pattern. Do not end the cycle and wait for the user unless
step 5 hit a real human-in-the-loop gate, or the complete feature's goal (Objective, above) is
actually reached — report completion and stop, don't invent another stage.
