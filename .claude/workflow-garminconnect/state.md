# State — Garmin Connect Integration

_Updated: 2026-07-05 — REQ-002 PyEmbeddedAdapter + AddCloudWizard tile-routing slice CLOSED: A3 clean (TR-01/-02/-04/-05 fixed via TEST-007 + strengthened TEST-005/006), VAL-008 PASS 9/9. TR-08 (uncancellable-native-wedge abort) + TR-06 (pystub fidelity) deferred, non-blocking. LSN-008 escalated to guard (2nd ledger-drift). A3 changeset committed `60a076848` (pre-commit clang-format reformatted testGarminConnectAuthChain.cpp → LSN-007 honored: from-scratch rebuild + 9/9 retest before commit). Next: start next REQ (REQ-007 download / REQ-003 MFA)._

## phase
- current: Phase 2.2
- active feature: REQ-002 — PyEmbeddedAdapter + AddCloudWizard tile-routing slice **CLOSED** (A3 clean, VAL-008 PASS 9/9); Authenticate flow slices all done. Ready to start next REQ.
- next gate: REQ-002 PyEmbeddedAdapter + tile-routing slice is CLOSED + COMMITTED (`60a076848`; A3 clean, VAL-008 PASS 9/9). Next: start the next REQ (candidate: REQ-007 activity download, or REQ-003 MFA prompt) via the TDD loop.
- last-clean-VAL: VAL-008 (2026-07-05 — REQ-002 A3 test-hardening changeset, PASS 9/9)
- last-cycle: A3 REQ-002 tile-routing (2026-07-05) — FINDINGS then FIXED: TR-01/-02/-04/-05 resolved (TEST-007 + TEST-006/005 strengthening), TR-08 new→defer; see cycles/active/a3-req-002-tile-routing.md

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

## des
| DES | implements | status | last |
|---|---|---|---|
| 001 | DEC-002, DEC-013 | drafted (Auth-only subset GREEN) | 2026-05-24 |
| 001a | DEC-013 | GREEN (Auth-only surface) | 2026-05-24 |
| 002 | DEC-003 | drafted | 2026-05-17 |
| 003 | DEC-004, DEC-012 | drafted | 2026-05-24 |
| 003a | DEC-012 | drafted | 2026-05-24 |
| 004 | DEC-001, DEC-005, DEC-006 | drafted | 2026-05-17 |
| 005 | DEC-007 | drafted | 2026-05-17 |
| 006 | cross-cutting (atomic writer) | drafted | 2026-05-17 |
| 007 | DEC-011 | drafted | 2026-05-17 |
| 008 | cross-cutting (error bus) | drafted | 2026-05-17 |
| 009 | uses DES-001/002/005/006 (bulk backfill) | drafted | 2026-05-17 |
| 010 | uses DES-001/002/005 (incremental sync) | drafted | 2026-05-17 |
| 011 | uses DES-001/004/012 (profile auto-fill) | drafted | 2026-05-17 |
| 012 | adapter seam over python-garminconnect (A2-004 fix) | stub-in-repo (REQ-002 GREEN partial) | 2026-05-23 |
| 013 | production PyEmbeddedAdapter (DEC-013 production side, DEC-002) | GREEN (Auth surface, TEST-005) | 2026-07-04 |

## reqs
| REQ | cat | DECs | DESs | TESTs | status |
|---|---|---|---|---|---|
| 001 | must | 001,005,011 | 004 | T-001 | deployed (6381b90f4) |
| 002 | must | 001,002,004,008,009,010,011,012,013 | 001,001a,003,003a,008,012,013 | T-002 (GREEN, 5), T-003 (GREEN, 13), T-004 (GREEN, 10), T-005 (GREEN, 10 slots), T-006 (GREEN, 7 slots), T-007 (GREEN, 4 slots) [slot counts exclude QTest auto init/cleanup] | production PyEmbeddedAdapter + wizard tile-routing GREEN; A3 clean (TEST-007 + strengthened 005/006); **VAL-008 PASS 9/9 — slice CLOSED + committed `60a076848`** (TR-08/TR-06 deferred) |
| 003 | must | 001,002,004 | 001,003,012 | — | not started |
| 004 | must | 001,003 | 002,006 | — | not started |
| 005 | must | 001,012 | 003,003a,012 | T-002 (adapter half: password-not-retained); T-003 (wizard side — GREEN) | tested (A3 pending) |
| 006 | must | 001,003 | 002,008 | — | not started |
| 007 | must | 001,002,006 | 001,004,012 | — | not started |
| 008 | must | 001,003,006 | 002,010,012 | — | not started |
| 009 | must | 001,004 | 003 | — | not started |
| 010 | must | 001,002,003,007 | 002,009,006,012 | — | not started |
| 011 | must | 001,005 | 004 | T-001 (capabilities mutants #2/#3) | tested via REQ-001 |
| 012 | should | 001,003 | 002,004 | — | not started |
| 013 | nice | 001 | 011,012 | — | not started |
| 014 | must | 001,004 | 003,008,012 | — | not started |
| 015 | must | 001,004 | 003,012 | — | not started |
| NF-Perf-001..003 | must | 001,002,007 | 001,005,010 | — | not started |
| NF-Sec-001..004 | must | 001,003 | 002,003,008 | T-002 (REQ-005 adapter-side) | partial |
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

## open
- needs-review: none
- open blocking findings: 0 — A3-R002-TR-01/-02 fixed (TEST-007 new; TEST-006/005 strengthened; all mutants killed, production byte-unchanged, garmin-fast 6/6 + garmin-py 1/1 from-scratch). TR-04/-05 follow-ons done in the same pass. VAL-008 PASS 9/9; committed `60a076848`. Slice CLOSED.
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
- VAL-008 — 2026-07-05 — PASS (9/9) — REQ-002 A3 test-hardening changeset (TEST-007 new + TEST-005/006 strengthened). First pass FAILed Check 6 (state.md ## reqs stale + named-slot-vs-QTest-total count conflation); fixed by standardizing on named-slot counts with QTest-total annotation, LSN-008 escalated advisory→guard, re-verified clean. Slice CLOSED.
- prior: VAL-007 — 2026-07-05 — PASS (7/9, 2 tracked WARN) — PyEmbeddedAdapter + tile-routing slice.
  First pass FAILed Check 6 (traceability.md primary DES index + REQ-002 matrix row stale vs
  appendix tables); orchestrator refreshed the index/matrix/banner, captured LSN-008
  (index-vs-detail-drift, advisory), re-verified clean. WARN-4 (no wizard-routing test) and
  WARN-9 (hasAthlete→25 dead branch + terminate() wedged-worker path) carried to A3.
- prior: VAL-006 — 2026-05-24 — PASS (9/9) — REQ-002 end-to-end slice scope
- next: A3 cycle vs the 4 flagged gaps, then continue REQ-002 remaining scope
