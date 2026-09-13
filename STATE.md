# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-13 by Inspector (B-STAGE9-08 FIXED + COMMITTED `c1948b513` — Stage 9 diagnostic exceptionType logging (PyEmbeddedAdapter.cpp's pyExceptionTypeName()), hardened across 8 reviewer delta-check rounds against 8 distinct leak-bypass classes (non-str stringification, double module-prefix, non-identifier-shaped names, unallowlisted modules, empty-moduleName bypass, spoofed __module__/__qualname__ not backed by the real object, genuinely-real-but-unallowlisted tp_name module root); round 8 GREEN, `garmin_codex_reviewer` independent delta-check PASS. Inspector independently rebuilt (full `GoldenCheetah` app target + all unittests, exit 0) and re-ran the full suite twice (once pre-commit, once post clang-format reformat): `testGarminConnectPyAdapter` 49/49, `ctest -L garmin-fast` 40/40, `ctest -L garmin-py` 4/4, 0 failures both times. B-STAGE9-01..08 all now FIXED/COMMITTED. Stage 9's live-account acceptance criterion still not met. **Live re-test attempts #3 and #4 (2026-09-13) both failed with the generic `Unknown` fold — root-caused to B-STAGE9-09 (OPEN, blocking).** B-STAGE9-08's own new `exceptionType` diagnostic earned its keep on first live use: `garmin_auth_unknown exception_type=builtins.AttributeError` (goldencheetah.log lines 14-17) proved the login actually SUCCEEDS (strategy 3 `widget+cffi`, after the two mobile 429s that were a red herring) and that the failure is POST-login in our own adapter — `garmin_client.py:137`/`:242` read `self._garmin.full_name_id`, an attribute the real python-garminconnect 0.3.15 `Garmin` does not have, escaping raw (outside the classification try/except) as a foreign exception that correctly folds to `Unknown`. Fix dispatched to the builder; cascade owed by the Inspector on `design.md:221` prose, DEC-014 OQ1 closure, and a lessons entry on fakes pinning a non-existent library API. Next: builder GREEN → reviewer delta-check → qmake rebuild → live re-test attempt #5.)
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
LAST_CLV:  clv_findings.py 2026-09-13 (re-run post-B-STAGE9-08 closure — builder GREEN
           on round 8, reviewer delta-check PASS after 8 fix rounds). **PASS — 0
           OUTSTANDING / 407 OK over 407 rows.** `ledger_drift_lint.py` re-run clean
           (EXIT=0) against both edited files (STATE.md/findings.md).
           Prior:
           2026-09-13 (re-run post-B-STAGE9-06 closure — builder GREEN,
           reviewer delta-check PASS). **PASS — 0 OUTSTANDING / 406 OK over 406 rows.**
           `ledger_drift_lint.py` re-run clean (EXIT=0) against both edited files
           (STATE.md/findings.md).
           Prior:
           2026-09-13 (re-run post-B-STAGE9-06 findings-ledger addition — live
           Stage-9 re-test #2's new curl_cffi/garminconnect defect, disposition `open`,
           investigation dispatched to `pch_investigator`). **FAIL — 1 OUTSTANDING / 404 OK over
           405 rows** — EXPECTED, not a ledger defect: B-STAGE9-06 is a genuinely open blocking
           finding, correctly carrying a `BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-009,
           TASK:REQ-012, TASK:REQ-017}` effect set (MISSING-EFFECT=0). `ledger_drift_lint.py`
           re-run clean (EXIT=0) against both edited files (STATE.md/findings.md).
           Prior:
           2026-09-13 (re-run post-B-STAGE9-01..05 findings-ledger closure —
           all five findings flipped to a closing disposition, `fixed`/`superseded`, once the
           Inspector independently re-verified both build systems GREEN). **PASS — 0
           OUTSTANDING / 404 OK over 404 rows** (400 + the 4 new B-STAGE9-02..05 rows;
           B-STAGE9-01 itself was already counted). `ledger_drift_lint.py` re-run clean
           (EXIT=0) against both edited files (STATE.md/findings.md).
           Prior:
           2026-09-13 (post-B-STAGE9-01 findings-ledger addition). **FAIL — 1 OUTSTANDING / 399
           OK over 400 rows** — this was EXPECTED, not a ledger defect: B-STAGE9-01 was a
           genuinely open blocking finding (DEC-052 accepted, builder dispatched, not yet
           built), correctly carrying a `BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-009,
           TASK:REQ-012, TASK:REQ-017}` effect set (MISSING-EFFECT=0) — now closed, see above.
           Prior:
           2026-09-12 (re-run post-B-I18N001-01/02 findings-ledger addition,
           REQ-NF-i18n-001/T-208 reviewer-caught-and-fixed findings). **PASS — 0 OUTSTANDING /
           399 OK over 399 rows** (397 + the 2 new B-I18N001 rows). `ledger_drift_lint.py` also
           re-run clean (EXIT=0). Inspector ran both directly against the just-edited files.
           Prior:
           2026-09-12 (re-run post-ORCH-062 findings-ledger addition — anti-
           duplication-guard false-positive on shell-redirect/loop-literal misparse, hit
           repeatedly during T-208 builder supervision; LSN-036 miss:4 recorded alongside it).
           **PASS — 0 OUTSTANDING / 397 OK over 397 rows.** Only 1 new row (ORCH-062) was
           added this pass; the jump from 394→397 (+3) is NOT reconciled here — consistent with
           the carried-forward NOTE below about this file's row count not tracking cleanly
           against other counts, not independently re-derived this pass either. The live
           clv_findings.py count above remains authoritative regardless. `ledger_drift_lint.py`
           also re-run clean (EXIT=0). Inspector ran both directly against the just-edited files.
           Prior:
           2026-09-12 (post-REQ-NF-Reliab-001+002/T-204..206 ledger update) PASS — 0 OUTSTANDING
           / 394 OK over 394 rows (unchanged — no new findings opened; the T-206 id-collision was
           a ledger-hygiene fix, not a finding).
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
           **TEST VERIFIED + COMMITTED `51bc6b5a8`** (2026-09-12) — see COUNTS-ADDENDUM-6 below.
           Stage 8's fifth item, REQ-NF-Perf-002, was found **already MET** on inspection
           (pre-existing test coverage, ledger was stale) — see COUNTS-ADDENDUM-7 below.
           Stage 8's sixth item, REQ-NF-Obs-001, needed a technical decision before it was
           buildable — DES-008's "ErrorBus" sample turned out to name a mechanism DEC-022 already
           found doesn't exist in the tree; **DEC-051** (Inspector, Three-Options Doctrine)
           reconciles this: the user-facing-errors clause is MET via DEC-023's `readFailed` signal
           + the pre-existing `errors` out-param convention, and only structured `qDebug`
           developer-trace logging remains genuinely open — see COUNTS-ADDENDUM-8 below, now
           dispatched to the builder. Also fixed this pass: the DEC registry (WIKI.md said
           `next:garmin-044`, true max was DEC-050) and the decisions.md Active index (DEC-046..050
           existed as full entries but were never added to the index table — the exact "no gaps"
           failure the index's own header warns about) — both ledger-hygiene misses, no code
           changed. Next atomic unit after the builder returns: REQ-NF-i18n-001 (NOT STARTED;
           NF-Perf-001/003's end-to-end clauses remain flagged manual-only, not CI-automatable).
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

COUNTS-ADDENDUM-7 (2026-09-12, not yet folded into the block above): Stage 8's fifth item,
           REQ-NF-Perf-002 (rate-limit pacing + concurrent-sync rejection), found **already MET**
           on inspection — no new code or tests needed, the ledger cell was stale. Both of
           prd.md:102's verification clauses were already covered by pre-existing tests:
           `test_rate_limited_paces_consecutive_calls`/`test_rate_limited_no_wait_once_interval_
           elapsed` (`test_gc_rate.py`, from the original REQ-010/DES-005 build) for the
           unit-test-on-the-rate-limiter clause, and `concurrentReaddirWhileOneInProgressIsRejected`
           (`testGarminConnectSync.cpp`, REQ-008 Slice-C's T-047/T-048 group) for the
           integration-test-on-concurrent-invoke clause — the latter asserts rejection with an
           "in progress" message matching the real production string in `GarminConnect.cpp:650`,
           and that the guard releases after the outer sync completes. **Independently re-run by
           the Inspector in isolation:** `testGarminConnectSync concurrentReaddirWhileOneInProgressIsRejected`
           3/3, the two pytest pacing tests 2/2 — both GREEN. NF-Perf-001 (first-connect SSO
           ≤30s) and NF-Perf-003's end-to-end clause (0-3-activity sync ≤5s) remain genuinely
           open — both are prd.md-specified manual-stopwatch-on-reference-network criteria, not
           CI-automatable without a real Garmin account and a controlled 50/10 Mbit link; flagged
           as a manual Phase-close item, same disposition as the pre-existing A3-R007-02/CLV WARN.
           No commit — ledger-only correction. Next Stage 8 item: REQ-NF-Obs-001 (structured
           ErrorBus logging) or REQ-NF-i18n-001 (tr() coverage), both NOT STARTED.

COUNTS-ADDENDUM-8 (2026-09-12, not yet folded into the block above): Stage 8's sixth item,
           REQ-NF-Obs-001 (structured logs), required a technical decision before it was buildable
           as originally worded. DES-008's sample had every sync op emit through an `ErrorBus`
           channel that a grep of `src/` confirms was never implemented (only three comment hits
           in `IGarminPyAdapter.h`/`GarminWorker.h`/`GarminConnect.cpp`) — and DEC-022 (2026-08-04)
           had already found and rejected this exact gap while resolving B-R017-06 ("ErrorBus DOES
           NOT EXIST in the tree... would be a new subsystem"), choosing instead DEC-023's
           `readFailed` signal. REQ-NF-Obs-001 was never updated to reflect that. **DEC-051**
           (Inspector's own Three-Options Doctrine scoring — an ordinary architecture
           reconciliation, not a human-in-the-loop gate) resolves it: Option B (reuse `readFailed`
           + the pre-existing `errors` out-param convention for the user-facing half; build only
           the missing `qDebug` structured trace logging) scored 5/5/5/5 against reliability/
           scalability/maintainability/best-practices, clearly ahead of Option A (build the
           never-existing `ErrorBus` class DES-008 described — would create a second, overlapping
           error-reporting mechanism alongside DEC-023's chosen one) and Option C (defer the whole
           REQ, which would discard the already-met user-facing-error coverage along with the one
           genuinely missing piece). `traceability.md`'s REQ-NF-Obs-001 row and `design.md`'s
           DES-008 both updated (dated addendum, sample kept verbatim for historical record, not
           deleted). **Two further ledger-hygiene defects found and fixed while researching this:**
           the DEC registry (`WIKI.md` said `next:garmin-044`; true max via
           `grep -oE 'DEC-[0-9]+' decisions.md \| sort -u` was DEC-050) and the decisions.md Active
           index table, which stopped at DEC-045 — DEC-046 through DEC-050 existed as full entries
           in the file but were never added to the index list, the exact "no gaps" failure the
           index's own header explicitly warns against (citing the prior VAL-017 incident). Both
           corrected; no code changed. Builder now dispatched to build the structured `qDebug`
           trace logging at each sync op boundary (open/readdir/readFile/backfill) per DEC-051's
           Option B and a log-format-review test (prd.md:112's own verification method).

COUNTS-ADDENDUM-9 (2026-09-12, not yet folded into the block above): Stage 8's sixth item,
           REQ-NF-Obs-001 (structured logs), is now **TEST VERIFIED, not yet committed** —
           the structured `qDebug` trace-logging clause DEC-051 (COUNTS-ADDENDUM-8) dispatched
           to the builder is complete. New `gcObsTrace()` helper in `src/Cloud/GarminConnect.cpp`
           emits one fixed key=value line at every `open()` ("auth") and `readdir()`
           ("sync_incremental") exit; new T-207 (`openEmitsStructuredObsTraceOnSuccess`,
           `openEmitsStructuredObsTraceWithKindOnFailurePaths`, `readdirEmitsStructuredObsTrace
           OnSuccessFailureAndGuard`) pins prd.md:112's log-format-review verification method as
           an executable test via a new `ObsCapture` Qt-message-handler hook. Two prior mechanical
           fixes (blockingRestore out-param init ordering; enum-cast-before-switch UB in the two
           Kind-mapper helpers) were applied directly by the predecessor Inspector session and
           reviewer-confirmed GREEN. This session then dispatched the reviewer for a FULL delta-
           check on the complete diff (not just those two fixes) and it came back BLOCKED with two
           further real, reviewer-caught defects — both fixed directly by the Inspector, each
           re-confirmed GREEN by a follow-up reviewer pass: **B-OBS001-01** (five of `readdir()`'s
           guard-rejection trace calls omitted `activity_count=0`, contradicting the method's own
           documented field set — fixed, regression assertion added) and **B-OBS001-02**
           (`ObsCapture`'s test-only Qt message handler was not reentrant/thread-safe per Qt's own
           logging docs — fixed in two rounds: a `QMutex` + `snapshot()` accessor, then a follow-up
           fix locking the constructor's handler-install + `current` assignment together, after the
           reviewer caught that first round left that assignment outside the lock). **Independently
           re-verified by the Inspector at each step**: `ctest -R "testGarminConnectOpen|
           testGarminConnectSync"` GREEN after every fix round, full `ctest -L garmin-fast` 40/40
           re-run twice (once after the two guard/mutex fixes, once again after the constructor-race
           fix) with 0 failures each time, `clang-format --dry-run --Werror` clean on all four
           touched files (`src/Cloud/GarminConnect.cpp`, `GarminConnect.h`,
           `testGarminConnectOpen.cpp`, `testGarminConnectSync.cpp`). `clv_findings.py` re-run PASS
           (0 OUTSTANDING / 396 OK — 394 + the two new B-OBS001 rows), `ledger_drift_lint.py`
           re-run clean (EXIT=0). WIKI.md's TEST registry corrected `next:garmin-T-207` →
           `next:garmin-T-208` (T-207 was already consumed by this REQ; the pointer had gone
           stale, same class of miss as the DEC-registry gap COUNTS-ADDENDUM-8 already fixed once
           this session). **Committed `e69dfa027` 2026-09-12** (8 files: STATE.md/WIKI.md/
           traceability.md/findings.md + GarminConnect.cpp/.h + the two test files). **All Stage 8
           REQ-NF items except REQ-NF-i18n-001 are now MET.** REQ-NF-i18n-001 (T-208, tr()
           coverage) is now also TEST VERIFIED + COMMITTED `da9ef33fa` — see
           COUNTS-ADDENDUM-10 below. **All Stage 8 REQ-NF items now MET and COMMITTED**
           (Build-001 `f118691ea`, Sec-001+003 `d271abae1`, Sec-004 `07501c477`,
           Reliab-001+002 `51bc6b5a8`, Obs-001 `e69dfa027`, i18n-001 `da9ef33fa`).
           **Stage 8 is CLOSED.** Next: Stage 9 (a real Garmin account exercising
           connect/MFA/sync/disconnect + Win/macOS/Linux installed-package smoke checklist)
           — a human-in-the-loop gate (real user credentials), not something the Inspector
           can build unattended.

COUNTS-ADDENDUM-10 (2026-09-12, not yet folded into the block above): Stage 8's last open item,
           REQ-NF-i18n-001 (T-208, tr() coverage for the Garmin-scope UI surface), is now
           **TEST VERIFIED + COMMITTED `da9ef33fa` 2026-09-12.** Builder converted 11 `QStringLiteral` prose
           sites in `GarminBackfillController.h`/`.cpp` to `tr()` (via
           `Q_DECLARE_TR_FUNCTIONS(GarminBackfillController)`, public: reopened per this
           repo's established pattern) and added new `unittests/buildguard/
           garmin_i18n_source_guard.py` (T-208, `testGarminI18nSourceGuard` — static: prose
           literals sit inside `tr()`, every extracted literal has a matching `<source>` in
           all 13 `gc_*.ts` files). Reviewer full delta-check found two real defects before
           commit — see B-I18N001-01/02 in findings.md: (1) blocking — `GarminConnect`
           (DES-014, deliberately no `Q_OBJECT`) inherits `CloudService::tr()` at runtime, so
           all 16 `GarminConnect`-context `.ts` entries `lupdate` extracted were
           translation-dead; fixed via `Q_DECLARE_TR_FUNCTIONS(GarminConnect)` +
           `#include <QCoreApplication>`, proven at runtime with a hand-written `.ts` +
           `lrelease` + a standalone `tr()`-calling program, not just via `lupdate`. (2)
           non-blocking — all 13 `.ts` diffs carried this dev machine's absolute checkout
           path in 51 newly-added `<location>` lines per file (a merge-script gap on 5
           whole-context block inserts); fixed, 0 residual absolute-path fragments
           confirmed by the reviewer's own re-grep. **Independently re-verified by the
           Inspector**: `clang-format --dry-run --Werror` clean on `GarminConnect.h` (run
           directly — the reviewer's own sandbox lacked the cached executable, an
           environment gap, not a code finding), full `ctest -L garmin-fast` re-confirmed
           40/40 (306.47s), `garmin_i18n_source_guard.py` PASS (33 files, 68 literals, 13
           .ts, 0 findings), `garmin_sec_source_guard.py` PASS (47 files, 0 findings) — no
           collateral. Dispatch mechanics note: the reviewer briefing's first `herdr agent
           send-keys ... enter` silently failed to submit (text sat unsent in the input box)
           because the pane was not focused; `herdr agent focus` before the keypress fixed
           it (Inspector-tooling note, not a project lesson — not added to lessons.md).
           **All Stage 8 REQ-NF items now MET and COMMITTED. Stage 8 is CLOSED** (22 files,
           `da9ef33fa`, includes STATE.md/WIKI.md/traceability.md/findings.md). Next:
           Stage 9 (human-in-the-loop gate, see above).

STAGE-9-KICKOFF (2026-09-12, user decision recorded live in conversation, not yet acted on):
           user chose to proceed with Stage 9 now rather than pause or hand it off entirely.
           Credential-flow agreement (both AskUserQuestion rounds, this session): the
           Inspector builds and launches the real app itself (prefer the qmake build per
           `garmin-build-system-duality` — qmake is the real CI/release path,
           `GC_WANT_GARMINCONNECT` is currently commented out in `src/gcconfig.pri.in` and
           needs enabling), then gives the user step-by-step instructions to follow inside
           the running app's own Garmin-connect wizard. **The user drives all credential
           entry themselves, directly into the app's dialog — never into chat, never typed
           by the Inspector.** The Inspector's job is to launch the app, narrate what to
           click/enter at each step (per prd.md US-1/US-3/US-4 and REQ-003/009/012/017's
           acceptance criteria), and record what the user reports back (connect success
           within ~30s incl. MFA round-trip, MFA 6-digit dialog behavior, sync pulling real
           activities, Disconnect deleting tokens immediately) as the Stage 9 evidence in
           traceability.md — this is the FIRST time this stage's acceptance criteria can be
           checked on a real account; nothing here may be inferred from the existing seam
           tests. **Known gap, flag to the user early:** REQ-NF-Pkg-001's own text calls for
           a "manual smoke checklist" for Win/macOS/Linux installed packages "documented in
           CONTRIBUTING" — grepped 2026-09-12, no Garmin content exists in `CONTRIBUTING.md`
           yet (this checklist was never actually written). This dev machine is Linux-only —
           the Win/macOS legs of that checklist cannot be executed here regardless; only the
           live-account connect/MFA/sync/disconnect flow (the other half of Stage 9) and a
           Linux installed-package smoke check are reachable from this session. Surface both
           gaps to the user rather than silently narrowing scope.

STAGE-9-BLOCKER (2026-09-13, discovered on the FIRST real live-account connect attempt,
           **code defect now FIXED, live-account re-test still pending**):
           qmake build (`GC_WANT_GARMINCONNECT=ON`, `GC_WANT_PYTHON` OFF per the kickoff plan
           above) + real launch + real credential entry all completed as planned, but every
           connect attempt failed with the generic "Connection to Garmin Connect failed
           (code: unknown)". Root-caused (not a credentials/network/package issue — a missing
           `garminconnect`/`curl_cffi` pip-install was found and fixed first, necessary but NOT
           sufficient): `Py_Initialize()` is never called anywhere reachable in this
           configuration — the only interpreter-init call in the whole tree
           (`src/Python/PythonEmbed.cpp:248`) is gated behind the separate `GC_WANT_PYTHON`
           flag, contradicting `src.pro`/`gcconfig.pri.in`'s own "either feature independent of
           the other" design claim. Invisible to all 40 prior `garmin-fast` GREEN runs because
           `testGarminConnectPyAdapter.cpp`'s own `initTestCase()` calls `Py_Initialize()`
           itself — REQ-002's "TEST VERIFIED (seam)" qualifier in traceability.md was exactly
           this gap, now proven real by a live run. Logged as **B-STAGE9-01** (findings.md),
           root cause independently confirmed by the reviewer (source + wiring + built binary's
           own symbol table). **DEC-052 accepted** (shared process-level CPython bootstrap,
           main-thread, feature-agnostic — Three-Options-scored against the reviewer's two
           researched alternatives). New `src/Python/PyProcessBootstrap.{h,cpp}` + wiring in
           `main.cpp`/`PythonEmbed.cpp`/`src.pro`. Two reviewer delta-check rounds on the diff
           found and closed four further defects before this was GREEN: **B-STAGE9-02**
           (qmake PCH-bypass missing for the new TU — `NO_PCH_SOURCES` routing added, mirroring
           `PyEmbeddedAdapter.cpp`'s existing LSN-007 pattern), **B-STAGE9-03** (inittab hook
           could lose the CPython-init race on an internal scripting-toggle restart — hook now
           passed whenever `GC_WANT_PYTHON` is compiled in, regardless of runtime toggle),
           **B-STAGE9-04** (documented GIL-release contract false on the externally-initialized
           path — superseded by B-STAGE9-05, below), **B-STAGE9-05** (B-STAGE9-04's own fix
           attempt introduced a real GIL-ownership bug — `PyGILState_Check()` alone can't tell
           "GIL held incidentally" from "GIL held deliberately by an unrelated caller scope";
           a regression test proved a genuine fatal Python abort against the buggy code before
           the fix removed the unsafe release entirely and narrowed the documented contract).
           All four findings closed; full detail in findings.md. **Independently re-verified by
           the Inspector on both build systems after the final fix:** CMake `ctest -L
           "garmin-fast|garmin-py"` 42/42 (0 failures, `testPyProcessBootstrap` 6/6), qmake
           `make -j8` exit 0 with `PyProcessBootstrap.o` compiled via its own non-PCH rule and
           the `GoldenCheetah` binary relinked. **COMMITTED `173135907`.**

STAGE-9-BLOCKER-2 (2026-09-13, live-account re-test #2, post-DEC-052 fix — B-STAGE9-06,
           still OPEN): Inspector launched the freshly-built qmake binary (confirmed containing
           `PyProcessBootstrap` symbols, confirmed `GC_WANT_GARMINCONNECT=ON`/`GC_WANT_PYTHON`
           off in the active `gcconfig.pri`); user re-entered real credentials into the app's own
           dialog. B-STAGE9-01's fix genuinely works — Python now bootstraps for real (proof: the
           UI showed the classified `GarminErrors::translate(Network)` message, not the old
           generic `Unknown` fold) — but the `garminconnect` 0.3.15 auth-strategy chain itself now
           fails end-to-end: two `ImpersonateError: Impersonating chrome120/chrome150 is not
           supported`, a real `429` IP rate-limit, a real `403` Cloudflare bot challenge, and a
           genuine `AttributeError: 'RequestsCookieJar' object has no attribute 'jar'` library
           bug (see B-STAGE9-06, findings.md, for full log detail and what's already been ruled
           out — single curl_cffi install confirmed, no dynamic libcurl symbol collision
           confirmed, identical calls confirmed working from a plain `python3` CLI in the same
           env). Root cause not yet isolated — appears specific to curl_cffi's impersonation
           running INSIDE GoldenCheetah's embedded-CPython process. **Dispatched to
           `pch_investigator`** (isolated scratch-dir repro: bare embedded-CPython harness
           mirroring `PyProcessBootstrap.cpp`'s init flags, testing both a Qt-free embed and a
           non-main-OS-thread call, to isolate the variable).

           **Root cause CONFIRMED (2026-09-13)** via the investigator's controlled A/B repro:
           ELF symbol interposition — GoldenCheetah's own linked `libcurl-gnutls.so.4` (system,
           GnuTLS) is already loaded process-globally by the time Python dlopens `curl_cffi`'s
           wrapper, which statically bundles its own patched libcurl-impersonate (BoringSSL)
           but exports its symbols with no `-Bsymbolic` protection — so `curl_easy_init`/
           `curl_version` silently bind to the wrong (system) libcurl while `curl_easy_impersonate`
           binds to curl_cffi's own `.so`, an ABI/internal-state mismatch that makes every Chrome
           impersonation target report "not supported." Proven NOT to be CPython embedding, the
           GIL, or `GarminAuthChain`'s QThread (a bare Qt-free embed on both the init thread and
           an explicit pthread worker does NOT reproduce it; globally preloading
           `libcurl-gnutls.so.4` DOES reproduce it, in the same bare harness). Fix: import
           `curl_cffi`'s wrapper with `os.RTLD_DEEPBIND` (glibc/Linux-only, guarded by
           `hasattr`) once, before `garminconnect` is imported, in `src/Python/garminconnect/
           garmin_client.py` — the single documented choke point for that import. **Dispatched
           to the builder**, TDD (RED via a test that recreates the interposition precondition,
           then GREEN on the fix), not yet complete.

           Also surfaced, non-causal: **B-STAGE9-07**, a pre-existing dangling-pointer bug in
           `PythonEmbed.cpp`'s `Py_SetProgramName()` call (traces back through several
           historical commits, not from this session's diff). Per this project's own
           scope-boundary policy for pre-existing/foundational code, surfaced to the user via
           `AskUserQuestion` — **user chose fix-now**, queued as its own atomic unit (separate
           commit) immediately after B-STAGE9-06 reaches GREEN.

           **Next: builder GREEN on B-STAGE9-06 → reviewer delta-check → Inspector independent
           rebuild/re-verify on both build systems → commit → same cycle for B-STAGE9-07 → THEN
           re-attempt the live-account test with the user a third time.** This does NOT reopen
           the credential-entry human-in-the-loop gate itself — the user already entered real
           credentials twice; only a fix-then-retry loop remains, not fresh credential entry
           each time unless the UI state requires it.

Detail lives in: traceability.md (per-id spine) · findings.md (finding disposition;
archive/findings-detail.md for any row whose cell was capped this pass) · decisions.md
(## Decision index, then entries) · validations/archive/ + cycles/archive/ (historical
execution evidence) · archive/state-history.md (§ 1-10 pre-2026-09-06 history, § 11 = the
complete pre-compaction STATE.md this draft replaces). workflow-aicoach/ is a retired ledger
(provenance only — see .claude/workflow-INDEX.md).
