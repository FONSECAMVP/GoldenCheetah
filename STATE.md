# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-10 by Inspector (ledger reconciliation + REQ-015/DEC-046 defer — see line 14 note)
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# ALL superseded cursor narrative -> .claude/workflow-garminconnect/archive/state-history.md
#   (§ 1-10 = pre-2026-09-06 history; § 11 = the FULL 883-line/117,284-char STATE.md this draft
#   replaces, extracted VERBATIM, md5 047736ac0f1d893794cf38b7d2298304 — nothing was deleted).
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58).
# Evidence tables, the gate-defect writeup, the stage table, the commit manifest and the
# exclusions list all moved to archive/state-history.md § 11 — read it for the full history.

PHASE:     2.2 · Garmin Connect integration, Stage 6 (UAF-family stubs) **CLOSED 2026-09-08**
           — Gates 1A/1B, Stages 2-5, and now Stage 6 all discharged on executed evidence.
           Stage 7 (missing Phase-1 product surface) **OPEN, IN PROGRESS**: REQ-009 and REQ-014
           both committed (`abd1e119b`, `ac1fa40ba`, 2026-09-08 — DEC-045 + T-177; full status
           lives only in traceability.md per DEC-015). Ledger reconciled 2026-09-10 —
           traceability.md's REQ-009/014 rows plus REQ-029/030's stale "uncommitted" status, all
           corrected by the Inspector, no code changed. REQ-015 (CAPTCHA path) research concluded
           2026-09-10 — DEC-046 recorded, full resolution and status only in decisions.md/
           traceability.md per DEC-015; no code exists for it. Remaining Stage 7 (buildable):
           REQ-010, REQ-013, REQ-NF-Pkg-001, REQ-NF-Compat-001. Stages 8-9 not yet open.
OPEN:      **STAGE 6 CLOSED 2026-09-08, COMMITTED `4a72d2279`** — all six REQs TEST VERIFIED
           on executed evidence. REQ-020/022/023/024 TEST VERIFIED 2026-09-06..08
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
           **Committed 2026-09-08 by user decision** (25 files, +8291/-1306; clang-format
           pre-commit hook reformatted 5 test files cosmetically, re-verified 5/5 PASS each
           before the final commit). **NEXT: Stage 7 (missing Phase-1 product surface) — see
           NEXT_GATE below.**
BLOCKING:  — (none; B-R025-01/A3-R021b-F2/B-R029-01 all closed 2026-09-08, see above)
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py EXIT=0)
LAST_CLV:  clv_findings.py 2026-09-08 (re-run post-Stage-6-close) **PASS — 0 OUTSTANDING / 382 OK
           over 382 rows** (0 MALFORMED, 0 UNKNOWN-SEVERITY, 0 UNKNOWN-DISPOSITION,
           0 NEEDS-DISPOSITION, MISSING-EFFECT 0). OUTSTANDING 2→0 (S-R031-01/REQ-024 disposition
           landed pre-session; B-R029-01 opened THEN closed same-session, 2026-09-08); rows
           381→382 (+B-R029-01, non-blocking-turned-fixed). Orchestrator ran this directly, not
           carried forward from a prior pass.
