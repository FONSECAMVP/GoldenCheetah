# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-27 by `garmin_inspector_v1_65`
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58). ALL
# superseded narrative is in archive/state-history.md, whose own header maps its sections: Stage 1-8,
# the live-account run log, and the v1_21..v1_54 cursor chain are § 13, commit-partition § 14.

PHASE:     2.2 · Garmin Connect integration, Stage 9 (real-account connect/MFA/sync/disconnect +
           installed-package smoke checklist). Stages 1-8 all discharged on executed evidence.
           Stage 9 is NOT closeable: the 2026-09-26 live run proved the sync journey fails on
           two real code defects (B-STAGE9-78/-79), and installer-provenance still needs a
           green AppVeyor run (DEC-065) that cannot be produced on this host.
OPEN:      Two blocking code defects from the live run, both Garmin-side, both decided:
             · B-STAGE9-78 (DEC-070/-072/-073) — the staged payload never reached the importer in a
               shape it could read. CODE IS DONE AND COMMITTED; mechanism and round history ->
               findings.md, pinned by T-218..T-227. STILL OPEN, and the ONLY thing left: a live
               re-run raising the ride library above 1146. A green gate does NOT discharge it, and
               that re-run needs the user's real Garmin account — a human-in-the-loop gate.
               Frozen, NOT this branch's: `ArchiveFile.cpp`'s empty GZIP arm,
               `CloudService.cpp:565`'s `gUncompress`, and `RideImportWizard`'s raw `Context*`
               (DEC-077/-080 / B-STAGE9-116) — upstream defects, HARD HOLD every round.
             · B-STAGE9-79 (DEC-071) — the dedup ledger records an activity at download time, so a
               cancelled import orphans it forever. THREE slices: slice 1 (store layer) COMMITTED
               `b82063118` + DEC-076's lock `54afaaf16`, reviewer-closed; slice 2 (both call routes +
               the dialog's completion seam via `RideCache::getRide`) committed `930329fec`; slice
               3 = the legacy migration, design settled by DEC-079. Under DEC-075 the store, not its
               callers, holds the invariant — slice 2 must NOT add a load-first fix at the call
               sites. Round history and residuals -> findings.md + DEC-076.
           The 8-commit partition is complete and pushed; upstream `origin/garmin/req028-row-
           lifetime` is set. HEAD's distance ahead is in CHANGESET. Detail -> archive § 14.
