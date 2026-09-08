# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-06 by librarian (Job-3 compaction)
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# ALL superseded cursor narrative -> .claude/workflow-garminconnect/archive/state-history.md
#   (§ 1-10 = pre-2026-09-06 history; § 11 = the FULL 883-line/117,284-char STATE.md this draft
#   replaces, extracted VERBATIM, md5 047736ac0f1d893794cf38b7d2298304 — nothing was deleted).
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58).
# Evidence tables, the gate-defect writeup, the stage table, the commit manifest and the
# exclusions list all moved to archive/state-history.md § 11 — read it for the full history.

PHASE:     2.2 · Garmin Connect integration, Stage 6 (UAF-family stubs) **CLOSED 2026-09-08**
           — Gates 1A/1B, Stages 2-5, and now Stage 6 all discharged on executed evidence;
           Stages 7-9 not yet open.
OPEN:      **STAGE 6 CLOSED 2026-09-08 (working tree, uncommitted) — all six REQs TEST VERIFIED
           on executed evidence.** REQ-020/022/023/024 TEST VERIFIED 2026-09-06..08
           (T-161..T-172); REQ-029 (DEC-041 base+amendment) + REQ-030 (new, DEC-044) TEST
           VERIFIED 2026-09-08 (T-173..T-175, `testGarminConnectFileIOLifetime`). Full gate run
           ONCE per the lean-evidence protocol: `ctest -L garmin-fast` **37/37 PASS, 0 FAIL**,
           331.73s — orchestrator independently confirmed against `build/Testing/Temporary/
           LastTest.log` (37 "Test Passed" / 0 "Test Failed"), not just the builder's report.
           B-R029-01/B-R025-01/A3-R021b-F2 all FIXED 2026-09-08 (both DECs mutation-verified;
           orchestrator also independently re-ran `testGarminConnectFileIOLifetime` directly on
           both backends before the full-gate pass — 5/5 PASS, zero ASan reports). REQ-030's
           fixture (`kFitSample`) swapped from a watts-less sample to `Garmin830_with_Stages.fit`,
           user-authorized 2026-09-08 (sole test-file edit; codex delta-check #5 confirmed no
           scope creep beyond the decided shape). Builder session mid-REQ-029 hit a weekly
           glm-5.3-flash quota exhaustion (resets 2026-09-12); replaced same-day by a fresh
           Sonnet 5 session (`garmin_builder_req029`, herdr pane w1:pM, tab w1:tE) which picked up
           the RED-phase test already on disk, found B-R029-01 before touching source, and
           carried both REQs to GREEN plus the Stage 6 close itself.
           **NEXT: commit decision is the user's (same as Stage 5) — not yet made this pass.**
BLOCKING:  — (none; B-R025-01/A3-R021b-F2/B-R029-01 all closed 2026-09-08, see above)
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py EXIT=0)
LAST_CLV:  clv_findings.py 2026-09-08 (re-run post-Stage-6-close) **PASS — 0 OUTSTANDING / 382 OK
           over 382 rows** (0 MALFORMED, 0 UNKNOWN-SEVERITY, 0 UNKNOWN-DISPOSITION,
           0 NEEDS-DISPOSITION, MISSING-EFFECT 0). OUTSTANDING 2→0 (S-R031-01/REQ-024 disposition
           landed pre-session; B-R029-01 opened THEN closed same-session, 2026-09-08); rows
           381→382 (+B-R029-01, non-blocking-turned-fixed). Orchestrator ran this directly, not
           carried forward from a prior pass.
NEXT_GATE: **Stage 6 CLOSED 2026-09-08** — all six REQs (020/022/023/024/029/030) TEST VERIFIED,
           all findings dispositioned, CLV PASS, full gate 37/37. Pending: user commit decision.
           Then Stage 7 (missing Phase-1 product surface) → Stage 8 (NF/coverage debt) → Stage 9
           (live Garmin + cross-platform). Full stage table → archive/state-history.md § 11.
CHANGESET: 2026-09-08 Stage-6-close pass touched src/FileIO/RideFile.cpp (DEC-041+DEC-044),
           unittests/Core/garminconnect/testGarminConnectFileIOLifetime.cpp (new, RED phase +
           the one authorized kFitSample edit) + its CMakeLists.txt wiring, plus governance
           (STATE.md/traceability.md/findings.md/decisions.md — REQ-029/REQ-030/DEC-044/
           B-R029-01). All uncommitted, working tree. **GIT-TRUTH FLAG (superseded from prior
           pass):** HEAD `441f7b510` still the last commit; this session's changes sit on top,
           uncommitted — commit decision is the user's, not yet made.
TEAM:      on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   WIKI ~3170tok/700 [BREACH, not in this pass's scope] · DECIDX ~2299tok/500
           [BREACH, not in this pass's scope] · LSN 68g/10 [BREACH — per-op-tag guard-merge
           needed, not fixed this pass] · FINDINGS 6 open (matches LAST_CLV). STATE/findings/
           lessons hot sizes all compacted this pass — full before/after table and the three
           newly-flagged (unfixed) breaches → archive/state-history.md § 12.
COUNTS:    REQ30 (REQ-020/022/023/024/029 built 2026-09-06..08; REQ-030 new+built 2026-09-08) ·
           DEC44 (44 with an entry) · DES14(+2 lettered) · TEST175 (T-161..163 REQ-020; T-164..166
           REQ-022; T-167..169 REQ-023; T-170..172 REQ-024; T-173..175 REQ-029+REQ-030) · LSN85 ·
           VAL18 · findings 382 rows (381 + B-R029-01, opened+closed same session 2026-09-08) ·
           last commit 441f7b510 (Stage 6 work still uncommitted working-tree; see CHANGESET)

Detail lives in: traceability.md (per-id spine) · findings.md (finding disposition;
archive/findings-detail.md for any row whose cell was capped this pass) · decisions.md
(## Decision index, then entries) · validations/archive/ + cycles/archive/ (historical
execution evidence) · archive/state-history.md (§ 1-10 pre-2026-09-06 history, § 11 = the
complete pre-compaction STATE.md this draft replaces). workflow-aicoach/ is a retired ledger
(provenance only — see .claude/workflow-INDEX.md).
