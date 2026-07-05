# Findings register

One row per finding ever raised by any cycle. The source of truth for CLV Check 5 (`grep '\| blocking \|' findings.md | grep -vE '\| (fix-now|deferred|accepted) \|'` must be empty). Cycle files hold the *why*; this file holds the *closure*.

| ID | cycle | severity | summary (≤60 chars) | disposition | resolved-by |
|---|---|---|---|---|---|
| A0-001 | A0/P0 | non-blocking | risk-adjusted MVP = download-only | fix-now | DEC-001 staged ship |
| A0-002 | A0/P0 | non-blocking | existing-tool probe — no fit | fix-now | DEC-001 |
| A0-003 | A0/P0 | informational | non-code path insufficient | informational | cycles/archive/a0.md § A0.2 |
| A0-004 | A0/P0 | informational | do-nothing cost > build cost | informational | cycles/archive/a0.md § A0.4 |
| A1-001 | A1/P1 | blocking | REQ-003 MFA scope ambiguous | fix-now | REQ-003 rewrite (PRD) |
| A1-002 | A1/P1 | blocking | REQ-007 FIT/TCX fallback unspecified | fix-now | REQ-007 (FIT default + TCX fallback) |
| A1-003 | A1/P1 | blocking | REQ-008 dedup mechanism undefined | fix-now | REQ-008 (RideCache + per-account sidecar) |
| A1-004 | A1/P1 | blocking | REQ-009 ToS notice wording undefined | fix-now | REQ-009 (verbatim text) |
| A1-005 | A1/P1 | blocking | REQ-010 backfill cancel/resume gap | fix-now | REQ-010 (paginated + atomic-per-page + resumable) |
| A1-006 | A1/P1 | blocking | REQ-NF-Perf-001/003 latency baseline missing | fix-now | NF-Perf-001/003 (50/10 Mbit reference) |
| A1-007 | A1/P1 | blocking | REQ-NF-Sec-003 trust-store / pinning unspecified | fix-now | NF-Sec-003 (OS trust store; verify=False forbidden) |
| A1-008 | A1/P1 | blocking | REQ-NF-Reliab-002 atomic-write gap | fix-now | NF-Reliab-002 (tmp+rename + fsync) |
| A1-009 | A1/P1 | blocking | REQ-NF-Threads-001 stall budget missing | fix-now | NF-Threads-001 (<100ms event-loop stall) |
| A1-010 | A1/P1 | blocking | REQ-NF-Obs-001 observability surface vague | fix-now | NF-Obs-001 (ErrorBus + qDebug structured fields) |
| A1-011 | A1/P1 | non-blocking | REQ-NF-Pkg-001 installer-CI coverage gap | defer | NF-Pkg-001 Phase 2 CI smoke job (Phase 1 = manual checklist) |
| A1-012 | A1/P1 | blocking | REQ-NF-Perf-002 concurrent-sync semantics undefined | fix-now | NF-Perf-002 (reject second; running continues) |
| A1-013 | A1/P1 | blocking | REQ-010 hard-cap missing | fix-now | REQ-010 (5y cap; user-editable advanced) |
| A1-014 | A1/P1 | blocking | REQ-010 empty-result = error | fix-now | REQ-010 (empty = success) |
| A1-015 | A1/P1 | blocking | REQ-008 dedup key (server time vs local) | fix-now | REQ-008 (startTimeGMT key) |
| A1-016 | A1/P1 | blocking | REQ-004 per-athlete dir + perms | fix-now | REQ-004 (0700 dir; per-athlete) |
| A1-017 | A1/P1 | blocking | atomic-write reads detect torn writes | fix-now | NF-Reliab-002 |
| A1-018 | A1/P1 | non-blocking | A1 persona/edge classes (multiple) | fix-now | per-PRD fixes; full list in cycles/archive/a1.md |
| A2-001 | A2/P1 | non-blocking | sub-interpreter wedge has no auto-recovery | defer | Phase 1.5 (thread-heartbeat + kill-and-recreate) |
| A2-002 | A2/P1 | informational | library-tracked SSO risk | accept | REQ-NF-Compat-001 (docs); DES-012 swap cost |
| A2-003 | A2/P1 | blocking | DES-005 limiter scope (per-call vs per-route) | fix-now | DES-005 update |
| A2-004 | A2/P1 | blocking | adapter seam absent → swap cost unbounded | fix-now | DES-012 (single point of library import) |
| A2-005 | A2/P1 | blocking | token-file perms not validated on read | fix-now | DES-002 invariant (refuse load on wider-than-owner) |
| A2-006 | A2/P1 | blocking | per-account sidecar partitioning needed | fix-now | DES-002 path scheme (imported-<uid>.json) |
| A2-007 | A2/P1 | informational | parse-cancel timing | accept | rationale in cycles/archive/a2.md |
| A2-008 | A2/P1 | non-blocking | phishing surface (fake Garmin login dialog) | defer | UX warning ticket |
| A3-R001-M5 | A3/REQ-001 | blocking | clone() could return `this` | fix-now | TEST-001 strengthened (QVERIFY2(cloned != s)) |
| A3-R001-M6 | A3/REQ-001 | non-blocking | fallback colour cosmetic | accept | DES-003 visual check |
| A3-R001-M9 | A3/REQ-001 | blocking | uiName() base-fallback "None" | fix-now | TEST-001 brand-substring assert |
| A3-R001-tool | A3/REQ-001 | non-blocking | no C++ mutation tool | defer | Phase 3 entry (mull-cxx or cosmic-ray-cpp) |
| A3-R002-M2 | A3/REQ-002 | non-blocking | dead `self._tokenstore_path` assignment | fix-now | Dropped in GREEN |
| A3-R002-M6 | A3/REQ-002 | blocking | over-broad except clause misroutes errors | fix-now | test_non_auth_exception_is_not_misclassified_as_auth |
| A3-R002-M10 | A3/REQ-002 | non-blocking | PEP 3134 chaining decorative | accept | .original is the contract; __cause__ cosmetic |
| A3-R002-M11 | A3/REQ-002 | blocking | empty-msg fallback never exercised | fix-now | test_auth_error_with_empty_message_still_yields_displayable_message |
| A3-R002-hypothesis | A3/REQ-002 | non-blocking | no hypothesis property tests | defer | A3/REQ-014 cycle (widens to ≥6 mappings) |
| A3-R002-mutmut | A3/REQ-002 | non-blocking | no Python mutation tool | defer | A3/REQ-007 cycle |
| D-01 | CLV/VAL-004 | non-blocking | Phase 2.1 bootstrap untracked in git | fix-now | commit e4ac2a88b (staged with REQ-002 RED) |
| D-02 | CLV/VAL-004 | non-blocking | .venv hygiene (must not enter history) | fix-now | per-directory .gitignore in e4ac2a88b |
| A3-R002-TR-01 | A3/REQ-002-TR | blocking | AddCloudWizard routing/lifecycle compiled by zero tests | fix-now | TEST-007 testGarminConnectWizardRouting.cpp (4 slots; kills M1/M2 routing, M3 idempotency, M4 dtor order) — working tree |
| A3-R002-TR-02 | A3/REQ-002-TR | blocking | TEST-006 teardown bound is a proven mutation survivor | fix-now | TEST-006 idle bound tightened to «kQuitWaitMs (LSN-009); kills delete-quit() mutant — working tree |
| A3-R002-TR-03 | A3/REQ-002-TR | non-blocking | hasAthlete→25 doubly-dead branch (silent landmine) | defer | guarded by TR-01 routing test, then accept-with-note |
| A3-R002-TR-04 | A3/REQ-002-TR | non-blocking | terminate() last-resort path has no intentional coverage | fix-now | TEST-006 deterministic non-GIL busy-loop terminate() test — working tree |
| A3-R002-TR-05 | A3/REQ-002-TR | non-blocking | PyEmbeddedAdapter malformed-result branches untested | fix-now | TEST-005 pystub non-dict/missing-key SCENARIOs + Unknown-not-Success asserts — working tree |
| A3-R002-TR-06 | A3/REQ-002-TR | informational | pystub fidelity vs real module unverified (out of scope) | defer | dedicated fidelity check vs test_adapter_login.py |
| A3-R002-TR-07 | A3/REQ-002-TR | informational | GARMIN_PY_MODULE_DIR deferral tracked; fail-safe covered | accept | missingModuleYieldsUnknownWithoutCrash (TEST-005) proxy; DES-007/NF-Pkg-001 |
| A3-R002-TR-08 | Builder/TR-04 | non-blocking | uncancellable native wedge → ~GarminAuthChain aborts (qFatal) | defer | Phase 1.5 with A2-001 (wedged-worker recovery); only pure native loop, realistic wedges unwind |
