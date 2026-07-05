# A3 — Test Hardening — REQ-002 PyEmbeddedAdapter + tile-routing slice

- **Cycle:** A3 (test hardening)
- **Date:** 2026-07-05
- **Scope:** `src/Cloud/PyEmbeddedAdapter.{h,cpp}`, `src/Cloud/GarminAuthChain.{h,cpp}`, `src/Cloud/AddCloudWizard.{h,cpp}`, `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp` (TEST-005), `testGarminConnectAuthChain.cpp` (TEST-006), `pystubs/garmin_client.py`
- **Run by:** `qgdw-adversary` (fresh context) — findings merged by orchestrator
- **Verdict:** FINDINGS — **2 blocking (TR-01, TR-02)**, 3 non-blocking, 2 informational. NOT adequate to exit A3 as-is.
- **Method note (LSN-007):** all PASS results from a fresh-from-scratch rebuild of the two test targets (8/8 + 10/10, `garmin-fast` 0.30s / `garmin-py` 0.04s). TR-02 is a **real executed mutant** (built + relinked + ran under scratchpad; no project file modified).

## Findings

| ID | severity | summary | disposition | resolution plan |
|---|---|---|---|---|
| A3-R002-TR-01 | blocking | AddCloudWizard.cpp routing/lifecycle compiled by zero tests | fix-now | GCStubPreamble-style routing test target exercising real `AddService/AddConsent::nextId()`, `ensureGarminAuthPage()` idempotency, destructor teardown order |
| A3-R002-TR-02 | blocking | TEST-006 teardown bound is a proven mutation survivor (masks broken graceful quit) | fix-now | tighten idle-teardown assert to «kQuitWaitMs (e.g. <200ms); add busy-loop FakePyAdapter test that forces quit-timeout→terminate() and asserts kill within kTerminateWaitMs |
| A3-R002-TR-03 | non-blocking | `hasAthlete ? 25 : 30` doubly-dead (no AthleteID + page 25 has no Garmin data source) | defer | unguarded landmine until TR-01 exists; guard via routing test, then accept-with-note |
| A3-R002-TR-04 | non-blocking | `GarminAuthChain::terminate()` last-resort path has no intentional coverage | fix-now | covered by TR-02's busy-loop terminate test (deterministic, non-GIL) |
| A3-R002-TR-05 | non-blocking | PyEmbeddedAdapter malformed-result branches (non-dict / missing keys) untested | fix-now | add SCENARIO branches to pystub returning non-dict + dict missing garmin_user_id/display_name; assert Unknown+message |
| A3-R002-TR-06 | informational | pystub fidelity vs real src/Python/garminconnect module not verified this cycle (out of scope) | defer | dedicated fidelity check vs real module + test_adapter_login.py |
| A3-R002-TR-07 | informational | GARMIN_PY_MODULE_DIR deferral genuinely tracked; missing-module fail-safe adequately covered | accept | clean refutation — `missingModuleYieldsUnknownWithoutCrash` (TEST-005) is an adequate proxy; deferral tied to DES-007/NF-Pkg-001 |

## Narrative (condensed)

**Exception-classification ladder (PyEmbeddedAdapter) — well defended.** TEST-005 asserts the NEGATIVE for every risky collapse: `rateLimitKindMapsToUnknownNotAuthFailed` and `valueErrorMapsToUnknownNotAuthFailed` assert `kind != AuthFailed` before the positive `Unknown`; `connectionKindMapsToNetwork` kills a connection→AuthFailed mutant. No hand-simulation needed — assertion shape already kills the mutants (guards LSN-006/A3-R002-M6 regression).

**TR-05 hole in the same file:** `PyEmbeddedAdapter.cpp:212-224` (non-dict login result, dict missing required keys) is never exercised — no pystub SCENARIO returns malformed data. A mutant deleting both checks compiles and passes everything, silently turning "Unknown+message" into a null-QString "Success" handed upstream as valid credentials. Cheap to close (2 scenarios + 2 asserts).

**TR-02 — teardown bound, proven not hypothesized.** `~GarminAuthChain()` = `m_thread.quit(); if(!m_thread.wait(kQuitWaitMs=2000)){ m_thread.terminate(); m_thread.wait(kTerminateWaitMs); }`. Adversary deleted `m_thread.quit();`, recompiled only that TU, relinked against untouched freshly-built objects, ran: **8 passed, 0 failed, 12306ms** (baseline 294ms), with "Qt has caught an exception thrown from an event handler" on every teardown. The elapsed asserts (`<3000ms`, test lines ~229/258) were sized for the SUM of both fallback ceilings, so they can't distinguish "graceful quit ran" from "always fell through to hard-kill." Sits directly on DES-001 invariant 3. Fix: tighten idle assert well under `kQuitWaitMs`; add explicit busy-loop terminate test.

