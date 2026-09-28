# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-28 by `claude_opus_5_5`
# Project cursor only (QGDW Tier-0 schema, quality-gated-dev-workflow references/state-and-tiers.md).
# Id counters live in WIKI.md REGISTRIES only.
# Per-id status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT);
# DEC status in decisions.md; finding severity/disposition in findings.md. Agent roster, launch
# and tooling rules live in .claude/skills/inspector-cycle/ — never here. Superseded narrative:
# archive/state-history.md (Stage 1-8, live-account run log, cursor chain § 13, partition § 14).

PHASE:     2.2 · Garmin Connect integration, Stage 9 (real-account connect/MFA/sync/disconnect +
           installed-package smoke checklist). Stages 1-8 all discharged on executed evidence.
           Stage 9 is NOT closeable: the 2026-09-26 live run proved the sync journey fails on
           two real code defects (B-STAGE9-78/-79), and installer-provenance still needs a
           green AppVeyor run (DEC-065) that cannot be produced on this host.
OPEN:      Two blocking code defects from the live run, both Garmin-side, both decided:
             · B-STAGE9-78 (DEC-070/-072/-073) — code committed, pinned by T-218..T-227. Only a
               live re-run raising the ride library above 1146 discharges it; that needs the
               user's real Garmin account (human-in-the-loop). Frozen upstream, NOT this branch's:
               `ArchiveFile.cpp`'s empty GZIP arm, `CloudService.cpp:565`'s `gUncompress`,
               `RideImportWizard`'s raw `Context*` (DEC-077/-080 / B-STAGE9-116) — HARD HOLD.
             · B-STAGE9-79 (DEC-071) — dedup ledger records an activity at download time, so a
               cancelled import orphans it. Slices 1-2 committed (`b82063118`, `54afaaf16`,
               `930329fec`); slice 3, the legacy migration, is the last. Under DEC-075 the store,
               not its callers, holds the invariant. History -> findings.md + DEC-076.
BLOCKING:
  B-STAGE9-78[CHECKPOINT:STAGE:9]
  B-STAGE9-79[CHECKPOINT:STAGE:9]
  B-STAGE9-130[STAGE:9]  B-STAGE9-154[STAGE:9]  B-STAGE9-155[STAGE:9]
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
           NEXT: slice 3, B-STAGE9-130 (DEC-079 placement + DEC-082 persistence, T-240) — the last
           thing holding -79. Round 1 is +575/-5 uncommitted; its review FAILed on -154 and -155.
           Repair round r2 on -154/-155 is unblocked: the builder is Claude Code again (-156).
           -79's completion seam is settled by measurement: no tolerance window without new
           measurement (DEC-071).
           KEEP `~/.goldencheetah/Andy/config/garminconnect/backfill/`'s two live staged files —
           the -78 regression fixture. NEVER commit the untracked `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: HEAD `0d192ff84`, 26 ahead of `origin/garmin/req028-row-lifetime`. Code commits:
           `b82063118` slice 1, `54afaaf16` DEC-076's lock, `930329fec` slice 2 + DEC-078,
           `ab0bb7586` -126/-125/-124, `4dacd8447` -127, `e15d863ba` -111/-133. Uncommitted and
           held: slice 3's +575/-5 (awaiting repair) plus findings -152..-156.
TEAM:      on(5 agents)
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   WIKI ~2,550tok/700 [BREACH] · DECIDX ~8kB · ROWS 1 over 4kB (DEC-029 slice) · ledgers
           compacted 2026-09-28: findings 76kB, decisions 134kB, traceability 48kB
