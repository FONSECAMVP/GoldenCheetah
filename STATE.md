# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-12 by Inspector (Stage 8 — REQ-NF-Sec-001..004 all MET+COMMITTED; REQ-NF-Reliab-001+002 T-204..206 TEST VERIFIED, not yet committed)
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# ALL superseded cursor narrative -> .claude/workflow-garminconnect/archive/state-history.md
#   (§ 1-10 = pre-2026-09-06 history; § 11 = the FULL 883-line/117,284-char STATE.md this draft
#   replaces, extracted VERBATIM, md5 047736ac0f1d893794cf38b7d2298304 — nothing was deleted).
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58).
# Evidence tables, the gate-defect writeup, the stage table, the commit manifest and the
# exclusions list all moved to archive/state-history.md § 11 — read it for the full history.

PHASE:     2.2 · Garmin Connect integration, Stage 6 (UAF-family stubs) **CLOSED 2026-09-08**,
           Stage 7 (missing Phase-1 product surface) **CLOSED 2026-09-12** (see COUNTS-ADDENDUM-2
           below), Stage 8 (NF/coverage debt) now OPEN — Gates 1A/1B, Stages 2-5, 6, and 7 all
           discharged on executed evidence. Stage 7 history follows, retained for context: REQ-009
           and REQ-014
           both committed (`abd1e119b`, `ac1fa40ba`, 2026-09-08 — DEC-045 + T-177; full status
           lives only in traceability.md per DEC-015). Ledger reconciled 2026-09-10 —
           traceability.md's REQ-009/014 rows plus REQ-029/030's stale "uncommitted" status, all
           corrected by the Inspector, no code changed. REQ-015 (CAPTCHA path) research concluded
           2026-09-10 — DEC-046 recorded, full resolution and status only in decisions.md/
           traceability.md per DEC-015; no code exists for it. REQ-010's backfill controller +
           DES-005's rate limiter committed `aa6131add` 2026-09-10 — a reviewer delta-check
           caught a real defect (see B-R010-01, fixed same session before commit) and a real
           design-vs-dependency reconciliation (DEC-047); full resolution and status only in
           decisions.md/traceability.md/findings.md per DEC-015. **REQ-010's UI wiring committed
           `2b8cedae3` 2026-09-11 — closes B-R010-04.** New `GarminBackfillDialog` reachable via
           the Athlete Accounts page; two reviewer delta-check passes + one orchestrator diff read
           found and fixed five further defects (B-R010-07..09, all FIXED; B-R010-10 deferred by
           user decision → DEC-048; B-R010-11 informational, disclosed not fixed). Full resolution
           only in decisions.md/traceability.md/findings.md per DEC-015. **Stage 7 CLOSED 2026-09-12**
           — REQ-013 (`e17262a0b`), REQ-NF-Pkg-001 (`e609215f0`), REQ-NF-Compat-001
           (`0f654f4a5`) all committed; see COUNTS-ADDENDUM-2 below for the reconciliation.
           **Stage 8 (NF/coverage debt) now OPEN** — see NEXT_GATE. Stage 9 not yet open.
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
           before the final commit). Stage 7 is now CLOSED (2026-09-12) — see NEXT_GATE below for
           Stage 8, the current next gate.**
BLOCKING:  — (none; B-R025-01/A3-R021b-F2/B-R029-01 all closed 2026-09-08, see above)
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py EXIT=0)
LAST_CLV:  clv_findings.py 2026-09-12 (re-run post-REQ-NF-Reliab-001+002/T-204..206 ledger update)
           **PASS — 0 OUTSTANDING / 394 OK over 394 rows** (unchanged — no new findings opened;
           the T-206 id-collision was a ledger-hygiene fix, not a finding). `ledger_drift_lint.py`
           also re-run clean (EXIT=0). Inspector ran both directly against the just-edited files.
           NOTE (carried forward, still unresolved):
           this run's row count (394) does not match COUNTS-ADDENDUM-2's stated "findings 401
           rows" — not independently re-derived this pass either; the live clv_findings.py count
           above remains authoritative. Prior: 2026-09-12 (post-REQ-NF-Build-001/T-202 update)
           PASS — 0 OUTSTANDING / 394 OK over 394 rows; 2026-09-11 (post-B-R010-04) PASS — 393/393.
