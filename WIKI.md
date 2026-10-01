# PROJECT WIKI — GoldenCheetah            (the brain · read me first)
root: /media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah   schema: wiki-v1
phase: 2 · Phase 2.2 — Garmin Connect   live status/gate → STATE.md · per-id status → traceability.md (DEC-015 SSOT)
This hub is NAVIGATION + ROLLUPS ONLY. It carries no per-id status and no verdicts (LSN-035).
"Which user journeys actually work?" → traceability.md `## Phase-1 capability matrix`.

## MAP — directory manifest (authoritative; check before creating ANYTHING)
WIKI.md · STATE.md · lessons.md   hub · SOLE live cursor (DEC-015) · LSN rules (index head is the hot read)
wiki/                    5 spokes → PAGES
.claude/skills/inspector-cycle/   project-authored Claude Code skill (not a vendor package) — Inspector's own
                         herdr-stage-delegate-poll-validate-document-commit supervision loop; SKILL.md +
                         references/ only, no scripts/hooks. Source of truth also kept at
                         /home/andy/Downloads/inspector-cycle/ (authoring copy).
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
                         archive/state-history.md = ALL superseded STATE narrative (§ 9 = the pre-2026-08-30 cursor,
                         § 11 = the full pre-2026-09-06 STATE.md, § 12 = that pass's BUDGETS breach detail)
                         archive/ = verbatim pre-compaction text (2026-09-06, 2026-09-28): findings-detail.md
                         (rows marked ↗), findings-notes.md, decisions-full.md (full DEC entries + old index
                         rows), traceability-history.md (full REQ rows ↗ + dated slice records) — Tier 3 only
.claude/workflow-aicoach/   CLOSED ledger (AI Coach) — provenance only, OWN DEC/REQ/TEST numbering
.claude/agents/ hooks/ settings.json   5 qgdw agent defs · installed guard + drift-lint copies · hook wiring
.claude/skills/          VENDOR TERRITORY, replaced wholesale on update — never put project files here (ORCH-004)
.claude/worktrees/       untracked evidence worktrees — do not edit in place
.claude/evidence-seals/  in-tree commit-safety seals (rt8+). **TRACKED, not untracked** — the five
                         DEC040-Stage2-*.log seals were committed in `fd7639f7a` and git history is now
                         their only durable copy; ../GoldenCheetah-recovery/ does NOT exist on this host
                         (B-STAGE9-74, 2026-09-24). Do not delete them expecting a copy elsewhere.
../GoldenCheetah-recovery/DEC-040/   **ABSENT on this host — verified 2026-09-24 (B-STAGE9-74).** Described
                         below as originally established, retained only so the convention is legible —
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
REQ  garmin:001–029 + 16 REQ-NF-* ids (10 traceability rows)   prd.md                        next:garmin-030
DEC  garmin:001–089 (index + full entries; DEC-040 ID NOTE covers the "parked teardown draft"
     mislabel — cite decisions.md, never this line)   decisions.md ## Decision index   next:garmin-090
DES  garmin:001–014 (+001a,003a)    design.md                                          next:garmin-015
TEST garmin:T-001–T-263 incl. T-261b (per-id mapping → traceability.md ONLY; T-240 used by
     slice 3; T-246 a declared never-written gap; B-STAGE9-95 owns the T-212..T-234 cell backfill)
     traceability.md                                                                  next:garmin-T-264
VAL  garmin:001–018                 traceability.md ## Validations run · validations/archive/   next:garmin-019
LSN  001–085  (contiguous; LSN-048 has an index line AND a cold entry — that is the file's two-tier
     shape, not a duplicate; per-id rule/check → lessons.md, hot index lines are the read)
     lessons.md                                                                        next:086
F    no F-### namespace — findings are `<cycle>-<seq>`; process series ORCH-001–062, Stage-9
     series B-STAGE9-001–185 (a reviewer's own ids always collide — re-number); disposition/
     severity SSOT is findings.md (cite id AND Cycle-column, ORCH-022)   next:ORCH-063 · next:B-STAGE9-186
-- retired ledger (provenance only): aicoach:DEC-001–013, REQ-001–020, TEST-001–020 — these COLLIDE with the
   garmin ranges; always qualify (coach:DEC-NNN / garmin:DEC-NNN).
why a number was reserved → wiki/registry-detail.md (DATED provenance, NOT a status source)

## PAGES — wiki spokes (read the one named; don't explore blindly)
wiki/architecture.md    components · Garmin auth data-flow · IGarminAuthClient/IGarminPyAdapter contracts · Watch
wiki/conventions.md     canonical locations · ID-namespace rule · anti-dup checklist — read BEFORE creating anything
wiki/glossary.md        project terms — read when a term is unfamiliar
wiki/map-detail.md      per-file MAP detail — read when a rolled-up MAP line is too coarse
wiki/registry-detail.md dated allocation provenance — read for WHY a number exists, never for whether it is done
lessons.md              LSN-001..085 — read the guards matching this operation BEFORE acting
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
