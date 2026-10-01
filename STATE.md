# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-10-01 by `garmin_inspector_v1_77`
# Project cursor only (QGDW Tier-0 schema, quality-gated-dev-workflow references/state-and-tiers.md).
# Id counters live in WIKI.md REGISTRIES only.
# Per-id status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT);
# DEC status in decisions.md; finding severity/disposition in findings.md. Agent roster, launch
# and tooling rules live in .claude/skills/inspector-cycle/ — never here. Superseded narrative:
# archive/state-history.md (Stage 1-8, live-account run log, cursor chain § 13, partition § 14).

PHASE:     2.2 · Garmin Connect integration, Stage 9 (real-account connect/MFA/sync/disconnect +
           installed-package smoke checklist). Stages 1-8 all discharged on executed evidence.
           Sync journey proven live 2026-09-29 (qmake binary at `5226bdbf6`): connect, 2FA, sync
           (library 1146→1148), cancel re-offers. Stage 9 now waits only on installer provenance —
           a green AppVeyor run (DEC-065), not producible on this host.
OPEN:      Installer class only (-48/-54/-57/-71), closes on AppVeyor post-PUSH. Frozen upstream,
           NOT this branch's: `ArchiveFile.cpp` GZIP arm, `CloudService.cpp:565` `gUncompress`,
           `RideImportWizard` raw `Context*` (DEC-077/-080 / B-STAGE9-116) — HARD HOLD.
BLOCKING:
  B-STAGE9-48[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-54[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-57[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-71[STAGE:9]
           # -48/-54/-57/-71 are one class, none is code: no local installer-build environment;
           # they close on a green AppVeyor run post-PUSH. -71 residual: unpinned 7z +
           # unverified BadCmd NSIS profile. -116 is blocking but FROZEN upstream (DEC-077).
           # Non-blocking, unlisted: B-STAGE9-73/-76/-77/-117..-120/-122..-125/-128.
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py clean at last run)
LAST_CLV:  clv_findings.py 2026-09-28: 0 MALFORMED / 0 UNKNOWN-* / 0 NEEDS-DISPOSITION /
           9 OUTSTANDING / 561 rows.
NEXT_GATE: Canonical gate `ctest -LE gate-exclude` (DEC-054, dod.md:16-47), not `-L garmin-fast`
           (B-STAGE9-115). -128 is subsumed by DEC-084 (landed in `e15d863ba`).
           -78/-79 discharged by the 2026-09-29 live run. -161/-166 fixed; -165/-167 accept-with-note.
           First fork AppVeyor run (ci.1, `26d6b6da5`) failed at setup on all 3 legs: -168..-172, fixed in
           `c26162841`. Upstream master is green on AppVeyor (Joern-R/goldencheetah-knhd8).
           ci.2 failed all 3 legs: -173 fixed `dce1079e2`, -174 fixed `4b5eaa79c`.
           ci.3 (54809173): Win built the installer and passed the DEC-069 check but hit the 60-min cap;
           Linux -175, macOS -176 timeout. Cause: fork CI recipe lagged upstream → DEC-089 sync
           (-175..-181 fixed, reviewer PASS).
           ci.4 (54816113): Linux GREEN (-54/-57 fixed); Win -182 fixed `f52975a52`; macOS timed out
           cold and a timeout saves no cache → DEC-089/5 primes it (-183 fixed).
           ci.5 (`a83c21f50`): Linux GREEN; Win -184 fixed `97a5d7a9e`; mac -185 fixed `439463ddb`.
           ci.6: Win hit 60-min cap (qwt rebuilt every run); Linux/mac lost the commit (-187, pushed
           mid-queue). Root cause -186: dependency-keyed qwt cache froze lib-less copies; fixed, local.
           NEXT: ci.7 (`439463ddb`) cannot pass (-186). Push -186 after ci.7 ends (needs user approval, -187) →
           one priming run saves qwt/lib on all legs → next run all 3 green closes -48/-71.
           AppVeyor connected 2026-09-29 (fork webhook live); `appveyor.yml` branch filter now includes
           this branch (fork-only CI edit — revert before any upstream PR).
           -79's completion seam is settled by measurement: no tolerance window without new
           measurement (DEC-071).
           KEEP `~/.goldencheetah/Andy/config/garminconnect/backfill/`'s two live staged files —
           the -78 regression fixture. NEVER commit the untracked `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: fork branch at `439463ddb` (pushed 2026-10-01); later commits stay local until the next
           user-approved push. Code commits:
           `b82063118` slice 1, `54afaaf16` DEC-076's lock, `930329fec` slice 2 + DEC-078,
           `ab0bb7586` -126/-125/-124, `4dacd8447` -127, `e15d863ba` -111/-133, `5226bdbf6` slice 3
           (-130/-154/-155/-157..-159/-162/-163, DEC-087).
TEAM:      on(5 agents)
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   WIKI ~2,550tok/700 [BREACH] · DECIDX ~8kB · ROWS 1 over 4kB (DEC-029 slice) · ledgers
           compacted 2026-09-28: findings 76kB, decisions 134kB, traceability 48kB