NEXT_GATE: **Stage 8's first item, REQ-NF-Build-001 (T-202, build regression guard), is
           TEST VERIFIED + COMMITTED `f118691ea` 2026-09-12.** Stage 8's second item,
           REQ-NF-Sec-001+003 (T-203), is **TEST VERIFIED + COMMITTED `d271abae1` 2026-09-12.**
           Stage 8's third item, REQ-NF-Sec-004 (docs-only file-based-token residual-risk
           disclosure), is **TEST VERIFIED + COMMITTED `07501c477` 2026-09-12** — see
           COUNTS-ADDENDUM-5 below. All four REQ-NF-Sec sub-items now MET. Stage 8's fourth item,
           REQ-NF-Reliab-001..002 (retry schedule + never-partial-write resumability), is
           **TEST VERIFIED + COMMITTED `51bc6b5a8`** (2026-09-12) — see COUNTS-ADDENDUM-6 below. Next
           atomic unit: the remaining Stage 8 NF items (Perf/Obs/i18n bars — none started, see
           traceability.md's REQ-NF-Perf-001..003/Obs-001/i18n-001 rows).
           Stage 6 CLOSED + COMMITTED
           2026-09-08 (`4a72d2279`, all six REQs TEST VERIFIED, all findings dispositioned, CLV
           PASS, full gate 37/37) and **Stage 7 CLOSED 2026-09-12** (history below, retained for
           context). Stage 7 history: REQ-009 (ToS notice) **COMMITTED `abd1e119b`**; REQ-014 (friendly error
           translation) **COMMITTED `ac1fa40ba`** (DEC-045). REQ-015 (CAPTCHA path) — two
           independent research passes (builder + a fresh Codex session) read the real
           `garminconnect==0.3.13` dependency's source directly and confirmed it discards
           CAPTCHA's structured signal before any caller-visible exception attribute; only a
           message-text substring survives, which this project's LSN-006 forbids classifying on.
           User decision 2026-09-10: not buildable as scoped — see DEC-046 for the full resolution
           and alternatives considered; no code was written. REQ-010 (bulk backfill) + DES-005
           (rate limiter) **COMMITTED `aa6131add`** 2026-09-10; a reviewer delta-check caught and
           the builder fixed a real defect same session, before commit
           (B-R010-01: silently-ignored atomic-write failures) and DES-005's coverage gap
           on `login`/`submit_mfa` (B-R010-02); the pagination-model reconciliation against the
           real dependency is recorded in DEC-047 (B-R010-03, accepted residual). **REQ-010's UI
           wiring COMMITTED `2b8cedae3` 2026-09-11 — closes B-R010-04.** New `GarminBackfillDialog`
           (Garmin-only, `#ifdef GC_WANT_GARMINCONNECT`-guarded) reachable via a "Backfill..."
           button on the Athlete Accounts page; visible progress, working Cancel,
           `RideImportWizard` hand-off. Two reviewer delta-check passes + one orchestrator diff
           read found five further defects before commit: B-R010-07 (default-build link
           break, no `#ifdef` guard), B-R010-08 (session-latch bypass, cross-account data-leakage
           risk), B-R010-09 (three composed `Context*`-lifetime defects), B-R010-10 (deeper
           pre-existing `GarminConnect`/`CloudService` internal context-handling gap, shared by
           REQ-007/008/012/017 — see DEC-048), B-R010-11 (informational). Disposition detail for
           each only in decisions.md/traceability.md/findings.md per DEC-015. **Stage 7 CLOSED
           2026-09-12** — REQ-013 (profile auto-fill, `e17262a0b`), REQ-NF-Pkg-001 (installer
           bundling, `e609215f0`), REQ-NF-Compat-001 (`docs/garminconnect-known-limits.md`,
           `0f654f4a5`) all committed and ledger-recorded. **Stage 8 now OPEN**
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
COUNTS-ADDENDUM (2026-09-11, not yet folded into the block above — a full COUNTS reconciliation
           is a separate task): REQ-010+DES-005 committed `aa6131add`/`f23e4e2bc` 2026-09-10 (DEC-047,
           B-R010-01..06); REQ-010's UI wiring (B-R010-04) committed `2b8cedae3` 2026-09-11 (DEC-048,
           B-R010-07..11) — last commit on this branch is now `2b8cedae3`. Tests grown to T-196
           (testGarminBackfillController 20 slots, testGarminConnectOpen 11 slots,
           testGarminBackfillDialogLifetime new 5 slots). DEC48. findings 393 rows (388 + 5:
           B-R010-07/08/09 fixed, B-R010-10 deferred, B-R010-11 informational — see LAST_CLV).