NEXT_GATE: **Stage 6 CLOSED + COMMITTED 2026-09-08 (`4a72d2279`)** — all six REQs
           (020/022/023/024/029/030) TEST VERIFIED, all findings dispositioned, CLV PASS, full
           gate 37/37. **Stage 7 (missing Phase-1 product surface) OPEN, IN PROGRESS:**
           REQ-009 (ToS notice) **COMMITTED `abd1e119b`**; REQ-014 (friendly error
           translation) **COMMITTED `ac1fa40ba`** (DEC-045). REQ-015 (CAPTCHA path) — two
           independent research passes (builder + a fresh Codex session) read the real
           `garminconnect==0.3.13` dependency's source directly and confirmed it discards
           CAPTCHA's structured signal before any caller-visible exception attribute; only a
           message-text substring survives, which this project's LSN-006 forbids classifying on.
           User decision 2026-09-10: not buildable as scoped — see DEC-046 for the full resolution
           and alternatives considered; no code was written. **Remaining, not yet
           started:** REQ-010 (bulk backfill — design DES-009 never accepted), REQ-013 (profile
           auto-fill, nice), REQ-NF-Pkg-001 (installer bundling — **`garminconnect`/`curl_cffi` are in NO
           requirements or installer file on any platform, so the feature cannot reach a user at
           all today**), REQ-NF-Compat-001 (`docs/garminconnect-known-limits.md` does not
           exist). Then Stage 8
           (NF/coverage debt: Perf/Sec/Reliab/Obs/i18n bars + a REQ-NF-Build-001 regression
           guard) → Stage 9 (a real Garmin account exercising connect/MFA/sync/disconnect +
           Win/macOS/Linux installed-package smoke checklist — neither may be inferred from a
           seam test). Full stage table → archive/state-history.md § 11.
CHANGESET: 2026-09-08 Stage-6-close pass touched src/FileIO/RideFile.cpp (DEC-041+DEC-044),
           unittests/Core/garminconnect/testGarminConnectFileIOLifetime.cpp (new, RED phase +
           the one authorized kFitSample edit) + its CMakeLists.txt wiring, plus governance
           (STATE.md/traceability.md/findings.md/decisions.md — REQ-029/REQ-030/DEC-044/
           B-R029-01). **GIT-TRUTH FLAG (superseded twice since this CHANGESET was written):**
           at the time of this pass HEAD was `441f7b510`, uncommitted on top — since then the
           Stage 6 pass this CHANGESET describes was **committed as `4a72d2279`**, then REQ-009
           as `abd1e119b`, then REQ-014 as `ac1fa40ba` (current HEAD, 2026-09-10 re-verified).
TEAM:      on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   WIKI ~3170tok/700 [BREACH, not in this pass's scope] · DECIDX ~2299tok/500
           [BREACH, not in this pass's scope] · LSN 68g/10 [BREACH — per-op-tag guard-merge
           needed, not fixed this pass] · FINDINGS 6 open (matches LAST_CLV). STATE/findings/
           lessons hot sizes all compacted this pass — full before/after table and the three
           newly-flagged (unfixed) breaches → archive/state-history.md § 12.
COUNTS:    REQ30 (REQ-020/022/023/024/029 built 2026-09-06..08; REQ-030 new+built 2026-09-08;
           REQ-009/014 committed 2026-09-08, ledger-reconciled 2026-09-10; REQ-015 research
           concluded 2026-09-10, not buildable as scoped, no code) · DEC46 (DEC-045 added
           2026-09-10, recording-only, REQ-014 translation locus+keying; DEC-046 added 2026-09-10,
           recording the CAPTCHA-path outcome — dependency discards the structured signal) · DES14(+2 lettered,
           +1 dated addendum under DES-008) · TEST177 (T-161..163 REQ-020; T-164..166 REQ-022;
           T-167..169 REQ-023; T-170..172 REQ-024; T-173..175 REQ-029+REQ-030; T-176 REQ-009;
           T-177 REQ-014) · LSN85 · VAL18 · findings 382 rows (381 + B-R029-01,
           opened+closed same session 2026-09-08) · last commit `ac1fa40ba` (REQ-014,
           2026-09-08; REQ-009 `abd1e119b` and Stage-6-close `4a72d2279` precede it — see
           CHANGESET, which still describes the now-committed Stage 6 pass)

Detail lives in: traceability.md (per-id spine) · findings.md (finding disposition;
archive/findings-detail.md for any row whose cell was capped this pass) · decisions.md
(## Decision index, then entries) · validations/archive/ + cycles/archive/ (historical
execution evidence) · archive/state-history.md (§ 1-10 pre-2026-09-06 history, § 11 = the
complete pre-compaction STATE.md this draft replaces). workflow-aicoach/ is a retired ledger
(provenance only — see .claude/workflow-INDEX.md).
