# REQ-028 Wave — Read-Only Process-Integrity & Architecture Audit

**Audit date:** 2026-08-22
**Repo:** `/media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah`
**Branch:** `garmin/req028-row-lifetime` @ `da455c79e8ead680157aa5bae105a5c2135f1de9`
**Scope:** process-integrity + architecture audit of the REQ-028 wave, starting from `/tmp/goldencheetah-req028-handoff.md`, treating every document, agent report, test count, decision, comment and "SSOT" declaration as a **claim**, not truth.
**Nature:** read-only. No repository artifact was modified, committed, restored, stashed, checked out or reformatted during the audit. This report file is the only thing written, and it was written afterwards on explicit instruction.

## Remediation status — 2026-08-22

The audit below remains the historical diagnosis. Its architecture blockers C1–C4 were remediated by formally reopening DEC-036 and building Option C: every write now carries an opaque operation ID from dispatch through completion; reads retain their buffer identity; Refresh makes outstanding records row-free without deleting their identity; no correctness-degrading retirement cap remains; and every suspended completion/driver frame rechecks its batch generation before mutating or re-driving. TEST-126 now observes operation-record conservation directly.

Verification is green: full sync-dialog offscreen **78/0**, focused Option-C checks **7/0** on offscreen and minimal, seeds **32** and **447** pass on both backends with ASan, `garmin-fast` is **25/0**, and the GoldenCheetah target builds. A focused adversarial pass found four blockers (SixCycle sender correlation, two post-open generation gaps, missing conservation assertion, and a test-fixture stack lifetime defect); all four were reproduced and repaired before those green runs.

**Preamble corrected 2026-08-22 (the audit body below is unchanged and remains the historical diagnosis):**

- **Commit pointers.** The remediation is implemented in **`e48f7d123`** (26 files, all under `src/Cloud/` or `unittests/Core/garminconnect/`) with the gate record in **`98df50535`**. This report's header pins `da455c79e`, which was HEAD at audit time and is correct for the audit; it is not the state of the remediation.
- **REQ-028(c) is CLOSED, and the sort route does not hold it open.** The sentence previously here — *"This does not close REQ-028(c): the independent sort-route/positional-driver defect remains the next slice"* — contradicted REQ-028's own acceptance criterion. `prd.md:154` has stated since 2026-08-16, by explicit user decision, that S-R028-01 is **out of REQ-028's scope, tracked separately**. Clause (c) is closed by DEC-036 Option C's explicit per-operation write identity. **The sort route is NOT closed and is not being quietly dropped: it remains open work, tracked on its own (B-R028-05's driver half), pending a decision on whether it earns its own REQ+DEC or an amendment.**
- **The fourth blocker, stated correctly.** "A test-fixture stack lifetime defect" understates it and mislocates the mechanism. It is **ORCH-033**: the test oracle is a member of the store, the queued delivery begins while that store is **alive**, and the store is deleted **re-entrantly inside** `CloudService::notifyWriteComplete` — the base call invokes the dialog's completion slot directly, that slot pumps events, and a teardown delivered there destroys dialog-then-store. The unsafe access is at the statement **after** the base call returns. Both lifetime windows are now guarded (`QPointer` handles on the queued delivery's captures; a `QPointer` self-guard across each base notification). **ORCH-033 is RESOLVED.**
- **Counting note.** `-functions` reports **73 declared test functions**; QtTest's `Totals:` reports **78 completed cases**, the difference being `initTestCase`, `cleanupTestCase` and data rows. The "78/0" above is correct. The two numbers are compatible and neither is drift.

The audit's process-compaction findings C5–C7 remain open; a lossless compaction plan exists, but automatic reduction is unsafe because 112 finding rows still require explicit classification.

---

## VERDICT: **SUBSTANTIAL PROCESS REDUNDANCY**

The *technical* mechanisms are largely orthogonal and defensible — only one genuinely redundant guard pair was found. The redundancy is in the **process apparatus**: per-id status is maintained in six places at once, the wave's governance checks are structurally incapable of detecting the contradictions they exist to catch, and the two files declared "the state" contradict each other on whether an agent is currently running.

Two **correctness/decision findings outrank the redundancy** — chiefly that the handoff's central safety claim ("the fuzzer is catching three live blocking defects") is false, and the blocker it misses is one the fuzzer is *structurally incapable* of seeing.

---

## 0. Executed evidence (separated from static reasoning)

All of the following was **executed** during the audit. Nothing else in this report was.

| Command | Result |
|---|---|
| `git rev-parse HEAD` | `da455c79e8ead680157aa5bae105a5c2135f1de9` |
| `git rev-list --left-right --count master...HEAD` | `0  1` (HEAD 1 ahead, master contains nothing extra) |
| `git rev-parse --abbrev-ref @{u}` | **fatal: no upstream configured** for `garmin/req028-row-lifetime` |
| `git status --porcelain \| wc -l` | **64** (52 modified tracked, 12 untracked) |
| `git diff --check` | **EXIT 0** |
| `python3 scripts/ledger_drift_lint.py .` | **EXIT 0** |
| `git stash list` | **1 stash** — `stash@{0}: On master: non-garmin WIP` |
| `git worktree list` | **2 worktrees** — second at `.claude/worktrees/cleangate-record` on `garmin/req027-cleangate-record` @ `6650a0288` |
| `git branch --no-merged master` | `garmin/req027-cleangate-record`, `garmin/req028-row-lifetime` |
| `ps aux` | **No agent, build or test in flight.** Only the auditing `claude` process. |
| binary vs source mtime | binary `Aug 21 22:43` **newer** than newest source (`…SyncDialogClose.cpp`, `Aug 21 22:41`) → **binary is not stale; no rebuild performed** |

### Test execution

Environment: `ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1`, bounded timeouts, no timeout fired.

