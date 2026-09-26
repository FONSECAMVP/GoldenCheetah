# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-26 by `garmin_inspector_v1_57`
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
               in a shape it could read. Rounds 1-3 each fixed one shape and the reviewer found
               the next; round 4 hit the 3-of-3 repair cap, discharged as remedy (a) = DEC-073,
               which INVERTS the predicate. The accept-set is now closed and independently
               confirmed: `GarminBackfillController.cpp` inflates a gzip member once, then
               accepts ONLY ZIP- or FIT-signed bytes and otherwise pauses as
               `PauseReason::UndecodablePayload` before staging, `recordImported` and the cursor
               advance. The reviewer's B-STAGE9-92 delta-check named no byte sequence that can
               reach `AtomicFile::writeOver` unsigned. Closed on that evidence: B-STAGE9-83,
               -86, -89, -90, -92, -93, -94. Pinned by T-218..T-224.
               STILL OPEN, and the ONLY thing left on this row: a live re-run that actually
               raises the ride library above 1146. A green gate does not discharge it — four
               green suites in a row missed this class, and the last blocking gap (B-STAGE9-92)
               was found by the Inspector READING THE DIFF, not by any test.
               B-STAGE9-91's residue is discharged in findings.md: T-225/T-226 pin the false
               positives and T-227 (a 13-byte `.FIT`-at-8 buffer) forces the length literal
               itself, mutation-isolated. The reviewer found nothing blocking, so the
               consecutive-same counter never reached 2. Gate 56/56 on that tree.
               Frozen, and NOT this branch's to fix: `ArchiveFile.cpp`'s empty GZIP arm and
               `CloudService.cpp:565`'s `gUncompress` — real upstream GoldenCheetah defects.
               Gate was green on this tree before the commit: `cmake --build . -j2` exit 0 and
               `ctest -LE gate-exclude` exit 0, **56/56, 0 failed, 318.6s**, logs at
               `/tmp/insp-exchange/gate2-{build,ctest}.log`. A green gate is NOT permission to
               close B-STAGE9-78 (see above), only to commit.
               The 20 code/test/`.ts` files are committed `02248b0d6`. pre-commit's clang-format
               rewrapped 3 of them (whitespace only); confirmed cosmetic by rebuilding both
               affected targets, 2/2 pass (`/tmp/insp-exchange/postformat-*.log`).
               NEXT: B-STAGE9-79 under DEC-071, dispatched in TWO slices against the one DEC —
               its remedy spans 5 files and 4 separable concerns, which is more than one atomic
               builder turn. Slice 1 = the store layer (`GarminSidecarStore.{h,cpp}` gains the
               versioned `pending` manifest; tests in `testGarminSidecarStore.cpp`). Slice 2 =
               both call routes + the dialog's completion evaluation, and ONLY slice 2 may do
               DEC-071's renames — renaming `recordImported` while its callers are frozen
               breaks the build.
               Builder replies overflow the pane cap every round; the
               `/tmp/insp-exchange/<unit>.md` spill is expected, not a fault.
             · B-STAGE9-79 (DEC-071) — dedup ledger records an activity at download time, so a
               cancelled import orphans it forever. Remedy fully specified, BUILDER-READY,
               not yet dispatched.
           The 8-commit partition is complete and pushed; upstream `origin/garmin/req028-row-
           lifetime` is set and HEAD is ahead 1 (the B-STAGE9-78..81 ledger commit). Partition
           and push detail -> archive § 14.
BLOCKING:
  B-STAGE9-78[CHECKPOINT:STAGE:9]
  B-STAGE9-79[CHECKPOINT:STAGE:9]
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
LAST_CLV:  clv_findings.py 2026-09-26, re-run by v1_59 after the `02248b0d6` commit — unchanged.
           0 MALFORMED / 0 UNKNOWN-SEVERITY / 0 UNKNOWN-DISPOSITION / 0 NEEDS-DISPOSITION /
           **6 OUTSTANDING** / 492 OK over 498 rows. The 6 are exactly the BLOCKING set above.
           FAIL at OUTSTANDING=6 is the honest floor: B-STAGE9-78 awaits the live re-run, -79
           awaits its build, and 4 await the external AppVeyor run. A naive `split("|")`/join on
           a findings row overwrites the BODY on rows with no trailing pipe, and an unescaped
           `||` inside a quote splits a cell: locate the disposition cell relative to the
           `BLOCKS:` cell and escape pipes as `\|`.
