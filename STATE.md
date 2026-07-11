# STATE — project cursor            (condensed from active ledger; read WIKI.md first)

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project already ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. REQ-002 Authenticate flow **CLOSED** (committed
`60a076848`). **REQ-007 activity-download — chain GREEN adapter→PyEmbeddedAdapter→worker, committed
`1eb5a6a16` + VAL-009 PASS; NOT fully deployed** (readFile staging + FIT→TCX fallback deferred, need REQ-004/006).
**REQ-004 token write-path GREEN + committed `54b7005e6`** (VAL-010 PASS; style/type gate clean via pre-commit).

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
**M3 (REQ-NF-Sec-002 end-to-end UNMET — library self-writes a 2nd unaudited token file) = DEFERRED → bundled with
REQ-006 (user disp); REQ-004 write-path GREEN but NOT security-closed until that slice rewrites __init__ +
test_adapter_login.py:103.** Non-blocking: A3-R004-04/05/06 (fault-injection defers), -07 (concurrency, mitigated
by DES-001 single-worker), -08 (root-run test), -09 (Windows CI). Prior deferred: TR-08 → Phase 1.5 w/ A2-001;
TR-06 fidelity check; TR-03 guarded by TEST-007. **B-R004-02 CLEARED** — style/type gate ran clean
(clang-format + ruff + mypy --strict) via pre-commit at `54b7005e6`.

CASCADE: DEC-014 REQ-004 write path GREEN, committed `54b7005e6`; dependents merged, none stale:
DES-002 (no amendment — confirmed literal; write path GREEN via GarminTokenStore), DES-006 (AtomicFile GREEN as designed),
DES-012 (+dump_tokens/load_tokens + 3 pytest + pystub methods DONE), DES-013 (+PyAuthOutcome.tokenBlob DONE;
stop-forwarding-tokenstorePath DEFERRED → B-R004-01 __init__ reconciliation slice), REQ-006 (load-side,
next builder). Prior: DEC-012 (IGarminAuthClient) Auth-only; DEC-013 (IGarminPyAdapter) carries authenticate()+
downloadActivity() — GREEN across all 4 FakePyAdapter stubs + PyEmbeddedAdapter + pystub (VAL-009 Check 9).

LAST_CLV: VAL-010 — 2026-07-11 — PASS — REQ-004 write-path changeset (uncommitted). First pass FAILed
Check 6+9: the DEC-014 cascade notes in design.md DES-012/013 asserted "stop forwarding tokenstorePath"
as DONE (present tense) though the build DEFERRED it (B-R004-01; code still forwards) — a false-done
contradicting code + findings; plus stale DES-012/013 traceability index rows. Fixed (notes → DEFERRED
tense; index rows synced); LSN-011 captured (guard), LSN-008 escalated recur:4/miss:3; re-verified clean.
Code/test spine clean on first pass (garmin-fast 9/9, garmin-py 1/1, pytest 15/15). Prior: VAL-009 —
2026-07-08 — PASS — REQ-007 download-chain (`1eb5a6a16`).
LAST_CYCLE: A3-R004 REQ-004 write path — 2026-07-11 — FINDINGS (real mutation harness) then RESOLVED:
M1 (AtomicFile perms-order — order-recording TmpWriter seam) + M2 (load_tokens no-op — per-instance fake)
FIXED + Verification-Gate PASS (mutants killed; garmin-fast 9/9, pytest 15/15); M3 (REQ-NF-Sec-002 end-to-end
unmet) DEFERRED → REQ-006 per user disp; 6 non-blocking dispositioned. Prior: A3 REQ-002 tile-routing (2026-07-05) FIXED.

