# Traceability Matrix — Garmin Connect Integration (Phase 1 — download only)

Last updated: 2026-05-24 (Phase 2.2 — REQ-002 C++ wizard-wiring slice TEST-003 **GREEN** — `IGarminAuthClient.h` + `GarminCredentialsPage.{h,cpp}` committed `4b4fd4dd5`; all 9 TEST-003 tests pass; garmin-fast suite 3/3 executables 23/23 tests green; pre-commit clang-format clean. A3 wizard-slice adversarial cycle next, then VAL-005). Prior milestone: REQ-002 adapter slice GREEN (commit `1c355a102`, A3 clean, VAL-004 PASS).

## REQ → DEC → DES → TEST → COMMIT

| REQ | Description | DEC(s) | DES(s) | TEST(s) | Commit |
|-----|-------------|--------|--------|---------|--------|
| REQ-001 | "Garmin Connect" tile in AddCloudWizard | DEC-001, DEC-005, DEC-011 | DES-004 (DES-003 → REQ-002) | TEST-001 | 6381b90f4 |
| REQ-002 | SSO auth via python-garminconnect | DEC-001, DEC-002, DEC-004, DEC-008, DEC-009, DEC-010, DEC-011, DEC-012, DEC-013 | DES-001, DES-001a, DES-003, DES-003a, DES-008, DES-012 | TEST-002 (5, adapter GREEN). TEST-003 (wizard-wiring slice — 13 GREEN; REQ-002 wizard side + REQ-005 + A3 kills). TEST-004 (end-to-end slice — 10 GREEN; GarminWorker + WorkerAuthClient + IGarminPyAdapter seam + REQ-NF-Threads-001 thread-id assertion) | e4ac2a88b (RED + bootstrap), 1c355a102 (adapter GREEN + A3 kills), fbb94cff7 (TEST-003 RED), 4b4fd4dd5 (TEST-003 GREEN), 58ae2704e (A3 wizard kills), 15d5e10c7 (VAL-005), _pending_ (TEST-004 + VAL-006 on branch worktree-garmin-req002-e2e) |
| REQ-003 | MFA OTP prompt | DEC-001, DEC-002, DEC-004 | DES-001, DES-003, DES-012 | — | — |
| REQ-004 | Per-athlete token storage | DEC-001, DEC-003 | DES-002, DES-006 | — | — |
| REQ-005 | Password never persisted | DEC-001, DEC-012 | DES-003, DES-003a, DES-012 | TEST-002 / `test_password_not_retained_on_adapter_instance` (adapter-seam in-memory shim — GREEN). TEST-003 wizard-side enforcement: `passwordFieldIsMasked`, `passwordFieldDisablesAutocomplete`, `passwordClearedAfterSubmit` — currently RED. | e4ac2a88b (RED stub), 1c355a102 (adapter-seam structurally complete), _pending_ (TEST-003 wizard-side RED) |
| REQ-006 | Token file 0600 / owner-only ACL | DEC-001, DEC-003 | DES-002, DES-008 | — | — |
| REQ-007 | Activity download into FIT/TCX pipeline | DEC-001, DEC-002, DEC-006 | DES-001, DES-004, DES-012 | — | — |
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

## DES index

| DES | Component | Implements | Status |
|-----|-----------|------------|--------|
| DES-001 | GarminWorker: worker thread + mailbox transport | DEC-002, DEC-013 | drafted (Auth-only subset GREEN — REQ-002 end-to-end slice) |
| DES-001a | `IGarminPyAdapter` pure-virtual interface (worker ↔ Python seam) | DEC-013 | GREEN (Auth-only surface — TEST-004 locks in `authenticate()` + `PyAuthOutcome`) |
| DES-002 | Per-athlete storage layer (tokens, sidecar, backfill state) | DEC-003 | drafted |
| DES-003 | AddCloudWizard pages (credentials, MFA, CAPTCHA, ToS, backfill) | DEC-004 | drafted |
| DES-004 | Cloud/GarminConnect CloudService subclass | DEC-001, DEC-005, DEC-006 | drafted |
| DES-005 | gc_rate.py: Python-side rate-limit + retry decorator | DEC-007 | drafted |
| DES-006 | Atomic-write helper (tmp + fsync + rename) | (cross-cutting) | drafted |
| DES-007 | CMake feature flag + installer manifest | DEC-011 | drafted |
| DES-008 | Error translation + ErrorBus integration | (cross-cutting) | drafted |
| DES-009 | Bulk backfill controller | (uses DES-001/002/005/006) | drafted |
| DES-010 | Incremental sync flow | (uses DES-001/002/005) | drafted |
| DES-011 | Optional profile auto-fill | (uses DES-001/004/012) | drafted |
| DES-012 | garmin_client.py adapter (stable seam over python-garminconnect) | (cross-cutting — A2-004 fix) | stub-in-repo (Phase 2.2 — methods raise NotImplementedError per REQ; class/error shape matches design) |
| DES-003a | `IGarminAuthClient` pure-virtual interface (page ↔ SSO seam) | DEC-012 | drafted (Phase 2.2 — RED only; GREEN adds the header + concrete `WorkerAuthClient`) |

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

## Drift / hygiene notes

| # | Drift | Why it matters | Disposition |
|---|-------|----------------|-------------|
| D-01 | Phase 2.1 bootstrap artifacts — `.pre-commit-config.yaml`, `src/Python/garminconnect/{pyproject.toml,__init__.py,tests/}` — were present on disk but **not git-tracked**. VAL-002 and VAL-003 silently passed over this because both validations read against the working tree, not the index. | The DoD universal floor expects "all hooks in `.pre-commit-config.yaml` pass on staged files" — but a non-tracked hook file is invisible to other clones, CI, or a future bisect. | **Closed by commit `e4ac2a88b`** (2026-05-23). All Phase 2.1 bootstrap files staged together with the REQ-002 RED artifacts. VAL-004 Checks 2 + 6 re-ran against `git ls-files` and confirmed closure. |
| D-02 | The local developer venv lived at `src/Python/garminconnect/.venv/` (created with `virtualenv` for pytest). | Not a workflow artifact; must stay out of git history. | **Closed by commit `e4ac2a88b`** (2026-05-23) — per-directory `src/Python/garminconnect/.gitignore` excludes `.venv/`, `__pycache__/`, `.pytest_cache/`, `.mypy_cache/`, `.ruff_cache/`, `.coverage`, `htmlcov/`, `*.egg-info/`. Scoped to this directory so we don't retrofit the repo-root `.gitignore`. |
