# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-10-03 by `garmin_inspector_v1_82`
# Project cursor only (QGDW Tier-0 schema, quality-gated-dev-workflow references/state-and-tiers.md).
# Id counters live in WIKI.md REGISTRIES only.
# Per-id status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT);
# DEC status in decisions.md; finding severity/disposition in findings.md. Agent roster, launch
# and tooling rules live in .claude/skills/inspector-cycle/ — never here. Superseded narrative:
# archive/state-history.md (Stage 1-8, live-account run log, cursor chain § 13, partition § 14).

PHASE:     2.2 · Garmin Connect integration, Stage 9 discharged 2026-10-02. Stages 1-8 on executed
           evidence; Stage 9: live journey 2026-09-29 (qmake binary `5226bdbf6`: connect, 2FA, sync,
           cancel) + AppVeyor ci.10 (`774ba3533`): all 3 installers built and pass the DEC-069 payload
           import from inside the shipped bundle (REQ-NF-Pkg-001). Feature ships on this fork.
OPEN:      Stage 10 · upstream hand-off (DEC-090): PR 1 Garmin-only branch `garmin/upstream-garmin` from
           upstream/master, fork CI all 3 green, then user opens PR; PR 2 Coach after. UPR-1 plan accepted
           (B-STAGE10-01..10): /tmp/upr1/contract.md. UPR-2: branch `garmin/upstream-garmin` (worktree
           .claude/worktrees/upstream-garmin, uncommitted, = plan byte-for-byte) qmake Garmin-on build GREEN,
           reviewer PASS; Garmin-off build GREEN (0 GarminConnect symbols). Branch committed by user `2c292d072`
           (local, pre-scrub; author identity auto-set). Amended by user to `acc6e9bd6` (UPR-4 scrub in, 0 ids;
           B-STAGE10-13 deferred). UPR-3 blocked by B-STAGE10-14: branch lacks Garmin appveyor packaging; UPR-3a port
           `476630af5` (7 CI files, reviewer PASS) + fork-only `994d338a1` filter, `40712ef4a` infra, `5eb437a0d` Win trim, `26f8fb08d` macOS ccache (DEC-091). Fork-only filter revert staged on
           local branch `garmin/upstream-pr` (`62b10f292`, worktree /tmp/gc-upstream-pr). Frozen upstream,
           NOT this branch's: `ArchiveFile.cpp` GZIP arm, `CloudService.cpp:565` `gUncompress`,
           `RideImportWizard` raw `Context*` (DEC-077/-080 / B-STAGE9-116) — HARD HOLD.
BLOCKING:
           # none on this branch. -116 is blocking but FROZEN upstream (DEC-077).
           # Non-blocking, unlisted: B-STAGE9-73/-76/-77/-117..-120/-122..-125/-128; -71 residual
           # (unpinned 7z, unverified BadCmd NSIS profile).
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py clean at last run)
LAST_CLV:  clv_findings.py 2026-09-28: 0 MALFORMED / 0 UNKNOWN-* / 0 NEEDS-DISPOSITION /
           9 OUTSTANDING / 561 rows.
NEXT_GATE: Canonical gate `ctest -LE gate-exclude` (DEC-054, dod.md:16-47), not `-L garmin-fast`
           (B-STAGE9-115). -128 is subsumed by DEC-084 (landed in `e15d863ba`).
           Fork CI history ci.1..ci.10 (-168..-193, DEC-089): findings.md rows; ci.10 is the first
           all-3-leg pass. macOS DMG root holds GoldenCheetah.app (ci.10 ls output).
           NEXT: ci.11 on `994d338a1` failed on fork infra (B-STAGE10-15/-16); fork-only infra commit `40712ef4a`
           (DEC-091, rev PASS). ci.12 on `40712ef4a`: Linux+macOS green incl. DEC-069 check; Windows >60 min (B-STAGE10-17) → fork-only `5eb437a0d`. ci.13: Win+Linux green;
           macOS capped twice on slow hosts (B-STAGE10-18) → UPR-3d fork-only `26f8fb08d` (macOS ccache +
           group-kill compile time-box, rev PASS) → ci.14 running on `26f8fb08d` (pushed 10-03). Never push while a build is queued (-187).
           AppVeyor connected 2026-09-29 (fork webhook live); `appveyor.yml` branch filter now includes
           this branch (fork-only CI edit — revert before any upstream PR).
           -79's completion seam is settled by measurement: no tolerance window without new
           measurement (DEC-071).
           KEEP `~/.goldencheetah/Andy/config/garminconnect/backfill/`'s two live staged files —
           the -78 regression fixture. NEVER commit the untracked `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: fork branch at `774ba3533` (pushed 2026-10-02); later commits stay local until the next
           user-approved push. Code commits:
           `b82063118` slice 1, `54afaaf16` DEC-076's lock, `930329fec` slice 2 + DEC-078,
           `ab0bb7586` -126/-125/-124, `4dacd8447` -127, `e15d863ba` -111/-133, `5226bdbf6` slice 3
           (-130/-154/-155/-157..-159/-162/-163, DEC-087).
TEAM:      on(5 agents)
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   WIKI ~645tok/700 · DECIDX ~8kB · ROWS 1 over 4kB (DEC-029 slice) · ledgers
           compacted 2026-09-28: findings 76kB, decisions 134kB, traceability 48kB
