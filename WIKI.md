# PROJECT WIKI — GoldenCheetah            (the brain · read me first)
root:  /media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah   schema: wiki-v1
phase: 2 · Phase 2.2 — Garmin Connect integration (per-id status lives in the traceability matrix — DEC-015 SSOT; do NOT restate it here)   live-status + current gate → STATE.md (read next)

## MAP — directory manifest (authoritative; check before creating ANYTHING)
WIKI.md                     this brain (hub) — read first, every session
wiki/                       spokes: architecture.md, conventions.md, glossary.md
STATE.md                    project cursor — the SOLE live cursor (DEC-015; no per-ledger state.md). Names the gate/blockers/commits, not per-id status (→ traceability.md)
lessons.md                  process lessons LSN-001..015 (index + cold entries); LSN-008 promoted to MECHANISM (DEC-015 lint)
CMakeLists.txt              new CMake build definition (migration in progress alongside qmake)
CMakePresets.json           CMake configure/build presets
build.pro, src/src.pro      legacy top-level qmake project files
vcpkg.json                  vcpkg dependency manifest (CMake toolchain)
.clang-format, .clang-tidy  C++ style/lint config
.pre-commit-config.yaml     pre-commit framework config, scoped to new Garmin paths (DEC-010)
BUILD_DEPENDENCIES_INSTALL.sh, INSTALL-CMAKE, INSTALL-LINUX, INSTALL-MAC, INSTALL-WIN32   build/install docs
README.md, CONTRIBUTING.md, COPYING, .gitattributes, .gitignore   standard project files
appveyor.yml + appveyor/    dated CI config + per-OS (linux/macos/windows) build scripts
.github/ISSUE_TEMPLATE/     issue templates only (no CI workflow files)
.vscode/settings.json       editor config
build/                      [SKIP] generated CMake+Ninja build tree — untracked+gitignored (anomaly resolved, commit 4a8856109)
.mypy_cache/, .ruff_cache/  [SKIP] generated tool caches
contrib/                    vendored third-party code (httpserver, kmeans, lmfit, qtsolutions, qxt, qzip, voronoi, RideLogger Android app, patches)
deprecated/                 retired source, excluded from build; kept for reference
doc/                        legacy doc archive: collaboration, contrib, design, doxygen, user, web, wiki (images/pdf/texinfo — asset dirs, not walked further) — name collides with new wiki/ spoke dir, unrelated content
docs/                       BUILD_NOTES.md, COACH_DEV_GUIDE.md, COACH_IMPLEMENTATION.md, COACH_USER_GUIDE.md, MODERNIZATION.md, project_analysis.md, WORKFLOW_GUIDE.md (workflow operating manual)
qwt/                        bundled Qwt plotting library (vendored, not walked further)
test/                       sample ride/workout/measurement fixture files (aerolab, bodymeasures, charts, coretemp, hrvmeasures, rides, roundtrip, rowing, runs, swims, workouts) — data, not automated test code
unittests/                  QTest units: Core/{coach,garminconnect,season*,signalSafety,splineCrash,units,utils}, Gui/calendarData
util/                       dev scripts (fit tooling, bundle fixups, safety-check linters, rpi packaging)
src/ANT/                    ANT+/ANT USB device protocol stack
src/Charts/                 chart windows & plotting widgets (LTM, histogram, scatter, PMC, etc.)
src/Cloud/                  CloudService integrations incl. GarminConnect.{h,cpp} (open/close/readFile — REQ-007, DES-004/016; + readdir incremental-sync orchestration — REQ-008 Slice C, DES-010: Tier-1 sidecar short-circuit + concurrent-guard + record-on-download), GarminWorker, GarminCredentialsPage, IGarminAuthClient, IGarminPyAdapter (+PyListOutcome/GarminActivitySummary/listActivitiesSince — REQ-008 Slice A listing seam), PyEmbeddedAdapter, GarminAuthChain, IGarminDownloadClient.h + GarminDownloadClient.{h,cpp} + GarminDownloadChain.{h,cpp} (download+restore seam+host, DES-014) + AtomicFile.{h,cpp} (DES-006, REQ-004) + GarminTokenStore.{h,cpp} (DES-002, REQ-004/006; + REQ-008 Slice D producer: persistConnectSuccess + active-account.json {garmin_user_id} writer/reader/clearer, DEC-018 B) + accountStillConnected() fail-closed gate on readFile/readdir (DEC-020, REQ-012) + GarminSidecarStore.{h,cpp} (DES-002/010, REQ-008 Slice B — imported-<uid>.json dedup map + backfill-state-<uid>.json cursor, DEC-017) — see wiki/architecture.md
src/Coach/                  AI Coach feature (LLM clients + tool executor) — SHIPPED, workflow-aicoach CLOSED
src/Core/                   core domain: Athlete, Context, DataFilter (lex/yacc), APIWebService, calendar model
src/FileIO/                 ride file format parsers/writers (FIT/TCX/GC/CSV/SRM/PWX/WKO/…) + athlete backup
src/Gui/                    main window, dialogs, sidebars, wizards (incl. AddCloudWizard)
src/Metrics/                ride metric calcs (power/HR/pace, CP models, TSS variants, PMC)
src/Planning/               training-plan window
src/Python/                 embedded CPython: PythonEmbed core, SIP/ bindings, garminconnect/ (vendored adapter pkg, own tests+pyproject — see architecture.md)
src/R/                      embedded R integration (REmbed, RGraphicsDevice, RTool)
src/Resources/              non-code assets (audio/charts/data/html/images/ini/json/translations/webservice)
src/Train/                  trainer/device control (BT40, Kettler, Computrainer, Daum, …) + Train window
src/*.o,moc_*,qrc_*,*_yacc*,*_lex*   [SKIP] generated in-source qmake build artifacts (untracked/gitignored)
.claude/settings.json       PreToolUse hook wiring (Write|Edit|MultiEdit|Bash → anti_duplication_guard.py)
.claude/hooks/anti_duplication_guard.py   deterministic anti-dup guard, enforces LSN-001
.claude/hooks/ledger_drift_lint.py        installed copy of the DEC-015 status-drift lint (source in skills/.../scripts/); pre-commit + CLV call it
.claude/agents/             5 qgdw subagent defs (scout, builder, adversary, validator, librarian) — canonical loaded copies; install-source duplicates live under skills/
.claude/skills/quality-gated-dev-workflow/   SKILL.md + agents/(install source) + references/*.md (methodology) + scripts/(anti_duplication_guard.py + install_hook.py + ledger_drift_lint.py + test_ledger_drift_lint.py — DEC-015 status-drift lint, TEST-017)
.claude/skills/test-driven-development/SKILL.md   TDD skill
.claude/workflow-INDEX.md   one-ledger-per-feature index; names active ledger
.claude/workflow-garminconnect/   ACTIVE ledger (Garmin Connect, Phase 2.2) — file-by-file breakdown → wiki/conventions.md
.claude/workflow-aicoach/   CLOSED ledger (AI Coach, SHIPPED, src/Coach/) — provenance only, own DEC/REQ/TEST numbering (see REGISTRIES) — file-by-file breakdown → wiki/conventions.md

## REGISTRIES — what exists (allocate next; never reuse, never re-create)
REQ  garmin:001–016+NF-*  full:.claude/workflow-garminconnect/prd.md         next:garmin-017 (REQ-016 = A3-R008-F1/F2 reliability follow-up ticket: dedup-record-after-confirm / reconcile — status → traceability)
DEC  garmin:001–020  index:.claude/workflow-garminconnect/decisions.md      next:garmin-021 (DEC-020 = the A3-R012-F1 disposition — Option C, a fail-closed token re-check in readFile/readdir plus the cheap hardening F2/F4/F5; instance-lifecycle work → a follow-on REQ. Choice + rationale → decisions.md)
DES  garmin:001–014(+001a,003a)  .claude/workflow-garminconnect/design.md   next:garmin-015
TEST garmin:T-001–T-056  .claude/workflow-garminconnect/traceability.md     next:garmin-T-060 (T-057..059 = DEC-020 fail-closed hardening, BUILT + Gate-PASSED + committed `f001c7d20` 2026-08-03: T-057 readFile/readdir fail closed once the account is disconnected (A3-R012-F1); T-058 clearAccount sweeps the `<path>.tmp` siblings (F2); T-059 empty-uid write guard pinned (F4, kills MUT-C) — plus TEST-055 strengthened with mode+mtime (F5, no new id). T-054..056 = REQ-012 disconnect-contract closure, committed `f001c7d20` 2026-08-03, working tree, testGarminConnectConnectPersist/garmin-fast, test-only slice — no production change: T-054 reconnect-requires-full-SSO after disconnectService (restoreCalls==0); T-055 prior-account sidecars byte-identical + not consulted after an account switch; T-056 same-account reconnect resumes imported history + backfill cursor; 4 reverted mutations prove failability — per-id status → traceability. T-053 = A3-R008-F3 fix: PyEmbeddedAdapter listActivitiesSince non-dict-item→Unknown coverage, garmin-py, mutation-proven; T-051..052 = REQ-008 DEC-019 trigger-wiring garmin-fast [T-051 wizard persist on both direct+post-MFA paths + stale-reply-no-overwrite, in testGarminConnectWizardRouting; T-052 GarminConnect::disconnect() clears tokens+active-account/preserves sidecars + base no-op, in testGarminConnectConnectPersist]; T-049..050 = REQ-008 Slice D connect→persist producer garmin-fast [T-049 producer unit: active-account.json + tokens.json 0600 writes, no-override disk resolve, disconnect-clears-preserves-sidecars; T-050 end-to-end connect→persist→resolve→readdir NO ctor override — LSN-024 proof]; T-042..044 = REQ-008 Slice A activity-listing seam: T-042 garmin_client.list_activities_since pytest, T-043 PyEmbeddedAdapter::listActivitiesSince garmin-py, T-044 GarminWorker ListActivities op garmin-fast; T-045..046 = REQ-008 Slice B GarminSidecarStore garmin-fast [T-045 imported-<uid> dedup map, T-046 backfill-state cursor]; T-047..048 = REQ-008 Slice C sync orchestration garmin-fast [T-047 GarminConnect::readdir listing+since+concurrent-guard, T-048 Tier-1 dedup short-circuit+record] — per-id status → traceability) (T-027..031 = REQ-003 MFA Slice A [seam]; T-032..035 = Slice B [UI]; T-036..041 = A3-R003 hardening: T-036 garmin-py real-bridge MFA coverage [closed BLOCKING A3-R003-01], T-037 WAC MFA wiring, T-038 pytest MFA retention, T-039 initializePage reentry reset, T-040 stale-failure real assertion [killed mutant M1], T-041 terminal-state dup-delivery guard. The old "T-027 reserved real-signal caller test" was never built and is now consumed by MFA; A3-R007-02 residual stays accepted+id-less) (T-018 restoreSession; T-019 GarminConnect open/close; T-020..023 readFile DEC-016 FIT/TCX table; T-024 readFile queued-not-synchronous completion — B-R007-01 regression; T-025 completion-context cancels pending post on destroy — A3-R007-01 UAF guard; T-026 readFile null-guard — A3-R007-03; working tree, ctest garmin 14/14 / readFile exe 13/13; T-011..T-013 REQ-004; T-014 REQ-006 Slice A load-side perm-refusal committed d86323246; T-015 pytest + T-016 garmin-py REQ-006 Slice B auth-only reconciliation committed 3edb705cb; T-017 ledger_drift_lint.py unittest suite — DEC-015, committed 88d4ea402; migration committed 2520ed034)
VAL  garmin:001–016  next:garmin-017 (VAL-016 = REQ-012 incremental CLV, ALLOCATED + IN FLIGHT 2026-08-02, verdict → traceability ## Validations run) · latest-recorded:VAL-015 (REQ-003 final commit-gate CLV — content PASS on all 9 checks [findings clean/0-blocking, no false-done, spine T-027..041, criterion covered+strengthened, design honest, registries consistent]; the git-less validator raised a lone FAIL it could NOT corroborate "uncommitted" from the stale session-start snapshot — orchestrator resolved via live `git status`/`git log`: ledger CONFIRMED, all REQ-003 files M/?? uncommitted, no REQ-003 commit at HEAD. Net: PASS; false-FAIL → LSN-023); VAL-014 PASS-WITH-WARN (REQ-003 Slices A+B) · full canonical VAL table → traceability.md ## Validations run; files → .claude/workflow-garminconnect/validations/
F    no F-### namespace in use; findings tracked as <cycle>-<seq> (A0-001…, A3-R00x-Mn/TR-nn, D-0x, W-DEC003) in findings.md; **0 open blocking — REQ-008 fully dispositioned.** A3-R008: F1 ACCEPTED-WITH-RATIONALE v1 (ticketed REQ-016, record-after-confirm) + F2 folded into REQ-016; F3 FIXED (TEST-053 coverage) + F4 FIXED (disconnect()→disconnectService() rename); F5 accept-note (OQ1, Phase-3 live spike). A3-R008-01 + D-R008-01 FIXED. **the incremental-sync feature landed as `ff9cce966` + docs-record `b47954308` (per-id status → traceability).** **A3-R012 (2026-08-02) raised 1 BLOCKING — A3-R012-F1: Disconnect does not stop a live opened session (user can keep downloading from a disconnected account); adjudicates B-R012-01 as a REAL unimplemented clause, roots in DEC-019, awaiting user disposition** + F2..F6 non-blocking + F7..F12 informational.   next:n/a (see conventions.md)
LSN  001–032  active:32 guards:20 mech:2 (LSN-001 anti-dup hook, LSN-008 ledger-drift lint)  lessons.md   next:033 (LSN-032 checkout-revert-destroys-uncommitted-work — never `git checkout --` a mutation revert on a file that is dirty; snapshot-and-restore instead) (LSN-028 unaudited-criterion — a REQ satisfied as a side-effect of another REQ's slice needs its own clause-by-clause audit; LSN-029 ordering-invariant-unobserved; LSN-030 secret-deletion-misses-tmp-sibling; LSN-031 untouched-means-bytes-only — all three from the A3-R012 mutation survivors) (LSN-024 consumer-of-deferred-contract; LSN-025 idempotency-record-before-confirm [A3-R008-F1]; LSN-026 rmw-not-salvaging-on-torn; LSN-027 qobject-method-name-hidden)
-- retired ledger (provenance only, do not extend): aicoach:DEC-001–013, aicoach:REQ-001–020, aicoach:TEST-001–020 — numeric collision with garmin ranges above; always use ledger prefix (coach:DEC-NNN / garmin:DEC-NNN)

## PAGES — wiki spokes (read the one named; don't explore blindly)
wiki/architecture.md — components + Garmin auth data-flow + IGarminAuthClient/IGarminPyAdapter contracts + Watch list · read when touching src/Cloud, src/Python/garminconnect, or the active ledger
wiki/conventions.md  — canonical locations, per-ledger ID-namespace rule, anti-dup checklist · read before creating any file/dir
wiki/glossary.md     — project terms (ride/activity, athlete dir, CloudService, worker/adapter seam, ledger, slice, …) · read when a term is unfamiliar
lessons.md           — checkable process rules (LSN-001..015) · read guards matching current operation before acting
(no per-ledger state.md — DEC-015 deleted it; root STATE.md is the SOLE live cursor. Per-id status → traceability.md)
.claude/workflow-INDEX.md — ledger convention + which ledger is active vs closed

## ORIENTATION PROTOCOL
1 read this first   2 navigate by MAP/PAGES, never blind   3 check MAP before create
4 allocate IDs from REGISTRIES (ledger-prefixed)   5 lost? re-read this   6 update MAP/REGISTRIES as byproduct
