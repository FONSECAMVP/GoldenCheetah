# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project already ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. REQ-002 Authenticate flow **CLOSED** (committed
`60a076848`). **REQ-007 activity-download — chain GREEN adapter→PyEmbeddedAdapter→worker, committed
`1eb5a6a16` + VAL-009 PASS; NOT fully deployed** (readFile staging + FIT→TCX fallback deferred, need REQ-004/006).
**REQ-004 token write-path GREEN + committed `54b7005e6`** (VAL-010 PASS; style/type gate clean via pre-commit).
**REQ-006 Slice A (load-side perm refusal) GREEN — committed `d86323246`.** `GarminTokenStore::loadChecked()` refuses wider-than-0600
`tokens.json` with typed `TokenPermissionsRejected` (T-014, garmin-fast 10/10).
**REQ-006 Slice B (__init__ auth-only reconciliation) GREEN — committed `3edb705cb`.** Library constructed AUTH-ONLY (`_gc.Garmin(email,password)`,
C-API `"ss"`, `PyEmbeddedAdapter(modulePath)`) → no self-written 2nd token file; **REQ-NF-Sec-002 end-to-end MET**. T-015 (pytest) + T-016
(garmin-py). Verification-Gate PASS on independent re-run: pytest 15/15, garmin-py 20/20, garmin-fast 10/10. Findings B-R004-01 + A3-R004-M3 RESOLVED.
A3-R006 hardening DONE (T-014 +3 slots; A3-R006-01/-02 FIXED, `458a72ba7`). VAL-011 CLV re-verify pending
(first two passes FAILed on ledger drift = LSN-008 recur → repaired). REQ-006 fully closed once VAL-011 goes clean.

OPEN: REQ-007 download chain GREEN, **committed `1eb5a6a16`** (3 slices):
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

BLOCKING: 0 open. A3-R004 M1 (AtomicFile order — order-recording TmpWriter seam) + M2 (load_tokens no-op —
per-instance fake) **FIXED + Verification-Gate PASS 2026-07-11** (garmin-fast 9/9, pytest 15/15; mutants killed).
**M3 (REQ-NF-Sec-002 end-to-end) = RESOLVED 2026-07-11 by REQ-006 Slice B** — library now auth-only, no self-written
2nd token file; C++-owned 0600 write is the sole token file. REQ-006 is security-closed.** Non-blocking: A3-R004-04/05/06 (fault-injection defers), -07 (concurrency, mitigated
by DES-001 single-worker), -08 (root-run test), -09 (Windows CI). Prior deferred: TR-08 → Phase 1.5 w/ A2-001;
TR-06 fidelity check; TR-03 guarded by TEST-007. **B-R004-02 CLEARED** — style/type gate ran clean
(clang-format + ruff + mypy --strict) via pre-commit at `54b7005e6`.

CASCADE: DEC-014 (token persistence) fully propagated + committed; no stale dependents:
DES-002 (write+read GREEN — GarminTokenStore save + loadChecked refuse-on-wider-than-owner), DES-006 (AtomicFile GREEN),
DES-012 (dump_tokens/load_tokens/session_expired + `__init__` auth-only construction DONE), DES-013 (PyAuthOutcome.tokenBlob +
1-arg auth-only ctor / stop-forwarding-tokenstorePath DONE, `3edb705cb`), REQ-004 (write path committed `54b7005e6`),
REQ-006 (load-side + auth-only reconciliation CLOSED, security-closed, `d86323246`+`3edb705cb`). Prior: DEC-012/013 seams GREEN.

