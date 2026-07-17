# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **REQ-007 DONE and COMMITTED `d312886a6` (2026-07-18) — feat(garmin), 25 files, path-scoped to
Garmin only.** All A3 findings dispositioned; the build-integration gap fixed and app-link confirmed. Two
builder slices landed + verified:
(1) B-R007-01 fix — readFile completion is a queued self-post via `postReadComplete`/`QMetaObject::invokeMethod`
(TEST-024, not-synchronous + fires-exactly-once); resolves B-R007-02 (no Q_OBJECT needed). (2) Hardening —
the queued-post context was retargeted from `m_client` to an OWNED bare `QObject m_completionContext` member
(GarminConnect.h:123, declared last → destroyed first → Qt cancels a pending post before the base is torn down),
closing the A3-R007-01 use-after-free BY CONSTRUCTION (TEST-025); + TEST-026 pins readFile's null-guard
(A3-R007-03). A3-R007-02 (real-signal caller test) ACCEPTED as documented residual (needs heavyweight
real-CloudService; mechanism already mutation-proven + now lifetime-safe); A3-R007-04 accepted. **B-R007-03
(orchestrator-caught):** the main GoldenCheetah binary couldn't link — src/CMakeLists.txt GC_WANT_GARMINCONNECT
listed GarminConnect.cpp but omitted GarminTokenStore.cpp + AtomicFile.cpp (referenced by open()→loadChecked);
FIXED by adding both (+deps confirmed present by inspection). **Verification Gate PASS on both slices**
(orchestrator independently re-ran garmin ctest 14/14, readFile exe 13/13; verified member-destruction order,
context thread-affinity, link-gap root cause in source). A3 mutation-killed the revert + double-post mutants.

NEXT_GATE: **REQ-003 (MFA)** — REQ-007 is DONE + COMMITTED (`d312886a6`). Only release/manual-only, NON-blocking
items trail REQ-007: REQ-NF-Perf-003 end-to-end ≤5s stopwatch = manual Phase-3 item (TEST-024 covers the
mechanism); OQ1 Assumption-B 404→Network live smoke (needs real Garmin). The docs(garmin) ledger-record commit
(this hash into traceability/STATE/WIKI) follows the feat commit per LSN-010. Carried: DEC-014 OQ1 pending bundled
wheel. NOTE the working tree still carries unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root
`CMakeLists.txt`, `vcpkg.json`, `.claude/skills/**`, `.claude/agents/*`) — NOT part of the Garmin ledger; leave
them for their own owners. A future Garmin commit must stay path-scoped, never `git add -A`.

BLOCKING: **0 open.** B-R007-01 FIXED (TEST-024); B-R007-02 resolved-by-mechanism; B-R007-03 FIXED (CMake link
wiring — pending app-link confirmation, non-blocking); A3-R007-01 FIXED (m_completionContext, TEST-025);
A3-R007-03 FIXED (TEST-026); A3-R007-02 + -04 accepted residuals. Carried non-blocking: A3-R004-04/05/06
(fault-injection), -07 (concurrency, mitigated by DES-001 single-worker), -08 (root-run test), -09 (Windows CI);
TR-08 → Phase 1.5 with A2-001; TR-06 fidelity check; TR-03 guarded by TEST-007. Detail → findings.md.

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
Housekeeping: `scripts/__pycache__/` now gitignored (C1). **REQ-007 closure build is WORKING-TREE,
UNCOMMITTED** (user has not asked to commit): new `src/Cloud/{IGarminDownloadClient.h,GarminDownloadClient.*,
GarminDownloadChain.*}` + modified GarminConnect/GarminWorker/IGarminPyAdapter/PyEmbeddedAdapter/
garmin_client.py + new test cpps + test_adapter_restore.py + CMake wiring. **2026-07-18 additions to the
same working tree:** GarminConnect.cpp/.h (queued completion `postReadComplete` + owned `m_completionContext`
member — B-R007-01 + A3-R007-01), testGarminConnectReadFile.cpp (TEST-024/025/026), and
`src/CMakeLists.txt` (GC_WANT_GARMINCONNECT += GarminTokenStore.cpp + AtomicFile.cpp — B-R007-03).
ctest garmin 14/14 + readFile exe 13/13 re-verified. **COMMITTED `d312886a6`** (feat(garmin), 2026-07-18, 25
files): all Garmin production + tests + the src/CMakeLists.txt GC_WANT_GARMINCONNECT hunk ONLY — staged path-scoped
(the mixed src/CMakeLists.txt was hunk-split so Coach/calendar changes stayed unstaged). Pre-commit hooks ran:
clang-format reformatted GarminConnect.cpp/GarminDownloadChain.cpp (brace/wrap only, no #include reorder) + ruff
unquoted one annotation in garmin_client.py; re-verified post-format (garmin ctest 14/14, GoldenCheetah re-links
clean) per LSN-007 before the commit. Unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`,
`vcpkg.json`, `.claude/skills/**`, `.claude/agents/*`) remain UNCOMMITTED — not Garmin, left for their owners.

LAST_CLV: VAL-013 — 2026-07-18 — incremental over the B-R007-01 fix changeset. Validator returned FAIL
on merge-lag ONLY (traceability/WIKI/findings not yet reflecting TEST-024 + the fix); substantive code
checks PASSED (readFile still matches DEC-016 verbatim, DES-014 non-Q_OBJECT honored, design.md honest/no
false-done). Merge-lag remediated this pass (TEST-024 rows added to REQ-007 + REQ-NF-Perf-003 + DES-004/
component cells; WIKI TEST→T-024/next-025; findings B-R007-01→fixed, -02→resolved-by-mechanism); each cited
target grep-verified present post-merge. Net: PASS-after-remediation. Prior: VAL-012 — 2026-07-13 (DEC-015);
VAL-011 — 2026-07-12 (REQ-006). Full canonical VAL table → traceability.md ## Validations run.
LAST_CYCLE: A3-R007 (REQ-007 B-R007-01 fix) — 2026-07-18 — FINDINGS: fix VERIFIED sound (mutation-killed
revert-to-Direct + double-post mutants; remotename-override cleared as non-defect vs Strava precedent). 4
non-blocking surfaced: -01 injected-seam UAF (ASan-demonstrated, not production-reachable), -02 TEST-024
non-QObject-stub proxy gap, -03 readFile null-guard uncovered, -04 QTRY CI-speed (accepted). Prior: A3-R006
(REQ-006) — 2026-07-12 (mask-narrowing + perms-cache fixed via TEST-014 +3 slots). Cycle narratives → cycles/.

Detail lives in: traceability.md (per-id status spine) · findings.md (finding disposition) ·
decisions.md (DEC entries) · validations/ + cycles/ (evidence). workflow-aicoach/ is a retired
ledger (provenance only, see .claude/workflow-INDEX.md).
