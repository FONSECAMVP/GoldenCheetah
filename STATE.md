# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **REQ-003 (MFA) CODE-COMPLETE — both slices built, WORKING TREE, UNCOMMITTED (user has not asked to
commit).** Slice A (seam) + Slice B (UI) done, each Verification-Gate-PASSED and merged. **Slice A:** two-step MFA
seam — `garmin_client.py` `login()` retains pending state + returns `{"mfa_required":True}`, `submit_mfa(code)`
resumes the retained session → identity dict (bad→kind='auth' by TYPE/LSN-006, no-pending→'unknown');
`IGarminPyAdapter::MfaRequired`+`submitMfa()`;
`GarminWorker` `submitMfa` slot + `mfaRequired(QUuid)`; `IGarminAuthClient` `mfaRequired`+`submitMfa`;
`WorkerAuthClient` forward+re-emit; `PyEmbeddedAdapter`. MFA routes via a dedicated `mfaRequired(QUuid)` signal, NOT
a GarminAuthFailure kind (D-R003-01). T-027..031. **Slice B:** `GarminMfaPage` (new, page 22 — 6-digit InputMask,
`hasSixDigits` gate, 3-attempt→`aborted()` non-retry); `GarminCredentialsPage` gained `MfaRequired` state +
`mfaPending()`+`onMfaRequired`; `AddCloudWizard` routes 21→22 on mfaRequired, MFA `aborted()`→`reject()`. T-032..035.
**LSN-018 satisfied** — GarminMfaPage.cpp in both test + main-app source lists; GoldenCheetah links clean (binary
carries 34 GarminMfaPage symbols, orchestrator-verified). **Verification Gate PASS on both slices** — orchestrator
independently re-ran pytest 19/19 + garmin-fast ctest 14/14, diffed FILES vs `git status`, confirmed symbols on
disk, verified the app-link, and audited the tests as faithful criterion encodings (mfaRequired-exactly-once,
no-MFA regression, bad-OTP→Auth, 6-digit gate, 3-strikes-abort, page-22 routing killing 4 mutants).
**Hardening slice (A3-R003 findings) DONE + Gate-PASSED:** T-036 real embedded-Python MFA bridge coverage
(closed BLOCKING A3-R003-01), T-037 WAC MFA wiring, T-038 MFA retention, T-039 initializePage reentry reset
(closed back-nav leak A3-R003-05), T-040 real stale-failure assertion (KILLED mutant M1, closed A3-R003-07),
T-041 terminal-state dup-delivery guard (closed A3-R003-06); T-035 hardened (B-R003-03). Coverage tests passed
first run — no latent bug found. Orchestrator re-verified garmin ctest 15/15 + garmin-py 23/23 + pytest 20/20 +
app-link + M1-kill. **REQ-003 DONE + COMMITTED `8cbc4722d`** (feat(garmin), 2026-07-19, 28 files, path-scoped to
Garmin only; final CLV VAL-015 PASS; pre-commit clang-format/ruff/mypy passed after 2 unused type:ignore removed
+ 2 files reformatted, re-verified per LSN-007). Prior: **REQ-007 DONE + COMMITTED `d312886a6`** (docs-record
`f637c138b`).

