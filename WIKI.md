# PROJECT WIKI — GoldenCheetah   (read me first)  schema: wiki-v1
root: /media/andy/TOSHIBA EXT/Backup2/Documents/GoldenCheetah · Phase 2.2 Garmin Connect
Navigation only; no per-id status (LSN-035). Live status → STATE.md · per-id status → traceability.md (DEC-015)

## MAP (check before creating ANYTHING; verbatim detail → wiki/map-detail.md)
WIKI.md STATE.md lessons.md   hub · sole cursor · LSN index
wiki/                    5 spokes (PAGES)
.claude/workflow-garminconnect/  ACTIVE ledger: prd decisions design traceability findings dod ambiguities intake options-catalog · cycles/ validations/ archive/ scripts/ (no open cycle; read STATE.md, not dir absence)
.claude/workflow-aicoach/  CLOSED ledger, own numbering
.claude/{workflow-INDEX.md,agents,hooks,settings.json}  ledger index · 5 qgdw agents · guards · wiring
.claude/skills/          VENDOR, never add project files (ORCH-004); inspector-cycle/ = project-authored
.claude/{worktrees,evidence-seals}/  untracked evidence · TRACKED seals (git sole copy); ../GoldenCheetah-recovery ABSENT
scripts/                 ledger_drift_lint.py(+test) · test_anti_duplication_guard_flags.py
src/                     13 dirs → wiki/architecture.md; active: Cloud/ python garminconnect/ · Coach/ shipped
unittests/ test/         QTest · fixture data
docs/ doc/               current docs · LEGACY (≠ wiki/)
util/ contrib/ qwt/ deprecated/   upstream/vendored
build+CI+style           CMake/qmake · appveyor* .github/ · .clang-* · .pre-commit · INSTALL-* README
build/ caches src/*.o,moc_*  [SKIP] generated

## REGISTRIES (point only; ledger .claude/workflow-garminconnect/; notes → wiki/registry-detail.md)
REQ garmin:001–030 +16 NF           next:garmin-031
DEC garmin:001–093                  next:garmin-094
DES garmin:001–014(+001a,003a)      next:garmin-015
TEST garmin:T-001–T-263(+261b)      next:garmin-T-264
VAL garmin:001–018                  next:garmin-019
LSN 001–087                         next:088
F  ORCH-001–067 · B-STAGE9-001–193 · B-STAGE10-01–18 (SSOT findings.md)   next:ORCH-068 · B-STAGE10-19
aicoach:DEC/REQ/TEST ids COLLIDE with garmin — always prefix

## PAGES
wiki/{architecture,conventions(read BEFORE create),glossary,map-detail,registry-detail}.md
lessons.md · decisions.md ## Decision index · traceability.md · archive/{state-history,findings-detail,findings-notes,decisions-full,traceability-history}.md (Tier 3)

## PROTOCOL  read first → navigate MAP/PAGES → check before create → allocate prefixed IDs → update as byproduct
