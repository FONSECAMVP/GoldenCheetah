# State — Garmin Connect Integration

_Updated: 2026-05-24 — VAL-005 PASS (9/9); A3 wizard-slice 4 survivors killed (M3/M4/M6/M8); commits 4b4fd4dd5 + 58ae2704e._

## phase
- current: Phase 2.2
- active feature: REQ-002 (slice: C++ wizard-wiring — **COMPLETE**; next: GarminWorker + WorkerAuthClient + AddCloudWizard wiring)
- next gate: VAL-006 (REQ-002 end-to-end)
- last-clean-VAL: VAL-005 (2026-05-24 — wizard-wiring slice)

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

## des
| DES | implements | status | last |
|---|---|---|---|
| 001 | DEC-002 | drafted | 2026-05-17 |
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

## reqs
| REQ | cat | DECs | DESs | TESTs | status |
|---|---|---|---|---|---|
| 001 | must | 001,005,011 | 004 | T-001 | deployed (6381b90f4) |
| 002 | must | 001,002,004,008,009,010,011,012 | 001,003,003a,008,012 | T-002 (GREEN, 5 tests), T-003 (GREEN, 13 tests — wizard slice + A3 kills) | VAL-005 PASS |
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

## vals
| VAL | trigger | result | file |
|---|---|---|---|
| 001 | P0 exit | PASS | validations/archive/val-001.md |
| 002 | P1 exit (after A2-iter2) | PASS | validations/archive/val-002.md |
| 003 | P2.2 REQ-001 / TEST-001 | PASS | validations/archive/val-003.md |
| 004 | P2.2 REQ-002 adapter slice | PASS | validations/active/val-004.md |
| 005 | P2.2 REQ-002 wizard-wiring slice | PASS | validations/active/val-005.md |

## open
- needs-review: none
- open blocking findings: 0
- deferred (with tickets):
  - A2-001 sub-interpreter wedge → Phase 1.5 follow-up (thread-heartbeat + kill-and-recreate)
  - A2-008 phishing surface — UX warning, separate ticket
  - sqlite-sidecar migration trigger (DEC-003 re-open if beta sidecar read-time > 500ms or > 10k entries)
  - A3-R002-mutmut — evaluate at A3/REQ-007 cycle
  - A3-R002-hypothesis — evaluate at A3/REQ-014 cycle
  - A3-R001-tool — C++ mutation tool (mull-cxx/cosmic-ray-cpp) at Phase 3 entry
- accept-with-rationale:
  - A2-002 library-tracked SSO risk → REQ-NF-Compat-001 (docs)
  - A2-007 parse cancel
  - A3-R002-M10 PEP 3134 chaining
  - A3-R001-M6 fallback colour cosmetic
- drift items: D-01 closed by e4ac2a88b; D-02 closed by e4ac2a88b

## last-clv
- VAL-005 — 2026-05-24 — PASS (9/9) — REQ-002 wizard-wiring slice scope
- next: VAL-006 on REQ-002 end-to-end (GarminWorker + WorkerAuthClient + AddCloudWizard wiring)
