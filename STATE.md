# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

PHASE: Phase 2.2 — Garmin Connect integration. REQ-002 PyEmbeddedAdapter + AddCloudWizard
tile-routing slice **CLOSED** (A3 clean, VAL-008 PASS 9/9). Authenticate flow slices all done.

OPEN: nothing blocking. A3 test-hardening changeset committed as `60a076848` (pre-commit
clang-format reformatted testGarminConnectAuthChain.cpp — LSN-007 honored: rebuilt from scratch
+ retested 9/9 GREEN before the commit landed). Ready to start the next REQ.

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

NEXT_GATE: none blocking. Commit the working-tree A3 changeset when the user asks, then start
the next REQ (candidates: REQ-007 activity download, REQ-003 MFA OTP) via the TDD loop.

CHANGESET: commits 212a4c258 (PyEmbeddedAdapter, DES-013, TEST-005) + e9e017fe1
(GarminAuthChain + AddCloudWizard routing + app-build wiring, TEST-006) + 60a076848 (A3 hardening:
TEST-007 wizard-routing + WizardStubPreamble.h; strengthened TEST-005/006 + pystub + unittests
CMakeLists; workflow ledgers) on master. garmin-fast 6/6 + garmin-py 1/1 from-scratch GREEN;
production src/Cloud/* untouched. Unrelated pre-session work (src/Coach/*, src/Gui/*, root
CMakeLists.txt, vcpkg.json, etc.) deliberately left unstaged. Only a tiny doc-only delta recording
this hash into the ledgers remains uncommitted.

COUNTS: DEC 13/13 accepted (garmin ns) · DES 13(+2 sub-ids) drafted/GREEN · REQ
1/22 deployed, REQ-002 Authenticate slices CLOSED (A3 clean, VAL-008 9/9), rest not-started ·
VAL 8/8 PASS (VAL-008 9/9) · TEST T-001..T-007 all GREEN (named slots: T-005→10, T-006→7, T-007→4)

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
