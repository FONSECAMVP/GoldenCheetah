# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)
BUDGETS: **RE-MEASURED 2026-08-22 (`wc -c`/4) — THE PRIOR "cap-ok / NOT due" VERDICT WAS STALE AND WRONG. WIKI.md
  35,498 chars = ~8,874 tok against the 700-tok cap = 12.7x BREACH.** The prior line recorded ~13.5k chars from the
  2026-08-05 compaction and was never re-measured across the REQ-027/REQ-028 waves; ~13.5k was ALREADY 3.4k tok = 4.8x
  over, so the verdict was wrong when written and is now wrong by an order of magnitude. **librarian Job-3 compaction is
  DUE, not "worth considering"** — see NEXT_GATE for the seam it runs in ([[LSN-078]]).
  WHERE the breach lives: the MAP is fine (~50 lines, structural). **REGISTRIES is ~28k of the 35k** — the REQ/DEC/TEST
  lines have absorbed per-id NARRATIVE the hub is supposed to POINT at (LSN-035 inverted: the hub restating what
  prd.md/decisions.md/traceability.md own). Job-3's WIKI action here is registry rollup, not MAP directory-rollup.
  DECIDX: **there is NO one-line-per-DEC index head in decisions.md at all** — the file opens straight into DEC-001's
  full entry, and its preamble still points at the `state.md ## decs` table DEC-015 deleted. 37 DECs, no active/dormant
  divider. The Tier-1 decision index the skill assumes is UNBUILT, so its 20-line/500-tok cap has never been measurable.
  LSN: active:78 guards:62 mech:3 (the prior 57/42/2 was stale by 21 lessons). lessons.md 190k chars — cold entries,
  index head is the hot read; per-op-tag guard counts not yet computed, so the ~10/op-tag cap is unverified.
  COLD entry files (drilled by id, not hot reads — no cap applies): decisions.md 197k · findings.md 368k/308 rows ·
  traceability.md 119k · STATE.md 341k/3177 lines (STATE is a HOT read but its bulk is superseded CURRENT-PRIOR/
  NEXT_GATE-PRIOR blocks below the live head — an archive candidate at the same seam, not a Job-3 target).
  DEFERRED BY USER DECISION (unchanged): bulk findings/lesson row compaction — 112 rows need explicit classification,
  not automatic deletion. That deferral does NOT cover the WIKI REGISTRIES rollup or the missing decision index.
