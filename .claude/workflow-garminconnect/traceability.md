# Traceability Matrix — Garmin Connect Integration (Phase 1 — download only)

Last updated: 2026-07-12 (Phase 2.2 — **REQ-006 CLOSED, security-closed**). Slice A load-side perm-refusal committed `d86323246` (GarminTokenStore::loadChecked refuses wider-than-0600 tokens.json with typed TokenPermissionsRejected — TEST-014, garmin-fast). Slice B `__init__` auth-only reconciliation committed `3edb705cb` (library constructed `(email,password)` only, no tokenstore path → self-writes no 2nd token file; C-API `"ss"`, `PyEmbeddedAdapter(modulePath)` — TEST-015 pytest + TEST-016 garmin-py). **REQ-NF-Sec-002 end-to-end MET**; findings B-R004-01 + A3-R004-M3 (blocking) RESOLVED. Verification-Gate PASS on independent re-run: pytest 15/15, garmin-py 20/20, garmin-fast 10/10. VAL-011 PASS (4th/closing pass; passes 1-3 FAILed on ledger drift only). Deferred: C++-side persistence wiring (GarminTokenStore::save routing) rides REQ-007-closure worker-in-CloudService lifecycle. Prior: **REQ-004 token-storage write path GREEN, committed `54b7005e6`** (AtomicFile TEST-011 + GarminTokenStore 0700-dir/0600-atomic-write TEST-012 + adapter dump_tokens/load_tokens & PyAuthOutcome.tokenBlob TEST-013; VAL-010 PASS; style/type gate clean, B-R004-02 cleared). DEC-014 accepted (Option B — C++ owns atomic 0600 write; DES-002 literal). Prior: REQ-007 download chain GREEN through the worker, committed `1eb5a6a16` + VAL-009 PASS (TEST-008/009/010; readFile staging + FIT→TCX fallback DES-004 DEFERRED → needs REQ-004/006 + PRD-Assumption-B; NOT fully deployed). Prior: REQ-002 CLOSED, committed `60a076848` (VAL-008 PASS 9/9; TEST-007 wizard-routing + strengthened TEST-005/006). Prior: VAL-006/007 (2026-05-24/07-05).

## REQ → DEC → DES → TEST → COMMIT