| Run | Backend | Exit | Result |
|---|---|---|---|
| full target | `offscreen` | **1** | `Totals: 71 passed, 1 failed, 0 skipped, 49301ms` |
| full target | `minimal` | **1** | `Totals: 71 passed, 1 failed, 0 skipped, 50008ms` |
| `GC_FUZZ_SEED=32` | `offscreen` | **1** | `INV-1 outstanding<=1 violated: 2 transfers outstanding for the LIVE batch at once`, 111ms |
| `GC_FUZZ_SEED=32` | `minimal` | **1** | same violation, 62ms |
| `GC_FUZZ_SEED=447` | `offscreen` | **1** | `AddressSanitizer: heap-use-after-free … qtreewidget.h:192 in QTreeWidgetItem::setText` |
| `GC_FUZZ_SEED=447` | `minimal` | **1** | same sanitizer summary |

Seed 447 ASan frames, verified against the ledger's claim:

- write: `CloudServiceSyncDialog::completedRead … src/Cloud/CloudService.cpp:2894`
- freed by: `CloudServiceSyncDialog::refreshClicked … src/Cloud/CloudService.cpp:1607`
- allocated by: `refreshClicked … CloudService.cpp:1745`
- reaching route in the stack: `completedRead:2986` (tail `processEvents`) → `downloadClicked:2125` → `syncNext:2323` → … → `completedRead:2894`

**ORACLE-F1's diagnosis is confirmed exactly as written.** So is the handoff's env/line guidance: `unittests/Core/garminconnect/CMakeLists.txt:1457` and `:1486` do pin the two backends with exactly the quoted `ASAN_OPTIONS`, and `ImportSeamStubs.cpp` is compiled into three targets at `:1093/:1234/:1356`.

---

## 1. Critical correctness / decision findings, by severity

### C1 — BLOCKING. The fuzzer covers **two** of the three blockers, and is structurally blind to the third.

The handoff (`/tmp/goldencheetah-req028-handoff.md:36-37`) states: *"the single failure is the new fuzzer, which is catching three live blocking defects."*

Measured: seed 32 reproduces **A3-R028c-F2** (INV-1). Seed 447 reproduces **ORACLE-F1** (ASan). **No seed reproduces A3-R028c-F1**, and none can. A3-R028c-F1's harm is *a stale write completion accepted as live* — no row is freed and no second transfer is dispatched, so neither INV-1, INV-2 nor ASan can observe it. The only invariant that could is **ticket conservation**, which the oracle explicitly **DROPPED** as unobservable:

> `testGarminConnectSyncDialogClose.cpp:399-408` — *"DROPPED - TICKET CONSERVATION. `retiredWrites.count() + (inflight.armed && inflight.isWrite)` cannot be evaluated from outside: `inflight` and `retiredWrites` are PRIVATE members … this target declares no friendship and has no accessor … There is no widget that reflects either."*

Confirmed by grep: `TEST-128`, `TEST-129`, `TEST-130` occur **zero times** in the test file. **There is no RED/GREEN slot for A3-R028c-F1 anywhere.** The acceptance criterion in `STATE.md:1117-1121` — *"seed 32, the nine other INV-1 seeds, and seed 447 all pass"* — would therefore be met **by a fix that does not touch A3-R028c-F1 at all**.

This is the audit's most consequential finding: the wave's stated mechanical acceptance criterion does not cover one of its three blockers.

### C2 — BLOCKING. A3-R028c-F2 triggers DEC-036's own written reopen condition; it was processed as an amendment instead.

DEC-036 records two invariants it makes load-bearing (`decisions.md:1289-1295`), and prescribes the consequence of the second failing in the entry's own words:

> `decisions.md:1293-1295` — *"**At most one transfer is outstanding per live batch.** … **If it can ever exceed 1, the single ticket must become a set and B's score rises.** The build must assert `<= 1` across the existing suite."*

Seed 32 measures that it exceeds 1, on both backends. DEC-036's Option-A scorecard says so too — its reliability score is explicitly conditioned on this (`decisions.md:1255`: *"separated only by the one-shot `armed` bit, which rests on the ≤1-outstanding invariant"*), and its scalability score reads *"a single ticket slot; concurrent transfers would need a set, which is a redesign."*

The response was `decisions.md:1491` — *"DEC-034 / DEC-036 AMENDMENT … **Not a new decision.**"* — applying `batchGeneration` to the three completion slots.

**That amendment is a legitimate fix for the re-entrancy, but it is not an answer to the question DEC-036 posed.** Serializing with `batchGeneration` prevents the *second dispatch* rather than making the correlation state hold *two* outstanding transfers. Three materially different architectures are now live options and **none is on the record**:

- **ticket set** — DEC-036's own named consequence; makes correlation independent of serialization;
- **drain-before-restart** — DEC-037 Option B, scored and rejected *on the premise that only one transfer is outstanding*, which is now false;
- **explicit operation identity** — DEC-036 Option C / DEC-034 Option B (key addressing), repeatedly deferred and repeatedly named "the deliberate end-state."

DEC-037's rejection of Option B was scored under the falsified premise, so its comparative score is stale. Under this project's own rule (`references/adversarial-cycles.md:38`: *"If a finding is rooted in a foundational DEC, re-open that DEC via cascade — fix the root, not the symptom"*) and its own Three Options Doctrine, **this requires a formal reopen with three re-scored options, not an amendment.** The handoff's *"Both fixes are already-written DEC amendments, so no research round is needed"* (`:41`) is the operative error.

The amendment's evidence base is itself defective: `decisions.md:1499-1500` claims `batchGeneration` *"is read at only `:962`, `:1994`, `:2517`"*. Grep of the current tree returns **seven** references (`:962, :1994, :2207, :2517, :2532, :3133, :3275`). The orchestrator caught this at pre-flight and recorded the correction in `STATE.md:1110-1113` as ORCH-032 — **but the decision entry was never corrected, and ORCH-032 has no row in `findings.md`** (grep: 0 hits), while `WIKI.md:62` still advertises `next:ORCH-032`. The finding is simultaneously allocated and unallocated. The amendment's *claim* does survive (all three extra sites are inside drivers, none in a completion slot) — the citation does not.

### C3 — HIGH. DEC-037's accepted residual contradicts clause (c) verbatim.

Clause (c), exact wording, `prd.md:154`:

