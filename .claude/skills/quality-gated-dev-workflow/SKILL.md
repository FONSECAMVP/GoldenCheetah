---
name: quality-gated-dev-workflow
description: A quality-gated, multi-agent workflow for solo developers and vibe-coders building software with AI assistance, organized around a wiki memory — a compact project "brain" mapping everything that exists — and an orchestrator that delegates to specialized subagents (adversary, validator, scout, builder, librarian). Use whenever the user wants to plan, scaffold, architect, or continue any software project or non-trivial feature — hobby projects, MVPs, scripts, and side projects all count. Trigger on "I want to build", "help me plan", "starting a new project", "architect this", "continue my project", "set up a codebase", or "vibe-coding". Enforces a Three Options Doctrine (options scored on reliability, scalability, maintainability, best practices), fresh-context adversarial review cycles, incremental cross-layer validation, and cascade propagation. Reads the wiki first each session so it never re-explores or re-creates existing files; a lessons memory turns past mistakes into checkable guards.
---

# Quality-Gated Development Workflow (wiki-memory edition)

A workflow for a solo developer plus an AI agent (you). There are no separate reviewers,
QA, or PMs, so the workflow takes those roles. As projects grow — many tests, many
decisions, many validation runs — the hardest problem is no longer cost; it is **staying
oriented**. An agent without a stable map of the project starts exploring blindly, pulls
stale or irrelevant context into the session, and re-creates files and folders that
already exist.

This edition solves that with a **wiki memory**: a small, stable "project brain" that maps
what exists and where it lives. You read it *first*, every session, and you navigate by
it — never by blind exploration.

## Two memories, never conflated

| Memory | File | Answers | Volatility |
|---|---|---|---|
| **Map (the brain)** | `WIKI.md` + `wiki/` | *What exists and where does it live?* | stable — changes only when structure changes |
| **Cursor (the status)** | `STATE.md` | *Where are we right now?* | volatile — changes every step |
| **Lessons (the learning)** | `lessons.md` | *What mistake must I avoid here?* | grows slowly; deduped by signature, escalates on recurrence |

`WIKI.md` is structural and rarely changes, so reading it first is cheap and keeps you
oriented. `STATE.md` is the live cursor. `lessons.md` carries checkable rules distilled from
past errors, surfaced only when their scope matches the current operation. Keep the three
distinct: the map tells you *where*, the cursor tells you *when*, the lessons tell you *what
to verify so you don't err again*.

---

## Principle 1 — Orient before you act (wiki-first research)

**Before any work, and whenever you are unsure where something is or whether it exists,
your first research step is to read `WIKI.md`.** It is the project brain — the map of
every artifact, every directory, every ID range, and every wiki page. From the map you
navigate directly to what you need.

Hard rules:
1. **Read `WIKI.md` first**, every session and on any uncertainty. Then read `STATE.md`
   for live status. Only then act.
2. **Navigate by the map, never by blind exploration.** Do not list/search the filesystem
   to "find" something the map already locates. Exploration adds wrong context; the map
   prevents it.
3. **Check before you create.** Before creating any file or directory, find it in the
   `WIKI.md` MAP. If it's there → open the existing one. If it's not → create it at the
   map's canonical location *and add it to the map in the same action*. A create that
   fails because the target already exists is a **manifest-drift signal**: fix the map and
   use the existing file. **Never duplicate; never re-create.**
4. **Allocate IDs from the registry.** Take the next number from `WIKI.md` REGISTRIES and
   bump it there. Never reuse, never guess.
5. **Pre-flight the lessons.** Before an operation (create-file, decision, cascade, phase
   exit…), load the lessons tagged for it and honor any **guards** as hard preconditions —
   this is how past mistakes stop recurring (Principle 10).
6. **The wiki is your recovery anchor.** If context feels wrong or you've lost the thread,
   STOP, re-read `WIKI.md`, and re-orient. Do not push forward on a confused context.

---

## The rest of the contract (principles 2–13)

2. **Minimal sufficient context.** After orienting, load only what the current operation
   names. Tiered loading: WIKI (Tier 0) → STATE → the one index/page you need (Tier 1) →
   one full entry by ID (Tier 2) → full corpus (Tier 3, explicit/release only). No blanket
   re-reading.
