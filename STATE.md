# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-27 by `garmin_inspector_v1_71`
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
LAST_CLV:  clv_findings.py 2026-09-27 (v1_71, fresh): 0 MALFORMED / 0 UNKNOWN-* / 0 NEEDS-
           DISPOSITION / **18 OUTSTANDING** / 555 rows. The script PRINTS them — read them from it,
           never from here. A disposition pass is owed at the next seam for every row that is committed
           or reviewer-closed yet still reads `open`; 4 are AppVeyor, not code.
           Editing a findings row: never `split("|")`/join — match on the row's TAIL and rebuild
           from it; escape pipes as `\|`.
NEXT_GATE: **CANONICAL GATE = `ctest -LE gate-exclude` (DEC-054, dod.md:16-47), NOT `ctest -L
           garmin-fast`** — that label omits the i18n guards and reads 40/40 GREEN on a tree whose
           real gate FAILS (B-STAGE9-115).
           Fix FORWARD, never `--amend`; never commit mid-round (B-STAGE9-106 — pre-commit stashes).
           **-128** (the latent text-compare class, DEC-081) is subsumed by DEC-084; **-129** is
           comment-only and rides the next commit.
           B-STAGE9-111 + -133 (DEC-083 watermark, DEC-084 instant primitive `src/Cloud/GarminTime.h`)
           are ACCEPTED and committed `e15d863ba` on reviewer PASS at unit B-STAGE9-133-r5-rev, gate
           57/57. Both censuses now return ZERO `startTimeGMT` text compares in `src/Cloud`.
           NEXT: **slice 3 on B-STAGE9-130 / DEC-079 placement + DEC-082 persistence, tests from
           T-240** — the last thing holding -79, and neither route closes it alone. Owed alongside it:
           the disposition pass on the 18 rows `clv_findings.py` still prints as OUTSTANDING (4 are
           AppVeyor, not code), and the reviewer is at 186k — `/new`-reset it on the unit change.
           LESSON (-132, -134 twice): scope a builder's PATHS to every target that COMPILES the changed
           contract. The full target set is on B-STAGE9-130's row — read it, do not re-derive it.
           LESSON, SEVEN instances (-118/-120/-123/-124/-126/-141/-142, and -129): a comment asserting
           a checkable fact the code contradicts is not cosmetic. Read the CODE before accepting any
           verdict — a reviewer's included, and its SEVERITY too: -128 and -137 were both filed
           BLOCKING and re-set against real on-disk data and the accepting row. Rows carry the traces.
           -111's built shape and its declared T-246 gap: its findings row.
           Slice 3 (DEC-079 placement + DEC-082 persistence, tests from T-240) is NOT optional: a legacy
           `imported` row reads as complete without it, and -79 closes on neither route alone.
           -78/-79 both come from the first live run on qmake `2e6e122a7`: CONNECT/MFA/DISCONNECT pass,
           SYNC FAILS, library stuck at 1146. -79's completion seam is settled
           by measurement — the three live sidecar entries match their imported/inner-FIT
           timestamps to the second, so NO tolerance window may be added without new measurement
           (DEC-071).
           A green gate has shipped a blocking defect EIGHT times (rows carry the instances, latest
           -148/-149 over an r4 gate the builder ran itself): never close on green, read the code.
           Source-tree runs need `GC_GARMIN_PYPATH=<repo>/src/Python/garminconnect` or auth dies
           as `code: unknown`. The AppImage ships the adapter.
           KEEP `~/.goldencheetah/Andy/config/garminconnect/backfill/`'s two live staged files —
           real ZIP payloads, and the -78 regression fixture.
           NEVER commit the untracked `python3.13-3.13.5/`, `python3.13_*.tar.xz`/`.dsc`/`.asc`,
           `FITmetadata.json`.
