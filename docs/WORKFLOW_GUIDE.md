# GoldenCheetah AI Workflow Guide

*How to use the project's installed Claude Code skills — `quality-gated-dev-workflow`
(wiki-memory edition) and `test-driven-development` — to keep improving GC without
losing rigor, context, or history.*

Last updated: 2026-07-04 · Everything described here is **project-scoped** (lives under
`.claude/`, nothing in `~/.claude/`).

---

## 1. What is installed

| Piece | Location | What it does |
|---|---|---|
| **quality-gated-dev-workflow** skill (wiki-memory edition) | `.claude/skills/quality-gated-dev-workflow/` | The governing process: wiki brain, decision ledgers, adversarial cycles A0–A5, cross-layer validation, multi-agent orchestration |
| **test-driven-development** skill | `.claude/skills/test-driven-development/` | RED→GREEN→REFACTOR discipline, the Prove-It pattern for bugs, test pyramid, anti-patterns |
| **5 subagents** | `.claude/agents/qgdw-*.md` | `adversary`, `validator`, `scout`, `builder`, `librarian` — fresh-context specialists (see §4) |
| **Anti-duplication guard** | `.claude/hooks/anti_duplication_guard.py` + `.claude/settings.json` | A deterministic `PreToolUse` hook that fires before every `Write`/`Edit`/`Bash`. **Denies** clobbering an existing path (`touch`/`>`/`tee`/`cp`/`mv`/`ln` over an existing file, `mkdir` of an existing dir); **asks** when a new artifact isn't registered in the WIKI MAP |

The two skills divide cleanly: **QGDW governs *what* gets built and *whether* it may
ship** (decisions, gates, validation); **TDD governs *how* each piece of code gets
written** (test first, prove bugs before fixing). The QGDW Phase-2 loop *is* the TDD
loop with IDs and traceability attached.

---

## 2. The mental model (wiki-memory edition)

The new skill version is organized around **three memories, never conflated**:

| Memory | File | Answers | Changes |
|---|---|---|---|
| **Map (the brain)** | `WIKI.md` + `wiki/` spokes | *What exists and where does it live?* | only when structure changes |
| **Cursor (the status)** | `STATE.md` | *Where are we right now?* | every step |
| **Lessons (the learning)** | `lessons.md` | *What mistake must I avoid here?* | grows slowly, deduped by signature |

**The iron rule: orient before acting.** Every session starts by reading `WIKI.md`, then
`STATE.md`. The agent navigates by the map — it never lists or greps the tree to
"discover" something the map already locates. For a codebase of GC's size (~1,500 C++
files plus qwt, Python bindings, and translations), this is what stops the agent from
pulling stale context or re-creating files that already exist.

**Tiered loading** keeps sessions cheap: Tier 0 = WIKI + STATE (~850 tokens); Tier 1 =
one named index or spoke; Tier 2 = one full entry by ID; Tier 3 = full-corpus walk,
sanctioned only at release or migration.

**Lessons escalate.** A mistake seen once becomes an *advisory*; a recurring one becomes
a *guard* (hard precondition checked before the matching operation); a guard that keeps
being missed becomes *mechanism* — the anti-duplication hook is exactly that endpoint,
enforcing lesson LSN-007 deterministically instead of by instruction.

### The traceability spine

Everything ties together as `REQ → DEC → DES → TEST → COMMIT`. Commits cite their IDs.
Cross-layer validation (CLV) checks the spine incrementally over the current changeset;
a full 9-check walk runs only at release gates. **FAIL blocks phase exit or deploy.**

### The quality gates

- **Three Options Doctrine** — every consequential decision is presented as exactly
  three alternatives scored 1–5 on Reliability / Scalability / Maintainability / Best
  Practices, with concrete cascade impact and a recommendation, *before* choosing.
- **Adversarial cycles A0–A5** — each phase ends with a named written challenge:
  A0 build-or-not · A1 requirements red team · A2 design pre-mortem · A3 test hardening
  (mutation testing — already GC practice on the Garmin work) · A4 pre-production
  hardening (rollback drill!) · A5 retrospective (the richest lesson-harvest point).
- Every finding gets a disposition: `fix-now`, `defer` (ticket), or `accept` (written
  rationale). Never "we'll get to it."

---

## 3. Current GC state — what exists, what's next

GC already ran the **previous** edition of this workflow. That history is preserved and
usable:

