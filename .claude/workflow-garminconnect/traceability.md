# Traceability Matrix — Garmin Connect Integration (Phase 1 — download only)

Last updated: 2026-07-11 (Phase 2.2 — **REQ-004 token-storage write path GREEN (uncommitted)** under DEC-014 Option B: `AtomicFile` (TEST-011) + `GarminTokenStore` 0700-dir/0600-atomic-write/two-athlete (TEST-012) + adapter `dump_tokens`/`load_tokens` & `PyAuthOutcome.tokenBlob` (TEST-013); re-verified independently garmin-fast 9/9, garmin-py 1/1, pytest 15/15. DEC-014 accepted (Option B — C++ owns the atomic 0600 write; DES-002 confirmed literal, no amendment). Deferred: `garmin_client.__init__` still forwards `tokenstore_path` so the real library self-writes → `__init__` reconciliation slice (finding B-R004-01); clang-format/ruff/mypy absent in env so DEC-009 gate unrun (B-R004-02). Next: VAL-010 incremental CLV + A3-R004. Prior: REQ-007 activity-download chain GREEN through the worker, committed `1eb5a6a16` (feature) + this ledger-record follow-up. Slice 3: GarminWorker gains a `downloadActivity(activityId, fmt, requestId)` slot + `downloaded(id,bytes)` / `downloadFailed(id, GarminDownloadFailure)` signals (GarminDownloadFailure = new metatype-registered value type, kinds Network/RateLimit/Unknown), mapping PyDownloadOutcome→signals off the GUI thread (DES-001). TEST-010 = 6 garmin-fast slots (new testGarminConnectDownloadWorker target); garmin ctest 8/8 (garmin-fast 7, garmin-py 1); clang-format clean (no mutation). RE-SCOPED from the original slice-3 plan: GarminConnect::readFile staging + FIT→TCX fallback are DEFERRED (need REQ-004/006 tokens/session + PRD-Assumption-B library validation), so REQ-007 is NOT yet fully deployed. Slice 2: IGarminPyAdapter gains `PyDownloadOutcome downloadActivity(activityId, fmt)` (DEC-013 compile-enforced seam, DES-001a); production PyEmbeddedAdapter (DES-013) marshals Python `bytes`→QByteArray binary-exact (embedded NUL preserved via PyBytes_AsStringAndSize), reuses the authenticated client it now retains (REQ-005 session model), classifies by exception TYPE then .kind (connection→Network, rate_limit→RateLimited, foreign/non-bytes→Unknown; LSN-006) via a shared takeRaisedException() helper refactored out of authenticate's classifier. TEST-009 = 8 `garmin-py` slots; testGarminConnectPyAdapter 20/20 (TEST-005 10 + TEST-009 8 + init/cleanup); 4 FakePyAdapters stubbed for the new pure-virtual → garmin-fast 6/6 still GREEN; clang-format reflowed → LSN-007 honored (rebuild+retest 7/7 post-format). Slice 1 (2026-07-08): garmin_client.download_activity (DES-012) fmt map ORIGINAL/TCX + connection/rate_limit translation, TEST-008 6 pytest, adapter suite 12/12. FIT→TCX fallback orchestration deferred to slice 3 C++ readFile (DES-004). Prior: REQ-002 A3 test-hardening fixes GREEN, committed `60a076848`: TEST-007 new wizard-routing suite (4 slots, `garmin-fast`, TR-01) + TEST-006 strengthened (teardown bound tightened + terminate() test, 7 slots, TR-02/TR-04) + TEST-005 strengthened (malformed-result scenarios, 10 slots, TR-05). [slot = named QTest test method; QTest reports +2 per suite for auto init/cleanupTestCase, so runtime totals are 6/9/12 respectively] All 4 TR-01 mutants + TR-02/TR-05 mutants killed; production sources byte-unchanged; garmin-fast 6/6, garmin-py 1/1 from-scratch. New finding TR-08 (uncancellable-native-wedge → `~GarminAuthChain` aborts) deferred → Phase 1.5 with A2-001. Next: VAL-008 drift CLV to close the slice). Prior: VAL-007 PASS 7/9 (2026-07-05); REQ-002 e2e slice GREEN (VAL-006, 2026-05-24).

## REQ → DEC → DES → TEST → COMMIT

| REQ | Description | DEC(s) | DES(s) | TEST(s) | Commit |
|-----|-------------|--------|--------|---------|--------|
| REQ-001 | "Garmin Connect" tile in AddCloudWizard | DEC-001, DEC-005, DEC-011 | DES-004 (DES-003 → REQ-002) | TEST-001 | 6381b90f4 |
| REQ-002 | SSO auth via python-garminconnect | DEC-001, DEC-002, DEC-004, DEC-008, DEC-009, DEC-010, DEC-011, DEC-012, DEC-013 | DES-001, DES-001a, DES-003, DES-003a, DES-008, DES-012, DES-013 | TEST-002 (5, adapter GREEN). TEST-003 (wizard-wiring slice — 13 GREEN; REQ-002 wizard side + REQ-005 + A3 kills). TEST-004 (end-to-end slice — 10 GREEN; GarminWorker + WorkerAuthClient + IGarminPyAdapter seam + REQ-NF-Threads-001 thread-id assertion). TEST-005 (10 slots, PyEmbeddedAdapter marshalling + malformed-result defenses GREEN, `garmin-py`; TR-05). TEST-006 (7 slots, GarminAuthChain lifecycle + tightened teardown bound + deterministic terminate() GREEN, `garmin-fast`; TR-02/TR-04). TEST-007 (4 slots, AddCloudWizard wizard-routing/lifecycle — nextId→page21, ensureGarminAuthPage idempotency, DES-001a destructor order GREEN, `garmin-fast`; TR-01) | e4ac2a88b (RED + bootstrap), 1c355a102 (adapter GREEN + A3 kills), fbb94cff7 (TEST-003 RED), 4b4fd4dd5 (TEST-003 GREEN), 58ae2704e (A3 wizard kills), 15d5e10c7 (VAL-005), 212a4c258 (PyEmbeddedAdapter / DES-013 / TEST-005 GREEN), e9e017fe1 (AddCloudWizard tile-routing + GarminAuthChain + app-build wiring / TEST-006 GREEN), 60a076848 (A3-R002-TR-01/02/04/05 test hardening — TEST-007 + TEST-005/006 strengthened; VAL-008 PASS) |
| REQ-003 | MFA OTP prompt | DEC-001, DEC-002, DEC-004 | DES-001, DES-003, DES-012 | — | — |
| REQ-004 | Per-athlete token storage | DEC-001, DEC-003, DEC-014 | DES-002, DES-006, DES-012, DES-013 | TEST-011 (AtomicFile — `garmin-fast`: perms-set-before-rename/no-world-readable-window, failed-write-leaves-dest-intact, tmp+fsync+rename atomic). TEST-012 (GarminTokenStore — `garmin-fast`: 0700 dir create + 0600 tokens.json + no .tmp residue + two-athlete independence + DES-002 existing-dir-not-tightened). TEST-013 (adapter dump_tokens/load_tokens — pytest: round-trip, reject-tampered-blob→session_expired, REQ-005 password-never-in-blob). All GREEN: garmin-fast 9/9, garmin-py 1/1, pytest 15/15 | _uncommitted_ (write path GREEN; **NOT-done:** garmin_client.__init__ still forwards tokenstore_path → __init__ reconciliation slice, finding B-R004-01; style/type gate unrun, B-R004-02) |
| REQ-005 | Password never persisted | DEC-001, DEC-012 | DES-003, DES-003a, DES-012 | TEST-002 / `test_password_not_retained_on_adapter_instance` (adapter-seam in-memory shim — GREEN). TEST-003 wizard-side enforcement: `passwordFieldIsMasked`, `passwordFieldDisablesAutocomplete`, `passwordClearedAfterSubmit` — currently RED. | e4ac2a88b (RED stub), 1c355a102 (adapter-seam structurally complete), _pending_ (TEST-003 wizard-side RED) |
| REQ-006 | Token file 0600 / owner-only ACL | DEC-001, DEC-003 | DES-002, DES-008 | — | — |
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

| DEC | Question | Status | Date |
|-----|----------|--------|------|
| DEC-001 | Solution shape: monolithic vs staged vs adapter | accepted | 2026-05-17 |
| DEC-002 | Python integration mechanism (direct / mailbox / per-request thread) | accepted (B) | 2026-05-17 |
| DEC-003 | Token + sidecar on-disk layout | accepted (B) | 2026-05-17 |
| DEC-004 | Credentials + MFA dialog UX shape | accepted (B) | 2026-05-17 |
| DEC-005 | Phase-1 CloudService capabilities (Query\|Download) | accepted | 2026-05-17 |
| DEC-006 | Activity file format + staging path (FIT default) | accepted | 2026-05-17 |
| DEC-007 | Rate-limit + retry placement (Python-side worker decorator) | accepted (B) | 2026-05-17 |
| DEC-008 | Testing toolchain (QTest+CTest C++ / pytest+coverage.py Python) | accepted (A) | 2026-05-17 |
| DEC-009 | Style/quality toolchain (existing clang-format/clang-tidy / ruff + mypy --strict Python) | accepted (A) | 2026-05-17 |
| DEC-010 | Pre-commit automation (pre-commit framework, scoped to new Garmin paths) | accepted (A) | 2026-05-17 |
| DEC-011 | Phase-1 rollout (CMake flag GC_WANT_GARMINCONNECT, default OFF) | accepted | 2026-05-17 |
| DEC-012 | Auth-dispatcher seam between `GarminCredentialsPage` and SSO layer (Option A — inject `IGarminAuthClient`) | accepted (A) | 2026-05-24 |
| DEC-013 | Worker ↔ Python adapter seam (Option A — inject `IGarminPyAdapter`) | accepted (A) | 2026-05-24 |
| DEC-014 | Token persistence: library-write vs GC-owned atomic write + perms | accepted (B — C++ owns atomic 0600 write; adapter dumps()/loads()) | 2026-07-11 |

## DES index

| DES | Component | Implements | Status |
|-----|-----------|------------|--------|
| DES-001 | GarminWorker: worker thread + mailbox transport | DEC-002, DEC-013 | drafted (Auth subset GREEN — REQ-002 e2e; + REQ-007 `downloadActivity` slot/`downloaded`/`downloadFailed` GREEN — TEST-010, `1eb5a6a16`) |
| DES-001a | `IGarminPyAdapter` pure-virtual interface (worker ↔ Python seam) | DEC-013 | GREEN (Auth + download seam — TEST-004 `authenticate()`/`PyAuthOutcome`; TEST-009 `downloadActivity()`/`PyDownloadOutcome`) |
| DES-002 | Per-athlete storage layer (tokens, sidecar, backfill state) | DEC-003, DEC-014 | write path GREEN (GarminTokenStore: 0700 dir + 0600 atomic tokens.json — TEST-012; DEC-014 confirmed literal, no amendment). Sidecar/backfill-state layers still drafted |
| DES-003 | AddCloudWizard pages (credentials, MFA, CAPTCHA, ToS, backfill) | DEC-004 | drafted |
| DES-004 | Cloud/GarminConnect CloudService subclass | DEC-001, DEC-005, DEC-006 | drafted |
| DES-005 | gc_rate.py: Python-side rate-limit + retry decorator | DEC-007 | drafted |
| DES-006 | Atomic-write helper (tmp + fsync + rename) | (cross-cutting) | GREEN (`src/Cloud/AtomicFile.{h,cpp}`: writeOver perms-before-rename, flush+fsync, native atomic rename — TEST-011, `garmin-fast`, Qt-only). A3-R004-M1 FIXED: order-recording `TmpWriter` test-seam (null-in-production; writeOver signature/behaviour unchanged) pins perms-set-before-first-byte-write (REQ-NF-Sec-002 no-world-readable-window) |
| DES-007 | CMake feature flag + installer manifest | DEC-011 | drafted |
| DES-008 | Error translation + ErrorBus integration | (cross-cutting) | drafted |
| DES-009 | Bulk backfill controller | (uses DES-001/002/005/006) | drafted |
| DES-010 | Incremental sync flow | (uses DES-001/002/005) | drafted |
| DES-011 | Optional profile auto-fill | (uses DES-001/004/012) | drafted |
| DES-012 | garmin_client.py adapter (stable seam over python-garminconnect) | (cross-cutting — A2-004 fix; DEC-014) | login/download GREEN; DEC-014 `dump_tokens`/`load_tokens` + `session_expired` GREEN (TEST-013); **`__init__` auth-only construction DEFERRED** (B-R004-01 — still forwards `tokenstore_path`). REQ-003/008/012/013 still NotImplementedError |
| DES-003a | `IGarminAuthClient` pure-virtual interface (page ↔ SSO seam) | DEC-012 | drafted (Phase 2.2 — RED only; GREEN adds the header + concrete `WorkerAuthClient`) |
| DES-013 | `PyEmbeddedAdapter`: production `IGarminPyAdapter` over embedded CPython | DEC-013 (production side), DEC-002, DEC-014 | GREEN (Auth + download surfaces — TEST-005 `authenticate` + TEST-009 `downloadActivity` binary-exact marshalling over retained session, `src/Cloud/PyEmbeddedAdapter.{h,cpp}`; GIL RAII, type-then-kind classification). DEC-014: surfaces `PyAuthOutcome.tokenBlob` on Success (GREEN); **stop-forwarding-`tokenstorePath` DEFERRED** (B-R004-01) |

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

| Validation | Trigger | Result | File |
|------------|---------|--------|------|
| 001 | Phase 0 exit | PASS | validations/validation-001.md |
| 002 | Phase 1 exit (after A2 iter 2 clean) | PASS | validations/validation-002.md |
| 003 | Phase 2.2 per-feature exit (REQ-001 / TEST-001) | PASS | validations/validation-003.md |
| 004 | Phase 2.2 per-feature exit (REQ-002 / TEST-002 — adapter slice) | PASS | validations/validation-004.md |

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

## Phase 2.2 REQ-004 artifacts (in repo — write path GREEN, uncommitted)

| Artifact | Path | Serves |
|----------|------|--------|
| Atomic-write helper | `src/Cloud/AtomicFile.{h,cpp}` | DES-006, REQ-NF-Reliab-002. `static writeOver(dest, contents, perms=ReadOwner\|WriteOwner)`: writes `<dest>.tmp` same-dir, sets perms BEFORE write/rename (no world-readable window), flush+fsync, native atomic rename (POSIX `rename(2)` / Win `MoveFileEx(REPLACE_EXISTING\|WRITE_THROUGH)`). Qt-only, Python-free. |
| Per-athlete token store (write side) | `src/Cloud/GarminTokenStore.{h,cpp}` | DES-002, REQ-004, REQ-NF-Sec-002. Statics `directoryFor`/`tokenFilePath`/`save`/`load`; creates `<athlete>/garminconnect/` at 0700 if absent (existing dir NOT tightened), writes `tokens.json` 0600 via AtomicFile. Qt-only. |
| TEST-011 AtomicFile suite | `unittests/Core/garminconnect/testAtomicFile.cpp` | REQ-004/NF-Reliab-002. `garmin-fast`. perms-before-rename, failed-write-leaves-dest-intact (`#ifndef Q_OS_WIN`), atomic-no-residue. |
| TEST-012 GarminTokenStore suite | `unittests/Core/garminconnect/testGarminTokenStore.cpp` | REQ-004. `garmin-fast`. 0700 dir + 0600 file + no .tmp residue + two-athlete independence + `existingDirPermsNotTightened` (DES-002). |
| TEST-013 adapter dump/load | `src/Python/garminconnect/tests/test_token_store.py` | DEC-014, REQ-005. pytest. dump→load round-trip, reject-tampered-blob→`session_expired`, `test_dumped_blob_never_contains_the_password`. |
| Adapter dump/load + tokenBlob | `src/Python/garminconnect/garmin_client.py` (+`dump_tokens`/`load_tokens`, `# NOTE(DEC-014 OQ1)` markers), `src/Cloud/IGarminPyAdapter.h` (`PyAuthOutcome.tokenBlob`), `src/Cloud/PyEmbeddedAdapter.cpp` (surfaces blob post-login), pystub | DEC-014 Option B. **NOT-done:** `__init__` still forwards `tokenstore_path` (B-R004-01). |

## Drift / hygiene notes

| # | Drift | Why it matters | Disposition |
|---|-------|----------------|-------------|
| D-01 | Phase 2.1 bootstrap artifacts — `.pre-commit-config.yaml`, `src/Python/garminconnect/{pyproject.toml,__init__.py,tests/}` — were present on disk but **not git-tracked**. VAL-002 and VAL-003 silently passed over this because both validations read against the working tree, not the index. | The DoD universal floor expects "all hooks in `.pre-commit-config.yaml` pass on staged files" — but a non-tracked hook file is invisible to other clones, CI, or a future bisect. | **Closed by commit `e4ac2a88b`** (2026-05-23). All Phase 2.1 bootstrap files staged together with the REQ-002 RED artifacts. VAL-004 Checks 2 + 6 re-ran against `git ls-files` and confirmed closure. |
| D-02 | The local developer venv lived at `src/Python/garminconnect/.venv/` (created with `virtualenv` for pytest). | Not a workflow artifact; must stay out of git history. | **Closed by commit `e4ac2a88b`** (2026-05-23) — per-directory `src/Python/garminconnect/.gitignore` excludes `.venv/`, `__pycache__/`, `.pytest_cache/`, `.mypy_cache/`, `.ruff_cache/`, `.coverage`, `htmlcov/`, `*.egg-info/`. Scoped to this directory so we don't retrofit the repo-root `.gitignore`. |