3. **Three Options Doctrine.** Every decision = exactly three alternatives, each scored on
   the Four Pillars, with a recommendation and concrete cascade impact, *before* the user
   chooses. It applies to **architectural choices, irreversible actions, public API changes,
   and materially different trade-offs** — **not** to a uniquely determined local bug repair,
   which is applied under the existing task/finding and recorded in the normal test/commit
   report. Template, rubric & boundary: `references/three-options-doctrine.md`.
4. **Four Pillars.** Score 1–5 with a one-line reason each: Reliability, Scalability,
   Maintainability, Best Practices.
5. **Decision ledger is index-first.** The hot artifact is the one-line-per-DEC index;
   full entries are cold, drilled by ID. The index is listed in the wiki PAGES.
6. **Cascade on change.** When a DEC's choice changes (or it's flagged `needs-review`),
   process its dependents as a queue of single drills.
7. **Adversarial cycles are mandatory — and bounded.** Each phase ends with a named
   challenge (A0–A5), run in writing; **one normal adversarial cycle per feature slice**.
   Only open findings enter the hot path, each carrying its BLOCKS effects (Principle 13).
   A defect in the harness, scaffolding, or process artifacts gets a **direct bounded
   repair**, not a fresh cycle (`references/adversarial-cycles.md`).
8. **Cross-Layer Validation, incremental by default.** Validates only what changed; full
   9-check walk only on explicit request or release. A FAIL blocks **exactly the effects
   assigned to it** — derived from the failed property, not from why the run was requested
   (`references/cross-layer-validation.md`).
9. **Update-as-byproduct.** State, indexes, **and the wiki map** are updated as the *tail*
   of the action that changed them — never a separate maintenance pass, and **never with a
   permission request**: recording an observable fact (TEAM, BUDGETS, COUNTS, a trace row)
   is the orchestrator's job, not a question for the user. The user is asked only for
   decisions (via the doctrine) and finding dispositions. If you created a file or
   allocated an ID and didn't update the wiki, the operation isn't done. **Byproducts are
   the tail of a *passing* action:** on a gate FAIL the byproduct step is **deferred, not
   partially performed** — write only the four-item failure record (Principle 13) and
   nothing else.
10. **Learn from mistakes (incremental smartness), above a threshold.** Capture or promote
    a lesson only when at least one is true: the mistake **recurs**; it **crosses component
    or project boundaries**; it reveals a **defective workflow rule or mechanism**; or it
    creates a **material security, data-loss, or irreversible-operation risk**. A
    first-occurrence local implementation or harness bug is a **repair, not automatically a
    lesson**. When the threshold is met, capture a *checkable rule* in `lessons.md` (a
    lesson, not a story). Recurrence escalates it: advisory → **guard** (a hard
    precondition, checked at pre-flight) → proposed deterministic enforcement. The memory
    tracks its own effectiveness (saves vs. misses). Mechanism:
    `references/lessons-memory.md`.
11. **Orchestrate, don't monologue.** With the team installed (`STATE.TEAM: on`) and
    `STATE.RIGOR` at standard/full, the main
    session is the **orchestrator**: it holds WIKI+STATE, dispatches role-work via the
    Delegation Table, verifies every report through the **Verification Gate**, and remains
    the **single writer** of every governance file. Role-work — building a REQ, running a
    cycle, running CLV, researching a DEC — done inline while the team is installed is a
    **process violation**: stop, capture a lesson against `op:delegate`, and re-dispatch.
    The orchestrator's own hands touch only: ledger merges, briefings, verification,
    user conversations, and single small edits. Contracts and plays:
    `references/orchestration.md`.
