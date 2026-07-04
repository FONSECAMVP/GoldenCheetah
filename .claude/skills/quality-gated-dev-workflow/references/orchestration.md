# Multi-Agent Orchestration

The workflow's roles — adversary, validator, scout, builder, librarian — can run as
specialized subagents, each in its own isolated context. The main session is the
**orchestrator**: it holds the brain (WIKI + STATE), routes work, merges reports, and is
the **single writer of all governance files**.

## Why subagents here (and the honest cost)

Three genuine wins:
1. **Independence.** An adversarial cycle run by a fresh-context agent cannot be anchored
   by the builder's reasoning. Self-review by the same context is structurally weaker —
   this is the biggest quality gain, not a convenience.
2. **Context isolation.** Builders accumulate heavy implementation context; the librarian's
   migration walk is exploration-noisy; the scout's research is web-noisy. All of that
   stays out of the orchestrator, which keeps holding only Tier-0.
3. **Parallelism.** Read-only work (adversary + validator) can run concurrently; multiple
   builders can run concurrently on **disjoint** requirements and file sets.

The honest cost: every subagent is its own context — agent-heavy flows can consume several
times the tokens of single-threaded work, and each handoff adds latency. So delegation
follows rules, not enthusiasm (below).

## The team

| Agent | Job | Writes | Parallel-safe |
|---|---|---|---|
| **orchestrator** (main session) | routes, merges, decides with the user | ALL governance files (sole writer) | n/a |
| `qgdw-adversary` | A0–A5 cycles | nothing (report only) | yes (read-only) |
| `qgdw-validator` | CLV incremental/full | nothing (report only) | yes (read-only) |
| `qgdw-scout` | 3-options research for one DEC | nothing (report only) | yes (read-only) |
| `qgdw-builder` | one REQ via TDD | tests + source ONLY | yes, on disjoint REQs/files |
| `qgdw-librarian` | migration map / archive-on-close | drafts + archive moves ONLY | no (structural) |

## The single-writer rule (non-negotiable)

Only the orchestrator writes `WIKI.md`, `STATE.md`, `decisions.md`, `traceability.md`,
`findings.md`, `lessons.md`, and `wiki/` spokes. Subagents return structured reports; the
orchestrator merges them and performs every ledger mutation as a byproduct (Principle 9).
This is what makes concurrency safe: builders may write disjoint source files in parallel,
but governance state has exactly one writer, so it can never fork or race.

Corollary: **the orchestrator allocates all IDs** (from WIKI REGISTRIES) and passes them in
briefings. Agents never invent numbers.

## Delegation rules — when to spawn vs. do it yourself

DELEGATE when at least one holds:
- **Independence matters** → all adversarial cycles and CLV runs go to the adversary /
  validator. Never self-run a cycle on artifacts you produced in this same context if the
  agents are installed.
- **The work is context-heavy** → feature implementation (builder), migration exploration
  (librarian), option research with web search (scout).
- **Parallelism on disjoint work** → 2–3 builders on independent REQs; adversary +
  validator concurrently at a phase gate.

DO IT YOURSELF (no agent) when:
- The op is a Tier-0/Tier-1 read (recap, status, a quick index lookup).
- It is a single small edit, a ledger mutation, or a user conversation (decisions are
  ALWAYS presented to the user by the orchestrator — the scout only drafts).
- Agents are not installed in this project (fallback: run the role inline, using the same
  briefing/report discipline mentally).

CONCURRENCY LIMITS:
- Builders in parallel ONLY on disjoint REQ sets with disjoint file footprints (check the
  briefings' canonical locations for overlap before spawning).
- Never two librarians. Never a builder concurrent with archive-on-close.
- Merge order when parallel reports return: builders first (they change the changeset),
  then validator, then adversary findings.

## Briefing contract (orchestrator → agent)

Every delegation passes a briefing with exactly these fields (omit none):

```
TASK: <one line — which cycle / which REQ / which DEC / which job>
SCOPE: <the artifact paths and IDs in scope — nothing else is to be read>
ORIENTATION: <paste of the relevant STATE lines + the relevant WIKI MAP/REGISTRY lines>
CONSTRAINTS: <governing DEC ids + their chosen options; user constraints verbatim>
ALLOCATED-IDS: <pre-allocated numbers the agent may use, e.g. TEST-045..047; or NONE>
RETURN: <name the report format from the agent's own definition>
```

Why paste orientation instead of "read WIKI.md": agents start with no skill context; the
briefing is their whole world. Pasting the *relevant slice* keeps them oriented AND scoped —
they get the map lines they need without license to wander.

## Report merge (agent → orchestrator) — the byproduct step

On every returned report, the orchestrator immediately:
1. **Builder report** → update trace rows + `STATE.OPEN/CHANGESET` from FILES; check
   API-SURFACE/DEPENDENCIES against governing DECs (mismatch = drift finding); queue the
   feature for adversary A3.
2. **Adversary report** → assign F-numbers, append open/blocking findings to `findings.md`
   (one line each), full narrative to `cycles/` (cold); LESSON-CANDIDATES → `lessons.md`
   capture-or-increment; blocking findings → `STATE.BLOCKING`.
3. **Validator report** → verdict line into `STATE.LAST_CLV`; clear `CHANGESET` on green;
   FAIL findings → `findings.md` + remediation routed (often a three-options ask).
4. **Scout report** → orchestrator formats the user-facing DECISION REQUIRED proposal,
   presents it, and on choice: full entry to `decisions.md` (cold), one index line,
   registry bump, STATE patch if dependents/one-way.
5. **Librarian report** → review drafts/anomalies, then the orchestrator itself applies the
   WIKI-EDITS-REQUIRED lines and commits drafts to live paths.

A report that doesn't parse against its contract is re-requested once with the contract
quoted; twice-failed = run the role inline and capture a lesson against `op:delegate`.

## Standard plays

**Phase gate (e.g., Phase 1 exit):**
spawn adversary (A1 on prd.md) ∥ validator (incremental CLV) → merge both → disposition
findings with the user → re-spawn only the failed role until clean → exit.

**Feature wave (Phase 2):**
pick 2–3 REQs with disjoint footprints → pre-allocate TEST ids → spawn builders in parallel
→ merge reports (trace/changeset) → spawn adversary A3 per feature (parallel, read-only) →
disposition → validator incremental over the merged changeset → green = wave done.

**Decision:**
spawn scout with the DEC question + constraints → merge draft → orchestrator presents the
three options to the user → log on choice. (The user-facing ask NEVER comes from the scout.)

**Migration:**
spawn librarian (Job 1) → audit its walked/mapped/skipped counts and ANOMALIES → orchestrator
commits WIKI/spokes → normal operation begins. Anomalies (pre-existing duplicates!) become
the first findings.

**Release:**
validator FULL walk ∥ adversary A4 → both must come back clean/dispositioned → DEC-011
rollout (scout optional) → deploy → A5 retrospective (adversary) feeds lessons.

## Installation & fallback

Agent definitions ship in the skill's `agents/` directory and install to the project with:
```
python3 scripts/install_hook.py <project>   # installs hook AND copies agents/
```
(They land in `<project>/.claude/agents/`; **restart the Claude Code session** to load
agents added on disk.) Without them, the workflow still runs single-context: the
orchestrator plays each role inline using the same briefing/report discipline — strictly
worse on independence, identical on rules. The anti-duplication PreToolUse hook applies to
subagent tool calls in the session too, so builders are guarded mechanically as well.