| REQ | Description | DEC(s) | DES(s) | TEST(s) | Commit |
|-----|-------------|--------|--------|---------|--------|
| REQ-001 | "Garmin Connect" tile in AddCloudWizard | DEC-001, DEC-005, DEC-011 | DES-004 (DES-003 → REQ-002) | TEST-001 | 6381b90f4 |
| REQ-002 | SSO auth via python-garminconnect | DEC-001, DEC-002, DEC-004, DEC-008, DEC-009, DEC-010, DEC-011, DEC-012, DEC-013 | DES-001, DES-001a, DES-003, DES-003a, DES-008, DES-012, DES-013 | TEST-002 (5, adapter GREEN). TEST-003 (wizard-wiring slice — 13 GREEN; REQ-002 wizard side + REQ-005 + A3 kills). TEST-004 (end-to-end slice — 10 GREEN; GarminWorker + WorkerAuthClient + IGarminPyAdapter seam + REQ-NF-Threads-001 thread-id assertion). TEST-005 (10 slots, PyEmbeddedAdapter marshalling + malformed-result defenses GREEN, `garmin-py`; TR-05). TEST-006 (7 slots, GarminAuthChain lifecycle + tightened teardown bound + deterministic terminate() GREEN, `garmin-fast`; TR-02/TR-04). TEST-007 (4 slots, AddCloudWizard wizard-routing/lifecycle — nextId→page21, ensureGarminAuthPage idempotency, DES-001a destructor order GREEN, `garmin-fast`; TR-01) | e4ac2a88b (RED + bootstrap), 1c355a102 (adapter GREEN + A3 kills), fbb94cff7 (TEST-003 RED), 4b4fd4dd5 (TEST-003 GREEN), 58ae2704e (A3 wizard kills), 15d5e10c7 (VAL-005), 212a4c258 (PyEmbeddedAdapter / DES-013 / TEST-005 GREEN), e9e017fe1 (AddCloudWizard tile-routing + GarminAuthChain + app-build wiring / TEST-006 GREEN), 60a076848 (A3-R002-TR-01/02/04/05 test hardening — TEST-007 + TEST-005/006 strengthened; VAL-008 PASS) |
| REQ-003 | MFA OTP prompt | DEC-001, DEC-002, DEC-004 | DES-001, DES-003, DES-012 | — | — |
| REQ-004 | Per-athlete token storage | DEC-001, DEC-003, DEC-014 | DES-002, DES-006, DES-012, DES-013 | TEST-011 (AtomicFile — `garmin-fast`: perms-set-before-rename/no-world-readable-window, failed-write-leaves-dest-intact, tmp+fsync+rename atomic). TEST-012 (GarminTokenStore — `garmin-fast`: 0700 dir create + 0600 tokens.json + no .tmp residue + two-athlete independence + DES-002 existing-dir-not-tightened). TEST-013 (adapter dump_tokens/load_tokens — pytest: round-trip, reject-tampered-blob→session_expired, REQ-005 password-never-in-blob). All GREEN: garmin-fast 9/9, garmin-py 1/1, pytest 15/15 | `54b7005e6` (write path GREEN; style/type gate clean via pre-commit — B-R004-02 CLEARED). Former NOT-done (`__init__` still forwards tokenstore_path, B-R004-01) RESOLVED by REQ-006 Slice B `3edb705cb` |
| REQ-005 | Password never persisted | DEC-001, DEC-012 | DES-003, DES-003a, DES-012 | TEST-002 / `test_password_not_retained_on_adapter_instance` (adapter-seam in-memory shim — GREEN). TEST-003 wizard-side enforcement: `passwordFieldIsMasked`, `passwordFieldDisablesAutocomplete`, `passwordClearedAfterSubmit` — currently RED. | e4ac2a88b (RED stub), 1c355a102 (adapter-seam structurally complete), _pending_ (TEST-003 wizard-side RED) |
| REQ-006 | Token file 0600 / owner-only ACL; non-conforming refused on load | DEC-001, DEC-003, DEC-014 | DES-002, DES-008, DES-012, DES-013 | TEST-014 (Slice A load-side perm-refusal — `garmin-fast`, 8 slots: 0600 loads, 0640/0644 refused with typed TokenPermissionsRejected + path + no bytes, absent→NotFound, three-state mutual-exclusion, + A3-R006 hardening: 0620 group-write-only + 0601 other-exec-only pin the FULL refusal mask, + re-stat-not-cached freshness). TEST-015 (Slice B pytest: adapter constructs library auth-only — ctor args==(email,password), len==2 — + dump_tokens() blob persistence). TEST-016 (Slice B `garmin-py`: adapter forwards NO tokenstore path, LAST_TOKENSTORE absent) | `d86323246` (Slice A load-side refusal), `3edb705cb` (Slice B auth-only reconciliation — A3-R004-M3 security-close, REQ-NF-Sec-002 end-to-end MET; findings B-R004-01 + A3-R004-M3 resolved). **NOT-done (deferred):** C++-side persistence wiring (route athlete-config-dir into GarminTokenStore::save) rides the REQ-007-closure worker-in-CloudService lifecycle |
| REQ-007 | Activity download into FIT/TCX pipeline | DEC-001, DEC-002, DEC-006, DEC-013 | DES-001, DES-001a, DES-004, DES-012, DES-013 | TEST-008 (adapter download — 6 pytest GREEN: fmt map ORIGINAL/TCX + connection/rate_limit translation + foreign-exc non-misclassify + unsupported-fmt reject). TEST-009 (PyEmbeddedAdapter.downloadActivity — 8 `garmin-py` slots GREEN: binary-exact bytes marshalling incl. embedded NUL, TCX-fmt forwarded, connection→Network, rate_limit→RateLimited, foreign→Unknown, non-bytes→Unknown, no-session→Unknown, worker-thread GIL). TEST-010 (GarminWorker DownloadActivity op — 6 `garmin-fast` slots GREEN: Success→downloaded(id,bytes) w/ same requestId, activityId+fmt forwarded verbatim, Network/RateLimit/Unknown→downloadFailed w/ matching kind, adapter runs on worker thread not caller [REQ-NF-Threads-001]) | `1eb5a6a16` (download chain GREEN adapter→PyEmbeddedAdapter→worker). **NOT-done (deferred):** GarminConnect::readFile staging bytes as garmin-<id>.<ext> + FIT→TCX fallback (DES-004) — needs REQ-004/006 worker-in-CloudService + token/session lifecycle, and the fallback trigger depends on unvalidated library behaviour (PRD Assumption B). REQ-007 NOT yet fully deployed. |
| REQ-008 | Incremental sync + dedup | DEC-001, DEC-003, DEC-006 | DES-002, DES-010, DES-012 | — | — |
| REQ-009 | First-connect ToS-risk notice | DEC-001, DEC-004 | DES-003 | — | — |
| REQ-010 | Bulk backfill, paginated, resumable | DEC-001, DEC-002, DEC-003, DEC-007 | DES-002, DES-009, DES-006, DES-012 | — | — |
| REQ-011 | Phase-1 read-only (capabilities = Query\|Download) | DEC-001, DEC-005 | DES-004 | — | — |
| REQ-012 | Disconnect deletes tokens; per-account sidecars preserved | DEC-001, DEC-003 | DES-002, DES-004 | — | — |
| REQ-013 | Optional profile auto-fill (nice) | DEC-001 | DES-011, DES-012 | — | — |
| REQ-014 | Friendly error translation | DEC-001, DEC-004 | DES-003, DES-008, DES-012 | — | — |
| REQ-015 | CAPTCHA-detected path | DEC-001, DEC-004 | DES-003, DES-012 | — | — |
| REQ-NF-Perf-001..003 | Perf budgets | DEC-001, DEC-002, DEC-007 | DES-001, DES-005, DES-010 | — | — |
| REQ-NF-Sec-001..004 | Security incl. file-storage residual risk | DEC-001, DEC-003 | DES-002, DES-003, DES-008 | — | — |
| REQ-NF-Compat-001 | Known limitations doc (incl. library-tracked SSO risk) | DEC-001 | DES-012 (adapter contains swap cost) | — | — |
| REQ-NF-Reliab-001..002 | Retry + resumable | DEC-001, DEC-003, DEC-007 | DES-005, DES-006, DES-009, DES-012 | — | — |
| REQ-NF-Threads-001 | Off-GUI execution | DEC-001, DEC-002, DEC-013 | DES-001, DES-001a | TEST-004 / `workerAuthClientForwardsAndAdapterRunsOnWorkerThread` — asserts `QThread::currentThread()` inside FakePyAdapter ≠ test/GUI thread and == worker thread | _pending_ (REQ-002 end-to-end slice) |
| REQ-NF-Cancel-001 | Cancellable sync | DEC-001, DEC-002 | DES-001, DES-009 | — | — |
| REQ-NF-Obs-001 | Structured logs | DEC-001 | DES-008 | — | — |
| REQ-NF-i18n-001 | tr() i18n | DEC-001, DEC-004 | DES-003, DES-008 | — | — |
| REQ-NF-Build-001 | CMake flag GC_WANT_GARMINCONNECT | DEC-001, DEC-011 | DES-007 | — | — |
| REQ-NF-Pkg-001 | Installer bundles Python deps | DEC-001, DEC-011 | DES-007 | — | — |

