# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **DEC-015 (ledger status SSOT + absence-check lint) — COMPLETE, VAL-012 PASS.** The lint
(TEST-017, 15/15) is built + installed (`.claude/hooks/ledger_drift_lint.py` + a pre-commit local
hook); the non-canonical governance set (STATE.md / WIKI.md / wiki/* / design.md) carries NO per-id
status token (lint exit 0 tree-wide); the per-ledger `state.md` is deleted (root STATE.md is the
sole cursor); LSN-008 promoted to MECHANISM; LSN-014 (SSOT design) + LSN-015 (cascade verification)
captured. The whole DEC-015 delta is now **committed to master** — `88d4ea402` (lint mechanism
+ TEST-017 + wiring + gitignore) + `2520ed034` (ledger normalization + local state.md deletion) —
see CHANGESET.

NEXT_GATE: feature queue (DEC-015 delta committed `88d4ea402`+`2520ed034`):
REQ-007 CLOSURE (GarminConnect::readFile staging garmin-<id>.<ext> + FIT→TCX fallback DES-004 —
needs the worker-in-CloudService lifecycle + PRD-Assumption-B library validation) OR REQ-003 (MFA).
Carried: DEC-014 OQ1 (real-lib dumps/loads + 2-arg `Garmin(email,password)` signatures) as a build
NOTE pending the bundled wheel; OQ2 → REQ-NF-Compat-001(b) `session_expired`.

BLOCKING: 0 open. Non-blocking carries: A3-R004-04/05/06 (fault-injection), -07 (concurrency,
mitigated by the DES-001 single-worker), -08 (root-run test), -09 (Windows CI); TR-08 → Phase 1.5
with A2-001; TR-06 fidelity check; TR-03 guarded by TEST-007. Finding disposition detail (incl.
A3-R004 M1/M2/M3 + A3-R006-01/-02 resolutions) → findings.md.

CASCADE: DEC-015 (status SSOT) fully propagated — traceability.md/decisions.md (canonical status),
design.md/STATE.md/WIKI.md/wiki/* (status stripped), local state.md (deleted), ledger_drift_lint.py +
install_hook.py + .pre-commit-config.yaml (mechanism), lessons.md (LSN-008→MECHANISM, LSN-014/015). No
stale dependents (VAL-012 confirmed). Prior: DEC-014 fully propagated + committed.

CHANGESET (recent commits — provenance): REQ-006 `d86323246` (Slice A) + `3edb705cb` (Slice B) +
`458a72ba7` (A3-R006 hardening) + docs-record `808fda03a`/`04ab54d63`; REQ-004 `54b7005e6`
(+`b67767380`); REQ-007 `1eb5a6a16`; REQ-002 `60a076848`. **DEC-015 governance delta committed to
master:** `88d4ea402` (drift lint + TEST-017 + install/pre-commit wiring + `__pycache__/` gitignore) +
`2520ed034` (ledger normalization + local state.md deletion + WIKI/STATE/conventions). Production
`src/Cloud/GarminConnect.*` untouched (readFile is a later slice). Still unstaged (out of DEC-015 scope):
pre-session work (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`, `vcpkg.json`) + broader
skill-methodology edits (`.claude/skills/**`, `.claude/agents/*`, `anti_duplication_guard.py`).
Housekeeping: `scripts/__pycache__/` now gitignored (C1).

LAST_CLV: VAL-012 — 2026-07-13 — PASS (full 9-check, DEC-015 status-SSOT migration; PASS on the
re-verification pass after a 1-pass repair — dangling state.md pointers + DEC-index Status column,
both outside the lint's scope → LSN-015). Prior: VAL-011 — 2026-07-12 (REQ-006); VAL-010 — 2026-07-11
(REQ-004). Full canonical VAL table → traceability.md ## Validations run.
LAST_CYCLE: A3-R006 (REQ-006) — 2026-07-12 — findings fixed (A3-R006-01 mask-narrowing + -02
perms-cache, via TEST-014 +3 slots; mutant-kill demonstrated). Prior: A3-R004 (REQ-004) —
2026-07-11 (M1/M2 fixed; M3 resolved by REQ-006 Slice B). Cycle narratives → cycles/.

Detail lives in: traceability.md (per-id status spine) · findings.md (finding disposition) ·
decisions.md (DEC entries) · validations/ + cycles/ (evidence). workflow-aicoach/ is a retired
ledger (provenance only, see .claude/workflow-INDEX.md).
