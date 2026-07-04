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

## The rest of the contract (principles 2–10)

2. **Minimal sufficient context.** After orienting, load only what the current operation
   names. Tiered loading: WIKI (Tier 0) → STATE → the one index/page you need (Tier 1) →
   one full entry by ID (Tier 2) → full corpus (Tier 3, explicit/release only). No blanket
   re-reading.
3. **Three Options Doctrine.** Every decision = exactly three alternatives, each scored on
   the Four Pillars, with a recommendation and concrete cascade impact, *before* the user
   chooses. Template & rubric: `references/three-options-doctrine.md`.
4. **Four Pillars.** Score 1–5 with a one-line reason each: Reliability, Scalability,
   Maintainability, Best Practices.
5. **Decision ledger is index-first.** The hot artifact is the one-line-per-DEC index;
   full entries are cold, drilled by ID. The index is listed in the wiki PAGES.
6. **Cascade on change.** When a DEC's choice changes (or it's flagged `needs-review`),
   process its dependents as a queue of single drills.
7. **Adversarial cycles are mandatory.** Each phase ends with a named challenge (A0–A5),
   run in writing. Only open/blocking findings enter the hot path
   (`references/adversarial-cycles.md`).
8. **Cross-Layer Validation, incremental by default.** Validates only what changed; full
   9-check walk only on explicit request or release. FAIL blocks exit/deploy
   (`references/cross-layer-validation.md`).
9. **Update-as-byproduct.** State, indexes, **and the wiki map** are updated as the *tail*
   of the action that changed them — never a separate maintenance pass. If you created a
   file or allocated an ID and didn't update the wiki, the operation isn't done.
10. **Learn from mistakes (incremental smartness).** When a mistake is detected — a CLV
    FAIL, a user correction, a failed create, a missed cascade, caught scope creep, or an
    A5 finding — capture it as a *checkable rule* in `lessons.md` (a lesson, not a story).
    Recurrence escalates it: advisory → **guard** (a hard precondition, checked at
    pre-flight) → proposed deterministic enforcement. The memory tracks its own
    effectiveness (saves vs. misses). This is what makes the workflow incrementally smarter
    instead of repeating itself. Mechanism: `references/lessons-memory.md`.
11. **Orchestrate, don't monologue.** When the project's subagents are installed, the main
    session acts as the **orchestrator**: it holds WIKI+STATE, routes role-work to
    specialists (`qgdw-adversary`, `qgdw-validator`, `qgdw-scout`, `qgdw-builder`,
    `qgdw-librarian`), and remains the **single writer** of every governance file. All
    adversarial cycles and CLV runs go to fresh-context agents — never self-review what
    this context produced when an independent agent can review it instead. Delegation
    rules, briefing/report contracts, concurrency limits, and standard plays:
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
findings.md            open/blocking findings
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

## Recurring procedures (orient first, then minimal load)

### Session start / recap / "continue my project"
0. **ORIENT** — read `WIKI.md` (the map). 1. Read `STATE.md` (the cursor). 2. Emit a
4-line status: phase · open · blocking · next gate. 3. If `BLOCKING ≠ —`, load just those
finding lines; if `CASCADE ≠ —`, load just those DEC index lines. 4. Resume at
`NEXT_GATE`. **Never list or search the filesystem before reading `WIKI.md`.**

### New-task / feature start
Orient (WIKI + STATE) → load the one target `REQ` (Tier 2) → load that REQ's decision-index
slice (Tier 1) → **pre-flight `op:create-file` guards** → run the TDD loop. **Before
creating any test/source file, check the WIKI MAP** (Principle 1.3) and honor the
anti-duplication guard; use the canonical location in `wiki/conventions.md`. *Writes:* trace
row, `STATE.OPEN/CHANGESET`, and — if a new file or ID was created — the WIKI MAP/REGISTRY.

### Routine decision (Three Options Doctrine)
Read the decision index (and drill prior DECs this depends on) → **pre-flight `op:decision`
guards** (e.g., flag expensive-reversibility dependents) → present the three-option proposal
in chat → on choice: append the full entry (cold), add one index line, bump `DEC.next` in
WIKI REGISTRIES, patch `STATE` if it adds a dependent/one-way door.