> **"(c) NO ROW IS LABELLED THAT THE FRAME DID NOT TRANSFER… every non-empty status cell names the outcome of a transfer that actually happened *to that row*"**

DEC-037's accepted residual, `decisions.md:1427-1431`:

> *"In the `[live, stale]` order **the two result strings land on the opposite rows.** … when they differ the user sees two labelled rows with swapped verdicts… **User decision 2026-08-19: accepted.**"*

Under `[live, stale]`, the live transfer's completion consumes the *retired* ticket and labels the abandoned row (`decisions.md:1422-1423`). That row's cell then names the outcome of a transfer that happened to the *other* row. That is clause (c)'s prohibition, not an edge of it.

Yet `decisions.md:1356` records `Serves: **REQ-028 clause (c) re-closed**`. **A clause cannot be "re-closed" while a residual accepted under the same decision violates its text.** The correct disposition is either (i) amend clause (c) in `prd.md` to carve out the same-activity two-row case, or (ii) record (c) as *partially met with a named residual*. Neither was done.

The later `CORRECTION 2026-08-21 (A3-R028c-F4)` at `decisions.md:1474-1477` argues the code is *more* compliant than the entry assumed — but it compares `result` against `tr("Aborted")` on the **single-row** route. It does not address the swap, and does not license the "re-closed" claim.

### C4 — HIGH. A3-R028c-F3's non-blocking disposition is incoherent, and the cap's justification rests on the invariant seed 32 falsified.

`src/Cloud/CloudService.h:859-861`:

> *"a dropped ticket degrades to exactly today's behaviour for that one transfer — its completion falls through to the live compare — **rather than to anything worse**"*

"Today's behaviour" is pre-DEC-037 behaviour, i.e. **A3-R028b-F3, which this register classifies BLOCKING**. A3-R028c-F3 (`findings.md:252`) says exactly this — *"That sentence describes the pre-DEC-037 blocking defect"* — and then dispositions itself **"OPEN — non-blocking."**

That is inconsistent with how severity is assigned everywhere else in this register: A3-R028-F1 is BLOCKING at four clicks, A3-R028c-F1 is BLOCKING at five. F3 needs eighteen. Reachability is graded by click count nowhere else; harm is the criterion. **Same harm, same code path, opposite severity.**

**Beyond the finding:** the cap's own premise is now falsified. `CloudService.h:853-855` justifies `maxRetiredWrites = 8` with *"An abort leaves at most one write outstanding, so k is 1 on every route this dialog can reach today."* Seed 32 measures two transfers outstanding for one live batch. If two writes can be outstanding, an abort abandons two — and the retire site (`CloudService.cpp:2081-2083`) retires only the single `inflight` ticket, so **the second abandoned write is lost without ever reaching the cap**. F2 does not merely break the ticket; it breaks the retire mechanism DEC-037 is built on. That consequence appears in no finding row.

### C5 — HIGH. `STATE.md` contradicts itself on whether an agent is running; ground truth says no.

- `STATE.md:20` (CURRENT): *"**Nothing is running.**"*
- `STATE.md:1105` (NEXT_GATE): *"`qgdw-builder` **DISPATCHED** 2026-08-21 on A3-R028c-F1 + F2 + ORACLE-F1 … **IN FLIGHT.**"*
- handoff `:39`: *"**Immediate next action** is the dispatch described in `STATE.NEXT_GATE`"* — i.e. not yet dispatched.

Three current-tier statements, three states. Ground truth from `ps` and from the absence of `TEST-128/129/130`: **nothing was dispatched, or it was dispatched and produced nothing.** A fresh agent following the handoff's instruction to read STATE second and "not re-derive" would wait for a returning agent that does not exist.

### C6 — MEDIUM. No adversarial-cycle or validation artifact exists for anything in this wave.

The audit brief was not to accept "Verification Gate PASS" without the artifact. There is none.

- `.claude/workflow-garminconnect/cycles/active/` — newest file `a3-req-002-tile-routing.md`, **2026-07-05**.
- `.claude/workflow-garminconnect/validations/active/` — newest file `val-012.md`, **2026-07-13**; `STATE.md:3066` names `LAST_CLV: VAL-017` (2026-08-05). **VAL-013…017 have no report file.**
- `validations/archive/` and `cycles/archive/` contain nothing from this wave.

Every "A3 verdict," "Verification-Gate PASS," and "orchestrator-mutation-proven" claim in this wave exists **only as orchestrator-authored prose inside `STATE.md` and `findings.md`**. `WIKI.md:61` advertises *"report files → .claude/workflow-garminconnect/validations/"* and `findings.md:3` asserts *"Cycle files hold the why"* — both are pointers to files that do not exist. This does not make the claims false; it makes them **unfalsifiable by anyone but their author**, which is the specific property the Verification Gate was designed to remove.

### C7 — MEDIUM. Neither governance check can detect the contradictions in this wave.

**The drift lint (`scripts/ledger_drift_lint.py`, exit 0).** Exactly what it does: scans **only** root `STATE.md`/`WIKI.md`, `wiki/*.md`, and `.claude/workflow-*/design.md` (`:93-105`, `:154-163`); `traceability.md`, `decisions.md`, `lessons.md`, `findings.md` are exempt by design (`:97-104`, `:113`). It flags an id token paired *on one line, in assignment shape, in binding scope* with one of **eight** status tokens (`:82-91`): `GREEN, CLOSED, DEFERRED, deferred, drafted, in build, _pending_, _uncommitted_`.

What it therefore **cannot** see: `ACCEPTED, BUILT, DONE, COMMITTED, ALLOCATED, PASS, BLOCKING, UNBUILT, STUB, ACTIVE, OPEN, RED`. Measured pairings of an id with one of those in the two files the lint *does* scan:

- **`WIKI.md`: 43** (ALLOCATED 12, BUILT 10, ACCEPTED 5, PASS 5, DONE 3, UNBUILT 2, STUB 2, BLOCKING 2, COMMITTED 1, ACTIVE 1)
- **`STATE.md`: 91** (PASS 25, ACCEPTED 13, BUILT 13, DONE 13, COMMITTED 12, ALLOCATED 8, BLOCKING 7)

