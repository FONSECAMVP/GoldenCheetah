# State — Garmin Connect Integration

_Updated: 2026-07-12 — **REQ-006 CLOSED, security-closed.** Slice A load-side perm-refusal committed `d86323246` (GarminTokenStore::loadChecked refuses wider-than-0600 tokens.json → typed TokenPermissionsRejected — TEST-014, garmin-fast). Slice B `__init__` auth-only reconciliation committed `3edb705cb` (library `(email,password)` only, no tokenstore path → no self-written 2nd file; C-API `"ss"`, PyEmbeddedAdapter(modulePath) — TEST-015 pytest + TEST-016 garmin-py). **REQ-NF-Sec-002 end-to-end MET;** findings B-R004-01 + A3-R004-M3 (blocking) RESOLVED. Verification-Gate PASS: pytest 15/15, garmin-py 20/20, garmin-fast 10/10. VAL-011 PASS. Prior: REQ-004 write path GREEN committed `54b7005e6` (VAL-010 PASS; B-R004-02 cleared). Prior: REQ-007 download chain GREEN committed `1eb5a6a16` (VAL-009 PASS; readFile/FIT→TCX deferred, NOT fully deployed). Prior: REQ-002 CLOSED `60a076848`._

## phase
- current: Phase 2.2
- active feature: REQ-006 — token-file perms (load-side refusal + auth-only reconciliation) GREEN, CLOSED, security-closed. Slice A `d86323246`, Slice B `3edb705cb`; REQ-NF-Sec-002 end-to-end MET. Prior: REQ-004 write path GREEN committed `54b7005e6` (VAL-010 PASS); REQ-007 download chain committed `1eb5a6a16` (NOT fully deployed); REQ-002 CLOSED.
- next gate: REQ-006 CLOSED, security-closed, **VAL-011 PASS** (4th/closing pass `d69f70673`). A3-R006 hardening DONE (T-014 +3 slots, A3-R006-01/-02 FIXED, `458a72ba7`). Recommended immediate: build the deterministic pre-CLV ledger-drift lint (LSN-008 recur:7 — durable fix). Downstream: REQ-007 CLOSURE (readFile + FIT→TCX, needs worker-in-CloudService lifecycle + PRD-Assumption-B). Alt deferred: REQ-003 (MFA).
- last-clean-VAL: VAL-011 (2026-07-12 — REQ-006 Slices A+B + A3-R006 hardening, PASS on 4th pass; passes 1-3 FAILed on ledger drift only, code/test spine clean throughout). Prior: VAL-010 (2026-07-11 — REQ-004 write-path, PASS after fix).
- last-cycle: A3-R006 REQ-006 Slices A+B (2026-07-12) — test-hardening, FINDINGS→FIXED (A3-R006-01 mask-narrowing survivor + A3-R006-02 freshness closed via TEST-014 +3 slots 0620/0601/re-stat, mutant-kill demonstrated; A3-R006-03 informational accept). Prior: A3-R004 REQ-004 write path (2026-07-11) — M1 (AtomicFile order — order-recording TmpWriter seam) + M2 (load_tokens no-op — per-instance fake) FIXED; M3 (REQ-NF-Sec-002 end-to-end) DEFERRED → RESOLVED by REQ-006 Slice B. 6 non-blocking dispositioned. Prior: A3 REQ-002 tile-routing (2026-07-05) FIXED.

