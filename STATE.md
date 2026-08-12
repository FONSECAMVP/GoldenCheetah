# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# Per-id lifecycle status lives ONLY in the traceability matrix (DEC-015 SSOT). This file names
# WHERE we are — the active gate, blockers, recent commits — not the status of every id.
# For "is REQ-x done?" read .claude/workflow-garminconnect/traceability.md.

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; project ran A0–A5 + STRIDE + per-slice CLV = FULL de facto)
BUDGETS: WIKI ~13.5k chars/cap-ok (compacted 2026-08-05 from 18.3k; grew across the REQ-021 wave) · LSN active:57 guards:42 mech:2 ·
  NOTE 2026-08-12: findings.md and STATE.md both grew substantially this wave (11 A3-R021 + 13 B-R021/B-R025 + 4 ORCH
  + 2 S- findings; 6 new lessons). Neither is a hot read (index heads only), so no cap breach — but a librarian Job-3
  compaction is worth CONSIDERING at the next phase close, not now. ·
  decisions.md 107k + findings.md 109k + lessons.md 98k = COLD entry files, drilled by id, not hot reads
  (the hot reads are their index heads). No cap breach; librarian Job-3 compaction NOT due.
COUNTS: REQ 26(+NF) · DEC 31 · DES 14(+2) · TEST 94 alloc/94 built · VAL 17 · LSN 57 · ORCH-findings 14 · S-findings 7   (registries → WIKI.md; per-id status → traceability.md)

PHASE: Phase 2.2 — Garmin Connect integration. Per-REQ/DES/TEST/VAL status → traceability.md.

CURRENT: **THE WAVE IS COMMITTED 2026-08-13 (user said proceed). Feature `6dc794caf` + lint fix `3f44c447f` + this docs
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
**STILL TO DO: the clean-worktree configure+build of the COMMITTED tree** — the gate that caught ORCH-005/006/007, and
the only gate that does not run inside the developer's working tree. Until it passes, this wave is committed but not
verified-from-clean.

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

NEXT_GATE: **THE CLEAN-WORKTREE BUILD GATE — the last step of this wave's close. Commits (a) `6dc794caf` and
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

BLOCKING: **NONE OPEN as of 2026-08-12 — `A3-R021b-F1` is FIXED and the fix is orchestrator-verified by execution.**
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