134 pairings of exactly the kind DEC-015 exists to forbid, in exactly the files the lint exists to protect, and the lint exits 0. **A passing lint here proves only that eight specific tokens are absent.**

**CLV Check 5** (`findings.md:3`) is worse. Its declared shape is `grep '| blocking |' findings.md | grep -vE '| (fix-now|fixed|deferred|accepted|accept-with-note) |'` → must be empty. Executed: it returns **exactly one row, `A3-R008-F1`**, whose disposition cell reads `ACCEPTED-WITH-RATIONALE (v1) 2026-07-20 — tracked as REQ-016` — dispositioned, but with a token absent from the allow-list. **A false positive.** It returns **none of A3-R028c-F1, A3-R028c-F2 or ORACLE-F1**, because their severity cell is `**BLOCKING — <narrative>**`, not the bare token ` blocking `. Only **92 of 309** finding rows still use a bare severity token; the declared 6-column schema has drifted to 8–17 columns (276 rows at 8, 21 at 9, 8 at 10, 2 at 12, 1 each at 11 and 17).

**Net: on the current register, CLV Check 5 has zero sensitivity and zero precision.**

---

## 2. State and SSOT contradictions — file:line evidence

| # | Contradiction | Evidence |
|---|---|---|
| S1 | Handoff says **"four modified files"**; tree has **64 dirty entries** (52 modified + 12 untracked) | handoff `:33` vs `git status --porcelain \| wc -l` = 64. Only 4 are REQ-028 code. |
| S2 | Handoff says *"`master` is untouched"* and omits all hidden work — violating this project's own **LSN-066** | `git stash list` = 1 entry; `git worktree list` = 2; `garmin/req027-cleangate-record` @ `6650a0288` holds a commit not reachable from master. `WIKI.md:63` states LSN-066 verbatim: *"check `git stash list`/`git worktree list`/`git branch --no-merged` at every inventory."* |
| S3 | `WIKI.md` declares per-id status out of scope, then restates it 43 times | `WIKI.md:3` *"per-id status lives in the traceability matrix — DEC-015 SSOT; **do NOT restate it here**"* vs `WIKI.md:57-63` (REQ/DEC/TEST registries carrying ACCEPTED / BUILT / PASS / DONE / ALLOCATED). |
| S4 | **traceability.md — the declared sole SSOT — has no row for TEST-119…130** | Highest TEST id in `traceability.md` is `T-118`. There is no `## TEST index` section (`grep '^## '` returns REQ/DEC/DES/Cycles/Validations/artifacts only). TEST-124/125/126/127 are claimed **BUILT + Verification-Gate PASS** in `WIKI.md:60` and `STATE.md:18`, and have **no status in the SSOT at all**. |
| S5 | `traceability.md` header says **"Last updated: 2026-07-13"** | `traceability.md:3`. The wave it is meant to govern ran 2026-08-14 → 2026-08-21. |
| S6 | `STATE.BUDGETS` counters are stale against `WIKI` | `STATE.md:8` `LSN active:57 guards:42 mech:2` vs `WIKI.md:63` `LSN 001–077 active:77 guards:61 mech:3`. |
| S7 | `STATE.COUNTS` stale | `STATE.md:14` `TEST 120 alloc/120 built` vs `WIKI.md:60` `T-001–T-130 … next:garmin-T-131`. |
| S8 | `WIKI.md` states **three different LSN ranges** | `:9` `LSN-001..064`; `:63` `LSN 001–077`; `:70` `LSN-001..015`. Actual max in `lessons.md` = **LSN-077**. Two of three are stale. |
| S9 | ORCH-032 allocated in STATE, absent from findings SSOT, still advertised as free in WIKI | `STATE.md:1110` (`→ ORCH-032`) vs `grep -c ORCH-032 findings.md` = **0** vs `WIKI.md:62` `next:ORCH-032`. |
| S10 | `decisions.md` points at three files that do not exist | `decisions.md:5` cites `references/formats.md` (absent), `decisions-history-archive.md` (absent), and *"The `state.md ## decs` table is the recap source"* — `state.md` was **deleted by DEC-015** (`WIKI.md:71`). |
| S11 | Decision index is absent from its schema-mandated home | `references/state-and-tiers.md:71-80` puts the Decision Index at the **head of `decisions.md`**, with an active/dormant split, ≤20 lines. `decisions.md` head has no index; `grep -c 'Dormant index'` = **0**. The index lives in `traceability.md:56-100` instead (complete for DEC-001…037 — one thing that *is* sound). |
| S12 | `findings.md` self-describes a division of labour that does not exist | `findings.md:3` *"Cycle files hold the why; this file holds the closure."* No cycle file exists for this wave (C6). The register is 367 KB / 309 rows ≈ 1.2 KB per "closure" row. |
| S13 | `WIKI.md` MAP lists a `docs/` file that is not on disk | `WIKI.md:26` names `QGDW_SKILL_RETROSPECTIVE.md`; `ls docs/` does not show it. Three untracked `COACH_*.md` files *are* on disk and *are* listed. |

---

## 3. Chronological decision audit (REQ-028 wave)

