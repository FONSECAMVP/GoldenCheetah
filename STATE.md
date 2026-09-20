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
LAST_CLV:  clv_findings.py 2026-09-19 (post-B-STAGE9-12/-27 disposition, `garmin_inspector_v1_32`).
           **PASS — 0 OUTSTANDING / 432 OK over 432 rows**, 0 MALFORMED, 0 MISSING-EFFECT.
           `ledger_drift_lint.py` re-run clean (EXIT=0) against the edited files. No blocking
           finding remains open on Stage 9's code; the two open rows are both non-blocking.
           Prior:
           clv_findings.py 2026-09-13 (re-run post-B-STAGE9-08 closure — builder GREEN
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

STAGE-9-CURSOR (2026-09-15, written by `garmin_inspector_v1_21` — this supersedes the
           line-1 header narrative above, which stopped being accurate after B-STAGE9-09 and
           should be read as history, not as the cursor. Per-id status lives only in
           findings.md; this section carries sequencing and the live gate only):
           B-STAGE9-01..11 and -13 are all committed — most recently B-STAGE9-13 (empty token
           blob gets its own `LoadStatus::Empty` and `gc_obs error_code=empty`) committed
           `1ae49a196`, 18 files, after reviewer round 3 came back clean and the Inspector
           independently re-ran the build, the 40-test `garmin-fast` label, both per-binary
           targets and the `empty_corrupt` mutation. Two findings remain open, both blocking:
           **B-STAGE9-12** (stderr buffering defeats REQ-NF-Obs-001's `gc_obs` traces in the
           real qmake binary) — RED delivered and reviewer-reviewed, repair queued, FROZEN
           under the hard hold below. **B-STAGE9-14** (prefix-matching assertions in the Garmin
           observation tests) — committed `625c1c337` 2026-09-15, reviewer round 1 clean, zero
           production changes; escalated from non-blocking on the reviewer's round-3 verdict,
           widened from the 2 assertions originally flagged to 11 by the Inspector's own grep
           and to 12 by the builder, which found a selector the grep missed.
           **B-STAGE9-15** — the guard-correctness half committed `d9ba4faad` 2026-09-15 by
           `garmin_inspector_v1_22`: `is_technical()` now recognises the developer-trace SHAPE
           as a category per DEC-053, so the three `qDebug` triage literals are exempt without
           touching the call sites. Sequencing worth keeping: two reviewer rounds, the first a
           real defect (the event-name rule matched any lower-case word, so `warning disk=full`
           was exempt), the second filed blocking and OVERRULED after `s915_i18n_second_opinion`
           independently sided with the Inspector — an all-`key=value` literal staying exempt is
           the irreducible consequence of a shape-based exemption, not a bug, and is now pinned
           as an accepted false negative so the boundary cannot move silently. The Inspector also
           closed a Unicode normalisation defect it found itself, and re-ran both mutations rather
           than relaying them. **B-STAGE9-16** — the gate-coverage half, split out of -15 rather
           than folded into its repair, and the reason a RED test survived two days: the
           verification ritual is opt-in. `garmin-i18n-guard` is outside `garmin-fast`, and the
           ruff/mypy hooks are scoped to `src/Python/garminconnect/` only, so nothing linted this
           unit's own Python — confirmed empirically when every hook skipped `d9ba4faad`. Not
           dispatched; it owes a scored decision (change the label, or change the ritual) and a
           DEC id, not a patch. Every `garmin-fast 40/40` claim in this ledger, the Inspectors'
           own re-runs included, is narrower than it reads. Two findings remain open, both
           blocking: B-STAGE9-12 and B-STAGE9-16.
           **THE HARD HOLD ON `src/Core/main.cpp` REMAINS IN FORCE**, and with it all of
           B-STAGE9-12's production fix: no edit to that file, no qmake, no `make` against
           `src.pro`. **THE ONE REAL HUMAN-IN-THE-LOOP GATE IS UNCHANGED AND STILL OPEN:** the
           user's live Garmin sync re-test of `e5936375c` has not run. Re-verified 2026-09-15 —
           GoldenCheetah not running, `~/.goldencheetah/Andy/activities/` still 1145 files,
           `tokens.json` still the 0-byte file from 2026-09-13 20:53. No qmake rebuild is needed
           for it (`src.pro` points `GARMIN_PY_MODULE_DIR` at the source tree, so the committed
           Python fix is live on next launch). Success criterion is a NEW file under that
           activities directory, NOT the UI; on failure the user must quit via File > Quit so
           the buffered `gc_obs` lines flush — which is B-STAGE9-12 itself. Release the hold
           once that test concludes.

STAGE-9-CURSOR (2026-09-16, written by `garmin_inspector_v1_25` — this supersedes the
           2026-09-15 block above, which should now be read as history. Per-id status lives
           only in findings.md; this section carries sequencing and the live gate only):
           **B-STAGE9-16 Part A landed `8611fb2d0`** 2026-09-16 (4 files, +2111/-7): the
           routine ctest gate is now `ctest -LE gate-exclude`, default-include, so enrolment
           is no longer the verification step and excusing a test requires a recorded reason
           plus an alternate tier that really selects it — both machine-checked by the new
           `testGarminGateCoverageGuard` / `testGarminGateCoverageGuardUnits`. `dod.md`'s
           "registered under the `garmin-fast` label" item, which was the defect written down
           as a rule, is replaced with the superseded text kept visible. Sequencing worth
           keeping: FOUR independent reviewer rounds, and every one of the first three found a
           real blocking defect — a shlex-executed CMake property (the Inspector proved
           `ctest -S evil.cmake -N` EXECUTES the dashboard script, so `-N` is not a capability
           boundary), the two skip properties that make ctest count a failing test as PASSED,
           and a basename match that certified a non-existent ctest binary as reachable. Round
           4 was a scoped sweep for other assertions shaped around the code's own blind spot
           and came back clean. This is the project's strongest evidence yet for
           re-reviewing until a round is genuinely clean rather than stopping at the first
           plausible pass.
           **B-STAGE9-16 Part B is the remaining work and is unassigned:** widen the
           `.pre-commit-config.yaml` `files:` regexes to every Garmin-owned path plus a
           lint-ownership guard. NOT a whole-tree widening — DEC-010's scoping principle
           stands, only its enumeration is stale. It must first resolve **B-STAGE9-18** (a
           committed, pre-existing ruff F841 at
           `unittests/buildguard/garmin_sec_source_guard.py:271`) or the widened gate is red
           on arrival, which is exactly the shape DEC-054 refuses. Part A's own commit
           demonstrated the Part B gap once more: all five pre-commit hooks reported no files
           to check and skipped, on a commit staging two brand-new Python files.
           Two findings remain outstanding, both blocking: B-STAGE9-12 and B-STAGE9-16.

STAGE-9-LIVE-TEST (2026-09-16 05:20-05:33, user-attended, supervised by
           `garmin_inspector_v1_25`): **THE LIVE SYNC RE-TEST HAS NOW RUN — attempt #5. It is
           NO LONGER THE PENDING GATE.** Run against `build/src/GoldenCheetah` (CMake,
           2026-09-15 05:42) on the user's own decision, after the Inspector established that
           the qmake binary predates `1ae49a196` and so lacks B-STAGE9-13's empty-token fix.
           No rebuild was performed; the `src/Core/main.cpp` hard hold was verified INTACT
           throughout and is NOT released by this test concluding.
           **AUTHENTICATION SUCCEEDED — the first time in this project.**
           `gc_obs op=auth outcome=ok duration_ms=35`, and `tokens.json` went from the 0-byte
           file it had been since 2026-09-13 to a real 2189-byte session. Two fixes are thereby
           CONFIRMED LIVE for the first time: B-STAGE9-13 (the 0-byte restore emitted the
           distinct `error_code=empty`, not the old `unknown` fold) and B-STAGE9-09's root-cause
           reading (strategies 1-2 still 429, strategy 3 succeeds and returns early).
           **THE FEATURE STILL FAILED, and the defect has moved downstream:** fifteen
           `gc_obs op=sync_incremental outcome=fail error_code=unknown activity_count=0` lines,
           zero files downloaded (activities 1145 -> 1145, diffed against a pre-launch
           snapshot). Filed as **B-STAGE9-19**, blocking. Localised to
           `GarminConnect.cpp:825` — all five guarded early-returns emit distinct codes and none
           fired, so the list call itself ran and failed, and `garminListKindCode` (`:100-111`)
           proves it was NEITHER network NOR rate-limit. Same undiagnosable-Unknown-fold class
           as B-STAGE9-09, one layer down. Recommendation recorded: do NOT schedule a sixth
           attended run until `GarminListFailure` gains a distinct kind and code, or it will
           produce the same uninterpretable line.
           Two process facts worth keeping: the `gc_obs` evidence exists ONLY because the user
           exited via File > Quit (B-STAGE9-12 reproducing itself live, second independent
           confirmation), and GoldenCheetah TRUNCATES `goldencheetah.log` on launch, so
           attempt #4's evidence survived only because it had been snapshotted first.
           Three findings now outstanding, all blocking: B-STAGE9-12, B-STAGE9-16, B-STAGE9-19.
           **THE HARD HOLD ON `src/Core/main.cpp` REMAINS IN FORCE.**
           CORRECTION by `garmin_inspector_v1_26` 2026-09-16: this block previously closed by
           re-verifying the gate markers as "`tokens.json` still the 0-byte file from
           2026-09-13 20:53", which CONTRADICTED its own paragraph above and was left over
           from a pre-test reading. Re-measured now: GoldenCheetah not running, activities
           still 1145 files, and `tokens.json` is the **2189-byte session blob written
           2026-09-16 05:21:24** — i.e. the auth artefact of attempt #5, exactly as the
           paragraph above reports. The marker set no longer describes a PENDING gate; what
           remains open is the downstream sync defect, not the connect step.

STAGE-9-CURSOR (2026-09-16, written by `garmin_inspector_v1_26` — supersedes the
           `garmin_inspector_v1_25` block above for SEQUENCING; that block's live-test record
           stands. Per-id status lives only in findings.md):
           **B-STAGE9-16 Part B is built and in review, not landed.** The builder delivered
           the lint-ownership guard (`unittests/buildguard/garmin_lint_ownership_guard.py`,
           +895, and its 683-line test file, both now staged) plus the widened
           `.pre-commit-config.yaml` regexes and the `garmin_sec_source_guard.py` cleanup that
           B-STAGE9-18 required so the widening is not red on arrival. Reviewer round 1 filed
           four findings; the builder's repair round closed all four and the Inspector
           re-verified three of them with its OWN probes rather than by relaying the report:
           `check_not_vacuous` is reached from `main()` (`:854`), a scratch repo with zero
           managed files now exits 1 with `LINT-VACUOUS` where it previously exited 0, and
           `check_model_is_faithful` fails closed on both a known narrowing key and an
           invented one. `ctest -R testGarminLintOwnership` is 2/2 green and the registered
           command really does pass `--build-dir`, so LINT-SELF's registry half is live rather
           than silently dead — checked specifically because that would have been round 1's
           vacuity defect a third time.
           UPDATE by `garmin_inspector_v1_27`: review rounds 2 and 3 have since completed.
           Round 2 filed ONE blocking finding — the guard modelled pre-commit's top-level
           `files:`/`exclude:` as a DEFAULT when pre-commit INTERSECTS three layers — which
           `garmin_inspector_v1_26` confirmed against the installed `pre_commit` 4.2.0 source
           before ordering the repair, then extended itself to `default_stages` and the
           nine-key top-level schema. Round 3 came back **CLEAN: no new blocking or
           non-blocking defect**, with a full selection-layer audit table
           (`/tmp/reviewer_s916_partB_round3_findings.md`). `garmin_inspector_v1_27`
           re-verified the deliverable independently before landing rather than relaying the
           reports: the guard is `PASS — 101/111` with all ten declared gaps intact, its unit
           suite is 86/86, and the `garmin_sec_source_guard.py` delta is ruff reflow plus one
           dead-variable removal (`raw_lines`, confirmed unreferenced) and nothing else.
           The ONE remaining unmodelled selection layer is the upstream hook manifest, which
           is the accepted follow-up held out in the brief's §8, not a defect: all 102 owned
           candidates are type-compatible with the pinned manifests today, and a hook revision
           bump must re-check it.
           **Sequencing that the next Inspector must not lose:**
           (1) Every commit in this tree is blocked until Part B lands, because pre-commit
           refuses to run while `.pre-commit-config.yaml` is itself unstaged and the builder
           owns that file. The pending ledger edits therefore ride WITH Part B's commit. Do
           not reach for `--no-verify`; the manual lint runs are green, which makes the bypass
           tempting and is exactly why it is refused.
           (2) Part B's commit must EXCLUDE `unittests/CMakeLists.txt` and the three untracked
           `unittests/Core/stderrbuf/*` files. Verified 2026-09-16: that hunk registers only
           `add_subdirectory(Core/stderrbuf)` and belongs to B-STAGE9-12, a different unit.
           (3) After Part B, the next atomic unit is B-STAGE9-19 — give `GarminListFailure` a
           distinct kind and code. That is the blocker on the whole feature now. It touches
           `src/`, so the `src/Core/main.cpp` hold and its scope need a decision from the user
           BEFORE that unit is dispatched; the Inspector does not release the hold itself.
           **Roster note:** `garmin_builder_stage9_v9` was soft-landed at 224k/250k while idle
           and clean, and replaced by `garmin_builder_stage9_v10` (`w1:pM`, auto mode verified
           from the status line). The old session is resumable as
           `claude --resume d16853cf-7b13-4544-9391-4449fc36b03d` if its reasoning is ever
           needed; its full report is `/tmp/builder_v9_report_s916_partB_repair1.md`. The
           reviewer pane `w1:pD` was anonymous and is now named `garmin_codex_reviewer`.
           Six findings now carry open work; three of them are the blocking set
           (B-STAGE9-12, B-STAGE9-16, B-STAGE9-19) and three are the non-blocking lint-gap
           follow-ups filed this session out of Part B's declared gaps (B-STAGE9-20,
           B-STAGE9-21, B-STAGE9-22), each re-measured with the PINNED tools before filing.
           ORCH-063 was also filed against the `anti_duplication_guard.py` hook — the fifth
           distinct false-positive root cause under LSN-036, whose counters are now miss:5
           and whose index/cold drift was repaired in the same edit.

STAGE-9-CURSOR (2026-09-16, written by `garmin_inspector_v1_27` — supersedes the
           `garmin_inspector_v1_26` block above for SEQUENCING; that block's record stands.
           Per-id status lives only in findings.md):
           **B-STAGE9-16 IS CLOSED — BOTH HALVES.** Part B landed at `8ea19e8e5` (10 files,
           +2665/-24) through the full pre-commit gate with NO `--no-verify`, carrying the
           accumulated ledger work of three Inspector sessions with it. The gate's own output
           on that commit is the finding's proof: `ruff-check` and `ruff-format` printed
           **Passed** on buildguard Python, where every prior commit printed
           "(no files to check)Skipped". The defect proved its own fix in the commit that
           closed it. The commit-blocking condition described in the block above is therefore
           LIFTED: `.pre-commit-config.yaml` is committed and ordinary commits work again.
           Verified after landing: findings register 424 rows, 0 malformed,
           **OUTSTANDING 3 -> 2**; `ledger_drift_lint.py` rc=0.
           **Sequencing that the next Inspector must not lose:**
           (1) **THE HARD HOLD ON `src/Core/main.cpp` IS RELEASED, BY THE USER, 2026-09-16.**
           `garmin_inspector_v1_27` put the question to the user rather than deciding it,
           because the record CONTRADICTED ITSELF: the hold as written by `_v1_25` says in its
           own text "Release the hold once that test concludes" (the 2026-09-15 block above),
           and the test concluded as attempt #5 on 2026-09-16 — yet `_v1_26` recorded that
           "the test concluding did NOT release it" and carried it forward as in force. The
           Inspector also established that only the qmake/make half of the hold constrained
           the next unit at all, since B-STAGE9-19 edits `src/Cloud/GarminConnect.cpp` and not
           `src/Core/main.cpp`. **The user chose release.** So `src/` is open and qmake against
           `src.pro` is permitted again, which also unblocks B-STAGE9-12's production fix and
           allows a rebuilt binary to carry new error codes into a future live run.
           The hold had been verified INTACT through Part B's landing; nothing in `8ea19e8e5`
           touches `src/`.
           (1b) The next atomic unit is **B-STAGE9-19** — give the list path distinct kinds and
           codes. It is now the sole blocker on the whole feature apart from B-STAGE9-12, and
           it is **DISPATCHED** to `garmin_builder_stage9_v11` (brief:
           `/tmp/builder_v11_brief_s919.md`). The Inspector EXTENDED the finding when briefing
           it, and the extension should be reviewed on its merits rather than inherited: the
           row says "give `GarminListFailure` a distinct kind plus code", but reading
           `blockingList()` line by line shows **THREE structurally different failure exits all
           reporting `Unknown`**, and one of them — `if (!client) return res;` at
           `GarminConnect.cpp:413-414` — **never constructs a `GarminListFailure` at all**, so
           no addition to that enum can reach it. The other two are the 60s timeout lambda
           (`:438-443`, which inherits the `:411` initialiser) and a genuine `listFailed`
           carrying `Unknown` (`:430-436`). `kListTimeoutMs` is 60000 (`:45`), so **none of the
           fifteen live failures was a timeout** — 643ms is two orders of magnitude short —
           which leaves the twelve 0-3ms failures matching the `!client` early return and the
           three sub-second ones matching a real adapter failure. That is a HYPOTHESIS the
           builder was explicitly asked to try to disprove, not a conclusion. Second defect
           found in the same reading: `GarminListFailure::rawMessage` is **discarded** at
           `:435` — `blockingList` copies only `failure.kind` — so the most diagnostic value
           in the path never reaches anyone. Ruling recorded: extend the enum and synthesize
           the kind at the local exits, REJECTING a parallel `failureCode` channel because two
           lists that must be kept in step is the exact defect class B-STAGE9-16 just spent
           three reviewer rounds abolishing one layer up.
           (2) Recommendation on the record, unchanged: **no sixth attended live run** until
           B-STAGE9-19 lands, or it will produce the same uninterpretable
           `error_code=unknown activity_count=0` line fifteen more times.
           (3) B-STAGE9-23's retrofit now explicitly OWES two files that Part B shipped at
           high comment density before the briefing-template fix propagated:
           `.pre-commit-config.yaml` (83 comment lines to 35 config) and
           `unittests/buildguard/CMakeLists.txt` (~46 to ~20). They were landed rather than
           re-opened because round 3 had certified them clean and a hand-edit by the Inspector
           would have invalidated that certification for a style defect. **The retrofit must
           NOT strip the `Gap(reason=...)` or `MODELLED_TOP_LEVEL_KEYS` strings — those are
           runtime DATA, printed and length-checked, not commentary.**
           **Roster note (2026-09-16): the builder runtime changed TWICE, and the dead end in
           the middle is the part worth keeping.** `garmin_builder_stage9_v10` (Claude,
           209k/250k and due a soft-landing) was exited cleanly; it remains resumable as
           `claude --resume 1cf9a230-ca31-43b0-9cb1-f8c7c733221d`.
           **Attempt 1 — a CODEX builder, on the user's instruction, FAILED AND SHOULD NOT BE
           RETRIED WITHOUT SOLVING THIS FIRST.** Codex came up in `w1:pM`, read the brief, and
           correctly REFUSED to do any work: `.codex/WORKFLOW.md` in this repo declares
           `LDW_CODEX_WORKTREE_REQUIRED` and mandates that a Codex agent create a dedicated
           linked git worktree plus a `codex/<task>` branch before any implementation, stating
           explicitly "do not implement in the checkout as a workaround". The builder was RIGHT
           to stop and its refusal was a correct policy read, not a malfunction. The policy is
           Codex-specific, which is exactly why four Claude builders never hit it. Honouring it
           is expensive here: `./build` is a **2.9 GB** CMake tree bound to the integration
           checkout's path, so a worktree needs a full from-scratch Qt/CMake configure+build
           before it can run one test. Two lesser Codex traps were also measured and are worth
           recording: plain `codex` defaults to a **read-only sandbox** (it needs
           `-s workspace-write` or it cannot write at all, and the resulting error looks like a
           read-only MOUNT — `findmnt` proved the mount is `rw`), and Codex 0.154.0 accepts
           only `on-request` or `never` for `--ask-for-approval`, with the Claude Code auto-mode
           classifier BLOCKING any launch that passes `-a never`.
           **Attempt 2 — ADOPTED, and this is the live roster.** On the user's revised
           instruction the pane was relaunched with **`claude-sr --permission-mode auto`**, a
           local wrapper that runs Claude Code against **glm-5.3** (z.ai Anthropic-compatible
           endpoint). This resolves the impasse structurally: it is Claude Code, so the `.codex`
           worktree policy does not apply, `./build` is reused, and real auto mode means no
           approval dialogs to babysit. Verified from the pane's own status line before
           dispatch: `glm-5.3[1m]`, effort high, `auto mode on`, `tok 0k/0k`. **NOTE it came up
           on the `glm-5.3-flash` slot and had to be raised with `/model opus`** — the wrapper
           maps the haiku slot to flash, so a fresh pane is on the WEAKEST model until moved.
           Check the status line; do not assume.
           **Supervision consequence:** this is a different model family from every previous
           builder on this stage. Verify its evidence rather than relaying it, which is the
           standing rule anyway but matters more here.
           The reviewer stays `garmin_codex_reviewer` (`w1:pD`, Codex, 213k/250k — **due a
           `/new` before its next round**). `s915_i18n_second_opinion` (`w1:pR`) idle, reusable.