**TR-01 — wizard routing untested, worse than "just untested."** `AddCloudWizard.cpp` compiles only into the app target (src/CMakeLists.txt:333, GC_WANT_GARMINCONNECT); no test `add_executable()` includes it. `testGarminConnectTile.cpp:114-139` hand-re-implements the tile filter loop rather than calling the real code — silent drift risk. Untested: `id()=="Garmin Connect"` branch in `AddService::nextId()`(:280)/`AddConsent::nextId()`(:347); `ensureGarminAuthPage()` idempotency guard (:144 — remove it → leak a GarminAuthChain+PyEmbeddedAdapter+QThread per Back/Next); destructor ordering (:130-133 — reverse → worker thread races freed adapter, the exact DES-001a invariant this slice guarantees). Untestable in current form: `AddGarminAuth` is a private nested class in the .cpp pulling in MainWindow/Athlete/Context headers → needs a GCStubPreamble target (pattern already proven for testGarminConnectTile.cpp).

**TR-03 — `hasAthlete ? 25 : 30` doubly dead.** GarminConnect.cpp never sets `CloudServiceSetting::AthleteID` nor overrides `listAthletes()/selectAthlete()`. Even if a future patch flips the flag without implementing `listAthletes()`, user lands on empty page 25 with no way forward — a silent regression trap no routing test would catch. Unguarded landmine until TR-01 exists.

**TR-04 / terminate() — partially testable.** Genuine GIL-held wedge isn't safely/deterministically reproducible (QThread::terminate() racing CPython C API is documented-unsafe → keep as explicit accepted-risk note). But a non-GIL busy-loop FakePyAdapter forces the same quit-timeout→terminate() path deterministically inside `garmin-fast` — that test doesn't exist; folded into TR-02's fix.

**TR-06 — fixture fidelity out of scope.** Stub shape matches what PyEmbeddedAdapter.cpp consumes line-for-line, but real module not read (scope rule). Verify explicitly vs `src/Python/garminconnect/tests/test_adapter_login.py` in a dedicated check.

**TR-07 — clean refutation.** `GARMIN_PY_MODULE_DIR` deferral genuinely tied to DES-007/REQ-NF-Pkg-001 (comments + CMake:1416-1419); `missingModuleYieldsUnknownWithoutCrash` (TEST-005:164-179) already proves an unresolvable modulePath yields Unknown not crash — adequate proxy.

## Exit disposition
A3 does NOT close. TR-01 + TR-02 are blocking test-hardening fixes that must be built and re-validated before the REQ-002 slice closes. TR-04/TR-05 are cheap fix-now follow-ons (bundle with TR-02/into the pystub). TR-03 defers behind TR-01. TR-06 → dedicated fidelity check. TR-07 accepted.

## Resolution (2026-07-05 — 2 parallel qgdw-builders, working tree, uncommitted)
- **TR-01 → TEST-007** `testGarminConnectWizardRouting.cpp` (4 slots, `garmin-fast`) + `stubs/WizardStubPreamble.h`. Compiles the real `AddCloudWizard.cpp` under a force-included guard-predefinition stub (no production edit). Kills 4 mutants: M1/M2 (nextId routing literal), M3 (`ensureGarminAuthPage` idempotency guard), M4 (`~AddCloudWizard` destructor order — "DES-001a violated: adapter destroyed while worker thread still running"). Residual (non-blocking): tests hit nextId/ensure/dtor directly, not via live QWizard navigation; edit-mode `setStartId(21)` not directly tested.
- **TR-02 → TEST-006** idle/post-request teardown bound tightened to <200ms (« kQuitWaitMs, LSN-009); confirmed to FAIL under the delete-`quit()` mutant that previously survived at 3000ms.
- **TR-04 → TEST-006** new deterministic non-GIL busy-loop test (`BusyPyAdapter`) forces quit-timeout→`terminate()`; asserts bounded clean finish. The "Qt caught an exception" warning adjudicated benign (pthread_cancel forced-unwind).
- **TR-05 → TEST-005** pystub gains non-dict + missing-key SCENARIOs; asserts Unknown-with-message, never spurious Success; kills the delete-both-defenses mutant.
- **TR-03** now guarded by TEST-007 routing coverage → accept-with-note.
- **TR-08 (NEW, from TR-04 build)** — uncancellable native wedge (no cancellation point) → `terminate()` can't take → `~GarminAuthChain` destroys a running `QThread` → `qFatal` abort. Non-blocking (only a pure native loop reaches it); **deferred → Phase 1.5 with A2-001**. Added to wiki/architecture.md Watch.
- Evidence: production `src/Cloud/*` byte-unchanged; garmin-fast 6/6 + garmin-py 1/1 from a from-scratch rebuild (LSN-007 honored). **Next: VAL-008 drift CLV to close the slice.**