BLOCKING:
  B-STAGE9-78[CHECKPOINT:STAGE:9]
  B-STAGE9-79[CHECKPOINT:STAGE:9]
  B-STAGE9-111[STAGE:9]  B-STAGE9-112[STAGE:9]  B-STAGE9-121[STAGE:9]
  B-STAGE9-48[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-54[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-57[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-71[STAGE:9]
           # -48/-54/-57/-71 are ONE class and none is code: no local installer-build environment
           # (/dev/fuse, a Python 3.11 embedding SDK, packaging tooling) and no CI artifact run yet.
           # They close on a green AppVeyor run post-PUSH — `garmin-build-system-duality` in its
           # final form. -71 residual: unpinned 7z + unverified BadCmd NSIS profile.
           # Non-blocking, unlisted (BLOCKS:{}): B-STAGE9-73/-76/-77/-117..-120/-122..-125/-128.
           # -126 and -127 are code-fixed and committed; their rows carry the traces.
           # DEC-080 narrowed DEC-077: additive default-no-op hooks OK, repairs still frozen.
           # -116 is blocking but FROZEN upstream by DEC-077; it no longer holds Stage 9.
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py clean at last run)
LAST_CLV:  clv_findings.py 2026-09-27 (v1_64): 0 MALFORMED / 0 UNKNOWN-* / 0 NEEDS-DISPOSITION /
           **10 OUTSTANDING** / 530 rows — STALE, predates -126/-127/-128 and DEC-081; re-run it.
           Honest floor: -78 awaits the live re-run, -79 awaits slice 3, -111 awaits its round,
           -112/-121/-125/-126/-127 are code-fixed and COMMITTED, 4 await AppVeyor.
           Editing a findings row: never `split("|")`/join — match on the row's TAIL and rebuild
           from it; escape pipes as `\|`.
NEXT_GATE: **THE CANONICAL GATE IS `ctest -LE gate-exclude` (DEC-054, dod.md:16-47). NOT
           `ctest -L garmin-fast`** — that label omits the i18n guards and is 40/40 GREEN on a
           tree whose real gate FAILS. Reading the right label is what found B-STAGE9-115.
           Slice 2 + DEC-078 + the -118/-120 repairs are committed `930329fec`/`1bb811c0c`, gated
           56/56 Inspector-run with all agents idle (B-STAGE9-106); the clang-format rewrap was
           proved cosmetic by a rebuild + re-gate at exit 0. Fix FORWARD, never `--amend`.
           **-126/-125/-124 committed `ab0bb7586`** and **-127 committed `4dacd8447`** — each on
           its own 56/56 `ctest -LE gate-exclude`, Inspector-run with all agents idle, each
           reviewer-read. Pre-commit clang-format rewrapped one QVERIFY2 per commit; rebuilt and
           reran that target each time to prove it cosmetic, then re-staged. Never `--amend`.
           -127 also carved out **B-STAGE9-128** (backfill timestamps are compared as TEXT at FIVE
           sites while the adapter forwards Garmin's original spelling). Latent, not live — every
           value in the real 2026-09-26 sidecars is `2026-09-26 09:54:01`, uniformly. Decided by
           **DEC-081**: canonicalise in the adapter, REJECT a non-zero fractional second (DEC-071's
           measured second-level compare must not be silently reshaped), rewrite sidecars once.
           NOT queued ahead of slice 3 or -111.
           -127 has NO residual round — the shipped shape is a scoped conditional; the reviewer's
           `start()`-stamps-v1 point is slice 3's under DEC-079's placement. Traces on the rows.
           B-STAGE9-129 (comment-only, `GarminSidecarStore.h` documents the retired `==`) rides
           the next commit; no builder round.
           NEXT: **B-STAGE9-111** (blocking, fully builder-ready under DEC-080), THEN slice 3
           (DEC-079 + its amendment, tests from T-240). Both are the blocking path to -79.
           LESSON, five instances (-118/-120/-123/-124/-126): a comment asserting a checkable fact
           the code contradicts is not cosmetic. Read the CODE, not the comment, before accepting
           any verdict — a reviewer's included, and its SEVERITY too (-128 was filed BLOCKING and
           re-set against real on-disk data). Rows carry the traces.
           THEN B-STAGE9-111, fully builder-ready — DEC-080 permits ONE additive default-no-op
           `CloudService` virtual and names the 6 abandonment paths it must not fire on. Seam is
           `NEEDS-NEW-SEAM` (its row): no promotion site exists in generic CloudService code and the
           route keeps NO durable staged file, so slice 2's re-offer-the-bytes shape does not
           transfer — an unpromoted entry must be re-downloaded. T-048 asserts
           `imported.contains("BBB")` right after `readFile()`, so -111 changes that test's
           contract, not just its code. THEN slice 3, design settled by DEC-079:
           dialog pre-start self-classification, keyed on `schema_version` absent == v0, tests from
           T-240. -79 closes on neither route alone.
           Slice 3 is NOT optional: a legacy `imported` row reads as complete without it.
           -78/-79 both come from the first live run on the qmake binary `2e6e122a7`: CONNECT, MFA
           and DISCONNECT pass; SYNC FAILS, library stuck at 1146. -79's completion seam is settled
           by measurement — the three live sidecar entries match their imported/inner-FIT
           timestamps to the second, so NO tolerance window may be added without new measurement
           (DEC-071).
           A green gate has shipped a blocking defect six times (-92, -101, -111..-114, -126, -127),
           each found by READING THE CODE. Never close on green.
           Source-tree runs need `GC_GARMIN_PYPATH=<repo>/src/Python/garminconnect` or auth dies
           as `code: unknown`. The AppImage ships the adapter.
           KEEP `~/.goldencheetah/Andy/config/garminconnect/backfill/`'s two live staged files —
           real ZIP payloads, and the -78 regression fixture.
           Neither DEC-069 CI arm has ever executed, so a failure there is expected-cost.
           NEVER commit the untracked `python3.13-3.13.5/`, `python3.13_*.tar.xz`/`.dsc`/`.asc`,
           `FITmetadata.json`.
CHANGESET: HEAD `4dacd8447`, 18 ahead of `origin/garmin/req028-row-lifetime` (`git rev-list --count
           origin/garmin/req028-row-lifetime..HEAD`; the master-relative 133 means nothing here).
           Code commits so far: `b82063118` slice 1, `54afaaf16` DEC-076's lock, `930329fec` slice 2
           + DEC-078, `ab0bb7586` -126/-125/-124, `4dacd8447` -127 — each gated 56/56+ and reviewer-read. Inspector
           lineage is in this file's git history. B-STAGE9-106 binds: pre-commit stashes the tree,
           so never commit while a builder round is in flight.
TEAM:      builder `garmin_builder_stage9_v36` (w1:pM, Claude/Sonnet; v35 soft-landed at 244.8k
           after reporting GREEN, relaunched `--permission-mode auto --model sonnet`, status line
           verified) · reviewer `garmin_codex_reviewer` (w1:pD, Codex; `/new`-reset at 207k
           2026-09-27, fresh prompt verified; brief it COLD every round) · investigator
           `s979_record_split_investigator` (w1:pR, Codex, 145k) · Inspector
           `garmin_inspector_v1_66` (w1:p33, Opus/auto); it closed v1_65's pane AND tab, workspace
           down to 4 panes / 2 tabs. Each successor retires its predecessor, never itself.
           Codex soft-lands via `/new` (same pane/PID/name) — VERIFY the fresh prompt before
           dispatching, a brief into a `/new` is silently swallowed. The Claude builder needs
           `/exit` then `agent start ... -- --permission-mode auto --model sonnet`; a soft-landed
           pane reads `unknown` until its first turn writes a transcript, expected. Numbers go stale fast: re-read each pane with
           scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58 — an
           unchanging value across wakes is a stale session, not a stable one).
           TRAP, cost v1_65 ~25k of overrun: the reader returned `unknown` ("no unique session-tasks
           fd") for the INSPECTOR'S OWN pane on every tick while it was really at 235k/210k.
           `unknown` on your own pane is NOT "probably fine" — cross-check the footer's `tok Nk` at
           once. Your own session id is the one in the background-task output path
           (/tmp/claude-1000/<project>/<session-id>/tasks/...), so you can sum the transcript's last
           assistant usage event yourself when the fd lookup fails.
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE 12kB cap, AT THE LINE — every add needs a matching trim of discharged narrative,
           and this file has been trimmed three times today to stay under · WIKI 16,350B [BREACH,
           next compaction target] · FINDINGS worst row 9,191B (B-STAGE9-16) vs DEC-074's 1,200B
           hard; [BREACH] deliberately, B-STAGE9-97 carries the ~50-row pass. `insp_wake.sh` prints
           the retired 200B row cap — B-STAGE9-105 · DECIDX 80 rows/500 [ok] · LSN unmeasured.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC81 (next:garmin-082) · DES14+2 lettered
           (next:garmin-015) · TEST max T-243 (next:T-244; T-240 reserved by DEC-079, so -111 takes T-244+ despite DEC-080's body) · findings 533 rows, max id
           B-STAGE9-129 (next:B-STAGE9-130 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace). Always re-number a reviewer's
           own finding ids; they collide every round.
           B-STAGE9-95 owns the T-212..T-234 traceability-cell backfill.