## decs
| DEC | question | status | rev | dep-count | last |
|---|---|---|---|---:|---|
| 001 | Solution shape: monolithic vs staged vs adapter | accepted (B-staged) | medium | 30 | 2026-05-17 |
| 002 | Python integration mechanism (direct / mailbox / per-request thread) | accepted (B-worker) | medium | 12 | 2026-05-17 |
| 003 | Token + sidecar on-disk layout | accepted (B-per-athlete) | low-med | 9 | 2026-05-17 |
| 004 | Credentials + MFA dialog UX shape | accepted (B-wizard pages) | high | 6 | 2026-05-17 |
| 005 | Phase-1 CloudService capabilities (Query\|Download) | accepted | high | 4 | 2026-05-17 |
| 006 | Activity format + staging path (FIT default) | accepted | medium | 3 | 2026-05-17 |
| 007 | Rate-limit + retry placement (Python-side worker decorator) | accepted (B) | high | 4 | 2026-05-17 |
| 008 | Testing toolchain (QTest+CTest C++ / pytest+coverage.py Python) | accepted (A) | high-py / med-cpp | 6 | 2026-05-17 |
| 009 | Style/quality toolchain (ruff + mypy --strict, scoped) | accepted (A) | high | 4 | 2026-05-17 |
| 010 | Pre-commit framework (scoped to new Garmin paths) | accepted (A) | high | 3 | 2026-05-17 |
| 011 | Phase-1 rollout (GC_WANT_GARMINCONNECT, default OFF) | accepted | high | 6 | 2026-05-17 |
| 012 | Auth-dispatcher seam (IGarminAuthClient interface, injected) | accepted (A) | high | 4 | 2026-05-24 |
| 013 | Worker ↔ Python adapter seam (IGarminPyAdapter interface, injected) | accepted (A) | high | 4 | 2026-05-24 |
| 014 | Token persistence: library-write vs GC-owned atomic write + perms | accepted (B — C++ owns atomic 0600 write; adapter dumps()/loads()) | med-exp | 6 | 2026-07-11 |

## des
| DES | implements | status | last |
|---|---|---|---|
| 001 | DEC-002, DEC-013 | drafted (Auth subset + REQ-007 download slot GREEN — TEST-010) | 2026-07-08 |
| 001a | DEC-013 | GREEN (Auth + download seam — TEST-004 + TEST-009) | 2026-07-08 |
| 002 | DEC-003, DEC-014 | write+read GREEN (GarminTokenStore TEST-012 write + TEST-014 load-side refuse-on-wider-than-owner→TokenPermissionsRejected, REQ-006; DEC-014 literal); sidecar/backfill drafted | 2026-07-12 |
| 003 | DEC-004, DEC-012 | drafted | 2026-05-24 |
| 003a | DEC-012 | drafted | 2026-05-24 |
| 004 | DEC-001, DEC-005, DEC-006 | drafted | 2026-05-17 |
| 005 | DEC-007 | drafted | 2026-05-17 |
| 006 | cross-cutting (atomic writer) | GREEN (src/Cloud/AtomicFile.{h,cpp}, TEST-011) | 2026-07-11 |
| 007 | DEC-011 | drafted | 2026-05-17 |
| 008 | cross-cutting (error bus) | drafted | 2026-05-17 |
| 009 | uses DES-001/002/005/006 (bulk backfill) | drafted | 2026-05-17 |
| 010 | uses DES-001/002/005 (incremental sync) | drafted | 2026-05-17 |
| 011 | uses DES-001/004/012 (profile auto-fill) | drafted | 2026-05-17 |
| 012 | adapter seam over python-garminconnect (A2-004 fix) + DEC-014 dump_tokens/load_tokens | GREEN (login/download + DEC-014 dump/load/session_expired TEST-013; `__init__` auth-only DONE `3edb705cb`, REQ-006 — B-R004-01 resolved); REQ-003/008/012/013 NotImplementedError | 2026-07-12 |
| 013 | production PyEmbeddedAdapter (DEC-013 production side, DEC-002) + DEC-014 blob-export | GREEN (Auth + download + tokenBlob on Success); DEC-014 DONE — 1-arg auth-only ctor, stop-forwarding tokenstorePath (`3edb705cb`, REQ-006 Slice B; A3-R004-M3 resolved) | 2026-07-12 |