LAST_CLV: VAL-011 — 2026-07-12 — REQ-006 Slices A+B + A3-R006 hardening (HEAD `458a72ba7`). Code/test spine clean
throughout (pytest 15/15, garmin-py 20/20, garmin-fast 10/10). First two passes FAILed on LEDGER DRIFT only — the LSN-008
signature recurring (5th+6th): banners/appendices current while primary matrix rows, DES index, design.md body, local
state.md, and root STATE.md's own CASCADE/NEXT_GATE/COUNTS lagged committed reality. Repaired across passes; re-verify
(3rd pass) pending. Prior: VAL-010 — 2026-07-11 — PASS — REQ-004 write path. VAL-009 — 2026-07-08 — PASS — REQ-007 chain.
LAST_CYCLE: A3-R006 REQ-006 Slices A+B — 2026-07-12 — FINDINGS (real manual mutants) → FIXED: A3-R006-01 (mask-narrowing
survivor — T-014 only exercised Read-class modes) + A3-R006-02 (perms-cache) closed by TEST-014 +3 slots (0620/0601/re-stat;
mutant-kill demonstrated); A3-R006-03 informational accept (T-016 masked, T-015 the real guard). M-A2/A3/B1/B2/B3 killed;
REQ-005 reconfirmed intact. Prior: A3-R004 REQ-004 write path (2026-07-11) — M1/M2 FIXED; M3 resolved by REQ-006 Slice B.

NEXT_GATE: REQ-006 CLOSED, security-closed (REQ-NF-Sec-002 end-to-end MET); 0 open blocking. Immediate: VAL-011 re-verify
must go clean (ledger drift repaired), then record VAL-011 PASS + bump VAL.next→012. Then the queue: REQ-007 CLOSURE
(GarminConnect::readFile staging garmin-<id>.<ext> + FIT→TCX fallback DES-004 — needs worker-in-CloudService lifecycle +
PRD-Assumption-B library validation) OR REQ-003 (MFA). DEC-014 OQ1 (real-lib dumps/loads + 2-arg `Garmin(email,password)`
signatures) carried as a build NOTE pending the bundled wheel; OQ2 resolved → REQ-NF-Compat-001(b) `session_expired`.

CHANGESET: REQ-006 landed on master in four commits + this repair:
· `d86323246` Slice A — NEW src/Cloud/GarminTokenStore loadChecked (LoadStatus/LoadResult) + NEW testGarminTokenStore_load.cpp (T-014) + CMake target.
· `3edb705cb` Slice B — MODIFIED garmin_client.py (`__init__(email,password)`), PyEmbeddedAdapter.{h,cpp} (1-arg ctor, C-API "ss"),
  AddCloudWizard.cpp (drop tokenstore arg), pystubs/garmin_client.py + WizardStubPreamble.h (narrowed ctors), testGarminConnectPyAdapter.cpp (T-016),
  test_adapter_login.py (T-015) + test_adapter_download.py + test_token_store.py (2-arg call sites).
· `458a72ba7` A3-R006 hardening — testGarminTokenStore_load.cpp +3 slots (0620/0601/re-stat) + the VAL-011 ledger-drift repair (this delta).
· Docs-record follow-ups `808fda03a` (Slice A) + `04ab54d63` (Slice B + LSN-012).
Prior: REQ-004 committed `54b7005e6` (+`b67767380`); REQ-007 `1eb5a6a16`; REQ-002 `60a076848`. production src/Cloud/GarminConnect.*
still untouched (readFile deferred). Unrelated pre-session work (src/Coach/*, src/Gui/*, root CMakeLists.txt, vcpkg.json) still unstaged.

COUNTS: DEC 14/14 accepted (garmin ns) · DES 13(+2 sub-ids); GREEN: DES-001a/002/006/012/013 · REQ 1/22 deployed (REQ-001);
CLOSED: REQ-002 (Authenticate), REQ-006 (token perms, security-closed); GREEN-committed: REQ-004 (write path `54b7005e6`),
REQ-007 (download chain `1eb5a6a16`, not fully deployed — readFile deferred); rest not-started · VAL 10/10 PASS (through VAL-010);
VAL-011 re-verify pending (REQ-006) · TEST T-001..T-016 all GREEN (T-014 GarminTokenStore load-side 8 slots incl. A3-R006
0620/0601/re-stat; T-015 pytest auth-only; T-016 garmin-py no-path-forwarded); testGarminConnectPyAdapter 20/20; Python adapter suite 15/15

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
