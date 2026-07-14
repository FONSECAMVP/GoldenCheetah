# Conventions (spoke)

## Canonical locations (do not invent new ones)
workflow machinery   → .claude/skills/quality-gated-dev-workflow/ (SKILL.md, agents/, references/, scripts/)
subagents (live)     → .claude/agents/qgdw-{scout,builder,adversary,validator,librarian}.md
                        (canonical copies also under skills/quality-gated-dev-workflow/agents/ — ANOMALY, see report; treat .claude/agents/ as the one Claude Code actually loads)
hook wiring          → .claude/settings.json (PreToolUse) + .claude/hooks/anti_duplication_guard.py
ledger index         → .claude/workflow-INDEX.md — always read this before assuming "the" ledger; this
                        project has TWO ledgers, one active one closed (see below)
live cursor          → root STATE.md — the SOLE cursor (DEC-015; no per-ledger state.md)
active feature ledger → .claude/workflow-garminconnect/ (Garmin Connect integration, Phase 2.2)
  - per-id status    → traceability.md   - decisions            → decisions.md (+ index head)
  - requirements     → prd.md            - design                → design.md
  - traceability     → traceability.md   - findings register     → findings.md
  - ambiguities      → ambiguities.md    - definition of done    → dod.md
  - original ask     → intake.md         - reusable options table→ options-catalog.md
  - cycle logs       → cycles/active/*.md (open) → cycles/archive/*.md (closed)
  - validations      → validations/active/val-NNN.md (recent) → validations/archive/ (older)
  - helper scripts   → scripts/clv-lite.sh
closed ledger        → .claude/workflow-aicoach/ (AI Coach, SHIPPED) — provenance only, flat
                        cycles/cycle-aN.md + validations/validation-NNN.md layout, no state.md
                        by design. NEVER rebuild an active state.md from this ledger.
GC application code  → src/<Component>/ (ANT, Charts, Cloud, Coach, Core, FileIO, Gui,
                        Metrics, Planning, Python, R, Resources, Train) — one dir per
                        subsystem; new subsystems get a new top-level src/<Name>/, not a
                        subfolder of an unrelated one.
Garmin C++ code      → src/Cloud/Garmin*.{h,cpp}, src/Cloud/IGarmin*.h
Garmin Python code   → src/Python/garminconnect/ (vendored adapter package; own
                        pyproject.toml, tests/, .venv — pytest + coverage.py per DEC-008)
GC tests             → unittests/<Area>/<feature>/ (QTest+CTest, e.g. unittests/Core/garminconnect)
                        + src/Python/garminconnect/tests/ (pytest)
fixture/sample data  → test/<kind>/ (rides, workouts, charts, …) — NOT test code, do not
                        confuse with unittests/
commit convention    → commits that close a REQ/TEST/DEC step cite the ids in the message
                        (see traceability.md "Commit" column for the pattern already in use,
                        e.g. "1c355a102" cited against TEST-002/REQ-002)
build systems        → CMakeLists.txt (new, in migration) AND *.pro/qmake (legacy, still
                        authoritative until migration completes) — check BOTH when adding a
                        new source file to a component; do not add to only one.
docs (current)       → docs/*.md      docs (legacy/asset archive) → doc/ (do not add new
                        markdown docs under doc/ — it is the pre-migration archive; use docs/)

## IDs
- Base scheme: REQ/DEC/DES/TEST/VAL · 3-digit zero-padded (DEC-001) · sequential · never
  reuse · retire as "[retired]".
- **Per-ledger namespace rule (mandatory — see LSN-002):** every feature ledger
  (`.claude/workflow-<feature>/`) owns its OWN REQ/DEC/DES/TEST/VAL counters starting at
  001. IDs are NOT globally unique across ledgers — garmin:DEC-013 and aicoach:DEC-013 are
  different decisions. Always qualify cross-ledger references with the ledger's short
  prefix (`coach:DEC-NNN`, `garmin:DEC-NNN`) — this convention is already established in
  workflow-garminconnect/decisions.md's own header; extend it, do not invent a new scheme.
- Findings do NOT use an "F-###" registry in this project: they use `<cycle>-<seq>` (e.g.
  `A1-001`, `A3-R002-M6`) or `D-0x` (drift items), scoped per ledger, tracked in that
  ledger's findings.md. If a future ledger wants F-### numbering, start it at F-001 in that
  ledger's own findings.md and say so explicitly in its header (do not retrofit old ledgers).
- LSN ids (process lessons) are project-global (one lessons.md at root), not per-ledger —
  they describe agent/process mistakes, not feature content.
- Allocate every id from WIKI.md REGISTRIES.next (ledger-qualified); bump next after use.

## Ledger status — one fact, one home (DEC-015, retires LSN-008)
A per-id lifecycle status is a SINGLE logical fact and MUST live in exactly one place.
Duplicating it across files is what caused LSN-008 (recur:7). Rules:
- **Canonical homes (the ONLY files that pair an id with a status token):**
  - REQ / DES / TEST / VAL status → `.claude/workflow-<feature>/traceability.md`
    (primary REQ→DEC→DES→TEST→COMMIT matrix; DES index Status cells; Validations table).
  - DEC status → `.claude/workflow-<feature>/decisions.md` per-DEC `Status:` field.
    (The traceability DEC-index carries Question+Date only — drill decisions.md for status.)
- **Everywhere else in the live governance set** (`STATE.md`, `WIKI.md`, `design.md`,
  `wiki/*`) an id may be *referenced* but MUST NOT appear on the same line as a lifecycle-
  status token. Root `STATE.md` is the sole live **cursor** (there is NO per-ledger
  `state.md` — deleted in DEC-015): it names the current gate, changeset (commit hashes =
  provenance), and open findings, and points to traceability for per-id status.
- **`design.md` describes design INTENT** (target shape), never live status — write DES
  bodies and "DEC-NNN refinement" notes in target tense, never "DONE"/"still forwards"
  (this also subsumes LSN-011).
- **Provenance is not status.** Dated per-slice "artifacts" appendix tables and commit
  hashes record what was built at a point in time; they are excluded from the status rule
  and MUST carry an `<!-- provenance: dated; excluded from status-lint -->` marker.
- **Controlled vocabulary + state machine (never invent synonyms; re-count from source,
  never copy a count between docs):**
  `drafted → GREEN → committed → CLOSED`, with `deferred` as an orthogonal tag on a
  not-yet-built slice. DEC-status uses `accepted (X)` / `superseded` / `[retired]`.
- **Enforcement:** the absence-check `ledger_drift_lint.py` (installed via
  `install_hook.py`, runs pre-commit + in the CLV) fails if any status token pairs with an
  id in the NON-canonical governance set (`STATE.md`, `WIKI.md`, `wiki/*.md`, `design.md`).
  The canonical ledgers (`traceability.md`, `decisions.md`) are the source of truth and are
  NOT scanned — they legitimately co-locate ids with status (the REQ→DEC→DES→TEST matrix,
  DEC cascade-notes). It is the mechanism LSN-008 was promoted into.

## Before creating a file or folder — anti-duplication checklist
1 In WIKI MAP? → open the existing file. Done.
2 Not present? → create at the canonical location above, then add it to WIKI MAP.
3 Create failed "already exists"? → MAP drift: add it to MAP, use the existing file.
4 About to create a SECOND ledger, docs dir, or agents/ copy? → check this file's
  "Canonical locations" table first — this project already has doc/ vs docs/ and
  .claude/agents/ vs skills/.../agents/ duplication; don't add a third variant.
Never create a second folder/file for an existing purpose. Never restructure the tree
without first updating WIKI MAP.