### Per-feature TDD loop (Phase 2)
RED (write `TEST` encoding the acceptance criterion; allocate `TEST.next` from the
registry; create at the canonical test path) → GREEN (minimal code) → REFACTOR. Commit
cites `REQ/TEST/DEC`. *Writes:* trace row, `CHANGESET`, registry bumps; then incremental CLV.

### After any structural change (the wiki-maintenance trigger)
Created a file or folder, allocated an ID range, added a component, or recorded a new
validation? Update the relevant WIKI MAP / REGISTRIES / `wiki/architecture.md` line **in
the same action**. The wiki only changes when structure changes, so this is infrequent and
cheap — but it is mandatory, because a stale map is how the agent gets lost again.

---

## Heavy mechanisms (unchanged rigor; condensed output)

- **Cross-Layer Validation** — incremental over `STATE.CHANGESET` by default; full 9-check
  walk on explicit request or release gate. Records one verdict line into `STATE.LAST_CLV`
  and bumps `VAL.next` in WIKI REGISTRIES so the **validation history is visible at a
  glance** (this is what keeps "many validation runs" from confusing you). **A FAIL is a
  lesson-capture trigger** (Principle 10). Details: `references/cross-layer-validation.md`.
- **Cascade propagation** — load the changed DEC's dependents (one drill), queue them in
  `STATE.CASCADE`, process one ID at a time, then a drift-scoped CLV over the touched IDs.
  A *missed* cascade detected later captures a lesson.
- **Adversarial cycles A0–A5** — full rigor; narrative to cold `cycles/`, only open/blocking
  findings to `findings.md`. Findings that touch system structure also update
  `wiki/architecture.md`'s "watch" list. **A5 is the richest lesson-harvest point** — turn
  recurring retrospective findings into guards.
- **Lessons memory** — capture a checkable rule whenever a mistake is detected; surface
  scoped guards at pre-flight; escalate on recurrence; track saves/misses. Mechanism:
  `references/lessons-memory.md`.
- **Deterministic anti-duplication guard (enforced, not just instructed).** This skill
  bundles a `PreToolUse` hook (`scripts/anti_duplication_guard.py`) that, once installed,
  fires before every `Write`/`Edit`/`Bash`. It **denies** re-creating a path that already
  exists (the LSN-007 mistake — `mkdir` of an existing dir, redirect/`touch` over an existing
  file) and **asks** when a new artifact isn't yet in the WIKI MAP. This is the escalation
  endpoint of Principle 10: a guard that kept being missed became mechanism. Install it once
  per project with `python3 scripts/install_hook.py` (see `references/lessons-memory.md`);
  it writes the hook into `.claude/settings.json`. When the hook denies or asks, treat its
  reason as ground truth — open the existing path or register the new one; don't work around
  it. If it isn't installed (or the runtime lacks Python), the same rule still applies by
  instruction via Principle 1.3.
- **Archive-on-close** — when a phase closes, move its cycle/validation files to
  `references/archive/` and update the WIKI MAP to point there; prune `recur:1` advisories
  to `archive/lessons-dormant.md`. History stays ID-addressable; routine work never
  re-ingests it.

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

---

## Phases (gates unchanged)

- **Phase 0 — Intake.** Capture request verbatim → 5-Whys → ambiguities → **A0** → DEC-001
  (solution shape). **Create `WIKI.md` and the `wiki/` spokes here** as the first artifacts,
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

- **"Continue my project / where are we?"** → read `WIKI.md`, then `STATE.md`; answer in
  4 lines. Never explore first.
- **"How does the system fit together?"** → `wiki/architecture.md` (one page), not all of
  `design.md`.
- **"Add a feature."** → orient → pre-flight `op:create-file` guards → check WIKI MAP for
  existing files → create only what's missing, at canonical locations → TDD loop → update
  map/registry.
- **"Where does X live / does X exist?"** → WIKI MAP/REGISTRIES answers instantly; do not
  search.
- **"Is anything broken?"** → incremental CLV over `CHANGESET`; verdict line + VAL bump; a
  FAIL captures a lesson.
- **Made (or nearly made) a mistake?** → capture/increment the lesson; if it recurred, it
  becomes a guard that's checked automatically next time.
- **Got lost / wrong context crept in?** → STOP, re-read `WIKI.md`, re-orient.

If a `TodoList` tool is available, mirror `STATE.NEXT_GATE` conditions as todos.

---

## Reference files (load on demand)
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
