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
docs/                       BUILD_NOTES.md, COACH_DEV_GUIDE.md, COACH_IMPLEMENTATION.md, COACH_USER_GUIDE.md, MODERNIZATION.md, project_analysis.md, WORKFLOW_GUIDE.md (workflow operating manual), QGDW_SKILL_RETROSPECTIVE.md (evidence-based review OF the workflow skill itself + a reusable skill-audit prompt)
qwt/                        bundled Qwt plotting library (vendored, not walked further)
test/                       sample ride/workout/measurement fixture files (aerolab, bodymeasures, charts, coretemp, hrvmeasures, rides, roundtrip, rowing, runs, swims, workouts) — data, not automated test code
unittests/                  QTest units: Core/{coach,garminconnect,season*,signalSafety,splineCrash,units,utils}, Gui/calendarData. Core/coach/ carries CMakeLists.txt + stubs/{GCStubPreamble,Athlete,Context,Season,Seasons}.h — COMMITTED 2026-08-07 (DEC-028) so committed CMake references only committed files; `unittests/CMakeLists.txt` adds Core/coach UNCONDITIONALLY (no EXISTS guard, deliberately — a guard would silently skip a tracked test)
util/                       dev scripts (fit tooling, bundle fixups, safety-check linters, rpi packaging) — UPSTREAM-owned; workflow-governance tooling goes in scripts/ instead
scripts/                    project-owned workflow mechanisms (CANONICAL source; deliberately NOT in .claude/skills/ — vendor territory, ORCH-004): ledger_drift_lint.py (DEC-015 status-drift lint) + test_ledger_drift_lint.py (TEST-017, 15 cases). Synced into .claude/hooks/ by `install_hook.py --extra-hook scripts/ledger_drift_lint.py .`
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
.claude/hooks/ledger_drift_lint.py        INSTALLED COPY of the DEC-015 status-drift lint — canonical source is `scripts/ledger_drift_lint.py` (ORCH-004, moved 2026-08-05); pre-commit + CLV call this copy. **Synced by `install_hook.py --extra-hook scripts/ledger_drift_lint.py .`** — re-run after every skill update AND after any edit to the source; the flag copies but deliberately does NOT wire settings.json (this lint is a pre-commit/CLV hook, not PreToolUse)
.claude/agents/             5 qgdw subagent defs (scout, builder, adversary, validator, librarian) — canonical loaded copies; install-source duplicates live under skills/
.claude/skills/quality-gated-dev-workflow/   SKILL.md + agents/(install source) + references/*.md (methodology) + scripts/(anti_duplication_guard.py + guard_selftest.py — 46-case bundled matrix, run it after EVERY skill update against the installed hook + install_hook.py). **VENDOR TERRITORY — replaced wholesale on every skill update; never put project-owned files here (the guard denies new files / warns on edits); project mechanisms live in `scripts/` — ORCH-004**
.claude/skills/test-driven-development/SKILL.md   TDD skill
.claude/workflow-INDEX.md   one-ledger-per-feature index; names active ledger
.claude/workflow-garminconnect/   ACTIVE ledger (Garmin Connect, Phase 2.2) — file-by-file breakdown → wiki/conventions.md
.claude/workflow-aicoach/   CLOSED ledger (AI Coach, SHIPPED, src/Coach/) — provenance only, own DEC/REQ/TEST numbering (see REGISTRIES) — file-by-file breakdown → wiki/conventions.md

## REGISTRIES — what exists (allocate next; never reuse, never re-create)
REQ  garmin:001–021+NF-*  full:.claude/workflow-garminconnect/prd.md         next:garmin-022   (REQ-019 Upload UAF = BUILT+A3'd 2026-08-08; REQ-020 OAuth-wizard UAF = STUB; REQ-021 COLLABORATOR-lifetime UAF = STUB, NEW 2026-08-08 from A3-R019-F1/F2, covers upload AND the shipped sync dialog; what each REQ is + its status → prd.md / traceability.md — DEC-015 SSOT)
DEC  garmin:001–029  index:.claude/workflow-garminconnect/decisions.md      next:garmin-030   (DEC-029 ALLOCATED 2026-08-07 to REQ-019 Upload-UAF fix shape — scout researching, not yet decided)   (question + chosen option + status → decisions.md; the DEC index table in traceability.md must list EVERY id — VAL-017 found it silently stopped at DEC-019)
DES  garmin:001–014(+001a,003a)  .claude/workflow-garminconnect/design.md   next:garmin-015
TEST garmin:T-001–T-080  .claude/workflow-garminconnect/traceability.md     next:garmin-T-081   (T-075 ctor-route ASan DONE; T-076 allocated-UNUSED [dead-code guards, LSN-022]; T-077 A3-R026-F2 fixture-gap coverage [4 guard slots] + T-078 syncCloud-entry teardown DONE 2026-08-06 [DEC-027]; T-079 upload-dialog teardown slots + T-080 exec()-window teardown probe ALLOCATED 2026-08-07 [REQ-019/DEC-029, NOT yet built]; what each TEST covers + whether it is built/green → traceability.md ONLY)
VAL  garmin:001–017  next:garmin-018   (verdicts + scope → traceability.md ## Validations run; report files → .claude/workflow-garminconnect/validations/)
F    no F-### namespace in use; findings tracked as <cycle>-<seq> (A0-001…, A3-Rxxx-Fn, B-Rxxx-nn, O-Rxxx-nn orchestrator-found, **ORCH-001–009 orchestrator/process+build — ALLOCATE FROM HERE, next:ORCH-010**, D-0x) in findings.md — **that file is the SSOT for severity + disposition; this hub POINTS, never restates** (LSN-035). Live blocking count → findings.md / STATE.BLOCKING.   next:n/a (see conventions.md)
LSN  001–047  active:47 guards:33 mech:2 (LSN-001 anti-dup hook, LSN-008 ledger-drift lint)  lessons.md   next:048   (each lesson's rule/check → lessons.md; the index lines there are the hot read)
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