| Ledger | Status | Contents |
|---|---|---|
| `.claude/workflow-garminconnect/` | **ACTIVE** — Phase 2.2 | Garmin Connect integration. 13 accepted DECs, VAL-006 PASS, REQ-002 auth slice complete. Next gate: VAL-007 (PyEmbeddedAdapter slice) |
| `.claude/workflow-aicoach/` | shipped, closed | AI Coach (src/Coach/, TEST-001…020). Provenance only — never rebuild state from it |
| `.claude/workflow-INDEX.md` | index | Names the active ledger; one ledger per feature |

**The brain exists (migrated 2026-07-04):** `WIKI.md` at the repo root is the hub —
read it first, every session — with spokes in `wiki/` (architecture, conventions,
glossary), the condensed project cursor in `STATE.md`, and `lessons.md` seeding
LSN-001…006 (5 guards + 1 advisory harvested from the Garmin ledger's A-cycle history).
REGISTRIES are **ledger-namespaced** (`garmin:` / `coach:`) because the two ledgers'
ID ranges collide numerically — never cite a cross-ledger ID without its prefix
(LSN-002). With the brain in place, the hook's MAP-registration asks are active:
creating a file not in the MAP prompts you to register it.

---

## 4. The agent team

The main session is the **orchestrator**: it holds WIKI+STATE, allocates all IDs, talks
to the user, and is the **single writer** of every governance file. Specialists run in
isolated fresh contexts:

| Agent | Job | Writes | Why fresh context matters |
|---|---|---|---|
| `qgdw-adversary` | every A0–A5 cycle | nothing (reports) | can't be anchored by the builder's reasoning — a genuine red team |
| `qgdw-validator` | every CLV pass | nothing (reports) | independent audit of the traceability spine |
| `qgdw-scout` | 3-options research for one DEC (has web search) | nothing (drafts) | research noise stays out of the main session |
| `qgdw-builder` | one REQ via TDD | tests + source ONLY | implementation context stays contained; parallel-safe on disjoint REQs |
| `qgdw-librarian` | migration map-building; archive-on-close | drafts + archive moves | exploration noise stays contained |

Iron rules: agents never touch WIKI/STATE/ledgers (they report; the orchestrator
merges); decisions are always presented to the user by the orchestrator; builders
parallelize only on disjoint file footprints; merge order is builders → validator →
adversary. Honest cost: multi-agent flows use several times the tokens of inline work —
delegate by the rules, not enthusiasm. Quick Tier-0/1 reads and small edits stay inline.

---

## 5. Playbooks — what to say, what happens

### Continue the Garmin work (most common)
> *"Continue my project"* / *"where are we?"*

Orchestrator reads WIKI.md (once it exists) → active ledger `state.md` → answers in 4
lines: phase · open · blocking · next gate. Currently that resumes at **VAL-007:
PyEmbeddedAdapter implementing IGarminPyAdapter (DEC-013), then AddCloudWizard
tile-routing**. Never re-explores.

### Migrate (one-time — build the brain)
> *"Migrate this project to the wiki-memory workflow"*

Librarian does the one sanctioned bounded exploration → drafts WIKI.md MAP, REGISTRIES,
spokes (`wiki/architecture.md`, `wiki/conventions.md`, `wiki/glossary.md`) → orchestrator
audits (walked/mapped/skipped counts, anomalies — pre-existing duplicates become the
first findings) → commits. Then normal operation begins.

### New feature
> *"I want to add ___"* / *"architect this"*

Phase 0 intake → 5-Whys → **A0 challenge** (should this exist? what does GC already do
that's close?) → DEC-001 solution shape via Three Options → new ledger
`.claude/workflow-<feature>/` registered in the INDEX → A1 red team on requirements →
architecture DECs → A2 pre-mortem → Phase 2 TDD.

### Feature wave (parallel implementation, Phase 2)
Pick 2–3 REQs with disjoint file footprints → orchestrator pre-allocates TEST ids →
spawns builders in parallel → merges reports → adversary A3 per feature → validator
incremental CLV over the merged changeset → green = wave done.

### Bug fix — the Prove-It pattern (TDD skill)
> *"There's a bug: ___"*

**Never fix first.** 1) Write a test reproducing the bug → watch it FAIL (bug
confirmed). 2) Fix. 3) Test passes. 4) Full affected suite — no regressions. For subtle
bugs, spawn a subagent to write the reproduction test *without* knowledge of the
intended fix — it makes the test more honest. The failing test also becomes a permanent
regression guard, which matters enormously in a codebase with GC's low test coverage.

### A decision is needed
> *"Should we use X or Y for ___?"*

