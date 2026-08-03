# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **REQ-012 DONE + COMMITTED `f001c7d20`** (feat(garmin), 2026-08-03, 7 files, +845/-12, path-scoped Garmin
only) — the disconnect contract plus the DEC-020 fail-closed hardening that closed the blocking A3-R012-F1. garmin
18/18, GoldenCheetah links clean. Pre-commit clang-format reformatted 2 files on the first attempt (wrapping +
preprocessor indent only, NO include reorder) — the commit aborted, files were re-verified per LSN-007 (rebuild +
18/18 + app-link), re-staged, and the retry passed every hook clean. **Only remaining step: the docs-record commit**
of the governance ledgers, mirroring `b47954308`. NOTE for the record: while verifying the gate the orchestrator
mutated `accountStillConnected()` to `return true` (4 slots died — the gate IS load-bearing) and then reverted with
`git checkout --` on a DIRTY file, destroying the builder's uncommitted production change; recovered byte-identically
from the builder's own scratchpad copy (diff-proven), captured as **LSN-032**. Prior state (superseded): TEST-ONLY slice, working tree, uncommitted (2026-08-02).** The disconnect MECHANISM already shipped inside REQ-008's DEC-019 trigger slice
(`ff9cce966`): `GarminConnect::disconnectService()` → `GarminTokenStore::clearAccount()` deletes tokens.json +
active-account.json and leaves the per-account sidecars alone; `CredentialsPage::deleteClicked()` drives it
generically. What was MISSING was criterion coverage — only 2 of the 5 prd.md:84 clauses had tests. TEST-054/055/056
close three more (full-SSO-on-reconnect with restoreCalls==0; prior-account sidecars byte-identical + NOT consulted
after an A→B account switch; same-account reconnect resumes imported history + backfill cursor), all in the existing
`testGarminConnectConnectPersist` target — NO new production code, NO new test target, NO CMake edit. All three
passed against unchanged production, so the builder proved failability with 4 targeted, reverted mutations (M1
clearAccount no-op, M2 dir wipe, M3 uid-blind importedFilePath, M4 cross-account loadImported fallback — M4 kills
ONLY T-055's "not consulted", so no test is a blanket duplicate). T-055/056 construct GarminConnect with NO ctor
uid-override, so identity flows the REAL producer path (LSN-024 discipline held). **Verification Gate PASS** —
orchestrator independently re-ran ctest 18/18, confirmed `src/Cloud/` clean (mutations genuinely reverted), diffed
FILES vs `git status` (exactly one file: unittests/Core/garminconnect/testGarminConnectConnectPersist.cpp), and
grep-cleared LSN-022 tautologies. **New finding B-R012-01 (non-blocking, criterion↔code):** the criterion's "deletes
the token file BEFORE clearing in-memory state" ordering clause is UNENCODABLE — disconnectService() clears no
in-memory state at all, and deleteClicked() mints a fresh instance that never opened a session; the real garth
session lives in a different instance and is never torn down. Builder reported it instead of faking a vacuous test
(correct call). B-R012-02 informational (test-fixture hygiene: makeZip/makeFitBytes duplicated from
testGarminConnectSync.cpp; FakeSyncClient's new originalBytesById is a forget-to-script footgun; "untouched" is
byte-identity not mtime). Note: clang-format could not run — no binary in this environment (LSN-012 residual;
pre-commit will run it, so re-verify per LSN-007 before accepting the commit).
Prior focus (superseded): **REQ-008 DONE + COMMITTED `ff9cce966`** (feat(garmin), 2026-07-20, 40 files, path-scoped Garmin only) —
incremental sync + dedup, LIVE end-to-end (A+B+C + Slice-D producer + DEC-019 trigger). garmin 18/18, pytest 25/25,
app links. Pre-commit reformatted 9 files → re-verified per LSN-007 before the clean commit. A3-R008 fully
dispositioned (F1 accepted+ticketed REQ-016; F3/F4 fixed; F2/F5 tracked). **Docs-record commit DONE `b47954308`**
(docs(garmin): record REQ-008 closure — DEC-017/018/019, A3-R008 findings + lessons), mirroring REQ-007's
f637c138b. **REQ-008 is fully closed: feature + ledger both on master; working tree carries NO Garmin files.**
Unrelated pre-session Coach/Gui/CMake/vcpkg/skill edits remain uncommitted
(NOT Garmin, left for their owners; verified absent from ff9cce966). The DEC-019 trigger slice closed the last blocker:
`CloudService::persistConnectSuccess`/`disconnect()` default-no-op virtuals; `GarminConnect` overrides both; one
`AddCloudWizard` capture drives persist on both pages' id-gated `succeeded(GarminAuthSuccess)` (direct + post-MFA);
`deleteClicked()` calls generic `disconnect()`. TEST-051 (both paths + stale-reply-no-overwrite, A3-R003-06
preserved) + TEST-052 (disconnect clears tokens/preserves sidecars + base no-op). **A3-R008-01 FIXED** (producer now
called → real uid resolves end-to-end), **D-R008-01 FIXED** (real disconnect() virtual + DES-002 prose corrected).
Also incidentally closes the REQ-004/006 C++ token-save wiring (GarminTokenStore::save now called in
production). Ratified during build: a 2nd base virtual `persistConnectSuccess` (symmetric to disconnect, both
no-ops) so the wizard dispatches through `CloudService*` without a Garmin cast (DEC-019 cascade note).
Prior focus (superseded): Slice D producer done, trigger-gated. Slice D (DEC-018 B) built the producer — `GarminAuthSuccess.tokenBlob`
carry-forward, `GarminTokenStore::persistConnectSuccess`(ordered tokens.json→active-account.json write)/`saveActiveAccount`/
`loadActiveAccountUserId`/`clearAccount`, `resolveGarminUserId` REPOINTED to active-account.json; TEST-049 (producer
unit) + TEST-050 (end-to-end, NO ctor override — the LSN-024 proof). **But the builder correctly FIRED the
stop-and-report hatch: the producer has NO production caller** — `persistConnectSuccess`/`clearAccount` are never
invoked on a real connect/disconnect, so A3-R008-01 is only PARTIALLY closed (producer green, trigger unwired). Three
constraints block a mechanical trigger (→ DEC-019): (i) the wizard deliberately defers config-dir routing
(AddCloudWizard.cpp:180-185); (ii) the MFA success path bypasses `GarminCredentialsPage::onAuthFinished` (early-returns
on MfaRequired≠InFlight), so ONE trigger must catch BOTH direct + post-MFA `finished(GarminAuthSuccess)`; (iii) there
is NO `removeSettings`/disconnect virtual in CloudService.h — DES-002's "removeSettings overridden to Disconnect"
surface does not exist under that name (D-R008-01, design↔code). Producer/reader/tests are green and ready to be
CALLED the moment DEC-019 picks the trigger layer.
Prior focus (still true): **A+B+C CODE-COMPLETE, A3-R008 partial.** A = listing seam (T-042/043/044), B =
dedicated `GarminSidecarStore` (DEC-017 A; T-045/046), C = `GarminConnect::readdir` sync orchestration composing
A+B onto the CloudService sync contract (T-047/048): concurrent-guard (REQ-NF-Perf-002), since=from/backfill-state/
now()-7d, off-thread list via new `IGarminDownloadClient::listActivities`, CloudServiceEntry per activity, Tier-1
short-circuit filters already-imported ids BEFORE download (DES-010 5a), readFile-success records via
GarminSidecarStore + advances backfill-state. **Verification Gate PASS on all three** — orchestrator independently
confirmed pytest 25/25 + garmin ctest 17/17 executables (REQ-007 readFile intact, +testGarminConnectSync/
testGarminSidecarStore), FILES vs git status, LSN-018 both-lists, short-circuit-in-readdir on disk, app links clean.
**A3-R008 attack surface (catalogued from builder NOTES):**
(1) **BLOCKING-for-end-to-end cross-slice gap** — `GarminConnect::resolveGarminUserId()` reads `garmin_user_id`
from tokens.json but NO connect-flow code writes it yet → production readdir resolves EMPTY uid + no-ops; needs a
REQ-002/004 connect-flow persistence slice (tests use the ctor uid-override to stay deterministic).
(2) since-format: readdir emits `"yyyy-MM-dd HH:mm:ss"` UTC, untested against the real Python `list_activities_since`
(OQ1-adjacent; ISO-8601 fallback exists on parse but not on emit).
(3) record-ordering: sidecar records "staged" not "imported into RideCache" (optimistic Tier-1; RideCache Tier-2 is
the safety net per DES-010) — a downstream parse-reject would still skip next sync.
(4) Slice-B residuals still live: `recordImported` RMW silently rewrites torn/owner-wider map; malformed entries
skipped as Ok-with-gaps; `<uid>` raw in filename (no sanitization); Windows-ACL TODO stub (A3-R004-09 posture).
(5) `m_pendingStartTimes` never pruned (bounded, harmless); `to` param ignored (incremental-only, by design);
capabilities() still `Query|Download` (no Sync bit in the CloudService enum).
(6) Slice-A residuals: OQ1 listing-API signature pin; summary normalization drops all keys but activityId/
startTimeGMT; non-dict-item branch surviving-mutant candidate (no `list_bad_item` pystub scenario).
**Slice A** mirrors the REQ-007 download seam: `garmin_client.list_activities_since(ts_gmt)` (calls
`get_activities_by_date` — OQ1-pinned by fake, real signature unconfirmed) → new `PyListOutcome`/`GarminActivitySummary{activityId,startTimeGMT}`
+ pure-virtual `IGarminPyAdapter::listActivitiesSince` → `PyEmbeddedAdapter` marshalling (iterator protocol,
classifyListException) → `GarminWorker` ListActivities op emitting `activitiesListed(QUuid,summaries)` /
`listFailed(QUuid,GarminListFailure)`. T-042/043/044. Every test Fake/stub gained a conforming
`listActivitiesSince` override (seam-conformance, 5 test files). **Verification Gate PASS** — orchestrator
independently re-ran pytest 25/25 + garmin ctest 15/15 executables, diffed FILES vs `git status` (13 M + 1 ??,
all Garmin-scoped, no governance writes), confirmed `PyEmbeddedAdapter` is the sole production implementer + app
links clean, and audited T-044 (sinceGmt+summaries verbatim + worker-thread) / T-042 (LSN-006 type-classify,
empty-is-success) as faithful criterion encodings. **A3-R008 not yet run** (deferred to cover Slices A+B together,
per the REQ-003 A+B→A3 pattern). Slice A residuals for A3: OQ1 listing-API signature pin; summary normalization
drops all keys but activityId/startTimeGMT (may need widening for RideCache dedup in C); non-dict-item branch has
a surviving-mutant candidate (no `list_bad_item` pystub scenario yet).

Prior: **REQ-003 (MFA) CODE-COMPLETE — both slices built, WORKING TREE, UNCOMMITTED (user has not asked to
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

NEXT_GATE: **Docs-record commit of the governance ledgers, then the user picks the next REQ.**
1. **Docs-record commit** (mirrors `b47954308`): STATE.md, WIKI.md, lessons.md, `.claude/workflow-garminconnect/`
   {traceability,findings,decisions,design}.md. Path-scoped — the tree still carries unrelated pre-session
   Coach/Gui/CMake/vcpkg/skill edits that must stay out (LSN-010/007). The DEC-015 drift lint runs as a pre-commit
   hook on exactly these files and currently exits 0.
2. **Then the user's call on the next REQ.** Strongest candidates: the **follow-on lifecycle REQ** seeded by
   A3-R012-F10/F12 (bind session lifetime to the account; close the TOCTOU residual and the CloudService dialog
   leaks — this is the honest remainder of DEC-020 Option A); **REQ-016** (dedup record-after-confirm, the ticketed
   LSN-025 debt); **REQ-010** (bulk backfill); **REQ-014** (error translation); **REQ-015** (CAPTCHA). Cheap
   cleanups available any time: A3-R012-F3/F6/F9. Manual Phase-3 residuals unchanged (REQ-NF-Perf-001/003 stopwatch,
   OQ1 live smoke — both need a real Garmin account).
--- superseded (kept for provenance): the DEC-020 build gate ---
NEXT_GATE-PRIOR: **DEC-020 hardening slice IN FLIGHT (builder dispatched 2026-08-02), then re-verify, then ONE combined commit.**
User dispositioned A3-R012-F1 as **DEC-020 Option C** (2026-08-02) and chose a single commit covering tests + fix.
1. **qgdw-builder building now:** TEST-057 (readFile/readdir fail closed once the account is disconnected — the F1
   mitigation, must go RED first against current production), TEST-058 (clearAccount sweeps the `<path>.tmp` siblings
   — F2), TEST-059 (empty-uid write guard pinned — F4, kills MUT-C), plus TEST-055 strengthened with mode+mtime (F5)
   and the two `!exists` assertions that make T-055's decorative disconnect load-bearing (F8). Explicitly OUT of
   scope: live-instance invalidation / DEC-019 rework, the CloudServiceSyncDialog + syncCloud leaks (F12), F3, F6,
   F9, F10, and anything LSN-025/REQ-016.
2. **Verification Gate** on return — re-run `ctest -R "Garmin|AtomicFile"` (must be 18/18), the `GoldenCheetah`
   app-link (LSN-018: this slice DOES touch production), FILES vs `git status`, and confirm TEST-057's RED evidence
   is real (a fail-closed test that never failed pre-fix proves nothing).
3. **Then ONE path-scoped commit** — `src/Cloud/{GarminConnect,GarminTokenStore}.*` + `unittests/Core/garminconnect/
   testGarminConnectConnectPersist.cpp` (+ any hunk-split `src/CMakeLists.txt` line, which is ALREADY dirty with
   unrelated Coach edits). NEVER `git add -A` (LSN-010/007). Pre-commit runs clang-format on these files for the
   first time — `clang-format` is absent from this environment, so a reformat is LIKELY: re-run garmin-fast + the
   app-link AFTER any hook mutation, before accepting the commit (LSN-007). Then the ledger-record step.
4. **Still open, non-blocking, NOT in this slice:** A3-R012-F3 (ordering invariants unobserved), F6 (discarded
   clearAccount bool / silent failed delete), F9 (uid unvalidated as a filename component), F10 (uid re-resolved not
   latched), F12 (CloudService instance leak — pre-existing, generic layer). F10+F12 seed the follow-on lifecycle REQ
   that DEC-020 defers. Also open: VAL-016's two coverage WARNs (backfill-state "not consulted" unproven; "full SSO"
   encoded only as its negative) and A3-R012-F7/F11 accept-notes.
--- superseded (kept for provenance): the A3+CLV dispatch gate ---
NEXT_GATE-PRIOR: **REQ-012 gate — A3-R012 adversary + incremental CLV, then disposition B-R012-01, then the commit.**
Order (RIGOR: FULL, so the feature gets a named cycle + a validation before it commits):
1. **Spawn `qgdw-adversary` for A3-R012** over the REQ-012 changeset. Brief it with: the prd.md:84 criterion
   verbatim; TEST-052/054/055/056; the B-R012-01 ordering question as its FIRST probe (can any long-lived opened
   GarminConnect instance survive a disconnect in production? trace the CloudService sync open/close lifetimes and
   AthletePages.cpp:164-169); plus the builder's own attack surface — the shared-FakeSyncClient failure-path footgun,
   byte-identity-vs-mtime "untouched", helper duplication, and whether LSN-025's record-before-confirm undermines
   T-055(c)'s premise. Fresh git state MUST be pasted (LSN-023 — the adversary is git-blind).
2. **Spawn `qgdw-validator`** for an incremental CLV over the REQ-012 changeset (in parallel — both read-only).
   Brief that the traceability row + WIKI registry bump + findings rows are ALREADY merged (avoids LSN-016 merge-lag
   false FAILs) and paste live `git status`/`git log`.
3. **Disposition B-R012-01 with the user** — (a) accept + reword the criterion to match the fresh-instance reality,
   (b) give disconnectService() a real in-memory teardown + test it, or (c) whatever A3-R012 turns up. Only open item.
4. **Then the REQ-012 commit** — path-scoped to `unittests/Core/garminconnect/testGarminConnectConnectPersist.cpp`
   ONLY (this slice touched nothing else). NEVER `git add -A`: the tree still carries unrelated pre-session
   Coach/Gui/CMake/vcpkg/skill edits (LSN-010/007). Pre-commit runs clang-format on this file for the first time —
   if it reformats, re-run garmin-fast BEFORE accepting the commit (LSN-007). Then the ledger-record step.
--- superseded (kept for provenance): the post-REQ-008 gate ---
NEXT_GATE-PRIOR: **AWAITING USER — pick the next REQ (nothing is blocked; REQ-008 closed at `b47954308`, 2026-07-20).**
Open candidates per traceability.md: REQ-016 (dedup record-after-confirm/reconcile — the ticketed A3-R008-F1+F2
follow-up, smallest and closes an accepted reliability debt), REQ-010 (bulk backfill, paginated/resumable — the
natural sequel to REQ-008's incremental sync, DES-009), REQ-014 (friendly error translation, DES-003/008),
REQ-015 (CAPTCHA path — shares the GarminMfaPage/GarminCredentialsPage family, also closes A3-R003-09),
REQ-009 (first-connect ToS notice), REQ-012 (disconnect deletes tokens — largely realized by the DEC-019 slice,
needs a REQ-012-scoped test + trace row rather than new code). Manual Phase-3 residuals remain: REQ-NF-Perf-001/003
stopwatch, OQ1 live smoke (needs a real Garmin account). On the user's pick: orient → load the REQ (Tier 2) →
DEC slice → allocate TEST ids from REGISTRIES → spawn `qgdw-builder`.
--- superseded (kept for provenance): the REQ-008 gate ---
NEXT_GATE-OLD: **USER CHOSE: build REQ-008 Slice D (uid-producer wiring) to close A3-R008-01 before commit.** BLOCKED on
subagent spawn until ~9:20pm Australia/Hobart (account-wide session-usage limit — the adversary died on it; builders
can't spawn either). Orchestrator briefing-prep DONE: the auth-success hook point is `GarminWorker::emitAuthOutcome`
Success arm (GarminWorker.cpp:58) → `finished(GarminAuthSuccess{garmin_user_id,display_name})` (IGarminAuthClient.h:29,
NOTE: GarminAuthSuccess does NOT currently carry the token blob) → `GarminCredentialsPage::onAuthFinished`
(GarminCredentialsPage.cpp:95, persists NOTHING today). `PyAuthOutcome` carries a tokenBlob (REQ-004 TEST-013); the
blob comes from `dump_tokens()` = raw garth `dumps()` (NO garmin_user_id key).

**Plan (in order):**
0. **DEC-018 DONE** (Option B, active-account.json) + **Slice D producer DONE** (green, trigger-gated) — see CURRENT.
1. **DEC-019 DONE (Option C)** — scout-researched (all citations orchestrator-verified), user chose C:
   `CloudService::disconnect()` default-no-op virtual + GarminConnect owns persist/disconnect; a single wizard-level
   `finished()` capture drives persist on both auth paths. Key reframe: REQ-012's delete-on-disconnect has NO sibling
   precedent (deleteClicked/AthletePages.cpp:143-161 only flips active flags; siblings keep tokens in QSettings
   forever) → Garmin is stricter, so a testable GarminConnect::disconnect() seam beats a magic-string GUI branch.
2. **DEC-019 trigger wiring slice DONE + Gate-PASSED + merged (2026-07-20).** TEST-051/052; stale-reply impl = (b)
   per-page id-gated `succeeded` signal; A3-R008-01 + D-R008-01 FIXED; DES-002 prose corrected; the persistConnectSuccess
   2nd-virtual ratified. garmin 18/18, app links.
3. **A3-R008 full pass DONE (2026-07-20) — VERDICT FINDINGS, 1 BLOCKING (F1).** See BLOCKING/LAST_CYCLE.
4. **NEXT GATE — USER DISPOSITION ON F1 (blocking).** Fix-now (reconcile pass or record-after-confirm — a small
   hardening slice) vs accept-with-rationale + ticket for v1. Recommend bundling the cheap non-blockers into the same
   slice IF fixing: F3 (add list_bad_item pystub coverage), F4 (rename disconnect()→disconnectService() to kill the
   QObject name-hiding footgun), F2 (sidecar RMW salvage). F5 stays an OQ1 accept-note (spike vs the real wheel).
5. **Then the REQ-008 feature commit** (path-scoped to the Garmin set — now also includes src/Gui/AthletePages.cpp +
   src/Cloud/CloudService.h + the wizard/page/GarminConnect edits; NEVER `git add -A`, unrelated pre-session Coach/Gui/
   CMake/vcpkg + skill edits stay out — per LSN-010/007).
--- superseded (kept for provenance): the old plan step 1 read ---
1-OLD. **DEC-garmin-018 FIRST (Three Options, present to user) — tokens.json envelope for garmin_user_id**, because
   resolveGarminUserId needs a top-level uid but tokens.json's schema is security-locked by REQ-006 (loadChecked
   perm-check) + REQ-007 (from_tokens/loadTokens parse). Candidate options: (A) WRAP tokens.json as
   `{garmin_user_id, tokens:<blob>}` — one file/one write, but from_tokens+loadTokens+loadChecked callers must read
   `.tokens` (touches the locked contract); (B) SEPARATE account-agnostic `garminconnect/active-account.json =
   {garmin_user_id}` — leaves tokens.json a pure OAuth blob (REQ-006/007 untouched), resolveGarminUserId reads the
   new file; adds one more perm/atomic-write file; (C) PARSE the uid out of the existing garth blob — no new write
   but fragile/OQ-risk on garth internals. Recommendation leans B (least disturbance to the security-locked file).
2. **qgdw-builder Slice D**: wire the chosen persistence into the auth-success path (carry the blob to onAuthFinished
   OR persist in the worker/CloudService lifecycle; call GarminTokenStore::save + write the uid per DEC-018), + an
   END-TO-END test that connects with a REAL (non-override) uid round-trip → readdir resolves it (the test that
   A3-R008-01 says is missing). TEST ids garmin-T-049 (+T-050 if the envelope needs its own unit). Then Verify+merge.
3. **qgdw-adversary — FINISH A3-R008** over the now-complete A+B+C+D (avoids running A3 twice): the unrun probes —
   since-format contract, stage-vs-import record ordering, concurrent-guard wedge/teardown, non-dict-item
   surviving-mutant (add a `list_bad_item` pystub scenario if it survives), sidecar RMW-over-corrupt data-loss.
4. Verify, disposition, re-spawn builder only for any BLOCKING finding, re-run until clean. **Then the REQ-008
   feature commit** — path-scoped to the Garmin set ONLY (src/Cloud/{GarminConnect.*,GarminDownloadClient.*,
IGarminDownloadClient.h,IGarminPyAdapter.h,PyEmbeddedAdapter.*,GarminWorker.*,GarminSidecarStore.*} +
src/Python/garminconnect/garmin_client.py + src/Python/garminconnect/tests/test_adapter_list.py +
unittests/Core/garminconnect/** + the two CMake hunks), NEVER `git add -A` (the working tree still carries unrelated
pre-session Coach/Gui/CMake/vcpkg + skill-methodology edits — NOT Garmin), per LSN-007 (re-verify post pre-commit
reformat) + LSN-010 (stamp the ledger record).

Prior gate: **REQ-003 COMPLETE + COMMITTED `8cbc4722d`.** LSN-010 ledger-record stamped (traceability Commit
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

BLOCKING: **0 open blocking — A3-R012-F1 FIXED (mitigated) 2026-08-03 in `f001c7d20`.** DEC-020 Option C landed:
`accountStillConnected()` gates readFile+readdir, clearAccount sweeps the .tmp siblings, TEST-057/058/059 + a
strengthened TEST-055 (per-id verdicts → traceability.md). F2/F4/F5/F8 FIXED in the same slice. **Accepted residuals — see DEC-020 in decisions.md; the lifecycle REQ owns them:** the gate is TOCTOU-racy — a disconnect landing between check and download still gets ONE activity
through — and the restored garth session stays alive in the worker, so a holder of `IGarminDownloadClient` that
bypasses GarminConnect is still unblocked. **Still open, non-blocking:** F3 (ordering invariants unobserved — MUT-1/
MUT-N survived), F6 (clearAccount's bool discarded; a 0500 dir gives a silent failed delete while the UI says
disconnected), F9 (uid unvalidated as a filename component), F10 (uid re-resolved per call, not latched), F12
(CloudService instance leak — pre-existing, generic layer). F10+F12 seed the follow-on lifecycle REQ. Plus VAL-016's
two coverage WARNs (backfill-state 'not consulted' unproven; 'full SSO' encoded only as its negative) and the
F7/F11 accept-notes. Superseded detail of the original blocking finding follows: **A3-R012-F1 (2026-08-02): Disconnect does not stop a live, already-`open()`ed session,
so a user can keep downloading from an account they just disconnected.** `disconnectService()` deletes the token
files and clears NO in-memory state; nothing enumerates or shuts down live GarminConnect instances. Reachable path,
ALL FOUR SITES ORCHESTRATOR-VERIFIED: AddCloudWizard finish-with-sync opens `CloudServiceSyncDialog` via
`syncnow->open()` (AddCloudWizard.cpp:889-893) — WINDOW-modal, parented to mainWindow — over an opened GarminConnect
holding a restored garth session; `ConfigDialog` is a PARENTLESS top-level QMainWindow (ConfigDialog.cpp:37-41) so
Options→Athlete→Accounts→Delete stays usable; `deleteClicked()` mints a DIFFERENT instance (AthletePages.cpp:164-169)
and never touches the live one; Download then calls `store->readFile(...)` (CloudService.cpp:1400) with NO token
re-check. `CloudServiceSyncDialog` declares a ctor but NO dtor (CloudService.h:336) and never closes its store;
`MainWindow::syncCloud` leaks its `db` outright (MainWindow.cpp:2561-2566), so the worker thread + embedded-interpreter
session live to process exit. Second-order: `recordImport` re-resolves the now-EMPTY uid and early-returns, so those
downloads are imported but recorded in NO sidecar — dedup silently broken for that account. **This adjudicates
B-R012-01: the "before clearing in-memory state" clause is UNIMPLEMENTED, not merely unencodable — do NOT reword it
away.** Root is DEC-019's choice to mint a fresh instance in deleteClicked() precisely to avoid touching live
instances, so the fix reopens DEC-019 or raises a follow-on REQ; not patchable in design.md. **The REQ-012 test slice
itself is safe to commit** (adversary's own assessment) — the blocker is on closing REQ-012 as "criterion covered",
not on the tests. **AWAITING USER DISPOSITION.** Non-blocking from the same pass: F2 (clearAccount never sweeps the
`tokens.json.tmp` sibling — a crash leaves the full OAuth blob through a Disconnect; ~2-line fix), F3 (both DEC-018
write-order and delete-order invariants unobserved — MUT-1/MUT-N survived), F4 (recordImport's empty-uid guard
unpinned — MUT-C survived), F5 (TEST-055's "untouched" is bytes-only — MUT-D2 chmod'd prior sidecars 0644 and PASSED),
F6 (clearAccount's bool discarded; 0500 dir ⇒ silent failed delete, UI says disconnected). Informational: F7 (T-055(c)
forward-coupled to LSN-025 — REQ-016 will break it), F8, F9 (uid unvalidated as a filename component), F10, F11,
F12 (pre-existing CloudService leak, NOT REQ-012 scope). Lessons LSN-029/030/031 captured from the survivors.
Prior REQ-012 findings: **B-R012-01** (criterion↔code —
the "delete tokens before clearing in-memory state" ordering clause has no mechanism; disconnectService() clears no
in-memory state and deleteClicked() uses a fresh instance; first probe for A3-R012, then user disposition) and
**B-R012-02** (informational, test-fixture hygiene). Lesson **LSN-028** captured (a REQ satisfied as a side-effect of
another REQ's slice needs its own clause-by-clause criterion audit). Prior: **F3 + F4 FIXED + Gate-PASSED + merged (2026-07-20)** — TEST-053 non-dict-item coverage
(mutation-proven) + `disconnect()`→`disconnectService()` rename (QObject name-hiding killed); garmin 18/18, pytest
25/25, app links. **All A3-R008 findings dispositioned — REQ-008 ready for the path-scoped commit.** **A3-R008-F1 — ACCEPTED-WITH-RATIONALE for v1, ticketed as REQ-016** (record-after-confirm/reconcile;
deferred). User chose accept+ticket (2026-07-20): read-only sync of Garmin's own generally-valid FIT; the proper fix
is non-trivial (sidecar records the transient staging name, not the final RideItem) and better scoped as REQ-016;
documented + lessoned (LSN-025), not buried. F2 folded into REQ-016. Details of the original defect:
The full A3-R008 adversary pass (2026-07-20) found it: `GarminConnect::readFile` records the Tier-1 dedup entry (imported-<uid>.json) at
GarminConnect.cpp:374/:392 BEFORE the async base import confirms a parse; on a parse-fail (CloudService.cpp:1937
ride==NULL → silent return, no file written) the activity is recorded-as-imported and PERMANENTLY, SILENTLY skipped
on every future sync — Tier-2 has nothing to catch. CONFIRMED on disk. Violates REQ-008/US-3 "sync gets everything."
Fix options: (1) reconcile pass (drop sidecar entries whose RideItem doesn't exist → re-download), (2) record-after-
confirm (new hook from base readComplete-success — touches CloudService base), (3) accept-with-rationale + ticket
(Garmin FIT usually valid; document the edge). Blocks the REQ-008 commit until dispositioned. Lesson LSN-025.
Non-blocking open from the same pass: F2 (sidecar RMW clobbers on torn precondition — cheap salvage fix, LSN-026),
F3 (PyDict_Check guard correct but ZERO coverage — add list_bad_item pystub, mutant-confirmed survivor), F4
(CloudService::disconnect() name-hides QObject::disconnect for ~15 subclasses — inert footgun, rename to
disconnectService, LSN-027). Accept-note: F5 (get_activities_by_date single-arg "since" may mismatch the real
two-arg date-range wheel API — DEC-014 OQ1, spike vs real wheel before live). Prior blockers all remain fixed:
A3-R008-01 FIXED (DEC-019 trigger wired — producer now called, live sync resolves a real uid end-to-end; TEST-051/052). D-R008-01 FIXED (CloudService::disconnect() virtual + DES-002 prose corrected). Accept-with-note
residual for the full A3-R008 pass: T-051 exercises the wizard→base-virtual→real-producer wiring through a STUB
CloudService (GarminConnect can't link into the Python-free wizard harness — same constraint as A3-R007-02); the real
GarminConnect override is covered by T-050 (persist) + T-052 (disconnect). The remaining catalogued A3-R008 probes
(since-format contract, stage-vs-import record ordering, concurrent-guard wedge, non-dict-item mutant,
RMW-over-corrupt) are STILL UNRUN — the full adversary pass is the next gate. Prior REQ-003/007 residuals all remain resolved/accepted:
A3-R003-01 FIXED (T-036 real-bridge coverage, BLOCKING CLEARED). B-R003-01/02/03 FIXED
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

LAST_CLV: **VAL-016 — 2026-08-02 — incremental over the REQ-012 changeset (TEST-054/055/056 + spine). Verdict
PASS-WITH-WARN: 0 FAIL, checks 2 (spine) / 3 (no false-done) / 7 (cross-REQ integrity) clean.** Spine confirmed:
DEC-001/003/017/018/019 + DES-002/004/010 all exist and genuinely govern REQ-012; every cited TEST id resolves to a
real slot; REQ-012's row credits the mechanism to REQ-008/`ff9cce966` rather than claiming it, and REQ-008's row
still reads correctly. 9 WARNs — **6 REMEDIATED this pass by the orchestrator** (all spot-checked at their cited
locations first): (1) WIKI VAL registry line carried TWO allocator pointers, `next:garmin-017` head + a stale
`next:garmin-016` tail — my own edit, trailing pointer deleted; (2) a WIKI line paired a REQ id with a status word in a
non-canonical file (DEC-015) — also my own edit, reworded to point at traceability; (3) design.md:489
still said `disconnect()` where A3-R008-F4/LSN-027 renamed the shipped symbol to `disconnectService()` — cascaded
(this is the SECOND instance of the D-R008-01 defect class: design naming a hook that doesn't exist under that name);
(4) DES-002's storage path block omitted `active-account.json` entirely and (5) its prose still said the uid is
"recorded once into tokens.json" — both pre-DEC-018 text, corrected with the write-ordering rationale; (6) DES-002's
invariant still prescribed initializing the library with an explicit `tokenstore=` path, which REQ-006 Slice B
(`3edb705cb`, A3-R004-M3) REMOVED in favour of auth-only construction — corrected. **NOT reproduced:** the validator's
check-6 status-SSOT WARN against STATE.md — the deterministic lint (`.claude/hooks/ledger_drift_lint.py`) exits 0
clean over the whole repo post-fix, so its hand-reasoning was stricter than the mechanism. **Still open (2 coverage
WARNs, handed to A3-R012):** "not consulted" is proven for `imported-<uid>` but NOT for `backfill-state-<uid>` (no
assertion that B's resolved `since` differs from A's cursor), and "full SSO on reconnect" is encoded only as its
negative (no silent restore) — the positive half lives in the REQ-002/003 wizard tests and is uncited. Validator
lesson-candidates recorded for A5: rename-cascade-into-design, and DEC-adds-a-file must update the owning DES's path
block in the same byproduct pass. Prior: VAL-015 — 2026-07-19 — FINAL REQ-003 commit-gate CLV over the complete feature (T-027..041 + hardening).
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
LAST_CYCLE: **A3-R012 (REQ-012 disconnect contract, TEST-054/055/056 + the production disconnect path) — 2026-08-02
— VERDICT: BLOCKING-FINDINGS.** 12 findings (1 blocking → see BLOCKING, 5 non-blocking, 6 informational). The pass
hand-applied 7 production mutations beyond the builder's 4 and rebuilt/re-ran the suite after each: **5 SURVIVED**
18/18 (MUT-1 write-order inversion, MUT-N delete-order inversion, MUT-B secrets renamed-not-removed, MUT-C empty-uid
guard dropped, MUT-D2 prior-sidecars rewritten+chmod-0644-with-bytes-preserved) — i.e. the feature's most loudly
documented invariants were its least defended. MUT-F (open() restores before failing) was killed by TEST-054's
`restoreCalls == 0`, proving that assertion load-bearing. **REFUTED (verified non-issues — do not re-litigate):**
R1 the shared-FakeSyncClient footgun is harmless (no pre-existing slot calls readFile; the class is TU-local —
testGarminConnectSync.cpp defines its own); R2 empty-uid on the READ side is guarded + covered; R3 path traversal via
a hostile uid is NOT reachable (`imported-` prefix glue means `../evil` needs a directory named `imported-..`, and
AtomicFile never mkpath()s); R4 a REQ-006-refused 0644 tokens.json still deletes cleanly (POSIX unlink authorises on
the DIRECTORY mode); R5 partial clearAccount deletes are killed by the existing pair of tests; R6 TEST-054's
restoreCalls assertion is NOT vacuous (MUT-F kill); R7 no syntactic tautologies anywhere in the new slots; R8
cross-account WRITE leaks are killed by the byte-compare (only same-bytes rewrites slip through); R9 LSN-025 does not
invalidate T-055(c) today but WILL break it at REQ-016. **RESTORATION VERIFIED BY THE ORCHESTRATOR** — adversary
mutated only GarminTokenStore.cpp + GarminConnect.cpp, restored via `git checkout --` with md5 proof; I independently
confirmed `git status --porcelain src/Cloud/` empty and re-ran ctest 18/18. Verification Gate PASS (I re-checked F1
at all four cited sites + F2's missing tmp-sweep myself before merging). Prior: A3-R008 (REQ-008 incremental sync, FULL feature A+B+C+D+trigger) — 2026-07-20 — VERDICT: FINDINGS
(complete pass, fresh adversary). 1 BLOCKING (F1 record-before-confirm — see BLOCKING) + F2/F3 non-blocking +
F4/F5 informational. REFUTED (verified non-issues): uid-producer holds end-to-end (wizard's cloudService is always a
real GarminConnect via clone(context)); stub-vs-real dispatch = test-harness-only; stale-reply guard double-gated
(id + InFlight); disconnect dir matches (same context); concurrent-guard RAII correct + tested. Live mutation
experiment on PyDict_Check (F3) — files snapshotted + md5-restored, ctest 18/18 clean after. Happy-path verdict: the
complete feature SATISFIES REQ-008/US-3 for well-formed data; F1 is the reliability gap on imperfect data. Lessons
LSN-025/026/027 captured. Prior: A3-R008 (partial, session-limit) — 2026-07-19. Superseded by this full pass.
LAST_CYCLE_PRIOR: A3-R008 (REQ-008 incremental sync, A+B+C) — 2026-07-19 — VERDICT: PARTIAL/FINDINGS (subagent adversary
terminated on a session-usage limit before reporting; orchestrator ran the highest-value hypothesis INLINE — the
commit-gating uid-resolution check). Result: **1 CONFIRMED blocking-for-end-to-end finding A3-R008-01** (uid-producer
gap — see BLOCKING). Happy-path verdict for a correctly-resolved uid: the A+B+C logic satisfies REQ-008/US-3
(Tier-1 short-circuit prevents re-download, records advance the cursor, concurrent-guard rejects) — the defect is a
missing PRODUCER, not consumer logic. The remaining catalogued probes (since-format contract, stage-vs-import record
ordering, concurrent-guard wedge/teardown, non-dict-item surviving-mutant, sidecar RMW-over-corrupt data-loss) were
NOT run — a fresh-context qgdw-adversary must complete A3-R008 after the 9:20pm Australia/Hobart reset before the
feature gate closes. Lesson captured: LSN-024 (consumer-of-deferred-contract). Prior:
A3-R003 (REQ-003 MFA, both slices) — 2026-07-18 — VERDICT: FINDINGS. Feature sound on the happy
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
