# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

PHASE: Phase 2.2 — Garmin Connect integration, active feature REQ-002 (slice: end-to-end
Authenticate — COMPLETE)

OPEN: PyEmbeddedAdapter (production embedded-Python adapter implementing IGarminPyAdapter)
+ AddCloudWizard tile-routing — next slice; needed for REQ-006 token persistence and to
let the wizard's Garmin tile connect a real account.

BLOCKING: 0 open blocking findings (findings.md: all dispositioned — fix-now/defer/accept)

CASCADE: DEC-012 (IGarminAuthClient) and DEC-013 (IGarminPyAdapter) are Auth-only surfaces,
GREEN; the same WorkerAuthClient/GarminWorker chain is reused unchanged when the production
PyEmbeddedAdapter lands — no test-shape rework required.

LAST_CLV: VAL-006 — 2026-05-24 — PASS (9/9) — REQ-002 end-to-end Authenticate slice

NEXT_GATE: VAL-007 — PyEmbeddedAdapter slice (production embedded-Python adapter
implementing IGarminPyAdapter)

CHANGESET: worktree garmin-req002-e2e merged to master; GarminWorker + WorkerAuthClient +
IGarminPyAdapter shipped; A3 sweep = 19 mutants KILLED + 1 accept-with-rationale (A3-R002-M10)

COUNTS: DEC 13/13 accepted (garmin ns) · DES 12(+2 sub-ids) drafted/partial-GREEN · REQ
1/22 deployed, 1 VAL-PASS-partial (REQ-002), rest not-started · VAL 6/6 PASS · TEST T-001..T-004
all GREEN

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