CHANGESET: HEAD `e15d863ba`, 23 ahead of `origin/garmin/req028-row-lifetime` (`git rev-list --count
           origin/garmin/req028-row-lifetime..HEAD`; the master-relative count means nothing here).
           Code commits so far: `b82063118` slice 1, `54afaaf16` DEC-076's lock, `930329fec` slice 2
           + DEC-078, `ab0bb7586` -126/-125/-124, `4dacd8447` -127, `e15d863ba` -111/-133 (DEC-083/-084)
           — each gated and reviewer-read.
           B-STAGE9-106 binds: pre-commit stashes the tree, so never commit mid-round.
TEAM:      builder `garmin_builder_stage9_v41` (w1:pM, Claude/Sonnet; v40 soft-landed pre-emptively at 178k
           between rounds -- a 4-item round does not fit 72k of headroom; relaunch verified Sonnet+auto on
           the pane's own status line, never on `agent start`'s reply) · reviewer
           `garmin_codex_reviewer` (w1:pD, Codex; brief it COLD every round) · investigator
           `s979_record_split_investigator` (w1:pR, Codex, `/new`-reset 2026-09-27 at 198k) ·
           Inspector `garmin_inspector_v1_72` (w1:p39/w1:t31, Opus/auto); v1_71 retired v1_70's pane AND
           tab. A successor started WITHOUT `--permission-mode auto` stops on a dialog whose FIRST option
           is already selected — one `down`, not two, reaches "switch to auto mode"; simpler to start it
           with the flag, as v1_72 was after that miss cost one relaunch. Each successor retires its predecessor, never itself.
           Codex `/new` then asks "where should the new conversation run?" — it needs a SECOND enter on
           option 1 (current checkout) or the next brief lands in that menu. Its footer `Context N%` is
           % USED, not remaining (0% verified fresh, 76% at the reader's 198k).
           Codex soft-lands via `/new` (same pane/PID/name) — VERIFY the fresh prompt before
           dispatching, a brief into a `/new` is silently swallowed. `dispatch.py --mode send` into an
           ALREADY-`working` pane exits 2 `send_failed`/timeout while the payload DID land and submit
           (the `--wait` probe has no status transition to see) — read the pane before re-sending, or a
           mid-round warn goes twice. The Claude builder needs
           `/exit` then `agent start ... -- --permission-mode auto --model sonnet`; a soft-landed
           pane reads `unknown` until its first turn writes a transcript, expected. Numbers go stale fast: re-read each pane with
           scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58 — an
           unchanging value across wakes is a stale session, not a stable one).
           TRAP that cost v1_65 ~25k: `unknown` on your OWN pane is NOT "probably fine" (it read unknown
           at a real 235k/210k) — cross-check the footer's `tok Nk` at once; your session id is in the
           background-task output path. A builder's sample FREEZES for a long turn: that is the turn.
           Reviewer held its r2-r5 context by DELIBERATE deviation from "brief it COLD" (r3+'s hunks are
           not isolable from the cumulative diff, and the agent that filed a finding checks its own
           repair). That unit is committed, so the deviation has EXPIRED: `/new` it before slice 3.
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE 12kB cap, AT THE LINE — every add needs a matching trim of discharged narrative
           · WIKI 10,184B, breach cleared 2026-09-27 (librarian Job-3; REGISTRIES now POINTs only)
           · FINDINGS worst row 9,191B (B-STAGE9-16) vs
           DEC-074's 1,200B; [BREACH] deliberately, B-STAGE9-97 carries the ~50-row pass and
           `insp_wake.sh` prints a retired 200B cap (B-STAGE9-105) · DECIDX 123/500 · LSN unmeasured.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC84 (next:garmin-085) · DES14+2 lettered
           (next:garmin-015) · TEST max T-263 incl. T-261b (next:T-264; T-240 reserved by DEC-079/-082; T-246 never written, declared gap) · findings 555 rows, max id
           B-STAGE9-151 (next:B-STAGE9-152 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace). Always re-number a reviewer's
           own finding ids; they collide every round.
           B-STAGE9-95 owns the T-212..T-234 traceability-cell backfill.
