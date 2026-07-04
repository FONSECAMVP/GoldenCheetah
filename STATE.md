# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

PHASE: Phase 2.2 — Garmin Connect integration, active feature REQ-002 (slice: PyEmbeddedAdapter
+ AddCloudWizard tile-routing — CODE-COMPLETE, CLV pending)

OPEN: run VAL-007 CLV pass over the changeset below, then A3 against the gaps the builders
flagged (no automated test of the wizard routing itself; GarminAuthChain terminate()
last-resort path unverified against a truly wedged worker; hasAthlete→25 branch is dead
code; GARMIN_PY_MODULE_DIR dev-path deferred to DES-007/NF-Pkg-001).

BLOCKING: 0 open blocking findings (findings.md: all dispositioned — fix-now/defer/accept)

CASCADE: DEC-012 (IGarminAuthClient) and DEC-013 (IGarminPyAdapter) are Auth-only surfaces,
GREEN; production PyEmbeddedAdapter + GarminAuthChain now wired into AddCloudWizard behind
GC_WANT_GARMINCONNECT — no further test-shape rework needed for this surface.

LAST_CLV: VAL-006 — 2026-05-24 — PASS (9/9) — REQ-002 end-to-end Authenticate slice

NEXT_GATE: VAL-007 CLV pass over the PyEmbeddedAdapter + tile-routing slice (code-complete
2026-07-05; not yet validated)

CHANGESET: commits 212a4c258 (PyEmbeddedAdapter, DES-013, TEST-005) + pending commit
(GarminAuthChain + AddCloudWizard routing + app-build wiring, TEST-006) on master;
garmin-fast 5/5 + garmin-py 1/1 GREEN; 6 flag-ON app objects independently compiled clean;
LSN-007 captured (pre-commit hook mutation must be rebuilt+retested before trusting a commit)

COUNTS: DEC 13/13 accepted (garmin ns) · DES 13(+2 sub-ids) drafted/GREEN · REQ
1/22 deployed, 1 code-complete-CLV-pending (REQ-002), rest not-started · VAL 6/6 PASS (VAL-007
not yet run) · TEST T-001..T-006 all GREEN

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
