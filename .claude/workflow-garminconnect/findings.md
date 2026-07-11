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
| B-R004-01 | Builder/REQ-004 | non-blocking | garmin_client.__init__ still forwards tokenstore_path → real library self-writes its own token file | fixed | RESOLVED by REQ-006 Slice B (auth-only reconciliation): `GarminClient.__init__` → `(email, password)`, C-API `"ss"`, `PyEmbeddedAdapter(modulePath)`; T-015 asserts no 3rd arg forwarded, T-016 asserts `LAST_TOKENSTORE` absent. Superseded finding A3-R004-M3 landed in same slice |
| B-R004-02 | Builder/REQ-004 | non-blocking | clang-format/ruff/mypy absent in build env → DEC-009 style/type gate not run | fix-now | run style/type gate (pre-commit) before REQ-004 commit; tests+py_compile pass, format/lint/type unverified |
| A3-R004-M1 | A3/REQ-004 | blocking | AtomicFile write-then-chmod ORDER unpinned — order-swap mutant SURVIVES T-011 (REQ-NF-Sec-002 no-world-readable-window untested) | fixed | FIXED 2026-07-11 — T-011 order-recording seam (`AtomicFile::TmpWriter`, null-in-production): `writeOver` routes setPermissions→write through the writer; test asserts order "PW", swap mutant now records "WP" and FAILS (testAtomicFile.cpp:153). Prod signature/behaviour unchanged. Orchestrator re-verified garmin-fast 9/9. Residual (noted): a mutant removing the seam entirely isn't caught — accept |
| A3-R004-M2 | A3/REQ-004 | blocking | load_tokens() no-op is undetectable by primary round-trip test (fake seeds identical session per instance) | fixed | FIXED 2026-07-11 — T-013 `_FakeGarmin` seeds per-instance session via class counter; round-trip now asserts `dst.dump != blob` precondition then equality post-load, so a `pass` load_tokens FAILS. pytest 15/15 re-verified |
| A3-R004-M3 | A3/REQ-004 | blocking | REQ-NF-Sec-002 end-to-end UNMET — library self-writes a 2nd token file at the still-forwarded tokenstore_path (perms never enforced/checked); test_adapter_login.py:103 REQUIRES that file exist as REQ-002 GREEN | fixed | RESOLVED by REQ-006 Slice B 2026-07-11: library constructed AUTH-ONLY (no tokenstore path → no self-written 2nd file); the only token file is the C++-owned 0600 write (GarminTokenStore, Slice A perm-checks its load). test_adapter_login.py:103 rewritten from mere-existence → auth-only + dump_tokens() blob persistence (T-015). REQ-NF-Sec-002 now end-to-end MET. Verification-Gate PASS: pytest 15/15, garmin-py 20/20, garmin-fast 10/10 |
| A3-R004-04 | A3/REQ-004 | non-blocking | fsync-skip unenforced (fsyncFile→true mutant survives) | defer | fault-injection ticket (REQ-NF-Reliab-002 durability) |
| A3-R004-05 | A3/REQ-004 | non-blocking | stale-.tmp cleanup removal survives (symlink/TOCTOU defense untested) | defer | fault-injection ticket; bounded by 0700 dir invariant |
| A3-R004-06 | A3/REQ-004 | non-blocking | short-write check removal survives (partial-write corruption path untested) | defer | disk-full/quota fault-injection ticket |
| A3-R004-07 | A3/REQ-004 | non-blocking | fixed dest+.tmp name → spurious-false under concurrent same-dest writes; save() has no retry | accept-with-note | DES-001 single-worker mailbox serializes token writes in production; add tmp-uniqueness/retry if a concurrent write path ever appears |
| A3-R004-08 | A3/REQ-004 | non-blocking | sole failed-write test is user-dependent (root DAC_OVERRIDE flips it RED, not silent-skip) | accept-with-note | only one write-failure mode tested; unprivileged CI assumed |
| A3-R004-09 | A3/REQ-004 | informational | Windows AtomicFile path (MoveFileExW/FlushFileBuffers) never built in Linux env | defer | cross-platform CI (REQ-NF-Pkg-001 Phase 2) |
| A3-R006-01 | A3/REQ-006 | blocking | mask-narrowing mutant SURVIVES T-014 — refusal mask reduced to ReadGroup\|ReadOther (drop Write*/Exe*) passes all 5 cases because T-014 only sets Read-class widened modes (0640/0644); production mask IS complete but the test doesn't pin it | fixed | FIXED 2026-07-12 — TEST-014 +2 slots: groupWriteOnlyWidenedRefused (0620) + otherExecOnlyWidenedRefused (0601) assert TokenPermissionsRejected + bytes empty. Builder demonstrated the kill: narrowed mask→both new cases FAIL, reverted→green. garmin-fast 10/10; production untouched (verified clean diff). Verification-Gate PASS |
| A3-R006-02 | A3/REQ-006 | non-blocking | stale-permissions-cache mutant SURVIVES — no test calls loadChecked() twice on one path with a perms change between, so the "read at load time, never cached" property (A3-R004-M1 comment) is unverified | fixed | FIXED 2026-07-12 — TEST-014 +1 slot permissionsReReadNotCachedAcrossCalls (0600→Ok, widen same file in place to 0640, re-load→TokenPermissionsRejected). Kills the static-cache mutant |
| A3-R006-03 | A3/REQ-006 | informational | T-016's `LAST_TOKENSTORE.isNull()` assertion is masked under the M-B1 "sss" mutant — the pystub's strict 2-arg __init__ raises TypeError → suite fails earlier at the Success QCOMPARE, so T-016's named check is never reached; the independent kill lives in T-015 (`len(forwarded)==2`) | accept | T-015 is the real, non-incidental guard; T-016 redundancy is harmless. No action unless T-015 is ever weakened, then give the pystub tolerant *args capture so T-016 becomes independent |
