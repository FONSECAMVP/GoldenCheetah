# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-27 by `garmin_inspector_v1_60`
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
             · B-STAGE9-78 (DEC-070/-072/-073) — the staged payload never reached the importer
               in a shape it could read. CODE IS DONE AND COMMITTED: `GarminBackfillController.cpp`
               inflates a gzip member once, then accepts ONLY ZIP- or FIT-signed bytes and
               otherwise pauses as `PauseReason::UndecodablePayload` before staging,
               `recordImported` and the cursor advance. Accept-set closed on the reviewer's
               B-STAGE9-92 delta-check; pinned by T-218..T-227. Round history -> findings.md.
               STILL OPEN, and the ONLY thing left on this row: a live re-run that actually
               raises the ride library above 1146. A green gate does NOT discharge it — four
               green suites in a row missed this class, and the last blocking gap (B-STAGE9-92)
               was found by the Inspector READING THE DIFF, not by any test. That re-run needs
               the user's real Garmin account: it is a human-in-the-loop gate, not builder work.
               Frozen, NOT this branch's to fix: `ArchiveFile.cpp`'s empty GZIP arm and
               `CloudService.cpp:565`'s `gUncompress` — real upstream GoldenCheetah defects.
               Committed `02248b0d6`; T-227 `81fd6d4e8`; both gated 56/56. Expect pre-commit
               clang-format to rewrap new C++ — rebuild the target to prove the rewrap cosmetic
               before re-staging, never `--amend`.
             · B-STAGE9-79 (DEC-071) — the dedup ledger records an activity at download time, so
               a cancelled import orphans it forever. TWO slices against the one DEC: the remedy
               spans 5 files and 4 separable concerns, more than one atomic builder turn.
               Slice 1 (store layer: `GarminSidecarStore.{h,cpp}` + `testGarminSidecarStore.cpp`)
               is built and through repair round 1 under DEC-075 — the store, not its callers,
               holds the invariant that `pending` survives a cursor write. The reviewer closed
               B-STAGE9-101/-102/-103 on its round-2 delta-check and raised two NEW blockers:
               -107 (the refusal is recoverable but its message is not, and that message lives in
               a slice-2 file — routed there, NOT a slice-1 repair) and -108 (the writers are now
               load-modify-write and nothing serializes them; `s979_record_split_investigator` is
               establishing whether two writers for one uid can actually overlap before a lock or
               a pin is chosen). The 3 dirty paths are still the only dirty paths in the tree.
               Slice 2 = both call routes (`GarminBackfillController.cpp` skip predicate + record
               order, `GarminConnect.cpp:906-931`) + the dialog's completion evaluation via
               `RideCache::getRide(startTimeGMT.toUTC())`. ONLY slice 2 may do DEC-071's renames:
               renaming `recordImported` while its callers are frozen breaks the build. Slice 2
               must NOT add a load-first fix at the call sites — DEC-075 rejected that option and
               makes it dead code.
               Both slice-1 rounds' replies fit the pane; the spill is an exception, not the norm.
           The 8-commit partition is complete and pushed; upstream `origin/garmin/req028-row-
           lifetime` is set. HEAD's distance ahead is in CHANGESET. Detail -> archive § 14.
