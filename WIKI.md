# PROJECT WIKI — GoldenCheetah            (the brain · read me first)
root: /media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah   schema: wiki-v1
phase: 2 · Phase 2.2 — Garmin Connect   live status/gate → STATE.md · per-id status → traceability.md (DEC-015 SSOT)
This hub is NAVIGATION + ROLLUPS ONLY. It carries no per-id status and no verdicts (LSN-035).
"Which user journeys actually work?" → traceability.md `## Phase-1 capability matrix`.

## MAP — directory manifest (authoritative; check before creating ANYTHING)
WIKI.md · STATE.md · lessons.md   hub · SOLE live cursor (DEC-015) · LSN rules (index head is the hot read)
wiki/                    5 spokes → PAGES
.claude/workflow-INDEX.md   ledger index; names the active ledger
.claude/workflow-garminconnect/   ACTIVE ledger: prd decisions design traceability findings dod ambiguities intake
                         options-catalog + cycles/archive/ validations/archive/ (ALL closed runs; 2026-08-30 the
                         last 3 cycles + 9 VALs moved here). There is no active/ dir — which means NO CYCLE OR
                         VALIDATION IS OPEN, and nothing more. Three separate things, because they are separately
                         true: (a) no active cycle/validation report; (b) the 41-path documentation + gate-repair
                         slice IS COMMITTED (2026-09-06, user decision, with the O-R027-01 reconciliation, the
                         fd7639f7a traceability repair and the STATE/WIKI cursor-drift corrections folded in —
                         exact paths in STATE.md ## COMMIT MANIFEST; the commit is the child of `3b8226ec4`);
                         (c) nothing is pushed. Take the live state from STATE.md, never
                         from the absence of a directory.
                         scripts/ = clv-lite.sh (runner) + clv_findings.py (THE canonical CLV Check 5) +
                         test_clv_findings.py (its 18 synthetic self-tests)
                         archive/state-history.md = ALL superseded STATE narrative (§ 9 = the pre-2026-08-30 cursor)
.claude/workflow-aicoach/   CLOSED ledger (AI Coach) — provenance only, OWN DEC/REQ/TEST numbering
.claude/agents/ hooks/ settings.json   5 qgdw agent defs · installed guard + drift-lint copies · hook wiring
.claude/skills/          VENDOR TERRITORY, replaced wholesale on update — never put project files here (ORCH-004)
.claude/worktrees/       untracked evidence worktrees — do not edit in place
.claude/evidence-seals/  untracked, in-tree commit-safety seals (rt8+). SECONDARY copy — the ESTABLISHED
                         recovery location is ../GoldenCheetah-recovery/ (below). NEVER staged with a slice.
../GoldenCheetah-recovery/DEC-040/   **THE ESTABLISHED RECOVERY LOCATION** (OUTSIDE the repo, same drive) —
                         durable DEC-040 evidence: pre-repair-*, post-deletelater-*, full-verification-*, rt8-*.
                         House convention per set: manifest.txt + sha256-per-file.txt (+ tar.gz/archive.sha256
                         for the archived ones). Seal order: content first, indexes LAST, nothing written after.
                         Session scratchpads under /tmp (rt3–rt7) are NOT durable — copy seals here
scripts/                 project-owned mechanisms (CANONICAL, not .claude/skills/): ledger_drift_lint.py + its
                         test · test_anti_duplication_guard_flags.py (ORCH-043 two-directional matrix; targets the
                         INSTALLED .claude/hooks/ copy, kept here so a skill reinstall cannot delete it)
util/                    dev scripts (fit, bundles, linters, rpi) — UPSTREAM-owned, not ours
src/                     app C++, 13 dirs: ANT Charts Cloud Coach Core FileIO Gui Metrics Planning Python R
                         Resources Train — roles → wiki/architecture.md
src/Cloud/               THE ACTIVE CODE: CloudService + GarminConnect worker/adapter/token/sidecar/download seams
src/Python/garminconnect/   vendored Python adapter pkg (own tests + pyproject)
src/Coach/               AI Coach — SHIPPED, ledger closed
unittests/               QTest units, Core/ + Gui/. Core/coach CMakeLists+stubs COMMITTED (DEC-028), added
                         UNCONDITIONALLY — a guard would silently skip a tracked test
test/                    ride/workout/measurement FIXTURE DATA — not test code
docs/                    WORKFLOW_GUIDE · QGDW_SKILL_RETROSPECTIVE · REQ028_PROCESS_AUDIT · BUILD_NOTES ·
                         MODERNIZATION · project_analysis · COACH_*
doc/                     LEGACY doc archive — name collides with wiki/, content unrelated; never conflate
contrib/ qwt/ deprecated/   vendored third-party · bundled Qwt · retired source
CMakeLists.txt CMakePresets.json vcpkg.json   CMake build (migration alongside qmake) · build.pro src/src.pro qmake
.clang-format .clang-tidy .pre-commit-config.yaml   style/lint (pre-commit scoped to Garmin paths, DEC-010)
appveyor.yml appveyor/ .github/ISSUE_TEMPLATE/   dated CI + per-OS scripts; issue templates only, no workflows
INSTALL-* BUILD_DEPENDENCIES_INSTALL.sh README.md CONTRIBUTING.md COPYING .git* .vscode/   docs, standard, editor
build/ .mypy_cache/ .ruff_cache/ src/*.o,moc_*,qrc_*,*_yacc*,*_lex*   [SKIP] generated, untracked+gitignored
per-file detail → wiki/map-detail.md (a file under a rolled-up dir IS mapped, via its parent)

## REGISTRIES — what exists (allocate next; never reuse, never re-create)
RULE: registries POINT. No per-id detail, no per-id status here (LSN-035). Ledger = .claude/workflow-garminconnect/
REQ  garmin:001–029 + 16 REQ-NF-* ids (10 traceability rows)   prd.md            next:garmin-030
DEC  garmin:001–043                 decisions.md ## Decision index  next:garmin-044
     43 allocated, all 43 have an ENTRY section as of 2026-09-05. The index carries 43 DEC rows + 1
     `DEC-034/036 AMENDMENT` row.
     DEC-040 = PROVIDER WATCHDOG (W3/S-1). The parked auto-downloader teardown draft is NOT DEC-040 and
     carries NO id — it was styled "DEC-040" in 2026-08-26 session narrative only. See DEC-040's ID NOTE.
     DEC-042/043 (2026-09-05) = the two Stage-5 release blockers (B-R028-17, A3-R028e-F1), accepted
     (Option A / Option C respectively) and BOTH BUILT + execution-verified 2026-09-05 (TEST-158;
     TEST-159+160) — both findings closed on executed evidence; see decisions.md entries for trade space.
DES  garmin:001–014 (+001a,003a)    design.md                       next:garmin-015
TEST garmin:T-001–T-160 (T-143–T-153 = DEC-040; per-id Stage-1/2 build state → traceability.md)   next:garmin-T-161
     T-154–T-156 BUILT 2026-09-04 (DEC-033: out-param correctness + syncNext/downloadNext clause-(e)
     coverage, all passing). TEST-076 (pre-existing id, allocated long before this session, previously
     unused/dead-code-pinned) also BUILT 2026-09-04 — closes B-R027-09. T-157 BUILT 2026-09-04 (B-R028-01
     coverage: InSaveRideAutoProcess + restartInsteadOfRefresh combination) — closes B-R028-01, discharging
     Stage 2 in full. T-158 BUILT 2026-09-05 (DEC-042: aParentTeardownInsideSaveRidesSecondAutoProcess
     MustNotFreeItUnderThat — 99/99 both backends, orchestrator-independently mutation-proven) — closes
     B-R028-17, one of Stage 5's two release blockers. T-159/T-160 BUILT 2026-09-05 (DEC-043:
     readComplete-after-owner-teardown + athleteClose cancel/join/delete, watchdog 170/170 both backends,
     orchestrator-independently mutation-proven) — closes A3-R028e-F1, the second and last one.
VAL  garmin:001–018                 traceability.md ## Validations run · validations/archive/   next:garmin-019
LSN  001–084  (contiguous; LSN-048 has an index line AND a cold entry — that is the file's two-tier
     shape, not a duplicate)   lessons.md                          next:085
     081 = caller-owned test budget · 082 = an unreadable input reported as non-blocking is failing
     open · 083 = a gate whose actions cannot change its own pass criteria · 084 = `git checkout --`
     on a file with PRIOR uncommitted changes discards all of them, not just the intended one
F    no F-### namespace — findings are `<cycle>-<seq>`; process series ORCH-001–059   next:ORCH-060
     052 = a real _minimal-QPA test-harness hang (kdialog config-write race), reproduction attempted and
     NOT reproduced via ctest — non-blocking, does not affect the DEC-033/TEST-076 commit's own evidence
     053 = shapeBC_timeoutBreaksTheListing's site-10 row is mutation-blind (single-activity fixture cannot
     see a continue-vs-break defect); its new Cancelled sibling built alongside T-147 piece 3 does not have
     this gap and is the genuine killing test for that site today
     054 = OPEN, self-inflicted: `git checkout --` during piece 3b's mutation-proof restore wiped
     CloudService.h's uncommitted DEC-040 Stage 2 pieces 1+2 content; reconstructed same session from
     decisions.md + CloudService.cpp's intact usage contract, full 168-test suite re-passes both QPA
     backends, but neither original mutation proof has been RE-RUN against the reconstructed bytes yet
     055 = OPEN, reviewer finding: T-148's rows overwrite `cancelToken_` with a fresh default `CancelToken()`
     via `driveSite`'s unconditional setter call, so they don't test the member's own construction-time
     default — `driveSite` needs "no token supplied" to genuinely skip the setter
     057 = OPEN, non-blocking: `CloudServiceAutoDownload::run()` reads `context->athlete->rideCache->rides()`
     on the WORKER thread itself (CloudService.cpp:4188) — a different surface from A3-R028e-F1/DEC-043's
     GUI-thread completion-slot UAF, found while scouting that fix, deliberately kept separate by user
     decision 2026-09-05, no REQ allocated
     058 = FIXED (local patch, 2026-09-05, non-blocking residue): `anti_duplication_guard.py` used to deny
     the RESTORE half of the LSN-084 snapshot/restore convention; `_is_snapshot_restore()` now tolerates
     decorated snapshot names — patch verified present in both copies, byte-identical, this session; NOT
     yet shipped upstream (LSN-036 miss recorded) — full terms in findings.md
     059 = OPEN, non-blocking, test-harness: TEST-160 drives a hand-mirrored stub `Athlete::close()`
     (ProviderSeamStubs.cpp) because no garminconnect test target links the real Athlete.cpp — nothing
     keeps stub and production in lockstep; verified matching by hand at DEC-043 closure, drift risk open
     findings.md is SSOT for severity + disposition. An id is NOT unique alone (A3-R027 collides) — cite id
     AND its Cycle-column value (ORCH-022).
-- retired ledger (provenance only): aicoach:DEC-001–013, REQ-001–020, TEST-001–020 — these COLLIDE with the
   garmin ranges; always qualify (coach:DEC-NNN / garmin:DEC-NNN).
why a number was reserved → wiki/registry-detail.md (DATED provenance, NOT a status source)

## PAGES — wiki spokes (read the one named; don't explore blindly)
wiki/architecture.md    components · Garmin auth data-flow · IGarminAuthClient/IGarminPyAdapter contracts · Watch
wiki/conventions.md     canonical locations · ID-namespace rule · anti-dup checklist — read BEFORE creating anything
wiki/glossary.md        project terms — read when a term is unfamiliar
wiki/map-detail.md      per-file MAP detail — read when a rolled-up MAP line is too coarse
wiki/registry-detail.md dated allocation provenance — read for WHY a number exists, never for whether it is done
lessons.md              LSN-001..083 — read the guards matching this operation BEFORE acting
decisions.md ## Decision index   one line per DEC, active/dormant split — the Tier-1 decision head
traceability.md         the REQ→DEC→DES→TEST→COMMIT spine — the ONLY home for per-id lifecycle status.
                        Take a status from its `Status (current)` column ONLY; the TEST(s) cell is dated
                        HISTORICAL EVIDENCE and is not maintained as a status (schema changed 2026-08-30)
archive/state-history.md   ALL superseded STATE narrative (CURRENT-PRIOR / NEXT_GATE-PRIOR chains; § 9 = the
                        entire pre-2026-08-30 cursor, extracted verbatim). History, never a status source.
.claude/workflow-INDEX.md  which ledger is active vs closed   (no per-ledger state.md — DEC-015 deleted it)

## ORIENTATION PROTOCOL
1 read this first   2 navigate by MAP/PAGES, never blind   3 check MAP before create
4 allocate IDs from REGISTRIES (ledger-prefixed)   5 lost? re-read this   6 update MAP/REGISTRIES as byproduct
