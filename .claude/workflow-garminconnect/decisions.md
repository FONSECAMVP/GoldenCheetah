# Decision Ledger — Garmin Connect Integration

> Decision IDs in this project start at DEC-001. Independent of the prior `workflow/decisions.md` (AI Coach project). Cross-project refs use `coach:DEC-NNN`.

Compact schema per `references/formats.md` § `decisions.md`. **The recap source is the `## Decision index` immediately below** — the Tier-1 head every session reads instead of scrolling the entries. (It used to say "the `state.md ## decs` table is the recap source"; DEC-015 DELETED that table on 2026-07-12 and no replacement head was ever built, so this file opened straight into DEC-001's full entry for thirteen months of work. Built 2026-08-23, librarian Job 3.) Full entries below are drilled only when creating, editing, or cascading from a specific DEC. Detailed deliberation notes from the original v1 ledger are preserved verbatim in `decisions-history-archive.md`.

---

## Decision index

> **PRECEDENCE (this index is a duplication risk — LSN-014/LSN-035 — so the rule is stated, not assumed).**
> DEC status is single-homed in `decisions.md` = THIS file, so the index sits inside its own canonical home.
> **On conflict the entry's `Status:` field wins and the index row is the drift.** A status change is not done
> until BOTH carry it. One line per DEC, every id, no gaps (VAL-017's failure was an index stopping at DEC-019).
> REQ/DES/TEST/VAL status is NOT here — that is `traceability.md`.
>
> **Split rule (auditable, applied 2026-08-23):** **Dormant** iff superseded by a later DEC, OR a fully-executed
> one-shot (toolchain / rollout / recording-only / build-repair) with no open dependent. Everything else is
> **Active**. Dormancy is ordering, not closure — a dormant DEC still binds, it just cannot change.

### Active index

| DEC | Question → chosen option | Status | Date |
|-----|--------------------------|--------|------|
| DEC-001 | Delivery sequence for bidirectional sync → B staged ship (download → upload → schedule) | accepted | 2026-05-17 |
| DEC-002 | C++ → embedded Python integration mechanism → B worker thread + mailbox | accepted | 2026-05-17 |
| DEC-003 | Token + sidecar on-disk layout → B per-athlete config dir | accepted | 2026-05-17 |
| DEC-004 | Credentials + MFA dialog UX shape → B `AddCloudWizard` pages | accepted | 2026-05-17 |
| DEC-006 | Activity file format + staging path → FIT default | accepted (recording-only) | 2026-05-17 |
| DEC-007 | Rate-limit + retry placement → B Python-side decorator inside the worker thread | accepted | 2026-05-17 |
| DEC-012 | Auth-dispatcher seam, page ↔ SSO layer → A inject `IGarminAuthClient` | accepted | 2026-05-24 |
| DEC-013 | Worker ↔ Python adapter seam → A inject `IGarminPyAdapter` | accepted | 2026-05-24 |
| DEC-014 | Token persistence write-ownership → B adapter exports the blob, C++ owns the atomic 0600 write | accepted | 2026-07-11 |
| DEC-015 | Ledger status drift → C one canonical home per id-class + absence-check lint; local `state.md` deleted | accepted · MECHANISM | 2026-07-12 |
| DEC-016 | FIT→TCX fallback trigger for `readFile` → C kind-aware retry + content-sniff backstop | accepted | 2026-07-14 |
| DEC-017 | REQ-008 sidecar-persistence shape → A dedicated `GarminSidecarStore` | accepted | 2026-07-19 |
| DEC-018 | Where `garmin_user_id` persists → B separate account-agnostic `active-account.json` | accepted | 2026-07-19 |
| DEC-019 | Connect/disconnect persist trigger → C `CloudService::disconnect()` virtual + wizard `finished()` capture | accepted | 2026-07-20 |
| DEC-020 | A live session outliving Disconnect → C fail-closed re-check + cheap hardening, lifecycle work → REQ-017 | accepted · implemented `f001c7d20` | 2026-08-02 |
| DEC-021 | How Disconnect invalidates a LIVE session → B account-epoch latched at `open()`, compared in memory | accepted | 2026-08-03 |
| DEC-022 | How a fail-closed `readFile` reports itself → A labelled completion | **PARTIALLY IMPLEMENTED** — GarminConnect half green; SHARED half BLOCKED + un-decided (original premise FALSE), superseded by DEC-023 | 2026-08-03 |
| DEC-023 | Carry read FAILURE explicitly → option 3, an explicit `readFailed` channel, not a `message` heuristic | accepted | 2026-08-04 |
| DEC-024 | Making the modeless sync dialog's self-deletion safe → A `done(int)` gate + BlockingCall depth counter | accepted (amended post-build: `closeEvent()` alone was insufficient) | 2026-08-05 |
| DEC-026 | No nested event loop under construction → B two-phase init, ctor builds the shell, `start()` does the work | accepted | 2026-08-06 |
| DEC-027 | Unify sync-dialog lifetime → A heap + `WA_DeleteOnClose` + modeless `open()` for BOTH callers | accepted | 2026-08-06 |
| DEC-029 | Upload-dialog UAF fix shape → B two-phase `start()` + heap/`WA_DeleteOnClose`, `exec()`/modal PRESERVED | accepted | 2026-08-07 |
| DEC-030 | Collaborator-lifetime UAF fix shape → B reparent both dialogs to `context->tab`, probe-first, QPointer rider | accepted | 2026-08-10 |
| DEC-031 | Reopening DEC-025 → B frame-counted DEFERRED REAPER replaces the deliberate store leak, probe-first | accepted | 2026-08-11 |
| DEC-032 | Closing the silent stall → A in-loop `continue` in the branch that arms nothing, adopting `uploadNext`'s shape | accepted | 2026-08-13 |
| DEC-033 | The `readFile` bool contract — `false` does NOT mean "armed nothing" | accepted — Option A, defaulted out-param; BUILT + execution-verified, COMMITTED `2e26106c1` 2026-09-04 (routed out of DEC-032; closes B-R027-01/02/03/09) | decided 2026-09-03 |
| DEC-034 | Who owns a `QTreeWidgetItem` across a nested event loop → C a `listGeneration` counter bumped by `refreshClicked` | accepted · NOT reopened | 2026-08-14 |
| DEC-034/036 AMENDMENT | Completion slots never got the drivers' guard (A3-R028c-F2) → snapshot `batchGeneration` there | accepted, folded into DEC-036's Option C build | 2026-08-21 |
| DEC-035 | Where the abort guard lives, `processEvents()` being no guaranteed drain → B co-locate it with the irreversible call | accepted | 2026-08-15 |
| DEC-036 | Correlating a completion with the transfer that asked for it → **C explicit per-operation write identity**, dispatch→completion | accepted + BUILT on formal reopen · `e48f7d123`, gate `98df50535` · closes REQ-028 (c) | 2026-08-18; re-decided 2026-08-22 |
| DEC-038 | The third list mutator, SORTING → A sorting is OFF for the batch's duration, as a DEC-034 AMENDMENT | accepted · BUILT + committed `514d8e88f` | 2026-08-22 |
| DEC-039 | libusb dependency wiring (ORCH-036) → B complete `find_path`/`find_library` for BOTH APIs | accepted · built on isolated branch `build/orch036-libusb-wiring` (`ae7655985`), **deliberately UNMERGED** | 2026-08-23 |
| DEC-040 | Bounding the 22 unbounded provider waits (S-R028f-F1/F3, C1–C6) → **W3: shared bounded-request outcome contract, then generic cooperative cancellation**, one release unit, with S-1 base-owned injectable QNAM | accepted · **STAGE 1 BUILT, VERIFICATION-GATE-PASSED and COMMITTED as `37710370e` (2026-08-30) — NOT PUSHED, NOT MERGED; DEC-040 NOT COMPLETE — Stage 2 cooperative cancellation NOT STARTED.** *(Cursor corrected 2026-08-30: this row read "on working-tree bytes, UNCOMMITTED", which was true at the Stage-1 gate and became stale when the slice landed.)* 10 providers / 22 bounded-request sites; 13 providers on base manager ownership (RideWithGPS, Selfloops, SportsPlusHealth manager-only, no watchdog rows); S-1 amended to LAZY null-or-valid; 5 legitimate `delete nam` remain in OpenData.cpp (zero is retired); no CancelToken/poll/reachable Cancelled in Stage 1. See **## DEC-040 STAGE-1 AMENDMENT & STATUS — 2026-08-30** | 2026-08-26 · amended 2026-08-30 |
| DEC-041 | File-IO layer Context UAF at `RideFile.cpp:999` (B-R025-01/A3-R021b-F2) → **Option B, hoist-and-capture: read the needed Context values before the suspending reader call, don't guard the read after it** — AMENDED 2026-09-08: census corrected (4 context touches, tail to :1133; a second post-suspension deref at :1055) and mechanism extended with a factory-entry-QPointer liveness bail at :1055 (user decision) | accepted · amendment accepted · **BUILT + mutation-verified 2026-09-08 (T-173/174/175, `testGarminConnectFileIOLifetime` 5/5 both backends, zero ASan reports, orchestrator-independently-rerun) — closes B-R025-01/A3-R021b-F2** | 2026-09-03 · amended 2026-09-08 |
| DEC-042 | `saveRide`'s post-`autoProcess` member dereferences vs. parent-teardown UAF (B-R028-17) → **Option A, in-function `QPointer` self-bail placed after the SECOND `autoProcess` call, at the proven hazard site, not the call site** | accepted · **BUILT + execution-verified 2026-09-05 (TEST-158, `99/99` both backends, `ctest -L garmin-fast` 27/27) — closes B-R028-17** | 2026-09-05 |
| DEC-043 | `CloudServiceAutoDownload` cross-thread lifetime UAF, `readComplete`/`readFailed` vs. athlete-tab teardown (A3-R028e-F1) → **Option C, guards in the completion slots PLUS cooperative cancel-and-`wait()` at teardown (extends DEC-040's `CancelToken`)** | accepted · **BUILT + execution-verified 2026-09-05 (TEST-159/TEST-160; watchdog 170/170 + syncdialog 99/99 both backends, `ctest -L garmin-fast` 27/27, app target linked) — closes A3-R028e-F1** | 2026-09-05 |
| DEC-044 | `RideFile::appendOrUpdatePoint` reads a deleted `RideFilePoint*` (`RideFile.cpp:1658` delete vs. `:1709-1711` unconditional read) (B-R029-01) → **Option A, alias the surviving point: track whichever pointer is still valid after the branch and pass that to `updateMin`/`updateMax`/`updateAvg`, not the possibly-deleted `point`** | accepted · **BUILT + mutation-verified 2026-09-08 (T-173, `testGarminConnectFileIOLifetime` 5/5 both backends, zero ASan reports, orchestrator-independently-rerun) — closes B-R029-01** | 2026-09-08 |
| DEC-045 | REQ-014 translation locus + keying (recording-only, build predates this entry) → **confirms DES-008's page-layer locus (worker/adapter stay raw); keys `GarminErrors::translate()` on `GarminAuthFailure::Kind`, not DES-008's sample (raw exception class name), which does not survive the Python→adapter→worker pipeline** | accepted · recording-only · **BUILT + committed `ac1fa40ba` (2026-09-08); T-177 (`testGarminConnectErrors`, 4/4)** | 2026-09-10 (build 2026-09-08) |
| DEC-046 | REQ-015 CAPTCHA detection → defer, no structured signal survives the real `garminconnect` dependency | accepted (user decision, option 2 of 3) · no code written | 2026-09-10 |
| DEC-047 | REQ-010/DES-009 pagination model → per-activity pacing/checkpointing, not per-page, against the real `garminconnect` dependency | accepted · recording-only · **BUILT** (formalizes a resolution already implemented + reviewer-confirmed during REQ-010's build) | 2026-09-10 |
| DEC-048 | B-R010-04 UI wiring scope split → fix the new backfill-local exposure now, defer `GarminConnect`'s own internal context-handling gap | accepted · recording-only · **BUILT + committed `2b8cedae3`** | 2026-09-10/11 |
| DEC-049 | REQ-NF-Pkg-001 scope expansion → port Garmin sources into the qmake release build before any installer/packaging work | accepted (user decision, live during session) · **BUILT + committed `e609215f0`** | 2026-09-11 |
| DEC-050 | REQ-013 scope → narrow the first slice to DOB/weight/height, defer HR-max/FTP | accepted (user decision, option 1 of 3) · **BUILT + committed `e17262a0b`** | 2026-09-12 |
| DEC-051 | REQ-NF-Obs-001's "ErrorBus" clause → satisfied by the project's real, already-decided error channels; build only the missing structured `qDebug` trace logging | accepted (Inspector, Three-Options Doctrine — ordinary architecture reconciliation, not a human-in-the-loop gate) | 2026-09-12 |
| DEC-052 | B-STAGE9-01 fix → shared process-level CPython bootstrap owned by neither `PythonEmbed` nor `PyEmbeddedAdapter` alone, invoked once on the main thread before either feature's workers can run | accepted (Inspector, Three-Options Doctrine — ordinary architecture decision, not a human-in-the-loop gate; reviewer-researched options) | 2026-09-13 |
| DEC-053 | B-STAGE9-15 remedy → developer-trace literals exempt from the i18n guard as a CATEGORY; the three flagged `qDebug` call sites are correct as written, the guard's heuristic is not | accepted (Inspector, Three-Options Doctrine — ordinary tooling-correctness decision, not a human-in-the-loop gate; standing-rule exception independently verified by a fresh unbriefed agent before recording) | 2026-09-15 |
| DEC-054 | B-STAGE9-16 remedy → the project's default verification scope becomes FAIL-SAFE: the routine ctest gate is `ctest -LE gate-exclude` (default-include, explicit opt-out) instead of `ctest -L garmin-fast` (opt-in), the pre-commit `files:` regexes are widened to every new Garmin-owned path rather than three enumerated ones, and two static coverage guards make both boundaries self-policing | accepted (Inspector, Three-Options Doctrine — ordinary tooling/process decision, not a human-in-the-loop gate; three scored options researched by a fresh unbriefed agent, decisive timing facts re-verified by the Inspector before recording) | 2026-09-15 |
| DEC-055 | B-STAGE9-26 remedy → the listing boundary's typed `response_invalid` contract covers EVERY key the GC-stable summary shape requires, `activityId` included; a missing key is a classified library-contract breach, not a bare `KeyError` | accepted (Inspector, Three-Options Doctrine — ordinary implementation-contract decision within REQ-NF-Obs-001/REQ-002's accepted scope, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-056 | B-STAGE9-29 remedy → Garmin listing entries are named from the activity's own LOCAL start time in the project-wide `yyyy_MM_dd_HH_mm_ss` form, marshalled from the library's `startTimeLocal`, with `startTimeGMT`-converted-to-local as the documented fallback; the shared dialog's filename gate is left untouched | accepted (Inspector, Three-Options Doctrine — ordinary implementation decision inside REQ-002/REQ-012's accepted scope, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-057 | B-STAGE9-28 remedy → an EMPTY managed root must be DECLARED pre-armed (naming the DEC that armed its regexes) or the lint-ownership guard goes RED; the finding's own option A is VOID, `git ls-files` already reads the index and already sees staged-but-uncommitted files | accepted (Inspector, Three-Options Doctrine — ordinary tooling decision on a project-owned guard, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-058 | B-STAGE9-38 remedy → the Garmin adapter ships as an installable distribution under a new import namespace, carried by the existing `pip install -r requirements.txt` step on all three platforms; `GARMIN_PY_MODULE_DIR`'s build-machine absolute path stops being consulted in package mode, and the Linux `--version` smoke becomes a real import check | accepted (Inspector, Three-Options Doctrine — ordinary packaging decision, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-059 | B-STAGE9-36 round 3 → a DEC arms a managed root by an explicit machine-readable `- Arms:` bullet naming the exact `patterns` globs, matched by whole-entry set membership inside the resolved entry body; amends DEC-057's proof mechanism, whose rule is unchanged, after two rounds of grepping prose were each defeated by a narrower instance of the same class | accepted (Inspector, Three-Options Doctrine — ordinary tooling decision, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-060 | B-STAGE9-36 round 8 → an Arms declaration names its own DEC (`- Arms DEC-054:`) and is honoured only inside that entry; an id that does not match the enclosing entry is a hard parse error, never a silent skip, so Markdown block context stops being consulted for the bullet at all. Amends DEC-059's grammar after seven rounds in which a conformant-CommonMark scanner was being hand-rolled one container type at a time | accepted (Inspector, Three-Options Doctrine — ordinary tooling decision on a project-owned guard, not a human-in-the-loop gate) | 2026-09-19 |
| DEC-061 | B-STAGE9-40 remedy → `src/Core/main.cpp:552` consumes `ensureInitialized()`'s failure `Result` nonfatally (retain the state, log it, continue startup) instead of discarding it, making `isInitialized()` assertable by an app-process smoke; the Garmin-local "Python unavailable" surface is a separate non-blocking follow-on, not this fix. Rests on the correction that the `main.cpp` hold was RELEASED by the user 2026-09-16 (`STATE.md:750`) and was carried forward as in force in error | accepted (Inspector, Three-Options Doctrine — ordinary implementation decision inside the finding's own mechanism, not a human-in-the-loop gate; the standing-rule correction independently verified by a fresh unbriefed agent before recording) | 2026-09-20 |
| DEC-062 | B-STAGE9-39 remedy → a Qt-side deployment locator returns `{home, programName}`, passed into `PyProcessBootstrap::Config` and set as explicit `PyConfig.home`/`program_name` BEFORE `PyConfig_Read`; `PythonEmbed` consumes the same result instead of mutating `PYTHONHOME`, and the deprecated `Py_SetProgramName` is retired. The locator prefers a deployed payload over a visible host stdlib | accepted (Inspector, Three-Options Doctrine — ordinary mechanism choice inside the finding's own remedy; MEASURED on five legs against the live reproducer before recording, including a conflict leg proving explicit fields beat a hostile `PYTHONHOME`) | 2026-09-20 |
| DEC-063 | B-STAGE9-41 remedy → CMake defines `GC_WANT_PYTHON` target-locally (matching `src/CMakeLists.txt:1508`'s existing `GC_WANT_GARMINCONNECT` convention) and the `GC_HAVE_PYTHON` spelling is DELETED, not aliased; it has exactly one occurrence in the live tree — its own definition at `:1085` — and no `#ifdef` consumer anywhere, while the 55 sites that do guard on `GC_WANT_PYTHON` never see it defined under CMake | accepted (Inspector, Three-Options Doctrine — ordinary build-system drift repair, not a human-in-the-loop gate; the investigator's census re-derived from source by the Inspector, which surfaced the in-file precedent the option report missed) | 2026-09-20 |
| DEC-064 | B-STAGE9-36 remedy → the Arms authorization stops being inferred from arbitrary Markdown bullets and moves into a structured record with a fail-closed grammar, superseding DEC-060's loose-recognizer doctrine for that field only | ACCEPTED — Option C, fixed-slot sentinel replacing the `:2646` declaration; decided on the Inspector's own measurement after the second opinion dissented | 2026-09-20 |
| DEC-065 | Stage 9's installer evidence is earned on the AppVeyor pipeline, not on a local build — the shared remote gets the branch and CI produces the Linux/macOS/Windows artifacts | accepted (USER decision 2026-09-20, the one human-in-the-loop gate of this session; both routes needed the user's own authority) | 2026-09-20 |
| DEC-066 | B-STAGE9-45 remedy → adapter-module provenance stops being read off Python-visible attributes and becomes a C++-owned per-interpreter identity ledger, validated AFTER import and cleared before finalization | accepted (Inspector, Three-Options Doctrine — ordinary architecture decision, not a human-in-the-loop gate; option A amended by the independent second opinion, which found it insufficient as first drafted) | 2026-09-20 |
| DEC-067 | B-STAGE9-59's smoke-assert class is terminated → one final strengthening (callable `Garmin`, real exception classes) and then the residual is a recorded, accepted false negative; there is no round 4 | accepted (Inspector, repair-round bound at 3 same-class rounds — ordinary engineering judgement, not a human-in-the-loop gate) | 2026-09-20 |
| DEC-068 | B-STAGE9-66's premise is refuted by three independent sources → the returned-object compare guards nothing under CPython's `PyImport_Import` semantics; it stays as a tripwire with a truthful comment, and the invariant it rests on becomes a test instead of an unstated assumption | accepted (Inspector, Three-Options Doctrine — ordinary engineering judgement on dead code, not a human-in-the-loop gate) | 2026-09-24 |
| DEC-069 | B-STAGE9-71 remedy — how many shipped platforms the Stage 9 payload assertion must cover before the installer findings may close → all three; Windows and macOS each gain an assertion against the PRODUCED artifact, not its staging tree | accepted (USER decision 2026-09-24 — a CI modification, so the user's call, not the Inspector's) | 2026-09-24 |
| DEC-070 | B-STAGE9-78 remedy — how a downloaded activity reaches `RideImportWizard` when Garmin returns a ZIP → stage under the payload's TRUE extension, sniffed from the leading magic bytes, and let the wizard's existing `expandFiles()` archive route unpack it; the controller reports the real staged path instead of it being recomputed from the activity id | accepted (Inspector, Three-Options Doctrine — ordinary technical call on Garmin-side code, not a human-in-the-loop gate) | 2026-09-26 |
| DEC-071 | B-STAGE9-79 remedy — how a cancelled import stops orphaning an activity → split the one overloaded record into a `pending` download/resume manifest plus a completion-only imported map, and take import-completion from `RideCache::getRide(startTimeGMT.toUTC())`, the seam the wizard itself uses; exact match justified by measurement, no tolerance window | accepted (Inspector, Three-Options Doctrine on the investigator's scored report plus the B-STAGE9-79-seam measurements) | 2026-09-26 |
| DEC-072 | B-STAGE9-83 remedy — how a gzip payload reaches the importer when `Archive::dir`'s GZIP arm is an empty block → inflate it in the controller at stage time with zlib's gzip window, re-sniff the inflated bytes and stage under THEIR true extension; amends DEC-070's false "gzip comes for free" premise, keeps `ZipReader` as the only unzip | accepted (Inspector, Three-Options Doctrine — ordinary technical call on Garmin-side code) | 2026-09-26 |
| DEC-073 | B-STAGE9-86 remedy after 3/3 same-class repair rounds — how the staged payload stops being a shape guess → invert the predicate: accept ONE complete single-member gzip (`Z_STREAM_END` + `avail_in == 0`) whose output is FIT-or-ZIP, refuse everything else as `PauseReason::UndecodablePayload` rather than staging it hopefully; discharges the repair cap as remedy (a), architectural | accepted (Inspector, Three-Options Doctrine; deterministic surface, so the pin option was rejected on evidence) | 2026-09-26 |
| DEC-074 | findings.md's row-size budget — whether the 200B `ROW` cap that has stood in BREACH since 2026-09-06 is the defect or the rows are → the CAP is the defect: retire the generic 200B figure for findings.md specifically and replace it with a register-specific ~600B soft target / ~1,200B hard cap, then fix the real defect (round-by-round narrative leaking into the hot row instead of being condensed at disposition) by routing it to the existing cold archive | accepted (Inspector, Three-Options Doctrine on the librarian's measured Job-3 draft — a ledger/tooling convention, not a human-in-the-loop gate) | 2026-09-26 |
| DEC-075 | DEC-071 slice 1's write contract — whether the callers or the store hold the invariant that `pending` survives a cursor write → the STORE holds it: `pending` becomes single-writer (`saveBackfillState` persists cursor fields and preserves the on-disk map, ignoring its argument's), and "nothing to lose" splits from "cannot model" — `NotFound`/`Torn` still self-heal by overwrite while `SidecarPermissionsRejected` and a new `PendingManifestMalformed` make all three writers refuse | accepted (Inspector, Three-Options Doctrine on the reviewer's B-STAGE9-79-s1 delta-check — ordinary technical call on Garmin-side code) | 2026-09-27 |
| DEC-076 | B-STAGE9-108 remedy after the isolated concurrency trace — whether the store's new load-modify-write writers need a lock or a pin → a LOCK: the investigator constructed a live same-uid interleaving between the GUI backfill clone and auto-download's `QThread`, so `GarminSidecarStore` serializes on the resolved sidecar path across each transaction's load-to-`writeOver`; the pending-map loss is latent only until slice 2 gives its mutators callers, so it lands first | accepted (Inspector, Three-Options Doctrine on `s979_record_split_investigator`'s B-STAGE9-108-concurrency evidence — ordinary technical call on Garmin-side code) | 2026-09-27 |
| DEC-077 | B-STAGE9-116's in-`process()` use-after-free — whether this branch repairs it or freezes it → FROZEN as upstream: an unbriefed investigator found `RideImportWizard`'s raw `Context*` (`RideImportWizard.h:92`, deref at `.cpp:1050,1118`) shared by four non-Garmin callers, and a Garmin-free reachable path through `MainWindow`'s manual import, so the repair is a rework of shared import code this branch never touched; same call already made for `ArchiveFile.cpp` and `CloudService.cpp:565`. B-STAGE9-114's dialog-sweep fix stands and stays this branch's | accepted (Inspector, on `s979_record_split_investigator`'s B-STAGE9-116-scope verdict — ordinary technical scope call, no external element) | 2026-09-27 |
| DEC-078 | B-STAGE9-112's remedy at the repair-round bound — whether to pick a better cursor rewind or remove the overload → REMOVE IT: `GarminBackfillController.cpp:283,336` uses one variable as both an exclusive last-success marker and an inclusive range bound, which is why two rounds of better rewind values both failed on the reviewer's `startTimeGMT == lastSuccess == rangeStart` case; splitting the roles also closes B-STAGE9-121, the earliest activity of every range being silently skipped | accepted (Inspector, at the repair-round bound after two consecutive SAME-class verdicts — ordinary technical call on Garmin-side code) | 2026-09-27 |
| DEC-079 | B-STAGE9-79 slice 3, the legacy migration — where it runs and how a pre-slice-2 `imported` row is classified → DIALOG PRE-START SELF-CLASSIFICATION: an absent `schema_version` loads as version 0 and is the ONLY thing that identifies a legacy sidecar pair, because an individual `imported` row is byte-for-byte indistinguishable from a current completion row; the dialog is the one seam that holds both the uid/config-dir and a populated `RideCache`, so it exact-matches each legacy row and moves the unmatched ones into `pending` before `GarminBackfillController::start()` | accepted (Inspector, on `s979_record_split_investigator`'s three scored designs — ordinary technical placement call on Garmin-side code) | 2026-09-27 |
| DEC-080 | B-STAGE9-111 cannot be built without touching `CloudService` code DEC-077 froze — whether to abandon DEC-071's both-routes requirement or narrow the freeze → NARROW THE FREEZE: DEC-077 stays absolute for REPAIRS and REWORKS of shared import logic (the `RideImportWizard` use-after-free, `ArchiveFile.cpp`'s GZIP arm, `CloudService.cpp:565`'s `gUncompress`) and is amended to permit an ADDITIVE, default-no-op virtual extension point, because every one of the investigator's three designs must add a call after ride registration and DEC-071's both-routes requirement is the reason this branch exists | accepted (Inspector, on `s979_record_split_investigator`'s explicit amendment-required verdict, unit B-STAGE9-111-design — ordinary scope call between two of this project's own accepted decisions, no external element) | 2026-09-27 |
| DEC-081 | Garmin backfill timestamps are compared as TEXT at five sites while the adapter forwards whatever ISO-8601 spelling Garmin sent (B-STAGE9-128) — canonicalise, or make C++ instant-aware → CANONICALISE AT THE ADAPTER BOUNDARY: the adapter already owns the authoritative parse, so normalising each accepted summary to one `yyyy-MM-dd HH:mm:ss` UTC spelling stops raw variants ever reaching C++ persistence or comparison, instead of duplicating instant policy across store, controller, dialog and the test fake | accepted (Inspector, on `s979_record_split_investigator`'s three scored options, unit B-STAGE9-128-design — ordinary technical call on Garmin-side code) | 2026-09-27 |

### Dormant index

*Superseded, or a fully-executed one-shot with no open dependent. Still binding; still one line per DEC.*

| DEC | Question → chosen option | Status | Date |
|-----|--------------------------|--------|------|
| DEC-005 | Phase-1 CloudService capabilities (Query\|Download) | accepted (recording-only; any other value contradicts REQ-011) | 2026-05-17 |
| DEC-008 | Testing toolchain → A extend the incumbent (QTest+CTest C++ / pytest+coverage.py Python) | accepted · one-shot, executed | 2026-05-17 |
| DEC-009 | Style/quality toolchain → A existing clang-format/clang-tidy + `ruff` + `mypy --strict` on new Python | accepted · one-shot, executed | 2026-05-17 |
| DEC-010 | Pre-commit automation → A the `pre-commit` framework, scoped to new Garmin paths | accepted · one-shot, executed | 2026-05-17 |
| DEC-011 | Phase-1 rollout strategy (CMake flag `GC_WANT_GARMINCONNECT`, default OFF) | accepted (recording-only) | 2026-05-17 |
| DEC-025 | Surviving PARENT TEARDOWN → A destructor declines to delete the store while busy (a deliberate leak) | **SUPERSEDED by DEC-031** — the DEC-030 reparent falsified its one load-bearing premise | 2026-08-05 |
| DEC-028 | Clean-checkout build repair → B commit the missing CMake wiring rather than guard around it (ORCH-001) | accepted · one-shot, executed; cited since as the precedent (DEC-039) | 2026-08-07 |
| DEC-037 | Separating an abandoned write from the live one → A a retired-ticket set | **SUPERSEDED 2026-08-22 by DEC-036 Option C** — not production behaviour. Its DISPROOF survives and is load-bearing | 2026-08-19 |

**Index measurements 2026-08-23 (librarian Job 3).** 39 ids, 39 rows, 0 gaps · Active 31 + 1 amendment row,
Dormant 8. The skill's split trigger is ~20 active lines; 31 exceeds it and the surplus was NOT forced into
Dormant — the remainder is the Garmin dialog-lifetime family and every one still governs live
`src/Cloud/CloudService.cpp` behaviour. A false dormant is worse than a long active list.

---

## DEC-001 — Solution shape: delivery sequence for bidirectional sync
- Status: accepted (B — staged ship)
- Reversibility: medium (filenames + Python-deps stick once users configure)
- Decided / last-reviewed: 2026-05-17
- Serves: user verbatim ask; constrained by A-03 (embedded Python), A-04 (per-athlete), A-05 (accept ToS risk); triggered by A0.3-001 (risk-adjusted MVP = download-only)
- Dependents: DEC-002, DEC-003, DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-009, DEC-010, DEC-011; all Phase-1 REQs; future Phase 2/3 workout-upload + schedule-push DEC families

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A monolithic single ship | 2 — ToS-risky writes ship with safer reads; can't roll back partial | 3 — surface locked in v1 | 2 — single big PR; long test cycles | 2 — violates A0.3-001 |
| B staged ship (download → upload → schedule) | 5 — each phase independently rollback-able; Phase 1 lowest ToS risk | 5 — each phase a separate DEC family; Phase 4 additive | 4 — three smaller PRs, reviewable in isolation | 5 — matches `GC_WANT_*` and Strava/Dropbox read-then-write precedent |
| C thin language adapter only | 3 — no CloudService isolation; callers see GC↔Python coupling | 2 — no UX surface; not discoverable | 3 — couples every caller to Python threading model | 2 — bypasses the CloudService pattern |

### Cascade impact (per option)
- A: Phase 1 PRD covers download+upload+schedule+auth at once; touches AI Coach (`coach:DEC-013`) day one; A4 stalls whole release if write paths fail.
- B (chosen): **Phase 1 = read-only `Cloud/GarminConnect.*`** (REQ-001…N); **Phase 2 = workout upload** (extends `coach:DEC-013`); **Phase 3 = schedule push** (`Train/Season*`); Phase 4 health metrics. Forces DEC-002 to ship a clean thread-isolation primitive reusable by Phases 2/3.
- C: no `Cloud/` file (would be `Python/GarminPython.{h,cpp}`); AI Coach tool gains direct Python calls; no Add-Cloud-Service wizard entry; feature invisible until each consumer surfaces it.

### Chosen
B — strategic scope is bidirectional; risk-adjusted MVP (A0.3-001) is read-only. Staging satisfies both by treating bidirectional as the goal and download-only as the v1 ship, with each capability as a discrete, gateable increment. Aligns with `coach:DEC-011`'s feature-flag precedent.

### Alignment probe
grep -E '^(if|elseif)\(GC_WANT_GARMINCONNECT\)' src/CMakeLists.txt | wc -l    # expect ≥1 block (sources gated by flag — confirms staging primitive in place)

---

## DEC-002 — Python integration mechanism (C++ → embedded Python)
- Status: accepted (B — worker thread + mailbox)
- Reversibility: medium (public API stable; worker-thread pattern sticky once tests rely on thread-id assertions)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-002, REQ-003, REQ-007, REQ-010, REQ-NF-Threads-001; constrained by existing `PythonEmbed` in `src/Python/`
- Dependents: DEC-007 (rate limiter at queue head); DES-001, DES-009, DES-010; REQ-NF-Threads-001/Cancel-001/Perf-002/Perf-003

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A direct call from UI thread | 2 — 10s-class latencies freeze UI | 1 — single in-flight | 4 — simplest | 1 — violates NF-Threads-001 |
| B worker thread + mailbox | 5 — off-UI; cancellation is a queue op | 5 — queue depth tunable; rate limiter at queue head | 3 — needs queue + signals + lifecycle | 5 — Qt signal-slot canonical |
| C thread-per-request | 3 — thread spawn OK; GIL contention high | 3 — GIL serialises anyway | 2 — per-call bookkeeping fragile | 3 — works but overkill |

### Cascade impact
- A: disqualified (would force removing NF-Threads-001).
- B (chosen): forces `GarminWorker : QObject` in `QThread`, Qt-signal `request()`/`finished()` API. Phase 2 tests assert thread-id at every Python call site. DEC-007 sits at queue head. **Sets the template Phase 2 (uploads) and Phase 3 (schedule) reuse.**
- C: per-call thread bookkeeping; no win over B.

### Chosen
B — anchors NF-Threads-001 directly and gives Phase 2/3 a reusable transport.

### Alignment probe
grep -rn 'QThread\|QObject' src/Cloud/Garmin* | wc -l   # expect ≥1 (worker subclass + QThread harness)

---

## DEC-003 — Token + sidecar on-disk layout
- Status: accepted (B — per-athlete config dir)
- Reversibility: low-medium (paths sticky after users have token files)
- Decided / last-reviewed: 2026-05-17 (path-convention refinement 2026-05-17 post-A2)
- Serves: REQ-004, REQ-006, REQ-008, REQ-010, REQ-NF-Reliab-002, REQ-NF-Sec-002/004
- Dependents: REQ-004, REQ-006, REQ-008, REQ-010, REQ-012; DEC-002 (worker uses the writer); DES-002, DES-006, DES-009, DES-010

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A library default (`~/.garminconnect/tokens.json`) | 3 — global file; multi-athlete collision | 1 — broken >1 athlete | 4 — least code | 1 — violates A-04 |
| B per-athlete dir + dedicated subfolder | 5 — fully isolated; tmp+rename trivial | 5 — N athletes scale | 4 — explicit paths; easy Disconnect cleanup | 5 — matches Strava precedent |
| C inside GC's QSettings INI | 2 — INI not designed for blobs + tmp+rename | 3 — cramps the INI | 2 — values not human-greppable | 2 — mixes secrets with app settings |

### Cascade impact
- B (chosen): defines exact paths used by every subsequent REQ. Token: `<athlete-dir>/garminconnect/tokens.json`. Sidecar dedup: `imported-<garmin_user_id>.json`. Resumable backfill state: `backfill-state-<garmin_user_id>.json`. All written via tmp+rename helper from DEC-002's worker.

### Chosen
B — aligns with Strava precedent; isolates per-athlete state per A-04. Post-A2 refinement: sidecar files partitioned per Garmin account ID (A2-006 Option C); `tokens.json` singular. Sidecar **format** in Phase 1 is JSON; Phase-1.5 cascade trigger to sqlite if beta observability shows sidecar-read-time > 500ms or entry-count > 10,000.

### Alignment probe
grep -nE 'tokens\.json|imported-.*\.json|backfill-state-.*\.json' src/Cloud/Garmin* src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 (paths reference per-athlete subfolder, never library default)

---

## DEC-004 — Credentials + MFA dialog UX shape
- Status: accepted (B — `AddCloudWizard` pages)
- Reversibility: high (page replacement is local)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-002, REQ-003, REQ-009, REQ-014, REQ-015
- Dependents: REQ-002, REQ-003, REQ-009, REQ-014, REQ-015; DES-003, DES-008, DES-011; AddCloudWizard infrastructure

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A blocking modal `QDialog`s | 4 — simplest cancel | 4 — new step = new dialog | 4 — self-contained | 3 — modal stack feels clunky |
| B `AddCloudWizard` pages | 5 — wizard handles cancel/back/forward | 5 — page = subclass; framework handles flow | 5 — matches GC convention | 5 — Strava/Dropbox precedent |
| C non-modal floating widget + state machine | 2 — modeless flows bug-friendly | 3 — state machine grows fast | 2 — non-modal hard to debug | 2 — anti-pattern for credentials |

### Cascade impact
- B (chosen): two new `QWizardPage` subclasses (`GarminCredentialsPage` + `GarminMfaPage`); CAPTCHA page surfaced only on detection. Reuses `AddCloudWizard` Back/Next/Cancel/Help machinery.

### Chosen
B — matches Strava/Dropbox precedent; reuses Qt wizard infrastructure.

### Alignment probe
grep -rn 'QWizardPage' src/Cloud/Garmin* 2>/dev/null | wc -l   # expect ≥2 once Phase 2.2 wizard slice ships (credentials + MFA pages)

---

## DEC-005 — Phase-1 CloudService capabilities
- Status: accepted (recording-only; alternatives don't apply — any other value contradicts REQ-011)
- Reversibility: high (Phase 2 flips in `Upload`)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-011
- Dependents: REQ-011; AddCloudWizard UI logic; DES-004 (`capabilities()` return)

### Chosen
`Query | Download` only. No `Upload`, `Sync`, `OAuth` (library SSO is used, not GC's generic OAuthDialog), no `Delete`.

### Alignment probe
grep -nE 'capabilities.*Query.*Download' src/Cloud/GarminConnect.cpp 2>/dev/null | wc -l   # expect 1 (exact bitmask) — TEST-001 mutants #2/#3 also gate this

---

## DEC-006 — Activity file format + staging path
- Status: accepted (recording-only)
- Reversibility: medium (filename pattern sticky if anyone scripts against it)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-007, REQ-008
- Dependents: REQ-007, REQ-008; DES-004, DES-010

### Chosen
FIT preferred (`download_activity(activity_id, dl_fmt=ORIGINAL)` — gives FIT for nearly all activities). TCX fallback only if Garmin returns a non-FIT original. Staging path: GC's existing import staging dir, filename `garmin-<activity_id>.<ext>` to avoid collisions.

### Alignment probe
grep -nE "dl_fmt|ORIGINAL|garmin-.*\.fit" src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 when REQ-007 lands

---

## DEC-007 — Rate-limit + retry placement
- Status: accepted (B — Python-side decorator inside worker thread)
- Reversibility: high (limiter file is local)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-NF-Perf-002, REQ-NF-Reliab-001
- Dependents: REQ-NF-Perf-002, REQ-NF-Reliab-001; sits inside DEC-002's worker; DES-005, DES-012

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A C++ limiter wrapping Python entry-points | 3 — every site must remember to wrap | 3 — N entry points | 3 — split brain | 3 — limiter near consumer |
| B Python-side decorator | 5 — single chokepoint at Python boundary | 5 — one decorator, all calls | 4 — testable in isolation | 5 — canonical |
| C trust `python-garminconnect` | 1 — library has no rich rate limiter | N/A | 2 — opaque external dep | 1 — assumes external behaviour |

### Cascade impact
- B (chosen): a small `gc_rate.py` module installed alongside the library, exposing decorator + retry helper. Worker (DEC-002) imports and applies to each library method. Tests assert call timing via fake clock.

### Chosen
B.

### Alignment probe
grep -rn '@.*rate_limit\|@retry\|token_bucket' src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 when DES-005 lands

---

## DEC-008 — Testing toolchain
- Status: accepted (A — extend incumbent; QTest+CTest C++ / pytest+coverage.py Python)
- Reversibility: high for Python; medium for C++ (QTest macros sprinkled)
- Decided / last-reviewed: 2026-05-17
- Serves: every Phase 2.2 TEST-NNN
- Dependents: every Phase 2.2 TEST-NNN; DES-001 (signal assertions), DES-005 (timing tests), DES-006 (atomic-write race tests), DES-012 (adapter contract tests); DEC-009; DEC-010

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A QTest+CTest C++ / pytest+coverage.py Python | 5 — both battle-tested; QSignalSpy native; pytest+freezegun canonical | 5 — same pattern coach tests use; Python scales by file | 5 — zero novelty for C++ devs | 5 — matches `unittests/Core/coach/` precedent + Python default |
| B GoogleTest/Catch2 C++ + pytest Python | 3 — GoogleTest lacks Qt signal/event-loop support; needs adapter | 3 — duplicate CMake helpers | 2 — first non-QTest framework here | 3 — industry-standard but not Qt-industry |
| C QTest only (subprocess pytest shims) | 2 — flaky exit-code plumbing | 2 — every Python test needs C++ harness | 2 — debug = parse stdout out of C++ log | 1 — anti-pattern |

### Cascade impact
- A (chosen): C++ REQs go in `unittests/Core/garminconnect/test*.cpp` using `QTest` macros + `QSignalSpy`; Python REQs in `src/Python/garminconnect/tests/test_*.py` using pytest + freezegun + pytest-cov. Coverage: gcov/lcov (C++), coverage.py (Python). CI fails on coverage-delta-drop for **changed files only**. Tests gated behind `GC_WANT_GARMINCONNECT`.

### Chosen
A — matches Qt convention + Python community default; zero new framework.

### Alignment probe
test -f unittests/Core/garminconnect/CMakeLists.txt && test -f src/Python/garminconnect/pyproject.toml && echo OK   # expect "OK"

---

## DEC-009 — Style/quality toolchain
- Status: accepted (A — existing `.clang-format`/`.clang-tidy` + `ruff` + `mypy --strict` scoped to new Python)
- Reversibility: high (single file config)
- Decided / last-reviewed: 2026-05-17
- Serves: every Phase 2 COMMIT
- Dependents: every Phase 2 COMMIT; DES-005 + DES-012 (type-checked); DEC-010 (which tools the pre-commit hook invokes); future i18n string-extraction

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A clang-format/-tidy + ruff + mypy --strict (scoped) | 5 — ruff covers pyflakes/pycodestyle/isort/bugbear; mypy catches DES-012 drift | 5 — ruff ~10–100× faster; matters for pre-commit | 5 — one `pyproject.toml` configures lint+format | 5 — 2025 community default |
| B clang-tools + black+flake8+isort + mypy | 4 — same coverage, three tools to align | 3 — three subprocess invocations | 2 — version-skew across three packages | 3 — established but legacy |
| C clang-tools + black only | 2 — uncaught name typos in adapter | 5 — fastest because does the least | 4 — least config; trades into bug cost | 1 — below industry hygiene bar |

### Cascade impact
- A (chosen): adds `pyproject.toml` scoped to `src/Python/garminconnect/`. Configures `ruff` (default + B + E/F/I) and `mypy --strict` on the same path. C++: zero new config — new files under `src/Cloud/Garmin*` + `unittests/Core/garminconnect/` picked up by existing `.clang-format` + `.clang-tidy`. CI runs the tools on changed files only. **Scope is narrow** — does not retrofit the rest of `src/Python/` (legacy chart scripts).

### Chosen
A.

### Alignment probe
grep -E 'strict.*=.*true' src/Python/garminconnect/pyproject.toml | wc -l   # expect 1 (mypy --strict configured)

---

## DEC-010 — Pre-commit automation
- Status: accepted (A — `pre-commit` framework, scoped)
- Reversibility: high (`.pre-commit-config.yaml` is one file)
- Decided / last-reviewed: 2026-05-17
- Serves: Phase 3.1 "fast local feedback"
- Dependents: every Phase 2 COMMIT; DES-007 (CMake flag — hook scope follows path set); CI pipeline (Phase 3.3); commit-message format

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A `pre-commit` framework + `.pre-commit-config.yaml` | 5 — versioned, pinned hook revisions; local+CI parity | 5 — adding a check = one YAML stanza | 5 — `pre-commit autoupdate`; well-documented | 5 — current OSS default for mixed-lang repos |
| B in-repo shell script via `core.hooksPath` | 3 — relies on PATH; silent version drift | 3 — script grows hand-maintained diff parsing | 3 — contributors must discover/enable | 3 — works but bespoke |
| C no local hook; CI only | 2 — bad code in history; bisect polluted | 5 — scales because does nothing | 4 — nothing to maintain locally; PR latency burns time | 1 — violates "seconds-not-minutes" |

### Cascade impact
- A (chosen): adds `.pre-commit-config.yaml` running on **staged files only**: `clang-format --dry-run --Werror`, `ruff check --fix` + `ruff format`, `mypy` (touched-files), `detect-secrets`, fast unit-test subset (CTest label `garmin-fast` + the Python tests). Onboarding: `pre-commit install` once. CI runs the same config via `pre-commit run --all-files` for the new paths. Hook **does not touch legacy** (scoped via `files:` regex).

### Chosen
A — standard tool, local/CI parity, scope-narrowing supported; catches secrets at commit time (DEC-003's threat model).

### Alignment probe
test -f .pre-commit-config.yaml && grep -E 'detect-secrets|ruff|clang-format' .pre-commit-config.yaml | wc -l   # expect ≥3

---

## DEC-011 — Phase-1 rollout strategy
- Status: accepted (recording-only)
- Reversibility: high (flag flip per-build)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-NF-Build-001, REQ-NF-Pkg-001
- Dependents: REQ-NF-Build-001, REQ-NF-Pkg-001; entire Phase-1 feature; DES-007 (CMake option + installer manifest delta)

### Chosen
New CMake option `GC_WANT_GARMINCONNECT`, default **OFF** until A4 passes. Mirrors `GC_WANT_COACH` (`coach:DEC-011`). When ON: build adds `src/Cloud/GarminConnect.{h,cpp}` + Python worker; installer bundles `garminconnect` + `curl_cffi`. When OFF: no Python network deps required.

### Alignment probe
grep -nE 'GC_WANT_GARMINCONNECT' src/CMakeLists.txt | wc -l   # expect ≥2 (option declaration + status line)

---

## DEC-012 — Auth-dispatcher seam (page ↔ SSO layer)
- Status: accepted (A — `IGarminAuthClient` interface, injected via constructor)
- Reversibility: high (replacing with signal-only shape is a constructor + WorkerAuthClient adapter refactor)
- Decided / last-reviewed: 2026-05-24
- Serves: REQ-002 (wizard-side acceptance), REQ-003 (MFA), REQ-005 (wizard-side enforcement), REQ-015 (CAPTCHA)
- Dependents: TEST-003 (encodes the interface in its fake); DES-001 (concrete `WorkerAuthClient` adapter added in REQ-002 GREEN); DES-003 (credentials-page seam); DES-003a (interface spec); REQ-003 + REQ-015 slices; `src/Cloud/IGarminAuthClient.h`; `unittests/Core/garminconnect/CMakeLists.txt`

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A inject `IGarminAuthClient` interface | 4 — compile-enforced; impossible to forget wiring | 4 — same interface extends trivially to MFA/CAPTCHA | 4 — small header; explicit seam visible at call site | 5 — mainstream DI; aligns with `CloudService` pure-virtual contracts |
| B page emits `submitRequested`; wizard glues to worker | 3 — signal/slot wiring is runtime; typo silently no-ops | 3 — MFA/CAPTCHA each need own signal taxonomy | 3 — logic spread across page + wizard glue | 4 — idiomatic Qt; matches existing `AddAuth` informal pattern |
| C page holds `GarminWorker*`; `enqueue()` virtual for tests | 2 — drags Python-embed headers into page build via `GarminWorker.h` | 2 — same coupling to every page; anti-cascade if DES-001 changes | 2 — two roles in one type; production deps leak into tests | 2 — concrete-class injection for test-only is a smell |

### Cascade impact
- A (chosen): refines DES-003 (credentials-page takes `IGarminAuthClient*` via ctor); new sub-section DES-003a specifies the pure-virtual surface + value-type result/error shapes. Refines DES-001 (worker ships `WorkerAuthClient : IGarminAuthClient` adapter forwarding `authenticate(...)` to `enqueue(Authenticate{...})` and re-emitting `finished`/`error` through the interface's signals). New file `src/Cloud/IGarminAuthClient.h` — compiles with `GC_WANT_GARMINCONNECT=OFF` (header-only contract). Unlocks REQ-003 (MFA) and REQ-015 (CAPTCHA) page tests: interface gains `mfaRequired(QUuid)` + `captchaDetected(QUuid)` when those slices land. No change to DEC-002/005/008.

### Chosen
A — only option giving compile-time guarantees AND keeping `garmin-fast` CTest Python-free AND naturally extending to MFA/CAPTCHA pages with no rework.

### Alignment probe
test -f src/Cloud/IGarminAuthClient.h && grep -nE 'class\s+IGarminAuthClient' src/Cloud/IGarminAuthClient.h | wc -l   # expect 1 when DES-003a GREEN lands

---

## DEC-013 — Worker ↔ Python adapter seam (page-side mirror of DEC-012, one layer down)
- Status: accepted (A — `IGarminPyAdapter` interface, injected into `GarminWorker`)
- Reversibility: high (swap to virtual `doAuthenticate()` or direct call is a header + worker-ctor refactor)
- Decided / last-reviewed: 2026-05-24
- Serves: REQ-002 (end-to-end Authenticate testable in C++ without spinning the embedded Python sub-interpreter); future REQ-003/006/007/010/012/014 worker-side ops (each adds a method to the same interface)
- Dependents: TEST-004 (will inject `FakePyAdapter` to drive worker behaviour); DES-001 (worker takes `IGarminPyAdapter*` via ctor); new DES-001a (interface spec, mirror of DES-003a); REQ-002 end-to-end slice; `src/Cloud/IGarminPyAdapter.h`; `src/Cloud/GarminWorker.{h,cpp}`

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A inject `IGarminPyAdapter` interface (mirror of DEC-012) | 5 — compile-enforced seam; tests cannot accidentally hit real Python | 5 — every future worker op (List, Download, Refresh, …) is one method on the same interface | 5 — small header; symmetry with DEC-012 reduces cognitive load | 5 — DI mainstream; matches the page-side pattern already accepted |
| B virtual `doAuthenticate()`; tests subclass `GarminWorker` | 3 — protected-virtual leak; tests reach into worker internals | 3 — every new op needs its own virtual + test subclass copy | 3 — two roles in one type; test-only ctor risk | 3 — viable but inferior to a real seam |
| C `#ifdef TESTING` swap real Python call for fake | 1 — production vs test code paths diverge by macro; ships test code by accident risk | 2 — every op needs its own #ifdef branch | 1 — macro pollution; review-resistant | 1 — anti-pattern |

### Cascade impact
- A (chosen): adds `src/Cloud/IGarminPyAdapter.h` — pure-virtual header-only contract compilable with `GC_WANT_GARMINCONNECT=OFF` (mirrors `IGarminAuthClient.h`). Refines DES-001: `GarminWorker(IGarminPyAdapter* py, QObject* parent=nullptr)` — worker no longer owns the embedded-Python sub-interpreter directly; the concrete `PyEmbeddedAdapter : IGarminPyAdapter` (production) does. Adds new sub-section DES-001a specifying the interface surface (mirror of DES-003a). REQ-002 GREEN uses `FakePyAdapter` in C++ tests; the real `PyEmbeddedAdapter` is exercised at the Python boundary by existing `test_adapter_login.py`. No change to DEC-002/004/012.

### Chosen
A — only option that gives compile-time guarantees, preserves the `garmin-fast` CTest Python-free invariant for the worker tests, and mirrors DEC-012 so every layer of the dispatch chain uses the same DI pattern. Future REQ slices (List/Download/Refresh/Profile/Disconnect) extend the interface additively with no test-shape rework.

### Alignment probe
test -f src/Cloud/IGarminPyAdapter.h && grep -nE 'class\s+IGarminPyAdapter' src/Cloud/IGarminPyAdapter.h | wc -l   # expect 1 when DES-001a GREEN lands

---

## DEC-014 — Token persistence: write-ownership + permission-enforcement seam
- Status: accepted (B — adapter exports blob via dumps()/loads(); C++ owns the atomic 0600 write)
- Reversibility: medium-expensive (not one-way; switching to A/C means reverting DES-013 forwarding + migrating or force-re-SSO existing athletes' blobs — no component becomes unbuildable)
- Decided / last-reviewed: 2026-07-11
- Serves: REQ-004, REQ-005, REQ-006, REQ-NF-Reliab-002, REQ-NF-Sec-002, REQ-NF-Sec-004; constrained by DEC-003 (per-athlete singular tokens.json, JSON), REQ-NF-Sec-004 (file-based Phase 1; keychain is Phase-1.5 non-goal)
- Dependents: DES-002 (confirmed literal — NO amendment), DES-006 (AtomicFile used as designed), DES-012 (+dump_tokens/load_tokens), DES-013 (stops forwarding tokenstorePath; exports blob post-login), REQ-004 + REQ-006 build slices
- Research: qgdw-scout 2026-07-11 (web-sourced; garth deprecated/unmaintained, python-garminconnect migrated to native OAuth engine, single `garmin_tokens.json`, in-memory `dumps()`/`loads()` API). Orchestrator Verification Gate: contract+goal PASS; evidence PASS with caveat — OQ1 below (library string-API method name unconfirmed against a repo pin; none exists, lib not installed in .venv).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A library writes, GC post-hardens | 2 — 0644 race window before chmod; non-atomic first write | 3 — fixed post-login cost | 2 — two write paths; depends on lib file-count staying single | 2 — "write insecure then fix perms" anti-pattern for secrets |
| B adapter exports blob, C++ owns atomic write | 5 — single tmp+fsync+rename at 0600, no exposure window | 4 — O(1) per athlete | 3 — new adapter surface kept in sync with lib string API | 5 — textbook seam; makes DES-002 literally true |
| C directory-level enforcement only | 3 — no race but no atomicity; torn-write detect over unknown file set | 3 — enumerate-dir blast radius | 2 — on-disk shape floats under GC across lib releases | 2 — externally-controlled layout as stored contract |

### Cascade impact
- B (chosen): **DES-002 needs NO amendment** (singular tokens.json / atomic / 0600 / lib-default-path-never-used all become literally true). DES-006 AtomicFile used exactly as designed. DES-012 `garmin_client.py` gains `dump_tokens() -> str` + `load_tokens(token_str: str)`; `login()` stops forwarding a tokenstore path (auth-only, in-memory session) → +2 pytest (round-trip; reject-tampered-blob) + matching pystub methods. DES-013 `PyEmbeddedAdapter` stops forwarding `tokenstorePath` to `GarminClient(...)`; surfaces the blob post-login (new `PyAuthOutcome.tokenBlob` on Success) so the worker/CloudService persists it via AtomicFile; `load_tokens(blob)` on resume replaces password login (the mechanism that lets the retained `m_client` skip SSO). REQ-006 load-side mode check gates the single `load_tokens()` call. garmin-fast stays Python-free (C++ tests hit the pystub only; the 2 new pytests run in the Python lane).

### Open questions
- OQ1 (exact string-API method names on the *bundled* library version) — CARRIED to build as a NOTE: builder implements against the pystub/fake contract (`dump_tokens`/`load_tokens`); real-library method-name wiring is confirmed when the wheel is bundled (DES-007 / REQ-NF-Pkg-001, already deferred). NOT GREEN-blocking for the C++/fake seam.
  - **RESOLVED 2026-09-13 by Stage 9 live-account evidence — and it came due exactly as this NOTE warned, as a real user-visible blocker (`B-STAGE9-09`).** Against the real installed python-garminconnect 0.3.15, the adapter's `Garmin.full_name_id` read does not exist (the object exposes `display_name` and `full_name` only), so `_login_impl`/`_submit_mfa_impl` raised a raw `builtins.AttributeError` that escaped classification and folded to the generic `Unknown` UI copy on four consecutive live connect attempts. The identity-attribute half of OQ1 is now CLOSED on real evidence: `garmin_user_id` ← `Garmin.display_name` (see `design.md` § `<garmin_user_id>`, prose corrected the same day). The `dump_tokens`/`load_tokens` string-API half remains as originally written, still pinned by the pystub/fake and still owed real-wheel confirmation under DES-007 / REQ-NF-Pkg-001.
  - **Standing lesson this OQ paid for (see `lessons.md` LSN entry, 2026-09-13):** "pinned by the fakes/pystub" is not a neutral holding position — a fake that *defines* an attribute the real library lacks makes the whole suite agree with the defect. Here it also made A3-REQ-002's mutation testing certify it: mutants M8/M9 swapped `full_name_id`↔`display_name` and were both recorded KILLED, because the happy-path identity assertion was checking the fake against itself. A mutation score computed entirely against fakes measures fake-fidelity, not real-library fidelity. Any remaining "pinned by the fake until the wheel is bundled" NOTE must therefore carry a real-wheel contract test (skipped, not failed, when the wheel is absent) rather than a prose promise.
- OQ2 (load_tokens() failure when Garmin invalidated the session server-side) — RESOLVED: routes to REQ-NF-Compat-001(b) "prompt full re-login, no silent reauth" with a DISTINCT error kind (`session_expired`) so DES-008 messages it differently from REQ-006's permission-rejection. Both end in fresh SSO; the surfaced message differs.

### Alignment probe
grep -nE 'dump_tokens|load_tokens' src/Python/garminconnect/garmin_client.py | wc -l   # expect ≥2 when REQ-004/006 GREEN lands

---

## DEC-015 — Ledger status: single canonical source + absence-check drift lint (LSN-008 promotion-to-mechanism)
- Status: accepted (C — normalize per-id status to ONE canonical home per id-class AND add a thin absence-check lint as regression backstop; sub-decision: collapse to a single live cursor by deleting the local `state.md`)
- Reversibility: medium (a schema/layout change to the governance tree; reverting means re-scattering status back across files — no artifact becomes unusable, but the migration edits every status-bearing governance file once)
- Decided / last-reviewed: 2026-07-12
- Committed: `88d4ea402` (lint mechanism + TEST-017 + install/pre-commit wiring + gitignore) · `2520ed034` (ledger normalization migration + local state.md deletion + WIKI/STATE/conventions). VAL-012 PASS.
- Serves: retiring LSN-008 (op:ledger-update / index-vs-detail-drift, recur:7 miss:6 — DEFINITIVE promotion trigger recorded across 7 loci); the workflow's own maintainability. Constrained by Principle 9 (update-as-byproduct), Principle 10 (promotion-to-mechanism), the wiki-first orientation contract (root STATE.md is the file read first each session), and user execution conditions (2026-07-12): lint-FIRST, explicit code-level scope, install via install_hook.py, vocabulary+state-machine into wiki/conventions.md, full CLV to close.
- Dependents: `traceability.md` (becomes canonical status ledger), `decisions.md` (canonical DEC-status), `design.md` (status stripped → intent tense), root `STATE.md` (cursor only, per-id status removed), `WIKI.md` REGISTRIES (ranges+next only), local `.claude/workflow-garminconnect/state.md` (DELETED), `wiki/conventions.md` (+vocabulary/state-machine), `scripts/ledger_drift_lint.py` + `install_hook.py` (new lint + wiring), `lessons.md` (LSN-008 promoted, LSN-014 portable capture). Allocated build id: TEST garmin-T-017 (lint's own test suite).
- Research: none dispatched — trade space is fully internal (our own ledger schema, read directly this session); constraints determine the options, so drafted inline per the Three Options Doctrine (no qgdw-scout).

### The problem
Each id's lifecycle status is ONE logical fact stored in 7–8 physical places; every byproduct update must hand-sync them and context loss between turns makes that lossy. The manual CHECK-list control was broadened 4× and still missed — enumerating loci and parsing free-prose status is the fragile part. The seven recorded loci (VAL-007…VAL-011):
1. `traceability.md` PRIMARY REQ→DEC→DES→TEST→COMMIT matrix rows (status strings + Commit column)
2. `traceability.md` DEC index + DES index Status cells (freeform status prose)
3. `traceability.md` per-slice appendix "artifacts" tables (`| Artifact | Path | Serves |`, carry "DONE/still forwards/GREEN" status language)
4. local `.claude/workflow-garminconnect/state.md` structured tables (`## reqs/des/vals/last-clv/open`)
5. root `STATE.md` body (CASCADE/NEXT_GATE/CHANGESET/COUNTS/VAL-registry) drifting vs its OWN banner (intra-file)
6. `design.md` prose (DES bodies + "DEC-NNN refinement"/"what this does NOT cover" notes + ctor code snippets + "Serves:"/"### Surface")
7. `WIKI.md` REGISTRIES per-id status strings ("in build"/status) + phase banner

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A pre-CLV/pre-commit drift lint only (no schema change) | 3 — deterministic catch but only for enumerated loci (same completeness gap that failed 4×); detects AFTER drift, repair can re-drift (VAL-011 passes 1-3) | 2 — must parse open-ended free-prose status (design.md); brittle N-way agreement grows with ledger | 3 — one isolated script, but perpetually chasing new status phrasings | 2 — band-aid over duplicated data; DRY violation intact |
| B normalize to one canonical home per id-class (strip/derive elsewhere) | 5 — status stored once CANNOT drift; bug class eliminated | 5 — new id = one row in one place; dual-cursor maintenance gone | 4 — sharp long-term drop; one-time migration is a large multi-file edit needing its own validation | 5 — textbook SSOT + design-intent/live-status separation |
| C normalize AND thin absence-check lint (chosen) | 5 — B's elimination + guards against a context-lost turn re-adding status prose (this project's exact failure mode); lint is an ABSENCE check with no vocabulary-completeness dependency | 5 — B's scaling + lint is a trivial scoped grep | 4 — B's cascade + ~30-line near-zero-upkeep lint | 5 — SSOT + a deterministic guard encoding the invariant, surviving agent/context turnover a manual CHECK-list provably does not |

### Cascade impact (per locus; the migration)
Canonical homes (the ONLY files where an id may be paired with a lifecycle-status token): **`traceability.md`** for REQ/DES/TEST/VAL status; **`decisions.md`** per-DEC `Status:` for DEC status. All other live-governance files carry references WITHOUT status tokens.
- Locus 1 → stays canonical; status strings normalized to the controlled vocabulary; Commit column = git provenance (kept).
- Locus 2 → DES index Status cells canonical, controlled vocabulary only (no freeform prose); traceability DEC-index Status column DROPPED (drill `decisions.md` for DEC status) to avoid DEC-status duplication.
- Locus 3 → appendix "artifacts" tables reclassified as immutable dated **provenance**, marked `<!-- provenance: dated; excluded from status-lint -->`; live-status verbs stripped (they describe what was built at a commit, not current state).
- Locus 4 → local `state.md` **DELETED**; all pointers (WIKI PAGES, workflow-INDEX, any "full live cursor" refs) repointed to root `STATE.md` as the sole cursor.
- Locus 5 → root `STATE.md` becomes the lean **cursor**: phase, TEAM, RIGOR, current gate/focus, open findings (by id, no status token), CHANGESET (commit hashes = provenance), pointers to traceability/validations. Per-id GREEN/committed/CLOSED enumeration removed; COUNTS rollup dropped (derivable from traceability).
- Locus 6 → `design.md` status assertions stripped → design-INTENT tense (target shape, not "DONE"/"deferred"); [[LSN-011]] sibling (design-note false-done) subsumed.
- Locus 7 → `WIKI.md` REGISTRIES = ranges + `next` only; phase banner names the phase, not per-id status.

### The invariant (what the lint enforces) — condition 1
Absence check, not agreement check. **The canonical ledgers `traceability.md` and `decisions.md` are the SOURCE OF TRUTH and are fully exempt (not scanned)** — they legitimately co-locate ids with status (the REQ→DEC→DES→TEST matrix cites a REQ's governing DECs/DES/TESTs on the same row as the REQ's status; DEC cascade-notes reference the REQ/DES a decision affects). The lint enforces status-ABSENCE only in the **non-canonical governance set**: root `STATE.md`, root `WIKI.md`, `wiki/*.md`, and each ledger's `design.md`. In those files no id token (`REQ-\d{3}`, `REQ-NF-\w+-\d{3}`, `DES-\d{3}[a-z]?`, `TEST-\d{3}`/`T-\d{3}`, `VAL-\d{3}`, `DEC-\d{3}`) may appear on the same line as a lifecycle-status token (`GREEN|CLOSED|DEFERRED|deferred|drafted|in build|_pending_|_uncommitted_`; "committed" excluded — commit-hash provenance entanglement; "in progress" dropped during migration — collides with the spec'd REQ-NF-Perf-002 "sync already in progress" message and is not in the controlled vocabulary). Explicit code-level exclusions: `archive/`, `cycles/`, `validations/`, `lessons.md`, `findings.md`, and any block after a `<!-- provenance: dated ... -->` marker (runs to the next `## ` heading). Runs pre-commit (framework entry scoped to ledger paths) + as a CLI the CLV step invokes. Installed via `install_hook.py` alongside `anti_duplication_guard.py`.
- *Refinement note (2026-07-12, discovered by running the built lint against the real tree):* the original briefing's "foreign-class catch" (flag DEC+status inside traceability.md) was dropped — it false-positived on legitimate traceability cross-citation. Correct rule = exempt the canonical ledgers entirely; the real duplication loci (STATE/WIKI/design/wiki-spokes + the deleted local state.md) are all still covered.

### Controlled vocabulary + state machine (→ wiki/conventions.md, condition 3)
Per-id lifecycle: `drafted → GREEN → committed → CLOSED`, with `deferred` as an orthogonal tag on a not-yet-built slice. (DEC-status stays `accepted (X)` in decisions.md.) One vocabulary, re-counted from source, never copied between docs.

### Execution order (user conditions 1–2, 6) & acceptance
1. Build the lint FIRST (TDD, qgdw-builder, TEST-017) with the scope above.
2. Wire + install via install_hook.py.
3. Run the migration (orchestrator = single writer of governance files); acceptance = **lint passes tree-wide**.
4. Full 9-check CLV (qgdw-validator); on PASS, close LSN-008 as promoted-to-mechanism and capture scope:portable LSN-014.

### Open questions / risks
- OQ1 — STATE.md is read first each session and must stay a *useful* cursor after status is stripped; risk it becomes too thin to orient. Mitigation: it keeps the current-gate + changeset + open-findings (the volatile cursor facts that legitimately live only here), and points to traceability for per-id status. Validate during migration that a cold session-start still orients from it.
- OQ2 — the migration edits every status-bearing file once; drift can be *introduced* during the cut (ironic). Mitigation: lint is the acceptance gate (must be GREEN tree-wide before commit) + closing full CLV.
- OQ3 (RESOLVED by the corrected invariant, see Refinement note above) — the appendix provenance tables live in `traceability.md`, which the corrected lint exempts UNCONDITIONALLY (canonical source-of-truth, never scanned). So the `<!-- provenance: dated -->` marker is NOT needed there and none were added — no false-positive risk exists for those tables. The marker mechanism still applies to any provenance block that appears in a SCANNED file (STATE.md / WIKI.md / wiki/* / design.md), where it exempts the block up to the next `## ` heading.

### Alignment probe
python3 scripts/ledger_drift_lint.py .   # expect exit 0 (no status token outside canonical homes) once migration lands
# Path corrected 2026-08-05 (ORCH-004): the lint's canonical source moved OUT of the skill tree to the
# project-owned `scripts/` this DEC's Dependents line specified all along; .claude/hooks/ holds the synced
# copy pre-commit + CLV invoke. The skill tree is vendor territory, replaced wholesale on every update.

---

## DEC-016 — FIT→TCX fallback-trigger contract for REQ-007 readFile (PRD Assumption-B resolution)
- Status: accepted (C — hybrid: kind-aware TCX retry + content-sniff backstop; RateLimited fails fast)
- Reversibility: cheap (internal adapter/readFile control-flow only; no persisted state, no new public interface, no schema commitment — a future DEC can revise the retry table with no migration if live Garmin testing shows different behaviour)
- Decided / last-reviewed: 2026-07-14
- Serves: REQ-007 (activity download → FIT/TCX pipeline); resolves PRD Assumption-B (Garmin's server-side signal for a FIT-less activity is unvalidated from local source — real `python-garminconnect` not installed, only stubs). Constrained by DEC-006 (FIT default, TCX fallback — non-negotiable), DEC-002 (GarminWorker is sole Python caller; trigger must be observable at the C++/adapter seam), LSN-006 (classify by exception TYPE, never leak raw library kinds).
- Dependents: DES-004 (readFile fallback decision-table + the ZIP-unwrap step — see OQ2); REQ-007 closure build (worker-in-CloudService lifecycle + readFile). Adapter `download_activity` and the `GarminDownloadFailure` kind enum are UNCHANGED under Option C (lower cascade footprint than A). Allocated build ids assigned at REQ-007 build-brief (TEST garmin-T-018+).
- Research: qgdw-scout dispatched (2026-07-14) — upstream landscape volatile (third-party lib, `garth` dep dropped + deprecated within the year), so web research per the doctrine rather than inline. Draft verified through the Verification Gate (three real options, dated sources on volatile claims, concrete cascade) before presentation.

### The problem
`readFile` requests the FIT (`ORIGINAL`) download and needs an OBSERVABLE condition to decide "no FIT here → retry as TCX." Corroborated evidence (two independent long-running export tools hitting the same Garmin endpoints) says a missing FIT original surfaces as an **HTTP 404**, which `python-garminconnect` collapses into its generic `GarminConnectConnectionError` (its public surface has only 4 exception classes — no not-found/format-specific type). So today's adapter already conflates "no FIT" (404) with "network died" under kind=`connection`/Network. The exact status→exception mapping could NOT be verified from source (real lib not installed; upstream fetch truncated) — this residual uncertainty IS Assumption B, and it decides which option is safe.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A new `NotAvailable` kind from adapter exception-cause inspection | 2 — depends on undocumented internals of a lib whose HTTP stack churned 2× in a year; silently fails to fall back if status isn't exposed | 4 — precise kind, no wasted retries | 2 — brittle; needs live-404 fixture re-validation each version bump; hard to unit-test w/o mocking lib internals | 4 — most directly implements LSN-006 IF internals cooperate |
| B C++-side content-sniff of returned bytes only | 2 — blind to the 404-EXCEPTION branch, which is the corroborated real trigger; only catches empty/wrong-content 200s | 4 — negligible CPU, no extra retries on exception branch | 5 — fully offline-testable with static byte fixtures, zero upstream-internals dependency | 2 — contradicts LSN-006 for the branch that actually fires; leaves 404 case unmet |
| C hybrid: kind-aware retry + content-sniff backstop (chosen) | 5 — every point in the (a)–(d) uncertainty space lands in a branch we already retry on; excludes RateLimited where auto-retry is harmful | 4 — bounded to one extra retry; RateLimited exclusion prevents throttle amplification at batch volume | 4 — more branches but each independently testable; retry table stated in one place | 3 — documented, bounded deviation from strict LSN-006 (Network kind stays coarse), not a silent violation |

### The decision table (what readFile does)
```
readFile(activityId):
  request ORIGINAL (FIT) via worker → await downloaded / downloadFailed
  on downloadFailed(kind):
      kind == RateLimited → FAIL fast (surface error; NO retry — anti retry-storm)
      kind == Network     → retry once as TCX
      kind == Unknown     → retry once as TCX
  on downloaded(bytes):
      unzip (ORIGINAL is ZIP-wrapped — see OQ2); sniff FIT magic (".FIT" @ offset 8)
      is FIT  → stage as garmin-<id>.fit
      not FIT → retry once as TCX
  TCX retry result → stage as garmin-<id>.tcx (or surface failure)
```
LSN-006 deviation (documented): the `Network` kind legitimately conflates a true network failure with a 404 "no original," because the library's coarse exception taxonomy makes them indistinguishable at the seam. Retrying TCX on `Network` costs at most one wasted attempt on a genuine outage (self-limiting), and is the only way to honour REQ-007's fallback under the real (coarse) upstream contract.

### Cascade impact
- Adapter `download_activity` (DES-013) — UNCHANGED (still maps connection→Network, rate_limit→RateLimited, else→Unknown).
- `GarminWorker` / `GarminDownloadFailure` kind enum (DES-001) — UNCHANGED (no new kind).
- DES-004 — gains the explicit readFile decision-table above + the ZIP-unwrap step + the LSN-006-deviation rationale.
- TEST-008/009/010 (already GREEN) — re-audit only: confirm none assert "Network kind → no TCX retry"; if any do, they update at REQ-007 build (bounded, explicit — unlike Option A's silent-failure risk).
- New build tests (garmin-T-018+) at REQ-007 build: Network→retries TCX, RateLimited→no retry, Unknown→retries TCX, content-sniff fixtures (valid zip+fit, zip+tcx/gpx, empty, HTML error page).

### Open questions / risks (carried to REQ-007 build)
- OQ1 (Assumption-B smoke, build NOTE, non-blocking) — once real credentials/network exist, smoke-test ONE manually-entered (FIT-less) activity to confirm the 404→Network path, before finalizing any TEST-008/009/010 rewrite. Cannot be done in this environment (stubs only). Option C is chosen precisely so a wrong guess here does NOT break the fallback.
- OQ2 (ZIP-unwrap, build-blocking prerequisite, independent of DEC-016) — ORIGINAL responses are ZIP-wrapped (per PyPI example `zip_data = ...ORIGINAL`), NOT bare FIT bytes. readFile must unzip before FIT-parsing/sniffing. Confirm whether GC already links a ZIP utility (exports use one) before the build; DES-004 must document the unwrap step regardless of this decision.

### Alignment probe
grep -nE 'NotAvailable|dl_fmt.*TCX|sniff|\.FIT' src/Cloud/GarminConnect.cpp | wc -l   # expect ≥1 once REQ-007 readFile fallback lands (retry table implemented)

---

## DEC-017 — REQ-008 sidecar-persistence component shape
- Status: accepted (A — dedicated GarminSidecarStore)
- Reversibility: medium (the class boundary + test surface stick once Slice C and REQ-010 backfill call it)
- Decided / last-reviewed: 2026-07-19
- Serves: REQ-008 (Slice B — Tier-1 dedup substrate), REQ-010 (backfill-state reuse), REQ-012 (sidecars preserved across Disconnect); constrained by DES-002 (per-account storage layout), DES-006 (AtomicFile), DES-008 (SidecarPermissionsRejected message key)
- Dependents: REQ-008 Slice C (sync orchestration reads/writes via this store), REQ-010 (bulk backfill resumability keys off backfill-state), DES-002 (gains a realized "sidecar store" note)

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A dedicated GarminSidecarStore.{h,cpp} (mirrors GarminTokenStore) | 5 — reuses the proven AtomicFile + perm-refusal path | 4 — both sidecar schemas in one class; a 3rd is a method | 5 — parallel to GarminTokenStore; unit-testable in isolation (garmin-fast) without the sync flow | 5 — matches the DES-002/006 seam; single-responsibility persistence boundary |
| B inline JSON into GarminConnect | 3 — same primitives but perm-check scattered in orchestration | 2 — sync + backfill both bloat GarminConnect.cpp | 2 — dedup substrate can't be tested without driving the whole flow | 2 — mixes persistence with orchestration |
| C generic JsonSidecar helper + typed wrappers | 5 — one perm-checked atomic-JSON path | 5 — any future sidecar reuses it | 4 — extra layer; GarminTokenStore isn't retrofitted, so a partial generalization | 4 — DRY but speculative; only two near-term consumers |

### Cascade impact (per option)
- A (chosen): adds `src/Cloud/GarminSidecarStore.{h,cpp}` (new files → LSN-018: wire into BOTH the unittest target AND the app's `GC_WANT_GARMINCONNECT`); DES-002 gains a realized "sidecar store owns imported-<uid>.json + backfill-state-<uid>.json" note; one new `garmin-fast` TEST group; production `GarminConnect.*` untouched until Slice C.
- B: no new files, but Slice C's sync orchestration inherits perm-check + torn-write handling inline; the dedup substrate becomes untestable in isolation; diverges from the GarminTokenStore precedent.
- C: adds `src/Cloud/JsonSidecar.{h,cpp}` PLUS the two typed wrappers (more surface than A) for no near-term payoff; the token store stays separate, so the generalization is only partial.

### Chosen
A — smallest change that keeps the Tier-1 dedup substrate independently testable and reads consistently with the existing GarminTokenStore. Generalization (C) is deferred until a third consumer outside this feature justifies it (YAGNI); inlining (B) is disqualified by the isolation-testability loss.

### Open questions / risks (carried to Slice B build)
- The perm-refusal semantics reuse REQ-006's `loadChecked` pattern verbatim (owner-only, refuse-and-report-path); confirm the DES-008 `SidecarPermissionsRejected` message key exists (it is drafted in design.md) or allocate it at build.
- Sidecar torn-write handling: parse failure → treat as absent (re-fetch), NOT a hard error, per DES-002 "fallback to last-good or trigger re-fetch". The store returns a typed absent/torn status; the caller (Slice C) decides.

### Alignment probe
grep -rln 'class GarminSidecarStore' src/Cloud | wc -l   # expect 1 once Slice B lands (dedicated component in place)

---

## DEC-018 — Where to persist garmin_user_id (the active-account producer for REQ-008 sync)
- Status: accepted (B — separate account-agnostic active-account.json)
- Reversibility: medium (the file name + resolveGarminUserId's read source stick once connect/disconnect write/clear it)
- Decided / last-reviewed: 2026-07-19
- Serves: REQ-008 Slice D (closes A3-R008-01 — the uid-producer gap), REQ-012 (disconnect clears it); constrained by REQ-006 (tokens.json perm-check schema is frozen), REQ-007 (from_tokens/loadTokens parse the raw garth blob), DES-002 (per-account sidecar paths need the uid BEFORE any account-specific read)
- Dependents: GarminConnect::resolveGarminUserId (repoint read source), the auth-success path (new writer), Disconnect/removeSettings (clear), REQ-008 Slice D end-to-end test
- Origin: A3-R008-01 (readdir resolves an empty uid in production → live sync no-ops; producer never built)

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A wrap tokens.json `{garmin_user_id, tokens:<blob>}` | 3 — one atomic write, but MUTATES the security-locked schema REQ-006 perm-checks + REQ-007 parses | 4 — envelope can hold future account metadata | 3 — open()/blockingRestore must unwrap `.tokens`; REQ-006/007 read path + tests churn | 3 — mixes identity into the secret file; touches a frozen contract |
| B separate account-agnostic `garminconnect/active-account.json = {garmin_user_id}` | 4 — tokens.json UNTOUCHED; a missing file already degrades to empty→no-op (graceful) | 4 — clean home for active-session metadata, separate from the secret | 5 — ZERO change to the locked token read path; resolveGarminUserId just reads a different file; disconnect clears it | 5 — identity ≠ secret; smallest blast radius on security-critical code |
| C parse uid out of the existing garth `dumps()` blob | 2 — depends on garth's undocumented token internals (OQ-risk; breaks on lib change) | 2 — no home for other metadata | 2 — brittle; violates the DES-012 don't-depend-on-library-internals seam | 2 — reads undocumented structure |

### Cascade impact (per option)
- A: write side saves the wrapped envelope; `GarminConnect::open()`/`blockingRestore` must extract `.tokens` before restoreSession; REQ-006 loadChecked + REQ-007 from_tokens tests may need envelope-awareness; any already-written tokens.json needs migration. resolveGarminUserId already reads `.garmin_user_id` top-level (would keep working).
- B (chosen): add a small writer at auth-success that persists `{garmin_user_id}` to `garminconnect/active-account.json` (atomic write via AtomicFile; the uid is not a secret so a tolerant read, not the strict perm-refusal tokens.json gets); repoint `GarminConnect::resolveGarminUserId()` (GarminConnect.cpp:168) from tokens.json to active-account.json; clear the file on Disconnect (REQ-012 — the token file is deleted, and the active-account pointer with it, but per-account sidecars are preserved). tokens.json schema + REQ-006/007 read path FROZEN, untouched. One new small file added to the garminconnect/ dir.
- C: no new write; resolveGarminUserId parses the loadChecked bytes for an embedded uid — but couples to garth internals, re-opening the OQ risk the DES-012 seam exists to contain.

### Chosen
B — least disturbance to the security-locked tokens.json and the REQ-006/007 read path; cleanest separation of "which account is active" from "the OAuth secret"; graceful degradation (absent file → empty uid → no-op, already the coded behaviour). The one cost — a second small file under the perm/atomic discipline — is cheap and non-security-critical (the uid is not a secret).

### Open questions / risks (carried to Slice D build)
- Consistency: active-account.json and tokens.json are written in the same auth-success step but are two files — order the writes so a crash leaves at worst a missing/stale active-account.json (→ empty uid → no-op), never a token file pointing at a wrong active account. Prefer: write tokens.json first, then active-account.json.
- Disconnect (REQ-012) must clear active-account.json alongside tokens.json deletion, but MUST preserve the per-account imported-<uid>/backfill-state sidecars (REQ-012, DES-002).
- End-to-end test (the one A3-R008-01 says is missing): connect with a REAL (non-override) uid → active-account.json written → a fresh GarminConnect (no ctor override) → resolveGarminUserId reads it → readdir resolves the account. This is the integration test that the ctor-override unit tests could not provide.

### Alignment probe
grep -rn 'active-account' src/Cloud/GarminConnect.cpp | wc -l   # expect ≥2 once Slice D lands (writer + resolveGarminUserId read source)

---

## DEC-019 — The production connect/disconnect persist TRIGGER insertion point
- Status: accepted (C — CloudService::disconnect() virtual + GarminConnect owns persist/disconnect; wizard-level finished() capture)
- Reversibility: cheap (both the wizard finished()-connection and the additive default-no-op CloudService::disconnect() virtual are additive; nothing locks a format/protocol/topology). Minor one-way risk: once other services override disconnect(), removing the virtual is a wider but mechanical revert.
- Decided / last-reviewed: 2026-07-20
- Serves: REQ-008 (closes A3-R008-01 — invokes the Slice-D producer on a real connect), REQ-012 (disconnect deletes tokens.json + active-account.json, preserves sidecars); constrained by DEC-014 (Garmin bypasses appsettings/CloudServiceFactory::saveSettings — file token store), the MFA-both-paths constraint, and the absence of any disconnect virtual in CloudService.h (D-R008-01)
- Dependents: the trigger-wiring build slice (TEST-051+), AddCloudWizard.cpp (single finished() connection), GarminConnect.{h,cpp} (persistConnectSuccess + disconnect() override), CloudService.h (new virtual), src/Gui/AthletePages.cpp (deleteClicked → generic disconnect()), DES-002 (prose correction — the "removeSettings overridden" surface is replaced by disconnect())
- Origin: A3-R008-01 (orphaned producer) + Slice D stop-and-report; research draft by qgdw-scout 2026-07-20 (three real options, all citations orchestrator-verified on disk)

### Key research findings (scout, verified)
- Both `AddGarminAuth` (pg 21) + `AddGarminMfa` (pg 22) are constructed with the SAME `garminChain->client()` (AddCloudWizard.cpp:188,195), so one wizard-level listener on that client's `finished(QUuid,GarminAuthSuccess)` catches BOTH direct + post-MFA success — the only structural fix for the MFA early-return miss.
- `GarminConnect::resolveConfigDir()` (GarminConnect.cpp:138-145) == `wizard->context`'s config dir — identical in every option.
- **REQ-012 has NO sibling idiom to copy**: the real disconnect UI `CredentialsPage::deleteClicked()` (src/Gui/AthletePages.cpp:143-161) only flips active/sync flags in appsettings; every sibling (Dropbox/Strava/Xert/Withings/…) leaves its OAuth token in QSettings forever (OAuthDialog.cpp). Garmin's "actually delete the token file" is stricter than anything shipped — must be newly designed, which is why a testable seam beats a magic-string GUI branch.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A persist inside both pages (credentials + MFA) | 2 — each re-gated by its own state machine → reproduces the MFA early-return bug class | 2 — no reuse; every future multi-step native-auth service re-copies it | 2 — ctor churn on 2 shipped widgets; pollutes 2 deliberately Context/Python-free test targets with disk I/O | 2 — contradicts the "commit once at the wizard" idiom (AddFinish::validatePage, AddCloudWizard.cpp:857-861) |
| B wizard owns one finished() connection, persists directly via GarminTokenStore | 3 — single ungated capture fixes MFA-both-paths; stale-reply dedup is an open Q | 3 — Garmin-specific inside AddCloudWizard.cpp, no reusable abstraction | 4 — smallest diff; reuses testGarminConnectWizardRouting target | 4 — matches idiom's spirit; but disconnect = untested Garmin-only string branch in shared AthletePages.cpp |
| C CloudService::disconnect() virtual + GarminConnect owns persist/disconnect (chosen) | 4 — same MFA fix as B PLUS disconnect logic unit-testable in the garmin-fast GarminConnect harness | 4 — real reusable seam (no sibling clears creds today; multi-account later has a seam) | 4 — wizard capture like B + trivial GarminConnect methods beside open()/close(); GUI shrinks to one generic line | 5 — best fit to letter+spirit; token-store knowledge colocated in GarminConnect.cpp; generic disconnect() |

### Cascade impact (chosen = C)
- CloudService.h: add `virtual void disconnect() {}` (default no-op — non-breaking for ~15 other services). **[Ratified 2026-07-20 during build:** a SYMMETRIC `virtual void persistConnectSuccess(uid, blob) {}` default-no-op was added alongside it, because the wizard dispatches through a `CloudService*` handle — the persist call is only well-typed on the base, and this keeps the wizard fully generic (no GarminConnect include/cast, no Python stack in the wizard test). Consistent with Option C's "wizard calls the service through the base handle"; both are safe no-ops. Orchestrator-accepted.]**
- GarminConnect.{h,cpp}: `persistConnectSuccess(uid, blob)` wrapping `GarminTokenStore::persistConnectSuccess(resolveConfigDir(), uid, blob)`; `disconnect()` override calling `GarminTokenStore::clearAccount(resolveConfigDir())`.
- AddCloudWizard.{h,cpp}: ONE `connect(garminChain->client(), &IGarminAuthClient::finished, …)` in ensureGarminAuthPage() (guarded by the existing `if (garminChain) return;` idempotency) → delegates to `wizard->cloudService->persistConnectSuccess(...)`.
- src/Gui/AthletePages.cpp deleteClicked(): call the service's generic `disconnect()` (pattern already used for sync-now, AddCloudWizard.cpp:868-871) — no Garmin string-compare.
- DES-002 (design.md ~489): correct the "removeSettings overridden to Disconnect" prose to the real `CloudService::disconnect()` virtual (resolves D-R008-01).
- Tests: persist covered in testGarminConnectWizardRouting (add GarminTokenStore/AtomicFile to its link list) or a new target; disconnect covered via the GarminConnect::disconnect() override in the existing garmin-fast harness (testGarminConnectConnectPersist/testGarminConnectOpen). TEST-051+.

### Chosen
C — satisfies the MFA-both-paths constraint identically to B (single ungated wizard-level finished() capture) while getting REQ-012's stricter, precedent-less disconnect behavior under automated test behind a small reusable virtual, instead of an untested Garmin-only branch in shared GUI code. The one objection — touching shared CloudService.h — is additive and safe (default no-op). B is an explicit, non-strawman fallback if minimizing shared-header churn ever outranks testable disconnect.

### Open questions / risks (carried to the build slice)
- **Stale-reply dedup at the wizard level:** the pages guard with `m_pendingId` to reject a slow/stale `finished()` from an abandoned earlier attempt (A3-R003-06). A wizard-level catch-all must NOT overwrite a fresh correct token with a late stale one — track the "current" pendingId on the wizard, or confirm GarminWorker never delivers out-of-order across distinct request UUIDs. Design call in the build slice.
- Confirm allocating a throwaway `CloudServiceFactory::newService(id, context)` in deleteClicked() purely to call disconnect() is acceptable (the sync-now path already does this), or expose disconnect() without a full clone.
- `WizardStubPreamble.h` Athlete::config() returns a hardcoded `/tmp/gc-garmin-test` (not a QTemporaryDir) — tighten to avoid parallel-CTest collisions before adding a persist-on-finished test there.

### Post-build correction (2026-07-20, A3-R008-F4 / LSN-027)
The base virtual was renamed `disconnect()` → **`disconnectService()`** during A3-R008 hardening: a bare `disconnect()` on a QObject-derived base name-hides `QObject::disconnect()` for all ~15 CloudService subclasses. So the realized surface is `CloudService::disconnectService()` (default no-op) + `GarminConnect::disconnectService() override`; `deleteClicked()` calls `disconnectService()`. `persistConnectSuccess` was unaffected (no QObject collision).

### Alignment probe
grep -rn 'void disconnectService' src/Cloud/CloudService.h src/Cloud/GarminConnect.h | wc -l   # expect ≥2 (base virtual + GarminConnect override)

---

## DEC-020 — Disposition of A3-R012-F1: a live session outliving Disconnect
- Status: accepted (C — fail-closed re-check + cheap hardening; full lifecycle work deferred to its own REQ) · IMPLEMENTED `f001c7d20` (2026-08-03)
- Reversibility: high (C is additive and local to GarminConnect; it does not foreclose A)
- Decided / last-reviewed: 2026-08-02
- Serves: REQ-012 (the "before clearing in-memory state" criterion clause), REQ-NF-Sec-* (a revoked account must stop syncing); triggered by A3-R012-F1 (blocking)
- Dependents: REQ-012 (closure), DEC-019 (its fresh-instance choice is the root cause and stays in force), REQ-016 (shares the readFile record path), a NEW follow-on REQ for instance-lifecycle/leak work (A3-R012-F10/F12)

**Problem.** `GarminConnect::disconnectService()` deletes tokens.json + active-account.json and clears no in-memory
state; nothing enumerates or shuts down live GarminConnect instances. A window-modal `CloudServiceSyncDialog`
(AddCloudWizard.cpp:889-893) can therefore keep downloading from a just-disconnected account, because the parentless
`ConfigDialog` (ConfigDialog.cpp:37-41) leaves Delete reachable concurrently and `readFile` (CloudService.cpp:1400)
re-checks no token. DEC-019 deliberately chose to mint a FRESH instance in `deleteClicked()` to avoid touching live
instances — that choice is exactly what leaves the live one running, so this is a DEC-level question, not prose.

**Options considered (Three Options Doctrine; scored R/S/M/BP 1-5).**
- **A — Full lifecycle binding.** Reopen DEC-019: invalidate live instances on disconnect (a registry of open
  services, or bind session lifetime to the account) AND fix the `CloudServiceSyncDialog` store leak. R5 S4 M4 BP5.
  Rejected FOR NOW: touches shared CloudService/GUI code owned by no current REQ, and drags the pre-existing
  instance-leak (A3-R012-F12) into REQ-012's scope.
- **B — Ticket and ship.** Commit the (safe) test slice, raise F1 as a new REQ, reopen DEC-019 later. R2 S3 M4 BP3.
  Rejected: the defect ships, and REQ-012 stays open on a security-relevant clause with no mitigation.
- **C — CHOSEN — Fail-closed re-check + cheap hardening.** `GarminConnect::readFile`/`readdir` re-check that the
  account is still connected (tokens present) and fail closed once it is not; plus the cheap non-blockers F2 (sweep
  the `<path>.tmp` siblings in `clearAccount`), F4 (pin the empty-uid write guard), F5 (assert mode + mtime, not just
  bytes, in TEST-055). R4 S3 M4 BP4. Severs the exploit path with a small, testable, local change and leaves A's
  remainder as honest, separately-scoped work.

**Cascade.** REQ-012 can close once C lands and its tests are green (the criterion clause is then MECHANISED, if not
by the literal ordering the prose describes — the prose should be re-read at that point, not before). A3-R012-F2/F4/F5
close with the slice. A3-R012-F3 (ordering invariants unobserved), F6 (discarded bool), F9 (uid unvalidated), F10
(uid re-resolved not latched) and F12 (CloudService leak) stay OPEN and non-blocking; F10+F12 are the seed of the
follow-on lifecycle REQ. DEC-019 is NOT reverted — its trigger design stands; C adds a guard on the consuming side.

### Alignment probe
grep -n 'resolveConfigDir\|loadChecked\|isConnected' src/Cloud/GarminConnect.cpp | head   # expect a token re-check reachable from readFile/readdir after this slice

---

## DEC-021 — The REQ-017 lifecycle-binding mechanism: how Disconnect invalidates a LIVE session
- Status: accepted (B — account-epoch counter latched at open(), compared in-memory)
- Reversibility: cheap (a private static hash + two free functions + two latched members, confined to `GarminAccountEpoch.{h,cpp}` + `GarminConnect.{h,cpp}`; deleting it reverts cleanly to DEC-020's status quo)
- Decided / last-reviewed: 2026-08-03
- Serves: REQ-017 (all five clauses a–e); closes the residual DEC-020 explicitly accepted; constrained by DEC-019 (its fresh-instance choice stays in force — NOT reopened), DEC-002/DES-001/REQ-NF-Threads-001 (single off-GUI worker, no `terminate()`), DEC-014 (Garmin owns a file token store), and the standing test constraint that GarminConnect cannot link into the Python-free wizard harness
- Dependents: REQ-017 build slices A + B, TEST-060..064, DES-002/DES-014 (add the epoch to the storage/lifecycle prose), A3-R012-F10 (uid latch) + F12 (CloudService leak) which close with this REQ, DEC-020 (its guard STAYS — the epoch is an independent second gate, not a replacement)
- Origin: A3-R012-F1 residual; research draft by qgdw-scout 2026-08-03 (three real options; orchestrator independently verified the central code claims — see below)

### Key research findings (scout, orchestrator-verified on disk)
- `blockingDownload()` (GarminConnect.cpp:231) runs a **nested `QEventLoop`** (:239, 60s watchdog `kDownloadTimeoutMs` at :260). The GUI event queue therefore keeps pumping during a download — which is precisely how a Disconnect click reaches us mid-flight, and why teardown must be QUEUED, never same-frame (deleting `m_client` under a live `blockingDownload()` frame is a UAF).
- **There is no cancel primitive anywhere**: `cancel|abort|interrupt|requestInterruption` matches NOTHING in `GarminWorker.{h,cpp}`, `IGarminDownloadClient.h`, `PyEmbeddedAdapter.h`. The download is a blocking Python call under the GIL and DES-001 invariant 3 forbids `terminate()`. True mid-flight cancellation is unreachable without new work — see the REQ-017(c) narrowing below.
- The queued-post idiom already exists and is documented: `m_completionContext` (GarminConnect.h:200, rationale at .cpp:545-580, the A3-R007-01 UAF guard). Any teardown reuses it rather than inventing one.
- Two ctors (:91, :93) + dtor (:100); `disconnectService()` :375; `accountStillConnected()` :165; `resolveGarminUserId()` defined :147 and re-resolved per call at :467 (readdir) and :521 (recordImport) — the F10 defect, fixed by the same open()-latch as the epoch.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A registry of live instances + queued self-close broadcast | 4 — self-contained, but the queued-close discipline must be exactly right | 4 — per-config-dir keying, O(1) | 4 — all state in GarminConnect.{h,cpp}; register/unregister across 2 ctors + dtor | 4 — standard observer/registry, more machinery than B for the same guarantee |
| B account-epoch latched at open(), in-memory compare (chosen) | 4 — fewest moving parts; `bump()` never reaches into another instance's state, so there is NO delete-out-from-under hazard | 5 — one static hash of ints, per config dir | 5 — lowest cognitive load; one hash, two functions, two latched members | 5 — the generation/epoch token is the canonical idiom for exactly this stale-handle bug class, and matches REQ-017's own framing ("by binding, not a per-call disk re-check") |
| C route Disconnect through the live instance (Context tracks it) | 3 — most "correct" ownership, but widest shared touch; ~15 siblings' open/close bookkeeping was never designed for tracking | 4 — per-Context map, fine for a desktop app | 2 — touches Context/MainWindow/AddCloudWizard/AthletePages/CloudService.h and reopens DEC-019 | 3 — RAII-correct in the abstract, but reopens an accepted decision AND is untestable under the Python-free harness (WizardStubPreamble.h stubs Context/MainWindow) |

### Chosen
B. It satisfies clauses (a), (c), (d) most cheaply, stays entirely inside `src/Cloud/` with **zero blast radius on the ~15 sibling services and no `CloudService.h` change**, leaves DEC-019 intact, and is fully testable on `garmin-fast`. Decisively: its clause-(a) test is the strongest available — leave `tokens.json` **valid and untouched**, bump the epoch alone, and assert the live instance issues zero list/download calls. That proves the binding without any reference to DEC-020's guard, so the test cannot silently pass for the wrong reason.

**The one gap, accepted with a compensating control.** B's invalidation is LAZY: `bump()` does not reach into the other live instance, so an *idle* sync dialog keeps its worker thread + embedded-Python session alive until its next call (which now fails fast, pre-network) or until its owner closes it. Clause (b) is therefore read FUNCTIONALLY — "never outlives the owning window", not "torn down at the instant of disconnect". To make that reading honest, REQ-017(e)'s `CloudServiceSyncDialog` dtor + `MainWindow::syncCloud` leak fix is **load-bearing, not parallel**: it is the guaranteed teardown trigger. Option A remains the documented, non-strawman fallback if the literal instant-of-disconnect reading is ever required.

### REQ-017(c) narrowed at decision time (user-approved 2026-08-03)
Clause (c) originally read "cancelled **or** its result discarded". Cancellation is unreachable (see findings). The clause is narrowed to **discard-only**: a download whose result lands after the disconnect is neither staged nor recorded, implemented as a **post-download, pre-stage recheck** of the epoch immediately before `recordImport`/`postReadComplete` — not only at `readFile`/`readdir` entry. **Accepted residual, deliberately visible:** the in-flight HTTP call still runs to completion (or the 60s watchdog); we discard its result rather than stopping it. An interruptible download path in `GarminWorker`/`PyEmbeddedAdapter` is separate, larger work and is NOT in REQ-017 — if it is ever wanted it needs its own REQ + DEC. Do not let a later cycle quietly re-read (c) as though cancellation were implemented.

### Cascade impact (chosen = B)
- NEW `src/Cloud/GarminAccountEpoch.{h,cpp}` — pure Qt, **no `Python.h`** (same seam discipline as `GarminDownloadChain.h`); `static QHash<QString,quint64> s_epoch` + `current(dir)` / `bump(dir)`. Must be added to `src/CMakeLists.txt` GC_WANT_GARMINCONNECT **and** the unittest target (LSN-018 — a missed main-binary link has bitten this project before).
- `GarminConnect.{h,cpp}`: latch `m_openedEpoch` + `m_openedUserId` in `open()`; gate `readFile`/`readdir` on the epoch compare **in addition to** DEC-020's `accountStillConnected()` (the guard is NOT removed — defence in depth, and REQ-017(a) is proven by neutralising it); `disconnectService()` calls `GarminAccountEpoch::bump()` alongside `clearAccount()`; `readdir`/`readFile`/`recordImport` consume `m_openedUserId` instead of re-resolving (F10).
- `CloudService.h`/`.cpp` + `src/Gui/MainWindow.cpp` + `AddCloudWizard.cpp`: clause (e) — a real `~CloudServiceSyncDialog` that `close()`s and deletes its store, and the `syncCloud` leak. **This is the only part that leaves `src/Cloud/`**, and both `MainWindow.cpp` and `src/CMakeLists.txt` are ALREADY dirty with unrelated pre-session edits ⇒ hunk-split at commit (LSN-010/007).
- Tests TEST-060..064 (see traceability). A3-R017's FIRST probe is mandated: neutralise `accountStillConnected()` and confirm the epoch alone still stops the exploit.
- DEC-020 is NOT superseded — its guard remains as the second layer.

### Alignment probe
grep -rn 'GarminAccountEpoch\|m_openedEpoch\|m_openedUserId' src/Cloud/ | wc -l   # expect >0 after slice A; 0 means the bind was never wired
grep -n 'GarminAccountEpoch.cpp' src/CMakeLists.txt unittests/Core/garminconnect/CMakeLists.txt   # expect BOTH (LSN-018)

---

## DEC-022 — How a refused (fail-closed) readFile reports itself to the sync dialog
- Status: **PARTIALLY IMPLEMENTED 2026-08-04 — GarminConnect half accepted + green (TEST-065); the SHARED half is BLOCKED and un-decided, because this entry's original safety premise was FALSE (see the correction below). Re-decide before touching `completedRead`.**
- Reversibility: cheap (two small edits; the shared change is confined to the existing failure branch of one slot)
- Decided / last-reviewed: 2026-08-03
- Serves: REQ-017(a) ("fails with a Garmin-labelled error"), B-R017-06 (blocking); also retro-fixes REQ-012/DEC-020's shipped guard
- Dependents: TEST-065/066, `GarminConnect::readFile` both fail-closed paths, `CloudServiceSyncDialog::completedRead`, B-R017-07 (uploadCloud leak, folded in)
- Origin: B-R017-06, surfaced by the Slice-B builder's report-only ErrorBus feasibility read and CONFIRMED on disk by the orchestrator

**Problem.** `syncNext` (CloudService.cpp:1413) and `downloadNext` (:1495) discard `readFile`'s bool and wait for a `readComplete` signal to advance. `GarminConnect::readFile`'s fail-closed paths (`sessionSuperseded()` :452, `accountStillConnected()` :460) return false and post NO completion ⇒ the dialog hangs at "Downloading n of N" and the `new QByteArray` at :1412/:1494 leaks. Already shipped via DEC-020's identically-shaped guard in `f001c7d20`.

**THE TRAP that constrains every option (verified).** `completedRead` is declared `(QByteArray*, QString, QString /*message*/)` — the parameter is literally unnamed/discarded at CloudService.cpp:1527. But **a non-empty `message` does NOT mean failure today**: `GarminConnect::postReadComplete` already passes `tr("Completed.")` on SUCCESS (GarminConnect.cpp:686-691). So no option may infer "error" from "message is non-empty" — that would relabel every successful Garmin download as an error.

### CORRECTION 2026-08-04 — the premise that made Option A's shared half "safe" was FALSE
This entry (and the build briefing derived from it) asserted that the ~15 other CloudService subclasses pass an
EMPTY message, so surfacing `message` on the `ride == NULL` branch would leave them untouched. **That is wrong, and
the orchestrator confirmed it on disk.** EVERY sibling passes `tr("Completed.")` on success —
`Strava.cpp:530`, `Dropbox.cpp:321`, `SportTracks.cpp:539`, `Xert.cpp:478`, `Azum.cpp:297`, `PolarFlow.cpp:219`,
`CyclingAnalytics.cpp:433`, `SixCycle.cpp:480`, `Nolio.cpp:247`, `LocalFileStore.cpp:150`. So
`if (!message.isEmpty()) show(message)` on the failure branch would render EVERY OTHER SERVICE'S PARSE FAILURE as
**"Completed."** — the trap above, inverted: failure relabelled as success, across ~10 shipped integrations.
The builder built the seam, RED-verified it, then REMOVED it and fired the stop-and-report hatch rather than ship a
cross-service regression against a briefing it could see was wrong. That was the correct call.
**Consequence:** the GarminConnect half (post a labelled completion, still return false) is INDEPENDENTLY correct and
shipped — it closes the blocking half of B-R017-06 (loop advances, buffer freed) with no regression for anyone. Only
the *display* of the reason is unresolved: a Garmin refusal currently shows `uncompressRide`'s text instead of the
reason. Strictly better than the hang. The shared half needs a fresh decision — candidate discriminators, none
chosen: (1) branch on `data->isEmpty()` (safe except for a genuine 0-byte download from another service, which would
then read "Completed."); (2) a canonical success-label accessor on `CloudService` so the shared side can ignore it
(~15 files, and cross-`tr()`-context string comparison is locale-fragile); (3) carry failure EXPLICITLY — an extra
signal argument or a `readFailed` signal (bigger blast radius, but the only one that is not a heuristic).
**Lesson [[LSN-034]]** — an orchestrator's "this is what makes it safe" premise is a claim about the tree and must be
grepped before it is written into a DEC, not after.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — post a labelled completion; surface `message` ONLY on the existing failure branch.** GarminConnect's refusal paths call `postReadComplete(data, name, tr("Garmin Connect: …"))` and still `return false`. In `completedRead`, the sole shared change is in the branch that already runs when `ride == NULL`: show `message` when it is non-empty, else fall back to today's `errors.join(" ")`. Success path (`ride != NULL`) is untouched, so `tr("Completed.")` can never be mistaken for an error, and the ~15 other services (which pass an empty message) keep their exact current behaviour. R4 S4 M5 BP5.
- **B — new out-of-band error channel (ErrorBus).** Rejected: `ErrorBus` DOES NOT EXIST in the tree — only DES-008 prose and a TODO at GarminConnect.cpp:649. It would be a new subsystem, and being a side channel it would label the error while leaving the loop still stalled. R2 S3 M2 BP3.
- **C — make the callers honour `readFile`'s bool.** Rejected for now: `syncNext`/`downloadNext` would need to advance the loop themselves on a false return, changing control flow for all ~15 services in a function whose completion-driven design is load-bearing. Larger blast radius for the same user-visible outcome. R3 S4 M2 BP3.

**Cascade.** `GarminConnect::readFile` both fail-closed paths post a completion before returning false (the buffer is then freed by `completedRead`'s existing `delete data`, killing the leak). `CloudServiceSyncDialog::completedRead` names its `message` param and uses it in the `ride == NULL` branch. B-R017-07 (`MainWindow::uploadCloud`'s identical `db` leak) is folded in, reusing Slice B's `closeAndDeleteStore()`. REQ-017(a) then holds on BOTH `readdir` and `readFile`, so B-R017-01/06 close together. DEC-020's guard is unchanged in behaviour — it merely reports properly now.

### Alignment probe
grep -n 'QString /\*message\*/' src/Cloud/CloudService.cpp   # expect ZERO after this slice (the param must be named + used)
grep -n 'postReadComplete' src/Cloud/GarminConnect.cpp        # expect a call on BOTH fail-closed paths, not just the success path

---

## DEC-023 — Carry read FAILURE explicitly instead of inferring it from the message
- Status: accepted (option 3 of DEC-022's correction — an explicit failure channel)
- Reversibility: moderate (a new signal is additive and the ~15 siblings never emit it; unwinding it later means re-auditing whoever came to depend on it)
- Decided / last-reviewed: 2026-08-04
- Serves: REQ-017(a) (the "Garmin-labelled error" half DEC-022 could not deliver), B-R017-06 (display half), B-R017-10 (the ~4 other silent `return false` sites); supersedes DEC-022's shared half
- Dependents: TEST-068/069, `GarminConnect::readFile` (all non-posting return sites), `CloudService` signal surface, `CloudServiceSyncDialog`, `CloudServiceAutoDownload`
- Origin: DEC-022's false premise (see its 2026-08-04 correction) — every sibling passes `tr("Completed.")`, so success and failure are INDISTINGUISHABLE on the existing `message` channel

**Why explicit beats every heuristic.** Both cheaper candidates infer failure from a proxy: "message is non-empty" is wrong because all ~10 shipped siblings pass `tr("Completed.")` on success; "payload is empty" is wrong for a genuine 0-byte download, which would then render as its success label. A dedicated failure channel needs no proxy, and it is the only option that also covers B-R017-10's remaining silent paths (`:490`, `:501` RateLimit, `:512`, `:520`, `:527` TCX-failed) with ONE mechanism instead of five special cases. Cost is a wider surface — hence its own DEC rather than a build-slice improvisation.

**Cascade.** `CloudService` gains a `readFailed(QByteArray* data, QString name, QString reason)` signal alongside `readComplete`. The ~15 siblings never emit it and are otherwise untouched — that is what keeps the blast radius honest (contrast DEC-022's shared edit, which would have changed behaviour for all of them). `GarminConnect::readFile`'s non-posting `return false` sites emit it with a reason. Both consumers connect it: the sync dialog shows the reason, deletes the buffer, and advances the loop (the same three things `completedRead` does); `CloudServiceAutoDownload` does the equivalent. TEST-065's GarminConnect-half assertions stay valid — the refusal paths keep posting; DEC-023 only changes WHICH signal they post on. B-R017-11 (readFile vs readdir wording drift) is aligned in the same slice.

### Alignment probe
grep -rn 'readFailed' src/Cloud/CloudService.h src/Cloud/GarminConnect.cpp   # expect the signal + emits on every silent return-false site
grep -c 'notifyReadComplete' src/Cloud/Strava.cpp src/Cloud/Dropbox.cpp      # expect UNCHANGED — siblings must not be touched

---

## DEC-024 — Making the modeless sync dialog's self-deletion safe (A3-R017-F1)
- Status: accepted (A — `closeEvent()` guard + a blocking-call-in-flight flag, shipped with an ASan-backed test)
- Reversibility: cheap (a flag, an override, and a deferred re-close; confined to `CloudServiceSyncDialog`)
- Decided / last-reviewed: 2026-08-05
- Serves: REQ-017(e) on the wizard path, and therefore REQ-017(b) — DEC-021 leans on this teardown as its compensating control; triggered by A3-R017-F1 (BLOCKING, ASan-reproduced)
- Dependents: TEST-070/071, `CloudServiceSyncDialog` (ctor `open()`, `refreshClicked` readdir :1019, `syncNext` readFile :1417, `downloadNext` readFile :1499, `cancelClicked` :980), AddCloudWizard.cpp:899

**Problem.** REQ-017 Slice B added `setAttribute(Qt::WA_DeleteOnClose)` to the MODELESS post-connect sync dialog (AddCloudWizard.cpp:899) so it would stop leaking its store. But `GarminConnect::readFile`/`readdir` run nested `QEventLoop`s, so GUI events are processed mid-call; closing the dialog then destroys it → `~CloudServiceSyncDialog` → `closeAndDeleteStore(store)` → the GarminConnect is deleted **while `readFile` is still executing on it**, and execution resumes after the nested loop on freed memory. ASan-reproduced with two independent harnesses. `cancelClicked()` (CloudService.cpp:980) `reject()`s unconditionally — it does not consult `downloading` — so the ordinary Cancel button reaches this too. **We converted a leak into a crash**, which is strictly worse; the fix must not simply re-introduce the leak.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — `closeEvent()` guard + deferred self-close.** A `m_blockingCallActive` flag is set around every dialog→store call that can run a nested loop (ctor `open()`, `refreshClicked`'s readdir :1019, `syncNext` :1417, `downloadNext` :1499). `closeEvent()` overrides: if the flag is set, record `m_closeDeferred`, `e->ignore()`, and **also set the existing `aborted` flag (CloudService.h:427) so the sync stops promptly** rather than leaving the user's click apparently ignored; when the last blocking frame unwinds, if `m_closeDeferred` then `close()` for real. `cancelClicked()` routes through the same guard instead of an unconditional `reject()`. R4 S4 M4 BP4. Keeps clause (e) fully satisfied on the wizard path — no leak, no crash — at the cost of a genuine ASan-backed test, which is exactly the discipline A3-R017's lesson demands.
- **B — Drop `WA_DeleteOnClose`, explicit owner (stack + `exec()`).** Mirrors the pattern `MainWindow::syncCloud` already proves safe; structurally eliminates the hazard rather than guarding it. R5 S4 M5 BP4. Rejected only because it converts the wizard's post-connect sync from modeless to MODAL — a real UX change to the feature's most common first-use path. Retained as the explicit fallback if A's guard proves fragile under A4.
- **C — Revert to the leak for this call site.** R2 S3 M5 BP2. Rejected: REQ-017(e) would go unmet on the one path DEC-021 explicitly called load-bearing, so clause (b)'s functional reading would lose its guaranteed teardown trigger — we would be back to a session outliving its window, which is the whole point of REQ-017.

**Cascade.** `CloudService.h`: two bools + a `closeEvent` override on `CloudServiceSyncDialog`. `CloudService.cpp`: bracket the four call sites, rewrite `cancelClicked`, add the deferred re-close. `AddCloudWizard.cpp:899` is UNCHANGED — the attribute stays. `MainWindow::syncCloud`/`uploadCloud` are unaffected (stack-allocated, never set the attribute; it is commented out at CloudService.cpp:436 for this exact reason). A3-R017-F3 (`completedRead`'s abort branch leaking `data`, CloudService.cpp:1537-1541) is folded in — same function family, one line.

**Evidence bar (non-negotiable).** The fix ships with a test that actually EXECUTES the close-during-nested-loop sequence, not a test that reasons about it. A3-R017's lesson is that a Qt lifetime claim ("DeferredDelete is throttled to the posting loop level, so this is safe") sounded correct, was written into a design comment, and was false in practice. Run it under AddressSanitizer.

### Post-build correction (2026-08-05) — RATIFIED: the gate must be on `done(int)`, not `closeEvent()`
**This entry as originally written was insufficient, and the builder proved it rather than following it.** DEC-024
step 3 prescribed a `closeEvent()` override. Measured on Qt 6.8.2: `reject()` and the Escape key destroy a
`WA_DeleteOnClose` dialog **even when `closeEvent()` calls `e->ignore()`**, because `QDialog::closeEvent` internally
routes to `reject()` → `done()`. The real choke point that ALL of `close()`/`reject()`/`accept()`/Escape funnel
through is **`QDialog::done(int)`**, so the guard is overridden there.

Mutation-verified, by the builder and independently re-run by the orchestrator:
- **M1, remove the `done()` gate → ASan `heap-use-after-free`.** The gate is load-bearing. (Orchestrator re-ran this
  itself: snapshot → mutate → ASan UAF reproduced → restore, md5 `0cd1dfb3…`, 26/26 green.)
- **M2, remove the `closeEvent()` gate → still PASSES.** The `closeEvent()` override is therefore **NOT
  independently load-bearing** — it is belt-and-braces, kept so the fix does not silently depend on that Qt internal
  ever staying true. Recorded honestly so a later reader does not mistake it for the mechanism.

Also ratified, all beyond what this entry specified:
- **Depth COUNTER, not a bool** (`blockingCallDepth`): `QApplication::processEvents()` inside a blocking call can
  dispatch a Refresh click, nesting a `readdir` frame inside a `readFile` frame — a bool would be cleared by the
  inner frame while the outer was still live.
- **The ctor guard is WIDER than specified** — the whole ctor body, not just `store->open()`, because the ctor also
  reaches `refreshClicked()` → `readdir`, and `delete this` must not happen while the ctor is on the stack at all.
- **`cancelClicked()`** rewritten off its unconditional `reject()`.
- **A3-R017-F3 folded in**: `completedRead`'s abort branch now `delete data` like its `failedRead` sibling.

**Accepted residuals (recorded, not fixed):** `writeFile` is deliberately unguarded at CloudService.cpp:1435/:1656 —
no service in this tree runs a nested loop there (all go async via `QNetworkReply`), but **if one ever blocks in
`writeFile`, the UAF returns on the upload path**. Direct destruction (`delete dialog`, or teardown via the parent
`MainWindow`) still bypasses the gate entirely — nothing routes through `close`/`done` — which is pre-existing and
by construction. The ASan target sets `detect_leaks=0` (LSan drowns in Qt/qwt startup allocations), so
whole-process leaks are not covered by it.

**Hang risk, deliberately tested:** gating `done()` is dangerous in the modal path — `MainWindow::syncCloud` uses
`exec()`, which waits on `done()`, so a gate that failed to reopen would HANG File > Sync rather than crash it.
TEST-071 drives exactly that shape with a 5-second watchdog and, probed with the guard wedged, reports
`exec() never returned`.

### Alignment probe
grep -n 'void done' src/Cloud/CloudService.h                               # expect the done(int) override — the actual gate
grep -n 'closeEvent' src/Cloud/CloudService.h src/Cloud/CloudService.cpp   # present, but belt-and-braces (see M2)
grep -n 'WA_DeleteOnClose' src/Cloud/AddCloudWizard.cpp                    # expect it STILL PRESENT (option A keeps it)

---

## DEC-025 — Surviving PARENT TEARDOWN: guarding the unsafe operation, not the close (A3-R017b-F1/F4)
- Status: accepted (A — the destructor declines to delete the store while a blocking call is in flight, plus self-death detection on the frames that resume)
- Reversibility: cheap (a depth check in one destructor + a `QPointer` in one RAII class; confined to `CloudServiceSyncDialog`)
- Decided / last-reviewed: 2026-08-05
- Serves: REQ-017(e) on the parent-teardown path; triggered by A3-R017b-F1 (BLOCKING, Qt-semantics proven by isolated repro) with A3-R017b-F4 as the root cause
- Dependents: TEST-072/073/074, `~CloudServiceSyncDialog` (CloudService.cpp:981-987), `BlockingCall` (:1003-1019), `deferCloseIfBusy` (:1030-1038), `syncNext` (:1518-1521), `downloadNext` (:1605-1608), `refreshClicked` (:1113), `closeAndDeleteStore` (CloudService.h:279-289); context: AddCloudWizard.cpp:899, MainWindow.cpp:143

**Problem.** DEC-024's gate sits on close-INITIATION — `done(int)`, with `closeEvent()` as belt-and-braces. Qt destroys child widgets **directly** when the parent is destroyed: no `closeEvent`, no `done`, no virtual dispatch, and a destructor cannot be vetoed the way a virtual can. The dialog is parented to `context->mainWindow` (CloudService.cpp:709) and **`MainWindow` itself carries `WA_DeleteOnClose`** (MainWindow.cpp:143), so closing the athlete's main window mid-sync runs `~CloudServiceSyncDialog` → `closeAndDeleteStore(store)` while a nested `QEventLoop` is on the stack — freeing the GarminConnect that `readFile` is still executing on. Exposure is up to 60s per call (GarminConnect.cpp:41-43). Pre-Slice-B there was no destructor, so this path LEAKED; **this changeset converted it into a crash**, exactly as it did for the self-close routes. All ~16 CloudService subclasses run nested loops in `open()`/`readdir()`, so the route is not Garmin-specific.

**The second half of the hazard, found while drafting this entry (O-R025-01) — declining to delete the store is NECESSARY BUT NOT SUFFICIENT.** Parent teardown frees the *dialog* too, while the dialog's own member functions are suspended in that nested loop. Verified on disk, not reasoned about:
- `BlockingCall::~BlockingCall` writes `dialog->blockingCallDepth` (CloudService.cpp:1011) and may call `dialog->close()` (:1018) — the first thing that runs when the loop unwinds, straight into freed memory.
- `downloadNext` then resumes at `QApplication::processEvents()` (:1608) and `syncNext` at (:1521); both continue walking `rideListDown`/`progressLabel`/`listindex` — all members of a destroyed object.

So a store-only guard leaves a use-after-free on the *identical trigger*, and the ASan test this decision mandates would still trip. The guard must cover both objects the teardown frees.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — guard the unsafe operation itself, in three parts.** (1) `~CloudServiceSyncDialog`: when `blockingCallDepth > 0`, keep `store->disconnect(this)` but **skip `closeAndDeleteStore(store)`** — deliberately leak the store, which is precisely the pre-REQ-017 behaviour on this path and strictly better than a UAF. (2) `BlockingCall` holds a `QPointer<CloudServiceSyncDialog>`; its destructor no-ops when the dialog is already gone. (3) `syncNext`/`downloadNext`/`refreshClicked` take a `QPointer` self before each blocking call and return immediately if it is null afterwards, touching no members. R4 S4 M4 BP4. Addresses A3-R017b-F4's root cause — the check moves onto the operation that is actually unsafe, so any future direct-destruction path (`delete dialog` tomorrow) is covered by construction, not by having enumerated the routes.
- **B — Reparent the dialog to `nullptr`.** A top-level, parentless dialog is never destroyed by MainWindow, so the route disappears rather than being guarded. R4 S3 M4 BP3. **Rejected:** it orphans the window — the sync dialog survives the athlete window that spawned it, and with `WA_DeleteOnClose` firing only on user close, the store is torn down only if the user happens to close the dialog. That forfeits REQ-017(b)'s functional reading ("never outlives the owning window"), which is the clause Slice B exists to satisfy.
- **C — MainWindow event filter / busy-veto deferring teardown while a child reports busy.** R2 S2 M2 BP2. **Rejected on three counts:** parent destruction is not an event, so there is nothing to filter at the moment that matters; it puts CloudService lifecycle knowledge into MainWindow, inverting the ownership direction the whole REQ is establishing; and it would block application quit for up to 60 seconds.

**Cascade.** `CloudService.h`: a `QPointer` member on `BlockingCall` (already non-copyable, :475-484). `CloudService.cpp`: the destructor's depth check, the `BlockingCall` destructor's null check, and the three resuming call sites. `closeAndDeleteStore` (CloudService.h:279-289) is UNCHANGED — the decision is *whether* to call it, not what it does, so TEST-064's contract stands. `AddCloudWizard.cpp:899` keeps `WA_DeleteOnClose`; `MainWindow.cpp:143` is not touched. The ~15 sibling services are untouched and inherit the fix, since the guard lives in the shared dialog. **A3-R017b-F2 folded in if cheap:** wrap the two `writeFile` sites (CloudService.cpp:1538 in `syncNext`, :1769 in `uploadNext`) in `BlockingCall` for symmetry — no service blocks there today (adversary verified across all 16), so this is pre-emptive consistency, not a fix, and it must be dropped rather than forced if it complicates the slice.

**Evidence bar (non-negotiable, and the reason this is not a one-line change).** The fix ships with a test that EXECUTES parent teardown mid-nested-loop — construct a parent, construct the dialog as its child, enter a blocking call, `delete parent` from inside that call, let the loop unwind — under AddressSanitizer, on the existing ASan target. Both directions proven: restoring the unconditional `closeAndDeleteStore` in the destructor must reproduce the ASan `heap-use-after-free`. DEC-024's lesson is that a Qt lifetime claim which *sounded* correct was false in practice; the same standard applies here to the QPointer half.

**Accepted residuals (recorded, not fixed).** The store LEAKS on the parent-teardown path — deliberate, user-chosen, and the pre-REQ-017 status quo; for GarminConnect that means the worker thread and its embedded-interpreter session live to process exit, but only for a sync that was in flight when the main window closed. The ASan target keeps `detect_leaks=0` (LSan drowns in Qt/qwt startup allocations), so that leak is not observable by the test either way. A deferred reaper that deletes the store once the call unwinds was considered and NOT taken: at parent-teardown time the application is already shutting down, so a reaper would be running against a dying event loop.

### Alignment probe
grep -n 'blockingCallDepth' src/Cloud/CloudService.cpp                     # expect a check in ~CloudServiceSyncDialog, not only in deferCloseIfBusy
grep -n 'QPointer' src/Cloud/CloudService.h src/Cloud/CloudService.cpp     # expect it on BlockingCall + the three resuming call sites
grep -n 'closeAndDeleteStore' src/Cloud/CloudService.h src/Cloud/CloudService.cpp  # expect the helper UNCHANGED, the CALL conditional

## DEC-026 — Two-phase init: no nested event loop under construction (closes the BLOCKING A3-R025-F1)
- Status: accepted (B — the constructor builds the widget shell only; the blocking work (`store->open()` + the initial `refreshClicked()`) moves to an explicit `start()` the caller invokes on a fully-constructed object)
- Reversibility: moderate (re-shapes one ctor + a new slot + two call sites; confined to `CloudServiceSyncDialog` and its two constructors of record)
- Decided / last-reviewed: 2026-08-06
- Serves: REQ-017(e) on the constructor-teardown path; triggered by A3-R025-F1 (BLOCKING) which escalated B-R025-01; completes the class DEC-024/DEC-025 left open (A3-R017b-F4)
- Dependents: TEST-075 (ctor route eliminated, ASan, both directions), TEST-076 (RED tests making the syncNext/downloadNext self-guards load-bearing — A3-R025-F2/B-R025-02), `CloudServiceSyncDialog::CloudServiceSyncDialog` (CloudService.cpp:709-974), new `CloudServiceSyncDialog::start()`, callers `MainWindow::syncCloud` (MainWindow.cpp:2577-2578), `AddCloudWizard` (AddCloudWizard.cpp:892-899); context: DEC-024, DEC-025

**Problem.** DEC-024 gated close-initiation; DEC-025 guarded the destructor + the resuming member frames for parent teardown. A3-R025-F1 (orchestrator-confirmed at CloudService.cpp:719/725/734/972; general Qt mechanism proven by the adversary's standalone ASan repro) showed a FOURTH live route to the identical use-after-free that neither closes: the **constructor itself**. `CloudServiceSyncDialog::CloudServiceSyncDialog` wraps its whole body in one depth-only `BlockingCall` and runs three blocking / nested-loop calls with no `QPointer` self-bail — `store->open()` (:725, `blockingRestore` 30s), `QMessageBox::exec()` (:734, UNBOUNDED user wait), the tail `refreshClicked()` (:972, readdir loop) — then resumes writing `this` members (`tabs = new QTabWidget(this)` :745…). A parent teardown (MainWindow `WA_DeleteOnClose`, MainWindow.cpp:143) landing in any of those frees the half-built dialog under its own ctor; reachable via the heap/modeless `AddCloudWizard.cpp:892`. DEC-025's three guards cannot help — there is no fully-formed object to stand down. This is the THIRD cycle of the same enumeration-miss family (DEC-024 → DEC-025 → here), recorded as a miss on [[LSN-037]], whose rule now names the constructor explicitly.

**Options considered (scored R/S/M/BP 1-5).**
- **A — Constructor self-bail.** `QPointer<CloudServiceSyncDialog> self(this)` + early-return after each of the three ctor blocking sites, touching no members if null. R2 S3 M3 BP2. **Rejected:** does NOT end the recurrence (same per-site enumeration that has missed a frame three times), and carries a CALLER-SIDE residual — after the ctor bails, `new` returns a dangling pointer and `AddCloudWizard.cpp:899` still calls `->open()` on freed memory, so A only fully closes the route if the caller is also guarded. Guards an anti-pattern (nested event loops in a constructor) rather than removing it.
- **B — CHOSEN — two-phase init.** The constructor builds only what does not depend on `open()` (title, size, member refs) and runs NO nested loop; a new `bool start()` slot does `store->open()`, the tab construction, and the initial `refreshClicked()`, returning false on open-failure. Both callers invoke `start()` on the fully-constructed object: `MainWindow::syncCloud` → `if (sync.start()) sync.exec();`; `AddCloudWizard` → `d->setAttribute(WA_DeleteOnClose); if (d->start()) d->open(); else delete d;` (open-failure cleanup preserved — today the ctor's failure path hides + queues close). R4 S4 M4 BP4. Eliminates the class structurally: no nested loop ever runs on the stack under construction, so there is no half-built frame to free; `start()` runs on a complete object where DEC-024/025's guards already apply, and as a resuming frame it takes the DEC-025 part-3 `QPointer` self-bail discipline. No caller-side residual. Blast radius is the TWO construction sites of record, not the ~15 sibling services (which inherit the shared dialog and never construct it).
- **C — Reparent the dialog to `nullptr`.** R3 S2 M2 BP2. **Rejected — same objection as DEC-025 option B:** a top-level parentless dialog is never destroyed by MainWindow, but it orphans the window and can outlive the athlete window that spawned it, forfeiting REQ-017(b)'s functional reading ("never outlives the owning window").

**Cascade.** `CloudService.h`: `CloudServiceSyncDialog` gains a public `bool start()`; the ctor's declared responsibility shrinks to shell-only. `CloudService.cpp`: split the ctor at the `store->open()` boundary into ctor + `start()`; `start()` carries the DEC-025 part-3 self-bail after each blocking call it makes. `MainWindow.cpp:2577-2578` and `AddCloudWizard.cpp:892-899`: the two callers gain the `start()` invocation + open-failure handling. DEC-024's `done()`/`closeEvent()`/`deferCloseIfBusy`/`BlockingCall` and DEC-025's dtor/`QPointer` guards are UNCHANGED and continue to protect `start()`'s loop and every post-construction frame — this decision removes the one frame they could not reach. TEST-070..073 must still pass. **Rider (A3-R025-F2 disposition, user-chosen):** add TEST-076 — RED tests that make `syncNext`'s and `downloadNext`'s `QPointer` self-guards load-bearing (today both mutations survive; only `refreshClicked`'s is caught), so no untested guard stands in front of the hazard.

**Evidence bar (non-negotiable).** TEST-075 EXECUTES parent teardown during `start()`'s nested loop under AddressSanitizer on the existing `testGarminConnectSyncDialogClose` target, and proves BOTH directions: reverting the split — moving `store->open()`/`refreshClicked()` back into the constructor body — must reproduce the ASan `heap-use-after-free`. TEST-076's two slots must each FAIL when their guard is removed. Extend the existing ASan target; no new file unless the build forces it.

**Accepted residuals (recorded, not fixed).** The deliberate busy-teardown store leak from DEC-025 is unchanged. `start()`-based init changes the construction contract for these two callers only; any future third caller of `CloudServiceSyncDialog` must call `start()` — enforced by making the ctor produce a non-functional (un-opened) dialog until `start()` runs.

### Alignment probe
grep -n 'start()' src/Cloud/CloudService.h                                 # expect a new public bool start() on CloudServiceSyncDialog
grep -n 'store->open\|refreshClicked' src/Cloud/CloudService.cpp           # expect these OUT of the ctor body, inside start()
grep -n 'CloudServiceSyncDialog' src/Gui/MainWindow.cpp src/Cloud/AddCloudWizard.cpp  # expect both callers to invoke start() before exec()/open()

## DEC-027 — Unify the sync-dialog lifetime: heap + WA_DeleteOnClose for BOTH callers (closes the BLOCKING A3-R026-F1)
- Status: accepted (A — convert `MainWindow::syncCloud` from a STACK modal dialog to the heap + `WA_DeleteOnClose` + modeless `open()` pattern `AddCloudWizard` already uses; the shared `CloudServiceSyncDialog` is unchanged)
- Reversibility: moderate (one call site changes storage class + modality; the dialog class itself is untouched)
- Decided / last-reviewed: 2026-08-06
- Serves: REQ-017(e) on the stack-caller path; triggered by A3-R026-F1 (BLOCKING); completes the class DEC-024/025/026 left open (the 5th and final route)
- Dependents: TEST-077 (A3-R026-F2 fixture-gap coverage — the 4 previously-unreachable `start()` self-bails), TEST-078 (syncCloud heap-conversion teardown, conditional), `MainWindow::syncCloud` (MainWindow.cpp:2571-2585); context: AddCloudWizard.cpp:892-910 (the reference pattern), DEC-024/025/026

**Problem.** A3-R026-F1 (orchestrator-confirmed self-parenting chain: MainWindow.cpp:143 `WA_DeleteOnClose` + `new Context(this)` :162/2038 + `sync` stack dialog parented to `context->mainWindow` :2583). `MainWindow::syncCloud` builds `CloudServiceSyncDialog` in AUTOMATIC storage parented to the very MainWindow whose teardown is the whole threat scenario. Qt's `QObjectPrivate::deleteChildren()` deletes every child UNCONDITIONALLY of the child's attribute — so parent teardown mid-sync (reachable during `start()`'s non-modal nested loops) calls `delete` on a STACK object (bad-free) then the scope double-destructs it. NO internal `self.isNull()` guard reaches this — the suspended frame is ON the freed object. This is the storage-class axis of the same family DEC-024/025/026 addressed; the prior [[A3-R017-F1]] disposition wrongly declared syncCloud "UNAFFECTED (no WA_DeleteOnClose)" — that attribute governs the CHILD's own close, not parent-initiated deletion (LSN-040).

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — heap + `WA_DeleteOnClose` + modeless `open()`.** Make syncCloud construct exactly as `AddCloudWizard` does: `new`, `setAttribute(WA_DeleteOnClose)`, `if (sync->start()) sync->open();` with NO `else delete` (open-failure posts a queued `close()` that self-deletes; an else-delete would double-free). R4 S4 M4 BP4. The dialog becomes a VALID heap child: `deleteChildren` frees it validly, DEC-025's dtor declines the store while `blockingCallDepth>0`, and `start()`'s `self.isNull()` sentinel fires (the dialog IS a child → it gets deleted → self goes null) BEFORE any frame touches the freed `context` — the exact geometry TEST-075 already proves. Unifies both callers on ONE lifetime contract. **Cost (deliberate, user-approved):** menu-triggered sync becomes MODELESS — it no longer blocks the main window — matching `AddCloudWizard`'s already-modeless sync. db ownership is preserved: the dialog's dtor still closes+deletes db (REQ-017(e)), now triggered by `WA_DeleteOnClose` deletion instead of stack-scope exit.
- **C — keep MODAL: heap + `WA_DeleteOnClose` + `QPointer`-guarded `exec()`.** R3 S3 M3 BP3. **Rejected:** the modal `exec()` frame is itself suspended ON the dialog, so it needs a self-check after `exec()` returns PLUS the dtor guard PLUS a new ASan test for the exec-suspended-frame teardown — more lifetime code and a second lifetime pattern, to dodge a hazard A removes structurally. Chosen only if modality were a hard requirement; it is not.
- **B — reparent the stack dialog to `nullptr`.** R2 S2 M2 BP2. **Rejected:** fixes the stack bad-free but the dialog is then NOT a child, so `start()`'s `self.isNull()` sentinel never fires and it proceeds to touch the freed `context` (owned by MainWindow) — a context use-after-free. Trades one UAF for another.

**Cascade.** `MainWindow::syncCloud` (MainWindow.cpp:2571-2585): stack `CloudServiceSyncDialog sync(...)` + `if (sync.start()) sync.exec();` → heap `new` + `setAttribute(WA_DeleteOnClose)` + `if (sync->start()) sync->open();`, no else-delete, db-ownership comment updated. `CloudServiceSyncDialog` itself is UNCHANGED — it already supports this pattern (AddCloudWizard uses it). DEC-024/025/026 machinery + TEST-070..075 unchanged/green. **Rider (A3-R026-F2, folded in):** extend the ASan fixture so the 4 currently-unreachable `start()` self-bails become testable — add an open-failure `BlockingStore::open()` mode (reaches :778/:782), a non-empty dirty `rideCache` (reaches :993), and a teardown armed during the readdir/`refreshClicked` frame (reaches :1023) — TEST-077, each guard RED-verified.

**Evidence bar.** TEST-077's four slots each FAIL when their guard is neutered (the F2 gap closed). The syncCloud conversion inherits TEST-075's proven parent-teardown geometry (both callers now identical); optionally TEST-078 drives a parent teardown through the syncCloud entry point specifically. Baseline: ASan target green, `ctest -R "Garmin|AtomicFile"` ≥25, full ≥26, `GoldenCheetah` links. DEC-024/025/026 byte-unchanged.

**Accepted residuals.** Menu-sync is now modeless (user-approved UX change). db lifetime is tied to `WA_DeleteOnClose` deletion, exactly as `AddCloudWizard`'s already is. A3-R026-F3 (no start()-called/re-entrancy invariant) stays a latent advisory — inert with the two known callers.

### Alignment probe
grep -n 'new CloudServiceSyncDialog\|WA_DeleteOnClose\|->start()\|->open()\|->exec()' src/Gui/MainWindow.cpp   # expect heap+WA_DeleteOnClose+open(), NO stack `sync`, NO exec()
grep -n 'CloudServiceSyncDialog sync' src/Gui/MainWindow.cpp                                                    # expect GONE (no stack construction)

## DEC-028 — Clean-checkout build repair: commit the missing wiring rather than guard around it (ORCH-001)
- Status: accepted (B — commit `unittests/Core/coach/CMakeLists.txt` + `stubs/` so committed CMake refers only to committed files)
- Reversibility: high (build-definition only; every part is a one-line revert, no product code changed)
- Decided / last-reviewed: 2026-08-07
- Serves: ORCH-001 (user dispositioned "fix it"); unblocks the mandated clean-worktree configure+build gate that has never once run in this project
- Dependents: commit `427da745b`; ORCH-005/006/007 (the three further causes the build gate exposed); the branch-merge gate for `garmin/req017-lifecycle-uaf`

**Problem.** ORCH-001 recorded that `master` does not configure from a clean checkout, from two causes. The decision point was cause 2: `unittests/CMakeLists.txt:1` unconditionally `add_subdirectory(Core/coach)` while `unittests/Core/coach/CMakeLists.txt` and its `stubs/` were never committed — even though `testCoachTools.cpp` and `coach.pro` WERE. The repair lands in files reserved for the pre-session Coach work's owner, so the scope was the user's call.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — commit the missing wiring** (`CMakeLists.txt` + 5 stub headers, 6 small files). R4 S4 M3 BP5. Root-cause fix: committed CMake then references only committed files, and the ALREADY-TRACKED `testCoachTools` actually builds. Every `src/Coach/*` source the wiring consumes (`GCToolExecutor.cpp`, `ToolConfirmCard.cpp`, `PlanPreviewCard.cpp`, `LLMService.h`) was already tracked, so nothing of the Coach feature itself is adopted. **Cost:** 6 files of the owner's WIP enter master, and the stubs may churn under their refactor.
- **A — guard `add_subdirectory` with an `EXISTS` check.** R4 S3 M4 BP2. **Rejected:** one line and zero ownership entanglement, but it leaves a TRACKED test permanently unbuilt from clean, silently — the precise "green but dead" class that produced REQ-018 and ORCH-001 itself. It masks the defect instead of fixing it, and would hide the NEXT forgotten CMakeLists.
- **C — both (commit AND guard).** R5 S4 M3 BP4. **Rejected:** the guard is cheap insurance but carries A's masking cost for future recurrences; with the files committed it protects only against someone deleting them, which git already reports.

**Cascade.** `unittests/Core/coach/{CMakeLists.txt,stubs/*.h}` become tracked (6 files) — WIKI MAP `unittests/` line updated. `unittests/CMakeLists.txt` is UNCHANGED (no guard added). Cause 1 (8 phantom refs) had no trade space and was fixed mechanically. **The repair then TRIPLED in scope**, because running the actual build exposed three further pre-existing causes invisible to configure: ORCH-005 (translations/lrelease never ported to CMake), ORCH-006 (11 tracked sources absent from the CMake lists, so HEAD could not link), ORCH-007 (`CMAKE_CXX_EXTENSIONS OFF` diverging the dialect from qmake and breaking the `QBluetoothUuid` link). All fixed in the same commit; see their findings rows.

**Evidence bar.** Verified against a clean extract of the STAGED tree (`git archive` of the index — never the working tree, which is dirty with unrelated work): configure+generate succeed, lrelease emits all 13 `.qm`, the full build links `GoldenCheetah` (27.9 MB) plus 26 test executables including `testCoachTools`, and `ctest` is **26/26**. The committed tree hash was confirmed byte-identical to the verified tree (`d8dfe490…`), so there is no gap between the evidence and the commit. Two-directional check on ORCH-006/007: with cause 5 alone reverted the link yields EXACTLY ONE undefined reference, at `KurtInRide.cpp`.

**Accepted residuals.** (i) `CMAKE_CXX_EXTENSIONS ON` now permits GNU extensions project-wide — this CONVERGES on the qmake reference build rather than diverging from it, but it does relax strict-ISO enforcement. (ii) The uncommitted `src/Train/KurtInRide.cpp` workaround in the working tree is now REDUNDANT and should be dropped by its owner rather than committed — it hand-rewrites byte-order handling to work around what ORCH-007 shows is a build-configuration defect. (iii) The `.qm` are generated into the SOURCE translations dir (matching qmake's `TS_DIR`) because `application.qrc` addresses them relative to the `.qrc`; they stay gitignored, so this does not dirty `git status`. (iv) All other pre-session Coach/libusb/Calendar work in both CMakeLists remains uncommitted and untouched.

### Alignment probe
grep -n 'DiaryWindow\|Velohero\|DiarySidebar' src/CMakeLists.txt        # expect NOTHING (8 phantom refs gone)
grep -n 'CMAKE_CXX_EXTENSIONS' CMakeLists.txt src/CMakeLists.txt        # expect ON in both
grep -n 'qt_add_translation\|LinguistTools\|AUTOGEN_TARGET_DEPENDS' src/CMakeLists.txt   # expect all three present
git ls-files unittests/Core/coach                                        # expect CMakeLists.txt + stubs/ + coach.pro + testCoachTools.cpp

---

## DEC-029 — Upload-dialog UAF fix shape: two-phase init + heap/WA_DeleteOnClose, MODAL preserved (REQ-019)
- Status: accepted (B — two-phase `start()` + heap + `WA_DeleteOnClose`, `exec()` retained, store ownership UNCHANGED)
- Reversibility: moderate — the heap/`WA_DeleteOnClose` conversion is effectively a one-way door already opened by DEC-027's stack-bad-free finding (reverting to stack allocation reopens a closed BLOCKING route). Switching B→A (async) afterward is a confined re-edit of `CloudService.{h,cpp}` + `MainWindow.cpp:2548-2565`; no on-disk artifact or schema involved.
- Decided / last-reviewed: 2026-08-07
- Serves: REQ-019 (A3-R027-F1, HIGH — live for 11 Upload-capable services); constrained by DEC-024, DEC-025, DEC-026, DEC-027 (the proven sync recipe) and by REQ-017's store-ownership contract (MainWindow.cpp:2558-2562)
- Dependents: REQ-019 build slice; TEST-079, TEST-080; ORCH-008 (test-home misnomer); REQ-020 (inherits whichever harness placement this establishes)

**Problem.** `CloudServiceUploadDialog`'s constructor (CloudService.cpp:326-412) performs every blocking operation while the object is a STACK dialog parented to the `WA_DeleteOnClose` MainWindow: `store->open()` (:346, a real nested `QEventLoop` over network I/O), an unsaved-changes `QMessageBox::exec()` (:363), `compressRide`/`writeFile` (:386/:389), and a failure `QMessageBox::exec()` (:401) — with zero `QPointer` self-bails. A parent teardown inside any of those loops resumes the ctor on freed memory, and it then touches `context->mainWindow->saveSilent` (:369) and `QWidget::hide()` (:403). The single caller `MainWindow::uploadCloud` (MainWindow.cpp:2548-2565) passes `this` as parent and then runs `closeAndDeleteStore(db)` at :2563 — a member call on a possibly-destroyed MainWindow. This is the pre-DEC-024..027 bug verbatim, in a sibling dialog.

**Two structural facts that make this NOT a straight recipe transfer** (scout-found, orchestrator-verified at the cited lines):
1. **No user-close route exists during the blocking work.** `okcancel` is created at :338 but is not connected to any slot until `completed()` fires (:431), and the dialog is never shown or `exec()`'d until after the ctor returns (`CloudService::upload` :82-83). DEC-024's `done()`/`closeEvent()`/`deferCloseIfBusy` gate exists to stop a *user-initiated* close racing a blocking call — a hazard that is structurally absent here. Transferring that machinery would add untested, unreachable code.
2. **Async conversion would force an ownership change.** `writeFile` is already asynchronous (completion arrives via `writeComplete` → `completed()`, :410/:425), and `exec()` (:415) is what waits for it. Making the dialog modeless means `uploadCloud`'s synchronous `closeAndDeleteStore(db)` at :2563 races the in-flight upload, so the store would have to move into the dialog — inverting the REQ-017 contract shipped days ago in `ae5a7a8ab`.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — two-phase init + heap/`WA_DeleteOnClose`, modal `exec()` preserved, caller keeps store ownership.** R5 S3 M5 BP5. Ctor becomes the widget shell only; a new `bool CloudServiceUploadDialog::start()` carries the body from `store->open()` onward with a `QPointer<CloudServiceUploadDialog>` self-bail after each of the four blocking calls, every result landed in a LOCAL (LSN-037). the construction site becomes heap + `WA_DeleteOnClose` + `if (start()) exec();`. **[CORRECTED 2026-08-08 post-build, O-R019-01 — this clause originally said "`uploadCloud` becomes…", naming the wrong function. `MainWindow::uploadCloud` does NOT construct the dialog; `CloudService::upload` (CloudService.cpp:78-87) does, and uploadCloud (MainWindow.cpp:2556) is only its single caller. The conversion landed in `CloudService::upload`, which is the same geometry this DEC intended and additionally avoids editing another owner's dirty file; `MainWindow::uploadCloud` took a comment-only update. The error was internal inconsistency — the correct site is cited accurately three paragraphs above in fact 1 — not an ungrepped premise.]** **Deliberately OMITS** the DEC-024 close-gate per fact 1. `closeAndDeleteStore(db)` at :2563 stays exactly where it is; the REQ-017 comment stays true. Smallest change that makes the hazard structurally impossible (user constraint #3) with zero ownership cascade (constraint #2).
- **A — full DEC-024..027 transfer: async/modeless, dialog takes store ownership.** R4 S5 M3 BP4. **Rejected:** highest precedent-fidelity and it would make uploads non-blocking, but it buys that with (i) a `writeComplete`-vs-late-teardown race that has no precedent and no test, (ii) deleting the `closeAndDeleteStore(db)` call and rewriting the REQ-017 contract comment, and (iii) making Upload non-modal for all 11 services — a separate UX decision from the one taken for sync in DEC-027, where the open-ended duration justified it. Upload is per-ride and short.
- **C — reparent the dialog to `nullptr`.** R2 S2 M2 BP2. **Rejected:** removes only the dialog-as-child free while leaving the `context`/`context->mainWindow` UAF exactly as exposed, so it still needs B's guards — strictly less protection for the same code. Also orphans a floating progress dialog that outlives its athlete window (unclear interaction with `quitOnLastWindowClosed`). This is the shape DEC-025 and DEC-027 each already examined and rejected for the structurally identical sync case.

**Blast radius.** 11 Upload-capable services reach this dialog — verified by reading each `capabilities()` override plus the subclasses that inherit the `CloudService.h:104` base default: explicit bit — RideWithGPS, CyclingAnalytics, Selfloops, SportsPlusHealth, TrainingsTageBuch, Xert; inherited default — Strava, Dropbox, SixCycle, SportTracks, LocalFileStore. **GarminConnect is NOT among them** (`GarminConnect.h:76` = `Query|Download`, read-only by DEC-005): this ledger's own subject is untouched by its own REQ. Under Option B the only observable change for all 11 is stack→heap allocation — still modal, still blocking, no UX delta.

**Evidence bar (non-negotiable, lifetime slice).** Acceptance is an EXECUTED ASan test, not reasoning about Qt semantics. **One claim in this decision is reasoned and not yet executed** and must be the FIRST RED test written: that `QDialog::exec()` self-protects when `this` is destroyed mid-loop, which is what lets `closeAndDeleteStore(db)` stay unguarded at :2563. This is the same class of assumption as DEC-024's original `closeEvent()` premise, which proved incomplete. **If it does not hold, Option B is falsified in part** and needs its own gate around `exec()` — re-open this DEC rather than patching around it.

**Test home (decided with the user).** The new slots go into the EXISTING ASan target `testGarminConnectSyncDialogClose` / its directory `unittests/Core/garminconnect/`, reusing the offscreen-QApplication + `-fsanitize=address` + blocking-store-stub harness (CMakeLists.txt:1339-1457). This is a knowing misnomer — the code under test is generic `CloudService`, not Garmin — accepted to reach executed evidence fast rather than front-loading a harness relocation. Registered as **ORCH-008** so the debt is visible; REQ-020 will inherit the same placement and strengthens the case for generalizing later.

### Alignment probe
grep -n 'bool start()' src/Cloud/CloudService.h                          # expect the new CloudServiceUploadDialog::start()
grep -c 'self.isNull()' src/Cloud/CloudService.cpp                        # expect >= 6 upload self-bails (CORRECTED 2026-08-08, O-R019-01: the original probe counted 'QPointer<CloudServiceUploadDialog>' and expected >=4, which the DEC-026 idiom this same DEC mandates makes UNSATISFIABLE — that idiom declares ONE QPointer per function and bails off it N times. Actual: 2 textual QPointer occurrences, 6 self-bails. Count the bails, not the declaration.)
grep -n 'closeAndDeleteStore(db)' src/Gui/MainWindow.cpp                  # expect STILL PRESENT in uploadCloud (~:2563) — ownership unchanged
grep -n 'deferCloseIfBusy\|closeEvent' src/Cloud/CloudService.cpp         # expect NO new upload-dialog close-gate (DEC-024 machinery is sync-only)
grep -n 'WA_DeleteOnClose' src/Gui/MainWindow.cpp                         # expect it on the new heap upload dialog

---

## DEC-030 — Collaborator-lifetime UAF fix shape: reparent to AthleteTab, PROBE-FIRST, with QPointer guards as a rider (REQ-021)
- Status: accepted (B — reparent both dialogs to `context->tab`, **conditional on an executed visibility probe**; Option A's collaborator `QPointer` guards ship alongside as a rider, NOT as an alternative)
- Reversibility: cheap but ASYMMETRIC — reverting the reparent after ship is a SECOND user-visible window-behaviour change across 17 shipped integrations, so the cost of being wrong is paid twice. This is precisely why the probe gates it. The harness work (below) is unconditionally reversible and unconditionally useful (required by every option, and by REQ-020 after this), so it is not a bet.
- Decided / last-reviewed: 2026-08-10
- Serves: REQ-021 (A3-R019-F1 BLOCKING + A3-R019-F2 live-on-master); constrained by DEC-024, DEC-025, DEC-026, DEC-027, DEC-029 (the dialog-lifetime recipe, explicitly NOT re-litigated) and by the standing evidence bar for lifetime slices (executed ASan test, guard on the layer performing the unsafe operation)
- Dependents: REQ-021 build slice; TEST-081..084; REQ-020 (inherits the harness AND the recipe); REQ-022, REQ-023 (new, this decision's sibling-scan byproducts); ORCH-008 (naming debt grows by one more non-Garmin subject); A3-R019-F3, B-R019-04, B-R019-05 (all closed by the harness half); S-R021-01 (folded in by user decision)

**Problem.** Five decisions (DEC-024/025/026/027/029) and four adversarial cycles closed a UAF class by guarding **the dialog's own lifetime** — `QPointer<T> self(this)` bails after every nested loop. That guard covers exactly ONE pointer: `this`. `context` and `item` are owned by the **AthleteTab**, which `MainWindow::removeAthleteTab` (MainWindow.cpp:2183-2185) deletes SYNCHRONOUSLY, while MainWindow's own `WA_DeleteOnClose` deletion is DEFERRED. So the dialog reliably OUTLIVES its Context, every `self.isNull()` stays false, and the resumed frames dereference freed memory. Both dialogs are affected; the sync half is already shipped on master in `ae5a7a8ab`.

**Three scout-found facts that shaped this decision** (all orchestrator spot-checked at the cited lines, 2026-08-10):
1. **`Context` and `RideItem` are BOTH `QObject`s** (`src/Core/Context.h:106`, `src/Core/RideItem.h:41`). `QPointer` works on them; no liveness registry is needed. This was flagged as check-do-not-assume in the briefing, and it is what makes Option C (a hand-rolled abandonment signal) redundant rather than merely costly.
2. **The sync dialog's collaborator surface is ~8x larger than REQ-021's stub stated** (S-R021-02). `MainWindow::syncCloud` (MainWindow.cpp:2604-2606) opens it MODELESSLY (`open()`, not `exec()`), so it outlives its caller and keeps running slots. Beyond the two `start()` sites, `context->` is dereferenced at NINE further sites in FIVE further slots: `refreshClicked` (:1318, :1371, :1443), `syncNext` (:1743, :1788), `downloadNext` (:1857), `uploadNext` (:1985), `saveRide` (:2064, :2088). **This is the decisive fact: a per-site fix closes ~18% of the surface; a structural one closes 100% for the same two lines.**
3. **A live `this`-axis gap remains on master** (S-R021-01): the sync dialog's Cancel branch runs `processEvents()` (:1107) then `invokeMethod(this,"close")` (:1108) with NO `self.isNull()` between them, while the upload dialog's identical branch DOES carry it (:451-452). Folded into REQ-021 by user decision.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — reparent both dialogs to `context->tab`.** R5 S5 M5 BP4. Two lines: `CloudService::upload` (:95) passes `context->tab` instead of `parent` (verified: exactly ONE production caller, MainWindow.cpp:2556), and the sync ctor (:825) becomes `QDialog(context->tab, Qt::Dialog)` (covers both construction sites at once). `context->tab` is an `AthleteTab*` (Context.h:135) and `AthleteTab : public QWidget` (AthleteTab.h:32). `delete tab` (:2183) precedes `delete athlete` (:2184) and `delete context` (:2185) — and `delete context` occurs EXACTLY ONCE in all of `src/` (grepped). So the dialog is destroyed by `deleteChildren()` strictly BEFORE the objects it points at, every existing `self.isNull()` bail becomes true again, and the nine modeless sync sites are covered for free. **It adds nothing to DEC-024..029; it restores the ordering precondition those decisions were designed against.**
- **A — per-site collaborator `QPointer` guards.** R4 S3 M4 BP4. **Rejected as the primary, RETAINED as a rider.** Widening ~11 bail sites to `self.isNull() || ctx.isNull() || ride.isNull()` uses the one mechanism already mutation-proven load-bearing in this exact file, with no unverified premise and no window-behaviour change. But it is per-SITE: it protects only the sites someone remembered, and every future blocking call added to shared `CloudService.cpp` owes two more checks with silent failure if forgotten. **It ships anyway as a rider** because it covers the one window B cannot — see below.
- **C — Context-driven abandonment signal.** R4 S4 M2 BP3. **Rejected.** `Context` already emits `athleteClose` (Context.h:181/288) from `Athlete::close()` (Athlete.cpp:214), called at MainWindow.cpp:2171 — twelve lines before `delete context`, on both routes — so the trigger genuinely exists. But it introduces a THIRD lifetime protocol alongside DEC-024's close-gate and DEC-025's blocking-depth decline, every future cycle must reason about all three interacting, and it needs explicit checks at ~40 sites anyway. It is the right answer in a codebase where the collaborators are NOT QObjects; here they are (fact 1). It also has a harness problem the others don't: `Athlete::close()` is stubbed in NEITHER `ImportSeamStubs.cpp` nor `SyncDialogSeamStubs.cpp`, so a test would drive the slot rather than the trigger — the exact fidelity gap that produced A3-R019-F3.

**THE GATING PROBE (this is what "probe-first" means, and it is not optional).** Option B rests on ONE Qt behaviour the scout could not verify from a primary source: **does hiding an `AthleteTab` also hide a child `QDialog` window in Qt 6.8.2?** `MainWindow::switchAthleteTab` (:2165) hides tabs on every athlete switch. Web search returned only undated forum anecdotes, no Qt source or documentation. This project's standing rule is that framework semantics are decided by execution, not reasoning — and its own history (DEC-024's `closeEvent` premise; the `deleteLater` loop-level claim in TEST-070's header) is that this exact class of plausible Qt claim is where it goes wrong. **Therefore: build the harness FIRST, run a ~20-line offscreen probe (parent a QDialog to a child QWidget, `open()` it, hide the parent, assert `isVisible()`), and only then write a production line.** If child windows follow the parent's hide, **Option B is dead on UX grounds and the fix falls back to Option A applied to ALL ELEVEN sites** (both `start()`s plus the nine modeless derefs) — pre-authorised by the user on 2026-08-10 so this does not require a second decision round.

**The rider, and why it is one decision and not a fourth option.** Option A's collaborator `QPointer`s ship in both `start()` methods regardless of the probe outcome. `MainWindow.cpp:2148-2171` runs `rideCache->cancel()` (:2148), `namedSearches->write()` (:2151), `tab->close()` (:2170) and `athlete->close()` (:2171, which runs autobackup) **BEFORE** the three deletes. A frame resuming inside any of those sees a partly-torn-down Athlete with all pointers still non-null. B fixes the destruction ORDER; it does not cover the pre-delete window. (Whether any of those four calls actually spins an event loop is UNVERIFIED — the scout read the call sites, not the implementations. The rider makes that question non-load-bearing, which is the point.)

**Blast radius.** Both dialogs live in shared `src/Cloud/CloudService.cpp`, reached by all **17** `CloudService` subclasses — enumerated by predicate (`grep -rn "public CloudService" src/Cloud/*.h`, one line per hit, membership not proximity, per LSN-034): CalDAVCloud, Dropbox, Azum, PolarFlow, CyclingAnalytics, Strava, LocalFileStore, GarminConnect, RideWithGPS, Selfloops, SportsPlusHealth, Nolio, Withings, SixCycle, SportTracks, Xert, TrainingsTageBuch. Option B changes VISIBLE window parenting on that path, which is the whole reason for the probe and for the two extra control assertions in the acceptance criterion.

**Test home.** The existing ASan target `testGarminConnectSyncDialogClose` (`unittests/Core/garminconnect/`), extended — no new target, no CMake wiring (`stubs/ImportSeamStubs.cpp` is already in its source list at CMakeLists.txt:1356). ORCH-008's naming debt grows by one more non-Garmin subject; REQ-021 is the THIRD non-Garmin lifetime slice, which is ORCH-008's own stated trigger to reconsider extraction. **Flagged, deliberately NOT acted on inside this REQ** (user-confirmed 2026-08-10).

**The harness half of this decision (mandatory, sequenced FIRST).**
1. **A purpose-built fake owner**, `FakeAthleteWindow`, living in the test `.cpp` (not the stubs): a `QWidget` holding `tabWidget` / `context` / `athlete` / `item`, with `closeAthleteTab()` doing `delete tabWidget; delete athlete; delete context;` SYNCHRONOUSLY (models MainWindow.cpp:2183-2185) and `closeWindow()` doing that for every tab then `deleteLater()` on itself (models closeEvent, MainWindow.cpp:1102-1103). The existing `killOwner` helper CANNOT be patched into shape — it deletes owner and context back-to-back, so the dialog always dies first and `self.isNull()` is already true on resume, making the collaborator axis unobservable by construction. Both production routes must be drivable: single-tab close (window survives, `AthleteView.cpp:213`) and whole-window close. The SAME class serves Option A (dialog child of the window) and Option B (dialog child of `tabWidget`), and hosts the visibility probe — which is why it is built first regardless of outcome.
2. **Member touches in `stubs/ImportSeamStubs.cpp`** so freed collaborators actually FAULT for the reason the test claims: `Context::metadataFlush()` (:212) and `RideItem::notifyRideMetadataChanged()` (:312) each load a real member; `MainWindow::saveSilent` (:383-387) can only do a QObject-level load (`isWidgetType()`) because the target's "MainWindow" is a `reinterpret_cast` of a plain QWidget (test file :1539) — **record that as a PRAGMATIC close of B-R019-05, not a strict one.** Also initialise `RideItem::isdirty` (:297), uninitialised today, which is B-R019-04 and fails as a HANG.
3. **Closes as a byproduct:** A3-R019-F3, B-R019-04, B-R019-05. Does **NOT** close A3-R019-F5 (7 of 9 slots hand-copy `CloudService::upload`), which stays open.

**Folded in by user decision (2026-08-10):** S-R021-01 — add the missing `if (self.isNull()) return false;` between CloudService.cpp:1107 and :1108.

**Alignment probes** (each RUN against the current tree before this DEC shipped, per LSN-045 — all fail NOW in the way that proves they will pass after):
```
grep -n 'class Context : public QObject' src/Core/Context.h                    # PASSES NOW (=1) - premise, not outcome
grep -n 'class RideItem : public QObject' src/Core/RideItem.h                  # PASSES NOW (=1) - premise, not outcome
grep -c 'delete context;' src/Gui/MainWindow.cpp                               # expect 1, unchanged by this DEC
grep -n 'context->tab' src/Cloud/CloudService.cpp                              # expect 0 NOW, >=2 after (Option B only)
sed -n '1107,1108p' src/Cloud/CloudService.cpp                                 # expect NO self.isNull() NOW, one after (S-R021-01)
grep -n 'FakeAthleteWindow' unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp  # expect 0 NOW, >0 after
```

---

## DEC-031 — Reopening DEC-025: a frame-counted DEFERRED REAPER replaces the deliberate store leak (REQ-021)
- Status: accepted (B — frame-counted deferred reaper, **conditional on an executed Qt loop-level probe**)
- Reversibility: cheap — one destructor branch, one `~BlockingCall` branch and a small record type, all inside `CloudService.h/.cpp`; the revert is a single-file diff and TEST-084's assertions revert with it. No subclass edits, no call-site edits, no user-visible behaviour change.
- Decided / last-reviewed: 2026-08-11
- Serves: REQ-021 (A3-R021-F4); **REOPENS DEC-025**; constrained by DEC-024 (`BlockingCall`), DEC-030 (the reparent that forced this), and REQ-017 (b)/(e) store ownership — *note: an earlier draft of this briefing miscited that contract as "DEC-017", which is actually REQ-008 sidecar-persistence shape; see O-R021-03*
- Dependents: TEST-084 (its assertions INVERT — see evidence bar), TEST-072 (inverts identically), TEST-073 (unchanged, and becomes the discriminator that the reaper did not merely move the leak); REQ-024 (independent, unaffected)

**Why DEC-025 is reopened rather than amended.** DEC-025 decided that `~CloudServiceSyncDialog` declines to delete the store while `blockingCallDepth > 0`, deliberately leaking it. Its rationale had exactly ONE load-bearing clause, stated verbatim in the code: *"Nothing is deferred: at parent-teardown time the application is already tearing down, so there is no live event loop left for a reaper to run on."* **DEC-030's reparent falsified it.** The dialog is now a child of `context->tab`, and `MainWindow::removeAthleteTab` deletes that tab on a route after which **the application keeps running with a live event loop**. A reaper is therefore not merely possible — it is the option DEC-025 said it would have taken had this fact held. Amending the comment would have left an accumulating leak blessed by a rationale that no longer exists → [[LSN-052]].

**Population, corrected (scout, orchestrator-verified).** The leak's trigger is NARROWER than A3-R021-F4 stated: with exactly one athlete tab, `MainWindow::closeAthleteTab()` takes the `closeWindow()` branch (MainWindow.cpp:2129-2130), NOT `removeAthleteTab`. The app-survives routes require **≥2 athlete tabs or ≥2 MainWindows**. The substance (routine, app keeps running, accumulating) survives.

**Leak quantified (this is what decided it).** The store is a **parentless** `QObject` (`CloudService::CloudService(Context*)` — `context` is a collaborator, not a QObject parent), so nothing in the tree can ever free it. Transitively: the subclass + its `QNetworkAccessManager`; the entire `list_` of `CloudServiceEntry`, which only `~CloudService` frees and which `newCloudServiceEntry` appends to on EVERY `readdir` (≈150–250 KB for a 12-month, ~500-activity listing, scaling with listing volume × refresh count). **For 16 services that is ~0.05–0.5 MB per occurrence and no threads. For GarminConnect it is categorically different:** `~GarminConnect` and `GarminConnect::close()` are what `quit()+wait()` the download worker **QThread** and free `m_adapter`, the **embedded CPython session** — so a leaked Garmin store means **one OS thread left running and one interpreter session resident, per occurrence, for the process lifetime**, not boundable from the code. That asymmetry, not the byte count, is the argument.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — frame-counted deferred reaper.** R4 S4 M3 BP4. The decline branch hands `(store, depth)` to a small refcounted orphan record instead of dropping it; `~BlockingCall`, on its `dialog.isNull()` stand-down path, releases one frame; the last release runs `closeAndDeleteStore`. Entirely within `CloudService.h/.cpp` — **zero edits to any of the 17 subclasses, zero call-site edits, zero user-visible change**, which matters because DEC-030 already spent this REQ's entire behaviour-change budget on the reparent. Failure mode on an unwrapped nested loop is *today's* behaviour, never worse.
- **C — the store guards its own lifetime** (busy-count moves INTO `CloudService`; the store defers its own destruction). R5 S4 M2 BP4. **Rejected for THIS REQ, and it is the technically superior answer** — the only option that closes S-R031-01 and A3-R021-F1 *by construction*, because the guard would live in the store's own frames rather than in the dialog's call sites. Rejected because it lands a correctness obligation on ~17 subclasses inside code that **A3-R021-F8 proved has ZERO test coverage** (no test executes `syncNext`/`downloadNext`/`uploadNext`/`completedRead`/`completedWrite`/`saveRide`), inside a REQ already carrying two BLOCKING findings — which is exactly how the DEC-024→025→026 recurrence happened. It is the right REQ-scale follow-up, not the right move here.
- **A — keep the leak, re-decided and bounded** (+ wrap the two latent `compressRide`/`writeFile` pairs). R2 S2 M4 BP2. **Rejected:** smallest diff and the only option needing no test changes, but it re-blesses an accumulating leak whose justification is gone, and **it provably cannot reach S-R031-01** — see the invariant note below.
- *Rejected before scoring, so the list is not padded:* parenting the STORE to `context->tab` (`delete tab` is the same synchronous instant — the UAF returns identically); parenting it to `context->mainWindow` (the pre-REQ-017 leak-to-exit with extra steps); a plain `store->deleteLater()` (actively unsafe — see the probe).

**THE GATING PROBE (same discipline as DEC-030, which is why that decision survived contact with reality).** Qt documents that `deleteLater` performs the deletion when control returns to **the event loop from which it was called** — here that loop IS the store's own suspended nested loop, so a naive `store->deleteLater()` (or a queued `invokeMethod` posted from inside that loop) **would delete the store under its own frame**. That is why the frame-counted variant is the only sound shape. **This project has already had one `deleteLater` loop-level claim falsified** (cited in DEC-030 re TEST-070's header), and its standing rule is that framework semantics are decided by execution, not documentation. **Therefore: before any production line, run a ~20-line offscreen probe that calls `deleteLater()` and a queued `invokeMethod` on an object from inside a nested `QEventLoop` and asserts whether the object dies before that loop returns.** Ship the probe's positive control in the same slot ([[LSN-050]]).

**A specification the builder must NOT be left to discover** (scout's open question 2): when TWO sync dialogs have suspended frames on the same stack, a single static counter is WRONG. The orphan record must be **per-dialog and refcounted, created by the first `BlockingCall`**. This is the implementation's main risk and is specified here deliberately.

**The `BlockingCall` invariant — split answer, and the split is the point (scout, orchestrator-verified).**
- The two unwrapped `compressRide`/`writeFile` pairs are in **`syncNext` and `uploadNext`** (NOT `saveRide`, which makes no store calls at all — O-R021-03). They are a **separate, LATENT defect**: all 11 `writeFile` implementations were checked and **none runs a nested loop**, and `compressRide` is `QTemporaryFile` + `writeRideFile` + `ZipWriter` with no loop. Pre-emptive consistency; does not gate this DEC.
- **But the invariant as WRITTEN is unachievable**, and that IS part of this decision. `Strava::readFileCompleted` enters a nested `QEventLoop` from a **signal-delivery frame the dialog never calls** (S-R031-01, → REQ-024) — you cannot wrap a call site that does not exist. **So `blockingCallDepth` is not a complete predicate.** Option A rested on it entirely; **B rests on it only for the leak-fixing property and degrades to today's behaviour where it fails**; C would not rest on it at all. DEC-024's invariant text must be narrowed to what is true: *every store call the DIALOG makes that can run a nested loop is wrapped* — it says nothing about loops the store enters on its own.

**Evidence bar (user decision 2026-08-11): TEST-084's inversion belongs to THIS DEC, and BLOCKS REQ-021 from closing.** TEST-084's `assertSyncDiedWithItsTab` currently asserts `!storeDestroyed` / `!storeClosed` across 8 runs — i.e. the suite encodes the leak as REQUIRED. Under B those invert to *"closed and deleted exactly once, AFTER the nested loop returned"*; TEST-072 inverts identically; **TEST-073 (the idle control) stays unchanged and becomes the discriminator that the reaper did not merely move the leak**. REQ-021 cannot close while a test in its own changeset asserts behaviour this decision overturns. A new RED direction comes free: drop the `release()` call and TEST-072/084 must fail on a POSITIVE assertion — which means leak coverage stops depending on `detect_leaks`, currently disabled on this target (A3-R027-F4).

**Alignment probes** (to be RUN against the tree before the build ships, per [[LSN-045]]):
```
grep -n 'blockingCallDepth > 0' src/Cloud/CloudService.cpp        # the decline branch this DEC replaces
grep -c 'deleteLater' src/Cloud/CloudService.cpp                   # expect NO naive store->deleteLater() after
grep -n 'storeDestroyed\|storeClosed' unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp   # assertions that must INVERT
```

---

## DEC-032 — Closing the silent stall: an in-loop `continue` in the branch that arms nothing (REQ-027)
- Status: accepted (A — in-loop `continue`, adopting `uploadNext`'s already-shipped shape)
- Reversibility: cheap (3 lines in one branch + 2 read-side checks; revert is a diff)
- Decided / last-reviewed: 2026-08-13
- Serves: REQ-027; closes B-R026-01, S-R027-01, S-R027-04; constrained by DEC-024 (BlockingCall depth), DEC-025/026 (guard placement), DEC-030 (synchronous parenting), DEC-031 (reaper drains on last counted frame), and REQ-026's abort invariant  **SUPERSEDED CLOSURE CLAIM — repaired 2026-09-01.** This line formerly read *"closes B-R026-01, O-R027-01, S-R027-01, S-R027-04"*. **`O-R027-01` is NOT closed by DEC-032.** This decision's own `### AMENDMENT 2026-08-13 — scope item 3 REMOVED from DEC-032, routed to DEC-033 (B-R027-01)` withdrew exactly that scope, and `traceability.md:132` records that O-R027-01's stall route cannot be closed at the call site. `O-R027-01` is routed to **DEC-033 / Stage 2**. **The other three closures stand.** A header field that outlived its own amendment — the [[ORCH-026]] shape.
- Dependents: TEST-095..100; B-R025-03 stays queued and is NOT resolved here

### The question
`CloudServiceSyncDialog::syncNext` (defined `CloudService.cpp:1967`) processes one checked row per
invocation and relies on a completion callback to re-drive it. Its four re-entry points are :1962,
:2273, :2326, :2472. The upload-side **parse-failure branch (:2067-2071)** initiates no async work
and falls to the unconditional `return true` at :2075, arming none of them — the batch stalls
silently. How should the loop re-drive itself without unbounded re-entry or stack growth?

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A in-loop `continue` (adopt `uploadNext`'s shape) | 5 — adds no suspension point; the one new hazard (`~BlockingCall` replaying `close()` under `continue`) is excluded by a grep-verified invariant, not a Qt semantic: `closeDeferred` is set true at :1475 only, and :1476 sets `aborted` on the next line | 4 — zero stack growth for any N, strictly better than today's baseline (S-R027-03); N failures run in one GUI turn, but :2069 pumps events each pass so Abort still lands | 5 — smallest diff, and it REMOVES a divergence: `syncNext` and `uploadNext` end with one shape between them | 4 — "return to the event loop between units of work" is the general consensus, which this does not do; wins here on smallest-change + shared-across-16-services, and the failing step is wholly synchronous so there is nothing to wait for |
| B queued self-invoke (`invokeMethod(this,"syncNext",Qt::QueuedConnection)`) | 3 — turns on Qt delivery semantics, and TEST-089 ALREADY MEASURED the adverse one: a queued invoke posted from inside a nested QEventLoop is delivered BEFORE that loop returns, in production's exact geometry (`CloudService.h:512-521`) | 4 — constant stack when the post lands on the main loop; one row per event-loop turn keeps the GUI repainted | 3 — header change + a second, invisible driver of a loop whose callbacks address rows by `listindex-1`; if a re-drive ever coexists with an in-flight transfer, `completedWrite` labels the WRONG row — silent, not a crash | 4 — the canonical idiom, but it buys a queue round-trip per failure to solve a problem this frame does not have |
| C explicit `StepOutcome` enum (split driver from step) | 4 — makes the defect unrepresentable, but relocates the code that DEC-024/025/026/030/031's PLACEMENT-sensitive guards are pinned to | 5 — one driver frame for any N; natural seam for a later yield policy | 3 — best shape to inherit, worst to land today: rewrites three functions and invalidates ~60 lines of DEC-provenance comments citing current line numbers | 5 — an explicit state machine is what a reviewer would ask for if this file were new |

### Decision
**Option A.** Three reasons, in the user's own constraint terms. (1) Smallest change that closes the
class: 3 lines in `syncNext` plus one read-side check each at :1996/:2135, and all ~16 services are
fixed with no seam change and no per-service work. (2) It is not a new design — `uploadNext:2394-2421`
has run this exact control flow since REQ-026 shipped, held by TEST-093, so A **converges** the two
functions rather than adding a third idiom. (3) It is the only option resting on a locally provable
invariant rather than a Qt delivery semantic — and where this project HAS measured that semantic
(TEST-089), the measurement argues against B. C is the better six-month shape and is the right
follow-up if this loop is reopened for other reasons; it is the wrong first move because it
re-derives five prior DECs' guard placements in order to fix one branch.

### Scope folded in by user decision (2026-08-13)
- **S-R027-01 — the live REQ-026 escape.** Neither `syncNext:2046-2062` nor `uploadNext:2374-2389`
  re-reads `aborted` between `openRideFile`'s nested loop and `compressRide`/`writeFile`, so an abort
  during a PARSEABLE row's parse still transfers that ride. One line at each site. Folded in rather
  than deferred because A's `continue` makes `syncNext` re-enter that path within the same frame, so
  leaving it open would weaken A's own abort story.
- **O-R027-01 — the discarded `readFile` bool**, closed on the READ side only, at :1996 and :2135.
  Sound because every implementation returns false only on paths that started nothing and will emit
  nothing (verified across LocalFileStore/Strava/Dropbox/Xert/SportTracks/CyclingAnalytics/PolarFlow/
  Nolio/SixCycle/GarminConnect). **NOT extended to `writeFile`** (:2062, :2389): `LocalFileStore`
  emits `writeComplete` AND returns false (`LocalFileStore.cpp:162-165`), so the same rule would
  double-drive the loop. The asymmetry is recorded in the code comment (S-R027-02).
- **S-R027-04 — the progress bar**, which does not advance for a failed row even in the branch that
  already continues. One line per site; it is what makes clause (c) real.
- **The batch-generation rider (S-R027-05).** Option A EXTENDS a pre-existing hazard from `uploadNext`
  to `syncNext`: two clicks delivered inside one `processEvents()` burst make `downloadClicked` reset
  `aborted=false, listindex=0` (:1923) and re-drive the loop, and a `continue`-shaped branch keeps the
  STALE frame iterating alongside the new batch, after which `listindex-1` addressing labels the wrong
  rows. A `return`-shaped branch stands the stale frame down. Because this fix is what extends the
  hazard, the guard ships WITH it rather than as a follow-up: one `int batchGeneration` member,
  incremented in `downloadClicked`'s start branch, snapshotted at loop entry, compared before each
  `continue`. This also closes the existing `uploadNext` instance. **This is an orchestrator scope
  call, recorded here rather than left to the builder** — it is in scope because Option A makes it so.

### Deliberately NOT in scope
- **B-R025-03** (the four uncounted `processEvents()` at :2012/:2144/:2246/:2303) stays its own queued
  decision. Option A adds no uncounted `processEvents` and relies on no property of `blockingCallDepth`
  beyond ">0 defers close", which is already true, so it does not depend on that incompleteness.
- **No completion-tail string change.** Adding "N failed" to :2097/:2166/:2442 is user-visible text in
  three tails shared by ~16 services and reopens translations. Visibility rests instead on the per-row
  status cell, the now-advancing bar, and `successful < downloadtotal`. Orchestrator call; revisit if
  a user reports the tail as confusing.

### Alignment probe (run against the CURRENT tree; must fail NOW in the way that proves it will pass after)
```
grep -n 'continue;' src/Cloud/CloudService.cpp | sed -n '1,40p'    # expect NO continue in syncNext's parse branch now
grep -c 'batchGeneration' src/Cloud/CloudService.h                  # expect 0 now, >=1 after
sed -n '2046,2062p' src/Cloud/CloudService.cpp                      # expect NO 'aborted' read between openRideFile and writeFile now
sed -n '2374,2389p' src/Cloud/CloudService.cpp                      # same, uploadNext
grep -n 'store->readFile' src/Cloud/CloudService.cpp                # expect the bool DISCARDED at :1996 and :2135 now
```

### AMENDMENT 2026-08-13 — scope item 3 REMOVED from DEC-032, routed to DEC-033 (B-R027-01)
The builder fired the stop-and-report hatch before writing any code, and it was right. This DEC's
scope item 3 rested on the premise that `readFile` returning `false` means "armed nothing". That is
**false for the majority implementation**: `GarminConnect::readFile` has eight `return false` sites
and seven post a completion on the line above, via `Qt::QueuedConnection` posters (:765-770, :780-785)
that land AFTER the return. Only :452 is silent — the one site the orchestrator sampled and
generalised from. Building item 3 as specified would have double-driven the loop on ~11 integrations
(synchronous bar-advance + continue, then the queued completion advancing again, labelling
`child(listindex-1)` and re-driving), i.e. exactly the double-drive this DEC correctly forbade for
`writeFile`, and a regression of DEC-022/023.
**Item 3 is not fixable at the call site**: `readFile` returns a bare bool, and "returned false AND
armed nothing" — the condition criterion (e) was phrased against — is never communicated to the
caller. It needs a contract decision (**DEC-033**): widen `readFile` to a tri-state/out-param across
~11 overrides; or make GarminConnect return true on the paths where it emitted (touching REQ-017/023
and their tests); or a deferred watchdog that advances only if no completion arrived. Also folded into
DEC-033: **B-R027-02**, the caller-allocated `QByteArray` (`syncNext:1985`, `downloadNext:2127`,
freed only in `completedRead:2191/2219` / `failedRead:2307`) which the briefed branch would have
leaked once per silent refusal, and **B-R027-03**, the requirement that any fixture return false
AFTER queueing a completion rather than emitting nothing.
**DEC-032 STANDS UNCHANGED for scope items 1, 2 and 4** — the in-loop `continue` (B-R026-01), the
S-R027-01 abort re-read, and the batch-generation rider. None of the three depends on the `readFile`
bool. Criterion clause (e) is DEFERRED to DEC-033 with REQ-027 clauses (a)-(d) and (f) unchanged.
Two riders adopted from the builder's report: **B-R027-04** — `uploadNext`'s parse-failure branch
gets the same `++downloadcounter` as `syncNext`'s, so the fix does not create a new divergence
between the two loops it exists to converge; and **B-R027-05** — criterion (a)'s `downloading == false`
is unreadable (private, `CloudService.h:466`) and is replaced by the tail-EXCLUSIVE proxies
(`progressLabel` :2097, `downloadButton` :2086, checkboxes :2092-2095), **recorded as a proxy**.
Scope item 4 is UNBLOCKED by this amendment: with item 3 gone, the `continue` sites are known and
finite, so the generation guard's coverage is fully specified. The builder additionally argued from
the code (not measured) that the double-click IS deliverable in the existing harness, so TEST-100 is
writable and must NOT be recorded as a dead guard without a measurement.

---

## DEC-033 — The `readFile` bool contract: `false` does not mean "armed nothing" (REQ-027 (e))
- Status: accepted (Option A — widen the contract via a defaulted out-param)
- Decided / last-reviewed: 2026-09-03
- Serves: REQ-027 clause (e); closes B-R027-01 (double-drive), B-R027-02 (buffer leak), B-R027-03
  (fixture-honesty gap); constrained by DEC-022/023 (writeFile double-drive precedent), DEC-024 (BlockingCall
  depth), DEC-025/026 (guard placement), DEC-030 (synchronous parenting), DEC-031 (reaper drains on last
  counted frame), DEC-035 (guard ordering), DEC-036 (buffer-identity contract)

**Problem, as scoped by DEC-032's 2026-08-13 AMENDMENT (above):** `GarminConnect::readFile` has eight
`return false` sites (`GarminConnect.cpp:452,474,493,519,540,557,572,588`); seven post a completion via
`Qt::QueuedConnection` (`postReadComplete`/`postReadFailed`, `:765-770`/`:780-785`) on the line above before
returning `false` — only `:452` is genuinely silent. `syncNext` (`CloudService.cpp:1996`) and `downloadNext`
(`:2135`) both DISCARD the bool, so a caller branch treating every `false` as "armed nothing" double-drives
the loop on ~11 majority-shape integrations. Also folded in: B-R027-02 (the caller-allocated `QByteArray`
leaks on a naive branch) and B-R027-03 (a test fixture must return `false` AFTER queueing a completion, or a
green slot proves nothing about production).

**Research — `qgdw-scout`, 2026-09-03, grounded (spot-checked by the orchestrator: base virtual signature at
`CloudService.h:263-265` and all 10 non-GarminConnect override sites confirmed by `grep -rn '::readFile'
src/Cloud/*.cpp`; TEST-022's exact assertion text confirmed in `testGarminConnectReadFile.cpp`):**

**Option A — Widen the contract: defaulted out-param on the base virtual.** R:5 S:5 M:4 BP:5. Only
`GarminConnect.cpp` gains real branching logic (its 7 "return false after arming" sites); the other 10
overrides (`LocalFileStore.cpp:129`, `PolarFlow.cpp:184`, `Strava.cpp:281`, `SportTracks.cpp:275`,
`Xert.cpp:371`, `SixCycle.cpp:349`, `Azum.cpp:266`, `CyclingAnalytics.cpp:283`, `Dropbox.cpp:232`,
`Nolio.cpp:221`) are mechanical signature edits with no behavior change, since the param defaults to their
existing "false=silent" shape. `syncNext`/`downloadNext` each gain one branch reading the out-param instead
of discarding the bool (closes B-R027-01), which also deletes the buffer before advancing (closes B-R027-02).
Zero existing TEST assertions change value (TEST-020..026, TEST-065(a)/(b), all 6 `driveRefusal()` sites).
Cheap to reverse (additive; drop the param + 2 branches).

**Option B — Normalize GarminConnect: flip the 7 "armed" sites to return `true`.** R:3 S:4 M:2 BP:3.
Cheapest by line count but inverts a deliberately-designed test doctrine ("Reporting is not permission",
`testGarminConnectReadFailure.cpp:227-236`) across 6 call sites plus TEST-022 and TEST-065(a)/(b) — a live
behavior-contract reversal, and possibly non-compliant with REQ-017(a)'s "readFile fails" wording (ambiguous
bool-vs-signal). Expensive to reverse post-ship.

**Option C — Deferred watchdog timer.** R:2 S:3 M:2 BP:2. Solves a problem dormant everywhere except one
already-unreachable base-class path (`Withings`, whose `readdir` is unimplemented); races a queued signal
that's already delivered deterministically on the caller's own `processEvents()`. Reintroduces the
timing-sensitive failure class DEC-035's own research characterized as "best-effort by construction" and
that cost a full stop-and-report cycle elsewhere (A3-R027b-F1/ORCH-017). Cannot close B-R027-01 standing
alone — needs A or B's branch-level fix underneath it. One-way door in practice once woven into the
DEC-024/025/030/031 guard stack.

**Chosen: Option A**, by user decision 2026-09-03 (Three Options Doctrine, presented by the orchestrator).
Narrowest logic footprint, zero existing-test churn, and makes the third state visible in the caller's data
instead of trusting the same convention class whose silent breakage (B-R027-01) is why this DEC exists.

**BUILT + execution-verified 2026-09-04 (uncommitted). Build scope, as implemented:**
- `CloudService.h:263-265` — base virtual `readFile` gains a defaulted out-param (richer small enum
  `ArmedNothing | ArmedCompletion`, preferred over a narrow bool per the scout's open question, so a future
  12th override that needs the same split does not force widening the signature again).
- 10 overrides (listed above) — signature-only edits, default value, no logic change.
- `GarminConnect.cpp` — the 7 "armed" `return false` sites (`:474,493,519,540,557,572,588`) set the out-param
  to `ArmedCompletion`; `:452` stays `ArmedNothing` (default).
- `CloudService.cpp` `syncNext`/`downloadNext` — read the out-param instead of discarding the bool; on
  `ArmedNothing`, label+advance+continue and `delete data` (closes B-R027-02); on `ArmedCompletion`, do
  nothing (the queued completion will drive the loop, as today).
- `testGarminConnectSyncDialogClose.cpp:1077-1131` (the fake store's `readFile`) — **CORRECTED 2026-09-03,
  reviewer catch (Codex) before any build landed: the bullet below previously named only ONE fixture mode and
  conflated it with the wrong shape.** The fake store currently has NO refusal mode at all — its only
  `return false` (`:1103`) is an unrelated UAF canary marker, per the scout's own verification. **Two
  distinguishable new modes are required, not one:**
  - **Mode 1 (clause (e), `ArmedNothing`)** — returns `false`, emits nothing. This is the genuinely-silent
    shape the O-R027-01 clause itself is testing (only `GarminConnect.cpp`'s one silent site, `:452`,
    behaves this way; most services' base-default shape also matches it).
  - **Mode 2 (B-R027-03, `ArmedCompletion`)** — returns `false` **AFTER queueing a completion** (a real
    `readComplete`/`readFailed` emission, delivered the same way GarminConnect's own 7 "armed" sites do via
    `Qt::QueuedConnection`). **This is the mode that actually matters for B-R027-03 and for proving
    `syncNext`/`downloadNext`'s new branches are correct**: it is the majority-shape scenario the original
    (withdrawn) DEC-032 scope item 3 would have mishandled as a double-drive, and a fixture that only offers
    Mode 1 would let a test go green without ever exercising that path — exactly the concealment B-R027-03
    names. **Do not build Mode 2 as "false + emits nothing" — that is Mode 1 again, and would silently retest
    clause (e) instead of covering the real bug.**
- `CloudServiceAutoDownload::run` (`CloudService.cpp:4147-4164`) — confirmed OUT of scope: it blocks on both
  signals with its own watchdog and never branches on the bool; structurally immune to B-R027-01.

**Alignment probe (run before any verdict is recorded as built, per [[LSN-045]]):**
```
grep -c 'ArmedNothing\|ArmedCompletion' src/Cloud/CloudService.h    # expect 0 before, >=2 after
grep -n 'readFile' src/Cloud/CloudService.cpp | grep -c 'ArmedCompletion\|ArmedNothing'  # expect 0 before, >=2 after (syncNext + downloadNext branches)
grep -c 'ArmedCompletion' src/Cloud/GarminConnect.cpp                # expect 0 before, 7 after
```

**Open questions, deferred to the builder briefing, not blocking the decision itself:** the exact enum name
and member spelling; whether `CloudServiceAutoDownload::run`'s exclusion needs a code comment citing this DEC
so a future editor does not "fix" it into scope by mistake.

---

## DEC-035 — Where the abort guard lives: co-locate it with the irreversible call (REQ-027)
- Status: accepted (B — guard the irreversible call)
- Reversibility: cheap (two guarded returns in one file; no schema, no interface, diff-sized revert)
- Decided / last-reviewed: 2026-08-15
- Serves: REQ-027(d); raised by the A3-R027b-F1 diagnosis
- Constrained by: DEC-032 (the shape it converges on), DEC-030/DEC-031/DEC-025 (lifetime machinery — deliberately untouched), REQ-026 (re-entrant abort logic — untouched)
- Dependents: TEST-105, TEST-106

### The question
`QApplication::processEvents()` is not a guaranteed drain — it is one non-blocking
`g_main_context_iteration`, and whether a pending queued call is dispatched inside it is not a
function of the queue's contents. Measured this week: same binary, same machine, outcome varied by
QPA backend. So DEC-032's completion-slot guard is **best-effort by construction** — it stops the
batch whenever the abort has landed and cannot when it has not.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A accept best-effort, reword REQ-027(d) only | 3 — no regression risk, but leaves `downloadNext` with zero abort reads | 5 — zero marginal cost | 5 — no new code in an already guard-dense class | 2 — leaves the criterion asserting a guarantee the measurement contradicts |
| **B co-locate the read with the irreversible call** | **4 — makes the "nothing pumps between check and call" invariant LOCAL, and on the download path adds the ONLY abort read in the loop** | **4 — one branch per row, same cost shape as the existing :2088 guard** | **3 — grows the guard count in a class already flagged for proliferation; needs comments + mutation-proof tests** | **4 — converges on the shape DEC-032 already accepted for the upload path rather than inventing a third idiom** |
| C bounded drain loop at every `processEvents()` | 3 — improves the odds, guarantees nothing, adds a new failure mode | 4 — bounded per call but runs per row; real UI stall at high row counts | 2 — a cap value nobody can re-derive in six months | 2 — leans on the exact API Qt's docs say to avoid, and reopens DEC-031's frame-counting reasoning |

### Why B
**The asymmetry is the argument, and it was measured, not assumed.** Abort reads per function:
`uploadNext` **2**, one of them immediately before its irreversible `writeFile` (S-R027-01, :2507);
`syncNext` **2**, but NEITHER before its `readFile` at :2011; `downloadNext` **0 — none at all**.
`downloadNext` issues a third-party download with no abort read anywhere in the function, and the
only guard protecting it sits in a DIFFERENT function, across a return and a call. The production
comment at :2367 already states this. So B is not merely "make an implicit invariant local" — on the
download path it adds the first abort read that exists in that loop, closing the upload/download
asymmetry that S-R027-01 closed on one side and never closed on the other.

**What B honestly does NOT buy.** It does not stop an abort the user presses one microsecond later,
and it does not close the `processEvents` delivery gap — nothing at this layer can. It buys locality
(the invariant becomes self-evident at the call site instead of true-by-luck across two functions)
and it forecloses a class of future silent regressions where someone inserts a pumping call between
the last check and the transfer. Claiming more than that would repeat the exact mistake this wave
just fixed in TEST-102's comment.

### Cascade
Touches `syncNext:2011` and `downloadNext:2225` only. Additive — does not replace DEC-032's
`continue`/`batchGeneration` logic, does not touch DEC-030/031/025's lifetime machinery or REQ-026's
re-entrant abort logic, so none of them is reopened and none needs re-verification. One fix serves
all ~16 sibling services through the shared dialog; no per-service work, consistent with DEC-032's
own binding rationale. Requires **TEST-105 and TEST-106**, mutation-proof, so the next maintainer
cannot mistake these guards for redundant with the completion-slot checks a few lines away — this
ledger has already had one wave where load-bearing guards were nearly deleted as "dead code" on the
strength of a surviving mutant ([[LSN-053]]).

### Recorded gaps
- **Real-user click delivery is UNVERIFIED here.** How a click travels from the window-system socket
  to `aborted=true` at Qt 6.8.2 could not be sourced or measured — the qpa private headers are not
  installed. Every claim touching it is labelled unverified rather than asserted, per this ledger's
  standing rule that an unsourced Qt semantic is worse than an admitted gap.
- **Interrupting the in-flight request itself** — the only path that would change the real guarantee
  rather than its locality — needs a cancel-capable contract across `CloudService` and touches all
  ~16 siblings' implementations. Explicitly out of scope; named here so it is a deliberate omission
  rather than an oversight.
- **The scout's proposed REQ-027(d) rewording was checked and NOT made — it was unnecessary.** The
  draft's open question asked whether (d) should change from "stops the batch" to "stops the batch
  once delivered". Reading (d) verbatim, it already says *"an abort **delivered by** the
  `processEvents()` inside the parse-failure branch stops the batch"* — the conditional is already
  there and was there from the start. Amending it would have been churn dressed as rigour. Recorded
  because a recommendation that survives into the ledger unchecked is how a criterion silently drifts.
  What IS added is a new clause (f) for the download path this DEC newly guards.

---

## DEC-034 — Who owns a row across a nested event loop: make the REBUILD observable (REQ-028)
- Status: accepted (C — list generation counter)
- Reversibility: cheap (one member + six compares in one file; no schema, no interface, no subclass API change; revert is the diff)
- Decided / last-reviewed: 2026-08-16
- Serves: REQ-028; raised by A3-R027-F2 (BLOCKING) / F3 / F8, cycle `A3/REQ-027 (2026-08-14)`
- Constrained by: DEC-032 (whose idiom this reuses), DEC-031 (blockingCallDepth must stay complete — this adds no suspension point), DEC-035 (guard-above-the-irreversible-write ordering), DEC-030/DEC-025 (lifetime machinery — untouched)
- Dependents: TEST-107, TEST-108, TEST-109, TEST-110; S-R028-01 (tracked, NOT closed by this DEC)

### The question
Three of this project's decisions — DEC-025, DEC-030, DEC-032 — each enumerated the set of things
that must survive a suspension, and all three enumerated `this`, `store` and `context`. **None ever
enumerated the ROW.** `refreshClicked` deletes every `QTreeWidgetItem` in all three lists and rebuilds
them; both transfer loops capture `curr` before `openRideFile`'s nested loop and use it after. That is
a live heap-use-after-free on unmodified production (PROBE-A), and REQ-027 widened it from a read to a
write. So: who owns a row across a suspension, and what must a driver do when its collection has been
rebuilt underneath it?

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A refuse at the mutator (`refreshClicked` stands down while a driver is live) | 3 — kills the UAF with the smallest diff, but the row stays an unowned raw pointer and any future list mutator silently reopens it | 4 — O(1), indifferent to list size | 4 — one guard, one function — but the predicate must be `downloading \|\| blockingCallDepth > 0` or it is defeated outright | 3 — answers "who owns the row" with "nobody, and we forbade the one thing that noticed" |
| B value addressing (driver holds a KEY, tree owns the item) | 5 — changes the failure CLASS: a missing row becomes a defined stand-down instead of a dangling pointer; the only shape robust to a mutator nobody has thought of | 3 — O(N) per suspension and per completion; at 100x scale needs a hash, which reintroduces the coherence problem | 2 — largest diff (~10 sites) in the most comment-dense function family here; adds a second coordinate system beside `listindex`, which stays positional | 4 — "hold a key, not a pointer, across a yield" is the correct consensus answer, but it touches the most shared code |
| **C list generation counter** | **4 — closes all three briefed axes at their stated roots and fails SAFE (stand down); but validates the CONTAINER, not the element** | **4 — two int compares per suspension, independent of list size** | **5 — DEC-032's existing idiom applied a second time: no new concept, no helper, guards adjacent to the ones already there** | **4 — epoch counters are the standard answer to "the collection changed under my iterator", and this project has already accepted the pattern by decision** |

### Why C
**It is the only option that closes all three briefed axes while satisfying every binding constraint
at once**: it adds no suspension point (DEC-031's `blockingCallDepth` completeness untouched), no new
`tr()` string across ~16 sibling services (the translation cost DEC-032 explicitly refused to reopen),
no subclass API change, and it places guards above the irreversible writes per DEC-035.

**Two verified facts decided it against A**, both confirmed by the orchestrator at their cited lines
before this was recorded:
1. **A's briefed predicate is defeated.** The stub proposed gating on `downloading`. But
   `downloadClicked`'s abort branch sets `downloading=false` at `:1915` *while the driver frame is
   still suspended inside `openRideFile`*, and that frame then resumes and WRITES
   `curr->setText(7, tr("Aborted"))` at `:2153`. The button re-enables inside the exact window
   DEC-035 widened from a read to a write. Not re-enabling there is worse: `downloading=false`
   otherwise occurs only at :2240/:2352/:2693, so Refresh would stay dead forever after any abort.
2. **A is untestable in this repo, and would sit behind a green suite proving nothing.**
   `refreshClicked` is a directly-callable slot (`CloudService.h:425`); the harness invokes
   `dialog->refreshClicked()` **19 times** and references `refreshButton` **zero** times. A
   `setEnabled(false)` fix is invisible to every existing and plausible test here. Testing it would
   need `QTest::mouseClick` under offscreen/minimal — the delivery-reliability minefield LSN-062 /
   A3-R027b-F1 / ORCH-017 spent two cycles on.
3. **A also guts TEST-091.** That fixture drives `refreshClicked()` from inside `readFile`'s loop
   *specifically* so its readdir nests a COUNTED frame inside an UNCOUNTED one — B-R031-01's whole
   premise. Refusing the Refresh means the inner readdir never happens and TEST-091 silently becomes
   a test of the new guard. **C preserves that geometry**: `refreshClicked` still runs, still enters
   readdir, still nests the frame.

**Why not B, which scores higher on reliability.** B is the better answer on the merits and is the
only shape that survives S-R028-01 (reordering). Its price is a column-1 uniqueness audit across ~16
subclasses — the sibling-scan shape REQ-024 records as having cost this project three cycles — plus a
rewrite (not an amendment) of DEC-032's rationale, since that comment names `child(listindex-1)`
addressing as the harm and B deletes the harm. For a BLOCKING live UAF, C buys most of B's safety for
a fraction of the blast radius. **User chose C with S-R028-01 tracked rather than folded in.**

### Stated honestly: what C does NOT do
- **It validates the CONTAINER, not the ELEMENT.** C is sound only while whole-list rebuild is the
  only way a row dies — true today (`:1532`/`:1539`/`:1544` are the only tree-item deletes in the
  file), but that is an invariant, not a guarantee. A future mutator that deletes ONE row defeats it.
- **It does not close S-R028-01 (the sort route).** A reorder frees nothing and bumps no counter, so
  it passes every guard C installs. Tracked as an open finding by user decision, not closed here.
- **F8's harm shape is still unmeasured.** `QTreeWidgetItem::child(int)` out-of-range behaviour is
  *unspecified* in Qt's documentation, so the current F8 damage is either a null deref or a silently
  wrong-row label — which need different guards. **The build therefore runs probe-first**, the
  TEST-081 / TEST-089 pattern that changed the answer both times it was used here.

### Suspension set (LSN-060 — stated explicitly, as the lesson requires)
`{ this: QPointer (DEC-025) | context: QPointer (DEC-030) | store: StoreReaper (DEC-031) |
intent: aborted (DEC-032/035) | batch: batchGeneration (DEC-032) | LIST: listGeneration (this DEC) }`.
The row is validated **indirectly**, by proving the container it came from was not replaced.

---

## DEC-036 — Correlating a completion with the transfer that asked for it: the in-flight ticket (REQ-028 clause (c))
- Status: accepted and built on formal reopen (C — explicit operation identity on the ambiguous write channel); verified 2026-08-22 by TEST-124/126/127/128/129, full offscreen 78/0, both historical seeds on both backends, and `garmin-fast` 25/0
- Reversibility: medium-expensive (coordinated shared write-interface migration; no persisted schema or remote artefact changes)
- Decided / last-reviewed: 2026-08-18 / reopened and re-decided 2026-08-22
- Serves: REQ-028 acceptance clause (c); closes B-R028-03, closes B-R028-06 as a side effect, narrows B-R028-05 to its driver half
- Constrained by: DEC-034 (the counter this layers on — NOT reopened), DEC-035 (guards sit at the irreversible call), DEC-032 (stand-down idiom), DEC-031/DEC-024 (reaper + `blockingCallDepth` must be undisturbed), DEC-030/DEC-025 (`QPointer` proves alive, never wanted)
- Dependents: TEST-107 (must be rewritten or retired — it currently pins the defect), TEST-111, TEST-112, new TEST-113/114/115/116; B-R028-04 (untouched, next in the queue)

### The question
DEC-034's `listGeneration` closes the *refresh* route. It cannot close the *abort+restart* route, and
this is provable rather than incidental: **two transfers are outstanding at once**, and the restarted
batch's own dispatch overwrites any shared member before the abandoned batch's completion arrives, so
`listGeneration`, `batchGeneration` and any new scalar all read EQUAL at the compare. Measured, not
argued: TEST-107 is **still green after the Phase-2 fix**, still asserting 4 `readFile` calls for 3
checked rows, `rowsRelabelled == [0]` (the wrong row) and a bar advanced for a batch that completed
nothing. Clause (c) — *"the reader is invoked exactly once per checked row"* — is unmet by measurement.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A in-flight ticket** (dispatch-side identity: buffer pointer for reads, remotename for writes, row POINTER for the label) | **4 — order-independent (identity, not arrival); the token cannot suffer address reuse because a buffer is freed only by its own completion, so stale and live buffers are simultaneously alive. Not 5: the write half's `name` collides when the restart re-dispatches the SAME row and is separated only by the one-shot `armed` bit, which rests on the ≤1-outstanding invariant** | **4 — two compares per completion, O(1) in list size. Not 5: a single ticket slot; concurrent transfers would need a set, which is a redesign** | **4 — the same snapshot-and-compare idiom family DEC-032/034 established, and it REMOVES net surface (seven positional derefs deleted). Cost: armed in one function, consumed in another (three pairs to keep in step)** | **4 — "correlate the response with the request" is the consensus answer for async completion, and it honours the no-subclass-API-change / no-new-`tr()` constraint. Tension: a heap address as a correlation id is unorthodox, sound HERE only because of the freed-exactly-once contract** |
| B per-batch relay (a QObject owns the three connections; abort kills it, so a stale completion is never delivered) | 4 — strongest containment: the slot is never entered, so it cannot mislabel, advance the bar or re-drive. Not 5: it puts object-lifetime management into the file whose last five decisions (DEC-024/025/030/031/035) were all lifetime bugs — the relay can be asked to die with a forwarding frame on its own stack | 5 — per-BATCH, not per-transfer: the only option that stays correct if the dialog ever issues concurrent transfers | 3 — "who handles readComplete" now takes two hops through an object with subtle lifetime rules; the topology is no longer greppable from one `connect` line | 3 — receiver-lifetime cancellation is idiomatic Qt with precedent here (`GarminConnect.cpp:737-743`), but it answers only "was this batch abandoned" and leaves positional addressing — the common root — untouched |
| C key-addressed rows + one-shot permit (DEC-034's rejected Option B resurrected) | 5 — changes the failure CLASS: "row not found" is a defined stand-down; survives reorder, shorter rebuild, late completion, and the single-row mutator DEC-034 admits defeats it | 3 — O(N) scan per completion; at 100x needs a hash, which reintroduces the coherence problem DEC-034 recorded at `:1181` | 2 — largest diff in the most comment-dense function family here; three different key columns for three list shapes, one of which (sync-upload, `:1800-1802`) has NO id column; a second coordinate system beside `listindex` | 5 — "hold a key, not a pointer or an index, across a yield" is the textbook answer and the only one closing S-R028-01. Tension: the user chose the smaller blast radius once already and asked for the sort route to be tracked, not folded in |

### Why A
It is the only option that closes clause (c) on **both** halves *and* deletes the seven unguarded
`child(listindex-1)` derefs (B-R028-06) *and* narrows the sort route, while changing **no** signature,
**no** service and **no** connection topology — the constraint the user has enforced twice
(`decisions.md:1186-1188`, `:1214-1215`). It buys most of C's safety at roughly B's diff size.

**Two facts verified at their cited lines decided it, both by the orchestrator rather than read from
the scout's report:**
- **The identity is already on the wire, on both halves.** `writeComplete(QString id, QString message)`
  (`CloudService.h:243`) has always carried an id; `completedWrite` (`:2831`) merely leaves the
  parameter unnamed and discards it. 10 of the 11 write services pass `replyName(sender())` after a
  `mapReply(reply, remotename)`, so the id IS the remotename the dialog passed. **The 11th,
  `LocalFileStore`, emits `""` — but emits it SYNCHRONOUSLY inside `writeFile`
  (`LocalFileStore.cpp:155-186`), so its completion can never be late and the empty-id gap is
  unreachable on this route.** (The contrary claim in B-R028-03 was false and was corrected → ORCH-027.)
- **The read buffer pointer survives even the most transformative service.** `Strava::prepareResponse`
  (`Strava.cpp:869`) mutates the buffer IN PLACE (`data->clear(); data->append(...)`, `:989-990`) and
  returns **the same pointer** to `notifyReadComplete` (`:528`). 11/11 read services hand back the
  caller's pointer.

### What A explicitly does NOT close
- **The sort route's DRIVER half (B-R028-05 / S-R028-01).** Labelling through the stored row POINTER is
  address-based, and TEST-112 measured that a sort permutes positions while item addresses are stable —
  so the correct row is labelled even mid-permutation. But `for (int i=listindex; …)` at `:2013/:2321/:2681`
  stays positional, so a sort inside the nested loop can still re-visit a transferred row and skip one.
  **User decision 2026-08-18: take what A gives; the driver half stays tracked as an open finding.**
  Closing it is C's job, and C is the deliberate end-state when the sort route is scheduled.
- **B-R028-04** (the Refresh-mid-batch wedge: `downloading` stays true, the button still reads "Abort").
  That is DEC-032's idiom shared across ~16 services and is next in the queue, not a drive-by edit here.

### The two invariants A makes load-bearing (must be discharged BY THE BUILD, not assumed)
1. **`readComplete` returns the pointer `readFile` was given.** True at 11/11 today, but the contract is
   **unwritten** at `CloudService.h:145` — it is written only for `readFailed` (`:160-161`). A makes it
   load-bearing, so the build must write it down beside the existing one and assert it.
2. **At most one transfer is outstanding per live batch.** Holds today because each driver dispatches
   and returns immediately (`:2288/:2224/:2401/:2755`). If it can ever exceed 1, the single ticket must
   become a set and B's score rises. The build must assert `<= 1` across the existing suite.

### Rejected without a slot, and the reason is worth keeping
**Abort-time accounting alone** ("count what is in flight, swallow that many") is **arrival-order
dependent**: with no ordering guarantee across `QNetworkReply`-based services, a live completion
arriving first is eaten by the counter while the stale one is admitted — wedging the live batch. It
also cannot fix the LABEL, since whichever completion is admitted still writes through
`child(listindex-1)`. Its order-independent refinement — a **one-shot permit** — survives, folded into
A as the `armed` bit, where it is paired with identity instead of used alone.

### Suspension set (LSN-060)
`{ this: QPointer (DEC-025) | context: QPointer (DEC-030) | store: StoreReaper (DEC-031) |
intent: aborted (DEC-032/035) | batch: batchGeneration (DEC-032) | list: listGeneration (DEC-034) |
TRANSFER: inflight ticket (this DEC) | ROW: inflight.row, validated indirectly by DEC-034 }`.
This DEC is the first entry that identifies the **transfer** itself; every prior entry identifies a
container or an intent. Note the direction of dependence: **A makes DEC-034 MORE load-bearing, not
less** — the compares at `:2448/:2630/:2844` are what prove `inflight.row` is not dangling, since a
whole-list rebuild (`:1532/:1539/:1544`) is the only way a row dies.

### AMENDMENT 2026-08-18 — the invalidation half (A3-R028-F1 BLOCKING, F2, B-R028-07)
**The entry above specified an ARM/CONSUME protocol and never enumerated INVALIDATION, and that omission is a
live heap-use-after-free.** The ticket is armed at four sites, consumed at three, and invalidated nowhere, while
`batchListGeneration` IS re-synced at every batch start (`:1951`) — so **a batch start that dispatches nothing hands a
stale ticket a freshly-valid generation stamp**, and the abandoned batch's completion then passes every guard in the
file and writes through a row `refreshClicked` already freed (`:1542` → `:2654`). Four ordinary clicks reach it:
Download → Refresh → Abort → Download.

**This is a severity RAISE that Option A itself caused, and the trade was visible in the entry's own text.** Replacing
`child(listindex-1)` with a held `inflight.row` moved this route from a MISLABEL (positional addressing re-reads the
rebuilt list, so it is never dangling) to a USE-AFTER-FREE. The DEC closed the row's lifetime on the routes it
enumerated; the route it did not enumerate is the one where the row dies and the ticket outlives it.

**Amendment (not a reopen — "ARMED IS ONE-SHOT" stands, it was simply incomplete):** the ticket is invalidated at
**two** further sites.
1. `inflight.armed = false;` in `downloadClicked`'s START branch, beside `batchListGeneration = listGeneration;`
   (`:1951`). Safe because that branch runs only when `downloading == false`, i.e. no live transfer belongs to this
   dialog. It disarms **before the restarted batch's first suspension**, which closes B-R028-07's `openRideFile`
   window, and **before any restart that arms nothing**, which closes A3-R028-F1 and F2.
2. `inflight.armed = false;` beside `listGeneration++` in `refreshClicked` (`:1526`). Independently correct — a Refresh
   frees the row the ticket holds, so the ticket is meaningless from that instant — and it makes the dangling
   `inflight.row` **unreachable** rather than merely unread. Site 1 alone closes the known routes; site 2 is what makes
   the invariant hold by construction instead of by enumeration, which is the whole lesson of this amendment.

**The lifecycle rule this DEC should have carried from the start, and which now generalises past it:** any state ARMED
at N sites and CONSUMED at M sites must have its INVALIDATION sites enumerated in the same entry. "Armed at four,
consumed at three, never disarmed" is an incomplete lifecycle, and the gap is invisible to mutation testing — every
guard mutated individually, every mutant died, and the defect was in the site that does not exist.

**Also corrected here (A3-R028-F4):** the entry's claim that `completedWrite`'s `isWrite == false` clause is TRACED
does not survive. `replyName()` never returns `""` for a write reply — every `writeFile` in `src/Cloud/` calls
`mapReply(reply, remotename)` first, at 20 call sites — and the only real empty-id emitter, `LocalFileStore`, emits
SYNCHRONOUSLY from inside `writeFile`, so its completion can never be late. The clause STAYS as defence-in-depth but is
re-labelled **"reasoned, not traced"** alongside its four siblings. The comment at `CloudService.cpp:2999-3010` must be
corrected rather than left asserting a route that does not exist.

### FORMAL REOPEN DECISION 2026-08-22 — Option C, explicit write-operation identity
**Trigger:** A3-R028c-F2 and seed 32 measured two transfers outstanding for one live batch, firing this
entry's own reopen condition. The prior single-slot premise is false; the completion-slot
`batchGeneration` amendment is a necessary resumption guard, not a replacement correlation
architecture.

**Common requirement under every option:** snapshot `batchGeneration` when each completion is admitted,
then re-check it after every suspension and before using a saved row, changing counters or re-driving a
driver. This closes the duplicate-tail route and the stale completion-local row route measured by seed
447. It does not identify two same-named writes and does not preserve correlation through Refresh.

| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A multi-ticket ledger + write-name exclusion | 4 — exact only while colliding write names cannot overlap | 4 — concurrent reads/non-colliding writes | 3 — dialog-local but lifecycle-heavy | 4 — sound backpressure over inferred identity |
| B drain before restart | 4 — quiescence removes ambiguity, but a missing completion wedges restart | 2 — serializes the user behind the slowest transfer | 3 — adds a draining UI/state machine | 3 — legitimate but poor recovery UX |
| **C explicit write-operation identity** | **5 — same-name writes and reversed delivery remain distinguishable** | **5 — immediate restart and future concurrency** | **2 — broad coordinated migration, simple steady-state model** | **5 — request/response identity is carried explicitly** |

**User decision 2026-08-22: C.** Allocate an opaque operation id at every write dispatch, carry it
through `writeFile` and return it with `writeComplete`, and key the dialog's write records by that id.
The read channel keeps its existing buffer-pointer identity; it does not need an API migration.
Refresh makes an outstanding record row-free but preserves its id until completion, so a late result is
consumed without touching a destroyed row. No correctness-degrading eviction cap is permitted; records
live until completion or dialog destruction.

**Verified migration surface:** 11 write-service override declarations and definitions; 24 write
completion emission sites; the shared `writeFile`/`notifyWriteComplete`/`writeComplete` API; three
`writeFile` callers; and the Upload-dialog and Sync-dialog consumers. The operation id is local metadata:
it does not alter a remote filename or stored user data.

**Cascade:** supersede DEC-037's receiver-side name matching, cap and accepted result swap; make
TEST-124 require exact result-to-producing-row association in both arrival orders; expose operation
record conservation to the oracle; add the Refresh row-free-record route; run every target compiling the
shared fake and both registered QPA backends. The separately known sort-route driver defect remains a
following stable-worklist slice. That sequencing accepts no residual: REQ-028 clause (c) remains
explicitly partial until the sort slice also lands.

**AMENDMENT 2026-08-23 (VAL-018 finding 1) — THE ANTECEDENT OF THE SENTENCE ABOVE HAS BEEN MET, so the
sentence no longer describes the present.** "Until the sort slice also lands" was written on 2026-08-22
while the sort slice was still a following item. It landed the next day: DEC-038 (Option A, sorting off
for the batch's duration), committed `514d8e88f`, orchestrator-mutation-proven, 89/0/0 on both QPA
backends, adversarially cleared by the DEC-038 A3 (findings, none blocking). **REQ-028 clause (c) is
therefore CLOSED, not partial**, and `prd.md:154` plus the REQ-028 row in traceability.md — both of which
already said so — are the SSOT. The original sentence is retained above rather than rewritten because it
was true when written; this amendment is what makes the entry readable in order. VAL-018 caught it as a
same-day cascade that was resolved in narrative but never reconciled in the dependent decision's own text.

---

## DEC-037 — Separating an abandoned write from the live one when the wire carries no identity (REQ-028 (c), A3-R028b-F3)
- Status: superseded by DEC-036's built and verified 2026-08-22 Option C; the retired-name/capped-ticket implementation is no longer production behavior
- Reversibility: cheap (one member + ~4 edits in one file; no service, no signature, no wire format touched — revert is the diff)
- Decided / last-reviewed: 2026-08-19
- Serves: REQ-028 clause (c) partially met; the accepted same-name result-swap residual contradicts the clause as written, so the former "re-closed" claim is withdrawn; A3-R028b-F3 remains the historical trigger
- Constrained by: DEC-036 (whose ticket SHAPE this supersedes while leaving its arm/consume/invalidate protocol intact), DEC-035, DEC-034, DEC-032, DEC-031, DEC-024
- Dependents: TEST-124, TEST-125; TEST-115's same-row half must be RE-ARGUED

### The question
`CloudServiceSyncDialog::completedWrite` cannot tell an abandoned transfer's completion from the live
one when both carry the same `remotename` — and they do: the Upload list's row and the Sync list's
upload row for one activity both carry `text(1) == ride->fileName` (`CloudService.cpp:1777`, `:1823`),
and both arm sites compute the key identically (`:2393`, `:3053`).

### THE DISPROOF THAT DECIDED IT — no receiver-side scheme can work, and this is why the entry exists
**The emitter is ONE SHARED `store`** — all three connections are made on it (`CloudService.cpp:1067-1072`,
orchestrator-verified) — **and the payload is identical.** `QObject::sender()` inside the slot is the
store, not the reply. Therefore:
- **A per-batch relay QObject does NOT close this**, and DEC-036's research was wrong to describe its
  Option B as closing that family "by construction". Qt severs a connection when a participant is
  destroyed; it does not filter emissions per batch. `downloadClicked`'s START branch creates the new
  batch's relay BEFORE `uploadNext` dispatches, so the abandoned write's `writeComplete` is delivered to
  the LIVE relay and forwarded — identical outcome to today. The `GarminConnect.cpp:737-743` precedent is
  sound for what it does (cancelling a post whose CONTEXT died) and does not transfer, because there the
  emitter is per-object and here it is shared.
- **A per-dispatch sequence number in the ticket does NOT close this** (the A3's suggestion, and the
  orchestrator's first instinct): `writeComplete(QString id, QString message)` (`CloudService.h:271`)
  carries only those two values, so any field stored in the ticket describes the LIVE transfer while the
  stale completion carries nothing to compare against.
- **Folding list identity into `remotename` is DISQUALIFYING:** it is the remote artefact name on the
  wire — `LocalFileStore.cpp:174` writes `path+"/"+remotename` to disk and `Dropbox.cpp:281` puts it in
  the `Dropbox-API-Arg` path — so it is a user-visible behaviour change to all 11 write services.
- **Disarming on abort does NOT close it either**, and the reason the abort branch was left alone is now
  re-derived rather than assumed: `uploadNext` RE-ARMS with the same name, so the stale completion still
  matches; and it would silently delete the "Aborted" label an abandoned write currently writes to its
  own row via the still-armed ticket (`:3212-3213`).
**Only three families survive: retain the abandoned state, forbid the overlap, or guess from arrival
order.**

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A retired-ticket set** (quarantine the abandoned write ticket at restart; match it FIRST) | **4 — order-INDEPENDENT on everything that breaks the invariant: exactly one re-drive, one increment, no swallowed completion, in BOTH arrival orders. Not 5: two same-name rows can exchange result strings under one order, and the retired list holds raw row pointers that the Refresh clear must keep valid** | **4 — O(k), k = writes abandoned since the last Refresh, ≤1 on every route driven in this wave, capped. Not 5: a future concurrent-dispatch design needs the LIVE side to become a set too** | **4 — no new concept; it is the escape hatch DEC-036's own comment names (`CloudService.h:773-775`, "would need a SET here, not a slot"). Cost: that entry's load-bearing paragraph must be REWRITTEN, and invalidation now covers two containers** | **5 — "retain an abandoned request's correlation state until its response lands" is the consensus answer for a shared-emitter channel with no per-request identity, and it violates none of the standing constraints: 0 of 11 services, 0 of 24 emit sites, 0 signatures, 0 new `tr()`, 0 new suspension points, empty ids still accepted** |
| B drain before restart (abort latches `draining`; START refuses until the outstanding write lands) | 4 — zero ambiguity while the drain holds, the strongest correctness story; not 5 because the deadline that prevents a permanent wedge hands the ambiguity straight back when it fires | 3 — serialises the USER: every abort costs a wait bounded by the slowest outstanding upload, and abort-during-a-large-batch is exactly when users abort | 3 — a three-state machine plus a timer in the function family whose last five decisions were all lifetime/re-entrancy bugs; timer-driven tests under `offscreen` AND `minimal` are the flakiest surface in this harness | 3 — "quiesce before restart" is legitimate, but it needs a new user-visible label, i.e. a new `tr()` string across the shared dialog's translations — the exact cost DEC-032 refused — and it DEEPENS B-R028-04 by adding a second state where the button lies |
| C abort-time outstanding counter | 2 — distinguishes by ARRIVAL POSITION, which is an assumption, not an identity: two writes really are outstanding here, as separate replies with no ordering guarantee, and if the LIVE one lands first the counter eats it — the live batch wedges mid-row and the stale one mislabels. **Strictly worse than the current defect, which at least keeps moving** | 5 — one int, O(1) | 5 — smallest possible diff, two sites, trivially reverted | 2 — the shape DEC-036 already rejected on the record (`decisions.md:1297-1303`) for exactly this reason; A3-R028b-F3 is not new evidence for it, and it cannot label the abandoned row at all |

### Why A
The choice was forced by the disproof, not by preference. **C is the option the record already rejected
and this finding does not rehabilitate it.** B is the only option with zero residual, but it buys that
with a new user-visible dialog state, a new `tr()` string across the shared dialog's translations, and
timer-driven tests — three costs the user has refused twice in this feature (`decisions.md:1186-1188`,
`:1214-1215`) — and it makes B-R028-04, the next queued item, harder. **A closes the BLOCKING harm in
both arrival orders by EXHAUSTION rather than by assumption**, and honours every standing constraint
exactly.

### The mechanism (four sites, two files, zero services)
1. `CloudService.h` — `QList<InFlight> retiredWrites;` beside `inflight` (`:786-793`), with a small cap.
   **The paragraph at `:769-775` must be REWRITTEN, not extended:** "a single ticket is enough" is still
   true per LIVE batch, and that is now exactly the point — a transfer can OUTLIVE its batch.
2. `CloudService.cpp:2029` — the START branch retires an armed WRITE ticket before disarming. Safe for
   the same recorded reason that line is already safe: the branch runs only with `downloading == false`.
3. `CloudService.cpp:1559` — `retiredWrites.clear()` beside the existing disarm, above `refreshClicked`'s
   deletes. **MANDATORY, not optional:** every retired ticket holds a raw `QTreeWidgetItem*`, so without
   this line the fix MANUFACTURES a new use-after-free of exactly the A3-R028-F1 shape.
4. `CloudService.cpp:3204` — scan `retiredWrites` oldest-first BEFORE the live compare, with the same
   empty-id allowance the live compare has. On a hit: label that ticket's row, erase the entry, RETURN —
   no counter, no `successful`, no re-drive.

### Exhaustion argument (this is the acceptance evidence, and it must be TESTED in both orders)
`[stale, live]` → stale consumes the retired ticket, labels `R_sync`, does not drive; live consumes the
live ticket, labels `R_up`, counts once, drives once. `[live, stale]` → live consumes the retired ticket
(labelling `R_sync`), stale consumes the live ticket (labels `R_up`, counts once, drives once). **In both
orders `uploadNext` is re-driven exactly once and row 0's completion is never swallowed**, so DEC-036's
invariant at `CloudService.h:769-775` holds.

### Accepted residual, stated rather than hidden
In the `[live, stale]` order **the two result strings land on the opposite rows.** Both rows are the same
activity to the same service, so the strings are usually identical; when they differ the user sees two
labelled rows with swapped verdicts. **This is the honest price of the fact that the wire carries no
identity**, and it is cosmetic and bounded. **User decision 2026-08-19: accepted.**

### Scope: WRITES ONLY (user decision 2026-08-19)
The read channel already has a genuinely unique per-transfer token — the buffer pointer
(`CloudService.h:789`), returned by 11/11 services — so it has no ambiguity to resolve; and the dialog
OWNS the buffer (freed at `:2693`/`:2720`), so a scheme that stops delivering `readComplete` would leak
one buffer per abort. The asymmetry is deliberate and is recorded here so it is not "fixed" later.

### Phase-1 dependency, and the single biggest cost
**The harness cannot currently deliver two write completions in a caller-specified order.** TEST-124 must
run the F3 route TWICE, once per arrival order; that needs a new ordering hook on the fake store. Without
it, A's order-independence is *reasoned* rather than *measured* and must be labelled so under clause (e).

### Cite corrections carried from this research
`writeComplete` is declared at `CloudService.h:271`; the DEC-036 entry cites `:243`, which was correct
when written and drifted when this wave added ~149 lines to that header ([[LSN-034]], recur:9). The
`InFlight` struct is at `:786-793`; `:724-785` is its comment block.

### AMENDMENT 2026-08-21 (A3-R028c-F1, BLOCKING) — the Refresh clear was costed for safety and never for what it DESTROYS
`refreshClicked` disarms (`:1559`) and clears `retiredWrites` (`:1579`) but **never retires**; the only
retire site is `downloadClicked`'s START branch (`:2081-2084`). So on **Abort → Refresh → restart** the
abandoned write's ticket is discarded before anything can retire it, the scan at `:3447` finds nothing,
and the stale completion falls through to the live compare and is ACCEPTED — **re-opening A3-R028b-F3 in
full**, on a five-click SINGLE-TAB route that needs no cross-list name coincidence at all.
**The clear itself is right and stays** — TEST-125 proves it, and the A3 refuted any suggestion of
removing it. What this entry got wrong is that `decisions.md:1413-1415` justified it as "MANDATORY" on
**memory-safety grounds only**. Nobody asked what it costs. **The row must die; the NAME must not.**
**Amendment: a ROW-FREE TOMBSTONE.** `refreshClicked` retires the armed write's *identity* while dropping
the pointer that must die — either `row = nullptr` in the retired copy, or a separate `QStringList`. A
tombstone match **swallows** the completion (no label, no count, no re-drive) rather than accepting it.
**Naïvely appending `inflight` at `:1559` would re-manufacture TEST-125's use-after-free**, which is why
the two containers cannot simply be merged.
**Generalised (→ [[LSN-075]]): an invalidation site reasoned purely on memory safety is always CORRECT
and can still be the defect** — the review question "is this safe?" returns yes and stops. Sibling of
[[LSN-068]]: that is a lifetime with no invalidation site; this is an invalidation site with no cost
analysis. Two failure modes of the same enumeration.

### CORRECTION 2026-08-21 (A3-R028c-F4) — this entry's disproof argues from a behaviour it changed
The disproof above rejects disarm-on-abort partly because it "would silently delete the 'Aborted' label
an abandoned write currently writes to its own row". **On the abort-then-restart route — the only route
this entry is about — `:3453` now writes `result`, not "Aborted".** The rejection stands on its other
grounds (`uploadNext` re-arms with the same name, so the stale completion still matches); the *reasoning*
does not. The cite `:3212-3213` has drifted to `:3465-3466`.
**And the code is MORE compliant than this entry assumed.** The A3 refuted B-R028-15: clause (c) asks
that a cell name "the outcome of a transfer that actually happened to that row" — the retired write DID
happen to that row and DID produce `result`, whereas `tr("Aborted")` would name the user's action on a
*different, already-dead* batch. → [[LSN-076]].

### KNOWN GAP, recorded by user decision 2026-08-21 (A3-R028c standing probe)
**This mechanism ships for TEN Upload-capable services and is exercised against ZERO of them.**
`GarminConnect.h:76` is `Query | Download`, so GarminConnect cannot upload and the fake `BlockingStore`
is the only driver any test uses. The wire behaviour this decision's disproof rests on — that
`writeComplete(QString id, QString message)` carries nothing else, and that one shared `store` emits for
every batch — is a **static audit over eleven services, measured against none.** Stated here so the next
cycle attacks a documented gap instead of rediscovering it, and so nobody mistakes the harness's green
for service-level evidence. Driving a real service needs a fixture layer that does not exist;
`LocalFileStore` is the cheapest candidate if that changes (no network, and the only empty-id emitter).

---

## DEC-034 / DEC-036 AMENDMENT 2026-08-21 (A3-R028c-F2, BLOCKING) — the completion slots never got the guard the drivers have
- Status: accepted; folded into DEC-036's Option C build and verified with it (`e48f7d123`, gate `98df50535`)
- Decided / last-reviewed: 2026-08-21
<!-- Status/Decided fields BACKFILLED 2026-08-23 (librarian Job 3): this amendment section carried NEITHER,
     so DEC-015's canonical home was empty for it while the WIKI hub and STATE both narrated its status. -->
`CloudService.h:783-789` states the invariant the single-slot ticket rests on: *"At most one transfer is
outstanding per LIVE batch: each of the four dispatch sites returns immediately after its store call, and
the only thing that re-drives a loop is a completion slot's tail, which has consumed the ticket first.
**Measured, not assumed.**"* **The second clause is false.** The tail consumes the ticket and *then*
suspends in `QApplication::processEvents()` (`:3474` write, `:2986` read, `:3103` failedRead). A
`downloadClicked` delivered into that suspension re-drives the loop itself and arms a fresh ticket — then
the tail re-drives again. `aborted` cannot catch it (the restart just reset it), and **the three
completion slots hold no `batchGeneration` snapshot**. Current-source grep finds seven references in
`CloudService.cpp`: construction, batch start, and the driver snapshots/compares/comment. None is in
`completedRead`, `failedRead` or `completedWrite` (ORCH-032). Measured: two rows reading "Uploading"
simultaneously, three writes for a two-row batch.
**The measurement that "discharged" this invariant was real and its SCOPE was narrower than the claim** —
61 slots, none of which drove a burst into a completion tail. [[LSN-074]]'s corollary was captured in this
same wave and is precisely what failed.
**Amendment: DEC-034's existing mechanism, applied to the three sites that never got it** — a
`batchGeneration` snapshot taken at each completion slot's ENTRY and compared before the tail's re-drive.
**The drivers were given exactly this guard for exactly this two-click burst** (`:2512-2517`, *"Continuing
here would put a second driver on the same list"*); the slots were not. Not a new decision.
**In scope: all three slots.** The shape is line-identical on the READ path (`:2986-3016`) and in
`failedRead` (`:3103`), **and those ARE on the GarminConnect route** — the write path is not.
**Also folded in:** there is no `aborted`/`batchGeneration` re-read between `saveRide` (`:2966`) and
`successful++` (`:2978`), so an abort+restart delivered inside `autoProcess` increments the NEW batch's
counter on the old batch's behalf. Same mechanism, same fix.

## DEC-038 = DEC-034 AMENDMENT 2026-08-22 (S-R028-01 / S-R028-02) — the sort route: sorting is OFF for the batch's duration
- Status: accepted (A — sorting disabled for the batch's duration); BUILT + committed `514d8e88f`
- Decided / last-reviewed: 2026-08-22
<!-- Status/Decided fields BACKFILLED 2026-08-23 (librarian Job 3): this entry carried NEITHER, so DEC-015's
     canonical home was empty for it while the WIKI hub and STATE both narrated its status. -->

**Question.** How is the third list mutator — SORTING — closed, and in what vehicle? A re-sort frees
nothing and bumps no counter, so it passes DEC-034's `listGeneration` guard and DEC-036's ticket **by
construction**; DEC-036 closed only the LABELLING half (completions address through a stored row POINTER,
which survives a permutation), leaving the DRIVER half — the positional `for (int i=listindex; …)
child(i)` walk in all three drivers — open.

**VEHICLE: an AMENDMENT to DEC-034, not a new REQ.** The user's rule of 2026-08-17 was *"if it reorders,
DEC-034 reopens with executed evidence"*. Both halves of that antecedent are now met: the reorder was
executed by TEST-112 (2026-08-18) and the TRIGGER — the half that was missing — was executed by TEST-131
(2026-08-22). REQ-028 is NOT reopened: `prd.md:154` has said since 2026-08-16 that the sort route is out
of its scope, and it closed on clauses (a)–(e) on 2026-08-22. Precedent for amending without touching the
parent REQ: the DEC-034/DEC-036 amendment of 2026-08-21, immediately above.

**CHOSEN: Option A — disable sorting on all three lists for the batch's duration.** Scored 5/5/5/5
(reliability: removes the mutator rather than tolerating it, adding no new lifetime state; scalability:
strictly cheaper than today, since it also avoids Qt's resort-per-`setText` during the batch;
maintainability: no new members, and it widens an idiom the file already runs at four sites; best
practices: Qt's own documented pattern for bulk-mutating a sorted view).
Rejected — **Option B, pointer-snapshot dispatch** (snapshot the checked rows at batch start; `listindex`
becomes a cursor into that): architecturally the cleaner end state and the one DEC-036's research already
named as *"the deliberate end-state when the sort route is scheduled"*, and the ONLY option that would let
a user re-sort **during** a live batch — but nothing asks for that, it adds a second per-item collection
beside the existing generation/ticket apparatus, and it is fully additive on top of A if that need ever
appears. Rejected — **Option C, per-item dispatched marker with linear rescan**: invents a state shape the
file does not use, is O(n²), and re-runs the stale-marker-across-restart bug class DEC-037 already paid to
learn once.

**WHAT THE PROBE (TEST-131) CHANGED, and why this entry is evidence-led rather than argued.**
1. **The route is LIVE with no click at all.** The scout's suspected self-trigger is CONFIRMED on Qt 6.8.2,
   both backends: a `setText` on the SORT column reorders (a write to a non-sort column does not — Q1c is
   the specificity control). End-to-end (Q5c): the user sorts by Status BEFORE pressing Synchronize — an
   INVISIBLE sort, because every Status cell is empty at that moment (`userSortChangedOrder=0`) — and then
   `syncNext`'s own `setText(7,"Uploading")` reorders the list underneath its own loop at
   `writeFileCalls==0`, giving `writeNames=[…10_00.json.zip , …10_00.json.zip]` (**the same row uploaded
   twice**) and `statuses=["" , "Completed."]` (the other row never transferred). **So B-R028-05's
   unreproducible synthetic `QTest::mouseClick` was never the load-bearing question**, and S-R028-01 is a
   live defect on an ordinary user route rather than a dormant hazard.
2. **The mechanism in the source claim was WRONG, in the unhelpful direction.** The resort is **not lazy**:
   `layoutChanged` is emitted INSIDE `setText` (`at write=1`; a later read adds none). There is no
   scheduled-but-unapplied window. `QTreeWidgetItem::child(int)`'s `executePendingSort()`
   (`qtreewidget.h:145-150`, verified verbatim on disk) is real but does NOT deliver this route — **so any
   design premised on catching the reorder at the addressing call is dead on arrival.**
3. **The orchestrator's own objection to this option was REFUTED by measurement.** The concern was that
   `setSortingEnabled(false)` might sever only the data-change vector and leave the header clickable.
   Measured: `clickable=0`, `indicatorShown=0`, `indicatorReorders=0`. **It closes both vectors.**
4. **There is no pending-sort residual** (Q3): because writes sort immediately, a disable arriving after a
   write cannot undo the move that write already made, and nothing is ever left pending.

**THE ONE HARD CONSTRAINT THE MEASUREMENT IMPOSES.** The disable must be in place **BEFORE the batch's
first `setText`** — placement, not existence, is what makes this guard load-bearing. This is DEC-035's
principle applied to a new mutator.

**THE INVALIDATION-SITE ENUMERATION, made a condition of this decision rather than left to the build
([[LSN-068]]).** Sorting disabled at batch start must be restored on **every** termination path, not only
the three completion tails (`:2511-2513`, `:2644-2645`, `:3257-3258`) and the abort branch (`:1924-1925`)
the draft named. A starting census of early returns inside the three drivers, orchestrator-grepped
2026-08-22 and **explicitly NOT closed** ([[LSN-073]] — the builder re-derives it by grepping, and a census
written as closed is the defect): `syncNext` `:2148` (stale-generation), `:2278` / `:2431` (`self.isNull()`),
`:2456` (`aborted`), `:2463` (`batchGeneration`), `:2497` (`listGeneration != listgen`), plus `:2244`,
`:2321`, `:2355`, `:2365`, `:2388`, `:2406`, `:2504`, `:2531`; `downloadNext` `:2546`, `:2606`, `:2633`,
`:2637`, `:2662`; `uploadNext` `:3081`, `:3134`, `:3145`, `:3153`, `:3167`, `:3183`, `:3271`. **A path that
exits without restoring leaves the user's lists permanently unsortable until the dialog is reopened** —
a silent, shipped UX regression that no memory-safety test would ever catch.
**Corollary, and the obvious wrong answer:** a batch is **not one stack frame**. These loops are
"process one row, return, let the completion re-drive me", so an RAII scope guard around a driver call
would re-enable sorting BETWEEN ROWS and reinstate the defect while looking correct. The disable must
persist across the whole batch and be released only at true batch termination.
**On the `self.isNull()` exits specifically:** the dialog is already gone, so there is nothing to restore
and the restore must be structured so it cannot touch a dead dialog.

**ALSO REQUIRED BY THE USER (2026-08-22), and part of the accepted option:** the restore must **preserve
the user's selected sort column and order** — `setSortingEnabled(true)` re-applies the current indicator,
so the pre-batch column/order must survive the round trip rather than silently resetting to Qt's default
(measured at Q1a: `setSortingEnabled(true)` alone establishes section 0, DESCENDING).

**Cascade.** `CloudService.h:733-742` and `:795-803` currently state, in production comments, that the sort
route is NOT closed — both become stale on this build and must be rewritten, not left. S-R028-01 and
S-R028-02 close. B-R028-05's DRIVER half closes; its LABELLING half stays closed by DEC-036. DEC-036/037
mechanisms are untouched — this amendment lives in the sorting toggles and the driver exits, adjacent to
but not overlapping the ticket's arm/consume sites. TEST-112 stays as the historical pre-fix measurement;
TEST-131's Q5c pins are MEASURED-NOT-DESIRED and **will go RED when this fix lands — that is the marker
the fix worked**, and whoever builds it must rewrite that block and say so.

**Accepted residual.** This is PREVENTION, not TOLERANCE: a future caller that re-enables sorting mid-batch
reintroduces the defect with no architectural guard to catch it (Option B is what would make the drivers
intrinsically reorder-tolerant). Accepted because no such caller exists today (grep-confirmed) and B
remains additive later. Second residual: the header is genuinely inert during a batch — a deliberate UX
cost, not a defect.

---

## DEC-039 — libusb dependency wiring: complete find_path/find_library for BOTH APIs, on an isolated unmerged branch (ORCH-036)
- Status: accepted (B — complete `find_path`/`find_library` include+link wiring for both the libusb-1.0 and legacy libusb-0.1 branches, implemented on an isolated branch and deliberately NOT merged)
- Reversibility: high (build-definition only; one file, one diff, no product code changed — and it is not merged, so reverting is deleting a branch)
- Decided / last-reviewed: 2026-08-23
- Serves: ORCH-036 (raised at the DEC-038 commit-safety gate — the only gate in this project that runs outside the developer's working tree)
- Dependents: branch `build/orch036-libusb-wiring`; ORCH-036 stays ADVISORY and OPEN until a reconciled change lands

**Problem.** With `GC_HAVE_LIBUSB=ON`, the CMake build offers no complete supported dependency wiring for either libusb API. It has no detection to choose a branch, no include-directory wiring, and no link wiring — zero libusb references across all nine `target_link_libraries` in `src/CMakeLists.txt`.

**Two earlier formulations of this problem were WRONG and are recorded as such** (see [[LSN-080]]), because the corrected statement is what this decision is scoped to:
- *"HEAD does not build from a clean checkout under either option value."* FALSE — an artifact of reconfiguring a build directory that still held moc output from a previous `ON` configure. HEAD's default configuration builds end to end (`[217/218] Linking CXX executable src/GoldenCheetah`, EXIT=0).
- *"HEAD cannot select the libusb-1.0 API under any configuration."* FALSE — `if(NOT LIBUSB_V_1)` reads an ordinary variable and an external `-DLIBUSB_V_1=ON` satisfies it. MEASURED: 426 compile entries carried `-DLIBUSB_V_1`, `EzUsb-1.0.c` compiled, legacy `EzUsb.c` not.

**The accurate defect, measured at `514d8e88f`:** selection PASSES, compilation PASSES, **linking FAILS** — `undefined reference to libusb_exit / libusb_free_config_descriptor / libusb_unref_device` from `LibUsb.cpp.o` and `libusb_error_name` from `EzUsb-1.0.c.o`. Compilation passes because this tree deliberately includes `<libusb-1.0/libusb.h>`, which resolves through the default `/usr/include` search root on a default-prefix Linux install. That resolution does NOT hold for a nonstandard prefix, macOS/Homebrew, or vcpkg, which is why include wiring is a requirement of the repair and not an optional extra.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — complete `find_path`/`find_library` wiring for both branches, isolated and unmerged.** R4 S4 M3 BP4. Fixes both defects; one code path serves Linux, macOS and Windows because `find_path`/`find_library` search the CMake prefix path, which the vcpkg toolchain file populates automatically. M is 3 rather than 4 **because of the ownership constraint, not the code**: `src/CMakeLists.txt` is already dirty under another owner in the main worktree, so implementation there is forbidden and the deliverable is a tested but unmerged branch.
- **A — adopt the uncommitted `pkg_check_modules` auto-detection.** R3 S2 M2 BP2. **Rejected:** cannot be implemented at all without absorbing another owner's uncommitted work, which the user explicitly forbade; pkg-config needs an executable on PATH that MSVC toolchains do not ship, leaving Windows/vcpkg unserved; and its `pkg_check_modules(LIBUSB0 REQUIRED libusb)` fallback hard-fails configure on a legacy package distributions are actively retiring. Revisitable once ownership is reconciled — it is the nicer long-term UX and can supersede B.
- **C — do nothing; the default is OFF and the feature is opt-in.** R2 S1 M3 BP2. **Rejected:** this is the exact trade DEC-028 already refused — it leaves TRACKED source (Fortius/Imagic) unbuildable from clean, and `vcpkg.json:50-55` already declares a `libusb` feature and `INSTALL-CMAKE:44` already advertises libusb as a supported optional library, both of which C leaves false.

**Ownership boundary (a first-class term of this decision, not a footnote).** Implemented on branch `build/orch036-libusb-wiring` in a separate worktree created from `514d8e88f`. The main worktree and the `cleanhead` evidence worktree were not modified. **The result must NOT be merged or cherry-picked until the existing owner of the uncommitted `src/CMakeLists.txt` changes has reconciled the overlap** — both diffs edit the same block, and that reconciliation is theirs to make, not this decision's to pre-empt. A likely resolution is that they drop their now-redundant detection in favour of this wiring, or that this branch is rebased onto their landed work; either is their call.

**Evidence bar — EXECUTED, fresh build directory per configuration (never a re-configure, per [[LSN-080]]).**
- v1-ON (`-DGC_HAVE_LIBUSB=ON -DLIBUSB_V_1=ON`): configure EXIT=0, discovered `/usr/include/libusb-1.0` + `/usr/lib/x86_64-linux-gnu/libusb-1.0.so`; build **EXIT=0**, zero FAILED, zero undefined references; **`ldd` shows `libusb-1.0.so.0 => /lib/x86_64-linux-gnu/libusb-1.0.so.0`**, which is what distinguishes "compiles" from "linked against the intended API"; `EzUsb-1.0.c` compiled (3 entries), legacy `EzUsb.c` not (0).
- Legacy branch with the dependency absent (`-DGC_HAVE_LIBUSB=ON`, no `LIBUSB_V_1`, and this machine has no `usb.h`): configure **EXIT=1** with a diagnostic naming both missing artefacts and the remedy (`-DLIBUSB_V_1=ON`, install `libusb-dev`, or `-DGC_HAVE_LIBUSB=OFF`). This discharges the requirement that a missing dependency fails at CONFIGURE time rather than several minutes into a link.
- OFF default: configure EXIT=0, `GC_HAVE_LIBUSB:BOOL=OFF`; build result recorded in traceability.

**Accepted residuals — UNEXECUTED, and labelled so deliberately. Code inspection is NOT verification and must not be described as multi-platform testing.**
1. **Legacy libusb-0.1 SUCCESSFUL build: NOT EXECUTED.** Only its failure path is executed. This machine has no libusb-0.1 headers, so a successful legacy configure+build+link was never observed. Requires a container/VM with `libusb-dev` present and `libusb-1.0-0-dev` absent.
2. **macOS / Homebrew: NOT EXECUTED.** No macOS host available in this session. The prefix layout (`/opt/homebrew` on Apple Silicon vs `/usr/local` on Intel) is precisely the case the include wiring exists for, and it is exactly the case not yet proven.
3. **Windows / vcpkg: NOT EXECUTED.** The reasoning that `find_path`/`find_library` resolve through the vcpkg toolchain's prefix path is sound and is why B was chosen over A, but it is REASONED, not measured.
4. The `GC_LIBUSB_*` variables are set inside the `if(GC_HAVE_LIBUSB)` block and consumed under the same guard in the link section — correct, but it is a convention a future edit could break silently.

### Alignment probe
grep -n 'find_path(LIBUSB1_INCLUDE_DIR\|find_library(LIBUSB1_LIBRARY' src/CMakeLists.txt   # expect both present (v1 detection)
grep -n 'find_path(LIBUSB0_INCLUDE_DIR\|find_library(LIBUSB0_LIBRARY' src/CMakeLists.txt   # expect both present (legacy detection)
grep -n 'GC_LIBUSB_INCLUDE_DIRS\|GC_LIBUSB_LIBRARIES' src/CMakeLists.txt                    # expect set in both branches + consumed once in the link section
git branch --list build/orch036-libusb-wiring                                               # expect the branch to EXIST and be UNMERGED
git log --oneline master..build/orch036-libusb-wiring                                       # expect commits present, i.e. not merged

## DEC-040 — Bounding every blocking provider wait: a shared bounded-request outcome contract, then generic cooperative cancellation (W3), on an S-1 base-owned injectable QNetworkAccessManager

**Status:** accepted 2026-08-26. **NOT BUILT.** Briefing prepared; no builder dispatched. Supersedes nothing; amends DES-001 invariant 3.

**ID NOTE — READ THIS BEFORE CITING.** Through the 2026-08-26 design gate the *parked auto-downloader teardown draft* was styled "DEC-040" in session narrative. It was never allocated. This id belongs to the PROVIDER-WATCHDOG decision. The parked teardown draft carries **NO id** and is BLOCKED-BY this DEC; it takes the next free number when approved. Cite the teardown work as "the parked auto-downloader teardown draft (A3-R028e-F1)", never as DEC-040.

### Question
Nine providers block in a `QEventLoop` with no watchdog; a tenth arms correctly but never checks whether the reply finished. How are all of them bounded, and how does a bounded wait become cancellable, without manufacturing false-success results?

### Options considered
- **W1 — per-provider in-place repair.** Fix arming order and add a finish check at each of the 22 sites independently. Reliability 3 · Scalability 2 · Maintainability 2 · Best Practices 2. Rejected: writes the same safety-critical idiom 22 times, and nothing structural stops the 23rd site being wrong — which is exactly how CyclingAnalytics (ordering) and SixCycle (missing check) arose.
- **W2 — shared bounded-request helper, no cancellation.** Reliability 4 · Scalability 4 · Maintainability 5 · Best Practices 4. Not rejected — adopted as Stage 1 of W3.
- **W3 — W2 plus generic cooperative cancellation. CHOSEN.** Reliability 5 · Scalability 5 · Maintainability 4 · Best Practices 4.

### Decision
W3, sequenced W2 then W3, as **one decision and one release unit**. Stage 1 establishes the outcome contract and migrates every site; Stage 2 adds cancellation. The parked teardown draft is NOT unblocked when Stage 1 lands — only completed W3 releases it, because Stage 1 alone leaves close latency at one full provider timeout, which is not the property that draft depends on.

### Helper contract
```cpp
enum class RequestOutcome { Finished, TimedOut, Cancelled, NetworkError };
struct RequestResult {
    RequestOutcome outcome = RequestOutcome::NetworkError;
    int httpStatus = -1;
    QNetworkReply::NetworkError error = QNetworkReply::NoError;
    QString errorString;
    QByteArray body;                       // ONLY when outcome == Finished
    bool ok() const { return outcome == RequestOutcome::Finished; }
};
RequestResult CloudService::blockingRequest(QNetworkReply* reply,   // ownership TRANSFERRED
                                            int timeoutMs,
                                            const CancelToken& cancel = CancelToken());
```

**Wait-state:** `naturalFinish_` (set ONLY by the `finished()` slot, ONLY while `!abortIssued_`), `abortIssued_` (set immediately before `abort()`), `pending_` (first writer wins among finished/timer/cancel-poll).

**Strict ordering: reconcile outcome -> snapshot fields -> dispose.** `abort()` is never called before the outcome is fixed, so an abort-induced `finished()` cannot reach reconciliation. `reply->isFinished()` is NEVER consulted for outcome determination; only `naturalFinish_` is.

**Transition table (normative):**

| # | Scenario | naturalFinish_ | pending_ | reply->error() | Outcome | body |
|---|---|---|---|---|---|---|
| 1 | natural finish, no error | true | Finished | NoError | **Finished** | yes |
| 2 | natural finish, network error | true | Finished | != NoError | **NetworkError** | no |
| 3 | timeout, no finish observed | false | TimedOut | not read | **TimedOut** | no |
| 4 | cancellation, no finish observed | false | Cancelled | not read | **Cancelled** | no |
| 5 | simultaneous timer + natural finish | true | TimedOut | per reply | **Finished** / **NetworkError** per rows 1/2 | per row |
| 6 | abort-induced finished() | false (abort guard) | TimedOut/Cancelled | not read | **unchanged** | no |
| 7 | spurious exec() return | false | unset | not read | **NetworkError** ("event loop returned without an outcome") | no |

Row 5 is NOT a promotion: the outcome follows from the natural `finished()` having been observed before any abort, and its error state then decides rows 1 vs 2. A natural finish carrying an error is NetworkError, never Finished. Only row 1 populates `body`; `readAll()` is called in exactly one place.

**Disposal.** Snapshot `httpStatus`/`error`/`errorString`/`body` BEFORE disposal. Then `abortIssued_ = true; if (!reply->isFinished()) reply->abort(); reply->deleteLater();` — exactly once on every path, RAII-guarded. **Acceptance criterion is ACTUAL destruction exactly once** (`QSignalSpy` on `QObject::destroyed` count == 1 AND a `QPointer` guard null after drain). The "call `deleteLater()` twice" mutation is REJECTED as unreliable — Qt coalesces repeated `deleteLater()` once the `DeferredDelete` event is posted, so the mutant is inert. Killing mutations are **M-D1** drop disposal (destruction count 0), **M-D2** transfer ownership back to the caller (count 0 in-window), **M-D3** `delete` instead of `deleteLater` (ASan UAF).

**Cancellation.** `kCancelPollMs = 250`; **maximum cancellation-observation interval <= 300 ms** (250 poll + 50 dispatch margin). Test acceptance: observed <= 500 ms with the timeout override at 5000 ms so a timeout cannot cause the exit. Killing mutation: raise `kCancelPollMs` to 2000 -> observed exceeds 500 ms.

```cpp
class CancelToken {                       // CloudService.h includes <atomic>, <memory> ONLY
public:
    CancelToken() = default;              // never cancels
    explicit CancelToken(std::shared_ptr<const std::atomic_bool> f) : flag_(std::move(f)) {}
    bool cancelled() const { return flag_ && flag_->load(std::memory_order_acquire); }
private:
    std::shared_ptr<const std::atomic_bool> flag_;
};
```
Lifetime-safe by `shared_ptr`; default token never cancels; **`CloudService.h` carries no include, forward declaration or friendship of `CloudServiceAutoDownload`.** No lock-freedom is claimed or required. Plumbed as a member (`setCancelToken`), NOT as a parameter, so `open()`/`readdir()` virtual signatures are unchanged.

### S-1 — base-owned injectable QNetworkAccessManager
- `CloudService` initializes the manager on **every** construction path. No indeterminate pointer is possible. This repairs a latent UB: all ten providers currently create `nam` only `if (context)` and never list it in the initializer list, so null-`Context` construction leaves it indeterminate.
- **Explicit constructor injection / default construction — NOT a virtual factory.** A virtual factory called from a base constructor would not dispatch to the derived override, which is precisely the trap this clause exists to avoid.
- Production callers keep their existing `Provider(Context*)` path unchanged.
- Tests inject a fake manager created on the same thread.
- **Ownership:** `CloudService` owns and destroys both default and injected managers exactly once, parented to the `CloudService` with matching thread affinity. All ten `~Provider()` bodies (`if (context) delete nam;`) are REMOVED; the base becomes the sole owner.
- **Timeout override and manager injection compile in the SAME production code version under test.** No test-only preprocessor branch. `int requestTimeoutOverrideMs_ = -1;` consumed inside `blockingRequest` as `const int t = (requestTimeoutOverrideMs_ >= 0) ? requestTimeoutOverrideMs_ : timeoutMs;` — one line, no per-call-site change.

**Ten-constructor audit (orchestrator-run 2026-08-26, no incompatible behavior found).** Nine of ten connect `nam::sslErrors` to their own `onSslErrors` slot. **Dropbox is the sole exception and has NO `onSslErrors` slot at all** (`grep -c onSslErrors src/Cloud/Dropbox.{cpp,h}` -> 0/0). That asymmetry is PRESERVED: the base creates and owns the manager, each provider keeps its own `sslErrors` connect, and Dropbox keeps none. No proxy, cache, cookie jar, redirect policy or other manager configuration exists in any of the ten. **One declared deviation:** the `sslErrors` connect becomes unconditional rather than `if (context)`-guarded, because the manager now always exists; this changes behavior only on the null-`Context` path, which is currently undefined behavior.

### Timeout policy
Generic **30 s** open/auth and **60 s** listing, calibrated on GarminConnect (30/60/60). **SixCycle KEEPS its provider-specific 5 s open and 10 s readdir** — not loosened without provider-specific evidence; its stale "10000 seconds" comment (C5) is corrected and the finish check (C1) added. Accepted residual: a legitimate response slower than its cap becomes a reported failure instead of waiting indefinitely.

### Cascade
Closes S-R028f-F1, S-R028f-F3, C1, C2, C3, C4, C6 on completion; C5 corrected in the SixCycle changeset. Amends DES-001 invariant 3 from one provider to all. Corrects `CloudService.h:305`'s flat "never terminate()" (ORCH-042 item 2). Unblocks the parked auto-downloader teardown draft only on completed W3. TESTs T-143..T-151. Untouched: B-R028-17 and its DEC-030 amendment changeset, ORCH-036, ORCH-042 items 1 and 3, S-R028f-F2.

### Alignment probe
grep -c "onSslErrors" src/Cloud/Dropbox.cpp src/Cloud/Dropbox.h        # expect 0 and 0 (asymmetry preserved)
grep -rn "delete nam" src/Cloud/                                        # expect ZERO after Stage 1 (base is sole owner)
grep -rn "CloudServiceAutoDownload" src/Cloud/CloudService.h            # expect ZERO inside the CancelToken/helper region
grep -c "loop.exec()" src/Cloud/*.cpp                                   # expect 0 for the ten migrated providers
grep -rn "abort()" src/Cloud/CloudService.cpp                           # expect the helper's single disposal site

## DEC-040 STAGE-1 AMENDMENT & STATUS — 2026-08-30 (reconciled against repository truth)

**THE ACCEPTED TEXT ABOVE IS PRESERVED VERBATIM AS PROVENANCE. Nothing in it was edited.** This block records
what Stage 1 ACTUALLY built, where the implementation deviates from the accepted design, and which acceptance
conditions changed. Where the two disagree, THIS BLOCK is repository truth and the text above is history.

### STATUS
- **Stage 1 is BUILT, its Verification Gate PASSED, and it is COMMITTED as `37710370e` (2026-08-30) —
  NOT PUSHED and NOT MERGED.** Branch `garmin/req028-row-lifetime`, parent `05ac6bc29`, 31 paths,
  +5780/-381 (re-derived from git 2026-08-30). The gate evidence is orchestrator-re-executed, not accepted
  from a report (seal `rt8-20260830-095658`, plus the post-commit seal
  `post-commit-20260830-112228`, both in `../GoldenCheetah-recovery/DEC-040/`). **Two of the 31 committed
  blobs are the pre-commit `clang-format` content version and therefore DIVERGE from rt8; the 29 production
  blobs still match it.**
  *(STATUS CORRECTED 2026-08-30 by the documentation-reconciliation pass. This bullet read "on the CURRENT
  WORKING-TREE BYTES — and it is UNCOMMITTED … @ `05ac6bc29`, 31 uncommitted paths, nothing staged". That
  was accurate at the Stage-1 verification gate and went stale the moment the slice was committed; `src/`
  and `unittests/` are clean against HEAD. Nothing about the slice's VERDICTS changed — only its provenance.)*
- **DEC-040 IS NOT COMPLETE.** Stage 2 (generic cooperative cancellation) is UNBUILT. A green Stage-1 gate is an
  INTERMEDIATE implementation checkpoint inside one release unit — it is NOT the provider-watchdog checkpoint, it
  does NOT discharge C1/C2/C3/C4/C6, and it does NOT release the parked auto-downloader teardown draft, which only
  COMPLETED W3 releases.

### SCOPE AS BUILT
- **TEN providers carry the 22 migrated bounded-request sites** — Azum 3, Dropbox 2, PolarFlow 1, CyclingAnalytics 1,
  Xert 3, Strava 3, SportTracks 2, TrainingsTageBuch 2, Nolio 3, SixCycle 2 = **22**.
- **THIRTEEN providers participate in base manager ownership.** `RideWithGPS`, `Selfloops` and `SportsPlusHealth`
  are **manager-only siblings**: they take the injected manager but have NO blocking wait, so they carry **NO
  watchdog rows** in T-143/144/145/147/150 and appear in T-153 only.

### S-1 AMENDED — LAZY, "NULL-OR-VALID", NOT EAGER
The accepted text says the base constructs the manager on every construction path. **That is superseded.** As built:
- **No manager exists before `QCoreApplication`** — factory-template construction is INERT.
- **First real use creates exactly one**, IN THE PROVIDER'S OWN AFFINITY THREAD. `nam()` REFUSES to initialise
  off-affinity (it diagnoses rather than silently `moveToThread`-ing); the invariant is **"never indeterminate"**,
  not "initialised on every construction path".
- **An injected manager is adopted EXACTLY AS SUPPLIED** — adoption is conditional on affinity; no cross-thread
  `setParent()`, no relocation, and no default is ever built alongside it.
- **Provider `sslErrors` wiring runs EXACTLY ONCE**, when the manager first comes into being — it cannot stay in the
  constructor, because at construction time there is no manager to connect to. Dropbox's documented absence of an
  `onSslErrors` slot is still PRESERVED.

### ACCEPTANCE CONDITION CHANGED — `delete nam` IS NOT ZERO
The accepted alignment probe expected **ZERO** `delete nam` after Stage 1. **That is UNSATISFIABLE and is retired.**
**FIVE live `delete nam` statements remain LEGITIMATELY in `OpenData.cpp`** (`:135`, `:152`, `:204`, `:332`, `:352`)
— they delete LOCAL managers, not the base-owned one, and `OpenData.cpp` is outside this slice. The thirteen migrated
providers retain the idiom only inside explanatory COMMENTS. **Five, not zero, is the acceptance condition.**

### STAGE 1 HAS NO CANCELLATION, DELIBERATELY
- **No `CancelToken` type exists** (`0` live occurrences in `src/Cloud/`), **no cancellation poll**, and **no
  reachable `Cancelled` outcome.** `RequestOutcome::Cancelled` is DECLARED but unreachable, RESERVED for Stage 2 so
  that Stage 2 adds no enum churn. `kCancelPollMs`, the <=300 ms observation interval and their killing mutation in
  the accepted text are **STAGE-2 acceptance criteria and are NOT Stage-1 conditions**.
- **W3 INTERFACE CORRECTION.** The accepted text's signature block shows
  `blockingRequest(QNetworkReply*, int timeoutMs, const CancelToken& cancel = CancelToken())`. **That is WRONG as
  the final interface, and it contradicts the accepted text's own prose two paragraphs later.** The correct
  description — and the as-built Stage-1 working-tree code — is:
  **`RequestResult CloudService::blockingRequest(QNetworkReply *reply, int timeoutMs);`** (`CloudService.h:436`),
  **UNCHANGED**, taking NO token parameter. **Cancellation is MEMBER-PLUMBED via `setCancelToken`**, which is why
  `open()`/`readdir()` virtual signatures are untouched. Stage 2 adds the member, not a parameter.

### TEST ACCOUNTING AS BUILT
| TEST | Stage-1 state |
|---|---|
| T-143 · T-144 · T-145 · T-150 · T-151 · T-152 · T-153 | **BUILT IN FULL** |
| T-146 · T-147 | **Stage-1 portions BUILT; cancellation portions DEFERRED to Stage 2** |
| T-148 · T-149 | **ALLOCATED-UNBUILT** — no `CancelToken` exists in Stage 1, so neither is drivable |

### ALIGNMENT PROBES — REPLACED (the accepted set is retired; it counted comments as code)
```sh
# P1  live `delete nam` STATEMENTS, comments excluded -> expect 5, ALL in OpenData.cpp
grep -rnE '^[^/]*\bdelete +nam\b' src/Cloud/*.cpp src/Cloud/*.h | grep -vE '^[^:]+:[0-9]+:\s*//'
# P2  live blockingRequest CALL sites across the TEN watchdog providers -> expect 22
#     (Azum 3 · Dropbox 2 · PolarFlow 1 · CyclingAnalytics 1 · Xert 3 · Strava 3 ·
#      SportTracks 2 · TrainingsTageBuch 2 · Nolio 3 · SixCycle 2)
grep -cE '^[^/]*blockingRequest *\(' src/Cloud/{Azum,Dropbox,PolarFlow,CyclingAnalytics,Xert,Strava,SportTracks,TrainingsTageBuch,Nolio,SixCycle}.cpp
# P3  providers accepting an injected manager -> expect 14 headers = 13 PROVIDERS + CloudService.h (the base)
grep -lE 'QNetworkAccessManager *\*[a-zA-Z]* *= *(NULL|nullptr)' src/Cloud/*.h
# P4  manager-only siblings must carry NO blocking wait -> expect 0, 0, 0
grep -cE '^[^/]*blockingRequest *\(' src/Cloud/{RideWithGPS,Selfloops,SportsPlusHealth}.cpp
# P5  Stage 1 carries no cancellation type -> expect NO live occurrence
grep -rcE '^[^/]*\bCancelToken\b' src/Cloud/*.cpp src/Cloud/*.h | grep -v ':0'
# P6  Dropbox's sslErrors asymmetry is preserved -> expect 0 and 0 (LIVE code only; see the note below)
grep -cE '^[^/]*\bonSslErrors\b' src/Cloud/Dropbox.cpp src/Cloud/Dropbox.h
```
**P1-P5 were RUN on 2026-08-30 against the current bytes and returned exactly these values, and were
INDEPENDENTLY RE-RUN on 2026-08-30 by the documentation-reconciliation pass with the same results
(P1=5 · P2=22 · P3=14 · P4=0,0,0 · P5=0).**
**P6 WAS CORRECTED, NOT DROPPED — it was the one probe that counted a COMMENT as code.** As originally
written (`grep -c "onSslErrors"`) it returns `Dropbox.h:0` but `Dropbox.cpp:1`, because the Stage-1 commit
added an explanatory comment naming `onSslErrors` at `Dropbox.cpp:33`. That is precisely the defect this
block criticises in the probes it retired, reproduced inside the replacement set. **The BEHAVIOUR it tests is
REAL and still holds:** the comment-blind form above returns 0 and 0 for Dropbox while siblings return
non-zero (Strava 2/1, Nolio 2, Azum 3), so the probe discriminates. The decision is unchanged; only the
probe text was wrong. P1/P2/P4/P5 are
written to distinguish LIVE STATEMENTS from COMMENTS, which is precisely what the retired probes did not do — the
old `grep -rn "delete nam" src/Cloud/` counted 13 explanatory comments as if they were code and made a satisfiable
condition look unsatisfiable.

---

## DEC-041 — File-IO layer Context UAF at RideFile.cpp:999: hoist-and-capture, not guard-and-decline

- Status: accepted (B — hoist-and-capture)
- Reversibility: cheap (single-function diff inside `RideFile.cpp`; swappable with Option A without touching any call site)
- Decided / last-reviewed: 2026-09-03
- Serves: REQ-029 (new, allocated this decision); raised by `B-R025-01` (Builder/REQ-025, 2026-08-12) and independently re-raised by `A3-R021b-F2` (A3/REQ-021 re-clear, 2026-08-12) — the same defect, same cite, two rows
- Dependents: REQ-029's acceptance criteria (to elaborate when the REQ starts); a new FileIO-layer teardown test category (today all Context-teardown tests live under `unittests/Core/garminconnect/`, Cloud-specific)
- Research: qgdw-scout 2026-09-03 (read-only source research; three options drafted, no external research needed — the trade space is fully internal to this codebase's own prior 6 DECs in the same bug family)

### The problem
`RideFileFactory::openRideFile` (`src/FileIO/RideFile.cpp:847-1002`) is handed a raw `Context*` and, after calling a format reader that for `.fit` payloads runs its own nested `QEventLoop` with a 5s timeout (`FitRideFile.cpp:172-184`), reads it again at `RideFile.cpp:999`: `if (context) result->setTag("Athlete", context->athlete->cyclist);`. The null check guards the pointer value, never the object's lifetime — `context` is a raw pointer, not a `QPointer`. An athlete-tab close delivered during that suspension frees the Context, and the fault lands **inside `RideFile.cpp`, before any of REQ-021's dialog guards, REQ-025's frame counting, or DEC-031's reaper can run** — this is a fourth, deeper layer of the same UAF class those three REQs already closed at the dialog/store/frame layers. `openRideFile` has exactly two `context` touches: `:906` (pre-suspension, safe) and `:999` (post-suspension, the hazard) — confirmed by direct read.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A guard-and-decline (`QPointer<Context> self`-bail at :999) | 4 — closes the crash at the deref point using the pattern already mutation-proven 6+ times in this ledger (REQ-021/024/025/026/028); residual is a silently-missing "Athlete" tag on the rare race, not a crash | 5 — O(1) pointer check | 4 — smallest diff, instantly pattern-matches every other guard in this file family, but adds one more entry to an already-large guard inventory this ledger's own findings (B-R025-03, A3-R026-F2) flag as an audit burden | 4 — consistent with this ledger's 8-DEC consensus; not the objectively cleaner fix |
| **B hoist-and-capture — CHOSEN** | 5 — strictly closes the race window; nothing reads `context` after the suspension returns, so there is no guard to omit and no partial-tag residual | 5 — same negligible cost, marginally cheaper (no `QPointer` indirection) | 4 — same single-function diff, but a fix SHAPE this ledger has never used for this bug class — a future reader has to recognize a genuinely different pattern in this one function rather than pattern-match a dozen others | 5 — "eliminate the race, don't detect it" is the stronger general engineering practice at the same diff size |
| C decouple FileIO from Context (signature change) | 5 — closes the entire class permanently at this boundary | 5 — no runtime difference; structurally safer against any future field added to the tail | 2 — touches ~15 call sites across Cloud/Core/Gui/Train; solo-dev review/regression surface an order of magnitude larger than the 2 defective lines require | 3 — architecturally "right" long-term, but LSN-041 ("sibling-scan work belongs in its own DEC, not folded into the triggering fix") argues directly against folding a project-wide refactor into a 3-row Gate-1B unblock |

### Cascade impact
- A: adds one more entry to the guard-liveness inventory this ledger's own findings already flag as growing; trivially reusable for the other ~14 non-Cloud callers of `openRideFile` at near-zero marginal cost, but does not itself make those callers' USE of a possibly-tag-incomplete `RideFile*` safe (REQ-022's OpenData thread axis stays open regardless).
- B (chosen): no new guard-liveness entry — a concrete reduction in future audit surface. The acceptance test is a STRONGER criterion than A's ("no crash") — it must also assert the Athlete tag is CORRECTLY set even when the Context dies mid-parse, a slightly bigger one-time test-writing cost for a permanently smaller one. Sets a precedent the next sibling-layer fix, if any, may be expected to follow.
- C: forces compile-and-smoke re-verification of all 15 call sites in one changeset — by far the largest PR of the three for a defect whose only PROVEN-reachable trigger today is two Cloud dialogs. Opens a new hosting/test topology (FileIO unit-testable with zero Context/Athlete fixture construction) that nothing today allows, but is a one-way door in both directions: reverting is itself a 15-site diff.

### Chosen
B — wins on Reliability (5) and Best Practices (5) at the same Maintainability cost as A (4) and a fraction of C's (2); eliminates the hazard rather than detecting it, stays a single-function diff, and still honours DEC-030's "guard sits at the unsafe layer" constraint. A is a legitimate fallback if idiom-consistency with the other 8 guard-shaped DECs in this ledger is weighted above the marginal reliability gain — recorded here as the reversibility path, not chosen.

**REQ scope, decided alongside:** REQ-029, a new id, rather than folding into the still-stub REQ-023. REQ-023 is scoped explicitly to the cloud STORE layer (`CloudService::uncompressRide`); this ledger's `DEC-030` convention has consistently kept STORE and FILE-IO as separate axes (REQ-022 thread axis, REQ-023 store axis already split this way), and file-IO is a fourth, distinct axis in the same UAF family (dialog=021 → store-uncounted-frame=024/025 → abort=026 → row-lifetime=028 → **file-IO=029, this**).

**`B-R021-10` reconciled alongside, not part of this decision's live trade space.** The scout's research measurement found it already discharged: both `store->writeFile` sites it named are lexically inside a `BlockingCall` scope as of `37710370e`, per REQ-025's commit `6dc794caf` (`traceability.md:664`, TEST-091/TEST-092) — orchestrator-confirmed by direct source read 2026-09-03. Recorded as its own finding disposition in `findings.md`, not folded into DEC-041.

### Alignment probe
grep -n "context->athlete->cyclist" src/FileIO/RideFile.cpp                       # expect the read moved BEFORE reader->openRideFile(...), not after
grep -c "if (context) result->setTag" src/FileIO/RideFile.cpp                     # expect 0 once the hoist lands (the guard-and-decline shape is gone, not merely widened)
grep -n "REQ-029" .claude/workflow-garminconnect/prd.md .claude/workflow-garminconnect/traceability.md   # expect both to name it once built

### AMENDMENT 2026-09-08 — census corrected; mechanism EXTENDED with a :1055 liveness bail (user decision)
**The Problem's recorded census was FALSE when written.** `openRideFile` spans :847-**:1133** (not -1002)
and has **FOUR** handed-in-`Context` touches, not two: `:906` (pre-suspension, safe), `:928`
(`result->context = context;` — a pointer STORE that implants the pointer into the returned ride; latent,
outside this decision), `:999` (the named hazard), and **`:1055` — `if (context) result->recalculateDerivedSeries();` — a second post-suspension deref this entry did not name** (plus `:943`
`GlobalContext::context()`, a different object, distinguished not feared). `RideFile.cpp` carries zero
uncommitted diff, so "exactly two context touches — confirmed by direct read" was wrong of the same bytes it
was written against: the [[O-R021-03]] class.

**Consequence:** Option B is exactly expressible for `:999` but structurally NOT for `:1055` —
`recalculateDerivedSeries` needs the parsed ride (cannot run pre-suspension) and reads `context->athlete`
through an algorithm (`:2545` zones chain, `:2557` cvalue) whose staleness gate cannot fire on a fresh parse
(`dstale` born true `:108`, set true by every `appendPoint`, cleared only `:3045` inside the function itself).
No `return` exists between `:999` and `:1055`. The builder hatched on this; the orchestrator independently
verified all five load-bearing claims by direct read 2026-09-08.

**Amended choice (user-picked 2026-09-08 from three scored options): B + a supplementary liveness bail at
:1055.** A `QPointer<Context>` captured at factory entry — where the pointer is known alive, the sound
capture-point adaptation this ledger already accepted for REQ-023's `contextGuard_` — consulted before the
`:1055` call; on a dead Context the derived-series recalculation is SKIPPED (residual: a ride opened during
that rare window ships with stale derived data — parallel to the accepted trade space, and strictly better
than a crash). **Why this does not reopen Option A:** A's QPointer-bail was rejected as the REPLACEMENT for
the hoist at `:999`; as a supplement for one call that provably cannot be hoisted, it is the only mechanism
that keeps REQ-029's whole acceptance ("no longer faults at the tail") true without a scope change.
Alternatives rejected this pass: narrowing REQ-029 to `:999` (leaves a blocking-class UAF open past Stage 6)
and shipping as-is (fails the acceptance's own first half). Amendment is additive: the `:999` hoist stands
exactly as decided 2026-09-03.

**Alignment probe additions:** `grep -n "recalculateDerivedSeries" src/FileIO/RideFile.cpp` — expect the
`:1055` call now gated by the factory-entry QPointer; the `:999` hoist unchanged; test target asserts BOTH
(no fault at either tail site with a mid-loop Context death; tag AND derived series correct when the Context
survives).

## DEC-042 — `saveRide`'s post-`autoProcess` member dereferences vs. parent-teardown UAF: guard the proven hazard site, not the call site

- Status: accepted (A — in-function `QPointer` self-bail after `saveRide`'s own second suspension point). **BUILT + execution-verified 2026-09-05.**
- Reversibility: cheap (single-function diff inside `CloudService.cpp`; swappable with either rejected option without touching any call site)
- Decided / last-reviewed: 2026-09-05
- **Build:** `qgdw-builder`, TEST-158 (`aParentTeardownInsideSaveRidesSecondAutoProcessMustNotFreeItUnderThat`, `testGarminConnectSyncDialogClose.cpp`). RED: orchestrator-independently reproduced the exact `heap-use-after-free` at `saveRide` (freed by `QObjectPrivate::deleteChildren()`) by mutating the guard to `if (false && self.isNull())`, snapshot/restore via `cp`+`cmp` (never `git checkout`, per `[[LSN-084]]` — the restore half hit `[[ORCH-057]]`'s sibling tooling gap, `[[ORCH-058]]`, worked around via the Edit tool). GREEN: 99/99 on BOTH `offscreen`/`minimal`, `autoProcessCallsAtTeardown=2` confirming the teardown landed precisely in the SECOND `autoProcess` call, `writeRideFileCalls=0`/`addRideCalls=0` confirming the accepted trade-off. Full `ctest -L garmin-fast` 27/27, EXIT=0. Durable log: `.claude/evidence-seals/DEC042-TEST158-saveRide-mutation-2026-09-05.log`. **Closes `B-R028-17`.**
- Serves: REQ-028 clause (e) hardening; raised by `B-R028-17` (Builder/REQ-028 clause-(e) slice, 2026-08-23; classified by A3-R028e, 2026-08-24) while building TEST-141 — pre-existing in shipped code, not introduced by that (test-only) changeset
- Dependents: a new ASan regression slot in the existing `testGarminConnectSyncDialogClose` target, driven through the already-built `gcstub::autoProcessAction` seam; the TEST-140 comment block's T-141-probe narrative (documents the call-site guard's insufficiency this decision closes)
- Research: qgdw-scout 2026-09-05 (read-only source + test-infrastructure research; three options drafted; found the seam needed to reproduce the hazard already exists, so none of the three options need new build targets)

### The problem
`CloudServiceSyncDialog::saveRide(RideFile*, QStringList&)` (`src/Cloud/CloudService.cpp`, currently ~3974-4014) calls `DataProcessorFactory::instance().autoProcess(ride, "Auto", "Import")` and, two statements later, `...autoProcess(ride, "Save", "ADD")` — either can nest a `QEventLoop` (e.g. `FixElevation::postProcess`, `FixElevation.cpp` ~283-313, an UNTIMED wait, reachable when the "Fix Elevation Data" processor's automation mode is "Auto"; default is "Manual", opt-in but supported, and the master `autoprocess` switch defaults TRUE). After the second call, with ZERO lifetime check anywhere in between or after, `saveRide` dereferences `this`'s members: `reader.writeRideFile(context, ride, file)`, `rideFiles<<targetnosuffix`, `context->athlete->addRide(...)`. The caller (`completedRead`, ~3507-3517) already does `{ BlockingCall blocking(this); saved = saveRide(ride, errors); } delete ride; if (self.isNull()) return;` — the SAME idiom `DEC-garmin-025` established elsewhere in this file — but that check fires one statement too late: it runs AFTER `saveRide` has already returned, while the hazard is INSIDE `saveRide`, between its own suspension and its own subsequent member reads. `BlockingCall`'s deferral mechanism only intercepts a *self-initiated* close (`deferCloseIfBusy`); it cannot and does not stop Qt destroying this dialog directly as a side effect of the PARENT (athlete tab) being torn down (`QObjectPrivate::deleteChildren()` is unconditional, no virtual dispatch, no veto) — exactly the class of gap `DEC-garmin-025` was written to close everywhere else in this file.

**Confirmed by direct read, not by trusting the finding's (now-stale) line citations**, and independently corroborated by test infrastructure already in the tree: the TEST-140 comment block in `testGarminConnectSyncDialogClose.cpp` documents a T-141 probe that already drove a seam-injected close from inside `saveRide`'s suspension and captured an ASan `heap-use-after-free` at exactly the post-`autoProcess` member reads — i.e. this project's own prior work already proved the call-site guard is insufficient, before this decision was drafted.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A in-function `QPointer` self-bail — CHOSEN** | 4 — eliminates the UAF; the only residual is a silently-skipped save on a rare parent-teardown mid-`autoProcess`, never a crash | 3 — O(1) per ride, a wash at 10x/100x batch size against the other two | 4 — ~4-line diff, reuses the `QPointer`/`isNull()` idiom already used 15+ times in this file, instantly recognizable later | 5 — matches this project's own more-recent, more-specific precedent (`DEC-garmin-025`) verbatim, placed at the site this project's OWN probe proved is the real hazard |
| B restructure — hoist the suspending calls out of any `this`-touching frame | 4 — structurally closes the bug class for any future processor added to `autoProcess` | 4 — same O(1) runtime, automatically covers future processors with no `saveRide`-side guard maintenance | 2 — splits `saveRide`'s contract across two call points, touches `completedRead` too, and invalidates ~100 lines of TEST-140's structural narrative (line-number citations, stack-trace framing) in the most fixture-dependent file in the suite | 4 — matches `DEC-garmin-026`'s "eliminate the frame, don't patch it" precedent, but applied where per-site guarding has NOT yet been tried-and-failed the way it had in the constructor case DEC-026 addressed |
| C snapshot escape + single tail guard (capture `context` locally as `ctx`, guard only the `rideFiles` write) | 3 — fixes today's known hazard but leaves a `ctx`/`context` dual-name trap: any future statement added after the suspension that reads `context` instead of `ctx` silently reopens the same UAF class | 3 — same wash | 3 — small diff, but a second name for the same pointer inside one function raises cognitive load and is exactly the miss-shape `DEC-garmin-026` warned against | 3 — a legitimate but idiosyncratic pattern, not established elsewhere in this codebase for this problem class |

### Cascade impact
- A (chosen): ~4-line change — hold `QPointer<CloudServiceSyncDialog> self(this)` before the first `autoProcess` call, `if (self.isNull()) return false;` immediately after the second (the only statement between the two, `ride->recalculateDerivedSeries()`, touches `ride`, not `this`, so one check suffices). New ASan test slot arms `gcstub::autoProcessAction` to deliver a close from inside the second `autoProcess` call; asserts no ASan report AND that `saveRide` returns without touching `context`/`rideFiles`; reverting the guard must reproduce the exact `heap-use-after-free` stack the T-141 probe already captured — satisfying this ledger's standing evidence bar (fails one way, passes the other). **Named trade-off, deliberately accepted:** on the rare teardown-mid-`autoProcess` path the ride's JSON file is now never written at all, which sits in tension with `completedRead`'s own nearby comment preferring "the write survives, only the label/bookkeeping stands down" for OTHER guards in the same function — this decision accepts that inconsistency for THIS specific window in exchange for the smallest, most reviewable diff; if that trade is judged unacceptable later, Option C is the documented fallback (see Chosen, below).
- B: forces a rewrite of TEST-140's structural narrative and splits `saveRide`'s contract across `completedRead` and a new helper — real six-month maintenance cost in the most fixture-heavy file in the suite, for a bug class that (unlike DEC-026's constructor case) has not actually been shown to keep recurring at new call sites.
- C: preserves the "file survives dialog death" property but plants a same-file dual-name pointer (`context` vs `ctx`) that depends on a durable comment fence to stay safe — a maintenance hazard of a different, subtler shape than A's.

### Chosen
A — wins on Best Practices (5, at the project's own proven hazard site) and Maintainability (4) at Reliability parity with B, for a fraction of B's six-month upkeep cost; C is the documented fallback if the "file survives dialog death mid-save" property (which A gives up) is later judged to matter more than diff size and idiom-consistency.

### Alignment probe
grep -n "QPointer<CloudServiceSyncDialog> self" src/Cloud/CloudService.cpp   # expect a NEW occurrence inside saveRide, in addition to the existing ones in completedRead/failedRead
grep -n "autoProcess(ride" src/Cloud/CloudService.cpp                       # expect both calls still in saveRide, with the guard between the second call and the writeRideFile/rideFiles/addRide reads below it
grep -n "self.isNull()" unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp   # expect a new T-141-closing slot asserting no ASan report under a seam-injected teardown mid-autoProcess

## DEC-043 — `CloudServiceAutoDownload` cross-thread lifetime UAF: guard the completion slots AND cooperatively cancel-and-wait the thread at teardown

- Status: accepted (C — Option A's guards plus Option B's cooperative cancel-and-`wait()`, combined). **BUILT + execution-verified 2026-09-05** — orchestrator-independently re-verified including its own killing mutation; durable log `.claude/evidence-seals/DEC043-TEST159-TEST160-CloudServiceAutoDownload-mutation-2026-09-05.log`; closes `A3-R028e-F1` (see findings.md). One build deviation from the decided wording (readFailed left UNGUARDED — see Alignment probe correction) and one residual ([[ORCH-059]], the stub-mirror sync gap), both recorded below/at the probe.
- Reversibility: expensive, not one-way (touches `CloudService.h`/`.cpp`'s public surface and `Athlete::close()`/tab-teardown's ordering; the two mechanisms are structurally independent, so either could later be reverted without unwinding the other)
- Decided / last-reviewed: 2026-09-05
- Serves: REQ-028 clause (e) hardening; raised by `A3-R028e-F1` (A3/REQ-028 clause-(e) slice, 2026-08-24) — pre-existing in shipped code, deliberately NOT folded into `B-R028-17`/DEC-042 (different class: cross-thread ownership, not same-thread reentrancy; different owner; different trigger; no nested loop or processor configuration required to reach it)
- Dependents: `CloudService.h`'s `CancelToken`/`kCancelPollMs` (DEC-040 Stage 2, `fd7639f7a`) — extended, not replaced; `Athlete::close()`/`~Athlete()`; `MainWindow`'s athlete-tab-close teardown sequence; a new test category (explicitly, per the finding, "not covered by any existing TEST id")
- Research: qgdw-scout 2026-09-05 (read-only source research over `CloudService.h/.cpp`, `Athlete.h/.cpp`, `MainWindow.cpp`, `RideCache.cpp`; three options drafted)
- **Explicitly out of scope, by user decision 2026-09-05:** `CloudServiceAutoDownload::run()`'s own WORKER-THREAD read of `context->athlete->rideCache->rides()` (`CloudService.cpp:4188`) is a different surface (concurrent worker-thread access, not GUI-thread queued-delivery timing), found while scouting this decision, and is tracked separately as `[[ORCH-057]]`, not fixed here. Whether this decision's `requestStop()+wait()` mechanism happens to also close it as a side effect (by ensuring `run()` has fully returned before `delete athlete`/`delete context`) is left for whoever next scopes `ORCH-057`'s own fix, not assumed here.

### The problem
`CloudServiceAutoDownload` (`src/Cloud/CloudService.h` ~1311, `: public QThread`) is constructed with a raw `Context*` and no QObject parent (`CloudServiceAutoDownload(Context *context) : context(context), initial(true) {}`), owned by `Athlete` as `cloudAutoDownload` (`Athlete.h:100`), created via `new CloudServiceAutoDownload(context)` at `Athlete.cpp:168` and wired to `refreshEnd()` at `:169`. The only other reference anywhere in the codebase is `MainWindow.cpp:2533`'s `checkDownload()` call — nothing ever calls `quit()`, `wait()`, or deletes it. Its `run()` (`CloudService.cpp` ~4073) executes on the worker thread; its `readComplete`/`readFailed` slots (currently ~4347-4431) are QUEUED cross-thread connections that therefore EXECUTE on the GUI thread whenever Qt dispatches them, and carry ZERO lifetime guards — confirmed by direct read — dereferencing `context->athlete->home->activities()` and `context->athlete->addRide(...)`. Confirmed independently by reading `Athlete.cpp` myself: `~Athlete()` (~235-263) explicitly deletes `rideCache`, `calendarDownload`, `namedSearches`, `routes`, `seasons`, `measures`, all zones, `autoImportConfig`, `autoImport` — but `cloudAutoDownload` appears nowhere in it, and `Athlete::close()` (~211-225) does not touch it either. So an athlete-tab-close teardown that synchronously deletes `athlete`/`context` can run while a queued `readComplete` metacall for this exact instance is still sitting in the GUI event queue; when Qt later dispatches it, the slot dereferences freed memory. Gated in practice by the opt-in "sync on startup" setting (default `false`), but once enabled the trigger is an ordinary user action, not a timing artifice.

This is a genuinely different shape from DEC-042: a cross-THREAD ownership problem (a worker thread outliving the GUI-thread object graph it reads from), not same-thread reentrancy into a suspended stack frame — so `DEC-garmin-025`'s `QPointer` self-bail idiom alone is a candidate, but not automatically sufficient, unlike at DEC-042's site.

**Existing project precedent, both directions:** `RideCacheRefreshThread` (`RideCache.h:237`) IS a worker `QThread` this codebase already knows how to retire safely — `RideCache::cancel()` (`RideCache.cpp:566`) calls `thread->wait()` on every tracked thread before discarding it. `CloudServiceAutoDownload` has no equivalent handling at all today, despite being the same class of problem (an owner with a worker QThread it must not outlive).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A guard-only (`QPointer`/self-validity checks in `readComplete`/`readFailed`) | 3 — race-free by construction (teardown and dispatch both run on the GUI thread, so the check is deterministic, not probabilistic) but leaves `run()`'s own worker-thread read AND the `cloudAutoDownload` leak open | 4 — O(1) per completion, flat cost regardless of volume | 5 — smallest diff, mirrors `DEC-garmin-025`'s idiom exactly | 4 — textbook Qt idiom for "receiver may already be destroyed when a queued call is dispatched," but treats the symptom at the delivery site, not the underlying "nobody owns this thread's lifetime" defect |
| B cooperative cancel + bounded `wait()` (extend `CancelToken` onto this thread; mirror `RideCache::cancel()`) | 2 — `CancelToken` today only interrupts SYNCHRONOUS `blockingRequest` calls inside `open()`/`readdir()`; the async `readFile`/`readComplete` path is untouched, so a download already dispatched still completes and still races freed `context`/`athlete` — does NOT close the exact hole this finding names | 4 — bounded by `kCancelPollMs` (~250ms) × remaining calls | 3 — new member + `requestStop()` API, new ordering dependency in an already order-sensitive teardown sequence | 3 — matches in-house precedent strongly, but stalling the GUI thread in `wait()` at tab-close, even bounded, is a UX cost at exactly the moment a user expects instant feedback |
| **C both, combined — CHOSEN** | 5 — closes both surfaces at once; the only option with no unaddressed residual within this finding's scope (and incidentally also closes `run()`'s own worker-thread read, since `run()` is joined before the object graph is freed — though that read is tracked separately as `[[ORCH-057]]`, not claimed as fixed by this decision alone) | 4 — inherits B's bound; A's guard overhead is O(1) on top | 3 — heaviest: two mechanisms to understand, and the sync/async asymmetry from B is WHY A can't be dropped once B ships — needs an explicit comment fence or a future "cleanup" silently reopens the hole | 5 — the textbook combination for "an async worker feeding a GUI-thread object that may be torn down": cancel what you can, guard what you can't; matches both in-house precedents (`DEC-garmin-025`, `RideCache`/`DEC-040`) simultaneously |

### Cascade impact
- A alone: cheap, deterministic, unit-testable with no thread start or network stub — but ships a release-blocking fix with two named, concrete residuals still open (the worker-thread read, the leak), inviting a future reader to assume the class is fully guarded when it is not.
- B alone: reuses `CancelToken` (one commit old, `fd7639f7a`) but does not close the literal hazard this finding names — an in-flight async download's queued completion still races. Adds a new ordering contract to `Athlete::close()`/tab-teardown that MUST run before `delete athlete`/`delete context`.
- C (chosen): same two call-site additions as B (worklist-loop `setCancelToken`, teardown `requestStop()+wait()`) plus A's two guard checks. Requires an explicit comment explaining WHY both mechanisms are needed — deleting either one later silently reopens the hole the other was covering. Two independently-cheap new test categories rather than one (a pure-slot unit test for A's guard; an `Athlete`/`Context`-fixture-driven teardown-ordering test for B's cancel+wait), neither requiring new CI infrastructure beyond what `CloudService`'s existing test target already has. Makes the teardown ordering contract load-bearing for two independent reasons at once — raises the cost of ever reverting to just one mechanism later without re-deriving both, which is the named reason Reversibility above is "expensive, not one-way" rather than "cheap".
- The pre-existing `cloudAutoDownload` leak (constructed at `Athlete.cpp:168`, never deleted anywhere) becomes a TRIVIAL one-line fix once C's `wait()` lands: after `wait()` returns, `run()` is guaranteed to have exited, so `delete cloudAutoDownload;` in `~Athlete()` is safe by construction — judged genuinely trivial (not requiring its own decision or its own finding) and folded into this same build, per explicit user instruction 2026-09-05. Before C's `wait()` exists, deleting it while `run()` might still be executing would itself be unsafe — this is why the leak was not fixed ahead of this decision.

### Chosen
C — both A and B individually leave a named, concrete residual on a release-blocking UAF (A: the worker-thread read and the leak; B: the exact in-flight-async-download race this finding names). The only stated constraint (solo-dev project) argues for keeping each piece of C simple — which both already are, since neither is novel, each extends a pattern already present in this exact codebase — rather than shipping a known-incomplete fix to save effort on a release blocker. Do this before REQ-028(e) hardening ships more code on top of the current teardown sequence, since the ordering contract only gets more expensive to change the more later code assumes it.

**Scope, decided alongside (user 2026-09-05):** `ORCH-057` (the `run()` worker-thread read) stays its own finding, not folded in — see the "Explicitly out of scope" note above. The `cloudAutoDownload` leak IS folded in, as a trivial one-line consequence of C's own `wait()` — see Cascade impact above. The ~250ms-per-remaining-call bound on athlete-tab-close latency that C's `wait()` implies is accepted as-is, no stricter SLA required (user 2026-09-05).

### Alignment probe
grep -n "requestStop\|setCancelToken" src/Cloud/CloudService.h                          # expect a new requestStop() (or equivalent) API on CloudServiceAutoDownload, and a setCancelToken call in run()'s worklist loop
grep -n "cloudAutoDownload" src/Core/Athlete.cpp                                        # expect a wait()+delete sequence in ~Athlete()/close(), not just the constructor line
grep -n "self.isNull()\|QPointer" src/Cloud/CloudService.cpp                            # probe line written at decision time said "inside readComplete/readFailed"; the BUILD landed the guard in readComplete ONLY, deliberately: readFailed touches no context/athlete state (logging only), a guard there would be dead code. Built guard is the stopRequested_->load(acquire) check at the head of readComplete, which frees the buffer (delete data) and returns
grep -n "context->athlete->rideCache->rides()" src/Cloud/CloudService.cpp              # ORCH-057's own site — expect it UNCHANGED by this decision's build (tracked separately)

## DEC-044 — `RideFile::appendOrUpdatePoint` reads a deleted point: alias the survivor, don't detect the fault

- Status: accepted (A — alias the surviving point)
- Reversibility: cheap (single-function diff inside `RideFile.cpp`, one new local variable, no signature or call-site change)
- Decided / last-reviewed: 2026-09-08
- Serves: REQ-030 (new, allocated this decision); raised by `B-R029-01` (Builder/REQ-029, 2026-09-08), found while confirming RED for `testGarminConnectFileIOLifetime`'s T-173 slot
- Dependents: REQ-029/DEC-041's own build — T-173 cannot be observed RED-for-the-intended-reason (a Context UAF at `:999`) or reach GREEN until this lands first, since the process currently aborts here on ANY `.fit` parse with a duplicate-`secs` record, live Context or not
- Research: orchestrator direct read 2026-09-08 (no external research needed — single function, trade space fully internal; user waived scout dispatch given the mechanical size of the fix)

### The problem
`RideFile::appendOrUpdatePoint` (`RideFile.cpp:1616-1712`) takes ownership of a heap-allocated `RideFilePoint *point`. On the duplicate-timestamp branch (`:1651-1667`, taken when `dataPoints_.at(idx)->secs == secs`), it copies `point`'s fields into the existing slot and frees `point`:
```
updatePoint(point, dataPoints_.at(idx));
*dataPoints_.at(idx) = *point;
delete point;                                    // :1658
```
The other two branches (`:1660-1663` insert, `:1669-1671` forceAppend) instead retain `point` — inserted into or appended onto `dataPoints_`, never freed. All three branches then fall through to:
```
updateMin(point);                                  // :1709
updateMax(point);                                  // :1710
updateAvg(point);                                  // :1711
```
`updateMin` dereferences `point->secs` at its first line (`:1249`). On the duplicate-timestamp branch this reads freed memory — confirmed live under ASan by REQ-029's builder, then independently confirmed by the orchestrator by direct read (`grep -n "void RideFile::updateMin" ...`, `git blame -L 1656,1659`). `git blame` places the surrounding branch structure at `c023572932` (2016-08-13) and the `delete point;` line itself at `3b17371687` (2022-07-23) — pre-existing in shipped code, not introduced by any UAF-family DEC in this ledger, and structurally unrelated to Context/suspension lifetime: it fires on ANY duplicate-`secs` record, single-threaded, no suspending call required. Deterministic for the FIT sample REQ-029's own fixture drives (`2012_01_11_11_51_01.fit` — it contains at least one duplicate-`secs` record).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A alias the surviving point — CHOSEN** | 5 — closes the race by never reading the freed object; the values are provably identical to pre-fix (`*dataPoints_.at(idx) = *point` already copied everything before the delete), so min/max/avg accumulate the same numbers, just from a live address | 5 — O(1), no new allocation | 5 — one new local (`RideFilePoint *newest = point;`, reassigned to `dataPoints_.at(idx)` on the delete branch), three call sites at the tail changed from `point` to `newest`, nothing else moves | 5 — mirrors this ledger's own "eliminate the race, don't detect it" precedent (DEC-041's hoist-and-capture reasoning) at a fraction of that decision's size |
| B inline the update calls per-branch (duplicate `updateMin/Max/Avg(point)` before the `delete`, leave the existing tail calls for the other two branches) | 5 — same correctness | 5 — same cost | 2 — triplicates a 3-line block across 3 branches; a future 4th accumulator (or a change to what "update" means) has to be added in three places and silently drifts if one is missed — the exact multi-copy risk this ledger already flagged once for CLV Check 5's old awk/shell duplication | 3 — works, but trades a one-line diff for a structural DRY violation for no reliability gain over A |
| C restructure ownership: don't delete on the match branch; defer the single delete to the very end of the function, gated by which branch ran | 4 — correct if the gating flag is right, but adds a new failure mode (double-free or leak if the flag logic is ever touched later) that A and B don't have | 5 — same cost | 2 — the most invasive of the three for a mechanical bug: reshapes control flow around `forceAppend` (which currently owns whether `point` is freed at all), the highest regression surface of the three options | 3 — architecturally tidier in the abstract, but LSN-041 ("sibling-scan work belongs in its own DEC, not folded into the triggering fix") argues against restructuring more than the fault requires |

### Cascade impact
- A (chosen): the only production-code change is inside `appendOrUpdatePoint` itself; no caller, header, or signature changes. Nothing else in this ledger's UAF family depends on this function's internals, so there is no cascade beyond REQ-029/T-173 becoming observable.
- B: same runtime behavior as A, but the triplicated block is a standing maintenance liability the next editor of this function inherits for free with A instead.
- C: touches the same lines B and A do, plus the branch-selection logic itself — the largest surface for a fix whose entire job is "don't read a pointer after freeing it."

### Chosen
A — strictly dominates B and C on maintainability at identical reliability/scalability, and is the smallest diff that eliminates (not merely detects) the hazard, consistent with this ledger's established preference (DEC-041) for closing a race at its source rather than adding a defensive check. Fix this before resuming REQ-029/DEC-041's build — T-173 cannot exercise its own intended assertions while this unrelated fault aborts the process first (user decision 2026-09-08).

### Alignment probe
grep -n "RideFilePoint \*newest" src/FileIO/RideFile.cpp        # expect the new alias local inside appendOrUpdatePoint
grep -n "updateMin(point)\|updateMax(point)\|updateAvg(point)" src/FileIO/RideFile.cpp   # expect 0 once the fix lands — the tail calls take the alias, not `point`
grep -n "REQ-030" .claude/workflow-garminconnect/traceability.md   # expect it to name a built DES/TEST once closed

## DEC-045 — REQ-014 error-translation locus + keying: confirm DES-008's page-layer design, key on `GarminAuthFailure::Kind` not the raw exception class name

- Status: accepted (recording-only — the builder already implemented and committed this resolution while building REQ-014; this entry formalizes it in the ledger so the deviation from DES-008's sample has a citable DEC instead of living only in a commit message and code comment)
- Reversibility: cheap (keying scheme only; no signature changes beyond what is already committed)
- Decided / last-reviewed: 2026-09-10 (retroactively recorded; the build landed 2026-09-08 in `ac1fa40ba`)
- Serves: REQ-014
- Dependents: `GarminErrors.h`/`.cpp` (the switch table itself), `GarminCredentialsPage.cpp`, `GarminMfaPage.cpp` (the only two translation call sites)
- Research: builder found the codebase's own `design.md` DES-008 already establishes page-layer translation (`IGarminPyAdapter.h` cites it directly in a comment); a deeper look then found the existing LOCKED test suite was self-contradictory about where translation happens (`testGarminConnectAuthClient.cpp` assumed worker-raw/page-translates; the page test files had hand-constructed pre-translated strings implying the opposite) — resolved in DES-008's favor, not re-litigated.

### The problem
Two related open questions surfaced while building REQ-014:
1. **Where does translation happen** — worker/adapter, or page? `design.md`'s DES-008 already says page layer (`GarminCredentialsPage`/`GarminMfaPage`), and `IGarminPyAdapter.h` cites it in a comment — but the existing test suite pulled in both directions: `testGarminConnectAuthClient.cpp` assumed the worker hands back raw, untranslated text; the page-layer test files (`testGarminConnectCredentialsPage.cpp`, `testGarminConnectMfaPage.cpp`) had hand-built pre-translated message strings, implying translation had already happened before the page saw them.
2. **What key drives the switch table** — DES-008's own sample (`design.md:723-757`) keys `kErrorMessages` on the raw Python exception class name (e.g. `"GarminConnectAuthenticationError"`). That string does not survive the pipeline: the Python adapter collapses it into a GC-stable `kind` string (`garmin_client.GarminError.kind`), and the C++ seam collapses that again into `PyAuthOutcome::Kind` / `GarminAuthFailure::Kind`. Nothing downstream of the adapter retains the original exception class name.

### Resolution
1. **Locus** — DES-008's page-layer design stands. Worker/adapter (`GarminWorker.cpp`; `PyAuthOutcome::rawMessage` and `GarminAuthFailure::translatedMessage` carry the raw library message verbatim per `IGarminPyAdapter.h`) never translates; `GarminCredentialsPage`/`GarminMfaPage` are the ONLY call sites for `GarminErrors::translate()`. The self-contradictory locked tests were the bug, not the design doc — `testGarminConnectAuthClient.cpp` was corrected to assert raw passthrough (matching the doc), not translated text.
2. **Keying** — `GarminErrors::translate()` takes a `GarminAuthFailure::Kind` (`Auth`/`Network`/`RateLimit`/`Unknown`), not a raw exception-class-name string. This deviates from DES-008's sample but is the only keying that actually survives the pipeline intact; see the dated DES-008 addendum this DEC references.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A confirm page-layer locus + Kind-keyed switch — CHOSEN** | 5 — matches the one thing downstream of the adapter that is actually still true (the Kind), no lossy string round-trip | 5 — adding a new Kind is one enum member + one switch arm, same cost regardless of locus | 5 — one translation site, one already-locked design doc now consistent with its own tests | 5 — resolves the doc/test contradiction in the doc's favor rather than picking a side arbitrarily |
| B move translation to the worker, key on Kind | 4 — same keying benefit | 4 — same | 2 — contradicts DES-008 outright, forces a `design.md` rewrite plus rework of every test file the locked suite already got right | 2 — no stated reason DES-008's locus choice was wrong; only the *sample's* key was |
| C keep DES-008's raw-class-name sample literally, thread the string through the pipeline | 2 — requires plumbing a new field end-to-end (adapter → worker → `GarminAuthFailure`) whose sole purpose is a string already representable by 4 enum values | 3 — one more field to keep in sync per new failure kind | 2 — two parallel representations of "what went wrong" (Kind AND class-name string) that can drift | 2 — no benefit over Kind; strictly more surface for the same information |

### Cascade impact
- A (chosen): no cascade beyond the already-committed REQ-014 diff (`ac1fa40ba`) — this entry documents a resolution already built, not a pending change.
- B: would require reopening REQ-014's build, moving the switch table + its tests into `GarminWorker.cpp`, and rewriting `IGarminPyAdapter.h`'s existing raw-passthrough contract.
- C: would require a new field on `PyAuthOutcome`/`GarminAuthFailure`, threaded from `garmin_client.py` through `PyEmbeddedAdapter.cpp` and `GarminWorker.cpp`, unused by anything except a switch table already served by Kind.

### Chosen
A — strictly dominates B and C; matches DES-008's own stated locus, and keys on the only value that actually reaches the page layer intact. Recorded retroactively (build + commit predate this ledger entry by two days) so the DES-008 deviation has a citable DEC.

### Alignment probe
grep -rn "GarminErrors::translate" src/Cloud/GarminCredentialsPage.cpp src/Cloud/GarminMfaPage.cpp src/Cloud/GarminWorker.cpp   # expect hits in the two page files only, zero in GarminWorker.cpp
grep -n "GarminAuthFailure::Kind" src/Cloud/GarminErrors.h   # expect translate()'s parameter type
grep -n "DES-008 addendum" .claude/workflow-garminconnect/design.md   # expect the dated addendum this DEC references

## DEC-046 — REQ-015 CAPTCHA detection: defer, no structured signal survives in the real dependency

- Status: accepted (user decision 2026-09-10, option 2 of 3 presented)
- Reversibility: cheap — nothing built, nothing to unwind; revisit if the dependency changes or a dependency-level fix is separately scoped
- Decided / last-reviewed: 2026-09-10
- Serves: REQ-015
- Dependents: none (no code exists yet for this REQ; this DEC blocks a build attempt until its premise changes)
- Research: two independent passes, both reading the actual `garminconnect==0.3.13` wheel source (not documentation) — (1) a `qgdw-builder` session, (2) a separately-dispatched fresh Codex session given only the narrow question "is there a structured, non-message-text way to detect this," with no knowledge of the first pass's conclusion. Both reached the same result independently.

### The problem
REQ-015 (prd.md) requires GC to detect a Garmin CAPTCHA challenge via "a recognisable signal or specific HTTP status" and show a dedicated dialog. This project's LSN-006 requires classifying errors by exception TYPE, never by scanning message text. The two research passes established, by reading `garminconnect/client.py` and `garminconnect/exceptions.py` directly:
- Garmin's server signals CAPTCHA as an ordinary HTTP 200 whose JSON body contains `responseStatus.type == "CAPTCHA_REQUIRED"` — a real structured signal exists on the wire.
- The library itself discards it: both the mobile-flow and portal-flow login paths (`client.py:696`, `client.py:1104`) see this field, then raise a bare `GarminConnectConnectionError("<string>")` — no response object, no parsed body, args is just the string. The 5-strategy retry loop (`client.py:550`) catches, retries, and on exhaustion builds a **fresh** generic exception with only sanitised text (no `raise ... from` chain preserving the original). The public `Garmin.login()` (`__init__.py:798`) wraps that once more.
- `GarminConnectConnectionError` (`exceptions.py:4`) does declare a `.response` attribute — the one property that looked like it might carry the structured field through — but it is `None` by construction on this path; it is only ever populated by an unrelated post-auth API-call decorator, never by the SSO login code. This was the specific thing checked and ruled out, not assumed.
- No lower-level interception point exists either: `Garmin.__init__` takes no session/client/hook injection argument and constructs its own `client.Client`; each login strategy builds its own throwaway `requests`/`curl_cffi` session internally, not retained or exposed.
- Correction to an earlier assumption: 0.3.13 does not depend on or import `garth` — the SSO implementation is entirely the library's own code (`client.py:473`).

So the only signal surviving to `garmin_client.py`'s call site, at any layer, is a free-text substring ("CAPTCHA required") inside `str(exception)` — exactly what LSN-006 forbids keying on.

### Resolution
Defer REQ-015. Do not build CAPTCHA detection now, and do not add message-substring matching to work around the gap. A CAPTCHA-triggered login continues to fall through as today's generic `Unknown`/`Auth` failure (REQ-014's existing translation) until either the dependency changes upstream, or a separately-scoped dependency-level fix (forking/patching `garminconnect` to add a real `GarminConnectCaptchaRequiredError` carrying the response) is decided and built as its own piece of work — not as part of REQ-015's original adapter-only scope.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **B defer, no build — CHOSEN** | 5 — no risk introduced; today's generic-error fallback is unchanged, known behavior | 5 — nothing to maintain | 5 — no code, no LSN-006 exception to track or eventually clean up | 5 — doesn't compromise the classify-by-type rule for one feature |
| A scoped LSN-006 exception (message-substring match, documented) | 3 — brittle by nature; a library text-string change silently breaks detection with no compile-time signal | 3 — same brittleness compounds with every future locale/library-version change | 2 — a permanent, deliberately-carved hole in an otherwise-enforced rule | 2 — matches design.md's original (pre-LSN-006) "heuristic" framing, but contradicts the rule as currently written |
| C fix upstream (fork/patch `garminconnect` to preserve the structured signal) | 5 — the only option that actually satisfies LSN-006 | 4 — one-time dependency-level fix, but now GC carries a fork/patch maintenance burden across upstream releases | 3 — real fix, but cross-repo scope, its own tests, its own packaging path (REQ-NF-Pkg-001 territory) | 5 — correct fix, wrong size for this REQ as scoped |

### Cascade impact
- B (chosen): none — no code exists to touch. `traceability.md`'s REQ-015 row is updated to cite this DEC and record the research conclusion in place of the prior "not started" note.
- A: would need its own DEC superseding this one, a code comment at the one detection site citing that DEC, and an explicit LSN-006 scope note.
- C: would need a new REQ (or an amendment widening REQ-015's scope) plus its own DEC, targeting the dependency itself rather than `garmin_client.py`; likely folds into REQ-NF-Pkg-001's dependency-packaging territory.

### Chosen
B — the real fix (C) is legitimate but out of proportion to REQ-015 as scoped (an adapter-only requirement) and CAPTCHA is a rare, low-stakes path (worst case: a slightly-generic error message, not silent data loss or a security gap). A (bending LSN-006) was rejected as a standing brittleness with no compile-time guard, for a feature this narrow. Revisit if `garminconnect` ships a fix, or if a dependency-level fix is separately proposed and scoped.

### Alignment probe
grep -n "REQ-015" .claude/workflow-garminconnect/traceability.md   # expect the row to cite DEC-046
find src/Cloud -iname "GarminCaptchaPage*"   # expect: no matches (nothing built)
git log --oneline -- src/Cloud/GarminCaptchaPage.h src/Cloud/GarminCaptchaPage.cpp   # expect: no output

## DEC-047 — REQ-010/DES-009 pagination model: per-activity pacing/checkpointing, not per-page, against the real `garminconnect` dependency

- Status: accepted (recording-only — the builder already implemented and the reviewer already confirmed this resolution while building REQ-010; this entry formalizes it in the ledger so the deviation from DES-009's sample has a citable DEC instead of living only in a build report and code comment)
- Reversibility: cheap — no signature changes beyond what is already built; a future upstream library change that exposes real page cursors would let a later REQ re-tighten pacing without touching the resumability contract
- Decided / last-reviewed: 2026-09-10
- Serves: REQ-010, DES-009, DES-005
- Dependents: `GarminBackfillController.{h,cpp}`, `gc_rate.py` (the only two implementation sites), STATE.md's Stage 7/8 narrative (REQ-NF-Reliab-001/002 also depend on DES-005)
- Research: builder confirmed against the real `garminconnect==0.3.13` wheel (not the fake/pystub) that `get_activities_by_date()` performs its own 20-per-page retrieval entirely inside one call — GC's adapter receives one aggregated result with no page cursor exposed. Reviewer delta-check independently confirmed the same reading of the dependency and traced the resulting rate-limiting gap (see B-R010-03).

### The problem
DES-009's "Paging" state (`design.md:807`) specifies 20 activities/page with a 1s inter-page delay "enforced" by DES-005, and DEC-007 formalized "pace calls through the rate limiter" as the decision. That model assumes GC's adapter drives the paging loop itself, one wire round-trip per page. The real dependency does not expose that seam: `list_activities_since()` returns only after the library has already fetched every page for the requested range internally. There is nothing for `GarminBackfillController` to pace *between pages* — the page boundary is invisible outside the library call.

### Resolution
1. **Pacing locus** — `gc_rate.py`'s `rate_limited`/`with_retry` decorators wrap `list_activities_since()` and `download_activity()` at the method-call level (one paced call per listing, one paced call per download), not a per-page loop. This is the only pacing granularity the real dependency's seam actually allows.
2. **Checkpointing locus** — `GarminBackfillController` checkpoints (persists resume cursor + dedup record) per-activity rather than per-page. This is finer-grained than DES-009's per-page sketch, not coarser, so resumability is not weakened — a crash mid-page now loses at most one activity's progress instead of a full page's.
3. **Accepted residual gap** — the *listing* call's internal multi-page fetch has no rate limiting applied to its own inner requests (only the outer call is paced); a large date range can still cause Garmin-side burst traffic during that one call. Tracked as B-R010-03, accepted with note, not fixed — see that finding for why (no seam exists to fix it at this layer; a real fix would require either upstream library changes or a lower-level HTTP-layer interception, out of proportion to REQ-010 as scoped).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A per-activity pacing/checkpointing, listing-call burst gap accepted — CHOSEN** | 4 — downloads fully paced; listing bursts are a real but narrow residual (one call per backfill run, not per activity) | 5 — no new seam invented; matches what the dependency actually exposes | 5 — one pacing locus (method-call), one checkpoint locus (per-activity), both simpler than a fictitious per-page loop | 4 — resolves the doc/dependency mismatch honestly rather than fabricating a page cursor that doesn't exist |
| B fabricate a page-level seam by re-implementing `get_activities_by_date`'s HTTP calls directly in the adapter (bypass the library's own pagination) | 3 — now GC owns Garmin API pagination semantics directly, a second place they can drift from upstream | 2 — couples `garmin_client.py` to Garmin's raw REST shape instead of the maintained library's abstraction | 1 — defeats DES-012's entire purpose (a stable seam over the library specifically so GC doesn't have to track Garmin's API directly) | 2 — technically satisfies DES-009's literal number, at the cost of the adapter-seam design DES-012 already established |
| C do not build DES-005/REQ-010 at all until upstream exposes page cursors | 5 — no risk, nothing built | 5 — nothing to maintain | 5 — no code | 1 — blocks REQ-010 and Stage 8's REQ-NF-Reliab-001/002 indefinitely on a change with no known timeline |

### Cascade impact
- A (chosen): no cascade beyond the already-built REQ-010/DES-005 diff (uncommitted, working tree) — this entry documents a resolution already built and reviewer-confirmed, not a pending change. `design.md`'s DES-009 section should get a short dated addendum (mirroring the DES-008 addendum DEC-045 references) noting the per-activity-not-per-page reality; not yet written, tracked alongside the REQ-010 ledger update.
- B: would require reopening the REQ-010/DES-012 build, moving raw HTTP pagination logic into `garmin_client.py`, and a new set of tests mocking Garmin's REST responses directly instead of the library's Python interface.
- C: blocks REQ-010 and, transitively, Stage 8's REQ-NF-Reliab-001/002 (both already list DES-005 as a dependency) with no forcing function to ever unblock.

### Chosen
A — strictly dominates B (which breaks DES-012's adapter-seam design for a number that isn't achievable anyway) and C (which blocks two REQs on an indefinite upstream change). The listing-call burst residual (B-R010-03) is real but narrow and accepted with note rather than fixed at disproportionate cost.

### Alignment probe
grep -n "get_activities_by_date" src/Python/garminconnect/garmin_client.py   # expect the single aggregated call site
grep -n "rate_limited\|with_retry" src/Python/garminconnect/gc_rate.py src/Python/garminconnect/garmin_client.py   # expect decorators on list_activities_since/download_activity/login/submit_mfa only
grep -n "B-R010-03" .claude/workflow-garminconnect/findings.md   # expect the accepted-with-note residual this DEC references

## DEC-048 — B-R010-04 UI wiring: fix the new backfill-local exposure now, defer GarminConnect's own internal context-handling gap

- Status: accepted (recording-only — the user decided the scope split live during the session; this entry formalizes it in the ledger)
- Reversibility: cheap — the deferred fix is additive (making `GarminConnect`/`CloudService`'s internal context handling UAF-safe does not require undoing anything built here); no signature changes beyond what is already built
- Decided / last-reviewed: 2026-09-11
- Serves: REQ-010, DES-009 (B-R010-04's UI wiring)
- Dependents: `GarminBackfillDialog.cpp` (the narrow fix site), `GarminConnect.cpp`/`CloudService.cpp` (the deferred fix's eventual site, not yet touched)
- Research: a reviewer's final confirmation pass on the B-R010-09 fix (continuously-tracked `QPointer<Context>` in `GarminBackfillDialog`) found that the new `SessionCheck` callback (B-R010-08's fix) still reaches `GarminConnect::backfillSessionStillValid()` → `sessionSuperseded()`/`accountStillConnected()` → `resolveConfigDir()`, and that last call chain dereferences `GarminConnect`/`CloudService`'s own internal RAW context pointer — a pre-existing limitation, not introduced by this backfill work, already shared by the committed REQ-007/008/012/017 incremental-sync paths (`readdir()`/`readFile()` reach the identical call chain). The backfill work's own contribution was adding ONE MORE caller (the controller's three per-activity session-check points) to an already-fragile path.

### The problem
Two different bugs were tangled together in one finding: (1) a narrow, dialog-local gap — the `SessionCheck` lambda called into `GarminConnect` unconditionally, even when the dialog's own (by-then-already-fixed, continuously-tracked) `context` had gone null; (2) a deep, pre-existing gap — `GarminConnect`/`CloudService` itself has never made its own internal context pointer UAF-safe, for ANY of its callers, not just this new one. Fixing only (1) closes the specific NEW exposure this session's work added. Fixing (2) would require touching `GarminConnect.cpp`/`CloudService.cpp`'s foundational session/context-handling code — code shared by every already-shipped Garmin sync path (REQ-007/008/012/017), not scoped to backfill.

### Resolution
1. **Fix now (B-R010-09's final piece)** — the `SessionCheck` lambda in `GarminBackfillDialog::startClicked()` now short-circuits on `!context.isNull()` (the dialog's own continuously-tracked guard) BEFORE ever calling `backfillStore->backfillSessionStillValid()`. Entirely dialog-local; `GarminConnect.cpp`/`CloudService.cpp` untouched.
2. **Defer, do not fix (B-R010-10)** — `GarminConnect`/`CloudService`'s own internal raw-context dereference in `resolveConfigDir()` (reached via `sessionSuperseded()`/`accountStillConnected()`) stays as-is. Recorded as a new, non-blocking finding for a future task; no REQ allocated yet.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A narrow dialog-local fix now, defer the foundational gap — CHOSEN** | 4 — closes the specific new exposure this session added; the foundational gap is real but pre-existing and no more likely to fire via backfill than via the already-shipped sync paths | 5 — no new seam invented, no scope creep into shared code | 5 — one small, self-contained fix; the deferred item is clearly tracked, not silently dropped | 4 — resolves the reviewer's finding honestly at the scope it was actually introduced at, rather than either ignoring it or overreaching |
| B fix everything now, including `GarminConnect`/`CloudService` internals in this same session | 3 — technically the most complete fix, but expands a UI-wiring follow-on into a foundational-code change late in a long session, raising the chance of a rushed mistake in shared, heavily-relied-upon code | 2 — touches code shared by REQ-007/008/012/017, none of which are in this task's test scope, so a regression there would not be caught by this session's own test runs | 2 — conflates two independently-shippable pieces of work into one commit, making a future revert or bisect harder | 3 — more thorough, but violates the principle of matching fix scope to where a defect was actually introduced |
| C stop here, accept the whole Context-lifetime class (including the narrow dialog-local gap) as one disclosed residual | 3 — leaves a cheap, already-diagnosed, already-understood fix un-applied for no real savings | 5 — nothing further to build | 5 — no code | 2 — leaves an easy, low-risk fix on the table for no benefit, when the harder foundational piece is genuinely the only part worth deferring |

### Cascade impact
- A (chosen): no cascade beyond `GarminBackfillDialog.cpp`'s own diff, already built, tested (T-196), and committed `2b8cedae3`. B-R010-10 sits in findings.md as a non-blocking, no-REQ-allocated item; whoever eventually resourced it would need a fresh REQ/DES slice touching `GarminConnect.cpp`/`CloudService.cpp`'s session-latch machinery directly.
- B: would reopen `GarminConnect.cpp`/`CloudService.cpp` for edits this session had no test coverage or time budget planned for, risking a late-session regression in code four other REQs depend on.
- C: leaves T-196's easy, already-diagnosed fix unapplied — no actual benefit over A, since the fix was already built and verified before this DEC was written.

### Chosen
A — the cheap, already-built, already-tested dialog-local fix closes the exposure this session's own work introduced; the foundational `GarminConnect`/`CloudService` context-handling gap is real but pre-existing, not scoped to backfill, and deserves its own REQ/DES treatment rather than a late-session patch.

### Alignment probe
grep -n "context.isNull()" src/Cloud/GarminBackfillDialog.cpp   # expect the SessionCheck lambda's short-circuit guard
grep -n "B-R010-10" .claude/workflow-garminconnect/findings.md   # expect the deferred, non-blocking residual this DEC references
git log --oneline -1 -- src/Cloud/GarminConnect.cpp src/Cloud/CloudService.cpp   # expect no commit from this session touching either file

## DEC-049 — REQ-NF-Pkg-001 scope expansion: port Garmin sources into the qmake release build before any installer/packaging work

- Status: accepted (user decision, 2026-09-11, live during session)
- Reversibility: cheap to start, moderate to fully undo once landed — the new `contains(DEFINES, "GC_WANT_GARMINCONNECT")` block in `src/src.pro` and the `gcconfig.pri.in` toggle are additive and flag-gated (default OFF, matching `GC_WANT_PYTHON`/`GC_WANT_R`'s existing pattern), so reverting is a clean file-level revert with no cascading signature changes; the 3 CI `before_build` script edits are equally additive/mechanical
- Decided / last-reviewed: 2026-09-11
- Serves: REQ-NF-Pkg-001 (PRD text: "Installer bundles `garminconnect`+`curl_cffi` **when the build flag is ON**")
- Dependents: `src/src.pro`, `src/gcconfig.pri.in`, `appveyor/linux/before_build.sh`, `appveyor/macos/before_build.sh`, `appveyor/windows/before_build.ps1` (this cycle); `src/Python/requirements.txt` + installer manifest deltas (deferred follow-on, same REQ)
- Research: independently confirmed (grep across `src/src.pro`, `build.pro`, `appveyor.yml`, `docs/MODERNIZATION.md`, `docs/BUILD_NOTES.md`) that GoldenCheetah's actual CI/release pipeline (`appveyor.yml`) builds exclusively via **qmake** (`qmake build.pro` → `src/src.pro`), not CMake. `docs/MODERNIZATION.md` explicitly documents CMake as an "alternative"/"New Method" build with "CI/CD integration with CMake" still an open, unchecked roadmap item. Every Garmin source file across all of Stage 1-7 (`GarminConnect.cpp` and ~15 more) was added only to `src/CMakeLists.txt`'s `GC_WANT_GARMINCONNECT` block (added `aa6131add` and earlier) — `grep -in garmin src/src.pro` returns zero matches. REQ-NF-Pkg-001's own PRD acceptance text ("bundles ... when the build flag is ON") presupposes a working `GC_WANT_GARMINCONNECT` qmake build that does not exist — an unstated dependency inside the requirement's own acceptance criteria, not previously surfaced by any A1/A2 red-team pass because those passes evaluated the CMake-side implementation, which does compile.

### The problem
REQ-NF-Pkg-001 was queued as "installer bundles two Python packages" — a small, low-risk packaging task. Investigation before dispatch found the real gap is one level deeper: the qmake build that `appveyor.yml` actually runs to produce the Windows NSIS installer, macOS DMG, and Linux AppImage contains **zero Garmin code at all**. Bundling `garminconnect`/`curl_cffi` into that installer would be necessary but not sufficient — the resulting binary still would not have the feature compiled in, so REQ-NF-Pkg-001's PRD acceptance could not actually be met by packaging work alone.

### Resolution
Split REQ-NF-Pkg-001 into two sequenced pieces under the same REQ:
1. **This cycle** — port the ~16 Garmin `.cpp`/`.h` files into `src/src.pro` via a new `contains(DEFINES, "GC_WANT_GARMINCONNECT")` block (mirroring the existing `GC_WANT_PYTHON` block at `src/src.pro:220-243`, which already solves embedded-CPython include/lib resolution per-platform — `src/gcconfig.pri.in:74-90`'s `python3-config` auto-detect fallback, and all 3 `appveyor/*/before_build.{sh,ps1}` scripts' existing enable-the-flag pattern). Verified locally by an actual `qmake6 && make` attempt with the flag ON, not just source-diff review.
2. **Deferred, same REQ, next cycle** — `src/Python/requirements.txt` + installer-manifest deltas (the originally-scoped packaging work), now meaningful because step 1 will make the flag reachable in the real release build.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **A port qmake first, packaging second, same REQ — CHOSEN** | 5 — fixes the actual blocker; packaging work done before this would be dead weight | 4 — mirrors an established per-feature qmake pattern (`GC_WANT_PYTHON`/`GC_WANT_R`), no new mechanism invented | 4 — flag-gated, additive, reversible; sequencing keeps each half independently verifiable (compiles first, then packages) | 5 — matches the project's own stated `contains(DEFINES, ...)` idiom for every other optional feature |
| B proceed with packaging only, as originally PRD-scoped; open a separate future REQ for the qmake port | 2 — ships something that still cannot reach a user even after "closing" REQ-NF-Pkg-001, misrepresenting the ledger's meaning of closure | 4 — low risk, small diff | 3 — leaves a known-insufficient REQ marked closed against its own PRD acceptance text, which the ledger's own governance rules treat as a real problem, not paperwork | 2 — technically satisfies literal packaging text while leaving the requirement's actual intent (user can use the feature) unmet |
| C treat this as a brand-new REQ (e.g. REQ-NF-Pkg-002) rather than extending REQ-NF-Pkg-001 | 4 — equally correct technically | 4 — same | 3 — splits one PRD requirement's closure across two REQ ids in the ledger, adding bookkeeping overhead for no traceability benefit since both trace to the identical PRD row | 3 — no established precedent in this ledger for this kind of split; REQ-010's own B-R010-04 follow-on precedent instead reused the SAME REQ id for a same-REQ scope split |

### Cascade impact
- A (chosen): `src/src.pro`, `src/gcconfig.pri.in`, 3 CI before_build scripts touched this cycle; `unittests/unittests.pro` (qmake test project) deliberately OUT of scope — Garmin tests continue running only via the existing CMake/ctest path (`garmin-fast` label), a pre-existing test-parity gap this DEC does not attempt to close. No other REQ's files touched.
- B: would require a second full dispatch cycle later to actually fix the real blocker, after having already marked REQ-NF-Pkg-001 "closed" once — a ledger integrity problem matching the class of near-miss in [[garmin-ledger-governance-mechanics]].
- C: purely a ledger bookkeeping choice; deferred, not blocking, revisit only if the split REQ id ever causes real traceability confusion.

### Chosen
A — port first, package second, same REQ. Matches the user's explicit instruction ("port Garmin sources into qmake first") and an established in-project precedent (`GC_WANT_PYTHON`) for exactly this shape of optional-feature qmake integration.

### Alignment probe
grep -n "GC_WANT_GARMINCONNECT" src/src.pro   # expect a new contains(DEFINES, ...) block, mirroring GC_WANT_PYTHON's shape
grep -n "GC_WANT_GARMINCONNECT" src/gcconfig.pri.in   # expect a new commented-out toggle line, alongside GC_WANT_R/GC_WANT_PYTHON
grep -rn "GC_WANT_GARMINCONNECT" appveyor/   # expect enable-lines in all 3 before_build scripts
git log --oneline -1 -- src/Python/requirements.txt   # expect NO commit yet from this cycle (deferred to step 2)

### Step 1 outcome (2026-09-11)
Built and independently re-verified GREEN by the orchestrator (real `qmake6`+`make` runs, not just the
builder's report). One real complication surfaced and was resolved during the build: `Cloud/PyEmbeddedAdapter.cpp`
needs Python.h ahead of any Qt header (documented in its own file comment — Qt's `slots` macro collides with a
`slots` field in CPython's `object.h`), but this project's qmake build forces a global PCH (`stable.h`, pulling in
Qt) on Linux/Windows. The builder's first fix (`<file>.CONFIG -= precompile_header`, the existing pattern already
used for `.c` files at `src/src.pro:874-884`) was confirmed a no-op for `.cpp` files under qmake's unix/GCC PCH
generator. Its second fix (a hand-rolled `QMAKE_EXTRA_COMPILERS` rule) compiled correctly but omitted
header-dependency tracking — a real incremental-rebuild staleness risk (a future header-only edit could leave the
object stale). This was caught by a user-directed parallel Codex investigator, dispatched read-only into an
isolated `/tmp` scratch reproduction alongside the builder (no shared working-tree risk), which verified both the
original PCH-exclusion failure and the dependency-tracking gap against real generated Makefiles, and found that
qmake ships a built-in (undocumented publicly, but real and shipped in `precompile_header.prf`) `NO_PCH_SOURCES`
mechanism that handles both correctly. The builder adopted it: `contains(DEFINES, "GC_WANT_GARMINCONNECT") { macx
{ SOURCES += Cloud/PyEmbeddedAdapter.cpp } else { NO_PCH_SOURCES += Cloud/PyEmbeddedAdapter.cpp } }`. The
orchestrator independently reproduced both build variants and the header-dependency fix from scratch (not reusing
the builder's binaries) before recording this outcome. Diff scope: exactly the 5 files listed under "Dependents"
above, +126 lines, all additive, no governance files touched by the builder. No new DEC needed for this
complication — resolved within DEC-049's own step 1 scope.

Probe: touch src/Cloud/PyEmbeddedAdapter.h && (cd src && make -j$(nproc)) 2>&1 | grep PyEmbeddedAdapter.cpp
  # expect exactly one recompile line for this file (no -include GoldenCheetah/stable.h flag on it), confirming
  # both the PCH exclusion and header-dependency tracking are still correct

## DEC-050 — REQ-013 scope: narrow the first slice to DOB/weight/height, defer HR-max/FTP

- Status: accepted (user decision 2026-09-12, option 1 of 3 presented)
- Reversibility: cheap — nothing built yet for this REQ; the deferred fields are additive (DES-011's `{dob, weight_kg, height_cm}` return shape already has room to widen to `hr_max, ftp_w` later without a breaking change)
- Decided / last-reviewed: 2026-09-12
- Serves: REQ-013
- Dependents: none yet (pre-build scope decision, same posture as DEC-046)
- Research: read the real `garminconnect==0.3.13` wheel (downloaded via `pip download`, extracted, inspected `__init__.py`/`typed.py` directly — same rigor as DEC-046) and GC's own data model (`src/Core/Athlete.cpp`, `src/Core/Settings.h`, `src/Core/DataFilter.cpp`)

### The problem
DES-011 specifies a `FetchProfile` op returning `{dob, weight_kg, height_cm, hr_max, ftp_w (if any)}`, all filled into GC's Athlete profile if currently empty. Two gaps surfaced on inspection, before any code was written:
- **Schema risk (all 5 fields):** the real library exposes plausible endpoints (`get_userprofile_settings()` → `/userprofile-service/userprofile/settings`; dedicated `get_cycling_ftp()`, `get_heart_rate_zones()`) but none are typed in the library (`typed.py` has no profile/settings model) and this project has never tested against a live Garmin account (`traceability.md`'s own "LIVE-SERVICE TESTED: NO — never, on any requirement" row). The exact raw JSON field names (e.g. `birthDate` vs `dateOfBirth`) cannot be confirmed from source alone. Unlike DEC-046's CAPTCHA case this is not a proven dead end — REQ-013's own "only fill missing fields" contract already degrades safely (a wrong/absent key name just means that field no-ops) — but it is a real, unverified external contract worth recording rather than silently assuming.
- **Architectural mismatch (hr_max/ftp_w only):** GC's DOB/weight/height map cleanly to existing simple per-cyclist app-settings scalars (`GC_DOB`/`GC_WEIGHT`/`GC_HEIGHT`, `Settings.h:279-281`, read via `appsettings->cvalue(cyclist, KEY)`). HR-max and FTP do NOT — in GC's real data model both live inside the date-ranged Zones/CP system (`Athlete::hrZones(sport)->getMaxHr(range)`, `DataFilter.cpp:4034`), not a simple scalar. "Filling if missing" for these two would mean creating or editing a dated zone range, a materially bigger and riskier change than a `setCValue` call, disproportionate for a `nice`-priority REQ.

### Resolution
Build REQ-013's first slice against `{dob, weight_kg, height_cm}` only, via the existing per-cyclist app-settings scalars, with defensive (try-multiple-candidate-keys, skip-if-absent) parsing of the real library's unverified JSON shape. Defer `hr_max`/`ftp_w` to a follow-up slice once the Zones-system interaction is separately designed — DES-011's own "(if any)" qualifier already anticipated these two being softer-effort than the other three.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **B narrow to dob/weight/height, defer hr_max/ftp_w — CHOSEN** | 4 — builds against a schema risk that degrades safely (absent field = no-op, not a crash), skips the higher-risk Zones-system surface entirely for now | 5 — the 3-field shape needs no Zones-system design work at all | 5 — small, reversible, additive; matches DES-011's own "(if any)" softness for the two deferred fields | 4 — proportionate to a `nice`-priority REQ; doesn't block on an unresolved Zones-interaction design |
| A build the full DES-011 scope now (all 5 fields) | 3 — same schema risk PLUS new Zones-system write logic, for a nice-priority item | 2 — Zones date-range writes are a new, more complex mechanism with its own edge cases (no current range? overlapping range? which sport?) | 2 — materially larger surface for later maintenance, undesigned today | 2 — disproportionate effort for a `nice` REQ relative to its value |
| C defer REQ-013 entirely | 5 — no risk introduced | 5 — nothing to maintain | 5 — no code | 2 — treats a degrades-safely schema risk the same as DEC-046's proven dead end, which it is not; throws away the 3 low-risk fields along with the 2 higher-risk ones |

### Cascade impact
- B (chosen): `design.md`'s DES-011 narrowed to the first-slice return shape; `hr_max`/`ftp_w` recorded as an explicit follow-up, not silently dropped. `traceability.md`'s REQ-013 row updated to cite this DEC.
- A: would need its own DEC for the Zones-system write mechanism (which range to edit, how to handle no-existing-range, sport selection) before any code.
- C: would need REQ-013's traceability row rewritten to DEFERRED, matching REQ-015's shape; revisit only if the user decides the 3 low-risk fields aren't worth it either.

### Chosen
B — matches the user's explicit instruction and keeps the REQ buildable now without inventing Zones-system design work it doesn't need yet.

### Alignment probe
grep -n "REQ-013" .claude/workflow-garminconnect/traceability.md   # expect the row to cite DEC-050
grep -n "hr_max\|ftp_w" .claude/workflow-garminconnect/design.md   # expect DES-011 to mark these deferred, not silently removed
find src/Cloud -iname "GarminProfile*"   # expect: no matches yet (nothing built until the builder dispatch runs)

---

## DEC-051 — REQ-NF-Obs-001's "ErrorBus" clause: satisfied by the project's real error channels; build only the missing structured trace logging

- Status: accepted (Inspector, Three-Options Doctrine — an ordinary architecture reconciliation against a design-doc premise already disproven by DEC-022, not a human-in-the-loop gate)
- Reversibility: cheap — this entry changes no code, only which existing mechanism REQ-NF-Obs-001's first clause is scored against; the new `qDebug` logging it authorizes is additive
- Decided / last-reviewed: 2026-09-12
- Serves: REQ-NF-Obs-001
- Dependents: `GarminConnect.{h,cpp}` (sync op sites: open/readdir/readFile/backfill), any new structured-log test
- Origin: dispatching Stage 8's next atomic unit surfaced that DES-008's "ErrorBus" sample (`design.md:774`) describes a channel DEC-022 already found **does not exist in the tree** ("`ErrorBus` DOES NOT EXIST — only DES-008 prose and a TODO at `GarminConnect.cpp:649`"), and DEC-023 already chose a different, real replacement (the `readFailed(QByteArray*, QString, QString)` signal) for the one place that gap actually blocked shipping. REQ-NF-Obs-001 was never updated to reflect that.

### The problem
`design.md:774-786`'s DES-008 sample has every sync op emit an `ErrorBus::emit({...})` struct with fields `{service, op, severity, duration_ms, activity_count, garmin_error_code, message}`. No `ErrorBus` class or free function exists anywhere in `src/` (confirmed by grep — the only three hits are `IGarminPyAdapter.h`/`GarminWorker.h`/`GarminConnect.cpp` referencing REQ-NF-Obs-001 in comments, not an implementation). DEC-022 (2026-08-04) already surfaced and rejected building this exact thing as "Option B — new out-of-band error channel (ErrorBus)... REJECTED: ErrorBus DOES NOT EXIST in the tree... It would be a new subsystem". DEC-023 (2026-08-04) chose instead a narrowly-scoped `readFailed` signal on `CloudService`, now built and tested, that already carries user-facing sync errors to both `CloudServiceSyncDialog` and `CloudServiceAutoDownload`. Separately, `GarminConnect::open()`/`readdir()` already surface errors via the existing `QStringList& errors` out-param convention shared by every `CloudService` (extensively tested — e.g. `openFailsWhenRestoreFails`, `listFailureSurfacesViaErrorsAndReturnsEmptyList`). REQ-NF-Obs-001's user-facing-errors clause is, in substance, already met by these two real mechanisms; only the requirement's own wording still names the never-built one. The genuinely open piece is DES-008's *second* sentence — "Structured `qDebug` mirrors the same fields for developer trace logging" — no such structured logging exists anywhere in the Garmin sources today (grepped for a duration/activity-count/error-code-keyed `qDebug` call, zero hits).

### Resolution
Score REQ-NF-Obs-001's "user-facing errors" clause as MET via the already-built, already-tested `readFailed` signal (DEC-023) + the `errors` out-param convention (DES-001-era, used throughout `GarminConnect`) — no new subsystem. Build ONLY the missing half: structured `qDebug` developer-trace logging at each sync op boundary (open/readdir/readFile/backfill), carrying the fields DES-008 specifies (op name, duration, activity count, Garmin error code — `service`/`severity` folded in as the op-name prefix and success/failure already implicit in whether an error code is present, since there is no severity enum anywhere else in this codebase to reuse). `design.md`'s DES-008 gets a dated addendum (mirroring DEC-045's own addendum pattern) marking the `ErrorBus` sample superseded-in-practice by DEC-022/023, not deleted (historical record).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **B — reuse `readFailed`+`errors` for the user-facing half, build only `qDebug` trace logging — CHOSEN** | 5 — reuses two mechanisms already shipped and covered by existing tests; zero new failure surface for the user-facing half | 5 — no new subsystem to scale | 5 — avoids a second, parallel error-reporting channel alongside DEC-023's chosen one; one less thing to keep in sync | 5 — directly consistent with DEC-022's own explicit rejection of a new ErrorBus channel; doesn't re-litigate a settled decision |
| A — build the full `ErrorBus` class DES-008 describes | 2 — new, unexercised subsystem; DEC-022 already flagged the identical proposal as leaving errors as "a side channel... while the loop still stalled" for the one case it was tested against | 2 — a second global bus is a wider blast radius than one REQ needs | 1 — now TWO overlapping error-reporting mechanisms (`readFailed`/`errors` AND `ErrorBus`) with no clear rule for which callers use which — a maintenance trap | 1 — contradicts DEC-022's own prior rejection of this exact option with no new information to justify reopening it |
| C — defer REQ-NF-Obs-001 entirely as "not buildable as scoped" (REQ-015/DEC-046 precedent) | 5 — no risk | 5 — nothing to maintain | 3 — leaves a real, easily-closeable gap (no structured trace logging at all) sitting open indefinitely | 2 — REQ-015's dead-end was a genuine external-dependency wall; this REQ has no such wall, just a stale requirement description — treating them the same undersells what's actually buildable |

### Cascade impact
- B (chosen): `traceability.md`'s REQ-NF-Obs-001 row updated to cite this DEC and split the requirement into its two now-separately-scored halves. `design.md`'s DES-008 gets a dated addendum. Builder dispatched to add structured `qDebug` calls + a log-format-review test (prd.md:112's own verification method) at the sync op boundaries.
- A: would need to design the `ErrorBus` class itself (thread-safety, subscriber model, retention) before any REQ-NF-Obs-001 code — a materially bigger unit than this REQ's scope.
- C: would need REQ-NF-Obs-001's row rewritten to DEFERRED, discarding the already-met user-facing-error coverage along with the genuinely-missing trace-logging piece.

### Chosen
B — reuses proven mechanisms, adds the one real gap, and does not reopen a question DEC-022 already answered.

### Alignment probe
grep -rn "ErrorBus" src/Cloud/   # expect: still zero implementation hits (comments only) — B does not build one
grep -n "readFailed" src/Cloud/CloudService.h   # expect: the DEC-023 signal, unchanged
grep -n "qDebug" src/Cloud/GarminConnect.cpp   # expect: new structured calls at sync op boundaries after the builder dispatch runs

## DEC-052 — B-STAGE9-01 fix: shared process-level CPython bootstrap, owned by neither feature alone

- Status: accepted (Inspector, Three-Options Doctrine — ordinary architecture decision within B-STAGE9-01's own scope, not a human-in-the-loop gate; options researched by the reviewer, not the Inspector, per this session's dispatch)
- Reversibility: moderate — touches `main.cpp`, `PythonEmbed.{h,cpp}`, and both build systems' `GC_WANT_PYTHON`/`GC_WANT_GARMINCONNECT` blocks; additive (a new bootstrap call), no removal of existing behavior
- Decided / last-reviewed: 2026-09-13
- Serves: B-STAGE9-01 (blocks Stage 9's real-account connect/MFA/sync/disconnect evidence for REQ-002/003/009/012/017)
- Dependents: `src/Core/main.cpp`, `src/Python/PythonEmbed.{h,cpp}`, `src/Cloud/PyEmbeddedAdapter.cpp`, `src/src.pro`, `src/CMakeLists.txt`, `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp` (its `initTestCase()` fail-safe assertions and Py_Initialize() call need to keep passing against whatever the new bootstrap's readiness check becomes)
- Origin: Stage 9's first real qmake-binary connect attempt (`GC_WANT_GARMINCONNECT=ON`, `GC_WANT_PYTHON` OFF) failed with the generic "Connection to Garmin Connect failed (code: unknown)" on every attempt, root-caused to `Py_Initialize()` never being called anywhere reachable in that configuration (see B-STAGE9-01 in findings.md). Reviewer (`garmin_codex_reviewer`) independently confirmed via source, wiring, and the built binary's own symbol table (`nm -D`/`readelf -d` show `libpython3.13` linked but no interpreter-init call site outside the `GC_WANT_PYTHON`-gated `PythonEmbed.cpp:248`), then researched and ranked fix options.

### The problem
`Cloud/PyEmbeddedAdapter.cpp` (DES-013) is deliberately Python-init-agnostic — every op checks `Py_IsInitialized()` and fails safe to `Unknown` if false, by design (a documented, tested fail-safe, not itself a bug). `src/Python/PythonEmbed.cpp:248`'s `Py_InitializeEx(0)` is the ONLY interpreter-init call in the whole tree, gated behind the separate `GC_WANT_PYTHON` flag. `src/src.pro`'s own `GC_WANT_GARMINCONNECT` block and `src/gcconfig.pri.in` both explicitly claim the two features are "independent... either can be enabled without the other" — a claim the runtime does not honor: with `GC_WANT_PYTHON` off, nothing ever brings the interpreter up, so `Py_IsInitialized()` is permanently false and the Garmin feature is completely non-functional in that configuration, regardless of credentials/network/installed packages. Invisible to all prior test evidence because `testGarminConnectPyAdapter.cpp`'s own `initTestCase()` calls `Py_Initialize()` itself to test the fail-safe path, bootstrapping a way the real app never does (REQ-002 is marked "TEST VERIFIED (seam)" in traceability.md for exactly this reason).

### Resolution
Introduce one shared, process-level CPython bootstrap function (new, small, Python-header-free public interface matching `PyEmbeddedAdapter.h`'s existing no-`Python.h`-in-headers invariant) compiled whenever `GC_WANT_PYTHON` OR `GC_WANT_GARMINCONNECT` is ON, called once from `main.cpp` on the main thread before any Garmin worker can start, using `Py_InitializeFromConfig()` with a checked `PyStatus` (not the bare `Py_InitializeEx(0)` PythonEmbed uses today) so a startup failure can be reported instead of the process aborting, with signal-handler installation disabled and the user site-directory (`~/.local/lib/python3.13/site-packages`, where this session's `garminconnect`/`curl_cffi` packages now live) explicitly preserved. `PythonEmbed`'s own `PyImport_AppendInittab("goldencheetah", ...)` registration must still run before this shared bootstrap's first `Py_Initialize` call (CPython's own inittab contract), so `PythonEmbed` keeps owning its scripting-specific setup and attaches to the already-running interpreter under the GIL rather than re-initializing it. This is the builder's implementation task, not decided in full here — the builder works out the exact call sequencing/ordering guard, covers the reviewer's flagged risks (concurrent first-use races, `PyEval_SaveThread()` only on the initializing path, worker-teardown `QThread::terminate()` interaction, restart-loop re-construction of `PythonEmbed`), and a builder-owned test plan covers: Garmin-ON/Python-OFF, both-ON with scripting active and disabled, concurrent first use, token restoration as the first op, and worker teardown/recreation — in addition to preserving the existing seam tests' fail-safe assertions.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **1 — shared process-level bootstrap, main-thread, feature-agnostic — CHOSEN** | 5 — single, well-defined interpreter lifecycle owner; preserves DES-013's existing "uninitialized → safe failure" contract unchanged (the guard just becomes reachable-true once Garmin alone is enabled) | 4 — adds interpreter startup cost whenever Garmin is enabled even if Python scripting is never used, but that cost is fixed and one-time | 5 — one place owns init/config/lifetime instead of two features silently relying on each other; matches CPython's own recommended `Py_InitializeFromConfig` pattern (reviewer-cited) | 5 — directly satisfies the codebase's own stated "independent" design intent for the first time; does not reopen or weaken any existing DES-013 test |
| 2 — lazy self-init inside `PyEmbeddedAdapter` | 2 — an adapter-local `once_flag` cannot coordinate with `PythonEmbed`'s own independent init path if both features are enabled — the exact double-init/GIL-race class the reviewer flagged as a real hazard, not a hypothetical | 3 — defers cost to first use, but at the price of two independent init owners racing | 2 — changes DES-013's own documented uninitialized-call fail-safe semantics, forcing a design-doc and test-contract rewrite for a smaller feature than the shared owner would need | 2 — the reviewer explicitly declined to recommend this without demonstrated scripting-first/Garmin-first race handling neither this option nor this session has built |
| 3 — require `GC_WANT_PYTHON` ON whenever `GC_WANT_GARMINCONNECT` is ON (build-time coupling) | 3 — works, but only in the specific case both flags are compiled in; the reviewer independently found even that doesn't guarantee init (`GC_EMBED_PYTHON=false`, `--no-python`, or failed interpreter discovery still skip it at runtime) | 2 — every future Garmin-only deployment now silently drags in the whole data-science Python stack | 1 — directly contradicts the codebase's own explicit "either feature can be enabled without the other" design comment (`src.pro`, `gcconfig.pri.in`) — a real regression of stated intent, not a neutral simplification | 1 — a diagnostic workaround the reviewer explicitly named as unsatisfactory, not a real fix |

### Cascade impact
- 1 (chosen): `findings.md`'s B-STAGE9-01 row updated to cite this DEC once the builder starts; `traceability.md`'s REQ-002 row will need its "(seam)" qualifier revisited once Stage 9's real-account evidence actually lands (separate, later ledger update — not part of this DEC). `design.md` gets a new DES entry (or a DES-013 addendum) once the builder's exact shape is implemented — deferred to the builder + orchestrator doc pass, not decided here.
- 2: would need DES-013 rewritten and its existing fail-safe tests renegotiated before any Garmin-only code change — a materially bigger, riskier unit than option 1 for a worse reliability outcome.
- 3: would need `GC_WANT_GARMINCONNECT`'s own `contains(DEFINES, ...)` build blocks rewritten to force-enable `GC_WANT_PYTHON`, contradicting this project's own stated design intent with no runtime guarantee even then.

### Chosen
1 — the only option that actually delivers on the codebase's own pre-existing "independent" design claim without reopening DES-013's tested fail-safe contract or accepting an unresolved double-init race.

### Alignment probe
grep -c "Py_Initialize" src/Python/PythonEmbed.cpp src/Cloud/PyEmbeddedAdapter.cpp <new-bootstrap-file>   # expect: bootstrap file owns the one Py_InitializeFromConfig call; PythonEmbed attaches, does not re-init
grep -n "GC_WANT_PYTHON\|GC_WANT_GARMINCONNECT" src/src.pro src/CMakeLists.txt   # expect: the new bootstrap TU compiled under an OR of both flags, not either alone
ctest -L garmin-fast   # expect: existing DES-013 fail-safe assertions in testGarminConnectPyAdapter.cpp still pass unchanged

## DEC-053 — B-STAGE9-15 remedy: developer-trace literals are exempt from the i18n guard as a CATEGORY; the three flagged call sites are correct as written

- Status: accepted (Inspector, Three-Options Doctrine — ordinary tooling-correctness decision, not a human-in-the-loop gate; the standing-rule exception was independently verified by a fresh unbriefed agent before recording, per this project's own rule)
- Reversibility: high — changes one predicate in a static guard script plus its tests; no production behavior, no shipped binary, no user-visible surface
- Decided / last-reviewed: 2026-09-15
- Serves: B-STAGE9-15 (a RED `testGarminI18nSourceGuard` in HEAD since `c1948b513`, blocking a truthful Stage 9 gate)
- Dependents: `unittests/buildguard/garmin_i18n_source_guard.py` (T-208), `src/Cloud/GarminCredentialsPage.cpp`, `src/Cloud/GarminMfaPage.cpp`, REQ-NF-i18n-001's traceability row
- Origin: found 2026-09-15 by `garmin_inspector_v1_21` while running the static guards after a clang-format reformat during B-STAGE9-13's commit. The guard reports 3 `I18N-TR-WRAP` findings, all on the literal `garmin_auth_unknown exception_type=%1`, emitted at all three sites via `qDebug().noquote()`.

### The problem
Two separate defects are tangled, and they must not be conflated.

(1) A GATE-COVERAGE defect. `testGarminI18nSourceGuard` carries the ctest label `garmin-i18n-guard`, which is NOT part of `garmin-fast` — the label this project's standing verification ritual runs after every unit, including the Inspector's own independent re-runs. A red test therefore sat in HEAD for two days unseen. The pre-commit hook does not run `ctest` either (clang-format/ruff/mypy/ledger-drift-lint only), so nothing caught it. This is the same blind-spot class as B-STAGE9-14 — a defect outside the habitual window is invisible to it no matter how many passes run — but demonstrated at the GATE layer rather than the review layer, which makes it the more serious of the two. Every "garmin-fast 40/40" claim recorded in this ledger since 2026-09-13 is narrower than it reads.

(2) A GUARD-CORRECTNESS question, which this DEC resolves. `garmin_auth_unknown exception_type=%1` is a developer diagnostic trace, not user-facing prose: at all three sites it goes only to `qDebug().noquote()`, while the UI separately receives `GarminErrors::translate(error.kind)`. Translating it into 13 languages would treat developer telemetry as interface text and permanently mistranslate diagnostic output — a worse outcome than the red test.

The guard's exemption logic was read directly rather than inferred (`is_technical()`, `garmin_i18n_source_guard.py:60-71`). A literal is exempt if it matches a date-format pattern, OR every whitespace-separated token contains `=`, OR it has `>= 2` `%` placeholders AND `>= 2` `=` signs. `gcObsTrace()`'s `"gc_obs op=%1 outcome=%2 error_code=%3 duration_ms=%4"` passes the third clause (4 and 4). The flagged literal fails all three: it has one `=` and one `%`, and its leading token `garmin_auth_unknown` is a bare event name carrying no `=`. So the guard is not drawing a principled user-facing-versus-developer-trace distinction at all — it is keying on incidental syntax, and the two structurally identical trace families land on opposite sides of it by accident.

### Resolution
Extend `is_technical()` so developer-trace literals are exempt as a CATEGORY rather than by placeholder arithmetic: a literal qualifies when it consists of `key=value` tokens optionally preceded by a SINGLE leading bare identifier-shaped event-name token (`snake_case`, no spaces), and contains at least one `key=value` token. This admits both existing trace families and the flagged one, while staying tight enough that real prose — which has multiple bare words and no `=` — is still caught. The three call sites are left unchanged. The repair is NOT complete without a RED-first test proving the new predicate still REJECTS a prose literal that merely contains an `=` sign; widening this predicate is exactly the kind of change that can silently turn the whole guard vacuous, which is this project's signature failure mode (B-STAGE9-13, B-STAGE9-14). Defect (1), the gate-coverage gap, is NOT fixed by this DEC and remains open in B-STAGE9-15 — the label-coverage question (whether `garmin-fast` should subsume the guard labels, or the ritual should change) is a separate unit.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **1 — exempt developer-trace literals as a category; leave the call sites alone — CHOSEN** | 5 — fixes the real defect (the heuristic), and keeps diagnostic output untranslated where every consumer of it expects stable ASCII keys | 5 — any future `qDebug` trace line is handled by the rule instead of needing a new special case or a fresh red gate | 5 — one predicate, in one place, expressing the distinction the guard was always trying to draw | 5 — log/telemetry text is conventionally never localized; `%1`-count arithmetic is not a category test |
| 2 — wrap the three call sites in `tr()` and add the literal to all 13 `.ts` files | 2 — makes a machine-parsed diagnostic key locale-dependent; a translator could reword `exception_type` and silently break any log reading | 1 — every future trace line needs 13 catalog entries and the same wrong treatment, compounding | 2 — leaves the misfiring heuristic in place, so the next structurally identical literal fails again for the same wrong reason | 1 — translating developer telemetry is a straightforward anti-pattern |
| 3 — exclude the two page files from the guard's file globs | 1 — silences the symptom and blinds the guard to GENUINELY user-facing prose in two real UI files, which is most of what those files contain | 2 — every future file with one trace literal gets dropped wholesale | 1 — turns a precise rule into a file-level opt-out, the vacuous-guard failure mode | 1 — suppressing a check rather than correcting it |

### Cascade impact
- 1 (chosen): `unittests/buildguard/garmin_i18n_source_guard.py` predicate + its own tests; `findings.md`'s B-STAGE9-15 row cites this DEC; REQ-NF-i18n-001's traceability row needs a note that the guard's technical-exemption category was widened after T-208 was closed, so "TEST VERIFIED" for that REQ now rests on a revised predicate. The gate-coverage half of B-STAGE9-15 stays open and unaddressed by this DEC.
- 2: would need 13 `.ts` files edited and would make defect (1) permanent by declaring the misfire correct.
- 3: would need the guard's `CPP_GLOBS` narrowed, silently dropping real UI prose coverage for two credential-facing dialogs.

### Chosen
1 — the only option that fixes the actual defect (a syntactic heuristic masquerading as a semantic category) rather than ratifying it or hiding from it. Independently verified before recording, per this project's rule that a DEC bending a standing rule needs a fresh unbriefed second opinion: a separate, read-only Codex agent (`s915_i18n_second_opinion`, given only the narrow question and explicitly told the brief might be wrong) read the guard's rule implementation and the three call sites and returned the same three answers — developer trace, not prose; the `>=2 %` / `>=2 =` clause is why `gcObsTrace` escapes and this does not; remedy (b), fix the guard. The Inspector then re-read `is_technical()` directly to confirm that account rather than relaying it.

### Alignment probe
python3 unittests/buildguard/garmin_i18n_source_guard.py .   # expect: 0 findings, and NOT because the predicate went vacuous
grep -n "garmin_auth_unknown" src/Cloud/GarminCredentialsPage.cpp src/Cloud/GarminMfaPage.cpp   # expect: still bare QStringLiteral, no tr()
grep -c "garmin_auth_unknown" src/Resources/translations/gc_de.ts   # expect: 0 — never enters a catalog

---

## DEC-054 — B-STAGE9-16 remedy: the default verification scope becomes fail-safe (default-include ctest gate with an explicit opt-out label; lint scope widened to every new Garmin-owned path; two coverage guards)
<!-- gc-arms/v1 {"dec": "DEC-054", "patterns": ["unittests/Core/stderrbuf/*"]} -->
- Status: accepted (Inspector, Three-Options Doctrine — ordinary tooling/process decision, not a human-in-the-loop gate; three materially different options were researched by a fresh unbriefed read-only agent, and the one fact capable of inverting the ranking was re-verified by the Inspector directly before recording)
- Reversibility: high — the gate is a command string plus two CMake `LABELS` entries plus one YAML file; no production code, no shipped binary, no user-visible surface. The two new guards are additive static checks.
- Decided / last-reviewed: 2026-09-15
- Serves: B-STAGE9-16 (the gate-coverage half split out of B-STAGE9-15; the reason a RED `testGarminI18nSourceGuard` survived in HEAD from `c1948b513` 2026-09-13 to 2026-09-15 unseen)
- Dependents: `unittests/buildguard/CMakeLists.txt` (the two flag-build guards' labels); `.pre-commit-config.yaml` (DEC-009's tool set, DEC-010's scoping); every future "gate GREEN" evidence line in this ledger; DEC-010 (whose recorded cascade prose is corrected below); B-STAGE9-12 (whose deliberately-RED stderr-buffering test becomes visible to the routine gate the moment it is registered — see Cascade impact)
- Origin: found 2026-09-15 by `garmin_inspector_v1_22` during B-STAGE9-15, then widened the same day by a second instance of the identical shape found while gating its own commit `d9ba4faad`.

### The problem
This project's verification ritual is OPT-IN at two independent layers, and both have now been caught failing in the same way: a thing that is not deliberately enrolled is invisible, and its absence is silent.

(1) The ctest layer. The habitual gate is `ctest -L garmin-fast`. A test is in that gate only if someone remembered to attach that label. `testGarminI18nSourceGuard` carries `garmin-i18n-guard`, so the habitual gate could never see it, and it sat RED for two days across multiple supervised agents and two Inspectors' own independent re-runs. The consequence is worth stating plainly rather than softening: every `garmin-fast 40/40` claim recorded in this ledger is narrower than it reads — 40 of 53 registered tests, not all of them.

(2) The pre-commit lint layer. `.pre-commit-config.yaml` scopes each hook with an enumerated `files:` regex naming three directories that existed when DEC-010 was written. `ruff`, `ruff-format` and `mypy` match only `^src/Python/garminconnect/.*\.py$`, and `clang-format` only `^(src/Cloud/Garmin[^/]*\.(cpp|h)|unittests/Core/garminconnect/.*\.(cpp|h))$`. So `unittests/buildguard/*.py` — Python under active edit in this very unit — is linted by nothing, and `unittests/Core/stderrbuf/*.cpp` would likewise be unlinted. Confirmed empirically, not inferred: commit `d9ba4faad` staged three `unittests/buildguard/` paths and EVERY hook reported "no files to check" and skipped.

The common defect is not a wrong label or a wrong regex. It is that in both layers ENROLMENT IS THE VERIFICATION STEP, so forgetting to enrol produces a green result rather than a loud one. Folding `garmin-i18n-guard` into `garmin-fast` and adding one directory to one regex would fix these two instances and leave the NEXT new label and the NEXT new directory invisible in exactly the same way.

### Measured facts (re-verified by the Inspector against the live `build/` tree, not accepted from the researcher's report)
- `ctest -N` lists **53** registered tests. Label census: `garmin-fast` 40, `garmin-i18n-guard` 2, `garmin-sec-guard` 2, `garmin-build-guard` 2, `garmin-py` 5, `garmin-stderr-buf` 1, and `testCoachTools` carrying no label at all — 53 exactly, disjoint.
- **The fact that decided the ranking, and the reason the naive "just use plain `ctest`" answer is wrong:** a plain unfiltered `ctest` DOES select both flag-build guards. `unittests/buildguard/CMakeLists.txt:30-31` registers them with `LABELS "garmin-build-guard"` and `TIMEOUT 5400`, and each performs a full from-scratch CMake configure + app build in a scratch directory. `build/Testing/Temporary/CTestCostData.txt` records them at **247.048 s** and **256.081 s** — about **503 s** added to every routine run in the good case, and up to 90 minutes each on a hang. A gate that expensive gets bypassed, which reintroduces the opt-in defect operationally.
- `ctest -N -LE garmin-build-guard` selects **51** tests. Its known cost floor is the 308 s `garmin-fast` suite plus 1.4 s i18n plus 0.3 s security; the coach test, the stderr-buffering test and the five Python tests are not currently measured (historical cost data records the Python five at ~0.24 s total and `testCoachTools` at ~0.001 s, which is indicative only). Actual measurement of the new gate command is an explicit build obligation of B-STAGE9-16, not an assumption of this DEC. Timeout values are ceilings, never evidence of runtime.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| **1 — default-include gate with explicit opt-out: routine gate becomes `ctest -LE gate-exclude`, the two flag-build guards carry `gate-exclude` plus a documented reason and an alternate tier; lint regexes widened to all new Garmin-owned paths; two coverage guards added — CHOSEN** | 5 — a newly registered test is in the gate without anyone remembering anything; the failure mode inverts from silent-omission to loud-inclusion | 5 — the exclusion list is short, reviewable, and grows only on deliberate act; new suites cost nothing to enrol | 4 — one command string, two `LABELS` entries, one YAML file, two guards; the opt-out list is the one place that can still rot, which is exactly what the coverage guard watches | 5 — fail-safe defaults are the standard for verification scope; a label keeps excluded tests discoverable and runnable, unlike `DISABLED` |
| 2 — curated umbrella label `garmin-gate`, enforced by a registry guard that fails when an in-scope test lacks it | 4 — the guard does catch a forgotten label, unlike today's ritual | 3 — retains a hand-curated inclusion list and a second mapping between test ownership and gate membership; both grow with every suite | 4 — familiar mechanism, no command change | 4 — safe only because a guard patches the unsafe default; encodes the weaker invariant |
| 3 — exhaustive plain `ctest` as the required gate, slow guards included, pre-commit stays lint-only | 5 — strongest possible signal; nothing can hide | 5 — nothing to maintain; scope is "everything" | 3 — ~13.5 min serial in the good case from the measured 503 s of scratch builds alone, up to 90 min per guard on a hang | 4 — correct as a release/periodic tier, wrong as a habitual local gate: cost that high predictably invites bypass, recreating the defect |

### Resolution
Adopt option 1, with option 3 retained as the required slower integration/release tier (`ctest -L garmin-build-guard`, run on a schedule and before release, not per unit).

Concretely:
- The routine gate command becomes `ctest -LE gate-exclude`. Bare `ctest` remains the exhaustive audit and is not weakened.
- The two flag-build guards get `LABELS "garmin-build-guard;gate-exclude"`, each with an adjacent comment giving the exclusion REASON and the TIER that does run it. An opt-out without a recorded reason is itself a defect.
- The 11 tests that newly enter the gate (2 i18n-guard, 2 sec-guard, 5 `garmin-py`, `testCoachTools`, and the stderr-buffering test once registered) must each be PROVEN green — or deliberately excluded with a reason — before the new gate is declared the gate. Adopting a gate that is red on arrival would be the same class of defect in a new costume.
- The lint half gets a DIFFERENT treatment from the ctest half, and deliberately so. It is NOT widened to the whole tree: DEC-010 scoped pre-commit to new Garmin paths precisely so as not to retrofit the legacy GoldenCheetah tree, and that reasoning is still correct. What is widened is the ENUMERATION — `files:` regexes must cover every new Garmin-owned path, including `unittests/buildguard/` (ruff, ruff-format; mypy only if a dedicated config for pytest-style guard tests is demonstrated clean, and if not then Ruff coverage is still required and no type coverage is claimed) and `unittests/Core/stderrbuf/` (clang-format). This is in the SPIRIT of DEC-010, not against it: DEC-010's defect is an enumeration that was accurate on 2026-05-17 and never grew, not the scoping principle itself.
- Detection, as a complement and never as the main protection: two cheap static checks join the existing `unittests/buildguard/` family — (a) a GATE-COVERAGE guard asserting that every registered ctest test is selected by `ctest -LE gate-exclude` unless it carries `gate-exclude`, and that every excluded test carries a reason and a named alternate tier; (b) a LINT-OWNERSHIP guard asserting that every file under the declared managed roots matches at least one hook's `files:` regex. Neither guard may itself carry `gate-exclude` — a coverage guard outside the gate is the original bug wearing a new hat.
- `ctest` stays OUT of pre-commit. It needs a configured, built tree and the routine suite is minutes-scale, while pre-commit is staged-file, seconds-scale hygiene. The gate belongs in the documented acceptance command and in required merge validation.

### Cascade impact
- 1 (chosen): `unittests/buildguard/CMakeLists.txt` labels; `.pre-commit-config.yaml` regexes; two new guards plus their own RED-first tests; the acceptance/evidence wording in this project's own governance so that future evidence lines cite the new command; `findings.md`'s B-STAGE9-16 row cites this DEC. **B-STAGE9-12 inherits an obligation:** its stderr-buffering test currently fails by design pending a production fix that is frozen under the `src/Core/main.cpp` hold, and it is registered under `garmin-stderr-buf`, which the new gate WILL select. It is untracked and absent from HEAD today (`git ls-files unittests/Core/stderrbuf/` is empty), so HEAD's gate is not red — but B-STAGE9-12's repair round must either land the fix or register the test with `gate-exclude` and a reason naming the hold. It may not land a RED test into the default gate.
- **Correction owed to DEC-010, recorded here rather than by rewriting its history:** DEC-010's cascade prose states that the hook runs a "fast unit-test subset (CTest label `garmin-fast` + the Python tests)". The live `.pre-commit-config.yaml` contains no ctest hook at all — only clang-format, ruff, ruff-format, mypy and ledger-drift-lint. The record and the config have disagreed for some time; this DEC ratifies the config (no ctest in pre-commit) as the intended behavior and supersedes that clause of DEC-010's cascade.
- Also unbuilt and worth naming rather than assuming: there is no checked-in CI workflow in this repository to host a required gate. "Required merge validation" is therefore an aspiration until such a job exists; today the gate's enforcement point is the documented acceptance command plus the two coverage guards. The qmake path remains the release/packaging authority and CMake/CTest remains the developer verification layer — this DEC does not change that division.
- 2: would need every in-scope test labeled plus the registry guard, and would leave a curated inclusion list as the thing that rots.
- 3: would need the 503 s of scratch builds accepted into every routine run, or a second "quick" command that immediately becomes the real habitual gate and reintroduces the defect.

### Chosen
1 — the only option that inverts the defective invariant rather than patching its two known instances. Option 2 is a respectable lower-disruption alternative and its guard would catch a forgotten label, but it still encodes "included because someone enrolled it"; option 1 encodes "included unless someone deliberately, visibly excused it". Option 3 is kept, not discarded — as the release tier it is strictly better than option 1, and as a habitual gate it is strictly worse, and those are not in conflict. The researcher's report was a fresh, unbriefed, read-only agent explicitly told the brief might be wrong; the Inspector then re-ran `ctest -N`, the per-label census, `ctest -N -LE garmin-build-guard`, the `TIMEOUT 5400` registration and the `CTestCostData.txt` figures directly, and read `.pre-commit-config.yaml` and DEC-010 in full, rather than relaying any of it.

### Alignment probe
cd build && ctest -N -LE gate-exclude | tail -1     # expect: every registered test minus the excused ones (53 as built 2026-09-15, 51 before this unit's own two guards were registered) — the POINT is that the count MOVES when a test is added, so a fixed number here would itself rot
grep -n "gate-exclude" unittests/buildguard/CMakeLists.txt   # expect: the two flag-build guards, each with GATE_EXCLUDE_REASON + GATE_EXCLUDE_TIER properties AND an adjacent prose comment
grep -n "unittests/buildguard" .pre-commit-config.yaml       # expect: at least the ruff and ruff-format file regexes (Part B — not yet true)
python3 unittests/buildguard/garmin_gate_coverage_guard.py build   # expect: 0 findings, and NOT because the check went vacuous. NOTE the argument is the BUILD directory, not the source root: the ctest registry lives in the build tree, and the ctest registration passes ${CMAKE_BINARY_DIR}. This probe originally said `.` — wrong, and corrected 2026-09-15 after `garmin_builder_stage9_v7` ran it as written, got exit 2 ("not a configured CMake build tree"), and declined to degrade a correct interface to match a wrong probe line. Recorded rather than silently amended because an alignment probe that does not run is the same disease this DEC is about.

## DEC-055 — B-STAGE9-26 remedy: one typed contract for every key the summary shape requires

- Status: accepted (Inspector, Three-Options Doctrine — ordinary implementation-contract decision inside the current REQ's accepted scope, not a human-in-the-loop gate)
- Reversibility: high — one guard in one loop body in one Python module, plus its unit coverage. No C++ change, no shipped-binary surface, no user-visible string.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-26 (split out of B-STAGE9-25 round 4 as the one non-blocking residual)
- Dependents: `src/Python/garminconnect/garmin_client.py` `_list_activities_since_impl`; the fakes/pystub listing double, which must reject what the real path rejects; `classifyListException` (`src/Cloud/PyEmbeddedAdapter.cpp:485-497`), whose still-open wiring gap is what currently hides the difference.
- Origin: raised by `garmin_codex_reviewer` on B-STAGE9-25 round 4, filed as its own row by `garmin_inspector_v1_30`, decided by `garmin_inspector_v1_32` 2026-09-19.

### The problem
`garmin_client.py:421` builds each summary as `{"activityId": str(a["activityId"]), "startTimeGMT": ts_value}`. Its sibling key one line above goes through `_as_utc_instant(..., "response_invalid")` and raises a typed `GarminError` on a bad shape. `activityId` does not: a record carrying a valid `startTimeGMT` but no `activityId` raises a bare `KeyError`, which is not a `GarminError`, so `with_retry` does not treat it as transient and `classifyListException` maps it to `PyListOutcome::Unknown` — the exact unclassified fold B-STAGE9-25 exists to abolish, reintroduced one line away from its own fix.

The code comment at `:414-416` asserts this is deliberate ("a library contract breach whose KeyError propagates unclassified"). That assertion is the thing being decided here, not evidence for it — this project does not accept a comment stating a checkable claim as a reason, whoever wrote it.

### Options scored (reliability / scalability / maintainability / best practices)
- **Option A — extend the typed contract to every required key (CHOSEN): 5/4/5/5.** Validate `activityId` the same way its sibling is validated and raise `GarminError("response_invalid", ...)` naming the missing key. One boundary, one contract, one classification path; a future required key joins it by construction rather than by memory.
- **Option B — record the bare `KeyError` as a deliberate unrecoverable breach: 2/3/3/2.** Cheapest, and it is what the comment already claims. But it leaves two keys of the same shape treated differently for no reason a caller can act on, and it keeps a path into the `Unknown` fold alive. It becomes actively wrong the moment the `classifyListException` wiring gap closes and the two kinds start reaching an operator differently — which is a planned change, not a hypothetical.
- **Option C — wrap the loop in `except KeyError` → `response_invalid`: 4/3/3/3.** Classifies correctly but discards which key was missing, and silently absorbs a future required key into the same bucket. A broad catch that hides its own cause is the shape this project has repeatedly been bitten by.

### Cascade impact
`traceability.md` REQ-002's listing row gains this contract. The fakes/pystub listing double must reject a record missing `activityId`, or the suite stays green while the real contract differs — the fifth-instance failure class B-STAGE9-25 already recorded once. No `STATE.md` stage boundary moves; B-STAGE9-26 is non-blocking and does not gate the attended live run.

## DEC-056 — B-STAGE9-29 remedy: name Garmin entries from the activity's own local start time

- Status: accepted (Inspector, Three-Options Doctrine — ordinary implementation decision inside the current REQs' accepted scope, not a human-in-the-loop gate)
- Reversibility: high — one marshalled key in one Python module, one field on one struct, one `QString` construction. No schema, no stored state, no user-visible string; the download and dedup keys do not move.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-29 (blocking — the sixth attended live run listed six activities and the dialog selected 0 of 0)
- Dependents: `src/Cloud/GarminConnect.cpp:869-878` (entry construction); `src/Python/garminconnect/garmin_client.py:407-425` (summary marshalling) and its pystub double; `GarminActivitySummary`. NOT dependents, verified this pass: `readFile` keys by `remoteid` (`GarminConnect.cpp:649-681`) and `recordImport` keys the sidecar by the same id (`:677-680`, `:898-910`), so renaming moves no download or dedup key.
- Origin: found by `garmin_inspector_v1_32` on the sixth attended live run; root cause falsification-tested by `s925_tz_investigator` (unit `B-STAGE9-29-confirm`) and confirmed on all counts; decided by `garmin_inspector_v1_33` 2026-09-19.

### The problem
`GarminConnect.cpp:872` names every entry `garmin-<id>.fit`. Both production consumers of that list gate on `RideFile::parseRideFileName`, which exact-matches a leading `yyyy_MM_dd_HH_mm_ss` (`src/FileIO/RideFile.cpp:2434-2451`) and `continue`s past anything else — the sync dialog's Download loop at `CloudService.cpp:2160-2172` (and therefore its Synchronize tab too, whose rows are built only inside that loop's post-parse branch at `:2224-2268`) and the auto-downloader at `CloudService.cpp:4254-4279`. `garmin-` cannot match. The service layer is correct and 100% of it is discarded, silently: six successful listings, `0 of 0 selected`, `activities/` unchanged at 1145. The dialog also never reads `e->modified`, which we do populate — it derives every date from the name it just parsed.

Every peer service names from LOCAL start time (Strava: `start_date_local` → `yyyy_MM_dd_HH_mm_ss` + suffix, `src/Cloud/Strava.cpp:258`). We marshal only `startTimeGMT`, and `parseGarminTime` stamps it `Qt::UTC` (`GarminConnect.cpp:119-127`).

### Options scored (reliability / scalability / maintainability / best practices)
- **Option A — marshal `startTimeLocal` and name from it (CHOSEN): 5/4/4/5.** The activity's own local start time is what every peer uses and what the athlete directory is already written in, so sorting, the dialog's date-range filter and its `Exists` check all agree with files GoldenCheetah wrote itself. The installed wheel declares the key at `garminconnect/typed.py:396-407` as `str | None` — optional, so absence must fall back, never fail the listing (failing it would reproduce the very blank dialog being fixed). Cost: one more marshalled key, one struct field, one pystub update.
- **Option B — convert the existing `startTimeGMT` to local in C++: 2/4/5/2.** Cheapest and touches no Python. But it names by the *machine's current* timezone rather than the activity's, so an activity recorded in another timezone gets a name off by the offset — it mis-sorts, falls outside the date-range filter at the boundary, and silently misses the `Exists` dedup check against the local-time file GoldenCheetah itself wrote. A wrong answer that is data-dependent and invisible is worse than the blank dialog it replaces.
- **Option C — relax or bypass the filename gate for Garmin: 3/2/2/2.** Requires editing two enumeration loops in `CloudService.cpp` that every provider runs, plus the auto-downloader, for one provider's benefit — the largest blast radius on the most shared code in the seam. It also leaves the staging name still non-conforming for everything downstream that parses names.

### Cascade impact
`traceability.md` REQ-002/REQ-012's listing rows gain the naming contract. The pystub listing double must carry `startTimeLocal` or the C++ suite proves nothing about the new field. `design.md`'s listing prose names `garmin-<id>.fit` as the entry name and must be corrected. No stage boundary moves, but this unblocks the seventh attended live run, which is Stage 9's acceptance criterion.

## DEC-057 — B-STAGE9-28 remedy: an empty managed root must be DECLARED empty, not silently passed

- Status: accepted (Inspector, Three-Options Doctrine — ordinary tooling decision on a project-owned guard, not a human-in-the-loop gate)
- Reversibility: high — one declaration field on a managed-root record, one branch in the empty-root report path, one amended unit test. No product code.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-28 (non-blocking — the lint-ownership guard passed 101/111 while proving nothing about a brand-new directory)
- Dependents: `unittests/buildguard/garmin_lint_ownership_guard.py` (`tracked_files` :703-723, `check_ownership` :789-819, `format_root_report` :1108-1119, the LINT-VACUOUS aggregate :1084-1102); `unittests/buildguard/test_garmin_lint_ownership_guard.py:702-711`. NOT dependents, verified: the other four guards in that directory enumerate by filesystem glob or the CTest registry, not the git index — `garmin_sec_source_guard.py:256-273`, `garmin_i18n_source_guard.py:280-292`, `garmin_gate_coverage_guard.py:735-752`, `garmin_flag_build_guard.sh:68-89`. This is one guard's rule, not a family bug.
- Origin: found by `garmin_inspector_v1_31`, measured on both sides of `git add` by `garmin_inspector_v1_32`, option space researched by `s925_tz_investigator` (unit `B-STAGE9-28-options`) and the finding's own option A falsified by `garmin_inspector_v1_34` 2026-09-19.

### The problem, restated — B-STAGE9-28's own option A is VOID

The finding proposed "teach the guard to consider untracked-but-staged files (`git status --porcelain` rather than `git ls-files`)". **It already does.** `git ls-files` reads the INDEX, so a `git add`ed never-committed file is in scope immediately, and `test_tracked_files_sees_a_newly_staged_never_committed_file` (`test_garmin_lint_ownership_guard.py:612-635`) pins exactly that, in those words, against a fixture with no HEAD at all. Option A is a no-op on an already-correct mechanism.

The investigator also killed the literal swap independently: on a clean tree `git diff --cached --name-only` and `git status --porcelain` yield no paths at all, and the guard's CTest invocation (`unittests/buildguard/CMakeLists.txt:248-257`) runs against the full source root with no staging set — so a status/diff-only inventory would trip LINT-VACUOUS (`:1084-1102`) rather than pass. It trades one blind window for a larger one.

So the blind window is NARROWER than the finding states, and it is not an enumeration bug. It is only this: between the files existing on disk and `git add`, a managed root reports `EMPTY — 0 tracked files. Regexes may be armed for it, but nothing is proven here.` — an honest line — and the run still PASSes. A gate whose PASS proves nothing is [[LSN-083]]'s shape.

### Options scored (reliability / scalability / maintainability / best practices)
- **Option A — an empty root must be DECLARED pre-armed, else RED (CHOSEN): 5/4/4/5.** The guard's stated philosophy is already "declared rather than hidden" (`:76`), and it already carries a declared-gap mechanism for known-uncovered things. This extends it one level: a managed root may report EMPTY only if its record declares it pre-armed and names the DEC that armed the regexes; an undeclared empty root is a finding. `unittests/Core/stderrbuf/` is the legitimate case and is declared, not exempted by accident. Cost: one field, one branch, and amending `test_an_empty_root_does_not_crash_and_does_not_go_red:702-711`, which currently pins the silent-green contract.
- **Option B — keep EMPTY green; make staging-time re-verification an Inspector obligation: 3/2/3/3.** This is already the de facto practice and it is what caught B-STAGE9-12 — but only because a supervisor remembered. It does not survive the supervisor, which is precisely the failure mode, and it does not scale past one attentive reader.
- **Option C — no change; document that the EMPTY line is the signal: 1/3/5/1.** Free, and buys nothing. The line was already printed and already read past once, under a guard built to prevent this defect class.

### Cascade impact
No product code, no stage boundary, no acceptance criterion moves. `findings.md`'s B-STAGE9-28 row must record that its own option A was falsified, so a later reader does not re-propose it. Sequencing: this is NON-BLOCKING and queues behind the B-STAGE9-26+29 slice and the seventh attended live run — it must not take a builder turn away from Stage 9's acceptance criterion.

## DEC-065 — Stage 9's installer evidence is earned on AppVeyor, not on a local build

- Status: **ACCEPTED 2026-09-20 — USER decision.** This is the one human-in-the-loop gate this session reached: both candidate routes required the user's own authority (sudo package installs on their machine, or a push to a shared remote), so neither was the Inspector's to take. Presented with three options and the measured evidence behind each; the user chose AppVeyor.
- The problem: B-STAGE9-48 established that no installer artifact has ever existed on this host and that live run 7 was a developer-tree binary, so Stage 9's acceptance criterion was never actually discharged. B-STAGE9-54 and B-STAGE9-57 then established that a local build is not reachable without sudo.
- Why a local build was rejected: the blocker is a linkable Python 3.11 SDK. The runtime AppImage the recipe downloads carries `include/python3.11/Python.h` but NO `python3.11-config` and NO `libpython3.11*`, and its interpreter declares no NEEDED libpython entry — it cannot serve the build `appveyor/linux/before_build.sh:35-36` demands. `patchelf` is also absent, and the native -dev packages at `appveyor/linux/install.sh:4-9,39,42` are not installed. A local build would also be Linux-only and, on gcc-14 against the recipe's gcc-11 portability pin, of unproven portability.
- What this route uniquely buys: the macOS and Windows legs. Both are unexecutable on this Linux-only host, so CI is the only path on which their recipes are ever exercised at all.
- Constraint 1: **the push is gated on a clean separation first.** This branch carries several unrelated uncommitted workstreams (Coach/Qt6.8, governance, the buildguard pair, the Garmin product work). Only the Garmin set goes; `git add -A` is forbidden and each file's own diff is read before staging.
- Constraint 2: the code queue is NOT blocked by this decision and did not wait on it — B-STAGE9-36, the B-STAGE9-38 u3 repair, DEC-063, B-STAGE9-42 and DEC-062 all continue independently and must land before the push is worth making.
- Constraint 3: B-STAGE9-42's pin-and-verify fix (`appveyor/linux/after_build.sh:5`, 3.11.16 + SHA-256, digest independently re-verified against the GitHub release API) MUST be in the pushed set. Without it the Linux leg fetches a 404 and the run proves nothing.
- Constraint 4: `APPIMAGE_EXTRACT_AND_RUN` is NOT needed for this route. It was the local-build workaround for an absent `/dev/fuse`; CI runners have their own. Do not carry it into the recipe.
- Reversibility: medium — a CI run is externally visible and consumes shared build minutes, but produces no release and no published artifact by itself.
- Serves: Stage 9's live-account/installer acceptance criterion; B-STAGE9-48, B-STAGE9-54, B-STAGE9-57.
- Dependents: `appveyor.yml`, `appveyor/linux/{install,before_build,after_build}.sh`, and every future "Stage 9 evidence" line in this ledger.
- Origin: `garmin_inspector_v1_46`, 2026-09-20, from `STAGE9-live-run7-provenance`, `STAGE9-installer-feasibility` and `STAGE9-installer-no-sudo-path`.

## DEC-058 — B-STAGE9-38 remedy: the Garmin adapter ships as an installable distribution, not as a compiled-in source path

- Status: accepted (Inspector `garmin_inspector_v1_36`, Three-Options Doctrine — ordinary packaging/tooling decision, not a human-in-the-loop gate)
- Reversibility: medium — a `[build-system]` block and an import-namespace rename in a project-owned Python directory, one requirements.txt entry, one runtime path branch, one CI smoke step. No product logic changes; the `GC_GARMIN_PYPATH` escape hatch is preserved throughout, so a bad step is always recoverable by an env var.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-38 (blocking — the Garmin feature is absent from the shipped package), REQ-NF-Pkg-001 (reopened), prd.md:115's Phase-1 checklist
- Dependents: `src/src.pro:274` (`GARMIN_PY_MODULE_DIR`), `src/Python/garminconnect/pyproject.toml`, `src/Python/requirements.txt`, `src/Cloud/PyEmbeddedAdapter.cpp:199,553-557`, `src/Cloud/AddCloudWizard.cpp:179-191`, `appveyor.yml:143,252`, `appveyor/linux/after_build.sh:18,50`, `appveyor/macos/install.sh:54`
- Origin: `s925_tz_investigator` units `REQ-NF-Pkg-001-linux-smoke` and `B-STAGE9-38-options`; every recipe claim re-read by the Inspector before this entry was written

### The problem, restated

Two independent mechanisms, either one fatal on a user machine. `src/src.pro` has ZERO `INSTALLS` entries and no recipe on any platform copies `src/Python/garminconnect/garmin_client.py` — this project's own top-level adapter, imported by bare name, distinct from the upstream `garminconnect` wheel that shares its directory name. Separately, `src.pro:274` compiles `GARMIN_PY_MODULE_DIR` as `$$PWD/Python/garminconnect`, an absolute path on the BUILD machine, which `appveyor/linux/after_build.sh:18`'s bare `cp` never rewrites. Shipping the file without removing the path fixes nothing, and removing the path without shipping the file fixes nothing — any accepted option must do both.

### Options scored (reliability / scalability / maintainability / best practices)
- **Option B — package the adapter as a real installable distribution under a NEW import namespace, carried by the existing `pip install -r requirements.txt` step (CHOSEN): 5/5/4/5.** All three platforms already install that file (`appveyor/linux/after_build.sh:50`, `appveyor/macos/install.sh:54`, `appveyor.yml:143`), so the carrier exists and is not three carriers. In package mode the import resolves from the bundled `site-packages` and `GARMIN_PY_MODULE_DIR` stops being consulted at all — the absolute path is removed by disuse rather than by rewriting, which is the only removal that cannot silently regress. Loses one point on maintainability: it is a genuine namespace change, not a no-op.
- **Option A — qmake `INSTALLS` staging plus an executable-relative resolver: 4/3/3/3.** `INSTALLS` alone removes nothing; it needs the resolver too, and no artifact job currently runs `make install` — Linux copies the binary straight into AppDir (`appveyor/linux/after_build.sh:18`). Adds install-prefix handling to all three packagers.
- **Option C — three per-recipe copy steps plus the same resolver: 3/2/2/2.** Three layouts (`appveyor/linux/after_build.sh`, `appveyor/macos/after_build.sh:161`, `src/Resources/win32/GC3.8-Master-W64-QT6.nsi:165`) and three smoke assertions that can drift independently. This is the shape REQ-NF-Pkg-001 already failed once.
- **Option D — embed the modules as Qt resources and extract at runtime: 4/4/1/2.** Found by the investigator, kept for the record. Removes the path, but buys a resource/importer lifecycle and extraction-error handling to avoid a packaging step, which is the wrong trade.

### Constraints the build must honour (Inspector-verified, not from the option report)
1. `pyproject.toml` has `[project] name = "gc-garminconnect"` but **no `[build-system]` section** — it is not installable as it stands.
2. The import namespace must change (e.g. `gc_garmin_adapter`); the collision with the upstream `garminconnect` import name is already documented in-repo at `pyproject.toml`'s mypy note, and `PyEmbeddedAdapter.cpp:199`'s allowlist plus its exception classification key off the current bare name.
3. **Windows installs with `--only-binary :all:` (`appveyor.yml:143`)**, which rejects a local sdist/path install outright. Either a wheel is built before that step or the flag is scoped. The option report missed this; it is the single most likely way this lands green in CI and broken on Windows.
4. Linux runs pip from `src/` (`Python/requirements.txt`) while macOS and Windows run it from the repo root (`src/Python/requirements.txt`). A RELATIVE local path in requirements.txt resolves against the CWD, not the requirements file, so it would differ per platform.
5. `GC_GARMIN_PYPATH` (`AddCloudWizard.cpp:179-191`) stays the FIRST override in every mode — it is the developer escape hatch and the only reason today's defect is survivable.
6. `appveyor.yml:252`'s Linux smoke is `AppImage --version`, which cannot fail on a missing module. It must become an extracted-bundle import that exits nonzero.

### Cascade impact
`REQ-NF-Pkg-001`'s traceability row is already REOPENED and cites B-STAGE9-38. `DES-007`'s packaging text and `prd.md:115`'s Phase-1 checklist both move: the checklist becomes the executable smoke step of constraint 6 plus a documented `CONTRIBUTING.md` section, with the Win/macOS legs DECLARED unexecutable on this Linux-only machine rather than silently dropped. Stage 9 cannot close until this is built. Separately flagged by the same investigation and NOT covered here: `src/Core/main.cpp:523-552` initializes CPython on the Garmin-only path without `PythonEmbed.cpp:236-254`'s deployed-`PYTHONHOME` setup, so bundled-Python search paths need their own verification — `main.cpp` gets its own finding and its own unit, not a hunk in this one (the hold once cited here was RELEASED by the user 2026-09-16, `STATE.md:750` — the separation is scope, not a hold; see DEC-061).

### Amendment 2026-09-19 (`garmin_inspector_v1_38`) — constraint 3 is FALSE; the adapter gets its own pip step

Measured, not argued. `s925_tz_investigator` (unit `DEC-058-pip-mechanics`) ran the exact Windows command form against a local source directory and pip installed it; the Inspector reproduced it independently in `/tmp/insp-pipcheck2` on pip 25.1.1 — `--only-binary :all:` built and installed `./pkg`, wheel and all. `--only-binary` constrains what pip may RESOLVE from an index, not a direct local path requirement.

- **Constraint 3 is struck.** It named the single most likely Windows failure and the failure does not exist. Nothing in this DEC may cite it.
- **Constraint 4 stands and is now the binding one**, confirmed by measurement in both directions: the same entry resolved from `src/` and from the repo root gave `ERROR: No such file or directory` and a clean install respectively. No repository-relative local entry in `requirements.txt` works from both CWDs, and an absolute one is not checked-in-able.
- **Therefore the carrier changes** (the DEC's chosen Option B is otherwise unchanged): the adapter is installed by its OWN explicit pip step in each recipe, with a path spelled relative to that recipe's own known CWD, NOT by an entry inside `-r requirements.txt`. `requirements.txt` keeps third-party deps only. Scored against the two alternatives — a portable relative entry (falsified above) and a prebuilt checked-in wheel (adds a build artifact and a staging dir to keep in sync, buying nothing once constraint 3 is gone) — the separate step wins on reliability and maintainability: CWD-relative resolution and `--only-binary` both stop applying to the adapter at all, rather than being worked around.
- **Layout verified buildable** by the same unit, and required by constraint 2: a `gc_garmin_adapter` package containing `{__init__,garmin_client,gc_rate}.py` with `[build-system] setuptools>=61`, tests excluded — wheel contents inspected, `import gc_garmin_adapter.garmin_client` succeeded from the install target. Intra-package imports become relative (`from .gc_rate`).
- Constraints 1, 2, 5 and 6 are unchanged.

### Amendment correction, same day (`garmin_inspector_v1_38`) — the distribution root, and the per-leg commands

The Amendment's layout bullet wrote the package path as `src/gc_garmin_adapter/` while retaining the sandbox's `where=["src"]`. `s925_tz_investigator` (unit `DEC-058-recipe-shape`) caught that those name two different distribution roots, and it was the Inspector's error, not the investigator's. Corrected against the real tree, which was re-read before this was written:

7. **The distribution root is `src/Python/garminconnect/`** — `pyproject.toml` already lives there (`src/Python/garminconnect/pyproject.toml`), and it already keys `mypy_path`, `pythonpath=["."]`, `testpaths=["tests"]` and `coverage source=["."]` to that directory. The package becomes `src/Python/garminconnect/gc_garmin_adapter/`, discovered with `where=["."]` + `include=["gc_garmin_adapter*"]`. The sandbox's extra `src/` nesting level is NOT adopted: it would move the modules out from under four existing tool configs to buy nothing.
8. **Per-leg install commands, each spelled from that leg's own CWD** (`--no-deps`, since third-party deps stay in requirements.txt): Linux runs from `src/` (`appveyor/linux/after_build.sh:12`), so `pip install -q --no-deps ./Python/garminconnect`, inserted after `:50` and before the `mv` at `:51`. macOS (`appveyor/macos/install.sh:54`) and Windows (`appveyor.yml:143`) both run from the repo root, so `./src/Python/garminconnect`.
9. **Windows must name the interpreter explicitly.** `appveyor.yml:138-143` prepends `C:\Python311-x64`, so the bare `python` there is NOT the bundled `C:\Python` that ships: use `C:\Python\python.exe -m pip ... -t C:\Python\lib\site-packages`, the tree copied wholesale at `appveyor.yml:216`.
10. **macOS and Windows must add an explicit repo-root `cd`.** Neither recipe sets one; both only imply it through a relative requirements path, so today's correctness is runner-dependent (investigator: UNVERIFIED from the recipe, and the recipe is all we ship). Linux already does this correctly.
11. **Constraint 6's smoke, concretely:** `--appimage-extract` into a temp dir, then run the EXTRACTED `usr/bin/python3` on `import gc_garmin_adapter` and assert the resolved `__file__` is under the extracted root — a host-resolved import must exit nonzero. Unexecuted here: no AppImage exists on this machine.
12. **The move is not import-neutral inside the dist, and the layout is otherwise PROVEN.** `s925_tz_investigator` (unit `DEC-058-layout-proof`) built the corrected layout from a copy of the real tree: the wheel carries exactly `gc_garmin_adapter/{__init__,garmin_client,gc_rate}.py` + dist-info and no `tests/`, and the installed import does not shadow upstream `garminconnect` (different name AND different file). ruff and mypy pass on the moved package. What constraint 7 did NOT account for: ELEVEN files under `src/Python/garminconnect/tests/` import the modules by their old top-level names (starting `tests/conftest.py:14`), so pytest stops at collection until they migrate to `gc_garmin_adapter.*`; `garmin_client.py:29`'s own `from gc_rate` becomes relative; and `pyproject.toml:26-31,42-44`'s comments still assert the modules are top-level, so they change with the move or they document a layout that no longer exists. Migrating those imports is part of the build unit, not a follow-up.
13. **The rename reaches C++, and a partial rename fails GREEN rather than loudly.** `s925_tz_investigator` (unit `DEC-058-import-callsites`) enumerated every binding of the old top-level name. Same commit, all of it: the six `PyImport_ImportModule("garmin_client")` calls at `src/Cloud/PyEmbeddedAdapter.cpp:555,666,736,796,871,926`; the exception-module allowlist root at `:198-210` and its `foreign_exception` reject at `:295-313` (a stale root silently declassifies every wrapper exception); the `sys.modules` type check at `:228-278`, whose input is now a dotted name; `src/Python/garminconnect/garmin_client.py:29`'s `from gc_rate`, which becomes relative; the eleven test files of constraint 12; the C++ fixture `unittests/Core/garminconnect/pystubs/garmin_client.py`, which becomes `pystubs/gc_garmin_adapter/garmin_client.py` with an `__init__.py`, plus its hard-coded consumers in `testGarminConnectPyAdapter.cpp:100-101,113,195-197`, `testGarminConnectPasswordPersistence.cpp:93,116-117,207-208`, `testPyProcessBootstrap.cpp:91-92`; and the two production path defaults `src/src.pro:266-274` and `src/CMakeLists.txt:1516-1520`, which must now name the directory CONTAINING `gc_garmin_adapter/`, as must the `GC_GARMIN_PYPATH` override at `src/Cloud/AddCloudWizard.cpp:182-183`. **The failure mode to design against:** `PyEmbeddedAdapter.cpp:93-109` prepends the supplied directory to `sys.path`, and that directory still holds the legacy top-level module — so any single missed literal resolves the OLD module and passes, hiding the packaging defect. The pystub filename is the same false-green route for the embedded tests, and `PyProcessBootstrap.cpp:79-90` leaves user site-packages enabled, so an externally installed old module is a third. Comment/diagnostic wording (`PyEmbeddedAdapter.h:37,60,73,80,88`, `IGarminPyAdapter.h:12,153,177,209,232,254,263`, `GarminWorker.cpp:53`, the old-name `qDebug` strings) may follow later; nothing executable may.

14. **Each leg's adapter step BUILDS a wheel, so it needs a build backend — and that is a third network dependency per leg, not a free one.** MEASURED by `s925_tz_investigator` (unit `DEC-058-pipleg-measure`) against a /tmp copy of the real post-rename tree: both constraint-8 commands, spelled exactly as written and run from exactly the right CWD, exit **1** — `Could not find a version that satisfies the requirement setuptools>=61`, because pip's default build isolation fetches the backend declared at `pyproject.toml:5-7` from the network. Nothing installed. The path spelling is NOT what failed. With `--no-build-isolation` and setuptools already present, both forms exit 0 and each target contains exactly `gc_garmin_adapter/{__init__,garmin_client,gc_rate}.py` plus `dist-info`, `tests`-leaked=0, and an import from outside the source tree resolves to the installed copy. The three appveyor legs do have network (they already install `-r requirements.txt` from PyPI), so build isolation will work there — but it is now a stated precondition rather than an assumption, and `--no-build-isolation` is the offline-safe spelling if any leg is ever pinned to a wheel cache.
15. **Ordering is load-bearing: the adapter step must follow the `requirements.txt` install, on every leg.** MEASURED in the same unit: with `--no-deps` and only the adapter's own target on `PYTHONPATH`, `import gc_garmin_adapter.garmin_client` SUCCEEDS and reports `_gc None` — the module deliberately swallows its dependency `ImportError` (`gc_garmin_adapter/garmin_client.py:31-58`) — while `import garminconnect` and `import curl_cffi` each fail with `ModuleNotFoundError`. So a leg that installs the adapter without its payload produces an importable, wholly non-functional adapter that no import smoke can catch. Constraint 8 already orders the Linux leg correctly (after `appveyor/linux/after_build.sh:50`); macOS and Windows owe the same explicit ordering, and constraint 11's smoke must assert something the swallowed `ImportError` cannot fake.
16. **Constraints 9 and 10 re-verified against the recipes as they stand (SOURCE-CHECK, not measurement — macOS/Windows are unexecutable on this Linux host).** Both still hold: `appveyor.yml:138` prepends `C:\Python311-x64` and `:141-143` uses a bare `python`, while `:216` copies the bundled `C:\Python` — so the explicit `C:\Python\python.exe -m pip` adapter step is still required and still absent. Neither `appveyor.yml:138-143` nor `appveyor/macos/install.sh:47-54` sets an explicit repo-root `cd`; the later `cd src\release` at `:208` is unrelated.

17. **Constraint 5's "first import precedence" needed a repair, and the repair is now settled (addendum 2026-09-20, `garmin_inspector_v1_46`).** `B-STAGE9-38-u3-r3-review` filed B-STAGE9-44/-45/-46: the explicit-override path guaranteed neither precedence nor origin. `B-STAGE9-44-46-repair-scope` settled the shape and refuted the Inspector's own two-option framing. Binding: (a) **sys.path** — find the FIRST exact occurrence of the override dir; at index 0 leave it, later remove that one occurrence and insert at 0, absent insert at 0. Leave later duplicates alone and do NOT canonicalize paths — symlink/relative equivalence changes Python semantics. A failed list op, or a `sys.path` that is not a list, must FAIL CLOSED on the explicit-override path, never clear the error and import. (b) **sys.modules** — neither evict nor blanket-reject. Inspect the cached `gc_garmin_adapter` and every `gc_garmin_adapter.*` entry and reuse ONLY entries proven loaded from the explicit directory; anything from elsewhere or unverifiable raises an import error. Eviction is unsound (live adapters keep objects from the old module while later imports build a second type universe; package C extensions need not reset safely), and a blanket reject breaks a legitimate second adapter construction in one process. (c) **Scope** — with NO override, resolving from CPython's existing `sys.path` stays INTENDED; only the explicit-override path is repaired. (d) Minimal change set: `src/Cloud/PyEmbeddedAdapter.cpp`, `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp`, as two isolated test cases, not one inherited-state case.

## DEC-059 — B-STAGE9-36 round 3: a DEC arms a managed root by an explicit `Arms:` declaration, never by prose a guard greps

- Status: accepted (Inspector `garmin_inspector_v1_37`, Three-Options Doctrine — ordinary tooling decision, not a human-in-the-loop gate)
- Reversibility: high — one resolver function, its tests, and one bullet added to DEC-054's own entry. No production code, no shipped surface.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-36 (blocking, NOT-CLOSED after two repair rounds), DEC-057 (whose mechanism this amends)
- Dependents: `unittests/buildguard/garmin_lint_ownership_guard.py` (`_pattern_directory_prefixes`, `check_empty_roots_are_declared`), `unittests/buildguard/test_garmin_lint_ownership_guard.py`, DEC-054's entry body, every future `pre_armed_by` declaration
- Origin: `garmin_codex_reviewer`, unit `B-STAGE9-36-r2-review`, item 1 BLOCKING; the Inspector confirmed the mechanism by reading `:933-941`, not by relaying the report

### The problem

DEC-057 requires an empty managed root to name "the DEC that armed **its** regexes." Two repair rounds have tried to decide that question by reading the ledger's prose, and each was defeated by a narrower instance of the same class. Round 1 scanned the whole ledger for `\bDEC-\d+\b`, so a bare mention counted as existence. Round 2 anchored on a real `^## DEC-ddd` heading and then required the entry's own body to contain one of the root's pattern prefixes — but `prefix in entry` is a raw substring test, so a DEC body saying "does not arm `pkg/`", or carrying the prefix inside a fenced example or an unrelated aside, authorizes the root exactly as a genuine arming statement would. Mention is still being read as authorization; only the haystack got smaller.

The class does not close by narrowing further, because "does this prose entry ARM this root" is not decidable from prose. `LSN-083`'s shape — a gate whose PASS proves nothing — survives every round that keeps grepping.

### Options scored (reliability / scalability / maintainability / best practices)

- **Option A — the DEC entry carries an explicit machine-readable `- Arms:` bullet listing the exact `patterns` globs it armed; the guard parses that bullet and requires exact set membership, never a substring over prose (CHOSEN): 5/5/5/5.** Exact match, so negation, fenced examples and quoted asides cannot authorize — the failure class is removed rather than narrowed. `- Arms:` is the same `- Key: value` shape the entries already use for `Status:`/`Serves:`/`Dependents:`, so it is a convention this ledger already reads, not a new format. Any future pre-armed root costs one bullet. Cost is one ledger edit: DEC-054 gains `- Arms: unittests/Core/stderrbuf/*`.
- **Option B — keep the substring test, exclude negation/fenced/quoted contexts: 2/2/2/1.** This is round 3 of the same move. The reviewer's negation example is one instance of an unbounded natural-language problem, and each exclusion rule is a new place for the next instance to hide.
- **Option C — accept the residual as a declared false negative and close, as B-STAGE9-15 did for its all-`key=value` literal: 2/3/3/2.** Rejected on the distinction that makes B-STAGE9-15's precedent inapplicable: that residual was the irreducible consequence of a shape-based exemption. This one is not irreducible — Option A removes it exactly, at the cost of one bullet — so accepting it would be accepting a defect that has a cheap exact fix.

### Constraints the build must honour

1. `- Arms:` is matched on a line-anchored bullet inside the resolved entry body only, never anywhere in the file, or Option A inherits round 1's defect at a different scope.
2. A root's `patterns` must be matched as whole entries against the parsed list, not as substrings of it: `pkg/*` must not be satisfied by an `Arms:` naming `pkg/sub/*`.
3. An entry with no `- Arms:` bullet is a finding with its own message — silent on the distinction is what DEC-057 exists to stop.
4. `_pattern_directory_prefixes`'s empty-tuple case (reviewer item 2, NON-BLOCKING, fail-closed) is superseded: matching is against `patterns` themselves, so a root with no `*`-suffixed pattern is no longer a special case.

### Amendment, 2026-09-19 (same day, `garmin_inspector_v1_37`, after round 3 was delivered GREEN)

Round 3 implemented the above and the Inspector verified it closes the reviewer's blocking class — but re-probing the real ledger found the same class reappearing one line lower, in the bullet itself. `_armed_patterns` harvests EVERY backtick-quoted token on the `- Arms:` line, so prose sharing that line becomes armed globs. Measured, not theorised: this decision's own first draft of DEC-054's bullet carried explanatory prose after the glob and armed four things — `unittests/Core/stderrbuf/*`, `garmin_lint_ownership_guard.py`, `pre_armed_by`, `ManagedRoot.patterns` — and `- Arms: \`pkg/*\` but explicitly NOT \`evil/*\`` arms `evil/*`. A negation on the Arms line authorizes exactly as round 2's prose negation did. Separately, `_ARMS_BULLET.search` honours only the FIRST bullet, so a second one is silently ignored.

Two further constraints, therefore:

5. The `- Arms:` line is a glob list and nothing else. A line carrying any non-whitespace outside its backtick-quoted tokens is a finding, not a silent harvest — the guard must reject it rather than parse around it. Commentary goes on its own `- Arms-note:` line, which the guard never reads. DEC-054's bullet was rewritten to this shape in the same pass.
6. More than one `- Arms:` bullet in a single entry is a finding. Silently honouring the first is the same failure as silently harvesting prose: the record says something the guard does not read.

### Cascade impact

DEC-057's mechanism is amended, not superseded — its rule ("an empty managed root must be DECLARED pre-armed or go RED") stands unchanged; only the proof that a declaration is true changes. DEC-054's entry gains the `- Arms:` bullet in this same pass, so the live record satisfies the new rule with no test-fixture special case. B-STAGE9-28 stays frozen and ships in the same commit. The reviewer's items 2, 3 and 4 are NON-BLOCKING and recorded as such; item 3 (the `## DEC-040` amendment heading truncating that entry's body early) can only produce a false finding, never a false PASS, and gets no unit of its own.

## DEC-060 — B-STAGE9-36 round 8: an Arms declaration names its own DEC, and ambiguity is a hard error, so Markdown block context stops being the guard's problem

- Status: accepted (Inspector `garmin_inspector_v1_39`, Three-Options Doctrine — ordinary tooling decision on a project-owned guard, not a human-in-the-loop gate)
- Reversibility: high — one regex, one equality check, one error branch, and a respelling of the two `- Arms:` bullets that exist today. No production code, no shipped surface.
- Decided / last-reviewed: 2026-09-19
- Serves: B-STAGE9-36 (blocking, NOT-CLOSED after SEVEN repair rounds), DEC-059 (whose grammar this amends), DEC-057 (whose rule is untouched)
- Dependents: `unittests/buildguard/garmin_lint_ownership_guard.py` (`_ARMS_LINE`, `_armed_patterns`, `_masked_ledger_lines`), `unittests/buildguard/test_garmin_lint_ownership_guard.py`, DEC-054's `- Arms:` bullet, every future declaration
- Origin: `garmin_codex_reviewer` found a NEW masking hole on each of rounds 2-7 (prose substring, fence grammar, HTML comment, raw-HTML type 1, now CommonMark HTML block type 6 `<div>`); the Inspector read the sequence as one defect rather than seven

### The problem, restated

DEC-059 asks the guard a question Markdown cannot answer cheaply: *is this `- Arms:` line real, or is it an illustration?* Nothing distinguishes the two but their container, so the guard grew a hand-rolled block-context scanner. Round 7's reviewer verdict is the seventh consecutive finding of the same class — `<div>` is CommonMark HTML block type 6, and types 3, 4, 5 and 7 are still open behind it, as are link reference definitions and indented code. Round 7 also introduced the mirror defect: a `<!--` inside a Markdown code span opens the scanner's comment state and silently REJECTS an authorized declaration. The scanner is converging on a conformant CommonMark block parser, which is not a thing a lint guard should contain.

### Options scored (reliability / scalability / maintainability / best practices)

- **Option C — the declaration names its own DEC, and ambiguity is a hard error (CHOSEN): 5/5/5/4.** A real declaration reads ``- Arms DEC-054: `glob` `` and is honoured only inside `## DEC-054`'s own entry. An `- Arms`-shaped line whose id does not match the enclosing entry raises `ArmsBulletMalformed` — a loud refusal, never a silent skip — which is what an illustration inside some *other* DEC's entry now produces, telling its author to spell the example `- Arms-example:` instead. The guard stops asking what container a line sits in, so no container can defeat it. Loses a point on best practices: it is a bespoke convention rather than a standard one.
- **Option B — parse the ledger with a conformant CommonMark library and read the bullet from the AST: 5/4/4/4, blocked on measurement.** Correct by construction, and it would fix the code-span false-negative for free. But MEASURED on this host: `markdown_it`, `commonmark`, `mistune`, `marko` and `markdown` are all absent, and `.pre-commit-config.yaml:100` carries `additional_dependencies: []` for this directory. DEC-054 refuses to adopt a gate whose arrival state cannot be measured, and the shellcheck declared-gap is this project's own precedent for refusing exactly that. Reconsider if a parser ever lands in the pinned tool set for another reason.
- **Option A — keep extending the scanner (add HTML block types 3-7): 2/2/2/2.** It is what the last five rounds did. Each round closed the instance the reviewer named and left the class open, and the remaining grammar is larger than the part already written.

### The accepted consequence, pinned so the boundary cannot move silently

Under Option C, ``- Arms DEC-054: `glob` `` sitting inside a fence, an HTML comment or a `<div>` **within DEC-054's own entry** is still honoured. That is irreducible for any context-free rule, and it is the correct reading: writing this entry's own id on an Arms line IS the declaration, whatever it is wrapped in. Recorded here as an accepted false positive on the same terms DEC-053 accepted one for B-STAGE9-15, so a future round cannot file it as a new finding.

### Cascade impact

DEC-059's constraints 1, 2 and 5 stand; its constraint 6 (exactly one bullet per entry) is subsumed — two bullets carrying this entry's own id is still `ArmsBulletMalformed`. The grammar in DEC-059's constraint text changes from `- Arms:` to `- Arms <DEC-id>:`, and DEC-054's own bullet is respelled with it. `_masked_ledger_lines` is NOT deleted: heading resolution still needs it, and round 5's single-pass property stands. DEC-057's rule is untouched. B-STAGE9-28 stays CLOSED and frozen and still ships in the same commit.

### Amendment 2026-09-20 (round 9) — near-miss recognition is deliberately loose; only the canonical spelling is honoured

Round 8's reviewer verdict changed the finding class from forgery to SUPPRESSION: no authorization can be forged, but a declaration that MISSES the canonical spelling falls out of `_ARMS_SHAPED` entirely and is silently skipped, so a valid bullet elsewhere in the entry still permits PASS while the ambiguity goes unreported. DEC-060's own rule requires ambiguity to be a hard error, so a skip is a rule violation, not a gap in it.

The verdict named two inputs. The Inspector probed both against the module directly rather than relaying them, and ONE is real. A second same-id bullet already raises (`_armed_patterns` collects then rejects on `len(bullets) > 1`), so that half is withdrawn; `findings.md`'s B-STAGE9-36 row is corrected in the same pass. The other is real and generalises — five shapes, each measured, each returning the VALID entry's globs when a valid bullet is also present and `None` when it is not: a missing space after `Arms`; two spaces after the list marker; a two-space-indented marker; an asterisk marker instead of a hyphen; and a lower-case `arms`.

The remedy is a second, deliberately LOOSE recognizer that is not the declaration parser. A line is Arms-SHAPED when its list marker (`-`, `*` or `+`; see the round-11 amendment below, which retires this sentence's original three-space indentation cap) is followed by the token `arms`, compared case-insensitively with interior whitespace collapsed. Anything Arms-shaped that is not EXACTLY canonical is `ArmsBulletMalformed`. This inverts the current failure direction on purpose: the loose recognizer over-reaches, and over-reach is a loud refusal a human fixes in one edit, whereas under-reach is a silent PASS nobody sees. Constraints:

1. The reserved illustration forms stay excluded at the shape level, still spelled `Arms-<word>:` — that exclusion is what makes commentary possible at all and it is unchanged.
2. Recognition is loose; honouring is strict. Exactly one spelling is honoured, and the malformed message must name what the line should have said.
3. Each of the five measured shapes gets its own regression assertion, both alone and beside a valid bullet, because "alone" and "beside a valid bullet" fail differently today and only the second is a false PASS.
4. Round 8's item 5 rides here: no test drives `check_empty_roots_are_declared` for the real DEC-054 pair, and the live path skips at `:1113-1115` because `unittests/Core/stderrbuf/` is no longer empty, so the live PASS proves nothing about that path. It needs a test that reaches the function with a genuinely empty managed root.
5. This amendment's own prose carries no line-initial Arms-shaped text, so widening the recognizer cannot take the ledger RED on the act of recording it.


### Amendment 2026-09-20 (round 11) — the three-space indentation cap is retired, and the leading run is defined by Unicode CATEGORY, not by whitespace

Two blocking items from `garmin_codex_reviewer`'s round-11 delta-check resolve into one correction, because they are the same mistake pointing in opposite directions.

**The cap is withdrawn.** The sentence above originally read "indented at most three spaces." That cap contradicts this amendment's own doctrine two sentences later: under-reach is a silent PASS nobody sees. A four-space or tab-indented `- Arms DEC-054:` is visible to a human as a declaration, reached `_armed_patterns`'s `continue`, and could be concealed by a valid neighbouring bullet — measured, not argued, at instance nine. The CommonMark case for the cap is real (four spaces denote an indented code block, so such a line is not a list item) but it loses here for the reason DEC-060 exists at all: this decision expressly DECLINED to recover Markdown block context, and already accepts a self-id'd declaration inside a fence. A recognizer that refuses to parse containers cannot then invoke container semantics to narrow itself. Adjudicated by the reviewer that raised the opposing finding, which then argued the other side and WITHDREW its own blocking item rather than defend it.

**The leading run is a category, not a character list.** Rounds 9, 10 and 11 each closed one class of leading character and left the next reachable — `{0,3}` spaces, then ASCII `[ \t]`, then `\s`. `\s` is still an enumeration in disguise: Python's `str.isspace()` is False for every Unicode FORMAT control, so U+200B, U+FEFF, U+200E/200F, U+202A-202E and U+2060 before the marker still reach `continue`. Measured by the Inspector rather than enumerated again (`/tmp/insp-exchange/probe_invisible_prefix_classes.py`): Unicode general categories `Zs`, `Zl`, `Zp`, `Cc` and `Cf` together are a strict SUPERSET of `str.isspace()` — 254 codepoints against 29, with nothing in `isspace()` falling outside them — and they cover every character the reviewer named. That set is the rule, because it is closed under "invisible or non-printing character an editor inserted" rather than being a list someone has to extend next round.

**Accepted false negative, pinned so the boundary cannot move silently.** Characters that RENDER blank but are real printing characters stay out of scope: U+115F and U+3164 (HANGUL FILLER, category `Lo`), U+FFA0, and U+2800 (BRAILLE PATTERN BLANK, `So`). Recognising them would make the guard reason about font rendering, which is unbounded. This is the same shape as DEC-053's pinned exemption: a known, recorded gap beats an open-ended chase.

Amended grammar, replacing the capped sentence's effect: a line is Arms-SHAPED when, after any leading run of characters in categories `Zs`/`Zl`/`Zp`/`Cc`/`Cf`, a list marker (`-`, `*` or `+`) is followed by the token `arms`, compared case-insensitively with interior whitespace collapsed. Indentation width and Markdown block context do not narrow recognition.

Rounds 9, 10 and 11 need no reclassification: every shape they closed remains a genuine suppression fix under the amended rule.

## DEC-061 — B-STAGE9-40 remedy: the startup path consumes `ensureInitialized()`'s failure `Result` nonfatally, at the call site that discards it

- Status: accepted (Inspector `garmin_inspector_v1_40`, Three-Options Doctrine — ordinary implementation decision inside B-STAGE9-40's own mechanism, not a human-in-the-loop gate; the standing-rule correction it rests on was independently verified by a fresh unbriefed agent before recording)
- Reversibility: high — one call site, one retained flag, one log line. No new class, no new channel, no shipped surface beyond a log record.
- Decided / last-reviewed: 2026-09-20
- Serves: B-STAGE9-40 (blocking), B-STAGE9-39 (which this makes observable rather than invisible)
- Dependents: `src/Core/main.cpp:552`, `src/Python/PyProcessBootstrap.cpp:90-106,121-124`, the app-process smoke B-STAGE9-39 owes, `appveyor.yml:248-256`
- Origin: `s925_tz_investigator` (unit `B-STAGE9-40-options`) scored three options; the Inspector found the scoring's shared premise false and re-scored

### The correction this decision rests on

All three options were scored against "`src/Core/main.cpp` is under a hard hold", which is FALSE on this project's own record. `STATE.md:750,758-760` records the user RELEASING that hold on 2026-09-16. Cursors `v1_36` through `v1_39` (`STATE.md:1432,1466,1495,1525,1563,1608,1643`, and the same wording in `findings.md:574` and this file at `:2791`) each carried "hard hold in force" forward without naming a re-imposing authority or act. `garmin_codex_reviewer`, fresh and unbriefed (unit `DEC-061-hold-premise`), searched all three ledgers for a post-2026-09-16 re-imposition and found none: verdict RELEASED. The hold is therefore not a cost on any option, and the option report's own penalty on B and C ("requires its own held-`main.cpp` unit and commit") is withdrawn.

### Options re-scored (reliability / scalability / maintainability / best practices)

- **Option B — consume the `Result` at `main.cpp:552`, nonfatally: retain the failure state, log it, continue startup (CHOSEN): 5/4/4/5.** It closes B-STAGE9-40's stated mechanism at the line that causes it, and it makes the failure observable to a log and to an app-process assertion rather than only to a user who happens to open the Garmin wizard. Same reviewer unit verified the change is behaviour-neutral: `Result` carries only `ok` and a diagnostic `error` (`PyProcessBootstrap.h:64-70`); the failure is already cached in `g_result` with no retry (`PyProcessBootstrap.cpp:31-44,90-106`); the other direct caller already consumes it (`PythonEmbed.cpp:285-287,382-397`); verdict NO-BEHAVIOUR-CHANGE beyond the added log. It also matches this codebase's own nonfatal-optional-subsystem precedent — failed R init becomes `rtool=nullptr` (`main.cpp:504-512`) and failed Python embedding warns, sets `loaded=false`, and returns (`PythonEmbed.cpp:382-398`).
- **Option A — leave `:552` as it is; refuse Garmin at first interaction with a localized "Python unavailable" outcome: 4/5/4/4.** Its top score depended on avoiding the held file. Without that premise it does not close the finding: the discarded `Result` stays discarded, the failure stays invisible to CI and to the log, and the app still reports success when its interpreter is dead. Kept as a SEPARATE, non-blocking follow-on for the user-facing surface — it is a better error message, not a fix for this defect.
- **Option C — treat bootstrap failure as fatal and exit nonzero: 5/2/4/1.** Couples whole-application availability to one optional cloud provider, contradicting both nonfatal precedents above and Garmin's own lazy construction (`AddCloudWizard.cpp:142-149,170-194`). Rejected on scalability and best practices, not on cost.

### Constraints the build must honour

1. The assertable predicate is `PyProcessBootstrap::isInitialized()` (`PyProcessBootstrap.cpp:121-124`) observed after `main.cpp:552`. It has no caller today; this decision gives it one.
2. No new error channel. DEC-051 rejects a parallel `ErrorBus`; the log record uses the existing route.
3. `--version` cannot host the assertion — argument parsing exits at `main.cpp:399-401` before initialisation, and `--version` is exactly what CI runs on Linux (`appveyor.yml:248-252`) and macOS (`:253-256`). The smoke surface is unit `B-STAGE9-40-smoke-surface`'s measured result, not an assumption.
4. Log-flush ordering is a real constraint, MEASURED: bootstrap is at `main.cpp:552`; stderr redirection and POSIX `_IOLBF` happen later at `:610-614` and `:173-213`. A startup record emitted before redirection cannot rely on B-STAGE9-12's redirected-file flushing.
5. `src/Core/main.cpp` still gets its own unit and its own commit — not because of a hold, but because B-STAGE9-39's own fix touches the same region and the two defects are independent.

### Cascade impact

`STATE.md`, `findings.md:573,574` and this file's DEC-058 entry at `:2791` all assert a hold that does not exist; each is corrected in the same pass that records this DEC. B-STAGE9-39's remedy is unchanged but is now verifiable, because a failed deployed-`PYTHONHOME` setup will report itself. DEC-058's diff is untouched: nothing here rides in it.

## DEC-062 — B-STAGE9-39 remedy: a Qt-side deployment locator feeds explicit `PyConfig.home`/`program_name` before `PyConfig_Read`; no `PYTHONHOME` mutation, no `Py_SetProgramName`

- Status: accepted (Inspector `garmin_inspector_v1_42`, Three-Options Doctrine — ordinary mechanism choice inside B-STAGE9-39's own remedy, not a human-in-the-loop gate; the mechanism was MEASURED against the live reproducer before recording, not reasoned about)
- Reversibility: medium — one new Qt-side locator, two call sites consume it, one deprecated API retired. No new class hierarchy, no new channel, no shipped surface.
- Decided / last-reviewed: 2026-09-20
- Serves: B-STAGE9-39 (blocking, REPRODUCED). Depends on DEC-061 landing first — until `main.cpp:552` consumes the `Result`, a failed home setup reports nothing (B-STAGE9-40)
- Dependents: `src/Python/PyProcessBootstrap.cpp:79-100,126-140`, `src/Python/PyProcessBootstrap.h:45,64-70`, `src/Python/PythonEmbed.cpp:237-255,269`, `src/CMakeLists.txt:1085`, the three installer recipes DEC-058 wired
- Origin: `s925_tz_investigator` scored three options (unit `B-STAGE9-39-remedy-options`) and then measured the chosen mechanism on five legs (unit `B-STAGE9-39-pyconfig-measure`)

### The measurement this decision rests on

Five legs against the same reproducer B-STAGE9-39 was reproduced under (user namespace, tmpfs over the host `/usr/lib/python3.13`, self-contained payload at `opt/python3.13`), reading process exit, `Py_InitializeFromConfig` result, and whether payload `encodings` imported:

| Leg | exit | `Py_InitializeFromConfig` | payload `encodings` |
| --- | --- | --- | --- |
| Baseline — home absent | 11 | `error:Failed to import encodings module` | no |
| Option 2 — `PYTHONHOME`=payload | 0 | ok | yes |
| Option 1 — env absent, explicit fields before `_Read` | 0 | ok | yes |
| Ordering control — explicit fields after `_Read` | 0 | ok | yes |
| Conflict control — bogus env + explicit fields before `_Read` | 0 | ok | yes |

Two results decide the shape. Explicit pre-`_Read` fields work with `PYTHONHOME` absent entirely, and they WIN over a deliberately hostile `PYTHONHOME` — so the environment variable is not needed as a carrier and not a hazard once the fields are set. And the deprecated `Py_SetProgramName` is not required: an `LD_PRELOAD` interceptor set to exit 92 on that symbol never fired while the Option 1 leg still exited 0 and imported payload `encodings`.

### Options scored (reliability / scalability / maintainability / best practices)

- **Option 1 — Qt-side locator returns `{home, programName}`, passed into `PyProcessBootstrap::Config` and set as explicit `PyConfig` fields before `PyConfig_Read`; `PythonEmbed` consumes the same result instead of owning the setup (CHOSEN): 5/5/4/5.** Config is passed, not published to global mutable environment, so two initialisers cannot disagree and a hostile inherited env cannot win. It is the only option the measurement actually exercised end-to-end, and it retires a deprecated API rather than preserving it.
- **Option 2 — the locator still `qputenv`s `PYTHONHOME`: 4/3/3/2.** It measurably works, which is why it is not rejected on reliability. It keeps configuration in mutable process-global state that any later code or any inherited environment can contradict, and it keeps the deprecated program-name API alive. Rejected on maintainability and best practices.
- **Option 3 — fold deployment discovery into `PyProcessBootstrap` itself: 3/3/2/3.** Puts application layout policy inside DEC-052's generic process-lifecycle owner, which every future embedder then inherits. Rejected on separation, not on whether it would work.

### Constraints the build must honour

1. Call order is the measured one: `PyConfig_InitPythonConfig` → `PyConfig_SetString(&config.home, <payload>)` → `PyConfig_SetString(&config.program_name, <payload>/bin/python3.13)` → `PyConfig_Read` → `Py_InitializeFromConfig`. Pre-`_Read` is the contract; the post-`_Read` control only passed with the environment unset and does not survive the conflict case.
2. The locator is Qt-side application policy and lives outside `PyProcessBootstrap`, which only sets what it is handed. `PyProcessBootstrap` is NOT Qt-free (`PyProcessBootstrap.h:45,64-70` uses `QString`) — its real boundary is settings-free and layout-free, so a `QString` in its `Config` is in-boundary and a search for the payload is not.
3. `PythonEmbed.cpp:237-255` stops mutating `PYTHONHOME` and consumes the same locator result, so the two initialisers can no longer compute different homes. `PyProcessBootstrap.cpp:126-140`'s deprecated `Py_SetProgramName`, reached by `PythonEmbed.cpp:269`, is retired — the measurement proves it is dead weight, not a fallback.
4. The locator PREFERS a deployed payload when one exists. Today the opposite holds: B-STAGE9-39 measured that with the host stdlib visible the copied payload is ignored entirely, so "fall back to the payload only when the host stdlib is missing" reproduces the shipped-bundle defect on any machine that happens to have a system Python.
5. Build-system cascade, not just a test-matrix note: `src/CMakeLists.txt:1085` defines `GC_HAVE_PYTHON` where qmake defines `GC_WANT_PYTHON`, so under CMake a Python-only configuration reaches NEITHER guarded initialiser and a both-on configuration reaches the Garmin bootstrap but not `PythonEmbed`. The remedy must name which macro each new call site is guarded by, in both build systems.
6. Own unit, own commit — DEC-061 constraint 5 already separates this from `main.cpp:552`, and DEC-061 lands first because it is what makes a failure here observable at all.
7. `PythonEmbed.cpp:291-296`'s dead `PySys_SetPath` stays dead. No option needs it revived.

### Amendment 2026-09-20 — the layout contract, measured; and the locator is an EXTRACTION, not new logic

Unit `DEC-062-locator-contract` (`s925_tz_investigator`) read all three recipes and corrected two premises this Inspector put in its own brief. Both corrections make the remedy smaller, not larger.

All three legs DO place a self-contained home beside the executable, at three different relative paths:

| Leg | home, relative to `applicationDirPath()` | placed by |
| --- | --- | --- |
| Windows | `.` — the app dir itself | `appveyor.yml:215-224` xcopy; `GC3.8-Master-W64-QT6.nsi:100-105,149-164` installs `python.exe`/`python311._pth`/`python311.zip`/`Lib/` into `$INSTDIR` |
| Linux | `opt/python3.11` | `appveyor/linux/after_build.sh:14-18,42-56` moves the extracted Python AppImage's `opt/` to `appdir/opt`; version pinned at `appveyor.yml:13-16` |
| macOS | `../Frameworks/Python.framework/Versions/Current` | `appveyor/macos/after_build.sh:22-28,62-68` |

**Correction 1 — the "macOS ships an unrelocated brew python" premise is FALSE.** `appveyor/macos/install.sh:49-59` installs into brew, but `after_build.sh:18-25,32-35,70-79,89-96` copies the framework into `Contents/Frameworks`, copies brew site-packages into it, and retargets the binary at the bundled framework. macOS is not the unfixable leg. A genuinely unrelocated brew-only leg would have to return NO deployed result rather than an absolute brew path — returning one would violate the installer contract, not satisfy it.

**Correction 2 — `PythonEmbed.cpp:237-255` ALREADY encodes all three layouts and its answer agrees with all three recipes** (`:238-242` for the macOS framework and the Linux `/opt/python3.<minor>` form, `:245-259` validating with `pythonInstalled` before selecting). So the locator is an EXTRACTION of working logic into a shared Qt-side function that `PyProcessBootstrap` can also be handed — not a new discovery algorithm. This lowers the risk of constraint 3 and raises its bar: the extraction must be behaviour-preserving for the existing `GC_WANT_PYTHON` path, which is a working shipped feature.

**Constraint 8 (new).** The predicate is ordered candidate `{home, program}` pairs tested with `QFileInfo::exists`/`isExecutable`, first complete pair wins — three layout candidates, one locator, no Python headers. MEASURED to return NO deployment in a source-tree run and under current CTest: none of the Windows, Linux or macOS candidate executables exist under `src/` or `build/unittests/Core/garminconnect/`. Constraint 3's "must not find a payload in a dev run" is therefore satisfied by this predicate as specified, not merely intended.

**Constraint 9 (new).** `program_name` is version-derived, never hardcoded. Constraint 1's `python3.13` spelling comes from the scratch reproducer; the shipped legs are `python3.11` today (`appveyor.yml:13-16`), and `PythonEmbed.cpp:241` already derives the minor version rather than fixing it. The extraction keeps that derivation.

The deployed-bundle leg remains unexecutable on this machine and was not attempted (`patchelf`, `linuxdeployqt`, `appimagetool` and `/dev/fuse` all absent). Every row above is READ from the recipes; the source-tree/CTest absence in constraint 8 is MEASURED.

### Amendment 2, 2026-09-20 — the extraction surface is much larger than "Correction 2" said, and constraint 5 is now a blocker

Unit `DEC-062-extraction-safety` (`s925_tz_investigator`) enumerated every reader of `PYTHONHOME` rather than arguing none exists, and corrected this Inspector a third time. The remedy does not change shape; its surface does.

**Premise correction — "stops mutating `PYTHONHOME`" is not "erases `PYTHONHOME`".** `PythonEmbed.cpp:245-252` deliberately FALLS BACK to an inherited, user-supplied `PYTHONHOME`. The extraction must preserve that fallback. Unsetting the variable, or ignoring an inherited one, is a regression this decision does not authorise and never intended.

**Reader census (READ, whole tree).** Exactly one production reader/writer: `PythonEmbed.cpp:85-162,245-259` — settings, then inherited `PYTHONHOME`, then its own validation `QProcess`, to which it passes the selected value explicitly. CPython itself is the other consumer, via `PyConfig_Read` at `PyProcessBootstrap.cpp:79-100`, which this decision's measured explicit fields override. `testPyProcessBootstrap.cpp:139-155,320-336` injects an invalid value into a re-exec child and stays valid precisely because it does not depend on the app setting one. No reference in R, CMake, qmake, installer rules, or production Garmin Python. The `.venv` activate scripts touch it only when a developer sources them. Four other child processes merely inherit the environment (`QuarqRideFile.cpp:27-39,67-92`, three QTest children, one pytest subprocess) and none reads this key.

**Constraint 3 RESTATED — the extraction surface.** "Correction 2" named `PythonEmbed.cpp:237-255` and that was too narrow. `PythonEmbed` also derives `pybin`, launches that interpreter to validate it, checks the required major/minor, records the resulting path (`:85-210,258-269`), and on Linux adds deployed-home `site-packages` ONLY when the selected home equals the deployed candidate (`:339-345`). A locator that only detects a payload regresses configured homes, inherited homes, PATH discovery, version validation, and that Linux condition. The behaviour-preserving contract is all of that selection-and-validation behaviour, not the deployed-path arithmetic alone.

**Constraint 5 UPGRADED to a precondition.** qmake can share one locator under `GC_WANT_PYTHON || GC_WANT_GARMINCONNECT` (`src/src.pro:220-243,256-260,317-353`; `main.cpp:515-552`). CMake cannot: Python sources are selected by `GC_WANT_PYTHON` while only `GC_HAVE_PYTHON` is defined (`src/CMakeLists.txt:1056-1086`), and Garmin defines `GC_WANT_GARMINCONNECT` (`:1448-1508`). So under CMake a Python-only configuration reaches NEITHER guarded initialiser and a both-on configuration reaches only the Garmin guard. This pre-existing mismatch BLOCKS any measured "both configurations reach the same home" claim until it is repaired, which makes it work this remedy must sequence, not a note in its test matrix.

Every item above is READ from source; nothing here was runtime-measured, because the extraction is unbuilt. `PythonEmbed.cpp:291-307`'s dead `PySys_SetPath` stays dead — no option implicates it.

### Cascade impact

`findings.md:573`'s B-STAGE9-39 row moves from "REMEDY OPTIONS SCORED / NOT YET A DEC" to decided-not-built. `DES-007`'s packaging text and the three installer recipes DEC-058 wired inherit a new requirement: the recipe must place the payload where the locator looks, which is now a named contract rather than an implicit `PYTHONHOME` side effect. DEC-052 is unchanged in substance — its bootstrap still owns process-level CPython lifetime; it simply receives the home rather than inferring it. The app-process smoke B-STAGE9-39/B-STAGE9-40 both owe is unchanged and still unexecutable on this Linux-only machine for the deployed-bundle leg.

### Scope-pass corrections (addendum 2026-09-20, `garmin_codex_reviewer` unit `DEC-062-scope`, adopted by `garmin_inspector_v1_46`)

Mechanism sound; implementation scope was BLOCKING-incomplete. Every file:line DEC-062 cites was re-verified at the origin and HOLDS. Four binding additions:

- **C1 (BLOCKING): `home` and `program_name` must be OPTIONAL.** `PyProcessBootstrap.h:59-62`, `.cpp:79-100` — when the locator selects no home, do NOT call `PyConfig_SetString(&config.home, <empty>)`; that destroys CPython's normal PATH/inherited-environment discovery. Check the `PyStatus` of every `PyConfig_SetString`, `PyConfig_Clear`, and return a failed `Result` BEFORE `PyConfig_Read`.
- **C2 (BLOCKING): `PythonEmbed::pythonInstalled()` cannot simply be deleted.** `PythonEmbed.h:85` is called by the settings UI at `src/Gui/Pages.cpp:428-440`. Keep it as a forwarding wrapper to the locator, or add `Pages.cpp` to the change set. Full extraction as written is a compile failure.
- **C3 (BLOCKING): the new locator compiles in the OR-bootstrap block of BOTH build systems** — `src/CMakeLists.txt:1428-1435` and `src/src.pro:326-347`. Wiring it only into CMake's `GC_SOURCES_PYTHON` / qmake's Python block leaves Garmin-only builds with unresolved locator calls. Wired there and after DEC-063, no CMake-configure, qmake or link failure remains.
- **C4 (ordering): DEC-063 is scope-passed but NOT landed** — `src/CMakeLists.txt:1085` still defines `GC_HAVE_PYTHON` live, so DEC-063 must be implemented first. Separately, the earlier worry about a conflict with B-STAGE9-38 unit u3 is REFUTED: u3's live edits are in `src/Cloud/PyEmbeddedAdapter.cpp` AFTER initialization and do not touch interpreter startup. No ordering constraint between them.
- **`Py_SetProgramName` call sites, exhaustive:** `PyProcessBootstrap.cpp:140` (the only real deprecated call; reached by shipped qmake/CMake builds when `PythonEmbed` is constructed), `PythonEmbed.cpp:269` (wrapper caller, same shipped path), and `unittests/Core/garminconnect/testPythonProgramNameLifetime.cpp:114,133,153` with `CMakeLists.txt:2575-2625` (test-only interposer). Removing the wrapper without removing or replacing that target is a compile failure. No other live direct calls.
- **Minimal change set:** new `src/Python/PythonDeploymentLocator.{h,cpp}`; `src/Python/PyProcessBootstrap.{h,cpp}`; `src/Python/PythonEmbed.cpp`; `src/Core/main.cpp`; `src/CMakeLists.txt`; `src/src.pro`; `unittests/Core/garminconnect/CMakeLists.txt`; delete-or-replace `unittests/Core/garminconnect/testPythonProgramNameLifetime.cpp`.

## DEC-063 — B-STAGE9-41 remedy: CMake defines `GC_WANT_PYTHON` target-locally; the unconsumed `GC_HAVE_PYTHON` spelling is deleted

- Status: accepted (Inspector `garmin_inspector_v1_43`, Three-Options Doctrine — ordinary build-system drift repair, not a human-in-the-loop gate; the investigator's central claim was re-derived from source by the Inspector before recording, not relayed)
- Reversibility: high — one macro name, one scope change, in one CMake block. No source file changes its guards.
- Decided / last-reviewed: 2026-09-20
- Serves: B-STAGE9-41 (blocking). Precondition for DEC-062's Amendment-2 constraint 5, which cannot prove "both configurations reach the same home" while a CMake Python-only build reaches neither initialiser
- Dependents: `src/CMakeLists.txt:1056-1085`, DEC-062's measured same-home claim, any future CMake Python-on build leg
- Origin: `s925_tz_investigator` (unit `B-STAGE9-41-options`) scored three options; the Inspector verified the census and found an in-file precedent the report did not cite

### Binding constraints from the scope pass (`DEC-063-scope`, `garmin_codex_reviewer`, 2026-09-20)

Added before any builder was dispatched. Two of these are BLOCKING and the first would have broken the build outright:

- **Constraint 1 (BLOCKING): the target does not exist yet at `:1085`.** Rewriting `add_definitions()` in place as `target_compile_definitions(GoldenCheetah …)` there causes a CMake CONFIGURE failure, because `add_executable(GoldenCheetah …)` is not reached until `:1398`. The change is therefore two edits, not one: DELETE `:1085`, and ADD the target-local definition after `:1398`.
- **Constraint 2 (BLOCKING, validation wording): "whole tree" is the wrong acceptance criterion.** `GC_HAVE_PYTHON` survives in historical `STATE.md` / `findings.md` / `decisions.md` prose — including this entry — so an acceptance check demanding zero repository-wide text hits fails for the wrong reason. Scope the check to LIVE BUILD INPUTS (`.cpp/.h/.pri/.pro/.cmake/.py`, shell and CI YAML), where the count is exactly one: `src/CMakeLists.txt:1085`, with zero readers.
- **Constraint 3: ordering.** DEC-063 must precede DEC-062 (it is what makes DEC-062's same-home claim measurable at all — before it, a CMake Python-only build reaches NEITHER guarded initialiser). It is semantically independent of `B-STAGE9-38-u3` and need not wait for it.
- **Measured, non-blocking, worth keeping:** the current directory-scoped `add_definitions()` leaks `GC_HAVE_PYTHON` into Coach's nine TUs (`src/Coach/CMakeLists.txt:4-28`) when Python and Coach are both on; none reads it. `unittests/` is a top-level sibling of `src/`, so NO unit-test target inherits the define and none relies on it — target-local `GC_WANT_PYTHON` reaches every `GoldenCheetah` TU that needs it.
- **Parity, non-blocking:** `src/src.pro:220-243,312-353` and `src/gcconfig.pri.in:52,65` already define and consume `GC_WANT_PYTHON`, so this NARROWS the two build systems' divergence rather than widening it — which is the point of B-STAGE9-41. `CloudDBChart.cpp` remains absent from CMake's source list, so its guarded code stays unmeasured; not regressed by this change.
- Minimal change set (adopted): `src/CMakeLists.txt`.

### The census this decision rests on

READ, whole tree, Inspector-run — the investigator's count re-derived rather than accepted. `GC_HAVE_PYTHON` has exactly ONE occurrence in the live tree: its own definition at `src/CMakeLists.txt:1085`. The only other hit is `.kilo/worktrees/scented-wood/src/CMakeLists.txt:1076`, an unrelated worktree copy, not this tree. There is no `#if`/`#ifdef` consumer of that spelling anywhere in production or tests. Every Python feature guard in `src/` is spelled `GC_WANT_PYTHON` (55 occurrences). So `:1085` defines a macro nothing reads, while the macro 55 sites do read is never defined under CMake at all — the block selects sources on the requested flag and then emits a different name.

**The precedent the option report missed.** This is not a new pattern to introduce; it is the only Python-block deviation from one the file already uses. `src/CMakeLists.txt:1508` reads `target_compile_definitions(GoldenCheetah PRIVATE GC_WANT_GARMINCONNECT)` — the sibling feature defines its own flag, under its own name, target-locally. The Python block at `:1085` is alone in using directory-scoped `add_definitions()` AND a second spelling. Option A is therefore convergence on an existing in-file convention, which raises its maintainability and best-practices scores above what the report assigned on generic grounds.

**qmake has no gap, confirmed.** `GC_WANT_PYTHON` is a `DEFINES +=` entry (`src/gcconfig.pri.in:52,65`), so it reaches the preprocessor under its own name; `src/src.pro:220,332` consume it directly. The defect is CMake-only.

### Options (reliability / scalability / maintainability / best practices)

- **Option A — replace `add_definitions(-DGC_HAVE_PYTHON)` with a target-local `GC_WANT_PYTHON` definition; delete the `GC_HAVE_PYTHON` spelling (CHOSEN): 5/5/5/5.** Activates the 47 intended scripting guards across 11 production files under a CMake Python-on build, matches `:1508`'s existing convention exactly, and leaves qmake untouched. Deleting rather than keeping the dead name is what stops the drift recurring: a spelling with no consumer and no definition cannot be mis-selected later.
- **Option B — define `GC_WANT_PYTHON` but retain `GC_HAVE_PYTHON` as a compatibility alias: 5/4/3/3.** Functionally identical today, but it preserves a spelling that has never had a consumer, which means inventing a removal rule and a deprecation window for a macro no code has ever read. The compatibility it buys is with nothing.
- **Option C — canonise `GC_HAVE_PYTHON`: change all 47 production guards and make qmake emit `HAVE` when `WANT` is selected: 4/2/1/1.** Widest possible change for the narrowest defect, and it ends with request/source-selection permanently spelled `WANT` while availability is spelled `HAVE` — the same two-names confusion, made load-bearing instead of removed. Rejected on maintainability and best practices.

### Constraints the build must honour

1. `GC_HAVE_PYTHON` is deleted, not aliased. After this change the string must not appear in the live tree; `.kilo/worktrees/` is out of scope and must not be touched.
2. Target-local, matching `:1508` — not `add_definitions()`. The current directory scope leaks the macro to every target defined below it in the same directory, which is a second, separate defect in the same line.
3. Behaviour under `GC_WANT_PYTHON=OFF` must be byte-identical: no macro defined, no sources selected. Only the ON leg changes.
4. Buildability of a CMake Python-on configuration is UNMEASURED (no CI workflow invokes it; `unittests/buildguard/garmin_flag_build_guard.sh:51-74` pins Python OFF). Activating 47 guards that have never been compiled under CMake may surface pre-existing compile errors. Those are this repair's findings to report, not a reason to withhold the fix — but the unit must actually configure and build that leg rather than asserting the macro's presence and stopping.
5. The existing flag guard's ON/OFF Garmin legs are semantically unchanged by this (it pins Python OFF), so its passing state is not evidence for this repair and must not be cited as such.

### Cascade impact

`findings.md`'s B-STAGE9-41 row moves from open-undecided to decided-not-built. DEC-062's Amendment-2 constraint 5 stops being a blocker once this lands and becomes an ordinary sequencing dependency. No production source file changes: the guards already read the surviving name. `WIKI.md`'s DEC registry advances to `next:garmin-064`.

## DEC-064 — B-STAGE9-36 remedy: the Arms authorization becomes a structured record, not a Markdown inference

- Status: **ACCEPTED 2026-09-20 — Option C** (`garmin_inspector_v1_45`). Supersedes DEC-060's loose-recognizer doctrine for the Arms field only. Accepted on the Inspector's own measurement (see "Why Option C", below), not on any agent's recommendation — the independent second opinion DISSENTED and the adjudicating reviewer is this DEC's own drafting agent, so neither could carry it.
- Constraint 1: the sentinel REPLACES DEC-054's declaration, and the old bullet is removed in the same change. It must not mirror it — a sentinel that duplicates a live prose bullet recreates exactly the drift Option B was rejected for. **Line numbers corrected 2026-09-20 after `B-STAGE9-36-optionC-scope`, verified at the origin by the Inspector:** the live declaration is `decisions.md:2647` (`:2646` is the `Origin:` line — the wrong cite came in through this entry's own Dependents line), its `- Arms-note:` companion is `:2648`, the `## DEC-054` heading is `:2639`, and the sentinel slot is the blank line at `:2640`. `:2641`, not `:2640`, is the `- Status:` line used throughout this entry as the legitimate-metadata example; the example is unaffected, the cite was off by one.
- Constraint 2: the sentinel is validated in its fixed slot only — immediately after the resolved DEC heading. No scan of later bullets, at any width. Absent, malformed or id-mismatched in that slot is an error, never a skip.
- Constraint 3: `_is_arms_loose_shaped` (sole call site `:1096`) and `_strip_arms_leading_run` (sole call site `:1036`) are DELETED with the doctrine they implement, not widened. DEC-060's pinned `Lo`/`So` accepted false negatives become moot rather than tolerated.
- Constraint 4 (added 2026-09-20 from the scope pass): `ArmsBulletMalformed` is NOT deleted. It is raised at `:1101/:1110/:1121/:1131` and converted to `LINT-EMPTY-BOGUS-DEC` at `:1200`; it remains the error transport for a malformed sentinel. Deleting it drops the malformed-record contract silently — the exact shape this whole finding is about.
- Constraint 5 (added 2026-09-20 from the scope pass): the sentinel is read as the RAW `entry[1]` returned by `_resolve_dec_entry()` (`:956-985`), regardless of that tuple's `masked=True` flag — an HTML comment necessarily masks itself, so rejecting masked entries rejects every valid sentinel. Anchoring to `- Status:` or to any later block instead would reintroduce prose discovery one level up.
- Constraint 6 (added 2026-09-20 from the scope pass): the `armed is None` / "no Arms bullet" branch (`:1167-1214`) becomes unreachable under constraint 2 and must be deleted or replaced with sentinel-specific error handling, not left dead. `LINT-EMPTY-UNDECLARED`, `LINT-EMPTY-BOGUS-DEC` and `LINT-VACUOUS` all remain valid and are retained.
- Constraint 7: exactly one live `- Arms DEC-…:` declaration exists (`:2647`), so no second migration is owed. `scripts/ledger_drift_lint.py` does not scan `decisions.md` (canonical, out of scope at `:97-103,144-160`), so the sentinel cannot trip it and that script needs no change.

- **Lifetime-test disposition settled (addendum 2026-09-20, `garmin_codex_reviewer` unit `DEC-062-lifetime-test-scope`, adopted by `garmin_inspector_v1_46`): DELETE `unittests/Core/garminconnect/testPythonProgramNameLifetime.cpp` and its registration at `unittests/Core/garminconnect/CMakeLists.txt:2575-2625`.** The Inspector raised this because a lifetime guard vanishing as a side effect of an API retirement is how a use-after-free returns two REQs later; the check cleared it. Deciding observation: the test proves ONLY `Py_SetProgramName`'s legacy BORROWED-pointer contract — it interposes the call at `:114-117`, stores the raw pointer, and dereferences it after the wrapper returns (`:135-138`, `:153-158`), which is why `PyProcessBootstrap::setProgramName()` retains a `std::list<std::wstring>` at `.cpp:126-140`. DEC-062's replacement `PyConfig_SetString` COPIES the wide string, so no caller-owned pointer survives the call; the only retained pointers are CPython-owned copies inside `PyConfig`, valid until `PyConfig_Clear`, which the bootstrap already performs at `.cpp:99-100`. Do NOT rewrite or migrate it: an ASan test of a copying setter cannot exercise a dangling caller buffer, so a replacement would manufacture coverage for a hazard the new API removes.
- Minimal change set (scope pass, adopted): `.claude/workflow-garminconnect/decisions.md`, `unittests/buildguard/garmin_lint_ownership_guard.py`, `unittests/buildguard/test_garmin_lint_ownership_guard.py`. Test replacement is the bulk of it — `test_an_arms_bullet_*` (`:851-1040`), the near-miss set (`:1077-1370`), both Unicode sweeps and the filler pin (`:1416-1512`), and the live-bullet/container tests (`:1613`, `:1724-1768`, `:1816`, `:2063`).
- Reversibility: medium — one new governed file or one relocated declaration, plus the guard's lookup path. No product code.
- Decided / last-reviewed: —
- Serves: B-STAGE9-36 (blocking)
- Dependents: `unittests/buildguard/garmin_lint_ownership_guard.py:900-1210`, `.claude/workflow-garminconnect/decisions.md:2646` (DEC-054's one live declaration), DEC-060
- Origin: `garmin_codex_reviewer` units `B-STAGE9-36-r12-review`, `B-STAGE9-36-r13-adjudicate`, `B-STAGE9-36-remedy-options`

### The problem, restated

The Arms authorization is a machine field discovered by scanning prose. Rounds 9-12 each pinned one character class as the leading-indent rule — ASCII, then `\s`, then Unicode categories `{Zs,Zl,Zp,Cc,Cf}` — and each was beaten by the next class; round 12 GREEN at 712/712 still loses to a leading U+034F (category Mn, default-ignorable, renders blank) and to an interior U+200B that survives `re.sub(r"\s+")`. In every case the line is SILENTLY SKIPPED and a neighbouring valid bullet authorizes the empty root.

The obvious inversion — every bullet-shaped line must strictly parse as Arms or raise — was adjudicated and REJECTED, and the rejection is the useful part: `decisions.md:2640`'s `- Status:` is legitimate metadata, as is every other skip in DEC-054's block except the one real declaration at `:2646`. With `garmin-cpp-stderrbuf` empty, that false malformed would surface downstream as `LINT-EMPTY-BOGUS-DEC` (`garmin_lint_ownership_guard.py:1199-1205`). The discriminator the inversion threw away is that a Markdown bullet is not an Arms declaration — the reserved FIELD NAME is.

### Options drafted (reliability / scalability / maintainability / best practices)
- **Option B — versioned structured sidecar manifest: 5/5/4/5 (drafting agent's recommendation, NOT yet the decision).** New `.claude/workflow-garminconnect/arms.json`, loaded by a strict standard-library JSON parser from `garmin_lint_ownership_guard.py:1141-1210`, mapping each DEC id to its exact glob array; heading resolution is retained only to prove the referenced DEC exists. One declaration moves (DEC-054 `:2646`); historical rationale is amended, not rewritten. Loses a point on maintainability: a second governed file needs an explicit ownership rule or it drifts from the prose it mirrors.
- **Option C — fixed-position embedded sentinel: 5/4/4/4.** Each `pre_armed_by` DEC carries an immediately-following fixed-format ASCII sentinel (`<!-- gc-arms/v1 {...} -->`) validated in that exact slot, never by scanning later bullets. Keeps the ledger self-contained, but its layout becomes schema.
- **Option A — source-resident registry: 4/3/3/3.** An immutable `DEC_ARMED_PATTERNS` map in the guard itself (`:288,1141-1210`), exact membership instead of recognition. Closes the Unicode class but puts machine configuration in executable guard code, and every future authorization needs a code change.
- **Rejected baseline, kept for the record — generic-bullet inversion.** Not viable: see "The problem, restated".

All three make DEC-060's pinned Lo/So accepted false negatives MOOT rather than merely tolerated, and none needs a new ctest registration — each extends `testGarminLintOwnershipGuardUnits` under the existing `garmin-lint-guard` label.

### Independent second opinion — DISSENTS (2026-09-20, `s920_arms_second_opinion`, unit `B-STAGE9-36-second-opinion`, collected by `garmin_inspector_v1_45`)

Verdict: recommends **REPAIR DEC-060, do not supersede**. Its three answers: (1) the "no character class can close this" premise is FALSE — stripping Unicode `Default_Ignorable_Code_Point` both while finding the `[-*+]` marker and while forming the loose keyword view makes leading U+034F and interior U+200B reach the strict grammar loudly, while `decisions.md:2640`'s `- Status:` still yields `status` and stays a legitimate skip (`garmin_lint_ownership_guard.py:1024-1040`); (2) NO to Option B on its own terms — a sidecar is sound only as the single authoritative record, otherwise it relocates the trust boundary and creates prose/sidecar drift (`:1061-1120`); (3) the pro-supersession history does not establish that the reserved field cannot be recognized under that normalization.

Not accepted as decisive, and not dismissed. Two facts verified directly by the Inspector after collecting it, neither addressed by the opinion: `garmin_lint_ownership_guard.py:1022` ALREADY pins `Lo`/`So` blank-rendering fillers as accepted false negatives, and U+3164/U+115F (`Lo`) and U+2800 (`So`) are not default-ignorable — so a DI strip leaves that half of the class open by DEC-060's own admission; and CPython exposes no `Default_Ignorable_Code_Point` accessor (`unicodedata` 15.1.0 has no `property()`), so the rule ships as a hand-maintained table, which is the rounds 9-12 enumeration shape at larger size. The opinion's objection (2) is also aimed at Option B's second file and does not obviously reach Option C, which adds none. Both points are now with `garmin_codex_reviewer` as `B-STAGE9-36-dec064-adjudicate`; accept or reject only on its answer to CLOSES-CLASS vs NARROW.

### Why Option C — the measurement that decided it (`garmin_inspector_v1_45`, 2026-09-20)

The second opinion's repair and the drafting agent's supersession were both settled by running the live guard rather than by argument. `_is_arms_loose_shaped` (`garmin_lint_ownership_guard.py:1035-1040`) was called directly at HEAD on eight inputs, each raw and NFKC-normalized:

| input | loose | NFKC+loose |
|---|---|---|
| `- Arms DEC-054:` (canonical) | True | True |
| leading U+034F (`Mn`) | False | False |
| interior U+200B (`Cf`) | False | False |
| fullwidth `- Ａｒｍｓ DEC-054:` | False | **True** |
| Greek Alpha `- Αrms DEC-054:` | False | **False** |
| leading U+3164 HANGUL FILLER (`Lo`) | False | False |
| leading U+2800 BRAILLE BLANK (`So`) | False | False |
| `- Status:` (legitimate metadata) | False | False |

Greek Alpha is the counterexample that ends the recognizer line: `unicodedata.normalize("NFKC","Α")` is `'Α'`, so the line is skipped under a DI strip AND under NFKC folding, reads to a human as an Arms declaration, and lets a neighbouring bullet authorize. `Lo`/`So` fillers — already pinned at `:1022` as accepted false negatives — are not default-ignorable and survive a DI strip untouched, so the repair closes at most the two inputs the second opinion named. `- Status:` stays False throughout, so the legitimate-skip constraint that killed the generic-bullet inversion is preserved by every candidate and discriminates between none of them.

Scoring (reliability / scalability / maintainability / best practices): **Option C 5/4/4/4** — exact slot, fail-closed, no recognition of prose at any width, and no second authority, so the second opinion's only substantive objection does not reach it. Option B 5/5/4/5 as drafted, but rejected: the dissent's drift argument against a second governed file is accepted. Option A 4/3/3/3 — machine configuration in executable guard code. Repair-DEC-060 — refuted by the table above.

### Cascade impact

DEC-060's loose-recognizer doctrine is superseded FOR THIS FIELD ONLY if this is accepted; rounds 9-12's work is not reclassified, and the round-11 amendment's falsified "closed invisible partition" claim is corrected by this entry rather than by editing its text. `findings.md`'s B-STAGE9-36 row moves from recognizer-bug to design-defect. Nothing is dispatched to the builder until this DEC is accepted.

## DEC-066 — B-STAGE9-45 remedy: module provenance is a C++-owned identity ledger, not an attribute read off the cached module

- Status: **accepted 2026-09-20** (Inspector `garmin_inspector_v1_49`, Three-Options Doctrine — ordinary architecture decision, not a human-in-the-loop gate). Option A as scored, plus constraints 1 and 2 below, which the independent second opinion produced and without which option A does NOT close B-STAGE9-45.
- Reversibility: medium — new C++ state in the adapter plus a replaced predicate; no Python-side or packaging change, and `GC_GARMIN_PYPATH` is untouched.
- Decided / last-reviewed: 2026-09-20
- Serves: B-STAGE9-45 (blocking), B-STAGE9-64 (folded in). Amends DEC-058 c17(b), which remains the governing decision for everything else it says.
- Dependents: `src/Cloud/PyEmbeddedAdapter.cpp:178-257` (`moduleFileIsUnderDir`/`explicitOverrideCacheIsSafe`, both replaced), `unittests/Core/garminconnect/testGarminConnectPyAdapter.cpp:296,1393-1401`, DEC-058 c17(b).
- Origin: `s925_tz_investigator` unit `DEC-066-cache-provenance` (three options, each with an executed CPython 3.13.5 repro); verified by `garmin_codex_reviewer` unit `DEC-066-option-a-check` before this entry was written.

### The problem, restated

Five repair rounds closed five forgeable attributes and each was replaced by the next one. The investigator's repro settles why: `CURRENT_FILE_CHECK_ACCEPTS=True` while `NORMAL_IMPORT_CHILD_ORIGIN=decoy` — the guard reads a real override `__file__` on the parent, and CPython then resolves the absent CHILD through a forged parent `__path__`. Every fact the predicate consults is writable by the code it is trying to judge. A sixth round would narrow the same recognizer, so the remedy changes WHERE the fact is read, not how prose is recognized.

### Options scored (reliability / scalability / maintainability / best practices)
- **Option A — C++ identity/provenance ledger (CHOSEN): 5/5/4/5.** With the GIL held, record `(interpreter, override-dir, full-name, PyObject*)` with a strong reference for every entry produced by the one clean import from the hoisted override dir; later adapter construction accepts only identical pointers. Reads no Python-visible attribute. Repro: `A_POINTER_LEDGER_SAME_OK=True`, `A_POINTER_LEDGER_HOSTILE_REJECTED=True`. Does not defend against trusted override code mutating its own genuine module object, or arbitrary code running during the initial import — both accepted residuals.
- **Option B — opaque private override namespace: 5/3/2/3.** A high-entropy C++-owned package alias via a file-location spec; the conventional cache is never read. Repro: `B_PRIVATE_ALIAS_CHILD_ORIGIN=override`, `B_STANDARD_HOSTILE_UNTOUCHED=True`. Rejected: it buys a private-import contract that every absolute import inside the package must then honour, auditable only by inspection.
- **Option C — C++ retained module handle: 4/4/3/4.** Import only on a verified-empty cache and hand back the retained `PyObject*` thereafter. Repro: `C_NORMAL_SECOND_ORIGIN=decoy`, `C_CPP_RETAINED_HANDLE_ORIGIN=override`. Rejected: it deliberately permits the process cache and the adapter's handle to diverge, which is a second source of truth.

### Constraints the implementation must honour

- **Constraint 1 (from the second opinion, BLOCKING as drafted).** Rejecting only pre-existing UNLEDGERED entries is insufficient. `PyEmbeddedAdapter.cpp:244-257`: an attacker keeps ledgered package P, deletes `sys.modules["gc_garmin_adapter.garmin_client"]`, and points `P.__path__` at a decoy; the pre-import check sees only a matching ledgered P, and `PyImport_ImportModule()` then creates the child from the decoy AFTER the check. The ledger must therefore be validated for exact cache/ledger membership and pointer identity **after** the import returns, not only before it. **VERIFIED 2026-09-20** by `DEC-066-constraint-repro` on a real CPython fixture: `C1_PRE_IMPORT_ONLY_ACCEPTS=True`, `C1_CHILD_LOADED_FROM=decoy`, `C1_POST_IMPORT_EXACT_ACCEPTS=False` — the pre-import-only check accepts and the decoy child loads; the post-import exact check is what rejects it.
- **Constraint 2 (from the second opinion; requirement VERIFIED, rationale AMENDED).** A process-lifetime ledger holding INCREF'd references outlives `Py_FinalizeEx()` (`testGarminConnectPyAdapter.cpp:1393-1401`). The ledger is cleared under that interpreter's GIL BEFORE finalization — every strong reference released and every entry keyed by that interpreter erased — and no ledgered Python object is inspected or decref'd after finalization. **Amended 2026-09-20 on executed evidence** (`DEC-066-constraint-repro`, native CPython 3.13 embedding fixture): the drafted "a later reinitialization may reuse either address" is only half right. Interpreter-address reuse IS real (`INTERP_ADDR_REUSED=1`), which is why the interpreter key cannot survive finalization. Module-address reuse while the ledger still holds its strong reference is REFUTED (`HELD_MODULE_ADDR_REUSED=0` after a million fresh module allocations) — the outstanding reference retains the allocation. A RELEASED module's address, by contrast, was reused immediately by a distinct module object in the same interpreter (`SAME_CYCLE_MODULE_REUSED=1`), so the hazard belongs to any entry the ledger drops, not to one it still owns.
- **Constraint 3.** No eviction of `sys.modules` and no blanket rejection of a cached entry — both remain settled from DEC-058 c17(b); a genuine second adapter construction over the same override must still succeed.
- **Constraint 4.** The ledger is only reached with the GIL held. Confirmed already true of all six current import paths (`PyEmbeddedAdapter.cpp:699,799,868,926,1011,1054`, each behind `GilGuard`); the rule is stated so a seventh path cannot be added without it.
- **Constraint 5.** The package currently imports its sole sibling (`gc_rate`) eagerly, so there is no live lazy-submodule case — but any future lazy `gc_garmin_adapter.*` import needs its own recording/validation rule, or it arrives unledgered and is rejected.

### Cascade impact

`moduleFileIsUnderDir` is replaced, not narrowed; B-STAGE9-45's findings row moves from recognizer-bug to design-defect, and B-STAGE9-64 closes with it. `testGarminConnectPyAdapter.cpp:296` gains the forged-`__path__` case, the legitimate-second-construction case, and a finalization case for constraint 2. DEC-058 c17(b) is amended to define provenance as C++-recorded identity; the rest of DEC-058 is unchanged.

## DEC-067 — the Linux smoke assert gets one last strengthening, then its residual class is pinned

- Status: **accepted 2026-09-20** (Inspector `garmin_inspector_v1_49`). This is the repair-round bound firing at 3 consecutive SAME-class rounds, taken as the skill's option (b) with the round's own terminal fix folded in — not a fourth repair round.
- Reversibility: high — three lines in `appveyor.yml`, no product code.
- Decided / last-reviewed: 2026-09-20
- Serves: B-STAGE9-59, DEC-058 C11/C15.
- Dependents: `appveyor.yml:257-259`; any future "the Linux leg proves the payload" claim.
- Origin: reviewer units `B-STAGE9-59-review`, `B-STAGE9-59-r2-scope`, `B-STAGE9-59-r2-review` — three rounds, each closing the named check and naming the next weaker one.

### Why this stops here

Rounds 1-3 ran the same shape: the assert proves something weaker than "the payload works," the reviewer names a bundle that satisfies it anyway, the assert gets one conjunct stronger. Round 3's case is `Garmin = None` plus non-exception attributes passing `hasattr`. Each strengthening has been correct, and each has been followed by a weaker-still hypothetical, because the only check that cannot be satisfied by a wrong-but-shaped bundle is actually CALLING the Garmin API — which needs credentials and network and is forbidden in a CI smoke step by DEC-065's own scope. The class is therefore not closable by a stronger static assert, and a fourth round would narrow the same recognizer.

### What is built (the terminal strengthening)

`appveyor.yml:258`'s assert replaces attribute-presence with usability: `callable(_gc.Garmin)`, and each of `GarminConnectAuthenticationError` / `GarminConnectConnectionError` / `GarminConnectTooManyRequestsError` must be a `type` and a subclass of `BaseException`. Everything else from the round-2 remedy is unchanged.

### What is pinned (the accepted false negative)

A bundle that ships a `garminconnect` satisfying every conjunct above and still fails at runtime passes this smoke test. That is an ACCEPTED residual, on the same footing as the shellcheck declared-gap and DEC-060's Lo/So fillers. It is detected instead by the live-account acceptance criterion Stage 9 already owns — which is where "the payload actually works" has always belonged. **No further B-STAGE9-59 repair round is authorized.** A reviewer finding of this class is recorded against this DEC and closed, not dispatched.

### Cascade impact

B-STAGE9-59 closes on the strengthened assert plus this pin, not on a clean reviewer pass. Stage 9's live-account criterion absorbs the residual; no other row changes.

### Scope-pass addendum (`DEC-067-scope`, reviewer, 2026-09-20 — binding on the builder)

Pre-scoped before the edit was written. No blocking hazard; five NON-BLOCKING shape constraints, all on `appveyor.yml:258`: one physical line (a preserved YAML newline in a `>-` fold changes the parsed source); double-quoted Python literals only (a literal `'` terminates the enclosing `sh` single-quoted argument); a missing `m._gc.Garmin` or `m._gc.exceptions` raises AttributeError rather than setting `ok = False`, which is fail-closed through `|| exit 1` and is accepted; and `issubclass` needs an `isinstance(x, type)` guard or a non-class raises TypeError.

The builder writes this exact source, unchanged — 522 chars, 0 single quotes, 0 newlines:

`import gc_garmin_adapter.garmin_client as m, os, sys; r = os.path.realpath("squashfs-root") + os.sep; ok = os.path.realpath(m.__file__).startswith(r) and m._gc is not None and m._gc.__name__ == "garminconnect" and os.path.realpath(m._gc.__file__).startswith(r) and callable(m._gc.Garmin) and all(isinstance(x := getattr(m._gc.exceptions, e), type) and issubclass(x, BaseException) for e in ("GarminConnectAuthenticationError", "GarminConnectConnectionError", "GarminConnectTooManyRequestsError")); sys.exit(0 if ok else 1)`

Inspector-verified independently, not accepted on the reviewer's word: `compile()` clean, and the exception conjunct executed against four fixtures — real exception classes `True`, a non-class value `False` (no TypeError), a real class that is not a `BaseException` subclass `False`, a missing name AttributeError (fail-closed). The walrus needs Python 3.8+; the bundled interpreter is 3.11.16 per B-STAGE9-42's pin.

## DEC-068 — B-STAGE9-66's premise is refuted; the returned-object compare stays as a tripwire, and the CPython invariant it rests on becomes a test

- Status: **accepted 2026-09-24** (Inspector `garmin_inspector_v1_51`, Three-Options Doctrine — ordinary engineering judgement on dead code, not a human-in-the-loop gate).
- Reversibility: high — one comment and one test; the product branch itself is unchanged either way.
- Decided / last-reviewed: 2026-09-24
- Serves: B-STAGE9-66, B-STAGE9-67, DEC-066 constraint 1.
- Dependents: `src/Cloud/PyEmbeddedAdapter.cpp:346-362`; any future change to how the adapter imports its module.

### What was established, and by whom

Three independent sources, two of which never saw the other's reasoning, agree that B-STAGE9-66's defect is unreachable:

- `garmin_builder_stage9_v28` (unit `DEC-066-impl-r3`) ran the mutation this Inspector specified — neutralising the `:353-362` compare — and its own new forgery test STAYED GREEN. It reported that against its own interest rather than claiming the fix worked.
- `garmin_codex_reviewer` (unit `DEC-066-impl-r3-review`, round 2, fresh context) returned CLOSED and retracted its own round-1 finding, citing CPython 3.13.5 `import.c`.
- `s925_tz_investigator` (unit `cpython-import-return-binding`) was given ONLY the narrow CPython question — not our adapter, not our conclusion, and explicitly told that "yes it can differ" was as welcome as "no it cannot". It probed `libpython3.13.so.1.0` through a C embedding fixture AND read the source, and reported: hook returns X without writing `sys.modules` → **NULL, KeyError**; hook writes decoy D and returns something else → **D**; hook writes D and returns D → **D**. Probe and source agreed with no exceptions.

Mechanism: `PyImport_Import` calls the `__import__` hook, immediately `Py_DECREF`s its result, then calls `import_get_module(tstate, module_name)`, which resolves the full dotted name in `sys.modules`. The object `PyImport_ImportModule` hands back is therefore always `sys.modules[name]` or NULL — never the hook's own return value. A NULL is caught by the existing `if (module == nullptr)` guard one line above the post-check, so `:353-362` is reached only when `module` is exactly the object the post-import scan already vetted.

### Options scored

1. **Delete `:353-362`.** Reliability neutral (it guards nothing today); maintainability +; best practices +. Rejected because it silently discards the tripwire: the binding guarantee is a property of *this* import call, and a future switch to `fromlist`, `importlib`, or `PyImport_ImportModuleLevelObject` would void it with nothing to notice.
2. **Keep it unchanged.** Rejected outright. Its comment asserts the compare is "a free, load-bearing assertion" — a checkable claim about behaviour that is now known to be FALSE, in a codebase whose standing rule is that no comment may state a checkable fact about behaviour. It would also leave a permanently untestable branch in a module whose entire standard is mutation-proved tests.
3. **Keep the branch, tell the truth about it, and make the invariant it rests on executable.** CHOSEN. Reliability + (the real risk — the invariant changing — becomes a RED test instead of an unstated assumption); maintainability + (the comment stops lying and cites this DEC); best practices + (converts an untestable claim into an assertion, which is exactly what the standing rule demands).

### What is built

- The `:346-353` comment is rewritten: the compare is REDUNDANT under CPython's current `PyImport_Import` semantics, retained as a tripwire, with the reason cited as DEC-068 rather than restated.
- A new test pins the INVARIANT, not the branch: with a hostile `builtins.__import__` that writes a decoy into `sys.modules` and returns a DIFFERENT object, `PyImport_ImportModule` must hand back the `sys.modules` object. If a future CPython or a future change to our own import call breaks that, this test goes RED and this DEC is revisited.

### What is pinned (the accepted residual)

The `:353-362` branch itself has no RED test and cannot be given one while the invariant holds — proven, not assumed. That is an ACCEPTED untestable branch, on the same footing as DEC-067's pinned false negative. **No repair round on "the `:353-362` branch is untested" is authorized**; a finding of that class is recorded against this DEC and closed. B-STAGE9-66's cold-cache first-import residual is unaffected and remains DEC-066's own accepted residual.

### Cascade impact

B-STAGE9-66 closes as premise-refuted, not as fixed — no code repaired it, because nothing was broken. B-STAGE9-67 (the new test names a gate it does not reach) closes on the rename plus the invariant test. DEC-066 constraint 1 is unchanged and still met by the post-import scan.

## DEC-069 — B-STAGE9-71 remedy: the Stage 9 payload assertion covers all three shipped legs

- Status: **accepted 2026-09-24** (USER decision. A CI modification and a question of how much
  evidence is enough before three blocking findings may close — cost/schedule, not a pillar-scored
  technical call, so it was the user's to make, not the Inspector's.)
- Reversibility: high — additions to two existing `appveyor.yml` test_script arms; no product code,
  no new build step on either leg.
- Decided / last-reviewed: 2026-09-24
- Serves: B-STAGE9-71 (blocking), and gates the honest closure of B-STAGE9-48, B-STAGE9-54,
  B-STAGE9-57. Extends DEC-067, which defined the assertion's predicates; DEC-067 is unchanged.
  Sits under DEC-065, which already settled that installer evidence is earned on AppVeyor.
- Dependents: `appveyor.yml` Windows `test_script` arm and macOS arm; partition commit 7.
- Origin: Inspector `garmin_inspector_v1_53` found the gap while pre-flighting commit 7;
  `s925_tz_investigator` unit `B-STAGE9-71-feasibility` established per-leg feasibility and patch
  shape from the recipe sources.

### The problem, restated

DEC-067's assertion lives at `appveyor.yml:258`, inside the `if $CI_LINUX;` guard, while `:8-11`
ships three images. A green run proved the Garmin payload on one of three platforms. Closing three
installer findings on it would have repeated STAGE-9-CURSOR-AMENDMENT-3 (d)'s recorded error — one
evidence source treated as sufficient for a whole stage — transposed onto the platform axis.

### Options put to the user (cost, not correctness — all four were honest)

- **Both legs (CHOSEN).** ~3 commands per leg, no new build step. All three platforms proven, so the
  three installer findings can close on evidence that matches what they claim.
- Windows only. Half the cost and half the untestable-locally risk; macOS stays unproven.
- Defer both, findings stay open. Nothing overclaimed, push not delayed, Stage 9 does not close.
- Defer both, close on Linux. Fastest; rejected — it records three installer findings as closed on
  evidence covering one platform.

### What the implementation must honour

- **Assert against the PRODUCED artifact, never its staging tree.** Windows: extract or silently
  install `GoldenCheetah_v3.8_x64.exe` and run the `python.exe` it installed. macOS: mount
  `GoldenCheetah_v3.8_x64.dmg` and run the bundled interpreter, provenance rooted at the mount, then
  detach. A `src\release`-only or staging-`.app`-only check proves staging input, not shipped output,
  and does not discharge this DEC.
- **Reuse the predicates, do not restate them.** DEC-067's `:258` payload is the definition of what
  "a usable payload" means; the new arms assert the same thing against their own interpreter.
- **The stated limit is carried, not quietly dropped.** On all three legs this proves the installer
  payload contains a usable adapter and its dependencies. It does NOT prove GoldenCheetah's own
  embedded-startup path. B-STAGE9-48/-54/-57 close against the former only.
- **Neither new assertion is executable on the development host.** A mistake surfaces only as a failed
  CI run costing another push cycle, so both arms are reviewed as unrunnable code — read, not tested.

### Cascade impact

Partition commit 7 grows from 4 hunks to 4 + 2 arms and must be re-pre-flighted before the push.
B-STAGE9-71 closes when the arms are written; B-STAGE9-48/-54/-57 close on the resulting green run,
against the payload claim only. DEC-067 and DEC-065 are unchanged.

## DEC-070 — B-STAGE9-78 remedy: stage under the payload's true extension, don't re-implement unzip

- Status: **accepted 2026-09-26** (Inspector, Three-Options Doctrine. Garmin-side product code
  with no credential, external, or irreversible element — an ordinary technical call.)
- Reversibility: high — the staged filename and one call-site signature; no persisted schema
  change (`imported-<uid>.json` already stores `localFilename` verbatim, whatever it is).
- Decided / last-reviewed: 2026-09-26
- Serves: B-STAGE9-78 (blocking). Adjacent to but NOT a fix for B-STAGE9-79, which is the
  download-vs-import record split and stays a separate unit.
- Dependents: `src/Cloud/GarminBackfillController.{h,cpp}` (`stagedFitPath`, the staging write
  and the `onProgress` hand-off), `src/Cloud/GarminBackfillDialog.cpp:190` (the read side that
  builds the wizard's file list).
- Origin: B-STAGE9-78, from the 2026-09-26 live run. Mechanism confirmed by the Inspector
  reading `src/Gui/RideImportWizard.cpp:443-447` directly.

### The problem, restated

Garmin's download-activity endpoint returns a ZIP containing `<activityId>_ACTIVITY.fit`.
`GarminBackfillController.cpp:282-284` writes those bytes verbatim to a `.fit` filename.
`RideImportWizard` DOES unpack archives — `expandFiles()` at `:443` matches `^(zip|gzip)$`
against `QFileInfo(file).suffix()` and extracts via `Archive::dir`/`Archive::extract` — but it
dispatches on the SUFFIX. A zip named `.fit` therefore skips expansion, passes the `suffixes()`
type check as a FIT, and reaches `openRideFile` as an unparseable blob. The import layer was
never missing; the filename lied to it.

### Options scored

- **A — true extension + existing wizard route (CHOSEN).** Sniff the leading bytes, write `.zip`
  on `PK\x03\x04`, `.fit` otherwise. Reliability: highest — GC's shipped unzip path is reused,
  so there is exactly one implementation of "unpack an archive". Scalability: covers `gzip` for
  free, since the same wizard table already lists it. Maintainability: a magic-byte compare, no
  archive logic in the controller. Cost: the staged path stops being derivable from the activity
  id alone, so the read side has to be told it.
- B — unzip inside the controller, stage the inner `*_ACTIVITY.fit`. Rejected: it creates a
  SECOND unzip implementation beside `Archive`'s, and must grow its own empty-zip/multi-entry
  handling. That is the two-copies-of-one-predicate failure this register's CLV Check 5 note
  records, transposed onto archive handling. Its only advantage is keeping `stagedFitPath`'s
  current shape.
- C — make `RideImportWizard` dispatch on content instead of suffix. Rejected on blast radius:
  it changes every import path in GoldenCheetah, and `GarminBackfillDialog.cpp:255-263` records
  the standing constraint that the wizard is pre-existing shared code not to be modified for
  this dialog.

### What the implementation must honour

- **Sniff, do not assume.** A bare FIT must still stage as `.fit`; the extension follows the
  bytes, never a hardcoded expectation about what Garmin returns.
- **One source for the staged path.** `GarminBackfillDialog.cpp:190` must not guess the
  extension. The controller knows what it wrote and reports it; a glob or a second sniff on the
  read side reintroduces the same split-truth defect at a new place.
- **DES-006 and DES-009 are unchanged.** The torn-write pause and the record-then-progress order
  at `:294-322` stay exactly as they are; only the filename and its propagation move.
- **The two already-staged live files are the regression fixture.** `garmin-24502154112.fit` /
  `garmin-24502696266.fit` under the athlete's `garminconnect/backfill/` are real ZIP payloads
  from a real account and must not be deleted or rewritten.

### Cascade impact

B-STAGE9-78 closes on a green delta-check plus a live re-run that actually raises the ride
library above 1146. B-STAGE9-79 is untouched and becomes the next unit. No REQ/DES row changes:
this repairs REQ-010's implementation against its existing contract rather than altering it.

## DEC-071 — B-STAGE9-79 remedy: two records, and RideCache is the import-completion seam

- Status: **accepted 2026-09-26** (Inspector, Three-Options Doctrine, on the investigator's
  scored report plus a measurement that settled its one open question. Product code, no
  credential or external element — an ordinary technical call.)
- Reversibility: medium — it adds a persisted `pending` record and changes which record the
  skip predicate consults. A migration touches real athlete sidecar files, so the write path
  must be atomic and the legacy shape must stay readable.
- Decided / last-reviewed: 2026-09-26
- Serves: B-STAGE9-79 (blocking). Depends on DEC-070 having landed the staged-path plumbing;
  does not alter it.
- Dependents: `src/Cloud/GarminSidecarStore.{h,cpp}` (schema + writers), `src/Cloud/
  GarminBackfillController.cpp:229-235` (the skip predicate) and `:282-324` (the record
  order), `src/Cloud/GarminBackfillDialog.cpp` (the completion evaluation), `src/Cloud/
  GarminConnect.cpp:874-931` (the incremental-sync route that writes the same two records).
- Origin: B-STAGE9-79 from the 2026-09-26 live run; option report and measurements by
  `s979_record_split_investigator`, units B-STAGE9-79 and B-STAGE9-79-seam.

### The problem, restated

One record is serving two questions. `recordImported()` fires at `GarminBackfillController.cpp
:301`, right after the bytes land — so it means "downloaded" — while `:233`'s
`imported.contains(s.activityId)` reads it as "imported" and skips the id forever. A cancelled
or failed wizard import therefore orphans the activity permanently: staged in a directory no
UI surfaces, and invisible to every future backfill. The name encodes the confusion.

### Chosen: separate the records, and take completion from RideCache

`backfill-state` gains a versioned `pending[id] = {startTimeGMT, local_filename}` manifest and
remains the download/resume record; `imported-<uid>.json` becomes import-completion ONLY; the
skip predicates at `GarminBackfillController.cpp:229-235` and `GarminConnect.cpp:874-879`
consult completion only. Scored 5/5/4/5 (reliability/scalability/maintainability/best
practices) against the two alternatives:

- Adding a `staged|complete` field to the existing imported map (2/4/2/2) — overloads one map
  with two lifecycles and cannot classify the existing three-field entries.
- Re-listing the whole requested range every run and skipping completed ids (4/2/4/3) —
  re-lists up to the five-year cap purely to rediscover pending work, gutting the cursor.

### The completion seam, settled by measurement not assumption

The investigator's report correctly flagged that no import-completion signal exists:
`RideImportWizard::process()` returns while awaiting Save (`src/Gui/RideImportWizard.cpp:
768-805`, Inspector-verified) and the wizard is a plain `QDialog` with no completion signal;
the real import happens later in `abortClicked()` at `:1118`. The seam is instead the
wizard's OWN already-imported test — `rideCache->getRide(ridedatetime.toUTC())` at `:1050` —
which needs no change to shared code and so respects DEC-070's freeze on that file.

That seam is only sound if the timestamps agree exactly, because `RideCache::getRide(QDateTime)`
(`src/Core/RideCache.cpp:807-813`) is a bare `item->dateTime == dateTime` compare with no
tolerance. A one-second drift would re-offer an activity forever — the same orphan reversed.
Measured on the three live sidecar entries, all three match to the second:

- `24344154258` (`.tcx`, the one that DID import): sidecar `startTimeGMT 2026-09-13 10:33:17`
  == the imported ride's `RIDE.STARTTIME` `2026/09/13 10:33:17 UTC`.
- `24502154112` and `24502696266` (the two ZIP fixtures): the inner `*_ACTIVITY.fit`'s session
  field 2 and first record field 253 both decode to `2026-09-26 09:10:43` / `09:54:01` UTC,
  exactly matching their sidecar `startTimeGMT`.

So: no FIT pre-parse and no tolerance window is justified by evidence, and neither may be added
without new measurement. Implement the exact seam.

### What the implementation must honour

- **Evaluate completion after the wizard finishes, per activity**, via
  `getRide(startTimeGMT.toUTC())`. A `pending` entry whose timestamp has no RideCache match
  stays pending and is re-offered; one that matches is recorded complete and dropped from
  pending. Do NOT use `process()`'s return as completion.
- **Migration self-classifies; it does not blanket re-offer.** The measurement above shows the
  seam separates the imported `.tcx` from the two never-imported ZIPs correctly, so legacy
  entries are classified by evaluating each against RideCache once. The investigator's earlier
  conservative "re-offer all legacy entries" is superseded by that measurement.
- **Crash order is pending-then-cursor, atomically, before progress is reported.** A crash
  between staging and completion must leave a `pending` entry, which is safely re-offered.
  DES-009's resumability contract at `:294-296` and DES-006's torn-write pause are preserved,
  not relaxed; do NOT fix this by moving `recordImported` after the wizard.
- **Both routes, not just backfill.** `GarminConnect.cpp:906-931` writes the same two records
  on the incremental path and must follow the same split, or the defect survives there.
- **Rename what the split reveals.** `recordImported` means "downloaded"; the new split is the
  moment to make each name state what its record actually holds (see B-STAGE9-85).

### Cascade impact

REQ-008/010/016; DES-006/009/010; TEST-045/046/048, T-176–T-189, T-194–T-196 need review for
the changed predicate, plus new migration and crash-order tests. B-STAGE9-79 closes on a green
delta-check AND a live re-run in which a cancelled import is successfully retried from the UI.
B-STAGE9-84's orphaned-staging-artifact edge is in this DEC's blast radius and should be
reconsidered here rather than separately.

## DEC-072 — B-STAGE9-83 remedy: inflate gzip at stage time; there is no gzip route to reuse

- Status: **accepted 2026-09-26** (Inspector, Three-Options Doctrine. Garmin-side product code,
  no credential/external/irreversible element — an ordinary technical call.)
- Reversibility: high — one private helper plus one branch in `stagedPayloadPath`'s caller; no
  persisted schema change, no shared-code edit.
- Decided / last-reviewed: 2026-09-26
- Serves: B-STAGE9-83 (blocking), B-STAGE9-85 (non-blocking naming). Amends DEC-070; does not
  supersede it — the ZIP half of DEC-070 is proven correct and stays.
- Dependents: `src/Cloud/GarminBackfillController.{h,cpp}` only.
- Origin: `garmin_codex_reviewer`'s B-STAGE9-78-r2 delta-check, mechanism re-verified by the
  Inspector reading `src/FileIO/ArchiveFile.cpp:62-65` and `src/Gui/RideImportWizard.cpp:450-455`
  directly.

### The problem, restated

DEC-070 claimed option A "covers `gzip` for free, since the same wizard table already lists it".
That premise is false. `Archive::dir()` recognises the `gz`/`gzip` suffixes and then runs
`case GZIP: { }` — an empty block — so it returns zero entries; `Archive::extract` has no GZIP
arm at all. `RideImportWizard::expandFiles()` reads that empty list and takes its
`if (contents.count() == 0) expanded << file` branch, handing the still-compressed blob to the
importer. Round 2's `.gzip` suffix is therefore routed and never expanded: for a gzip payload the
original B-STAGE9-78 failure survives its own fix. The wizard's suffix table advertises a
capability the archive layer does not implement.

### Options scored

- **A — inflate in the controller, then re-sniff and stage the INNER payload (CHOSEN).** Reliability:
  highest, and the only option whose fix is provable from the unit gate — the expansion happens in
  the code under test, so an assertion can require staged bytes to be the decompressed payload
  rather than trusting a shared route. Scalability: re-sniffing means gzip-of-zip stages `.zip` and
  reaches the working ZIP route, so nesting costs nothing extra. Maintainability: zlib's gzip window
  (`inflateInit2(&strm, 15 + 16)`) is already the in-tree idiom at `src/Cloud/CloudService.cpp:565`
  and `src/FileIO/RideFile.cpp:819`, and zlib is already linked in BOTH build systems
  (`src/gcconfig.pri:15` + `src/src.pro:87`; `ZLIB::ZLIB` in the garmin unittest targets) — no
  build-system change, so `garmin-build-system-duality` cannot bite here.
- B — implement the GZIP arms in `Archive::dir`/`Archive::extract`. Scores highest on scalability
  alone: the empty arm is a real pre-existing GoldenCheetah defect for every user who drops a `.gz`
  file, not a Garmin one. Rejected on blast radius (shared import code for all providers, under
  DEC-070's own freeze) and on provability: `ArchiveFile.cpp` is not linked into the garmin test
  target, so the blocking defect could not be shown closed by this project's gate. Recorded as
  upstream-worthy, deliberately out of scope for this branch.
- C — pin-DEC: declare gzip unreachable (both live payloads were ZIP) and delete the branch. This
  was `garmin_inspector_v1_56`'s parting recommendation in B-STAGE9-86's disposition, on three
  grounds. Two do not survive: its "both suffixes fail identically" ground is true only while
  nothing inflates — option A makes `.fit`-with-inflated-bytes import successfully — and its
  "`ArchiveFile.cpp` is frozen shared code" ground argues FOR a controller-side fix, which option A
  is. Its first ground (no local evidence Garmin returns gzip) is accepted as true and makes this
  low-priority, not safe to leave: with the branch deleted, gzip bytes stage as `.fit`, which is
  B-STAGE9-78 verbatim. A pin here buys a silent trap for one helper's worth of saving.
- **Accepted cost of A, stated up front.** This is a THIRD copy of the `gUncompress` shape
  (`CloudService.cpp:565`, `RideFile.cpp:819`, now the controller). Real duplication, accepted
  because the only de-duplicating alternative is a new shared header — shared-code surgery on a
  blocking checkpoint, for a 40-line function the tree already keeps two copies of. A future
  consolidation is upstream work, not this branch's.

### What the implementation must honour

- **Sniff the INFLATED bytes, then stage.** Extension follows the bytes at every level: gzip-of-FIT
  → `.fit`, gzip-of-zip → `.zip`. No hardcoded expectation about what Garmin returns.
- **A failed inflate is visible, never fabricated.** Empty/short inflate output → stage the original
  bytes verbatim under `.gzip` and let the import fail loudly. Do not invent a `.fit` name for
  bytes that were never proven to be FIT — that is the defect class itself.
- **DES-006 / DES-009 unchanged.** The torn-write pause and record-then-report order stay exactly
  as they are; only the bytes chosen for staging and their suffix move.
- **No second ZIP implementation.** DEC-070's option-B rejection still binds for ZIP: `ZipReader`
  via `Archive` is a real shipped route and stays the only unzip. This DEC is narrow to gzip
  precisely because gzip has no such route.
- **Naming closes with it (B-STAGE9-85).** Comments and identifiers still saying "FIT bytes" /
  "staged FIT" on paths that now hold arbitrary payloads are corrected in the same round.

### Cascade impact

B-STAGE9-83 and B-STAGE9-85 close on a reviewer delta-check plus the canonical 56-test gate;
B-STAGE9-78's own closure condition is unchanged and still requires the live re-run that raises the
ride library above 1146. No REQ/DES row changes. The empty `Archive::dir` GZIP arm remains a filed
upstream defect, owned by no unit on this branch.

## DEC-073 — B-STAGE9-86 remedy: an undecodable payload is a download FAILURE, not a staging guess

- Status: **accepted 2026-09-26** (Inspector, Three-Options Doctrine. Discharges the repair-round
  cap at 3/3 as remedy (a), an architectural change to WHERE the fact comes from — not round 4 of
  the same recognizer. Garmin-side product code; ordinary technical call.)
- Reversibility: high — one helper's exit condition, one new PauseReason, no persisted schema change.
- Decided / last-reviewed: 2026-09-26
- Serves: B-STAGE9-86, B-STAGE9-89, B-STAGE9-90, B-STAGE9-91. Amends DEC-072, keeps its inflate.
- Dependents: `src/Cloud/GarminBackfillController.{h,cpp}` only.
- Origin: `garmin_codex_reviewer`'s B-STAGE9-78-r3 delta-check (3 blocking mechanisms), on a tree
  that passed the canonical 56-test gate 56/56 — the THIRD consecutive green gate to miss a blocker.

### Why this is not another round of the same fix

Rounds 1-3 each asked the same question — "what shape is this payload, so what extension do I
guess?" — and each round the answer had one more case: zip, gzip, gzip-under-a-failed-inflate,
gzip-of-gzip, concatenated members, a truncated member, magic bytes that lie. That surface is
open-ended by construction: it enumerates what a payload MIGHT be, and enumeration never closes.
The cap exists to stop exactly this. So the predicate is inverted and the fact-source moves:

**The controller no longer guesses a shape. It either produces bytes the import route provably
handles, or it declares the download failed.** Fail-closed is already this project's idiom
(REQ-017 clause a, DES-006's torn-write pause, the `sessionStillValid` pair); this puts the payload
path under the same rule instead of letting it stage hopefully and fail at the parser.

### Options scored

- **A — fail closed on any payload not resolvable to a handled shape (CHOSEN).** Reliability:
  highest and, critically, CLOSED — the accept-set is exactly two shapes (a complete single-member
  gzip whose output is FIT-or-ZIP, or a bare FIT/ZIP), and every other input, named or unnamed
  today, takes the same refusal branch. Truncation, concatenation, gzip-of-gzip and lying magic all
  collapse into one already-tested path rather than four new recognizers. Scalability: a new exotic
  shape needs NO code change — it is refused by default. Maintainability: deletes the question
  "which extension do I guess for this?" instead of answering it again.
- B — keep guessing, add `Z_STREAM_END` + `avail_in` + a nesting check. Rejected: this is round 4.
  It fixes the three mechanisms the reviewer happened to find and leaves the enumeration open for
  round 5; the cap is a judgement that this trade has already been made three times.
- C — pin the residual as an accepted gap (remedy (b)). Rejected on evidence: unlike the
  shellcheck/DEC-060 precedents, this surface is fully DETERMINISTIC — zlib reports stream
  completion (`Z_STREAM_END`) and unconsumed input (`avail_in`) exactly, so a closed predicate is
  available for the writing. A pin is for surfaces where no deterministic recognizer exists; that
  is not the case here, and the cost of being wrong is a silently corrupt ride library.

### What the implementation must honour

- **One inflate, never two.** A single gzip member only. If the inflated bytes are themselves
  gzip-signed (gzip-of-gzip), that is a refusal, not a second pass — `RideImportWizard` expands to
  one depth, so depth 2 could not import even if staged.
- **Completeness is zlib's answer, not a guess.** Accept inflate output only on `Z_STREAM_END`
  with `avail_in == 0`. `Z_OK`/`Z_BUF_ERROR` at loop exit, or input left over, is a refusal.
  A partial member must never be staged: that is B-STAGE9-89, the worst of the three, because
  partial FIT bytes can parse far enough to import a truncated ride.
- **Refusal is a pause, not a silent skip.** New `PauseReason::UndecodablePayload`; the activity is
  NOT recorded imported and the cursor does NOT advance past it. ACCEPTED COST: if Garmin were ever
  to serve that shape persistently, the backfill re-fetches it each run and makes no progress. That
  is the same trade DES-006 already takes for a torn write, it is loud rather than silent, and the
  per-activity skip-list that would fix it properly belongs to DEC-071's record split, not here.
- **ZIP keeps its signature sniff.** ZIP reaches a REAL implementation (`ZipReader` via `Archive`)
  which validates and fails visibly on its own; the controller does not need to second-guess it.
  gzip must be fully resolved here precisely because nothing downstream implements it
  (`ArchiveFile.cpp:62-65` is still an empty block, still frozen, still an upstream GC defect).
- **The refusal branch is where the tests go.** Truncated member, concatenated members,
  gzip-of-gzip, empty inflate, and a FIT-signature lie must each assert the refusal, not a suffix.

### Cascade impact

DES-006's PauseReason set gains a member — a design-surface change, so DES-006's row needs review
when this lands. B-STAGE9-87 is CLOSED and rides in the pending commit. B-STAGE9-78 still does not
close on any gate: it needs the live re-run raising the ride library above 1146. If a future
maintainer wants gzip to import rather than be refused, the fix is `ArchiveFile.cpp`'s empty arm
plus `Archive::extract`, upstream and shared — not another shape branch in this controller.

## DEC-074 — findings.md's 200B row cap is the defect, not the rows

- Status: **accepted 2026-09-26** (Inspector, Three-Options Doctrine, on the librarian's
  measured Job-3 compaction draft. A ledger convention and its gate trigger — no product
  code, no credential, no external element.)
- Reversibility: high — it changes one budget line in `STATE.md` and the rows it already
  compacted are recoverable verbatim from `archive/findings-detail.md`.
- Decided / last-reviewed: 2026-09-26
- Serves: the `FINDINGS worst single row` BUDGETS breach, standing since 2026-09-06.
- Dependents: `STATE.md` BUDGETS line; `archive/findings-detail.md` (append-only);
  `scripts/clv_findings.py` is UNCHANGED and stays the gate.
- Origin: librarian Job-3 COMPACTION draft, 2026-09-26, dispatched by
  `garmin_inspector_v1_59`.

### The problem, restated

`STATE.md` has carried `FINDINGS worst single row 19,477B/200B cap [BREACH]` for 20 days.
A budget line that is permanently red is a line nobody reads — and this register's own
header exists because the project once produced a clean gate over a live blocker.

### Why the cap, not the rows

Three measurements, each independently sufficient:

1. **The schema cannot meet 200B even fully compacted.** findings.md declares "this file
   holds the closure" — a 6-cell record (id, cycle, severity, summary, disposition,
   resolved-by). 200B is the generic `ROW` figure from `references/state-and-tiers.md`,
   written for DECIDX/LSN one-liners and applied to this register without adjustment. It is
   closer to the budget for the summary cell alone.
2. **The project already set its own de facto floor, and it is not 200B.** The 2026-09-06
   compaction produced `B-R017-06` — four of six cells archived — at 538B. The live median
   is 496B. The median row already matches the project's own prior target.
3. **The defect is tail leakage, not median bloat.** All five worst rows were
   already-resolved findings whose 4-13 adversarial repair rounds accumulated inline
   instead of being condensed at disposition. A 200B cap and a 1,200B cap are both blown
   through by a 19kB round-13 saga sitting in a hot row; only routing it to the cold
   archive fixes it.

### Chosen, and the two alternatives rejected

Scored 5/5/5/5 (reliability/scalability/maintainability/best practices): retire the flat
200B cap for findings.md, set ~600B soft / ~1,200B hard, and route the leakage to the
existing archive. Rejected:

- **Compact all 507 rows to meet 200B** (2/1/2/2) — re-compacts rows the project already
  compacted once, and would have to destroy closure content to hit a number no evidence
  supports.
- **Keep 200B, pin it as an accepted permanent breach** (2/3/2/2) — leaves a red line in
  BUDGETS forever and trains readers to skip the section. The precedent for a pin-DEC
  (shellcheck's declared gap, DEC-060's fillers) is a residual the machine genuinely
  cannot recognise; a budget number the maintainer picked is not that.

### Executed in this pass, and independently verified

Five rows compacted 66,734B -> 5,173B; 61,561B moved VERBATIM to
`archive/findings-detail.md` (append-only, its prior 2,808 lines untouched). Nothing
deleted. `clv_findings.py` re-run by the Inspector after applying: 499 data rows, 0
MALFORMED, 0 UNKNOWN-SEVERITY, 0 UNKNOWN-DISPOSITION, 0 NEEDS-DISPOSITION, 493 OK, the
SAME 6 OUTSTANDING ids at the same lines — byte-identical to the pre-apply run. Worst row
is now 9,191B (B-STAGE9-16), still over the new 1,200B hard cap, so BUDGETS stays in
breach honestly rather than being declared green: B-STAGE9-97 carries the follow-up pass.

### Cascade impact

No REQ/DES/TEST row changes. B-STAGE9-96..100 record the findings this pass turned up,
including one the gate structurally cannot catch (B-STAGE9-98).

---

## DEC-075 — slice 1's write contract: `pending` is single-writer, and a file we cannot model is never overwritten

- Status: **accepted 2026-09-27** (Inspector, Three-Options Doctrine, on `garmin_codex_reviewer`'s
  B-STAGE9-79-s1 delta-check. Product code, no credential or external element — an ordinary
  technical call, not a human-in-the-loop gate.)
- Reversibility: medium — it adds a `LoadStatus` value and turns three writers' silent overwrite
  into a typed refusal. Callers already map `false` to `PauseReason::StatePersistFailed`.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-101/-102/-103 (all blocking), raised against DEC-071's slice 1. Amends DEC-071's
  write path only; its record split and `RideCache` completion seam are untouched.
- Dependents: `src/Cloud/GarminSidecarStore.{h,cpp}`, `unittests/Core/garminconnect/
  testGarminSidecarStore.cpp`. Slice 2's call routes need NO load-first change once this lands.
- Origin: the B-STAGE9-79-s1 review round; the clobber was first seen by the Inspector reading the
  diff and independently confirmed BLOCKING by the reviewer.

### The problem, restated

DEC-071 put `pending` inside `BackfillState`, and slice 1's serializer writes the whole struct. Both
existing callers (`GarminBackfillController.cpp:270-274`, `:405-409`) build a FRESH `BackfillState`
and save it, so every sync run writes `pending: {}` over the manifest DEC-071 exists to protect —
the orphaned-activity defect survives its own fix. Two adjacent holes came out of the same review:
the writers treat an owner-wider (`SidecarPermissionsRejected`) cursor as empty and overwrite it at
0600, destroying a cursor that a `chmod` would have recovered; and a present-but-non-object
`pending` entry is silently skipped on load, so the next write makes that omission permanent.

### Chosen: the store holds the invariant, not its callers

1. `pending` is **single-writer**: only `recordPendingBackfill`/`dropPendingBackfill` mutate it.
   `saveBackfillState` persists the three cursor fields and preserves the on-disk map verbatim,
   ignoring its argument's `pending`. No caller can forget to load first, because loading first is
   no longer the caller's job.
2. Split "nothing to lose" from "something we cannot model". `NotFound` and `Torn` (unparseable
   bytes — no recoverable content by definition) keep self-healing by overwrite, the existing
   `recordImported` precedent. `SidecarPermissionsRejected` and a new `PendingManifestMalformed`
   (parsed fine, but `pending` or one of its entries is not an object) make all three writers
   return `false` and write nothing.

Scored 5/4/5/5 (reliability/scalability/maintainability/best practices) against:

- **Fix the two call sites in slice 2 to load first** (2/4/2/2). The reviewer rejected this
  explicitly: it leaves the unsafe default in place, so every future caller of a cursor writer must
  remember an invariant nothing enforces. Cheapest now, and the same class of bug is B-STAGE9-79
  itself.
- **Move `pending` into its own sidecar file** (4/4/3/3). Removes the clobber by construction, but
  adds a third file, a migration, and a second permission gate, and reaches past slice 1's
  chartered files for no reliability gain over option 1.

Also rejected, and worth recording because it is the obvious shortcut: making malformed `pending`
plain `Torn` and refusing to write on `Torn`. That converts today's self-healing corrupt-cursor
case into one needing manual file deletion — a user-visible regression on a path that already
recovers — so the fifth status is what buys the guarantee without it.

### Cascade impact

No REQ/DES row changes. `LoadStatus` gains a fifth value, so every `switch` over it is a cascade
site; slice 2 must not add a load-first workaround this makes dead. TEST ids for the three new
assertions are allocated when the repair round reports.

---

## DEC-076 — B-STAGE9-108 remedy: the sidecar store serializes its own load-modify-write transactions

- Status: **accepted 2026-09-27** (Inspector, Three-Options Doctrine, on
  `s979_record_split_investigator`'s B-STAGE9-108-concurrency trace. Product code, no credential or
  external element — an ordinary technical call, not a human-in-the-loop gate.)
- Reversibility: high — it adds a lock inside four existing store functions; no signature, file
  format, or caller changes.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-108 (blocking). Extends DEC-075's write contract; DEC-071's record split and
  `RideCache` completion seam are untouched.
- Dependents: `src/Cloud/GarminSidecarStore.{h,cpp}`, `unittests/Core/garminconnect/
  testGarminSidecarStore.cpp`. Slice 2's call routes need NO change from this.
- Origin: B-STAGE9-108, raised by the reviewer against slice 1 and referred to an isolated
  investigator because a lock and a pinned non-issue were both live on the evidence then available.

### The evidence that decides it

`CONCURRENT WRITERS POSSIBLE: YES`, and wider than the finding claimed. Two `GarminConnect` clones
for the SAME athlete and uid write the same files on different threads with nothing between them:
the GUI backfill controller runs on its caller thread (`GarminBackfillController.h:26-29`, created
per invocation at `AthletePages.cpp:210-235`), while auto-download calls `readFile()` from its own
`QThread::run()` (`CloudService.h:1388-1401`, `CloudService.cpp:4125-4129,4316-4341`) whenever
sync-on-startup is enabled (`Athlete.cpp:167-169`, or Check Cloud at `MainWindow.cpp:2529-2536`).
The only guard, `m_syncInProgress`, is per-object and covers `readdir()` alone
(`GarminConnect.cpp:787-800`, `GarminConnect.h:288-292`); DEC-002's mailbox is per clone
(`GarminConnect.cpp:209-225`, `GarminDownloadChain.cpp:20-28`) and sidecar writes return to the
caller thread, not the worker (`GarminConnect.cpp:937-952`). The filename is uid-derived
(`GarminSidecarStore.cpp:163-175`), so same-uid clones hit the identical path.

Two corrections to the finding's framing, both material:

1. **The live race is not slice 1's.** `recordImported` has had this exact load-modify-write shape
   all along (`GarminSidecarStore.cpp:212-235`) with two production callers
   (`GarminBackfillController.cpp:395-409`, `GarminConnect.cpp:921-931`), and `saveBackfillState`
   can lose a cursor advance the same way (`:300` load, `:304-308` write).
2. **The pending-map loss specifically is latent, not live.** `recordPendingBackfill` and
   `dropPendingBackfill` exist only as declarations and definitions
   (`GarminSidecarStore.h:181-187`, `.cpp:311-335`) — slice 2 is what gives them callers. So the
   fix must land BEFORE slice 2, not after it.

### Chosen: a per-file lock held across the whole transaction, inside the store

Serialize on the resolved sidecar path inside `GarminSidecarStore`, covering `saveBackfillState`,
`recordImported`, `recordPendingBackfill` and `dropPendingBackfill` from their load through their
`AtomicFile::writeOver`. `AtomicFile` keeps giving per-write atomicity; the lock supplies what it
never did, atomicity across the read-modify-write.

Scored 5/4/5/5 (reliability/scalability/maintainability/best practices) against:

- **Pin B-STAGE9-108 as an accepted non-issue** (1/5/3/2). This was genuinely live before the
  trace and is now refuted by it: the interleaving is constructible today, on the shipped
  `recordImported` path, with no new code at all.
- **Serialize at the call sites** (2/3/2/2). Reaches into slice 2's frozen files, contradicts
  DEC-075's finding that the store holds the invariant rather than its callers, and puts the
  obligation on every future caller — the same shape as the defect it would be fixing.

### Residual, recorded rather than left implicit

An in-process lock does not serialize two GoldenCheetah processes sharing one athlete config
directory. That is out of scope here and NOT covered by this DEC: it is a pre-existing whole-app
condition across every sidecar and cache GoldenCheetah writes, not something DEC-071 introduced.

### Cascade impact

No REQ/DES row changes, no signature changes, no format change. TEST ids for the new assertions are
allocated when the round reports.

## DEC-077 — B-STAGE9-116 is upstream GoldenCheetah, not this branch's: frozen, not repaired

- Status: **accepted 2026-09-27** (Inspector, on `s979_record_split_investigator`'s
  B-STAGE9-116-scope verdict, dispatched fresh and unbriefed with only the narrow scope question.
  Product-scope call, no credential or external element — not a human-in-the-loop gate.)
- Reversibility: high — it defers a repair; nothing is written, and a later branch can take it.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-116 (blocking, reclassified pre-existing). Leaves B-STAGE9-114's fix standing:
  that one is the dialog's own post-`process()` sweep and IS this branch's.
- Dependents: none in `src/`. `src/Gui/RideImportWizard.{h,cpp}`, `src/Core/Athlete.cpp` and
  `src/Core/Context.h` stay on HARD HOLD for every builder round on this branch.
- Origin: the reviewer's confirmation pass on unit B-STAGE9-79-s2-r2rev raised the in-`process()`
  use-after-free as a fifth lifetime round against the same dialog.

### The evidence that decides it

`VERDICT: PRE-EXISTING-UPSTREAM`. `RideImportWizard` holds a raw `Context*` member
(`src/Gui/RideImportWizard.h:92`) and dereferences it at `src/Gui/RideImportWizard.cpp:1050,1118`.
Five callers share that one body, four of them nothing to do with Garmin: `MainWindow`'s two manual
imports (`src/Gui/MainWindow.cpp:1676-1677,1851-1852`), `WebPageWindow` download import
(`src/Train/WebPageWindow.cpp:271-284`), `TrainSidebar` import
(`src/Train/TrainSidebar.cpp:1631-1635`), and `Athlete` auto-import
(`src/Core/Athlete.cpp:387-400`).

The exposure is real, and reachable without Garmin: `MainWindow`'s manual import builds an
unparented wizard (`src/Gui/MainWindow.cpp:1851-1852`; default parent null,
`src/Gui/RideImportWizard.h:46-48`), `process()` pumps GUI events
(`src/Gui/RideImportWizard.cpp:1019-1023`), and athlete-tab close tests only `athlete->autoImport`
(`src/Gui/MainWindow.cpp:2088-2100`) so it does not see a manual wizard. `removeAthleteTab()` then
deletes Athlete and Context (`src/Gui/MainWindow.cpp:2169-2185`) and `~Athlete()` deletes
`rideCache` (`src/Core/Athlete.cpp:250-253`). Auto-import alone is protected, by the
`importInProcess()` checks at `src/Gui/MainWindow.cpp:1089-1110,2088-2094,2117-2122`.

Garmin neither creates nor widens the gap: `src/Cloud/GarminBackfillDialog.cpp:297-323` constructs
the same wizard and calls the same method, and its close veto
(`src/Cloud/GarminBackfillDialog.cpp:381-397`) protects the dialog, not the wizard's raw
collaborators.

### Why frozen rather than repaired

The remedy is a lifetime rework of a shared import path used by four non-Garmin callers, in files
this branch has never touched. Taking it here means owning regression risk for manual import, train
import and web import to close a defect none of this branch's code introduced — the same call
already recorded for `ArchiveFile.cpp`'s empty GZIP arm and `CloudService.cpp:565`'s `gUncompress`.
B-STAGE9-116 stays a blocking finding against GoldenCheetah upstream; it stops blocking Stage 9.

### Cascade impact

No REQ/DES/TEST row changes. B-STAGE9-117's stub-fidelity residual is pinned with it: a faithful
stub needs the owning teardown and nested loop that only this repair would build.

## DEC-078 — B-STAGE9-112 remedy: the resume cursor and the range start stop being one variable

- Status: **accepted 2026-09-27** (Inspector, at the repair-round bound. Two rounds against
  B-STAGE9-112 both narrowed the recogniser instead of moving the fact, and the reviewer's round-3
  boundary case is the same class again. Product code, no external element — not a gate.)
- Reversibility: medium — it changes one filter predicate and one initialisation inside
  `GarminBackfillController::start()`; no file format, no signature, no caller changes.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-112 (blocking), B-STAGE9-121 (blocking), B-STAGE9-122 (test weakness). Extends
  DEC-071's resume behaviour and DEC-075/-076's store contract; neither is amended.
- Dependents: `src/Cloud/GarminBackfillController.cpp`, `src/Cloud/GarminSidecarStore.{h,cpp}`,
  `unittests/Core/garminconnect/{testGarminBackfillController,testGarminSidecarStore}.cpp`.
- Origin: `garmin_codex_reviewer`, unit B-STAGE9-112-r3rev.

### The evidence that decides it

`GarminBackfillController.cpp:283` seeds `cursor` with `rangeStartGmt`, then `:336` filters
`if (s.startTimeGMT <= cursor || s.startTimeGMT > rangeEndGmt) continue;`. The same variable is
therefore read two incompatible ways: as an EXCLUSIVE "everything at or before this already landed"
marker when a prior success exists, and as an INCLUSIVE "start of the requested window" when none
does. Every attempt to fix B-STAGE9-112 by choosing a better rewind value fails on that conflation:
clearing the cursor falls back to `rangeStart`, and `<= cursor` then excludes an activity sitting
exactly on `rangeStart` — the reviewer's midnight case. The same conflation silently drops the
earliest activity of every range even with no drop involved (B-STAGE9-121).

### The decision

Split the two roles. Keep the persisted `lastSuccessStartTimeGMT` as the exclusive resume marker and
apply it only when a prior success is in range; when there is none, `rangeStartGmt` is INCLUSIVE.
The filter becomes: skip if a prior success exists and `startTimeGMT <= lastSuccess`; skip if
`startTimeGMT < rangeStartGmt`; skip if `startTimeGMT > rangeEndGmt`. With that, the store's clear
from B-STAGE9-112's round becomes correct as written — a cleared cursor means "no prior success",
`rangeStart` is honoured inclusively, and the dropped entry is re-listed. No greatest-surviving
rewind is needed, so the store keeps the simpler transaction DEC-076 already locks.

### Why not a fourth repair round

Rounds 2 and 3 both returned `CLASS: … — SAME`. Per the repair-round bound, the next dispatch must
change where the machine reads its fact, not how it recognises the case. This does: it removes the
overloaded variable rather than picking a cleverer value for it.

### Cascade impact

No REQ/DES row changes. B-STAGE9-122's assertions must name expected cursor VALUES rather than
inequalities, in the same round. New TEST ids from T-238, including one pinning an activity exactly
on `rangeStart`.

## DEC-079 — B-STAGE9-79 slice 3: the legacy migration classifies itself at the dialog, before start()

- Status: **accepted 2026-09-27** (Inspector, on `s979_record_split_investigator`'s three scored
  designs, unit B-STAGE9-79-s3-design. Placement and data-classification call on Garmin-side code,
  no external element — not a gate.)
- Reversibility: medium — it adds a one-shot migration pass and a `schema_version` stamp; no existing
  file format field changes meaning, and an un-migrated sidecar keeps loading exactly as it does now.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-79 (blocking, slice 3 — the last slice). Extends DEC-071's measured exact-compare
  seam and DEC-075's store-owned invariant; amends neither.
- Dependents: `src/Cloud/GarminBackfillDialog.cpp`, `src/Cloud/GarminSidecarStore.{h,cpp}`,
  `unittests/Core/garminconnect/testGarminBackfillDialogLifetime.cpp`.
- Origin: `s979_record_split_investigator`, unit B-STAGE9-79-s3-design.

### The evidence that decides it

`imported-<uid>.json` is an unversioned map of `activityId -> {startTimeGMT, local_filename}`
(`GarminSidecarStore.h:14-24,67-70`, parser `.cpp:201-232`). A legacy row and a post-slice-2
completion row are therefore byte-for-byte identical — nothing in the row says which code wrote it.
What DOES distinguish them is the sibling state file: `backfill-state-<uid>.json` carries
`schema_version`, and a missing one loads as version 0 (`.cpp:299-317`). The live sidecar pair from
the 2026-09-26 run confirms it — three imported ids, each with only those two fields, beside a state
file holding only the three cursor/range keys and no version and no pending.

The placement follows from what each candidate seam can actually reach. The store's APIs are static
and receive only config-dir and uid (`GarminSidecarStore.h:139-194`), so it cannot consult
`RideCache` without violating DEC-075. `GarminBackfillController`'s constructor takes only
client/config-dir/uid (`.h:84-87`) and it writes state before listing (`.cpp:284-312`), which would
stamp v1 before anything was classified. The dialog holds both halves: uid and config-dir
(`GarminBackfillDialog.cpp:161-177`) and protected `Athlete`/`RideCache` access (`:319-347`) — the
same `RideCache::getRide(QDateTime)` exact compare (`RideCache.cpp:807-813`) that DEC-071 measured
and slice 2 already promotes through.

### The decision

Run a one-shot migration in the dialog, BEFORE `GarminBackfillController::start()`, gated on
`state.isOk() && schemaVersion == 0`. Exact-match every legacy `imported` entry against `RideCache`;
retain the matches as completion, move the misses into `pending`, then stamp the schema version so
the pass never repeats. It must be idempotent — the store's locks are per-file, not cross-file
(`GarminSidecarStore.cpp:150-169`), so a crash mid-pass must leave a state a re-run can finish.

Arms that must NOT guess: `NotFound` no-ops and the first ordinary backfill writes v1
(`.cpp:207-212,268-272`); a torn sidecar yields no recoverable entries and must not be classified or
promoted (`.cpp:214-232,274-278`); permission-rejected and malformed-pending states stay untouched
because every cursor writer already refuses them (`.cpp:327-335,344-380`).

### Amendment 2026-09-27 — the gate needs a guard on the non-dialog writer (B-STAGE9-127)

The decision above is unreachable as written. `serializeBackfillState()`
(`GarminSidecarStore.cpp:121-137`) stamps the current version on EVERY state write, and the
non-dialog route performs one: `readFile()` -> `GarminConnect::recordImport()` (`.cpp:691`) loads
the legacy state and calls `saveBackfillState()` (`:927-931`). One ordinary sync therefore stamps a
legacy athlete v1 and `schemaVersion == 0` never holds again.

Remedy, inside this decision's placement and needing no new DEC: for precisely
`bf.isOk() && bf.state.schemaVersion == 0`, `recordImport()` keeps `recordImported()` and SKIPS only
the cursor save, so the dialog stays the sole upgrader of a legacy v0 file. B-STAGE9-111's
replacement pending writer takes the same guard.

Scope limit, load-bearing: the guard applies ONLY to an existing Ok/v0 state. `NotFound` and torn
states have no legacy data to classify and keep DEC-075's self-healing write
(`GarminSidecarStore.cpp:321-335`) — a blanket non-dialog write ban would contradict this decision.

Cost accepted: a v0 athlete who never opens Backfill re-lists from the old cursor every sync.
That is repeated listing work, not loss or duplicate download — Tier-1 `imported` dedup
(`GarminConnect.cpp:855-879`) skips every recorded id, and no consumer reads the cursor as proof an
import completed. Independently checked before this amendment was recorded: `garmin_codex_reviewer`,
unit B-STAGE9-127-remedy, which also confirmed `schema_version` is the sound hinge (a separate
one-shot marker would duplicate migration state and widen the cross-file crash surface).

### Why not the alternatives

Injecting a completion-lookup callback into the controller (reliability 5, maintainability 2) can run
before the controller's first state write, but it widens a deliberately UI-decoupled controller API
(`GarminBackfillController.h:99-111`) and duplicates completion policy the dialog already owns.
Re-offering every v0 row unconditionally (reliability 2, existing-data risk 2) is mechanically
simplest but re-presents known-complete rides and contradicts DEC-071's measured self-classification
outright — rejected by authority, not on score.

### Cascade impact

No REQ/DES row changes. One new `schema_version` write path in the store and one migration pass in
the dialog. Tests from T-240, extending the promotion test at
`testGarminBackfillDialogLifetime.cpp:451-498`, plus torn/NotFound/idempotence arms.

## DEC-080 — DEC-077's freeze is narrowed: additive no-op extension points are allowed, repairs are not

- Status: **accepted 2026-09-27** (Inspector, on `s979_record_split_investigator`'s unit
  B-STAGE9-111-design, which stopped and said plainly that all three of its designs need this
  amendment. A scope call between two accepted decisions of this project's own, no external element
  — not a gate. Amends DEC-077; does not touch DEC-071.)
- Reversibility: high — the amendment authorises one default-no-op virtual and its call sites. Reverting
  means deleting them; no non-Garmin provider is asked to implement anything.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-111 (blocking), and through it B-STAGE9-79's both-routes requirement.
- Dependents: `src/Cloud/CloudService.{h,cpp}`, `src/Cloud/GarminConnect.{h,cpp}`,
  `unittests/Core/garminconnect/testGarminConnectSync.cpp`.
- Origin: `s979_record_split_investigator`, unit B-STAGE9-111-design.

### The conflict that forces a decision

DEC-071 requires the completion-only dedup fix on BOTH download routes. DEC-077 froze
`CloudService`/`RideImportWizard`/`ArchiveFile` as upstream GoldenCheetah code this branch never
touched. B-STAGE9-111 is the second route — and it cannot be fixed without a promotion site, because
the only two places that register a ride (`CloudService.cpp:3547-3564` after `saveRide`, and
`:4490-4498` after `Athlete::addRide`) hold a generic `CloudService*` while the latched uid and
config-dir accessors belong to `GarminConnect` alone (`GarminConnect.h:141-149`). The investigator
scored three designs and reported that literal byte-identity of shared code cannot hold for ANY of
them: each must add a call after registration. So one of the two decisions has to yield.

### The decision

DEC-071 wins; DEC-077 is narrowed rather than overridden. The freeze keeps its full force for what it
was actually written about — REPAIRING defects in shared import code, where the fix is a rework with
four non-Garmin callers and a Garmin-free reachable path. It does not extend to ADDING a
default-no-op extension point, where every non-Garmin provider inherits the no-op and its observable
behaviour is unchanged.

Concretely authorised: one `virtual void rideRegistrationCompleted(const QString& remoteId)` with an
empty default body on `CloudService`, beside the existing default-no-op provider hooks
(`CloudService.h:287-297,337-340`); its invocation after each SUCCESSFUL registration only; and
`GarminConnect`'s override doing the promotion through the DEC-075/-076 guarded order. Design A
(reliability 5 / maintainability 5 / blast radius 4) over a base-held `std::function` (4/3/3, adds
callback-lifetime state) and a Qt signal (4/2/2, adds connection and thread-order machinery for no
extra capability).

### The boundary that keeps this from becoming a general licence

Three things stay frozen and are NOT reopened by this amendment: the `RideImportWizard` raw `Context*`
use-after-free (B-STAGE9-116), `ArchiveFile.cpp`'s empty GZIP arm, and `CloudService.cpp:565`'s
`gUncompress`. Any future change in this area that is not (a) additive and (b) default-no-op for every
existing provider needs its own decision, not this one.

### Cascade impact

The hook must NOT fire on any abandonment path: `CloudService.cpp:3389-3397` (abort before parse),
`:3465-3479` (abort after parse), `:4029-4034` (existing-file refusal), `:4408-4415` (auto teardown),
`:4439-4455` (parse failure), `:4470-4479` (duplicate-file refusal). T-048's contract changes rather
than passing unchanged: BBB becomes pending with the cursor advanced and absent from `imported`, and
the next listing re-offers it. A `context == nullptr` path has no registration consumer, so it
persists pending and never promotes. Tests from T-240.

## DEC-081 — Garmin backfill timestamps are canonicalised at the adapter boundary, not compared as text

- Status: **accepted 2026-09-27** (Inspector, on `s979_record_split_investigator`'s three scored
  options, unit B-STAGE9-128-design. Ordinary technical call on Garmin-side code, no external
  element — not a gate.)
- Reversibility: medium — the adapter change is one function; the on-disk rewrite is a one-shot
  migration that a reader accepting both spellings makes safe to roll back.
- Decided / last-reviewed: 2026-09-27
- Serves: B-STAGE9-128 (non-blocking, latent). Does NOT hold Stage 9 and must not displace
  B-STAGE9-79 slice 3 or B-STAGE9-111 in the builder queue.
- Dependents: `src/Python/garminconnect/gc_garmin_adapter/garmin_client.py`,
  `src/Cloud/GarminSidecarStore.cpp`, `src/Cloud/GarminBackfillController.cpp`,
  `src/Cloud/GarminBackfillDialog.cpp`, and the two Garmin test files.
- Origin: `garmin_codex_reviewer` unit B-STAGE9-126-rev raised it; the Inspector re-set its
  severity against the real sidecar; `s979_record_split_investigator` scored the options.

### The evidence that decides it

The comparison is textual at FIVE sites, not the two the finding first named:
`GarminSidecarStore.cpp:375-378` (the drop-rewind), and in the controller the cursor filter
(`:349`), cursor-in-range selection (`:292-295`), the range filter (`:351`) and the sort
(`:357-359`). `GarminBackfillDialog.cpp:47-52`'s completion parser accepts only the current
space-second spelling before DEC-071's exact `RideCache::getRide()` lookup (`:337-346`).

The adapter parses an accepted timestamp to a real instant (`garmin_client.py:79-92`) purely to
compare it, then emits the ORIGINAL text (`:431-435`). So an offset or `Z` spelling would flow
through unnormalised and sort lexically against a differently-spelled cursor.

It is latent rather than live: every non-empty value in the 2026-09-26 run's real sidecars is
space-separated UTC seconds with no `T`, no offset and no fractional part.

### The decision

Canonicalise in the adapter (option A, scored 5/5/5/5). Each accepted summary carries one
`yyyy-MM-dd HH:mm:ss` UTC spelling; the adapter keeps parsing every variant it accepts today. The
rejected alternatives: instant-aware C++ compares (4/3/2/3) duplicate instant policy across five
sites plus the dialog and the fake, and leave raw text on disk for ever; schema-v2 sidecars
(5/4/2/3) buy no capability over A at the highest migration cost.

Binding constraints on the implementation:
- Canonicalisation must REJECT a non-zero fractional second rather than round or truncate it.
  DEC-071's completion seam is a measured second-level exact compare; silently reshaping a value
  it matches on would invalidate that measurement.
- Sidecars already on disk are rewritten once, atomically, cursor and ranges and pending and
  imported together; readers accept the old spelling for as long as legacy files are supported.
- `FakeBackfillClient::listActivities()` (`testGarminBackfillController.cpp:150-162`) repeats the
  production text compare, so it must move to the same canonical basis or no controller test can
  detect a regression here.

### Cascade impact

No REQ/DES row changes. Regression coverage the investigator named: an equivalent-instant
cursor/summary pair (`...+00:00` vs `...Z`) must neither re-download nor strand — one test in
`testGarminBackfillController.cpp`, its twin in `testGarminSidecarStore.cpp`.