Scout researches three genuinely viable options (web search allowed) and drafts the
scored proposal → orchestrator presents it → you choose → full entry to `decisions.md`,
one index line, registry bump. Reversibility is always tagged (cheap / expensive /
one-way door) — it sets how much debate is warranted. This is already how the Garmin
ledger's 13 DECs were made; the new edition just delegates the research.

### Retrofit tests to legacy code (GC's structural weak point)
Treat each target area (e.g. `src/FileIO/` FIT/TCX parsers, `src/Metrics/`) as a REQ:
characterization tests first (pin current behavior), then the TDD loop for changes.
Prioritize pure-logic code — metric computation and file parsing are ideal small-test
targets per the test pyramid (~80% unit). The A3 mutation-testing discipline already
proven on the Garmin worker (19 killed mutants) is the quality bar.

### Release / merge to master
Validator FULL 9-check walk ∥ adversary A4 (staging soak, **rollback drill**, chaos
pass, alert dry-run — scale to what applies to a desktop app: build matrix green,
feature-flag off-path verified, upgrade/downgrade of athlete data tested) → both clean
or dispositioned → ship → A5 retrospective feeds lessons.

---

## 6. Living with the anti-duplication guard

The hook is active now, in every session, including subagents' tool calls:

- **DENY** — you'll see `[anti-duplication guard] '<path>' already exists…` when a
  command would clobber an existing file. The reason is ground truth: open the existing
  path instead. Don't work around it.
- **ASK** (once WIKI.md exists) — creating a file not in the MAP prompts registration.
  Add the MAP line as a byproduct of creating; that keeps the brain fresh.
- It never blocks edits of existing files, `cp`/`mv` *into* directories, or anything
  outside the project root. Known gaps (`install -d`, scaffolders) are accepted; if a
  duplication slips through, record a `miss` on LSN-007.
- Temporarily disable via `/hooks` if it genuinely misfires — then capture why as a
  lesson.

---

## 7. Improvement roadmap — where the skills bite hardest

Priorities for "keep improving GC," each mapped to the machinery above:

1. **Finish Garmin Connect (Phase 2.2 → ship).** Resume via the active ledger;
   builder/adversary/validator loop through VAL-007+; A4 before enabling
   `GC_WANT_GARMINCONNECT` by default; A5 harvests lessons into the new `lessons.md`.
2. **Build the brain (migration).** Unblocks everything else: orientation cost drops,
   MAP-registration asks activate, sessions stop re-deriving project structure.
3. **Grow the test estate.** The Prove-It rule from day one: *every* bug fixed from now
   on leaves a regression test behind. Add characterization suites to parsers and
   metrics opportunistically (the retrofit playbook). Track TEST ids in the registry so
   growth is visible.
4. **qmake → CMake completion.** Run it as a proper feature ledger: A2 pre-mortem on
   platform-specific traps (macOS bundle, Windows deploy, translations), Three Options
   on any structural choice, incremental CLV per converted module.
5. **CI modernization (AppVeyor → GitHub Actions matrix).** A feature ledger with an A4
   that actually exercises all three platform builds before switching over.

---

## 8. Cheat sheet

| You want | Say | Machinery |
|---|---|---|
| Resume work | "continue my project" | WIKI → STATE → 4-line status |
| Project brain built | "migrate to the wiki-memory workflow" | librarian Job 1 |
| New feature | "I want to build ___" | Phase 0 → A0 → ledger |
| Implement a REQ | "implement REQ-NNN" | builder, TDD loop |
| Parallel REQs | "run a feature wave on REQ-X, REQ-Y" | 2–3 builders, disjoint files |
| Fix a bug | "bug: ___" | Prove-It: failing test first |
| Make a decision | "should we ___?" | scout → three options |
| Red-team a phase | "run A1 on the PRD" / "run A3 on ___" | adversary, fresh context |
| Health check | "is anything broken?" | validator, incremental CLV |
| Full audit | "full CLV walk" | validator, 9 checks (release only) |
| Close a phase | "close phase N" | archive-on-close via librarian |
| Something felt wrong | "capture a lesson: ___" | lessons.md advisory/guard |

**Session hygiene:** one feature ledger active at a time (see
`.claude/workflow-INDEX.md`); commits cite `REQ/TEST/DEC`; if the agent seems lost, tell
it to re-read `WIKI.md` — that is the designed recovery anchor, not a workaround.

---

*Skill internals, briefing contracts, and templates live in
`.claude/skills/quality-gated-dev-workflow/references/` — this guide is the GC-specific
operating manual on top of them.*
