# State & Tiers — orientation and minimal loading

Two memories, read in order, then escalate only by named trigger:

- **`WIKI.md` (the map / brain)** — what exists and where. Read **first**, every session.
- **`STATE.md` (the cursor / status)** — where we are right now. Read **second**, for any
  work-resuming operation.

`WIKI.md` is detailed in `references/wiki-memory.md`; this file covers `STATE.md`, the
indexes, and the tiered loading model.

## Tiered loading (deterministic)

| Tier | Loaded | Trigger | Typical cost |
|---|---|---|---|
| **0 — orient** | `WIKI.md` (map) then `STATE.md` (cursor) | every session / any uncertainty | ~600 + ~250 tok |
| **1** | one index/spoke (decision index, trace digest, findings, or a `wiki/` page) | the op names it | ~25 tok/line · spoke ~200 |
| **2** | one full entry by ID (DEC / DES / one cycle) | that ID must be inspected/changed | 200–600 tok |
| **3** | full-corpus walk / archive | explicit request, release audit, or migration | grows; rare |

Escalate **only** by a named trigger. "Let me look around to be safe" is a process
violation — the map at Tier 0 already locates everything.

**The ~25 tok/line estimate assumes row discipline holds.** A read tool that returns fewer
lines than a file's line count (check with `wc -l` first, or grep for the specific ID you
need instead of reading the whole file) is a symptom of bloated rows silently truncating
the read — not a smaller file than expected. Treat that as a compaction trigger (librarian
Job 3, row-length rule) and re-read the specific ID(s) you actually need via grep rather
than trusting a truncated whole-file read.

## Map vs. cursor — keep them distinct

| | `WIKI.md` (map) | `STATE.md` (cursor) |
|---|---|---|
| Answers | what exists, where it lives, what governs it | current phase, open work, blocking, next gate |
| Changes when | *structure* changes (new file/dir/id/component/validation) | *every step* |
| Role | orientation + anti-duplication anchor | resume point for work |

Merging them would make the map churn and lose its value as a stable anchor. They
cross-reference (the wiki's `live-status →` line points to STATE; STATE never restates the
map).

## `STATE.md` schema (Tier 0 cursor, ≤ ~250 tokens)

```
# STATE — <project>            updated: <ISO> by <op>
PHASE:     <0|1|2|3> · <sub-step / feature + stage>
OPEN:      <REQ-ids with (stage)>
BLOCKING:  <finding-id[EFFECT;EFFECT] … | —> # ids + BLOCKS effects; detail in findings.md
CASCADE:   <DEC-ids + trigger | —>
LAST_CLV:  <date or VAL-id> <PASS|FAIL> · <n WARN(summary)>
NEXT_GATE: <gate name> → <remaining conditions>
CHANGESET: <ids touched since LAST_CLV>      # seeds incremental CLV
TEAM:      on(<n> agents) | off              # qgdw subagents installed? checked at session start
RIGOR:     light | standard | full [(REQ-nnn@full)]   # tier from Phase 0 calibration (+ per-feature overrides)
BUDGETS:   WIKI <n>/700 · DECIDX <n>/500 · LSN <n>g/10 · ROWS <n> over cap   # measured at the wave gate
```

`OPEN/BLOCKING/CASCADE` carry IDs only — never inlined detail. STATE never restates the
map: id counters live only in WIKI REGISTRIES. `CHANGESET` is appended on
change and cleared when CLV goes green over it.

**`BLOCKING` carries effects, not just ids.** Each entry is `F-nnn[EFFECT;EFFECT]`, where an
effect is one of `TASK:<task-or-id>`, `CHECKPOINT:<slice>`, `RELEASE`, `DEPLOY`
(`references/orchestration.md`). Multi-line form when there is more than one:

```
BLOCKING:
  F-018[TASK:REQ-028;CHECKPOINT:REQ-028;RELEASE]
  F-021[CHECKPOINT:REQ-029]
```

Findings with an **empty effect set are advisory and never listed here** — they live in
`findings.md` only. An **untagged legacy id** defaults temporarily to `TASK:<active-slice>`,
**never** to a global block, and is classified at the next read. A gate fails only when an
open finding's effects intersect that gate.

**Schema upgrade (older projects).** A project created under an earlier skill version may
lack fields the current schema defines. At orientation, backfill missing fields as a
byproduct — never stall on them and never ask permission for the observable ones:
- **Observable facts → record silently:** `TEAM` (list `.claude/agents/qgdw-*.md` once),
  `BUDGETS` (measure the hot files), `CHANGESET` (∅ if unknown — the next CLV rebuilds it).