| Date | Decision | Original premise | Selected / rejected | Later evidence | Verdict now | Residual vs acceptance |
|---|---|---|---|---|---|---|
| 08-16 | **DEC-034** — list-generation counter (`decisions.md:1160`) | Whole-list rebuild is the only way a row dies (`:1218-1220`) | **C** (listGeneration). Rejected **A** refuse-at-mutator (predicate defeated at `:1915`, untestable, guts TEST-091), **B** key addressing (blast radius) | Premise **holds** — `refreshClicked` remains the sole deleter. But C validates the *container*, and ORACLE-F1 shows a **stack local** copied out before the compare defeats it | **Valid, but insufficient at the element level** — as its own §"what C does NOT do" predicted | S-R028-01 explicitly not closed |
| 08-18 | **DEC-036** — in-flight ticket (`:1235`) | Two invariants declared load-bearing: `readComplete` returns caller's buffer; **≤1 outstanding per live batch** (`:1289-1295`) | **A** ticket. Rejected **B** relay, **C** key addressing | Invariant 1 holds. **Invariant 2 FALSIFIED by seed 32 (executed).** DEC-037 later also disproves the reasoning behind rejecting B (`:1370-1376`) | **MUST FORMALLY REOPEN.** The entry names the consequence itself: *"the single ticket must become a set and B's score rises"* | Clause (c) closure claim inherits the falsification |
| 08-18 | **DEC-036 AMENDMENT** — invalidation half (`:1314`) | Arm/consume was specified, invalidation never enumerated → UAF | Two disarm sites (`:1526`, `:2004`-adjacent) | Sound; TEST-117/118 built; not challenged since | **Valid** | — |
| 08-18 | **Defer sort-route DRIVER half** (`:1284`) | Positional `for (i=listindex; …)` survives; user chose smaller blast radius | Track, don't fold | B-R028-05 **measured the harm executed** (`findings.md:239`): items permuted, wrong row labelled, no counter bumped | **Valid as a deferral, but it is a known live mislabel** — same harm class clause (c) forbids | **Contradicts clause (c)** on the same axis as C3 |
| 08-19 | **DEC-037** — retired-ticket set, writes only (`:1352`) | Wire carries no identity; disproof rests on one shared `store` (`:1067-1072`) and ≤1 outstanding | **A** retired set. Rejected **B** drain (new `tr()`, timers), **C** counter (already rejected) | Disproof of the shared emitter **stands**. But B was scored under the now-falsified ≤1 premise; and A3-R028c-F4 shows the disproof argues from behaviour DEC-037 itself changed | **Needs re-scoring, not just amendment** — B's cost/benefit changed with the premise | **Accepted residual violates clause (c) as written (C3)** |
| 08-19 | **Defer B-R028-04** (wedged dialog) (`:1286`) | It is DEC-032's idiom shared across ~16 services | Queue it | Untouched | **Valid** | User-visible stuck state, no test |
| 08-21 | **DEC-037 AMENDMENT** — row-free tombstone (`:1449`) | Refresh clear was costed for safety, never for what it destroys | Retire the name, drop the pointer | **Unbuilt.** No TEST-128; no fuzzer seed can see it (C1) | **Valid in shape, unverified and unverifiable by the current oracle** | Blocker remains open |
| 08-21 | **DEC-037 CORRECTION** (A3-R028c-F4) (`:1468`) | Entry's disproof cites behaviour it changed | Correct the record; rejection stands on other grounds | Verified: `:3453` writes `result` | **Valid** — but the cite it fixes (`:3212-3213`→`:3465-3466`) is one of many still stale | — |
| 08-21 | **DEC-034/036 AMENDMENT** — F2 (`:1491`) | *"Not a new decision"* | `batchGeneration` snapshot at three completion slots | Its supporting enumeration is wrong (7 sites, not 3); pre-corrected in STATE only | **REOPEN REQUIRED (C2)** | Chooses serialization over three unrecorded architectures |
| 08-21 | **Build the oracle/fuzzer BEFORE the fixes** (LSN-077) | An oracle nobody has seen fail is not evidence | Build first, must rediscover known defects | **Vindicated**: rediscovered F2 mechanically and found ORACLE-F1 | **Valid — the best decision in the wave** | Its acceptance criterion is now over-trusted (C1) |
| 08-21 | **Fold ten upload-capable services into REQ-028's blast radius** (`:1479-1487`) | Mechanism ships for 10 services, exercised against 0; GarminConnect is `Query\|Download` and cannot upload | Record as KNOWN GAP by user decision | Confirmed at `GarminConnect.h:76`; A3-R028-F6 says zero tests construct `LocalFileStore` | **Valid as disclosure, but scope-inconsistent** — REQ-028 is a row-lifetime requirement now carrying a 10-service write-correlation protocol with zero service-level evidence | Not covered by any clause (a)–(e) |

---

## 4. Mechanism map — defence-in-depth vs redundancy

Two mechanisms are counted redundant **only** if they protect the same property, at the same lifecycle boundary, with no independent failure mode or evidence value. Adjacency alone is not redundancy.

### Essential / orthogonal

| Mechanism | Property | Lifecycle boundary | Independent failure mode |
|---|---|---|---|
| **QPointer / self checks** (DEC-025/030) | *The receiver still exists* | Resumption after any nested loop | Dialog destroyed mid-suspension |
| **BlockingCall / store reaper** (DEC-024/031) | *The collaborator outlives frames referencing it* | Frame-counted deferred deletion | Athlete-tab close during transfer |
| **`aborted` / user intent** (DEC-032/035) | *The user still wants this work* | Immediately above the irreversible call | LSN-054: alive ≠ wanted. Not substitutable by QPointer |
| **`listGeneration` / `batchListGeneration`** (DEC-034) | *The row collection was not rebuilt* | Before every row deref and before `saveRide` | A Refresh bumps list-gen but not batch-gen |
| **`batchGeneration`** (DEC-032) | *This frame's batch is still the live batch* | Driver entry + post-suspension | A restart bumps batch-gen but only **re-stamps** list-gen |
| **in-flight ticket** (DEC-036) | *This completion belongs to the transfer we dispatched* | Completion-slot entry | Identity, not epoch: catches a completion the generations read as valid |
| **`retiredWrites` / tombstones** (DEC-037) | *An abandoned write is still attributable* | Completion slot, matched before the live compare | Extends the ticket across a batch boundary |

`listGeneration` and `batchGeneration` are **adjacent but not redundant** — verified in code: `refreshClicked` bumps only `listGeneration` (`CloudService.cpp:1526`), while `downloadClicked`'s start branch bumps `batchGeneration` (`:1994`) and merely **re-stamps** `batchListGeneration = listGeneration` (`:2004`). Each is invisible to the other's trigger. This asymmetry is what `WIKI.md:60` relies on for the conditional T-130.

### Overlapping but justified