NEXT_GATE: **REQ-003 COMPLETE + COMMITTED `8cbc4722d`.** LSN-010 ledger-record stamped (traceability Commit
column + STATE/WIKI banners flipped). Remaining optional: a docs-record commit of the Garmin governance files
(STATE/WIKI/lessons + .claude/workflow-garminconnect/*) mirroring REQ-007's `f637c138b`, if the user wants the
ledger history committed too. Next feature is the user's call — candidate REQs still open per traceability:
REQ-008 (incremental sync + dedup), REQ-010 (bulk backfill), REQ-014 (error translation), REQ-015 (CAPTCHA —
shares the GarminMfaPage/GarminCredentialsPage family, would also close A3-R003-09's onMfaRequired guard). Manual
Phase-3 residuals: REQ-NF-Perf-001/003 stopwatch, OQ1 live smoke (need a real Garmin account). NOTE the working
tree still carries unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`, `vcpkg.json`,
`.claude/skills/**`, `.claude/agents/*`) — NOT Garmin; a future Garmin commit must stay path-scoped. The Garmin commit
must include ONLY: `src/Cloud/{IGarminPyAdapter.h, GarminWorker.*, IGarminAuthClient.h, WorkerAuthClient.*,
PyEmbeddedAdapter.*, GarminMfaPage.*, GarminCredentialsPage.*, AddCloudWizard.cpp}` + `src/Python/garminconnect/
garmin_client.py` + `src/Python/garminconnect/tests/test_adapter_mfa.py` + `unittests/Core/garminconnect/**`
(new MfaPage test + seam-conformance edits + pystub + the two CMake hunks) + the `src/CMakeLists.txt`
GC_WANT_GARMINCONNECT GarminMfaPage.cpp hunk (hunk-split from the unrelated Coach/root edits). NOTE the working
tree still carries unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`, `vcpkg.json`,
`.claude/skills/**`, `.claude/agents/*`) — NOT Garmin; the Garmin commit must stay path-scoped, never `git add -A`.

BLOCKING: **0 open.** A3-R003-01 FIXED (T-036 real-bridge coverage, BLOCKING CLEARED). B-R003-01/02/03 FIXED
(T-037/038/035). A3-R003-05 FIXED (T-039), A3-R003-06 FIXED (T-041), A3-R003-07 FIXED (T-040, mutant M1 killed).
accept-with-note residuals: D-R003-01 (design↔code MFA-routing note), A3-R003-08 (OQ1 sentinel guess, bounded),
A3-R003-09 (onMfaRequired terminal guard — unreachable, deferred to next page-touch). OQ1: real-garth needs-MFA
sentinel + `resume_login()` unconfirmed vs unbundled wheel (pinned by fake). Carried from REQ-007: A3-R007-02/-04
accepted residuals. Carried non-blocking: A3-R004-04/05/06 (fault-injection), -07 (concurrency, mitigated by
DES-001 single-worker), -08 (root-run test), -09 (Windows CI); TR-08 → Phase 1.5 with A2-001; TR-06 fidelity
check; TR-03 guarded by TEST-007. Detail → findings.md.

CASCADE: DEC-015 (status SSOT) fully propagated — traceability.md/decisions.md (canonical status),
design.md/STATE.md/WIKI.md/wiki/* (status stripped), local state.md (deleted), ledger_drift_lint.py +
install_hook.py + .pre-commit-config.yaml (mechanism), lessons.md (LSN-008→MECHANISM, LSN-014/015). No
stale dependents (VAL-012 confirmed). Prior: DEC-014 fully propagated + committed.

CHANGESET (recent commits — provenance): REQ-006 `d86323246` (Slice A) + `3edb705cb` (Slice B) +
`458a72ba7` (A3-R006 hardening) + docs-record `808fda03a`/`04ab54d63`; REQ-004 `54b7005e6`
(+`b67767380`); REQ-007 `1eb5a6a16`; REQ-002 `60a076848`. **DEC-015 governance delta committed to
master:** `88d4ea402` (drift lint + TEST-017 + install/pre-commit wiring + `__pycache__/` gitignore) +
`2520ed034` (ledger normalization + local state.md deletion + WIKI/STATE/conventions). Production
`src/Cloud/GarminConnect.*` untouched (readFile is a later slice). Still unstaged (out of DEC-015 scope):
pre-session work (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`, `vcpkg.json`) + broader
skill-methodology edits (`.claude/skills/**`, `.claude/agents/*`, `anti_duplication_guard.py`).
Housekeeping: `scripts/__pycache__/` now gitignored (C1). **REQ-007 closure build is WORKING-TREE,
UNCOMMITTED** (user has not asked to commit): new `src/Cloud/{IGarminDownloadClient.h,GarminDownloadClient.*,
GarminDownloadChain.*}` + modified GarminConnect/GarminWorker/IGarminPyAdapter/PyEmbeddedAdapter/
garmin_client.py + new test cpps + test_adapter_restore.py + CMake wiring. **2026-07-18 additions to the
same working tree:** GarminConnect.cpp/.h (queued completion `postReadComplete` + owned `m_completionContext`
member — B-R007-01 + A3-R007-01), testGarminConnectReadFile.cpp (TEST-024/025/026), and
`src/CMakeLists.txt` (GC_WANT_GARMINCONNECT += GarminTokenStore.cpp + AtomicFile.cpp — B-R007-03).
ctest garmin 14/14 + readFile exe 13/13 re-verified. **COMMITTED `d312886a6`** (feat(garmin), 2026-07-18, 25
files): all Garmin production + tests + the src/CMakeLists.txt GC_WANT_GARMINCONNECT hunk ONLY — staged path-scoped
(the mixed src/CMakeLists.txt was hunk-split so Coach/calendar changes stayed unstaged). Pre-commit hooks ran:
clang-format reformatted GarminConnect.cpp/GarminDownloadChain.cpp (brace/wrap only, no #include reorder) + ruff
unquoted one annotation in garmin_client.py; re-verified post-format (garmin ctest 14/14, GoldenCheetah re-links
clean) per LSN-007 before the commit. Unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`,
`vcpkg.json`, `.claude/skills/**`, `.claude/agents/*`) remain UNCOMMITTED — not Garmin, left for their owners.
**REQ-003 (MFA) Slice A — WORKING TREE, UNCOMMITTED (2026-07-18):** modified `src/Cloud/{IGarminPyAdapter.h,
GarminWorker.h,GarminWorker.cpp,IGarminAuthClient.h,WorkerAuthClient.h,WorkerAuthClient.cpp,PyEmbeddedAdapter.h,
PyEmbeddedAdapter.cpp}` + `src/Python/garminconnect/garmin_client.py`; new `src/Python/garminconnect/tests/
test_adapter_mfa.py`; + seam-conformance overrides (the new pure-virtual forces every implementer to build) in
`unittests/Core/garminconnect/{testGarminConnectAuthClient.cpp (holds T-029..031 + FakePyAdapter MFA scripting),
testGarminConnectDownloadWorker.cpp,testGarminConnectRestoreWorker.cpp,testGarminConnectAuthChain.cpp,
testGarminConnectCredentialsPage.cpp,stubs/ReadFileStubPreamble.h,stubs/WizardStubPreamble.h}`. No CMake edits
(T-029..031 landed in the already-wired testGarminConnectAuthClient target). pytest 19/19 + garmin ctest 14/14
re-verified by the orchestrator. A future Garmin commit must stay path-scoped to these paths only.
**REQ-003 (MFA) Slice B — WORKING TREE, UNCOMMITTED (2026-07-18):** NEW `src/Cloud/GarminMfaPage.{h,cpp}` +
`unittests/Core/garminconnect/testGarminConnectMfaPage.cpp`; MODIFIED `src/Cloud/GarminCredentialsPage.{h,cpp}`
(MfaRequired state + mfaPending()+onMfaRequired), `src/Cloud/AddCloudWizard.cpp` (page 22 routing + aborted→reject),
`src/CMakeLists.txt` (GarminMfaPage.cpp added to GC_WANT_GARMINCONNECT — LSN-018), `unittests/Core/garminconnect/
CMakeLists.txt` (testGarminConnectMfaPage target + GarminMfaPage.cpp on the routing target),
`unittests/Core/garminconnect/testGarminConnectWizardRouting.cpp` (T-035). garmin-fast 14/14 + GoldenCheetah
link re-verified by the orchestrator. Combined REQ-003 (Slice A+B) commit, when the user asks, stays path-scoped
to the Garmin src/Cloud + src/Python + unittests/Core/garminconnect + the two hunk-split CMake lists ONLY.
**REQ-003 (MFA) hardening slice — WORKING TREE, UNCOMMITTED (2026-07-19):** MODIFIED source `src/Cloud/
GarminMfaPage.{h,cpp}` (initializePage reset + terminal guards + test-only attemptCount()), `src/Cloud/
GarminCredentialsPage.{h,cpp}` (initializePage reset + terminal guards); MODIFIED tests/fixtures
`unittests/Core/garminconnect/{pystubs/garmin_client.py (T-036 MFA scenario), testGarminConnectPyAdapter.cpp
(T-036), testGarminConnectAuthClient.cpp (T-037), testGarminConnectMfaPage.cpp (T-040 tautology fix + T-041/T-039),
testGarminConnectCredentialsPage.cpp (T-041/T-039), testGarminConnectWizardRouting.cpp (T-035 harden)}` +
`src/Python/garminconnect/tests/test_adapter_mfa.py` (T-038). No CMake edits (no new production .cpp; MfaPage
already wired by Slice B). garmin ctest 15/15 + garmin-py 23/23 + pytest 20/20 + GoldenCheetah link re-verified.
**ALL THREE SLICES (A+B+hardening) COMMITTED TOGETHER as `8cbc4722d`** (feat(garmin), 2026-07-19, 28 files,
path-scoped): staged the Garmin src/Cloud + src/Python/garminconnect + unittests/Core/garminconnect set + the
hunk-split `src/CMakeLists.txt` GarminMfaPage.cpp hunk ONLY (the Coach/calendar hunks of src/CMakeLists.txt
stayed unstaged via `git apply --cached` of an extracted patch). Pre-commit: clang-format reformatted
GarminMfaPage.cpp + testGarminConnectMfaPage.cpp (brace/wrap only) + 2 unused `# type: ignore` removed from
test_adapter_mfa.py (mypy --strict); re-verified garmin-fast 14/14 + pytest 20/20 post-fix per LSN-007, then
committed clean (all hooks passed). Unrelated pre-session edits (`src/Coach/*`, `src/Gui/*`, root `CMakeLists.txt`,
`vcpkg.json`, `.claude/skills/**`, `.claude/agents/*`) remain UNCOMMITTED — verified none entered `8cbc4722d`.

LAST_CLV: VAL-015 — 2026-07-19 — FINAL REQ-003 commit-gate CLV over the complete feature (T-027..041 + hardening).
Content verdict PASS (9/9): findings clean/0-blocking, no false-done (every `fixed` finding's code confirmed on
disk), spine T-027..041 consistent, all 5 criterion clauses covered + strengthened (real-bridge/retention/reentry/
dup-guard), design.md honest, registries consistent. The git-less validator raised ONE FAIL it could not
corroborate — "is REQ-003 really uncommitted?" — because it reasoned from the FROZEN session-start git snapshot
(predates this session's files) with no git tools. Orchestrator resolved with live `git status --porcelain` +
`git log`: LEDGER CONFIRMED — all REQ-003 files are M/?? (GarminMfaPage.* + test_adapter_mfa.py +
testGarminConnectMfaPage.cpp untracked; seam/page/wizard files modified), HEAD is still `f637c138b` (no REQ-003
commit). Net: **PASS, clean to commit.** The false-FAIL → LSN-023 (brief git-less agents with live git state).
Prior: VAL-014 — 2026-07-18 (REQ-003 Slices A+B, PASS-WITH-WARN). Superseded verdict line below retained for
the cycle detail:
LAST_CLV_PRIOR: VAL-014 — 2026-07-18 — incremental over the whole REQ-003 (MFA) changeset (T-027..035). Verdict
PASS-WITH-WARN: spine (REQ→DEC→DES→TEST) consistent, all 5 criterion clauses have covering assertions, design
honesty confirmed (D-R003-01 enum note matches on-disk `{Auth,Network,Unknown}`), findings register 0 open-blocking,
registries consistent, LSN-018 build-graph truthful (GarminMfaPage.cpp in src/CMakeLists.txt). The 3 recorded gaps
(B-R003-01/02/03) confirmed as the ONLY gaps — no new coverage hole, 0 FAIL. Two hygiene WARNs raised + REMEDIATED
this pass by the orchestrator: design.md:219 "future GarminMfaPage" prose (now realized-note) + STATE "GREEN"
status-word for submit_mfa (reworded; verdict lives in traceability). Prior CLV runs and their verdicts →
traceability.md ## Validations run (the canonical VAL home per DEC-015); this cursor names only the latest.
LAST_CYCLE: A3-R003 (REQ-003 MFA, both slices) — 2026-07-18 — VERDICT: FINDINGS. Feature sound on the happy
paths (3-strikes boundary correct-by-trace, no-MFA regression intact, no OTP/password logging, wizard
idempotency structural). 8 findings + 1 surviving mutant. **1 BLOCKING: A3-R003-01** — the REAL embedded-Python
MFA bridge (PyEmbeddedAdapter dict-sentinel + submitMfa refcounting) has ZERO `garmin-py` coverage while every
sibling op has a pystub scenario. Confirmed B-R003-01/02/03 (→ A3-R003-02/03/04). NEW non-blocking: A3-R003-05
(Back-nav state leak — edited creds silently discarded), A3-R003-06 (dup-delivery terminal-state undefended).
Informational: A3-R003-07 (TEST-034 tautology `||true` → why mutant **M1** [drop stale-id guard in
GarminMfaPage::onAuthFailed] survives), A3-R003-08 (OQ1 sentinel guess, bounded blast radius, accepted).
Verification Gate on the report: PASS (orchestrator spot-checked F1 pystub/PyAdapter emptiness + the line-221
tautology — both accurate). Lessons captured: LSN-020 (real-bridge-behind-fake), LSN-021 (wizard-page reentry
reset), LSN-022 (tautological assertion). Prior: A3-R007 (REQ-007) — 2026-07-18. Cycle narratives → cycles/.

Detail lives in: traceability.md (per-id status spine) · findings.md (finding disposition) ·
decisions.md (DEC entries) · validations/ + cycles/ (evidence). workflow-aicoach/ is a retired
ledger (provenance only, see .claude/workflow-INDEX.md).
