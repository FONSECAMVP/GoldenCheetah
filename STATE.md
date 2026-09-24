# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-25 by `garmin_inspector_v1_55`
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT).
# For a DEC's status read decisions.md. For a finding's severity/disposition read findings.md.
# ALL superseded cursor narrative -> .claude/workflow-garminconnect/archive/state-history.md
#   (§ 1-11 = history through the 2026-09-06 compaction; § 12 = that pass's BUDGETS/git-truth
#   detail; § 13 = the FULL 3415-line/290,364-char STATE.md this file replaces, extracted
#   VERBATIM, md5 a81d6e6bcddda3c0f2e198bdc166f93c, re-verified after append — nothing deleted).
# This file carries ONLY the Tier-0 cursor schema (references/state-and-tiers.md line 43-58).
# Stage 1-8 history, the live-account run log, and the entire STAGE-9-CURSOR/-AMENDMENT chain
# (v1_21..v1_54) are in archive § 13.

PHASE:     2.2 · Garmin Connect integration, Stage 9 (real-account connect/MFA/sync/disconnect +
           installed-package smoke checklist). Stages 1-8 all discharged on executed evidence.
           Stage 9's code queue is wound down and its commit partition is complete except for
           this ledger commit. Installer-provenance — the actual Stage 9 acceptance criterion —
           is NOT met and cannot be met on this host: no AppImage/installer builds here. It
           closes only on a green AppVeyor run after the push (DEC-065).
OPEN:      Commit partition, 8 planned, 5 and 6 merged into one (they share
           `unittests/Core/garminconnect/CMakeLists.txt`, which interleaves DEC-058 and DEC-062
           so splitting yields a commit whose tests cannot compile):
             1 `224937824` tooling · 2 `496af3faa` Coach+UI · 3 `803d2e359` build/libusb ·
             4 `e7b39549c` DEC-063 · 5+6 `577475bac` merged Garmin set, 22 files ·
             7 `a6fca237e` CI (appveyor.yml + after_build.sh).
           REMAINING: commit 8 = ledgers, then PUSH. The 5 `.claude/evidence-seals/
           DEC040-Stage2-*.log` files B-STAGE9-74 restored need no commit — they are byte-present
           at HEAD and the restore returned them to that state.
           Partition trap, resolved, do not re-introduce: an earlier note called `appveyor.yml` a
           mixed Coach/Garmin file needing a hunk split. It is not — without the `:71` QTDIR fix
           the Linux leg dies before qmake, so every hunk is Stage 9 CI. Committed whole.
BLOCKING:
  B-STAGE9-48[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-54[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-57[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-71[STAGE:9]
           # All four are ONE class and none is code: there is no local installer-build
           # environment (/dev/fuse, a linkable Python 3.11 embedding SDK, packaging tooling)
           # and no CI artifact run yet. They close on a green AppVeyor run post-PUSH, not on
           # builder work. This is `garmin-build-system-duality` in its final form.
           # B-STAGE9-71 residual: the image's 7z version is unpinned and the Windows
           # installer's own BadCmd NSIS profile is unverified until that run.
           # Non-blocking and deliberately not listed (BLOCKS:{}): B-STAGE9-73 (masked hdiutil
           # detach, accepted as-is — PY_RC is captured first so it cannot mask a failed
           # assertion), B-STAGE9-76 (gate-procedure corrections), B-STAGE9-77 (clang-format
           # scope note on the i18n markers).
CASCADE:   — (DEC-015 fully propagated; ledger_drift_lint.py clean at last run)
LAST_CLV:  clv_findings.py 2026-09-24 (`garmin_inspector_v1_55`, post-B-STAGE9-75 closure).
           0 MALFORMED / 0 UNKNOWN-SEVERITY / 0 UNKNOWN-DISPOSITION / 0 NEEDS-DISPOSITION /
           **4 OUTSTANDING** / 477 OK over 481 rows. The 4 are the BLOCKING set above. The gate
           reports FAIL at OUTSTANDING=4, and that is its floor until the AppVeyor run — a FAIL
           here means "blocked on an external run", not "unfinished work". Full history of every
           prior CLV run -> archive § 13.
NEXT_GATE: **THE PUSH — the human-in-the-loop STOP gate** (autonomy-boundary.md: pushing to a
           shared remote and modifying CI both need the user). Everything before it is done:
           commit 5's gate ran clean and commit 7 landed. Remaining Inspector work is commit 8
           (ledgers) only.
           Commit-ready evidence for 5 and 7, executed not claimed: `ctest -LE gate-exclude`
           (DEC-054's canonical default-include run) **56 tests, 0 failed, 315s**, re-run after
           the clang-format hook's own reformat plus a full rebuild to confirm that reformat was
           cosmetic. Labels reached: garmin-fast 40, garmin-py 5, gate-guard 2, i18n-guard 2,
           lint-guard 2, sec-guard 2, stderr-buf 2 (=55) plus 1 test carrying no `garmin-*`
           label at all — exactly the reach DEC-054 bought and a union-of-labels run misses.
           Count source is the OUTER ctest's stdout summary. NOT `LastTest.log`:
           `testGarminGateCoverageGuard`'s own `--show-only=json-v1` rewrites that file as an
           empty run in the same build dir (B-STAGE9-76).
           NEVER commit the untracked local artifacts `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
           After the push: the AppVeyor run is the evidence that closes B-STAGE9-48/-54/-57/-71.
           Neither DEC-069 CI arm (Windows 7z/NSIS, macOS hdiutil) has ever executed — they are
           unrunnable on a Linux host by design, so their first run is that one, and a failure
           there is expected-cost, not regression.
CHANGESET: HEAD `a6fca237e`. This session (`garmin_inspector_v1_55`) closed B-STAGE9-75 (i18n
           guard markers, reviewer delta-check clean), ran the canonical gate for the first time,
           landed commits 5 and 7, filed B-STAGE9-77, and took the librarian compaction that
           produced this file. Uncommitted at this line: the ledgers themselves (commit 8).
TEAM:      builder `garmin_builder_stage9_v30` (w1:pM, Claude/Sonnet, soft-landed fresh at 0k) ·
           reviewer `garmin_codex_reviewer` (w1:pD, Codex, 153k) · investigator
           `s925_tz_investigator` (w1:pR, Codex, 30k) · Inspector `garmin_inspector_v1_55`
           (w1:p2Q, Opus, ~80k of 210k). Numbers go stale fast — re-read each pane with
           scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58: an
           unchanging value across wakes is a stale-session symptom, not a stable one).
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE (this file) ~7kB/12kB cap [FIXED this pass, was 290,364B] · WIKI 16,284B
           [BREACH, next compaction target] · FINDINGS worst single row 19,477B/200B cap
           [BREACH, flagged since 2026-09-06] · DECIDX active index 109 lines/500 [ok] ·
           LSN/DECIDX whole-file sizes not measured this pass.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC69 (next:garmin-070) · DES14+2 lettered
           (next:garmin-015) · TEST~T-211 (next:garmin-T-212) · findings 481 data rows, max id
           B-STAGE9-77 (next:B-STAGE9-78 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace).
