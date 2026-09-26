# STATE — GoldenCheetah (garmin/req028-row-lifetime)   updated: 2026-09-26 by `garmin_inspector_v1_56`
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
OPEN:      Two blocking code defects from the live run, both Garmin-side:
             · B-STAGE9-78 — staged payload is a ZIP under a `.fit` name, so nothing imports.
               Remedy decided in DEC-070; builder round in flight.
             · B-STAGE9-79 — dedup ledger records an activity at download time, so a cancelled
               import orphans it forever. Investigator drafting the three-option remedy report.
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
LAST_CLV:  clv_findings.py 2026-09-26 (`garmin_inspector_v1_56`, re-run on its own orientation).
           0 MALFORMED / 0 UNKNOWN-SEVERITY / 0 UNKNOWN-DISPOSITION / 0 NEEDS-DISPOSITION /
           **6 OUTSTANDING** / 479 OK over 485 rows. The 6 are the BLOCKING set above. FAIL at
           OUTSTANDING=6 is the honest floor: 2 await builder work, 4 await the external run.
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
           project already missed this defect once.
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
CHANGESET: HEAD `182f6a587` (ahead 1 of upstream). `garmin_inspector_v1_55` filed
           B-STAGE9-78..81 from the live run and committed them, then was retired at 255k.
           `garmin_inspector_v1_56` took DEC-070 (B-STAGE9-78 remedy) and dispatched the
           B-STAGE9-78 builder round and the B-STAGE9-79 investigation. Nothing staged.
TEAM:      builder `garmin_builder_stage9_v30` (w1:pM, Claude/Sonnet, dispatched from 0k) ·
           reviewer `garmin_codex_reviewer` (w1:pD, Codex, 153k) · investigator
           `s979_record_split_investigator` (w1:pR, Codex, 30k, renamed from `s925_tz_`) ·
           Inspector `garmin_inspector_v1_56` (w1:p2S, Opus, ~50k of 210k). Numbers go stale
           fast — re-read each pane with scripts/claude_context.py / codex_context.py before
           trusting one (B-STAGE9-58: an unchanging value across wakes is a stale-session
           symptom, not a stable one).
RIGOR:     full (Phase 0 backfill 2026-07-11; A0-A5 + STRIDE + per-slice CLV)
BUDGETS:   STATE (this file) ~7kB/12kB cap [FIXED this pass, was 290,364B] · WIKI 16,284B
           [BREACH, next compaction target] · FINDINGS worst single row 19,477B/200B cap
           [BREACH, flagged since 2026-09-06] · DECIDX active index 109 lines/500 [ok] ·
           LSN/DECIDX whole-file sizes not measured this pass.
COUNTS:    REQ29+16 REQ-NF (next:garmin-030) · DEC70 (next:garmin-071) · DES14+2 lettered
           (next:garmin-015) · TEST~T-211 (next:garmin-T-212) · findings 485 data rows, max id
           B-STAGE9-81 (next:B-STAGE9-82 — grep `B-STAGE9-[0-9]+` for the true max before
           allocating; the REGISTRIES pointer does not cover this namespace).
