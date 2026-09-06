# Map detail (spoke)            updated:2026-08-23

<!-- provenance: dated -->
**What this is.** The per-file MAP lines that used to live in `WIKI.md`'s `## MAP` block,
relocated VERBATIM on 2026-08-23 by the `qgdw-librarian` Job-3 compaction pass. Nothing was
edited or deleted; only this preamble was added.

**Why it moved.** MAP budget & rollup rule (wiki-memory.md): *"The MAP has a hard budget:
~40 lines / ~700 tokens. Small projects map file-level; as the tree grows, roll up to directory
level ... Fine-grained per-file notes, if ever needed, live in `wiki/map-detail.md` (Tier 1,
read on demand) — never in the hub."*

**Coverage rule is unchanged.** A file under a rolled-up directory IS "in the MAP" via its
parent — the anti-duplication guard already resolves parent-dir coverage. Read the hub MAP
first; drill here only when the directory-level line is too coarse to answer
"does this already exist?".

---

## Pre-compaction MAP block, verbatim (was WIKI.md:5–55, 2026-08-23)

## MAP — directory manifest (authoritative; check before creating ANYTHING)
WIKI.md                     this brain (hub) — read first, every session
wiki/                       spokes: architecture.md, conventions.md, glossary.md
STATE.md                    project cursor — the SOLE live cursor (DEC-015; no per-ledger state.md). Names the gate/blockers/commits, not per-id status (→ traceability.md)
lessons.md                  process lessons LSN-001..064 (index head is the hot read; full entries cold). THREE promoted to MECHANISM: LSN-001 anti-dup hook, LSN-008 DEC-015 drift lint, LSN-062 dual-backend ctest registration
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
docs/                       BUILD_NOTES.md, COACH_DEV_GUIDE.md, COACH_IMPLEMENTATION.md, COACH_USER_GUIDE.md, MODERNIZATION.md, project_analysis.md, WORKFLOW_GUIDE.md (workflow operating manual), QGDW_SKILL_RETROSPECTIVE.md (evidence-based review OF the workflow skill itself + a reusable skill-audit prompt), **REQ028_PROCESS_AUDIT.md (the 2026-08-22 process/architecture audit that raised A3-R028c-F8 — UNTRACKED in git and previously ABSENT from this MAP; registered 2026-08-22 at the commit-safety gate)**
qwt/                        bundled Qwt plotting library (vendored, not walked further)
test/                       sample ride/workout/measurement fixture files (aerolab, bodymeasures, charts, coretemp, hrvmeasures, rides, roundtrip, rowing, runs, swims, workouts) — data, not automated test code
unittests/                  QTest units: Core/{coach,garminconnect,season*,signalSafety,splineCrash,units,utils}, Gui/calendarData. Core/coach/ carries CMakeLists.txt + stubs/{GCStubPreamble,Athlete,Context,Season,Seasons}.h — COMMITTED 2026-08-07 (DEC-028) so committed CMake references only committed files; `unittests/CMakeLists.txt` adds Core/coach UNCONDITIONALLY (no EXISTS guard, deliberately — a guard would silently skip a tracked test)
util/                       dev scripts (fit tooling, bundle fixups, safety-check linters, rpi packaging) — UPSTREAM-owned; workflow-governance tooling goes in scripts/ instead
scripts/                    project-owned workflow mechanisms (CANONICAL source; deliberately NOT in .claude/skills/ — vendor territory, ORCH-004): ledger_drift_lint.py (DEC-015 status-drift lint; **assignment-shape discrimination added 2026-08-12, ORCH-010** — an adjectival status word is prose, not an assignment; **BINDING SCOPE added 2026-08-15, ORCH-015** — a status binds an id only within the same sentence+quotation scope, and a PARENTHESISED id is a citation that binds nothing — the rule is DIRECTIONAL, so an id inside an aside binds no outside status, while a status inside an aside still binds an id outside it, which is the real drift shape) + test_ledger_drift_lint.py (TEST-017, **44 cases**, both directions). Synced into .claude/hooks/ by `install_hook.py --extra-hook scripts/ledger_drift_lint.py .`
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