## DEC index

> DEC **status** is single-homed in `decisions.md` (each DEC's `Status:` field) — DEC-015 SSOT.
> This index carries Question + Date only; drill `decisions.md` for a DEC's accepted-option/status.

| DEC | Question | Date |
|-----|----------|------|
| DEC-001 | Solution shape: monolithic vs staged vs adapter | 2026-05-17 |
| DEC-002 | Python integration mechanism (direct / mailbox / per-request thread) | 2026-05-17 |
| DEC-003 | Token + sidecar on-disk layout | 2026-05-17 |
| DEC-004 | Credentials + MFA dialog UX shape | 2026-05-17 |
| DEC-005 | Phase-1 CloudService capabilities (Query\|Download) | 2026-05-17 |
| DEC-006 | Activity file format + staging path (FIT default) | 2026-05-17 |
| DEC-007 | Rate-limit + retry placement (Python-side worker decorator) | 2026-05-17 |
| DEC-008 | Testing toolchain (QTest+CTest C++ / pytest+coverage.py Python) | 2026-05-17 |
| DEC-009 | Style/quality toolchain (existing clang-format/clang-tidy / ruff + mypy --strict Python) | 2026-05-17 |
| DEC-010 | Pre-commit automation (pre-commit framework, scoped to new Garmin paths) | 2026-05-17 |
| DEC-011 | Phase-1 rollout (CMake flag GC_WANT_GARMINCONNECT, default OFF) | 2026-05-17 |
| DEC-012 | Auth-dispatcher seam between `GarminCredentialsPage` and SSO layer (Option A — inject `IGarminAuthClient`) | 2026-05-24 |
| DEC-013 | Worker ↔ Python adapter seam (Option A — inject `IGarminPyAdapter`) | 2026-05-24 |
| DEC-014 | Token persistence: library-write vs GC-owned atomic write + perms | 2026-07-11 |
| DEC-015 | Ledger status: single canonical source + absence-check drift lint (LSN-008 promotion) | 2026-07-12 |

## DES index

> Status cells use the controlled vocabulary (DEC-015): `drafted | GREEN | committed | CLOSED`
> (+`deferred` tag) led first; the design narrative lives in each DES body in `design.md`.

| DES | Component | Implements | Status |
|-----|-----------|------------|--------|
| DES-001 | GarminWorker: worker thread + mailbox transport | DEC-002, DEC-013 | GREEN (Auth + REQ-007 download slots — TEST-004/TEST-010); further slots drafted |
| DES-001a | `IGarminPyAdapter` pure-virtual interface (worker ↔ Python seam) | DEC-013 | GREEN (Auth + download seam — TEST-004/TEST-009) |
| DES-002 | Per-athlete storage layer (tokens, sidecar, backfill state) | DEC-003, DEC-014 | GREEN (write + load-side refusal — TEST-012/TEST-014); sidecar/backfill drafted |
| DES-003 | AddCloudWizard pages (credentials, MFA, CAPTCHA, ToS, backfill) | DEC-004 | drafted |
| DES-004 | Cloud/GarminConnect CloudService subclass | DEC-001, DEC-005, DEC-006 | drafted |
| DES-005 | gc_rate.py: Python-side rate-limit + retry decorator | DEC-007 | drafted |
| DES-006 | Atomic-write helper (tmp + fsync + rename) | (cross-cutting) | GREEN (AtomicFile — TEST-011) |
| DES-007 | CMake feature flag + installer manifest | DEC-011 | drafted |
| DES-008 | Error translation + ErrorBus integration | (cross-cutting) | drafted |
| DES-009 | Bulk backfill controller | (uses DES-001/002/005/006) | drafted |
| DES-010 | Incremental sync flow | (uses DES-001/002/005) | drafted |
| DES-011 | Optional profile auto-fill | (uses DES-001/004/012) | drafted |
| DES-012 | garmin_client.py adapter (stable seam over python-garminconnect) | (cross-cutting — A2-004 fix; DEC-014) | GREEN (login/download + dump/load/session_expired + auth-only __init__ — TEST-013); REQ-003/008/012/013 drafted |
| DES-003a | `IGarminAuthClient` pure-virtual interface (page ↔ SSO seam) | DEC-012 | drafted (RED only) |
| DES-013 | `PyEmbeddedAdapter`: production `IGarminPyAdapter` over embedded CPython | DEC-013 (production side), DEC-002, DEC-014 | GREEN (Auth + download + tokenBlob + auth-only ctor — TEST-005/TEST-009) |

## Cycles run

| Cycle | Result | File |
|-------|--------|------|
| A0 (Phase 0) | 4 findings, all dispositioned | cycles/cycle-a0.md |
| A1 (Phase 1, iter 1) | 8 FAIL + 11 WARN + persona findings; all dispositioned (15 fix-now, 3 user-input → recommendations accepted, 4 defer, 4 document) | cycles/cycle-a1.md |
| A2 (Phase 1, iter 1) | 8 findings: 5 fix-now (A2-003/004/005/006/docs), 2 defer-with-ticket (A2-001 sub-interp wedge, sqlite-sidecar), 1 accept-with-rationale (A2-002 SSO-change risk → REQ-NF-Compat-001 extended, A2-007 parse cancel, A2-008 phishing) | cycles/cycle-a2.md |
| A2 (Phase 1, iter 2) | Re-run after fix-now remediations applied; clean pass | cycles/cycle-a2-iter2.md |
| A3 / REQ-001 (Phase 2.2) | 4 findings (2 fix-now mutant kills applied to TEST-001, 1 accept-with-rationale, 1 defer-with-ticket for C++ mutation tooling); clean after remediation | cycles/cycle-a3.md |
| A3 / REQ-002 (Phase 2.2) | 13-mutation manual pass; 4 survivors (M2 dead-code, M6 over-broad catch, M10 PEP 3134, M11 empty-msg fallback); 3 fix-now (dead-code dropped + 2 new mutation-kill tests) + 1 accept-with-rationale (M10); re-runs after remediation confirm KILLED. 2 defer-with-ticket entries for tooling (mutmut, hypothesis). | cycles/cycle-a3.md |

## Validations run

This table is the canonical VAL history (DEC-015 — VAL status single-homed here).

| Validation | Trigger | Result | File |
|------------|---------|--------|------|
| 001 | Phase 0 exit | PASS | validations/archive/val-001.md |
| 002 | Phase 1 exit (after A2 iter 2 clean) | PASS | validations/archive/val-002.md |
| 003 | Phase 2.2 per-feature exit (REQ-001 / TEST-001) | PASS | validations/archive/val-003.md |
| 004 | Phase 2.2 REQ-002 adapter slice | PASS | validations/active/val-004.md |
| 005 | Phase 2.2 REQ-002 wizard-wiring slice | PASS | validations/active/val-005.md |
| 006 | Phase 2.2 REQ-002 end-to-end slice | PASS | validations/active/val-006.md |
| 007 | Phase 2.2 REQ-002 PyEmbeddedAdapter + tile-routing slice | PASS (7/9, 2 tracked WARN) | validations/active/val-007.md |
| 008 | Phase 2.2 REQ-002 A3 test-hardening changeset | PASS (9/9) | validations/active/val-008.md |
| 009 | Phase 2.2 REQ-007 download-chain changeset (`1eb5a6a16`) | PASS (after ledger-record fix) | validations/active/val-009.md |
| 010 | Phase 2.2 REQ-004 token write-path changeset (`54b7005e6`) | PASS (after fix) | validations/active/val-010.md |
| 011 | Phase 2.2 REQ-006 Slices A+B + A3-R006 hardening | PASS (4th/closing pass, `d69f70673`) | validations/active/val-011.md |
| 012 | DEC-015 governance migration (status SSOT + drift lint) | PASS (full 9-check, after 1 repair pass) | validations/active/val-012.md |

## Phase 2.1 bootstrap artifacts (in repo)

| Artifact | Path | Serves DEC |
|----------|------|------------|
| CMake option + status line | `src/CMakeLists.txt` (lines around 784, 1458) | DEC-011 (rollout flag wiring) |
| Unittest wiring | `unittests/CMakeLists.txt` | DEC-008 |
| C++ smoke test CMake | `unittests/Core/garminconnect/CMakeLists.txt` | DEC-008 |
| C++ smoke test source | `unittests/Core/garminconnect/testGarminConnectSmoke.cpp` | DEC-008 |
| Python sub-project config | `src/Python/garminconnect/pyproject.toml` | DEC-008, DEC-009 |
| Python package markers | `src/Python/garminconnect/__init__.py`, `tests/__init__.py` | DEC-008 |
| Python smoke test | `src/Python/garminconnect/tests/test_smoke.py` | DEC-008 |
| Pre-commit config | `.pre-commit-config.yaml` | DEC-010 |
| Definition of Done | `.claude/workflow-garminconnect/dod.md` | All — Phase 2 exit gate |

## Phase 2.2 REQ-001 artifacts (in repo)

| Artifact | Path | Serves |
|----------|------|--------|
| GarminConnect CloudService subclass | `src/Cloud/GarminConnect.{h,cpp}` | REQ-001, DES-004, DEC-005 |
| CMake gate for GarminConnect sources | `src/CMakeLists.txt` (`if(GC_WANT_GARMINCONNECT)` block adjacent to Coach) | DEC-011, REQ-NF-Build-001 |
| TEST-001 tile-contract suite | `unittests/Core/garminconnect/testGarminConnectTile.cpp` | REQ-001 acceptance + DoD must-have negative path |
| Test-isolation stub preamble | `unittests/Core/garminconnect/stubs/GCStubPreamble.h` | DEC-008 (test toolchain — Coach-pattern isolation) |
| Garmin-fast test executable wiring | `unittests/Core/garminconnect/CMakeLists.txt` | DEC-008, DEC-010 (CTest `garmin-fast` label) |

## Phase 2.2 REQ-002 artifacts (in repo — adapter slice GREEN)

| Artifact | Path | Serves |
|----------|------|--------|
| GarminClient adapter (GREEN: __init__ + login) | `src/Python/garminconnect/garmin_client.py` | DES-012; lazy `garminconnect` import (tests monkeypatch `_gc`); login() returns `{garmin_user_id, display_name}` or raises `GarminError('auth', …, original)`. REQ-003/007/008/012/013 still raise NotImplementedError per slice. |
| TEST-002 SSO adapter contract | `src/Python/garminconnect/tests/test_adapter_login.py` | 5 tests: happy + bad-creds (DoD negative) + REQ-005 in-memory shim + A3 mutation-kill pair (non-auth-not-misclassified, empty-msg-still-displayable) |
| pytest pythonpath + mypy resolution | `src/Python/garminconnect/pyproject.toml` (`pythonpath = ["."]`, `mypy_path = ["src/Python/garminconnect"]`, `explicit_package_bases = true`) | DEC-008 (pytest collection); DEC-009 (mypy --strict can resolve adapter modules without a "found twice under different module names" collision against the `garminconnect` package on PyPI) |

## Phase 2.2 REQ-002 wizard-wiring slice artifacts (in repo — TEST-003 GREEN)

| Artifact | Path | Serves |
|----------|------|--------|
| `IGarminAuthClient` pure-virtual interface | `src/Cloud/IGarminAuthClient.h` | DES-003a, DEC-012. Defines `GarminAuthSuccess`/`GarminAuthFailure` value types + `authenticate()` + `finished`/`failed` signals. Header-only; compiles without Python/worker deps. |
| `GarminCredentialsPage` wizard page | `src/Cloud/GarminCredentialsPage.{h,cpp}` | DES-003, DEC-004. `QWizardPage` subclass injected with `IGarminAuthClient*`. State machine: Idle→InFlight→Success/Error. Password masked + IME hints (REQ-005); cleared at dispatch time. |
| TEST-003 credentials-page contract | `unittests/Core/garminconnect/testGarminConnectCredentialsPage.cpp` | REQ-002 wizard-side (positive + negative) + REQ-005 wizard-side (mask/IME-hints/post-submit-clear) + REQ-NF-Perf-002 page-side (in-flight disables Next). 9 tests, **GREEN**. |
| TEST-003 CTest wiring | `unittests/Core/garminconnect/CMakeLists.txt` (`testGarminConnectCredentialsPage` target, `garmin-fast` label, AUTOMOC ON, Qt Widgets linked; page sources added for GREEN) | DEC-008, DEC-010, DEC-012. |

## Phase 2.2 REQ-002 PyEmbeddedAdapter slice artifacts (in repo — TEST-005 GREEN)

| Artifact | Path | Serves |
|----------|------|--------|
| `PyEmbeddedAdapter` production bridge | `src/Cloud/PyEmbeddedAdapter.{h,cpp}` | DES-013, DEC-013 (production side), DEC-002. Header Python-free; only the .cpp includes `Python.h` (before Qt headers — `slots` macro clash). GIL via RAII guard; exception classification by type-then-kind (LSN-006 / A3-R002-M6): GarminError.kind 'auth'→AuthFailed, 'connection'→Network, else Unknown; foreign exceptions can never reach AuthFailed. Requires CPython ≥3.12 (`PyErr_GetRaisedException`). |
| TEST-005 embedded-marshalling suite | `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp` | REQ-002 production-adapter side. 10 slots GREEN: uninit-interpreter fail-safe, success marshal + ctor-arg fidelity, auth/connection/rate_limit/ValueError classification (negatives assert NOT AuthFailed), missing-module fail-safe, off-main-thread call, GIL balance + unicode round-trip. |
| Scriptable Python stub module | `unittests/Core/garminconnect/pystubs/garmin_client.py` | TEST-005 fixture — mirrors the real DES-012 surface (`GarminClient`, `GarminError.kind`); scenario-driven via `PyRun_SimpleString`. Real adapter behavior stays owned by `src/Python/garminconnect/tests/` (pytest). |
| TEST-005 CTest wiring | `unittests/Core/garminconnect/CMakeLists.txt` (`testGarminConnectPyAdapter` target, label **`garmin-py`**, links `Python3::Python` via `Development.Embed`) | DEC-008, DEC-010. `garmin-fast` stays Python-free by construction; the new label isolates the embedded-CPython dependency. |

## Phase 2.2 REQ-002 wizard tile-routing slice artifacts (in repo — TEST-006 GREEN, VAL-007 scope)

| Artifact | Path | Serves |
|----------|------|--------|
| `GarminAuthChain` RAII assembly | `src/Cloud/GarminAuthChain.{h,cpp}` | DES-003 (impl. note), DES-001 invariant 3, DES-001a lifecycle. Owns QThread+GarminWorker+WorkerAuthClient around a non-owned `IGarminPyAdapter*`; bounded quit()+wait() teardown. Python-free (LSN-007 verified). |
| TEST-006 chain contract | `unittests/Core/garminconnect/testGarminConnectAuthChain.cpp` | 6 slots GREEN: construction, success round-trip, off-thread adapter call, failure-kind propagation, idle teardown, post-request teardown. `garmin-fast` label, FakePyAdapter (DEC-013). |
| TEST-006 CTest wiring | `unittests/Core/garminconnect/CMakeLists.txt` (`testGarminConnectAuthChain` target) | DEC-008, DEC-010. |
| AddCloudWizard Garmin tile-routing | `src/Cloud/AddCloudWizard.{h,cpp}` | DES-003 impl. note. New page id 21 (`AddGarminAuth` : `GarminCredentialsPage`); `AddService`/`AddConsent::nextId()` branch on `cloudService->id() == "Garmin Connect"`; `ensureGarminAuthPage()` lazily builds `PyEmbeddedAdapter`+`GarminAuthChain`; `~AddCloudWizard()` tears down in DES-001a order. All fenced `#ifdef GC_WANT_GARMINCONNECT`; every other service's routing byte-for-byte unchanged. No automated test of the routing itself — flagged for A3. |
| App-build wiring | `src/CMakeLists.txt` (`if(GC_WANT_GARMINCONNECT)` block only) | DES-007, DES-013. Adds the 6 new Garmin sources to `target_sources`; `find_package(Python3 COMPONENTS Development.Embed)` + `Python3::Python` link (flag-gated); `GARMIN_PY_MODULE_DIR` compile definition (dev default). Verified: 6 flag-ON app objects (AddCloudWizard, GarminAuthChain, PyEmbeddedAdapter, GarminCredentialsPage, GarminWorker, WorkerAuthClient) compile clean under `GC_WANT_GARMINCONNECT=ON`. |

## Phase 2.2 REQ-002 A3 test-hardening slice artifacts (in repo — committed `60a076848`)

| Artifact | Path | Serves |
|----------|------|--------|
| TEST-007 wizard-routing/lifecycle suite | `unittests/Core/garminconnect/testGarminConnectWizardRouting.cpp` (4 slots, `garmin-fast`) | REQ-002 / A3-R002-TR-01. Compiles the real `src/Cloud/AddCloudWizard.cpp` under test (GUI headers neutralised by force-included stub). Kills 4 mutants: `AddService::nextId`/`AddConsent::nextId` "Garmin Connect"→page-21 routing (M1/M2), `ensureGarminAuthPage()` idempotency guard (M3), `~AddCloudWizard()` DES-001a destructor order chain-before-adapter (M4). Fake Python-free `IGarminPyAdapter` keeps it off CPython. |
| Wizard-routing test stub preamble | `unittests/Core/garminconnect/stubs/WizardStubPreamble.h` | TEST-007 fixture — guard-predefinition force-include (same pattern as `GCStubPreamble.h`) short-circuiting co-located real headers (`CloudService.h`, `OAuthDialog.h`, `PyEmbeddedAdapter.h`, `RideItem.h`, `Perspective.h`). |
| TEST-006 strengthened | `unittests/Core/garminconnect/testGarminConnectAuthChain.cpp` (6→7 named slots; 9 QTest-reported incl. auto init/cleanup) | REQ-002 / A3-R002-TR-02+TR-04. Idle/post-request teardown bound tightened to «kQuitWaitMs (LSN-009) so a skipped graceful `quit()` now FAILs; new deterministic non-GIL busy-loop test forces quit-timeout→`terminate()` and asserts bounded clean finish. |
| TEST-005 strengthened | `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp` (8→10 named slots; 12 QTest-reported incl. init/cleanup) + `pystubs/garmin_client.py` | REQ-002 / A3-R002-TR-05. New SCENARIOs (non-dict result; dict missing `garmin_user_id`/`display_name`) + asserts that the outcome is Unknown-with-message, never a spurious Success — kills a delete-both-defensive-checks mutant. |
| New finding TR-08 (deferred) | `src/Cloud/GarminAuthChain.cpp` (`~GarminAuthChain` last-resort path) | A3-R002-TR-08. A genuinely uncancellable native busy-loop (no cancellation point) defeats `QThread::terminate()`; the dtor then destroys a running `QThread` → `qFatal` abort. Only reachable via a pure native wedge; realistic wedges unwind cleanly. Deferred → Phase 1.5 with A2-001 (wedged-worker recovery). |

## Phase 2.2 REQ-004 artifacts (write path GREEN, committed `54b7005e6`)

| Artifact | Path | Serves |
|----------|------|--------|
| Atomic-write helper | `src/Cloud/AtomicFile.{h,cpp}` | DES-006, REQ-NF-Reliab-002. `static writeOver(dest, contents, perms=ReadOwner\|WriteOwner)`: writes `<dest>.tmp` same-dir, sets perms BEFORE write/rename (no world-readable window), flush+fsync, native atomic rename (POSIX `rename(2)` / Win `MoveFileEx(REPLACE_EXISTING\|WRITE_THROUGH)`). Qt-only, Python-free. |
| Per-athlete token store (write side) | `src/Cloud/GarminTokenStore.{h,cpp}` | DES-002, REQ-004, REQ-NF-Sec-002. Statics `directoryFor`/`tokenFilePath`/`save`/`load`; creates `<athlete>/garminconnect/` at 0700 if absent (existing dir NOT tightened), writes `tokens.json` 0600 via AtomicFile. Qt-only. |
| TEST-011 AtomicFile suite | `unittests/Core/garminconnect/testAtomicFile.cpp` | REQ-004/NF-Reliab-002. `garmin-fast`. perms-before-rename, failed-write-leaves-dest-intact (`#ifndef Q_OS_WIN`), atomic-no-residue. |
| TEST-012 GarminTokenStore suite | `unittests/Core/garminconnect/testGarminTokenStore.cpp` | REQ-004. `garmin-fast`. 0700 dir + 0600 file + no .tmp residue + two-athlete independence + `existingDirPermsNotTightened` (DES-002). |
| TEST-013 adapter dump/load | `src/Python/garminconnect/tests/test_token_store.py` | DEC-014, REQ-005. pytest. dump→load round-trip, reject-tampered-blob→`session_expired`, `test_dumped_blob_never_contains_the_password`. |
| Adapter dump/load + tokenBlob | `src/Python/garminconnect/garmin_client.py` (+`dump_tokens`/`load_tokens`, `# NOTE(DEC-014 OQ1)` markers), `src/Cloud/IGarminPyAdapter.h` (`PyAuthOutcome.tokenBlob`), `src/Cloud/PyEmbeddedAdapter.cpp` (surfaces blob post-login), pystub | DEC-014 Option B. `__init__` auth-only (no `tokenstore_path` forwarding) DONE in REQ-006 Slice B `3edb705cb` — B-R004-01 RESOLVED. |

## Phase 2.2 REQ-006 artifacts — Slice A (load-side perm refusal, GREEN, committed `d86323246`)

REQ-006 → DEC-003/DEC-014 → DES-002 (load invariant) / DES-008 (`TokenPermissionsRejected` key). Slice A = the acceptance criterion proper (C++ load-side). Slice B (the `__init__` auth-only reconciliation / A3-R004-M3 security-close, supersedes B-R004-01) is a SEPARATE later dispatch.

| Artifact | Path | Serves |
|----------|------|--------|
| Token store load-side perm check | `src/Cloud/GarminTokenStore.{h,cpp}` (+`enum class LoadStatus{Ok,NotFound,TokenPermissionsRejected}`, `struct LoadResult{status,bytes,path,isOk(),isRejected()}`, `static LoadResult loadChecked(athleteConfigDir)`) | DES-002, REQ-006, REQ-NF-Sec-002. POSIX: refuses `tokens.json` with ANY group/other bit set — no bytes returned, path exposed (DES-008 `%1`), caller forces fresh SSO. Perms read at load time (A3-R004-M1); decided from mode bits so it holds under root (A3-R004-08). Plain `load(bool*)` left untouched (REQ-004 callers unchanged). Windows ACL = TODO stub (A3-R004-09, Phase-2 CI). Qt-only, Python-free. |
| TEST-014 load-side suite | `unittests/Core/garminconnect/testGarminTokenStore_load.cpp` (8 slots, `garmin-fast`) | REQ-006. 0600 loads exact bytes; 0640/0644 refused (typed, no bytes, path); absent→NotFound (distinct); three-state mutual-exclusion; + A3-R006 hardening (`458a72ba7`): 0620 group-write-only + 0601 other-exec-only pin the FULL refusal mask, + re-stat-not-cached freshness. garmin-fast 10/10. |

## Phase 2.2 REQ-006 artifacts — Slice B (__init__ auth-only reconciliation, GREEN, committed `3edb705cb`) — closes NF-Sec-002 end-to-end

A3-R004-M3 security-close (supersedes B-R004-01). Library constructed AUTH-ONLY so it self-writes no 2nd token file; C++-owned 0600 write is the sole token file. Verification-Gate PASS: pytest 15/15, garmin-py 20/20, garmin-fast 10/10.

| Artifact | Path | Serves |
|----------|------|--------|
| Adapter auth-only ctor | `src/Python/garminconnect/garmin_client.py` (`__init__(email, password)`; `_gc.Garmin(email, password)`; OQ1 real-lib 2-arg NOTE) | REQ-006, DEC-014 Option B, A3-R004-M3, REQ-NF-Sec-002. Drops all `tokenstore_path` forwarding — library persists nothing. |
| PyEmbeddedAdapter 1-arg ctor | `src/Cloud/PyEmbeddedAdapter.{h,cpp}` (`PyEmbeddedAdapter(modulePath)` explicit; dropped `tokenstorePath` member; C-API `"sss"`→`"ss"`), `src/Cloud/AddCloudWizard.cpp` (dropped unused `tokenstorePath` local + 2nd ctor arg; C++-persist wiring deferred to REQ-007-closure) | REQ-006 production seam. |
| TEST-015 auth-only pytest | `src/Python/garminconnect/tests/test_adapter_login.py` (`test_login_happy_path_constructs_auth_only_and_exposes_blob`) | REQ-006/M3. Captures ALL forwarded ctor args, asserts `== (email,password)` + `len==2` (no path), persistence via `dump_tokens()` blob. Replaces old `assert tokenstore.exists()`. |
| TEST-016 no-path-forwarded garmin-py | `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp` (`QVERIFY2(stubAttr("LAST_TOKENSTORE").isNull())`) + `pystubs/garmin_client.py` (2-arg ctor, dropped LAST_TOKENSTORE) + `stubs/WizardStubPreamble.h` (1-arg stub ctor) + test_adapter_download.py / test_token_store.py (fakes+call sites → 2-arg) | REQ-006/M3. Inverts the old forwarded-path assertion; 20/20 PyAdapter subtests intact. |

## Drift / hygiene notes

| # | Drift | Why it matters | Disposition |
|---|-------|----------------|-------------|
| D-01 | Phase 2.1 bootstrap artifacts — `.pre-commit-config.yaml`, `src/Python/garminconnect/{pyproject.toml,__init__.py,tests/}` — were present on disk but **not git-tracked**. VAL-002 and VAL-003 silently passed over this because both validations read against the working tree, not the index. | The DoD universal floor expects "all hooks in `.pre-commit-config.yaml` pass on staged files" — but a non-tracked hook file is invisible to other clones, CI, or a future bisect. | **Closed by commit `e4ac2a88b`** (2026-05-23). All Phase 2.1 bootstrap files staged together with the REQ-002 RED artifacts. VAL-004 Checks 2 + 6 re-ran against `git ls-files` and confirmed closure. |
| D-02 | The local developer venv lived at `src/Python/garminconnect/.venv/` (created with `virtualenv` for pytest). | Not a workflow artifact; must stay out of git history. | **Closed by commit `e4ac2a88b`** (2026-05-23) — per-directory `src/Python/garminconnect/.gitignore` excludes `.venv/`, `__pycache__/`, `.pytest_cache/`, `.mypy_cache/`, `.ruff_cache/`, `.coverage`, `htmlcov/`, `*.egg-info/`. Scoped to this directory so we don't retrofit the repo-root `.gitignore`. |