BLOCKING:
  B-STAGE9-78[CHECKPOINT:STAGE:9]
  B-STAGE9-79[CHECKPOINT:STAGE:9]
  B-STAGE9-107[STAGE:9]
  B-STAGE9-108[STAGE:9]
  B-STAGE9-48[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-54[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-57[CHECKPOINT:STAGE9;RELEASE]
  B-STAGE9-71[STAGE:9]
           # B-STAGE9-83/-86/-89/-90/-92 were ONE defect class (payload shape reaching the
           # importer); all five are closed in findings.md on the reviewer's B-STAGE9-92
           # delta-check, so only -78 remains from that class and only on live evidence.
           # The last four are ONE class and none is code: there is no local installer-build
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
LAST_CLV:  clv_findings.py 2026-09-27 (v1_60), after B-STAGE9-101..108.
           0 MALFORMED / 0 UNKNOWN-SEVERITY / 0 UNKNOWN-DISPOSITION / 0 NEEDS-DISPOSITION /
           **8 OUTSTANDING** / 504 OK over 512 rows. The 8 are exactly the BLOCKING set above.
           FAIL at OUTSTANDING=8 is the honest floor: -78 awaits the live re-run, -107 is slice
           2's, -108 awaits the concurrency trace, -79 awaits both slices, 4 await AppVeyor.
           A naive `split("|")`/join on
           a findings row overwrites the BODY on rows with no trailing pipe, and an unescaped
           `||` inside a quote splits a cell: locate the disposition cell relative to the
           `BLOCKS:` cell and escape pipes as `\|`.
NEXT_GATE: **B-STAGE9-78, then B-STAGE9-79.** Both are code, both blocking, both from the
           first live end-to-end run on the qmake binary rebuilt from `2e6e122a7` (`make` exit 0,
           `releaseModuleProvenanceLedger` in the binary — the CMake-only evidence gap is closed).
           That run proved CONNECT, MFA (first live exercise ever, 2FA account) and DISCONNECT
           all pass; SYNC FAILS with the library unchanged at 1146. Detail -> archive § 13.
           B-STAGE9-78 closes on a reviewer delta-check PLUS a live re-run that actually raises
           the library above 1146 — a green ctest does not discharge it; every green gate in this
           project already missed this defect once. Proof of that: round 1 passed the canonical
           56-test gate (rebuild exit 0, 0 failed, 328s, verified by the Inspector itself) and
           the reviewer still found a blocking gap in it.
           B-STAGE9-79's completion seam is settled by measurement, not assumption: the three
           live sidecar entries match their imported/inner-FIT timestamps to the second, so
           `RideCache::getRide(startTimeGMT.toUTC())`'s exact compare is safe and no tolerance
           window may be added without new measurement (DEC-071).
           Slice 1 repeated the pattern: 56/56 green and three blocking findings anyway.
           B-STAGE9-101 was visible in the diff and invisible to every test.
           Source-tree runs need `GC_GARMIN_PYPATH=<repo>/src/Python/garminconnect` or auth dies
           as a bare `code: unknown` (ModuleNotFoundError). The AppImage ships the adapter.
           Keep the two live staged files under `~/.goldencheetah/Andy/config/garminconnect/
           backfill/` — real ZIP payloads from a real account, the B-STAGE9-78 regression fixture,
           and B-STAGE9-79 means the app can never re-offer them.
           Still true for the installer class: B-STAGE9-48/-54/-57/-71 close only on a green
           AppVeyor run, unobservable from this host. Neither DEC-069 CI arm (Windows 7z/NSIS,
           macOS hdiutil) has ever executed, so a failure there is expected-cost, not regression.
           NEVER commit the untracked local artifacts `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: HEAD `1142a2ff6` (ahead 6 of upstream): `02248b0d6` B-STAGE9-78 code, `d58afde4a`
           DEC-071..073, `81fd6d4e8` T-227, `def6aabcb` DEC-074, `1142a2ff6` cursor. Lineage
           v1_55..v1_60 is in this file's git history; v1_58 retired at 217k, v1_59 at 211k by
           v1_60 (the successor retires the predecessor, never itself).
           Dirty: ONLY B-STAGE9-79 slice 1's 3 files. Nothing staged.
TEAM:      builder `garmin_builder_stage9_v32` (w1:pM, Claude/Sonnet, 89k) · reviewer
           `garmin_codex_reviewer` (w1:pD, Codex, 115k; brief it COLD every round) · investigator
           `s979_record_split_investigator` (w1:pR, Codex, 156k, idle) · Inspector
           `garmin_inspector_v1_60` (w1:p2X, Opus, auto). Codex soft-lands via `/new` (same pane/PID/name);
           the Claude builder needs `/exit` then `agent start ... -- --permission-mode auto
           --model sonnet`, and a soft-landed pane reads `unknown` until its first turn writes a
           transcript — expected, not a fault. Numbers go stale fast — re-read each pane
           with scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58:
           an unchanging value across wakes is a stale-session symptom, not a stable one).
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE (this file) 12.0kB/12kB cap [at the line — trim discharged narrative before
           adding] · WIKI 16,350B [BREACH, next compaction target] · FINDINGS worst single row
           9,191B (B-STAGE9-16) against DEC-074's register-specific 1,200B hard / 600B soft pair
           (live median 496B, p90 1,793B). Still [BREACH] deliberately: B-STAGE9-97 carries the
           ~50-row follow-up pass. `insp_wake.sh` still prints the retired 200B cap — B-STAGE9-105
           · DECIDX active index 76 rows/500 [ok] · LSN not measured.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC75 (next:garmin-076) · DES14+2 lettered
           (next:garmin-015) · TEST max T-227, T-228..232 reserved for slice 1's 5 new slots
           (next:T-233) · findings 512 rows, max id
           B-STAGE9-108 (next:B-STAGE9-109 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace).
           B-STAGE9-95: traceability.md's TEST cells stop at T-211 while T-212..T-227 are built
           and committed; slice 1's 5 new slots are T-228..T-232, also uncelled. Non-blocking.
