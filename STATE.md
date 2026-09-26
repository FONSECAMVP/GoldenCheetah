# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-27 by `garmin_inspector_v1_64`
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# ALL superseded cursor narrative -> .claude/workflow-garminconnect/archive/state-history.md
#   (§ 1-11 = history through the 2026-09-06 compaction; § 12 = that pass's BUDGETS/git-truth
#   detail; § 13 = the FULL 3415-line/290,364-char STATE.md this file replaces, extracted
#   VERBATIM, md5 a81d6e6bcddda3c0f2e198bdc166f93c, re-verified after append — nothing deleted;
#   § 14 = the discharged commit-partition / push-mechanics narrative, removed 2026-09-26).
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58).
# Stage 1-8 history, the live-account run log, and the entire STAGE-9-CURSOR/-AMENDMENT chain
# (v1_21..v1_54) are in archive § 13.

PHASE:     2.2 · Garmin Connect integration, Stage 9 (real-account connect/MFA/sync/disconnect +
           installed-package smoke checklist). Stages 1-8 all discharged on executed evidence.
           Stage 9 is NOT closeable: the 2026-09-26 live run proved the sync journey fails on
           two real code defects (B-STAGE9-78/-79), and installer-provenance still needs a
           green AppVeyor run (DEC-065) that cannot be produced on this host.
OPEN:      Two blocking code defects from the live run, both Garmin-side, both decided:
             · B-STAGE9-78 (DEC-070/-072/-073) — the staged payload never reached the importer in a
               shape it could read. CODE IS DONE AND COMMITTED; mechanism, accept-set and round
               history -> findings.md, pinned by T-218..T-227. STILL OPEN, and the ONLY thing left:
               a live re-run raising the ride library above 1146. A green gate does NOT discharge
               it. That re-run needs the user's real Garmin account — a human-in-the-loop gate, not
               builder work.
               Frozen, NOT this branch's: `ArchiveFile.cpp`'s empty GZIP arm,
               `CloudService.cpp:565`'s `gUncompress`, and now `RideImportWizard`'s raw `Context*`
               (DEC-077 / B-STAGE9-116) — upstream GoldenCheetah defects, HARD HOLD every round.
               Pre-commit clang-format rewraps new C++: rebuild to prove it
               cosmetic, never `--amend`.
             · B-STAGE9-79 (DEC-071) — the dedup ledger records an activity at download time, so a
               cancelled import orphans it forever. THREE slices: slice 1 (store layer) COMMITTED
               `b82063118` + DEC-076's lock `54afaaf16`, reviewer-closed; slice 2 (both call routes +
               the dialog's completion seam via `RideCache::getRide`) in the tree, uncommitted; slice
               3 = the legacy migration, design settled by DEC-079. Under DEC-075 the store, not its
               callers, holds the invariant — slice 2 must NOT add a load-first fix at the call
               sites. Round history and residuals -> findings.md + DEC-076.
           The 8-commit partition is complete and pushed; upstream `origin/garmin/req028-row-
           lifetime` is set. HEAD's distance ahead is in CHANGESET. Detail -> archive § 14.
