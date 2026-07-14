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
src/Cloud/                  CloudService integrations incl. GarminConnect.{h,cpp}, GarminWorker, GarminCredentialsPage, IGarminAuthClient, IGarminPyAdapter, PyEmbeddedAdapter, GarminAuthChain + AtomicFile.{h,cpp} (DES-006, REQ-004) + GarminTokenStore.{h,cpp} (DES-002, REQ-004/006) — see wiki/architecture.md
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
REQ  garmin:001–015+NF-*  full:.claude/workflow-garminconnect/prd.md         next:garmin-016
DEC  garmin:001–015  index:.claude/workflow-garminconnect/decisions.md      next:garmin-016
DES  garmin:001–013(+001a,003a)  .claude/workflow-garminconnect/design.md   next:garmin-014
TEST garmin:T-001–T-017  .claude/workflow-garminconnect/traceability.md     next:garmin-T-018 (T-011..T-013 REQ-004; T-014 REQ-006 Slice A load-side perm-refusal committed d86323246; T-015 pytest + T-016 garmin-py REQ-006 Slice B auth-only reconciliation committed 3edb705cb; T-017 ledger_drift_lint.py unittest suite — DEC-015, committed 88d4ea402; migration committed 2520ed034)
VAL  garmin:001–012  latest:VAL-012 PASS (DEC-015 status-SSOT migration, full 9-check after 1 repair pass); VAL-011 PASS (REQ-006) · full canonical VAL table → traceability.md ## Validations run; files → .claude/workflow-garminconnect/validations/  next:garmin-013
F    no F-### namespace in use; findings tracked as <cycle>-<seq> (A0-001…, A3-R00x-Mn/TR-nn, D-0x, W-DEC003) in findings.md; 0 open blocking   next:n/a (see conventions.md)
LSN  001–015  active:15 guards:9 mech:2 (LSN-001 anti-dup hook, LSN-008 ledger-drift lint)  lessons.md   next:016
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