COUNTS-ADDENDUM-2 (2026-09-12, not yet folded into the block above): REQ-NF-Pkg-001 (DEC-049,
           src.pro/CMakeLists.txt packaging-absence fix) committed `e609215f0` 2026-09-11,
           ledger-recorded `ee15a6e6e`, stale US-table row fixed `98b89f327`. REQ-NF-Compat-001
           (known-limits doc + connect-dialog notice) committed `0f654f4a5` 2026-09-11,
           ledger-recorded `e79545bb7` — independently re-verified 40/40 `garmin-fast`. A
           user-requested qmake/CMake Garmin-parity audit (separate Codex session) came back
           clean: no other REQ shares REQ-NF-Pkg-001's src.pro-absence gap; two low-priority
           side findings only (untranslated Garmin strings, covered by the already-deferred
           REQ-NF-i18n-001; an unrelated pre-existing CMake GC_WANT_PYTHON/GC_HAVE_PYTHON naming
           mismatch, no REQ allocated). DEC-050 (REQ-013 scope narrowed to dob/weight/height,
           hr_max/ftp_w deferred) recorded `459990bb6` (HEAD at the time this paragraph was first
           written — since superseded, see below). **REQ-013 built and TEST VERIFIED** — new
           T-201 (Python `test_adapter_profile.py`
           16 slots; `testGarminConnectPyAdapter` +8 marshalling slots; `testGarminConnectWizardRouting`
           15→20 slots). Reviewer delta-check caught a real blocking UAF (raw `Context*` dangling
           across an async fetch after athlete-tab-close) — **FIXED same session, ASan-proven**
           (B-R013-01). Two self-found pre-existing link-failure stubs also fixed (incomplete
           `PyEmbeddedAdapter` overrides in `ImportSeamStubs.cpp`/`ProviderSeamStubs.cpp`).
           Independently re-run by the orchestrator: `garmin-fast`+`garmin-py` 41/41, full `ctest`
           42/42, clang-format clean on gate-matched files. findings 401 rows (400 + B-R013-01).
           **REQ-013 COMMITTED `e17262a0b`** 2026-09-12 (28 files, Garmin-only set, verified
           zero overlap with the unrelated Coach/Qt6.8 work still sitting uncommitted in this
           same tree) — traceability.md's REQ-013 row and the US-5 row both said
           "UNCOMMITTED"/"Commit pending" until this pass; that was stale ledger prose written
           before the commit landed and never corrected. Reconciled 2026-09-12 by the Inspector
           (inspector-cycle pilot run), no code changed. **Stage 7 (missing Phase-1 product
           surface) is now CLOSED (2026-09-12)** — its three remaining items are all committed:
           REQ-013 `e17262a0b`, REQ-NF-Pkg-001 `e609215f0`, REQ-NF-Compat-001 `0f654f4a5`.
           **NEXT: Stage 8 (NF/coverage debt)** — see NEXT_GATE below.
