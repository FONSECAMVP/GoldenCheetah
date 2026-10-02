# Findings register

One row per finding. Checker: `scripts/clv_findings.py`. Rows marked ↗ have their full
pre-compaction record in `archive/findings-detail.md`; the old preamble and historical
notes are in `archive/findings-notes.md`. Row shape: ledger writing contract (QGDW
`references/state-and-tiers.md`).

| ID | cycle | severity | summary (≤60 chars) | disposition | resolved-by |
|---|---|---|---|---|---|
| A0-001 | A0/P0 | non-blocking | risk-adjusted MVP = download-only | fix-now | DEC-001 staged ship |
| A0-002 | A0/P0 | non-blocking | existing-tool probe — no fit | fix-now | DEC-001 |
| A0-003 | A0/P0 | informational | non-code path insufficient | informational | cycles/archive/a0.md § A0.2 |
| A0-004 | A0/P0 | informational | do-nothing cost > build cost | informational | cycles/archive/a0.md § A0.4 |
| A1-001 | A1/P1 | blocking | REQ-003 MFA scope ambiguous | fix-now | REQ-003 rewrite (PRD) |
| D-R003-01 | VG REQ-003 | non-blocking | DES-003a sketch put MFA as... | accept-with-note | — ↗ |
| B-R003-01 | B REQ-003 | non-blocking | WAC::submitMfa/mfaRequired re-emit + IGarminAuthClient... | fixed | T-037 ↗ |
| B-R003-02 | B REQ-003 | non-blocking | pending-MFA state retention across a bad OTP (re-prompt... | fixed | T-038 ↗ |
| B-R003-03 | B REQ-003 | non-blocking | TEST-035 asserts synchronously w/o pumping the loop; a... | fixed | T-035 ↗ |
| A3-R003-01 | A3 A3-R003 | blocking | PyEmbeddedAdapter's REAL embedded-Python MFA bridge... | fixed | T-036 ↗ |
| A3-R003-05 | A3 A3-R003 | non-blocking | Back-navigation state leak: no... | fixed | T-039 ↗ |
| A3-R003-06 | A3 A3-R003 | non-blocking | onAuthFinished/onAuthFailed guard only on... | fixed | T-041 ↗ |
| A3-R003-07 | A3 A3-R003 | informational | TEST-034 line 221 QVERIFY2(…text().isEmpty()==false //... | fixed | T-040 ↗ |
| A3-R003-09 | A3 A3-R003 | non-blocking | GarminCredentialsPage::onMfaRequired has no... | accept-with-note | REQ-015 ↗ |
| A3-R003-08 | A3 A3-R003 | informational | DEC-014-OQ1 residual (already self-flagged in code)... | accept-with-note | DEC-014 ↗ |
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
| A3-R002-TR-01 | A3 REQ-002 | blocking | AddCloudWizard routing/lifecycle compiled by zero tests | fix-now | TEST-007 ↗ |
| A3-R002-TR-02 | A3 REQ-002 | blocking | TEST-006 teardown bound is a proven mutation survivor | fix-now | TEST-006,LSN-009 ↗ |
| A3-R002-TR-03 | A3/REQ-002-TR | non-blocking | hasAthlete→25 doubly-dead branch (silent landmine) | defer | guarded by TR-01 routing test, then accept-with-note |
| A3-R002-TR-04 | A3/REQ-002-TR | non-blocking | terminate() last-resort path has no intentional coverage | fix-now | TEST-006 deterministic non-GIL busy-loop terminate() test — working tree |
| A3-R002-TR-05 | A3 REQ-002 | non-blocking | PyEmbeddedAdapter malformed-result branches untested | fix-now | TEST-005 ↗ |
| A3-R002-TR-06 | A3/REQ-002-TR | informational | pystub fidelity vs real module unverified (out of scope) | defer | dedicated fidelity check vs test_adapter_login.py |
| A3-R002-TR-07 | A3/REQ-002-TR | informational | GARMIN_PY_MODULE_DIR deferral tracked; fail-safe covered | accept | missingModuleYieldsUnknownWithoutCrash (TEST-005) proxy; DES-007/NF-Pkg-001 |
| A3-R002-TR-08 | B TR-04 | non-blocking | uncancellable native wedge → ~GarminAuthChain aborts... | defer | — ↗ |
| B-R004-01 | B REQ-004 | non-blocking | garminclient.init still forwards tokenstorepath → real... | fixed | REQ-006 ↗ |
| B-R004-02 | B REQ-004 | non-blocking | clang-format/ruff/mypy absent in build env → DEC-009... | fixed | 54b7005e6,REQ-004 ↗ |
| A3-R004-M1 | A3 REQ-004 | blocking | AtomicFile write-then-chmod ORDER unpinned — order-swap... | fixed | T-011 ↗ |
| A3-R004-M2 | A3 REQ-004 | blocking | loadtokens() no-op is undetectable by primary round-trip... | fixed | T-013 ↗ |
| A3-R004-M3 | A3 REQ-004 | blocking | REQ-NF-Sec-002 end-to-end UNMET — library self-writes a... | fixed | REQ-006 ↗ |
| A3-R004-04 | A3/REQ-004 | non-blocking | fsync-skip unenforced (fsyncFile→true mutant survives) | defer | fault-injection ticket (REQ-NF-Reliab-002 durability) |
| A3-R004-05 | A3/REQ-004 | non-blocking | stale-.tmp cleanup removal survives (symlink/TOCTOU defense untested) | defer | fault-injection ticket; bounded by 0700 dir invariant |
| A3-R004-06 | A3/REQ-004 | non-blocking | short-write check removal survives (partial-write corruption path untested) | defer | disk-full/quota fault-injection ticket |
| A3-R004-07 | A3 REQ-004 | non-blocking | fixed dest+.tmp name → spurious-false under concurrent... | accept-with-note | — ↗ |
| A3-R004-08 | A3 REQ-004 | non-blocking | sole failed-write test is user-dependent (root... | accept-with-note | — ↗ |
| A3-R004-09 | A3/REQ-004 | informational | Windows AtomicFile path (MoveFileExW/FlushFileBuffers) never built in Linux env | defer | cross-platform CI (REQ-NF-Pkg-001 Phase 2) |
| A3-R006-01 | A3 REQ-006 | blocking | mask-narrowing mutant SURVIVES T-014 — refusal mask... | fixed | TEST-014 ↗ |
| A3-R006-02 | A3 REQ-006 | non-blocking | stale-permissions-cache mutant SURVIVES — no test calls... | fixed | TEST-014 ↗ |
| A3-R006-03 | A3 REQ-006 | informational | T-016's LASTTOKENSTORE.isNull() assertion is masked... | accept | T-015,T-016 ↗ |
| W-DEC003 | A2 | non-blocking | sqlite-sidecar migration trigger (per-account sidecar perf) | defer | DEC-003 ↗ |
| B-R007-01 | B REQ-007 | blocking | readFile emits readComplete synchronously → caller's... | fixed | — ↗ |
| B-R007-02 | B REQ-007 | non-blocking | GarminConnect left non-QOBJECT (briefing said make it... | resolved-by-mechanism | B-R007-01 ↗ |
| A3-R007-01 | A3 REQ-007 | non-blocking | postReadComplete's queued lambda captures this... | fixed | — ↗ |
| A3-R007-02 | A3 REQ-007 | non-blocking | TEST-024 proves not-synchronous/fires-once only against... | accept | — ↗ |
| A3-R007-03 | A3 REQ-007 | non-blocking | readFile()'s pre-existing data==nullptr //... | fixed | TEST-026 ↗ |
| A3-R007-04 | A3 REQ-007 | informational | QCOMPARE→QTRYCOMPARE conversion on T-020/021/023 is... | accept | — ↗ |
| B-R007-03 | Orch REQ-007 | was-blocking-for-production | main GoldenCheetah binary fails to link with undefined... | fixed | — ↗ |
| A3-R008-01 | A3 REQ-008 | blocking-for-end-to-end | REQ-008 incremental sync cannot resolve a garminuserid... | FIXED | DEC-019,TEST-051 ↗ |
| D-R008-01 | Design REQ-008 | non-blocking | DES-002 (design.md ~489) states... | FIXED | DEC-019 ↗ |
| A3-R008-F1 | A3 REQ-008 | blocking | readFile() records the Tier-1 dedup entry BEFORE the... | ACCEPTED-WITH-RATIONALE | REQ-016 ↗ |
| A3-R008-F2 | A3 REQ-008 | non-blocking | GarminSidecarStore::recordImported CLOBBERS (not merges)... | ACCEPTED | REQ-016 ↗ |
| A3-R008-F3 | A3 REQ-008 | non-blocking | PyEmbeddedAdapter's per-item PyDictCheck guard... | FIXED | TEST-053 ↗ |
| A3-R008-F4 | A3 REQ-008 | informational | CloudService::disconnect() (new plain member fn... | FIXED | — ↗ |
| A3-R008-F5 | A3 REQ-008 | informational | since-timestamp contract: readdir emits... | accept-with-note | DEC-014 ↗ |
| A3-R012-F1 | A3 REQ-012 | BLOCKING | Disconnect does not stop a live, already-open()ed... | FIXED | f001c7d20,DEC-020 ↗ |
| A3-R012-F2 | A3 REQ-012 | non-blocking | clearAccount never removes the tokens.json.tmp sibling... | open | f001c7d20 ↗ |
| A3-R012-F3 | A3 REQ-012 | non-blocking | The two ordering invariants this feature documents most... | open | — ↗ |
| A3-R012-F4 | A3 REQ-012 | non-blocking | recordImport's empty-uid guard (GarminConnect.cpp:481)... | open | f001c7d20,TEST-059,DEC-018 ↗ |
| A3-R012-F5 | A3 REQ-012 | non-blocking | TEST-055's headline claim "prior-account sidecars sit on... | open | f001c7d20,TEST-055 ↗ |
| A3-R012-F6 | A3 REQ-012 | non-blocking | clearAccount's bool return is DISCARDED at... | open | — ↗ |
| A3-R012-F7 | A3 REQ-012 | informational | TEST-055(c) asserts imported-<B>.json is populated the... | accept-with-note | REQ-016,T-055 ↗ |
| A3-R012-F8 | A3 REQ-012 | informational | TEST-055's gcA.disconnectService() call is decorative —... | accept-with-note | f001c7d20,TEST-055 ↗ |
| A3-R012-F9 | A3 REQ-012 | informational | The server-supplied garminuserid flows unvalidated into... | accept-with-note | — ↗ |
| A3-R012-F10 | A3 REQ-012 | informational | The uid is re-resolved per call rather than latched... | accept-with-note | — ↗ |
| A3-R012-F11 | A3 REQ-012 | informational | TEST-054 proxies "full SSO": it proves no silent restore... | accept-with-note | REQ-002 ↗ |
| A3-R012-F12 | A3 REQ-012 | informational | CloudService instances leak with a live garth session +... | accept-with-note | REQ-012 ↗ |
| B-R012-01 | B REQ-012 | non-blocking | REQ-012's criterion clause "deletes the token file... | open | REQ-012 ↗ |
| B-R012-02 | B REQ-012 | informational | Test-fixture hygiene from the REQ-012 slice: (i)... | accept-with-note | A3-R012 ↗ |
| B-R017-06 | B REQ-017 | BLOCKING | A superseded/disconnected readFile STALLS the sync loop... | FIXED | DEC-022 ↗ |
| B-R017-01 | B REQ-017 | non-blocking | REQ-017(a) requires a superseded session to fail "with a... | open | A3-R017 ↗ |
| B-R017-02 | B REQ-017 | non-blocking | The LAZY latch (ensureSessionLatched) is the softest... | open | A3-R017 ↗ |
| B-R017-03 | B REQ-017 | informational | close() does not clear the latch — only a successful... | accept-with-note | — ↗ |
| B-R017-04 | B REQ-017 | informational | mpendingStartTimes is not cleared on re-latch, so... | open | — ↗ |
| A3-R017-F1 | A3 REQ-017 | BLOCKING | Slice B's WADeleteOnClose turns the pre-existing leak... | FIXED | DEC-024 ↗ |
| ORCH-001 | Orch | BLOCKING-for-commit-verification | master (HEAD 9ba1d33d3) does not configure from a clean... | FIXED | 427da745b,DEC-028 ↗ |
| A3-R017b-F1 | A3rc REQ-017 | BLOCKING | The DEC-024 gate is on the wrong LAYER. It intercepts... | FIXED | DEC-025,TEST-072,A3-R017b-F4 ↗ |
| A3-R017b-F2 | A3rc REQ-017 | non-blocking | Residual claim (a) — "no service runs a nested loop in... | open | DEC-025 ↗ |
| A3-R017b-F4 | A3rc REQ-017 | informational | Architectural: the guard is wired to the dialog's... | open | DEC-025 ↗ |
| A3-R017-F2 | A3 REQ-017 | non-blocking | testGarminConnectAccountEpoch.cpp:452-464 (TEST-062, the... | open | TEST-062 ↗ |
| A3-R017-F3 | A3 REQ-017 | informational | CloudServiceSyncDialog::completedRead()'s abort branch... | accept-with-note | — ↗ |
| A3-R017-F4 | A3 REQ-017 | informational | CloudServiceAutoDownload::readFailed() mirrors... | accept-with-note | A3-R017 ↗ |
| A3-R017-F5 | A3 REQ-017 | informational | The anti-duplication hook (LSN-001 mechanism) denied the... | FIXED | — ↗ |
| B-R023-01 | B DEC-023 | non-blocking | After DEC-023, GarminConnect::readFile reports failure... | open | TEST-065 ↗ |
| B-R023-02 | B DEC-023 | informational | On the autodownload path the this-slot connection... | accept-with-note | A3-R017 ↗ |
| B-R023-03 | B DEC-023 | informational | TEST-069 replaces global operator new/delete for one... | accept-with-note | A3-R017 ↗ |
| B-R023-04 | B DEC-023 | informational | CloudServiceAutoDownload's half of DEC-023 — its... | accept-with-note | B-R018-01 ↗ |
| B-R018-01 | B REQ-018 | non-blocking | REQ-018's criterion says the activity must import "in... | open | A3-R017 ↗ |
| B-R018-02 | B REQ-018 | non-blocking | Three more silent-refusal paths found while crossing the... | open | DEC-023 ↗ |
| B-R018-03 | B REQ-018 | informational | The fix forced an additive edit to SHARED test... | accept-with-note | REQ-017 ↗ |
| B-R018-04 | B REQ-018 | informational | The builder flagged its own risky choices... | accept-with-note | A3-R018 ↗ |
| B-R017-09 | B DEC-022 | BLOCKING | Every SUCCESSFUL Garmin download currently fails to... | FIXED | REQ-018,TEST-067 ↗ |
| B-R017-10 | B DEC-022 | non-blocking | DEC-022 as scoped fixes only 2 of ~6 non-posting return... | open | DEC-022 ↗ |
| B-R017-11 | B DEC-022 | informational | readFile's two refusal paths now both emit tr("Garmin... | accept-with-note | — ↗ |
| B-R017-12 | B DEC-022 | informational | Slice A's testGarminConnectAccountEpoch.cpp:286/342... | accept-with-note | — ↗ |
| B-R017-07 | B REQ-017 | informational | MainWindow::uploadCloud (MainWindow.cpp:2555) leaks its... | open | B-R017-06 ↗ |
| B-R017-08 | B REQ-017 | informational | Re-entrancy the builder reasoned about but did not test... | accept-with-note | A3-R017 ↗ |
| B-R017-05 | B REQ-017 | informational | TEST-060 slot a1... | accept-with-note | — ↗ |
| ORCH-002 | Orch 08-05 | non-blocking | The QGDW package update re-narrowed the anti-duplication... | FIXED | — ↗ |
| ORCH-003 | Orch 08-05 | non-blocking | The new installhook.py dropped copylint() and... | FIXED | — ↗ |
| ORCH-004 | Orch 08-05 | non-blocking | The DEC-015 lint's canonical source lives in vendor... | FIXED | 9162da7d1 ↗ |
| O-R025-01 | Orch DEC-025 08-05 | non-blocking | Declining to delete the store is necessary but NOT... | FIXED | DEC-025 ↗ |
| B-R025-01 | B DEC-025 08-06 | non-blocking | The dialog's CONSTRUCTOR is still unprotected.... | FIXED | DEC-026,TEST-075,A3-R025-F1 ↗ |
| A3-R025-F1 | A3rc DEC-025 | BLOCKING | The constructor is the open route.... | FIXED | DEC-026,TEST-075 ↗ |
| A3-R025-F2 | A3rc DEC-025 | informational | syncNext's and downloadNext's QPointer self-guards are... | accept-with-rationale | TEST-076 ↗ |
| A3-R025-F4 | A3rc DEC-025 | advisory | TEST-072/073 model parent teardown as a direct delete... | accept-with-note | — ↗ |
| A3-R025-F3 | A3rc DEC-025 | REFUTED | A3-R017b-F2's deferral (writeFile symmetry) is safe... | closed | DEC-025 ↗ |
| A3-R025-F5 | A3rc DEC-025 | REFUTED | The shared-dialog regression surface (ordinary/idle... | closed | — ↗ |
| B-R025-02 | B DEC-025 08-06 | informational | syncNext/downloadNext's QPointer self-guards are... | accept-with-rationale | A3-R025-F2,DEC-026 ↗ |
| A3-R026-F1 | A3rc DEC-026 | BLOCKING | A stack CloudServiceSyncDialog parented to a... | FIXED | DEC-027,TEST-078 ↗ |
| A3-R026-F1a | A3rc DEC-026 | informational | The test file encodes the misconception that exempts the... | accept-with-note | DEC-027 ↗ |
| A3-R026-F2 | A3rc DEC-026 | non-blocking | Only the guard after store->open()... | FIXED | DEC-027,TEST-077 ↗ |
| A3-R026-F3 | A3rc DEC-026 | advisory | start() has no re-entrancy guard and no "was start()... | accept-with-note | DEC-027 ↗ |
| A3-R026-F4 | A3rc DEC-026 | informational | The stack caller's open-failure start() posts a queued... | accept-with-note | — ↗ |
| A3-R027-CLOSURE | A3rc DEC-027 | informational | Fresh adversary verified: all FIVE routes closed for... | closed | — ↗ |
| A3-R027-F1 | A3rc DEC-027 | BLOCKING | The UPLOAD path reproduces the entire pre-DEC-024..027... | DEFERRED | REQ-019 ↗ |
| A3-R027-F2 | A3rc DEC-027 | BLOCKING | AddCloudWizard::AddAuth::doAuth()... | DEFERRED | REQ-020 ↗ |
| A3-R027-F3 | A3rc DEC-027 | non-blocking | AddSettings::browseFolder() (AddCloudWizard.cpp:784-813)... | accept-with-note | LSN-042 ↗ |
| A3-R027-F4 | A3rc DEC-027 | informational | The raw testGarminConnectSyncDialogClose binary exits 1... | accept-with-note | — ↗ |
| ORCH-005 | Orch 08-07 | BLOCKING-for-clean-build | src/Resources/application.qrc embeds 13 translations/.qm... | fixed | — ↗ |
| ORCH-006 | Orch 08-07 | BLOCKING-for-clean-build | 11 TRACKED sources were missing from the CMake source lists | fixed | — ↗ |
| ORCH-009 | Orch REQ-019 08-08 | non-blocking | Blocking the REQ-019 docs commit, the lint reported two... | OPEN | — ↗ |
| A3-R019-F1 | A3 REQ-019 08-08 | BLOCKING | CloudServiceUploadDialog is parented to the MainWindow | DEFERRED | REQ-021,REQ-019 ↗ |
| A3-R019-F2 | A3 REQ-019 08-08 | BLOCKING-class | The identical F1 gap exists in the sync dialog this... | DEFERRED | REQ-021,A3-R027-CLOSURE ↗ |
| A3-R019-F3 | A3 REQ-019 08-08 | non-blocking | TEST-079 guard-B's comment (and DEC-029's text) says the... | OPEN | B-R019-05,LSN-047 ↗ |
| A3-R019-F4 | A3 REQ-019 08-08 | cosmetic | DEC-029 fact 1 ("no user-initiated close can race these... | OPEN | — ↗ |
| A3-R019-F5 | A3 REQ-019 08-08 | non-blocking | 7 of the 9 new slots hand-copy CloudService::upload()'s... | OPEN | — ↗ |
| A3-R019-F6 | A3 REQ-019 08-08 | informational | CloudDBCommon::replyReceivedAndOk... | OPEN | REQ-020 ↗ |
| B-R019-01 | B REQ-019 08-08 | non-blocking | The upload-failure branch previously did hide() +... | ACCEPTED | — ↗ |
| B-R019-02 | B REQ-019 08-08 | non-blocking | On the failure path CloudService::upload returns false... | OPEN | REQ-019 ↗ |
| B-R019-03 | B REQ-019 08-08 | cosmetic | CloudServiceUploadDialog::exec()'s else branch (status... | OPEN | — ↗ |
| B-R019-04 | B REQ-019 08-08 | non-blocking | ImportSeamStubs' RideItem constructor leaves isdirty... | OPEN | — ↗ |
| B-R019-05 | B REQ-019 08-08 | non-blocking | REQ-019's criterion demands zero UAF "on the dialog... | OPEN | — ↗ |
| B-R019-06 | B REQ-019 08-08 | non-blocking | The compressRide/writeFile pair carries ONE self-bail... | ACCEPTED | LSN-022 ↗ |
| O-R019-01 | Orch DEC-029 08-08 | non-blocking | DEC-029 as first written (a) named... | FIXED | LSN-045 ↗ |
| ORCH-008 | Orch DEC-029 08-07 | non-blocking | Generic CloudService lifetime tests are being written... | ACCEPTED | DEC-029 ↗ |
| ORCH-007 | Orch 08-07 | BLOCKING-for-clean-build | CMAKECXXEXTENSIONS OFF (root CMakeLists.txt:8... | fixed | — ↗ |
| S-R021-01 | Scout DEC-030 08-10 | blocking BLOCKS: {TASK:REQ-021} | CloudServiceSyncDialog::start()'s dirty-rides Cancel... | fixed | ORCH-050,REQ-021 ↗ |
| S-R021-02 | Scout DEC-030 08-10 | blocking BLOCKS: {TASK:REQ-021} | REQ-021's stub names only the two start() deref sites.... | fixed | ORCH-050,REQ-021,DEC-030 ↗ |
| S-R021-03 | Scout DEC-030 08-10 | blocking BLOCKS: {TASK:REQ-020} | AddCloudWizard holds Context context... | fixed | REQ-020,T-161,T-163,LSN-041 ↗ |
| S-R021-04 | Scout DEC-030 08-10 | blocking BLOCKS: {TASK:REQ-022} | OpenData (OpenData.h:35,61 — a QThread) holds Context... | fixed | REQ-022,T-164,T-166,DEC-043 ↗ |
| S-R021-05 | Scout DEC-030 08-10 | blocking BLOCKS: {TASK:REQ-023} | CloudService itself holds Context context... | fixed | REQ-023,T-167,T-169,DEC-030 ↗ |
| S-R021-06 | Scout DEC-030 08-10 | informational | Six classes match the predicate (holds Context, blocks)... | OPEN | — ↗ |
| O-R021-01 | Orch REQ-021 08-10 | non-blocking | The briefing asserted "MainWindow::switchAthleteTab... | CLOSED | TEST-081,LSN-034 ↗ |
| B-R021-01 | B REQ-021 08-10 | non-blocking | DEC-030 authorised a QObject-level load (isWidgetType())... | OPEN | B-R019-05 ↗ |
| B-R021-02 | B REQ-021 08-10 | non-blocking | probeContext.metadataFlush() and... | OPEN | LSN-047 ↗ |
| B-R021-03 | B REQ-021 08-10 | non-blocking | sizeof(Context)-based ASan poison queries assume new... | OPEN | — ↗ |
| B-R021-04 | B REQ-021 08-10 | informational | TEST-081's verdict was measured under the offscreen QPA... | ACCEPTED | DEC-030 ↗ |
| O-R021-02 | Orch REQ-021 08-11 | blocking | Context::tab is declared at src/Core/Context.h:135 and... | FIXED | — ↗ |
| B-R021-05 | B REQ-021 08-11 | refuted | Mutation M8: reverting the seven other widened bails... | PARTIALLY | — ↗ |
| B-R021-06 | B REQ-021 08-11 | non-blocking | runCtorTeardown (TEST-075) and runUploadTeardown... | OPEN | — ↗ |
| B-R021-07 | B REQ-021 08-11 | non-blocking | The 5-second watchdogs were armed on qApp with a pointer... | FIXED | — ↗ |
| B-R021-08 | B REQ-021 08-11 | non-blocking | TEST-085(b) measures the UPLOAD dialog only (exec()... | OPEN | B-R021-04,TEST-081 ↗ |
| B-R021-09 | B REQ-021 08-11 | non-blocking | Comments throughout CloudService.cpp and the test file... | OPEN | ORCH-009 ↗ |
| A3-R021-F1 | A3 REQ-021 08-11 | BLOCKING BLOCKS: {TASK:REQ-021} | completedRead (CloudService.cpp:1989→:1991), failedRead... | fixed | ORCH-050,REQ-021,S-R021-02 ↗ |
| A3-R021-F2 | A3 REQ-021 08-11 | BLOCKING BLOCKS: {TASK:REQ-021} | Adversary re-pointed an existing slot's suspension from... | fixed | ORCH-050,REQ-021 ↗ |
| A3-R021-F3 | A3 REQ-021 08-11 | non-blocking | Standalone Qt 6.8.2 probe under offscreen QPA on a... | OPEN | — ↗ |
| A3-R021-F4 | A3 REQ-021 08-11 | non-blocking | ~CloudServiceSyncDialog's DEC-025 comment... | OPEN | DEC-025,DEC-030 ↗ |
| A3-R021-F5 | A3 REQ-021 08-11 | non-blocking | REQ-021's criterion says "each new guard ... must be... | OPEN | B-R021-05 ↗ |
| A3-R021-F6 | A3 REQ-021 08-11 | non-blocking | The elaborated acceptance names FIVE sync suspension... | OPEN | — ↗ |
| A3-R021-F7 | A3 REQ-021 08-11 | informational | The criterion demanded zero UAF on context->mainWindow.... | RESOLVED | B-R019-05,B-R021-01 ↗ |
| A3-R021-F8 | A3 REQ-021 08-11 | informational | No test executes downloadClicked, syncNext... | OPEN | — ↗ |
| A3-R021-F9 | A3 REQ-021 08-11 | informational | The destructor and sync-ctor blocks still say "parented... | OPEN | B-R021-09 ↗ |
| A3-R021-F10 | A3 REQ-021 08-11 | informational | AthleteTab.cpp:129-137 does not null context->tab, so... | OPEN | O-R021-02 ↗ |
| A3-R021-F11 | A3 REQ-021 08-11 | informational | reinterpretcast<AthleteTab>(QWidget) at 5 sites works... | OPEN | B-R021-06 ↗ |
| S-R031-01 | Scout DEC-031 08-11 | blocking BLOCKS: {TASK:REQ-024} | Strava::readFileCompleted (Strava.cpp:520-531) is a... | fixed | REQ-024,T-170,T-172,DEC-040 ↗ |
| O-R021-03 | Orch DEC-031 08-11 | non-blocking | (1) ID error: the briefing cited "DEC-017 (b)/(e) store... | CLOSED | LSN-034 ↗ |
| O-R021-04 | Orch REQ-021 08-11 | non-blocking | The briefing told the builder to DROP all seven... | CLOSED | — ↗ |
| B-R021-10 | B REQ-021 08-11 | blocking BLOCKS: {TASK:REQ-025} | syncNext (CloudService.cpp:1919) and uploadNext (:2185)... | closed | 37710370e,REQ-025,S-R031-01 ↗ |
| B-R021-11 | B REQ-021 08-11 | blocking BLOCKS: {TASK:REQ-025} | RideFileFactory::openRideFile on a .fit payload runs a... | fixed | ORCH-050,REQ-025 ↗ |
| B-R021-12 | B REQ-021 08-11 | blocking BLOCKS: {TASK:DEC-031} | At sync :1270, when refreshClicked() has bailed on a... | fixed | ORCH-050,DEC-031 ↗ |
| B-R021-13 | B REQ-021 08-11 | informational | (a) TEST-087 invokes... | OPEN | — ↗ |
| ORCH-010 | Orch REQ-021 08-11 | non-blocking | Reproduced in ISOLATION (2-line scratch file): NEXTGATE... | OPEN | LSN-034 ↗ |
| ORCH-013 | Orch 08-12 | non-blocking | The WIKI MAP instructs re-running installhook.py... | CLOSED | — ↗ |
| ORCH-011 | Orch REQ-021 08-11 | informational | cd <scratchpad> && … && cat > STATE.md was DENIED... | CLOSED | — ↗ |
| B-R031-01 | B DEC-031 08-12 | BLOCKING | The reaper fires when the LAST counted BlockingCall... | RESOLVED | REQ-025,DEC-031 ↗ |
| ORCH-012 | Orch DEC-031 08-12 | non-blocking | LSN-032/LSN-038 mandate cp file file.orig → mutate → cp... | OPEN | — ↗ |
| B-R025-01 | B REQ-025 08-12 | blocking | RideFileFactory::openRideFile dereferences the Context... | FIXED | DEC-041,REQ-029,T-173,TEST-092 ↗ |
| B-R025-02 | B REQ-025 08-12 | informational | RideCache::~RideCache() does not delete its RideItems... | OPEN | — ↗ |
| B-R025-03 | B REQ-025 08-12 | non-blocking | syncNext :2012 (download branch), downloadNext :2144... | OPEN | DEC-031 ↗ |
| B-R025-04 | B REQ-025 08-12 | non-blocking | uploadDialogExecStillBlocksTheWholeApplication armed its... | FIXED | B-R021-07 ↗ |
| A3-R021b-F1 | A3 REQ-021 08-12 | BLOCKING | CloudServiceSyncDialog::uploadNext is defined at... | FIXED | REQ-026,TEST-093,TEST-087 ↗ |
| O-R021-05 | Orch A3-R021b 08-12 | informational | The adversary framed F1 entirely around... | CONFIRMED | A3-R021b-F1 ↗ |
| A3-R021b-F2 | A3 REQ-021 08-12 | blocking | RideFileFactory::openRideFile's tail derefs... | FIXED | B-R025-01,REQ-021,REQ-024 ↗ |
| A3-R021b-F3 | A3 REQ-021 08-12 | non-blocking | completedRead is defined at :2175; its single aborted... | FIXED | REQ-026,TEST-094 ↗ |
| ORCH-014 | Orch A3-R021b 08-12 | non-blocking | This file's header (:3) defines Check 5 as grep '/... | OPEN | ORCH-010,A3-R021b ↗ |
| B-R026-01 | B REQ-026 08-12 | blocking BLOCKS: {TASK:REQ-027} | syncNext's parse-failure branch... | fixed | ORCH-050,REQ-027 ↗ |
| B-R027-06 | B REQ-027 08-14 | non-blocking | Its mutation script classified an ASan abort as... | CLOSED | — ↗ |
| B-R027-07 | B REQ-027 08-14 | non-blocking | TEST-102's completedRead site used a .gcblock payload.... | CLOSED | — ↗ |
| B-R027-08 | B REQ-027 08-14 | non-blocking | The download list's column-1 header is "Workout Name"... | CLOSED | TEST-102 ↗ |
| B-R027-09 | B DEC-033 09-04 | blocking | DEC-033's new syncNext/downloadNext branches... | CLOSED | TEST-076 ↗ |
| ORCH-015 | Orch REQ-027 08-14 | blocking | The lint (scripts/ledgerdriftlint.py) failed the tree on... | FIXED | — ↗ |
| ORCH-016 | Orch 08-15 | non-blocking | Four residuals, all confirmed by the orchestrator's own... | OPEN | — ↗ |
| A3-R027b-F1 | A3rc REQ-027 08-15 | BLOCKING | Found by the adversary, then reproduced by the... | FIXED | — ↗ |
| ORCH-017 | Orch A3-R027b 08-15 | non-blocking | (i) THE SAME LATENT SHAPE SURVIVES IN TWO FIXTURES NOT... | OPEN | — ↗ |
| ORCH-021 | Orch 08-16 | blocking | The docs commit aborted on four violations... | FIXED | b1e4c4fad ↗ |
| ORCH-022 | Orch DEC-034 08-16 | non-blocking | The <cycle>-<seq> convention derives the cycle token... | OPEN | — ↗ |
| B-R028-03 | B REQ-028 08-18 | blocking | The executed proof is that TEST-107 is STILL GREEN after... | CLOSED | DEC-036,DEC-034,DEC-031 ↗ |
| B-R028-04 | B REQ-028 08-18 | non-blocking | Self-disclosed by the builder, which flagged it as... | OPEN | — ↗ |
| B-R028-05 | B REQ-028 08-18 | blocking BLOCKS: {TASK:REQ-028} | Identical under both offscreen and minimal. Open... | accepted-with-rationale | REQ-028,DEC-034 ↗ |
| B-R028-06 | Orch REQ-028 08-18 | blocking | Orchestrator-derived while auditing the builder's note 2... | CLOSED | DEC-036 ↗ |
| A3-R028b-F1 | A3rc REQ-028 08-19 | BLOCKING | Orchestrator-confirmed at every link rather than... | CLOSED | — ↗ |
| A3-R028b-F2 | A3rc REQ-028 08-19 | BLOCKING | Orchestrator-confirmed: :2426 bar, :2428 processEvents ←... | CLOSED | — ↗ |
| A3-R028b-F3 | A3rc REQ-028 08-19 | BLOCKING | Orchestrator-confirmed at all four cited lines: both... | CLOSED | DEC-037,TEST-115 ↗ |
| A3-R028b-F4 | A3rc REQ-028 08-19 | non-blocking | Verified as stated: save() occurs at exactly two places... | OPEN | — ↗ |
| A3-R028b-F5 | A3rc REQ-028 08-19 | non-blocking | testGarminConnectSyncDialogClose.cpp:10727-10728 if... | CLOSED | TEST-120 ↗ |
| A3-R028b-F6 | A3rc REQ-028 08-19 | informational | Guards at :2146/:2510/:2965; contract at... | OPEN | — ↗ |
| A3-R028b-F7 | A3rc REQ-028 08-19 | non-blocking | :1516 slot; takes+deletes children at :1570-1588 BEFORE... | OPEN | — ↗ |
| A3-R028b-F8 | A3rc REQ-028 08-19 | non-blocking | testGarminConnectSyncDialogClose.cpp:9541-9544 and the... | CLOSED | — ↗ |
| ORACLE-F1 | TEST 08-21 | BLOCKING | Fuzzer-EXECUTED, reproducible in isolation... | resolved | ORCH-050 ↗ |
| A3-R028c-F1 | A3rc REQ-028 08-21 | BLOCKING | Orchestrator-confirmed at the code: refreshClicked... | resolved | ORCH-050 ↗ |
| A3-R028c-F2 | A3rc REQ-028 08-21 | BLOCKING | Orchestrator-confirmed at the code: completedWrite... | resolved | ORCH-050 ↗ |
| A3-R028c-F3 | A3rc REQ-028 08-21 | BLOCKING | CloudService.h says a dropped ticket degrades to the... | RESOLVED | DEC-036 ↗ |
| A3-R028c-F4 | A3rc REQ-028 08-21 | informational | `decisions.md:1384-1387` vs `CloudService.cpp:3453` and... | OPEN | B-R028-15 ↗ |
| A3-R028c-F5 | A3rc REQ-028 08-21 | informational | Established by exhaustion over all four mutators... | OPEN | — ↗ |
| A3-R028c-F6 | A3rc REQ-028 08-21 | informational | Cited lines | OPEN | LSN-073,A3-R028b-F1 ↗ |
| A3-R028c-F7 | A3rc REQ-028 08-21 | non-blocking | ImportSeamStubs.cpp:135/:454-465 arms an action inside... | scheduled | T-140 ↗ |
| A3-R028c-F8 | REQ REQ-028 08-22 | BLOCKING | A3-R028c-F2 measures two transfers on the wire while the... | RESOLVED | DEC-036,DEC-037 ↗ |
| ORCH-032 | REQ REQ-028 08-22 | non-blocking | DEC-034/036 amendment claimed only three batchGeneration... | fix-now | DEC-036 ↗ |
| ORCH-033 | REQ REQ-028 08-22 | BLOCKING | heap-use-after-free READ of size 1 at... | fix-now | e48f7d123 ↗ |
| B-R028-15 | B DEC-037 08-20 | refuted | Self-disclosed. The retired scan labels with result... | CLOSED | — ↗ |
| B-R028-16 | B DEC-037 08-20 | refuted | Self-disclosed. The builder re-argued in place rather... | CLOSED | — ↗ |
| B-R028-13 | B A3-R028b-F1 08-19 | non-blocking | Measured, both backends: the BlockingCall around... | SUPERSEDED | A3-R028c-F7 ↗ |
| B-R028-14 | B A3-R028b-F1 08-19 | non-blocking | The fix guards what remains: the row LABEL, successful... | A3-RATIFIED | — ↗ |
| B-R028-12 | B A3-R028-F5 08-19 | non-blocking | Self-disclosed, and it corrected the briefing that sent... | OPEN | — ↗ |
| ORCH-031 | Orch A3-R028-F5 08-19 | non-blocking | Orchestrator-verified after the report, at the builder's... | CORRECTED | ORCH-028 ↗ |
| B-R028-11 | B DEC-036 08-19 | non-blocking | Self-disclosed. The premises are what stop it being... | OPEN | — ↗ |
| ORCH-030 | Orch DEC-036 08-19 | non-blocking | Orchestrator-executed, so this is not taken from the... | CORRECTED | — ↗ |
| A3-R028-F1 | A3 REQ-028 08-18 | BLOCKING | EXECUTED by the adversary: AddressSanitizer... | CLOSED | DEC-036 ↗ |
| A3-R028-F2 | A3 REQ-028 08-18 | BLOCKING | EXECUTED: bar 0→1 with downloadtotal 0 (max 3), row 0... | CLOSED | TEST-118,A3-R028-F1 ↗ |
| A3-R028-F3 | A3 REQ-028 08-18 | non-blocking | EXECUTED: CompletedWritePE syncListCount = 1, versus... | CLOSED | — ↗ |
| A3-R028-F4 | A3 REQ-028 08-18 | non-blocking | EXECUTED grep: replyName() never returns "" for a write... | CLOSED | — ↗ |
| A3-R028-F5 | A3 REQ-028 08-18 | blocking | Reasoned, not executed (the A3 labelled it so itself).... | CLOSED | — ↗ |
| A3-R028-F6 | A3 REQ-028 08-18 | informational | EXECUTED grep: LocalFileStore appears in unittests/ only... | OPEN | — ↗ |
| A3-R028-F7 | A3 REQ-028 08-18 | informational | `CloudService.cpp:1526`; the claim at `CloudService.h:719` | OPEN | — ↗ |
| B-R028-07 | B DEC-036 08-18 | blocking | downloadClicked runs straight from its start branch to... | CLOSED | A3-R028-F1 ↗ |
| B-R028-08 | B DEC-036 08-18 | non-blocking | inflight.isWrite in all three completion slots, plus the... | OPEN | LSN-063 ↗ |
| B-R028-09 | B DEC-036 08-18 | non-blocking | (a) TEST-087 broke because its apparatus FABRICATED... | OPEN | TEST-087 ↗ |
| B-R028-10 | B DEC-036 08-18 | non-blocking | Self-disclosed. Reaching it requires a service to... | OPEN | — ↗ |
| ORCH-028 | Orch DEC-036 08-18 | non-blocking | Orchestrator-verified by execution rather than accepted... | CORRECTED | LSN-062 ↗ |
| ORCH-029 | Orch DEC-036 08-18 | non-blocking | Found by the residue sweep in this gate (find . -name... | OPEN | LSN-032 ↗ |
| ORCH-027 | Orch DEC-036 08-18 | non-blocking | Verified by Grep while assembling the DEC-036 briefing... | CORRECTED | B-R028-03 ↗ |
| ORCH-026 | Orch REQ-028 08-17 | non-blocking | DEC-034 is accepted — decisions.md:1160-1161 reads... | CORRECTED | — ↗ |
| B-R028-01 | B REQ-028 08-16 | blocking BLOCKS: {TASK:REQ-028} | Chosen for TEST-102's stated reason (.gcfail runs no... | fixed | ORCH-050,REQ-028 ↗ |
| B-R028-02 | B REQ-028 08-16 | non-blocking | Self-flagged in the slot's own header ("MEASURED, NOT... | ACCEPTED | TEST-107 ↗ |
| ORCH-025 | Orch REQ-028 08-16 | non-blocking | The Phase-1 briefing said the F8 probe "needs a... | CLOSED | — ↗ |
| S-R028-01 | Scout DEC-034 08-16 | blocking BLOCKS: {TASK:REQ-028} | Orchestrator-confirmed by grepping every... | fixed | ORCH-050,REQ-028 ↗ |
| ORCH-034 | Orch DEC-038 08-23 | non-blocking | MY BRIEFING CARRIED TWO REQUIREMENTS THAT CANNOT BOTH... | resolved | TEST-112 ↗ |
| B-R038-01 | B DEC-038 08-23 | non-blocking | A QT LANDMINE UNDER THE FIX: a REDUNDANT... | not-a-defect | — ↗ |
| B-R038-02 | B DEC-038 08-23 | informational | TWO ADDED CONTROLS SURVIVE INDIVIDUAL MUTATION AND SHIP... | accept-with-note | — ↗ |
| B-R038-03 | B DEC-038 08-23 | non-blocking | **THREE ACCEPTED RESIDUALS ON THE RESTORE PATH... | accepted | — ↗ |
| S-R028-02 | Scout DEC-038 08-22 | blocking | THE SORT ROUTE NEEDS NO CLICK AT ALL — the drivers' own... | fixed | 514d8e88f,DEC-038 ↗ |
| ORCH-024 | Orch DEC-034 08-16 | non-blocking | The briefing's second paragraph said "Verify that md5... | CLOSED | — ↗ |
| ORCH-023 | Orch DEC-034 08-16 | advisory | git stash list → a single entry, stash@{0}: On master... | OPEN | — ↗ |
| ORCH-020 | Orch A3-R027c 08-15 | non-blocking | Beyond ORCH-018's :2274, the DEC-035 comment block... | FIXED | — ↗ |
| ORCH-019 | Orch A3-R027c 08-15 | non-blocking | Cycles absent from the table but real, evidenced... | OPEN | — ↗ |
| A3-R027c-F1 | A3rc DEC-035 08-15 | Non-blocking | Every one of the four call sites into... | OPEN | DEC-035 ↗ |
| A3-R027c-F2 | A3rc DEC-035 08-15 | Non-blocking | The TEST-105/106 block comment says "The realistic... | OPEN | ORCH-018 ↗ |
| A3-R027c-F3 | A3rc DEC-035 08-15 | Non-blocking | TEST-105/106 carry the diagnostic labels "syncNext:2011"... | OPEN | LSN-034 ↗ |
| ORCH-018 | Orch A3-R027c 08-15 | non-blocking | The new downloadNext guard comment states that the label... | OPEN | A3-R027c ↗ |
| A3-R027b-F2 | A3rc REQ-027 08-15 | blocking | completionSlotsStandDownWhenTheAthleteTabDiesInTheirProce... | FIXED | — ↗ |
| A3-R027-F1 | A3 REQ-027 08-14 | BLOCKING BLOCKS: {TASK:REQ-027} | syncNext:2121's if (self.isNull()) return true; sat... | fixed | ORCH-050,REQ-027 ↗ |
| A3-R027-F2 | A3 REQ-027 08-14 | BLOCKING BLOCKS: {TASK:REQ-028} | refreshClicked (:1516) deletes every QTreeWidgetItem in... | fixed | ORCH-050,REQ-028,DEC-025 ↗ |
| A3-R027-F4 | A3 REQ-027 08-14 | blocking BLOCKS: {TASK:REQ-027} | Nothing reads aborted between completedRead:2349 /... | fixed | ORCH-050,REQ-027 ↗ |
| A3-R027-F3 | A3 REQ-027 08-14 | blocking BLOCKS: {TASK:REQ-028} | refreshClicked (:1516) deletes and rebuilds all three... | fixed | ORCH-050,REQ-028,REQ-027 ↗ |
| A3-R027-F8 | A3 REQ-027 08-14 | blocking BLOCKS: {TASK:REQ-028} | The batchGeneration header comment names... | fixed | ORCH-050,REQ-028,DEC-034 ↗ |
| A3-R027-F5/F6/F7/F9 | A3 REQ-027 08-14 | non-blocking | F5 — mutation M11 (delete delete ride; at :2089, in the... | OPEN | — ↗ |
| B-R027-01 | B REQ-027 08-13 | BLOCKING BLOCKS: {TASK:DEC-033} | The briefing (and the scout's F1, and O-R027-01 as first... | fixed | O-R027-01,REQ-027,DEC-033 ↗ |
| B-R027-02 | B REQ-027 08-13 | blocking BLOCKS: {TASK:DEC-033} | syncNext:1985 and downloadNext:2127 allocate new... | fixed | DEC-033,DEC-022,A3-R017-F3 ↗ |
| B-R027-03 | B REQ-027 08-13 | blocking BLOCKS: {TASK:DEC-033} | The orchestrator specified a fixture flag... | fixed | DEC-033 ↗ |
| B-R027-04 | B REQ-027 08-13 | non-blocking | uploadNext's parse-failure branch does NOT advance the... | open | S-R027-04 ↗ |
| B-R027-05 | B REQ-027 08-13 | non-blocking | Criterion (a) asks for downloading == false; downloading... | ACCEPT | — ↗ |
| S-R027-01 | Scout DEC-032 08-13 | blocking BLOCKS: {TASK:REQ-026} | REQ-026 added the abort re-read to the parse-failure... | fixed | ORCH-050,REQ-026,LSN-046 ↗ |
| S-R027-02 | Scout DEC-032 08-13 | informational | LocalFileStore::writeFile emits writeComplete AND... | ACCEPT-AND-DOCUMENT | DEC-032,O-R027-01 ↗ |
| S-R027-03 | Scout DEC-032 08-13 | informational | LocalFileStore::readFile emits readComplete... | INFORMATIONAL | DEC-032 ↗ |
| S-R027-04 | Scout DEC-032 08-13 | cosmetic | Neither parse-failure branch (syncNext:2067-2071... | OPEN | REQ-027 ↗ |
| O-R027-01 | Orch REQ-027 08-13 | blocking BLOCKS: {TASK:DEC-033} | Both syncNext (CloudService.cpp:1996) and downloadNext... | fixed | ORCH-050,DEC-033 ↗ |
| B-R026-02 | B REQ-026 08-12 | non-blocking | GlobalContext::context()->rideMetadata is nullptr in... | ACCEPT-WITH-NOTE | — ↗ |
| B-R026-03 | B REQ-026 08-12 | non-blocking | TEST-094: saveRide has no real side effect in this... | ACCEPT-WITH-NOTE | TEST-094,LSN-047,LSN-050 ↗ |
| B-R026-04 | B REQ-026 08-12 | non-blocking | TEST-093's row[0] uses .tcx, deliberately: a missing... | ACCEPT-WITH-NOTE | — ↗ |
| B-R026-05 | B REQ-026 08-12 | non-blocking | rideListUp has setSortingEnabled(true) (:1146), and... | ACCEPT-WITH-NOTE | — ↗ |
| B-R026-06 | B REQ-026 08-12 | non-blocking | TEST-093 drives the downloadClicked() Abort-button route... | ACCEPT | — ↗ |
| O-R021-06 | Orch REQ-026 08-12 | non-blocking | The briefing (and the orchestrator's message to the... | CONFIRMED | LSN-034 ↗ |
| A3-R038-F1 | A3 DEC-038 08-23 | non-blocking BLOCKS: {} | deferCloseIfBusy() is a FOURTH dialog-terminating class... | ACCEPT-WITH-NOTE | — ↗ |
| A3-R038-F2 | A3 DEC-038 08-23 | non-blocking BLOCKS: {} | restoreListSorting's per-list enabled bit is UNTESTED BY... | SCHEDULED | T-142,B-R038-02 ↗ |
| A3-R038-F3 | A3 DEC-038 08-23 | informational BLOCKS: {} | STALE LINE CITATIONS IN THE DEC-034 COMMENT BLOCK — an... | CONFIRMED | LSN-034 ↗ |
| ORCH-035 | Orch DEC-038 08-23 | informational | process defect — the orchestrator's briefing SWAPPED the... | CONFIRMED | LSN-034 ↗ |
| ORCH-036 | Orch DEC-038 08-23 | non-blocking BLOCKS: {} | CORRECTED 2026-08-23, SAME DAY, BEFORE ANY WORK WAS... | open | — ↗ |
| ORCH-037 | Orch 08-23 | non-blocking | THE lessons.md "INDEX HEAD IS THE HOT READ" CLAIM IS... | OPEN | — ↗ |
| ORCH-038 | Orch 08-23 | non-blocking | 25 ALLOCATED TEST IDS HAVE NO CITATION ANYWHERE IN... | OPEN | — ↗ |
| ORCH-039 | Orch 08-23 | informational | THE SKILL'S OWN TIER-0/TIER-1 BUDGETS ARE MUTUALLY... | RATIFIED | — ↗ |
| B-R028-17 | B REQ-028 08-23 | blocking BLOCKS: {} | Self-disclosed by the builder as REPORT-ONLY, then... | CLOSED | DEC-042,TEST-158,TEST-141 ↗ |
| B-R028-18 | B REQ-028 08-23 | non-blocking BLOCKS: {} | SIX SELF-DISCLOSED EVIDENCE RESIDUALS ON THE TWO NEW... | ACCEPT-WITH-NOTE | A3-R028e,B-R038-02,LSN-050 ↗ |
| B-R028-19 | B REQ-028 08-23 | informational BLOCKS: {} | PREMISE REWRITTEN ACCURATELY — the original wording... | OPEN | TEST-141,LSN-070 ↗ |
| A3-R028e-F1 | A3 REQ-028 08-24 | blocking BLOCKS: {} | A SECOND, DISTINCT LIFETIME DEFECT —... | CLOSED | DEC-043,TEST-159,TEST-160 ↗ |
| A3-R028e-F2 | A3 REQ-028 08-24 | blocking BLOCKS: {CHECKPOINT:REQ-028 clause-(e) slice} | TEST-140 IS GREEN-BUT-VACUOUS ON ITS OWN CENTRAL PREMISE... | FIXED | REQ-028,A3-R028e,B-R028-18 ↗ |
| A3-R028e-F3 | A3 REQ-028 08-24 | non-blocking BLOCKS: {} | TEST-142's "MUTATION-KILLED TWICE, INDEPENDENTLY"... | FIXED | — ↗ |
| ORCH-040 | Orch A3-R028e 08-24 | non-blocking BLOCKS: {} | THE ANTI-DUPLICATION GUARD BLOCKS THE MUTATION-RESTORE... | OPEN | — ↗ |
| ORCH-041 | Orch 08-24 | non-blocking BLOCKS: {} | THE REPAIRED CHECK-5 NEEDS-DISPOSITION BUCKET DOES NOT... | OPEN | B-R028-17 ↗ |
| ORCH-042 | Scout 08-25 | non-blocking BLOCKS: {} | THE terminate() PROHIBITION IS STALE IN ONE PLACE AND... | OPEN | — ↗ |
| S-R028f-F1 | Sweep 08-26 | non-blocking BLOCKS: {} | NINE OF THE SEVENTEEN REACHABLE CloudService PROVIDERS... | OPEN | — ↗ |
| S-R028f-F2 | Sweep 08-26 | non-blocking BLOCKS: {} | AN EXTERNAL QThread::quit() CANNOT BE USED TO BOUND THE... | OPEN | S-R028f-F1 ↗ |
| S-R028f-F3 | Sweep 08-26 | non-blocking BLOCKS: {} | CyclingAnalytics ARMS ITS ADVERTISED 30-SECOND TIMEOUT... | OPEN | REQ-028 ↗ |
| C1 | Census 08-26 | blocking BLOCKS: {} | CORRECT ARMING IS NOT SUFFICIENT, AND THE ONLY... | CLOSED | S-R028f-F1 ↗ |
| C2 | Census 08-26 | blocking BLOCKS: {} | Strava::addSamples IS AN UNBOUNDED NESTED WAIT INSIDE... | CLOSED | — ↗ |
| C3 | Census 08-26 | blocking BLOCKS: {} | Xert::readActivityDetail IS AN UNBOUNDED WAIT EXECUTED... | CLOSED | C2 ↗ |
| C4 | Census 08-26 | blocking BLOCKS: {} | NO PROVIDER ABORTS OR DELETES ANY QNetworkReply — THERE... | CLOSED | — ↗ |
| C5 | Census 08-26 | non-blocking BLOCKS: {} | SixCycle.cpp:244 DOCUMENTS ITS 10000 ms TIMEOUT AS... | OPEN | — ↗ |
| C6 | Census 08-26 | blocking BLOCKS: {} | THREE UNBOUNDED WAITS RUN ON THE GUI THREAD, OUTSIDE... | CLOSED | S-R028f-F1 ↗ |
| ORCH-043 | Orch DEC-040 08-26 | non-blocking | THE ANTI-DUPLICATION GUARD PARSED A READ-ONLY grep FLAG... | CLOSED | — ↗ |
| B-R040-01 | B DEC-040 08-27 | blocking BLOCKS: {} | CloudService::nam() SATISFIES THE LETTER OF THE AFFINITY... | FIXED | — ↗ |
| B-R040-02 | B DEC-040 08-27 | blocking BLOCKS: {} | THE T-153(a) PRE-MAIN PROBE IS ITSELF... | FIXED | B-R040-01,LSN-050 ↗ |
| A3-R040-F1 | A3 DEC-040 08-29 | non-blocking | CloudServiceAutoDownload::run() DROPPED errors ON THE... | FIXED | — ↗ |
| A3-R040-F2 | A3 DEC-040 08-29 | non-blocking BLOCKS: {} | Strava::addSamples' sample-merge loop can spin forever... | PRE-EXISTING | — ↗ |
| A3-R040-F3 | A3 DEC-040 08-29 | non-blocking | managercreatedInObjectAffinityThread had no RAII around... | FIXED | — ↗ |
| A3-R040-F4 | A3 DEC-040 08-29 | informational | The QSemaphore comment overstated the guarantee — it... | FIXED | — ↗ |
| A3-R040-DD | A3 DEC-040 08-29 | non-blocking BLOCKS: {} | blockingRequest's reply->deleteLater() (DeferredDelete)... | open | — ↗ |
| A3f-R040-A | A3f DEC-040 08-29 | non-blocking BLOCKS: {} | THE SAME errors LIST IS STILL DROPPED ONE BRANCH EARLIER... | FIXED | A3-R040-F1 ↗ |
| A3f-R040-B | A3f DEC-040 08-29 | informational BLOCKS: {} | autoDownloadErrors() HAS ZERO PRODUCTION CONSUMERS, AND... | ACCEPTED | A3-R040-F1 ↗ |
| A3f-R040-C | A3f DEC-040 08-29 | non-blocking BLOCKS: {} | "qCritical … does not abort" IS TRUE ONLY UNDER THE... | CLOSED | — ↗ |
| A3f-R040-D | A3f DEC-040 08-29 | informational BLOCKS: {} | THE F1 KILLING MUTATIONS WERE RUN SINGLE-SLOT... | ACCEPTED | — ↗ |
| A3f-R040-E | A3f DEC-040 08-29 | informational BLOCKS: {} | TEST-151's GLOBAL STATE IS NOT RAII-GUARDED — the F3... | partially | — ↗ |
| A3f-R040-F | A3f DEC-040 08-29 | informational BLOCKS: {} | WorkerGuard's !destructionObserved PATH IS THE ONE PATH... | ACCEPTED | — ↗ |
| ORCH-044 | Orch DEC-040 08-30 | non-blocking | THE ORCHESTRATOR'S REPAIR BRIEFING CITED THE WRONG CTest... | CLOSED | — ↗ |
| ORCH-045 | Orch DEC-040 08-30 | non-blocking BLOCKS: {} | THE ANTI-DUPLICATION GUARD DENIED A READ-ONLY COMMAND... | OPEN | ORCH-043 ↗ |
| ORCH-046 | Orch DEC-040 08-30 | non-blocking BLOCKS: {} | THE PROVIDER-WATCHDOG TEST GUARDS SHARED CODE BUT RUNS... | OPEN | — ↗ |
| ORCH-047 | Orch 08-30 | blocking-for-gate-trust BLOCKS: {} | THE GARMIN GATE'S VERDICT WAS A FUNCTION OF MACHINE... | FIXED | — ↗ |
| ORCH-048 | Orch 08-30 | blocking-for-spine BLOCKS: {} | TWO must REQUIREMENTS HAD NO TRACEABILITY ROW AT ALL AND... | FIXED | — ↗ |
| ORCH-049 | Orch 08-30 | blocking-for-gate-trust | CLV CHECK 5 WAS SILENTLY SKIPPING 122 OF 361 FINDING... | fixed | — ↗ |
| ORCH-050 | Orch 08-30 | blocking-for-gate-trust | THREE FINDINGS ARE SIMULTANEOUSLY "RESOLVED" AND... | resolved | — ↗ |
| ORCH-051 | Orch 08-30 | non-blocking BLOCKS: {…} BLOCKS: {RELEASE} | THE PUBLISHED NEXTGATE COULD NOT BE SATISFIED BY ITS OWN... | fixed | — ↗ |
| ORCH-052 | Peer DEC-033 09-04 | non-blocking | An independent peer session (garmincodexnew), reviewing... | OPEN | — ↗ |
| ORCH-055 | Rev DEC-040 09-05 | non-blocking | T-148's "GUI-thread default-token" ROWS DO NOT ACTUALLY... | FIXED | ORCH-053 ↗ |
| ORCH-054 | Orch DEC-040 09-05 | non-blocking | DURING THE PIECE-3B MUTATION-PROOF RESTORE, git checkout... | RESOLVED | LSN-084 ↗ |
| ORCH-056 | Orch 09-05 | non-blocking | THE ASYNC readFile REPLY LEAKS ACROSS ALL NINE MIGRATED... | OPEN | C4 ↗ |
| ORCH-057 | Orch DEC-043 09-05 | non-blocking | CloudServiceAutoDownload::run() READS... | OPEN | A3-R028e-F1,DEC-043 ↗ |
| ORCH-058 | Orch DEC-042 09-05 | non-blocking | THE antiduplicationguard.py HOOK (LSN-007) DENIES THE... | FIXED | REQ-020 ↗ |
| ORCH-059 | Orch DEC-043 09-05 | non-blocking | TEST-160 EXERCISES A HAND-MIRRORED STUB... | OPEN | DEC-043 ↗ |
| ORCH-053 | B+Orch DEC-040 09-05 | non-blocking | A PRE-EXISTING, ALREADY-"BUILT-AND-PASSING" TEST HAS A... | OPEN | T-147,LSN-083 ↗ |
| ORCH-060 | Orch 09-06 | informational | 18 full `ctest -L garmin-fast` reruns logged across this... | accept-with-note | — ↗ |
| ORCH-061 | Orch 09-06 | informational | test binary ~8,874+ lines / 87+ slots / 512-seed fuzzer... | deferred | — ↗ |
| ORCH-062 | Insp T-208 09-12 | non-blocking | THE anti_duplication_guard.py HOOK (same hook as... | OPEN | T-208,ORCH-058,LSN-036 ↗ |
| ORCH-063 | Insp 09-16 | non-blocking | Guard misses that `mkdir -p` made the dir its `cd` enters | OPEN | LSN-036,ORCH-058,ORCH-062 ↗ |
| ORCH-064 | Insp 09-19 | non-blocking BLOCKS: {} | insp_wake.sh never fired on its success path | FIXED | — ↗ |
| ORCH-065 | Insp 09-19 | non-blocking BLOCKS: {} | Wake counted the Inspector's own pane as a supervised worker | FIXED | ORCH-064 ↗ |
| ORCH-066 | Insp 09-19 | non-blocking BLOCKS: {} | dispatch.py classified every Codex reply as truncated | FIXED | B-STAGE9-25 ↗ |
| ORCH-067 | User 09-19 | non-blocking BLOCKS: {} | Arming a wake with nothing dispatched is a no-op that... | open | — ↗ |
| B-R020-01 | B REQ-020 09-06 | non-blocking | dormant: wizard death inside the STACK... | deferred | A3-R027-F3,REQ-020,LSN-041 ↗ |
| B-R020-02 | B REQ-020 09-06 | non-blocking | pre-existing: production never deletes the heap... | accept-with-note | REQ-020 ↗ |
| B-R022-01 | B REQ-022 09-06 | non-blocking | inherent residue of the sanctioned DEC-043-reduced... | accept-with-note | REQ-022,DEC-043 ↗ |
| B-R023-01 | Builder REQ-023 09-07 | non-blocking BLOCKS: {} | Dialog tail deref of freed Context (download/sync) | open | — ↗ |
| B-R023-02 | Builder REQ-023 09-07 | non-blocking BLOCKS: {} | AutoDownload readComplete derefs Context post-suspend | open | — ↗ |
| B-R024-01 | Builder REQ-024 09-08 | non-blocking BLOCKS: {} | Other async completion slots lack AsyncCompletionFrame | open | — ↗ |
| B-R029-01 | Builder REQ-029 09-08 | blocking | appendOrUpdatePoint UAF: deref point after delete | FIXED | DEC-044,T-173 ↗ |
| B-R010-01 | Reviewer REQ-010 09-10 | blocking | Controller ignored save/record failure; no pause | FIXED | T-179 ↗ |
| B-R010-02 | Reviewer REQ-010 09-10 | non-blocking | login/submit_mfa lack rate-limit/retry wrapping | FIXED | test_gc_rate.py ↗ |
| B-R010-03 | Bldr+Rev REQ-010 09-10 | non-blocking | Real lib pages internally; DES-005 can't pace per page | accept-with-note | DEC-047 ↗ |
| B-R010-04 | Bldr+Rev REQ-010 09-10 | non-blocking | Controller has no production call site or UI wiring | FIXED | GarminBackfillDialog ↗ |
| B-R010-05 | Builder REQ-010 09-10 | informational | Timestamp format mismatch; stale NotImplementedError doc | open | — ↗ |
| B-R010-06 | Builder REQ-010 09-10 | informational | New persist-fail regression test is order-dependent | deferred | T-179 ↗ |
| B-R010-07 | Reviewer UIwiring 09-10 | blocking | New Garmin includes unguarded by GC_WANT_GARMINCONNECT | FIXED | — ↗ |
| B-R010-08 | Reviewer UIwiring 09-11 | blocking | Backfill bypasses session-latch; cross-account risk | FIXED | — ↗ |
| B-R010-09 | Reviewer UIwiring 09-11 | blocking | Dialog Context UAF: raw ptr, wizard bypass, running flag | FIXED | T-194..T-196 ↗ |
| B-R010-10 | Rev final-confirm 09-11 | non-blocking | backfillSessionStillValid still hits raw Context chain | deferred | DEC-048 ↗ |
| B-R010-11 | Builder UIwiring 09-11 | informational | Wizard holds raw Context; non-empty path untested | deferred | — ↗ |
| B-R013-01 | Reviewer REQ-013 09-12 | blocking | profileFetched lambda guards raw Context, not QPointer | FIXED | — ↗ |
| B-OBS001-01 | Rev Obs-001 09-12 | non-blocking | 5 guard-reject sites omit trailing activityCount arg | FIXED | — ↗ |
| B-OBS001-02 | Rev Obs-001 09-12 | non-blocking | ObsCapture static state unsynchronized; ctor race | FIXED | — ↗ |
| B-I18N001-01 | Rev i18n-001 09-12 | blocking | tr() context mismatch: extraction vs runtime resolve | FIXED | — ↗ |
| B-I18N001-02 | Rev i18n-001 09-12 | non-blocking | Merge script left absolute paths in 5 msg blocks | FIXED | — ↗ |
| B-STAGE9-01 | Stage9 connect 09-13 | blocking BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-009, TASK:REQ-012, TASK:REQ-017} | No Py_Initialize w/o GC_WANT_PYTHON | FIXED | DEC-052,173135907 ↗ |
| B-STAGE9-02 | Rev DEC-052 09-13 | blocking | PyProcessBootstrap.cpp missing from qmake NO_PCH_SOURCES | FIXED | — ↗ |
| B-STAGE9-03 | Rev DEC-052 09-13 | blocking | Bare bootstrap call skips inittab hook on restart | FIXED | — ↗ |
| B-STAGE9-04 | Rev DEC-052 09-13 | non-blocking | Doc claims GIL always released; false for ext-init | SUPERSEDED | B-STAGE9-05 ↗ |
| B-STAGE9-05 | Rev DEC-052 fix 09-13 | blocking | PyGILState_Check fix could steal unrelated thread's GIL | FIXED | — ↗ |
| B-STAGE9-06 | Stage9 retest2 09-13 | blocking BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-009, TASK:REQ-012, TASK:REQ-017} | curl_cffi ImpersonateError: libcurl symbol clash | FIXED | 0c05f7c18 ↗ |
| B-STAGE9-07 | pch_investigator 09-13 | non-blocking | Py_SetProgramName dangling temp wstring ptr (<=3.12) | FIXED | 28958dc16 ↗ |
| B-STAGE9-08 | Rev chain 8rnd 09-13 | blocking | exceptionType field: 8 rounds of untrusted-string bypass | FIXED | — ↗ |
| B-STAGE9-09 | Stage9 retest4 09-13 | blocking BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-003} | full_name_id missing on real lib; raises unclassified | FIXED | 4cffe2835 ↗ |
| B-STAGE9-10 | Insp audit 09-13 | blocking BLOCKS: {STAGE:9, TASK:REQ-003} | MFA seam broken: sentinel unreachable, args reversed | FIXED | e5936375c ↗ |
| B-STAGE9-11 | Builder flag 09-13 | blocking BLOCKS: {STAGE:9, TASK:REQ-004} | dumps/loads missing on real lib; empty token blob | FIXED | e5936375c ↗ |
| B-STAGE9-12 | Stage9 live sync 09-13 | blocking | gc_obs lines buffered/invisible live, out of order | FIXED | T-209,a0f9045e6 ↗ |
| B-STAGE9-13 | Insp audit 09-14 | blocking | loadChecked reports 0-byte token file as Ok | fixed | 1ae49a196 ↗ |
| B-STAGE9-14 | Builder flag 09-14 | blocking BLOCKS: {STAGE:9, TASK:REQ-004, TASK:REQ-NF-Obs-001} | Prefix-match assertions miss field delimiter (11 sites) | fixed | 625c1c337 ↗ |
| B-STAGE9-15 | Insp static-guard 09-15 | blocking BLOCKS: {STAGE:9, TASK:REQ-NF-i18n-001} | testGarminI18nSourceGuard RED in HEAD, gate-invisible | FIXED | d9ba4faad ↗ |
| B-STAGE9-16 | Insp v1_22 09-15 | blocking BLOCKS: {STAGE:9, TASK:REQ-NF-Build-001} | Verification ritual opt-in: ctest label + lint gaps | fixed | DEC-054,8611fb2d0,8ea19e8e5 ↗ |
| B-STAGE9-17 | Insp v1_23 09-15 | non-blocking BLOCKS: {} | DEC-054 excused tier is named but never executed | open | — ↗ |
| B-STAGE9-18 | Builder v8 09-16 | non-blocking BLOCKS: {} | garmin_sec_source_guard.py: dead var + format RED | open | — ↗ |
| B-STAGE9-19 | Live sync #5 09-16 | blocking BLOCKS: {STAGE:9} | sync_incremental fails 15x, folded to error_code=unknown | FIXED | 6f1f7b182 ↗ |
| B-STAGE9-20 | Builder v9 09-16 | non-blocking BLOCKS: {} | PyEmbeddedAdapter.cpp clang-format RED, declared gap | open | — ↗ |
| B-STAGE9-21 | Builder v9 09-16 | non-blocking BLOCKS: {} | pystub garmin_client.py ruff RED; E402 is deliberate | open | — ↗ |
| B-STAGE9-22 | Builder v9 09-16 | non-blocking BLOCKS: {} | No shell linter; garmin_flag_build_guard.sh unmeasured | open | — ↗ |
| B-STAGE9-23 | User+Insp v1_26 09-16 | non-blocking BLOCKS: {} | Buildguard comment prose exceeds code; unverified claims | open | — ↗ |
| B-STAGE9-24 | Builder v10 09-16 | non-blocking BLOCKS: {} | Nothing checks pre-commit hook was ever installed | open | — ↗ |
| B-STAGE9-25 | Builder v11 09-17 | blocking | Adapter sends datetime to date-only lib call; raises | FIXED | B-STAGE9-26 ↗ |
| B-STAGE9-26 | Rev round4 09-19 | non-blocking BLOCKS: {} | Missing activityId still folds to Unknown, not typed | fixed | ce2b3402d,DEC-055 ↗ |
| B-STAGE9-27 | Rev round1 09-19 | blocking BLOCKS: {STAGE:9, TASK:REQ-NF-Obs-001} | WIN32 setvbuf stays buffered; return value unchecked | FIXED | B-STAGE9-12 ↗ |
| B-STAGE9-28 | Insp v1_31 09-19 | non-blocking BLOCKS: {} | Lint-ownership guard blind to untracked new directory | open | DEC-057 ↗ |
| B-STAGE9-29 | Insp v1_32 09-19 | blocking BLOCKS: {STAGE:9, TASK:REQ-002, TASK:REQ-008} | garmin-<id>.fit can't match ride-filename regex; dropped | fixed | ce2b3402d,DEC-056 ↗ |
| B-STAGE9-30 | Insp v1_33 09-19 | non-blocking BLOCKS: {} | pystub scenario added; no C++ test selects it | open | — ↗ |
| B-STAGE9-31 | Rev delta 09-19 | non-blocking BLOCKS: {} | Contract comment still promises bare KeyError (stale) | fixed | ce2b3402d ↗ |
| B-STAGE9-32 | Insp v1_32 09-19 | informational BLOCKS: {} | Run-6 abort is QtWebEngine int3, unrelated to feature | not-a-defect | STAGE9-CRASH-CORE ↗ |
| B-STAGE9-33 | Rev delta 09-19 | non-blocking BLOCKS: {} | parseGarminTime relabels offset timestamp, not converts | open | — ↗ |
| B-STAGE9-34 | Rev delta 09-19 | non-blocking BLOCKS: {} | PyObject_Str can fabricate timestamp from non-str value | open | — ↗ |
| B-STAGE9-35 | Builder v16 09-19 | non-blocking BLOCKS: {} | pystub comment falsely claims it hits C++ naming fallback | open | B-STAGE9-30 ↗ |
| B-STAGE9-36 | Rev delta 09-19/20 | blocking BLOCKS: {CHECKPOINT:B-STAGE9-28 commit} | pre_armed_by is unchecked free text; gate is forgeable | fixed | DEC-064 ↗ |
| B-STAGE9-37 | Insp v1_35 09-19 | non-blocking BLOCKS: {} | gc_obs op=auth measures token restore, not SSO time | open | — ↗ |
| B-STAGE9-38 | Investigator 09-19 | blocking | AppImage omits garmin_client.py; path leaks build dir | closed | DEC-058 ↗ |
| B-STAGE9-39 | Investigator 09-19 | blocking BLOCKS: {STAGE:9} | Garmin-only startup skips deployed-PYTHONHOME setup | closed | DEC-062 ↗ |
| B-STAGE9-40 | Investigator 09-19/20 | blocking BLOCKS: {STAGE:9} | main.cpp discards ensureInitialized() failure Result | fixed | DEC-061,933126d0a ↗ |
| B-STAGE9-41 | Investigator 09-20 | blocking BLOCKS: {STAGE:9} | CMake defines GC_HAVE_PYTHON not GC_WANT_PYTHON | closed | DEC-063 ↗ |
| B-STAGE9-42 | Investigator 09-20 | blocking BLOCKS: {STAGE:9} | Pinned python-appimage 3.11.14 asset now 404 upstream | closed | — ↗ |
| B-STAGE9-43 | builder u3-r2 09-20 | non-blocking BLOCKS: {} | 4 stub fakes miss ctor access-rule divergence | open | — ↗ |
| B-STAGE9-44 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-38;CHECKPOINT:B-STAGE9-38} | override dir not moved to front of sys.path | fixed | B-STAGE9-38-u3-r4 ↗ |
| B-STAGE9-45 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-38;CHECKPOINT:B-STAGE9-38} | cached module accepted w/o real provenance check | closed | DEC-066 ↗ |
| B-STAGE9-46 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-38;CHECKPOINT:B-STAGE9-38} | test allows vacuous pass, override untested | fixed | B-STAGE9-38-u3-r4 ↗ |
| B-STAGE9-47 | reviewer 09-20 | non-blocking BLOCKS: {} | header comments assert false exclusivity claims | open | B-STAGE9-38-u3-r5 ↗ |
| B-STAGE9-48 | investigator 09-20 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | live run used dev binary, not an installer artifact | fixed | DEC-065: ci.10 774ba3533 all 3 installers green ↗ |
| B-STAGE9-49 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-36;CHECKPOINT:B-STAGE9-36} | json.loads collapses dup keys, guard authorizes wrongly | fixed | B-STAGE9-36-r3-review ↗ |
| B-STAGE9-50 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-36;CHECKPOINT:B-STAGE9-36} | strip() eats unicode WS, bypasses sentinel match | fixed | B-STAGE9-36-r3-review ↗ |
| B-STAGE9-51 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-36;CHECKPOINT:B-STAGE9-36} | heading regex \b matches counterfeit shadow heading | fixed | — ↗ |
| B-STAGE9-52 | reviewer 09-20 | non-blocking BLOCKS: {} | docstring says entry needs own Arms bullet (stale) | fixed | — ↗ |
| B-STAGE9-53 | reviewer 09-20 | blocking BLOCKS: {TASK:B-STAGE9-36;CHECKPOINT:B-STAGE9-36} | next() takes first dup heading, skips real entry | fixed | — ↗ |
| B-STAGE9-54 | investigator 09-20 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | AppImage build infeasible: missing many host tools | fixed | DEC-065: AppVeyor ci.4 Linux green, 07babb430 ↗ |
| B-STAGE9-56 | reviewer 09-20 | non-blocking BLOCKS: {} | sha256 pin verified exact; pipeline lacks pipefail | open | — ↗ |
| B-STAGE9-55 | reviewer 09-20 | non-blocking BLOCKS: {} | DEC-063 cites stale '55-site' census (actually 47) | open | — ↗ |
| B-STAGE9-57 | investigator 09-20 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | no no-sudo AppImage path: py AppImage not linkable SDK | fixed | DEC-065: AppVeyor ci.4 Linux green, 07babb430 ↗ |
| B-STAGE9-58 | insp v1_46 09-20 | non-blocking BLOCKS: {} | context.py froze stale token count across pane rotate | open | — ↗ |
| B-STAGE9-59 | reviewer 09-20 | blocking BLOCKS: {TASK:DEC-058;CHECKPOINT:STAGE:9} | Linux CI smoke never proves adapter is in AppImage bundle | closed | DEC-067 ↗ |
| B-STAGE9-60 | investigator 09-20 | blocking BLOCKS: {TASK:DEC-065;CHECKPOINT:STAGE:9} | QTDIR path wrong on Linux; qmake --version fails | closed | B-STAGE9-42-60-review ↗ |
| B-STAGE9-61 | investigator 09-20 | non-blocking BLOCKS: {} | FTDI D2XX fetch 403s on CI egress, unverified | open | — ↗ |
| B-STAGE9-62 | investigator 09-20 | non-blocking BLOCKS: {} | several recipe paths unvalidated before use (macOS etc) | open | — ↗ |
| B-STAGE9-63 | builder v25 09-20 | non-blocking BLOCKS: {} | CMake Python-on build misses sip_core.c, link fails | open | — ↗ |
| B-STAGE9-64 | reviewer 09-20 | non-blocking BLOCKS: {} | root-path override prefix test breaks ('/' -> '//') | open | DEC-066 ↗ |
| B-STAGE9-65 | insp v1_49 09-20 | non-blocking BLOCKS: {} | qmake half of DEC-062 wiring never compiled/verified | open | — ↗ |
| B-STAGE9-66 | reviewer 09-20 | blocking BLOCKS: {STAGE:9, TASK:DEC-066} | post-import gate doesn't bind returned import object | closed | DEC-068 ↗ |
| B-STAGE9-67 | reviewer 09-24 | non-blocking BLOCKS: {} | test named for gate it never reaches (hook fails earlier) | open | DEC-068 ↗ |
| B-STAGE9-68 | insp v1_51/52 09-24 | non-blocking BLOCKS: {} | mkdir guard regex misparses redirection as 2nd operand | open | — ↗ |
| B-STAGE9-69 | reviewer 09-24 | non-blocking BLOCKS: {} | hostile-hook test restore not exception-safe | open | — ↗ |
| B-STAGE9-70 | insp v1_52 09-24 | blocking BLOCKS: {} | ctest garmin-py fails: 3/7 PyProcessBootstrap cases | fixed | B-STAGE9-70-fix ↗ |
| B-STAGE9-71 | insp v1_53 09-24 | blocking BLOCKS: {STAGE:9} | CI proves payload ships on Linux leg only, not all 3 | fixed | DEC-069: ci.10 774ba3533 payload check passes on all 3 ↗ |
| B-STAGE9-72 | reviewer 09-24 | blocking BLOCKS: {} | Py_Finalize with no ledger release after -70 fix | fixed | B-STAGE9-72-fix ↗ |
| B-STAGE9-73 | reviewer 09-24 | non-blocking BLOCKS: {} | hdiutil detach failure masked by earlier PY_RC check | open | — ↗ |
| B-STAGE9-74 | insp v1_54 09-24 | non-blocking BLOCKS: {} | WIKI.md wrongly claims seals untracked/recoverable | fixed | — ↗ |
| B-STAGE9-75 | insp v1_54 09-24 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | 4 new PyErr_SetString literals fail i18n-tr-wrap guard | fixed | B-STAGE9-75-fix-review ↗ |
| B-STAGE9-76 | insp v1_54 09-24 | non-blocking BLOCKS: {} | LastTest.log unreliable count source for multi-label run | open | — ↗ |
| B-STAGE9-77 | insp v1_55 09-24 | non-blocking BLOCKS: {} | reviewer's format-stability reasoning was wrong basis | open | — ↗ |
| B-STAGE9-78 | insp v1_55+user 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | downloaded files are ZIP not FIT; imports never happen | fixed; live 09-29 library 1146→1148 | DEC-070 ↗ |
| B-STAGE9-79 | insp v1_55+user 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | cancel/fail still marks activity imported permanently | fixed; live 09-29 cancel re-offers, v1 | DEC-071 ↗ |
| B-STAGE9-80 | insp v1_55+user 09-26 | non-blocking BLOCKS: {} | backfill dialog has no Qt parent, opens behind settings | open | — ↗ |
| B-STAGE9-81 | insp v1_55 09-26 | non-blocking BLOCKS: {} | backfill run emits zero log lines; silent failure unseen | open | — ↗ |
| B-STAGE9-82 | insp v1_56 09-26 | non-blocking BLOCKS: {} | ledger commit stashes builder's in-flight edits mid-round | open | — ↗ |
| B-STAGE9-83 | reviewer 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | gzip signature not checked despite DEC-070 claim | CLOSED | B-STAGE9-92 ↗ |
| B-STAGE9-84 | reviewer 09-26 | non-blocking BLOCKS: {} | stale staged file left behind on sidecar-write retry | open | — ↗ |
| B-STAGE9-85 | reviewer+insp 09-26 | non-blocking BLOCKS: {} | stagedFitPath name wrong after it can hold zip path | CLOSED | — ↗ |
| B-STAGE9-86 | reviewer 09-26 | blocking-adjacent BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | gzip expansion is a no-op everywhere; DEC-070 claim false | CLOSED | B-STAGE9-92 ↗ |
| B-STAGE9-87 | reviewer 09-26 | non-blocking BLOCKS: {} | comments still say 'FIT bytes' after payload can be zip/gz | CLOSED | B-STAGE9-78-r3 ↗ |
| B-STAGE9-88 | builder v30 09-26 | non-blocking BLOCKS: {} | brief cited zlib link scoped to wrong CMake target | closed | B-STAGE9-78-r3b ↗ |
| B-STAGE9-89 | reviewer 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | inflate accepts truncated/concat gzip as success | CLOSED | DEC-073 ↗ |
| B-STAGE9-90 | reviewer 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | gzip-of-gzip reaches frozen no-op archive route | CLOSED | B-STAGE9-92 ↗ |
| B-STAGE9-91 | reviewer 09-26 | non-blocking BLOCKS: {} | stale doc claim + gzip edge cases untested (trunc/nest) | open | T-227 ↗ |
| B-STAGE9-92 | insp v1_58 09-26 | blocking BLOCKS: {STAGE:9;CHECKPOINT:STAGE:9} | non-FIT/gzip blob still falls through and stages as .fit | CLOSED | DEC-073 ↗ |
| B-STAGE9-93 | builder v31 09-26 | non-blocking BLOCKS: {} | T-215 fixture stale; fitBytesFor lacks real FIT header | CLOSED | B-STAGE9-92 ↗ |
| B-STAGE9-94 | insp v1_58 09-26 | non-blocking BLOCKS: {} | new tr() string missing from all 13 translation files | CLOSED | — ↗ |
| B-STAGE9-95 | insp v1_59 09-26 | non-blocking BLOCKS: {} | traceability.md missing T-212..T-227 test ids | open | — ↗ |
| B-STAGE9-96 | librarian J3 09-26 | advisory BLOCKS: {} | STATE.md 200B row cap misapplied to findings.md rows | CLOSED | DEC-074 ↗ |
| B-STAGE9-97 | librarian J3 09-26 | non-blocking BLOCKS: {} | ~50 more findings rows still carry inline narrative bloat | open | — ↗ |
| B-STAGE9-98 | librarian J3 09-26 | non-blocking BLOCKS: {} | gate can't detect content misfiled in wrong cell (pipe) | open | — ↗ |
| B-STAGE9-99 | librarian J3 09-26 | non-blocking BLOCKS: {} | resolved-by cell can go stale vs later disposition | open | — ↗ |
| B-STAGE9-100 | librarian J3 09-26 | cosmetic BLOCKS: {} | 60-char summary convention stated but never enforced | open | — ↗ |
| B-STAGE9-101 | insp v1_60 09-27 | blocking BLOCKS: {B-STAGE9-79} | saveBackfillState overwrote whole state, erasing pending | fixed | B-STAGE9-79-s1-r2-review ↗ |
| B-STAGE9-102 | reviewer 09-27 | blocking BLOCKS: {B-STAGE9-79} | permission-rejected treated as empty then rewritten 0600 | fixed | B-STAGE9-79-s1-r2-review ↗ |
| B-STAGE9-103 | reviewer 09-27 | blocking BLOCKS: {B-STAGE9-79} | loadBackfillState drops non-object pending entries | fixed | — ↗ |
| B-STAGE9-104 | reviewer 09-27 | non-blocking BLOCKS: {} | load accepts schema v2 silently, future write downgrades it | open | — ↗ |
| B-STAGE9-105 | insp v1_60 09-27 | non-blocking BLOCKS: {} | insp_wake.sh hardcodes old 200B cap, desensitizes breach | open | B-STAGE9-97 ↗ |
| B-STAGE9-106 | insp v1_60 09-27 | non-blocking BLOCKS: {} | pre-commit stash races builder's in-flight edits | open | — ↗ |
| B-STAGE9-107 | reviewer 09-27 | blocking BLOCKS: {} | persist failure collapsed to one generic message | fixed | B-STAGE9-107-rev ↗ |
| B-STAGE9-108 | reviewer 09-27 | blocking BLOCKS: {B-STAGE9-79} | concurrent RMW writers can drop pending entries (no lock) | CLOSED | DEC-076 ↗ |
| B-STAGE9-109 | reviewer 09-27 | non-blocking BLOCKS: {} | lock keyed on path string, not canonical fs identity | accepted | — ↗ |
| B-STAGE9-110 | reviewer 09-27 | non-blocking BLOCKS: {} | new lock tests are probabilistic, not deterministic | accepted | — ↗ |
| B-STAGE9-111 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | auto-download route completes before import handoff lands | closed | DEC-080 ↗ |
| B-STAGE9-112 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | missing staged file orphans pending entry permanently | closed | DEC-078 ↗ |
| B-STAGE9-113 | reviewer 09-27 | blocking BLOCKS: {} | recordImported() return value discarded before drop | fixed | B-STAGE9-79-s2-r2rev ↗ |
| B-STAGE9-114 | reviewer 09-27 | blocking BLOCKS: {} | raw Athlete/RideCache ptrs dangle after wizard event loop | fixed | B-STAGE9-79-s2-r2rev ↗ |
| B-STAGE9-115 | insp v1_62 09-27 | blocking BLOCKS: {} | 3 new tr() literals absent from all 13 .ts catalogs | fixed | B-STAGE9-115-rev ↗ |
| B-STAGE9-116 | reviewer 09-27 | blocking BLOCKS: {} | wizard itself dangles raw ptrs during nested event loop | deferred | DEC-077 ↗ |
| B-STAGE9-117 | reviewer 09-27 | non-blocking BLOCKS: {} | stub teardown is sync, doesn't mirror real ownership | accepted | DEC-077 ↗ |
| B-STAGE9-118 | reviewer 09-27 | non-blocking BLOCKS: {} | switch default: arm silently swallows new LoadStatus | fixed | B-STAGE9-118-120-r2 ↗ |
| B-STAGE9-119 | reviewer 09-27 | non-blocking BLOCKS: {} | PendingManifestMalformed path has no controller-level test | open | — ↗ |
| B-STAGE9-120 | builder v35 09-27 | non-blocking BLOCKS: {} | comment names re-offer mechanism code doesn't implement | fixed | B-STAGE9-118-120-r2 ↗ |
| B-STAGE9-121 | Inspector 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | activity exactly at rangeStart excluded from every run | closed | T-238 ↗ |
| B-STAGE9-122 | reviewer 09-27 | non-blocking BLOCKS: {} | vacuous cursor-clear tests don't prove correct rewind | fixed | B-STAGE9-112-121-122-rev ↗ |
| B-STAGE9-123 | Inspector 09-27 | non-blocking BLOCKS: {} | build has no -Wswitch; exhaustive switch guard is inert | open | — ↗ |
| B-STAGE9-124 | Inspector 09-27 | non-blocking BLOCKS: {} | docs claim exclusive bound; code filters inclusive (>=) | open | — ↗ |
| B-STAGE9-125 | reviewer 09-27 | non-blocking BLOCKS: {} | fake listActivities ignores sinceGmt bound entirely | open | — ↗ |
| B-STAGE9-126 | Inspector 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | cursor rewind only on exact-match drop, not <= pending | closed | ab0bb7586 ↗ |
| B-STAGE9-127 | investigator 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | every state write stamps v1, skipping legacy v0 migration | closed | 4dacd8447 ↗ |
| B-STAGE9-128 | reviewer 09-27 | non-blocking BLOCKS: {} | cursor compared as text; offset spellings sort wrong | open | DEC-081 ↗ |
| B-STAGE9-129 | insp v1_66 09-27 | non-blocking BLOCKS: {} | header doc-comment still states old '==' drop predicate | open | — ↗ |
| B-STAGE9-130 | investigator 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-79} | legacy migration needs atomic 2-file move; none converge | fixed s3 (T-240, gate 57/57) | DEC-082, DEC-087 ↗ |
| B-STAGE9-131 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-111;B-STAGE9-79} | GUI-sync promote hook skipped on generation change | fixed | B-STAGE9-111-r2-rev ↗ |
| B-STAGE9-132 | insp v1_66 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-111;B-STAGE9-79} | v0 sync records nothing in imported map; dedup dead | fixed | T-247 ↗ |
| B-STAGE9-133 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-111;B-STAGE9-79} | successful promotion wipes durable resume cursor | closed | e15d863ba ↗ |
| B-STAGE9-134 | insp v1_67 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-111} | 2 stale tests assert imported map exists before promotion | closed | e15d863ba ↗ |
| B-STAGE9-135 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | moved cursor assertion will break once DEC-083 lands | closed | e15d863ba ↗ |
| B-STAGE9-136 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | controller rewrites stale cursor over newer promoted one | closed | e15d863ba ↗ |
| B-STAGE9-137 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133;B-STAGE9-79} | promotion can stamp v1 on half-migrated v0 sidecar | closed | e15d863ba ↗ |
| B-STAGE9-138 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | watermark can move backward via mixed timestamp spellings | fixed | DEC-084 ↗ |
| B-STAGE9-139 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | 3 tests weakened, no longer pin partial-progress subject | closed | e15d863ba ↗ |
| B-STAGE9-140 | reviewer 09-27 | NON-BLOCKING BLOCKS: {} | pending-skip reads stale snapshot from before filter loop | open | — ↗ |
| B-STAGE9-141 | reviewer 09-27 | NON-BLOCKING BLOCKS: {} | 2 comments still describe download-time cursor advance | open | B-STAGE9-133-r2-rev ↗ |
| B-STAGE9-142 | reviewer 09-27 | NON-BLOCKING BLOCKS: {} | new comment says drop path calls dropPendingBackfill | open | B-STAGE9-133-r3-rev ↗ |
| B-STAGE9-143 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133;B-STAGE9-79} | survivor guard is text compare; misses earlier pending row | fixed | e15d863ba ↗ |
| B-STAGE9-144 | reviewer 09-27 | NON-BLOCKING BLOCKS: {} | tests can't distinguish working parser from dead one | fixed | e15d863ba ↗ |
| B-STAGE9-145 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | offset-restamp bug also lives at 2 more call sites | fixed | e15d863ba ↗ |
| B-STAGE9-146 | insp v1_70 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | sort comparator is text <, breaks monotonic cursor gate | fixed | e15d863ba ↗ |
| B-STAGE9-147 | builder v39 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | invalid QDateTime ordering causes loss or UB per site | fixed | DEC-084 ↗ |
| B-STAGE9-148 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | abandonment site still text-compares startTimeGMT | fixed | T-261 ↗ |
| B-STAGE9-149 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | resume-cursor range check is text compare; can drop range | fixed | T-262 ↗ |
| B-STAGE9-150 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-133} | no test covers cursor advance over mixed spellings | fixed | T-263 ↗ |
| B-STAGE9-151 | reviewer 09-27 | non-blocking BLOCKS: {} | comment points to deleted symbol kGarminTimeFormat | fixed | e15d863ba ↗ |
| B-STAGE9-152 | Inspector 09-27 | non-blocking BLOCKS: {} | STATE.md commit landed while builder was mid-round | CLOSED | — ↗ |
| B-STAGE9-153 | Inspector 09-27 | non-blocking BLOCKS: {} | roster ref flips builder to Codex, conflicts w/ TEAM line | closed | — ↗ |
| B-STAGE9-154 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-130;B-STAGE9-79} | dialog ignores migrate() bool; v1 stamp on half-migration | fixed r3 via DEC-087 | DEC-087 ↗ |
| B-STAGE9-155 | reviewer 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-130;B-STAGE9-79} | phase-2 prune drops undecoded raw imported rows | fixed r2; residual B-STAGE9-160 | — ↗ |
| B-STAGE9-156 | insp v1_72 09-27 | blocking BLOCKS: {STAGE:9;B-STAGE9-130;B-STAGE9-154;B-STAGE9-155} | Codex worktree mandate conflicts w/ shared-checkout roster | closed | DEC-086 ↗ |
| B-STAGE9-157 | reviewer 09-28 | blocking BLOCKS: {STAGE:9;B-STAGE9-130} | stale unmatched: row added after dialog classify gets v1 unclassified | fixed r4 | DEC-087 |
| B-STAGE9-158 | reviewer 09-28 | blocking BLOCKS: {STAGE:9;B-STAGE9-130} | skipped migration (null cache/no imported) pauses backfill | fixed r3 | DEC-087 |
| B-STAGE9-159 | reviewer 09-28 | blocking BLOCKS: {STAGE:9;B-STAGE9-130} | added comments still multi-line prose (slice-3 files) | fixed r6 | r5 line list; then non-blocking |
| B-STAGE9-160 | reviewer 09-28 | non-blocking | phase-2 compact re-serialize may reformat hand-edited rows; values kept | accept-with-note | store writes Compact |
| B-STAGE9-161 | builder r3 09-28 | non-blocking | GarminConnect.cpp recordImport v0 cursor-skip now dead under DEC-087 | fixed; T-243a/T-244b/T-247 re-pinned | DEC-083, DEC-087 |
| B-STAGE9-162 | reviewer 09-28 | blocking BLOCKS: {STAGE:9;B-STAGE9-130} | NotFound imported still writes phase 1 before phase 3 | fixed r4 | DEC-087 b3 |
| B-STAGE9-163 | insp gate 09-28 | blocking BLOCKS: {STAGE:9;B-STAGE9-130} | new dialog tr() string absent from 13 .ts catalogs (i18n guard) | fixed | 13 .ts rows, reviewer PASS |
| B-STAGE9-164 | insp gate 09-28 | blocking | compaction 9f99d95cf moved DEC-054 gc-arms sentinel out of its fixed slot | fixed | sentinel restored |
| B-STAGE9-165 | user live 09-29 | non-blocking | mobile login 429s logged each auth; fallback auth still ok | accept-with-note | upstream garminconnect 0.3.15 client.py strategy log; gc_obs auth ok |
| B-STAGE9-166 | user live 09-29 | blocking BLOCKS: {STAGE:9} | Backfill dialog opens behind modal Athlete settings; unusable until it closes | fixed; user-verified live 09-29 | reviewer PASS |
| B-STAGE9-167 | user live 09-29 | non-blocking | pre-fix ZIP .fit staged files re-offered via v0 migration; reuse skips sniff | accept-with-note | pre-fix only, DEC-072 |
| B-STAGE9-168 | AppVeyor ci.1 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Win leg: root vcpkg.json forces manifest mode; `vcpkg install gsl` refused | fixed | vcpkg --classic, rev CLOSED |
| B-STAGE9-169 | AppVeyor ci.1 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Linux leg: Qt/6.8/gcc_64 absent; image has flat Qt/6.8.3 (-60 premise false) | fixed | rev2 CLOSED |
| B-STAGE9-170 | AppVeyor ci.1 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS leg: FTDI D2XX zip now 403 to scripts; no D2XX cache on fork | fixed | rev2 CLOSED |
| B-STAGE9-171 | reviewer 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Linux leg: D2XX wget also 403s on empty cache; unguarded under set -e | fixed | rev2 CLOSED |
| B-STAGE9-172 | reviewer 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS D2XX: zip lacking dmg/files aborts; partial D2XX/ skips refetch | fixed; residual pinned | DEC-088 marker gate |
| B-STAGE9-173 | AppVeyor ci.2 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Win+mac legs: PyEmbeddedAdapter.cpp:652 3.12 API; CI Python 3.11 | fixed | PY_VERSION_HEX shim, rev PASS |
| B-STAGE9-174 | AppVeyor ci.2 09-29 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Linux leg: Qt/6.8 qmake needs libicui18n.so.73, absent (exit 127) | fixed | upstream loader path, rev PASS |
| B-STAGE9-175 | AppVeyor ci.3 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Linux: branch predates upstream e27be4772 autotools apt; no aclocal | fixed | DEC-089 sync, rev PASS |
| B-STAGE9-176 | AppVeyor ci.3 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS: 60-min timeout; fork sonoma vs upstream monterey 81893217c | fixed | DEC-089 monterey, rev PASS |
| B-STAGE9-177 | AppVeyor ci.3 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Win: installer + DEC-069 pass, 60-min cap at cache save | fixed | DEC-089/4 cache + -j2; run unproven |
| B-STAGE9-178 | reviewer 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | srmio cache name unchanged; ci.3's unconfigured srmio/ still restorable | fixed | rev2 CLOSED |
| B-STAGE9-179 | reviewer 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | windows/after_build.ps1: windeployqt/makensis exit codes unchecked | fixed | rev2 CLOSED |
| B-STAGE9-180 | reviewer 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macos/after_build.sh:120 drops upstream pip shebang/_sysconfigdata fix | fixed | rev2 CLOSED |
| B-STAGE9-181 | reviewer 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | windows/*.ps1: no ErrorActionPreference Stop; cmdlet errors pass | fixed | rev3 CLOSED |
| B-STAGE9-182 | AppVeyor ci.4 09-30 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Win leg: EAP Stop turns native stderr (R --version) into a fatal error | fixed | Invoke-NativeChecked, rev PASS |
| B-STAGE9-183 | reviewer 10-01 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | DEC-089/5 qwt step: failed make sub-qwt no longer fails the step | fixed | make \|\| exit 1, rev PASS |
| B-STAGE9-184 | AppVeyor ci.5 10-01 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | Win qwt jom -j2 races Debug/Release moc dir: Permission denied | fixed | jom -j1 as upstream, rev PASS |
| B-STAGE9-185 | AppVeyor ci.5 10-01 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS: no cache restore/save; DEC-089/5 priming unsaved | fixed | fail by command status, not exit; rev PASS |
| B-STAGE9-186 | AppVeyor ci.6 10-01 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | qwt cache key qwtconfig.pri.in froze lib-less copies | fixed | qwt/lib content-keyed, cold legs prime; PASS |
| B-STAGE9-187 | AppVeyor ci.6 10-01 | non-blocking | push while a build is queued: clone_depth 1 loses its commit, later legs fail | accept-with-note | push only after the running build ends |
| B-STAGE9-188 | AppVeyor ci.9 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS DEC-069 check: python3.11 not at mounted-DMG path | fixed | abs mount, find 1 python, status end; PASS |
| B-STAGE9-189 | rev -188-r1 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | check hard-coded the .app path; -srcfolder may put Contents at root | fixed | find exactly one interpreter; PASS |
| B-STAGE9-190 | rev -188-r1 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS check: abort-on-error shell skips PY_RC capture + detach | fixed | rc captured, detach always runs; rev PASS |
| B-STAGE9-191 | rev -188-r2 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS check: `\|\| true` hid failed detach; green w/ DMG mounted | fixed | detach rc kept; python rc first; PASS |
| B-STAGE9-192 | rev -188-r3 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | macOS check ends by shell exit, skips finalization (as -185) | fixed | no exit; ci.8: cache saved after false |
| B-STAGE9-193 | Inspector r4 10-02 | blocking BLOCKS: {CHECKPOINT:STAGE9;RELEASE} | 2 final [ ] tests: without sh -e a python fail is hidden | fixed | single && verdict line; rev PASS |
| B-STAGE10-01 | rev UPR-1-r1 10-02 | blocking | plan misses CloudServiceSyncDialog::start (AddCloudWizard.cpp:1130) | fixed | wt contract; rev r2 CLOSED |
| B-STAGE10-02 | rev UPR-1-r1 10-02 | blocking | defaulted 4-arg readFile hides upstream 3-arg overrides (CloudService.h:341) | fixed | wt contract; rev r2 CLOSED |
| B-STAGE10-03 | rev UPR-1-r1 10-02 | blocking | contract omits sync/auto-download read-fail receivers (CloudService.cpp:1441,4337) | fixed | wt contract; rev r2 CLOSED |
| B-STAGE10-04 | rev UPR-1-r1 10-02 | blocking | contract omits rideRegistrationCompleted + consumers (CloudService.cpp:3561,4505) | fixed | wt contract; rev r2 CLOSED |
| B-STAGE10-05 | rev UPR-1-r1 10-02 | blocking | whole-file src.pro carries non-Garmin flags/lrelease (src.pro:115) | fixed | wt contract; rev r2 CLOSED |
| B-STAGE10-06 | rev UPR-1-r2 10-02 | blocking | wt failedRead uses current listindex: stale fail advances new batch (CloudService.cpp:1631) | fixed | via -09/-10; rev r4 PASS |
| B-STAGE10-07 | rev UPR-1-r2 10-02 | blocking | unconditional PyProcessBootstrap ignores --no-python, Garmin off (main.cpp:524) | fixed | wt main.cpp; rev r3 CLOSED |
| B-STAGE10-08 | rev UPR-1-r2 10-02 | blocking | nostderr buffering change rides along, non-Garmin (main.cpp:189) | fixed | wt main.cpp; rev r3 CLOSED |
| B-STAGE10-09 | rev UPR-1-r3 10-02 | blocking | Refresh List keeps batchId: stale fail labels new row (wt CloudService.cpp:1651) | fixed | ReadTicket row+batch, recheck; rev r4 PASS |
| B-STAGE10-10 | rev UPR-1-r3 10-02 | blocking | no batchId recheck after processEvents: advances restarted batch (wt :1667) | fixed | ReadTicket row+batch, recheck; rev r4 PASS |
| B-STAGE10-11 | rev UPR-2 10-02 | non-blocking | upstream branch comments cite internal ids (DEC/B-STAGE/DES), ~76 lines in 10 modified files | fixed | UPR-4a..e: 0 ids left, C++ rev PASS |
| B-STAGE10-12 | rev UPR-4a-r1 10-02 | non-blocking | fork-only refs left after id scrub (pyproject, Slice A/B, Unit 3) | accept-with-note | 3 SAME rounds; rest -> B-STAGE10-13 |
| B-STAGE10-13 | rev UPR-4e-r3 10-02 | non-blocking | 7 .py prose nits: Option B, Open residual, stray ), stale contracts | deferred | upstream review polish |
| B-STAGE10-14 | insp UPR-3 10-02 | blocking | upstream branch lacks Garmin CI/packaging (appveyor.yml + appveyor/*): DEC-069 check cannot run | open | UPR-3a `476630af5` PASS; closes on CI |
