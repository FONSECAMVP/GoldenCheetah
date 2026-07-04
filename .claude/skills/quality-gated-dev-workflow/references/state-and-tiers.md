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
BLOCKING:  <finding-ids | —>                 # ids only; detail in findings.md
CASCADE:   <DEC-ids + trigger | —>
LAST_CLV:  <VAL-id> <PASS|FAIL> · <n WARN(summary)>
NEXT_GATE: <gate name> → <remaining conditions>
CHANGESET: <ids touched since LAST_CLV>      # seeds incremental CLV
COUNTS:    REQ<n> DEC<n> DES<n> TEST<n> · last commit <#>
```

`OPEN/BLOCKING/CASCADE` carry IDs only — never inlined detail. `CHANGESET` is appended on
change and cleared when CLV goes green over it.

## Index formats (Tier 1, one line per item)

**Decision Index** — head of `decisions.md`:
```
DEC-003 | persistence | chosen:managed-relational | rev:expensive | accepted | deps:5
```
**Trace Digest** — head of `traceability.md`: status counts + only non-Deployed rows.
**Open-Findings** — `findings.md`: only open/blocking, one line each.

The wiki's PAGES section names these so the agent knows they exist and when to read them.

## Update-as-byproduct

| Operation | STATE patch | Index patch | WIKI patch |
|---|---|---|---|
| Start feature | OPEN+=REQ; CHANGESET+=REQ | trace row | — (unless new file) |
| Create test/source file | CHANGESET+=id | trace row | **MAP line + TEST.next** |
| Accept a decision | CASCADE if new dependent | decision index +1 | **DEC.next** |
| Disposition a finding | BLOCKING-=id | findings line→archive | — |
| CLV green | LAST_CLV; CHANGESET cleared | — | **VAL.next; latest:** |
| Close a phase | PHASE; NEXT_GATE | — | **MAP repoint → archive** |

If `STATE.md` (and, on structural change, `WIKI.md`) weren't touched, the operation isn't
finished.

## Lessons at pre-flight
Guards from `lessons.md` are loaded *by tag* at the start of the matching operation (not
held in `STATE.md`). A guard currently blocking work appears in `STATE.BLOCKING` as a
finding id. Capturing or incrementing a lesson is a byproduct of detecting a mistake (CLV
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
