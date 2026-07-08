# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

PHASE: Phase 2.2 — Garmin Connect integration. REQ-002 Authenticate flow **CLOSED** (committed
`60a076848`). **REQ-007 activity-download — chain GREEN adapter→PyEmbeddedAdapter→worker, uncommitted;
NOT fully deployed** (readFile staging + FIT→TCX fallback deferred, need REQ-004/006).

OPEN: REQ-007 download chain GREEN in the working tree, **uncommitted** (3 slices):
· Slice 1 (Python, DES-012): garmin_client.download_activity — fmt ORIGINAL/TCX map, bytes verbatim,
  connection/rate_limit by exception TYPE (LSN-006), unsupported-fmt rejected. TEST-008 = 6 pytest.
· Slice 2 (C++ seam, DEC-013/DES-001a/DES-013): IGarminPyAdapter pure-virtual downloadActivity;
  PyEmbeddedAdapter marshals Python bytes→QByteArray binary-exact, reuses the RETAINED authenticated
  client (REQ-005 session), classifies connection→Network / rate_limit→RateLimited / foreign+non-bytes→
  Unknown via shared takeRaisedException(). TEST-009 = 8 garmin-py; testGarminConnectPyAdapter 20/20.
· Slice 3 (worker, DES-001): GarminWorker `downloadActivity(activityId, fmt, requestId)` slot +
  `downloaded(id,bytes)` / `downloadFailed(id, GarminDownloadFailure)` signals (new metatype-registered
  failure type), mapping PyDownloadOutcome→signals off the GUI thread. TEST-010 = 6 garmin-fast slots
  (new testGarminConnectDownloadWorker target).
· DEFERRED (REQ-007 not fully deployed): GarminConnect::readFile staging garmin-<id>.<ext> + FIT→TCX
  fallback (DES-004) — needs REQ-004/006 tokens/worker-in-CloudService lifecycle; fallback trigger
  depends on unvalidated library behaviour (PRD Assumption B).

BLOCKING: 0. Deferred, non-blocking: TR-08 (uncancellable-native-wedge → ~GarminAuthChain qFatal
abort) → Phase 1.5 with A2-001; TR-06 (pystub-vs-real-module fidelity) → dedicated check; TR-03
now guarded by TEST-007 → accept.

CASCADE: DEC-012 (IGarminAuthClient) and DEC-013 (IGarminPyAdapter) are Auth-only surfaces,
GREEN; production PyEmbeddedAdapter + GarminAuthChain now wired into AddCloudWizard behind
GC_WANT_GARMINCONNECT — no further test-shape rework needed for this surface.

LAST_CLV: VAL-008 — 2026-07-05 — PASS (9/9) — REQ-002 A3 test-hardening changeset (Check-6
ledger drift + slot-count conflation fixed, LSN-008 escalated to guard, re-verified clean)
LAST_CYCLE: A3 REQ-002 tile-routing — 2026-07-05 — FINDINGS then FIXED: TR-01/-02/-04/-05
resolved (TEST-007 + strengthened 005/006), TR-08 new→defer; LSN-009 captured. Slice CLOSED.

NEXT_GATE: none blocking. Commit the REQ-007 download chain when the user asks (cite
REQ-007/TEST-008/009/010/DEC-006/DEC-013). An incremental CLV over the REQ-007 changeset is the right
gate before moving on. REQ-007 CLOSURE is deferred: readFile staging + FIT→TCX fallback (DES-004) need
REQ-004/006 (token storage + worker-in-CloudService lifecycle) and PRD-Assumption-B (library FIT-
availability signal) validated. Candidate next work: REQ-004/006 (tokens, unlocks REQ-007 closure) or
REQ-003 (MFA).

CHANGESET: REQ-002 landed on master (212a4c258 + e9e017fe1 + 60a076848); ledger hash-backfill
committed `caa5c3e5b`. **Uncommitted working tree — REQ-007 download chain (slices 1+2+3):**
· Slice 1 — MODIFIED src/Python/garminconnect/garmin_client.py + NEW tests/test_adapter_download.py (T-008).
· Slice 2 — MODIFIED src/Cloud/IGarminPyAdapter.h (PyDownloadOutcome + pure-virtual downloadActivity),
  src/Cloud/PyEmbeddedAdapter.{h,cpp} (retained-client session + marshalling + shared classifier),
  testGarminConnectPyAdapter.cpp (T-009), pystubs/garmin_client.py, 4 FakePyAdapter stubs.
· Slice 3 — MODIFIED src/Cloud/GarminWorker.{h,cpp} (GarminDownloadFailure + downloadActivity slot +
  downloaded/downloadFailed signals + metatype reg) + NEW testGarminConnectDownloadWorker.cpp (T-010) +
  its CMakeLists target.
· plus this byproduct ledger delta (WIKI/STATE + workflow-garminconnect state.md/traceability.md/design.md
  + wiki/architecture.md). garmin ctest 8/8 (garmin-fast 7, garmin-py 1; PyAdapter 20/20); pytest 12/12;
  clang-format + ruff + mypy clean. production src/Cloud/GarminConnect.* still untouched (readFile deferred).
  Unrelated pre-session work (src/Coach/*, src/Gui/*, root CMakeLists.txt, vcpkg.json) still unstaged.

COUNTS: DEC 13/13 accepted (garmin ns) · DES 13(+2 sub-ids) drafted/GREEN · REQ
1/22 deployed, REQ-002 Authenticate CLOSED, REQ-007 download chain GREEN (uncommitted, not fully
deployed — readFile deferred), rest not-started · VAL 8/8 PASS (VAL-008 9/9) · TEST T-001..T-010 all
GREEN (named slots: T-005→10, T-006→7, T-007→4, T-008→6 pytest, T-009→8 garmin-py, T-010→6 garmin-fast);
testGarminConnectPyAdapter 20/20; Python adapter suite 12/12

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