## reqs
| REQ | cat | DECs | DESs | TESTs | status |
|---|---|---|---|---|---|
| 001 | must | 001,005,011 | 004 | T-001 | deployed (6381b90f4) |
| 002 | must | 001,002,004,008,009,010,011,012,013 | 001,001a,003,003a,008,012,013 | T-002 (GREEN, 5), T-003 (GREEN, 13), T-004 (GREEN, 10), T-005 (GREEN, 10 slots), T-006 (GREEN, 7 slots), T-007 (GREEN, 4 slots) [slot counts exclude QTest auto init/cleanup] | production PyEmbeddedAdapter + wizard tile-routing GREEN; A3 clean (TEST-007 + strengthened 005/006); **VAL-008 PASS 9/9 — slice CLOSED + committed `60a076848`** (TR-08/TR-06 deferred) |
| 003 | must | 001,002,004 | 001,003,012 | — | not started |
| 004 | must | 001,003,014 | 002,006,012,013 | T-011 (AtomicFile GREEN, garmin-fast), T-012 (GarminTokenStore GREEN, garmin-fast), T-013 (adapter dump/load GREEN, 3 pytest) | write path GREEN, committed `54b7005e6`. VAL-010 PASS; A3-R004 M1/M2 FIXED; style/type gate clean (B-R004-02 CLEARED). M3 (REQ-NF-Sec-002 end-to-end) + B-R004-01 RESOLVED by REQ-006 Slice B `3edb705cb` — REQ-004 now security-closed |
| 005 | must | 001,012 | 003,003a,012 | T-002 (adapter half: password-not-retained); T-003 (wizard side — GREEN) | tested (A3 pending) |
| 006 | must | 001,003,014 | 002,008,012,013 | T-014 (Slice A load-side perm-refusal GREEN, garmin-fast), T-015 (Slice B auth-only ctor GREEN, pytest), T-016 (Slice B no-path-forwarded GREEN, garmin-py) | **CLOSED, security-closed.** Slice A `d86323246` (loadChecked refuses wider-than-0600 → TokenPermissionsRejected). Slice B `3edb705cb` (auth-only ctor, no tokenstore path → no self-written 2nd file). REQ-NF-Sec-002 end-to-end MET; findings B-R004-01 + A3-R004-M3 resolved. Verification-Gate PASS (pytest 15/15, garmin-py 20/20, garmin-fast 10/10). VAL-011 PASS. Deferred: C++-persist wiring → REQ-007 closure |
| 007 | must | 001,002,006,013 | 001,001a,004,012,013 | T-008 (GREEN, 6 pytest — adapter download_activity). T-009 (GREEN, 8 garmin-py — PyEmbeddedAdapter.downloadActivity marshalling). T-010 (GREEN, 6 garmin-fast — GarminWorker DownloadActivity op: Success→downloaded, Network/RateLimit/Unknown→downloadFailed, args forwarded, adapter off-GUI-thread) | download chain GREEN adapter→PyEmbeddedAdapter→worker, committed `1eb5a6a16` (VAL-009 PASS). **NOT-done (deferred):** GarminConnect::readFile staging garmin-<id>.<ext> + FIT→TCX fallback (DES-004) → needs REQ-004/006 tokens/session + PRD-Assumption-B library validation. REQ-007 NOT fully deployed |
| 008 | must | 001,003,006 | 002,010,012 | — | not started |
| 009 | must | 001,004 | 003 | — | not started |
| 010 | must | 001,002,003,007 | 002,009,006,012 | — | not started |
| 011 | must | 001,005 | 004 | T-001 (capabilities mutants #2/#3) | tested via REQ-001 |
| 012 | should | 001,003 | 002,004 | — | not started |
| 013 | nice | 001 | 011,012 | — | not started |
| 014 | must | 001,004 | 003,008,012 | — | not started |
| 015 | must | 001,004 | 003,012 | — | not started |
| NF-Perf-001..003 | must | 001,002,007 | 001,005,010 | — | not started |
| NF-Sec-001..004 | must | 001,003 | 002,003,008 | T-002 (REQ-005 adapter-side), T-012/T-014 (NF-Sec-002 0600 write + load-side refusal) | partial — **NF-Sec-002 end-to-end MET** (REQ-006 `3edb705cb`: 0600 write + refuse-on-wider-than-owner load + auth-only ctor so library self-writes no 2nd file). NF-Sec-001/003/004 still partial |
| NF-Compat-001 | must | 001 | 012 | — | doc-only |
| NF-Reliab-001..002 | must | 001,003,007 | 005,006,009,012 | — | not started |
| NF-Threads-001 | must | 001,002 | 001 | — | not started |
| NF-Cancel-001 | must | 001,002 | 001,009 | — | not started |
| NF-Obs-001 | must | 001 | 008 | — | not started |
| NF-i18n-001 | must | 001,004 | 003,008 | — | not started |
| NF-Build-001 | must | 001,011 | 007 | (smoke build OFF/ON works) | partial (D-01 closure) |
| NF-Pkg-001 | must | 001,011 | 007 | — | not started |

Full motivation cells and acceptance fragments live in `prd.md`. Use this table for cascade walks and CLV; drill into `prd.md` only when a row changes status.

## cycles
| cycle | scope | result | findings(open/total) | file |
|---|---|---|---:|---|
| A0 | Phase 0 | clean | 0/4 | cycles/archive/a0.md |
| A1 | Phase 1 (iter 1) | clean | 0/19 (15 fix-now, 3 user-input, 4 defer, 4 document — counted in findings.md) | cycles/archive/a1.md |
| A2 | Phase 1 (iter 1) | dispositioned | 0/8 | cycles/archive/a2.md |
| A2-iter2 | Phase 1 (re-run) | clean | 0/0 | cycles/archive/a2-iter2.md |
| A3 | REQ-001 | clean | 0/4 | cycles/archive/a3-req-001.md |
| A3 | REQ-002 (adapter slice) | clean | 0/6 | cycles/active/a3-req-002.md |
| A3 | REQ-002 (e2e slice) | clean | 0/20 (19 KILLED + 1 non-mutation) | cycles/active/a3-req-002-e2e.md |
| A3 | REQ-002 (tile-routing slice) | FINDINGS→fixed | 0 blocking (TR-01/-02/-04/-05 resolved via TEST-007 + TEST-006/005 strengthening; TR-03 now guarded→accept; TR-06 defer; TR-07 accept; TR-08 NEW defer→Phase 1.5) | cycles/active/a3-req-002-tile-routing.md |
| A3 | REQ-004 (write path) | FINDINGS→fixed | 0 blocking (M1/M2 fixed via order-recording seam + per-instance fake; M3 deferred→RESOLVED by REQ-006 Slice B; 04-09 non-blocking dispositioned) | findings.md A3-R004-* |
| A3-R006 | REQ-006 (Slices A+B) | FINDINGS→fixed | 0 blocking (A3-R006-01 mask-narrowing + A3-R006-02 freshness FIXED via TEST-014 +3 slots 0620/0601/re-stat, mutant-kill demonstrated; A3-R006-03 informational accept — T-016 masked, T-015 is real guard). M-A2/A3/B1/B2/B3 KILLED; REQ-005 reconfirmed intact | findings.md A3-R006-* |

## vals
| VAL | trigger | result | file |
|---|---|---|---|
| 001 | P0 exit | PASS | validations/archive/val-001.md |
| 002 | P1 exit (after A2-iter2) | PASS | validations/archive/val-002.md |
| 003 | P2.2 REQ-001 / TEST-001 | PASS | validations/archive/val-003.md |
| 004 | P2.2 REQ-002 adapter slice | PASS | validations/active/val-004.md |
| 005 | P2.2 REQ-002 wizard-wiring slice | PASS | validations/active/val-005.md |
| 006 | P2.2 REQ-002 end-to-end slice | PASS | validations/active/val-006.md |
| 007 | P2.2 REQ-002 PyEmbeddedAdapter + wizard tile-routing slice | PASS (7/9, 2 WARN) | validations/active/val-007.md |
| 008 | P2.2 REQ-002 A3 test-hardening changeset (TEST-007 + strengthened 005/006) | PASS (9/9) | validations/active/val-008.md |
| 009 | P2.2 REQ-007 download-chain changeset (commit `1eb5a6a16`) | PASS (after ledger-record fix; first pass FAIL Check 6) | validations/active/val-009.md |
| 010 | P2.2 REQ-004 token-storage write-path changeset | PASS (after fix; first pass FAIL Check 6+9 — design-note false-done + stale DES-012/013 index → LSN-011/LSN-008) | validations/active/val-010.md |
| 011 | P2.2 REQ-006 Slices A+B + A3-R006 hardening (commits d86323246, 3edb705cb, 458a72ba7) | **PASS (4th/closing pass, HEAD d69f70673)**. Passes 1-3 FAILed on ledger drift only (LSN-008 5th/6th/7th recur — primary matrix/DES-index/design-body/local-state.md → root STATE.md body → prose subsections+slot-count); code/test spine clean throughout (15/20/10). Repaired + grep-swept | validations/active/val-011.md |

## open
- needs-review: none
- open blocking findings: 0. **A3-R006-01 FIXED** 2026-07-12 (TEST-014 +2 slots 0620/0601 pin the full refusal mask; mutant-kill demonstrated); A3-R006-02 FIXED (freshness re-stat slot); A3-R006-03 informational accept. A3-R004-M3 RESOLVED by REQ-006 Slice B `3edb705cb`; A3-R004-M1/M2 FIXED. Prior: REQ-002 slice CLOSED (`60a076848`).
- resolved (were deferred):
  - B-R004-01 (REQ-004) — __init__ still forwarded tokenstore_path — RESOLVED by REQ-006 Slice B `3edb705cb` (auth-only ctor across adapter + pystub + WizardStub + AddCloudWizard + REQ-002 tests).
  - B-R004-02 (REQ-004) — DEC-009 style/type gate — CLEARED: ran clean via pre-commit at REQ-004 commit `54b7005e6`.
- deferred (with tickets):
  - A3-R002-TR-08 (NEW) uncancellable-native-wedge → ~GarminAuthChain destroys running QThread → qFatal abort. Only via a pure native loop (no cancellation point); realistic wedges unwind cleanly. Deferred → Phase 1.5 with A2-001 (wedged-worker recovery: thread-heartbeat + kill-and-recreate, or dtor hardening — detach/leak rather than destroy a running thread).
  - A3-R002-TR-03 hasAthlete→25 dead branch — now guarded by TEST-007 routing coverage; accept-with-note
  - A3-R002-TR-06 pystub fidelity vs real src/Python/garminconnect module — dedicated check vs test_adapter_login.py
  - A2-001 sub-interpreter wedge → Phase 1.5 follow-up (thread-heartbeat + kill-and-recreate)
  - A2-008 phishing surface — UX warning, separate ticket
  - sqlite-sidecar migration trigger (DEC-003 re-open if beta sidecar read-time > 500ms or > 10k entries)
  - A3-R002-mutmut — evaluate at A3/REQ-007 cycle
  - A3-R002-hypothesis — evaluate at A3/REQ-014 cycle
  - A3-R001-tool — C++ mutation tool (mull-cxx/cosmic-ray-cpp) at Phase 3 entry
  - VAL-007 slice code-complete (2026-07-05); CLV pass is the immediate next action, then A3 against the gaps the builder flagged: no automated test of the wizard routing itself (page 21 / nextId branches / chain lifecycle across Back-Next), `AddGarminAuth::nextId()`'s hasAthlete→25 branch is dead code until an athlete-select slice exists, `GarminAuthChain`'s `terminate()` last-resort path is unverified against a truly wedged (GIL-held) worker, and `GARMIN_PY_MODULE_DIR` bakes a dev-tree absolute path (installed-path handling deferred to DES-007/NF-Pkg-001 — confirm that deferral is still tracked).
- accept-with-rationale:
  - A3-R002-TR-07 GARMIN_PY_MODULE_DIR deferral tracked; missingModuleYieldsUnknownWithoutCrash is adequate fail-safe proxy (DES-007/NF-Pkg-001)
  - A2-002 library-tracked SSO risk → REQ-NF-Compat-001 (docs)
  - A2-007 parse cancel
  - A3-R002-M10 PEP 3134 chaining
  - A3-R001-M6 fallback colour cosmetic
- drift items: D-01 closed by e4ac2a88b; D-02 closed by e4ac2a88b

## last-clv
- VAL-011 — 2026-07-12 — **PASS (4th/closing pass, HEAD `d69f70673`)** — REQ-006 Slices A+B + A3-R006 hardening. Code/test
  spine clean on EVERY pass (orchestrator-run 15/20/10 — validator has no exec tool); passes 1-3 FAILed on LEDGER DRIFT only,
  the LSN-008 signature recurring across successively deeper loci: 5th (traceability primary matrix REQ-004/006 + DES-012/013
  index + design.md body + entirely-stale local state.md + WIKI "in build"), 6th (root STATE.md's own CASCADE/NEXT_GATE/COUNTS
  vs its banner), 7th (design.md/traceability PROSE subsections + a stale TEST-014 slot count). Repaired each pass; 4th clean
  after a deterministic grep-sweep of the ledger tree. LSN-008 recur:7 miss:6 — deterministic pre-CLV lint is the flagged fix.
- prior: VAL-009 — 2026-07-08 — PASS — REQ-007 download-chain changeset (commit `1eb5a6a16`). First pass
  FAILed Check 6 (LSN-008 class): the committed ledger byproduct still read "uncommitted"/`_pending_`
  (Commit column, banners) and the DES-001/001a/013 index status cells were left Auth-only despite the
  prose design.md sections being current; CASCADE note still called IGarminPyAdapter "Auth-only". Fixed
  by this ledger-record follow-up (Commit columns filled with `1eb5a6a16`, banners flipped, DES index +
  CASCADE refreshed); LSN-008 escalated (recur:3, miss:2, check broadened to DES-index status cells),
  new LSN-010 (commit-column-staleness) captured. Re-verified clean → PASS. Code/design/test spine was
  clean on the first pass (Checks 1-3/8/9); WARN-4 (readFile acceptance untested) is the known deferral.
- prior: VAL-008 — 2026-07-05 — PASS (9/9) — REQ-002 A3 test-hardening changeset (TEST-007 new + TEST-005/006 strengthened). First pass FAILed Check 6 (state.md ## reqs stale + named-slot-vs-QTest-total count conflation); fixed by standardizing on named-slot counts with QTest-total annotation, LSN-008 escalated advisory→guard, re-verified clean. Slice CLOSED.
- prior: VAL-007 — 2026-07-05 — PASS (7/9, 2 tracked WARN) — PyEmbeddedAdapter + tile-routing slice.
  First pass FAILed Check 6 (traceability.md primary DES index + REQ-002 matrix row stale vs
  appendix tables); orchestrator refreshed the index/matrix/banner, captured LSN-008
  (index-vs-detail-drift, advisory), re-verified clean. WARN-4 (no wizard-routing test) and
  WARN-9 (hasAthlete→25 dead branch + terminate() wedged-worker path) carried to A3.
- prior: VAL-006 — 2026-05-24 — PASS (9/9) — REQ-002 end-to-end slice scope
- next: A3 cycle vs the 4 flagged gaps, then continue REQ-002 remaining scope