COUNTS (**RECOMPUTED 2026-08-22 from the source-of-truth files, not copied — commands recorded**): REQ **28** + **11**
  `REQ-NF-*` (`grep -oE "REQ-NF-[A-Za-z0-9]+" prd.md | sort -u | wc -l`) · DEC **37**, max DEC-037, next 038
  (`grep -cE "^#+ *DEC-[0-9]{3}" decisions.md`) · LSN **78**, max LSN-078, next 079 (`grep -oE "LSN-[0-9]{3}" lessons.md
  | sort -u`) — the earlier `LSN 77` was one short · VAL max **VAL-017**, next 018 · TEST allocated to **T-130**;
  **T-119..T-129 are BUILT (in-source markers present, 2..16 each), T-130 is UNBUILT (zero markers)** — the conditional
  allocation for ORACLE-F1 was never needed, which must be recorded as a resolved conditional per the TEST-076/[[LSN-022]]
  precedent, not left dangling · **ASan target declares 73 test functions** (`./testGarminConnectSyncDialogClose
  -functions | wc -l`), which does NOT reconcile with the `78/0` the prior verdict quotes — treat 78 as unsourced.
  **Active residuals corrected 2026-08-22 (user disposition, cross-checked against findings.md rows):** B-R028-15 and
  B-R028-16 CLOSED · B-R028-13 SUPERSEDED by A3-R028c-F7 (which re-diagnoses it as a coverage gap, not a reachability
  gap — so clause (e)'s reachability exemption narrows) · B-R028-14 RATIFIED · A3-R028c-F3 RESOLVED · A3-R028c-F4..F7
  OPEN · **A3-R028c-F8 is RESOLVED (2026-08-22) and was absent from the correction list** — it exists, it is the
  process-audit finding derived from F2, and its row already carries the resolution.
  Stale prior line, retained for provenance: REQ 28(+NF) · DEC 37 (037 ACCEPTED 2026-08-19, build QUEUED behind the F1/F2 builder — same file, and builders parallelise only on DISJOINT files) · DES 14(+2) · TEST 120 alloc/**120 built** (T-119/120 built 2026-08-19, ASan target 63→**65** both backends) (T-113..116 built 2026-08-18, ASan target 57→**61** both backends; TEST-107 rewritten, TEST-111/112 amended, TEST-087 apparatus repaired) (T-098 allocated-unused; T-107 built 2026-08-16, its verdict unchanged by the Phase-2 fix — which is the marker that clause (c) is open; T-108..T-112 built 2026-08-18, ASan target 52→**57**; per-id verdicts → traceability.md) · VAL 17 · LSN 77 · ORCH-findings 31 · S-findings 8 · **B-R028 16 · A3-R028 7 · A3-R028b 8 · A3-R028c 7**   (registries → WIKI.md; per-id status → traceability.md)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **ORCH-033 REPAIRED AND THE CHECKPOINT IS COMMITTED — `e48f7d123` on `garmin/req028-row-lifetime`, 26 files,
harness-only fix + the REQ-028 correlation slice. NOT PUSHED. `master` untouched at `82a0de52a`.**
**Gate evidence, orchestrator-executed and re-collected AFTER pre-commit's clang-format rewrite** (the first commit
attempt was REJECTED by the hook, which reformatted 472 lines of the test file — so the pre-rewrite evidence described a
different file and was discarded, [[LSN-064]]): full ASan target **78 passed / 0 failed / 0 `SUMMARY: AddressSanitizer`,
EXIT=0** under BOTH `QT_QPA_PLATFORM=offscreen` and `=minimal` with the pinned
`ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1`; `ctest -R testGarminConnectSyncDialogClose` **EXIT=0,
2/2 Passed**; `ctest -L garmin-fast` **25/25**, which is what clears the [[LSN-056]] risk that the shared
`stubs/ImportSeamStubs.cpp` breaks the 23 other targets. Committed blob md5 == tested worktree md5, verified.
**THE 73-vs-78 "DISCREPANCY" I RAISED AT THE PREVIOUS GATE WAS MY ERROR, AND IT IS RESOLVED:** `-functions` reports
**73 declared test functions**; QtTest's `Totals:` reports **78**, which additionally counts `initTestCase`,
`cleanupTestCase` and data rows. Both numbers are correct and measure different things — the prior verdict's "78" was
never unsourced. Reporting them separately, as the user required, is what settled it.
**MUTATION (individually attributable, [[LSN-059]]):** removing ONLY the `notifyWriteComplete` self-guard reproduced the
original signature — `heap-use-after-free … :726 in oracle::TransferOracle::checkConservation` — on both backends;
restored from a `cp` snapshot, `cmp` clean, md5 back to `d8055ea3…`. Production `CloudService.{cpp,h}` md5-unchanged
across the whole exercise.
**THE USER'S DIAGNOSIS CORRECTED MINE, AND THE CORRECTION WAS LOAD-BEARING.** I reported the queued lambda as running on
an already-dead store; in fact the store is ALIVE at entry and dies RE-ENTRANTLY inside `CloudService::notifyWriteComplete`
— the base call invokes the completion slot directly, that slot pumps events, and the teardown deletes dialog-then-store.
The crash frame at `:1222` is the statement AFTER the base call, which is the proof. A fix aimed only at my window would
have left the real one open.
**ALSO CORRECTED, a standing residual that is simply no longer true:** `clang-format` DOES exist in this environment —
pre-commit ran it and rewrote the file. The note carried at `LAST_CLV`/[[LSN-012]] that "no `clang-format` binary exists
in this environment" is stale, and the checksum-changes-at-commit-time effect it was used to explain is now DIRECTLY
OBSERVED rather than reasoned.
**Prior gate result, superseded by this repair:** COMMIT-SAFETY GATE 2026-08-22 — VERDICT: FAIL. NOTHING WAS STAGED, NOTHING WAS COMMITTED. The wave's recorded
suite verdict is FALSIFIED BY EXECUTION → ORCH-033 (BLOCKING).** The ASan target aborts at test **32 of 73** on BOTH
backends with a `heap-use-after-free` at `testGarminConnectSyncDialogClose.cpp:712` in the TEST ORACLE itself
(`oracle::TransferOracle::checkConservation`), reading its own freed `this`: the oracle is a member of the store, the
dialog destructor deletes the store (`closeAndDeleteStore`, `CloudService.h:325`), and the queued delivery lambda at
`:5180` holds `dialog`/`store` as RAW pointers. Deterministic — 4/4 isolated runs, 2 per backend, identical signature.
**Commands, exit codes, counts (orchestrator-executed, not read):**
  `QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1 ./testGarminConnectSyncDialogClose`
  → EXIT=1, 31 PASS, 1 `SUMMARY: AddressSanitizer`, 23.6s. Same command with `QT_QPA_PLATFORM=minimal` → EXIT=1, 31 PASS,
  1 ASan summary, 23.0s. `ctest -R testGarminConnectSyncDialogClose` → **EXIT=8, 0 of 2 passed** (both registrations FAIL).
  `ninja testGarminConnectSyncDialogClose` → `no work to do`, EXIT=0 (binary 11:42:52 is NEWER than every source it
  compiles, so this is the built tree, not a stale artifact).
**THE REPORTED `minimal` HANG DID NOT REPRODUCE.** `GC_FUZZ_SEED=32` and `=447`, run in isolation against
`randomClickSequencesMustNotBreakTheTransferInvariants` under the pinned ASAN_OPTIONS, PASS on BOTH backends in
0.22–0.39s (4/4 runs, zero ASan output). What is real is the abort above, which kills the whole-binary run long before
the fuzzer slot is reached — the most likely thing behind the "hang" report is a whole-binary or ctest invocation.
**The count itself does not reconcile: the target declares 73 test functions, not the 78 the prior verdict quotes.**
**Prior verdict, retained as the falsified claim ([[LSN-078]] save #2):** DEC-036 OPTION C IS BUILT AND VERIFIED: explicit write-operation identity, per-dispatch read/write records, row-free Refresh preservation, and post-suspension batch-generation checks. Evidence: full offscreen 78/0, focused offscreen/minimal 7/0 each, seeds 32/447 green on both backends, `garmin-fast` 25/0, GoldenCheetah build green, fresh attack-the-fix with no blockers, and incremental CLV PASS 9/9. TEST-126's final oracle refinement also passes 6/0 on each backend. The sort-route fix remains the following slice, so REQ-028 stays partial/open. The lossless compaction plan is verified; unsafe bulk findings/lesson compaction remains deferred.**

CURRENT-PRIOR-REQ028-ORACLE: **THE INVARIANT ORACLE AND FUZZER ARE BUILT AND THEY WORK — TEST-126/127 INDEPENDENTLY REDISCOVERED A3-R028c-F2
AND FOUND A THIRD USE-AFTER-FREE THAT SIX ADVERSARIAL CYCLES MISSED. Verification-Gate PASS. THE SUITE IS RED BY
DESIGN and stays that way until the fixes land. Nothing is running.**
**THE ACCEPTANCE CRITERION WAS MET WITHOUT BEING HANDED THE ROUTES.** The briefing described the two live defects by
HARM only and forbade seeding the sequence. The fuzzer found:
**(1) `GC_FUZZ_SEED=32` — INV-1 `outstanding<=1` violated**, both backends: *"2 transfers outstanding for the LIVE batch
at once"*. That is A3-R028c-F2, rediscovered mechanically. The builder AUDITED rather than inferred it —
`__sanitizer_print_stack_trace` names the second dispatch's frame as `downloadNext` ← `completedRead:3016`, a completion
slot suspended in its own `processEvents()` re-driving the loop for the LIVE batch. **Orchestrator-reproduced in
isolation: 59ms, deterministic.** Nine further independent INV-1 seeds in 1..446 (77, 128, 244, 271, 312, 336, 395, 402,
410), some of them WRITE pairs.
**(2) `GC_FUZZ_SEED=447` — the ASan oracle fired → ORACLE-F1, and it is a THIRD DEFECT, not a restatement of the other
two.** Orchestrator-diagnosed, because the builder deliberately declined to chase the route and said so: the
`listGeneration` guard IS correctly placed at `:2876`, above the abort branch at `:2891`, **and it passes** — `row` is a
stack local copied from `inflight.row` before `uncompressRide`'s nested loop, so a Refresh frees the row and disarms the
ticket (the amendment working as designed), a restart then re-stamps `batchListGeneration = listGeneration`, and the
compare sees two EQUAL values while the local points at freed memory. **The ticket disarm cannot help: the pointer was
copied out before it fired.** Third instance of one shape — a stale thing handed a freshly-valid generation stamp: the
stale TICKET (A3-R028-F1), the stale RETIRED ENTRY (A3-R028c-F1), and now the stale STACK LOCAL.
**THE NEGATIVE CONTROL IS WHAT MAKES THIS EVIDENCE AND NOT NOISE, and I verified it myself: all 71 pre-existing slots
stay green with the oracle live** — including the three slots (TEST-115/124/125) that park an abandoned write across a
restart ON PURPOSE — **and 436 of 446 seeds pass.** PASS-name lists diffed: not one existing slot changed verdict. Exactly one
function fails, on both backends, and it is the fuzzer.
**FILES: the test file ONLY.** Verified: zero oracle markers in `CloudService.{h,cpp}` or the stubs, and
`CloudService.cpp` still md5 `716406f7…` — the builder was forbidden to touch production and did not.
**RUNTIME: the oracle costs nothing measurable** (−130ms against a 48881ms baseline, noise); the fuzzer stops at seed 32
today, so the suite still runs at ~49s/50s per backend.
**ONE INVARIANT DROPPED HONESTLY RATHER THAN FAKED:** ticket conservation is UNOBSERVABLE — `inflight` and
`retiredWrites` are private, the target declares no friendship, no accessor and no widget reflects them, and this slice
was forbidden to touch production. **Not approximated.** Restart idempotence was RELOCATED (it is an action, not a
passive observation) into the fuzzer's alphabet, ran 757 times, failed 0 — with `listindex` explicitly NOT covered.
**And it tuned the fuzzer once and UNDID IT, leaving the record in the code:** a weighted action table quadrupled INV-1
density and **stopped reaching the use-after-free class entirely** in 4096 seeds. The committed draw is uniform. That is
the single most useful sentence in the report — density is not coverage.

Prior (superseded): **THIRD A3 RE-CLEAR RETURNED 2026-08-21 — VERDICT: FINDINGS, TWO BLOCKING. THE WAVE IS NOT SHIPPABLE, AND THE
NEXT ACTION IS A USER DECISION ON STRATEGY, NOT A DISPATCH. Nothing is running.**
**SIXTH CONSECUTIVE ADVERSARIAL CYCLE ON THIS LEDGER TO FIND A REAL DEFECT BEHIND A GREEN SUITE** — this time behind
71/71 on two backends with zero sanitizer output, in a wave that has now passed five Verification Gates. **Both new
blocking findings are rooted in decisions I WROTE, not in builder error.**
**A3-R028c-F1 (BLOCKING) — Abort → REFRESH → restart re-opens A3-R028b-F3 IN FULL.** `refreshClicked` disarms
(`:1559`) and clears (`:1579`) but **never retires**; the only retire site is `downloadClicked`'s START branch
(`:2081-2084`), orchestrator-confirmed. So DEC-037's scan has nothing to match and the stale completion is taken for the
live one. Adversary-executed on TWO routes, including a **single-tab five-click route needing no name coincidence at
all**: the abandoned batch's verdict lands on the LIVE row, the bar advances for work never done, the loop is re-driven,
and the live upload's own completion is then swallowed. **DEC-037 justified that clear as "MANDATORY" on MEMORY-SAFETY
grounds only — and it is right, TEST-125 proves it. Nobody asked what the clear COSTS.** The row must die; the NAME must
not. → [[LSN-075]].
**A3-R028c-F2 (BLOCKING) — TWO TRANSFERS, ONE TICKET, and it falsifies an invariant THIS LEDGER RECORDED AS MEASURED.**
`completedWrite` consumes the ticket, THEN `processEvents()`, THEN re-drives; a restart delivered into that suspension
arms a fresh ticket and the tail re-drives again. The `aborted` guard below cannot catch it — the restart just reset the
flag — and **the three completion slots hold no `batchGeneration` snapshot** (read at only `:962`/`:1994`/`:2517`).
Executed: two rows reading "Uploading" simultaneously. **`CloudService.h:783-789` claims "at most one transfer is
outstanding per LIVE batch … Measured, not assumed" — the measurement was 61 slots that never drove a burst into a
completion tail. [[LSN-074]]'s corollary about stating a measurement's SCOPE was captured in THIS WAVE and is exactly
what just failed.** The drivers were given this guard for this two-click burst (`:2512-2517`); the slots were not.
**The same shape sits verbatim on the READ path (`:2986-3016`) and `failedRead` (`:3103`) — which ARE on the
GarminConnect route.**
**FIVE MORE, non-blocking:** F3 the cap's uncovered eviction branch, whose recorded consequence UNDERSTATES the harm
("degrades to exactly today's behaviour" IS the blocking harm F1 measures) · F4 DEC-037's disproof argues from a
behaviour DEC-037 itself changed → [[LSN-076]] · F5 the retired row's lifetime proof is an unstated COUPLING (correct
today by exhaustion over four mutators; nothing writes it down) · F6 `saveRide`'s suspension census misses
`autoProcess("Save","ADD")` and `addRide()` · F7 the two untraced `saveRide` guards are a COVERAGE gap, not a
reachability one — **drivable through the seam this wave already built**.
**NINE REFUTATIONS, and they closed FOUR of my own open findings.** B-R028-15 refuted as a code defect — writing
`result` is **strictly MORE compliant** with clause (c) than "Aborted", which would name the user's action on a
different dead batch; the defect is in the RECORD (→F4). B-R028-16 refuted — TEST-115's flip is a measurement, not an
argument, with three order-independent invariants still asserted beside it. B-R028-13 superseded — the keep-argument and
the ORDERING are both correct and the guards are drivable (→F7). **B-R028-14 RATIFIED — the shipped DEC-035 trade is
right, and the comment UNDERSELLS it: by `addRide()` the activity is already REGISTERED WITH THE ATHLETE, so the only
thing lost on the stale path is the status cell.** Also refuted: that `retiredrow` can dangle on any reachable route
(exhaustion over all four mutators), that the `:3336` return can strand a retired ticket, the empty-id allowance, and
the writes-only scope. Tautology scan clean.

Prior (superseded): **ALL THREE BLOCKING RE-CLEAR FINDINGS ARE NOW CLOSED AND ORCHESTRATOR-VERIFIED; the last of them was closed
by the retired-ticket build (DEC-037) on 2026-08-20, Verification-Gate PASS on all three checks. A THIRD A3 RE-CLEAR IS IN FLIGHT; nothing else is running.**
**EVIDENCE — orchestrator-EXECUTED:** **71/71 under BOTH backends, EXIT=0, zero `SUMMARY: AddressSanitizer`** (69 + 2
new) · **ctest 27/27** · FILES matched `git status` · no slot dropped (PASS-name lists diffed: all 69 prior names
present, exactly 2 new).
**M-ORCH-V — the line DEC-037 called MANDATORY is mandatory, and I proved it rather than trusting the entry:** removing
`retiredWrites.clear()` from `refreshClicked` ALONE manufactures exactly the use-after-free the decision warned of —
`heap-use-after-free … QTreeWidgetItem::setText … completedWrite CloudService.cpp:3453`, freed at
`refreshClicked:1607` — caught by TEST-125 on both backends. Restored `cmp`-clean (md5 `716406f7…`), 71/71 again.
**THE BRIEFED PHASE-1 DEPENDENCY DID NOT EXIST, and the builder proved it instead of building it.** I briefed a
completion-ORDER hook as "the single biggest cost" of this option. The harness already owns the timing
(`completeWrite=false` makes the test body the sole emitter), and because the two transfers' ids are BYTE-IDENTICAL,
"which arrived first" IS "which result string was emitted first". **So DEC-037's exhaustion argument over both arrival
orders is MEASURED, not reasoned** — the clause (e) label the entry reserved is not needed. Tenth briefing premise this
ledger's hatch has caught, and the fourth of mine about the EVIDENCE apparatus rather than the code.
**Mutation, measured not predicted:** sites 2 and 4 each kill three slots (T-124, T-125, TEST-115); site 3 kills by
process death under ASan on both backends; **the empty-id allowance SURVIVES and is labelled "reasoned, not traced";
the cap's `removeFirst` eviction branch is executed by NO test and its policy is written down rather than claimed.**
**TWO DISCLOSURES → B-R028-15 and B-R028-16, both self-reported.** **(15) The retired path ignores `aborted`** — an
abandoned write arriving with `aborted == true` used to write "Aborted" to its row through the live ticket and now
writes the service's string instead; no test covers the difference, and **DEC-037's own disproof cited that "Aborted"
label as a reason not to adopt disarm-on-abort, so the entry argues from a behaviour this build may have changed.**
**(16) TEST-115 changed in BOTH halves, not the one I briefed** — the different-row half flipped from "the stale write
is SWALLOWED" to "it is ROUTED TO ITS OWN ROW". The builder re-argued it in place and named it as the thing to attack,
conceding its own justification is an argument rather than a measurement.

Prior (superseded): **TWO OF THE THREE BLOCKING RE-CLEAR FINDINGS ARE CLOSED AND VERIFIED 2026-08-19 (A3-R028b-F1 and F2);
DEC-037's build is IN FLIGHT on the third. Nothing else is running.** Verification Gate PASS on all three checks.
**EVIDENCE — orchestrator-EXECUTED across ALL THREE targets that compile the shared stub:**
`testGarminConnectSyncDialogClose` **69/69**, `testGarminConnectImport` **4/4**, `testGarminConnectReadFailedConsumer`
**4/4** — six runs, both backends, every EXIT=0, **zero `SUMMARY: AddressSanitizer` across all six** · **ctest 27/27** ·
FILES matched `git status` exactly (4 files) · **slot accounting exact: 65 − 1 + 5 = 69**, the one dropped name being the
old combined TEST-120 slot, correctly replaced by its two split halves (the A3-R028b-F5 fix).
**[[LSN-056]] DISCHARGED PROPERLY:** the builder was told the stub file compiles into three targets and was told to COUNT
rather than trust me — it did (`:1093/:1234/:1356`), armed the seam through a `std::function` that is **null by default
and self-clears after firing**, and built and ran all three. The other two are unchanged.
**M-ORCH-W:** removing the post-`saveRide` `listGeneration` compare ALONE reproduces `heap-use-after-free …
QTreeWidgetItem::setText … completedRead CloudService.cpp:2921`, freed at `refreshClicked:1587`, EXIT=1. Restored
`cmp`-clean (md5 `e1c82a38…`), snapshot deleted, 69/69 again.
**TWO DISCLOSURES WORTH MORE THAN THE FIX, both against the builder's own interest.**
**(1) → B-R028-13: two of the three new guards SURVIVE individual mutation** — the `BlockingCall` around `saveRide` and
the `self.isNull()` bail after it, 69/0 both backends — **and shipped labelled "reasoned, not traced" instead of as
coverage.** The keep-argument is sound and must not be lost: `self.isNull()` is NOT redundant with the compare below it,
because that compare reads `listGeneration`, a MEMBER — if the dialog died inside `saveRide`, the compare would itself
be a read of freed memory. Order matters and the suite does not prove it. Deleting them on the strength of surviving
mutants would remove a real protection ([[LSN-063]]).
**(2) → B-R028-14: DEC-035's ordering principle CANNOT be honoured at `saveRide`, and the builder refused to settle it
alone.** DEC-035 puts the guard above the irreversible act; here the suspension is INSIDE it — the activity's `.json` is
already on the athlete's disk before any guard can run. It guarded what remains (the label, `successful`, the loop),
stated what is given up, and **declined the alternative — skipping `saveRide` entirely on a stale list, which SILENTLY
DISCARDS a downloaded activity — because that is a decision, not a builder's judgement.** Same instinct that produced
DEC-036 out of B-R028-03. **It needs explicit ratification rather than living in a comment.**
**A3-R028b-F5 and F8 also closed:** TEST-120 split into two slots (so the upload arm's coverage no longer depends on the
sync arm passing — demonstrated, since the two new driver guards each kill exactly one slot and neither kills the
other's), and the false closed-world "five delivery points" claim restated in the test file AND in the production
comments. **The replacement residual list is honestly incomplete BY DESIGN** — it names its own gaps rather than
claiming an audit nobody performed.

Prior (superseded): **A3 RE-CLEAR RETURNED 2026-08-19 — VERDICT: FINDINGS, THREE BLOCKING. THE WAVE IS NOT SHIPPABLE, AND THE NEXT
ACTION IS A USER DECISION ON SCOPE, NOT A DISPATCH. Nothing is running.**
**Fifth consecutive A3 on this ledger to find a real defect behind a green suite — this time behind 65/65 on two
backends with zero sanitizer output, in code that had already passed three Verification Gates with every guard
individually mutation-proven.** All three blocking findings were confirmed BY THE ORCHESTRATOR at their cited lines.
**F1 (BLOCKING) — `saveRide()` is a suspension point nobody counted, and THE HARNESS'S OWN STUBS ARE WHY THE SUITE IS
GREEN.** It sits at `:2811`, between `completedRead`'s last row-liveness compare (`:2786`) and its row writes
(`:2812`/`:2815`); the first lifetime re-check is `:2830`, three writes too late. In production it reaches
`autoProcess` (`:3267`) → `DataProcessor.cpp:221` → `FixElevation.cpp:295-300`, an UNTIMED `QEventLoop` on an HTTPS
round trip. In the target, `autoProcess`/`setLinkedDefaults`/`addRide`/`RideCache::save` are all no-op stubs
(`ImportSeamStubs.cpp:423/:400/:205/:394`), so `saveRide` is straight-line and non-pumping. **The `.gcblock` fixture DOES
drive the ride-bearing branch — the coverage is real, and it covers a version of `saveRide` with the hazard surgically
removed.** Gated on `configKeyAutomation == "Auto"` (default "Manual"), so it is a SUPPORTED-CONFIGURATION defect;
the A3 said so rather than inflating it, and clause (a) admits no configuration exemption. → [[LSN-072]].
**F2 (BLOCKING) — the F5 fix guards the four ENTRIES and misses the two RESUMPTIONS.** The parse-failure `continue`
resumes across its own `processEvents()` (`:2428`, `:3078`) reading `self`, `aborted` and `batchGeneration` but **never
`listGeneration`** — while the frame's own local `listgen` exists at `:2167` and is used one branch away at `:2356`. The
loop then walks the rebuilt list to the tail and calls `rideCache->save()` at `:2493` for a destroyed batch: PROBE-B's
harm, which was BLOCKING when first found. Not memory-unsafe (rows are re-fetched) — the A3 refuted its own UAF framing
on severity discipline. **One line each, reusing an existing local; NOT a DEC problem.** → [[LSN-073]]: an ENTRY guard
answers "was it replaced before I was CALLED", a resumption needs "while I was SUSPENDED".
**F3 (BLOCKING) — DEC-036's write ticket keys on `remotename`, and TWO LISTS PRODUCE THE SAME KEY.** Both the Upload
list's row and the Sync list's upload row carry `text(1) == ride->fileName` (`:1777`, `:1823`), and both arm sites
compute the key identically (`:2393`, `:3053`); the compare at `:3204-3205` has nothing that discriminates the list. Four
clicks, NO Refresh: Sync→Synchronize→Abort→Upload tab→Upload, and the abandoned sync write's completion is ACCEPTED
against the live batch's row — mislabels it, counts work never done, and re-drives `uploadNext` while row 0's real write
is still outstanding, **breaking the "at most one transfer outstanding per live batch" invariant DEC-036 rests on**
(`CloudService.h:769-775`). **That invariant HAD been discharged by measurement — 0 overlaps across 61 slots with a
working positive control — but every slot ran within ONE TAB. The measurement was sound and narrower than the claim it
supported.** → [[LSN-074]]. Closing it is a **DEC-036 amendment** (a per-dispatch sequence number, or list identity in
the key) — a mechanism change, therefore the user's call.
**FIVE MORE, non-blocking:** F4 the `rideCache->save()` proxy is sound today and pinned by nothing (a third `save()`
added anywhere not below a tail sentence is invisible to TEST-119/120) · F5 TEST-120's UPLOAD arm is gated behind the
SYNC arm's success, so a sync failure silently hides the third driver's coverage · F6 the entry guards' `true` contradicts
the header contract (no live defect — the A3 REFUTED my attack suggestion and downgraded it honestly) · F7 `refreshClicked`
is not re-entrancy-guarded, so a Refresh inside a Refresh duplicates every row · F8 the test file's "THE FIVE DELIVERY
POINTS" is a closed-world claim that three consecutive A3s have each falsified by one more entry.
**EIGHT REFUTATIONS, and two of them killed MY OWN briefing's attack suggestions** — including the one that mattered
most: "every fixture row is `.gcfail`, so the ride-bearing branch never runs" is FALSE, and the truth makes things
worse, not better. It also refuted, with a call-site trace, that the ticket can ever hold a freed `inflight.row` at any
of the three consumption sites — **the amendment is correct, and F1's hole is strictly downstream of the copy into the
local.**

Prior (superseded): **EVERY A3-R028 FINDING IS NOW CLOSED, AND THE WAVE IS BACK UNDER A FRESH ADVERSARY. `qgdw-adversary` IS IN
FLIGHT on the A3 RE-CLEAR; nothing else is running.** A3-R028-F5 closed 2026-08-19 — Verification-Gate PASS on all three
checks. Uncommitted on `garmin/req028-row-lifetime`, still 1 ahead of `master`.
**THE FIX, and the builder's placement argument is the part worth keeping:** three ENTRY guards, one per driver
(`syncNext:2146`, `downloadNext:2510`, `uploadNext:2965`), using **DEC-034's existing member and idiom** — no new member,
no new concept, no signature change. It chose the driver entry over the three tails **by DEC-035's principle**: the
irreversible call, the tail's `rideCache->save()`, is INSIDE the driver, so guarding at the tails would be the "different
function, across a return and a call" arrangement DEC-035 exists to end — and the entry guard also covers the loop's
DISPATCH, not just its tail, and covers all four callers rather than the three that exist today.
**EVIDENCE — orchestrator-EXECUTED:** **65/65 under BOTH `offscreen` and `minimal`, EXIT=0, zero `SUMMARY:
AddressSanitizer`** (63 + 2 new) · **ctest 27/27** · FILES matched `git status` · **no slot dropped — PASS-name lists
diffed, all 63 prior names present, exactly 2 new** · **M-ORCH-Z**: removing `downloadNext`'s guard ALONE → 64/1,
TEST-119 dead on its own criterion (`progressText.isEmpty()` FALSE), TEST-120 still green — individually attributable,
matching the builder's three-mutant matrix (each guard kills exactly one arm, confirmed under both backends, **nothing
survived**). Restored `cmp`-clean (md5 `be37a044…`), snapshot deleted, 65/65 again.
**MY BRIEFING WAS WRONG A THIRD TIME, AND IN THE SAME PLACE EACH TIME → ORCH-031, [[LSN-071]].** It told the builder to
"reuse the suite's existing `rideCache->save()` counter". **There is no counter** — orchestrator-verified: every hit in
the test file is PROSE, and the stub at `stubs/ImportSeamStubs.cpp:394` is a non-virtual no-op shared by three targets.
TEST-109 asserts clause (b) through a labelled PROXY (the tail's "…successfully" sentence, sound because `save()` is
called at exactly two unconditional sites one line below it). The builder reused THAT and labelled it, rather than
stopping or fabricating a count. **Three falsified premises in three consecutive dispatches — ORCH-028 (stripped the
target's pinned sanitizer config), ORCH-030 (predicted the mutation matrix), ORCH-031 (imagined an apparatus) — and all
three are about the EVIDENCE APPARATUS, not the code. The code claims were made from the file; the evidence claims were
made from memory of the ledger.** The check is one grep per named apparatus.
**RESIDUALS → B-R028-12, and (i) is the one to schedule:** the zero-`save()` claim is a PROXY, not a count, and holds
only while `save()` stays unconditional and adjacent to that sentence; (i) **the F5 delivery window has an UNDRIVEN
neighbour** — every fixture row is `.gcfail`, so `completedRead`'s ride-bearing branch never runs, and `saveRide()` sits
in exactly the gap the fixture needs event-free, so if it pumps events a real user's parseable download lands the Refresh
in a frame no test measures; (ii) "inside the tail's `processEvents`" is BOUNDED by two premises, not proven; (iii)
`failedRead`'s tail is covered by construction but walked by no run.
**B-R028-04 IS NOW PINNED BY TESTS** — TEST-119/120's second verdict asserts `buttonTextAtEnd == "Abort"` with
`downloading` true, so whoever fixes the wedge must change those two slots. The builder flagged that rather than leaving
a later wave to discover it.

Prior (superseded): **THE A3'S BLOCKING FINDINGS ARE CLOSED AND VERIFIED 2026-08-19 — the invalidation amendment is BUILT and
Verification-Gate PASS on all three checks. One A3 finding remains open (F5) and `qgdw-builder` is IN FLIGHT on it.**
Uncommitted on `garmin/req028-row-lifetime` (still 1 ahead of `master`). Two files this run: `src/Cloud/CloudService.cpp`
and the test file; `CloudService.h` is unchanged from the prior wave (+149, verified untouched). No API surface change.
**EVIDENCE — orchestrator-EXECUTED:** rebuild exit 0 · **63/63 under BOTH `offscreen` and `minimal`, EXIT=0, zero
`SUMMARY: AddressSanitizer`** (61 baseline + 2 new) · full **ctest 27/27** · FILES matched `git status` · **no slot
silently dropped by the builder's `private:` insertion — verified by DIFFING THE PASS-NAME LISTS: all 61 prior names
present, exactly 2 new** (the builder flagged that insertion for re-checking in its own notes, which is why I checked it
by name rather than by count).
**MY TWO MUTATIONS, and the first one CORRECTED MY OWN BRIEFING → ORCH-030:** **M-ORCH-X** — remove invalidation site 1
(`downloadClicked`'s START branch) ALONE → **TEST-118 dies on its own criterion** (`'out.barAfter <= downloadtotal'
returned FALSE — the bar advanced to 1 for a batch whose total is 0`) and **TEST-117 PASSES**, 62/1. My briefing predicted
both would die; TEST-117's route contains the Refresh, so site 2 has already voided the ticket before the restart. It is a
test of the PAIR. **M-ORCH-Y** — remove BOTH sites → **the use-after-free reproduces**
(`heap-use-after-free … QTreeWidgetItem::setText`, freed at `refreshClicked CloudService.cpp:1575`), which is what proves
the pair load-bearing against the blocking defect. Restored `cmp`-clean (md5 `ee505f93…`), snapshot deleted, 63/63 again.
**THE BUILDER REPORTED THE MISMATCH INSTEAD OF RESHAPING TEST-117 TO SATISFY MY PREDICTION** — the briefing had fixed that
geometry as "exactly the four clicks", so bending it would have been the easy and wrong move. Seventh premise this
ledger's hatch has caught, and the first about EVIDENCE rather than code. **Site 2 alone kills nothing and is labelled
"reasoned, not traced" in the source** rather than claiming coverage ([[LSN-063]]).
**A3-R028-F3 closed by WIDENING with an honest rider:** TEST-087's fixture gained a second upload row and asserts
`syncListCount >= 2` as an explicit premise — but the builder MEASURED that the widening buys no mutation-killer (removing
`completedWrite`'s `self.isNull()` bail aborts on the REQ-027 `aborted` re-read first), and wrote that measurement into the
slot instead of glossing it. **F4 closed:** guard kept, the untermin­ated "traced" claim withdrawn and relabelled.
**B-R028-07 closed** as a side effect of site 1.
**F5 IS STILL OPEN AND THE BUILDER REFUTED THE GUESS THAT THIS AMENDMENT CLOSES IT** — the harm is not ticket-mediated at
all: all three slots consume the ticket far ABOVE their tail `processEvents()`, so the disarm is a no-op there, and the
tail's re-drive is UNCONDITIONAL (`if (sync) syncNext(); else downloadNext();`) with `downloadNext` carrying no
`listGeneration` guard of any kind. A Refresh delivered in that tail still walks the rebuilt list and calls
`rideCache->save()` for a dead batch — which is REQ-028 clause (b) verbatim, and the same family as A3-R027-F3/PROBE-B
that was BLOCKING when found. **T-119/T-120 allocated; dispatched 2026-08-19.**
**Residuals the builder volunteered → B-R028-11:** TEST-117's assertion is STRUCTURAL (process death under the pinned
`halt_on_error=1`), so without those `ASAN_OPTIONS` it degrades to its premises only; its REDs were **offscreen-only**;
and "four ordinary clicks" is an ARGUMENT, not a measurement — the geometry calls slots directly, the file's own idiom,
so nobody has proved a user reaches the abort branch through the real button (that is B-R028-04's territory).

Prior (superseded): **A3-R028 RAN 2026-08-18 — VERDICT: FINDINGS, ONE BLOCKING, AND IT IS A DEFECT THIS WAVE'S OWN FIX CREATED.
THE SLICE IS NOT SHIPPABLE AS IT STANDS. `qgdw-builder` DISPATCHED on the DEC-036 amendment; nothing else is running.**
**A3-R028-F1 (BLOCKING) — a LIVE heap-use-after-free reachable in four ordinary clicks on the Download tab:**
Download → Refresh → Abort → Download. The in-flight ticket is armed at four sites, consumed at three, and
**invalidated NOWHERE**, while `batchListGeneration` IS re-synced at every batch start (`:1951`) — so a restart that
**dispatches nothing** hands the STALE ticket a freshly-valid generation stamp, and the abandoned batch's completion
then passes the DEC-034 compare, the ticket compare, and the abort read before writing through a row `refreshClicked`
already freed. Adversary-executed: `heap-use-after-free READ of size 8 … QTreeWidgetItem::setText … completedRead()
CloudService.cpp:2654`, freed at `:1542`, EXIT=141. **Orchestrator-confirmed STATICALLY at every cited line rather than
accepted from the report:** `inflight.armed = true` at :2094/:2238/:2414/:2878, `= false` at ONLY :2544/:2748/:3015 —
no invalidation in `refreshClicked`, the abort branch, or the START branch; `:1951` re-syncs the stamp; `:2654` is
`row->setText(col, …)` with `row = inflight.row`.
**THE PART THAT MATTERS MOST: OUR OWN UNCOMMITTED FIX RAISED THIS ROUTE'S SEVERITY.** Pre-DEC-036 it cost a MISLABEL
through `child(listindex-1)`, which re-reads the rebuilt list and is therefore never dangling; post-DEC-036 it is a
USE-AFTER-FREE through a held pointer. DEC-036's own comment at `:2596-2607` names that trade. **Replacing positional
addressing with a stored pointer converts every unclosed lifetime route from wrong-data into memory-unsafe** →
**[[LSN-068]]**.
**A3-R028-F2 (BLOCKING) — the same root cause with NO Refresh**, so nothing is freed: after a restart that arms
nothing the abandoned completion is ACCEPTED, relabels row 0 and advances the bar to 1 with `downloadtotal` 0, *after*
the label already read "Downloaded 0 of 0 successfully". Clause (c) violated verbatim on a route the new slots do not
drive.
**WHY 61/61 UNDER TWO BACKENDS WITH ZERO SANITIZER OUTPUT DID NOT SEE IT, and this is the reusable part:** every
abort/restart fixture in the file **restarts onto a CHECKED row**, so the restart always overwrites the ticket. The
defect needs a restart that arms NOTHING. **Every guard was individually mutation-proven and every mutant died — the
defect was in a site that does not exist**, which is precisely what mutation cannot measure.
**FOUR MORE FINDINGS, none blocking:** **F3** TEST-087's repaired premise is VACUOUS (`writeFileCalls == 1` over a
ONE-row fixture — it holds whether or not the slot stands down; this is exactly the question B-R028-09 asked the A3 to
judge, and the answer was "the weaker reading") → [[LSN-069]] · **F4** DEC-036's claim that `completedWrite`'s `isWrite`
clause is TRACED does not terminate — `replyName()` never returns `""` for a write reply (every `writeFile` calls
`mapReply` first, 20 sites) and the only empty-id emitter, `LocalFileStore`, emits SYNCHRONOUSLY; keep the guard,
correct the comment → [[LSN-070]] · **F5** the completion slots' TAIL `processEvents()` can deliver a Refresh with no
re-check before the loop is re-driven (reasoned, not executed — the A3 labelled it so itself) · **F6** NO test anywhere
drives `LocalFileStore`, the service whose synchronous-completion geometry DEC-036 explicitly rests on · **F7** signed-int
`++` is UB at INT_MAX, not the "wraparound" the comment claims.
**THE A3 ALSO REFUTED FOUR THINGS, which is why its blocking findings are credible:** the B-R028-09 repair fabricates
nothing any more (full sweep of every completion emission in the suite — no fabricated completion remains anywhere);
the write-side buffer/id audit holds (20 `mapReply` sites); all seven `child(listindex-1)` dereferences really are gone;
and the read-path `armed` disarms are correctly classified "reasoned, not traced". Tautology scan clean.

Prior (superseded): **DEC-036 BUILT + VERIFICATION-GATE PASS (all three checks) 2026-08-18 — REQ-028 ACCEPTANCE CLAUSE (c) IS
CLOSED, AND THE PART THAT WAS "NOT DELIVERABLE" A DAY AGO NOW HAS AN EXECUTED TEST ASSERTING THE DESIRED NUMBERS.**
Uncommitted on `garmin/req028-row-lifetime` (still 1 ahead of `master`, HEAD `da455c79e`). Three files, all declared:
`src/Cloud/CloudService.{h,cpp}` and `unittests/…/testGarminConnectSyncDialogClose.cpp` (+2556/-194 cumulative for the
whole REQ-028 wave). **No public API change** — `struct InFlight` and the `inflight` member are private; no signature,
no service, no `connect`, no new `tr()` string.
**EVIDENCE CHECK — PASS, every number orchestrator-EXECUTED:** rebuild exit 0 · ASan target run DIRECTLY under **BOTH
`offscreen` and `minimal` with the target's PINNED `ASAN_OPTIONS`** — **61/61 each, EXIT=0, zero `SUMMARY:
AddressSanitizer` lines** (57 + 4 new, the four confirmed BY NAME) · full **ctest 27/27** · FILES claim matched
`git status` exactly (every other dirty entry predates the run; governance-file mtimes predate it too) · **all seven
`child(listindex-1)` sites verified GONE**, only two prose mentions left · the three driver loops verified STILL
positional, which is the scope line holding.
**MY OWN TWO MUTATIONS, individual and separately attributable ([[LSN-059]]):** **M-ORCH-1** — neuter `completedRead`'s
token compare ALONE → **exactly ONE slot dies**, TEST-113, on the CRITERION's own assertion (`readFileCallsAfter 4 != 3`,
60 passed/1 failed). **M-ORCH-2** — drop arm site 4/4 (`uploadNext` write) ALONE → TEST-114 dies on **its** criterion
(`writeFileCalls 1 != 2`), which independently confirms the builder's own repair of a weak-kill it had disclosed
(ARM4 had first died only on other slots' PREMISE assertions, so it ADDED a healthy-upload run to get a criterion kill).
Restored `cmp`-clean (md5 `5474ac95…`), snapshot deleted, 61/61 again.
**GOAL AUDIT — PASS.** TEST-113 encodes clause (c) **verbatim**, both halves quoted in-slot, and guards it with premise
assertions (`readFileCallsBefore == 3`; the two batches on DIFFERENT rows) so it cannot pass vacuously. TEST-107's
defect-pinning numbers are GONE — it was **rewritten, not retired**, and the builder's reason is right: its P1 half is a
fact about Qt that no fix here can change, while its P2 half now lives in TEST-113 asserting the DESIRED values.
**BOTH INVARIANTS DEC-036 REFUSED TO ASSUME WERE DISCHARGED BY MEASUREMENT, NOT ARGUMENT.** The
`readComplete`-returns-the-caller's-buffer contract was audited across all 11 implementations and is now WRITTEN DOWN at
`CloudService.h:145` beside the `readFailed` one. ≤1-outstanding was INSTRUMENTED across all 61 slots — 0 overlaps under
both backends — **with a positive control proving the probe was reached** (dropping the batch-generation clause makes it
fire exactly 4 times, the four abort+restart runs). A single ticket is sufficient; no set needed.
**FOUR RESIDUALS, ALL SELF-DISCLOSED, NONE BLOCKING — and B-R028-07 is the one that matters:** the ticket is armed at
four dispatches and consumed at three slots but is **never DISARMED on abort**, so a stale completion delivered inside
`openRideFile`'s nested loop (before the restart re-arms) still matches and re-drives the loop. It labels the CORRECT
row, so (c)'s labelling half holds, but it is a second driver. **The fix is plausibly one line at a FIFTH site DEC-036
does not name — governance, not improvisation, and the builder refusing to invent it is the correct call.** Also:
**B-R028-08** five guard clauses survive individual mutation, so clause (e) is NOT fully met and the builder said so
(`completedWrite`'s `isWrite` is TRACED, load-bearing, and must not be deleted on the strength of a surviving mutant —
[[LSN-063]]) · **B-R028-09** TEST-087's apparatus was FABRICATING completions production now correctly swallows, and its
premise assertion was altered to repair it · **B-R028-10** ABA on the raw-address read token.
**B-R028-06 CLOSED BY CONSTRUCTION** (the seven sites are gone). **B-R028-05 HALF CLOSED** — labelling fixed, driver
half open by user decision. **B-R028-04 untouched and queued.**
**ORCH-028 — MY OWN BRIEFING MANDATE WAS WRONG AND THE BUILDER CAUGHT IT.** "Run with no ctest env override" stripped
`ASAN_OPTIONS=detect_leaks=0`, which this target PINS at `unittests/Core/garminconnect/CMakeLists.txt:1457/:1486`
because it leaks by design. Verified by execution: with no options the target exits **1** with `126280 byte(s) leaked in
331 allocation(s)`; with the pinned options, 61/61 exit 0. → **[[LSN-067]]**: naming the ONE variable that is the gate's
DIMENSION and keeping everything the target deliberately pins. Second consecutive dispatch whose mandate was corrected by
the agent executing it ([[LSN-065]] was the first).

Prior (superseded): **REQ-028 PHASE 2 BUILT 2026-08-18 — VERIFICATION GATE: EVIDENCE PASS, GOAL AUDIT **PARTIAL**. The guards are
real and load-bearing; **acceptance clause (c) is NOT delivered, and the builder says it is not DELIVERABLE under
DEC-034.** Nothing is running. The next action is a USER DECISION, not a gate.**
**The work is sound and the shortfall was self-disclosed, not discovered.** Uncommitted on `garmin/req028-row-lifetime`
(still 1 ahead of `master`, HEAD `da455c79e`). 3 files: `src/Cloud/CloudService.{h,cpp}` (+132/-1, +47/-0) and
`unittests/…/testGarminConnectSyncDialogClose.cpp` (+1133/-9). Two new PRIVATE members (`listGeneration`,
`batchListGeneration`), six compares, no public API change.
**EVIDENCE CHECK — PASS, every number orchestrator-EXECUTED, not read from the report:** rebuild unpiped **exit 0**
(`ninja: no work to do`) · ASan target run DIRECTLY with no ctest env under **BOTH `offscreen` and `minimal`** —
**57/57 each, EXIT=0, zero `SUMMARY: AddressSanitizer` lines** (baseline 52 + 5 new) · full **`ctest` 27/27** · the five
new slots confirmed **BY NAME**, not by the aggregate · drift lint exit 0 on both copies · scope clean (zero REQ-028
content in `MainWindow.cpp`/`src/CMakeLists.txt`/root `CMakeLists.txt`, whose mtimes predate the run) · **dirty
accounting reconciles exactly 60 → 63 as the three builder files** · zero `.orig` residue from me.
**MY OWN TWO MUTATIONS, individual and separately attributable ([[LSN-059]] honoured):** **M-ORCH-1** — delete the single
`listGeneration++` at :1526 → `heap-use-after-free READ of size 8 … QTreeWidgetItem::text(int) … syncNext()
CloudService.cpp:2219`, **exit 1, 1 summary**: PROBE-A's exact signature, so the whole mechanism is load-bearing on one
line. **M-ORCH-2** — neuter the `completedRead` entry compare alone → **exactly ONE slot dies**, TEST-111, on the
CRITERION's own assertion (`'out.labelledRows == 0' returned FALSE … labelled 1 row(s) … [Aborted|]`), 56 passed/1
failed — individually attributable, not an aggregate notice. Both restored `cmp`-clean (md5 `02b06a26…`), snapshot
deleted.
**GOAL AUDIT — THE PART THAT IS NOT DONE, verified by execution rather than accepted from the report.**
**THE PHASE-1 PROBE'S VERDICT IS UNCHANGED BY THE FIX, where the briefing predicted it would flip.** That is the
finding (per-id verdicts → traceability.md). I re-read TEST-107's assertions
on disk: it still asserts `readFileCallsAfter == 4` for **3** checked rows, `readNames == [row0, row1, row0, …]`,
`barAfter == 1` on a batch that completed nothing, `rowsRelabelled == [0]` (the WRONG row) and
`statusesAfter == [verdict, "Downloading", ""]`. Green therefore means **the P2 abort/restart defect is LIVE AND
UNCHANGED** — clause (c)'s "the reader is invoked exactly once per checked row" is unmet, by measurement.
**WHY it is not deliverable under DEC-034, and I find the argument sound:** on the abort+restart route **two transfers
are outstanding at once**, and the restarted batch's own dispatch **overwrites any shared snapshot before the stale
completion arrives**, so every scalar — `listGeneration`, `batchGeneration`, or a new one — reads EQUAL at the compare.
The completion carries no identity (`completedWrite` has no payload; `completedRead`'s `name` is documented at :2466 as
possibly not what was asked for). Closing it needs **per-transfer identity** (the buffer pointer) or **abort-time
accounting of outstanding transfers** — a mechanism choice with no governing DEC. The builder declined to invent one and
said so. → **B-R028-03.**
**THE BRIEFING'S "TWO GUARDS, NOT ONE" PREMISE DID NOT SURVIVE, and the builder was right to fire the hatch: NO NULL
CHECKS WERE ADDED.** Confirmed by grepping the diff — zero null guards. Its reasoning: once the generation compare sits
ABOVE every `child(listindex-1)` site, the refresh route stands down earlier, so a null check can never be individually
mutation-proven and adding it would violate clause (e) itself. That is [[LSN-063]]'s coverage-vs-reachability
distinction applied correctly, and it is the sixth briefing premise this ledger's delegation hatch has caught.
**But it leaves a question the A3 must chase (→ B-R028-06):** on the still-open abort/restart route the generation guard
does NOT fire, so if the rebuild comes back SHORTER, `child(listindex-1)` can still be null there — and the builder's
own note 6 records that the empty/shorter-rebuild route is driven by no run.
**A NEW USER-VISIBLE STATE THE FIX INTRODUCED, disclosed by the builder and untested (→ B-R028-04):** a Refresh
mid-batch now stands the batch down but leaves `downloading` true and the button reading "Abort", so the dialog is
wedged until the user clicks Abort. It follows DEC-032's existing stand-down idiom, but it did not exist before.
**TEST-112 — THE SORT ROUTE, MEASURED, AND THE ANSWER IS SPLIT (→ B-R028-05).** Executed, both backends: the properties
are open (`isSortingEnabled`/`sectionsClickable`/`isSortIndicatorShown` all true on `rideListSync`) and a sort arriving
inside the nested loop does real harm — `[10:00, 11:00]` → `[11:00, 10:00]`, **same two item addresses permuted, nothing
freed**, so it bumps no counter and passes every guard DEC-034 installs; the completion then labelled the WRONG row,
`writeFile` ran twice for the SAME row, and index 0 was never transferred. **But a synthetic `QTest::mouseClick` on the
header viewport did NOT reorder under either backend** — the reorder came from the `setSortIndicator` fallback. So
"a sort inside the loop does this" is EXECUTED; "the user's click causes that sort" remains an API-contract claim. The
builder recorded the two separately instead of presenting one as the other, and deliberately did not pin the mechanism
([[LSN-062]]/ORCH-017 — synthetic mouse delivery is the backend-dependent thing). **The lazy-sort sharpening is
CONFIRMED at Qt's source:** `qtreewidget.h:145-150` bounds-checks against the sort-invariant size, THEN calls
`executePendingSort()`, then returns — so the re-sort executes INSIDE the `child()` expression and no guard wrapped
around it can see the pre-sort order.
**Also disclosed and worth keeping:** clause (b)'s `rideCache->save() == 0` is asserted BY PROXY, not counted
(`RideCache::save` is a non-virtual no-op stub shared by three targets); `batchListGeneration` is a SECOND member where
DEC-034 costed "one member + six compares" (the compare count is exactly six); and dozens of pre-existing `file:NNN`
citations elsewhere in both files are now further stale by these insertions and were deliberately left alone, though the
12 the edits directly moved were repaired.

Prior (superseded): **REQ-028 PHASE 2 DISPATCHED 2026-08-17 — `qgdw-builder` IN FLIGHT on the production fix, branch
`garmin/req028-row-lifetime`. Nothing is blocked; the next action is the Verification Gate on its return.**
Phase 2 implements **DEC-034 Option C** (a `listGeneration` counter bumped by `refreshClicked`, snapshotted by both
drivers and the three completion slots) **plus a null-safety guard** — TWO guards, because Phase 1 proved the two routes
need different ones: a **null check** on all seven `child(listindex-1)` sites for the refresh route (P1, a crash), and a
**generation/identity check** for the abort/restart route (P2, where `child()` is non-null and a null check does nothing).
**Five slots allocated: TEST-108** (clause (a), row-UAF ASan, must be RED against unmodified production) · **TEST-109**
(clause (b), phantom completion tail) · **TEST-110** (clause (d), the idle-Refresh positive control, without which every
other slot is satisfiable by refusing to act) · **TEST-111** (clause (c) WORST HARM — the parse-SUCCEEDS fixture for
B-R028-01, never measured, where a real ride reaches `saveRide()` and writes an activity into the athlete's folder for a
batch that no longer exists) · **TEST-112** (the S-R028-01 sort-route reachability TRACE).
**USER DECISION 2026-08-17 on S-R028-01, reversing the 2026-08-16 "track, not fold in":** trace the route's
REACHABILITY inside Phase 2 rather than reopening DEC-034 up front. The reasoning: folding S-R028-01 in as a binding
constraint would falsify Option C by construction — a re-sort frees nothing and bumps no counter, so it passes the
guard, and only value/key addressing survives a reorder — so it is a DEC REOPEN, not a scope tweak (the
DEC-031-reopens-DEC-025 pattern). But the route is recorded **"reasoned, not traced"**, so the reopen would rest on an
inference. TEST-112 settles it by measurement; **if it reorders, DEC-034 reopens with executed evidence; if the click
cannot be delivered, Option C stands unchallenged.** Orchestrator narrowed the open question while briefing: all three
lists are built `setSortingEnabled(true)` (`:1116/:1146/:1186`) and **nothing anywhere calls `setSectionsClickable(false)`**,
and Qt's `setSortingEnabled(true)` makes header sections clickable as part of its contract — so the route looks
reachable. **That is REASONED FROM THE API CONTRACT, NOT EXECUTED** ([[LSN-063]]), and this ledger has had exactly that
class of claim falsified by measurement twice (TEST-081, TEST-089). It is TEST-112's job, not a finding.
**TWO STALE-STATUS DRIFTS FOUND AND CORRECTED DURING THE ORIENT, both of which had already misdirected this session:**
`WIKI.md` REGISTRIES said DEC-034 was *"not yet researched"* and REQ-028's acceptance was *"TO BE ELABORATED BY
DEC-034"* — **both false since 2026-08-16.** DEC-034 is ACCEPTED (Option C) at `decisions.md:1160` and prd.md:154 carries
the elaborated clauses (a)–(e). The stale prose caused the orchestrator to frame the next gate as a `qgdw-scout`
research dispatch when it is a `qgdw-builder` implementation dispatch, and to put a scoping question to the user built
on a decision that was already made. Caught by opening `decisions.md` at the DEC's own entry rather than trusting the
hub. WIKI corrected in place; **ORCH-026 raised** — this is the [[LSN-035]] family (the hub restating a status the SSOT
owns) and the third time this project has been misled by a registry line that outlived its subject.
**The briefing was built from FRESHLY RE-DERIVED offsets, not the recorded ones** — the citations in prd.md and
findings.md have shifted under three consecutive waves. Verified on disk 2026-08-17: `refreshClicked` :1516, deletes
:1532/:1539/:1544, rebuilds :1617/:1682/:1727/:1774; `downloadClicked` :1908 with `batchGeneration++` :1931; `syncNext`
:1976, snapshot :1985, capture :1988, **post-suspension read :2098** (recorded as :2096), `openRideFile` :2124, compare
:2221; `downloadNext` :2259; `completedRead` :2370; `failedRead` :2511; `uploadNext` :2553, snapshot :2566, compare
:2680; `completedWrite` :2707. **The seven `child(listindex-1)` sites: 2388, 2437, 2444, 2525, 2532, 2717, 2724.**
`CloudService.h:644` declares `batchGeneration`; `listGeneration` does not exist yet.
**Explicitly briefed, because the last dispatch did it unasked: DO NOT COMMIT.** Git history on this ledger is the
user's call. Also mandated: individual (never simultaneous) guard mutation, a call-site trace per guard, `.orig`
snapshot + `cmp` restore with `git checkout` FORBIDDEN on files carrying uncommitted work, the binary run DIRECTLY under
BOTH `offscreen` and `minimal` with the exit code and `SUMMARY: AddressSanitizer` grep read instead of the test output,
and no bare N/N without its backend. Baseline to hold: **ASan 52/52 both backends, exit 0, zero sanitizer summaries;
full ctest 27/27.** TEST-107 must be **rewritten or retired and the builder must say which** — it pins current behaviour
(B-R028-02), so a green TEST-107 after the fix would mean the fix did not work.

Prior (superseded): **REQ-028 PHASE 1 DONE + VERIFICATION-GATE PASS 2026-08-16 — THE GATING PROBE ANSWERED BOTH QUESTIONS, AND ONE
ANSWER RAISES A FINDING'S SEVERITY. Nothing is running; the next action is a user decision on Phase 2's shape.**
TEST-107 committed as `da455c79e` on `garmin/req028-row-lifetime` (1 ahead of `master`, NOT merged, NOTHING PUSHED),
one file, +356/-0, **production byte-identical — the STOP GATE held** (`CloudService.cpp` md5 `7e92e51aa…`,
`.h` `7eefd7ae…`).
**P1 — `QTreeWidgetItem::child(int)` returns NULL out of range.** Measured four ways by the builder AND confirmed by me
at Qt's own source (`qtreewidget.h:145-150` is inline: `if (index < 0 || index >= children.size()) return nullptr;`), so
the "unspecified in the docs" question that gated this decision is settled. **Consequence: A3-R027-F8's refresh route is
a NULL DEREFERENCE — a crash — not the mislabel the ledger recorded. The severity was understated, and Phase 2 needs a
null check, not a row-identity check, on that route.**
**P2 — the abort/restart route IS reachable, and is a SECOND, different defect.** There `child(listindex-1)` is
NON-null (the restart zeroes `listindex`, the new batch raises it back into range), and the old batch's late completion
labels **the row the NEW batch is transferring** — measured `["Downloading","Downloading",""]` →
`["TEST-096 unparseable activity","Downloading",""]`, with the row that actually finished never told. The restarted
bar advances for a row it never completed, and `completedRead`'s tail re-drives `downloadNext`: **4 `readFile` calls for
3 checked rows**, rows 0 and 1 each transferred twice, row 2 never reached. So F8's original "second driver" framing is
CONFIRMED verbatim; only its severity on the other route was wrong.
**Orchestrator-executed evidence, not read from the report:** rebuilt (unpiped exit 0), ran the binary DIRECTLY under
BOTH `offscreen` and `minimal` — **52/52 each, EXIT=0, zero `SUMMARY: AddressSanitizer` lines** (baseline 51 + 1);
commit diffed (one file, +356/-0, zero out-of-scope markers); dirty accounting reconciled 54 → 60 as exactly the six new
governance files; `master` untouched, nothing pushed.
**Two residuals the builder disclosed rather than buried.** **B-R028-01 — the WORST harm was not measured:** every row
is `.gcfail`, so the parse-SUCCEEDS path never runs, and on that path a real ride reaches `saveRide()` and **writes an
activity into the athlete's folder on behalf of a batch that no longer exists** — persistent, unlike anything TEST-107
saw. **B-R028-02 — TEST-107 locks in current behaviour** and will go RED when production is fixed; it must be rewritten
or retired in Phase 2, and its green run is not reassurance. Its CONTROL is what carries the weight, and two of the
builder's own predictions died against that control before the values were locked.
**S-R028-01 (the sort route) got HARDER, found while verifying P1:** `child()` calls `executePendingSort()` before
returning, so Qt's sorting is LAZY and a pending re-sort executes **inside the addressing expression itself** — no
guard placed around `child(listindex-1)` can observe the pre-sort order. Any Phase-2 design assuming "check the
container, then address it" has a window it cannot close on that route.
**One deviation to note: the builder COMMITTED without being asked.** The commit is clean (test-only, one file, zero
other-owner content, `master` untouched, unpushed) and the message is accurate, so nothing is at risk — but git history
on this ledger has always been the user's call, and the Phase-2 briefing will say so explicitly.

Prior (superseded): **PHASE 1 DISPATCHED — `qgdw-builder` IN FLIGHT on the gating probe, branch
`garmin/req028-row-lifetime`.** The scout's draft passed the Verification Gate; the orchestrator confirmed every
decisive claim at its cited lines before recording anything. **Phase 1 is probe-only and production is under a STOP
GATE** (`CloudService.cpp` must stay md5 `7e92e51aa7b771948bca62ecfc7841e2`), because the probe's whole point is to
measure UNMODIFIED production — the TEST-081/TEST-089 pattern, which changed the answer both times this ledger used it.
**What the probe must settle: `QTreeWidgetItem::child(int)` out-of-range behaviour is UNSPECIFIED in Qt's docs**, so
A3-R027-F8's harm today is either a null deref (a crash, more severe than recorded) or a silently wrong-row label —
different guards. TEST-107 (the probe) and TEST-108 (the PROBE-A row-UAF slot, which must be RED against unmodified production)
allocated; TEST-109/110 held for Phase 2.
**FIRST DISPATCH STALLED (600s no-progress watchdog) having written nothing — ORCH-025.** No damage, verified not
assumed: production md5s unchanged (the STOP GATE held), no branch, nothing under `unittests/`, no `.orig` residue.
**The briefing defect it exposed is the part worth keeping:** it told the builder to build a late-completing store
"the harness does not have", a claim inherited from the scout's draft and never checked — and it is FALSE.
`BlockingStore::completeRead` (test file `:707`) already withholds the completion and
`CloudService::notifyReadComplete` (`CloudService.h:145`) delivers it on demand, so the fixture is a few lines of use.
The dispatch also said "study the harness" against a 7,850-line file with no range. Re-dispatched narrower: TEST-107
ONLY (TEST-108 split out), reading fenced to `:420–716` and `:5355–5372`, the false claim corrected.
**Three verified facts decided C over the stub's own candidate (a)** — all orchestrator-confirmed, not read from the
report: (i) gating on `downloading` is DEFEATED, because the abort branch sets `downloading=false` at `:1915` while the
driver is still suspended inside `openRideFile`, and that frame then WRITES `curr->setText(7, …)` at `:2153`; (ii) a
`refreshButton->setEnabled(false)` fix is INVISIBLE to every test here — the harness calls `dialog->refreshClicked()`
19 times and references `refreshButton` zero times, so it would sit behind a green suite proving nothing; (iii) it would
GUT TEST-091, whose fixture drives `refreshClicked()` from inside `readFile`'s loop precisely to nest a counted frame
inside an uncounted one (B-R031-01's premise). Option C preserves that geometry.
**S-R028-01 — a THIRD list mutator nobody had recorded: SORTING.** All three lists are built sorting-ENABLED; at batch
start only `rideListDown` is disabled, `rideListUp` is explicitly RE-enabled (`:1921`), and `rideListSync` is never
disabled at all. A reorder frees nothing and bumps no counter, so it passes every guard C installs. **User decision:
TRACK, not fold in.** Reasoned-not-traced — the scout declined to claim the route without confirming the header is
clickable. Zero production code touched so far this wave. **Briefed from SYMBOLS plus freshly re-derived offsets, not from the
recorded ones:** every line number in the REQ-028 stub and in the A3-R027-F2/F3/F8 rows (cycle `A3/REQ-027
(2026-08-14)`) had SHIFTED, because DEC-035's guard blocks landed above them on 2026-08-15 — `uploadNext`'s generation
snapshot moved :2430 → :2566 and F8's seven completion-slot citations moved :2193…:2464 → :2388…:2724. Briefing from the
recorded numbers would have pointed the scout into unrelated prose, which is the ORCH-020 failure exactly.
Two findings raised as byproducts of the orientation, both non-blocking and neither acted on: **ORCH-022** — the
finding-id namespace COLLIDES (`A3-R027-F2`/`F3` each denote two unrelated findings, from the A3 on DEC-027 and the A3
on REQ-027); it is the one id family with no allocator, the same shape as [[LSN-044]]. **ORCH-023** — an orphaned
`git stash` from REQ-002 (2026-05-23) holds 18 files / +651 of the Coach owner's work that is present nowhere in the
working tree and whose blobs differ from HEAD; untouched and reported, since whether HEAD superseded it needs its owner,
not an inference.

Prior (the tree state this wave starts from, still true): **THE WAVE IS CLOSED 2026-08-16 — committed AND verified from a clean extract (per-id lifecycle →
traceability.md).** **LANDED ON `master` AND PUSHED 2026-08-16 (user authorised both).** `master` fast-forwarded
`2f0de993d` → **`82a0de52a`** (11 commits: the REQ-019, REQ-021 and REQ-027 waves, which had been stacked unmerged),
then `master` + the feature branch pushed to **`origin` = the user's FORK (FONSECAMVP/GoldenCheetah)**. All four refs —
local/remote `master`, local/remote `garmin/req027-silent-stall` — now read `82a0de52a`. **`upstream`
(GoldenCheetah/GoldenCheetah) was NOT touched and no PR was opened; that stays the user's call.**
The FF was done in place (`git branch -f master HEAD` + checkout), NOT as a merge-from-master, precisely so it touched
zero working-tree files: the 53 dirty entries survived, and the seven at-risk owner files were md5-verified `OK` after
the switch as well as after both pre-commit stash cycles. Four commits on
`garmin/req027-silent-stall`: `3c7fa7385` (feature — REQ-027 +
DEC-032 + DEC-035), `b1e4c4fad` (ORCH-021, the ORCH-015 drift-lint repair that had been living in the working tree only
and therefore was NOT what the gate ran), `ac5d5f67d` (docs record — DEC-035, A3-R027c, LSN-063/064), `82a0de52a` (this gate's docs record). The clean-extract
gate PASSED on `ac5d5f67d` — full evidence in NEXT_GATE. Our four files are committed; the 53 dirty entries left in the
working tree are OTHER OWNERS' pre-session churn (Coach/Gui/CMake/vcpkg/skill), untouched and still theirs to land.
BLOCKING: **NONE — ORCH-033 RESOLVED 2026-08-22 by `e48f7d123`, evidence in CURRENT (two backends, both ctest
registrations, garmin-fast 25/25, one attributable mutation). With the apparatus running to completion again, the fix
wave's own claim is now supportable: ORACLE-F1 / A3-R028c-F1 / A3-R028c-F2 sit behind a suite that actually finishes.**
  Open non-blocking, carried forward: A3-R028c-F4..F7 · B-R028-04 (deferred by user decision) · B-R028-14 (ratified,
  governance question still unanswered) · clause (e)'s reachability half (two guards ship "reasoned, not traced";
  A3-R028c-F7 re-diagnoses that as a DRIVABLE coverage gap) · the sort route, separately tracked per `prd.md:154`.
  Superseded verdict (2026-08-22, pre-repair): **ONE — ORCH-033: the ASan target does not run to completion on either
backend. This blocks the commit gate absolutely — a red suite is not a checkpoint.**
  Superseded verdict (recorded 2026-08-21/22, falsified 2026-08-22): **none, as of the fix gate recorded in CURRENT — the three that were open (ORACLE-F1, A3-R028c-F1, A3-R028c-F2) are recorded CLOSED by the fix wave: built, Verification-Gate PASS, fresh attack-the-fix with no blockers, incremental CLV PASS 9/9, seeds 32/447 green on both backends. NOT YET COMMITTED — the whole REQ-028 fix wave lives in the working tree on `garmin/req028-row-lifetime` (HEAD is still `da455c79e`, the TEST-107 probe). This line was internally contradictory until 2026-08-22 (it carried both a "none" from the 08-19 amendment and a "THREE" from the 08-21 A3 re-clear); it now states one verdict with its provenance ([[LSN-078]] — a verdict line carries what produced it).**
  Superseded detail, retained for provenance: A3-R028-F1/F2 CLOSED 2026-08-19 by the invalidation amendment (DEC-036), orchestrator-mutation-proven (both sites removed → the UAF reproduces; site 1 removed alone → TEST-118 dies on its own criterion). The three later blockers were: **ORACLE-F1 (a THIRD use-after-free, found MECHANICALLY by the new fuzzer and by no adversarial cycle: the DEC-034 compare in `completedRead` is correctly placed and defeated anyway, because `row` is a STACK LOCAL copied before the suspension — orchestrator-diagnosed), A3-R028c-F1 (Abort→Refresh→restart re-opens A3-R028b-F3 in full, because `refreshClicked` DROPS the abandoned ticket instead of retiring it) and A3-R028c-F2 (an Abort+restart burst inside a completion tail's own `processEvents()` puts TWO transfers on the wire against ONE ticket). Both orchestrator-confirmed at the code. Both are rooted in the ORCHESTRATOR's decisions, not in builder error.** Open non-blocking: B-R028-13/14/15/16, B-R028-04 (deferred by user decision), A3-R028b-F4/F6/F7, the sort route's driver half. Open non-blocking: A3-R028b-F4..F8, B-R028-04 (needs a DECISION), B-R028-05 driver half, A3-R028-F6/F7, B-R028-08/10/11. Cite any A3 finding by id AND its Cycle value `A3/REQ-028 wave (2026-08-18)` — ORCH-022, the token alone is ambiguous.**

Prior (superseded — the state before the commits): **REQ-027 BUILT + VERIFICATION-GATE PASS 2026-08-14 (uncommitted); A3 RUNNING. DEC-032 accepted (Option A) on `garmin/req027-silent-stall`.** The scout's draft
passed the Verification Gate and **found three things that changed the picture, all orchestrator-confirmed at their cited
locations before anything was recorded:**
**(1) S-R027-01 — a LIVE REQ-026 escape in code committed yesterday.** REQ-026 added the abort re-read to the
parse-failure branch of `uploadNext` (:2420) and to `completedRead` (:2239) but NOT to the SUCCESS branch of either
loop: `openRideFile` (:2374 / :2046) → `self.isNull()` (:2379 / :2051) → `compressRide` → `writeFile` (:2389 / :2062)
with no `aborted` read between. Press Abort while a PARSEABLE ride is being read and it is transferred anyway. Confirmed
by grepping every `aborted` occurrence in the file. **Folded into REQ-027 by user decision.**
**(2) My own O-R027-01 was WRONG and is corrected in place.** I recorded the discarded-`readFile`-bool hazard as DORMANT
because I enumerated "which services LACK a `readFile` override" (answer: `Withings`, unreachable). The right predicate
was "which can return `false` without emitting" — and that is nearly all of them, on ordinary error paths.
`LocalFileStore::readFile` alone has four such returns, one of which (`!file.exists()`) fires whenever a listed file is
deleted before download. **The stall is LIVE today with no code change.** Captured as a miss + refinement on [[LSN-048]]:
re-deriving the surface is not enough if you enumerate by the wrong predicate.
**(3) S-R027-03 — the "no stack growth" constraint I wrote into the briefing had a false baseline.**
`LocalFileStore::readFile` emits `readComplete` SYNCHRONOUSLY (`LocalFileStore.cpp:150`), so `completedRead`→`syncNext`
already runs inside `readFile` inside the `BlockingCall` scope: that service recurses ~3 frames per row today. Options
had to be scored against that, not against zero.
Option A won on three grounds: smallest change (3 lines + 2 read-side checks, all ~16 services fixed with no seam
change), it converges `syncNext` onto `uploadNext`'s already-shipped REQ-026 shape rather than adding a third idiom, and
it is the only option resting on a locally provable invariant (`closeDeferred` :1475 ⇒ `aborted` :1476, single setter,
verified) rather than a Qt delivery semantic — where the project HAS measured that semantic, TEST-089
(`CloudService.h:512-521`), the measurement argues against Option B.

Prior: **REQ-027 OPENED + DEC-032 ALLOCATED 2026-08-13; `qgdw-scout` RESEARCHED the fix shape.** User
picked B-R026-01 — the silent sync stall — off the wave-close queue. REQ-027's acceptance is deliberately left
**TO BE ELABORATED BY DEC-032**, because the finding itself established this is a design question, not a one-liner: a
naive re-drive must not re-enter unboundedly on a list of unparseable files. Byproducts already merged: REQ-027 stub in
prd.md, O-R027-01 in findings.md, B-R026-01 relinked to its REQ, WIKI registries bumped (REQ next:garmin-028,
DEC next:garmin-033). Zero code touched. Details of both axes → NEXT_GATE.

Prior (still true of the tree): **THE WAVE IS COMMITTED 2026-08-13 (user said proceed). Feature `6dc794caf` + lint fix `3f44c447f` + this docs
record, on branch `garmin/req021-collaborator-uaf` (now 5 ahead of `master`, NOT merged, NOTHING PUSHED — pushing is the
user's call).** The wave covers REQ-021 + DEC-030 + DEC-031 + REQ-025 + REQ-026.
**Feature commit `6dc794caf`** — 5 files, +4730/-94: `src/Cloud/CloudService.{h,cpp}`, `src/Core/Context.cpp`,
`unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp` + `stubs/ImportSeamStubs.cpp`.
**Lint commit `3f44c447f`** — 3 files, +150/-4 (ORCH-010: `scripts/ledger_drift_lint.py` + its test + the installed
`.claude/hooks/` copy).
**NO HUNK-SPLITTING WAS NEEDED — and that was verified, not assumed.** Unlike REQ-017/REQ-019, this changeset reaches
neither `src/Gui/MainWindow.cpp` nor `src/CMakeLists.txt`, so every file was staged WHOLE by explicit path. A scan of the
added lines in all eight code/tooling files for `coach|anthropic|openai|gemini|libusb|calendar` returned zero in each.
**The Coach owner's work was proven intact, not hoped intact:** an md5 baseline of eight at-risk files
(`MainWindow.cpp`, `src/CMakeLists.txt`, root `CMakeLists.txt`, `vcpkg.json`, `AnthropicClient.cpp`, `KurtInRide.cpp`,
`application.qrc`, the skill `SKILL.md`) was taken BEFORE staging and re-checked after **both** pre-commit stash cycles —
all eight `OK` every time.
**clang-format DID rewrite both test files on attempt 1** and aborted the commit (the [[LSN-007]] pattern, third wave
running). Proven cosmetic the strong way — with all whitespace stripped, the formatted and staged files were
byte-identical (md5 `8baffc5f…` / `7c1df818…`) and zero `#include` lines moved — and then **re-verified BY EXECUTION
anyway** (rebuild, ASan 42/42, full ctest 26/26) before re-staging and retrying, because "obviously cosmetic" has been
wrong in this repo before.
**One deliberate exclusion:** `.claude/hooks/anti_duplication_guard.py` is byte-identical to its install source
(md5 `658575d4…` both), so it is a sync artifact of the USER's pre-session skill update, not this wave's work. Left
uncommitted with the rest of the skill/tooling churn, which remains the user's call.
**CLEAN-WORKTREE GATE: PASSED 2026-08-13 on the committed tree** (`4c3608e89`, tree `37335557f…`) — 822/822 targets,
`GoldenCheetah` 27,984,000 B, 26 test executables, ctest 26/26, the wave's ASan target 42/42. Details in NEXT_GATE.
**This wave is committed AND verified-from-clean.**

Prior (superseded): **AT THE COMMIT GATE 2026-08-12. The A3 re-clear found one BLOCKING defect, it was fixed under REQ-026, and the
fix is orchestrator-verified BY EXECUTION (42/42 ASan, 26/26 ctest, both guards mutation-proven load-bearing under a
SIMULTANEOUS revert). Nothing is running; nothing is blocked on evidence. The next action writes git history, so it waits
for the user.** The wave was uncommitted on `garmin/req021-collaborator-uaf` (3 ahead of `master`, HEAD `da07b2227`).
**The lesson of this session, stated plainly:** the wave had already passed a Verification Gate and stood at 40/40 + 26/26
green when the A3 was proposed as "not a formality". It found a defect that shipped rides to a third-party service after
the user pressed Abort. Three A3s in a row have now found real defects behind a green suite.

Prior (superseded): **A3 RE-CLEAR DONE 2026-08-12 — VERDICT NOT CLEAR. The wave is NO LONGER shippable as it stands: one BLOCKING
defect (A3-R021b-F1, `uploadNext` never reads `aborted`) must be dispositioned before the commit gate. Nothing is
running; the next action is a user decision (→ NEXT_GATE). Everything below about the built state remains accurate —
the suite is still 40/40 + 26/26 green, which is precisely the point: the defect is invisible to it.**

Prior (still accurate as a description of the built tree): **SESSION SAVE-POINT 2026-08-12 — the whole REQ-021 wave sits built and Verification-Gate-passed in the
working tree, UNCOMMITTED (per-id lifecycle → traceability.md), on branch `garmin/req021-collaborator-uaf` (3 ahead of `master`, HEAD `da07b2227`, 67 dirty
entries of which 9 are ours). NOTHING IS IN FLIGHT — no agent running, no gate half-done. NEXT ACTION IS A DECISION,
see NEXT_GATE.**

**Green everywhere, independently re-verified by the orchestrator, not read from reports:** ASan target
`testGarminConnectSyncDialogClose` **40/40** · full `ctest` **26/26** · `src/GoldenCheetah` links · drift lint exit 0
(both the canonical `scripts/` copy and the installed `.claude/hooks/` copy) · `test_ledger_drift_lint.py` **21/21** ·
`guard_selftest.py` 0 core / 0 gap failures.

**OUR 9 DIRTY FILES (everything else in the 67 is other owners' pre-session work — Coach/Gui/CMake/vcpkg/skill):**
`src/Cloud/CloudService.cpp` + `.h` (REQ-021 reparent + riders, DEC-031 reaper, REQ-025 frames) ·
`src/Core/Context.cpp` (one line, `tab = NULL;`, O-R021-02) ·
`unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp` + `stubs/ImportSeamStubs.cpp` (TEST-081..092) ·
`scripts/ledger_drift_lint.py` + `scripts/test_ledger_drift_lint.py` + `.claude/hooks/ledger_drift_lint.py` (ORCH-010
fix + sync) · `.claude/hooks/anti_duplication_guard.py` (re-copied by `install_hook.py`, byte-identical to its install
source — verified, nothing clobbered, ORCH-013).

**WHAT LANDED THIS WAVE, in dependency order:**
1. **REQ-021 / DEC-030** — collaborator-lifetime UAF. Both cloud dialogs reparented to `context->tab` via a
   `cloudDialogParent(context)` helper (`context->tab ?: context->mainWindow`, the A3-R021-F3 fallback), plus
   `QPointer<Context>`/`QPointer<RideItem>` riders in both `start()`s covering the pre-delete window at
   MainWindow.cpp:2148-2171. Gated on TEST-081, which MEASURED that child `QDialog` windows do NOT follow a parent
   widget's hide on Qt 6.8.2 (with a sensitivity control proving the apparatus can see a hide).
2. **A3 remediation** — A3-R021-F1 (four `processEvents()`→`this` sites in the modeless completion slots, newly
   exposed BY the reparent), F2 (an EXECUTED UAF: `refreshClicked`'s guard was self-only while `start()` checked `ctx`
   too late), F3, F5, F6.
3. **DEC-031** — reopened DEC-025. Its deliberate store leak was justified by "the application is already tearing
   down", which the reparent falsified. Replaced by a frame-counted, per-dialog, REFCOUNTED `StoreReaper` drained by
   the unwinding `BlockingCall`. TEST-089 probe first: both naive `deleteLater` and queued-invoke forms die INSIDE the
   nested loop, so the naive reaper would free the store under its own frame — the frame-counted shape was required.
   Five assertion sites inverted from "the store leaks" to "closed and deleted exactly once, only after the loop
   returned"; TEST-073 byte-unchanged as the discriminator.
4. **REQ-025** — DEC-031 was UNSAFE alone (B-R031-01): an UNCOUNTED nested loop can ENCLOSE a counted one, so the
   reaper fired while an outer frame was suspended. Counted three sync-dialog sites + added the upload-dialog lazy-open
   bail. RED-verified then closed; orchestrator reproduced the UAF independently by stripping the site-1 frame+bail.
5. **ORCH-010** — the DEC-015 drift lint (a MECHANISM) was repaired, not routed around, after escalating from 1 false
   positive to 27. `find_statuses()` gained assignment-shape discrimination; 15 pre-existing cases still pass + 6 new
   two-directional cases = 21/21, mutation-proven, installed copy synced.

Prior (superseded): **REQ-021 PHASES 1+2 BUILT + Verification-Gate PASS 2026-08-11 (working tree); A3 RUNNING.** The collaborator
UAF is fixed per DEC-030: both dialogs reparented to `context->tab` (so `delete tab` at MainWindow.cpp:2183 destroys them
BEFORE the Athlete and Context they point at), `QPointer<Context>`/`QPointer<RideItem>` riders in both `start()`s covering
the pre-delete window at :2148-2171, and S-R021-01's missing bail closed. **ASan target 32/32 (was 25), full ctest 26/26,
`GoldenCheetah` links.** Production diff is `src/Cloud/CloudService.cpp` ONLY — `CloudService.h` and `MainWindow.cpp`
untouched, so the Coach owner's uncommitted work was never at risk.
**Orchestrator-executed gate evidence (not read):** reproduced the Phase-1 probe including its sensitivity control; own
mutation reverting **BOTH** reparents simultaneously → **5 slots FAIL** (the builder had only reverted them singly);
own mutation of the harness (stop deleting the Context) → all 3 harness slots FAIL; restorations byte-identical
(md5 `873de31f…` and `65eec658…`, `cmp` clean, zero `.orig` residue); FILES reconciled exactly.
**One HAZARD THE FIX ITSELF INTRODUCED, found by the BUILDER in its own work and confirmed + fixed by the orchestrator
(O-R021-02 → [[LSN-049]]):** the fix makes both dialog ctors READ `Context::tab`, which `Context::Context` never
initialised — it is assigned only by `AthleteTab`'s ctor, and `MainWindow::openAthleteTab` (:2038) publishes a Context via
`openingAthlete` well before that. An uninitialised pointer handed to `QDialog` is undefined behaviour, i.e. strictly
worse than the UAF being fixed. Closed with one line (`tab = NULL;` + comment) in `src/Core/Context.cpp`, verified 26/26.
**The POLICY residual was deliberately left undecided and routed to A3**, not silently chosen: a null parent makes the
dialog an unparented top-level that nothing destroys, versus falling back to `mainWindow`.
**Builder self-disclosed five more residuals rather than burying them (B-R021-05..09)** — chief among them a NEGATIVE
mutation result kept on the record: only 3 of the 11 widened bails are load-bearing; the other 7 survive removal at
32/32. Also: the sync dialog's modality is REASONED not measured (B-R021-08), and `QTest::mouseClick(QWidget*)` turned out
to be BLIND to modal blocking on Qt 6.8.2, which produced a false alarm before the `QWindow` overload fixed it →
[[LSN-050]]. That is the second time this wave a green test proved nothing because the apparatus could not see the
phenomenon ([[LSN-047]] was the first).
Prior (superseded): **REQ-021 PHASE 1 DONE + Verification-Gate PASS 2026-08-10 — the GATING PROBE CLEARED Option B; Phase 2 (the
production fix) is IN FLIGHT.** TEST-081/082 built on branch `garmin/req021-collaborator-uaf`; ASan target 25/25, full
ctest 26/26, **zero `src/` changes — the stop gate was honoured.** **PROBE VERDICT (measured, not reasoned): child
`QDialog` windows do NOT follow their parent widget's hide** on Qt 6.8.2 here — a modeless `QDialog(tab, Qt::Dialog)`
stayed visible through a raw `hide()` AND through `QStackedWidget::setCurrentIndex` (the mechanism `switchAthleteTab`
actually uses), with no sticky hide, while a non-window control child DID go invisible, so the apparatus is demonstrably
sensitive rather than vacuously green. **Orchestrator verification was independent, not a read:** re-ran the target and
reproduced the measurement incl. the control, reconciled FILES exactly (2 modified under `unittests/`, `src/Cloud/
CloudService.{h,cpp}` entirely CLEAN, zero REQ-021 content anywhere in `src/`), and ran its OWN mutation — the harness
`closeAthleteTab` stops deleting the Context → **all THREE harness slots FAIL** — then restored byte-identical (md5
`65eec658…`, `cmp` clean, no `.orig` residue) and re-confirmed 26/26. Closes A3-R019-F3, B-R019-04, B-R019-05.
**The builder caught a FALSE PREMISE in the orchestrator's own briefing, and it was the probe's own premise** (O-R021-01,
[[LSN-034]] recur:3): the briefing said `MainWindow::switchAthleteTab` hides tabs "at MainWindow.cpp:2165" — :2165 is a
CALL SITE, the definition is at :2367 and contains no compiled `hide()` at all (only `#if 0` blocks); the real mechanism
is `tabStack->setCurrentIndex()` at :2389. **It cost nothing only because the builder probed BOTH routes.** The guard's
failure shape is now pinned: quoting a line number for a symbol without opening that symbol's DEFINITION — three for
three caught by the delegation hatch. Builder also self-disclosed four honest residuals (B-R021-01..04), the sharpest
being that its `saveSilent` stub load is STRICTER than production, which never touches a MainWindow member.
Prior (superseded): **REQ-021 OPENED + DEC-030 ACCEPTED 2026-08-10; `qgdw-builder` BUILDING PHASE 1 (harness + gating probe) on
branch `garmin/req021-collaborator-uaf`.** The scout's research changed the answer's shape twice, and both changes were
orchestrator-verified on disk before they were acted on. (i) **`Context` and `RideItem` are BOTH `QObject`s**
(Context.h:106, RideItem.h:41) — the fact the briefing told the scout to CHECK rather than assume — which makes the
hand-rolled abandonment-signal option (C) redundant rather than merely costly. (ii) **The sync dialog's collaborator
surface is ~8× larger than REQ-021's stub claimed** (S-R021-02): `MainWindow::syncCloud` opens it MODELESSLY
(`open()`, not `exec()`), so it outlives its caller and keeps running slots — nine further `context->` derefs across
`refreshClicked`/`syncNext`/`downloadNext`/`uploadNext`/`saveRide`, all nine spot-checked and confirmed. **That
correction is what selected the option**: a per-site guard closes ~18% of the surface, a structural reparent closes
100% for two lines → [[LSN-048]]. (iii) The scout also found **S-R021-01, a live one-line `this`-axis gap on master**
that four adversarial cycles missed: the sync dialog's Cancel branch runs `processEvents()` (:1107) then
`invokeMethod(this,"close")` (:1108) with no `self.isNull()` between them, while the upload dialog's identical branch
HAS it (:451-452) — folded into REQ-021 by user decision. **DEC-030 = Option B (reparent to `context->tab`),
PROBE-FIRST, + Option A guards as a rider** covering the pre-delete window (MainWindow.cpp:2148-2171) that reparenting
cannot reach. The scout REFUSED to assert the one Qt premise it could not source (child-window hide semantics) — correct
per this project's execution-over-reasoning rule — so TEST-081 measures it and the fallback to Option A across all 11
sites is pre-authorised. Sibling scan routed by user: `AddCloudWizard`→REQ-020 (widened), QThread cases→**REQ-022**,
store layer→**REQ-023**, and a dedicated pass queued for the six-class tail + A3-R019-F6 (S-R021-06).
Prior (superseded): **REQ-019 COMMITTED `e8833682f` 2026-08-08 — the upload-dialog UAF is closed on the axis it targeted, and the
clean-checkout gate passed on the ACTUAL COMMITTED TREE.** 4 files, +697/-14, on branch `garmin/req019-upload-uaf`
(now 1 ahead of master). Staged via plumbing: 3 whole files + `src/Gui/MainWindow.cpp` HUNK-SPLIT to its 16-line comment
hunk — the committed copy contains ZERO Coach content and the worktree copy is md5-identical to its pre-session state
through BOTH pre-commit stash cycles, so the Coach owner's uncommitted work was never at risk. clang-format rewrote the
test file on attempt 1 (cosmetic only) → re-staged and RE-VERIFIED BY EXECUTION (20/20, 26/26) before retrying, per
LSN-007. Committed tree `1ab2135a…` == intended staged tree. **Clean gate run TWICE**: once on the pre-format staged
tree, then AGAIN on the committed tree after the reformat rather than assuming whitespace was harmless — `git archive
HEAD` extract → configure + build → GoldenCheetah 27,983,584 B + 26 test executables, ctest 26/26, ASan 20/20.
**A3 verdict: the targeted axis is CLOSED; a SECOND axis is not.** A3-R019-F1/F2 (orchestrator spot-checked and
CONFIRMED at all four cited sites): the guards protect the DIALOG's lifetime only — `context`/`item` belong to the
AthleteTab, which `MainWindow::removeAthleteTab` deletes SYNCHRONOUSLY while MainWindow's own WA_DeleteOnClose deletion
is DEFERRED, so the dialog reliably outlives its Context and `self.isNull()` never fires. **The same gap is LIVE ON
MASTER in the shipped sync dialog** (`ae5a7a8ab`), which weakens the recorded A3-R027-CLOSURE claim. User decision:
commit REQ-019, fix that axis in the new **REQ-021** across BOTH dialogs, with a purpose-built fake modelling the real
two-phase teardown order (the current stubs cannot express it). A3 also REFUTED four hypotheses with evidence.
Lessons this wave: [[LSN-045]] (DEC self-consistency + run the alignment probe), [[LSN-046]] (a self-lifetime guard
covers exactly ONE pointer — `this`), [[LSN-047]] (an inert stub cannot prove the fault it is credited with).
Prior (superseded): **REQ-019 BUILT + VERIFICATION-GATE PASS 2026-08-08 (uncommitted); A3 RUNNING.** The upload-dialog UAF is
fixed per DEC-029 Option B: ctor is a widget shell, `start()` carries the blocking work behind 6 self-bails, the
construction site (`CloudService::upload`, NOT uploadCloud) is heap + `WA_DeleteOnClose` + `if(start()==false) return`
+ `exec()` with no else-delete, modal preserved, store ownership untouched. 9 new ASan slots (11→20), full ctest 26/26.
**Gate evidence, orchestrator-executed:** own mutation of the post-`open()` self-bail → `heap-use-after-free` at
CloudService.cpp:418, restored byte-identical (md5 `dba7edb8…`, `cmp`); FILES reconciled (62 dirty = 59 pre-existing + 3
touched, zero residue); `MainWindow.cpp` diff read line-by-line = COMMENT-ONLY with the Coach owner's hunks intact.
**TEST-080's gating question HELD** — `QDialog::exec()` does self-protect when `this` dies mid-loop on Qt 6.8.2 here, so
DEC-029 Option B is not falsified. **Two governance defects found by the BUILDER, in the orchestrator's own DEC text**
(O-R019-01 → LSN-045): DEC-029 named `MainWindow::uploadCloud` as the construction site (it is `CloudService::upload`;
the same DEC cited it correctly two paragraphs earlier), and shipped an alignment probe unsatisfiable under the very
idiom it mandated. Both corrected in place. This is the SECOND briefing-accuracy failure this wave after the LSN-034
proximity-grep miss — the delegation hatch caught both, which is the only reason neither cost anything. Six residuals
open (B-R019-01..06), none blocking; B-R019-02 is a real latent UAF one edit away.
Prior (superseded): **BRANCH LANDED + REQ-019 WAVE OPENED 2026-08-07 (user decided both).** (i) `garmin/req017-lifecycle-uaf`
(4 ahead / 0 behind) was **fast-forwarded onto `master`** — `master` is now `2f0de993d`, carrying REQ-017 (`ae5a7a8ab`),
its docs record (`3651de222`), the ORCH-001 build fix (`427da745b`) and its docs record. Landed via `git branch -f
master HEAD` + checkout rather than a merge-from-master, deliberately: 52 pre-session dirty entries (Coach/Gui/CMake/
vcpkg/skill WIP) would have blocked a branch switch, and the ff-in-place touches zero working-tree files — all 53
entries verified preserved after (53 = the 52 + this cursor). **Nothing pushed**; local `master` is 50 ahead of
`origin/master`, which is the user's call. (ii) **DEC-029 ALLOCATED** (WIKI bumped next:garmin-030) for the REQ-019
Upload-UAF fix shape; `qgdw-scout` dispatched with a grep-verified briefing (upload-path lines re-verified on disk
today; Upload-capable services enumerated from `capabilities()` = ~11 shipped integrations incl. Strava via the
CloudService.h:104 base default, NOT Garmin-only — LSN-034 pre-flight honoured). Scout must also propose the elaborated
REQ-019 acceptance criterion. Known seam for the proof: `testGarminConnectSyncDialogClose` (unittests/Core/
garminconnect/CMakeLists.txt:1339-1457) is an EXISTING ASan+offscreen ctest target and the direct template; no Upload
ASan test exists yet. **SCOUT RETURNED + Verification-Gate PASS; DEC-029 ACCEPTED (Option B) and REQ-019 DISPATCHED to
`qgdw-builder` on branch `garmin/req019-upload-uaf` — see NEXT_GATE.** The scout FALSIFIED the orchestrator's own
briefing premise (LSN-034 recur:2, saves:1 miss:2): the pasted service list was built by a PROXIMITY grep
(`-A3 … | grep -B1 Upload`) which asserts co-occurrence, not membership — it wrongly named Azum/Nolio/Withings AND
GarminConnect itself as Upload-capable when all four omit the bit (`GarminConnect.h:76` = `Query|Download`, DEC-005).
Corrected membership: 6 explicit (RideWithGPS, CyclingAnalytics, Selfloops, SportsPlusHealth, TrainingsTageBuch, Xert)
+ 5 inheriting the `CloudService.h:104` base default (Strava, Dropbox, SixCycle, SportTracks, LocalFileStore) = 11.
**REQ-019 is therefore a NON-Garmin fix living in the Garmin ledger** — hence ORCH-008 (test-home misnomer, accepted).
Byproducts merged: DEC-029 full entry + trace DEC row + REQ-019 trace row + elaborated prd acceptance + ORCH-008
finding + WIKI bumps (DEC next:030, TEST next:T-081, ORCH next:009).
Prior (superseded): **ORCH-001 FIXED + COMMITTED `427da745b` 2026-08-07 (DEC-028) — the clean-checkout gate now RUNS, for the
first time in this project.** `git worktree`/`git archive` extract of the staged tree → configure+generate OK, lrelease
emits all 13 `.qm`, `GoldenCheetah` (27.9 MB) + 26 test executables link, **ctest 26/26**. Committed tree hash is
byte-identical to the verified tree (`d8dfe490…`), so evidence and commit cannot have drifted. **ORCH-001 turned out to
be 5 pre-existing causes, not the 2 on record** — the extra three (ORCH-005 translations/lrelease never ported from
qmake; ORCH-006 eleven TRACKED sources absent from the CMake lists so HEAD could not link; ORCH-007
`CMAKE_CXX_EXTENSIONS OFF` diverging the dialect from qmake and breaking the `QBluetoothUuid` link) were **invisible to
configure** and only a real build exposed them → LSN-043. 8 files, +233/-12, hunk-split out of two CMakeLists dirty with
unrelated Coach/libusb/Calendar work (the root hunk split BY HAND — git had merged the dialect change with an unrelated
`cmake_minimum_required` bump). **Deliberately NOT adopted:** the uncommitted `src/Train/KurtInRide.cpp` byte-order
rewrite — ORCH-007 shows that hazard is a build-configuration defect, so that workaround is now redundant and is its
owner's to drop. All other pre-session WIP untouched (53 dirty entries preserved through the pre-commit stash cycle).
LSN-044 captured (ORCH-002/003/004 were GUESSED for new findings and collided with three existing ones; the ORCH series
is now registered in WIKI REGISTRIES, next:ORCH-008; commit message amended, tree unchanged). LSN-001 scored its first
SAVE (the hook denied a `cp` clobber of `unittests/CMakeLists.txt`; honoured, not routed around).
Prior (superseded): **REQ-017 lifecycle changeset COMMITTED `ae5a7a8ab` 2026-08-07** on branch `garmin/req017-lifecycle-uaf`
(feat(garmin), 19 files, +5221/-33) — the epoch bind + the CloudServiceSyncDialog UAF-class fix (DEC-021..027),
path-scoped to Garmin. Staged as 17 whole Garmin files + 2 hunk-split patches (MainWindow.cpp uploadCloud+syncCloud
hunks, src/CMakeLists.txt the GarminAccountEpoch source line); the Coach hunks in those two files were LEFT
uncommitted (partial-stage verified — committed MainWindow.cpp carries zero Coach refs). Pre-commit clang-format
reformatted 6 Garmin files (cosmetic line-joins only, LSN-007 class) → re-staged, retry passed every hook. ORCH-001
caveat stated in the commit message (branch cannot clean-build-verify). **Docs-record commit (d) DONE `3651de222`
2026-08-07** (9 governance files: decisions/design/findings/prd/traceability + STATE/WIKI/lessons/architecture).
**REQ-017 is fully closed — feature + ledger both committed.** Branch `garmin/req017-lifecycle-uaf` is now
**2 ahead / 0 behind master and NOT merged**; working tree carries ZERO Garmin files (only the pre-session
Coach/Gui/CMake/vcpkg/skill dirt, left for its owners). F1/F2 became follow-up REQ-019 (Upload UAF, HIGH) + REQ-020 (OAuth-wizard UAF) — lifecycle
status in traceability.md; F3 tracked. Pre-session skill/tooling churn (c) LEFT to the user. LEAVE uncommitted: Coach
WIP, ORCH-001 CMake cleanup, unrelated pre-session dirt. NOTE: the scratchpad `COMMIT_PLAN.md` + patches this cursor
previously named were a prior session's and did NOT survive into this one — the plan was reconstructed from git + the
prior cursor. Prior (superseded): **COMMIT PLAN PREPARED + HANDED TO USER 2026-08-07 ("prepare the plan, you run it").**
Plan: branch `garmin/req017-lifecycle-uaf` → (b) whole Garmin files + 2 hunk-split patches [feat] → (d) governance
ledgers [docs] → (c) tooling churn = user's call. Prior (superseded): **SCOPE RESOLVED 2026-08-07 — ship the DEC-024..027 sync changeset now; F1/F2 routed to new REQ-019/020.
At the COMMIT GATE.** The sync-dialog UAF class is CONFIRMED CLOSED (A3-R027-CLOSURE, fresh-adversary verified) and the
changeset is verified done (26/26, app links). F1 (Upload)→REQ-019 HIGH, F2 (OAuth)→REQ-020, F3 tracked — all registered
as byproduct (prd.md stubs, trace rows, findings deferred, REQ next:garmin-021). NEXT: execute the commit gate — the
(b)/(c)/(d) split — but two constraints shape it and NEED a plan checkpoint with the user before touching git history:
ORCH-001 (master doesn't configure from a clean checkout) and the dirty pre-session Coach hunks in MainWindow.cpp /
src/CMakeLists.txt (hunk-split, NEVER git add -A — LSN-007/010). On master → should branch first. Prior (superseded): **DEC-027 A3 RE-CHECK RAN (2026-08-06) → the CloudServiceSyncDialog UAF class (DEC-024/025/026/027) is
CONFIRMED CLOSED for the sync/download path; found sibling F1/F2. Awaiting SCOPE decision.** Adversary mutation-killed all four new start() guards, confirmed no third
CloudServiceSyncDialog caller + clean db lifetime (sync class genuinely closed); orchestrator spot-checked F1 (CloudService.cpp:78-88/326-412
+ MainWindow.cpp:2548-2565) and F2 (AddCloudWizard.cpp:104-112/474-508) and CONFIRMED both — same class (unguarded
nested QEventLoop in a dialog parented under a WA_DeleteOnClose ancestor), different dialogs, out of REQ-017 scope.
F3 dormant (folder-browse, no service sets the flag), F4 informational (raw ASan exit-1 = intentional DEC-025 leak,
ctest Passed). Findings merged; LSN-041 (sibling-scan — would have mapped F1/F2 four DECs ago) + LSN-042 (dormant-hazard)
captured. **The DEC-024..027 SYNC changeset is DONE + shippable; F1/F2 do NOT block its commit (pre-existing).** NEXT:
user decides scope — ship sync + track F1/F2 as new REQs, or expand now. Prior (superseded): **DEC-027 BUILT + VERIFICATION-GATE PASS (2026-08-06, working tree) — A3-R026-F1 FIXED, BLOCKING CLEARED
(pending A3 re-clear).** `MainWindow::syncCloud` (MainWindow.cpp:2595-2598) converted to heap + WA_DeleteOnClose +
modeless open() (stack/exec GONE, no else-delete); CloudServiceSyncDialog BYTE-UNCHANGED. TEST-077 closed the
A3-R026-F2 fixture gap — all 4 previously-unreachable start() self-bails (:778/:782/:993/:1023) now RED-verified;
TEST-078 drives the syncCloud entry route. **Verification Gate PASS (independent):** ASan 11 slots green, Garmin 25/25,
full 26/26, app links; FILES == git status (MainWindow.cpp + test file only); OWN :1023 mutation → UAF at the syncCloud
entry slot (test:1348 `dialog->open()`), byte-restored (md5 `fa9851ac…`, `.orig` snapshot per LSN-038). Both dialog
construction sites now use ONE proven lifetime pattern; grep confirms no third caller. NEXT: A3 re-check on DEC-027 (the
mandated gate) — the 4th fix in this UAF class, and DEC-027 is the adversary's OWN prescribed fix, so closure is
expected but a fresh adversary is the judge. Prior (superseded): **DEC-027 ACCEPTED (Option A) + `qgdw-builder` DISPATCHED 2026-08-06 — build IN FLIGHT.** DEC-027 recorded (decisions.md + DEC index + slice row); registries
bumped DEC next:garmin-028, TEST next:garmin-T-079 (T-077 A3-R026-F2 fixture-gap coverage, T-078 syncCloud teardown
conditional). Builder briefed: convert the ONE call site (MainWindow.cpp:2571-2585), leave CloudServiceSyncDialog
unchanged, fold in the F2 fixture gaps so the 4 untested start() guards go RED, keep DEC-024/025/026 + TEST-070..075
green. On return: Verification Gate (ctest + app link, FILES vs git status, per-guard mutation for TEST-077, LSN-032
snapshot with a `.orig` suffix per LSN-038). Prior (superseded): **DEC-026 A3 RE-CHECK RAN (2026-08-06) → NEW BLOCKING A3-R026-F1 (a FIFTH route, the STACK caller). DEC-026's
construction fix VERIFIED SOUND, but the class is NOT closed — awaiting user disposition on DEC-027.** The fresh
adversary confirmed DEC-026 closes A3-R025-F1 (independently reproduced guard :764 load-bearing) but found `MainWindow::syncCloud`'s
STACK dialog, parented to the `WA_DeleteOnClose` MainWindow, is a bad-free/double-destruction on parent teardown no
internal guard reaches (spot-check CONFIRMED the parenting chain). Also surfaced A3-R026-F2 (4 of 5 start() self-bails
untested — fixture can't reach the branches; recommend folding the fixture gaps into DEC-027's test). Findings merged;
LSN-039/040 captured. NEXT: present DEC-027 (three options) to the user. Prior (superseded): **DEC-026 BUILT + VERIFICATION-GATE PASS (2026-08-06, working tree) — A3-R025-F1 FIXED, BLOCKING CLEARED
(pending A3 re-clear).** Two-phase init landed: ctor (CloudService.cpp:726-742) is now a widget SHELL with no nested
loop; new `bool start()` (:754) carries the `store->open()`-onward body with a `QPointer` self-bail after EVERY
nested-loop call (open/2×msgBox/processEvents/refreshClicked) and results in locals (LSN-037). Callers updated:
MainWindow.cpp:2584 `if (sync.start()) sync.exec();`, AddCloudWizard.cpp:910 `if (syncnow->start()) syncnow->open();`
(no else-delete — builder caught the briefing's literal `else delete` as a double-free CONTRACT-CONFLICT and correctly
matched today's queued-close teardown). DEC-024/025 dtor machinery BYTE-UNCHANGED; TEST-070..073 green. TEST-075 built
(ctor route ASan). **Verification Gate PASS (independent):** ASan 1/1, Garmin 25/25, full 26/26, GoldenCheetah links;
FILES reconciled (the 2 extra-dirty files are pre-existing REQ-017 scaffolding); OWN mutation neutering start()'s
post-open self-bail → ASan crash at CloudService.cpp:790, byte-restored (md5 `e417ef5b…`). **TEST-076 NOT written
(LSN-022 honesty):** builder proved by mutation the syncNext/downloadNext self-guards protect no member access today =
dead code; **user CONFIRMED accept-with-rationale 2026-08-06** — guards kept + marked UNTESTED-BY-DESIGN in code
(CloudService.cpp ~:1636/~:1727) citing A3-R025-F2, Watch entry added to wiki/architecture.md, finding recorded
accept-with-rationale; TEST-076 stays allocated-unused. (A duplicate B-R025-02 row I introduced during the findings
merge was caught and consolidated.) LSN-038 captured (snapshot suffix must be guard-recognized `.orig/.bak/.backup`; the `.ORIG_ORCH` denial was operator
error, NOT an LSN-036 recurrence — guard_selftest 46/46). Prior (superseded): **DEC-026 ACCEPTED (Option B, two-phase init) + `qgdw-builder` DISPATCHED 2026-08-06 — build IN FLIGHT.**
User chose B over A/C to close A3-R025-F1 (the ctor UAF route) structurally, and chose "add RED tests" for the
A3-R025-F2 dead-code guards. DEC-026 recorded (decisions.md full entry + DEC index row + slice row); registries bumped
DEC next:garmin-027, TEST next:garmin-T-077 (T-075 ctor ASan both-directions, T-076 syncNext/downloadNext guard RED
tests). Builder briefed: split the ctor (CloudService.cpp:709-974) at the `store->open()` boundary into a shell-only
ctor + a new `bool start()` slot (carrying the DEC-025 part-3 self-bail); update the TWO callers of record only
(MainWindow.cpp:2577-2578 stack+exec, AddCloudWizard.cpp:892-899 heap+open, open-failure cleanup preserved); DEC-024/025
machinery + TEST-070..073 stay green; extend the existing SyncDialogClose ASan target. On return: Verification Gate
(ctest Garmin+full + app link, FILES vs git status, orchestrator mutation reverting the split → ASan UAF, LSN-032
snapshot-restore — CloudService files dirty, `git checkout` FORBIDDEN). Prior (superseded): **DEC-025 A3 RE-CHECK RAN (2026-08-06) → NEW BLOCKING A3-R025-F1 (the constructor route). Awaiting user
disposition on DEC-026.** Fresh `qgdw-adversary` verified DEC-025 closes the two POST-construction routes (mutations
confirm parts 1+2 load-bearing; of part 3 only `refreshClicked`'s self-guard is load-bearing — syncNext/downloadNext's
survive removal = dead code today, A3-R025-F2) but does NOT close the class it claims: the CONSTRUCTOR (:709-974) runs
three blocking/nested-loop calls with no self-bail (F1, spot-check CONFIRMED). Refuted leads: writeFile deferral safe
across all 16 subclasses (F3), non-Garmin close path not regressed (F5), test geometry adequate (F4). Findings merged
to findings.md; B-R025-01 escalated to BLOCKING. NEXT: present DEC-026 (three options) to the user. Prior (superseded): **DEC-025 BUILT + VERIFICATION-GATE PASS (2026-08-06, working tree) — A3-R017b-F1 FIXED, BLOCKING CLEARED.**
TEST-072 (parent teardown mid-call, BOTH suspended frames: readFile/`syncNext` and readdir/`refreshClicked`) +
TEST-073 (positive control: an IDLE teardown still closes AND deletes the store, so the fix cannot degenerate into a
dtor that never deletes). No new file, no CMake change — both slots went into the existing ASan target. All three
DEC-025 parts landed; `closeAndDeleteStore` and DEC-024's `done()`/`closeEvent()`/`deferCloseIfBusy` are
byte-unchanged, TEST-070/071 still pass. **26/26 full · 25/25 Garmin · app links.** **O-R025-01 was RIGHT and was
proven by execution:** with the store guard alone the ASan test still trips — builder M3 gave
`heap-use-after-free READ of size 4 in ~BlockingCall` at `--dialog->blockingCallDepth`, freed by
`QObjectPrivate::deleteChildren()`. **My own two mutations (LSN-032 snapshot-and-restore, md5 `38a1aa24…` both
times, 26/26 after each):** removing the dtor depth guard → `heap-use-after-free READ of size 8` inside
`BlockingStore::readFile`; removing the `BlockingCall` null check → SEGV in `~BlockingCall` at CloudService.cpp:1040
← `syncNext`. Both halves are load-bearing. FILES matched `git status` exactly (3 modified, 0 created, 0 governance
writes). **Three things left open on purpose:** A3-R017b-F2 (`writeFile` symmetry) NOT folded in — TEST-074 allocated
but UNUSED, because builder M5 showed a guard on a call nothing blocks in is just untested code; **B-R025-01** (the
dialog's CONSTRUCTOR is still unprotected — it wraps its whole body in a `BlockingCall` and calls `refreshClicked()`
at :972, so a teardown there destroys a half-constructed object; no guard at this layer can help, needs its own DEC);
**B-R025-02** (`syncNext`/`downloadNext` self-guards are defence-in-depth with no RED — M5 removes `syncNext`'s and
the suite still passes; `refreshClicked`'s IS load-bearing via M4). LSN-037 captured. Prior (superseded): **DEC-025 WRITTEN + `qgdw-builder` DISPATCHED (2026-08-05) — the parent-teardown UAF (A3-R017b-F1).**
DEC-025 accepted option A, user-chosen and now recorded in decisions.md with a DEC index row (DEC-024 was missing
from that index too — LSN-035 recurrence, both backfilled). Registries bumped: DEC next:garmin-026, TEST
next:garmin-T-075 (T-072 parent-teardown ASan, T-073 not-busy positive control, T-074 conditional on folding in
A3-R017b-F2's upload symmetry). **Scope grew by one verified finding, O-R025-01:** declining to delete the store is
necessary but NOT sufficient — parent teardown frees the DIALOG too while its own member functions are suspended in
the nested `QEventLoop`. `BlockingCall::~BlockingCall` writes `dialog->blockingCallDepth` (CloudService.cpp:1011)
and may call `dialog->close()` (:1018) as the loop unwinds; `downloadNext` then resumes at `processEvents()` (:1608)
and `syncNext` at (:1521), both still walking `rideListDown`/`progressLabel`/`listindex` on a destroyed object. So
DEC-025 option A is three parts, not one: dtor declines the delete, `BlockingCall` holds a `QPointer` and no-ops when
the dialog is gone, and the three resuming frames bail on a null self. That dialog half is PRE-EXISTING (the dialog
has always been a child of `context->mainWindow`, CloudService.cpp:709); only the store half is ours. Builder briefed
to falsify the premise before building, to EXTEND the existing ASan target
`unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp` rather than create a file, to prove both
directions (restoring the unconditional `closeAndDeleteStore` must reproduce the ASan UAF), and under the LSN-032
snapshot mandate since both CloudService files are dirty. Baseline to hold: 25/25 Garmin · 26/26 full · app links.
Prior focus (superseded): REQ-017 Slice A DONE + Verification-Gate PASS; builder built Slice B. Slice A shipped the epoch bind: NEW `src/Cloud/GarminAccountEpoch.{h,cpp}` +
private-only additions to GarminConnect (`latchSession`/`sessionSuperseded`/`downloadResultStillWanted`,
`m_openedEpoch`/`m_openedUserId`); gates sit AHEAD of DEC-020's `accountStillConnected()`, which is KEPT as layer 2;
uid now consumed from the open()-time latch (closes A3-R012-F10). TEST-060..063, 5 of 8 slots RED pre-fix.
**ctest 19/19** (was 18/18 + 1 new target), GoldenCheetah links, FILES == git status exactly (3 new + 4 modified,
zero governance writes). **The clause-(a) proof is mine, not the builder's:** I mutated `accountStillConnected()` to
`return true` — neutralising DEC-020's guard exactly as the criterion demands — and the ENTIRE epoch target still
passed while `testGarminConnectConnectPersist` failed, so the mutation was live and the epoch stops the exploit on
its own. Restored via snapshot-and-restore per LSN-032 (GarminConnect.cpp was dirty — NO `git checkout`), md5
`963b918a…` matched, 19/19 re-confirmed. **Honest gaps recorded, not buried:** B-R017-01 (readFile has NO error
channel — `CloudService::readFile` takes no `errors` out-param, so clause (a)'s "Garmin-labelled error" is met on
readdir but only as `return false` on readFile), B-R017-02 (the LAZY latch — an instance never `open()`ed binds at
first use; A3-R017's mandated first probe), B-R017-03/04/05 (latch not cleared on close; `m_pendingStartTimes`
stale on re-latch; TEST-060 a1 passed pre-fix so it is a pin, not new coverage). Slice B (TEST-064) now building:
`~CloudServiceSyncDialog` + the `MainWindow::syncCloud`/`AddCloudWizard` leaks — the guaranteed teardown trigger
that makes clause (b)'s functional reading honest. Prior focus (superseded): DEC-021 accepted, Slice A dispatched. DEC-021 chose the EPOCH bind: a pure-Qt `GarminAccountEpoch` (`static QHash<configDir,
quint64>`) latched into `m_openedEpoch` at `open()`, bumped by `disconnectService()`, compared as an int in
`readFile`/`readdir` — zero disk I/O, so a live session dies by BINDING, not by DEC-020's per-call re-check (which
STAYS as layer 2; DEC-019 is NOT reopened; zero blast radius on the ~15 sibling services; fully testable on
`garmin-fast`). Scout draft Verification-Gate PASSED — I independently confirmed the two facts the whole design
rests on: `blockingDownload()` (GarminConnect.cpp:231) runs a **nested QEventLoop** (:239, 60s watchdog :260) so the
GUI pumps mid-download and any teardown MUST be queued (same-frame `delete m_client` = UAF), and **no cancel
primitive exists anywhere** (`cancel|abort|interrupt|requestInterruption` matches nothing in GarminWorker.{h,cpp},
IGarminDownloadClient.h, PyEmbeddedAdapter.h; DES-001 invariant 3 forbids `terminate()`). **REQ-017(c) therefore
NARROWED by user decision to discard-only** — a post-download pre-stage recheck drops a late result; the HTTP call
still runs to completion or its watchdog. Accepted, deliberately visible residual: an interruptible download path
needs its own REQ+DEC — do NOT let a later cycle re-read (c) as though cancellation were implemented. **Clause (b) is
read FUNCTIONALLY** (never outlives the owning window), which is what makes Slice B's dialog-dtor fix load-bearing
rather than optional. Slice A = TEST-060..063 (Garmin-local); Slice B = TEST-064 (ownership/leaks, the only part that
leaves src/Cloud). Prior focus (superseded): DEC-021 research in flight. REQ-017 = the honest remainder of DEC-020 Option A: bind session lifetime to the
account so a live pre-disconnect session dies by BINDING (not by DEC-020's per-call `accountStillConnected()` disk
re-check), tear down the garth session/worker, cancel-or-discard an in-flight download (closes the accepted TOCTOU
residual), latch the uid at open (A3-R012-F10), and stop `CloudServiceSyncDialog`/`MainWindow::syncCloud` leaking the
store (A3-R012-F12). Criterion written verbatim into prd.md (5 clauses a–e, outcome-shaped, explicitly NOT a
restatement of the DEC-020 guard); trace row added; WIKI registries bumped REQ→next:garmin-018, DEC-021 ALLOCATED→
next:garmin-022. **Blast radius reaches SHARED CloudService/GUI code (~15 sibling services), which is why DEC-021
precedes any build.** Scout briefed with the 12 verified code facts, the binding priors (DEC-019's fresh-instance
root cause, DEC-020's rejected Option A, DEC-002/DES-001 worker-thread constraint, the garmin-fast link constraint)
and told to flag honestly whether clause (c) — interrupting a blocking embedded-Python download — is even reachable.
Prior focus (superseded): **REQ-012 DONE + COMMITTED `f001c7d20`** (feat(garmin), 2026-08-03, 7 files, +845/-12, path-scoped Garmin
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

RESUME-NOTES (read if you are a NEW SESSION picking this up — written 2026-08-15):
1. **Everything needed is on disk. No agent context is required to continue.** The scout/builder/adversary contexts do
   NOT survive a session; do not try to message them, spawn fresh ones. Every result they produced is already merged
   into findings.md / traceability.md / lessons.md / this file, which is the point of the single-writer rule.
2. **THE WORK IS UNCOMMITTED. That is the real exposure.** Branch `garmin/req027-silent-stall`, 7 ahead of `master`,
   nothing pushed. **Recounted 2026-08-15: 66 dirty entries = 55 OTHER OWNERS' pre-session churn
   (Coach/Gui/CMake/vcpkg/skill) + 7 governance files + 4 ours** — `src/Cloud/CloudService.{h,cpp}`,
   `unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp`, and
   `unittests/Core/garminconnect/CMakeLists.txt` (the LSN-062 dual-backend ctest registration). The earlier note said 3
   and omitted the CMakeLists — the file that carries a MECHANISM. Corrected as ORCH-018's byproduct.
   **Never `git add -A`, never `git checkout --`/`git restore` on a tracked file, never `git stash` the tree** — LSN-032
   was written after exactly that destroyed a builder's uncommitted work here.
3. **THE PRE-COMMIT LINT IS GREEN AGAIN — this note is REVERSED as of 2026-08-15.** It previously said the lint was red
   on purpose (ORCH-015) and must not be silenced by rewording the prose. **That repair LANDED:** binding scope was
   added — an id and an assignment-shaped status pair only within one `(sentence, quotation-region)` scope, with a
   markdown table row counted as a single record — and the suite went 21 → **44 cases**. Re-verified by execution this
   session: `python3 scripts/ledger_drift_lint.py .` exits **0** on the real tree and `test_ledger_drift_lint.py` runs
   **44/44**. **ORCH-021, raised and CLOSED 2026-08-16 — for two sessions "green" was true of the WORKING-TREE copies
   and NOT of the gate:** `pre-commit` stashes unstaged work, so it ran the COMMITTED hook, which was still the
   pre-ORCH-015 lint and failed the docs commit on four of its own false positives. **The repair is now landed
   (`b1e4c4fad`) and the committed copy verified: md5 `1b7b256b…`, exit 0 on the real tree, committed test suite 44/44.**
   The check that finds this class, and the only one that does — run the COMMITTED artifact:
   `git show HEAD:.claude/hooks/ledger_drift_lint.py > /tmp/h.py && python3 /tmp/h.py .` → [[LSN-064]]. The rule the note was protecting still stands and is the reason it is worth re-reading: when this lint
   fires on prose you believe is correct, **repair the lint, never bend the sentence** — that is the
   precision-by-subtraction failure [[LSN-008]] exists to prevent. Residual false-positive cousins are carried as
   ORCH-016, non-blocking. Corrected as ORCH-018's byproduct; a resume note that tells the next session to expect a red
   gate on a green tree burns exactly the trust the note exists to create.
4. **The build tree is live** (`build/`, gitignored): `cmake --build build --target testGarminConnectSyncDialogClose`
   then run it with `QT_QPA_PLATFORM=offscreen ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1`.
   Baseline to hold, **updated 2026-08-15 after DEC-035**: ASan **51/51** under `offscreen`, ctest **27/27** across BOTH
   registrations, ambient wayland **51/51**, `GoldenCheetah` links. `src/Cloud/CloudService.cpp` md5 at the verified
   state is now **`7e92e51aa7b771948bca62ecfc7841e2`** — check it before trusting any of the numbers above.
   **CORRECTED 2026-08-16: the previously recorded `4e5a6e5a07f50fa17e32825f4aa48bb2` was the PRE-COMMIT WORKING-TREE
   state and no longer exists anywhere.** The file is now committed and clean; its blob has been identical since
   `3c7fa7385` (`git rev-parse 3c7fa7385:src/Cloud/CloudService.cpp` == HEAD's == `f7e0a25ae…`), so the content did not
   drift after the commit — the checksum changed AT commit time. The cause is almost certainly the pre-commit
   clang-format rewrite that fired in three prior waves; that is **REASONED, NOT VERIFIED**, because no `clang-format`
   binary exists in this environment (the standing [[LSN-012]] residual) and the pre-commit file is gone. All the
   numbers above still describe this tree. (The value before that, `704601760b…`, was the PRE-DEC-035 state.)
   **The recurring shape is worth more than the correction: a checksum recorded from the WORKING TREE is invalidated by
   the commit that lands it, because the pre-commit hooks rewrite the file. Record the checksum AFTER committing, or
   record the blob hash, which the hooks cannot change.** Same family as [[LSN-064]] — working-tree evidence and
   committed-artifact evidence are different claims. **Never quote a bare N/N without the backend it was
   collected under — [[LSN-062]] is a MECHANISM on this ledger.**
5. **Scratchpad contents are GONE on session end** — my `.ORIG_ORCH` snapshots (no longer needed; all restores are
   `cmp`-clean with zero residue) and the adversary's out-of-tree mutation harness. No governance file depends on those
   paths; PROBE-A/B/C/D results are recorded inline in findings.md. If you need the harness again, rebuild it.
6. **When checking a mutation, read the process EXIT CODE and the `SUMMARY: AddressSanitizer` line — not the test
   output.** A crashing process prints neither `FAIL!` nor `Totals:`, which is how B-R027-06 turned the strongest kill
   in the matrix into a reported "survivor".

NEXT_GATE: **USER'S CHOICE OF FOUR, none blocked by the other — the harness blocker is gone, so read-only scouting and
  bookkeeping are unblocked too.** (1) **`qgdw-scout` on the sort route** — settle amendment-vs-own-REQ, three real
  options, invariants, test plan, migration risk, acceptance criteria; read-only, dispatchable now. (2) **Clause (e)'s
  reachability half** — A3-R028c-F7 says the two "reasoned, not traced" guards ARE drivable through the seam this wave
  built, so this is a builder slice, not an accepted residual. (3) **A3-R028c-F4..F7 + B-R028-14's governance question.**
  (4) **`qgdw-librarian` Job 3** — WIKI REGISTRIES rollup, the one-line DEC index that has never existed, recomputed
  counts, active/superseded STATE separation; the in-flight files have now stopped changing, so its seam is OPEN. The
  user-deferred bulk findings/lessons row deletion is NOT in that scope.
  **Not yet run for this wave and owed before any release commit:** an incremental CLV over the `e48f7d123` changeset
  (VAL-018) and a clean-worktree configure+build of HEAD. Neither blocks the four above.
  Prior gate (DONE, `e48f7d123`): **THE ORCH-033 REPAIR GATE — harness only, both lifetime windows, mutation-proven.**
  **SCOPE CONTRADICTION RESOLVED 2026-08-22, in the PRD's favour:** `prd.md:154` states, in REQ-028's own acceptance
  cell, *"NOT in scope, tracked separately: S-R028-01 (the sort route) … user decision 2026-08-16 was to track, not fold
  in."* The prior NEXT_GATE sentence — *"REQ-028 closes only after that separate sorting slice"* — CONTRADICTED the
  acceptance criterion and is drift; the acceptance cell is the SSOT. **REQ-028 closes on clauses (a)–(e) alone.** The
  sort route is a SEPARATELY TRACKED requirement, and the open question is not whether it blocks REQ-028 but whether
  T-112's measurement (harm EXECUTED — items permuted, wrong row labelled, `writeFile` twice — but the synthetic header
  click never delivered under either backend, so the user-trigger stays an API-contract claim, B-R028-05) warrants its
  own REQ+DEC or an amendment. That is the scout's question, and it is queued BEHIND this gate, not ahead of it.
  **Clause (e) status, stated explicitly because the prior summary only tracked (a)–(d):** mutation half MET
  (each new guard individually mutation-killed, per the recorded orchestrator mutations); reachability half MET ONLY BY
  RECORDED EXEMPTION — two guards ship labelled *"reasoned, not traced"*, and A3-R028c-F7 now says that is a COVERAGE
  gap that IS drivable through the seam this wave already built. So clause (e) is **not fully discharged**, and closing
  REQ-028 requires either those traces or an explicit accepted residual.
  **THEN, and only after the in-flight files stop changing:** `qgdw-librarian` Job 3 (WIKI REGISTRIES rollup, the
  one-line DEC index that has never existed, recomputed counts, active/superseded STATE separation). The user-deferred
  bulk findings/lessons row deletion is NOT in that scope.
  Prior wording (superseded): **Stable-worklist sort-route decision/build. REQ-028 closes only after that separate sorting slice and final CLV.**
  **THEN, AT THAT SEAM AND NOT BEFORE: `qgdw-librarian` Job 3 (compaction), scoped to WIKI REGISTRIES rollup + standing
  up the missing one-line decision index (active/dormant split).** The budget breach is real and measured (BUDGETS,
  12.7x), but the skill's own rule is "before the NEXT feature wave" — compaction is scheduled work, and the wave
  currently in flight is still writing the exact files it would rewrite. The user's deferral of bulk findings/lesson
  row classification stands and is NOT in this scope.

NEXT_GATE-PRIOR-FIX-BRIEFING: **THE FIX GATE — `qgdw-builder` DISPATCHED 2026-08-21 on A3-R028c-F1 + F2 + ORACLE-F1, all three of which
share one root (a stale thing handed a freshly-valid generation stamp by a restart). IN FLIGHT.** T-128/T-129 allocated
to it, T-130 allocated CONDITIONALLY for ORACLE-F1. Briefed from the FINDING ROWS and the two written amendments
([[LSN-061]]), not from a fresh reproduction. **On its return: Verification Gate — re-run both backends myself, diff
FILES against `git status`, and mutate each new guard INDIVIDUALLY ([[LSN-059]]).**
**ONE BRIEFING PREMISE PRE-CORRECTED BEFORE DISPATCH → ORCH-032, a [[LSN-073]] SAVE.** The A3-R028c-F2 amendment
(`decisions.md:1499-1500`) states `batchGeneration` "is read at only `:962`, `:1994`, `:2517`". Grepped at pre-flight
rather than trusted: it is ALSO read at `:2207`, `:3133` and `:3275`. **The amendment's CLAIM survives** — all three
extra sites are inside the DRIVERS (`syncNext:2130`, `uploadNext:3112`), none inside the three completion slots — and
the two driver sites are in fact the IDIOM the fix must copy (snapshot `:2207`/`:3133`, compare `:2517`/`:3275`), so
the correction made the briefing better rather than blocking it. The builder was handed the correction explicitly and
told to treat every line number in the briefing as a claim to re-verify at the definition.
**Prior wording of this gate (pre-dispatch), retained because its acceptance evidence IS the gate's criterion:**
**Its acceptance evidence is now MECHANICAL and was created before the fix existed: the oracle and fuzzer slots
(TEST-126/127) must all pass**, which
means seed 32, the nine other INV-1 seeds, and seed 447 all pass — **and a full sweep of seeds 1..446+ must find
nothing new.** That is a far stronger criterion than any hand-written slot, and it is why the oracle was built first.
**Confirm explicitly whether F2's `batchGeneration` snapshot ALSO closes ORACLE-F1** (it plausibly does — the restart
bumps `batchGeneration` where it merely re-stamps `batchListGeneration`) **or whether the stale stack local needs its own
guard. If seed 447 goes green from F2's fix alone, say so; if not, it needs its own.**
**Both fixes remain AMENDMENTS, already written into `decisions.md`** — F1 a row-free tombstone (retire the NAME, drop
the pointer that must die; naïvely appending `inflight` at `:1559` re-manufactures TEST-125's UAF), F2 DEC-034's
existing `batchGeneration` guard applied to the three completion slots that never got it, **including the read-path and
`failedRead` twins, which ARE on the GarminConnect route.** T-128/T-129 allocated.
**Carry these residuals into the briefing** (the oracle builder disclosed all of them): INV-1's scoping reads
`downloading` as `cancelButton->isHidden()`, exact today at all five write sites but an invariant of production, not a
guarantee — **any future show()/hide() silently WEAKENS the oracle** · the oracle attaches lazily at first dispatch, so
**3 dispatches in `CloudServiceUploadDialog` runs are unattached and NOT covered** · write matching is by name, FIFO, so
two writes under one remotename are indistinguishable — DEC-037's own point about the wire · **INV-2 has never fired in
71 slots or 446 seeds and must be treated as UNPROVEN, not as a passing check.**
**After the fix:** Verification Gate → **a fourth A3, now with the fuzzer available to it** → B-R028-04 (deferred),
A3-R028c-F3..F7, A3-R028b-F4/F6/F7 → commit gate → clean-extract gate. **Commit only on the user's word.** Nothing is
committed, `master` is untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-ORACLE: **THE ORACLE GATE — `qgdw-builder` DISPATCHED 2026-08-21 to build TEST-126 (the continuous invariant
oracle) and TEST-127 (the randomized click-sequence fuzzer), IN FLIGHT. It is FORBIDDEN from touching
`src/Cloud/CloudService.{h,cpp}` — its job is to CATCH the two live blocking defects, not to fix them.**
**USER DECISION 2026-08-21: fix both blocking findings AND build the oracle** (the third option, stopping to reassess
the whole wave, was not taken). **Second decision: DEC-037's zero-service coverage is RECORDED as a known gap** rather
than closed by building a `LocalFileStore`-driven slot — the paragraph is in the entry.
**ORCHESTRATOR SEQUENCING CALL, made rather than asked: THE ORACLE IS BUILT FIRST, AGAINST THE UNFIXED TREE.** An
oracle nobody has seen fail is not evidence. **Its acceptance criterion is that it INDEPENDENTLY REDISCOVERS A3-R028c-F1
and/or F2 without being told the route** — the briefing describes those defects by their HARM only and explicitly
forbids seeding the sequence or writing a slot that drives them directly. **"It did not find them, here is what it
covered" is a legitimate reportable outcome and was briefed as such**, because a staged success here would be worse than
no oracle at all. The fuzzer must be SEEDED with the seed printed on failure — an unreproducible fuzz failure is a
rumour.
**EXPECT THE SUITE TO GO RED AND STAY RED.** The defects are still live; the builder was told NOT to disable, skip or
`QEXPECT_FAIL` anything to get back to green. **Turning it green is the NEXT builder's acceptance evidence.**
**BOTH FIXES ARE AMENDMENTS, NOT NEW DECISIONS — no scout round needed.** F1 keeps DEC-037's retire-and-match mechanism
and changes WHAT is retired (a row-free tombstone: retire the NAME, drop the pointer that must die — and naïvely
appending `inflight` at `:1559` would re-manufacture TEST-125's UAF, which is why the containers cannot merge). F2
applies DEC-034's EXISTING `batchGeneration` guard to the three completion slots that never got it — **including the
read-path and `failedRead` twins, which ARE on the GarminConnect route where the write path is not.** Both amendments
are written into `decisions.md`; T-128/T-129 allocated.
**Order after the oracle returns:** Verification Gate → dispatch the F1/F2 fix builder (it must turn TEST-126/127 green,
and that IS its acceptance evidence) → Verification Gate → **a fourth A3** → then B-R028-04 (deferred), A3-R028c-F3..F7,
A3-R028b-F4/F6/F7 → commit gate → clean-extract gate. **Commit only on the user's word.** Nothing is committed,
`master` is untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-STRATEGY: **AWAITING USER — A STRATEGY DECISION. → RESOLVED 2026-08-21: fix both + build the oracle; record DEC-037's service-coverage gap rather than closing it.** Nothing is running, nothing is blocked on evidence.**
Six adversarial cycles have now each found real defects behind a green suite, and **the last three each found defects
created or missed by the FIX for the previous one.** The two new blocking findings both need DEC work (F1: reopen
DEC-037 for a row-free tombstone — retire the NAME, drop the pointer that must die; F2: reopen DEC-034/DEC-036 for a
`batchGeneration` snapshot at each completion slot's entry, plus the read-path and `failedRead` twins).
**But the adversary's own answer to the standing question is the more important item, and I agree with it:** at six
interacting mechanisms, hand-enumerated click sequences have stopped being the right tool. It recommends a **continuous
invariant oracle** every existing run evaluates — `outstanding <= 1`, ticket conservation, restart idempotence — plus a
**randomized click-sequence driver** with the ASan process as a second oracle. **F2 would have been caught mechanically
in whichever run happened to drive the burst, instead of requiring someone to guess the route.** → [[LSN-077]]; this is
the LSN-001/008/062 escalation path (advisory → guard → MECHANISM) applied to test oracles instead of process.
**Also worth the user's attention: DEC-037's entire mechanism ships for TEN Upload-capable services and is exercised
against ZERO of them** — `GarminConnect.h:76` is `Query | Download`, so the fake `BlockingStore` is its only driver.
The wire behaviour the disproof rests on is asserted about eleven real services and measured against none.
**Remaining queue after whatever is chosen:** B-R028-04 (deferred), A3-R028c-F3..F7, A3-R028b-F4/F6/F7, the sort route's
driver half → commit gate → clean-extract gate. **Commit only on the user's word.** Nothing is committed, `master` is
untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-A3C: **THE THIRD A3 RE-CLEAR — RE-DISPATCHED 2026-08-21 after the first attempt died on an infrastructure error. → RETURNED: FINDINGS, TWO BLOCKING (A3-R028c-F1/F2), five non-blocking, nine refutations.** (The 2026-08-20 dispatch
DIED ON AN INFRASTRUCTURE ERROR — subscription access disabled mid-run — before doing any work; its only output was an
opening line. **Tree verified untouched afterwards before re-dispatching:** same four modified files, `CloudService.cpp`
still md5 `716406f7…`, zero `.orig` residue, HEAD unmoved at `da455c79e`. A read-only role that never got past orienting
leaves nothing, but it was checked rather than assumed.) Attacking against the full
SIX-MECHANISM stack (DEC-034 · DEC-036 · its invalidation amendment · the driver-entry and resumption compares · the
`saveRide` three-part guard · DEC-037's retired-ticket set). The wave has been amended THREE times since the last cycle.
**Pointed at the five self-disclosed residuals first** (B-R028-15 the retired path ignoring `aborted`, B-R028-16
TEST-115's flipped expectation, the never-emptied retired ticket and its untested eviction branch, the two untraced
`saveRide` guards, and the DEC-035 trade) **and then past them** — with an explicit task to enumerate the reachable
states of `inflight` × `retiredWrites` × `listGeneration` × `aborted` × `downloading` at each of the three completion
slots and three drivers, because a lifecycle with an unenumerated site is what found the first UAF.
**Told what NOT to re-report:** everything closed, plus the five items open BY DECISION.
**AFTER IT RETURNS the remaining queue is all user-facing:** disposition its findings → **B-R028-15** (does the retired
path preserve the "Aborted" label, or does DEC-037 record that it should not?) → **B-R028-14** (ratify the DEC-035
deviation at `saveRide` or open a DEC) → **B-R028-04** (the wedged dialog, deferred) → **B-R028-13** (trace or
permanently accept the two surviving guards) → A3-R028b-F4/F6/F7 → **the commit gate, then the clean-extract gate on a
clean checkout. Commit only on the user's word.** Nothing is committed, `master` is untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-DEC037: **THE DEC-037 BUILD GATE — `qgdw-builder` DISPATCHED 2026-08-19. → RETURNED, VERIFICATION GATE PASS on all three checks.**
It was queued behind the F1/F2 builder rather than run beside it: both touch `CloudService.cpp`, and builders
parallelise only on DISJOINT files. On return, run the Verification Gate.
**DEC-037 ACCEPTED 2026-08-19 (user chose Option A, the retired-ticket set, WRITES ONLY).** Four sites: retire an armed
write ticket in `downloadClicked`'s START branch (`:2029`), `retiredWrites.clear()` beside the Refresh disarm (`:1559`,
**MANDATORY — every retired ticket holds a raw row pointer, so omitting it MANUFACTURES an A3-R028-F1-shaped UAF**),
match retired-first in `completedWrite` (`:3333`) labelling only — no count, no re-drive — and the member plus a REWRITE
of the "a single ticket is enough" paragraph, which is now exactly the claim that changed.
**T-124 must run the route TWICE, once per ARRIVAL ORDER — that is what makes the exhaustion argument MEASURED rather
than reasoned, and it needs a completion-ORDER hook the harness does not have. Build the hook first; if it genuinely
cannot be built, leave the second order undriven and LABEL the claim, per the TEST-076/[[LSN-022]] precedent.** T-125
proves the Refresh clear is load-bearing under ASan. **The accepted residual must be ASSERTED, not hidden:** make the two
results differ and record which string lands on which row in each order.
**Also briefed:** TEST-115's same-row half must be RE-ARGUED — its exculpation rests on the one-shot `armed` bit being
the sole separator on the write channel, and after this it is not.
**After it returns:** Verification Gate → **a third A3 re-clear** (five in a row have found real defects here, and the
last one found three BLOCKING behind a 65/65 two-backend green suite) → then the queue that needs the user: **B-R028-14**
(ratify the DEC-035 deviation at `saveRide` or open a DEC), **B-R028-04** (the wedged dialog, deferred by decision),
B-R028-13 (trace or permanently accept the two surviving guards), A3-R028b-F4/F6/F7 → then the commit gate, then the
clean-extract gate on a clean checkout. **Commit only on the user's word.** Nothing is committed, `master` is untouched.

--- prior gate ---
NEXT_GATE-PRIOR-F1F2: **TWO AGENTS IN FLIGHT ON DISJOINT WORK (builder on F1+F2, scout on DEC-037). → BOTH RETURNED: builder Verification-Gate PASS; scout's draft accepted as DEC-037 Option A.**
**USER DECISION 2026-08-19: all three blocking findings go in THIS wave** (rather than splitting F1 into its own REQ) —
**and the deciding fact was that all three are DEC-036's own blast radius, not neighbours.** Pre-DEC-036, F1's route
labelled through `child(listindex-1)`, which re-reads the rebuilt list and is never dangling, so a Refresh during
`saveRide` cost a MISLABEL; the stored `inflight.row` converted it into a use-after-free. **Third instance of that same
conversion in this wave** ([[LSN-068]]). **Second user decision: B-R028-04 (the wedged dialog) is DEFERRED to its own
slice after this wave commits** — it is user-visible but neither memory-unsafe nor data-losing, its fix touches DEC-032's
idiom across ~16 services, and TEST-119/120 now deliberately pin the current behaviour so the fix must change them too.
**(1) `qgdw-builder` — A3-R028b-F1 + F2**, the two well-specified blocking fixes. F1: a `BlockingCall` + self-bail +
`listGeneration` re-compare between `saveRide` (`:2811`) and the row writes (`:2812`/`:2815`), with the builder asked to
reason explicitly about whether the `BlockingCall` is safe there given DEC-031's reaper. F2: one line each before
`:2463` and `:3113`, reusing the frame's existing `listgen` local — DEC-034's mechanism, not a new decision.
**T-121/T-122/T-123 allocated. T-121 requires ARMING the `ImportSeamStubs.cpp:423` `autoProcess` seam, and [[LSN-056]] is
binding: that stub file is compiled into multiple targets, so the arming must be inert unless enabled and EVERY target
that compiles it must be built and run — the builder was told to COUNT them rather than trust my number.** T-122/T-123
are two SLOTS, not two arms in one, per A3-R028b-F5. Hygiene folded in: F5 (split TEST-120's arms) and F8 (restate the
false closed-world "five delivery points" census in the test file AND in the production comments at
`CloudService.h:704-720` / `CloudService.cpp:3149`) — F8 because the builder is about to ADD two delivery points.
**(2) `qgdw-scout` — DEC-037**, the A3-R028b-F3 cross-list key collision. Read-only, so it runs safely alongside the
builder; the briefing fences it off `completedRead`'s `saveRide` window and the parse-failure branches, which are being
edited right now.
**I CORRECTED THE A3'S PROPOSED FIX BEFORE DISPATCHING IT, and this is the substance of the scout's brief:** the A3
suggested "a per-dispatch monotonic sequence number in the ticket, compared as a third clause" — **compared against
WHAT?** `writeComplete(QString id, QString message)` carries only those two values, so a sequence stored in the ticket
describes the LIVE transfer while the stale completion carries nothing to compare. **Any ticket-local field is blind to
two completions that are identical on the wire** — the same wall the original clause-(c) research hit with shared
scalars. The scout must answer "how is the ARRIVING completion distinguished?" for every option it proposes, and it is
pointed at abort-time accounting (known arrival-order weakness), the per-batch relay QObject (DEC-036's rejected
Option B, rejected on maintainability not correctness — and it closes this by construction), disarming on abort, and
changing the two lists' keys (probably disqualifying, since the remotename is what goes to the service).
**Order after both return:** Verification Gate on the builder → present DEC-037's options to the user → build it →
**another A3 re-clear** (five in a row have found real defects here) → commit gate → clean-extract gate on a clean
checkout. **Commit only on the user's word.** Nothing is committed, `master` is untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-SCOPE: **AWAITING USER — A SCOPE DECISION. → RESOLVED 2026-08-19: all three blocking findings in this wave; B-R028-04 deferred to its own slice.** Nothing is running,
nothing is blocked on evidence.** Three blocking findings are recorded and confirmed; what is open is how much of them
belongs to REQ-028.
**The scope question, stated honestly: REQ-028 began as "a row item held across a suspension must stay valid" and is now
FOUR layered mechanisms plus an amendment, with a fifth amendment (F3) and a new suspension-point FAMILY (F1) queued
behind it.** F2 is unambiguously this REQ's business — one line each, reusing an existing local, closing a route the F5
fix could not see. F3 is a DEC-036 amendment: same mechanism, a key that is too narrow. **F1 is arguably a different
REQ**: the hazard is in `saveRide`'s data-processor family, which no census in this project has ever classified as
suspending, its guard sits in a different function family, and its test requires arming a stub shared by three targets
([[LSN-056]]). Folding it in grows a wave that has already grown four times; splitting it out leaves a known
use-after-free uncommitted-but-unfixed, which must then be recorded as an open REQ rather than forgotten.
**Also still open and needing the user: B-R028-04** (the wedged dialog — DEC-032's convention across ~16 services, now
deliberately PINNED by TEST-119/120's second verdict, so the fix must change those slots too).
**Whatever is chosen, the order after it is:** build → Verification Gate → **another A3 re-clear** (five in a row have
found real defects here, and each of the last three falsified the suspension-point census by one more entry) → commit
gate → clean-extract gate on a clean checkout. **Commit only on the user's word.** Nothing is committed, `master` is
untouched, nothing is pushed.

--- prior gate ---
NEXT_GATE-PRIOR-RECLEAR: **THE A3 RE-CLEAR — `qgdw-adversary` DISPATCHED 2026-08-19. → RETURNED: FINDINGS, THREE BLOCKING (A3-R028b-F1/F2/F3), five non-blocking, eight refutations.** against the whole wave as it now
stands (DEC-034 + DEC-036 + the invalidation amendment + the F5 entry guards, four mechanisms layered in one dialog).
**This is a fresh attack on changed code, not a re-read** — the wave has been amended TWICE since the last A3, and that
A3 found a heap-use-after-free in code which had already passed a full Verification Gate with every guard individually
mutation-proven. The briefing says so, and names the two shapes that have repeatedly paid here: **a slot that passes for
the wrong reason**, and **a lifetime whose invalidation sites were never enumerated**.
**Pointed at:** the `rideCache->save()` PROXY (can it be broken without breaking the test?) · whether `saveRide()` pumps
events, which would move the F5 delivery window for any parseable download · TEST-117's STRUCTURAL assertion and whether
each premise is reachable-and-failable · **an enumeration of every reachable `inflight` state at each of the three slots
and three drivers**, since the last headline was exactly an unenumerated lifecycle site · the entry guards returning
`true`, which all four callers ignore, making "stood down" and "dispatched" the same value.
**Told what NOT to re-report:** the closed findings, and the four items open BY DECISION (the sort route's driver half,
B-R028-04, F6, F7).
**AFTER IT RETURNS, exactly two things stand between this wave and the commit gate:** (1) disposition whatever it finds;
(2) **B-R028-04, which is a DECISION, not a build** — the wedged dialog is DEC-032's stand-down convention shared across
~16 services, and the fix now also has to change TEST-119/120, which deliberately pin the current behaviour. Then the
commit gate, then the clean-extract gate on a clean checkout. **Commit only on the user's word;** nothing is committed,
`master` is untouched, nothing is pushed.

--- prior gate (the fix this re-clear is attacking) ---
NEXT_GATE-PRIOR-F5: **THE A3-R028-F5 GATE — `qgdw-builder` DISPATCHED 2026-08-19. → RETURNED, VERIFICATION GATE PASS on all three checks.** The last open finding from this
wave's A3. On its return run the Verification Gate. **This is an application of DEC-034's existing mechanism, NOT a new
decision** — the builder chooses between re-comparing `listGeneration` immediately before each tail re-drive, or giving
`downloadNext` its own snapshot and fixing the siblings' POST-refresh timing, and must justify the choice by DEC-035's
principle (the guard belongs where the unsafe action happens). **If neither closes it without a new member or a signature
change, the hatch fires — that would be a DEC, not a build.** T-119 (download route) and T-120 (sync twin) allocated;
**T-120 to stay UNBUILT with a recorded residual if the route is undrivable**, per the TEST-076/[[LSN-022]] precedent.
**Briefed differently in one deliberate respect, because of ORCH-030:** the briefing does NOT predict the mutation matrix.
It asks for what actually dies, including "nothing", and says a surviving guard must be labelled rather than explained
away. Also mandated: REDs under BOTH backends this time (the last run's were offscreen-only, B-R028-11).
**Then, in order:** a fresh **A3 re-clear** on the whole wave (four A3s in a row on this ledger have found real defects,
and this wave has now been amended twice since the last one) → **B-R028-04**, which needs a DECISION because it touches
DEC-032's stand-down idiom across ~16 services → then the commit gate, then the clean-extract gate on a clean checkout.
**Still open and NOT scheduled:** the sort route's driver half (user decision, tracked), A3-R028-F6 (no test drives
`LocalFileStore`, the service DEC-036's geometry rests on), F7, B-R028-08/10/11. **Commit only on the user's word.**

--- prior gate (the remediation this gate verified) ---
NEXT_GATE-PRIOR-AMEND: **THE A3 REMEDIATION GATE — `qgdw-builder` DISPATCHED 2026-08-18 on the DEC-036 AMENDMENT. → RETURNED, VERIFICATION GATE PASS on all three checks.**
On its return run the Verification Gate; nothing merges on the report's word. **The amendment is recorded at the end of
`decisions.md` and is an AMENDMENT, not a reopen** — "ARMED IS ONE-SHOT" stands, the entry simply never enumerated the
INVALIDATION half. **Two sites:** `inflight.armed = false;` in `downloadClicked`'s START branch beside `:1951` (safe —
that branch runs only when `downloading == false`; it disarms before the restarted batch's first suspension, closing
**B-R028-07**, and before any restart that arms nothing, closing **F1** and **F2**), and the same line beside
`listGeneration++` at `:1526` (a Refresh frees the row the ticket holds, so the ticket is meaningless from that instant —
this is what makes the dangling `inflight.row` UNREACHABLE rather than merely unread). **T-117/T-118 ALLOCATED, both
MUST BE RED against the current tree** — T-117 the UAF geometry premised on `isPoisoned(row0)` so it cannot pass by
never reaching the row ([[LSN-050]]), T-118 the no-Refresh twin on the bar invariant `value() <= downloadtotal`.
**Also briefed, both small and both A3-found:** widen or re-label TEST-087's vacuous `writeFileCalls == 1` premise (F3),
and correct the `:2999-3010` comment to "reasoned, not traced" while KEEPING the guard (F4).
**Mutation mandated with an honest-answer clause:** remove site 1 alone → T-117 and T-118 must die on their own
criteria; remove site 2 alone → **if nothing dies, say so and label site 2 "reasoned, not traced" rather than claiming
coverage it does not have** ([[LSN-063]]).
**Out of scope and still queued after this:** B-R028-04 (the wedged dialog, A3-confirmed), the sort route's driver half,
F5 (the tail-`processEvents` route — but the builder was asked to say whether site 2 closes it as a side effect), F6
(no test drives `LocalFileStore`), F7.
**After remediation:** re-clear with a fresh A3 (this ledger's pattern is that remediation waves get re-attacked, and
four A3s in a row here have found real defects) → then the commit gate → then the clean-extract gate on a clean
checkout. **Commit only on the user's word;** nothing is committed and `master` is untouched.

--- prior gate (the cycle that produced this remediation) ---
NEXT_GATE-PRIOR-A3R028: **THE REQ-028 A3 GATE — `qgdw-adversary` DISPATCHED 2026-08-18. → RETURNED: FINDINGS, ONE BLOCKING (A3-R028-F1), plus F2 blocking-by-the-same-root-cause and five more.** against the built,
Verification-Gate-PASSED working tree (DEC-034 + DEC-036 together, uncommitted). Read-only on production; snapshot
discipline mandated for any mutation it runs. **Pointed at B-R028-07 FIRST** (the never-disarmed ticket), then
B-R028-09 (TEST-087's fabricated completions — and a sweep for OTHER slots fabricating the same way), B-R028-08 (the
five surviving clauses: is `completedWrite`'s `isWrite` route correctly traced, and is any of the other four actually
drivable, which would make it a missing test rather than a residual), then B-R028-10 and B-R028-04.
**The briefing states the standing question rather than only the residual list:** three A3s in a row on this ledger have
found a real defect behind a green suite, and the four self-disclosed residuals are where the build already KNOWS it is
weak — so the interesting finding is probably elsewhere. Named as most likely: **a slot that passes for the wrong reason
under the new swallow logic** (one that used to exercise a path production now stands down on stays GREEN while
measuring nothing — which is exactly what TEST-087 turned out to be), and **whether `inflight.row` can be made dangling
on any route DEC-034's compares do not cover.**
**[[LSN-067]] APPLIED IN THIS VERY BRIEFING, one turn after capture:** the exact env line is quoted, including the
target's PINNED `ASAN_OPTIONS`, with the reason it must not be stripped. Baseline for the adversary to hold: 61/61 each
backend, EXIT=0, zero sanitizer summaries; ctest 27/27.
**After the A3:** disposition its findings with the user → decide **B-R028-07** (a DEC-036 amendment or a build-level
detail — it is a fifth site the DEC does not name) → cover **B-R028-04** → then the commit gate, then the clean-extract
gate on a clean checkout. **Commit only on the user's word;** nothing is committed and `master` is untouched.

--- prior gate (the build this A3 is attacking) ---
NEXT_GATE-PRIOR-DEC036BUILD: **THE DEC-036 BUILD GATE — `qgdw-builder` DISPATCHED 2026-08-18. → RETURNED, VERIFICATION GATE PASS on all three checks (contract, evidence, goal audit).** On its return run the
Verification Gate; nothing merges on the report's word. **DEC-036 ACCEPTED 2026-08-18 (user chose Option A, the
in-flight ticket) and the full entry is appended at the end of `decisions.md`; the DEC index row is in
traceability.md; WIKI REGISTRIES carry the accepted wording.** **TEST-113..116 ALLOCATED** — T-113 stale read
swallowed (3 reads for 3 rows, zero rows relabelled, bar at 0) · T-114 the HEALTHY-batch positive control, which is
what makes each of the four ARM sites individually mutation-provable rather than jointly ([[LSN-050]]/[[LSN-059]]) ·
T-115 the stale WRITE on the Upload tab incl. the same-row case where only the one-shot `armed` bit separates the two
transfers · T-116 buffer accounting (freed exactly once). **TEST-107 must be REWRITTEN OR RETIRED and the builder must
say which** — it pins the defect, so a green TEST-107 after this fix would mean the fix did not work.
**USER DECISION 2026-08-18 ON THE SORT ROUTE: take what Option A gives.** Its LABELLING half closes (the fix labels
through a stored row POINTER, and TEST-112 measured that a sort permutes positions while item addresses stay stable);
its DRIVER half (`for (int i=listindex; …)` at `:2013/:2321/:2681`) **stays positional and stays TRACKED as B-R028-05**.
Closing that is DEC-034's Option C, a deliberate future reopen — briefed as explicitly OUT OF SCOPE so the builder does
not quietly widen into it. **B-R028-04 is also out of scope and queued next** (DEC-032's idiom, ~16 services).
**Mandated in the briefing:** RED step shown per slot; individual (never simultaneous) mutation with the killing
assertion being the CRITERION's own; a call-site trace per guard; `cp`/`cmp` snapshot restore with `git checkout`
FORBIDDEN on a tree carrying uncommitted work ([[LSN-032]]); the binary run DIRECTLY under BOTH `offscreen` and
`minimal` with exit code + `SUMMARY: AddressSanitizer` grep instead of the test output's own line ([[LSN-062]]); no
bare N/N without its backend; **DO NOT COMMIT.** Baseline to hold: **ASan 57/57 both backends, exit 0, zero sanitizer
summaries; full ctest 27/27.**
**AND THE BUILD MUST DISCHARGE TWO INVARIANTS THE DEC REFUSES TO ASSUME:** (1) the UNWRITTEN
`readComplete`-returns-the-caller's-buffer contract at `CloudService.h:145` — written only for `readFailed` at
`:160-161` today — must be written down and asserted, because Option A makes it load-bearing; (2) ≤1 outstanding
transfer per live batch must be asserted across the existing suite, not assumed from the four dispatch-and-return
sites. If either fails, the hatch fires and the ticket becomes a set — which is a DEC revisit, not a build fix.

--- prior gate (the decision this build implements) ---
NEXT_GATE-PRIOR-DEC036: **THE DEC-036 DECISION GATE — SCOUT RETURNED 2026-08-18, VERIFICATION GATE **PASS**, PROPOSAL PRESENTED,
AWAITING THE USER'S CHOICE. → RESOLVED 2026-08-18: user chose **Option A** and, on the sort route, "take what the chosen option gives".** Three options, genuinely distinct in KIND: **A in-flight ticket**
(dispatch-side identity — buffer pointer for reads, remotename for writes, row POINTER for the label; deletes all seven
positional derefs; no signature/service/connection change) · **B per-batch relay** (a QObject owning the three
connections, killed on abort so a stale completion is never delivered; the only option correct under concurrent
transfers) · **C key-addressed rows + one-shot permit** (DEC-034's REJECTED Option B resurrected — therefore a DEC-034
REOPEN, not DEC-036, because it kills three of DEC-034's six compares). **Scout recommends A.**
**FOUR CLAIMS SPOT-CHECKED BY THE ORCHESTRATOR AT THEIR CITED LINES, not read from the report:** `LocalFileStore`
emits `writeComplete("")` **synchronously inside `writeFile`** (`LocalFileStore.cpp:155-186`) — so the one empty-id
service can never produce a LATE completion, which is what makes A's write half sound · the seven `child(listindex-1)`
sites are exactly `:2460/:2538/:2545/:2634/:2641/:2848/:2855` · `Strava::prepareResponse` (`Strava.cpp:869`) mutates
the buffer IN PLACE (`data->clear()/append()`, :989-990) and returns **the same pointer** to `notifyReadComplete`
(`:528`), so pointer identity survives the most transformative service · `decisions.md:1181` is verbatim DEC-034's
rejected Option B, scored 5/3/2/4, confirming C's reopen framing.
**THE BRIEFING'S OWN SHAPE 2 WAS KILLED BY THE SCOUT AND THE REASON IS WORTH KEEPING:** swallow-by-count is
**ARRIVAL-ORDER DEPENDENT** — with no ordering guarantee across `QNetworkReply` services, a live completion arriving
first is eaten by the counter and the stale one is admitted, wedging the live batch; and it cannot fix the LABEL either,
since whichever completion is admitted still writes through `child(listindex-1)`. It survives only as the **one-shot
permit** folded into A and C, where it is paired with identity instead of used alone. Sixth briefing premise this
ledger's delegation hatch has corrected.
**TWO OPEN QUESTIONS GATE OPTION A AND MUST BE ANSWERED BY THE BUILD, NOT ASSUMED:** (Q1) does any store ever deliver
`readComplete` with a pointer other than the one passed to `readFile`? — static audit says 11/11 return the caller's
buffer, but the contract is UNWRITTEN at `CloudService.h:145` (it is written only for `readFailed`, `:160-161`) and A
makes it load-bearing, so it must be written down and asserted; (Q2) can two transfers be outstanding on one LIVE
batch? — A's single ticket rests on ≤1, which holds today by the four dispatch-and-return sites, and if it ever exceeds
1 the ticket must become a set and B's score rises.

--- prior gate (the dispatch this gate verified) ---
NEXT_GATE-PRIOR-SCOUTDISPATCH: **THE DEC-036 DECISION GATE — `qgdw-scout` DISPATCHED 2026-08-18, IN FLIGHT.** User instruction was to keep
closing what is open rather than ship REQ-028 partially, so the clause-(c) shortfall goes to a decision instead of into
the ledger as an accepted residual. **DEC-036 ALLOCATED** (the option set may return it as a DEC-034 REOPEN — that is
one of the things the scout must state). The scout drafts; the orchestrator presents; nothing is decided by an agent.
**Briefed as ONE decision with two riders, so the user answers once:** the clause-(c) mechanism (per-transfer identity
vs abort-time accounting vs a genuine third shape), and — scored per option, not asked separately — whether it also
closes **B-R028-05** (the sort route, whose harm is executed and whose click trigger is not) and **B-R028-06** (the
shorter-rebuild null on the same still-open route), plus whether it worsens **B-R028-04** (the wedged dialog).
**Briefing built from FRESHLY RE-DERIVED offsets, not the recorded ones** — verified on disk 2026-08-18: `.h:644`
`batchGeneration`, `:690` `listGeneration`, `:691` `batchListGeneration`; `refreshClicked` :1516 with the bump :1526;
`downloadClicked` :1918 with :1941/:1951; `syncNext` :1996 (:2005/:2011, compares :2189/:2281); `downloadNext` :2319
(buffer `new QByteArray` :2382); `uploadNext` :2662 (:2675/:2679, compares :2730/:2804); `completedRead` :2430
(compares :2448/:2520); `failedRead` :2612 (:2630); `completedWrite` :2831 (:2844); connections :1067-1072.
**ONE PREMISE OF THE OPEN FINDING WAS FALSE AND WAS CORRECTED BEFORE THE DISPATCH → ORCH-027.** B-R028-03 said
`completedWrite` has no payload; `writeComplete(QString id, QString message)` is declared at `.h:243` and the SLOT at
:2831 merely discards an unnamed parameter. The surviving true claim is narrower: the id is empty on `LocalFileStore`
(:163/:170/:179/:183) though real on Strava/SportTracks/CyclingAnalytics/RideWithGPS/Selfloops. Uncorrected, it would
have costed the per-transfer-identity option as a ~16-service signature cascade on the upload half when it may be a
one-slot change. The finding's MEASUREMENT (TEST-107 still green after the fix) stands untouched.
**Also mandated in the briefing, because the scout has NO Bash ([[LSN-065]] — never mandate a step outside the role's
tool grant):** every code claim must be Read/Grep-verifiable with file:line, unverifiable ones must be labelled "not
verified", override counts must be re-derived by the scout rather than copied from my number, and it must not edit any
file. On return: verify the draft (three REAL options? scores justified? cascade concrete?), then the orchestrator
presents it to the user. **Nothing is committed and `master` is untouched — the Phase-2 work is still uncommitted on
`garmin/req028-row-lifetime`.**

--- prior gate (this is the decision the dispatch above is researching) ---
NEXT_GATE-PRIOR-USERDEC: **AWAITING USER — ONE DECISION, AND IT IS A DEC-SHAPED ONE. Nothing is running, nothing is blocked on
evidence. The Phase-2 guards are verified done; what is open is a piece of REQ-028's acceptance that DEC-034 cannot
reach.**
**The decision: how to close acceptance clause (c) — the abort/restart route where a late completion labels the live
batch's row and `completedRead`'s tail re-drives the loop (4 reads for 3 rows).** DEC-034's counter provably cannot: two
transfers are outstanding at once and the restart overwrites any shared scalar snapshot before the stale completion
arrives, so every scalar reads EQUAL at the compare. The two shapes the builder named, neither of which it was
authorised to invent: **per-transfer identity** (carry the buffer pointer, which IS unique per transfer, through to the
completion) or **abort-time accounting of outstanding transfers**. Both are mechanism choices → **this needs a DEC**,
either **DEC-036 (new)** or a **DEC-034 REOPEN** in the DEC-031-reopens-DEC-025 pattern. Present three scored options.
**Second, smaller call, and it can ride with the first:** **B-R028-05 / S-R028-01.** The measured harm on the sort route
is real and passes every guard, but the user-click trigger is still an API-contract claim, not executed. The user's
2026-08-17 rule was "if it reorders, DEC-034 reopens with executed evidence" — the sort DID reorder inside the loop, but
the CLICK did not deliver it under either backend, so the rule's antecedent is only half met. Options: fold the sort
route into whichever DEC closes clause (c) (it is the same "the row moved under me" family and the same fix — value/key
addressing — would close both), or trace the click on a real backend first.
**Once decided, the order is:** build the clause-(c) fix → **rewrite or retire TEST-107** (it is currently green
BECAUSE the defect is live; the fix must flip it, and that flip is the acceptance evidence) → cover **B-R028-04** (the
wedged-dialog state the fix introduced) and **B-R028-06** (the shorter-rebuild null on the still-open route) → then the
**A3** with `qgdw-adversary`, which should be pointed at B-R028-06 first. **Three A3s in a row on this ledger have found
real defects behind a green suite.** Then the commit gate, then the clean-extract gate. **Commit only on the user's
word** — nothing is committed and `master` is untouched.
**If instead the user wants to ship what exists:** the guards close the refresh route (the BLOCKING A3-R027-F2 UAF) and
are mutation-proven; clauses (a), (b) and (d) are delivered. That is a real improvement and is committable on its own,
with clause (c) carried as an open REQ. It is a legitimate choice, not a shortcut — but it must be recorded that REQ-028
would then be PARTIALLY closed, and TEST-107 stays green as the standing marker of what is left.

--- prior gate (the dispatch this gate verified) ---
NEXT_GATE-PRIOR-P2DISPATCH: **THE REQ-028 PHASE-2 BUILD GATE — `qgdw-builder` DISPATCHED 2026-08-17, IN FLIGHT.** On its return, run the
Verification Gate; nothing merges on the report's word.
**Contract check:** all mandatory fields present (`FILES`, `API-SURFACE`, `DEPENDENCIES`, `TEST-107 DISPOSITION`,
`TEST-112 RESULT`, and a non-empty `NOTES` — an empty NOTES is INCOMPLETE, not clean, and four consecutive builders on
this ledger have disclosed the weakest part of their own work). Ids only from the allocated set **TEST-108..112**.
**Evidence check — orchestrator-EXECUTED, never read from the report:**
1. Rebuild UNPIPED and read cmake's own exit code (a `tail`-ed build returns `tail`'s status — that error is on this
   ledger's record).
2. Run the ASan target **directly, no ctest environment, under BOTH `offscreen` and `minimal`**; read the process exit
   code and grep `SUMMARY: AddressSanitizer`, not the test output. Baseline 52 slots + the new ones. **Never record a
   bare N/N without its backend** — [[LSN-062]] is a MECHANISM here. Confirm the new slots **BY NAME**, not by the
   aggregate count.
3. **Mutate each guard INDIVIDUALLY** — the null check and the `listGeneration` compare are separate guards on separate
   routes, and a simultaneous revert proves only that some slot notices the aggregate ([[LSN-059]]). The assertion that
   kills each mutant must be the CRITERION's own. Snapshot `.orig`, restore, `cmp`, zero residue; `git checkout`
   FORBIDDEN — the CloudService files carry uncommitted work ([[LSN-032]]).
4. **Reachability, not just coverage** ([[LSN-063]]): each guard needs a call-site TRACE of the production route that
   reaches it, or an explicit "reasoned, not traced". A guard can be load-bearing against its own test and unreachable
   from production.
5. Diff claimed `FILES` against `git status` / `git diff --stat`. Scope is `src/Cloud/CloudService.{h,cpp}` +
   `unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp` (+ that dir's CMakeLists only if genuinely
   needed). **Anything touching `src/Gui/MainWindow.cpp`, `src/CMakeLists.txt` or root `CMakeLists.txt` is drift** —
   those carry another owner's uncommitted work. Reconcile the ~60 dirty entries: 6 ours, the rest untouched.
6. **This slice's whole point is a guard, so "tests pass" is not evidence** — the mutation in (3) is what makes it
   evidence. And REQ-028 changes what a frame may still be HOLDING across a suspension, so the acceptance evidence must
   be an EXECUTED test under ASan, never reasoning about Qt semantics.
**Goal audit:** re-read prd.md:154 clauses (a)–(e) VERBATIM and confirm each slot encodes the criterion as written, not
a weaker paraphrase — in particular that **TEST-110's positive control is intact** (without it every other clause is
satisfiable by refusing to act, [[LSN-050]]) and that **TEST-111 actually drives the parse-SUCCEEDS path** (the whole
point of B-R028-01 is that TEST-107's all-`.gcfail` fixture could not reach the persistent harm).
**Then, in order:** disposition **TEST-112's** result with the user — if the sort route reorders, **DEC-034 REOPENS**
and that is a decision, not a fix; if the click cannot be delivered, record the undrivable with its reason and Option C
stands. Then merge byproducts (trace rows for T-108..112, findings dispositions for B-R028-01/02 and S-R028-01,
registry bumps), then queue the **A3** with `qgdw-adversary`. **Three A3s in a row on this ledger have found real
defects behind a green suite — it is not a formality.** Commit only on the user's word.

--- prior gate (resolved 2026-08-16 — that wave closed, landed and pushed) ---
NEXT_GATE-PRIOR-WAVE027: **WAVE CLOSED, LANDED AND PUSHED. The clean-extract gate PASSED on the COMMITTED tree 2026-08-16; `master` was
then fast-forwarded to `82a0de52a` and pushed to the user's fork. Nothing is running, nothing is blocked, and the next
action is the user picking the next item from the queue below.**
**Push mechanics worth keeping, because the first attempt failed:** the `origin` https remote has NO credential helper
configured, so `git push` died with `could not read Username for 'https://github.com'`. `gh` IS authenticated
(FONSECAMVP, `repo` scope, keyring), so the push ran through a ONE-SHOT helper —
`git -c credential.helper='!gh auth git-credential' push …` — deliberately instead of `gh auth setup-git`, which would
have written a helper into the user's global git config as a side effect of a push. Next session: same one-shot, or ask
before persisting anything.

**CLEAN-EXTRACT GATE — PASS, run on `git archive HEAD` of `ac5d5f67d`, extracted to a scratch dir with its own fresh
build tree:**
**Evidence-vs-commit integrity was established BEFORE any build, not asserted after:** the extract was re-hashed with its
own throwaway index and came back `36fddc199d294f4cc77fb7b1eaf461e5049593da` — byte-identical to `HEAD^{tree}`. Every
number below therefore describes the committed tree and no other.
configure + generate exit 0 (Qt 6.8.2, C++17, Release, Ninja) with **Garmin Connect: ON** and **Unit Tests: ON** (note
`BUILD_TESTS` still defaults to **OFF**, so a gate that forgets it proves nothing about the tests) · **822/822 targets
built, zero errors** · `src/GoldenCheetah` linked, **27,988,096 bytes** · **26 test executables** · **`ctest` 27/27**
including BOTH registrations of the ASan target (`testGarminConnectSyncDialogClose` **and**
`…_minimal`) · the wave's own ASan target **51/51 under THREE backends run directly with no ctest
environment: ambient wayland (`WAYLAND_DISPLAY=wayland-0`) · `offscreen` · `minimal`** — exit 0 and **zero
`SUMMARY: AddressSanitizer` lines** in all three, read from the process exit code and the SUMMARY grep rather than the
test output ([[LSN-062]] honoured on the gate that used to be single-environment; the RESUME-NOTE-6 trap avoided).
**The DEC-035 slots were confirmed BY NAME, not by the aggregate count** —
`anAbortAlreadySetMustStopSyncNextIssuingTheDownload` (T-105) and
`anAbortAlreadySetMustStopDownloadNextIssuingTheDownload` (T-106), alongside the five earlier abort slots.
**Build completion was re-established, not read off a log line:** a second `cmake --build` returned `ninja: no work to
do` with an UNPIPED exit 0 — the prior wave's `BUILD EXIT: 0` was `tail`'s status, not cmake's.
**The governance mechanism was gated from the committed artifact too ([[LSN-064]], the ORCH-021 class):** run from
inside the extract, BOTH copies of the drift lint — canonical `scripts/` and the installed `.claude/hooks/` copy
pre-commit actually executes — exit **0** on the committed tree and are byte-identical (md5 `1b7b256b…`, the value the
RESUME-NOTE predicted); the committed test suite runs **44/44**. So the repair is green in the place it failed.
**Binary size differs from the working tree's 28,736,984 B and that is EXPECTED, not drift:** the working-tree build
carries the Coach/Gui owner's uncommitted changes. The comparable number is the prior clean gate's 27,984,000 B at
`4c3608e89`; this tree is 4,096 B larger, consistent with a two-guard production diff.
**Pre-existing warning, not ours, not fixed:** `GarminConnect.cpp:60` still uses the deprecated
`QDateTime::setTimeSpec(Qt::UTC)`. Cosmetic, predates this wave, left alone deliberately — same disposition as last wave.

--- prior gate (resolved 2026-08-16 — the clean gate PASSED) ---
NEXT_GATE-PRIOR-CLEANGATE: **THE FEATURE IS COMMITTED — `3c7fa7385` 2026-08-16 (user said "run it"). Branch `garmin/req027-silent-stall`,
now 8 ahead of `master`, NOT merged, NOTHING PUSHED — pushing stays the user's call.** 4 files, +2244/-33:
`src/Cloud/CloudService.{h,cpp}`, `unittests/Core/garminconnect/CMakeLists.txt` + `testGarminConnectSyncDialogClose.cpp`.
**The wave landed as THREE commits, not two:** `3c7fa7385` (feature), **`b1e4c4fad` (fix(workflow): ORCH-021 — landing
the ORCH-015 drift-lint repair, which had been living in the working tree only and therefore was NOT what the gate ran;
user-authorised when the docs commit exposed it)**, and this docs record.
**NO HUNK-SPLITTING WAS NEEDED — verified, not assumed.** As in the REQ-021 wave and unlike REQ-017/REQ-019, this
changeset reaches neither `src/Gui/MainWindow.cpp` nor `src/CMakeLists.txt`, so all four files were staged WHOLE by
explicit path. A scan of the committed added lines for `coach|anthropic|openai|gemini|libusb|calendar` returned **zero**.
**The Coach owner's work was proven intact, not hoped intact:** the seven at-risk files (`MainWindow.cpp`,
`src/CMakeLists.txt`, root `CMakeLists.txt`, `vcpkg.json`, `AnthropicClient.cpp`, `KurtInRide.cpp`, `application.qrc`)
were md5-baselined BEFORE staging and re-checked after **both** pre-commit stash cycles — all seven `OK` every time.
**clang-format DID rewrite the test file on attempt 1** and aborted the commit — the fourth consecutive wave it has done
this ([[LSN-007]]). Proven cosmetic the strong way: with all whitespace stripped the staged and formatted files were
byte-identical (md5 `68d08d3a…` both) and the 40 `#include` lines matched in content, count AND order, zero moved. Then
**re-verified BY EXECUTION anyway** before re-staging — rebuild, ASan **51/51 bare · 51/51 offscreen · 51/51 minimal**,
ctest **27/27**, `GoldenCheetah` links (28,736,984 B) — because "obviously cosmetic" has been wrong in this repo before.
**STILL OPEN, deliberately:** the clean-worktree build gate has NOT been run on the committed tree yet — every number
above was collected in the working tree, which is the blind spot that gate exists to close. That is the next gate.

--- prior gate (resolved 2026-08-16 — the commit ran) ---
NEXT_GATE-PRIOR-COMMIT: **REMEDIATION DONE + RE-VERIFIED BY EXECUTION 2026-08-15. AT THE COMMIT GATE. Nothing is running, nothing is
blocked, and the next action writes git history — so it waits for the user.**
**Green, orchestrator-executed after the edits (not read, and not assumed from "comments only" — [[LSN-007]]):**
ASan target **51/51 under bare/no-override, 51/51 `offscreen`, 51/51 `minimal`** · full **ctest 27/27** ·
`GoldenCheetah` links, 28,736,984 B. `CloudService.cpp` is now md5 `7e92e51aa7b771948bca62ecfc7841e2`.
**A3-R027c-F1/F2/F3 + ORCH-018 are CLOSED in the tree.** F1's remedy states the reachability and names its KIND
(distributed invariant across four callers, not local dead code) and explicitly forbids deleting the guards on the
strength of a mutation run. F2's misattributing sentence is replaced by what is actually true — there is no realistic
route to these guards, so it is covered nowhere because it does not exist. F3's assertion strings now read
`syncNext:2058` / `downloadNext:2317`, re-derived LAST after every other edit had settled and verified to land on
`if (aborted == true) {`.
**ORCH-020, and it is the substantive finding of the remediation: ORCH-018 was not one bad citation but SIX.** Fixing it
meant re-deriving every `file:NNN` in the DEC-035 blocks, and `:2507`, `:2088`, `:2151`, `:2372`, `:2432`, `:2626` ALL
failed to point at an abort read — they land on a `self.isNull()` guard, an unrelated branch, DEC-030 prose, a
`QByteArray` declaration and TEST-097 narrative. Two more (`:2214`, `:2277`) were stale before this session began.
**None was caught by the builder, the Verification Gate, or a fresh adversary, because nothing executes a comment** —
and they were plausibly correct when written, invalidated by the very wave that wrote them. So the fix was to remove the
CLASS: every volatile citation in those blocks now names a SYMBOL, with numbers kept only where the number is the
payload. **Disclosed against my own change:** these edits added ~30 lines above the completion slots and thereby aged
many pre-existing citations elsewhere (TEST-087/TEST-101, the DEC-024/025/030/031 blocks) by a further ~30 lines. Left
alone deliberately, on the ORCH-017 (v) precedent — another wave's fixture, changes test output, and a file-wide
renumbering does not belong in a changeset one step from a commit gate.
**WHAT THE COMMIT GATE MUST RESPECT HERE — read before staging anything:** 66 dirty entries, of which **55 are OTHER
OWNERS' pre-session churn** (Coach/Gui/CMake/vcpkg/skill). **Never `git add -A`; never `git checkout --`/`git restore`/
`git stash`** ([[LSN-032]] was written after exactly that destroyed a builder's uncommitted work here). Ours: 4 code
files + 7 governance files. Neither `src/Gui/MainWindow.cpp` nor `src/CMakeLists.txt` is in our set, so — as in the
REQ-021 wave and unlike REQ-017/REQ-019 — **no hunk-splitting should be needed, but that must be VERIFIED, not
assumed.** Expect clang-format to rewrite the two touched code files on attempt 1 (it has on all three prior waves); if
it does, the commit aborts and every prior green number is void until re-run ([[LSN-007]]).
**The adversary confirmed the mechanism on every axis it was sent to break:** both guards individually load-bearing
(neutralising `:2030` leaves 50/51 with TEST-105 the sole failure on its own criterion `readFileCalls == 0`; `:2268`
gives the identical result for TEST-106; nothing else in the suite moves either way) · baseline reproduced **51/51 under
bare/no-override, `offscreen` AND `minimal`**, both ctest registrations 2/2 · tree byte-identical afterwards
(`4e5a6e5a07…` / `7eefd7ae…`, zero residue, no scratch orphan).
**It REFUTED five hypotheses with evidence, which on this ledger is worth as much as the findings:** the row-item
lifetime hazard does NOT apply at these guards (nothing pumps between the `curr` fetch and the guard on any route — the
real REQ-028/DEC-034 hazard is index-based reads across a genuine suspension, a different window) · the above-the-buffer
placement claim HOLDS (`BlockingCall`'s ctor is pure bookkeeping) · [[LSN-058]] does NOT apply, because DEC-035's exit
sits strictly BEFORE the suspension, so there is no "between" region — both adjacent `self.isNull()` guards were
mutated individually and both survived, exactly as their own `UNTESTED-BY-DESIGN` comments claim · the `return true`
contract is inert (all four callers discard the bool; post-abort UI coherence is established synchronously inside
`downloadClicked`'s own abort branch) · the fixture-device disclaimer is HONEST (nothing in production connects to those
lists' `currentItemChanged`).
**A3-R027c-F1 is the finding of the cycle, and I ACCEPTED ITS TRACE AND REJECTED ITS REMEDY.** With `aborted == true`
neither guard is reachable through any production route in today's tree: all four call sites re-establish
`aborted == false` in the same statement block with no pump in between (`completedRead:2435`→`:2438/:2440`,
`failedRead:2495`→`:2498/:2500`, `completedWrite:2689`→`:2692`, `downloadClicked:1923`→`:1969/:1971`), and `syncNext`'s
only suspension-crossing `continue` (`:2195`) re-checks at `:2186`/`:2193`. **I opened all four sites and both continue
paths myself and CONFIRMED them.** The adversary then recommended relabelling both guards in the `UNTESTED-BY-DESIGN`
vocabulary the adjacent `self.isNull()` guards use. **That would be less accurate than what is there and would invite
the very deletion the guard exists to survive:** those guards are unreachable for a LOCAL reason a reader verifies in
fifteen lines, while DEC-035's are unreachable only because four REMOTE callers each happen to re-check — a distributed
invariant no reader of `syncNext` can confirm and any future fifth caller silently breaks. DEC-035's own comment already
names this ("true-by-luck … it holds only as long as every route into this loop happens to have read `aborted` on the
way"), so the code was never claiming a live reachable gap. Captured as **[[LSN-063]]**: mutation proves COVERAGE, only a
call-site TRACE proves REACHABILITY, and the two are indistinguishable from inside a mutation matrix.
**F2 and F3 are documentation defects in this slice's own new text**, both folded into one remediation with ORCH-018:
F2 — the block comment says "the realistic routes are TEST-093/094/097/099/102's subject", and not one of those five
reaches either guard (each is intercepted earlier by an older guard); given F1 the honest sentence is that there is no
realistic route. F3 — `"syncNext:2011"` / `"downloadNext:2225"` are baked into the new tests' assertion MESSAGE STRINGS
as stale pre-DEC-035 numbers, and `:2225` is not in `downloadNext` at all (it is `rideCache->save()` in `syncNext`'s
tail; `downloadNext` starts at `:2231`) — so a developer chasing a red TEST-106 by its own failure message lands in the
wrong function. Recorded as [[LSN-034]] REFINEMENT 7, **recur:10 and a MISS, not a save**: it shipped into the artifact
and the next cycle found it. New position for that guard: a `file:NNN` inside a STRING LITERAL is read only under
failure, when the reader is least able to doubt it.

--- prior gate (resolved 2026-08-15 — the cycle returned CLEAR) ---
NEXT_GATE-PRIOR-A3C: **A3-R027c DISPATCHED 2026-08-15 — `qgdw-adversary`, scoped to DEC-035 ONLY, IN FLIGHT. The user chose the
mandated FULL-rigor gate over going straight to the commit gate.** The wave's earlier A3 re-clear ran before DEC-035
existed, so its two new guards have never faced a fresh adversary. Nothing else is running; nothing is blocked.
**Finding namespace allocated: A3-R027c-Fn** (A3-R027 and A3-R027b are spent on earlier cycles of this same REQ).
**THE TREE IS DELIBERATELY FROZEN WHILE THE CYCLE RUNS.** The briefing passes coordinates into `CloudService.cpp` and
`testGarminConnectSyncDialogClose.cpp`, so neither file may be touched until the adversary returns — editing an artifact
while a cycle holds positions in it is [[LSN-034]] REFINEMENT 5 ("a position in a file you are still writing to is
invalid the moment you write"). ORCH-018 is therefore recorded-but-unfixed on purpose.
**Every fact in the briefing was opened at its definition before it was written** — the guards at `CloudService.cpp`
`:2030-2033` (syncNext, definition `:1976`, guarded transfer `:2046`) and `:2268-2271` (downloadNext, definition
`:2231`, guarded transfer `:2281`), the column headers at `:1104` / `:1172`, the discrimination at `:2330` / `:2469`,
and the `aborted` writers at `:1916` / `:1476`. That pass is what produced ORCH-018.
**What the adversary was told to attack, beyond "is it load-bearing":** (A) per-guard mutation with the killing
assertion named and confirmed to be the criterion's own (`readFileCalls == 0`, not the corroborating row label) —
[[LSN-059]]; (B) **the `return true` contract** — REQ-027 exists because a branch that returns without arming a
re-entry point stalls a batch silently, and intended-stop looks identical to silent-stall from inside the function, so
every caller and every post-abort UI/state contract gets enumerated; (C) [[LSN-058]] guards promoted from dead to
load-bearing by a new early exit and therefore ABSENT FROM THE DIFF, including the `UNTESTED-BY-DESIGN` comment at
`:2288`; (D) [[LSN-060]] is `curr` provably alive where the guards write to it, given `refreshClicked` deletes every
item; (E) the above-the-buffer placement claim and the "nothing between the guard and the transfer pumps events" claim;
(F) whether the comments' disclaimers under-claim; (G) whether the synchronous `currentItemChanged` fixture device
leaves a realistic route uncovered by TEST-105/106 AND by every existing slot.
**Environment discipline is mandatory, not advisory** — direct run with no override, then `offscreen` AND `minimal`,
every number labelled with the backend it came from, mutation verdicts read from the EXIT CODE and the
`SUMMARY: AddressSanitizer` line rather than test output. [[LSN-062]] became a MECHANISM one day ago precisely because
a fresh adversary running the un-pinned command found a COMMITTED test failing 47/2 while every gate reported green.
Snapshot discipline restated in the briefing verbatim: every file involved is dirty and uncommitted, so `cp`/`cmp` only
and no `git checkout --` / `git restore` / `git stash` under any circumstances ([[LSN-032]]).
**ON RETURN:** Verification Gate (spot-check 1–2 cited findings at their cited locations before accepting the verdict —
this ledger has had an adversary frame a real defect through a route that was not its cheapest trigger, [[LSN-055]]),
then merge findings, then disposition with the user. The remediation step also carries ORCH-018's one-number comment fix
plus a re-run of the target, because "comments only" has been wrong in this repo before ([[LSN-007]]).
**AFTER THAT: the commit gate, which stays the user's call — it writes git history.**

--- prior gate (resolved 2026-08-15 — the user chose the A3 re-clear) ---
NEXT_GATE-PRIOR-DEC035: **DEC-035 BUILT + VERIFICATION-GATE PASS 2026-08-15. The gate→fixture→decision order the user authorised is
COMPLETE. One thing stands between this wave and the commit gate, and it is a rigor question, not a defect: the wave's
A3 predates DEC-035's code.**
**What landed:** TEST-105/106 RED-verified before the fix on the criterion's own assertion (`readFileCalls == 0`), then
two guards added immediately ahead of the transfers at `syncNext` and `downloadNext`. Abort reads per function are now
syncNext 3, **downloadNext 1 (was ZERO)**, uploadNext 2 — the upload/download asymmetry that S-R027-01 closed on one
side is now closed on the other. ASan target **49 → 51**, ctest **27/27 across BOTH backend registrations**, ambient
wayland **51/51**.
**My gate evidence, executed not read:** `CloudService.h` byte-identical (md5 `7eefd7ae…`, DEC-035 is .cpp-only) ·
both ctest registrations green · wayland 51/51 · **my OWN mutation script, my own anchors, run individually: deleting
the sync guard kills ONLY TEST-105 (50/1), deleting the download guard kills ONLY TEST-106 (50/1)** — so neither test is
killed by the other's guard nor by the completion-slot guards a few lines away, which was the specific trap here
([[LSN-059]]) · both killing assertions are the criterion's own number.
**THE BUILDER CAUGHT A DEFECT IN MY BRIEFING THAT WOULD HAVE SHIPPED SILENTLY.** I pasted a literal snippet using
`setText(7, …)` for BOTH sites. **Column 7 is wrong on the download list** — it has six columns with Status at 5
(`:1104-1106`) against the sync list's eight with Status at 7 (`:1172`), and production already discriminates with
`int col = sync ? 7 : 5` (`:2330`/`:2462`). The label would have gone to a column the view does not display, and no
read-count assertion would ever have noticed. Caught only because the same briefing also said "verify the column
yourself", and the builder followed that over my literal text → **[[LSN-034]] recur:9**: a pasted snippet is an
unverified premise at every site but the one it came from, and the "verify it yourself" clause is a checksum — never
drop it as redundant just because the briefing also supplies the answer.
**Two comments that DEC-035 made FALSE were amended by me, not left:** the `completedRead` guard's comment
(`CloudService.cpp`) and its mirror in the test file both stated that neither download-path function reads `aborted`
before its transfer. True when written, false now. Amended in place with the date and the reason rather than silently
rewritten — a comment that quietly stops being true is what this wave spent three cycles learning to distrust. Re-ran
the full target after the edits (51/51) rather than trusting "comments only", because this repo has been wrong about
that before ([[LSN-007]]).
**NEXT, and it is the user's call:** the A3 re-clear was run against the REQ-027 slice BEFORE DEC-035 existed, so the
new guards have never faced a fresh adversary. At RIGOR:FULL the mandated gate is an A3 scoped to the DEC-035 addition;
the alternative is to go straight to the commit gate on the strength of the individual mutation proofs. Committing
writes git history and stays the user's decision either way.

--- prior gate (resolved 2026-08-15 — both phases landed and verified) ---
NEXT_GATE-PRIOR-FIXTURE: **GATE-WIDENING + FIXTURE REPAIR DISPATCHED 2026-08-15 — `qgdw-builder`, TWO PHASES, RED STEP MANDATED.
User said "proceed in the order you recommend"; the order is GATE → FIXTURE → DECISION, and the reason is not
ceremony.** Fixing the fixture and validating it under `offscreen` would prove nothing, because `offscreen` is the
backend that HIDES the defect — the gate has to be able to see the failure before any fix means anything. The DEC goes
last because it is genuinely the user's call and must not block a fixture repair that stands on its own.
**PHASE 1 — widen the gate, MUST GO RED.** A second ctest registration of the same executable under
`QT_QPA_PLATFORM=minimal`, alongside the existing `offscreen` one at `unittests/Core/garminconnect/CMakeLists.txt:1454-1457`,
with a comment recording WHY the target is registered twice. `minimal` chosen deliberately: headless and therefore
CI-portable, unlike wayland. `ctest` must now FAIL — a Phase 1 that stays green has widened nothing.
**PHASE 2 — fix the FIXTURE, not production.** Hard scope wall: `src/Cloud/CloudService.{h,cpp}` are OFF LIMITS, and if
the only way to pass is to change production, the builder must STOP and report — that outcome is DEC-035, not a fix it
may make. The rule the repair must satisfy: queue order guarantees RELATIVE order of delivery, never delivery WITHIN a
nominated `processEvents()`; the fixture must make the abort/teardown ALREADY APPLIED, not merely pending. Also
mandated: correct TEST-102's comment at `testGarminConnectSyncDialogClose.cpp:508-514` — its "by ordering rather than by
timing" claim is measurably false and is precisely what made this finding hard to see, and its embedded line numbers are
stale (flagged as stale in the briefing rather than passed on). And prove each fixed slot STILL kills a broken guard by
out-of-tree mutation — a fixture that passes because it stopped exercising the guard is worse than the flake.
**Validation demanded before any green is accepted:** full-target runs under `offscreen`, `minimal` AND ambient wayland,
plus `ctest`; wayland run TWICE, because that is where TEST-102's flake lives. Both measured traps were written into the
briefing: the failing test PASSES when run alone (5/5) so only full-suite runs count, and an ASan abort prints neither
`FAIL!` nor `Totals:` so classification is by exit code + `SUMMARY:` line.
**DEC-035 ALLOCATED (not yet researched), the H2 residual:** `processEvents()` is not a guaranteed drain, so DEC-032's
abort guard is best-effort BY CONSTRUCTION — it stops the batch whenever the abort has landed and cannot when it has
not. If REQ-027's criterion means "an abort pressed before the next transfer starts must stop it", the placement immune
to delivery timing is at the START of the next transfer (before `store->readFile`, `CloudService.cpp:2225`/`:2011` (NOT the `:2220` the production comment at :2367 states — verified at the definition, [[LSN-034]])),
not only at the completion slot's tail. Deliberately NOT dispatched yet: Phase 2's outcome informs its shape.

--- prior gate (resolved 2026-08-15 — diagnosis returned H1) ---
NEXT_GATE-PRIOR-DIAG: **A3-R027b-F1 DIAGNOSIS DISPATCHED 2026-08-15 — `qgdw-builder`, DIAGNOSE-ONLY, EXPLICITLY NOT AUTHORISED TO
FIX. ORCH-015 is CLOSED. The commit gate is shut until F1 is answered.** User chose the bounded diagnosis over fixing
blind. The single question: **H1 the FIXTURE cannot reliably place its simulated abort where it intends (queue order is
not a cross-backend guarantee, production is fine), or H2 DEC-032's guard shape depends on a Qt delivery semantic it does
not control (a design-level gap needing its own decision).** Those have completely different fixes, which is exactly why
no fix was authorised before the answer. Method mandated: instrument OUT OF TREE, log the write of `aborted`, the
delivery of the queued action and each guard's read, then diff the EVENT ORDER between `offscreen` and `minimal` — the
answer lives in that diff. Also asked for: verify-or-refute my reasoning that our diff only ADDS early returns and so
cannot have turned a 1 into a 2 (⇒ the committed slot was already failing at HEAD under non-offscreen backends), with
the HEAD-build left to the builder's cost judgement rather than mandated; and A3-R027b-F2 only if the instrumentation
answers it for free. The stale line numbers inside the fixture's own comment were flagged in the briefing as stale
rather than passed on as fact ([[LSN-034]] discipline, and I re-read the comment verbatim before quoting it).

--- prior gate (both agents returned; ORCH-015 closed, A3 returned FINDINGS) ---
NEXT_GATE-PRIOR-DISPATCH: **REQ-027 A3 RE-CLEAR + ORCH-015 REPAIR — BOTH DISPATCHED 2026-08-15, RAN IN PARALLEL ON DISJOINT
FOOTPRINTS.** User chose the re-clear first, with the lint repair alongside it (the lint blocks the commit, not the
cycle). Two agents are in flight; nothing is blocked on the user.

**(1) `qgdw-adversary` — A3 re-clear, finding prefix `A3-R027b-Fn`,** against the remediated working tree (3 code
files, uncommitted). Briefed read-only on tracked files with the previous A3's out-of-tree mutation technique
(patched TUs relinked against existing objects, `git status` untouched) and with the B-R027-06 correction stated as a
constraint: **classify a mutation by EXIT CODE + the `SUMMARY: AddressSanitizer` line, never by parsed QTest output** —
an ASan abort prints neither `FAIL!` nor `Totals:`, which is how the strongest kill in the builder's own matrix was
first scored a survivor. Charged specifically at: **`batchGeneration` as a NEW INVARIANT** (one monotonic int, bumped
in exactly one place at :1931, snapshotted at :1985/:2454, read at :2158/:2568 — while `downloadNext` and all three
completion slots take no snapshot at all); whether the `continue` terminates under every reachable combination and
cannot re-enter unboundedly; whether the new tests encode the criterion or a weaker paraphrase, and whether any is
green for a reason other than the one it names (the B-R027-07 class); **individual** re-mutation of the guard
inventory it derives itself; and whether the three new read-side `return`s can leave a batch half-done with no
completion tail — REQ-027's own failure mode, reintroduced on the read side. Out-of-scope-but-report-if-worsened:
A3-R027-F2 (→ REQ-028/DEC-034), F3/F8, F7/F9, B-R025-03, A3-R021b-F2. Refutations-with-evidence count as findings.

**(2) `qgdw-builder` — ORCH-015,** scoped to exactly two Python files (`scripts/ledger_drift_lint.py` + its test),
explicitly barred from `.claude/hooks/`, governance, `src/` and `unittests/` (the adversary is in there). **The lint is
now RED on THREE lines, not one** — `STATE.md:536`, `:580`, `:591` — and all three are false positives of the same
shape. `:591` is the cleanest: `47/47 STILL GREEN` is an assignment ABOUT THE TEST SUITE while `DEC-030` is an
unrelated parenthetical citation that happens to land on the same wrapped line. `:536`/`:580` are prose *about this
very false positive*, so bending them is not available. **Diagnosis handed over as a hypothesis to falsify, not a
prescription:** `scan_file` (:221) flags pure co-occurrence of `find_ids` and `find_statuses` with no relationship
required; ORCH-010 already taught `find_statuses` (:205) to reject adjectival status words via `_is_adjectival`, so the
residue looks like **subject binding** — the status is assignment-shaped but its subject is not the id on the line.
Held to ORCH-010's standard: RED first with the three real lines quoted verbatim, all 21 existing cases still green,
new cases proving GENUINE drift is still caught (a repair that silences the false positive by silencing real drift is a
regression), and a self-mutation showing the new discrimination is load-bearing. Blanket suppression, a STATE.md
exemption, and a magic-comment escape were all ruled out in the briefing — LSN-036 class, the mechanism gets repaired,
never routed around.

**ORCH-015 RETURNED + VERIFICATION-GATE PASS 2026-08-15 — AND THE GATE FOUND THE HALF I NEVER BRIEFED. Re-dispatched;
the COMMIT BLOCKER IS CLEARED either way** (`python3 .claude/hooks/ledger_drift_lint.py .` → exit 0 on the real tree,
which is the exact entry point `.pre-commit-config.yaml:50` runs). The builder added BINDING SCOPE — an id and an
assignment-shaped status pair only when they share a `(sentence, quotation-region)` scope, with a markdown table row
deliberately treated as ONE record and therefore one scope. 21 → 37 cases.
**My evidence was executed, not read:** FILES reconcile exactly (2 modified, 2 gitignored `.orig`) · 37/37 · **a 16-line
drift corpus I wrote MYSELF rather than reusing the builder's test file**, over which the old-vs-new diff leaves every
genuine shape caught — and on the one corpus line carrying a real status assignment AND a suite adjective in separate
sentences, the lint keeps the first and drops the second, which is the sharpest proof available that this is subject
discrimination and not suppression (the literal lines live in the ORCH-015 row) · **recall delta on the
REAL in-scope corpus is exactly the four false positives, zero true positives lost** · my own mutation forcing
`_scope_map` flat restores all four suppressions AND kills 7 of the 37 cases · my own mutation disabling `_is_table_row`
MISSES a genuine drift row whose status cell is backticked, which independently justifies the exception the builder added
on its own recall measurement. Installed copy synced by me via `install_hook.py --extra-hook` — chosen over a `cp` after
verifying the installer's other effects were byte-identical no-ops (guard + all 5 agent defs), with `settings.json`
snapshotted first and `cmp`-verified unchanged after.
**THE GATE'S REAL CATCH, AND IT IS MY MISS:** I briefed the repair from the four false positives the lint was FIRING
that morning — all of ONE shape — instead of from the ORCH-015 findings row, which records **TWO**. Sub-defect (b), the
PREDICATE-ADJECTIVE position (a gap inside the ORCH-010 repair itself), does not appear in today's output at all, so no
reproduction from HEAD could ever have surfaced it. It still fires on both of the row's own predicate-position examples
(a status word in predicate position, followed by a full stop, with the id in a parenthetical citation in the SAME
sentence — verbatim lines in the ORCH-015 row), while the true assignment and the attributive case both behave
correctly. **(b) IS NOW CLOSED TOO — re-dispatch returned + Verification-Gate PASS 2026-08-15, ORCH-015 FIXED in full.**
The builder established that the three predicate constructions cannot be separated on the status side AT ALL, and found
the discriminator on the other side: a parenthesised id is a CITATION, and a citation is never the grammatical subject.
The rule is DIRECTIONAL and that is its whole load — an id inside an aside does not bind a status outside it, but a
status inside an aside DOES bind an id outside it, because that is the real drift shape. 37 → 44 cases.
**My evidence, executed:** 44/44 · repo lint exit 0 · my own 12-line probe reproduces all four of (b)'s recorded lines
plus the real-drift control · own mutation reverting the citation rule kills 3 cases · **own mutation making the aside
SYMMETRIC makes the real-drift control VANISH**, which is what proves the asymmetry is load-bearing and not decorative ·
the builder's monotonicity check over 8 governance files means (b) can only REMOVE findings, never invent them · recall
89 → 88 corpus-wide, the single loss inspected and correct. Installed copy re-synced, `settings.json` `cmp`-identical.
**Residual cousins recorded as ORCH-016 (OPEN, non-blocking, NOT scheduled):** the citation rule covers PARENTHESES
only, so preposition/possessive/**em-dash** asides still false-positive — em-dash being the commonest aside in these
ledgers and the largest remaining hole; an object-of-preposition over-report that predates ORCH-015; a table-row recall
hole; and one HONEST SURVIVING MUTANT the builder declined to fake a kill for, with its reasoning recorded. All four
were self-disclosed in the report's NOTES and then reproduced by me. **The general case is a parser problem, not a
missing heuristic — the copula heuristic was measured to lose real drift, so it is ruled out in writing.**
**The builder delivered exactly what I asked on the first pass, to a high standard, and the tree
went green — which is precisely how a narrowed briefing hides behind a passing gate.** Re-dispatched with the four
lines and their required directions. → **[[LSN-061]]**: brief from the FINDING ROW, never from a fresh reproduction; a
finding is FIXED only when every shape its own row records has a passing case.
**Also caught, by the builder, against me:** the briefing asked for a `DEC-030 = ACCEPTED` test case and **"ACCEPTED" is
not in `STATUS_TOKENS`** — it flagged the false premise instead of weakening the vocabulary to satisfy it. And I shifted
`STATE.md:580`/`:591` out from under the builder by editing this very file minutes after briefing it to quote those
positions. Both → **[[LSN-034]], now recur:8 saves:8** (a vocabulary is a premise too; a position in a file you are
still writing to is invalid the moment you write). Mechanism promotion for LSN-034 is overdue on the record.
**AND THE REPAIRED LINT IMMEDIATELY SCORED A TRUE-POSITIVE SAVE AGAINST ME.** Writing this very entry, I pasted four of
the corpus's literal drift lines into STATE as illustrations; the lint fired on all four, CORRECTLY — they are real
id+status pairings in a non-canonical file. The fix was NOT to bend the prose and NOT to widen the lint: verbatim
finding detail belongs in `findings.md`, which is the SSOT and is deliberately out of lint scope, while this cursor
POINTS at it ([[LSN-035]]). So the lint enforced the DEC-015 architecture against the orchestrator within minutes of
being repaired — the strongest argument yet for keeping it as a pre-commit gate. → [[LSN-008]] save.
**Accepted residual, builder-disclosed and orchestrator-confirmed:** a status value in backticks/quotes on a NON-table
line no longer binds an unquoted id (`REQ-004 status: `_uncommitted_`` is now missed). Real drift lives in table rows
and bare prose, both still caught — but it is a genuine precision-for-recall trade and is on the record, not buried.

**ON RETURN:** Verification Gate on both, independently — for the adversary, spot-check 1–2 cited findings at their
cited locations before accepting the verdict; for the builder, re-run the lint and its suite myself and re-derive the
two-directional matrix rather than reading it. Then I sync `.claude/hooks/ledger_drift_lint.py` from the canonical
`scripts/` copy (deliberately NOT delegated — it is the installed copy and the builder's footprint stays tight). The
commit gate opens only if the re-clear comes back CLEAR and the lint exits 0.

--- prior gate (resolved 2026-08-15 — dispatched; superseded above) ---
NEXT_GATE-PRIOR-REMED: **A3 REMEDIATION DONE + VERIFICATION-GATE PASS 2026-08-15. Nothing is running. The next action is a USER
DECISION: run the A3 re-clear, or repair ORCH-015 and go to the commit gate.**

**THE BLOCKING FINDING IS CLOSED, AND I PROVED IT MYSELF.** A3-R027-F1's guard (`syncNext`'s parse-failure
`self.isNull()`, now :2126) SURVIVED deletion at 47/47 before this remediation. I deleted it again on the remediated
tree: **`heap-use-after-free … CloudService.cpp:2150 in syncNext()`, exit 1**, with the stack naming the new slot
`syncNextParseFailureStandsDownWhenTheAthleteTabDiesInItsProcessEvents` (test file :4579). That is exactly the
acceptance the A3 specified. Restored byte-identical (md5 `70460176…`, `cmp` clean), re-confirmed **49/49**, ctest
**26/26**, `GoldenCheetah` relinks (exit 0). FILES reconcile; **`refreshClicked` has ZERO hunks in the diff**, so
REQ-028's scope was respected.

**F4 shipped with each of the three sites justified INDIVIDUALLY, not assumed identical** — and the third is the
interesting one: `completedWrite:2626` does not prevent a write (S-R027-01 already stops that one `openRideFile` later);
what it uniquely prevents is row[1] being **opened and fully parsed**. Killing assertion is `rideOpens`, not
`writeFileCalls`, and both the code and the slot say so, so nobody "simplifies" it later. `failedRead:2432` turned out
to have the **widest** window of the three — entry check only, no re-read, no nested loop.

**THE BUILDER SELF-DISCLOSED TWO PROCESS FAILURES, BOTH OF THE "APPARATUS CANNOT SEE THE PHENOMENON" CLASS:**
**B-R027-06** — its own mutation script classified an **ASan abort as SURVIVED** (a crashing process prints neither
`FAIL!` nor `Totals:`), so **M10, the strongest kill in the matrix, was initially reported as a survivor.** Fixed to
detect `SUMMARY: AddressSanitizer` and non-zero exit. This is why I ran M10 myself against the **exit code and the
SUMMARY line** rather than parsed test output. **B-R027-07** — TEST-102's `completedRead` site initially passed for the
WRONG REASON: with a `.gcblock` payload, `uncompressRide`'s nested loop delivered the abort before :2318, so TEST-094's
guard caught it and the site under test was never exercised. Green, proving nothing. Found and fixed by the builder
asking whether its own RED was red for the reason it intended. **B-R027-08** — crossing the untested Download-tab
boundary immediately produced a silent-zero fixture bug (the column-1 header is `"Workout Name"`, not `"File"`).
**15 guards, each mutated INDIVIDUALLY per [[LSN-059]], zero survivors.**

**BEFORE ANY COMMIT: ORCH-015 must be repaired** — the drift lint is a pre-commit hook and is currently RED on correct
prose (`STATE.md:533`, where "GREEN" describes the suite and `DEC-030` is an incidental citation on the wrapped line).
The prose was deliberately NOT bent. Repair needs ORCH-010's two-directional matrix.

--- prior gate (resolved 2026-08-15 — remediation built and verified) ---
NEXT_GATE-PRIOR-A3FIND: **A3 RAN 2026-08-14 — VERDICT: FINDINGS, TWO BLOCKING. The slice is NOT shippable as it stands. Nothing is
running; the next action is a USER DISPOSITION.** Nine findings, four refutations-with-evidence. The adversary built an
OUT-OF-TREE mutation harness (patched TUs relinked against the existing objects) and modified no tracked file —
verified: `git status` still shows only our 3 code files + governance.

**A3-R027-F1 (BLOCKING) — reproduced BY THE ORCHESTRATOR.** DEC-032's `continue` promoted `syncNext:2121`'s
pre-existing `if (self.isNull()) return true;` from dead code to the only thing between a synchronously-destroyed
dialog (DEC-030) and a loop that keeps iterating on `this`. **I deleted that line myself: 47/47 STILL GREEN.** The
byte-identical guard in `uploadNext:2516` IS covered (TEST-087 `UploadNextParsePE` kills it with a heap-use-after-free).
The asymmetry is a missing fixture frame — `CompletionFrame` has no `SyncNextParsePE`. ~5 lines to close.
**My own simultaneous mutation could not have found this: the line is not in the diff.**

**A3-R027-F2 (BLOCKING) — a LIVE heap-use-after-free on UNMODIFIED production, pre-existing, which this slice WIDENS.**
`refreshClicked` deletes every row item (:1527-1545, three `delete curr;` blocks — I confirmed); the loops capture
`curr` at :1988/:2433 BEFORE `openRideFile`'s nested loop and read it at :2096/:2491 after; `refreshButton` is **never
disabled** (I confirmed: four references, no `setEnabled` anywhere) and the code's own comment at :2044 already says a
Refresh is deliverable from inside that call. REQ-027 adds a WRITE into that window (`curr->setText(7, tr("Aborted"))`).
**DEC-025, DEC-030 and DEC-032 each enumerated a suspension set and none ever included the row item** → [[LSN-060]].

**A3-R027-F4 (open) — the abort is not honoured on the READ side**: `readFileCalls=2` after abort in the adversary's
PROBE-C, i.e. a fresh third-party download starts after the user stopped. ~3 one-liners. And `downloadNext` has **zero
batch coverage anywhere in the repo** (`grep -rn "selectAllChanged" unittests/` → nothing).
**F3/F8 (open)** — the generation guard does not cover `refreshClicked`, and the completion slots its own comment names
as the harm carry no snapshot. **F5/F6/F7/F9 (informational)** — M11 (`delete ride;`) and M12 (bar placement) SURVIVE;
F9 is the subtle one: clause (d)'s own assertion is shadowed by the S-R027-01 guard.

**Three lessons captured, one of them against MY method:** [[LSN-058]] (a change that makes a frame RESUME promotes
UNEDITED guards from dead to load-bearing), [[LSN-059]] (**simultaneous mutation proves only that SOME slot notices the
aggregate — never per-guard coverage**; and the assertion that kills a mutant must be the CRITERION's own),
[[LSN-060]] (a suspension set must include the container element the frame holds).

--- prior gate (resolved 2026-08-14 — A3 returned FINDINGS) ---
NEXT_GATE-PRIOR-A3: **REQ-027 A3 GATE — `qgdw-adversary` DISPATCHED 2026-08-14** against the built, Verification-Gate-PASSED
working tree. Mandatory for this class and not a formality: the last three A3s each found a real defect behind a fully
green suite, and **this slice exists because the previous one's abort fix was incomplete in a way its own tests could
not see** (S-R027-01). The adversary was briefed to assume that has happened again.

**BUILD IS DONE AND THE GATE PASSED — evidence is orchestrator-EXECUTED, not read.** ASan **47/47** (was 42), full ctest
**26/26**, `GoldenCheetah` links (28,736,984 B). FILES reconciled exactly (3 files: `src/Cloud/CloudService.{h,cpp}` +
the test file; `downloadNext`, `ImportSeamStubs.cpp`, CMake, MainWindow all untouched; zero `.orig` residue).
**My own mutation removed ALL NINE guards SIMULTANEOUSLY — the builder had reverted them only singly — and exactly 4 of
the 5 new slots failed while ALL 42 BASELINE SLOTS STAYED GREEN**, which is the property single-reverts cannot
establish: no new guard is propping up pre-existing coverage. The 5th (TEST-097) survived only because reverting the
`continue` restores the pre-fix unconditional return that makes its assertion vacuous — the interaction the builder had
already declared in the slot's own source — and **M3 in isolation kills it** (`'out.rideOpens == 0' returned FALSE`).
Restored byte-identical (md5 `c96d7fb5…`, `cmp` clean), re-confirmed 47/47, and **I ran the `GoldenCheetah` link the
builder explicitly flagged it had NOT re-run after its final restore** — exit 0.
**Goal audit PASS:** clauses (a)/(b)/(c)/(d)/(f) encoded as written, behind real anti-vacuity premises
(`abortDelivered`, `abortTookTheAbortBranch`, `row0Action == "Upload"`); **both proxies are labelled as proxies in the
source** — criterion (a)'s private `downloading` and criterion (b)'s stack property, the latter marked "RECORDED
RESIDUAL, not covered". One judgement call beyond the criterion, disclosed and tested: the S-R027-01 bails label the row
`tr("Aborted")` rather than leaving it reading "Uploading" forever.
**The builder's honest negative, kept on the record: M7 SURVIVED its first matrix run** — the `uploadNext` bar advance
was uncovered until it added an upload-tab run to TEST-096 — and it reported that rather than shipping the line
unproven. **`downloadNext` deliberately has NO generation snapshot** (its only `continue` does not suspend); **if
DEC-033 adds a suspending `continue` there, the snapshot must be added with it** — that is the one thing a DEC-033
reviewer must not forget.

**AFTER A3:** disposition findings → REQ-026's trace row is already amended (its criterion is honestly closed only WITH
TEST-099) → then the commit gate, which is a USER decision because it writes git history. Two open calls queued for the
user: **DEC-033** (the `readFile` bool contract) and **promoting [[LSN-034]] to MECHANISM** (now recur:6).

--- prior gate (resolved 2026-08-14 — built, gate passed) ---
NEXT_GATE-PRIOR-BUILD2: **REQ-027 BUILD GATE, RE-BRIEFED 2026-08-13 — the builder fired the stop-and-report hatch on my briefing and
was RIGHT. Zero code was written; scope item 3 is pulled out to DEC-033; items 1, 2, 4 are re-dispatched to the SAME
builder (context intact).**

**WHAT THE HATCH CAUGHT (B-R027-01) — orchestrator re-ran the falsifying command and CONFIRMED.** My briefing asserted
"every real implementation returns `false` only on paths that armed nothing". False for the majority implementation:
`GarminConnect::readFile` has EIGHT `return false` sites (452/474/493/519/540/557/572/588) and **SEVEN post a completion
on the line immediately above**, through `Qt::QueuedConnection` posters (:765-770, :780-785) that land AFTER the return.
Only :452 is silent — **the one site I sampled and generalised from.** Building it would have double-driven the loop on
~11 integrations: bar advanced synchronously by the new branch, then again by the queued completion, which labels
`child(listindex-1)` (a different row by then) and re-drives — the precise double-drive the same briefing correctly
forbade for `writeFile`, and a regression of DEC-022/023. **It is not fixable at the call site** (`readFile` returns a
bare bool; "returned false AND armed nothing" is never communicated), so it needs **DEC-033**: tri-state/out-param
across ~11 overrides, or GarminConnect returning true where it emitted (touches REQ-017/023), or a deferred watchdog.
Carries **B-R027-02** (the briefed branch leaks the caller's `QByteArray` — allocated :1985/:2127, freed only at
:2191/:2219/:2307) and **B-R027-03** (any fixture must return false AFTER queueing, or it re-hides this).

**THIS IS LSN-034 recur:6 AND IT IS THE ORCHESTRATOR'S DEFECT.** Two compounding failures, both mine: the claim was
inherited from the scout's F1 and carried into a briefing after spot-checking only `LocalFileStore` — one of ten cited
locations, verdict generalised — and it is the identical wrong-predicate shape I had recorded in [[LSN-048]]'s own
refinement minutes earlier. **Writing a lesson did not install it.** Six recurrences, six hatch catches, zero escapes,
and this one would have been a silent production regression rather than a test failure. The `file:NNN`-and-universal-
quantifier lint is the only move left; promotion is no longer arguable, it is overdue.

**RE-BRIEFED SCOPE:** items 1 (in-loop `continue`), 2 (S-R027-01 abort re-read, both loops), 4 (batch-generation rider,
now UNBLOCKED because item 3's removal makes the `continue` sites finite and the guard's coverage fully specified).
Plus two riders adopted from the builder: **B-R027-04** (`uploadNext`'s parse branch gets the same `++downloadcounter`,
so the fix does not create a new divergence between the loops it exists to converge) and **B-R027-05** (criterion (a)'s
`downloading` is private at `CloudService.h:466`; replaced by tail-EXCLUSIVE proxies :2097/:2086/:2092-2095, **labelled
in the test as a proxy**). TEST-095/096/097/099/100; **TEST-098 released back to the registry** — clause (e) is DEC-033's, not this slice's.
TEST-100 must MEASURE the double-click (builder's reachability argument was explicitly argued-not-measured).

**ON RETURN:** the same Verification Gate as before — independent re-run, `FILES` vs `git status`, and my own mutation of
every guard **singly AND simultaneously** (REQ-021 precedent: a builder reverting singly can miss a guard propping up a
pre-existing slot). Goal-audit (a)/(b)/(c)/(d)/(f) verbatim; (b)'s **exactly-N** reader count is the clause that
discriminates a re-entering driver from a stalled one.

--- prior gate (superseded 2026-08-13 — hatch fired, scope corrected) ---
NEXT_GATE-PRIOR-BUILD1: **REQ-027 BUILD GATE — `qgdw-builder` DISPATCHED 2026-08-13 on new branch `garmin/req027-silent-stall`**
(created from `f2a278b56`; `git checkout -b` moved the ref only — dirty count went 52 → 59 and the 7 new entries are
exactly my governance edits, verified, so no working-tree file was touched). DEC-032 is ACCEPTED (Option A) and both
user decisions are in. Building TEST-095..100 + four scope items, RED-first.

**ON RETURN — the Verification Gate, and this slice raises the bar on it.** DEC-032's whole point is a set of guards, so
per the standing rule "tests pass" is not evidence a guard is load-bearing: independently re-run the ASan target and
full ctest, diff `FILES` against `git status`, and run MY OWN mutation of every added guard — and, per the REQ-021
precedent, mutate them **simultaneously** as well as singly, because a builder that only reverts them one at a time can
miss a guard propping up a pre-existing slot. Snapshot with `.orig` + `cmp`, never `git checkout` (52 dirty entries are
other owners'). Goal-audit clauses (a)-(f) verbatim against the tests — especially (b)'s **exactly-N** reader count,
which is the clause that discriminates a re-entering driver from a stalled one, and (f), which is the REQ-026 escape.

**S-R027-01 CHANGES THE WAVE-CLOSE RECORD AND MUST NOT BE LOST:** REQ-026 shipped in `6dc794caf` with its criterion
open on the sibling branch. The clean-extract gate, the 42/42 ASan run and the 26/26 ctest were all real — and all
blind to it, because TEST-093's row[1] is never aborted during its OWN `openRideFile`. That is the fourth consecutive
wave in which a green suite was silent on a live defect, and the third found by a fresh context rather than by the
suite. **When REQ-027 closes, REQ-026's traceability entry needs amending** — its criterion is not honestly closed
until clause (f) is green.

--- prior gate (resolved 2026-08-13 — DEC-032 accepted Option A; S-R027-01 folded in) ---
NEXT_GATE-PRIOR-DEC032: **DEC-032 DECISION GATE — the REQ-027 silent-stall fix shape. `qgdw-scout` DISPATCHED 2026-08-13.**
User picked **B-R026-01** off the wave-close queue, so REQ-027 is OPEN and DEC-032 is ALLOCATED. Nothing is blocked;
the next action is the scout returning a three-option draft, which the ORCHESTRATOR then verifies and presents.

**The surface was re-derived from code at REQ time ([[LSN-048]]) and it is LARGER than the finding's stub — two axes:**
1. **LIVE.** `CloudServiceSyncDialog::syncNext` (defined `CloudService.cpp:1967`) has exactly four re-entry points —
   :1962 (initial dispatch), :2273 (`completedRead`), :2326 (`failedRead`), :2472 (`completedWrite`), each grep-verified
   one line per site. Its upload-side **parse-failure branch (:2067-2071)** sets "Parse failure", runs `processEvents()`
   (:2069), bails on `self.isNull()` (:2070), then falls through :2072/:2074 to the unconditional `return true` at
   **:2075** — initiating NO async work, so it arms NONE of the four. The batch dies silently: row labelled, progress bar
   frozen, remaining checked rows never processed. REQ-026 is genuinely satisfied there (it does not over-transfer),
   which is exactly why no guard was added and why this is a distinct class.
2. **DORMANT — O-R027-01, orchestrator-found at REQ opening.** `syncNext` (:1996) and `downloadNext` (:2135) both
   **discard `readFile`'s bool return**. Base `CloudService::readFile` (`CloudService.h:142-144`) returns false and emits
   NEITHER `readComplete` nor `readFailed`, so a Download-capable service inheriting it stalls identically. Enumerated by
   matching the predicate itself, not by proximity ([[LSN-034]] refinement 1): of the six services declaring `Download`
   explicitly and the five inheriting the `CloudService.h:104` default, **only `Withings` (`Withings.h:43`) overrides
   neither `readFile` nor `readdir`** — confirmed by opening `Withings.h`/`Withings.cpp`, not by subtracting two greps.
   Unreachable today because base `readdir` (`CloudService.h:205-207`) returns an empty list; **live the moment anyone
   implements `Withings::readdir`.** [[LSN-042]] class, so DEC-032 must close it or accept-with-rationale IN WRITING.

**Constraints pasted into the scout briefing, all verified on disk:** REQ-026's abort invariant must survive the re-drive
(a re-drive re-enters the very loop REQ-026 guards) · no unbounded re-entry and no stack growth on an all-unparseable
list · any queued self-invoke lands via the event loop that also delivers the SYNCHRONOUS athlete-tab destroy (DEC-030) ·
the failure must become VISIBLE in the completion tail (:2079-2102) rather than silently under-reporting · blast radius
is ~16 shared cloud services, not Garmin-only. **B-R025-03 is explicitly OUT of scope** (the four uncounted
`processEvents()` at :2012/:2144/:2246/:2303 stay queued as their own decision) but each option must state whether it
DEPENDS on that incompleteness. Scout is also asked to sharpen the REQ-027 acceptance criterion and to say which clauses
are drivable in `testGarminConnectSyncDialogClose` (42 slots today) — and to mark any Qt semantic it cannot source as
needing a GATING PROBE rather than asserting it.

**On return:** Verification Gate (three real options? scores justified? cascade concrete? every quoted `file:NNN` and DEC
id re-opened? volatile Qt claims sourced or flagged as probes?), then the orchestrator presents the three options.

--- prior gate (resolved 2026-08-13 — user picked B-R026-01; REQ-027 + DEC-032 opened) ---
NEXT_GATE-PRIOR-WAVECLOSE: **WAVE CLOSED. The clean-worktree gate PASSED on the COMMITTED tree 2026-08-13. Nothing is running, nothing is
blocked, and the next action is the user picking the next REQ from the queue below.**

**CLEAN-EXTRACT GATE — PASS, run on `git archive HEAD` of `4c3608e89` (tree `37335557f156ed175bc5427ba73b284b6d711327`),
extracted to a scratch dir with its own fresh build tree:**
configure + generate clean (Qt 6.8.2, C++17, Release, Ninja) with **Garmin Connect: ON** and **Unit Tests: ON** (note
`BUILD_TESTS` defaults to **OFF**, so a gate that forgets it proves nothing about the tests) · **822/822 targets built** ·
`src/GoldenCheetah` linked, **27,984,000 bytes** · **26 test executables** · **`ctest` 26/26** · the wave's own ASan
target **42/42**, including both REQ-026 slots by name
(`anAbortInsideUploadNextMustStopTheLoopNotJustProveTheDialogIsAlive`,
`anAbortInsideUncompressRideMustStopCompletedReadFinishingSaveRide`).
**Completion was verified properly, not read off a log line:** the background build's `BUILD EXIT: 0` was `tail`'s status,
not cmake's — the pipe swallowed the real one — so completion was re-established by re-running the build and getting
`ninja: no work to do` with an unpiped exit 0, plus the artifact counts above. A pipeline's `$?` is not the build's `$?`.
**Pre-existing warning, not ours, not fixed:** `GarminConnect.cpp:60` uses the deprecated `QDateTime::setTimeSpec(Qt::UTC)`
(Qt 6 wants `setTimeZone()`). Cosmetic, predates this wave, left alone deliberately.
**Evidence-vs-commit integrity:** the gate ran against the tree of `4c3608e89`. The clean-gate RESULT is recorded in a
SEPARATE follow-up commit rather than by amending `4c3608e89`, precisely so the verified tree stays byte-identical to the
tree that was verified — amending would have made the recorded evidence describe a tree that no longer existed.

**THE QUEUE — user picks (nothing here is blocked):** a new REQ for **B-R026-01** (the silent sync stall — newly found,
and arguably the most user-visible item: a frozen sync with no error) · a new REQ for **A3-R021b-F2** (file-IO layer,
`RideFile.cpp:999`) · **REQ-024** (Strava store-side UAF at `blockingCallDepth == 0`; needs a sibling scan of the other
16 subclasses first — only Xert was checked) · **REQ-020** (OAuth wizard, WIDENED to `AddCloudWizard`) · **REQ-022**
(QThread cross-thread: `OpenData`, `CloudServiceAutoDownload` — scout-reported, NOT orchestrator-verified, spot-check
first) · **REQ-023** (store-layer `uncompressRide`) · **B-R025-03's DEC** (the `blockingCallDepth` predicate question) ·
**ORCH-014** (repair CLV Check 5's grep — vacuous since 2026-08-08) · ORCH-011/012/013 · B-R026-02 (harness
`rideMetadata` UB) on the next slice touching `ImportSeamStubs.cpp`.
**Also worth deciding soon:** promoting [[LSN-034]] to MECHANISM (recur:5, five-for-five hatch catches, never once
costly) — the same path LSN-001 and LSN-008 took. Restating it a sixth time is the option this project has already
learned not to take.
**Housekeeping left to the user, unchanged:** the 52 dirty entries (Coach/Gui/CMake/vcpkg/skill churn incl.
`.claude/hooks/anti_duplication_guard.py`, byte-identical to its install source) and whether to merge/push
`garmin/req021-collaborator-uaf` (7 ahead of `master`, 0 behind, NOT merged, NOTHING PUSHED).

--- prior gate (resolved 2026-08-13 — clean gate PASSED) ---
NEXT_GATE-PRIOR-CLEAN: **THE CLEAN-WORKTREE BUILD GATE — the last step of this wave's close. Commits (a) `6dc794caf` and
(b) `3f44c447f` are DONE; (c) this docs record is being written now. What remains is a configure+build+ctest of the
COMMITTED tree from a clean extract (`git archive HEAD` / `git worktree`), which is mandatory at a feature close and is
the gate that caught ORCH-005/006/007. Every other gate this wave ran inside the developer's working tree.**

--- prior gate (resolved 2026-08-13 — user said proceed; commits landed) ---
NEXT_GATE-PRIOR-COMMIT: **THE COMMIT GATE. REQ-026 is BUILT and Verification-Gate-PASSED; the A3 blocker is closed; nothing is
running and nothing is blocked on evidence. AWAITING USER GO — the next action writes git history, so it is not taken
unasked.**

**WHAT SHIPS:** the REQ-021 wave + DEC-031 + REQ-025 + REQ-026, i.e. 11 of the working tree's 67 dirty entries.
**OUR FILES ARE NOW ELEVEN** (was 9 — REQ-026 touched two that were already ours, so the set is unchanged in membership):
`src/Cloud/CloudService.cpp` + `.h` · `src/Core/Context.cpp` · `unittests/Core/garminconnect/
testGarminConnectSyncDialogClose.cpp` + `stubs/ImportSeamStubs.cpp` · `scripts/ledger_drift_lint.py` +
`scripts/test_ledger_drift_lint.py` + `.claude/hooks/ledger_drift_lint.py` + `.claude/hooks/anti_duplication_guard.py` ·
plus the governance ledgers (`STATE.md`, `WIKI.md`, `lessons.md`, `.claude/workflow-garminconnect/*`).

**HUNK-SPLITTING IS NOT NEEDED THIS WAVE — re-verified on disk 2026-08-12, do not carry the REQ-017/REQ-019 constraint
forward by habit.** Those waves had to hunk-split because their changesets reached into `src/Gui/MainWindow.cpp` and
`src/CMakeLists.txt`, which carry the Coach owner's work. **This wave touches neither** (both confirmed untouched by both
agents), and every one of our files is EXCLUSIVELY ours: a scan of the added lines in all eight code/tooling files for
`coach|anthropic|openai|gemini|libusb|calendar` returns **zero** in each, and `src/Core/Context.cpp`'s entire diff is the
one `tab = NULL;` statement plus its comment. So whole-file staging is safe and `git add <path>` per file is the method.
**`git add -A` remains forbidden** ([[LSN-007]]/[[LSN-010]]/[[LSN-032]]) — 56 of the 67 entries are other owners'.
**ONE EXCLUSION, deliberate:** `.claude/hooks/anti_duplication_guard.py` is **byte-identical to its install source**
(md5 `658575d4…` both) — it is a sync artifact of the USER's pre-session skill update, not this wave's work, so it stays
uncommitted with the rest of the skill churn, matching the standing rule that pre-session tooling churn is the user's
call. That makes it 10 files to stage, not 11.
Proposed split, in order:
(a) **feat(garmin)** — `src/Cloud/CloudService.{h,cpp}` + `src/Core/Context.cpp` + the two test files. This is REQ-021 +
    DEC-030 + DEC-031 + REQ-025 + REQ-026 as one coherent lifetime-and-abort changeset.
(b) **fix(workflow)** — the three lint files (ORCH-010's assignment-shape repair, 21/21, mutation-proven).
(c) **docs(garmin)** — the governance ledgers.
**Then the clean-worktree gate, which is NOT optional:** `git archive HEAD` / `git worktree` extract → configure →
generate → full build → `ctest`. This is the gate that caught ORCH-005/006/007, and every other gate this session ran
inside the developer's working tree. Expect pre-commit `clang-format` to rewrite files on the first attempt; if it does,
**re-verify BY EXECUTION before retrying** ([[LSN-007]]) rather than assuming the reformat was cosmetic.

**THE QUEUE AFTER THE COMMIT, in priority order:** a new REQ for **B-R026-01** (the silent sync stall — newly found, and
arguably the most user-visible thing on this list: a frozen sync with no error) · a new REQ for **A3-R021b-F2** (file-IO
layer, `RideFile.cpp:999`) · **REQ-024** (Strava store-side UAF at `blockingCallDepth == 0`; needs a sibling scan of the
other 16 subclasses first — only Xert was checked) · **REQ-020** (OAuth wizard, WIDENED to `AddCloudWizard`) ·
**REQ-022** (QThread cross-thread: `OpenData`, `CloudServiceAutoDownload` — scout-reported, NOT orchestrator-verified,
spot-check first) · **REQ-023** (store-layer `uncompressRide`) · **B-R025-03's DEC** (the predicate question) ·
**ORCH-014** (repair CLV Check 5's grep — it has been vacuous since 2026-08-08) · ORCH-011/012/013 (guard `cd`-blindness;
guard blocks the mandated `cp` restore; `install_hook.py` side effects) · B-R026-02 (harness `rideMetadata` UB) on the
next slice that touches `ImportSeamStubs.cpp`.

**ALSO WORTH A DECISION SOON, not urgent:** [[LSN-034]] is at recur:5 with five-for-five hatch catches and no cost ever
incurred. That is the profile of a guard that should become MECHANISM (a lint extracting every `file:NNN` claim from a
briefing and re-reading it), exactly as LSN-001 and LSN-008 did. Restating it a sixth time is the option this project has
already learned not to take.

--- prior gate (resolved 2026-08-12 — REQ-026 built + gate PASS) ---
NEXT_GATE-PRIOR-REQ026: **REQ-026 BUILD GATE — `qgdw-builder` DISPATCHED 2026-08-12 to close A3-R021b-F1 (BLOCKING) + F3, RED-first.**
User chose "fix F1+F3 now, then commit" over committing first or bundling B-R025-03's predicate decision. **B-R025-03
therefore stays QUEUED and is explicitly out of the builder's scope** — the four bare `processEvents()` at
:2012/:2144/:2246/:2303 must be left alone, because wrapping them would defer the very close they exist to deliver.

**No DEC was written, deliberately:** the fix shape is forced, not chosen (re-read the abort flag in the one branch that
suspends and keeps iterating), so it is not a one-way door and the Three Options Doctrine does not apply. The DEC-worthy
question in this family — should `blockingCallDepth` become a complete predicate — is B-R025-03's, and it stays open.

**REQ-026 written to prd.md** with an outcome-shaped criterion ("when the user aborts, no further ride is transferred"),
NOT a restatement of the fix. IDs allocated and passed in the briefing: **TEST-093** (the F1 abort test) and
**TEST-094** (F3's `completedRead` re-read, CONDITIONAL — if the builder cannot drive it, it must stay unbuilt with a
recorded residual rather than shipping an untestable guard; the TEST-076/LSN-022 precedent).

**Briefing pre-flight, honoured ([[LSN-034]]):** every identifier was opened on disk first — `uploadNext` bracketed
2314-2411 by DEFINITION line, all three `aborted` reads (:2187/:2292/:2421) and the lone :2399 assignment enumerated one
line per read, `deferCloseIfBusy` at :1471, `downloadClicked` at :1908 with the :1916 assignment and :1918 relabel,
the parse-failure branch at :2376-2387, DEC-024/025/031 titles read from decisions.md (:699/:759/:946) rather than
recalled, and the harness machinery confirmed (`obs::writeFileCalls` :255/:274/:482-485, `namespace rideopen` :551-566,
`BlockingRideFileReader::openRideFile` :568+, `store->closeAction` :513/:516/:371-380, TEST-087's `UploadNextParsePE`
model at :3988/:4083-4101). The briefing also names the ONE thing that makes this not a copy-paste job: the fixture needs
**asymmetric** rows — row[0] missing, row[1] parseable — because if both are missing the slot cannot distinguish
"stopped correctly" from "continued and failed again", and would pass for the wrong reason.

**ON RETURN — Verification Gate, orchestrator-EXECUTED not read:** re-run the ASan target (41+ slots) and full `ctest`
(26/26 baseline) myself · confirm `src/GoldenCheetah` links · diff FILES against `git status`/`git diff --stat` ·
**mutate the new guard myself and confirm the extra upload returns** (this slice's whole point IS a guard, so "tests
pass" is not evidence — "tests fail when I break it" is) · verify the RED output was genuinely observed pre-fix ·
confirm `blockingCallDepth`/`StoreReaper`/`BlockingCall` are byte-unchanged · confirm `MainWindow.cpp` and
`src/CMakeLists.txt` were not touched · all mutations under the [[LSN-032]] `cp`/`cmp` snapshot mandate with zero
`.orig` residue. Then the commit gate.

--- prior gate (resolved 2026-08-12 — user chose: fix F1+F3 now, then commit) ---
NEXT_GATE-PRIOR-A3FINDINGS: **A3 RE-CLEAR RETURNED FINDINGS 2026-08-12 — VERDICT: NOT CLEAR. One BLOCKING defect (A3-R021b-F1),
orchestrator-CONFIRMED and found to be MORE reachable than reported. AWAITING USER DISPOSITION. Nothing is running.**

**THE A3 WAS WORTH RUNNING.** It found a real, outward-facing defect that 40 green ASan slots and a passed Verification
Gate had both missed: `uploadNext` never reads `aborted`, so an abort during its parse-failure branch uploads one more
ride to the cloud service after the user said stop (full detail → BLOCKING). Third A3 in a row to find a genuine defect
in a changeset that had already passed a Verification Gate.

**WHAT THE ADVERSARY ALSO DID, and it matters for how much to trust the rest:** it REFUTED five of its own hypotheses
with EXECUTED evidence rather than reasoning — TEST-089's deleteLater/queuedInvoke measurement carries a working
positive AND negative control in one run (`deleteLater=1 queuedInvoke=1` vs `late(inverse control)=0`); TEST-081's
child-window-hide measurement still shows its sensitivity control (`dialog rawHide=1` vs `control child rawHide=0`);
the shared-stub change was verified against ALL THREE targets that compile `ImportSeamStubs.cpp` (40/40, 4/4, 4/4 —
→ [[LSN-056]]); TEST-090 Part 1's nesting was shown REACHABLE from one ordinary user action (Refresh during a download),
not synthetic; and — importantly — **the literal B-R031-01 shape was REFUTED for all four uncounted `processEvents()`
sites**: at each of :2012/:2144/:2246/:2303 `blockingCallDepth` has already returned to 0, so the reaper cannot fire
underneath them. The danger at those sites turned out to be the ABORT flag, not the reaper. `StoreReaper::release()`
calling `closeAndDeleteStore(NULL)` on ordinary unwinds was also refuted as a bug (documented null-tolerant path).

**THE DECISION NOW (asked 2026-08-12, not yet answered):** A3-R021b-F1 blocks the commit gate. Three shapes —
(1) **fix F1 (+F3) now, then commit**: dispatch `qgdw-builder` for a RED-first `if (aborted) return true;` beside the
existing bail at :2385, plus the prescribed test (Upload tab, 2 checked rows, row[0]'s file missing, abort injected from
inside row[0]'s `processEvents()` via the queued pattern TEST-090/091 already use, assert row[1] never opened/written).
Bundle F3 or leave it. (2) **commit the wave as-is and fix F1 as its own REQ** — defensible only if F1 is judged
pre-existing, which it is NOT purely: the branch is old, but REQ-025 is the wave that made this window counted and
`self.isNull()`-guarded, so the wave touched it and left the abort half undone. (3) **fix F1 and take B-R025-03's
predicate decision at the same time**, since F1/F3/B-R025-03 share one root cause (the depth counter is treated as a
complete predicate, and guards stand in for it).

**STILL TRUE, and unchanged by the A3:** the commit gate needs hunk-splitting — 67 dirty entries, only 9 ours (listed in
CURRENT). **Never `git add -A`** (LSN-007/010/032). `src/Gui/MainWindow.cpp` and `src/CMakeLists.txt` carry the Coach
owner's work and must NOT be staged. Split: (a) feat — `src/Cloud/CloudService.{h,cpp}` + `src/Core/Context.cpp` + the
two test files; (b) fix(workflow) — the three lint files (ORCH-010); (c) docs — the governance ledgers. A clean-worktree
configure+build of the COMMITTED tree is mandatory at the feature close (the gate that caught ORCH-005/006/007).

**THE QUEUE AFTER THAT, in priority order:** a new REQ for A3-R021b-F2 (file-IO layer, `RideFile.cpp:999`) · REQ-024
(Strava store-side UAF at `blockingCallDepth == 0`, needs a sibling scan of the other 16 subclasses first — only Xert was
checked) · REQ-020 (OAuth wizard, WIDENED to `AddCloudWizard`) · REQ-022 (QThread cross-thread: `OpenData`,
`CloudServiceAutoDownload` — scout-reported, NOT orchestrator-verified, spot-check first) · REQ-023 (store-layer
`uncompressRide`) · B-R025-03's DEC (fold with F1/F3 if option 3 is chosen) · **ORCH-014** (repair CLV Check 5's grep) ·
ORCH-011/012/013 (guard `cd`-blindness; guard blocks the mandated `cp` restore; `install_hook.py` side effects).

**UNMEASURED, carried forward honestly (adversary's own list):** whether F1's blast radius is bounded at exactly one
extra row under real Qt event-delivery timing (reasoned, not instrumented) · A3-R021b-F2's end-to-end reachability (needs
a fixture the read-only adversary could not write) · real `GarminConnect::readFile`/`uncompressRide` timing beyond the
stub's comments · MainWindow/AthleteTab's actual Context-vs-tab destruction order (out of the adversary's scope; it
relied on the verified anchor) · suite flakiness under parallel/loaded CI (wall-clock fixtures, 5000ms watchdogs) ·
an exhaustive per-slot audit of whether B-R025-02's RideCache strictness makes any of the 40 slots vacuous (spot-checked
only).

--- prior gate (resolved 2026-08-12 — A3 ran, returned FINDINGS) ---
NEXT_GATE-PRIOR-A3RECLEAR: **REQ-021 WAVE A3 RE-CLEAR — `qgdw-adversary` DISPATCHED 2026-08-12 against the working tree.** User chose
A3-before-commit (the recommended option) over committing first. The adversary is pointed, in order, at the six surfaces
enumerated in the prior gate below: B-R025-03 (the four uncounted `processEvents()` — the reaper reasons as though the
`blockingCallDepth` predicate were complete), B-R025-01 (the file-IO layer at `src/FileIO/RideFile.cpp:999`), DEC-031's
two second-order timing shifts, the four self-disclosed harness shortcuts, B-R025-02's deliberate strictness (flag only
if it makes a slot vacuous), and TEST-090 part 2's shape-only refcount coverage.

**Briefing pre-flight, honoured before dispatch ([[LSN-034]] recur:4, [[LSN-023]]):** every identifier in the briefing was
OPENED first, not recalled. `RideFile.cpp:999` confirmed as exactly the `context->athlete->cyclist` deref; all four
uncounted loops confirmed at 2012/2144/2246/2303 with their enclosing symbols bracketed BY DEFINITION LINE, not by call
site (`syncNext` 1967-2105, `downloadNext` 2106-2174, `completedRead` 2175-2278, `failedRead` 2279-2313); the
`StoreReaper`/`BlockingCall` anchors confirmed at `CloudService.h:490/541-568/573/583-604` and
`CloudService.cpp:961/1369-1370/1385/1409-1419/1426-1447`. A FRESH `git status --porcelain` was pasted because the
adversary has no git tools and its own prompt snapshot is frozen; the 58 out-of-scope dirty entries were named as
out-of-scope explicitly so it cannot raise findings against another owner's work.

**ON RETURN — the Verification Gate applies to the adversary too:** spot-check 1-2 cited findings at their cited
locations before accepting the verdict; the adversary is read-only, so any mutation it prescribes is MINE to execute
under the [[LSN-032]] snapshot mandate (`cp file file.orig`, restore from the copy, `cmp` — NEVER `git checkout --` on
these files, all nine carry uncommitted work). Then disposition with the user and proceed to the commit gate as
described below.

--- prior gate (resolved 2026-08-12 — user chose A3 first) ---
NEXT_GATE-PRIOR-DECISION: **AWAITING USER — one decision, then the path to commit is short. Nothing is running.**

**THE DECISION (asked 2026-08-12, ANSWERED 2026-08-12 → A3 first):** run a **fresh A3 re-clear on the working tree BEFORE committing**
(recommended), or **commit REQ-021 + DEC-031 + REQ-025 as one changeset first** and run A3 against the committed tree.
Why it is not a formality: the changeset has grown a new lifetime mechanism (`StoreReaper`), four newly-counted frames,
five inverted assertions and three self-disclosed harness shortcuts since the last adversarial pass — and **the previous
A3 found two BLOCKING defects in a changeset that had already passed a Verification Gate**, while the one before it
found the axis that created REQ-021 in the first place.

**IF A3 IS CHOSEN — dispatch `qgdw-adversary` and point it at these, in order:**
1. **B-R025-03 — `blockingCallDepth` is STILL not a complete predicate.** Four bare `QApplication::processEvents()`
   remain uncounted (`syncNext` :2012, `downloadNext` :2144, `completedRead` :2246, `failedRead` :2303). They are safe
   by GUARD (each is followed by a `self.isNull()` bail), not by PREDICATE — **and DEC-031's reaper reasons as though
   the predicate were complete.** Wrapping them would defer the very close they exist to deliver, so it needs a DEC.
2. **B-R025-01 — a THIRD layer of the same bug class**, unfixed: `RideFileFactory::openRideFile` derefs
   `context->athlete->cyclist` at **`src/FileIO/RideFile.cpp:999`** AFTER the reader's nested loop. An athlete close
   during a FIT open faults inside the file-IO layer, upstream of every guard this wave built. Class is now
   dialog (REQ-021) → store (REQ-024) → file-IO (this).
3. **DEC-031's two second-order timing shifts** (builder-disclosed, no existing assertion observes them):
   `completedRead` now raises depth on EVERY download completion, so a close arriving during `uncompressRide` is now
   DEFERRED and replayed from `~BlockingCall`; same for `uploadNext`'s parse-failure `processEvents()`.
4. **Harness shortcuts, all self-disclosed:** TEST-092's fixture deliberately KEEPS THE CONTEXT ALIVE to dodge
   B-R025-01, making it a WEAKER teardown than production; TEST-087 invokes completion slots directly rather than via
   `store->notifyReadComplete()`; the suspending reader uses a synthetic `.gcblock` suffix, not a real `.fit`;
   `RideItem::ride` in `ImportSeamStubs.cpp` is shared by THREE targets; wall-clock timing fixtures throughout.
5. **B-R025-02** — the harness is STRICTER than production (`RideCache::~RideCache()` does not delete its `RideItem`s,
   so they leak). Do not "correct" it; several `ride`-guard proofs depend on the stricter model.
6. TEST-090 part 2 (two-dialog refcount) is a SHAPE test — no production route to two concurrent sync dialogs was found.

**THEN THE COMMIT GATE.** Hunk-splitting is required — 67 dirty entries, only 9 ours (listed in CURRENT). **Never
`git add -A`** (LSN-007/010/032). `src/Gui/MainWindow.cpp` and `src/CMakeLists.txt` carry the Coach owner's work and
must NOT be staged. Suggested split: (a) feat — `src/Cloud/CloudService.{h,cpp}` + `src/Core/Context.cpp` + the two test
files; (b) fix(workflow) — the three lint files (ORCH-010); (c) docs — the governance ledgers. A clean-worktree
configure+build of the committed tree is mandatory at the feature close (this is the gate that caught ORCH-005/006/007).

**AFTER THE COMMIT, the queue in priority order:** REQ-024 (Strava store-side UAF at `blockingCallDepth == 0`, needs a
sibling scan of the other 16 subclasses first — only Xert was checked) · a new REQ for B-R025-01 (file-IO layer) ·
REQ-020 (OAuth wizard, WIDENED to cover `AddCloudWizard` itself) · REQ-022 (QThread cross-thread: `OpenData`,
`CloudServiceAutoDownload` — scout-reported, NOT orchestrator-verified, spot-check first) · REQ-023 (store-layer
`uncompressRide`) · B-R025-03's DEC · ORCH-011/012/013 (guard `cd`-blindness; guard blocks the mandated `cp` restore;
`install_hook.py` side effects).
--- prior gate (resolved) ---
NEXT_GATE-PRIOR-REMED: **REQ-021 A3-REMEDIATION IN FLIGHT + DEC-031 ACCEPTED, ITS BUILD QUEUED BEHIND IT (2026-08-11).**
A3 returned FINDINGS (2 blocking, orchestrator-confirmed). User dispositioned all three questions.
**(1) `qgdw-builder` IS RUNNING** on the five A3 fixes: F2 (the EXECUTED UAF — `QPointer<Context>` in `refreshClicked`,
widen the :1392 guard, then audit the rest of that function), F1 (self-bails at the four `processEvents()`→`this`
completion-slot sites **plus a full sibling sweep of every member function in both dialogs, already-safe ones included**),
F3 (the `context->tab ? context->tab : context->mainWindow` fallback at both construction sites — `tab = NULL` stays),
F5 (**DROP** the 7 unproven widenings so every remaining `ctx` guard is mutation-proven), F6 (drive the 5th sync
suspension point or prove it undrivable). TEST-086/087/088 allocated. The destructor was explicitly FENCED OFF as
DEC-031's territory, including its now-stale comment.
**(2) DEC-031 ACCEPTED (Option B — frame-counted deferred reaper), build NOT yet dispatched** — it edits the same file
as the in-flight builder, so it is sequenced behind. Gating Qt loop-level probe first (a naive `store->deleteLater()`
would delete the store under its own suspended loop; this project already had one `deleteLater` loop-level claim
falsified). The per-dialog refcounted orphan record is SPECIFIED in the DEC, not left for the builder to discover.
**(3) TEST-084's inversion is DEC-031's evidence bar and BLOCKS REQ-021 closing** (user decision) — the suite currently
asserts the leak as REQUIRED across 8 runs, so REQ-021 cannot close on a decision that contradicts its own tests.
Sequence to the commit gate: A3-remediation builder → verify → DEC-031 probe+build → verify → **fresh A3 re-clear**
(this A3 found two blocking defects first time through; re-clearing is not a formality) → commit gate (hunk-split; 63+
dirty entries of other owners' work; never `git add -A`).
**REQ-024 (S-R031-01, the Strava store-side UAF at `blockingCallDepth == 0`) is TRACKED, build AFTER REQ-021 closes**,
and needs a sibling scan of the other 16 subclasses first.
--- prior gate (resolved) ---
NEXT_GATE-PRIOR-A3: **REQ-021 A3 GATE — `qgdw-adversary` DISPATCHED 2026-08-11** against the built, Verification-Gate-PASSED
changeset (uncommitted, branch `garmin/req021-collaborator-uaf`). Briefed to attack 8 things, and told plainly that the
last FOUR A3s each found a new axis so a clean pass is not the expected outcome. The three where this changeset is
genuinely weak, in order: (1) **O-R021-02's undecided residual** — is a dialog on a tab-less Context reachable, and is
`tab = NULL` the right policy versus falling back to `mainWindow`? The orchestrator fixed the undefined behaviour and
deliberately did NOT decide the policy. (2) **Do the NINE modeless sync derefs actually get protected?** That is the
entire reason Option B beat per-site guards, and the protection is STRUCTURAL (asserted via `dialogDestroyed` /
`dialogDiedBeforeItsContext`) — no test drives those nine slots directly. (3) A **sibling scan of the reparent's own
blast radius**: anything assuming a cloud dialog's parent is the MainWindow (`parentWidget()` walks, `window()` calls,
geometry/centring, modality, multi-monitor placement). Also: B-R021-05's negative result (7 of 11 bails not
load-bearing — keep or drop?), B-R021-08 (sync-dialog modality is REASONED, not measured), B-R021-04 (offscreen QPA
only), B-R021-06 (the `reinterpret_cast` harness rewiring), and the acceptance criterion read VERBATIM.
On A3 return: disposition findings with the user, re-spawn only what fails, then the COMMIT GATE. **The commit gate will
need hunk-splitting again** — the tree carries 63 dirty entries of other owners' work; `src/Cloud/CloudService.cpp` and
`src/Core/Context.cpp` are ours and clean-but-for-this-slice, but never `git add -A` (LSN-007/010/032).
AFTER commit the queue is REQ-020 (OAuth wizard + `AddCloudWizard` itself), then REQ-022 (QThread) / REQ-023 (store layer).
--- prior gate (resolved) ---
NEXT_GATE-PRIOR-P2: **REQ-021 PHASE-2 GATE (the production fix) — `qgdw-builder` DISPATCHED 2026-08-10.** Phase 1 is DONE and
Verification-Gate PASSED, and **the gating probe CLEARED Option B**, so no decision is pending — Phase 2 builds the
pre-authorised path. Three parts: (1) the REPARENT — `CloudService::upload` (def :78) passes `context->tab` at :95, and
`CloudServiceSyncDialog`'s ctor (def :824) becomes `QDialog(context->tab, Qt::Dialog)` at :825; (2) the RIDER — Option A
collaborator `QPointer`s in BOTH `start()`s (defs :407 and :852) covering the pre-delete window at MainWindow.cpp:2148-2171
that the reparent cannot reach; (3) **S-R021-01** — the missing `self.isNull()` bail between :1107 and :1108.
TEST-083/084/085 allocated (upload teardown / sync teardown incl. a slot that fails without part 3 / the two structural
controls the reparent obliges: tab-switch visibility + `exec()` app-modality). Every guard AND the reparent must be
mutation-proven load-bearing. On return: Verification Gate (independent re-run + orchestrator's OWN mutation + goal audit
against the elaborated criterion verbatim), then **A3 with `qgdw-adversary`** — mandatory for this class, and the last
four A3s each found a new axis, so budget for findings rather than a clean pass.
**Every line number in the Phase-2 briefing was re-verified as a DEFINITION, not a call site** — the specific LSN-034
failure shape recorded this session (O-R021-01).
--- prior gate (resolved) ---
NEXT_GATE-PRIOR-P1: **REQ-021 PHASE-1 GATE (harness + GATING PROBE) — `qgdw-builder` DISPATCHED 2026-08-10 on new branch
`garmin/req021-collaborator-uaf`** (created at `da07b2227`; branch creation touched zero working-tree files — 58 dirty
entries verified preserved before and after). **DEC-030 is ACCEPTED: Option B (reparent both dialogs to `context->tab`),
PROBE-FIRST, with Option A's collaborator `QPointer` guards as a RIDER.** Scope = all 11 sites if the fix ends up
per-site (user pre-authorised the fallback, so no second decision round); S-R021-01 folded in.
**The builder is under a STOP GATE and may not touch `src/` this phase.** It builds TEST-082 (the `FakeAthleteWindow`
harness modelling synchronous Context deletion + deferred owner `deleteLater()`, which the existing `killOwner` helper
structurally cannot express) and then runs TEST-081 — the gating probe: **does hiding an AthleteTab-equivalent parent
hide a child `QDialog` window on Qt 6.8.2 here?** That single measured answer decides the production change:
visible → Option B reparent (2 lines); hidden → Option B is DEAD on UX grounds and the fix falls back to Option A across
all 11 sites. The probe exists because DEC-030's one unverifiable premise is a Qt behaviour with no primary source, and
this project decides framework semantics by execution (the scout correctly refused to assert it).
On return: Verification Gate (contract / independent re-run of the ASan target + own mutation of the new stub touches /
goal audit vs the elaborated criterion), then present the probe verdict + Phase-2 dispatch (TEST-083/084/085).
**Hazard the builder was warned about:** 58 dirty entries of other owners' work, incl. `src/Gui/MainWindow.cpp` and
`src/CMakeLists.txt` — no `git checkout --`/`restore`/`stash`/`add -A` under any circumstances (LSN-032); `.orig` copies
+ `cmp` only.
--- prior gate (resolved) ---
NEXT_GATE-PRIOR-DEC030: **DEC-030 DECISION GATE — the REQ-021 collaborator-lifetime UAF fix shape. `qgdw-scout` DISPATCHED 2026-08-10**
→ RESOLVED 2026-08-10: scout returned, Verification-Gate PASS (orchestrator spot-checked the QObject-ness of
`Context`/`RideItem`, all nine claimed modeless derefs, the :1107 gap, the single `delete context`, and the single
`CloudService::upload` caller — all CONFIRMED). User chose Option B probe-first + all-11-site fallback scope + fold in
S-R021-01 + route all four sibling groups. Byproducts merged: DEC-030 full entry (6 alignment probes RUN against the
tree per LSN-045), DEC index row, slice row, REQ-021 acceptance ELABORATED in prd.md, REQ-020 WIDENED to cover
`AddCloudWizard` itself, NEW REQ-022 (QThread cross-thread) + REQ-023 (store layer) stubs + trace rows, findings
S-R021-01..06 registered (new `S-` scout-found prefix, added to WIKI REGISTRIES), WIKI bumps (REQ next:garmin-024,
DEC next:garmin-031, TEST next:garmin-T-086, LSN next:049), LSN-048 captured, saves recorded on LSN-034/041/045/047.
(user chose REQ-021-first over REQ-020/ff-merge/bench, 2026-08-10). DEC-030 ALLOCATED (WIKI bumped next:garmin-031).
The briefing was built from facts the orchestrator **re-grepped on disk today** (LSN-034/LSN-045 pre-flight): the three
synchronous deletes in `removeAthleteTab` (MainWindow.cpp ~:2179-85), `closeEvent`'s per-tab call (~:1101-03) vs the
DEFERRED WA_DeleteOnClose self-delete, the second route `AthleteView.cpp:213` → `closeAthleteTab(QString)`
(MainWindow.cpp:2103) → `closeTabClicked`, both dialogs' unguarded derefs (upload ~:424/:443-445/:469/:473; sync
~:1070/:1094-99), the sync dialog's explicit `QDialog(context->mainWindow)` parenting at :824-825, the four inert stub
bodies in `ImportSeamStubs.cpp` (:210/:212/:312/:314/:383), and a **predicate** enumeration of the blast radius
(`grep -rn "public CloudService" src/Cloud/*.h` = **17 subclasses**, one line per hit — membership, not proximity).
Scout is required to return: (A) a by-predicate SIBLING SCAN accounting for `OAuthDialog` (OAuthDialog.cpp:32 — REQ-020),
`OpenDataDialog` (OpenData.cpp:355) and `AddCloudWizard` as same-bug/tracked/dormant/N-A — a missing enumeration is
itself a report defect (LSN-041, burned twice); (B) three real options; (C) Four Pillars; (D) cascade vs DEC-024..029 +
TEST-070..080 + A3-R019-F1/F2/F3 + B-R019-05; (E) recommendation; (F) **the HARNESS sub-decision** — the minimum
`ImportSeamStubs`/target change that models the real two-phase teardown AND makes freed collaborators actually FAULT
(this also closes A3-R019-F3 / B-R019-05); (G) the elaborated REQ-021 acceptance criterion; (H) explicit flagging of
anything unverified. On return: Verification Gate (three REAL options? scores justified? sibling scan complete? is
`Context`/`RideItem` actually a QObject — the scout was told to CHECK, not assume? cascade concrete?), then the
orchestrator PRESENTS to the user. REQ-020 stays queued behind this because REQ-021 may change the recipe it applies.
--- prior gate, still-live user-facing items (kept) ---
NEXT_GATE-PRIOR-AWAITING: **AWAITING USER — REQ-019 is closed (feature `e8833682f` + this docs record). Two open calls, in priority
order.** → ANSWERED 2026-08-10: item 1 (REQ-021) chosen and started; items 3/4 remain available.
1. **REQ-021 — the collaborator-lifetime UAF axis (A3-R019-F1/F2). Recommend this next.** It is the only BLOCKING item,
   it spans BOTH the upload dialog and the **already-shipped** sync dialog (F2 is live on master via `ae5a7a8ab`), and
   it needs a DEC plus harness work FIRST: a purpose-built owner that deletes its Context synchronously while deferring
   its own `deleteLater()`, since `ImportSeamStubs` cannot express production's two-phase order. Fixing the stubs also
   closes A3-R019-F3 / B-R019-05.
2. **REQ-020 — OAuth-wizard UAF** (`AddCloudWizard::AddAuth::doAuth`), the last known member of the ORIGINAL
   dialog-lifetime class. Note REQ-021 may change the recipe REQ-020 should apply, so REQ-021 first is the cheaper order.
3. **Branch disposition:** `garmin/req019-upload-uaf` is **3 ahead / 0 behind** `master` (re-measured 2026-08-10:
   `e8833682f` feat + `30a5a58ad` docs-record + `da07b2227` ORCH-009 docs), unmerged, clean-build-verified — a
   fast-forward is available whenever you want it (same in-place `git branch -f` technique as last time; the 62 dirty
   working-tree entries make a branch switch unsafe otherwise).
4. **Cheap, non-blocking bench:** A3-R019-F3/F4/F5/F6, B-R019-01..06, ORCH-008 (test-home misnomer), plus the older
   A3-R012/B-R017/B-R018 items and VAL-016's two coverage WARNs. F6 in particular is an UNANSWERED sibling-scan question
   (CloudDB* dialogs) — cheap to answer, and this project has been burned twice by skipped sibling scans (LSN-041).
5. **Not the agent's call:** the pre-session Coach/skill/tooling dirt stays with its owners; nothing has been pushed to
   `origin` (local master is 50 ahead).
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR-COMMIT: **REQ-019 COMMIT GATE — IN PROGRESS 2026-08-08.** A3 is DONE (verdict FINDINGS; F1/F2 confirmed by
orchestrator spot-check and routed by user decision into the new REQ-021, whose lifecycle row lives in
traceability.md; F3-F6 open non-blocking; four A3 hypotheses
REFUTED with evidence, incl. the TEST-080 ESC/self-close worry and B-R019-02). Changeset STAGED via plumbing:
3 whole files (CloudService.{h,cpp}, the ASan test) + `src/Gui/MainWindow.cpp` HUNK-SPLIT to 16 lines (the comment hunk
only — the Coach owner's 4 hunks excluded; staged blob verified to contain ZERO Coach content, working tree md5-verified
untouched). Clean-checkout gate RUNNING against `git write-tree` extract (configure OK, `BUILD_TESTS=ON`; full build in
flight). Remaining: build+ctest green from clean → commit (b) feat → commit (d) governance/docs. THEN the queue is
REQ-021 (collaborator-lifetime axis, covers BOTH dialogs, F2 half is live on master) and REQ-020 (OAuth wizard).
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR-A3: **REQ-019 A3 GATE — `qgdw-adversary` dispatched 2026-08-08 against the built, Verification-Gate-PASSED
changeset (uncommitted, branch `garmin/req019-upload-uaf`).** The build is DONE and independently verified (20/20 ASan
slots, ctest 26/26, own orchestrator mutation → UAF at CloudService.cpp:418, restored byte-identical). A3 is briefed to
attack 8 specific things, chief among them: the TEST-080 `exec()`-self-protection conclusion (verifies Qt, not the fix,
and had no pre-fix RED), the DELIBERATELY-OMITTED DEC-024 close-gate (falsify "no close route exists during blocking
work"), a **sibling scan** (LSN-041 — skipping it four DECs ago is precisely how REQ-019 was born), and the latent
B-R019-02. On A3 return: disposition findings with the user, re-spawn only what fails, then the COMMIT GATE. **Note the
commit gate will need hunk-splitting again** — `src/Gui/MainWindow.cpp` carries the Coach owner's uncommitted work
alongside this changeset's comment-only hunk (LSN-007/010; never `git add -A`). AFTER commit: REQ-020 (OAuth-wizard
UAF) is the last known member of this UAF class.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR-0: **REQ-019 BUILD GATE — `qgdw-builder` dispatched 2026-08-07 on branch `garmin/req019-upload-uaf` (@ `2f0de993d`,
branched off the freshly-landed master).** DEC-029 is ACCEPTED (Option B): two-phase `start()` + heap/`WA_DeleteOnClose`,
modal `exec()` PRESERVED, store ownership UNCHANGED, DEC-024's close-gate deliberately OMITTED (no user-close route
exists during upload's blocking work — `okcancel` is unconnected until `completed()` fires). TEST-079 + TEST-080
allocated to the builder; test home = the existing ASan target `testGarminConnectSyncDialogClose` (knowing misnomer,
ORCH-008). **TEST-080 is the gating probe:** it must EXECUTE-verify that `QDialog::exec()` self-protects when `this`
dies mid-loop — the one DEC-029 claim that is reasoned, not executed, and the thing that lets `closeAndDeleteStore(db)`
stay unguarded at MainWindow.cpp:2563. If it does NOT hold, DEC-029 Option B is partly falsified and RE-OPENS (builder
was told to stop and report, not to work around it). On return: Verification Gate (contract / independent re-run +
own mutation of a guard / goal audit vs the verbatim acceptance criterion), then merge byproducts, then A3 with the
adversary. **Hazard the builder was warned about:** `src/Gui/MainWindow.cpp` is ALREADY DIRTY with the Coach owner's
uncommitted work — no `git checkout --`/`git restore` on it under any circumstances (LSN-032); `.orig` copies + `cmp`.
REQ-020 (OAuth-wizard UAF) stays queued behind this.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR-1: **DEC-029 DECISION GATE — the REQ-019 (Upload UAF, HIGH) fix shape.** → RESOLVED 2026-08-07: user chose Option B + reuse-the-Garmin-harness. `qgdw-scout` is researching the three
options against the proven DEC-024..027 recipe; on return the orchestrator runs the Verification Gate (three REAL
options? scores justified? sibling count re-verified? cascade concrete?) and PRESENTS the proposal to the user. On the
user's choice: append the full DEC entry + index line, elaborate the REQ-019 acceptance criterion in prd.md, allocate
TEST ids from WIKI (next:garmin-T-079), branch off `master`, and dispatch `qgdw-builder`. REQ-020 (OAuth-wizard UAF)
stays queued behind it. Evidence bar for this wave is non-negotiable (lifetime/ownership slice): the acceptance
evidence must be an EXECUTED ASan test that tears the parent down mid-upload, and the guard must sit on the layer that
performs the unsafe operation. Blast radius is ~11 shipped non-Garmin integrations — shared-code caution per LSN-034.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR-2: **AWAITING USER — REQ-017 closed, ORCH-001 fixed and clean-build-verified. Two open calls: (i) land the
branch on master, (ii) start the REQ-019/020 follow-up wave.** → BOTH ANSWERED 2026-08-07: (i) ff-merged, (ii) started.
1. **Branch disposition — the blocker is GONE.** `garmin/req017-lifecycle-uaf` = 4 ahead / 0 behind `master` (re-measured 2026-08-07, after the ORCH-001 docs-record commit `2f0de993d`),
   unmerged, fast-forward available. **ORCH-001 is FIXED (`427da745b`), so branch HEAD now configures, builds and
   passes 26/26 from a clean checkout — the gate that blocked verified merging for three commits.** The two prior
   Garmin commits carry an ORCH-001 caveat in their messages that is now historical, not live. Recommend merging.
   Note for whoever reviews: the merge carries 6 files of the Coach owner's test wiring (DEC-028 option B) and a
   project-wide `CMAKE_CXX_EXTENSIONS ON`.
2. **The follow-up wave — REQ-019 (Upload UAF, HIGH, live for every Upload service) + REQ-020 (OAuth-wizard UAF).**
   Both are prd.md STUBS with trace rows, not started; no DEC yet (candidate: reuse the DEC-024..027 recipe —
   two-phase init + heap/WA_DeleteOnClose + QPointer self-bails — which is now a proven pattern, so this may be a
   cheap DEC rather than a fresh three-way research). REQ-019 needs an ASan test target; none exists for Upload today.
   These are the ONLY BLOCKING items and they block only the "UAF class closed codebase-wide" claim.
3. **Cheap, available any time (open, non-blocking):** A3-R012-F3/F6/F9, B-R017-03/04/12, B-R018-01/02/03/04,
   B-R023-01, A3-R017-F2/F4, VAL-016's two coverage WARNs, DEC-024's accepted residuals.
4. **Not the agent's call:** the pre-session Coach/skill/tooling dirt (c) stays with the user.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR: **DEC-026 (the CONSTRUCTOR UAF route, A3-R025-F1) must be decided → built → A3-cleared, THEN the commits.**
-3. **SCOPE RESOLVED 2026-08-07 (user: ship sync now + new REQs).** F1→REQ-019 (Upload UAF, HIGH, prd.md stub + trace
    row), F2→REQ-020 (OAuth-wizard UAF), F3 tracked (dormant). REQ registry bumped next:garmin-021. These are a
    FOLLOW-UP wave, NOT part of the sync commit. **The DEC-024..027 sync changeset is now clear to commit.**
-2. **DEC-027 BUILT + Verification-Gate PASS (2026-08-06).** syncCloud converted; TEST-077 (4 guards RED) + TEST-078
    landed; the sync-dialog UAF class is CLOSED (A3-R027-CLOSURE, fresh-adversary confirmed). This is the committable changeset.
-1. **DEC-026 BUILT + Verification-Gate PASS (2026-08-06)** — construction route (A3-R025-F1) closed and verified; the
    A3 re-check confirmed that half sound. TEST-076 disposition RESOLVED (accept-with-rationale). NOT the end of the
    class — see -2.
    Then the commits.
0. **REQ-018 COMMITTED `fc807717b`** (2026-08-05, 4 files, +950/-1) — the dead-download fix is now on master.
   Staged via git plumbing so none of the in-flight REQ-017 work leaked in; clang-format reformatted the two new
   test files → re-verified per LSN-007 before accepting. Working tree verified intact after the commit
   (md5 `ab53ac94…`/`0cd1dfb3…`, 70 files, all REQ-017 surfaces present).
1. **DEC-025 WRITTEN + BUILDER IN FLIGHT (dispatched 2026-08-05)** — TEST-072/073(/074). Option A in three parts
   (dtor declines the delete + `QPointer` on `BlockingCall` + self-death bail on the three resuming frames), per
   O-R025-01. Closes A3-R017b-F1 AND its whole class (A3-R017b-F4), since a dtor cannot be vetoed like a virtual.
   **On return, the Verification Gate must:** re-run `ctest -R "Garmin|AtomicFile"` (≥25) + full ctest (≥26) + the
   `GoldenCheetah` link; diff `FILES` vs `git status`; and — this slice's whole point being a guard — run my OWN
   mutation, restoring the unconditional `closeAndDeleteStore(store)` in the dtor and confirming ASan reports the
   heap-use-after-free (snapshot-and-restore per LSN-032; both CloudService files are dirty, `git checkout` FORBIDDEN).
   Confirm the store leak on that path is the DELIBERATE, recorded one and not a silently widened one.
2. **Then a focused A3 re-check** on DEC-025, then the remaining commits: (b) REQ-017 + DEC-021/022/023/024/025,
   (c) workflow/tooling (guard fix, WIKI compaction, lessons), (d) docs-record.
3. **ORCH-001 is open and shapes all of it:** master does not configure from a clean checkout (8 phantom source
   refs in src/CMakeLists.txt + an untracked `unittests/Core/coach/CMakeLists.txt`), so NO commit on this branch
   can be build-verified in isolation. REQ-018 was committed with that caveat stated in its own message. Repairing
   it lands in the dirty pre-session Coach files, so it needs hunk discipline or its owner's sign-off.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR: **A3-R017b re-check IN FLIGHT (fresh adversary on the DEC-024 fix) → then the commits.**
0. **DONE this wave:** REQ-017 Slices A+B, DEC-022 Garmin half, REQ-018, DEC-023, DEC-024. Suite **25/25 Garmin ·
   26/26 full**, app links, first ASan target in the tree. VAL-017 FAIL fully remediated (DEC index backfilled
   DEC-020..023, design.md cascaded into DES-002/004/014, B-R017-06 flipped). Guard patched + 9-case matrix
   (LSN-036). WIKI compacted 18.3k→10.1k, drift lint 0.
1. **`qgdw-adversary` re-checking A3-R017-F1 only** (dispatched 2026-08-05): is the UAF gone or merely harder to
   hit (Escape, WM close, accept(), closeAllWindows, parent teardown, a queued close at a different loop level)?
   can the gate WEDGE the dialog or HANG the modal `exec()` path? can the deferred close be LOST? are the recorded
   residuals worse than stated (especially the claim that NO service blocks in `writeFile`)? does gating `done()`
   regress any of the ~15 non-Garmin services that share this dialog?
2. **Then the commits — recommend SPLITTING into three**, since they are independently reviewable and one is a
   production fix unrelated to the lifecycle work:
   (a) **REQ-018** — `downloadCompression = none` + TEST-067. Tiny, independent, fixes a feature that is DEAD on
       master today. Could land first on its own.
   (b) **REQ-017 + DEC-021/022/023/024** — the lifecycle changeset: epoch bind, ownership, `readFailed`, the UAF gate.
   (c) **workflow/tooling** — the guard fix (both copies) + WIKI compaction + lessons. Not Garmin product code.
   All path-scoped. This wave is the FIRST to leave `src/Cloud/`: it carries `src/Cloud/CloudService.{h,cpp}` and
   `src/Gui/MainWindow.cpp`. **`MainWindow.cpp` and `src/CMakeLists.txt` are dirty with unrelated pre-session Coach
   edits ⇒ HUNK-SPLIT; NEVER `git add -A`** (LSN-010/007). Pre-commit runs clang-format on these files for the first
   time ⇒ re-verify suite + app-link after any reformat, then the docs-record commit.
3. **Open, not blocking:** A3-R017-F2 (TEST-062 checks readCompleteCount but not readFailedCount — mutation-confirmed
   overclaim), A3-R017-F4, B-R023-01 (entry guards still on the empty-payload heuristic DEC-023 rejected),
   B-R017-03/04/12, B-R018-01/02(i)(ii)/03/04, A3-R012-F3/F6/F9, VAL-016's two coverage WARNs, and DEC-024's
   accepted residuals (`writeFile` unguarded, direct-delete bypass, `detect_leaks=0`).
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR: **A3-R017 + VAL-017 IN FLIGHT (dispatched in parallel 2026-08-04, both read-only) → disposition → commit(s).**
0. **All five build slices DONE + Verification-Gate PASSED** (REQ-017 A, REQ-017 B, DEC-022 Garmin half, REQ-018,
   DEC-023). Suite **24/24 Garmin · 25/25 full**, app links, all uncommitted working tree.
1. **`qgdw-adversary` running A3-R017** with four mandated first probes: (i) re-confirm clause (a) independently on
   FINAL code by stubbing `accountStillConnected()` true; (ii) B-R017-02 — hunt a PRODUCTION path reaching
   readFile/readdir without `open()`, which would collapse the lazy latch into a self-bind; (iii) B-R017-08 +
   B-R023-02 — closing the sync dialog inside `blockingDownload`'s nested QEventLoop (leak→UAF), and the
   queued-vs-direct ordering where the loop quits BEFORE `readFailed` frees the buffer; (iv) **"what else is green
   but dead?"** — REQ-018 proved the suite's integration blind spot is real, so hunting more uncrossed boundaries is
   now a first-class probe. Briefed with live git state + the LSN-032 snapshot-and-restore mandate (EVERY file it
   might mutate is dirty; `git checkout --` is forbidden).
2. **`qgdw-validator` running VAL-017** (incremental). Briefed that uncommitted is EXPECTED (not a FAIL — the
   LSN-023 false-FAIL), that all ledger merges are ALREADY done (LSN-016 merge-lag), and that **design.md has NOT
   been updated for the epoch / readFailed / ownership contract — I expect that to come back as a legitimate
   finding** naming which DES entries to cascade.
3. **Then disposition + commit(s).** Likely split: REQ-018 is a small independent production FIX (`downloadCompression`
   one-liner + TEST-067) that stands alone and could land first; REQ-017+DEC-021/022/023 is the large lifecycle
   changeset. Either way path-scoped, and this is the FIRST Garmin commit to leave `src/Cloud/` — it carries
   `src/Gui/MainWindow.cpp` and `src/Cloud/CloudService.{h,cpp}`. **`MainWindow.cpp` and `src/CMakeLists.txt` are
   already dirty with unrelated pre-session Coach edits ⇒ HUNK-SPLIT; NEVER `git add -A`** (LSN-010/007).
   Pre-commit runs clang-format on these files for the first time ⇒ re-verify suite + app-link after any reformat.
4. **Open, not in these slices:** B-R023-01 (entry guards still on the empty-payload heuristic DEC-023 rejected),
   B-R017-03/04/12, B-R018-01/02(i)(ii)/03/04, B-R023-02/03/04, A3-R012-F3/F6/F9, VAL-016's two coverage WARNs.
--- superseded (kept for provenance) ---
NEXT_GATE-PRIOR: **DEC-023 (`readFailed`) BUILDING → then A3 + CLV → then commit(s). REQ-018 DONE.**
1. **REQ-018 DONE + Verification-Gate PASS 2026-08-04** (working tree). `downloadCompression = none` in BOTH ctors
   (GarminConnect.cpp:102/:110); TEST-067 crosses the `uncompressRide` boundary for FIT + TCX. Both briefing
   premises were CHECKED by the builder and confirmed. I ran my own mutation (`none`→`zip`): both slots went RED
   with the real production text, restored byte-identically (md5 `bd962996…`), 23/23 green. **B-R017-09 FIXED.**
2. **`qgdw-builder` building DEC-023 now** (dispatched 2026-08-04): TEST-068/069. New
   `CloudService::readFailed(data,name,reason)` + `notifyReadFailed`; GarminConnect's FIVE remaining silent
   `return false` sites (**`:502` `:513` RateLimit `:524` `:532` `:539` TCX-also-failed** — the last two are
   ORDINARY failures, so an everyday failed download hangs the dialog today) emit it with distinguishable reasons;
   both consumers show the reason, free the buffer, advance. Folds in B-R018-02(iii) (AutoDownload discards
   `errors`) and B-R017-11 (readFile/readdir wording drift). **HAZARD flagged to the builder:**
   `CloudService.cpp:1883` quits a blocking QEventLoop on `readComplete` ONLY — `readFailed` must also release it
   or auto-download trades one hang for another. Siblings must stay untouched (that is what bounds the blast radius).
3. **Then A3-R017 adversary + incremental CLV in parallel** (both read-only). MANDATED first probes: (i) clause (a) —
   neutralise `accountStillConnected()` and confirm the epoch ALONE still stops the exploit (I did this once and it
   held; the adversary confirms independently on final code); (ii) B-R017-02 — is there a PRODUCTION path reaching
   `readFile` without `open()`? if yes the lazy latch must become open()-only; (iii) B-R017-08 — closing the sync
   dialog DURING a `blockingDownload` nested QEventLoop, where Slice B's teardown could turn a leak into a UAF;
   (iv) hunt for MORE integration boundaries no test crosses — B-R017-09 proves the suite's blind spot is real, so
   "what else is green but dead?" is now a first-class probe. Paste live `git status`/`git log` (LSN-023).
4. **Then the commit(s).** Consider splitting: REQ-018 is a small independent production FIX and could land first on
   its own; REQ-017's lifecycle work is the bigger changeset. Either way path-scoped — the tree carries unrelated
   pre-session Coach/Gui/CMake/vcpkg/skill edits, and BOTH `src/Gui/MainWindow.cpp` and `src/CMakeLists.txt` are
   already dirty with them ⇒ HUNK-SPLIT, NEVER `git add -A` (LSN-010/007). Pre-commit runs clang-format on these
   files for the first time ⇒ re-verify suite + app-link after any reformat.
5. **Still open, not in these slices:** A3-R012-F3/F6/F9, B-R017-03/04/12, VAL-016's two coverage WARNs.
--- superseded (kept for provenance): the DEC-022 build gate ---
NEXT_GATE-PRIOR: **DEC-022 fix slice IN FLIGHT, then A3-R017 + CLV, then ONE commit.**
1. **`qgdw-builder` building now** (dispatched 2026-08-03): TEST-065 (a refused readFile posts a labelled completion
   so the loop ADVANCES and the buffer is freed — both fail-closed paths, queued not synchronous per the
   A3-R007-01 idiom) + TEST-066 (`completedRead` surfaces `message` on the `ride == NULL` branch ONLY, with a
   positive control that the SUCCESS path — which already carries `tr("Completed.")` — is not relabelled as an
   error, and a fallback control that an empty message keeps today's `errors.join(" ")` for the ~15 other services).
   Folds in B-R017-07 (`MainWindow::uploadCloud`'s identical leak, `uploadCloud` hunk ONLY).
2. **Verification Gate** on return — `ctest -R "Garmin|AtomicFile"` ≥ 20/20, full ctest ≥ 21/21, `GoldenCheetah`
   links, FILES vs `git status`, and specifically: trace buffer ownership on the refusal path. `completedRead`
   already does `delete data` (CloudService.cpp:1546); once a refusal posts a completion that delete runs on the
   refusal path too, so confirm nothing else frees it — **getting this wrong turns a leak into a double-free**.
3. **A3-R017 adversary + incremental CLV in parallel** (both read-only). MANDATED first probes for A3: (i) clause
   (a) — neutralise `accountStillConnected()` and confirm the epoch ALONE still stops the exploit (I have already
   done this once and it held; the adversary must confirm independently on the final code); (ii) B-R017-02 — is
   there a PRODUCTION path that reaches `readFile` without `open()`? If yes the lazy latch must become open()-only;
   (iii) B-R017-08 — closing the sync dialog DURING a `blockingDownload` nested QEventLoop, the one place Slice B's
   teardown could turn a leak into a use-after-free. Paste live `git status`/`git log` (LSN-023 — agents are git-blind).
4. **Then ONE path-scoped commit.** First Garmin commit to leave `src/Cloud/`: it also carries
   `src/Cloud/CloudService.{h,cpp}` and `src/Gui/MainWindow.cpp`. **Both `MainWindow.cpp` and `src/CMakeLists.txt`
   are already dirty with unrelated pre-session Coach edits ⇒ HUNK-SPLIT; NEVER `git add -A`** (LSN-010/007).
   Pre-commit runs clang-format on these files for the first time — re-verify the suite + app-link after any
   reformat before accepting the commit. Then the docs-record commit.
5. **Not in scope, still open:** A3-R012-F3/F6/F9, B-R017-03/04, VAL-016's two coverage WARNs.
--- superseded (kept for provenance): the DEC-021 decision gate ---
NEXT_GATE-PRIOR: **REQ-017 gate — DEC-021 (lifecycle-binding mechanism) must be decided before any code.**
1. **`qgdw-scout` researching DEC-021 now** (dispatched 2026-08-03): three real mechanisms for deterministically
   invalidating live GarminConnect sessions on disconnect + who owns/destroys an opened CloudService. Candidate
   shapes handed over: a live-service registry broadcast to on disconnect; an account-generation/epoch latched at
   open() and compared without a disk read; owner-responsibility (real dtor + close() on CloudServiceSyncDialog,
   fix syncCloud, route Disconnect through the LIVE instance — which reopens DEC-019's fresh-instance sub-choice).
2. **Verification Gate on the draft** — three genuinely distinct options? scores justified? clause-by-clause a–e
   coverage stated including what each option does NOT satisfy? cascade concrete (does shared CloudService.h change,
   is DEC-019 reopened, which GUI files)? volatile Qt-lifetime claims sourced? Spot-check its cited file:line facts.
3. **Orchestrator presents DEC-021 to the user** (the scout never presents). On the user's choice: append the full
   DEC entry + index line, patch DES-002/004/014 as the cascade requires, allocate TEST ids from REGISTRIES
   (next: garmin-T-060), then spawn `qgdw-builder` with the REQ-017 criterion quoted verbatim.
4. **RIGOR FULL** ⇒ after the build: A3-R017 adversary (its FIRST probe must be clause (a) — neutralise
   `accountStillConnected()` and confirm the binding alone stops the exploit; a fix that only works because the
   DEC-020 guard is still there does not satisfy REQ-017) + an incremental CLV, then ONE path-scoped commit.
   Expect the commit to reach OUTSIDE src/Cloud for the first time (src/Gui/MainWindow.cpp, CloudService.{h,cpp}) —
   path-scoping still applies, NEVER `git add -A` (LSN-010/007); the tree carries unrelated pre-session
   Coach/Gui/CMake/vcpkg/skill edits, and MainWindow.cpp/CMakeLists.txt are ALREADY dirty with them ⇒ hunk-split.
5. **Not in REQ-017 scope** (still open, cheap, available any time): A3-R012-F3 (ordering invariants unobserved),
   F6 (discarded clearAccount bool / silent failed delete), F9 (uid unvalidated as a filename component). Also open:
   VAL-016's two coverage WARNs and the F7/F11 accept-notes.
--- superseded (kept for provenance): the post-REQ-012 idle gate ---
NEXT_GATE-PRIOR: **AWAITING USER — pick the next REQ. Nothing is blocked; REQ-012 is fully closed (feature + ledger).**
1. **Docs-record commit DONE `9ba1d33d3`** (2026-08-03, 7 files: STATE/WIKI/lessons + workflow-garminconnect
   {decisions,design,findings,traceability}.md), mirroring `b47954308`. Path-scoped, pre-commit + DEC-015 drift lint
   clean. Working tree now carries NO Garmin files — only the unrelated pre-session Coach/Gui/CMake/vcpkg/skill edits.
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

BLOCKING: **NONE OPEN as of 2026-08-15 — `A3-R027b-F1` and `A3-R027b-F2` are BOTH FIXED (one root cause) and the fix is
orchestrator-verified by execution in the environment that used to fail.** The gate was widened FIRST and proven to go
RED on the unfixed tree, then the fixture was repaired; production `CloudService.{cpp,h}` were never touched and are
byte-identical to the pre-task baseline. **My evidence, executed:** ctest both registrations `100% passed, 0 failed
out of 2` · **ambient wayland full target 49/49 TWICE, where both slots previously failed 3/3** · **my OWN out-of-tree
mutations under WAYLAND — deleting `completedRead`'s abort guard fails on the criterion's own assertion, deleting its
`self.isNull()` gives `heap-use-after-free … completedRead`** — so the fixture's sensitivity is backend-INDEPENDENT
now, not just its green runs · goal audit: a mechanical diff of every `QCOMPARE`/`QVERIFY` payload shows **zero
assertions present before and absent after**, and the CMakeLists change has no deleted lines. Residuals → ORCH-017,
none blocking. **[[LSN-062]] was promoted to MECHANISM one day after capture** (the dual-backend registration) — the
third mechanism on this ledger. Prior: **ONE OPEN as of 2026-08-15 — `A3-R027b-F1`, and it is a finding about the EVIDENCE ITSELF, not about a
guard.** Run the ASan target the way the briefing specifies, with no environment override, and it is **47 passed,
2 failed**, deterministically — one failure being this slice's own TEST-102, with `a further store->readFile was issued
after the user aborted (2 reads, expected 1)`, the exact harm A3-R027-F4 exists to prevent. **DIAGNOSED 2026-08-15 = H1, a TEST-HARNESS
artifact; the production guard is sound.** A nested `QApplication::processEvents()` is one non-blocking
`g_main_context_iteration` and is **NOT a guaranteed drain**, so the fixture's queued abort is pending-but-undispatched
when the guard reads it; forcing `sendPostedEvents` turns that site green and moves the failure to the next site with
the same fixture shape, proving the event was reachable all along. **INHERITED, not introduced — I ran the builder's
HEAD build myself: `offscreen` 42/0, `minimal` 41/1** on exactly the committed slot. **CORRECTED MATRIX** (my first
`minimal` datapoint was taken while two agents saturated the machine and did not hold up; stable idle-machine,
full-suite): **wayland 47/2 (both slots, 3/3) · `minimal` 48/1 (committed slot only; TEST-102 passes 4/4, and still
passes under synthetic 8-core load) · `offscreen` 49/49**. So TEST-102 fails under **wayland specifically — the
environment a developer actually runs in**. Both headless backends still disagree, so this is backend-sensitive
scheduling, not "needs a display". **And the failing test PASSES when run alone (5/5) — it needs the full-suite
context, so single-test triage reports green and misleads.** **Surviving H2-flavoured residual, a DEC question and not
a test fix:** DEC-032's guard is best-effort BY CONSTRUCTION; the placement immune to delivery timing is at the START of
the next transfer, not the completion slot's tail.
`unittests/Core/garminconnect/CMakeLists.txt:1456` pins `offscreen` for ctest, so every gate that ran through ctest —
builder run, my re-run, CLV, and the clean-worktree build gate — inherited the one backend that hides it. **One of the
two failing slots is COMMITTED AT HEAD** (`6dc794caf`, the REQ-021 wave), so this is at least partly inherited rather
than introduced; our diff only ADDS early-return guards, which can only reduce `readFileCalls`. **Not yet established:
test-harness artifact vs production defect** — different fixes, and the adversary declined to guess. `A3-R027b-F2`
(the second slot's own race, un-root-caused) is sequenced behind F1's diagnosis. **Do NOT pin the developer's shell to
`offscreen` to make this go away** — that re-hides what was just found ([[LSN-036]]). → [[LSN-062]].
Prior: **NONE OPEN as of 2026-08-12 — `A3-R021b-F1` is FIXED and the fix is orchestrator-verified by execution.**
Closed under **REQ-026** (TEST-093 + TEST-094), both **RED-verified before the fix**. Two additive production hunks and
nothing else: `if (aborted == true) return true;` in `uploadNext` (now `CloudService.cpp:2420`, beside the existing
`self.isNull()` bail and deliberately NOT relabelling the row — row `i` really did fail to parse; what stands down is the
LOOP), and the symmetric `delete ride` + "Aborted" + `return` block in `completedRead` (:2239), mirroring the entry check
at :2187. **A3-R021b-F3 turned out DRIVEABLE, so it was BUILT rather than deferred as an untestable guard.**
**Gate evidence, orchestrator-EXECUTED not read:** ASan target **42/42** (was 40) and full **ctest 26/26** re-run here ·
FILES reconciled against `git status` (exactly 2 files moved, `CloudService.h`/`Context.cpp`/`ImportSeamStubs.cpp`
byte-identical, no new files, no CMake edit) · **my own mutation removed BOTH new guards SIMULTANEOUSLY** — the builder
had only reverted them singly — and **exactly the 2 new slots FAILED while the other 40 stayed green**, so each guard is
independently load-bearing AND neither propped up a pre-existing slot · restored byte-identical (md5
`a722806a9aa95baa5b38832eab648265`, `cmp` silent, zero `.orig` residue) and re-confirmed 42/42.
**Goal audit PASS:** TEST-093 encodes BOTH halves of the criterion as written — `writeFileCalls == 0` AND
`rideOpens == 0` — behind seven anti-vacuity premise assertions, including that the button really was labelled "Abort"
and that `downloadClicked` really took its abort branch. Not a weaker paraphrase.
**One NEW defect found by the builder and confirmed by the orchestrator, deliberately NOT fixed (B-R026-01):** a SILENT
STALL. `syncNext`'s parse-failure branch returns without arming ANY of syncNext's four re-entry points
(:2273/:2326/:2472/:1962), because a parse failure emits no async work at all — so a sync with one unparseable local file
freezes on that row forever. A hang, not an over-transfer; out of REQ-026's scope, needs its own REQ. → [[LSN-057]].
Also carried: B-R026-02..06 (harness `rideMetadata` UB; both slots observe "the call was made" not "bytes moved"; `.tcx`
vs `.fit` fidelity; row-order asserted not controlled; only one of two abort routes executed) and **O-R021-06** (my
briefing put the "Abort" relabel at :1918 — it is :1924; :1918 is the `return`. Caught by the builder, no code impact,
[[LSN-034]] now recur:5 and a mechanism candidate).
Still open, NON-blocking, unchanged by this build: **A3-R021b-F2** (file-IO third layer, `src/FileIO/RideFile.cpp:999`) ·
**B-R025-03** (the `blockingCallDepth` predicate decision — deliberately left queued; the four bare `processEvents()`
were NOT wrapped) · **ORCH-014** (CLV Check 5's grep).
Superseded (kept for provenance): **ONE OPEN as of 2026-08-12 — `A3-R021b-F1`, raised by the A3 re-clear and CONFIRMED by the orchestrator.**
**`CloudServiceSyncDialog::uploadNext` (CloudService.cpp:2314-2411) never READS `aborted`.** Every read of that member in
the whole file is at :2187 (`completedRead`), :2292 (`failedRead`) and :2421 (`completedWrite`) — grep-verified, one line
per read; inside `uploadNext` the identifier appears only at :2399, as an ASSIGNMENT in the completion tail. So an abort
arriving during the parse-failure branch's `processEvents()` (:2378) clears the `self.isNull()` bail at :2385 — a
MEMORY-SAFETY check, not a stop-when-asked check — and the loop **falls through to the next row and compresses + uploads
it to the cloud service after the user said stop.** The success path is safe only incidentally (it `return`s, and
re-entry goes through `completedWrite`'s :2421 check), so the gap is exactly the one branch that suspends and keeps
iterating. **Untested by construction:** TEST-087's `UploadNextParsePE` slot (test file :3988, :4083-4101) drives PARENT
TEARDOWN into this branch, never a user abort — all 40 green ASan slots are silent on it.
**Orchestrator spot-check found it MORE reachable than reported (O-R021-05):** the adversary argued it only through
`deferCloseIfBusy()` (:1471-1478, gated on `blockingCallDepth > 0`), but `downloadClicked` (:1908) relabels the Download
button to "Abort" (:1918) mid-batch and sets `aborted=true` at :1916 **synchronously** — no deferral, no depth condition,
no teardown. The primary documented abort control reaches it. → [[LSN-054]], [[LSN-055]].
This is an outward-facing effect (ride data sent to a third party post-cancel) and it **blocks the commit gate**.
Also newly open, NON-blocking: **A3-R021b-F2** (the file-IO third layer, `src/FileIO/RideFile.cpp:999` — orchestrator-
verified as exactly `if (context) result->setTag("Athlete", context->athlete->cyclist);`, the null check guarding the
POINTER and never the object; needs its own REQ, and the harness is deliberately blind to it) · **A3-R021b-F3**
(`completedRead` reads `aborted` at entry only, :2187 — same root cause as F1, lower stakes) · **ORCH-014** (CLV Check
5's blocking-finding grep has been vacuous since 2026-08-08 because findings.md's severity column switched from a bare
token to bold prose — a mechanism whose predicate stopped matching its data, ORCH-010's class; repair, don't route
around).
Superseded (kept for provenance): **NONE OPEN as of 2026-08-12.** All three blockers raised earlier this wave are CLOSED and independently re-verified:
A3-R021-F1 (four `processEvents()`→`this` sites the reparent exposed) and A3-R021-F2 (the EXECUTED `refreshClicked` UAF)
were fixed in the A3 remediation; **B-R031-01** (DEC-031's reaper firing under an enclosing UNCOUNTED frame — it
converted a latent leak into a live UAF on GarminConnect's mainline `.fit` path) was closed by REQ-025, RED-verified by
the builder and reproduced independently by the orchestrator before and after.
**Nothing blocks the commit gate on evidence.** What remains open is a queue of NON-blocking findings and new REQs
(see NEXT_GATE), plus one honest caveat worth carrying into any A3: **B-R025-03 — `blockingCallDepth` is still not a
complete predicate, and DEC-031's reaper reasons as though it were.** It is sound today because the four uncounted
`processEvents()` sites are each guarded, which is a weaker guarantee than the DEC's own reasoning assumes.
Superseded (kept for provenance): **A3-R021-F1 + A3-R021-F2 (2026-08-11, A3 on REQ-021) — the REQ-021 fix is INCOMPLETE, and one hole is an
EXECUTED use-after-free.** **F2 (executed):** the Part-2 rider has a hole on the tail path — `start()` calls
`refreshClicked()` at CloudService.cpp:1203 and only checks `ctx.isNull()` at :1204, while `refreshClicked`'s own
post-`readdir` guard at :1392 is `self`-ONLY and it then walks `context->athlete->rideCache->rides()` at :1401. The
adversary re-pointed one existing slot's suspension (one token) and got `heap-use-after-free READ of size 8` at :1401.
This directly contradicts the changeset's own comment at :919-923. **F1 (read, orchestrator-confirmed at all four sites):
the REPARENT ITSELF newly exposes four unguarded `processEvents()` → call-on-`this` sequences** in the modeless
completion slots (:1989→:1991 `completedRead`, :2036→:2038 `failedRead`, :2125→:2127 `completedWrite`, :2083→:2047
`uploadNext`) — reachable now because `delete tab` is SYNCHRONOUS from inside ordinary event delivery, which
`processEvents` delivers, whereas MainWindow died only via a posted `DeferredDelete`, which it does not. **The reparent
closed the `context` axis in those slots and opened a `this` axis in the same slots** → [[LSN-051]].
Orchestrator spot-checked F1 (all four), F2 (full control flow) and F4 (both quoted clauses) and CONFIRMED all three.
**AWAITING USER DISPOSITION.** Also open from this A3: F3 (`tab = NULL` is the WRONG policy — MEASURED: a parentless
dialog blocks nothing, survives MainWindow close, and keeps `quitOnLastWindowClosed` from firing; adopt the
`context->tab ? context->tab : context->mainWindow` fallback), F4 (DEC-025's premise FALSIFIED — its deliberate store
leak was justified by "the application is already tearing down", but parent teardown is now a ROUTINE athlete close, so
a once-per-exit leak became once-per-close, and TEST-084 asserts it as REQUIRED → [[LSN-052]]), F5/F6/F7 (three
acceptance clauses unmet or unfalsifiable as written), F8 (the ENTIRE sync transfer loop is green but DEAD — no test
executes any of it, which is where all nine S-R021-02 derefs and all four F1 holes live), F9/F10/F11.
**A3 also REFUTED seven hypotheses with evidence**, including O-R021-02(a) reachability (by control-flow proof, not
"unlikely"), B-R021-08 sync modality (MEASURED identical blocked set), and the reparent creating a new sibling class
(no `parentWidget()`/`window()` walks anywhere in src/Cloud).
Superseded (kept for provenance): **A3-R019-F1 + A3-R019-F2 (2026-08-08, A3 on REQ-019) — a SECOND UAF axis nobody had looked at:
the guards protect the DIALOG's lifetime; NOTHING protects `context`/`item`.** The dialog is parented to MainWindow,
but Context/RideItem belong to the narrower-lived AthleteTab, and `MainWindow::removeAthleteTab` (MainWindow.cpp:2183-85)
deletes them SYNCHRONOUSLY while MainWindow's own WA_DeleteOnClose deletion is DEFERRED — so the dialog reliably
OUTLIVES its Context, every `self.isNull()` bail stays false, and `start()` resumes onto freed memory at
CloudService.cpp:424 and :443-445. Reachable by ordinary window close (closeEvent :1103) AND by single-tab close
(`AthleteView.cpp:213` → `closeAthleteTab`). **F2: the identical gap is in the SHIPPED sync dialog** (CloudService.cpp:1070,
:1094-1099 — REQ-017/DEC-026/027, commit `ae5a7a8ab`, already on master), which weakens the A3-R027-CLOSURE "sync class
CLOSURE assertion, and the "proven recipe" premise that decision leaned on.
Orchestrator spot-checked all four cited sites for F1
and F2 and CONFIRMED. NOT executable in the current harness (MainWindow/Context/RideItem are inert stubs in
`ImportSeamStubs.cpp`) and TEST-079's `killOwner` structurally cannot express it (it deletes owner+context back-to-back,
so the dialog always dies first). **AWAITING A USER SCOPE DECISION** — same shape as the A3-R027-F1/F2 call. Lessons
[[LSN-046]] (self-guard ≠ collaborator guard) + [[LSN-047]] (inert stubs cannot prove the fault they are credited with).
The REQ-019 dialog-lifetime fix itself is DONE, verified and NOT in question — it closes the axis it targeted.
Superseded (kept for provenance): **A3-R027-F1 (Upload path) + A3-R027-F2 (OAuth wizard auth) — the SAME UAF class in SIBLING dialogs, both
PRE-EXISTING and OUT of REQ-017 scope.** These block a "class closed codebase-wide" claim but do NOT block committing
the DEC-024..027 SYNC changeset (which is verified done — the sync class IS closed, A3-R027-CLOSURE, and the changeset
neither introduces nor worsens F1/F2). **Disposition is a user SCOPE decision** (ship sync + new REQs for F1/F2, or
expand now). F1: `CloudServiceUploadDialog`/`CloudService::upload`/`MainWindow::uploadCloud` — stack dialog + unguarded
ctor nested loop, live for every Upload service. F2: `AddCloudWizard::AddAuth::doAuth` `oauthDialog->exec()` + unguarded
member touches, live for every OAuth service. F3 dormant. The DEC-024..027 sync work is the committable changeset.
Superseded detail follows: **A3-R026-F1 (the STACK syncCloud caller) FIXED 2026-08-06 by DEC-027, and the DEC-024/025/026/027
sync-dialog class CLOSURE was CONFIRMED by the fresh DEC-027 A3 re-check (A3-R027-CLOSURE).**
Superseded (kept for provenance): "— none open. A3-R025-F1 FIXED 2026-08-06 (DEC-026 two-phase init, TEST-075),
Verification-Gate PASS." Open non-blocking: A3-R025-F2/B-R025-02 (syncNext/downloadNext dead-code guards — accept-with-rationale,
resolved), A3-R017b-F2 (writeFile symmetry, deferral confirmed safe). Superseded detail of the finding
follows: **A3-R025-F1 — the CONSTRUCTOR was a FOURTH route to the identical use-after-free.**
The DEC-025 A3 re-check RAN 2026-08-06 (fresh `qgdw-adversary`, verdict FINDINGS; orchestrator spot-checked F1 at
CloudService.cpp:719/725/734/972 and CONFIRMED). DEC-025 closes the two enumerated POST-construction routes (proven
by the adversary's mutations) but does NOT close the root-cause class it claims (A3-R017b-F4): `CloudServiceSyncDialog`'s
constructor (:709-974) runs `store->open()` (:725, 30s nested loop), `QMessageBox::exec()` (:734, UNBOUNDED) and the
tail `refreshClicked()` (:972) with no `QPointer` self-bail, so a parent teardown frees the half-built dialog under
its own ctor. Reachable via AddCloudWizard.cpp:892 (heap, modeless). **This escalates B-R025-01 from non-blocking to
BLOCKING and needs its own DEC-026** (candidates: A ctor QPointer self-bails, B two-phase init / move blocking work
to a `start()` slot, C reparent-to-null). Superseded (kept for provenance): "— none open. A3-R017b-F1 FIXED 2026-08-06
(DEC-025, TEST-072/073), ASan-verified in both directions; A3 re-check had not yet run."
Open non-blocking from the slice: A3-R025-F2/B-R025-02 (syncNext/downloadNext guards NOT load-bearing — proven dead
code by 3 independent mutations; refreshClicked's IS), A3-R017b-F2 (writeFile symmetry, deferral CONFIRMED safe by
A3-R025-F3 across all 16 subclasses), A3-R025-F4 (test uses direct `delete owner` vs real deleteLater chain — adequate).
A3-R025-F5 refuted the non-Garmin regression concern (TEST-073 positive control).
Superseded detail of the finding follows: **A3-R017b-F1 (2026-08-05): a SECOND route to the same use-after-free, via PARENT
TEARDOWN, which the DEC-024 gate cannot structurally intercept.** Qt destroys child widgets DIRECTLY when the
parent dies — no `closeEvent`, no `done`, no virtual dispatch — so `~CloudServiceSyncDialog` still runs
`closeAndDeleteStore(store)` with a nested `QEventLoop` on the stack. Orchestrator-verified: dialog parented to
`context->mainWindow` (CloudService.cpp:709), **MainWindow itself carries `WA_DeleteOnClose`** (MainWindow.cpp:143),
and the dtor (CloudService.cpp:981-987) has NO depth check. Adversary proved the Qt semantics with an isolated
repro: `delete parent` ran only the real destructor, neither gated override fired. Exposure is up to **60 seconds**
per call (GarminConnect.cpp:41-43), triggered by closing the athlete's MainWindow mid-sync — which the modeless
dialog exists to permit. **Pre-Slice-B there was no destructor, so this path LEAKED rather than crashed: our
changeset introduced this route too.** Affects all ~16 CloudService subclasses, not just Garmin. Root cause
(A3-R017b-F4): the guard is on close-INITIATION, not on the unsafe operation (deleting the store while
`blockingCallDepth > 0`) — a destructor cannot be vetoed like a virtual, so the check must move INTO the dtor.
**Needs DEC-025 before any commit.** Prior: **A3-R017-F1 FIXED 2026-08-05 (DEC-024, TEST-070/071, ASan-verified
both directions) — closed for all three self-close routes plus the modal `exec()` path; the adversary confirmed
that half and said so plainly.**
The gate lives on `QDialog::done(int)`, the choke point close/reject/accept/Escape all funnel through, plus an RAII
depth counter around all four nested-loop call sites, replaying the close once the count reaches 0. **DEC-024 as I wrote it was
INSUFFICIENT — the builder proved it rather than following it:** on Qt 6.8.2 `reject()`/Escape destroy a
`WA_DeleteOnClose` dialog even when `closeEvent()` ignores, because `QDialog::closeEvent` routes to `reject()`→`done()`.
Amendment ratified in decisions.md. I re-ran the decisive mutation myself: removing the `done()` gate reproduces the
ASan `heap-use-after-free`; restored byte-identically (md5 `0cd1dfb3…`), 26/26 green. **A3-R017-F1 must still be
re-checked by a fresh adversary before the commit** (re-spawn the failed role until clean — RIGOR: FULL).
Superseded detail: Slice B's `WA_DeleteOnClose` had turned the pre-existing leak
into a USE-AFTER-FREE. ASan-reproduced by the adversary with two independent harnesses; all four cited sites
orchestrator-confirmed on disk.** `AddCloudWizard.cpp:899` sets `WA_DeleteOnClose` on a MODELESS sync dialog;
`GarminConnect::readFile`/`readdir` run nested `QEventLoop`s so GUI events are processed mid-call; closing the
dialog then (X, or `cancelClicked()` at CloudService.cpp:980 which `reject()`s regardless of `downloading`) runs
`~CloudServiceSyncDialog` → `closeAndDeleteStore(store)` → deletes the GarminConnect **while readFile is still
executing on it**, and execution resumes after the nested loop touching freed members. No `closeEvent()` override
exists. **This is a REGRESSION INTRODUCED BY THIS CHANGESET** and precisely the hazard the build briefing named
("turns a leak into a UAF, strictly worse than the bug we are fixing") — the builder reasoned it safe from Qt's
DeferredDelete loop-level semantics and did not test it; the adversary executed it and it crashes. **NOT a flaw in
DEC-021's epoch mechanism**, which survived all five mutations — it is a flaw in REQ-017(e)'s teardown, the very
compensating control DEC-021 leans on. `MainWindow::syncCloud` is UNAFFECTED (stack-allocated, never sets the
attribute; it is even commented out at CloudService.cpp:436 for exactly this reason). **Must be dispositioned as
DEC-024 and fixed before any commit; the fix must ship with an ASan-backed test.** Prior (still true):
**B-R017-06 FIXED 2026-08-05 (DEC-022 GarminConnect half + DEC-023), B-R017-09 FIXED
(REQ-018). A3-R017 still running; it may raise new ones.** B-R017-06's stall+leak is closed on every path: the two
entry guards post a labelled completion (TEST-065) and the five mid-flight sites post `readFailed` (TEST-068), with
the dialog showing the reason, freeing the buffer and advancing (TEST-069). I mutation-proved the consumer half
myself — neutering the connect at CloudService.cpp:753 stalled TEST-069 at 1 of 3 activities; restored byte-identically
(md5 `64e77dfa…`), 25/25 green. B-R017-09 (every successful download rejected by `uncompressRide`) closed by
REQ-018's `downloadCompression = none` + TEST-067, also mutation-proved (`none`→`zip` ⇒ RED with the real production
text). Superseded detail of B-R017-06 follows: a superseded/disconnected
`readFile` STALLED the sync loop instead of erroring it. `CloudServiceSyncDialog::syncNext` (CloudService.cpp:1413)
and `downloadNext` (:1495) call `store->readFile(data,…)`, **discard the bool**, and wait for a `readComplete` signal
to advance; `GarminConnect::readFile`'s two fail-closed paths (`sessionSuperseded()` :452, `accountStillConnected()`
:460) return false and post NO completion — so the dialog hangs at "Downloading n of N" and the per-attempt
`new QByteArray` (CloudService.cpp:1412/1494, "gets deleted when read completes") leaks. Reachable by exactly the
A3-R012-F1 scenario REQ-017 exists to fix. **NOT introduced by REQ-017 — DEC-020's guard is identically shaped, so
this is ALREADY SHIPPED in `f001c7d20` (REQ-012); Slice A widens it.** Security intent holds (nothing downloads);
the failure MODE is wrong, and REQ-017(a) demands an error, which a stall is not. Surfaced by the Slice-B builder's
report-only ErrorBus feasibility read — a question asked as an aside found a shipped defect. Note `ErrorBus` DOES NOT
EXIST in the tree (only DES-008 prose + a TODO at GarminConnect.cpp:649). Awaiting user disposition.
Prior (still true): **A3-R012-F1 FIXED (mitigated) 2026-08-03 in `f001c7d20`.** DEC-020 Option C landed:
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

LAST_CLV: **VAL-017 — 2026-08-05 — incremental over REQ-017 + REQ-018 + DEC-021/022/023 (T-060..069 + spine).
Verdict FAIL on check 6, ALL findings REMEDIATED this pass by the orchestrator; re-run of the deterministic lint is
now clean.** Checks 1:P 2:P 3:W 4:W 5:W 6:F 7:P 8:P 9:W. The FAIL: traceability.md's `## DEC index` table stopped at
**DEC-019** while DEC-020/021/022/023 were fully specified in decisions.md and actively cited by rows in the SAME
file — DEC-020 had been missing since REQ-012, so the index has been silently falling behind for two REQs. I
spot-checked and confirmed it, then appended the four rows (and caught that my first insert put them in descending
order — table is ascending — and fixed it). **Check-3/9 WARN (the cascade I predicted when dispatching):** design.md
had ZERO mentions of `epoch|readFailed|closeAndDeleteStore|sessionSuperseded` — grep-confirmed — even though
DEC-021's own cascade note earmarked "DES-002/DES-014 (epoch + lifecycle prose to be added at build)". Remediated:
DES-002 gained the account-epoch section (incl. WHY it is deliberately not persisted and why `bump()` touches
nothing but the map), DES-004 gained the `downloadCompression` contract + the refusal-reporting contract, DES-014
gained the three layered contracts (session binding / explicit failure channel / store ownership); the three DES
index rows were refreshed. **Check-5 WARN:** B-R017-06 still read "open — BLOCKING" after being fixed — flipped to
FIXED with the mutation evidence, and STATE.BLOCKING refreshed. **Validator was right about all of it.** It also
could not run `ctest` or the lint (no Bash/git tools) and SAID SO rather than guessing — the LSN-023 discipline
holding. **Byproduct discovery:** running the drift lint myself returned **71 findings, all on the three WIKI
REGISTRIES lines I had just edited** (the lint pairs every id on a line with every status word on it; the TEST line
alone was 4891 chars with ~65 ids). Root cause is DEC-015 + LSN-014: the hub was RESTATING per-id status that
traceability.md owns. Compacted REQ/DEC/TEST/VAL/LSN registry lines to range + next + pointer — **WIKI 18,293 →
10,058 chars, lint 71 → 0, no information lost** (all detail already lives in the canonical files). Captured as
LSN-035. Prior: VAL-016 — 2026-08-02 — incremental over the REQ-012 changeset (TEST-054/055/056 + spine). Verdict
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