12. **Proportional rigor.** The FIRST decision of every project — and of any significant
    feature — is the rigor tier: **LIGHT / STANDARD / FULL**, calibrated once in Phase 0
    from objective signals (blast radius, users, data sensitivity, exposure, lifespan,
    size, reversibility), presented via the Three Options Doctrine, and recorded as
    `STATE.RIGOR`. The tier changes *which* mechanisms run, never how carefully they run:
    LIGHT collapses ceremony for small tools (inline role-work permitted, one combined
    pre-flight instead of full cycles); FULL adds named evidence (threat model, chaos
    drill, per-exit **incremental** CLV — a FULL walk still only on explicit request or at
    the release gate, per Principle 8) without adding approval loops. Invariants at every tier:
    check-before-create, doctrine for one-way doors, done-when written before building,
    verified agent reports, STATE.md exists. Escalation is a cascade event with objective
    triggers — a light project that grows re-calibrates and backfills at the boundary.
    Matrix and triggers: `references/rigor-tiers.md`.
13. **Scoped blocking — severity is not blocking effect.** Agents report *severity*
    (`blocking | non-blocking | informational`); the **orchestrator** assigns **BLOCKS**, a
    *set* of zero or more effects:

    | Effect | Stops |
    |---|---|
    | `TASK:<task-or-id>` | only that task |
    | `CHECKPOINT:<slice>` | only that slice's verified checkpoint |
    | `RELEASE` | merge / release |
    | `DEPLOY` | deployment |

    An **empty set is advisory** and blocks nothing. Effects compose, and **scope is proven,
    not assumed** — anything beyond `TASK:` needs a stated reason it reaches that far;
    unproven scope means `TASK:<active-slice>`, never a global block. **A blocker never
    means "nothing else runs" unless it can affect every independent operation.** Unrelated
    read-only work is always permitted; unrelated write work is permitted when paths,
    ownership, generated artifacts and evidence inputs are disjoint. On a FAIL, write only:
    the current verdict · the evidence pointer with command and exit code · the BLOCKS
    effects and their explicit scope · the next repair. Effects, the parallel-work rule, the
    twelve-scenario behavioral matrix, and the snapshot/checkpoint/release split:
    `references/orchestration.md`.

---

## The wiki memory — `WIKI.md` (Tier 0, the project brain)

Read first, always. Target ~400–600 tokens — dense, structural, stable. Full schema and
spoke-page templates are in `references/wiki-memory.md`.

```
# PROJECT WIKI — <name>            (the brain · read me first)
root:  /abs/path/to/project         schema: wiki-v1
phase: 2                            live-status → STATE.md (read next)

## MAP — directory manifest (authoritative; check before creating ANYTHING)
prd.md                 requirements
decisions.md           decision index (head) + full entries
design.md              DES entries
traceability.md        trace digest (head) + full table
findings.md            open findings + BLOCKS effects
STATE.md               live cursor
WIKI.md  wiki/         this brain + spokes
cycles/                A0–A5 narratives        → archive/cycles/ on phase close
validations/           VAL reports             → archive/validations/ on phase close
tests/<area>/          test_<req>.py
src/<component>/        code
references/archive/     cold history (ID-addressable)

## REGISTRIES — what exists (allocate next; never reuse, never re-create)
REQ  001–045  full:prd.md                         next:046
DEC  001–030  index:decisions.md head             next:031
DES  001–035  design.md                           next:036
TEST 001–044  tests/                              next:045
VAL  001–013  latest:VAL-013 PASS · validations/  next:014
LSN  001–012  active:9 guards:4 · lessons.md       next:013
F    open→findings.md  resolved→archive/          next:F-018

## PAGES — wiki spokes (read the one named; don't explore blindly)
wiki/architecture.md — component & data-flow map; which DEC governs what · read before structural/integration changes
wiki/conventions.md  — naming, ID rules, canonical locations, check-before-create list · read before creating files
wiki/glossary.md     — canonical term definitions · read when a term is ambiguous
lessons.md           — checkable rules from past mistakes · read guards before the matching operation

## ORIENTATION PROTOCOL  (see Principle 1)
1 read this first  2 navigate by MAP/PAGES, never blind  3 check MAP before create
4 allocate IDs from REGISTRIES  5 pre-flight guards for the operation  6 lost? re-read this
7 update MAP/REGISTRIES/lessons as a byproduct
```

### The spokes (`wiki/`, Tier 1 — read the one you need)
- **`wiki/architecture.md`** — the detailed project map: components, data flow, integration
  contracts, and which DEC governs each part, plus "watch" items from pre-mortems. This is
  how you understand *how the system fits together* without reading all of `design.md`.