BLOCKING:
  B-STAGE9-78[CHECKPOINT:STAGE:9]
  B-STAGE9-79[CHECKPOINT:STAGE:9]
  B-STAGE9-111[STAGE:9]  B-STAGE9-112[STAGE:9]  B-STAGE9-121[STAGE:9]  B-STAGE9-126[STAGE:9]
  B-STAGE9-48[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-54[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-57[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-71[STAGE:9]
           # -48/-54/-57/-71 are ONE class and none is code: no local installer-build environment
           # (/dev/fuse, a linkable Python 3.11 embedding SDK, packaging tooling) and no CI
           # artifact run yet. They close on a green AppVeyor run post-PUSH, not on builder work.
           # This is `garmin-build-system-duality` in its final form.
           # B-STAGE9-71 residual: unpinned 7z version + unverified BadCmd NSIS profile.
           # Non-blocking, unlisted (BLOCKS:{}): B-STAGE9-73/-76/-77/-117..-120/-122..-125.
           # DEC-080 narrowed DEC-077: additive default-no-op hooks OK, repairs still frozen.
           # -116 is blocking but FROZEN upstream by DEC-077; it no longer holds Stage 9.
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py clean at last run)
LAST_CLV:  clv_findings.py 2026-09-27 (v1_64), after DEC-078's round landed in the tree and
           DEC-079 + B-STAGE9-123 were filed. 0 MALFORMED / 0 UNKNOWN-* / 0 NEEDS-DISPOSITION /
           **10 OUTSTANDING** / 530 rows. Honest floor: -78 awaits the live re-run, -79 awaits
           slices 2+3, -111 and -126 await their rounds, -112/-121 are code-fixed but held by
           B-STAGE9-125's test fidelity, 4 await AppVeyor.
           Editing a findings row: never `split("|")`/join — match on the row's TAIL and rebuild
           from it; escape pipes as `\|`.
NEXT_GATE: **THE CANONICAL GATE IS `ctest -LE gate-exclude` (DEC-054, dod.md:16-47). NOT
           `ctest -L garmin-fast`** — that label omits the i18n guards and is 40/40 GREEN on a
           tree whose real gate FAILS. Reading the right label is what found B-STAGE9-115.
           Slice 2 + DEC-078 + the -118/-120 comment repairs are COMMITTED: `ctest -LE gate-exclude`
           56/56 GREEN, Inspector-run, on the exact tree committed. All three agents idle at the
           commit, per B-STAGE9-106.
           NEXT BUILDER ROUND, ready to brief: **B-STAGE9-126 (blocking) + -124 + -125**, one round,
           all in the controller and its test. -126 is the real defect: the store rewinds only on
           `==` the cursor, so dropping a pending entry STRICTLY BEFORE it loses that activity for
           ever. Remedy is inside DEC-078's shape — widen `==` to `<=`, never a greatest-surviving
           rewind. -125 (the fake ignores `sinceGmt`) is what holds -112/-121 open; -124 is prose.
           THEN B-STAGE9-111, now fully builder-ready: DEC-080 narrowed DEC-077's freeze to permit
           ONE additive default-no-op `CloudService` virtual, and DEC-080 names the 6 abandonment
           paths it must not fire on plus T-048's new contract.
           LESSON of this session, five instances (-118/-120/-123/-124/-126): a comment asserting a
           checkable fact the code contradicts is not cosmetic. -124's made an independent reviewer
           file a FALSE BLOCKING verdict (it read `IGarminDownloadClient.h:52-57`'s "newer than"
           prose; production lists INCLUSIVELY, `garmin_client.py:430` `>= cursor`, deliberate since
           B-STAGE9-25 r3). -126's was load-bearing for a fix's correctness. Read the code, not the
           comment, before accepting ANY verdict — including a reviewer's.
           THEN B-STAGE9-111, seam settled `NEEDS-NEW-SEAM` (its row carries the trace): no promotion
           site exists in generic CloudService code, and the route keeps NO durable staged file, so
           slice 2's re-offer-the-bytes shape does not transfer — an unpromoted entry must be
           re-downloaded. T-048 asserts `imported.contains("BBB")` right after `readFile()`, so -111
           changes that test's contract, not just its code. THEN slice 3, design settled by DEC-079:
           dialog pre-start self-classification, keyed on `schema_version` absent == v0, tests from
           T-240. -79 closes on neither route alone. -107 and -115 are closed both ways, gate and independent read.
           A `setTimeSpec(Qt::UTC)` deprecation in `parsePendingStartTimeUtc` is unfiled — judge
           it yourself.
           Slice 3 is NOT optional: until it lands, a legacy `imported` row still reads as complete
           and -79's orphans survive for every existing athlete.
           -78 and -79 both come from the first live run on the qmake binary from `2e6e122a7`:
           CONNECT, MFA (2FA account) and DISCONNECT pass; SYNC FAILS, library stuck at 1146.
           -79's completion seam is settled by measurement: the three live sidecar entries match
           their imported/inner-FIT timestamps to the second, so the exact compare is safe and no
           tolerance window may be added without new measurement (DEC-071).
           The pattern that keeps holding: a green gate has shipped a blocking defect four times
           (-92, -101, -111..-114), each found by READING THE DIFF. Never close on green.
           Source-tree runs need `GC_GARMIN_PYPATH=<repo>/src/Python/garminconnect` or auth dies
           as `code: unknown` (ModuleNotFoundError). The AppImage ships the adapter.
           Keep the two live staged files under `~/.goldencheetah/Andy/config/garminconnect/
           backfill/` — real ZIP payloads and the -78 regression fixture.
           Installer class: -48/-54/-57/-71 close only on a green AppVeyor run, unobservable here.
           Neither DEC-069 CI arm has ever executed, so a failure is expected-cost, not regression.
           NEVER commit the untracked local artifacts `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: HEAD `ab78fe1f8`, 13 commits ahead of `origin/garmin/req028-row-lifetime` (count it with
           `git rev-list --count origin/garmin/req028-row-lifetime..HEAD`; the master-relative number
           is 133 and means nothing here).
           Last two code commits: `b82063118` slice 1 (gated 56/56), `54afaaf16` DEC-076's lock
           (gated 58/58, reviewer-closed). Inspector lineage is in this file's git history; the
           successor retires the predecessor, never itself, and closes its pane AND tab.
           Dirty: NOTHING tracked once this ledger commit lands. B-STAGE9-106 still binds: pre-
           commit stashes the tree, so never commit while a builder round is in flight.
TEAM:      builder `garmin_builder_stage9_v35` (w1:pM, Claude/Sonnet, 118k) · reviewer `garmin_codex_reviewer`
           (w1:pD, Codex, 96k; brief it COLD every round) · investigator
           `s979_record_split_investigator` (w1:pR, Codex, 96k) · Inspector
           `garmin_inspector_v1_64` (w1:p31, tab w1:t2S, Opus, auto); v1_63 retired, pane and tab closed. Codex soft-lands via `/new` (same pane/PID/name);
           the Claude builder needs `/exit` then `agent start ... -- --permission-mode auto
           --model sonnet`, and a soft-landed pane reads `unknown` until its first turn writes a
           transcript — expected, not a fault. Numbers go stale fast — re-read each pane
           with scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58:
           an unchanging value across wakes is a stale-session symptom, not a stable one).
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE (this file) 12.0kB/12kB cap [ok] [AT THE LINE — every add needs a matching trim
           of discharged narrative] · WIKI 16,350B [BREACH, next compaction target] · FINDINGS
           worst row 9,191B (B-STAGE9-16) vs DEC-074's 1,200B hard / 600B soft; [BREACH]
           deliberately, B-STAGE9-97 carries the ~50-row follow-up pass. `insp_wake.sh` still
           prints the retired 200B cap — B-STAGE9-105 · DECIDX 76 rows/500 [ok] · LSN unmeasured.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC80 (next:garmin-081) · DES14+2 lettered
           (next:garmin-015) · TEST max T-239 (next:T-240) · findings 530 rows, max id
           B-STAGE9-126 (next:B-STAGE9-127 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace). The reviewer
           numbers its own findings and they COLLIDE every round — its r2rev -115/-116 became
           -116/-117. Always re-number a reviewer's ids.
           B-STAGE9-95 owns the T-212..T-234 traceability-cell backfill; not each unit's.