COUNTS-ADDENDUM-3 (2026-09-12, not yet folded into the block above): Stage 8's first item,
           REQ-NF-Build-001 (the `GC_WANT_GARMINCONNECT` CMake-flag build regression guard),
           **TEST VERIFIED — COMMITTED `f118691ea` 2026-09-12.** New T-202: `unittests/buildguard/
           garmin_flag_build_guard.sh` + `unittests/buildguard/CMakeLists.txt` (ctest label
           `garmin-build-guard`, TIMEOUT 5400, registered unconditionally for both flag values)
           + a 4-line `unittests/CMakeLists.txt` wire-in. RED verified for the right reason
           first (an injected B-R010-07-class leak failed the OFF leg at link with the expected
           undefined reference; reverted, `git diff` empty on the touched file). GREEN
           independently re-run by the Inspector (not just relayed): `ctest -L
           garmin-build-guard` 2/2 Passed (OFF 741.14s, ON 768.24s — both full fresh builds,
           since unrelated in-flight tree churn invalidated the scratch dirs' warm state
           between the builder's own run and this one) and `ctest -L garmin-fast` re-confirmed
           40/40 (315.11s), no collateral. Findings: none opened (only RED was the intentional,
           reverted mutation). Judgment calls recorded in traceability.md's REQ-NF-Build-001 row
           (CMake/CTest-only, not qmake-CI wiring; deliberately outside the `garmin-fast` label).
           No pre-commit hook regex covers the three touched paths.
COUNTS-ADDENDUM-4 (2026-09-12, not yet folded into the block above; commit status superseded —
           see NEXT_GATE above): Stage 8's second item,
           REQ-NF-Sec-001+003 (password-never-persisted + no-`verify=False` source guards),
           **TEST VERIFIED — COMMITTED `d271abae1` 2026-09-12.** New T-203: `unittests/buildguard/
           garmin_sec_source_guard.py` (static, `testGarminSecSourceGuard` — 8 regex rules over
           47 Garmin-scope files, comment/docstring-stripped) + `unittests/Core/garminconnect/
           testGarminConnectPasswordPersistence.cpp` (runtime — real embedded CPython,
           HOME/XDG_*/TMPDIR redirect pre-`Py_Initialize`, UTF-8+UTF-16LE byte scan of the whole
           sentinel tree, canary file proves the scan isn't vacuous) + two CMakeLists.txt
           registrations, label `garmin-sec-guard`. RED verified for the right reason on both
           halves (4 injected static violations caught by rule id, comment-only mentions
           correctly produced zero findings; a mutated stub persisting the password under the
           redirected HOME caught at the runtime layer, pinpointing the leak file). Two real bugs
           self-found and fixed in the guard itself during RED (a regex alternation-precedence
           bug letting bare `password` match anywhere; a fail-open Python comment-stripper that
           only scanned comment lines). **Independently re-run by the Inspector on a
           from-scratch rebuild** (deleted the builder's own build artifacts first, not just
           re-executed them): `ctest -L garmin-sec-guard` 2/2 Passed (0.19s), `garmin-fast`
           re-confirmed 40/40 (313.41s), `pytest` (src/Python/garminconnect) re-confirmed 51/51
           (0.06s) — no collateral. Cross-file citation verified real (the C++ test cites
           `tests/test_token_store.py::test_dumped_blob_never_contains_the_password`, confirmed
           to exist exactly as cited). REQ-NF-Sec-002 confirmed untouched and still MET
           (re-verified, not just trusted, that testAtomicFile/testGarminTokenStore/
           testGarminSidecarStore already self-tag it). NF-Sec-004 confirmed still NOT STARTED
           (docs-only gap — checked `docs/garminconnect-known-limits.md` directly, the
           same-user-malware-can-replay-tokens disclosure is not there). **Awaiting commit** —
           **Committed `d271abae1` 2026-09-12** (7 files: STATE.md/WIKI.md/traceability.md +
           the two new tests + two CMakeLists.txt wirings). Next Stage 8 item: NF-Sec-004
           (docs-only) — see COUNTS-ADDENDUM-5.
COUNTS-ADDENDUM-5 (2026-09-12, not yet folded into the block above): Stage 8's third item,
           REQ-NF-Sec-004 (file-based-token-storage residual-risk disclosure, docs-only per
           prd.md's verification method — no new test harness), **TEST VERIFIED — COMMITTED
           `07501c477` 2026-09-12** (4 files: `docs/garminconnect-known-limits.md` new fourth
           section, `README.md` pointer extended, `src/Cloud/GarminCredentialsPage.cpp`
           first-connect `setSubTitle()` extended, `unittests/Core/garminconnect/
           testGarminConnectCredentialsPage.cpp` +1 supplementary slot `pageHasSecurityNotice()`
           beside the existing `pageHasReloginNotice()` — no new T-id consumed, matching
           REQ-NF-Compat-001's precedent; WIKI.md's TEST registry unchanged, still
           `next:garmin-T-204`). **Independently rebuilt and re-run by the Inspector**:
           `testGarminConnectCredentialsPage` (18/18) + `testGarminConnectWizardRouting` (20/20)
           both GREEN including the new slot, `garmin-sec-guard` 2/2, full `garmin-fast` 40/40 —
           no collateral; clang-format `--dry-run --Werror` clean on both touched `.cpp` files;
           `git status --short` on the 4 named paths matched the builder's claimed footprint
           exactly before staging. `clv_findings.py` re-run PASS (0 OUTSTANDING/394 OK,
           unchanged), `ledger_drift_lint.py` re-run clean (EXIT=0). **All four REQ-NF-Sec
           sub-items (001/002/003/004) now MET.** Next Stage 8 item: the remaining NF bars
           (Perf/Reliab/Obs/i18n — none started).

COUNTS-ADDENDUM-6 (2026-09-12, not yet folded into the block above): Stage 8's fourth item,
           REQ-NF-Reliab-001..002 (retry schedule + never-partial-write resumability),
           **TEST VERIFIED, not yet committed.** New T-204 (`test_production_GarminClient_
           retry_binds_the_spec_schedule`, `test_gc_rate.py` — pins the real wrapped
           `GarminClient` retry decorator's 250ms/2s/3-attempt schedule via closure-cell +
           behavioural halves), T-205 (`openWithTornTokenContentFailsWithRestoreLabel`,
           `testGarminConnectOpen.cpp` — torn on-disk token content reaches the restore seam
           verbatim and fails open() with a labelled sign-in-again error), T-206
           (`sigkillMidWriteLeavesDestinationIntact`, `testAtomicFile.cpp` — 12-cycle real
           fork/SIGKILL loop against `AtomicFile::writeOver`, byte-exact payload verification,
           timing-independent). RED verified for the right reason on all three (4 retry-schedule
           mutations, one non-atomic AtomicFile mutation, each caught then reverted byte-clean).
           **Ledger-hygiene defect found and fixed by the Inspector before documenting:** the
           builder self-assigned id `T-206` to BOTH the retry test and the SIGKILL test (an
           internal collision) instead of checking `WIKI.md`'s `next:garmin-T-204` REGISTRIES
           pointer — corrected by renumbering the retry test's in-code comment to `T-204` and
           adding `T-205` to the previously-uncited torn-read test; no test behavior changed.
           **Independently re-verified by the Inspector on rebuilt targets:** `ctest -R
           "testAtomicFile|testGarminConnectOpen"` 4/4 Passed, `sigkillMidWriteLeavesDestination
           Intact` re-run 3/3 clean, `pytest tests/test_gc_rate.py` 9/9, `ruff`/`ruff format
           --diff`/`mypy --strict` clean on the Python file, `clang-format --dry-run --Werror`
           clean on both `.cpp` files, full `ctest -L garmin-fast` re-confirmed 40/40 (319.73s) —
           no collateral. Footprint exactly 3 files, all test-only; every production file touched
           during RED (`gc_rate.py`, `AtomicFile.cpp`) reverted and `git status`-verified clean.
           Flagged: REQ-NF-Reliab-001's "UI inspection" clause (permanent failures surface with
           the Garmin error code) is separate surface, not addressed here. **Committed
           `51bc6b5a8` 2026-09-12** (6 files: STATE.md/WIKI.md/traceability.md + the three test
           files). Next Stage 8 item: the remaining NF bars (Perf/Obs/i18n — none started).

Detail lives in: traceability.md (per-id spine) · findings.md (finding disposition;
archive/findings-detail.md for any row whose cell was capped this pass) · decisions.md
(## Decision index, then entries) · validations/archive/ + cycles/archive/ (historical
execution evidence) · archive/state-history.md (§ 1-10 pre-2026-09-06 history, § 11 = the
complete pre-compaction STATE.md this draft replaces). workflow-aicoach/ is a retired ledger
(provenance only — see .claude/workflow-INDEX.md).