- **`wiki/conventions.md`** — canonical file locations, ID rules, and the
  check-before-create anti-duplication checklist. Read this before creating anything.
- **`wiki/glossary.md`** — canonical meanings of project terms, so ambiguous words never
  pull in wrong context.
- **`lessons.md`** — checkable rules distilled from past mistakes. Load the lessons tagged
  for the current operation and honor guards as preconditions (Principle 10).

The decision index, trace digest, and findings table are existing operational files; the
wiki PAGES section points to them rather than duplicating them.

---

## Recurring procedures (orient first, then delegate role-work)

Every procedure below has the same shape: **orient → check TEAM → delegate role-work to the
named agent (if TEAM:on) → verify the report (see Verification Gate) → merge as byproduct.**
Doing role-work inline while the team is installed is a process violation (Principle 11) —
inline is the *fallback* for TEAM:off, never a shortcut.

### Session start / recap / "continue my project"
0. **ORIENT** — read `WIKI.md` (the map). 1. Read `STATE.md` (the cursor). 1b. **Schema check**
— if STATE lacks fields the current schema defines (older project), backfill silently:
record `TEAM: on(<n> agents)|off` (list `.claude/agents/qgdw-*.md` once), `BUDGETS`, and
`COUNTS` without asking; a missing `RIGOR` gets the calibration proposal (the one
legitimate ask), then is recorded. 2. Emit a 4-line status: phase · open ·
blocking · next gate. 3. If `BLOCKING ≠ —`, load just those finding lines **with their BLOCKS
effects**, so the status says what is actually stopped (and what is not); if
`CASCADE ≠ —`, load just those DEC index lines. 4. **Resume at `NEXT_GATE` by dispatching**:
name the gate's remaining conditions, map each to its agent via the Delegation Table (below),
and spawn — do not start doing the gate's role-work yourself. **Never list or search the
filesystem before reading `WIKI.md`.**

### New-task / feature start
Orient (WIKI + STATE + TEAM) → load the one target `REQ` (Tier 2) → load that REQ's
decision-index slice (Tier 1) → pre-allocate `TEST` ids from REGISTRIES → **spawn
`qgdw-builder`** with a complete briefing (task, scope paths, acceptance criterion quoted
verbatim, governing DECs, allocated ids). On return: run the **Verification Gate**, then
merge (trace row, `STATE.OPEN/CHANGESET`, registry bumps) and queue A3 with the adversary.
*TEAM:off fallback:* run the TDD loop inline under the same briefing discipline, with
pre-flight `op:create-file` guards and the WIKI MAP check before creating anything.

### Routine decision (Three Options Doctrine)
If research is needed (unknown landscape, volatile facts): **spawn `qgdw-scout`** with the
question + REQ ids + prior-DEC constraints + user constraints verbatim → verify the draft
(three real options? scores justified? cascade concrete? sources for volatile claims?) →
the **orchestrator** formats and presents the proposal to the user. If no research is
needed (constraints fully determine the trade space), draft inline. Either way: on choice,
append the full entry (cold), one index line, bump `DEC.next`, patch `STATE` if it adds a
dependent/one-way door.

