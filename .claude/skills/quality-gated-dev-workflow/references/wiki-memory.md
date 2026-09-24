# Wiki Memory — the project brain

The wiki memory is a small, stable map of the project that the agent reads **first**, every
session, to stay oriented. It exists to stop three failure modes that appear as a project
grows: getting lost, pulling wrong/stale context, and re-creating files or folders that
already exist.

## What the wiki is (and is not)

- **Is:** a *map* — what exists, where it lives, what governs it. Structural. Changes only
  when the project's structure changes (a new file, folder, ID range, component, or
  validation).
- **Is not:** the live status. That's `STATE.md` (the cursor). Never merge them; a map that
  churns every step stops being a reliable anchor.

Hub-and-spoke: `WIKI.md` is the hub (always read); `wiki/*.md` are spokes (read by name
from the hub's PAGES section). This keeps the always-on cost small while the detail is one
named hop away.

---

## `WIKI.md` — the hub (Tier 0, ~400–600 tokens)

```
# PROJECT WIKI — <name>            (the brain · read me first)
root:  /abs/path/to/project         schema: wiki-v1
phase: <0|1|2|3>                    live-status → STATE.md (read next)

## MAP — directory manifest (authoritative; check before creating ANYTHING)
<path or dir>            <one-line purpose>            [→ archive target if any]
...one line per real file/dir that matters...

## REGISTRIES — what exists (allocate next; never reuse, never re-create)
REQ  <lo>–<hi>  full:<loc>                      next:<n>
DEC  <lo>–<hi>  index:<loc>                      next:<n>
DES  <lo>–<hi>  <loc>                            next:<n>
TEST <lo>–<hi>  <loc>                            next:<n>
VAL  <lo>–<hi>  latest:<VAL-id> <verdict> · <loc> next:<n>
F    open→findings.md  resolved→archive/         next:F-<n>

## PAGES — wiki spokes (read the one named; don't explore blindly)
wiki/architecture.md — <what> · read when <trigger>
wiki/conventions.md  — <what> · read when <trigger>
wiki/glossary.md     — <what> · read when <trigger>
<other operational indexes the agent should know about, e.g. decisions.md head>

## ORIENTATION PROTOCOL
1 read this first   2 navigate by MAP/PAGES, never blind   3 check MAP before create
4 allocate IDs from REGISTRIES   5 lost? re-read this   6 update MAP/REGISTRIES as byproduct
```

**MAP rules.** One line per file/dir that matters (skip noise like `__pycache__`). Each
line is `location — purpose [→ archive]`. The MAP is *authoritative*: if reality and the
MAP disagree, that is drift to be reconciled (below), not a reason to create a duplicate.

**MAP budget & rollup (scale rule).** The MAP has a hard budget: **~40 lines / ~700
tokens**. Small projects map file-level; as the tree grows, **roll up to directory level**
— one line per directory whose contents share a purpose (`src/reader/  reader window UI
(11 files)`), with file-level lines kept only for governance files (WIKI, STATE, ledgers)
and single-file components. A file under a rolled-up directory is "in the MAP" via its
parent (the anti-duplication guard already resolves parent-dir coverage). When the MAP
exceeds budget, that is a compaction trigger: dispatch the librarian (Job 3) to roll up.
Fine-grained per-file notes, if ever needed, live in `wiki/map-detail.md` (Tier 1, read on
demand) — never in the hub.

**REGISTRIES rules.** **REGISTRIES point, they never restate.** Range + `next` + a pointer
to the canonical file — never per-id detail or per-id status (that lives in exactly one
canonical home; a restated copy is a second thing that drifts, and it is how a REGISTRIES
block grows to 18k chars unnoticed). Every ID type gets a contiguous range plus a `next`
counter and the canonical location of the full records. After editing WIKI.md, assert its
size against the budget (`wc -c` / 4 vs the ~700-token cap) as a byproduct. Allocation = read `next`, use it, increment it
here. IDs are never reused; a retired ID is marked `[retired]`, never recycled.

---

## Spoke templates

### `wiki/architecture.md` — the detailed project map (token-light)
The agent reads this to understand *how the system fits together* without ingesting all of
`design.md`. Tables and arrows, not prose.

```
# Architecture Map (spoke)            updated:<iso>

## Components
<Cn> <name> | <role one-liner> | gov:DEC-<ids> | code:<path>
...

## Data flow
<client/source> → <Cn> →(<edge label>)→ <Cn> → ...

## Integration points / contracts
<Cn>↔<Cm>: <contract one-liner> (DES-<id>)
...

## Watch  (live risks, usually from A2 pre-mortem / accepted findings)
- <risk> (<DEC/F id>; condition that would reopen it)
```

### `wiki/conventions.md` — where things go + anti-duplication
Read before creating anything. This is the front-line defense against re-creating folders.

```
# Conventions (spoke)

## Canonical locations (do not invent new ones)
requirements → prd.md          decisions → decisions.md (+index head)
design       → design.md       tests     → tests/<area>/test_<req>.py
cycle logs   → cycles/cycle-aN.md  (archive on phase close → archive/cycles/)
validations  → validations/VAL-NNN.md (→ archive/validations/)
live status  → STATE.md        map → WIKI.md + wiki/   code → src/<component>/

## IDs
REQ/DEC/DES/TEST/VAL/F · 3-digit zero-padded · sequential · never reuse · retire as
"[retired]". Allocate from WIKI REGISTRIES.next.

## Before creating a file or folder — anti-duplication checklist
1 In WIKI MAP? → open the existing file. Done.
2 Not present? → create at the canonical location above, then add it to WIKI MAP.
3 Create failed "already exists"? → MAP drift: add it to MAP, use the existing file.
Never create a second folder/file for an existing purpose. Never restructure the tree
without first updating WIKI MAP.
4 NEVER place project-owned files under `.claude/skills/` — that tree is vendor territory,
  replaced wholesale on every skill update; anything you put there is destroyed by the next
  reinstall (the guard denies new files there mechanically). Project tooling lives at a
  project path (`scripts/`, `tools/`) registered in the WIKI MAP. Local edits to skill
  files are lost the same way — fold improvements upstream instead.
```

### `wiki/glossary.md` — canonical meanings (prevents wrong-context drift)
```
# Glossary (spoke)
<term> = <definition>   [source: REQ/DEC/intake]
...
```

---

## Orientation protocol (how the agent uses the brain)

**Every session, and on any uncertainty:**
1. Read `WIKI.md`. This is research step one — the map of all project knowledge.
2. Read `STATE.md` for the live cursor.
3. From MAP/PAGES, go *directly* to what the task needs. Do not list or grep the tree to
   "discover" something the map already locates — that is how wrong context enters.

**Before creating anything:** run the `wiki/conventions.md` checklist. The single most
important rule: *a create that fails because the target exists means the MAP is stale —
fix the MAP and use the existing file. Never duplicate.*

**Before allocating an ID:** take `REGISTRIES.next`, use it, bump it.

**When lost or context feels wrong:** STOP. Re-read `WIKI.md`. Re-orient. The wiki is the
recovery anchor — do not push forward on a confused context, and do not start exploring
to "figure out where you are." The map already knows.

---

## Maintenance — update only on structural change (byproduct)

The wiki changes **only** when structure changes, which is far rarer than status change.
Triggers and the one-line edit each requires:

| Structural change | WIKI edit |
|---|---|
| New file/dir created | add a MAP line |
| New ID allocated | bump the relevant `REGISTRIES.next`; extend the range |
| New component / integration | add a line to `wiki/architecture.md` |
| New validation run | bump `VAL.next`; update `latest:` |
| Phase closed (archive-on-close) | repoint the MAP line `→ archive/...` |
| New project term defined | add a `wiki/glossary.md` line |

Each edit is the *tail* of the action that caused it — never a separate "update the wiki"
pass. Because the edits are tiny and infrequent, the brain stays both fresh and cheap.

---

## Reconciliation (healing drift without re-exploring)

The MAP is authoritative, but reality can drift (a file created outside the workflow, a
manual rename). Reconcile **only on a signal**, never as routine scanning:

- **Signal: a create failed because the target exists.** → Add the existing file to the
  MAP; use it. (Most common; this is the anti-duplication path.)
- **Signal: a referenced ID/file isn't where the MAP says.** → Do *one targeted* check of
  the canonical location, update the MAP, continue. Do not broaden into a full-tree scan.
- **Explicit user request, or project migration (§Migrating in SKILL.md).** → The one time
  a full bounded pass over the tree is sanctioned, to (re)build the MAP from scratch.
- **Wave gate in deny-only guard mode.** → With registration asks suppressed, nothing
  nudges new files into the MAP, so at each wave gate run the MAP-freshness count (tree
  entries not covered directly or via parent rollup); past a handful, dispatch a librarian
  sync. This is a scheduled check, not exploration.

Outside these signals, trust the map. Routine "let me just look around to be safe" is the
exact behavior the wiki exists to eliminate.

---

## Multi-project note

Each project has its own `WIKI.md` at its root, and that file's `root:` line states which
project it is. If work could touch more than one project, confirm the active project by its
`WIKI.md root:` before acting, so artifacts and IDs never bleed across projects.