NEXT_GATE: **B-STAGE9-78, then B-STAGE9-79.** Both are code, both blocking, both from the
           first live end-to-end run on the qmake binary rebuilt from `2e6e122a7` (`make` exit 0,
           `releaseModuleProvenanceLedger` in the binary — the CMake-only evidence gap is closed).
           What that run proved:
             · CONNECT passes; MFA PASSES, first live exercise ever, on a 2FA account —
               B-STAGE9-10's reversed-`resume_login` fix is proven; DISCONNECT passes twice,
               credentials deleted and ledgers preserved per REQ-012.
             · SYNC FAILS. Download/listing/dedup/resume all work; the staged file is a ZIP, so
               nothing imports. Ride library unchanged at 1146.
           B-STAGE9-78 closes on a reviewer delta-check PLUS a live re-run that actually raises
           the library above 1146 — a green ctest does not discharge it; every green gate in this
           project already missed this defect once. Proof of that: round 1 passed the canonical
           56-test gate (rebuild exit 0, 0 failed, 328s, verified by the Inspector itself) and
           the reviewer still found a blocking gap in it.
           B-STAGE9-79's completion seam is settled by measurement, not assumption: the three
           live sidecar entries match their imported/inner-FIT timestamps to the second, so
           `RideCache::getRide(startTimeGMT.toUTC())`'s exact compare is safe and no tolerance
           window may be added without new measurement (DEC-071).
           Source-tree runs need `GC_GARMIN_PYPATH=<repo>/src/Python/garminconnect` or auth dies
           as a bare `code: unknown` (ModuleNotFoundError). The AppImage does not — the adapter
           ships in its bundled site-packages.
           Keep the two live staged files under `~/.goldencheetah/Andy/config/garminconnect/
           backfill/` — real ZIP payloads from a real account, the B-STAGE9-78 regression fixture,
           and B-STAGE9-79 means the app can never re-offer them.
           Still true for the installer class: B-STAGE9-48/-54/-57/-71 close only on a green
           AppVeyor run, unobservable from this host. Neither DEC-069 CI arm (Windows 7z/NSIS,
           macOS hdiutil) has ever executed, so a failure there is expected-cost, not regression.
           NEVER commit the untracked local artifacts `python3.13-3.13.5/`,
           `python3.13_*.tar.xz`/`.dsc`/`.asc`, `FITmetadata.json`.
CHANGESET: HEAD `02248b0d6` (the B-STAGE9-78 fix, ahead 3 of upstream). Inspector lineage
           v1_55..v1_58 and what each filed is recoverable from this file's own git history and
           the B-STAGE9-78..94 rows; v1_58 retired at 217k of 210k, its pane closed by v1_59.
           Dirty: decisions/findings/STATE only (this ledger pass).
TEAM:      builder `garmin_builder_stage9_v32` (w1:pM, Claude/Sonnet, 0k — v31 soft-landed at
           217k) · reviewer `garmin_codex_reviewer` (w1:pD, Codex, 64k after a `/new` at 211k;
           brief it COLD every round) · investigator `s979_record_split_investigator` (w1:pR,
           Codex, 156k, idle) · Inspector `garmin_inspector_v1_59` (w1:p2W, Opus). A soft-landed
           pane reads `unknown` until its first turn writes a transcript — expected, not a
           fault. Numbers go stale fast — re-read each pane
           with scripts/claude_context.py / codex_context.py before trusting one (B-STAGE9-58:
           an unchanging value across wakes is a stale-session symptom, not a stable one).
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE (this file) 10.2kB/12kB cap [ok, recompacted this pass] · WIKI 16,284B
           [BREACH, next compaction target] · FINDINGS worst single row 19,477B/200B cap
           [BREACH, flagged since 2026-09-06] · DECIDX active index 109 lines/500 [ok] ·
           LSN/DECIDX whole-file sizes not measured this pass.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC73 (next:garmin-074) · DES14+2 lettered
           (next:garmin-015) · TEST max T-227 (next:T-228) · findings 499 rows, max id
           B-STAGE9-95 (next:B-STAGE9-96 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace).
           B-STAGE9-95: traceability.md's TEST cells stop at T-211 while T-212..T-227 are built
           and committed. Non-blocking, deliberately deferred to the next ledger seam.