### Phase gate / validation / adversarial cycle
**Spawn `qgdw-validator`** (incremental CLV over `CHANGESET`) and, at cycle points,
**`qgdw-adversary`** (the phase's A-cycle) — in parallel when both are due (both read-only).
Verify both reports, merge findings into `findings.md`/`STATE`, disposition with the user,
re-spawn only the failed role until clean. *Never self-run a cycle on artifacts this
context produced while TEAM:on.*

### After any structural change (the wiki-maintenance trigger)
Created a file or folder, allocated an ID range, added a component, or recorded a new
validation? Update the relevant WIKI MAP / REGISTRIES / `wiki/architecture.md` line **in
the same action**. The wiki only changes when structure changes, so this is infrequent and
cheap — but it is mandatory, because a stale map is how the agent gets lost again.

### Delegation Table (consult on every "continue" / gate dispatch)
| NEXT_GATE condition mentions… | Spawn |
|---|---|
| implement / build / TDD / a REQ to code | `qgdw-builder` (one per REQ; parallel only on disjoint files) |
| A0/A1/A2/A3/A4/A5 / red-team / pre-mortem / hardening | `qgdw-adversary` |
| CLV / validation / drift / release audit | `qgdw-validator` |
| a pending DEC needing research | `qgdw-scout` |
| migration / build-the-brain / archive-on-close / a STATE.BUDGETS breach (compaction) | `qgdw-librarian` |

---

## Heavy mechanisms (unchanged rigor; condensed output)

- **Cross-Layer Validation** — incremental over `STATE.CHANGESET` by default; full 9-check
  walk on explicit request or release gate. Records one verdict line into `STATE.LAST_CLV`
  and bumps `VAL.next` in WIKI REGISTRIES so the **validation history is visible at a
  glance** (this is what keeps "many validation runs" from confusing you). **A FAIL is a
  lesson-capture *candidate*** evaluated against the Principle-10 threshold **after the repair
  passes** — and its effects come from the failed property, never from why the run was
  requested. Details: `references/cross-layer-validation.md`.
- **Three preservation/release operations** — a **snapshot commit** (explicitly unverified,
  local-only, `WIP`/`SNAPSHOT` message naming every red or ungated item, explicit user
  authorization, never pushed or promoted, leaves every existing effect active); a **verified
  checkpoint** (the slice's own tests + touched suites green, contract and goal audit passed,
  path-scoped staging — unrelated release gates are *not* preconditions); and a **release
  commit** (all applicable acceptance and release gates, no open finding carrying
  `RELEASE`/`DEPLOY`). Never call an unverified snapshot a checkpoint. Definitions:
  `references/orchestration.md`.
- **Cascade propagation** — load the changed DEC's dependents (one drill), queue them in
  `STATE.CASCADE`, process one ID at a time, then a drift-scoped CLV over the touched IDs.
  A *missed* cascade detected later is a lesson candidate (Principle 10 threshold).
- **Adversarial cycles A0–A5** — full rigor, **one normal cycle per feature slice**; narrative
  to cold `cycles/`, only open findings to `findings.md`, each carrying its BLOCKS effects. A
  harness/scaffolding/process defect gets a direct bounded repair instead of a new cycle, and
  two unsuccessful *material* repair attempts return to the user. Findings that touch system
  structure also update
  `wiki/architecture.md`'s "watch" list. **A5 is the richest lesson-harvest point** — turn
  recurring retrospective findings into guards.
- **Lessons memory** — detecting a mistake creates a **lesson *candidate***, not a lesson.
  Capture happens **after the repair passes** and only when the Principle-10 threshold is met
  (recurrence · crosses component/project boundaries · a defective workflow rule or mechanism ·
  material security, data-loss, or irreversible-operation risk). A first-occurrence local
  implementation or harness bug is **normally repaired without creating a lesson**. Once
  captured: surface scoped guards at pre-flight; escalate on recurrence; track saves/misses.
  Mechanism: `references/lessons-memory.md`.
- **Deterministic anti-duplication guard (enforced, not just instructed).** This skill
  bundles a `PreToolUse` hook (`scripts/anti_duplication_guard.py`) that, once installed,
  fires before every `Write`/`Edit`/`Bash`. It **denies** re-creating a path that already
  exists (the LSN-007 mistake — `mkdir` of an existing dir, redirect/`touch` over an existing
  file) and **asks** when a new artifact isn't yet in the WIKI MAP. This is the escalation
  endpoint of Principle 10: a guard that kept being missed became mechanism. Install it once
  per project with `python3 scripts/install_hook.py` (see `references/lessons-memory.md`);
  it writes the hook into `.claude/settings.json`. When the hook denies a genuine clobber,
  its reason is ground truth — open the existing path or register the new one. But when it
  denies an operation you believe is LEGITIMATE, that is a **mechanism bug, not an obstacle
  to route around** (LSN-036 class): never invent a bypass — reproduce the denial in
  isolation, record it as a finding + a `miss` on the guard's lesson, and fix the mechanism
  with a two-directional behavior matrix (genuine violations still denied AND the legitimate
  case passing). The denial carries `{TASK:<denied op>}` — it stops that operation, not the
  session — and after two unsuccessful *material* repair attempts, return to the user. An
  agent working around a hook is itself a mechanism bug of equal weight to a missed catch. If the hook isn't installed, the same rules apply by instruction.
- **Archive-on-close** — when a phase closes, move its cycle/validation files to
  `references/archive/` and update the WIKI MAP to point there; prune `recur:1` advisories
  to `archive/lessons-dormant.md`. History stays ID-addressable; routine work never
  re-ingests it.
- **Budget telemetry & compaction (the skill tracks its own footprint).** `STATE.BUDGETS`
  records hot-artifact size vs cap (`WIKI n/700 · DECIDX n/500 · LSN g/10 · FINDINGS n`),
  refreshed as a byproduct of editing those files. A breach dispatches `qgdw-librarian`
  Job 3 (compaction: MAP directory-rollup, active/dormant index split, lessons merge)
  before the next feature wave. Evidence has single owners at scale: **the complete affected
  suite runs once per final content version, by one designated owner** — verification runs
  targeted checks and inspects the owner's command/log/exit code, A3 runs mutation tooling
  only, and one wave gets ONE CLV, so nothing is executed or validated twice on the same
  content version. A repair after a failed run, or any formatter/hook edit to a compiled or
  executed input, creates a **new content version** and requires a fresh run of the complete
  affected target (never of unrelated suites). At every
  feature close / wave gate, one **clean-worktree configure+build of HEAD** runs — almost
  every other gate runs inside the developer's working tree, and that blind spot is how a
  feature ships inert behind a green suite or `master` stops building from a clean checkout.
  Rules: `references/orchestration.md` (Scale discipline).

---

## Multi-agent orchestration (when agents are installed)

The workflow's roles run as specialized subagents, each in its own isolated context. The
main session is the **orchestrator** and sole writer of governance files. Full contracts
and plays in `references/orchestration.md`; the operating summary:

| Delegate to | For | Returns |
|---|---|---|
| `qgdw-adversary` | every A0–A5 cycle (fresh context = unanchored red team) | findings report |
| `qgdw-validator` | every CLV pass (incremental or full) | verdict + findings |
| `qgdw-scout` | researching the three options for a pending DEC | scored draft proposal |
| `qgdw-builder` | one REQ via TDD (parallel-safe on disjoint REQs) | build report + changeset |
| `qgdw-librarian` | migration map-building; archive-on-close | drafts + WIKI edit list |

Iron rules: **single writer** (only the orchestrator touches WIKI/STATE/ledgers — agents
report, orchestrator merges as the byproduct); **orchestrator allocates all IDs** and passes
them in briefings; **decisions are always presented to the user by the orchestrator** (the
scout only drafts); builders parallelize **only on disjoint file footprints**; merge order
builders → validator → adversary. Every briefing pastes the relevant STATE + WIKI slice —
agents don't see this skill, so the briefing is their whole world. If agents aren't
installed, play each role inline with the same briefing/report discipline (worse on
independence, identical on rules).

### Verification Gate — every report is double-checked before anything merges

No agent report enters the ledgers or the codebase on trust. On every return, the
orchestrator runs three checks in order; failing any one means **re-brief once** (quoting
the contract and the specific defect) and, on a second failure, run the role inline and
capture a lesson against `op:delegate`.

1. **Contract check** — does the report parse against the agent's declared format? All
   mandatory fields present (builder's `NOTES` and `FILES` especially)? IDs only from the
   allocated set?
2. **Evidence check** — verify the report's central claims against reality, don't read
   them: for a **builder**, independently re-run the feature's own tests plus any suites
   the diff touches (targeted — the complete suite runs once per final content version by
   one owner; inspect that owner's command, log and exit code rather than re-running the
   same content version), and diff the claimed `FILES` list against the actual changes on
   disk
   (`git status`/`git diff --stat`); check
   `API-SURFACE`/`DEPENDENCIES` against the governing DECs — an undeclared dependency or
   surface is drift, not detail. **For any slice whose point is a guard, gate, or
   invariant**, additionally run one orchestrator-executed mutation of the mechanism itself
   (break the guard; confirm the tests die) — "tests pass" is not evidence a guard is
   load-bearing; "tests fail when I break it" is. Snapshot discipline is mandatory:
   `cp file file.orig` before mutating, restore from the copy and `cmp` — NEVER
   `git checkout --` on a file carrying uncommitted work (LSN-032). **For any slice that
   changes object lifetime, ownership, or concurrency**, the acceptance evidence must be an
   *executed* test (under a sanitizer where available) reproducing the hazard — reasoning
   about framework semantics is explicitly insufficient, and ask whether the guard sits on
   the layer that performs the unsafe operation, not the layer that initiates it. For a
   **validator/adversary**, spot-check 1–2 cited findings at their cited locations before
   accepting the verdict. For a **scout**, confirm the options are three *real* candidates
   and volatile claims carry sources.
3. **Goal audit** — before the work is merged/committed, re-read the briefing's `TASK` and
   the REQ's acceptance criterion **verbatim** and confirm the delivered work satisfies
   *that goal*, not a nearby one: the test must encode the criterion as written (not a
   weaker paraphrase), and the diff must contain nothing outside the briefed scope.
   Out-of-scope changes are reverted or re-briefed — never silently kept.

Every run's evidence is reported as **separate measurements** — command · environment ·
declared · executed · pass/fail/skip · sanitizers · exit code — each an observed value or
`N/A` with a concrete reason (`references/orchestration.md`). "N tests passed" alone is not
evidence; a gate is **not** failed merely because an inapplicable measurement is `N/A`; and
`declared ≠ executed` is expected wherever init/cleanup functions and data rows exist — it is
a finding only when the runner's accounting cannot reconcile the difference.

**Only after all three checks pass does the byproduct step run** (trace rows, STATE, WIKI,
findings). On a failure the byproduct step is **deferred, not partially performed**: write the
four-item failure record — verdict · evidence pointer with command and exit code · BLOCKS
effects and their explicit scope · next repair — and nothing else. Counts, registry rewrites,
narratives, compaction, archive moves and lesson capture wait until the repair passes; only a
security or data-loss discovery may add the minimum warning needed to prevent unsafe use.
The gate is cheap relative to what it prevents: an unverified builder report is exactly how
wrong code enters a codebase with a green-looking ledger.

---

## Phases (gates unchanged)

- **Phase 0 — Intake.** Capture request verbatim → **RIGOR calibration** (three tiers via
  the doctrine, from the intake's objective signals; record `STATE.RIGOR` — at LIGHT the
  rest of this list collapses per the tier matrix) → 5-Whys → ambiguities → **A0** →
  DEC-001 (solution shape). **Create `WIKI.md` and the `wiki/` spokes here** as the first artifacts,
  so the brain exists from day one. Seed `STATE.md`. **Install the anti-duplication guard
  and the agent team** for the project: `python3 scripts/install_hook.py` (one-time; wires
  the check-before-create hook into `.claude/settings.json` and copies the five subagents
  into `.claude/agents/` — restart the session to load the agents).
- **Phase 1 — Requirements & Architecture.** `REQ`s → **A1** → architecture DECs (by role)
  → `DES` entries → **A2** pre-mortem. Populate `wiki/architecture.md` and `wiki/glossary.md`
  as the design takes shape.
- **Phase 2 — TDD implementation.** Toolchain DECs → per-feature TDD loop → **A3**. Every
  new test/source file is checked against and added to the WIKI MAP.
- **Phase 3 — Review, integration, deploy.** Pre-commit gate → PR → CI → **A4** → DEC-011
  rollout → manual promotion. Full CLV at the release gate. **A5** retrospective feeds the
  next iteration's Phase 0.

The traceability spine `REQ→DEC→DES→TEST→COMMIT` lives as index lines + trace rows, updated
as a byproduct.

---

## Migrating an existing project (build the brain first)

For a project already underway (any version, or no workflow at all):
1. **Reconstruct the map.** Do *one* bounded pass over the project tree and write `WIKI.md`
   MAP from what actually exists. (This is the one sanctioned exploration — to build the
   brain. After this, navigate by the map.)
2. **Build REGISTRIES.** Scan for existing IDs; record ranges and `next` values. If IDs
   were never used, assign them now and note the mapping.
3. **Write the spokes.** Derive `wiki/architecture.md` from the current components,
   `wiki/conventions.md` from where files actually live, `wiki/glossary.md` from existing
   terms.
4. **Condense status.** Build/refresh `STATE.md`, the decision index, trace digest, and
   `findings.md`; archive old cycle/validation detail.
5. **Seed lessons.** Create `lessons.md`; copy in any `scope:portable` guards from prior
   projects (hard-won workflow rules travel forward). Past mistakes *in this* project that
   are evident from history become initial guards.
6. **Invariant:** every existing file and ID appears in the map exactly once. From here on,
   the check-before-create rule prevents the duplication that prompted this.

---

## Optimized daily usage

- **Starting anything new** → calibrate rigor FIRST (LIGHT/STANDARD/FULL, one scored
  proposal); a significant feature in an existing project gets a per-feature override.
- **"Continue my project / where are we?"** → read `WIKI.md`, then `STATE.md` (incl. TEAM
  and RIGOR — the tier governs how much of the machinery below applies);
  answer in 4 lines; then **dispatch the next gate's work via the Delegation Table** —
  don't start doing it yourself. Never explore first.
- **"How does the system fit together?"** → `wiki/architecture.md` (one page), not all of
  `design.md`.
- **"Add a feature."** → orient → pre-flight `op:create-file` guards → check WIKI MAP for
  existing files → create only what's missing, at canonical locations → TDD loop → update
  map/registry.
- **"Where does X live / does X exist?"** → WIKI MAP/REGISTRIES answers instantly; do not
  search.
- **"Is anything broken?"** → incremental CLV over `CHANGESET`; verdict line + VAL bump on
  green. A FAIL writes the four-item failure record only.
- **A gate FAILed?** → write verdict · evidence pointer (command + exit code) · BLOCKS
  effects and their scope · next repair. No counts, no registry work, no narrative, no lesson
  until the repair passes. Then repair, rerun the complete affected target on the final
  content version, and continue — a harness defect does **not** earn a new A-cycle or CLV
  wave. Two unsuccessful *material* attempts → return to the user.
- **"Save my work now."** → which operation? A **verified checkpoint** if the slice's gate is
  green (unrelated red gates are irrelevant); a **snapshot** if it isn't — explicitly
  authorized, `WIP`/`SNAPSHOT`, every ungated item named, all existing effects still active.
- **Made (or nearly made) a mistake?** → repair it. Capture a lesson only above the
  Principle-10 threshold (recurrence · crosses components/projects · a defective workflow
  rule · material security/data-loss/irreversible risk); on recurrence it becomes a guard
  that's checked automatically next time.
- **Got lost / wrong context crept in?** → STOP, re-read `WIKI.md`, re-orient.

If a `TodoList` tool is available, mirror `STATE.NEXT_GATE` conditions as todos.

---

## Reference files (load on demand)
- `references/rigor-tiers.md` — LIGHT/STANDARD/FULL tier matrix, calibration signals,
  escalation triggers, per-feature overrides, all-tier invariants.
- `references/orchestration.md` — multi-agent delegation rules, briefing/report contracts,
  concurrency limits, standard plays (phase gate, feature wave, decision, migration, release).
- `references/wiki-memory.md` — `WIKI.md` schema, spoke-page templates, orientation
  protocol, anti-duplication rules, maintenance triggers.
- `references/lessons-memory.md` — lesson capture, generalization, scoping/tags, promotion
  to guards, saves/misses, pruning, portable lessons.
- `references/state-and-tiers.md` — `STATE.md` schema, indexes, tiered loading with the
  wiki at Tier 0, archive layout.
- `references/three-options-doctrine.md` — proposal template, rubric, worked example.
- `references/adversarial-cycles.md` — A0–A5 prompts and disposition rules.
- `references/cross-layer-validation.md` — the 9 checks, incremental vs. full.