STAGE-9-CURSOR (2026-09-17, written by `garmin_inspector_v1_28` — supersedes the
           `garmin_inspector_v1_27` block above for SEQUENCING; that block's record stands.
           Per-id status lives only in findings.md):
           **B-STAGE9-19 IS CLOSED — two builder rounds, two independent reviewer passes, NO
           BLOCKING FINDING IN EITHER.** The list path now reports distinct codes
           (`no_client`, `timeout`) at exits that previously all folded to `unknown`, and the
           adapter's untranslated message reaches its own `gc_obs_raw` line instead of being
           discarded. Verified by the Inspector directly, not relayed: the suite ran
           **15 passed / 0 failed**, and an Inspector-authored mutation (removing the
           `NoClient` assignment) produced exactly ONE failure, in the right slot, folding back
           to kind 2 — then reverted to an md5 byte-identical file with the suite back to 15/15.
           **THE HEADLINE IS NOT THIS UNIT — IT IS WHAT THE BUILDER FOUND WHILE DISPROVING THE
           BRIEF.** `garmin_inspector_v1_27` asked the builder to try to prove its hypothesis
           wrong, and the builder did. The claim that the twelve 0-3ms live failures were
           `blockingList()`'s `!client` early return is **REFUTED**: that exit is unreachable
           from `readdir()`, which pre-guards a null client at `GarminConnect.cpp:759` and
           emits `no_session` — and ZERO `no_session` lines appear in the live log. The real
           cause is now **B-STAGE9-25** (filed this session, blocking): `garminconnect` 0.3.15
           validates `startdate` as date-ONLY, our `kGarminTimeFormat` is a full datetime, and
           `garmin_client.py:386` forwards it verbatim while catching only the two Garmin
           exception classes — so the `ValueError` propagates unclassified and folds to
           Unknown. The Inspector re-executed the wheel's own validator to confirm this rather
           than accepting the report. ONE cause explains all four measured symptoms. This is
           DEC-014 OQ1's deferred risk landing exactly where that comment said it might.
           **Sequencing the next Inspector must not lose:**
           (1) **B-STAGE9-25 is the next atomic unit, and it is the one that actually unblocks
           the live run.** B-STAGE9-19 only made the failure legible; -25 makes it stop
           failing. It needs BOTH halves: normalise sinceGmt to date-only, AND update the
           fakes/pystub so the stub rejects what the real wheel rejects — without the second
           half the same defect class stays invisible to the suite. Decide explicitly whether
           dropping time-of-day needs same-day re-filtering.
           (2) **NO SIXTH ATTENDED LIVE RUN until BOTH -19 and -25 have landed.** Running with
           -19 alone would produce fifteen newly-legible lines that all still fail.
           (3) Two non-blocking follow-ups are OWED and not yet filed as rows: (a) the
           exhaustiveness guard added in round 2 uses `#pragma GCC diagnostic error "-Wswitch"`,
           which is GCC/Clang-only and **silently degrades to no guard on MSVC** — the reviewer
           dissented from the Inspector's provisional acceptance and was right; the portable fix
           is a `KindCount` sentinel plus a `static_assert` on the table's row count. The
           runtime table still pins every current mapping on every compiler, which is why this
           is non-blocking. (b) `blockingDownload`'s two local exits BOTH fold to
           `GarminDownloadFailure::Network` (a 0-initialiser, no explicit init) and the download
           op emits **no gc_obs trace at all** — worse than the list path's version, since it is
           a WRONG code rather than merely an unhelpful one.
           (4) Measured operational constraint: ~20 parallel `cc1plus` thrash this 11.6 GB
           machine into swap and cost an hour of wall clock. Cap builds at `-j4`.