| Overlap | Why it is not redundancy |
|---|---|
| Ticket **and** generation compares at completion slots | DEC-036 `:1310-1312`: *"A makes DEC-034 MORE load-bearing, not less — the compares are what prove `inflight.row` is not dangling."* The ticket proves *attribution*; the generation proves *the pointer is alive*. Removing either leaves a live defect. |
| `self.isNull()` **and** the `listGeneration` compare below it | B-R028-13's keep-argument (`findings.md:259`) is sound: the compare reads a **member**, so if the dialog died the compare is itself a UAF. Different failure modes. |
| Deterministic slots (TEST-107…125) **and** the oracle (TEST-126) | Different evidence class: the slots pin *named routes* and are mutation-provable; the oracle covers *every run*, including routes nobody enumerated. Seed 32 rediscovering F2 without being seeded is the proof. |
| Three full A3 cycles on one subject | Empirically **not** redundant: A3-R028, A3-R028b and A3-R028c each returned new BLOCKING findings, and each found a defect created or missed by the previous fix (`STATE.md:54-56`). Independent evidence value is measured, not assumed. |
| Two QPA backends | LSN-062 records a green slot failing under the second backend. Measured cost: ~50 s. Justified. |

### Genuinely redundant

| Redundancy | Same property? | Same boundary? | Independent failure mode? |
|---|---|---|---|
| **Per-id status maintained in WIKI + STATE + traceability + findings + handoff + lessons + production comments + test comments** | Yes — "is id X built/passing/blocking" | Yes — every ledger write | **No.** Each copy can only *disagree*; none can detect that it is wrong. S3–S9 are the realized cost. |
| **Orchestrator's Verification-Gate full-target re-run when it *is* the wave-gate run on an unchanged tree** | Yes — "this target's verdict" | Yes — post-build, same tree | **No**, when nothing changed between the two runs. (It *is* independent against a builder's misreport — see §5.) |
| **`retiredWrites` cap eviction as a distinct safety layer** | It claims to bound pathology, but its own comment concedes eviction degrades to the pre-DEC-037 **blocking** defect (`CloudService.h:859-861`) | — | **No.** It protects no property the retire mechanism doesn't already own; it only bounds memory. Its "not anything worse" justification is false (C4). |

### Invalidated or incomplete

| Mechanism | Status |
|---|---|
| **Single-slot in-flight ticket** | **INVALIDATED by measurement** (seed 32, both backends). Its ≤1 premise is false. |
| **`batchGeneration` at completion slots** | **ABSENT.** Verified: no read in `completedRead` (2725–3038), `failedRead` (3038–3112) or `completedWrite` (3318–3494). |
| **`retiredWrites` retire coverage** | **INCOMPLETE.** Only retire site is `CloudService.cpp:2083`; `refreshClicked` clears (`:1579`) without retiring (A3-R028c-F1). Additionally lossy under F2 (C4). |
| **Oracle INV-2** | **UNPROVEN.** `STATE.md:1136-1137`: never fired in 71 slots or 446 seeds. A check that has never fired is not a passing check. |
| **Oracle ticket conservation** | **DROPPED** — honestly, but it is precisely the blind spot in C1. |
| **Oracle scoping via `cancelButton->isHidden()`** | **FRAGILE.** The test comment (`:365-372`) says *"four statements"* and lists **five** line pairs; `STATE.md:1133` says *"all five write sites."* Any future `show()/hide()` silently weakens the oracle — disclosed, unguarded. |
| **CLV Check 5** | **NON-FUNCTIONAL** (C7). |
| **Drift lint** | **Functional within an 8-token scope**; blind to 134 real pairings (C7). |

---

## 5. Process-drift findings

### Documentation duplication

`STATE.md` is **340,034 bytes / 3,173 lines** against a schema of **"Tier 0 cursor, ≤ ~250 tokens"** (`references/state-and-tiers.md:36`) — roughly **340×** over. It is not a cursor; it is a journal: **33** `Prior (superseded):` blocks and **~60** `NEXT_GATE-PRIOR-*` fields, each a full historical narrative.

`WIKI.md` is **35,498 bytes** against `BUDGETS: WIKI <n>/700` tokens (`:49`) — roughly **12×** over — and its own BUDGETS line (`STATE.md:8`) reports *"WIKI ~13.5k chars/cap-ok"*, a figure that is both stale by 22 k characters and measured against a cap the schema does not contain.

`findings.md` retains **all 309 rows ever raised**, of which ~105 carry resolved dispositions, with **no `findings-resolved.md`** and no `archive/` directory, against a schema that specifies both (`:82`, `:107-113`).

### Test-execution duplication

`references/orchestration.md:197-206` assigns one owner per run: builder runs the full suite once; **"orchestrator verification … does NOT re-run the full suite per feature"**; A3 runs mutation tooling only; the full suite re-runs **once per wave/gate**. The handoff mandates the opposite: *"Running the tests — this exact env, both backends, **every time**"* (`:48`) plus *"no agent report is trusted. Re-run its tests yourself"* (`:78`).

Measured cost per full execution: **49.3 s** (offscreen) + **50.0 s** (minimal) ≈ **100 s per pass**. Per dispatch that is builder (2 runs) + orchestrator verification (2) ≈ **200 s**, plus the wave-gate run.

**Important qualifier:** `testGarminConnectSyncDialogClose` is the target the diff touches, so the orchestrator's re-run is **within** the "targeted" allowance. The genuine duplication is narrower than the handoff implies — it is the wave-gate run landing on an unchanged tree already verified, and the reflexive both-backends re-run when only one backend's evidence was ever in question. Against this, the anti-trust discipline has a measured record: this wave repeatedly found agent premises falsified. **It should be scoped, not removed.**

### Source-comment duplication

Added lines in the two production files:

| File | Added | Comment | Blank | **Executable** |
|---|---|---|---|---|
| `src/Cloud/CloudService.cpp` | 776 | **630** | 33 | **113** |
| `src/Cloud/CloudService.h` | 221 | **206** | 3 | **12** |
| **Total** | **997** | **836** | 36 | **125** |