- **Judgment fields → one proper ask:** a missing `RIGOR` means the project predates
  proportional rigor — run the calibration NOW as a standard three-options proposal from
  the project's current signals (size, users, exposure, data), then record it. That is the
  only question the upgrade should generate.
Then proceed to dispatch. "STATE is missing a field" is a 30-second backfill, not a
blocker and not a menu of options for the user.

## Index formats (Tier 1, one line per item)

**Decision Index** — head of `decisions.md`:
```
DEC-003 | persistence | chosen:managed-relational | rev:expensive | accepted | deps:5
```
**Active/dormant split (scale rule).** The head index holds only **ACTIVE** DECs — those
`accepted` with live dependents, `open`, or `needs-review`. Superseded DECs and accepted
one-shot decisions with no live dependents move under a `## Dormant index` divider (still
one line each, still ID-addressable, just not read by default). Routine decision-making
reads the active section only; budget ~20 lines / ~500 tokens. Breach = compaction trigger
(librarian Job 3).
**Trace Digest** — head of `traceability.md`: status counts + only non-Deployed rows.
**Open-Findings** — `findings.md`: only open findings, one line each, each carrying its BLOCKS
effect set (an empty set is written `[]` and means advisory):
```
F-018 | [TASK:REQ-028;CHECKPOINT:REQ-028;RELEASE] | use-after-free on the late-completion path | evidence:asan.log
F-022 | []                                        | stale internal note in wiki/architecture.md
```

Every row follows the ledger writing contract below.

The wiki's PAGES section names these so the agent knows they exist and when to read them.

## Ledger writing contract (the one place these rules live)

Ledgers are a cursor and an index, not a diary. Writing them costs tokens on every read
that follows, so write less, less often:

1. **When — at seams only:** a unit accepted or closed, a gate verdict, a decision taken, a
   phase change. A step, a round in flight, or a re-read writes nothing; a repair round
   adds at most a new finding row.
2. **Rows — fixed shape, ≤200 chars:** id · status · effects · one clause · one pointer.
   Correcting a row replaces a field; it never appends.
3. **DEC full entry — ≤~40 lines:** question · options (one line each with R/S/M/BP) ·
   chosen + why (≤3 lines) · cascade and dependents (≤3 lines) · reversibility. A revision
   is one dated line; a changed choice is a new DEC.
4. **No investigation prose in any ledger.** Mechanisms, measurements, round history, and
   reviewer back-and-forth live in the test, the commit message, or an agent report; the
   ledger holds a pointer (commit hash, test id, path:symbol, report path).
5. **One fact, one home.** Every other file points; nothing is restated or summarised.
6. **Over a cap = wrong home.** Move the content to where rule 4 sends it; never compress
   prose to squeeze it under the cap.

## Update-as-byproduct

| Operation | STATE patch | Index patch | WIKI patch |
|---|---|---|---|
| Start feature | OPEN+=REQ; CHANGESET+=REQ | trace row | — (unless new file) |
| Create test/source file | CHANGESET+=id | trace row (TEST ids only) | MAP line only if no line or parent dir covers it; **TEST.next** |
| Accept a decision | CASCADE if new dependent | decision index +1 | **DEC.next** |
| Disposition a finding | BLOCKING-=id | findings line→archive | — |
| CLV green | LAST_CLV; CHANGESET cleared | — | — (VAL.next only when a report file is written) |
| **Gate FAIL** | **verdict + blocker line (id[effects]) ONLY** | — | **— (no bump, no registry)** |
| Close a phase | PHASE; NEXT_GATE | — | **MAP repoint → archive** |

The index patch happens in the SAME edit as the cold-entry write, never later. An
operation that moved the cursor isn't finished until STATE reflects it; an operation that
didn't move it writes nothing. The Gate FAIL row follows failed-gate governance
(`orchestration.md`).

## Lessons at pre-flight
Guards from `lessons.md` are loaded *by tag* at the start of the matching operation (not
held in `STATE.md`). A guard currently blocking work appears in `STATE.BLOCKING` as a
finding id **with its BLOCKS effects**, scoped to what it actually stops. Capturing or incrementing a lesson is a byproduct of detecting a mistake (CLV
FAIL, user correction, failed create, missed cascade, A5 finding); see
`references/lessons-memory.md`.

## Archive layout (Tier 3)

```
references/archive/
  decisions-full.md     cycles/     validations/     findings-resolved.md
phase-digests.md        # one line per closed phase
```
ID-addressable; never read by routine work — only on an explicit Tier-3 trigger. The WIKI
MAP holds the pointers, so even archived material is located via the map, not by searching.