STAGE-9-CURSOR (2026-09-19, written by `garmin_inspector_v1_30` — supersedes the
           `garmin_inspector_v1_28` block above for SEQUENCING; that block's record stands.
           Per-id status lives only in findings.md):
           **B-STAGE9-25 IS CLOSED — the date-only cursor defect that actually broke the live
           sync is fixed.** Reviewer round 4 returned no blocking defect and closed (a)-(f).
           Round-1 (c) had survived three rounds unanswered and was settled this pass: an
           isolated investigator established from the installed wheel that the listing sends no
           timezone parameter at all, so Garmin's UTC-vs-local day choice is NOT establishable
           from source and was not measured. Ruled closed on a conditional-coverage argument
           that holds under either interpretation, with the unmeasured server behaviour recorded
           as an accepted residual — not as proof. Independently re-verified here, not relayed:
           pytest 74/74, mypy --strict clean, two mutations RED in the predicted slots with
           byte-identical restores, ctest 4/4. One builder self-report was FALSE (claimed
           gate-pinned ruff clean; it was not) and was repaired. Non-blocking residual split out
           as B-STAGE9-26.
           **THE SUPERVISION LOOP ITSELF WAS BROKEN, and that is the bigger finding of this
           pass — ORCH-064/065/066, all fixed.** `insp_wake.sh` had never once fired on its
           success path (a bare `wait` deadlocked on the `tee` from its own stdout redirect), and
           `dispatch.py` classified EVERY Codex reply as `truncated` because that TUI bullets the
           opening sentinel — which is the likely reason a previous round-3 review was recorded
           as never returning a verdict. Both were found only because the USER asked why no wake
           had fired; the Inspector had reported the wake armed on the absence of an error.
           **Sequencing the next Inspector must not lose:**
           (1) **B-STAGE9-12 is now the only outstanding blocking finding** and the next atomic
           unit: stderr buffering hides `gc_obs` traces in the real qmake binary. Its RED is
           already delivered and reviewer-reviewed; the production fix was frozen under the
           `src/Core/main.cpp` hold, and that hold was RELEASED by the user 2026-09-16, so the
           repair is permitted now.
           (2) Both preconditions for a sixth attended live run (-19 and -25 landed) are now MET.
           It is worth scheduling once B-STAGE9-12 lands, so the run's traces actually flush
           without depending on File > Quit.
           (3) Two tooling gaps found while working, NOT yet owed to a row: `.claude/settings.json`
           pointed its anti-duplication PreToolUse hook at a path that does not exist, which
           blocked every Write/Edit/Bash until repaired (the guard had therefore been dead, not
           merely noisy). RESOLVED, and the deletion was DELIBERATE, not drift: the user removed
           `.claude/hooks/` because each guard belongs to the package that owns it — the
           anti-duplication guard to the QGDW skill (`skills/quality-gated-dev-workflow/scripts/`,
           where settings.json now points) and the ledger-drift lint to the project's own
           `scripts/ledger_drift_lint.py`, which is tracked and has a test beside it. One real
           loose end came with it: `.pre-commit-config.yaml` still invoked the deleted synced copy,
           so that gate passed at commit time ONLY because pre-commit stashes unstaged changes and
           restored the file for the run, and it could not be reproduced by hand. Repointed to the
           canonical `scripts/` path (verified byte-identical to the deleted copy, rc=0 run
           directly), and the deletions staged, so the gate no longer depends on a stash.
           (4) The three repair rounds of 2026-09-18 were never given a STATE cursor; they are
           reconstructed only inside findings.md's B-STAGE9-25 row.

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_30`, written immediately before
           self-succession at 224,926/210,000 — over threshold. Sequencing only; per-id status
           lives in findings.md):
           **NOTHING IS IN FLIGHT. No agent is working, no wake is armed, nothing is dispatched.**
           Three commits landed this session and the register is consistent (0 malformed, the one
           OUTSTANDING row is genuinely open): `2ba7b500a` B-STAGE9-25, `3d4aa6168` the
           supervision-loop repairs, `d4e282393` the hook consolidation.
           **THE NEXT ATOMIC UNIT IS B-STAGE9-12 AND IT IS NOT DISPATCHED.** Successor: dispatch it,
           do not re-derive whether it is next. Its RED is already delivered and reviewer-reviewed;
           the `src/Core/main.cpp` hold that froze its production fix was RELEASED by the user
           2026-09-16, so the repair is permitted. The untracked `unittests/Core/stderrbuf/*` files
           plus the `unittests/CMakeLists.txt` hunk belong to THIS unit, not to any other — earlier
           cursors correctly excluded them from other commits.
           **A REAL SKILL DEFECT IS OPEN AGAINST THE LOOP ITSELF: ORCH-067.** The predecessor closed
           a unit, named B-STAGE9-12, armed a wake and stopped — with every agent idle the wake waits
           on a dispatch only the Inspector can make, so it was a no-op that read as supervision. The
           user caught it, not the tooling. Do not repeat it: identifying the next unit and arming a
           wake is stopping.
           **UNCOMMITTED AND DELIBERATELY NOT LANDED:** the ORCH-067 partial fix (settled heartbeat
           900000 -> 300000 in `insp_wake.sh`, plus the SKILL.md stall clause). `SKILL.md` and four
           reference files under `skills/inspector-cycle/` were ALREADY modified by someone else
           before this session, so committing them would have landed another workstream's in-flight
           edits. Verify each diff before staging; do not assume the whole skill directory is yours.
           **Roster, verified live:** `garmin_builder_stage9_v14` (`w1:pM`, Claude, Sonnet 5, auto
           mode confirmed from the status line, 0 tokens — never yet dispatched),
           `garmin_codex_reviewer` (`w1:pD`, ~74k), `s925_tz_investigator` (`w1:pR`, ~54k, idle and
           reusable). All three well under 250k.
           **Do not schedule a sixth attended live run yet.** Both stated preconditions (-19 and -25
           landed) are now met, but land B-STAGE9-12 first or the run's `gc_obs` traces only survive
           if the user quits via File > Quit — which is B-STAGE9-12 itself.

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_31` — supersedes the ADDENDUM above for
           SEQUENCING. Per-id status lives only in findings.md):
           **B-STAGE9-12 is DISPATCHED** to `garmin_builder_stage9_v14` (`w1:pM`, Sonnet 5,
           auto mode and `tok 0k/0k` verified from the pane before dispatch). The brief asks for
           the production repair plus the four repairs reviewer round 1 owed: the POSIX
           `setvbuf`, a WIN32 `_IONBF` companion, removal of the `GC_STDERR_MIRROR_APPLY_FIX`
           false-green channel in the mirror, and an executable source-contract check over
           `nostderr()` that asserts call ORDER separately from the fix's presence. T-209
           allocated to this unit (WIKI REGISTRIES pointer was already correct; grep-confirmed
           unused). DEC-054's obligation is carried into the brief: the registered test may not
           enter the default gate red and may not take `gate-exclude`.
           **Succession done:** `garmin_inspector_v1_30` exited cleanly and its pane `w1:p1X`
           and tab `w1:t1N` are closed; resumable as
           `claude --resume 37dadd82-9863-4fab-9d46-c6886a7cacd6`. Two tabs remain: `w1:tE`
           (builder/reviewer/investigator) and `w1:t1P` (Inspector).
           **Ledger hygiene this pass:** WIKI.md's DEC registry read `next:garmin-054` while
           DEC-054 already existed — corrected to `next:garmin-055`, no code changed.
           **Sequencing the next Inspector must not lose:** after the builder reports, the
           reviewer delta-check is mandatory BEFORE any Inspector rebuild, and this unit touches
           `src/Core/main.cpp`, so a qmake build against `src.pro` is owed as Inspector-side
           verification — the builder was told not to run one. Cap builds at `-j4`.
           The sixth attended live run becomes schedulable once this unit lands.
           **Round 1 outcome (2026-09-19):** the builder delivered, the reviewer's delta-check
           filed one BLOCKING defect — **B-STAGE9-27**, the WIN32 half of the fix — and the
           repair is DISPATCHED. Accepted from round 1 and not to be redone: the false-green
           env toggle is gone, the test is renamed and ungated, the driver hardening landed, and
           the builder's Qt finding (`qInstallMessageHandler(myMessageOutput)` at main.cpp:701
           is inside the `GC_START_HTTP||server` branch at :692, so Qt's own self-flushing
           default handler is what runs otherwise) was re-verified by the Inspector directly.
           That narrows which configuration exhibited the live symptom and is worth a look
           before the sixth attended run is designed.
           **Wake-script label is STALE, do not act on it:** `insp_wake.sh:159` hard-codes the
           released `src/Core/main.cpp` hold and prints `hold: VIOLATED` as soon as the builder
           touches that file — which this unit's brief authorises. It reports dirtiness, not
           permission. The neutral-wording repair was attempted 2026-09-19 and REFUSED by the
           Claude Code auto-mode classifier; not routed around. Read the hold from this cursor.

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_31`, written immediately before
           self-succession at 197,564/210,000. Sequencing only; per-id status lives in findings.md):
           **THE B-STAGE9-12 CODE FIX IS VERIFIED AND IS NOT THE OPEN QUESTION.** `nostderr()` now
           makes ONE `setvbuf` call at the only position C11 7.21.5.6p2 allows, mode selected by
           `#ifdef WIN32` (`_IONBF`) / `#else` (`_IOLBF`), return value checked with a `qDebug`.
           Re-verified by the Inspector, not relayed: both registered tests 2/2, `src/Core/main.cpp`
           md5 `f6f163a87c3bd40690711c2806c25210`, and an Inspector-authored mutation the builder was
           never asked for — swapping the two modes between branches, i.e. B-STAGE9-27's defect
           wearing the correct shape — went RED on exactly two lines and restored byte-clean.
           **IN FLIGHT: the B-STAGE9-27b commit-readiness round**, dispatched to
           `garmin_builder_stage9_v14`. Four items: clang-format the mirror (it is RED against the
           pinned 18.1.8 and IS matched by the hook, so the commit fails as-is), `ruff format` the two
           new `.py` files, widen `.pre-commit-config.yaml`'s ruff/ruff-format/mypy `files:` regexes to
           `unittests/Core/stderrbuf/.*\.py$`, and re-run both coverage guards. Arrival state was
           MEASURED with the pinned tools before the ruling — `ruff check` clean, `mypy --strict`
           clean, `ruff format` reflow-only — so mypy is included and NO gap may be declared for this
           directory. If widening mypy makes the buildguard tool_gap declaration go stale, the builder
           was told to stop rather than delete it.
           **Sequencing the next Inspector must not lose:**
           (1) Collect `--unit B-STAGE9-27b`, then dispatch the reviewer for a delta-check on the FULL
           diff — it has only ever seen the pre-repair version. Then the qmake rebuild against
           `src.pro` is OWED and is the Inspector's own: the builder was forbidden to run one, so no
           evidence yet exists that the real release binary compiles this change. Cap at `-j4`.
           (2) The commit set is `src/Core/main.cpp`, `unittests/CMakeLists.txt`, the four
           `unittests/Core/stderrbuf/*` files, `.pre-commit-config.yaml`, plus the ledgers. Verify each
           diff; the Coach/Qt6.8 workstream is unrelated and must not ride along.
           (3) `src/Core/main.cpp` is clang-format RED for PRE-EXISTING reasons and is deliberately
           NOT in the hook's regex (DEC-010 no-retrofit). Do not reformat it.
           (4) B-STAGE9-28 filed this session: the lint-ownership guard enumerates TRACKED files, so a
           new managed directory is invisible to it exactly while it is being created. It printed
           `garmin-cpp-stderrbuf: EMPTY` and still PASSed. Owes a decision, not a patch.
           (5) The builder is at 212k/250k and is due a soft-landing after this round.
           (6) T-209 is consumed by this unit; WIKI REGISTRIES advanced to `next:garmin-T-210`.
           Also corrected this session: the DEC pointer read `next:garmin-054` while DEC-054 existed.

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_32` — supersedes the two `v1_31` blocks
           above for SEQUENCING. Per-id status lives only in findings.md):
           **B-STAGE9-12 and B-STAGE9-27 are both fixed and committed; the last blocking
           finding on Stage 9's code is gone.** One `#ifdef WIN32`-selected `setvbuf` in
           `nostderr()` (`src/Core/main.cpp:188-195`), T-209's two tests, the
           `unittests/Core/stderrbuf/` directory, and a three-line
           `.pre-commit-config.yaml` widening. Committed `a0f9045e6` (11 files, +532/-10;
           all five pre-commit hooks passed).
           **Evidence that did not exist before this pass, and is the point of it:**
           (1) qmake `make -j8` capped to `-j4` against `src.pro` exit 0, binary relinked,
           `nm -uC src/GoldenCheetah` shows `U setvbuf@GLIBC_2.2.5` — the release build
           actually carries the fix, which no CMake run can tell you.
           (2) `s925_tz_investigator` MEASURED the fix instead of arguing it (glibc 2.41,
           isolated `/tmp` harness): 30/30 bytes on disk before exit and 30/30 surviving
           SIGKILL with `_IOLBF`, versus 0 and 0/30 without. It also reported the boundary
           rather than burying it — once `std::cerr` writes occur both variants flush, so
           the distinguishing window is `fprintf`-style writes, which is Qt's default
           handler path and therefore the live symptom.
           (3) Full default gate `ctest -LE gate-exclude` 56/56.
           **Sequencing the next Inspector must not lose:**
           (1) **THE SIXTH ATTENDED LIVE RUN IS NOW SCHEDULABLE AND IS THE NEXT UNIT.** Every
           stated precondition is met: -19, -25 and now -12 have all landed. `src.pro` points
           `GARMIN_PY_MODULE_DIR` at the source tree, and the binary was relinked this pass,
           so no further build is owed before it. Traces no longer depend on the user quitting
           via File > Quit. This is the one real human-in-the-loop gate — the user enters
           credentials into the app's own dialog, never into chat. Success criterion is a NEW
           file under `~/.goldencheetah/Andy/activities/` (1145 at last check), not the UI.
           (2) The two remaining open findings are B-STAGE9-26 and B-STAGE9-28, both
           non-blocking; -28 owes a scored decision and a DEC id, not a patch. Neither blocks
           the live run. Correction to the `v1_31`/`v1_30` cursors, which both named
           B-STAGE9-16 as still open: it is not — both halves were already fixed
           (`8611fb2d0`, `d9ba4faad`). Verified against findings.md this pass, not assumed.
           (3) B-STAGE9-28's blind spot was confirmed by measurement on both sides of
           `git add` this pass: the lint-ownership guard reports `EMPTY` and still passes
           while a managed directory is untracked. Until that is decided, the Inspector must
           re-run the guard AND `pre-commit run --files` at staging time — a new directory is
           invisible to the gate exactly while it is being created.
           (4) `insp_wake.sh:159` still hard-codes the released `src/Core/main.cpp` hold and
           prints `hold: VIOLATED`. It reports dirtiness, not permission. The neutral-wording
           repair was refused by the Claude Code auto-mode classifier 2026-09-19 and was not
           routed around; it is still owed.
           (5) Cap builds at `-j4` — ~20 parallel `cc1plus` swap this 11.6 GB machine.

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_32`, written immediately before
           self-succession at 229,356/210,000 — over threshold. Sequencing only; per-id status
           lives in findings.md):
           **THE SIXTH ATTENDED LIVE RUN HAPPENED AND IT CHANGED THE PICTURE. Read this before
           dispatching anything.**
           **What the live run PROVED (first time, all three):** `gc_obs op=auth outcome=ok`
           in 243ms against the real account off the stored token — authentication works.
           `gc_obs op=sync_incremental outcome=ok activity_count=1`, six consecutive times —
           B-STAGE9-25's cursor fix works against the live server. And every one of those lines
           was readable WHILE THE APP RAN, which is B-STAGE9-12 earning its commit on its first
           live use. Evidence snapshots (the log truncates on every launch, so these are the
           only copies): /tmp/gc-log-before-run6.log, /tmp/gc-run6-evidence-141416.log,
           /tmp/gc-run6-final.log.
           **WHAT IT FOUND — B-STAGE9-29, blocking, filed, NOT yet dispatched.** The service
           layer is correct and the DIALOG discards 100% of it, silently. `readdir` names
           entries `garmin-<id>.fit`; `CloudService.cpp:2164` skips anything failing
           `RideFile::parseRideFileName`, which exact-matches `yyyy_MM_dd_HH_mm_ss.<ext>`. Six
           successful listings, `0 of 0 selected` in the dialog, `activities/` still 1145.
           Every peer service (Strava, Xert, Nolio, Azum, CyclingAnalytics, SportTracks,
           SixCycle) names from LOCAL start time; we marshal only `startTimeGMT`. **The fix
           needs a DEC first** — naming from UTC would mis-sort, mis-filter and break the
           dialog's `Exists` check against a local-time athlete directory. Allocate DEC-056;
           WIKI pointer `next:garmin-055` was consumed by DEC-055 this pass, so advance it.
           **THE APP CRASHED on exit: SIGTRAP, exit 133, core dumped.** The core is PRESENT
           and preserved — `coredumpctl` PID 956840, 81.8M, 2026-09-19 14:50:13. Nobody has
           looked at it. Get a backtrace before it is rotated away; it is not yet filed as a
           row because its cause is unknown and a row asserting one would be a guess. Note the
           crash is itself evidence FOR B-STAGE9-12: an abnormal termination preserved every
           trace line, which is exactly the 0/30-bytes case the isolated measurement predicted
           would have been lost before the fix.
           **IN FLIGHT, both dispatched, neither collected:**
           (1) `s925_tz_investigator` (`w1:pR`) — unit `B-STAGE9-29-confirm`, asked to FALSIFY
           the root cause above and to read the installed wheel for the local-start-time key
           that decides the fix shape. Its answer to question 4 is the input to DEC-056.
           (2) `garmin_codex_reviewer` (`w1:pD`) — unit `B-STAGE9-26`, delta-check on the
           builder's diff.
           **B-STAGE9-26 is GREEN in the working tree, UNREVIEWED-UNTIL-(2)-RETURNS,
           UNCOMMITTED.** DEC-055 Option A: typed `response_invalid` naming the missing key,
           pystub double updated to match, RED shown first, mutation RED at the predicted point
           with byte-identical md5 restore, pinned ruff/mypy clean. Builder report:
           /tmp/insp-exchange/B-STAGE9-26.md. Do not commit it on that report alone.
           **CLV is FAIL — OUTSTANDING=1, and that is CORRECT, not drift:** B-STAGE9-29 is a
           genuinely open blocking row carrying a proper effect set (MISSING-EFFECT=0).
           `ledger_drift_lint.py` EXIT=0.
           **Roster, verified live:** builder `garmin_builder_stage9_v15` (`w1:pM`, Sonnet 5,
           auto mode, ~111k) — refreshed this session from `_v14` at 219,750. Reviewer
           `w1:pD` ~160k. Investigator `w1:pR` ~64k. All under 250k.
           **Do not re-run the live test until B-STAGE9-29 lands** — the dialog will show
           nothing again, and that wastes an attended session.

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_33` — supersedes the two `v1_32` blocks
           above for SEQUENCING. Per-id status lives only in findings.md):
           **B-STAGE9-29's root cause is confirmed and its remedy is decided and building.**
           `s925_tz_investigator` was asked to falsify the diagnosis and could not; it added
           three things the finding did not have (the Synchronize tab shares the Download
           loop, so one `continue` drops both; the auto-downloader at
           `CloudService.cpp:4254-4279` is a second independent dropping consumer; neither
           tab ever reads `e->modified`, which we do populate). DEC-056 accepted, Option A:
           name entries from the activity's own local start time via the library's
           `startTimeLocal`, `startTimeGMT`-converted-to-local as the fallback, shared dialog
           gate untouched. Renaming is safe because `readFile` fetches by `remoteid` and
           `recordImport` keys the sidecar by the same id — verified, not assumed.
           **Two findings opened from the Inspector's own read of B-STAGE9-26's diff**, which
           the reviewer's delta-check did not surface: B-STAGE9-30 (the pystub gained a
           `list_missing_activity_id` scenario no C++ test selects — dead coverage) and
           B-STAGE9-31 (the reviewer's own non-blocking item, contract prose at
           `garmin_client.py:385-389` that DEC-055 made false). -31 rides in B-STAGE9-29's
           dispatch; -30 is the next unit after it.
           **B-STAGE9-26 is FIXED and deliberately UNCOMMITTED.** Reviewer delta-check found
           no blocking defect; the Inspector read the diff independently and agrees. It stays
           in the tree because B-STAGE9-29 edits the same two files — one file's hunks split
           across two commits is a hazard this project has already paid for.
           **Sequencing the next Inspector must not lose:**
           (1) The seventh attended live run is still the acceptance criterion, and is still
           BLOCKED until B-STAGE9-29 lands. Do not schedule it before then — the dialog will
           show nothing again and an attended session is wasted. Success is a NEW file under
           `~/.goldencheetah/Andy/activities/` (1145 at last check), not the UI.
           (2) Run 6's abort is ANSWERED and is not ours — B-STAGE9-32, `not-a-defect` on
           disassembly evidence: a QtWebEngineCore `int3` on a Wayland screen-removal event,
           no Garmin or CloudService frame in any of 44 threads. Two corrections to the run-6
           narrative fell out of it: it was not "on exit" (the main thread was still in
           `QCoreApplication::exec`), and WebEngine debug symbols are absent so no further
           precision is available. Backtrace preserved at `/tmp/gc-core-956840-gdb-bt.txt`.
           (3) CLV is FAIL — OUTSTANDING=1, and that is CORRECT, not drift: B-STAGE9-29 is a
           genuinely open blocking row with a proper effect set (MISSING-EFFECT=0).
           `ledger_drift_lint.py` EXIT=0. Live row count 435; the carried-forward NOTE about
           this file's counts not reconciling against other counts still applies.
           (4) `insp_wake.sh:159` still hard-codes the released `src/Core/main.cpp` hold and
           prints `hold: VIOLATED`. It reports dirtiness, not permission. Still owed.
           (5) Cap builds at `-j4` — ~20 parallel `cc1plus` swap this 11.6 GB machine.
           **Roster, verified live:** builder `garmin_builder_stage9_v16` (`w1:pM`, Sonnet 5,
           auto mode, fresh) — `_v15` soft-landed at 221k after reporting honestly, exited,
           pane relaunched with `--permission-mode auto --model sonnet` and both confirmed on
           its status line. Reviewer `garmin_codex_reviewer` (`w1:pD`) `/new`-refreshed at
           182k while idle. Investigator `s925_tz_investigator` (`w1:pR`, 155k) idle.
           `garmin_inspector_v1_32` retired and its pane closed this pass.

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_33`, written before self-
           succession. Sequencing only; per-id status lives in findings.md):
           **B-STAGE9-29's implementation is COMPLETE ON DISK, passing, and UNPROVEN.**
           8 files, +299/-27. Do not re-implement any of it.
           **What is established:** builder `_v15` reported honestly rather than forcing a
           a pass — Python 77 passed, T-210 shown failing first; `testGarminConnectPyAdapter`
           and `testGarminConnectSync` both pass; four further Garmin targets link clean
           against the changed struct. `garmin_codex_reviewer` delta-check returned NO
           BLOCKING finding and answered the central question: every path by which
           `startTimeLocal` can be absent, None, empty or non-str lands on the documented
           GMT-to-local fallback, so no path reinstates the blank dialog. It also confirmed
           the edited `garmin-BBB.fit` assertion was retargeted, not weakened, and that the
           new tests would fail against pre-fix bytes.
           **What is NOT established, and is the next thing to collect:** T-211's C++ tests
           were written AFTER the production change and were never shown RED, and NEITHER
           mutation was run. Mutation 1 (revert `e->name` to the `garmin-%1.fit` form) is
           therefore the only evidence those tests detect the defect at all — if it does not
           go RED the tests are vacuous and this unit is not done. `garmin_builder_stage9_v16`
           is running exactly this as unit `B-STAGE9-29-proof`, plus mutation 2 (blank
           `startTimeLocal`, the fallback assertion must EXECUTE, not skip) and a Python
           re-run that a late comment-only edit invalidated. Collect with
           `dispatch.py --mode collect --target w1:pM --unit B-STAGE9-29-proof`.
           **Two non-blocking rows opened from the reviewer's read:** B-STAGE9-33
           (`parseGarminTime` relabels an offset-bearing timestamp instead of converting it —
           latent and pre-existing, but DEC-056 moved it onto a live path) and B-STAGE9-34
           (the C++ boundary stringifies a non-str `startTimeLocal` instead of rejecting it;
           safe only because production Python normalises first). Neither blocks the commit.
           **Commit shape when the proof lands:** B-STAGE9-26 and B-STAGE9-29 go in as ONE
           slice — they edit the same two files and splitting a file's hunks across two
           commits is a hazard already paid for here. Verify each path's diff before staging;
           the tree carries several unrelated workstreams. Expect pre-commit (clang-format,
           ruff, `mypy --strict`, ledger-drift-lint) to be the real bar, not ctest green.
           **Still owed and NOT done:** DEC-056's cascade on `design.md`'s listing prose,
           which still says `garmin-<id>.fit` — the builder was correctly out of paths for it.

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_34` — supersedes the `v1_33` blocks above
           for SEQUENCING. Per-id status lives only in findings.md):
           **B-STAGE9-29's tests are proven non-vacuous. One mutation is still owed.**
           Mutation 1 (revert `e->name` to `garmin-%1.fit`) went RED on four slots in
           `testGarminConnectSync`, restored byte-identical (md5
           `e5c5145ca4d5d18fd58eeb5635cb49f3`); pytest 77/77.
           Mutation 2 as briefed by `v1_33` could not reach the branch it targeted and the
           builder said so instead of banking the green — the pystub feeds only
           `testGarminConnectPyAdapter`, while every `testGarminConnectSync` slot uses
           `FakeListPyAdapter`. Filed as B-STAGE9-35. Re-dispatched as unit
           `B-STAGE9-29-proof2`: delete the C++ fallback at `GarminConnect.cpp:886-888`;
           ONLY `readdirNameFallsBackToGmtConvertedToLocalWhenStartTimeLocalIsMissing` may
           go RED.
           **DEC-056's design.md cascade is DONE and was narrower than `v1_33` recorded.**
           Only the LISTING name moved. `readFile` still stages `garmin-<id>.<ext>`
           (`GarminConnect.cpp:690`, `:752`) and that name never reaches disk — `saveRide`
           names from the parsed RideFile. design.md gained a section stating both names and
           why they differ; its four staging-name references were correct and left alone.
           **`B-STAGE9-29-proof2` LANDED — the unit's evidence is now complete.** Deleting
           the C++ fallback at `GarminConnect.cpp:887-888` made EXACTLY ONE slot go RED
           (`readdirNameFallsBackToGmtConvertedToLocalWhenStartTimeLocalIsMissing`, expected
           `2026_07_15_22_00_00.fit`, got `.fit`); the other three naming slots and
           `testGarminConnectPyAdapter` were unaffected — exactly the branch isolation the
           decision describes.
           Restored byte-identical, md5 `e5c5145ca4d5d18fd58eeb5635cb49f3` — Inspector
           re-checked the md5 against the live file, not the report. Both mutations now
           proven; T-211 is not vacuous on either the primary or the fallback path.
           **B-STAGE9-28 is DECIDED as DEC-057, and its own option A was FALSIFIED.**
           `git ls-files` reads the index, so the guard already sees staged-but-uncommitted
           files — `test_garmin_lint_ownership_guard.py:612-635` pins exactly that. Nothing
           to widen. DEC-057 instead requires an empty managed root to be DECLARED pre-armed
           or go RED. Non-blocking; queues behind the live run. WIKI DEC pointer advanced to
           `next:garmin-058`.
           **Inspector's own independent re-verification of the slice, not the builder's:**
           `cmake --build . -j4` exit 0 (561 targets, `src/GoldenCheetah` relinked),
           `ctest -LE gate-exclude` **56/56, 0 failed**, 319.51s; `pytest tests -q` 77/77.
           **THE SLICE IS COMMITTED `ce2b3402d`** (8 files, +299/-27). `garmin_codex_reviewer`
           (unit `B-STAGE9-26-29-commit`) answered the commit-readiness question NO — no
           real-wheel record reaching `readdir` is silently discarded by the filename gate or
           shadows an athlete file — and filed nothing blocking. Five non-blocking
           observations, all already-filed rows. All five pre-commit hooks passed on the
           second attempt; clang-format reflowed two columns of continuation alignment inside
           a regex string literal in `testGarminConnectSync.cpp`, which the Inspector
           confirmed cosmetic by rebuilding and re-running that target (1/1) before
           re-staging. B-STAGE9-26, -29 and -31 are closed against that hash.
           **CLV is PASS for the first time this stage — 0 OUTSTANDING / 439 OK.**
           `ledger_drift_lint.py` EXIT=0 (it caught one violation in this Inspector's own
           STATE edit — DEC-056 paired with `GREEN` — since fixed).
           **Sequencing:**
           (1) **THE SEVENTH ATTENDED LIVE RUN IS THE NEXT UNIT AND IT IS THE ONE REAL
           HUMAN-IN-THE-LOOP GATE.** Every stated precondition is now met: -19, -25, -12 and
           -29 have all landed. The user enters credentials into the app's own dialog, never
           into chat. Success is a NEW file under `~/.goldencheetah/Andy/activities/` (1145 at
           last check), not the UI. **A qmake relink is owed FIRST and is NOT done** —
           `ce2b3402d` is CMake-verified only and the shipped binary is the qmake one.
           `garmin_inspector_v1_34` started `make -j4` in `src/` at 18:11; `IGarminPyAdapter.h`
           changed, so it is regenerating moc widely and is slow. It was NOT verified before
           that Inspector handed off and must be re-run (make resumes; it is idempotent).
           Verify by mtime on `src/GoldenCheetah` AND `nm -uC src/GoldenCheetah | grep
           setvbuf` before scheduling the run. `src.pro:274` points `GARMIN_PY_MODULE_DIR` at
           the source tree, so the Python half needs no build step.
           (2) B-STAGE9-28 is decided (DEC-057) but NOT built. B-STAGE9-30, -33, -34, -35
           remain open and non-blocking. None blocks the live run.
           (3) `insp_wake.sh:159`'s hold check reported `main.cpp: CLEAN` correctly this pass
           — the false `hold: VIOLATED` was dirtiness, as recorded. Nothing owed unless it
           misreports again.
           (4) Cap builds at `-j4`.
           **Roster, verified live:** builder `garmin_builder_stage9_v16` (`w1:pM`, 108k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, 131k), investigator
           `s925_tz_investigator` (`w1:pR`, 189k) — all idle, all under 250k.
           (2) The seventh attended live run is still Stage 9's acceptance criterion and is
           still blocked until that slice commits. It is the one human-in-the-loop gate.
           Success is a NEW file under `~/.goldencheetah/Andy/activities/` (1145 at last
           check), not the UI.
           (3) B-STAGE9-28's option research is dispatched to `s925_tz_investigator` (unit
           `B-STAGE9-28-options`); it owes a scored DEC, not a patch.
           (4) `insp_wake.sh:159` still prints a false `hold: VIOLATED`. Still owed.
           (5) Cap builds at `-j4`.
           **Roster, verified live:** builder `garmin_builder_stage9_v16` (`w1:pM`, ~91k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, ~97k, idle), investigator
           `s925_tz_investigator` (`w1:pR`, ~155k). `garmin_inspector_v1_33` retired and its
           pane and tab closed this pass (5→4 panes, 3→2 tabs).

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_35` — supersedes the `v1_34` block above
           for SEQUENCING. Per-id status lives only in findings.md):
           **The qmake relink owed before the live run is DONE and independently verified.**
           `v1_34` reported its own `make` finished but said plainly it had never captured
           make's exit code. The Inspector re-ran `make -j4` in `src/` rather than inherit
           that: it was NOT a no-op — `GarminConnect.o` and four moc units recompiled and
           the binary relinked again. Real exit 0; `make -q` then exit 0 (up to date);
           `src/GoldenCheetah` 18:32:53, 27,400,336 bytes; `nm -uC | grep -c setvbuf` = 1.
           `src.pro:274` points `GARMIN_PY_MODULE_DIR` at the source tree, so the Python
           half needs no build step. The shipped binary now carries `ce2b3402d`.
           **B-STAGE9-33 does not block the live run.** `s925_tz_investigator` (unit
           `B-STAGE9-33-confirm`) falsified it for the primary path: DEC-056 names via
           `parseGarminLocalTime` (`GarminConnect.cpp:129`, `:885`, `:893`), which never
           calls `setTimeSpec(Qt::UTC)`. The relabelling risk survives only on the C++
           fallback (`:887-889`) and only if Garmin sends an offset-bearing `startTimeGMT`
           while `startTimeLocal` is absent — conditional, never observed. The wheel types
           both fields as bare optional `str` (`garminconnect/typed.py:406-407`), so no
           stronger claim is available from source.
           **B-STAGE9-28 (DEC-057) is built in the working tree, unreviewed, uncommitted.**
           Footprint is exactly the two allowed paths, +96/-5. The builder's reply exceeded
           the pane cap and was re-requested as a verbatim spill.
           **Sequencing:**
           (1) **THE SEVENTH ATTENDED LIVE RUN IS THE NEXT UNIT AND IS THE ONE REAL
           HUMAN-IN-THE-LOOP GATE.** Every precondition is now met. The user enters
           credentials into the app's own dialog, never into chat. Success is a NEW file
           under `~/.goldencheetah/Andy/activities/` (1145 at last check), not the UI.
           (2) B-STAGE9-28 owes a reviewer delta-check before any commit. B-STAGE9-30, -33,
           -34, -35 remain open and non-blocking.
           (3) Export `SELF_PANE` when arming `insp_wake.sh` — the wake block reports the
           Inspector's own context as `unknown` without it.
           (4) Cap builds at `-j4`.
           **Roster, verified live:** builder `garmin_builder_stage9_v16` (`w1:pM`, ~146k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, ~131k, idle), investigator
           `s925_tz_investigator` (`w1:pR`, ~228k — nearest the 250k soft-landing bar).
           `garmin_inspector_v1_34` retired, pane `w1:p21` and tab `w1:t1S` closed this pass
           (4→3 panes, 2 tabs remain).

STAGE-9-LIVE-RUN-7 (2026-09-19, `garmin_inspector_v1_35`, attended — **THE SYNC HALF OF
           STAGE 9'S ACCEPTANCE CRITERION IS MET FOR THE FIRST TIME**):
           A real activity reached disk from the real account. Inspector-read, not relayed:
           `~/.goldencheetah/Andy/activities/2026_09_13_20_33_17.json`, 2299 bytes, written
           18:38:31; the directory went 1145 → 1146. Traces: `gc_obs op=auth outcome=ok
           error_code= duration_ms=330` and `gc_obs op=sync_incremental outcome=ok
           error_code= duration_ms=1314 activity_count=1`. Sidecars written the same second:
           `backfill-state-ee9c52d8-….json`, `imported-ee9c52d8-….json`.
           **What this discharges that six prior runs could not:** DEC-056's local-start-time
           naming survived `CloudService.cpp:2164`'s `parseRideFileName` gate against the
           live server — the gate that silently dropped 100% of entries in run 6. Auth ran
           off the stored token, so the credential and MFA dialogs were NOT exercised.
           Evidence snapshot (the log truncates on every launch): `/tmp/gc-run7-evidence-*.log`.
           **Token baseline captured before the Disconnect leg:**
           `Andy/config/garminconnect/tokens.json` 2189 bytes, md5
           `b081b6d123e3ee88401e98696d3ad7f0`, mtime 2026-09-16 05:21:24.
           **DISCONNECT LEG — PASSED, both halves of REQ-012's own row title.** Against the
           baseline above: `tokens.json` and `active-account.json` both absent at 18:44:28
           with the app STILL RUNNING (immediate, no quit needed); the two sidecars survived
           byte-identical at their 18:38:31 mtimes. The `Trust Tokens` files under
           `Andy/temp/` are QtWebEngine's own Chromium storage, not ours.
           **RECONNECT LEG — PASSED, and proved a clause nobody had evidence for.**
           `tokens.json` recreated 18:49:27, 2189 bytes, **mode 600** — REQ-006's owner-only
           requirement confirmed against a real file for the first time. Three subsequent
           `gc_obs op=sync_incremental outcome=ok activity_count=0` with the activities
           directory holding at 1146: after a full credential cycle the sync did NOT
           re-download the already-imported activity, so REQ-012's "sidecars preserved" is
           functionally load-bearing, not just a file-existence claim.
           **MFA IS NOT EXECUTABLE ON THIS ACCOUNT — the user has no two-factor enabled.**
           REQ-003's live leg is therefore UNEXECUTABLE here, not passed. It stays at its
           seam verdict; do not infer a live pass from the successful reconnect.
           **B-STAGE9-37 opened from this run's own traces** (non-blocking): `gc_obs op=auth`
           times the token RESTORE, not the SSO, so no `duration_ms` figure it emits may be
           used to close REQ-NF-Perf-001's first-connect clause.

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_35`, written before self-
           succession at ~193k/210k. Sequencing only; per-id status lives in findings.md):
           Run 7 is committed `ebe194a32`. **What the next Inspector owes, in order:**
           (1) **The traceability.md per-id cascade for run 7 is NOT done** — this is the
           single biggest owed item. REQ-002, REQ-006, REQ-012 and REQ-017 all now have LIVE
           evidence recorded only in STATE.md's LIVE-RUN-7 block; their rows still carry
           seam-only or pre-live wording. REQ-003 must be marked live-UNEXECUTABLE (no
           two-factor on this account), NOT passed. Read the LIVE-RUN-7 block, then write
           each row once.
           (2) `garmin_builder_stage9_v16` is mid-repair on B-STAGE9-36 (unit `B-STAGE9-36`,
           collect with `dispatch.py --mode collect --target w1:pM --unit B-STAGE9-36`). It
           was at 206k when dispatched — **check it against the 250k bar before dispatching
           anything further; it is the nearest agent to a soft landing.** B-STAGE9-28 stays
           UNCOMMITTED until -36 closes and the reviewer re-checks; the two units touch the
           same two files and splitting a file's hunks across commits is a hazard already
           paid for here.
           (3) CLV FAIL with OUTSTANDING=1 is CORRECT — B-STAGE9-36 is genuinely open with a
           proper effect set. `ledger_drift_lint.py` EXIT=0.
           (4) Still open and non-blocking: B-STAGE9-30, -33, -34, -35, -37.
           (5) `insp_wake.sh` needs `SELF_PANE=<your pane>` exported when arming or it reports
           the Inspector's own context as `unknown`. Cap builds at `-j4`.
           (6) The qmake binary at `src/GoldenCheetah` (18:32:53) is current and carries
           `ce2b3402d`; no rebuild is owed. GoldenCheetah was still RUNNING at handoff.
           **Roster, verified live:** builder `garmin_builder_stage9_v16` (`w1:pM`, 206k,
           working), reviewer `garmin_codex_reviewer` (`w1:pD`, 158k, idle), investigator
           `s925_tz_investigator` (`w1:pR`, 23k, idle).

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_36` — supersedes the `v1_35` blocks above
           for SEQUENCING. Per-id status lives only in findings.md / traceability.md):
           **The run-7 traceability cascade the ADDENDUM called the biggest owed item is
           DONE.** Five rows written once each from the LIVE-RUN-7 block: REQ-002 (LIVE
           VERIFIED, via the RECONNECT leg only — the first leg ran off the stored token and
           is not SSO evidence), REQ-006 (LIVE VERIFIED, real `tokens.json` mode 600;
           load-refusal half still seam), REQ-012 (LIVE VERIFIED, both halves), REQ-017 (LIVE
           CORROBORATED, NOT a full pass — no sync was in flight at disconnect), REQ-003
           (**LIVE LEG UNEXECUTABLE** — no two-factor on this account; not a pass, and the
           successful reconnect must never be read as one).
           **Sequencing:**
           (1) B-STAGE9-36 builder-GREEN collected (spill `/tmp/insp-exchange/B-STAGE9-36.md`,
           now transcribed). `pre_armed_by` gains a shape gate (`^DEC-\d{3}$`) plus an
           existence check against `decisions.md`, failing loudly on an unreadable ledger.
           Reviewer dispatched on the COMBINED B-STAGE9-28+36 diff (+221/-5 over the two
           `unittests/buildguard/` files) as unit `B-STAGE9-36-review`. Nothing commits until
           that verdict lands — the two units touch the same two files and must ship as one.
           (2) Stage 9's one remaining non-live gap is REQ-NF-Pkg-001's Phase-1 smoke
           checklist (prd.md:115). CONTRIBUTING.md still has zero Garmin content.
           `s925_tz_investigator` dispatched as unit `REQ-NF-Pkg-001-linux-smoke` on the real
           question: `src.pro:274` points `GARMIN_PY_MODULE_DIR` at the SOURCE TREE, so an
           installed Linux package may reach a user with no Garmin Python at all. Win/macOS
           legs stay unexecutable on this Linux-only machine.
           (3) **B-STAGE9-38 opened and it is the biggest thing found today.** The
           investigator's packaging answer came back NO and the Inspector re-read every
           recipe claim rather than relay it: `src/src.pro` has ZERO `INSTALLS` entries and
           no recipe on any platform copies `src/Python/garminconnect/garmin_client.py` —
           this project's own adapter module, which `PyEmbeddedAdapter.cpp` imports by bare
           name and which is NOT the upstream wheel beside it. Separately `src.pro:274`
           compiles `GARMIN_PY_MODULE_DIR` as a build-machine absolute path that
           `appveyor/linux/after_build.sh:18`'s bare `cp` never rewrites. `e609215f0`, the
           commit REQ-NF-Pkg-001 was closed on, added two lines to `requirements.txt` and
           nothing else. Every green ctest run and all seven live runs used the source tree,
           so no existing evidence touches the packaged path. REQ-NF-Pkg-001's row is
           REOPENED. **Decided the same pass as DEC-058** over the investigator's four
           researched options: the adapter ships as a real installable distribution under a
           NEW import namespace, carried by the `pip install -r requirements.txt` step all
           three platforms already run, so the build-machine macro stops being consulted
           rather than being rewritten. Two constraints the option report missed are pinned
           in the DEC — Windows's `--only-binary :all:` (`appveyor.yml:143`) rejects a local
           sdist outright, and Linux runs pip from `src/` while macOS/Windows run it from the
           repo root, so a relative path resolves differently per platform. NOT YET BUILT.
           Stage 9 cannot close on the live evidence alone.
           (4) **B-STAGE9-39 opened, SUSPECTED and not yet reproduced:** `main.cpp:523-552`
           initialises CPython on the Garmin-only path without `PythonEmbed.cpp:236-254`'s
           deployed-`PYTHONHOME` setup. Invisible in a source-tree run; likely fatal inside a
           bundle. Same class as B-STAGE9-01. Verify it through DEC-058's bundle-import smoke
           step rather than a new harness; `main.cpp` is under the hard hold, so it owes its
           own unit and its own commit.
           (5) Still open and non-blocking: B-STAGE9-30, -33, -34, -35, -37.
           (6) `clv_findings.py` FAIL / OUTSTANDING=3 is CORRECT — B-STAGE9-36, -38 and -39
           are all genuinely open and blocking with proper effect sets. `ledger_drift_lint.py`
           EXIT=0. WIKI DEC registry bumped to `next:garmin-059`.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`. Cap builds at `-j4`. The qmake
           binary at `src/GoldenCheetah` still carries `ce2b3402d`; no rebuild owed.
           **Roster:** builder **`garmin_builder_stage9_v17`** (`w1:pM`) — soft-landed at 242k
           while idle and clean rather than after a repair round pushed it past the bar;
           relaunched on Sonnet in auto mode, 0k, UNBRIEFED and awaiting the reviewer verdict.
           Reviewer `garmin_codex_reviewer` (`w1:pD`, 158k, working), investigator
           `s925_tz_investigator` (`w1:pR`, 37k, working). `garmin_inspector_v1_35` retired,
           pane `w1:p22` and tab `w1:t1T` closed (3→2 tabs).

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_36`, written before self-
           succession at ~180k/210k. Sequencing only; per-id status lives in findings.md):
           Everything above is committed `925328f75` (governance only — run-7 cascade,
           DEC-058, B-STAGE9-38/-39, B-STAGE9-36 round 1). **What the next Inspector owes,
           in order:**
           (1) `garmin_builder_stage9_v17` (`w1:pM`) is mid-round-2 on B-STAGE9-36. Collect
           with `dispatch.py --mode collect --target w1:pM --unit B-STAGE9-36-r2`. Round 1's
           reply overflowed the pane twice, so expect to re-request a verbatim spill to
           `/tmp/insp-exchange/B-STAGE9-36-r2.md`. On GREEN, the reviewer (`w1:pD`) MUST
           delta-check round 2 before any commit — round 1 came back NOT-CLOSED and the same
           reviewer has now found a real defect in this unit twice.
           (2) B-STAGE9-28 still stays UNCOMMITTED until -36 closes; same two files, one
           commit. Nothing else may touch those two paths.
           (3) **DEC-058 is decided and NOT built** — this is the largest open piece of work
           and it is what Stage 9 now blocks on, not the live account. Read the DEC's
           "Constraints the build must honour" section before dispatching: three of the six
           are things the option research got wrong or missed.
           (4) B-STAGE9-39 is SUSPECTED, not reproduced. Do not dispatch a harness for it —
           it is verified through DEC-058's bundle-import smoke step or not at all, and
           `main.cpp` is under the standing hard hold.
           (5) `clv_findings.py` FAIL / OUTSTANDING=3 is CORRECT (B-STAGE9-36, -38, -39).
           `ledger_drift_lint.py` EXIT=0.
           (6) Export `SELF_PANE=<your pane>` when arming `insp_wake.sh`. Cap builds at `-j4`.
           GoldenCheetah is no longer running; the live gate is discharged and does not need
           reopening.
           **Roster, verified live:** builder `garmin_builder_stage9_v17` (`w1:pM`, 157k,
           working — soft-landed once already this session at 242k), reviewer
           `garmin_codex_reviewer` (`w1:pD`, 178k, idle — nearest the 250k bar), investigator
           `s925_tz_investigator` (`w1:pR`, 106k, idle).

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_37` — supersedes the two `v1_36` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / traceability.md):
           (1) B-STAGE9-36 round 2 collected and independently re-verified (244 buildguard
           tests, live guard CLI exit 0, all five `pre_armed_by` inputs probed against the
           REAL ledger). Reviewer round 2 returned **B-STAGE9-28 CLOSED, B-STAGE9-36
           NOT-CLOSED** — `prefix in entry` is a substring test over prose, so a negation or
           a fenced example authorizes a root. **DEC-059** decided and round 3 dispatched:
           a DEC arms a root by an explicit `- Arms:` bullet matched as whole-entry set
           membership, not by prose a guard greps. DEC-054's entry gained its
           `- Arms: unittests/Core/stderrbuf/*` bullet this pass — Inspector's edit, the
           builder may not touch the ledger.
           (2) B-STAGE9-28 still ships in the same commit as -36; those two buildguard files
           stay untouched by anything else.
           (3) DEC-058 remains decided and UNBUILT — still what Stage 9 blocks on, not the
           live account. `s925_tz_investigator` dispatched as `DEC-058-pip-mechanics` to
           settle the three unverified pip constraints (local path under
           `--only-binary :all:`, per-platform relative-path CWD, minimal build backend)
           before a builder starts on it.
           (4) B-STAGE9-39 stays SUSPECTED and un-harnessed; `main.cpp` hard hold in force.
           (5) `clv_findings.py` FAIL / OUTSTANDING=3 is CORRECT (B-STAGE9-36, -38, -39).
           `ledger_drift_lint.py` EXIT=0. WIKI DEC registry bumped to `next:garmin-060`.
           (6) Export `SELF_PANE` when arming `insp_wake.sh`. Cap builds at `-j4`.
           **Roster, verified live:** builder `garmin_builder_stage9_v17` (`w1:pM`, 187k,
           working round 3), reviewer `garmin_codex_reviewer` (`w1:pD`, 210k, idle — nearest
           the 250k bar, soft-land before its next long pass), investigator
           `s925_tz_investigator` (`w1:pR`, 107k, working). `garmin_inspector_v1_36` retired,
           pane `w1:p23` and tab `w1:t1V` closed (3→2 tabs).

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_37`, written before self-
           succession at ~189k/210k. Sequencing only; per-id status lives in findings.md):
           Committed this pass: `ee998e6b8` (DEC-059) and `dc12db32b` (its Amendment).
           **What the next Inspector owes, in order:**
           (1) `garmin_builder_stage9_v18` (`w1:pM`, fresh, Sonnet/auto) is mid-ROUND 4 on
           B-STAGE9-36. Collect with `dispatch.py --mode collect --target w1:pM --unit
           B-STAGE9-36-r4`; expect a spill to `/tmp/insp-exchange/B-STAGE9-36-r4.md`.
           Round 4 closes four parse defects in the `- Arms:` bullet reader: prose on the
           bullet line being harvested, a second bullet silently ignored, `\s*` crossing a
           newline, and a fenced-block bullet counting. Two were found by the Inspector, two
           by the reviewer.
           (2) **On GREEN the reviewer MUST delta-check round 4 before any commit.** This
           reviewer has now found a real blocking defect in this unit on three consecutive
           rounds, and the Inspector found two more it had missed. Do not shorten this loop.
           (3) B-STAGE9-28 is CLOSED on the reviewer's own verdict but stays UNCOMMITTED
           until -36 closes; same two files, one commit. Nothing else may touch those paths.
           (4) **DEC-058 is decided and NOT built — still the largest open piece and what
           Stage 9 blocks on, not the live account.** `s925_tz_investigator` (`w1:pR`) is
           mid-unit `DEC-058-pip-mechanics`, settling the three unverified pip constraints
           before a builder starts. Collect it before dispatching any DEC-058 build.
           (5) B-STAGE9-39 stays SUSPECTED, un-harnessed; `main.cpp` hard hold in force.
           (6) `clv_findings.py` FAIL / OUTSTANDING=3 is CORRECT (B-STAGE9-36, -38, -39).
           `ledger_drift_lint.py` EXIT=0. WIKI DEC registry at `next:garmin-060`.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`. Cap builds at `-j4`.
           (8) Inspector-tooling note, not a project lesson: committing governance while the
           builder is live makes pre-commit stash and restore its unstaged work. It survived
           twice this pass, but re-check the builder's diff after every such commit.
           **Roster, verified live:** builder `garmin_builder_stage9_v18` (`w1:pM`, fresh 0k,
           working round 4 — v17 soft-landed at 227k while idle and clean), reviewer
           `garmin_codex_reviewer` (`w1:pD`, 61k after a `/new` at 210k, idle), investigator
           `s925_tz_investigator` (`w1:pR`, 114k, working). `garmin_inspector_v1_36` retired,
           pane `w1:p23` and tab `w1:t1V` closed this pass (3→2 tabs).

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_38`, written before self-
           succession at ~185k/210k. Sequencing only; per-id status lives in findings.md):
           Committed this pass: `b8bb7ec55` (B-STAGE9-35 closed, DEC-058 Amendment + its
           correction).
           **What the next Inspector owes, in order:**
           (1) B-STAGE9-36 ROUND 6 is GREEN and Inspector-verified (258 tests, live CLI
           exit 0, all three round-5 forgery inputs now None, real ledger still
           `('unittests/Core/stderrbuf/*',)`). The builder is IDLE.
           (2) **The round-6 reviewer delta-check is IN FLIGHT** (unit
           `B-STAGE9-36-r6-review`, `w1:pD`) and MUST land before any commit — it has found
           a real blocking defect on five consecutive rounds. Collect it, then either
           commit -36 + -28 together or dispatch round 7.
           (3) B-STAGE9-28 is CLOSED and stays UNCOMMITTED until -36 closes; same two files,
           one commit. Nothing else may touch those paths.
           (4) **Soft-land the builder once -36 closes, BEFORE dispatching DEC-058** — it
           will be near 230k and DEC-058's build is the largest remaining unit.
           (5) **DEC-058 is decided, amended twice on measurement, and NOT built — still what
           Stage 9 blocks on, not the live account.** Its Amendment (constraints 7-11 in
           `## DEC-058`) now pins the distribution root, the three per-leg pip commands, the
           Windows interpreter, the missing repo-root `cd` on macOS/Windows, and the AppImage
           import smoke, and constraint 12 records the layout PROVEN against a /tmp copy of
           the real tree plus the eleven test files whose imports migrate with it.
           `s925_tz_investigator` (`w1:pR`) is mid-unit `DEC-058-import-callsites`,
           enumerating every place the old bare import name is baked in; collect it before
           dispatching the build.
           (6) B-STAGE9-39 stays SUSPECTED, un-harnessed; `main.cpp` hard hold in force.
           (7) `clv_findings.py` FAIL / OUTSTANDING=3 is CORRECT (B-STAGE9-36, -38, -39).
           `ledger_drift_lint.py` (at `scripts/`, takes a ROOT argument) EXIT=0.
           (8) Export `SELF_PANE` when arming `insp_wake.sh`, and invoke the script directly —
           the `VAR=val bash script` form is refused by the auto-mode classifier every time.
           (9) The pre-commit stash/restore hazard is real and recurred this pass: commit in
           the builder's idle gap between units, and re-run its own test subset afterward.
           **Roster, verified live:** builder `garmin_builder_stage9_v18` (`w1:pM`, ~190k,
           working round 6), reviewer `garmin_codex_reviewer` (`w1:pD`, 149k, idle),
           investigator `s925_tz_investigator` (`w1:pR`, ~164k, working).
           `garmin_inspector_v1_37` retired, pane `w1:p24` and tab `w1:t1W` closed this pass
           (3→2 tabs).

STAGE-9-CURSOR (2026-09-19, `garmin_inspector_v1_39` — supersedes the two `v1_38` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) **B-STAGE9-36 round 7 GREEN and Inspector-verified** (263 buildguard tests,
           live guard CLI exit 0, PASS 104/115 unchanged) but reviewer round 7 returned
           **NOT-CLOSED** on a seventh consecutive new container type: `<div>`, CommonMark
           HTML block type 6. Types 3/4/5/7 sit behind it.
           (2) **DEC-060 decided** — the scanner was becoming a hand-rolled CommonMark
           parser, so the grammar changes instead: an Arms declaration NAMES ITS OWN DEC
           (`- Arms DEC-054:`), is honoured only inside that entry, and a mismatched id is
           a hard parse error, never a silent skip. Block context stops being consulted for
           the bullet. The CommonMark-library option is blocked on measurement (no parser
           installed; `additional_dependencies: []`), and the self-id'd-line-inside-a-fence
           false positive is pinned as accepted on DEC-053's terms. **Round 8 is NOT yet
           dispatched** — it is queued behind the builder, and it also owes the respelling
           of DEC-054's own `- Arms:` bullet, which is the Inspector's edit, not the
           builder's.
           (3) B-STAGE9-28 stays CLOSED, frozen and UNCOMMITTED until -36 closes; same two
           files, one commit. Nothing else may touch those paths.
           (4) **DEC-058 unit 1 of 2 (`DEC-058-rename`) is IN FLIGHT** on a fresh builder.
           New **constraint 13** records every executable binding of the old top-level name
           (six C++ import literals, the `kAllowedModuleRoots` root, both path defaults, the
           pystub fixture and its three consumers, eleven test files) and the reason a
           partial rename passes GREEN: `PyEmbeddedAdapter.cpp:93-109` prepends a directory
           that still holds the legacy module. The brief's defence is to delete the legacy
           modules FIRST. Unit 2 (per-leg pip steps + the smoke) is not dispatched.
           (5) **B-STAGE9-39's verification route was FALSE and is withdrawn.** DEC-058
           constraint 11 runs the extracted `usr/bin/python3`, never the GoldenCheetah
           binary, so it cannot reach `main.cpp:523-552`. It owes an APP-PROCESS smoke,
           MEASURED unexecutable here (`patchelf`/`linuxdeployqt`/`appimagetool` absent, no
           `/dev/fuse`, downloads prohibited). `qmake` is NOT missing — it is `qmake6` /
           `/usr/lib/qt6/bin/qmake`, not the bare `qmake` the recipe calls.
           `s925_tz_investigator` is mid-unit `B-STAGE9-39-app-process-smoke`; collect it.
           (6) `main.cpp` hard hold in force. `clv_findings.py` OUTSTANDING=3 still CORRECT
           (-36, -38, -39). WIKI DEC registry bumped to `next:garmin-061`.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`. The anti-duplication guard's `2>/dev/null` false positive (ORCH-062 /
           LSN-036) blocks the builder on read-only commands — approve and move on.
           **Roster, verified live:** builder `garmin_builder_stage9_v19` (`w1:pM`, fresh —
           v18 soft-landed at 223k while idle and clean), reviewer `garmin_codex_reviewer`
           (`w1:pD`, `/new` at 197k), investigator `s925_tz_investigator` (`w1:pR`, 116k).
           `garmin_inspector_v1_38` retired, pane `w1:p25` and tab `w1:t1X` closed (3→2).

STAGE-9-CURSOR-ADDENDUM (2026-09-19, `garmin_inspector_v1_39`, written before self-
           succession at ~181k/210k. Sequencing only; per-id status lives in findings.md):
           Nothing committed this pass — the builder has been live throughout and this
           tree's pre-commit stashes unstaged work. **The governance edits below are
           UNCOMMITTED and owed a commit in the builder's next idle gap:** DEC-058
           constraint 13, DEC-060 + its index row, the WIKI pointer, B-STAGE9-39's
           correction, B-STAGE9-40, and this cursor.
           **What the next Inspector owes, in order:**
           (1) `garmin_builder_stage9_v19` (`w1:pM`, fresh) is mid-unit `DEC-058-rename`.
           Collect with `dispatch.py --mode collect --target w1:pM --unit DEC-058-rename`.
           On GREEN the reviewer MUST delta-check before any commit.
           (2) **B-STAGE9-36 round 8 is decided but NOT dispatched.** Build it to DEC-060,
           not to round 7's shape. It also owes DEC-054's bullet being respelled
           `- Arms DEC-054:` — that is the Inspector's ledger edit, never the builder's.
           B-STAGE9-28 stays CLOSED, frozen, and ships in the same commit as -36.
           (3) **B-STAGE9-40 is NEW, blocking, and the most consequential thing found this
           pass.** `main.cpp:552` discards `ensureInitialized()`'s failure Result — MEASURED
           on a real app process: empty `PYTHONHOME` produced `Failed to import encodings
           module` and the app ran on with exit 0. It is why -39 would be invisible rather
           than merely latent, and it is independent of -39's own fix. Also MEASURED:
           `--version` exits at `main.cpp:399-401` BEFORE init, and `--version` is all CI
           runs on Linux and macOS today — so the existing smoke proves nothing about Python
           init anywhere. `s925_tz_investigator` (`w1:pR`) has FINISHED unit `B-STAGE9-40-options`
           (three scored options for what the startup path should DO on failure) and the
           reply is UNCOLLECTED — collect it first. That owes a DEC (next id `garmin-061`)
           before any patch, and `main.cpp`'s hard hold means its own unit and commit.
           (4) DEC-058 unit 2 (per-leg pip steps, constraints 8-11) is NOT dispatched. Its
           AppImage smoke is MEASURED unexecutable on this machine; expect to DECLARE that
           leg rather than fake it.
           (5) `clv_findings.py` FAIL / OUTSTANDING=**4** is CORRECT (-36, -38, -39, -40).
           `ledger_drift_lint.py` (repo `scripts/`, takes a ROOT arg) EXIT=0.
           (6) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`. The anti-duplication guard blocks the builder on read-only commands
           containing `2>/dev/null` (ORCH-062 / LSN-036) — approve and move on.
           **Roster, verified live:** builder `garmin_builder_stage9_v19` (`w1:pM`, ~80k,
           working), reviewer `garmin_codex_reviewer` (`w1:pD`, `/new` at 197k, idle and
           UNBRIEFED), investigator `s925_tz_investigator` (`w1:pR`, ~133k, working).
           `garmin_inspector_v1_38` retired, pane `w1:p25` and tab `w1:t1X` closed (3→2).

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_40` — supersedes the two `v1_39` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) **The hard hold on `src/Core/main.cpp` is NOT in force and has not been since
           2026-09-16**, when the user released it (line 750 of this file). Cursors `v1_36`
           through `v1_39` carried "hard hold in force" forward without naming any re-imposing
           authority or act; `garmin_codex_reviewer`, fresh and unbriefed (unit
           `DEC-061-hold-premise`), searched all three ledgers and found none. The false
           assertions in findings.md and in DEC-058's entry are corrected. `insp_wake.sh:159`
           still hard-codes the released hold as a check — harmless, still unfixed.
           (2) **DEC-061 decided and recorded** — B-STAGE9-40's remedy is to CONSUME
           `ensureInitialized()`'s failure `Result` at `main.cpp:552` nonfatally, not to add a
           Garmin-local refusal and leave the discard. The investigator's recommendation was
           re-scored down once the hold premise it rested on was found false. Not yet built;
           `main.cpp` still gets its own unit and commit, for scope reasons only.
           (3) `DEC-058-rename` (unit 1 of 2) came back GREEN — 77 pytest, `ctest -L garmin-py`
           5/5, wheel built and imported from an install target. **Reviewer delta-check
           `DEC-058-rename-review` is IN FLIGHT and must land before any commit of it.**
           Unit 2 (per-leg pip steps, constraints 8-11) is not dispatched.
           (4) **B-STAGE9-36 round 8 is decided (DEC-060) and still NOT dispatched.** It also
           owes DEC-054's bullet being respelled `- Arms DEC-054:` — the Inspector's ledger
           edit, deliberately held until round 8 is dispatched, because respelling it early
           takes the round-7 guard in the tree RED. B-STAGE9-28 stays CLOSED, frozen,
           and ships in the same commit as -36.
           (5) `B-STAGE9-40-smoke-surface` is in flight on `s925_tz_investigator` — measure
           the cheapest surface that actually reaches `main.cpp:552`, given `--version` exits
           at `:399-401` and AppImage tooling is absent here.
           (6) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -40) still correct.
           `ledger_drift_lint.py` EXIT=0. WIKI DEC registry bumped to `next:garmin-062`.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`.
           **Roster, verified live:** builder `garmin_builder_stage9_v19` (`w1:pM`, 137k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, 73k), investigator `s925_tz_investigator`
           (`w1:pR`, 182k). `garmin_inspector_v1_39` retired, pane `w1:p26` and tab `w1:t1Y`
           closed (3→2 tabs).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_40`, written before self-
           succession at ~190k/210k. Sequencing only; per-id status lives in findings.md):
           Committed this pass: `a8dd771b4` (DEC-061 + the hold correction) and `80f9e17ef`
           (DEC-058 unit 1, the `gc_garmin_adapter` rename).
           **What the next Inspector owes, in order:**
           (1) **B-STAGE9-36 round 8 is NOT-CLOSED** on an eighth consecutive reviewer
           finding, and the class finally changed: it is SUPPRESSION, not forgery. An
           Arms-SHAPED near-miss is silently skipped instead of raising
           `ArmsBulletMalformed`, so a valid first bullet still permits PASS. Two BLOCKING
           inputs at `garmin_lint_ownership_guard.py:998,1043-1068`. Round 9 is NOT dispatched.
           Reviewer item 5 (a real coverage gap, NON-BLOCKING) rides with it.
           (2) **Unit `DEC-058-recipes` is built and UNCOMMITTED** — three
           recipe legs, `bash -n` and YAML-parse clean, which caught a real defect (a
           double-quoted `C:\Python\python.exe` is an illegal YAML escape). Constraint 11's
           AppImage smoke is DECLARED unverified, not faked, and is correct to leave so.
           **It owes a reviewer delta-check before any commit**, same as unit 1.
           (3) B-STAGE9-28 stays CLOSED, frozen, and ships in the same commit as -36.
           (4) **DEC-061 is decided and NOT built** — consume `ensureInitialized()`'s
           `Result` at `main.cpp:552`. `B-STAGE9-40-smoke-surface` MEASURED that no honest
           smoke exists until that fix lands, so this is the precondition for verifying
           B-STAGE9-39, not a parallel task.
           (5) `B-STAGE9-39-reproduce` is IN FLIGHT on `s925_tz_investigator` — settle a
           finding that has been SUSPECTED for days, by simulating the bundled Python home
           in /tmp against the real binary. Collect it.
           (6) **There is no hold on `src/Core/main.cpp`.** Any ledger line still saying so
           is stale; see the `v1_40` cursor above. `insp_wake.sh:159` still hard-codes the
           check — harmless, unfixed.
           (7) DEC-058 gained constraints 14-16 from measurement: each leg's pip step builds
           a wheel and needs a build backend, and install ORDER is load-bearing because the
           adapter swallows its own dependency `ImportError`.
           (8) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`. Commit in the builder's idle gap — pre-commit stashes its unstaged work.
           **Roster, verified live:** builder `garmin_builder_stage9_v20` (`w1:pM`, 79k —
           v19 soft-landed at 278k after reporting GREEN), reviewer `garmin_codex_reviewer`
           (`w1:pD`, 171k, nearest the bar), investigator `s925_tz_investigator` (`w1:pR`,
           fresh after `/new` at 203k). `garmin_inspector_v1_39` retired, pane `w1:p26` and
           tab `w1:t1Y` closed (3→2 tabs).

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_41` — supersedes the two `v1_40` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) **B-STAGE9-36 round 9 DISPATCHED** (`B-STAGE9-36-r9`). Round 8's verdict named
           two BLOCKING inputs; the Inspector probed `_armed_patterns` directly and ONE is
           withdrawn — a second same-id bullet already raises. The survivor generalises to
           five silently-skipped near-miss shapes. `DEC-060` gained an Amendment 2026-09-20
           (loose shape recognizer, strict honouring) as the round's authority.
           (2) The three-leg installer recipe unit is **committed `ad11e8429`** — reviewer
           returned nothing blocking on all six items; the Inspector re-verified all three
           legs itself and fixed one stale line reference in a comment. Staged as four
           explicit paths, with the findings hunk extracted so the in-flight B-STAGE9-36 row
           stayed out.
           (3) **B-STAGE9-39 is REPRODUCED** (no longer "suspected") — the Garmin-only path
           loses `encodings` when the host stdlib is absent, and ignores a deployed payload
           when it is present. `B-STAGE9-39-remedy-options` dispatched for the three-option
           scoring; a DEC is owed before any build.
           (4) **DEC-061 is decided and NOT built** — consume `ensureInitialized()`'s `Result`
           at `main.cpp:552`. Next builder unit after round 9. B-STAGE9-39's own fix touches
           the same region and stays a separate unit and commit (DEC-061 constraint 5).
           (5) B-STAGE9-28 stays CLOSED, frozen, ships in the same commit as -36.
           (6) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -40). There is no hold on
           `src/Core/main.cpp`; `insp_wake.sh:159` still hard-codes the released check.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`. Commit in the builder's idle gap — pre-commit stashes unstaged work.
           **Roster, verified live:** builder `garmin_builder_stage9_v20` (`w1:pM`, 79k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, 171k, nearest the bar), investigator
           `s925_tz_investigator` (`w1:pR`, 51k). `garmin_inspector_v1_40` retired at 204k,
           pane `w1:p27` and tab `w1:t1Z` closed (3→2 tabs).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_41`, written before self-
           succession at ~205k/210k. Sequencing only; per-id status lives in findings.md):
           Committed this pass: `ad11e8429` (the three-leg installer recipe unit).
           **All three agents were dispatched and working at handoff — collect, do not
           re-dispatch:**
           (1) `B-STAGE9-36-r9-review` on `garmin_codex_reviewer` (`/new`-refreshed from
           197k first, so it is fresh). Round 9's builder work is GREEN and Inspector-probed:
           all five near-miss shapes now raise, both alone and beside a valid bullet, 128
           tests pass, live CLI exit 0, real ledger still resolves DEC-054. **The Inspector
           found a NINTH instance of the same class before dispatching** — a 4-space, 8-space
           or tab-indented declaration beside a valid one still returns the valid globs
           silently, because `_ARMS_MARKER`'s `^ {0,3}` reintroduces indentation as a
           block-context boundary that DEC-060 round 9 retired. It is hunt item 1 in the
           reviewer's brief; expect round 10 to cover it plus whatever the reviewer adds.
           (2) `DEC-061-build` on `garmin_builder_stage9_v20` — consume the discarded
           `Result` at `main.cpp:552`. NOTE: the builder holds B-STAGE9-36 round 9's
           uncommitted work in the same tree and was told not to touch those two files.
           (3) `B-STAGE9-39-pyconfig-measure` on `s925_tz_investigator` — measures whether
           explicit `PyConfig.home`/`program_name` works with `PYTHONHOME` absent. **DEC-062
           is deliberately NOT yet recorded**: the options are scored (Option 1 leads) but
           the mechanism is unmeasured, and the measurement decides between Options 1 and 2.
           `WIKI.md` still reads `next:garmin-062`; verify before allocating.
           (4) B-STAGE9-39 is REPRODUCED, no longer suspected. B-STAGE9-28 stays CLOSED,
           frozen, ships in the same commit as -36.
           (5) Two premises corrected by measurement this pass, both worth not re-deriving:
           `PyProcessBootstrap` is NOT Qt-free, and CMake defines `GC_HAVE_PYTHON` where
           qmake defines `GC_WANT_PYTHON`, so the two build systems reach different
           initializers. Detail in findings.md's B-STAGE9-39 row.
           (6) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -40); `ledger_drift_lint.py`
           EXIT=0. There is no hold on `src/Core/main.cpp`; `insp_wake.sh:159` still
           hard-codes the released check, harmless.
           (7) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly. Cap builds
           at `-j4`. Commit only in the builder's idle gap — pre-commit stashes its unstaged
           work. `dispatch.py` enforces the line caps strictly; count before sending.
           **Roster, verified live:** builder `garmin_builder_stage9_v20` (`w1:pM`, 157k),
           reviewer `garmin_codex_reviewer` (`w1:pD`, fresh after `/new`), investigator
           `s925_tz_investigator` (`w1:pR`, 113k). `garmin_inspector_v1_40` retired, pane
           `w1:p27` and tab `w1:t1Z` closed (3→2 tabs).

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_42` — supersedes the two `v1_41` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) **DEC-062 is recorded and NOT built** — B-STAGE9-39's remedy. A Qt-side
           deployment locator feeds explicit `PyConfig.home`/`program_name` BEFORE
           `PyConfig_Read`; `PythonEmbed` consumes the same result instead of mutating
           `PYTHONHOME`, and `Py_SetProgramName` is retired. Measured, not reasoned:
           five legs, including a conflict leg proving explicit pre-`_Read` fields beat a
           hostile `PYTHONHOME`, and an `LD_PRELOAD` interceptor that never fired on
           `Py_SetProgramName`. DEC-061 lands first — it is what makes a failure here
           observable. `WIKI.md` now reads `next:garmin-063`.
           (2) **B-STAGE9-36 round 9 came back NOT-CLOSED, BLOCKING** — three mechanisms,
           all three re-confirmed by the Inspector's own probe, not relayed: `_ARMS_MARKER`'s
           `^ {0,3}` still silently skips a 4-space/8-space/tab-indented declaration
           (the ninth instance, already predicted); `_ARMS_DECLARATION`'s `\s+` HONOURS
           `- Arms<NBSP|tab|2sp>DEC-054:` as canonical, so a near-miss is accepted rather
           than raised; and the strict-before-loose ordering is what makes that reachable.
           Round 10 is owed and is the builder's unit after DEC-061.
           (3) Builder WARNED for soft landing at 217k — finish its current unit to a
           clean stop, start nothing new. It holds round 9's two files uncommitted; they must not ride in
           DEC-061's commit.
           (4) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -40); `ledger_drift_lint.py`
           EXIT=0. `ledger_drift_lint.py` lives at `scripts/`, not under
           `.claude/workflow-garminconnect/scripts/` — only `clv_findings.py` is there.
           (5) Export `SELF_PANE` when arming `insp_wake.sh`; invoke it directly, no args.
           Cap builds at `-j4`. Commit only in the builder's idle gap. `dispatch.py` caps
           an adhoc brief at 30 lines and rejects over it; count before sending. A
           `send_failed: timeout` against an already-working pane is the ack-wait, not a
           lost payload — read the pane before resending.
           **Roster, verified live:** builder `garmin_builder_stage9_v20` (`w1:pM`, 218k,
           warned), reviewer `garmin_codex_reviewer` (`w1:pD`, 72k, idle awaiting DEC-061's
           diff), investigator `s925_tz_investigator` (`w1:pR`, 129k, on
           `DEC-062-locator-contract`). `garmin_inspector_v1_41` retired at 212k, pane
           `w1:p28` and tab `w1:t10` closed (3→2 tabs, 5→4 panes).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_42`, written before self-
           succession at ~196k/210k. Sequencing only; per-id status lives in findings.md):
           Committed this pass: **`933126d0a`** — DEC-061's build (main.cpp:552 consumes the
           Result) plus this pass's governance. All pre-commit hooks passed; the builder's
           unstaged round-10 work was stashed and restored intact, verified after.
           **Two agents have UNCOLLECTED work — collect, do not re-dispatch:**
           (1) `B-STAGE9-41-options` on `s925_tz_investigator` (`w1:pR`) is DONE and was
           never collected. It answers whether `GC_HAVE_PYTHON` is a deliberate want-vs-have
           distinction or drift — that answer decides the repair, and a DEC is owed.
           (2) `B-STAGE9-36-r10-review` on `garmin_codex_reviewer` (`w1:pD`) was dispatched
           at 04:08. Round 10 is builder-GREEN with real mutation proofs; the builder itself
           flagged that the beside-a-valid-bullet leg of its 20 new tests can be satisfied by
           an unrelated duplicate-bullet-count check, so some may be green for the wrong
           reason. That is hunt item 5 in the reviewer's brief.
           (3) Round 10 is NOT yet Inspector-verified and NOT committed. It touches only the
           two `unittests/buildguard/` files.
           (4) **DEC-062 gained TWO amendments this pass, both from measurement that
           contradicted this Inspector.** The extraction surface is `PythonEmbed.cpp:85-210,
           245-269,339-345`, not the `:237-255` first written — it also validates the home by
           launching the interpreter, checks major/minor, does PATH discovery, and adds Linux
           deployed site-packages conditionally. And "stops mutating `PYTHONHOME`" is NOT
           "erases it": `:245-252` deliberately falls back to a user-supplied inherited value
           and that must survive.
           (5) **New finding B-STAGE9-41** — CMake selects Python sources on `GC_WANT_PYTHON`
           but defines only `GC_HAVE_PYTHON` (`src/CMakeLists.txt:1056-1086`), so a CMake
           Python-only build reaches neither initializer. It blocks DEC-062's same-home claim
           and must be sequenced before that build. qmake has no gap (`src/src.pro:332`).
           (6) `clv_findings.py` OUTSTANDING=5 (-36, -38, -39, -40, -41); `ledger_drift_lint.py`
           EXIT=0. Note `ledger_drift_lint.py` lives at `scripts/`, NOT under
           `.claude/workflow-garminconnect/scripts/` — only `clv_findings.py` is there.
           (7) Mechanics worth not re-deriving: `clang-format` is NOT on PATH — use
           `/home/andy/.cache/pre-commit/repolumxrndb/py_env-python3.13/bin/clang-format`. Its
           pre-commit regex covers `unittests/Core/garminconnect/.*` but NOT `src/Core/main.cpp`.
           A findings.md cell containing a literal `|` breaks the 6-column schema —
           `clv_findings.py` reports MALFORMED; rephrase rather than escape. `dispatch.py`
           caps briefs at 50 lines (builder), 40 (reviewer), 30 (adhoc) and rejects over.
           A `send_failed: timeout` against an already-working pane is the ack-wait, not a
           lost payload — read the pane before resending.
           (8) **The 5h plan window is ACCOUNT-WIDE and hit 99% at 04:10**, on the
           Inspector's own fresh pane as well as the builder's. Expect a rate-limit fallback
           or stall on the next long unit; check each pane's model line before trusting a
           context reading (a fallback rotates the transcript and reads as `unknown`).
           (9) `garmin_codex_reviewer` went `done` on `B-STAGE9-36-r10-review` at ~04:10 —
           that verdict is waiting and uncollected, alongside item (1).
           **Roster, verified live:** builder `garmin_builder_stage9_v21` (`w1:pM`, 135k,
           restarted fresh this pass after v20 soft-landed at 221k), reviewer
           `garmin_codex_reviewer` (`w1:pD`, 127k, working), investigator
           `s925_tz_investigator` (`w1:pR`, 93k, `/new`-refreshed from 213k this pass).
           `garmin_inspector_v1_41` retired, pane `w1:p28` and tab `w1:t10` closed.

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_43` — supersedes the two `v1_42` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) Both of `v1_42`'s uncollected results are COLLECTED; neither was re-dispatched.
           (2) **B-STAGE9-36 round 10 is NOT committable.** The reviewer filed it blocking on
           non-ASCII indentation, and the Inspector's own probe widened it: `_ARMS_MARKER`
           (`unittests/buildguard/garmin_lint_ownership_guard.py:1017`) accepts only `[ \t]`, so
           SIX whitespace characters — U+00A0, U+202F, U+2003, U+1680, U+000C, U+000B — silently
           skip an Arms bullet, returning None alone and a neighbour's globs beside a valid
           bullet. Same false-PASS shape round 10 closed for ASCII. Round 11 dispatched with one
           directive: the loose recognizer's indent class must be Unicode-maximal, not an
           enumerated list, proven by a codepoint SWEEP rather than a hand-written list — a list
           of six repeats the defect and earns a round 12. Probe kept at
           `/tmp/insp-exchange/probe_arms_unicode_indent.py`.
           (3) **DEC-063 recorded, NOT built** — B-STAGE9-41's remedy. CMake defines
           `GC_WANT_PYTHON` target-locally and `GC_HAVE_PYTHON` is deleted, not aliased. The
           Inspector re-derived the census instead of relaying it, which surfaced a precedent the
           option report missed: `src/CMakeLists.txt:1508` already does
           `target_compile_definitions(GoldenCheetah PRIVATE GC_WANT_GARMINCONNECT)`, so the
           Python block is the only one deviating in both spelling and scope. `WIKI.md` now reads
           `next:garmin-064`. Its constraint 4 carries a real unmeasured risk — a CMake Python-on
           build has never been run — now dispatched to the investigator as its own measurement.
           (4) **Ledger drift corrected:** B-STAGE9-40's findings row still read `open` although
           its fix landed in `933126d0a`. Flipped to fixed with the residual named, not buried:
           `main.cpp` links into no CTest target, so the `qDebug()` emission is source-verified
           only, and the app-process smoke stays owed under B-STAGE9-39.
           (5) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -41), down from 5 by (4) alone;
           `ledger_drift_lint.py` EXIT=0. Nothing committed this pass — all three agents hold
           uncommitted or unbuilt work.
           (6) The anti-duplication hook still misreads a `2>/dev/null` redirect as a new file
           and blocks the builder on it (ORCH-062/LSN-036, unfixed upstream). Expect to clear it
           by hand each mutation round.
           (7) **Round 11 is builder-GREEN, 200/200, awaiting reviewer verdict — not committed.**
           `_ARMS_MARKER` went to `\s*`, and the builder did the thing the brief asked for
           instead of the cheap version: it swept all of 0x0-0x10FFFF to confirm regex `\s` and
           `str.isspace()` actually agree on this interpreter, then derived the 58 sweep cases
           from `str.isspace()` so the two are cross-checked rather than assumed. Mutation RED
           exactly on the 27 non-ASCII members, GREEN on space and tab. Report at
           `/tmp/insp-exchange/B-STAGE9-36-r11.md`.
           (8) **B-STAGE9-38's remaining scope is now exact, and it is two mechanisms, not the
           whole finding** — see its findings.md row. Twelve of DEC-058's sixteen constraints are
           discharged. What is left: `src/src.pro:275`'s absolute `$$PWD` path still PREPENDED at
           `PyEmbeddedAdapter.cpp:554` (so source beats the installed package and a missing
           payload is false-green), and `appveyor.yml:255`'s `--version` smoke that exits before
           any adapter import. Dispatched as `B-STAGE9-38-u3`. The reviewer corrected one of this
           Inspector's premises in the process: `GC_GARMIN_PYPATH` is no longer the masking path.
           (9) Next after the reviewer's round-11 verdict: commit round 11 if CLOSED, else round
           12. DEC-063's build stays BLOCKED until the investigator's CMake Python-on measurement
           lands — do not dispatch it into unmeasured compile errors.
           **Roster, verified live:** builder `garmin_builder_stage9_v21` (`w1:pM`, 169k, on
           `B-STAGE9-38-u3`), reviewer `garmin_codex_reviewer` (`w1:pD`, `/new`-refreshed from
           205k this pass, on `B-STAGE9-36-r11-review`), investigator `s925_tz_investigator`
           (`w1:pR`, 100k, on `DEC-063-cmake-python-on`). `garmin_inspector_v1_42` retired at
           203k, pane `w1:p29` and tab `w1:t21` closed (3→2 tabs, 5→4 panes).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_43`, written before self-
           succession at 211k/210k. Sequencing only; per-id status lives in findings.md):
           (1) **Nothing committed by this Inspector. Three uncommitted workstreams sit in the
           tree at once and must NOT be mixed into one commit:** the two
           `unittests/buildguard/` lint-ownership files (B-STAGE9-36 rounds 10+11),
           `src/Cloud/PyEmbeddedAdapter.cpp` + `unittests/Core/garminconnect/
           testGarminConnectPyAdapter.cpp` (B-STAGE9-38-u3 DO-1), and this pass's governance.
           (2) **B-STAGE9-36 round 11 reviewed NOT-CLOSED; round 12 dispatched.** Round 12's
           rule is the closed one: the loose recognizer's leading run is defined by Unicode
           general CATEGORY (Zs/Zl/Zp/Cc/Cf via `unicodedata.category`), not `\s`, not a list.
           Measured basis: that set is a strict superset of `str.isspace()` (254 vs 29, nothing
           lost) and covers the Cf format controls `\s` misses. Probe at
           `/tmp/insp-exchange/probe_invisible_prefix_classes.py`.
           (3) **DEC-060 gained a round-11 amendment** retiring its own three-space indentation
           cap. That cap contradicted the decision's own doctrine and was silently outrun by
           rounds 10/11. The reviewer raised the conflict as blocking, was asked to adjudicate
           it, argued both sides and WITHDREW its own finding — the independent second opinion
           the skill requires before a DEC bends a standing rule. Rounds 9-11 need no
           reclassification.
           (4) **B-STAGE9-38-u3 is HALF done.** DO-1 (new `importAdapterModule()`: plain import
           first, prepend-and-retry only on failure, six call sites) is builder-GREEN 51/51 with
           a real mutation proof, and is under reviewer delta-check as `B-STAGE9-38-u3-review`.
           **DO-2 (the `appveyor.yml:255` extracted-bundle import smoke) is entirely NOT
           STARTED** — the builder soft-landed before reaching it. That is the next builder unit
           after round 12.
           (5) The central question on DO-1 is still open and is the reviewer's to answer: the
           fallback still succeeds wherever the source tree exists, so verify the masking moved
           rather than merely relocated before accepting it.
           (6) **DEC-063's build stays BLOCKED** pending `s925_tz_investigator`'s
           `DEC-063-cmake-python-on` measurement — a CMake Python-on build has never been run
           and may surface pre-existing errors. Do not dispatch that build until it lands.
           (7) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -41); `ledger_drift_lint.py`
           EXIT=0. Two schema breaks were made and repaired this pass: text appended PAST a
           row's trailing `|`, and a literal `|` inside a cell (`Zs|Zl|...`). Rephrase, never
           escape; re-run `clv_findings.py` after every findings.md edit.
           (8) `dispatch.py` line caps bite constantly — builder 50, reviewer 40, adhoc 30.
           Count before sending. A reply can come back under the WARN unit's sentinel rather
           than the work unit's, so a `no_reply` may just mean you collected the wrong unit id.
           **Roster, verified live:** builder `garmin_builder_stage9_v22` (`w1:pM`, fresh 0k —
           v21 soft-landed at 223k and was relaunched with `--permission-mode auto --model
           sonnet`, status line confirmed) on `B-STAGE9-36-r12`; reviewer `garmin_codex_reviewer`
           (`w1:pD`, `/new`-refreshed from 205k this pass) on `B-STAGE9-38-u3-review`;
           investigator `s925_tz_investigator` (`w1:pR`, 132k) on `DEC-063-cmake-python-on`.
           (9) **THREE UNCOLLECTED RESULTS — collect all three first, do not re-dispatch.**
           All settled at 08:20-08:22, after this cursor's roster line was written, so the
           roster above reads as working and is stale on this point only:
           - `dispatch.py --mode collect --target w1:pD --unit B-STAGE9-38-u3-review`
             (reviewer). Its central question is item (5) above.
           - `dispatch.py --mode collect --target garmin_builder_stage9_v22 --unit
             B-STAGE9-36-r12` (builder, round 12).
           - `dispatch.py --mode collect --target s925_tz_investigator --unit
             DEC-063-cmake-python-on` (investigator). This is what unblocks item (6).
           A reply can land under a WARN unit's sentinel instead of the work unit's, so a
           `no_reply` may just mean the wrong unit id — read the pane before re-sending.
           Flagged deliberately: an uncollected verdict is how the previous two handoffs each
           lost a day.

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_44` — supersedes the two `v1_43` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) Succession complete. `garmin_inspector_v1_43` retired at 221k; pane `w1:p2A`
           and tab `w1:t22` closed (3→2 tabs, 5→4 panes).
           (2) **`v1_43`'s uncollected result is COLLECTED.** `B-STAGE9-38-u3-review` is
           NOT-CLOSED with two blocking mechanisms — the masking relocated instead of moving,
           and `GC_GARMIN_PYPATH` lost the first-import precedence DEC-058 constraint 5 gives
           it. Mechanisms and the repair shape are in the B-STAGE9-38 findings row. The repair
           is not a new decision; it is constraints 5 and 13 as already written.
           (3) **DEC-063's build is UNBLOCKED.** `DEC-063-cmake-python-on` measured a CMake
           Python-on configure plus an 8-TU guarded compile sweep on both legs: zero
           pre-existing errors, zero newly exposed. Two residuals are declared in the
           B-STAGE9-41 row; the unmeasured link is covered by the builder's own build, so it
           needs no separate pass.
           (4) **B-STAGE9-36 round 12 is builder-done but its reply TRUNCATED on collect.**
           Re-prompted for a verbatim spill at `/tmp/insp-exchange/B-STAGE9-36-r12.md` as unit
           `B-STAGE9-36-r12-spill`. Collect that before judging round 12.
           (5) Builder queue, in order, one unit per turn: round 12 verdict → commit or round
           13 → `B-STAGE9-38-u3-r2` (the constraint 5/13 repair, plus the vacuous precedence
           assertion the reviewer filed non-blocking) → `B-STAGE9-38-u3` DO-2 → DEC-063.
           (6) Nothing committed this pass. Three uncommitted workstreams still sit in the
           tree separately: the two `unittests/buildguard/` lint-ownership files, the
           `PyEmbeddedAdapter.cpp` + `testGarminConnectPyAdapter.cpp` DO-1 pair (now known
           NOT committable), and this pass's governance.
           (7) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -41) over 441 OK, MISSING-EFFECT=0;
           `ledger_drift_lint.py` EXIT=0.
           **Roster, verified live:** builder `garmin_builder_stage9_v22` (`w1:pM`, 87k) on
           `B-STAGE9-36-r12-spill`; reviewer `garmin_codex_reviewer` (`w1:pD`, 131k) on
           `B-STAGE9-38-u3-r2-scope`; investigator `s925_tz_investigator` (`w1:pR`, 151k) on
           `B-STAGE9-38-DO2-measure`. Inspector `garmin_inspector_v1_44` (`w1:p2B`, Opus, auto).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_44`, written before self-
           succession. Sequencing only; per-id status lives in findings.md / decisions.md):
           (1) **B-STAGE9-36 stopped being a recognizer bug this pass.** Round 12 was
           builder-GREEN 712/712 and reviewed NOT-CLOSED on two live bypasses; the inversion
           the Inspector proposed for round 13 was then adjudicated and REJECTED on a concrete
           counterexample. Rounds 9-12 each pinned one character class and each lost to the
           next. No round 13, and no fifth character class.
           (2) **DEC-064 recorded PENDING, deliberately not accepted.** It supersedes DEC-060
           for one field, so the drafting agent's own recommendation cannot carry it. Options
           A/B/C are in the entry; `s920_arms_second_opinion` (`w1:p2C`, tab `w1:t24`, fresh
           Codex) holds the independent second opinion as unit `B-STAGE9-36-second-opinion`.
           **Collect it, then accept or reject — do not dispatch a builder before that.**
           `WIKI.md` now reads `next:garmin-065`.
           (3) `B-STAGE9-38-u3-r2` is the builder's live unit — the constraint 5/13 repair.
           The reviewer's scope pass corrected the Inspector's premise before it was built:
           there are TWO production construction sites, and `AddCloudWizard.cpp:184` reads the
           macro unguarded while `GarminConnect.cpp:218` guards it, so define-deletion and
           refactor are one change or neither. No CTest goes RED from deleting the fallback —
           which is itself the defect: the 57 stub-path constructor arguments pass only
           because `setScenario()` preinserts the pystub.
           (4) `B-STAGE9-38-DO2-measure` is in flight. Already surfaced: `appveyor/linux/
           after_build.sh:5`'s pinned `PYTHON_APPIMAGE_VERSION=3.11.14` returns 404 upstream.
           If the report confirms it, that is a separate CI defect and owes its own finding id
           — it is not part of DO-2.
           (5) Nothing committed. Four uncommitted workstreams, still not to be mixed: the two
           `unittests/buildguard/` files (rounds 10-12, now blocked behind DEC-064), the
           `PyEmbeddedAdapter.cpp` + `testGarminConnectPyAdapter.cpp` pair (being rewritten by
           u3-r2), this pass's governance, and the unrelated Coach/Qt6.8 work.
           (6) `clv_findings.py` OUTSTANDING=4 (-36, -38, -39, -41), MALFORMED=0,
           MISSING-EFFECT=0; `ledger_drift_lint.py` EXIT=0 after every edit.
           (7) The investigator blocks on a per-command approval roughly every two minutes
           while it works in `/tmp/insp-exchange/bstage9-38-do2`. Approve the one-time option
           only; the offered "don't ask again" scopes are wider than the visible command.
           **Roster, verified live:** builder `garmin_builder_stage9_v22` (`w1:pM`, 158k) on
           `B-STAGE9-38-u3-r2`; reviewer `garmin_codex_reviewer` (`w1:pD`, 63k) IDLE with no
           unit — give it one; investigator `s925_tz_investigator` (`w1:pR`, 172k) on
           `B-STAGE9-38-DO2-measure`; `s920_arms_second_opinion` (`w1:p2C`) on the DEC-064
           second opinion, and it is short-lived — retire its pane and tab once collected.

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_45` — supersedes the two `v1_44` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) Succession complete. `garmin_inspector_v1_44` retired at 192k; pane `w1:p2B`
           and tab `w1:t23` closed. `s920_arms_second_opinion` collected and retired with it
           (`w1:p2C`, tab `w1:t24`). 4→2 tabs, 6→4 panes.
           (2) **DEC-064 is ACCEPTED — Option C.** The second opinion dissented (repair the
           recognizer via a Default_Ignorable strip); the reviewer adjudicated NARROW and
           recommended Option C, but that pane is DEC-064's own drafting agent, so neither
           could carry the decision. The Inspector settled it by calling
           `_is_arms_loose_shaped` directly at HEAD on eight inputs — the table is in the DEC
           entry. Greek Alpha `- Αrms DEC-054:` is skipped under BOTH a DI strip and NFKC,
           and DEC-060's own `Lo`/`So` fillers are not default-ignorable, so the recognizer
           class is not closable. Three binding constraints are in the entry; the sentinel
           REPLACES `decisions.md:2646` and the two loose-recognizer helpers are deleted.
           (3) **The builder SOFT-LANDED at 250k on `B-STAGE9-38-u3-r2`; its report is
           COLLECTED and transcribed here. This is the resume point — no file survives.**
           NOT GREEN, and it stopped rather than forcing it. Both production sites are DONE
           and compiled clean in their own targets: `AddCloudWizard.cpp:184` (was unguarded)
           and `GarminConnect.cpp:218` (was guarded) now both build a `GarminPyModulePath`
           from `GC_GARMIN_PYPATH` only; the `GARMIN_PY_MODULE_DIR` DEFINES are deleted from
           `src/src.pro`, `src/CMakeLists.txt` and all 7 lines in
           `unittests/Core/garminconnect/CMakeLists.txt` (grep-confirmed 0 remain).
           `PyEmbeddedAdapter.{h,cpp}` complete; the no-override branch is one plain import,
           fallback deleted. 50 + 5 + 2 ctor call sites rewrapped across
           `testGarminConnectPyAdapter.cpp`, `testPyProcessBootstrap.cpp` and
           `testGarminConnectPasswordPersistence.cpp`; two new tests written
           (`explicitOverrideWinsOverAnAlreadyImportableModule` = DO-6,
           `noOverrideNeverTouchesSysPathWhenPackageIsAbsent` = DO-2 proof).
           BROKEN, diagnosed, NOT fixed: the 25-target batch failed at step 112/214,
           `stubs/ImportSeamStubs.cpp:625`, `'constexpr GarminPyModulePath::
           GarminPyModulePath()' is private within this context` — see B-STAGE9-43.
           The fix the builder identified but did not apply: move
           `GarminPyModulePath() = default;` from `private:` to `public:` in all three
           declaring files (`PyEmbeddedAdapter.h`, `WizardStubPreamble.h`,
           `ReadFileStubPreamble.h`), keeping the 1-arg ctor private behind
           `explicitOverride()`.
           **UNVERIFIED, carry forward:** that fix itself (diagnosed only);
           `ProviderSeamStubs.cpp:720` never attempted; ~12 targets never reached after the
           failure point; and **zero `ctest` runs this entire unit** — everything to date is
           compile-only, so both new tests' runtime behaviour is completely unexercised.
           (3b) **B-STAGE9-43 opened** from that report: four independent hand-written
           `PyEmbeddedAdapter` fakes must each track any ctor-signature change or ~20 targets
           stop compiling. Non-blocking, but it is why a change that built clean in four
           standalone targets still broke the batch.
           (3a) `B-STAGE9-36-optionC-scope` is COLLECTED and it corrected the shape before a
           build round paid for it. Three cites in DEC-064 were wrong and are fixed in the
           entry (declaration is `:2647`, not `:2646`; slot is `:2640`; the `- Status:`
           example is `:2641`) — the Inspector re-verified all four at the origin rather than
           relaying them. Four new binding constraints added: `ArmsBulletMalformed` is
           RETAINED, the sentinel is raw `entry[1]` regardless of `masked=True`, the
           `armed is None` branch must be deleted not left dead, and the old bullet is removed
           in the same change. Minimal change set is three files.
           (4) **`B-STAGE9-38-DO2-measure` is COLLECTED and fully MEASURED.** The version-
           independent `squashfs-root/usr/bin/python3` import smoke is RED without the adapter
           (exit 1) and GREEN after its pip step (exit 0); relocation preserves bundled
           site-packages. The exact replacement lines for `appveyor.yml:255-259` are in the
           reply and owe transcription into the B-STAGE9-38 row when its unit resumes.
           (5) **`B-STAGE9-42` opened and CONFIRMED live.** `appveyor/linux/after_build.sh:5`'s
           `PYTHON_APPIMAGE_VERSION=3.11.14` returns HTTP 404, measured. Blast radius measured
           too and it is ONE: every other version-pinned CI asset HEADs 200. Two FTDI pins 403
           on HEAD and are deliberately NOT classified — HEAD alone cannot separate a dead
           asset from a hotlink block. Measured replacement (resolve from the release tag's
           API listing, not a literal filename) is in the finding row. Not folded into
           B-STAGE9-38; it needs its own unit and no CTest covers `appveyor/`.
           (5a) **`DEC-063-scope` is COLLECTED and it stopped a build break.** Verdict SAFE,
           with two BLOCKING corrections now binding in the DEC entry: the `GoldenCheetah`
           target does NOT exist at `src/CMakeLists.txt:1085`, so rewriting `add_definitions()`
           in place would fail CMake CONFIGURE — it is DELETE `:1085` plus ADD after
           `add_executable()` at `:1398`, two edits; and the Inspector's "whole tree" acceptance
           wording is wrong, since historical ledger prose (including DEC-063 itself) contains
           the string — scope the check to live build inputs, where the count is exactly one
           with zero readers. Ordering settled: DEC-063 precedes DEC-062, independent of
           `B-STAGE9-38-u3`. Minimal change set is one file.
           (5b) **B-STAGE9-42's fix direction is DECIDED: PIN-TO-A-LIVE-VERSION, not the API
           resolver.** `B-STAGE9-42-fix-scope` rejected the investigator's own measured
           replacement on three blocking grounds — curl/jq are never provisioned by
           `appveyor/linux/install.sh:4-54`; the tag rotates its patch, so identical revisions
           would fetch different interpreters unreviewed; and without `pipefail` a rate-limit
           body yields an empty URL that fails opaquely. Decided repair: pin 3.11.16 plus a
           hard-coded SHA-256 checked after download and before extraction. Full reasoning in
           the finding row.
           (6) **Builder queue, in order, one unit per turn:** `B-STAGE9-38-u3-r3` (LIVE now)
           → B-STAGE9-36 Option C (fully scoped, DEC-064 constraints 1-7 are binding) →
           DEC-063 (fully scoped, two edits in `src/CMakeLists.txt`, must precede DEC-062) →
           B-STAGE9-42 (pin + SHA-256) → DEC-062.
           (6a) **Everything in that queue except the live unit is already scope-passed.** Do
           not re-scope them; dispatch and build.
           (7) Nothing committed. Four uncommitted workstreams still unmixed: the two
           `unittests/buildguard/` files, the `PyEmbeddedAdapter.cpp` +
           `testGarminConnectPyAdapter.cpp` pair, governance, and the unrelated Coach/Qt6.8
           work.
           (8) The investigator blocks on a per-command approval every few minutes while it
           works under `/tmp/insp-exchange/`. One-time option only — the offered "don't ask
           again" scopes are wider than the visible command.
           (9) Live gate markers moved since the 2026-09-15 cursor and are NOT yet explained:
           `~/.goldencheetah/Andy/activities/` is 1146 files (was 1145) and `tokens.json` is
           2189 bytes dated 2026-09-19 18:49 (was 0 bytes, 2026-09-13). Newest activity is
           still dated 2026-09-13. Establish what wrote that token blob before reading the
           live gate as passed or failed.
           **Roster, verified live:** builder **`garmin_builder_stage9_v23`** (`w1:pM`, fresh
           at 0k — v22 retired at 250k, relaunched with `--permission-mode auto --model
           sonnet`, both confirmed on the new status line) on `B-STAGE9-38-u3-r3`; reviewer
           `garmin_codex_reviewer` (`w1:pD`, 149k) idle after `B-STAGE9-36-optionC-scope` —
           give it a unit; investigator `s925_tz_investigator` (`w1:pR`, 198k) on
           `B-STAGE9-42-ci-pin-sweep`. Inspector `garmin_inspector_v1_45` (`w1:p2D`, tab
           `w1:t25`, Opus, auto, 142k of 210k).

STAGE-9-CURSOR-ADDENDUM (2026-09-20, `garmin_inspector_v1_45`, written before self-
           succession. Sequencing only; per-id status lives in findings.md / decisions.md):
           (1) **DEC-064 ACCEPTED (Option C) is this pass's one real decision.** Neither
           supervised agent could carry it — the second opinion dissented, and the adjudicating
           reviewer was DEC-064's own drafting agent. It was settled by running
           `_is_arms_loose_shaped` at HEAD on eight inputs; the table is in the DEC entry.
           Greek Alpha `- Αrms DEC-054:` survives both a DI strip and NFKC, which ends the
           recognizer line. Constraints 1-7 are binding and three of its cites were corrected
           at the origin (`:2647`, not `:2646`).
           (2) **Four scope passes ran before any builder touched their code, and every one
           found a real error in the Inspector's own premise.** That is the pattern worth
           keeping, not a coincidence: `B-STAGE9-36-optionC-scope` (three wrong cites,
           `ArmsBulletMalformed` nearly deleted), `DEC-063-scope` (a CMake CONFIGURE break —
           the target does not exist at `:1085`), `B-STAGE9-42-fix-scope` (rejected the
           API resolver outright). Scope-pass before dispatch, every time.
           (3) The builder soft-landed at 250k and was relaunched as
           `garmin_builder_stage9_v23`; its predecessor's mid-flight state is transcribed in
           item (3) of the cursor above and nowhere else — the spill buffer was deleted.
           **Zero ctest runs have happened on `B-STAGE9-38-u3` across its entire life.**
           (4) Reviewer `garmin_codex_reviewer` (`w1:pD`) is at 211k and idle. **It needs a
           `/new` before its next unit** — Codex reset keeps the process, pane and name.
           Investigator was already reset this pass and is at 39k.
           (5) `STAGE9-live-gate-forensics` is IN FLIGHT on the investigator and is the only
           unit touching the human-in-the-loop gate. It answers what wrote the 2189-byte
           `tokens.json` on 2026-09-19. Do not report the live gate as passed or failed until
           it lands.
           (6) Nothing committed, by design. Five uncommitted workstreams, still not to be
           mixed: the two `unittests/buildguard/` files, the `B-STAGE9-38-u3` production+test
           set, this pass's governance, the unrelated Coach/Qt6.8 work, and nothing else.
           (7) `clv_findings.py` OUTSTANDING=5 (-36, -38, -39, -41, -42), MALFORMED=0,
           MISSING-EFFECT=0; `ledger_drift_lint.py` EXIT=0 after every edit.
           **Roster, verified live:** builder `garmin_builder_stage9_v23` (`w1:pM`, 114k) on
           `B-STAGE9-38-u3-r3`; reviewer `garmin_codex_reviewer` (`w1:pD`, 211k) IDLE, needs
           `/new` then a unit; investigator `s925_tz_investigator` (`w1:pR`, 39k) on
           `STAGE9-live-gate-forensics`. Successor `garmin_inspector_v1_46` (`w1:p2E`, tab
           `w1:t26`, Opus, auto mode confirmed) is live; `garmin_inspector_v1_45` (`w1:p2D`,
           tab `w1:t25`) retired at ~190k and is the successor's to close.

Detail lives in: traceability.md (per-id spine) · findings.md (finding disposition;
archive/findings-detail.md for any row whose cell was capped this pass) · decisions.md
(## Decision index, then entries) · validations/archive/ + cycles/archive/ (historical
execution evidence) · archive/state-history.md (§ 1-10 pre-2026-09-06 history, § 11 = the
complete pre-compaction STATE.md this draft replaces). workflow-aicoach/ is a retired ledger
(provenance only — see .claude/workflow-INDEX.md).

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_46` — supersedes the two `v1_45` blocks
           above for SEQUENCING. Per-id status lives only in findings.md / decisions.md):
           (1) Succession complete. `garmin_inspector_v1_45` retired at 198k; pane `w1:p2D`
           and tab `w1:t25` closed. 3->2 tabs, 5->4 panes.
           (2) **`B-STAGE9-38-u3-r3` reported GREEN and its report is COLLECTED.** The
           compile break is fixed at `src/Cloud/PyEmbeddedAdapter.h:45` — the defaulted
           `GarminPyModulePath()` ctor moved `private:`->`public:`, the 1-arg override ctor
           stayed private; the same edit was mirrored into `stubs/ReadFileStubPreamble.h` and
           `stubs/WizardStubPreamble.h`. `stubs/ProviderSeamStubs.cpp:720` needed no source
           change of its own — the header fix covers it. Builder-measured: 37 garminconnect
           targets, 231/231 steps exit 0, `grep -c error:` 0; first ctest of the unit 4/4 on
           `testGarminConnectPyAdapter|testPyProcessBootstrap|
           testGarminConnectPasswordPersistence|testGarminConnectWizardRouting`; both
           mutate/revert cycles flipped only their own named case and reverted byte-clean.
           NOT independently re-verified yet and NOT reviewed yet — see (3).
           (3) **`B-STAGE9-38-u3-r3-review` is LIVE** on `garmin_codex_reviewer`, which was
           `/new`-ed first (211k -> 0). Scope is `git diff -- src/Cloud
           unittests/Core/garminconnect src/src.pro src/CMakeLists.txt`, ~430 lines, every
           hunk in scope. Its central question is whether any path survives by which
           `garmin_client` resolves from a directory `GC_GARMIN_PYPATH` did not name. Five
           hazards named, two of them the Inspector's own reads of the diff: the new comment
           block at `PyEmbeddedAdapter.h:34-44` asserts checkable behaviour ("collapse to the
           same value", "exactly one plain import is attempted") against the project's own
           no-checkable-comment rule; and the now-public defaulted ctor may let production
           code build a path state outside `none()`/`explicitOverride()`.
           (4) **`B-STAGE9-36-optionC-r1` is LIVE** on the builder, per DEC-064's binding
           constraints 1-7 and its adopted minimal change set (decisions.md +
           `unittests/buildguard/garmin_lint_ownership_guard.py` + its test file). DEC-064
           constraint 1's four cites were re-verified at the origin before dispatch and all
           four hold: heading 2639, sentinel slot 2640, `- Status:` 2641, the one live
           `- Arms DEC-054:` 2647 with `- Arms-note:` 2648.
           (5) Queue after these two, in order, all already scope-passed — do not re-scope:
           DEC-063 (two edits in `src/CMakeLists.txt`, must precede DEC-062) -> B-STAGE9-42
           (pin 3.11.16 + hard-coded SHA-256, per `B-STAGE9-42-fix-scope`) -> DEC-062.
           (6) **DEBT: the exact `appveyor.yml:255-259` replacement lines measured by
           `B-STAGE9-38-DO2-measure` were never transcribed into the B-STAGE9-38 row.** The
           reply lived in a retired session; recover them from the investigator rollout
           before B-STAGE9-38 closes, or re-measure. Do not close the row on memory of them.
           (7) Nothing committed. Five uncommitted workstreams still unmixed.
           (8) The live gate stays unread either way: `STAGE9-live-gate-forensics` is the only
           unit touching it, and the 2189-byte `tokens.json` dated 2026-09-19 is still
           unexplained. Nobody may call the live-account criterion passed or failed until it
           lands.
           **Roster, verified live:** builder `garmin_builder_stage9_v23` (`w1:pM`, 116k) on
           `B-STAGE9-36-optionC-r1`; reviewer `garmin_codex_reviewer` (`w1:pD`, 0k post-`/new`)
           on `B-STAGE9-38-u3-r3-review`; investigator `s925_tz_investigator` (`w1:pR`, 69k)
           still working. Inspector `garmin_inspector_v1_46` (`w1:p2E`, tab `w1:t26`, Opus,
           auto, 55k of 210k).

STAGE-9-CURSOR-AMENDMENT (2026-09-20, `garmin_inspector_v1_46`, amends items (6) and (8)
           of the `v1_46` block above; sequencing only):
           (a) **Item (8) is CLOSED and its premise was wrong.** `STAGE9-live-gate-forensics`
           came back EVIDENCE-OF-SYNC and independently corroborates this file's own
           STAGE-9-LIVE-RUN-7 block: the 2189-byte `tokens.json` dated 2026-09-19 18:49:27 is
           the reconnect leg's own recreated token, already recorded there, and the 1146th
           activity's 2026_09_13 filename encodes activity date, not import time (real mtime
           18:38:31, with a matching Garmin import sidecar). Nothing was unexplained; two
           successive cursors carried the marker as open without reading run 7's block.
           (b) **The real open question is PROVENANCE, not the markers.** Run 7's evidence
           says a real account synced; it does not say which BUILD SYSTEM produced the binary
           that did it, and Stage 9's criterion is an installer. `STAGE9-live-run7-provenance`
           is LIVE on the investigator to settle INSTALLER vs DEV-BUILD and whether run 7's
           evidence must be re-earned once B-STAGE9-38 lands.
           (c) The builder hit the Claude Pro usage limit at 09:26 and auto-resumed at 12:50;
           `B-STAGE9-36-optionC-r1` is running. Its pane came back in **manual mode**, not
           auto — resolve the first permission dialog by selecting that dialog's own
           switch-to-auto option; do not send shift+tab into a working pane.
           (d) STATE.md is in cursor BREACH: the live cursor belongs in the head block, and
           this file's body is archive. Librarian Job-3 COMPACTION is owed at the next seam
           with no builder round in flight.

STAGE-9-CURSOR-AMENDMENT-2 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (e) **`B-STAGE9-38-u3-r3-review` came back BLOCKING — the builder's GREEN does
           not hold.** Three blocking findings filed, B-STAGE9-44/-45/-46: first-import
           precedence is not actually guaranteed on the explicit-override path, and neither
           route is covered by the two new tests. B-STAGE9-47 (non-blocking) records the
           unbacked checkable comment claims. The unit is NOT closable and nothing about it
           may be committed until these clear.
           (f) The reviewer's own literal correction is binding and narrows the repair: with
           NO override, resolving from CPython's existing `sys.path` is INTENDED under
           DEC-058, not a residual module-path input. Only the explicit-override path is
           defective. The Inspector's review brief framed this too widely.
           (g) `B-STAGE9-44-46-repair-scope` is LIVE on the reviewer, scoping the repair
           before a build round pays for it. Its open design question is B-STAGE9-45: evict
           `gc_garmin_adapter` from `sys.modules` under an explicit override, versus fail
           closed on an already-cached module. That is an in-scope technical call the
           Inspector settles on the scope pass's answer — not a user gate.
           (h) Builder pane restored to **auto mode** by selecting the permission dialog's
           own switch-to-auto option; `B-STAGE9-36-optionC-r1` still running, so the u3
           repair round queues behind it — one unit per builder turn.
           (i) Repair-round state for u3, for the next reviewer dispatch: stated class NONE,
           consecutive-same count 0. Rounds r1-r3 were builder compile rounds, not reviewer
           NOT-CLOSED rounds, and do not count against the step-5 bound.

STAGE-9-CURSOR-AMENDMENT-3 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (j) **The u3 repair is SETTLED and recorded as DEC-058 constraint 17** — the
           scope pass refuted the Inspector's own two-option framing of B-STAGE9-45 and
           supplied a third, correct shape: origin-validated reuse of the `sys.modules`
           cache, neither eviction nor blanket rejection. sys.path gets first-exact-
           occurrence-to-index-0 with no path canonicalization, and fail-closed on any
           failed list op. Read constraint 17, not this line, before dispatching the repair.
           That is the fifth consecutive scope pass to find a real error in a premise.
           (k) Builder WARNED at 219k and is landing softly on `B-STAGE9-36-optionC-r1`; it
           acknowledged mid-edit and will report rather than force GREEN. Expect a partial
           report, not a verdict. Its `dispatch.py --mode send` returned `send_failed`
           purely because the pane was already `working` so no ack transition fired — the
           payload landed; do not re-send on that signal alone, read the pane.
           (l) Builder queue, in order, one unit per turn: whatever `B-STAGE9-36-optionC-r1`
           leaves unfinished -> **B-STAGE9-38 u3 repair under DEC-058 c17** (B-STAGE9-44/-45
           /-46 are its acceptance) -> DEC-063 -> B-STAGE9-42 -> DEC-062.
           (m) `DEC-062-scope` is LIVE on the reviewer — the queue tail is the one item never
           scope-passed. It also asks whether DEC-062 conflicts with u3's own edits to
           `src/Cloud/PyEmbeddedAdapter.cpp` and the interpreter startup path.

STAGE-9-CURSOR-AMENDMENT-4 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (n) **Stage 9's live-account criterion is further from met than any prior cursor
           said, and B-STAGE9-48 records why.** `STAGE9-live-run7-provenance` returned
           DEV-BUILD-PROVENANCE: run 7 came from a developer-tree binary, not an installer.
           No AppImage or packaged artifact exists on this host at all. Both registered
           launch routes resolve to `build/src/GoldenCheetah` (CMake/Ninja). A second,
           previously unrecorded qmake binary exists at `src/GoldenCheetah` (mtime
           2026-09-19 18:32:53, built with `GARMIN_PY_MODULE_DIR`), but it too is a direct
           source-tree output, not installer provenance — and which of the two actually ran
           is CANNOT-DETERMINE, because the log records no executable path or build id.
           **Run 7's evidence must be RE-EARNED against a post-B-STAGE9-38 installer build.**
           Do not carry "the sync half of the acceptance criterion is met" forward without
           this qualifier.
           (o) **`B-STAGE9-36-optionC-r1` soft-landed NOT GREEN at 221k; report transcribed
           here, no file survives. This is the resume point.** DONE and consistent:
           `decisions.md` carries the `<!-- gc-arms/v1 {...} -->` sentinel at 2640 with the
           old 2647/2648 bullets deleted, no other ledger line touched; and
           `garmin_lint_ownership_guard.py` has DO-1/2/3 complete — `_parse_arms_sentinel`
           added reading `entry_lines[1]` raw, `ArmsBulletMalformed` retained, and
           `_ARMS_SHAPED`/`_ARMS_DECLARATION`/`_BACKTICK_TOKEN`/`_ARMS_LEADING_CATEGORIES`/
           `_ARMS_MARKER`/`_strip_arms_leading_run`/`_is_arms_loose_shaped`/the old
           `_armed_patterns` and the `armed is None` branch all deleted (grep-confirmed zero
           surviving references; `ast.parse` clean).
           **BROKEN, and it is the next unit:** DO-4 never started, so
           `test_garmin_lint_ownership_guard.py` still calls `guard._armed_patterns` (~1388,
           1419, 1430, 1476, 1487, 1509, 1623, 1822) and names deleted symbols — the file
           raises AttributeError on import and the whole `garmin-lint-guard` label cannot
           collect. **ZERO ctest this unit; `_parse_arms_sentinel` has never executed against
           the real ledger or any fixture, and the entire PROVE-IT set is unattempted.**
           (p) Builder soft-landed and RESTARTED as **`garmin_builder_stage9_v24`** on the
           same pane `w1:pM`, relaunched with `--permission-mode auto --model sonnet`; both
           confirmed on the fresh status line at `tok 0k`. `B-STAGE9-36-optionC-r2` is LIVE
           on it, scoped to DO-4 only — the test-file rewrite plus the first ctest.
           (q) `B-STAGE9-38-appveyor-remeasure` is LIVE on the investigator, recovering the
           debt in item (6): the lost `appveyor.yml:255-259` lines are re-measured from
           scratch, not recalled.
           (r) **`DEC-062-scope` is COLLECTED and it found three BLOCKING gaps plus an
           ordering fact; all are now binding under DEC-062's own "Scope-pass corrections"
           addendum.** Every file:line DEC-062 cited was re-verified at the origin and
           holds. The gaps: `PyConfig.home`/`program_name` must be OPTIONAL, since setting
           an empty home destroys CPython's normal discovery; `PythonEmbed::
           pythonInstalled()` cannot be deleted because `src/Gui/Pages.cpp:428-440` calls
           it; and the new locator must compile in the OR-bootstrap block of BOTH build
           systems or Garmin-only builds fail to link. DEC-063 must land first —
           `src/CMakeLists.txt:1085` still defines `GC_HAVE_PYTHON` live. The u3-vs-DEC-062
           ordering worry is REFUTED: u3 edits `PyEmbeddedAdapter.cpp` after init and does
           not touch interpreter startup.
           (s) **Correction to this cursor's own briefing language: there is NO `src/Core/
           main.cpp` hold.** The user RELEASED it 2026-09-16 (`STATE.md:750`); DEC-061
           records the release and a fresh unbriefed agent found no re-imposing act.
           Cursors `v1_36`-`v1_45` and this session's first two builder briefs all carried
           "HARD HOLD" forward in error. Keeping a unit off `main.cpp` is a SCOPE choice —
           say scope, not hold, and note DEC-062 legitimately needs `main.cpp` in its set.
           (t) `B-STAGE9-36-guardhalf-review` is LIVE on the reviewer (`/new`-ed, 159k -> 0)
           against the frozen guard half only; the test file is out of its scope while the
           builder rewrites it.
           (u) Inspector-verified directly, not relayed: the sentinel sits at 2640
           immediately after the `## DEC-054` heading (so `entry_lines[1]` reaches it), is
           pure ASCII with no trailing bytes under `cat -A`, opens and closes on one line,
           names its own entry's id, and carries the same single glob the deleted bullet
           declared. Both old bullets are gone and the block runs heading -> sentinel ->
           `- Status:` .. `- Origin:` -> `### The problem`. Guard half agrees with its report.
           (v) **Path to the GOAL, since B-STAGE9-48 changed it:** land B-STAGE9-38 u3 (repair
           under DEC-058 c17) -> land the `appveyor.yml` adapter step -> BUILD A REAL
           INSTALLER, which has never once been done on this host -> re-earn run 7's live sync
           against that artifact. No installer exists here today, so no amount of ctest green
           advances the acceptance criterion. Not a human-in-the-loop gate; no credentials are
           needed until the live re-test itself.

STAGE-9-CURSOR-AMENDMENT-5 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (w) **The guard half of B-STAGE9-36 is BLOCKING — the sentinel is spoofable three
           ways.** B-STAGE9-49/-50/-51, each re-confirmed by the Inspector at the origin, not
           relayed: `json.loads` collapses duplicate `dec` keys; `line.strip()` strips Unicode
           whitespace so a U+00A0-prefixed line still matches; and `^## <id>\b` resolves a
           counterfeit `## DEC-054-shadow` heading, letting a sentinel under it authorize
           while the real entry carries none. B-STAGE9-52 (non-blocking) is a stale docstring.
           **The architecture is NOT implicated:** the reviewer found the rejection half
           clean — no valid sentinel is wrongly rejected — so these are implementation bugs
           inside a holding design, not another recognizer-narrowing round. The step-5
           repair-round bound is therefore not tripped; record that judgment with the round,
           because the bound exists for exactly this finding's history.
           (x) **The `appveyor.yml:255-259` debt in item (6) is DISCHARGED, and it was
           misconceived.** `B-STAGE9-38-appveyor-remeasure` corrected the premise: nothing
           adapter-related belongs at those lines — they are the AppImage version-stamping
           block, and a pip install there would mutate an already-built artifact. DEC-058
           requires the step in `appveyor/linux/after_build.sh` after the requirements
           install, and it is ALREADY THERE (`:49-53` puts requirements immediately before
           it). Nothing was ever owed transcription. Proof measured this pass: extracted-
           interpreter import RED exit 1 -> pip step -> GREEN exit 0 with `__file__` under the
           extracted root, no host import accepted. One live fact for DEC-058 c14:
           `setuptools>=61` is the isolated build backend (`pyproject.toml:5-7`) and was
           ABSENT from the extracted payload, so the step is network-dependent on every leg.
           (y) `B-STAGE9-49-51-repair-scope` LIVE on the reviewer; `STAGE9-installer-
           feasibility` LIVE on the investigator, establishing whether an AppImage can be
           produced on this host at all before anyone spends hours on a build.

STAGE-9-CURSOR-AMENDMENT-6 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (z) **`B-STAGE9-36-optionC-r2` GREEN, and independently useful:** `ctest -L
           garmin-lint-guard` 2/2, pytest 117, 662 lines of recognizer-era test code deleted
           with the symbols they tested, 9 sentinel tests written including one decoy case
           that plants U+034F, U+200B, Greek-Alpha and fullwidth `Arms` bullets AFTER a valid
           sentinel and proves none of them leaks into the armed set. Three real-ledger
           mutations run and reverted byte-clean. Scope exactly the three DEC-064 files.
           NOT approved as closing B-STAGE9-36 — see (aa).
           (aa) **`B-STAGE9-49-51-repair-scope` found a FOURTH bypass, B-STAGE9-53**, and
           confirmed the architecture is not implicated: `garmin_lint_ownership_guard.py:973`
           `next(...)` takes the FIRST of several canonical `## DEC-ddd — ` headings, so an
           earlier duplicate's sentinel authorizes while the intended entry carries none.
           Resolution must require EXACTLY ONE. The ledger already carries amendment-style
           duplicates for DEC-034/040, so "canonical" is the primary `## DEC-ddd — ` form —
           that pair is the regression this repair can plausibly cause, and it is in the
           builder's PROVE-IT list.
           (bb) `B-STAGE9-36-optionC-r3` LIVE on the builder: all four repairs as one unit,
           each stated as the predicate it must enforce rather than as a patch.
           (cc) `DEC-063-cite-recheck` LIVE on the reviewer. DEC-063 was scope-passed against
           `src/CMakeLists.txt` as it stood BEFORE u3 deleted `GARMIN_PY_MODULE_DIR` from
           that same file, so its recorded line numbers are suspect and its repair may have
           changed, not just moved. Queue-tail items scope-passed against a since-edited file
           are not still scope-passed.

STAGE-9-CURSOR-AMENDMENT-7 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (dd) **`STAGE9-installer-feasibility` = FEASIBLE-AFTER-FIXES, recorded as
           B-STAGE9-54.** Disk is ample (504 GiB free). Missing and merely FIXABLE-LOCALLY:
           gcc-11, `$HOME/Qt/6.8/bin/qmake` (host has `qmake6` only), lupdate/lrelease,
           patchelf, appimagetool, linuxdeployqt and ~8 -dev packages. Genuinely harder:
           `python3.11-config` (host is 3.13) and an ABSENT `/dev/fuse` while
           `appveyor/linux/after_build.sh:40,46,72` executes downloaded AppImages directly.
           Asset states measured: FTDI 403 even to a wget UA, Python 3.11.14 the known 404,
           linuxdeployqt/appimagetool/CRAN/deadsnakes/PyPI all 200.
           (ee) **This is the shape of the eventual human-in-the-loop gate, and it is NOT
           reached yet.** Installing system packages with sudo changes the user's machine
           and is outside the codebase. Before asking for it, `STAGE9-installer-no-sudo-path`
           is testing whether both hard items dissolve: `APPIMAGE_EXTRACT_AND_RUN=1` for the
           three fuse-dependent call sites, and whether the Python 3.11 payload the recipe
           ALREADY downloads can supply the build-time `python3.11-config` rather than a
           system SDK. Do not raise the gate until that answers.
           (ff) **`DEC-063-cite-recheck`: my drift suspicion was WRONG and DEC-063 is now
           re-verified, which is the point of asking.** Coordinates hold — u3's
           `GARMIN_PY_MODULE_DIR` deletions were in a separate Garmin block. Exact change
           set is `src/CMakeLists.txt` ALONE: DELETE `:1085` `add_definitions(
           -DGC_HAVE_PYTHON)` (no other args, no side effects), ADD
           `target_compile_definitions(GoldenCheetah PRIVATE GC_WANT_PYTHON)` after `:1403`,
           the closing paren of the `add_executable` opened at `:1398`. qmake already emits
           and consumes `GC_WANT_PYTHON` and needs no matching edit. `GC_HAVE_PYTHON` has
           exactly ONE live occurrence — its own definition — and zero readers of any kind.
           B-STAGE9-55 records that DEC-063's "55 sites" census is 47/11 files today.
           (gg) `B-STAGE9-42-recipe-recheck` LIVE on the reviewer (`/new`-ed, 118k -> 0).

STAGE-9-CURSOR-AMENDMENT-8 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (hh) **`STAGE9-installer-no-sudo-path` = STILL-BLOCKED, recorded as B-STAGE9-57.
           One of the two hard items dissolved; the other is real.** FUSE dissolves:
           `APPIMAGE_EXTRACT_AND_RUN=1` exported before `after_build.sh` is inherited by all
           three call sites and needs NO recipe edit — measured at `:46` (exit 0,
           `squashfs-root` created without `/dev/fuse`), with argument forwarding measured
           too; `:40`/`:72` are unproven only because no local linuxdeployqt/appimagetool
           copies exist. Qt is also local: `lupdate`/`lrelease` are already on disk at
           `/usr/lib/qt6/bin/`, merely off PATH, and gcc-11 is a reproducibility pin, not a
           source requirement (`src/src.pro:41` asks only C++17).
           **The real blocker is the Python 3.11 SDK.** The runtime AppImage carries
           `include/python3.11/Python.h` but NO `python3.11-config` and NO `libpython3.11*`,
           and its interpreter has no NEEDED libpython entry — it is not linkable, so it
           cannot serve the build that `before_build.sh:35-36` demands. Also outstanding:
           a user-local `patchelf` (`after_build.sh:59`), the 3.11.14 404 replacement
           (B-STAGE9-42, already fixed and verified), the FTDI 403, and the native -dev
           packages `install.sh:4-9,39,42` installs.
           (ii) **The human-in-the-loop gate is now CHARACTERIZED and has been put to the
           user; it does NOT stop the loop.** Both routes to a real installer need the
           user's own authority — sudo package installs on their machine, or a push to the
           shared AppVeyor remote. Everything still queued (B-STAGE9-36 r3, the u3 repair,
           DEC-063, B-STAGE9-42, DEC-062) is independent of that answer and continues.
           (jj) **DEC-065 ACCEPTED (user, 2026-09-20): Stage 9's installer evidence is earned
           on AppVeyor, not on a local build.** Four constraints bind and are in the entry —
           the push is gated on separating the Garmin set from the other uncommitted
           workstreams first; the code queue continues independently and must land before the
           push is worth making; B-STAGE9-42's pin-and-verify MUST be in the pushed set or the
           Linux leg fetches a 404 and proves nothing; and `APPIMAGE_EXTRACT_AND_RUN` is a
           local-build artifact that must NOT be carried into the recipe.
           (kk) **`DEC-062-lifetime-test-scope`: DELETE is correct, and the check was worth
           making.** `testPythonProgramNameLifetime.cpp` proves only `Py_SetProgramName`'s
           BORROWED-pointer contract; DEC-062's `PyConfig_SetString` copies, so no
           caller-owned pointer survives and the hazard is removed rather than untested.
           Recorded as a DEC-062 addendum, with the instruction not to rewrite or migrate it.

STAGE-9-CURSOR-AMENDMENT-9 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (ll) **`B-STAGE9-36-optionC-r3` GREEN** — `ctest -L garmin-lint-guard` 2/2, pytest
           123 (117+6), all four bypasses closed, `decisions.md` byte-unchanged by the unit.
           Five new tests verified RED against a hand-reverted r2 guard then GREEN, and the
           named regression risk — the DEC-034/040 amendment duplicates — has its own test
           that passed against BOTH baselines.
           (mm) **The builder substituted a STRICTER predicate than was specified, and it is
           now the reviewer's first hunt item.** For B-STAGE9-51 it used
           `rf"^## {re.escape(dec_id)} — "` (space em-dash space) at `:983` instead of the
           prescribed `(?= |$)`, and for B-STAGE9-53 it raises on >1 canonical heading but
           returns None on 0 rather than raising. Inspector-verified against the live ledger
           before accepting either: 67 `## DEC-` headings, 64 match the canonical form, zero
           duplicate ids, and the 3 non-matching are exactly the DEC-034/036, DEC-038 and
           DEC-040 amendment headings that MUST be excluded. Both deviations are fail-closed
           here; whether they reject anything legitimate later is what `B-STAGE9-36-r3-review`
           is for.
           (nn) Builder soft-landed at 218k and restarted as **`garmin_builder_stage9_v25`**
           on `w1:pM` with `--permission-mode auto --model sonnet`, both confirmed at
           `tok 0k`. `B-STAGE9-38-u3-r4` is LIVE on it — the u3 repair under DEC-058 c17,
           which is the last thing standing between this branch and a commit-ready Garmin set.

STAGE-9-CURSOR-AMENDMENT-10 (2026-09-20, `garmin_inspector_v1_46`; sequencing only):
           (oo) **B-STAGE9-36 is CLOSED and COMMITTED `02cf0c23c`** — reviewer
           `B-STAGE9-36-r3-review` PASS on BOTH halves (no authorization bypass survives,
           and no legitimate decision is now wrongly rejected), and the Inspector re-ran
           `ctest -L garmin-lint-guard` 2/2 and pytest 123 himself, then AGAIN after
           ruff-format reformatted two files, to confirm the reformat was cosmetic.
           B-STAGE9-49/-50/-51/-52/-53 all closed with it. Full pre-commit gate passed:
           ruff-check, ruff-format, ledger-drift-lint.
           (pp) Both builder deviations were adjudicated SAFE by the reviewer, not waved
           through: the stricter literal `^## <id> — ` heading form rejects nothing
           legitimate under the ledger's established contract, and the 0-headings-returns-
           None path has no permissive caller. Recorded because a future ledger that stops
           using the em-dash heading form would silently start failing closed.
           (qq) Commit hygiene held: the staged set was 6 files, zero `src/`, zero Coach or
           Qt6.8 contamination, and WIKI's DEC registry pointer was corrected from a
           two-id-stale `next:garmin-063` in the same change.
           (rr) **Pre-push work is now LIVE on both non-builder agents, because DEC-065 makes
           the CI recipes the artifact that must be right:** `DEC-058-recipe-audit` on the
           reviewer (do the three legs' adapter steps actually satisfy constraints 8-11 and
           14-16 as COMMITTED, not as described) and `STAGE9-ci-blocker-sweep` on the
           investigator (`/new`-ed, 187k -> 0; what would kill a CI run for reasons
           unrelated to our code — FTDI 403 hotlink-vs-dead, other dead assets, dead PPAs,
           and whether each leg's declared image still provides its pinned toolchain).
           (ss) **SELF-SUCCESSION, and READ THIS BEFORE TRUSTING ANY CONTEXT NUMBER.**
           `garmin_inspector_v1_46` crossed its 210k threshold unnoticed and ran to 300k
           because `claude_context.py` reported a FROZEN 115701 for ~15 consecutive wakes:
           the pane's session rotated (`60f24d0f` -> `a6d4d624`) and the reader kept
           resolving the stale file, returning a confident wrong number rather than
           `unknown`. Recorded as B-STAGE9-58. **Successor: cross-check your own pane's
           script reading against the visible footer's `tok Nk` at least every few wakes,
           and treat an unchanging number across wakes as STALE, not as stable.** The wake
           block's own `--- context ---` line carries the same stale value, so it is not an
           independent check.
           (tt) Successor `garmin_inspector_v1_47` is LIVE on pane `w1:p2F`, tab `w1:t27`,
           Opus, `auto mode on` confirmed on its own status line after the single
           predecessor click the mechanism allows. `v1_46` does not retire itself and has
           armed NO further wake — supervision is the successor's from here, and two armed
           wakes would mean two Inspectors polling the same panes.
           **In flight at handoff, all three dispatched, none collected:**
           `B-STAGE9-38-u3-r4` on builder `garmin_builder_stage9_v25` (`w1:pM`, fresh);
           `DEC-058-recipe-audit` on `garmin_codex_reviewer` (`w1:pD`);
           `STAGE9-ci-blocker-sweep` on `s925_tz_investigator` (`w1:pR`, `/new`-ed).
           **Builder queue after u3-r4:** DEC-063 (exact: `src/CMakeLists.txt` only, DELETE
           `:1085`, ADD `target_compile_definitions(GoldenCheetah PRIVATE GC_WANT_PYTHON)`
           after `:1403`) -> B-STAGE9-42 (exact patch in the B-STAGE9-56 row, digest
           verified) -> DEC-062 (scoped, 4 binding corrections in its addendum).
           **Then, and only then:** separate the Garmin set from the Coach/Qt6.8 work,
           commit, and push for the AppVeyor run DEC-065 authorizes.

STAGE-9-CURSOR (2026-09-20, `garmin_inspector_v1_47` — supersedes the `v1_46` block and its
           ten amendments; sequencing only):
           (a) Succession complete. `garmin_inspector_v1_46` retired at 309k; pane `w1:p2E` and
           tab `w1:t26` closed (workspace now 4 panes / 2 tabs). Graceful `/exit` was
           UNREACHABLE on that pane — every Enter landed in Claude Code's agents sidebar
           ("describe a task for a new session") regardless of `agent send-keys`, `pane
           send-keys`, `pane run` or double `ctrl+c`; the pane was closed only after the agent
           was confirmed `idle`, handed off, and holding no armed wake.
           (b) All three of `v1_46`'s in-flight results are COLLECTED.
           (c) `B-STAGE9-38-u3-r4` GREEN on `garmin_builder_stage9_v25`. `hoistOverrideDirToFront`
           replaces prepend-if-absent and fails CLOSED; `explicitOverrideCacheIsSafe` rejects an
           import whose `sys.modules` entry is not proven under the override dir; two isolated
           tests, each proven RED against its own targeted mutation (mutation B isolates the
           cache guard alone). 3 files, +387/-75, unstaged. B-STAGE9-44/-45/-46/-47 stay `open`
           until the reviewer's verdict — a builder self-report closes nothing.
           (d) `DEC-058-recipe-audit` (reviewer) returned: Windows and macOS legs CORRECT by
           recipe reading; Linux DEFECTIVE on two counts — the already-known 3.11.14 404
           (B-STAGE9-42) and a NEW gap recorded as **B-STAGE9-59**: `appveyor.yml:255-264`
           smoke-tests only `--version`, so DEC-058 C11's payload assertion is never made.
           All three legs rely on pip build isolation for setuptools (DEC-058 C14, acknowledged).
           (e) `STAGE9-ci-blocker-sweep` is IN FLIGHT on the investigator; its HEAD-request
           probes need one approval each and `ftdichip.com` does not resolve from this host, so
           the FTDI question may return unmeasurable rather than answered.
           (f) IN FLIGHT: `B-STAGE9-38-u3-r4-review` on the reviewer (`w1:pD`);
           `DEC-063-cmake-want-python` on the builder (`w1:pM`, scratch build dir
           `/tmp/gc-build-pyon` so `./build` is not reconfigured under the other agents).
           (g) Builder queue after DEC-063: B-STAGE9-42 (exact patch in the B-STAGE9-56 row) ->
           DEC-062 (4 binding corrections in its addendum) -> B-STAGE9-59. Then separate the
           Garmin set from the Coach/Qt6.8 work, commit, and push for the AppVeyor run DEC-065
           authorizes.
           (h) Context at handoff: builder 108k, reviewer 149k, investigator 47k, Inspector 64k.
           Per B-STAGE9-58, the Inspector's own script reading is cross-checked against the
           pane footer, and an unchanging number across wakes is treated as STALE.