**A 6.7 : 1 comment-to-code ratio.** 184 comment lines cite a TEST/LSN/DEC/finding id or a suite count. Copied test counts are embedded in production: `:1552` *"63/63, EXIT=0"*, `:2077` *"68 passed / 3 failed under QPA offscreen"* — both now stale against the measured **71/1**. The header carries a whole decision transcript, including the falsified invariant (`CloudService.h:783-789`) and a mutation-coverage disclosure (`:862-864`).

### Stale references

Nine cited line numbers were spot-checked against the current tree; **seven are stale**:

| Cite | Claimed | Actually at that line | Real location |
|---|---|---|---|
| `decisions.md:1411` `:2029` | START branch retires the write ticket | a comment fragment | `:2083` |
| `decisions.md:1416` `:3204` | the retired-set scan | `curr->setText(7, tr("Aborted"))` | `:3447` |
| `decisions.md:1363` `:1777` / `:1823` | both lists set `text(1)` | `}` / a `QString` format call | — |
| `decisions.md:1364` `:2393` / `:3053` | the two write arm sites | both are comment lines | — |
| `CloudService.cpp:2874` `:2627` | "above `saveRide`" | a comment fragment | `saveRide` call is `:2966` |
| `CloudService.cpp:2889` `:2187` | "the entry check" | a mutation-record comment | — |
| test file `:401` `CloudService.h:846/:906` | `inflight` / `retiredWrites` decls | a comment line / class closing brace | `retiredWrites` is `:864` |
| `decisions.md:1499` | `batchGeneration` read at 3 sites | 7 sites | — |

Only `:1559` and `:1579` — the newest amendment's cites — were accurate. `LSN-034` is logged at **recur:9** for exactly this failure mode (`decisions.md:1445-1447`), which means the project has diagnosed it nine times and has not changed the practice that produces it.

### Duplicated agent definitions and hooks

Five `qgdw-*.md` files exist twice (`.claude/agents/` and `.claude/skills/…/agents/`), plus `anti_duplication_guard.py` (×2) and `ledger_drift_lint.py` (`scripts/` + `.claude/hooks/`).

**All seven pairs were verified with `cmp`: currently byte-identical.** The sync mechanism is a manual `install_hook.py` invocation documented at `WIKI.md:48`. The duplication is real and the drift risk is real; it has **not** drifted today. This is stated without overstatement.

### Ineffective governance checks

See C7: the drift lint's 8-token scope vs 134 real pairings, and CLV Check 5's zero sensitivity and zero precision. Add that `git diff --check` exits 0 and proves only the absence of whitespace errors.

---

## 6. Minimal remediation plan

Ordered; preserves every mechanism with independent evidence value.

### Immediate — before any further build dispatch

1. **Reconcile the live cursor.** Set `STATE.CURRENT` and `STATE.NEXT_GATE` to one state. Ground truth: nothing is running, TEST-128/129/130 are unbuilt, the tree is at `da455c79e` + 64 dirty entries. *(C5)*
2. **Reopen DEC-036 formally** with three re-scored options — ticket set, drain-before-restart, explicit operation identity — since its own text names the first as the consequence of the falsified premise, and DEC-037's rejection of the second was scored under that premise. The `batchGeneration` serialization may well win; it must win **on the record**. *(C2)*
3. **Correct the clause-(c) claim.** Either amend `prd.md:154` to carve out the same-activity two-row case, or change `decisions.md:1356` from *"clause (c) re-closed"* to *"partially met; residual R1 (result-string swap) and R2 (sort-route driver half) open."* *(C3, and the B-R028-05 row)*
4. **Re-disposition A3-R028c-F3 as blocking**, or record in the row the explicit reachability rule that makes 18 clicks non-blocking where 5 is blocking. Correct `CloudService.h:859-861` — "today's behaviour" is a blocking defect. Add the derived finding: **F2 makes the single-ticket retire lossy independent of the cap.** *(C4)*
5. **Add a ticket-conservation observation seam** so the fuzzer can see A3-R028c-F1. The oracle's stated blocker is that `inflight`/`retiredWrites` are private with no accessor. A test-only const accessor, or `friend class TestGarminConnectSyncDialogClose`, closes it. **Without this, the wave's acceptance criterion does not cover one of its three blockers.** *(C1 — highest-value single change in this list)*
6. **Register ORCH-032** in `findings.md` and propagate the `batchGeneration` site correction from `STATE.md:1110-1113` into `decisions.md:1499-1500`. *(S9)*

### Cheap, high-leverage governance repairs

7. **Fix CLV Check 5** — normalize the severity column to a bare token across all 309 rows (or change the check to match `BLOCKING` case-insensitively anywhere in columns 3–4 and widen the resolved-token list to include `ACCEPTED-WITH-RATIONALE`, `CLOSED`, `informational`). Verify by asserting the check returns exactly `{A3-R028c-F1, A3-R028c-F2, ORACLE-F1}`. *(C7)*
8. **Widen the drift lint's `STATUS_TOKENS`** (`scripts/ledger_drift_lint.py:82-91`) to include `ACCEPTED, BUILT, DONE, COMMITTED, ALLOCATED, PASS, BLOCKING, UNBUILT, OPEN`, then run it and fix what it finds. Expect ~134 hits. **Add the new cases to `scripts/test_ledger_drift_lint.py` (currently 44 cases) first.** Re-sync `.claude/hooks/` via `install_hook.py` after. *(C7)*
9. **Stop citing line numbers.** Cite symbols — `refreshClicked`'s clear, `downloadClicked`'s retire, `completedWrite`'s retired scan. LSN-034 is at recur:9; the ninth recurrence means the practice, not the instance, is the defect.

### Test-execution scoping

10. Keep the orchestrator's distrust-and-re-run — it is load-bearing here and has a measured record. **Scope it:** re-run the target under **one** backend for evidence verification, and reserve the dual-backend run for (a) the wave gate and (b) any change touching event delivery, widget visibility or QPA-sensitive timing — which is what LSN-062 actually taught. Saves ~50 s per verification without losing a class of evidence.
11. Skip the wave-gate full run when the tree hash is unchanged since the verification run. Record the hash.