NEXT_GATE: none blocking. REQ-007 download chain committed (`1eb5a6a16`) + VAL-009 PASS. REQ-007
CLOSURE is deferred: readFile staging + FIT→TCX fallback (DES-004) need REQ-004/006 (token storage +
worker-in-CloudService lifecycle) and PRD-Assumption-B (library FIT-availability signal) validated.
**DEC-014 ACCEPTED (B)** 2026-07-11 — the token-persistence seam is resolved: adapter exports the token blob
via `dumps()`/`loads()`, **C++ owns the single atomic 0600 write** via AtomicFile (DES-002 unchanged, literal).
**REQ-004 write path GREEN (uncommitted)** + Verification-Gate PASS — orchestrator re-ran garmin-fast 9/9,
garmin-py 1/1, pytest 15/15; FILES match git; T-012 encodes 0700/0600/atomic/two-athlete, T-013 encodes REQ-005.
AtomicFile (T-011) + GarminTokenStore (T-012) + adapter dump_tokens/load_tokens + PyAuthOutcome.tokenBlob (T-013).
**VAL-010 (incremental CLV) + A3-R004 DISPATCHED** (parallel, read-only). NOT-done → __init__ tokenstore_path
forwarding (B-R004-01, __init__ reconciliation slice). B-R004-02 cleared at commit.
**REQ-006 (load-side refuse-on-bad-perms + resume) is the next gate** — also closes deferred M3 (NF-Sec-002). Alt deferred:
REQ-003 (MFA). DEC-014 OQ1 (real-lib string-API method name) carried as build NOTE; OQ2 resolved →
REQ-NF-Compat-001(b) `session_expired` kind.

CHANGESET: REQ-002 landed on master (212a4c258 + e9e017fe1 + 60a076848); ledger hash-backfill
committed `caa5c3e5b`. **REQ-007 download chain (slices 1+2+3) committed `1eb5a6a16`; VAL-009 ledger-record
follow-up commit records the hash into the ledgers.** Files:
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
**REQ-004 write path (uncommitted, 2026-07-11):** NEW src/Cloud/AtomicFile.{h,cpp}, src/Cloud/GarminTokenStore.{h,cpp},
unittests/Core/garminconnect/testAtomicFile.cpp (T-011), testGarminTokenStore.cpp (T-012),
src/Python/garminconnect/tests/test_token_store.py (T-013). MODIFIED src/Cloud/IGarminPyAdapter.h (PyAuthOutcome.tokenBlob),
src/Cloud/PyEmbeddedAdapter.cpp (surface blob post-login), src/Python/garminconnect/garmin_client.py (+dump_tokens/load_tokens,
OQ1 NOTE), unittests/Core/garminconnect/CMakeLists.txt (2 new garmin-fast targets), pystubs/garmin_client.py. Deps: Qt-only
for the C++ helpers. Re-verified garmin-fast 9/9, garmin-py 1/1, pytest 15/15. Style/type gate ran clean via pre-commit (B-R004-02 CLEARED).
Plus this byproduct ledger delta. **Committed `54b7005e6`** 2026-07-11.

COUNTS: DEC 14/14 accepted (garmin ns) · DES 13(+2 sub-ids) drafted/GREEN (DES-006 GREEN; DES-002 write-path GREEN) · REQ
1/22 deployed, REQ-002 Authenticate CLOSED, REQ-007 download chain GREEN + committed `1eb5a6a16` (not fully
deployed — readFile deferred), REQ-004 write path GREEN (uncommitted), rest not-started · VAL 9/9 PASS (VAL-009); VAL-010
running (REQ-004 incremental CLV) · TEST T-001..T-013 all GREEN (named slots: T-005→10, T-006→7, T-007→4, T-008→6 pytest,
T-009→8 garmin-py, T-010→6 garmin-fast, T-011 AtomicFile garmin-fast, T-012 GarminTokenStore garmin-fast, T-013→3 pytest);
testGarminConnectPyAdapter 20/20; Python adapter suite 15/15

Full detail lives in the active ledger: .claude/workflow-garminconnect/state.md
(workflow-aicoach/ is CLOSED — provenance only, see .claude/workflow-INDEX.md)