---

## 7. Proposed compact target structure

**`WIKI.md`** — ≤700 tokens, structure only. MAP (one line per directory), REGISTRIES as **`REQ 001–028 next:029 → prd.md`** with *no status words and no narrative*, PAGES, ORIENTATION. Every parenthetical currently carrying ACCEPTED/BUILT/PASS moves to `traceability.md`. Fix the three LSN ranges to one; fix the `docs/` MAP line (S13). Target: 76 lines → ~35 lines, 35 KB → ~3 KB.

**`STATE.md`** — the schema at `references/state-and-tiers.md:38-51`, verbatim, ~20 lines: `PHASE / OPEN / BLOCKING (ids only) / CASCADE / LAST_CLV / NEXT_GATE / CHANGESET / TEAM / RIGOR / BUDGETS / COUNTS`. **One state, no `Prior` blocks, no `NEXT_GATE-PRIOR-*`.** The 33 superseded blocks and ~60 prior gates move verbatim to `.claude/workflow-garminconnect/journal.md`, id-addressable, never read at Tier 0. Target: 3,173 lines → ~20; 340 KB → ~2 KB.

**Traceability** — restore it as the SSOT it is declared to be. Add the missing **`## TEST index`** with one row per id, `T-001 … T-130`, columns `id | REQ | DEC | built? | green? | backends | commit`. Update the header date. Add a **Trace Digest** head: status counts + non-Deployed rows only.

**Active findings vs archive** — `findings.md` holds **open + blocking only**, one line each, in the declared 6 columns with a bare severity token: today that is 3 blocking + ~12 open non-blocking/informational ≈ 15 rows. The other ~294 rows move to `references/archive/findings-resolved.md`. Restore the per-cycle *why* to `cycles/active/a3-r028{,b,c}.md` so a Verification-Gate PASS has an artifact behind it.

**Decisions index vs cold entries** — put the index at the **head of `decisions.md`** per schema, active/dormant split, ~20 lines: `DEC-036 | completion correlation | chosen:in-flight-ticket | rev:cheap | **needs-review** | deps:8`. Keep the traceability DEC table as a pointer only, or drop it. Delete the three dangling references at `decisions.md:5`.

**Handoffs** — a handoff should carry only what the ledgers *cannot*: the exact env line, the hard rules, the one strategic insight, and **a verifiable tree fingerprint** (`git rev-parse HEAD`, `git status --porcelain | wc -l`, stash count, worktree count, last measured suite totals with backend names). It must not restate status — and it must not say "do not re-derive," which is what let four claims in this one go stale. Suggested rule: **every quantitative claim in a handoff carries the command that produced it.**

**Production comments** — a guard comment should state (i) the property protected, (ii) the boundary, (iii) the *named* test that kills its mutant. Not the decision transcript, not suite counts, not line numbers. The DEC rationale belongs in `decisions.md`; the mutation record belongs in the finding row. Target for this patch: ~836 comment lines → ~180, at roughly 1.5 : 1 rather than 6.7 : 1.

---

## 8. Claims that could not be verified

1. **"436 of 446 seeds pass"** and the nine further INV-1 seeds (77, 128, 244, 271, 312, 336, 395, 402, 410) — `STATE.md:29-31`. Only seeds 32 and 447 were run; a 446-seed sweep was outside the bounded-timeout budget.
2. **Every mutation claim** — "orchestrator-mutation-proven", "M-ORCH-V confirmed", "removing this line alone takes TEST-124/125/115 red — 68 passed / 3 failed". Verifying these requires editing production files, which a read-only audit cannot do.
3. **"Verification Gate PASS"** at every gate in this wave — no artifact exists on disk (C6). Neither that the checks ran nor that they passed can be verified.
4. **"A3 verdict: FINDINGS, N BLOCKING"** for A3-R028, A3-R028b, A3-R028c — no cycle file exists for any of them (C6).
5. **"the oracle costs nothing measurable (−130 ms against a 48,881 ms baseline)"** — totals of 49,301 ms / 50,008 ms were measured, but there is no oracle-disabled baseline to difference against.
6. **`CloudService.cpp` md5 `716406f7…`** (`STATE.md:44`) — the current file can be hashed, but not what it was when the oracle builder finished, so "the builder did not touch production" is unverifiable from here.
7. **"11/11 read services hand back the caller's pointer"**, **"10 of 11 write services pass `replyName(sender())`"**, **"20 `mapReply` call sites"** — static claims across all of `src/Cloud/`; not re-derived.
8. **"the builder's instrumented run for REQ-028 (c)"** that allegedly measured `outstanding <= 1` (`CloudService.h:788-789`) — the run artifact is not in the tree. A3-R028c-F2 asserts its scope was narrower than its claim; the claim can be confirmed false, but not what the run actually covered.
9. **"Ten briefings in this wave had a premise falsified by the agent executing them"** (handoff `:79-80`) — not auditable from repository artifacts.
10. **Whether the sort route (S-R028-01) is reachable by a real user click** — B-R028-05 records the harm as executed and the user-click trigger as not; no attempt was made to drive it.
11. **The relationship between `garmin/req027-cleangate-record` @ `6650a0288` and this wave** — it holds a commit not reachable from master and its worktree is clean, but its content was not audited.

---

## Audit provenance

The audit itself was **read-only**. No file in the repository was created, modified, committed, restored, stashed, checked out, reformatted or deleted while it ran. No rebuild was performed — the existing binary was confirmed newer than every source file. The only writes during the audit were test-output logs under the session scratchpad, outside the project tree. All 64 dirty entries, the stash and both worktrees were left exactly as found.

This file (`docs/REQ028_PROCESS_AUDIT.md`) was written afterwards, on explicit instruction, as a new document. It modifies no audited artifact.

> **Note for the ledger:** adding this file makes `WIKI.md:26`'s `docs/` MAP line stale by one entry. That line was deliberately **not** updated — doing so would have edited an audited artifact. Update it as a byproduct of the next governance write, alongside the `QGDW_SKILL_RETROSPECTIVE.md` correction in S13.
