# STATE — project cursor            (the live cursor; read WIKI.md first, then this)
# This file names WHERE we are: the active gate, the current blockers, the current evidence, the next gate.
# Per-id lifecycle status lives ONLY in .claude/workflow-garminconnect/traceability.md (DEC-015 SSOT),
# in that table's `Status (current)` column. For a DEC's status read decisions.md. For a finding's severity
# and disposition read findings.md. Nothing here duplicates those.
# ALL superseded cursor narrative → archive/state-history.md (§ 9 = the pre-2026-08-30 file, § 10 = the
# cursor this pass replaced, both extracted VERBATIM and round-trip verified; nothing was deleted).

TEAM: on (5 agents: scout, builder, adversary, validator, librarian)
RIGOR: FULL (Phase 0 backfill 2026-07-11; the project ran A0–A5 + STRIDE + per-slice CLV)

PHASE: Phase 2.2 — Garmin Connect integration, Phase 1 scope (activity DOWNLOAD only, DEC-001).

## VOCABULARY
This cursor uses one controlled vocabulary and never the words "done", "green", "complete", "ready" or
"shipped" unqualified. Each dimension is stated separately, because they are separately true or false:
NOT STARTED · PARTIAL · CODE COMPLETE · BUILD VERIFIED · TEST VERIFIED · VERIFIED IN WORKING TREE —
UNCOMMITTED · COMMITTED · MERGED · DEFERRED · BLOCKED · RELEASE BLOCKED · LIVE-SERVICE TESTED ·
CROSS-PLATFORM VERIFIED · PRODUCTION READY.

## CURRENT

**Code: DEC-040 is COMPLETE — Stage 1 COMMITTED at `37710370e` (parent `05ac6bc29`, 31 paths, +5780/−381);
Stage 2 (W3, cooperative cancellation) COMMITTED, BUILD VERIFIED and TEST VERIFIED at `fd7639f7a`** (2026-09-05,
8 paths incl. the Stage-2 test slots, +848/−26; watchdog 155→158→165→168/168 across the pieces, killing
mutations for T-147/T-148/T-149 independently re-executed with durable logs). It discharges C1/C2/C3/C4/C6
(reconciled at Stage-4 close, 2026-09-05) and carries `REQ-NF-Cancel-001` to TEST VERIFIED · COMMITTED.
**CORRECTED 2026-09-06:** an earlier revision of this paragraph read "Stage 2 NOT STARTED — no `CancelToken`
type exists" — that was STALE cursor drift: `fd7639f7a` landed without its STATE/traceability byproduct, and
both were repaired 2026-09-06. The parked auto-downloader teardown draft stays UNRELEASED (no id, per
DEC-040's ID NOTE).

**Code: DEC-033 is COMMITTED, BUILD VERIFIED and TEST VERIFIED at `2e26106c1`** (parent `37710370e`, same
branch, 31 paths — `src/Cloud/{CloudService,GarminConnect,Azum,CyclingAnalytics,Dropbox,LocalFileStore,
Nolio,PolarFlow,SixCycle,SportTracks,Strava,Xert}.{h,cpp}` + 6 `unittests/Core/garminconnect/` test files,
+856/−71). Closes `B-R027-01/02/03/09`; discharges REQ-027 clause (e). Execution evidence and the
RED-under-mutation proof are in `findings.md`'s `B-R027-09` row and `traceability.md`'s REQ-027 row.
Reviewed by an independent peer session (garmin_codex_new) before commit, which reproduced the same
mutation/ASan proof and made one non-functional comment fix (landed in this commit). Governance/ledger
files documenting this decision were deliberately NOT bundled into this commit — they travel with the
broader documentation-reconciliation slice below, matching this project's own precedent (`e48f7d123`
committed code-only, ledger separately). **Both commits: NOT PUSHED, NOT MERGED** (`git rev-parse @{u}`
fails; master does not contain either).

**Governance: the documentation-reconciliation + gate-repair + Gate-1A/1B-classification slice is COMMITTED
(2026-09-06, user decision D2) in this pass's second commit — the immediate child of `3b8226ec4` on
`garmin/req028-row-lifetime`.** Its content is exactly the 41 paths under `## COMMIT MANIFEST` below, at
their current bytes — this pass folded in, on manifest paths only: the `O-R027-01` reconciliation (user
decision D3), the `fd7639f7a` / `REQ-NF-Cancel-001` traceability repairs, the STATE/WIKI cursor-drift
corrections, and the `3b8226ec4` commit-chain citations. **Gate 1B PASSED 2026-09-03** (this line was stale
— see `## NEXT_GATE` for the current stage path; it is NOT still "Gate 1B failed"). It contains one
behavioural change to a tracked source file — the four `TIMEOUT`
properties in `unittests/Core/garminconnect/CMakeLists.txt`, now re-based from the unmeasured 900/600 onto
measured budgets of 420/180 — and **no change to any OTHER `src/` file** (DEC-033's `src/Cloud/` changes are
now committed separately, above, not part of this slice). It also adds the project's first executable,
self-tested CLV validator (`.claude/workflow-garminconnect/scripts/clv_findings.py` +
`test_clv_findings.py`, 18/18), repairs 29 finding rows that no mechanism could read, and makes all rows'
severity and disposition readable. **Current: 6 OUTSTANDING / 367 OK over 373 rows — see `## BLOCKING` and
`## LAST_CLV` for the current figure; do not trust any older count printed above this line in file order.**

**Code: DEC-042 and DEC-043 are COMMITTED, BUILD VERIFIED and TEST VERIFIED at `3b8226ec4`** (user decision
2026-09-06; parent `fd7639f7a`, 7 paths, +717/−1: `src/Cloud/CloudService.{h,cpp}` (DEC-042 saveRide
self-bail + DEC-043 readComplete guard/cancel wiring), `src/Core/Athlete.cpp` (close-tail
requestStop/wait/delete), `testGarminConnectSyncDialogClose.cpp` (TEST-158), `testCloudProviderWatchdog.cpp`
(TEST-159/160) and their two stub files). Same content as the 2026-09-05 execution-verified slice, plus
clang-format normalization on the two test files applied by the pre-commit hook — a new content version, so
the affected targets were RE-RUN before the commit landed: **4/4 ctest registrations PASS, EXIT=0** (both
binaries, both QPA backends), and the three DEC-033 binaries re-run separately (3/3, EXIT=0, fresh build) as
the `O-R027-01` reconciliation evidence. Durable logs in `.claude/evidence-seals/` (DEC042-…, DEC043-…).
Stage 5's `{RELEASE}` effect set was discharged on the 2026-09-05 evidence; the commit makes it durable.

**~45 other dirty entries** (Coach/, Gui/, root CMake, vcpkg, `.claude/` agent+hook+skill tooling, and the
untracked build/evidence directories) belong to other owners. This workflow has never staged them, does not
own them, and they are enumerated under `## EXCLUSIONS`.

## EVIDENCE — executed 2026-08-30, this session, stated with scope

Every row below was RE-EXECUTED here. None is copied forward from a report, a seal or a prior cursor. **ALL COUNTS AND FINGERPRINTS IN THIS TABLE ARE HISTORICAL, AS-OF 2026-08-30/31.** `OUTSTANDING=34`, `330 OK` and the sealed fingerprints below were correct for THOSE bytes and are NOT current. **Current: `27 OUTSTANDING / 337 OK` — see `## BLOCKING` and `## LAST_CLV`; the current 41-path fingerprint lives in the recovery `HANDOFF.md`, reproducible with `fingerprint.sh verify`.**
The R5 stability rows record the pre-reconciliation 41-path fingerprint. The 2026-08-30 review correction
subsequently changed governance text and CMake comments, but changed no timeout value or executable source.
Therefore R5 remains behavioural evidence for the unchanged code, **not a byte-identical seal of the current
41-path manifest**. Current-byte focused checks are recorded below; a new full exact-byte stability pair is
required after Gates 1A–1B and before commit.

| Check | Command | Result | Scope |
|---|---|---|---|
| Build config | `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DGC_WANT_GARMINCONNECT=ON -DGC_HAVE_LIBUSB=OFF` | configure EXIT=0 | clean worktree `.claude/worktrees/cleangate-37710370e` @ `37710370e`, `git status` empty |
| Full build | `ninja` | **EXIT=0**, `src/GoldenCheetah` LINKED | all targets |
| Build freshness | `ninja` (2nd run) | `ninja: no work to do` | proves the results below are from current source |
| **OFF build (NEW)** | `cmake -DGC_WANT_GARMINCONNECT=OFF -DBUILD_TESTS=ON -DGC_HAVE_LIBUSB=OFF` then `cmake --build` | **configure EXIT=0, build EXIT=0, `src/GoldenCheetah` LINKED** (581 targets, 27.8 MB) | **closes a dod.md universal-floor item and the OFF half of REQ-NF-Build-001, which had NEVER been executed** |
| CTest | `ctest -L garmin-fast --output-on-failure --timeout 180` | **27/27 passed, 0 failed, EXIT=0** (228.3 s) | 27 registered Garmin tests, IDLE machine |
| **Gate stability** | the same suite's two long tests, re-run under concurrent build load | **1 of 3 loaded runs of `testGarminConnectSyncDialogClose_minimal` was KILLED at 180 s (exit 137)** | **this is the reproduction of the reported 25/27 — see `## THE GATE DEFECT`** |
| Watchdog, `minimal` | 5 consecutive direct runs | **155 passed / 0 failed, EXIT=0** every run; 24.9 s ±0.02 s | no flakiness, no hang |
| **Per-slot timing (NEW)** | every slot of `testGarminConnectSyncDialogClose` timestamped, `minimal` | **91 passed, 0 failed, 99.0 s** — and **36.2 s of it is ONE slot**, the TEST-127 fuzzer | localises the cost instead of inferring it |
| **Fuzzer scaling (NEW)** | the fuzz slot alone at 64/128/256/512 seeds, `offscreen` | **52.6 / 53.2 / 51.6 / 53.3 ms per seed — FLAT** | rules out unbounded retry and cross-seed accumulation |
| **Backends interleaved (NEW)** | fuzz slot alone, 3 matched pairs, neither backend given a quieter machine | `offscreen` **33.8 / 29.7 / 33.9 s** · `minimal` **40.4 / 37.1 / 45.0 s** | `minimal` costs a consistent ~25% more for identical work |
| **Contention range (NEW)** | the same fixed 512-seed workload, quiet vs busy machine | **27.1 s → 69.7 s, a 2.6× range on identical bytes** | this, not a hang, is what a 180 s budget could not absorb |
| **Validator self-tests (NEW)** | `python3 -m unittest discover -s scripts -p 'test_clv_findings.py'` | **18/18 PASS, EXIT=0**; 6 cases assert exit 0 and 12 assert non-zero (10 exit 1, 2 exit 2) | the suite DISCRIMINATES in both directions, it does not merely pass ([[LSN-050]]) |
| **Findings repair (NEW)** | 29 structurally-broken rows rewritten, content-stream invariant asserted per row | **29/29 — every non-whitespace character of each original row present in its repaired row**; `MALFORMED` 29 → 0 | no evidence deleted, no row summarised |
| Whitespace | `git diff --check` | EXIT=0 | working tree |
| DEC-015 lint | `python3 scripts/ledger_drift_lint.py .` | **EXIT=0, 0 findings** | ledger |
| CLV-lite (PRE-Gate-1A, HISTORICAL) | `.claude/workflow-garminconnect/scripts/clv-lite.sh` | **FAIL — Check 5: 114 UNKNOWN-SEVERITY, 8 UNKNOWN-DISPOSITION, 15 OUTSTANDING, 0 MALFORMED** (Checks 0/1b/2 PASS, 1 and 6 WARN) | **HISTORICAL — this is the run that DEFINED Gate 1A's scope, not a current status.** The current verdict is the row 12 lines below and in `## LAST_CLV`. |
| Archive integrity | original body vs archive destination, all 12 moved files | **12/12 BODY PRESERVED VERBATIM** (each +6-line snapshot header) | `cycles/archive/`, `validations/archive/` |
| **Clean-worktree configure (R5)** | `cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DGC_WANT_GARMINCONNECT=ON -DGC_HAVE_LIBUSB=OFF` | **EXIT=0** | worktree `.claude/worktrees/gauntlet-slice` @ `37710370e` + ONLY this slice's 41 paths; build dir FRESH, never reused ([[LSN-080]]) |
| **Full build + app link (R5)** | `ninja` | **EXIT=0, 844 targets, 25m14s**, `src/GoldenCheetah` LINKED (28.1 MB) | and `ninja` again → `no work to do`, proving the results below are from these bytes |
| **`ctest -L garmin-fast` (R5 pass 1)** | `--output-on-failure`, no `--timeout` override | **27/27 passed, 0 failed, EXIT=0**, 236.9 s | dialog-close **84.5 s** offscreen / **92.9 s** minimal; watchdog **25.3 s** / **25.0 s** |
| **`ctest -L garmin-py` (R5)** | `--output-on-failure` | **1/1 passed, EXIT=0** | |
| **Garmin pytest (R5)** | `python3 -m pytest src/Python/garminconnect -q` | **25 passed, EXIT=0** | |
| **Effective timeouts (R5)** | read from the GENERATED `CTestTestfile.cmake`, not from the source | dialog-close **420 / 420**, watchdog **180 / 180** | the property, which is what actually applies — see the override note in `## THE GATE DEFECT` |
| **Adversary: timeout cannot mask a failure** | synthetic ctest project, a failing test with `TIMEOUT 9999`, run under `ctest --timeout 3` | **STILL FAILS** | a timeout bounds DURATION; it cannot turn a non-zero exit into a pass. Enlarging one can hide a HANG, never a failure. |
| **Adversary: interruption leaves nothing** | SIGKILL the dialog-close suite mid-run, then re-run the watchdog | clean baselines **25.05 / 25.04 s**; after the kill **25.10 s**; `pgrep` leftovers **NONE**, pid fully reaped | an interrupted run does not slow the next one |
| **Adversary: Check 1b really sees NF** | delete a grouped NF row, a single NF row and a numeric row from a COPY | **FAIL naming REQ-NF-Sec-001..004, then + REQ-NF-i18n-001, then + REQ-024** | mutation-proven in three directions; the real files were never touched |
| **Both drift-lint copies (R5)** | project-owned `scripts/` and installed `.claude/hooks/`, over the slice | **EXIT=0 both**, byte-IDENTICAL (md5 `1b7b256b…`), self-test 44/44 | |
| **Archive bodies (R5)** | each archived file vs its deleted original at HEAD | **12/12 BODY PRESERVED VERBATIM** | |
| **Exclusion manifest (R5)** | every dirty path in the main tree, classified in-or-out | **45 excluded**, and the slice worktree contains NONE of them | no `src/`, Coach, Gui, vcpkg or root-CMake change can reach this commit |
| **OFF build (R5, re-executed on THIS slice)** | `-DGC_WANT_GARMINCONNECT=OFF -DBUILD_TESTS=ON -DGC_HAVE_LIBUSB=OFF`, FRESH build dir | **configure EXIT=0, build EXIT=0** (13m34s), `src/GoldenCheetah` LINKED (27.8 MB); `ctest -N -L garmin-fast` reports **Total Tests: 0** | both directions of `REQ-NF-Build-001`: ON builds and registers 27 Garmin tests, OFF builds and registers NONE. Still an executed build, NOT a regression guard — the guard is Stage 8. |
| **STABILITY — the whole gate run TWICE on byte-identical inputs** | `ctest -L garmin-fast` + `-L garmin-py` + pytest + validator self-tests + CLV + both drift-lint copies, twice, back to back, on an IDLE machine (load 0.7-1.1) | **PASS A: 27/27 + 1/1, 229.2 s · PASS B: 27/27 + 1/1, 221.5 s.** Both EXIT=0. **No test skipped, none interrupted, none timed out.** Per-test, A vs B: dialog-close **87.0 / 80.2 s** (offscreen) and **86.9 / 86.9 s** (minimal); watchdog **25.2 / 25.3 s** and **25.1 / 25.1 s**. CLV output **byte-IDENTICAL** between passes. | both QPA backends registered and run in each pass; transcripts preserved under `round5-pass1/` and `round6-pass2/` in the evidence directory |
| Cursor extraction | `archive/state-history.md` § 10 vs the STATE.md it replaced | **byte-IDENTICAL**, md5 `89c5ed76bacb11526d6a193bf00c2fde` | nothing lost in the rewrite |
| **Review correction (PRE-Gate-1A, HISTORICAL)** | both validator runners; `clv-lite.sh`; both drift-lint copies; `git diff --check`; targeted stale-claim scan | validator **18/18 twice**; CLV reproduced **0 malformed / 114 unknown severity / 8 unknown disposition / 15 outstanding / 227 OK**; both lints and diff check EXIT=0 | **HISTORICAL — the bytes it covers were superseded by Gate 1A.** Governance and CMake-comment changes only; no timeout value or executable source changed. |
| **GATE 1A — classification** | `scripts/clv_findings.py` before and after; `test_clv_findings.py` under BOTH runners | **PASS.** `MALFORMED 0 → 0 · UNKNOWN-SEVERITY 114 → 0 · UNKNOWN-DISPOSITION 8 → 0 · NEEDS-DISPOSITION 0 → 0`. 130 cells edited across 126 rows (114 severity + 16 disposition; 4 rows took both). Self-tests **18/18 under `unittest` AND 18/18 under `pytest`**. | 12 dispositions were repaired, not 8: `classify()` returns at the first failing bucket, so 4 unreadable dispositions were hidden behind an unreadable severity (`B-R028-05`, `A3-R028c-F7`, `A3-R027-F8`, `B-R027-04`) |
| **GATE 1A — ORCH-050 adjudicated BY EXECUTION** | `testGarminConnectSyncDialogClose` at `37710370e`, both QPA backends, target's pinned `halt_on_error=1` | **TEST-128 PASS · TEST-129 PASS on all 3 completion channels AND both drivers · `GC_FUZZ_SEED=447` PASS** — 8 runs, EXIT=0 every one | the override block's cited evidence REPRODUCED before any verdict was moved; prescribed fixes located in shipped source (`row = nullptr` in `refreshClicked`; `completionGeneration` at `CloudService.cpp:3171/:3488/:3780`) |
| **GATE 1B — effect sets** | `scripts/clv_findings.py` before and after the 27 `BLOCKS: {…}` writes | **`MISSING-EFFECT` 8 → 0, and `OUTSTANDING` moved by EXACTLY 0** (34 → 34) | [[LSN-083]] executed rather than asserted: classification is not disposition |
| **GATE 1B — routing** | manual routing of all 34 against the stage table, checked mechanically | **FAILED — 15 routed, 19 NOT ROUTED** | **CORRECTED TWICE 2026-08-31 by independent audit.** The 19 split **A 8 / B 8 / C 3**, one class each, in `## BLOCKING`. It is NOT true that all 19 need a new decision — 8 carry explicit closure authority and 8 more have an artefact to inspect. **Only 3 need new governance.** **SUPERSEDED 2026-09-01:** `S-R021-01` closed by execution, so the current split is **27 outstanding — 15 routed, 12 not — A 1 / B 8 / C 3**. |
| **CURRENT-BYTE gate, isolated worktree — HISTORICAL, 2026-08-30/31 BYTES; SUPERSEDED 2026-09-01** | `.claude/worktrees/gauntlet-slice` @ `37710370e` + ONLY these 41 paths, `git status` exactly 41 entries, ZERO excluded paths | `ninja` **no work to do** · `ctest -L garmin-fast` **27/27, EXIT=0, 220.4 s** · `ctest -L garmin-py` **1/1** · `pytest src/Python/garminconnect` **25 passed** · validator **18/18 × 2 runners** · `clv-lite` **FAIL (Check 5, OUTSTANDING=34)** · both drift-lint copies **EXIT=0**, md5 `1b7b256b…` identical · `git diff --check` EXIT=0 | 0 skipped, 0 timed out, 0 sanitizer summaries. **The `OUTSTANDING=34` here is the count AT THAT TIME. Current is 27/337 — see `## LAST_CLV` and `## BLOCKING`.** |
| **OFF build, same slice** | `-DGC_WANT_GARMINCONNECT=OFF -DBUILD_TESTS=ON -DGC_HAVE_LIBUSB=OFF` | `ninja` **no work to do**, `src/GoldenCheetah` LINKED (27.8 MB); **`ctest -N -L garmin-fast` → Total Tests: 0** | both directions of `REQ-NF-Build-001` still hold. Still an EXECUTED build, NOT a regression guard — the guard is Stage 8. |
| **Archive integrity, current bytes** | each archived file vs its deleted original at `HEAD` | **12/12 BODY PRESERVED VERBATIM** after a 6-line snapshot header | `cycles/archive/`, `validations/archive/` |
| **STABILITY PAIR — the whole applicable gate run TWICE on one byte fingerprint** | `ninja` · `ctest -L garmin-fast` · `ctest -L garmin-py` · `pytest src/Python/garminconnect` · both validator runners · `clv-lite` · both drift-lint copies · `git diff --check` · the session consistency gauntlet | **PASS 1: 27/27 + 1/1, 219.58 s, whole pass 222 s · PASS 2: 27/27 + 1/1, 219.46 s, whole pass 221 s.** Both `ctest` EXIT=0. **0 skipped · 0 not-run · 0 timed out · 0 aborted · 0 sanitizer summaries · 0 leftover processes.** Identical 27-test inventory. `clv_findings.py` and `clv-lite.sh` output **byte-IDENTICAL between passes** (both FAIL, `OUTSTANDING=34`). Heavy slots, P1 vs P2: dialog-close **78.89 / 78.91 s** (offscreen) and **85.21 / 85.21 s** (minimal); watchdog **25.18 / 25.22 s** and **25.02 / 25.05 s**. | Fingerprint over all 41 paths, identical before and after both passes: **`01f259c5d4206029712a069c49f19bf03be7e1cb18b6990e2caa608cec7aee29`**. **Stated so it cannot be over-read: STATE.md was edited AFTER the pair — this row, the row below it, and one arithmetic correction in `## NEXT_GATE` — so the sealed fingerprint covers the other 40 paths byte-exactly and STATE.md as it stood before those three edits. The final tree's fingerprint is different and is recorded in the session report.** No commit is made, so nothing is represented as a commit seal. |
| **Post-edit re-verification — HISTORICAL, those FINAL bytes; SUPERSEDED 2026-09-01** | every doc-level check re-run on the then-final bytes: both validator runners, `clv-lite.sh`, both drift-lint copies, `git diff --check`, the consistency gauntlet | **reproduces IDENTICALLY** — `0/0/0/0/34/330`, `MISSING-EFFECT 0`, validator 18/18 × 2, lints EXIT=0, gauntlet PASS | proves the three post-pair STATE.md edits changed no verdict and no count. **`0/0/0/0/34/330` was correct for those bytes; the current run is `0/0/0/0/27/337` after the seven 2026-09-01 closures listed in `## BLOCKING`.** |

**What this evidence does NOT establish, stated so it cannot be misread:** it is ONE platform (Linux), ONE
compiler, ONE commit, and ONE machine — a developer laptop with a browser running, which is exactly why the
timing numbers carry a range rather than a value. There is **no live Garmin Connect evidence of any kind** —
every Garmin test in the suite runs against a seam, stub or fake. There is **no packaging evidence** on any
platform, and no Windows or macOS build of any kind.

## THE GATE DEFECT — reproduced, root-caused by measurement, and re-budgeted

The reported "clean rerun produced 25/27, the two `_minimal` tests timed out" **is real.** A previous pass
diagnosed it as "no `TIMEOUT` property" and set 900/600. The first half was right; **the second half was a
round number with no measurement behind it, and it is now replaced.**

**Where the time actually goes.** Per-slot timestamping of the whole `testGarminConnectSyncDialogClose`
binary under `minimal` (87 slots, 91 cases, all PASS, 99.0 s total):

| slot | time |
|---|---|
| `randomClickSequencesMustNotBreakTheTransferInvariants` (TEST-127, the fuzzer) | **36.2 s** |
| the other 86 slots, together | 62.8 s |

**What it is NOT — each ruled out by a measurement, not by argument:**

| candidate cause | measurement that rules it out |
|---|---|
| unbounded retry / event-loop wait | per-seed cost is **FLAT** from 64 to 512 seeds: 52.6 / 53.2 / 51.6 / 53.3 ms (offscreen). No growth, no accumulation. |
| an excessively repeated fixed wait | the per-seed drain loop's own ceiling is 50 passes × 5 ms = 250 ms; measured per-seed cost is 53 ms, so it never approaches its budget |
| leaked process or thread | `pgrep` between runs is empty; runtime does not grow across consecutive runs |
| test-order dependence in the VERDICT | exit 0 on every run — whole-binary and standalone, both backends |
| non-determinism | the seed range is fixed (1..512, 24 steps, 8 event passes per step — the author chose a fixed pass count over a millisecond budget precisely so a seed's outcome cannot depend on load) |

**What it IS: a fixed CPU-bound workload measured against a budget with no headroom.** TEST-127 is 512
seeds × 24 steps of real work. Its wall time therefore scales with the CPU it is given, and this is a
developer machine, not a build farm:

- **Backends interleaved** so neither gets a quieter machine — 3 matched pairs, fuzz slot alone:
  `offscreen` **33.8 / 29.7 / 33.9 s** vs `minimal` **40.4 / 37.1 / 45.0 s**. `minimal` costs a
  consistent **~25% more** for identical work, in all three pairs.
- **Same work, quiet vs busy machine:** 27.1 s → 69.7 s. A **2.6×** range on identical bytes.
- **Whole binary, idle:** `offscreen` 92.5 s · `minimal` 99.1 s and 118.3 s.

The gate passed `--timeout 180` over a workload whose idle runtime is 92–118 s. That is 1.5–1.8× headroom
against contention that alone accounts for up to 2.6×. **The budget was provisioned below the range its own
workload occupies** — so the verdict was decided by machine load, which is a FALSE RED, and a gate that
cries wolf is how a TRUE red gets waved through.

**The repair, and why it is not 900/600.** The dialog budget uses an explicit rule over the measured upper
bounds: **round up to the next 60 s after `1.25 × worst observed contention multiplier × slowest measured
idle runtime`**. With 2.6× contention and 118.3 s idle, that is `1.25 × 2.6 × 118.3 = 384.5 s`, rounded to
**420 s**. The 1.25 factor is the declared margin; it is not presented as another measurement. The watchdog
uses a separate **180 s timer-suite floor** because it drives real timers. That floor is conservative policy,
not a measured need, and must not be used to claim a reproduced watchdog hang.

| test | was proposed | now | justification |
|---|---|---|---|
| dialog-close, both backends | 900 | **420 s** | `ceil-to-60(1.25 × 2.6 × 118.3 s) = 420 s`; also 2.3× the worst COMPLETED loaded run recorded (179 s) |
| watchdog, both backends | 600 | **180 s** | explicit timer-suite floor; about 7× its 25.5–25.8 s runtime, stable to ±0.3 s across 7 runs. This is conservative policy, not measured necessity. |

**Nothing was weakened.** No test is skipped, shortened, excluded or relaxed; all 91 dialog cases and all
155 watchdog cases still run, under BOTH QPA backends, under the same ASAN options. Only the wall-clock
allowance changed.

**Stated because it is easy to misread: a `TIMEOUT` test property OVERRIDES `ctest --timeout`.** After this
change `ctest -L garmin-fast --timeout 180` does **not** kill these four at 180 s — their effective budgets
are 420 and 180. Anyone quoting the gate's timeout must quote those, not the command line.

**`testCloudProviderWatchdog` never reproduced a hang** — 25.5–25.8 s across 7 runs on both backends. Its
budget is preventive headroom, not a fix for anything observed. **If it fails again on the reporter's
machine, nothing measured here explains it** — say so rather than assuming this repair covered it.

**The residual risk, stated plainly:** this repair does not make the runtime stable, it makes the budget
honest. On a machine busier than this one the dialog-close binary could still exceed 420 s, and that would
be a real red requiring re-measurement, not another budget increase. The durable fix is to stop shipping 87
slots and a 512-seed fuzzer in one executable; splitting it was considered and NOT done here, because QTest
can select functions but cannot exclude one, so a split would mean naming the other 86 by hand and would
silently drop every slot added afterwards — a worse defect than the one being repaired.

## BLOCKING

Per-finding severity and disposition are single-homed in `findings.md`. This section names only the CURRENT
effect on the CURRENT gate, and every number below comes from `scripts/clv_findings.py`, which is the ONE
implementation of CLV Check 5.

**Re-run 2026-09-04 (second pass, after TEST-157 landed), over 366 rows: 0 MALFORMED · 0 UNKNOWN-SEVERITY ·
0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 14 OUTSTANDING · 352 OK · MISSING-EFFECT 0.** Exit 1.
**Re-run 2026-09-05 (recomputed per [[LSN-078]] after this session's ORCH-054/ORCH-055 disposition
edits — a stray literal `\|\|` in quoted C++ source briefly mis-split one row into 8 cells,
MALFORMED momentarily 1, fixed by escaping it `\\|\\|`), over 369 rows: 0 MALFORMED · 0
UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 14 OUTSTANDING · 355 OK ·
MISSING-EFFECT 0.** Exit 1. `OUTSTANDING` unchanged (both edited rows were already non-blocking,
not counted); `OK` moved 352→355 and the row count 366→369 — neither reconciled against the prior
count this pass, same unreconciled-drift caveat this section already carries for the 365→366 step.
(Same-day sequence: 18/346 over 364 rows (pre-DEC-033-build) → 19/346 over 365 rows (`B-R027-09` raised) →
15/350 (`B-R027-01/02/03/09` all dispositioned once DEC-033 was built + TEST-076 landed) →
**14/352** (`B-R028-01` dispositioned once `TEST-157` landed + was builder-executed AND independently
orchestrator-re-executed + mutation-proven, see the Stage 2 row below). **Row count 365→366 measured, not
reconciled against the prior count this pass — `MALFORMED=0` and `B-R028-01` itself is confirmed to appear
exactly once, so the +1 predates this edit and is not investigated further here.** 2026-09-01: 26/338.
2026-08-30: 34/330.)

**Re-run 2026-09-05 (Stage 4 — provider-watchdog checkpoint dispositioned), over 370 rows: 0 MALFORMED · 0
UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 9 OUTSTANDING · 361 OK · MISSING-EFFECT 0.**
Exit 1. `OUTSTANDING` moved **14 → 9** — `C1`, `C2`, `C3`, `C4` and `C6` all dispositioned CLOSED on
re-executed evidence (`testCloudProviderWatchdog` 168/168 both `offscreen`/`minimal`, EXIT=0, byte-identical
PASS lists, zero ASan; `ctest -L garmin-fast` 27/27, EXIT=0, 259.35s), discharging the complete
`{CHECKPOINT:provider-watchdog slice}` effect set — see `## NEXT_GATE`'s Stage 4 row and `findings.md`'s C1-C6
rows for the per-finding detail. `OK` moved 355→361 (the 5 closures plus 1 new row) and the row count
369→370 — the +1 is `ORCH-056`, a genuine pre-existing defect (the async `readFile` reply leaks across all
nine migrated providers, never disposed) found while verifying C4's own documented scope exclusion; raised
non-blocking, `{TASK:readFile-reply-lifetime — no REQ allocated}`, flagged for a scope decision rather than
fixed here (out of DEC-040's stated 22-bounded-wait scope). The 9 outstanding rows at that point were
exactly Stage 5's two release blockers (`B-R028-17`, `A3-R028e-F1`), Stage 6's five UAF-family stubs
(`S-R021-03/04/05`, `S-R031-01`, `B-R025-01`, `A3-R021b-F2`) and the stale `O-R027-01` row — see the note
immediately below.

**Re-run 2026-09-05 (DEC-042 built, `B-R028-17` dispositioned), over 372 rows: 0 MALFORMED · 0
UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 8 OUTSTANDING · 364 OK · MISSING-EFFECT
0.** Exit 1. `OUTSTANDING` moved **9 → 8** — `B-R028-17` dispositioned on re-executed evidence (99/99
both `offscreen`/`minimal`, `ctest -L garmin-fast` 27/27 EXIT=0, orchestrator-independent mutation-proof
reproduced the exact UAF), discharging one of Stage 5's two `{RELEASE}` findings — see `decisions.md`'s
DEC-042 entry and `traceability.md`'s TEST accounting for the full build/verification detail. `OK` moved 361→364, row count 370→372 (+2:
`ORCH-057`, the `run()` worker-thread read found while scouting DEC-043, deliberately not folded into it;
`ORCH-058`, a tooling friction between `anti_duplication_guard.py` and this project's own LSN-084
snapshot/restore convention, hit and worked around while verifying DEC-042). **The 8 remaining outstanding
rows are exactly Stage 5's ONE remaining release blocker (`A3-R028e-F1`, DEC-043 decided, not yet built),
Stage 6's five UAF-family stubs, and the stale `O-R027-01` row.**

**Re-run 2026-09-05 (DEC-043 built, `A3-R028e-F1` dispositioned — Stage 5 COMPLETE), over 373 rows: 0
MALFORMED · 0 UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 7 OUTSTANDING · 366 OK ·
MISSING-EFFECT 0.** Exit 1. `OUTSTANDING` moved **8 → 7** — `A3-R028e-F1` dispositioned on re-executed
evidence (TEST-159/TEST-160, watchdog 170/170 + syncdialog 99/99 both `offscreen`/`minimal`,
`ctest -L garmin-fast` 27/27, orchestrator-independent mutation-proof reproduced the exact UAF at
`uncompressRide`, real `Athlete.cpp` linked via `ninja GoldenCheetah`), discharging the SECOND and last of
Stage 5's `{RELEASE}` findings — see `decisions.md`'s DEC-043 entry, `findings.md`'s A3-R028e-F1 row and
`.claude/evidence-seals/DEC043-TEST159-TEST160-CloudServiceAutoDownload-mutation-2026-09-05.log` for the
full build/verification detail. `OK` moved 364→366, row count 372→373 (+1: `ORCH-059`, the TEST-160
stub-mirror sync residual disclosed by the builder and raised at verification, non-blocking). **The 7
remaining outstanding rows are exactly Stage 6's five UAF-family stubs (`S-R021-03`, `S-R021-04`,
`S-R021-05`, `S-R031-01`, `B-R025-01` + `A3-R021b-F2` — six findings across five REQs) and the stale
`O-R027-01` row. Stage 5 no longer contributes any outstanding row.**

**Re-run 2026-09-06 (O-R027-01 reconciled — Stage 2's finding set now complete), over 373 rows: 0 MALFORMED ·
0 UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 6 OUTSTANDING · 367 OK · MISSING-EFFECT
0.** Exit 1. `OUTSTANDING` moved **7 → 6** — the stale `O-R027-01` row, flagged in this section since the
Stage-4 pass, transcribed-closed after this session's own re-execution of the three DEC-033 binaries (3/3,
EXIT=0, fresh build); see the reconciliation note below and the row itself. No other counter moved; row count
unchanged at 373. **The 6 remaining outstanding rows are exactly Stage 6's UAF-family set (`S-R021-03` /
REQ-020, `S-R021-04` / REQ-022, `S-R021-05` / REQ-023, `S-R031-01` / REQ-024, `B-R025-01` + `A3-R021b-F2` /
REQ-029). Nothing outside Stage 6 is outstanding.**

**`O-R027-01` RECONCILED 2026-09-06 — the flag this section carried since the Stage-4 pass is discharged.**
The row was transcribed-closed to the [[ORCH-050]] bar: verify first, then transcribe. The verification is
THIS session's own execution — `ninja`: no work to do (fresh build), then `ctest` over the three DEC-033
binaries (`testGarminConnectReadFile` + `testGarminConnectReadFailure` + `testGarminConnectRefusalCompletion`)
— **3/3 PASSED, EXIT=0**. The stall class is closed at the loop level by DEC-033's
`ArmedNothing`/`ArmedCompletion` out-param, COMMITTED `2e26106c1`; `O-R027-01` was the last Stage-2 finding
left untranscribed when Stage 2 closed. Full verdict in the row itself (`findings.md` is SSOT). With it,
`OUTSTANDING` moved 7 → 6 and the six remaining rows are exactly Stage 6's UAF-family set.

**GATE 1B ROUTING COMPLETE 2026-09-03 — all 18 outstanding rows now map to exactly one stage; 0 unrouted.**
This session (a) technically adjudicated the 8 Class B rows against `37710370e` (qgdw-validator, spot-checked),
(b) re-executed `testGarminConnectSyncDialogClose` on BOTH `offscreen` and `minimal` in a clean worktree
(`.claude/worktrees/cleangate-37710370e` / `build-37710370e`), **1/1 PASSED, EXIT=0, 88.82s / 90.56s**, and
transcribed closures for 7 of the 8 to the [[ORCH-050]] bar: `S-R021-02`, `A3-R021-F1`, `A3-R021-F2`,
`B-R021-11`, `B-R021-12`, `A3-R027-F1`, `A3-R027-F4` — all `fixed`. The 8th, `B-R028-01`, the validator found
**genuinely unaddressed** (no artefact of any kind names it) and it stays open, now correctly reclassified
(folded into Stage 2, below) rather than miscounted as a cheap adjudication. (c) `B-R021-10` (Class C)
reconciled `closed` — orchestrator read `CloudService.cpp:2788-2880` and `:3603-3659` directly and confirmed
both `writeFile` sites the row named are lexically inside a `BlockingCall` scope that never closes early,
corroborated by `traceability.md:664`'s independent citation of `TEST-091`/`TEST-092`; no execution needed,
the claim is provable by inspection. (d) `B-R025-01` / `A3-R021b-F2` (Class C, the identical file-IO
Context-lifetime UAF at `RideFile.cpp:999`, independently raised twice) — qgdw-scout researched three
options, user chose **Option B, hoist-and-capture**; `REQ-029` allocated and `DEC-041` recorded accepting it;
both rows now carry `BLOCKS: {TASK:REQ-029}` and route to Stage 6. **`OUTSTANDING` moved 26 → 18 (−8: the 7
execution closures + `B-R021-10`); the 2 Class-C file-IO rows and `B-R028-01` stay counted, now ROUTED rather
than unrouted.**

**OUTSTANDING ROSE 15 → 34. That is the gate working.** It was 15 only because 122 rows carried a severity
or disposition no mechanism could read; they were unjudged, not harmless. The arithmetic: **−3** (ORACLE-F1,
A3-R028c-F1, A3-R028c-F2 — adjudicated on RE-EXECUTED evidence, [[ORCH-050]] below), **−1** (ORCH-050 itself,
resolved by that adjudication), **+23** rows that were already blocking and open and are now legible.

**ORCH-050 is CLOSED, and it was closed by execution, not by transcription.** The prose block
`### REQ-028 Option C disposition override — 2026-08-22` declared three rows RESOLVED while those rows' own
disposition cells still read `**BLOCKING — reopen DEC-037/DEC-034/DEC-036**`. Before moving any verdict this
pass re-ran the block's cited evidence at `37710370e`: **TEST-128** (`refreshMustMakeOutstandingWritesRowFree
WithoutLosingTheirIdentity`) **PASS**; **TEST-129** across all three completion channels AND both drivers
**PASS**; **`GC_FUZZ_SEED=447` PASS** — each on BOTH `offscreen` and `minimal`, under the target's pinned
`halt_on_error=1`. It also located the prescribed fixes in the shipped source: the row-free tombstone
(`i.value().row = nullptr` for every `readOperations`/`writeOperations` entry inside `refreshClicked`) and the
entry snapshots (`const int completionGeneration = batchGeneration;` at `CloudService.cpp:3171/:3488/:3780`,
compared at `:3302/:3400/:3419/:3529/:3838`). Each verdict now lives in its own row's disposition cell with
the original text preserved; the prose block is relabelled HISTORICAL NARRATIVE — NO CURRENT AUTHORITY.

**EVERY outstanding row now carries an explicit effect set — `MISSING-EFFECT` is 0** — and writing all 27 of
them moved `OUTSTANDING` by exactly zero, which is [[LSN-083]] executed rather than asserted. Scope was taken
PROVEN, not assumed (`references/orchestration.md`): a row that records no evidence of wider scope names its
raising TASK, never a global block.

### The 26, routed — 15 land in a stage, 11 NOT YET ROUTED

**ROUTED (15) — each appears in exactly one stage:**

| effect | rows (id + cycle, because an id is not unique alone, [[ORCH-022]]) | stage |
|---|---|---|
| `{CHECKPOINT:provider-watchdog slice}` | `C1`,`C2`,`C3`,`C4`,`C6` — all `Census/provider-watchdog design gate (2026-08-26)` | **4** |
| `{RELEASE}` | `B-R028-17` (`Builder/REQ-028 clause-(e) slice (2026-08-23)`) · `A3-R028e-F1` (`A3/REQ-028 clause-(e) slice (2026-08-24)`) | **5** |
| `{TASK:REQ-020}` | `S-R021-03` (`Scout/DEC-030 research (2026-08-10)`) | **6** |
| `{TASK:REQ-022}` | `S-R021-04` (`Scout/DEC-030 research (2026-08-10)`) | **6** |
| `{TASK:REQ-023}` | `S-R021-05` (`Scout/DEC-030 research (2026-08-10)`) | **6** |
| `{TASK:REQ-024}` | `S-R031-01` (`Scout/DEC-031 research (2026-08-11)`) | **6** |
| `{TASK:DEC-033}` | `B-R027-01`,`B-R027-02`,`B-R027-03` (`Builder/REQ-027 (2026-08-13)`) · `O-R027-01` (`Orchestrator/REQ-027 opening (2026-08-13)`) | **2** |

Gate 1A also promoted `B-R027-01` into OUTSTANDING: its severity already read `**BLOCKING for scope item 3**`
and was legible, but its disposition cell held prose. It is now `open`, and Stage 2 already named it.

**NOT YET ROUTED (11, of the 19 first tabulated) — GATE 1B STOPS HERE, but NOT for the reason first recorded.**

> **CORRECTION 2026-08-31, from an independent audit (Codex, read-only). The original sentence here read:
> *"Nothing in any row, in `traceability.md` or in `decisions.md` records whether the committed work
> discharged the finding."* THAT SENTENCE WAS FALSE, and it was false because Gate 1B searched the wrong
> places.** It read each finding's own row and each REQ's `Status (current)` cell, and never searched
> `decisions.md`'s `Serves:` / `closes …` fields or `traceability.md`'s per-slice sections. Those fields
> carry explicit closure authority for several of the 19. The audit produced the counter-evidence by
> command; it is reproduced below. **A gate that concludes "no evidence exists" without searching the file
> that holds the evidence has not proven absence — it has proven it did not look.** Same shape as
> [[ORCH-049]], one layer up: there a mechanism could not read its input; here the operator did not open it.

**Class A — closure authority EXISTS in the ledger. RECONCILIATION, not a new decision. 8 identified, all 8 dispositioned (7 on 2026-09-01, the 8th — `B-R028-05` — on 2026-09-02):**

| row | the statement that closes or folds it | where |
|---|---|---|
| ~~`B-R026-01`~~ · ~~`S-R027-01`~~ **both dispositioned 2026-09-01** | DEC-032 — *"Serves: REQ-027; closes B-R026-01, S-R027-01, S-R027-04"* (current bytes, as repaired 2026-09-01; the withdrawn `O-R027-01` claim is retained on that line as a SUPERSEDED marker) | `decisions.md:1060` |
| ~~`S-R028-01`~~ · ~~`B-R028-05`~~ (driver half) **both dispositioned, 2026-09-01 and 2026-09-02** | *"Closes S-R028-01 and S-R028-02 (the sort route) and B-R028-05's DRIVER half"*, committed `514d8e88f`; reconciled 2026-09-02 against native-desktop regression evidence (10/10 PASS, `probeWhatAColumnSortDoesToARunningBatch` + the three `aPreBatchStatusSortMustNotMake…TransferARowTwice` guards). | `traceability.md:341` |
| ~~`B-R028-05`~~ (labelling half) **dispositioned 2026-09-02** | DEC-036 — *"narrows B-R028-05 to its driver half"*; and *"its LABELLING half stays closed by DEC-036"*; reconciled against TEST-112's `labelledAtFirstCompletion == transferredRow` assertion, PASS on native desktop 10/10. | `decisions.md:1311`, `:1721` |
| ~~`S-R021-01`~~ **dispositioned 2026-09-01** | *"**Folded in by user decision (2026-08-10):** S-R021-01 — add the missing `if (self.isNull()) return false;`"*, and the DEC-030 slice recorded BUILT + Verification-Gate PASS. **Reconciled into the finding row after re-execution; no longer owed.** | `decisions.md:1004`, `traceability.md:68` |
| ~~`A3-R027-F2`~~ · ~~`A3-R027-F3`~~ · ~~`A3-R027-F8`~~ **all three dispositioned 2026-09-01** | The governing decision is bound to these three findings by name and its chosen option is recorded as closing *"all three briefed axes at their stated roots"*; the spine states the refresh-route defect of `A3-R027-F2` closed outright. **Read the verdicts at the two cites — this hub does not restate them (DEC-015).** | `decisions.md:1236`, `:1254`, `traceability.md:65` |

**A ROUTING CONFLICT — RAISED 2026-08-31, RESOLVED ON EVIDENCE, LEDGER REPAIRED 2026-09-01.**
`O-R027-01` is routed above to Stage 2 under `{TASK:DEC-033}`. UNTIL the round-4 repair,
`decisions.md:1060` (DEC-032's header) ALSO claimed DEC-032 *"closes … O-R027-01"*. **The DEC-033 route
is correct and that header claim WAS stale:** DEC-032's own `### AMENDMENT 2026-08-13 — scope item 3
REMOVED from DEC-032, routed to DEC-033 (B-R027-01)` withdrew exactly that scope, and `traceability.md:132`
states *"O-R027-01's stall route cannot be closed at the call site"*. The [[ORCH-026]] shape.
**The route stands, and `decisions.md:1060`'s closure claim for `O-R027-01` WAS marked superseded by its
own amendment on 2026-09-01 (round 4).** That line now closes only B-R026-01, S-R027-01 and S-R027-04;
the withdrawn claim is preserved verbatim in an inline marker. The ledger no longer self-contradicts.

**Class B — an ARTEFACT TO INSPECT exists (a guard or test naming the finding, or a ledger record
naming its exact subject), but no ledger statement closes the row (8):** `S-R021-02`, `A3-R021-F1`,
`A3-R021-F2`, `B-R021-11`, `B-R021-12`, `A3-R027-F1`, `A3-R027-F4`, `B-R028-01`. Each needs technical
adjudication against `37710370e` — cheaper than a governance decision, because the thing to look at is
already named.

**Class C — nothing names the finding in the ledger OR in source; a genuine owner decision is owed (3):**
`B-R025-01` (*Builder/REQ-025 2026-08-12*), `A3-R021b-F2` — one defect, `RideFile.cpp:999`, with **no REQ
allocated at all** — and `B-R021-10`.

> **`B-R021-10` MOVED B → C on 2026-08-31 by this pass's own mechanism, reversing part of the round-2
> audit finding.** The audit grouped it with five rows that carry a guard naming the finding; it does not.
> Nothing in `src/` or `unittests/` names `B-R021-10`, and measurement contradicts the coverage argument:
> its two `store->writeFile` sites (`CloudService.cpp:2880`, `:3659`) are **91 and 55 lines past the
> nearest enclosing `BlockingCall` scope**, and REQ-025's record claims the wrap around `openRideFile`,
> not around `writeFile`. Its sibling `B-R021-11` stays in B because REQ-025's record names *its* exact
> subject — the uncounted `openRideFile` nested loop — and both call sites (`:2791`, `:3606`) are inside
> `BlockingCall blocking(this)` in current source. **Same REQ, different evidence, different class.**

> **`B-R025-01` MOVED B → C on 2026-08-31, and the reason is a live [[ORCH-022]] id collision.**
> `findings.md` carries **TWO** rows with this id: **L244 `build/DEC-025 (2026-08-06)`** — *"The dialog's
> CONSTRUCTOR is still unprotected"* — and **L319 `Builder/REQ-025 (2026-08-12)`** — the file-IO UAF at
> `RideFile.cpp:999`. Only L319 is outstanding. Every mention outside `findings.md`
> (`decisions.md:866`, `traceability.md:73`) is about **L244**, so crediting it to L319 would be exactly
> the defect ORCH-022 records. `A3-R021b-F2` — the same defect, independently re-raised — has **zero**
> mentions outside `findings.md`. Both therefore sit in C. **An id is not unique alone; cite id AND cycle.**

**The remaining 11 account exactly once: A 0 + B 8 + C 3 = 11.** With the routed 15 that is all 26 outstanding rows.

**Gate 1B still FAILS**, for two different reasons: Class A rows are open in `findings.md` while the ledger
says they are closed, and an unreconciled contradiction is not a route; and Class C still needs decisions
this cursor cannot make. **Scale, stated precisely — and it moved twice under audit, both times toward LESS
governance being owed:** 1 of the 12 still open carries explicit closure authority; 8 more have an artefact to
inspect and need adjudication, not a decision; **only 3 of 12 genuinely need new governance** — the file-IO pair, which
needs a REQ allocated, and `B-R021-10`. Transcribing a closure into a finding row remains a disposition of a blocking finding and
is not done here on paper alone — the [[ORCH-050]] precedent sets the bar at verify-by-execution first.

**Class A is EXECUTABLE, not merely documented — and the first case is now CLOSED, 2026-09-01.**
`S-R021-01`'s guard is in shipped source at `src/Cloud/CloudService.cpp:1587-1598`, carrying the comment
*"S-R021-01 - THE BAIL THIS BRANCH WAS MISSING"*, and its TEST-084 slot
`syncDialogCancelBranchDoesNotInvokeCloseOnAFreedDialog` was RE-RUN under
`ASAN_OPTIONS=halt_on_error=1:detect_leaks=0`: **3 passed / 0 failed / EXIT=0 on BOTH `offscreen` and
`minimal`**. The verdict was THEN transcribed into the finding row, in that order — the [[ORCH-050]] bar.
`OUTSTANDING` 34 → 33 → 32 → 31 → 30 → 29 → 28 → 27 → 26, no other counter moving. **0 Class A rows remain.**

**THE 19 AS FIRST TABULATED — rows 1, 2, 3, 4, 5, 8, 9 and 10 dispositioned (row 5 on 2026-09-02, the rest 2026-09-01), 11 STILL OPEN. Same accounting as the prose above.**
Open membership is exact and non-overlapping: A 0 + B 8 + C 3 = 11. Cite id AND cycle ([[ORCH-022]]).

| # | id | cycle | effect | class | what is actually owed |
|---|---|---|---|---|---|
| ~~1~~ | `S-R021-01` | Scout/DEC-030 research (2026-08-10) | `{TASK:REQ-021}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Closure transcribed into the row in `findings.md` after RE-EXECUTION: TEST-084 slot **3 passed / 0 failed / EXIT=0 on BOTH `offscreen` and `minimal`**, `ASAN_OPTIONS=halt_on_error=1:detect_leaks=0`, 2026-09-01 at `37710370e`. Authority `decisions.md:1004`; guard `CloudService.cpp:1587-1598`. `OUTSTANDING` 34 → 33, no other counter moved. |
| ~~2~~ | `B-R026-01` | Builder/REQ-026 (2026-08-12) | `{TASK:REQ-027}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Authority `decisions.md:1060` — DEC-032 *"closes B-R026-01"*. Transcribed after RE-EXECUTION: the two slots naming this defect (`…MustNotKillTheRestOfTheBatch` :7582, `…MustStopTheBatch` :7687) each **3/3, EXIT=0 on BOTH backends**, `halt_on_error=1:detect_leaks=0`. `OUTSTANDING` 33 → 32, no other counter moved. |
| ~~3~~ | `S-R027-01` | Scout/DEC-032 (2026-08-13) | `{TASK:REQ-026}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Authority `decisions.md:1060`; the OQ-1 framing was already retired. Transcribed after RE-EXECUTION: TEST-099 `anAbortInsideOpenRideFileMustLeaveTheRowUnwritten` **3/3, EXIT=0 on BOTH backends**. The demanded 1-line guard is shipped at BOTH sites — `CloudService.cpp:2862` (syncNext) and `:3643` (uploadNext); every line number the row itself cites is stale. `OUTSTANDING` 32 → 31, no other counter moved. |
| ~~4~~ | `S-R028-01` | Scout/DEC-034 (2026-08-16) | `{TASK:REQ-028}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Authority `traceability.md:341` (DEC-038 slice, `514d8e88f`). Transcribed after RE-EXECUTION: the three-driver sort guard family (:13497/:13530/:13559) each **3/3, EXIT=0 on BOTH backends**. Mechanism re-located in CURRENT source — `CloudService.cpp:1851` (guard pair), `:1885` off, `:1949` restore, armed `:2395`, released `:2988/:3121/:3736`; the row's own `:1116/:1146/:1186` cites are stale. `OUTSTANDING` 31 → 30, no other counter moved. |
| ~~5~~ | `B-R028-05` | Builder/REQ-028 TEST-112 (2026-08-18) | `{TASK:REQ-028}` | **A — dispositioned 2026-09-02** | **DONE, not owed. Both halves reconciled.** DRIVER half: authority `traceability.md:341` (DEC-038 slice, `514d8e88f`); mechanism confirmed in CURRENT source — `CloudService.cpp:1885` off, `:1949` restore, armed `:2395`, released `:2988/:3121/:3736`. LABELLING half: authority `decisions.md:1311`, `:1721` (DEC-036). Both corroborated by the SAME slot, TEST-112 (`probeWhatAColumnSortDoesToARunningBatch`) — `labelledAtFirstCompletion == transferredRow` (labelling) and `sortingDuringBatch`/`clickableDuringBatch`/`indicatorShownDuringBatch` all zero with no reorder (driver) — PASS, native desktop, 10/10 under `QT_ACCESSIBILITY=0`, `--timeout 420`, no outer wrapper (83.78-96.72s, mean 86.1s, exit 0 every run); the three `aPreBatchStatusSortMustNotMake` Sync-/Upload-/DownloadNextTransferARowTwice guards PASS in the same runs. Review-sandbox non-completion of this row's gating ctest registration accepted as an infrastructure limitation, user decision 2026-09-02 — see `findings.md:340`. `OUTSTANDING` 27 → 26, no other counter moved. |
| 6 | `S-R021-02` | Scout/DEC-030 research (2026-08-10) | `{TASK:REQ-021}` | **B** | `decisions.md:983` states the finding inside DEC-030; the ~8× collaborator-surface scope call (DEC-030 OQ-2) is the open half. Adjudicate. |
| 7 | `A3-R021-F1` | A3/REQ-021 (2026-08-11) | `{TASK:REQ-021}` | **B** | Named in DEC-030's option analysis (`decisions.md:1033`) without a closure statement. Adjudicate against `37710370e`. |
| ~~8~~ | `A3-R027-F2` | A3/REQ-027 (2026-08-14) | `{TASK:REQ-028}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Transcribed after RE-EXECUTION: TEST-108 `aRefreshInsideARowsOpenMustNotLeaveTheLoopHoldingThatRow` (clause a — this row's exact defect), TEST-109 and TEST-110 each **3/3, EXIT=0 on BOTH backends, zero sanitizer output**. DEC-034's `listGeneration` mechanism re-located in CURRENT source: bumped `CloudService.cpp:1964`, per-batch snapshot `:2431`, driver-local `listgen` `:2646` compared at **`:2830`** — between `openRideFile` `:2791` and the row read `:2880`, the exact route this row named; upload twin `:2974`. Only clauses (a)/(b)/(d) claimed for F2; clause (c) was handled SEPARATELY under `B-R028-03`, which is `closed` (DEC-036 Option A, in-flight ticket) and sits in validator bucket OK — **not a live remainder**. `OUTSTANDING` 30 → 29. **Original authority note follows.** | The spine records this finding's refresh-route defect as closed (`traceability.md:65`); the governing decision's chosen option is recorded as closing all three briefed axes at their roots (`decisions.md:1254`). Reconcile. |
| ~~9~~ | `A3-R027-F3` | A3/REQ-027 (2026-08-14) | `{TASK:REQ-028}` | **A — dispositioned 2026-09-01** | **DONE, not owed.** Transcribed after RE-EXECUTION of the six slots naming this finding or owning its harm — TEST-119/TEST-120 across all three loops, TEST-109, TEST-110 and the sorting-restore slot — each **3/3, EXIT=0 on BOTH backends, zero sanitizer output** (twelve runs). Neither shape this row proposed shipped: DEC-034 added a SECOND counter instead, so `refreshClicked` bumps `listGeneration` (`CloudService.cpp:1964`) and the completion tail stands down at **`:2625`** (`if (listGeneration != batchListGeneration) return true;`, twin `:3023`), placed at the irreversible call per DEC-035. Source names this finding at `:2580`, `:2822`, `:2955`, `:2965`, `:3297`. `OUTSTANDING` 29 → 28. **Original authority note follows.** | As row 8 — the governing decision is bound to F2/F3/F8 by name (`decisions.md:1236`) and its option covers all three axes. Reconcile. |
| ~~10~~ | `A3-R027-F8` | A3/REQ-027 (2026-08-14) | `{TASK:REQ-028}` | **A — dispositioned 2026-09-01** | **DONE, not owed. Both defects checked separately.** (1) The six positional `child(listindex-1)` sites are GONE — the string survives only in comments (`:3161`, `:3286`, `:3346`); DEC-036 replaced positional addressing with a stored pointer guarded by the row-null stand-down at `:3204`. (2) Handed to `B-R028-03`, which the validator places in bucket OK, token `closed` (DEC-036 Option A, in-flight ticket; TEST-113 asserts clause (c) verbatim). Regression-direction slots TEST-113/115/121/128 each **3/3, EXIT=0 on BOTH backends** — eight runs. TEST-107 deliberately NOT used: it is an inverted probe. `OUTSTANDING` 28 → 27. **Original authority note follows.** | As row 8; the spine also records F8's refresh route as a NULL DEREF and its repair (`traceability.md:65`). Reconcile. |
| 11 | `B-R028-01` | Builder/REQ-028 TEST-107 (2026-08-16) | `{TASK:REQ-028}` | **B — RE-ADJUDICATED 2026-09-03, coverage-only** | REQ-028 is `TEST VERIFIED · COMMITTED`; the guard shape is already decided/built/gate-verified (DEC-034/036 AMENDMENT 2026-08-21). **Not a decision gap** — the earlier "genuine decision gap" call was wrong. Owed: one new test combining existing `RebuildSpec::where = InSaveRideAutoProcess` + existing `restartInsteadOfRefresh`. |
| 12 | `A3-R021-F2` | A3/REQ-021 (2026-08-11) | `{TASK:REQ-021}` | **B** | Named guard in shipped source — `CloudService.cpp:2032` *"A3-R021-F2 - THE COLLABORATOR HALF"* — and TEST-086 names the finding. Adjudicate. |
| 13 | `B-R021-10` | Builder/REQ-021 sweep (2026-08-11) | `{TASK:REQ-025}` | **C** | **Nothing names it** in ledger or source. Its two `store->writeFile` sites (`:2880`, `:3659`) are 91 and 55 lines past the nearest enclosing `BlockingCall` scope, and REQ-025's record claims the wrap around `openRideFile`, not `writeFile`. Owner decision. |
| 14 | `B-R021-11` | Builder/REQ-021 sweep (2026-08-11) | `{TASK:REQ-025}` | **B** | REQ-025 covers it in shipped source: both real `openRideFile` sites (`:2791`, `:3606`) are inside `BlockingCall blocking(this)`, the uncounted FIT loop this row names. Adjudicate and close. |
| 15 | `B-R021-12` | Builder/REQ-021 remediation (2026-08-11) | `{TASK:DEC-031}` | **B** | Named fix in shipped source — `CloudService.cpp:1613` *"B-R021-12 - THE ORPHAN THIS BAIL USED TO LEAVE BEHIND"* — posting the queued close. Adjudicate. |
| 16 | `B-R025-01` | **Builder/REQ-025 (2026-08-12)** | `{TASK:file-IO collaborator lifetime — NO REQ ALLOCATED}` | **C** | `RideFile.cpp:999`. **No REQ allocated.** Every out-of-file mention of this id belongs to the OTHER `B-R025-01` (L244, `build/DEC-025`) — see the ORCH-022 note above. Allocate a REQ. |
| 17 | `A3-R021b-F2` | A3/REQ-021 re-clear (2026-08-12) | `{TASK:file-IO collaborator lifetime — NO REQ ALLOCATED}` | **C** | Same defect as row 16, independently re-raised; **zero** mentions outside `findings.md`. Allocate a REQ. |
| 18 | `A3-R027-F1` | A3/REQ-027 (2026-08-14) | `{TASK:REQ-027}` | **B** | `SyncNextParsePE` frame and TEST-101, which names the finding, both exist in the suite. Adjudicate (the row's re-run of M14 is still unrecorded). |
| 19 | `A3-R027-F4` | A3/REQ-027 (2026-08-14) | `{TASK:REQ-027}` | **B** | Three named guards in shipped source (`CloudService.cpp:3421`, `:3531`, `:3840`, each *"REQ-027 (A3-R027-F4)"*, two carrying `if (aborted == true) return;`) plus TEST-102. Adjudicate. |

**What this cursor deliberately does NOT do:** assert that any of the remaining 12 is fixed or harmless.
Transcribing a closure into a finding row is a DISPOSITION of a blocking finding, and the [[ORCH-050]]
precedent sets the bar at verify-by-execution-then-transcribe, not paper alone.

**What is owed, per class — NOT one blanket decision (that framing was the disproven claim):**
**Class A (0)** — all reconciled; no new governance owed.
**Class B (8)** — technical adjudication against `37710370e`; an artefact is already named, so this is
cheaper than a decision.
**Class C (3)** — a genuine owner decision: the file-IO pair, which needs a REQ that was never allocated,
and `B-R021-10`.
Until each of the 11 lands in a stage, they are open blockers with an effect and no route, and
`COMMIT_READY` is NO.

**Release-blocking, separately: ORCH-036.** `BLOCKS: {}` today, but at `37710370e` `GC_HAVE_LIBUSB=ON`
configures EXIT=0 and then FAILS to build (`#include <usb.h>` not found, 12 error lines). Whoever owns the
release must decide whether an ON-configuration that cannot build is a release blocker. The fix is real and
parked UNMERGED on `ae7655985` (DEC-039). This cursor records the measurement and does not pre-empt the call.

**Not blocking, carried forward:** A3-R028c-F4/F5/F6 · B-R028-04 (user-deferred) · B-R028-14 (ratified) ·
B-R028-18/19 · ORCH-014 (half discharged) · ORCH-034 · ORCH-037/038/039 · ORCH-040 (remaining half) ·
ORCH-041 (retired — the branch it described is gone; `NEEDS-DISPOSITION` now fails rather than
self-clearing) · ORCH-045 · ORCH-049, ORCH-050 and ORCH-051 (all three raised AND closed by this wave) ·
B-R038-01/03 · the Stage-1 informational residuals A3f-R040-B/-D/-F and the factory-unregistration half of -E.

## NEXT_GATE

**GATE 1A — make every finding row's severity and disposition READABLE BY THE MECHANISM. → PASSED 2026-08-30.**

- **Exit criteria, all four met, on a run anyone can reproduce** (`scripts/clv_findings.py`):
  `MALFORMED = 0`, `UNKNOWN-SEVERITY = 0` (was 114), `UNKNOWN-DISPOSITION = 0` (was 8),
  `NEEDS-DISPOSITION = 0`. `test_clv_findings.py` still passes **18/18 under `python3 -m unittest` AND
  18/18 under `pytest`**, so the counts were not obtained by loosening the vocabulary.
- **What was done, per row:** the severity or disposition cell was PREFIXED with the controlled-vocabulary
  token the sentence in that row's own body already established, and the original text kept verbatim after
  the prefix. **130 cells were edited across 126 rows: 114 severity cells and 16 disposition cells.** The 16
  break down as the 8 the mechanism reported, plus 4 that had been HIDDEN behind an unreadable severity
  (`B-R028-05`, `A3-R028c-F7`, `A3-R027-F8`, `B-R027-04` — `classify()` returns at the first failing bucket,
  so an unreadable severity masks an unreadable disposition beneath it), plus the 3 ORCH-050 rows and
  ORCH-050's own. Four rows needed both cells, which is why 130 edits touch 126 rows.
  **No row's meaning was changed and no evidence was deleted**: the file's non-whitespace character count
  rose by exactly the inserted text.
- **`OUTSTANDING` rose 15 → 34, as this gate predicted it would.** Detail and arithmetic → `## BLOCKING`.
- **`ORCH-050` was adjudicated, not transcribed.** Its cited evidence was RE-EXECUTED at `37710370e` before
  any verdict moved. Detail → `## BLOCKING`.

**GATE 1B — freeze reality, propagate it, and rebuild the route. → PASSED 2026-09-03. (FAILED 2026-08-30, at step 5; the step-5 detail below is the HISTORICAL record of why, kept because the reasoning stayed correct even as the count changed.)**

**Step 5 exit, 2026-09-03: all 18 outstanding rows now map to exactly one stage — 0 unrouted.** The 12
step-5 originally could not route (of the 19 first tabulated, later corrected to 12/11) are now fully
accounted: **Class A (1, `S-R021-01`)** was already closed by execution before this session. **Class B (8)**
— all 8 technically adjudicated against `37710370e` this session (qgdw-validator, orchestrator-spot-checked
and then orchestrator-executed): 7 confirmed and closed on dual-backend re-execution (`S-R021-02`,
`A3-R021-F1`, `A3-R021-F2`, `B-R021-11`, `B-R021-12`, `A3-R027-F1`, `A3-R027-F4`); the 8th, `B-R028-01`, the
validator found genuinely unaddressed — reclassified and folded into Stage 2 rather than left miscounted as
a cheap adjudication. **Class C (3)** — `B-R021-10` reconciled `closed` on direct source inspection (both
`writeFile` sites now provably inside a `BlockingCall` scope); `B-R025-01`/`A3-R021b-F2` (the same file-IO
defect, independently raised twice) got their own REQ (`REQ-029`) and DEC (`DEC-041`, Option B
hoist-and-capture, user-chosen) and route to Stage 6. **Detail and the dual-backend evidence citations are in
`## BLOCKING`, above.**

Steps 1-4 completed; step 5 stopped on evidence, which is the behaviour this gate specifies. **This step table is the HISTORICAL record of the 2026-08-30 run; its `34/330` was correct then. Current: 27/337, 15 routed / 12 not.**

| step | action | result |
|---|---|---|
| 1 | freeze the validator output | DONE — `0/0/0/0/34/330`, `MISSING-EFFECT 0`, exit 1 |
| 2 | update `findings.md`'s current-run summary | DONE — equals the frozen output |
| 3 | update STATE counts + `## BLOCKING` | DONE — equal to the same output |
| 4 | every outstanding row carries an explicit effect set | DONE — 27 written, `MISSING-EFFECT` 8 → 0, and `OUTSTANDING` moved by exactly 0 ([[LSN-083]]) |
| 5 | route every active effect to exactly one stage / checkpoint / release decision / authorised deferral | **FAILED — 15 routed, 19 NOT YET ROUTED** |
| 6 | `traceability.md` where evidence changes a verdict | no verdict changed — the ORCH-050 adjudication concerns three FINDING rows, not a requirement's status |
| 7 | WIKI registries if an id was allocated | none allocated; `next:ORCH-052`, `next:084` unchanged |

**Why step 5 failed — RESTATED 2026-08-31, because the first statement of it was FALSE.** It read:
*"No row, and nothing in `traceability.md` or `decisions.md`, records whether the committed work discharged
the finding."* An independent audit disproved that by command. Gate 1B had searched each finding's own row
and each REQ's `Status (current)` cell, and never searched `decisions.md`'s `Serves:` / `closes …` fields or
`traceability.md`'s per-slice sections — which is where the closure authority lives.

Step 5 fails for THREE different reasons, and only the third is a missing decision:
1. **Class A (1)** — the ledger says this is closed while `findings.md` says it is open. An
   unreconciled contradiction is not a route. Reconciliation is owed, not governance.
2. **Class B (8)** — an artefact naming the finding, or a ledger record naming its exact subject, exists.
   Technical adjudication against `37710370e` is owed. **Not a decision.**
3. **Class C (3)** — nothing names the finding in ledger or source. Only these are blocked on governance.
The 12, their effects, their class and their exact obstruction are tabulated once in `## BLOCKING`.

- **The decisions actually required — Class C ONLY, and it is three rows, not nine:** allocate the missing
  REQ for the file-IO layer (`B-R025-01` *Builder/REQ-025 2026-08-12* and `A3-R021b-F2`, both
  `RideFile.cpp:999`), and disposition `B-R021-10`.
  **Explicitly NOT decisions, though an earlier revision of this list said they were:** `S-R021-02`,
  `A3-R021-F2`, `A3-R021-F1`, `B-R021-11`, `B-R021-12`, `A3-R027-F1`, `A3-R027-F4`, `B-R028-01` are
  **Class B — adjudication**, each with a named artefact. `S-R027-01` is **Class A — reconciliation**:
  `decisions.md:1060` (DEC-032) explicitly closes it, and the earlier "awaiting DEC-032 OQ-1" statement
  here was wrong.
- **That ledger repair is DONE (round 4, 2026-09-01):** `decisions.md:1060`'s header no longer claims
  DEC-032 closes `O-R027-01`. The claim its own
  `AMENDMENT 2026-08-13 — scope item 3 REMOVED from DEC-032, routed to DEC-033` withdrew now carries an
  inline SUPERSEDED marker. **The ledger no longer contradicts itself on `O-R027-01`**, and the
  B-R026-01 / S-R027-01 / S-R027-04 closures on that same line are untouched.
- **What is NOT owed:** a re-run of Gate 1A. It passed and its result is stable.
- **Exit, reached 2026-09-03:** Gate 1B exits when all effects are routed. All 18 currently-outstanding
  effects now map to a stage (2, 4, 5 or 6) — this development-unlocking condition is now met. **This does
  NOT mean the findings are resolved** — 18 still carry open work; it means every one has a home to be
  resolved IN, which is what Gate 1B checks.

**COMMIT_READY = NO — unchanged, and for a DIFFERENT reason than before.** CLV Check 5 is still red
(`OUTSTANDING = 18`, down from 27) because 18 findings still have open work, but Gate 1B itself has EXITED —
every one of those 18 now routes to exactly one stage, so `COMMIT_READY = NO` no longer means "the route is
missing," it means "the routed work isn't done yet." The 41-path slice is VERIFIED IN WORKING TREE and
UNCOMMITTED; nothing is staged. **This session's ledger edits (findings.md, decisions.md, prd.md, STATE.md,
WIKI.md) all touch paths ALREADY named in `## COMMIT MANIFEST`'s 41-path list below — no new path was added,
so the manifest itself needs no update, only its byte content (already current, since it's a path list, not
a diff snapshot).**

### The stage path — REBUILT 2026-08-30 from the Gate 1B blocker set

No stage may be exited while a requirement assigned to it is unbuilt. "US-1..US-5 demonstrable" is NOT a
valid exit for any stage, because the non-functional and release criteria are tracked separately below.
**Every incomplete requirement appears in exactly one stage. Of the 27 outstanding findings, 15 appear in
exactly one stage below and 12 appear in NONE — those 12 are listed in `## BLOCKING` and are why Gate 1B
failed. A finding absent from this table is not a finding that stopped mattering.**

| Stage | Entry | Scope — the complete assignment | Exit |
|---|---|---|---|
| **2 — decision backlog** | Gate 1B | **`DEC-033` BUILT + execution-verified 2026-09-04** (Option A, defaulted out-param). `T-154/155/156` all passing (targeted regression: `testGarminConnectReadFile` 13/13, `testGarminConnectReadFailure` 9/9, `testGarminConnectRefusalCompletion` 5/5, `testGarminConnectSyncDialogClose` 97/97 both `offscreen`/`minimal`, `testCloudProviderWatchdog` both backends, `testGarminConnectReadFailedConsumer` 4/4). `B-R027-01/02/03/09` were all dispositioned 2026-09-04 (see `findings.md` for the current verdicts) — the guard-load-bearing residual (`B-R027-09`) is discharged by `TEST-076(a)/(b)`, RED-under-mutation proven independently by both the builder and the orchestrator (a real ASan heap-use-after-free at `CloudService.cpp:2784` with the guard neutered; restored byte-identical via `cmp`, reproduced passing again). **Still uncommitted** — sits in this session's working-tree slice alongside the rest of the ledger work; nothing pushed. **`B-R028-01` dispositioned 2026-09-04, on execution evidence (see `findings.md` for its full verdict).** `qgdw-builder` added `TEST-157` (`aRestartInsideSaveRideAutoProcessMustStandTheOldGenerationDown`, `testGarminConnectSyncDialogClose.cpp`) driving exactly the untested combination (`RebuildSpec::where = InSaveRideAutoProcess` + `restartInsteadOfRefresh = true`), test-only, zero `src/` change. **Orchestrator independently re-executed**: new slot PASS on both `offscreen`/`minimal`, EXIT=0, zero ASan output, `ninja: no work to do` (proving current source); **independently repeated the builder's mutation, with a durable log** (`.claude/evidence-seals/B-R028-01-TEST-157-mutation-2026-09-04.log`, sha256 `b1177f05…`, untracked/in-tree) — suppressed both governing compares at `CloudService.cpp:3473`/`:3492` under `cp`/`cmp` snapshot-restore discipline, reproduced the identical RED signature on BOTH `offscreen` and `minimal` (`binary exit=1` each), restored byte-identical (`cmp` exit 0, `git diff --stat` empty) and rebuilt clean, PASS re-confirmed; **regression** `ctest -L garmin-fast` 27/27 passed, EXIT=0, 236.9s. Full evidence and the disclosed (deliberately out-of-scope) residual — a possible second physical write on the same route in a non-stubbed build — are in `findings.md`'s `B-R028-01` row. **Stage 2 is now fully discharged: all its findings (`B-R027-01/02/03/09`, `B-R028-01`) are dispositioned.** | `clv-lite.sh` Check 6 PASSes (needs DEC-033 built+verified) **— MET.** `B-R028-01`'s new test slot lands, passes, and carries a closing disposition **— MET 2026-09-04.** **STAGE 2 EXIT REACHED.** |
| **3 — DEC-040 Stage 2** | Gate 1B | Generic cooperative cancellation: the `CancelToken` type, member-plumbed via `setCancelToken`; the cancellation halves of T-146/T-147; T-148/T-149 in full; `RequestOutcome::Cancelled` made reachable. **Carries `REQ-NF-Cancel-001`.** **PIECE 1 OF 3 LANDED 2026-09-04 (deliberately incremental, real production code — not a test-only slice like Stage 2's predecessor):** the `CancelToken` class itself (`src/Cloud/CloudService.h`, +29/-0, byte-identical to the class body in `decisions.md`'s DEC-040 entry), declaration-only — no member field, no `setCancelToken`, no wiring into `blockingRequest`, `RequestOutcome::Cancelled` still unreachable. Builder-built, **orchestrator-independently-verified**: `git diff` matches the accepted design exactly; `grep -rn CancelToken src/Cloud/*.{h,cpp}` outside `CloudService.h` returns zero (nothing consumes it yet); `git status --porcelain src/Cloud/` shows only `CloudService.h`; the one target that actually includes the header, `testCloudProviderWatchdog`, independently re-run on both `offscreen`/`minimal` — **155 passed/0 failed each**, exact match to the pre-change baseline (`traceability.md:569`); full regression `ctest -L garmin-fast` **27/27 passed, EXIT=0**, 222.0s. Proves the piece is genuinely behavior-inert, not merely claimed to be. **Still uncommitted**, no TEST id allocated or spent (none applies — declaration-only, per the briefing's own no-usage constraint). **PIECE 2 OF 3 LANDED 2026-09-04:** member-plumbed `setCancelToken`/`cancelToken_`/`kCancelPollMs=250` onto `CloudService` (`CloudService.h`, +55/-0) and wired a second, repeating `cancelPoll` timer into `blockingRequest`'s wait loop (`CloudService.cpp`, +45/-0), declared with the same context-object ordering rule as the existing `watchdog` timer (destroyed before `loop` on every exit path). `RequestOutcome::Cancelled` is now genuinely reachable — the one new production write site is `CloudService.cpp:305`, inside `cancelPoll`'s timeout lambda — and needed **zero** special-casing in STEP 1/3/4's outcome-reconciliation/disposal/post-disposal-invariant logic, confirming those were already generic over `pending_`'s value (an existing in-file comment had predicted exactly this). **T-146's Stage-2 remainder (row 4 + row 6's Cancelled arm) and T-149 both BUILT IN FULL** (`testCloudProviderWatchdog.cpp`, +186/-18) — see `traceability.md`'s DEC-040 test-accounting table for the per-test detail. **Builder-built, orchestrator-independently-verified:** `git diff` matches the intended design on all 3 files; rebuilt and re-ran `testCloudProviderWatchdog` on both `offscreen`/`minimal` myself — **158 passed/0 failed each** (155 baseline + 3 new slots, zero regressions); **independently redid the T-149 killing-mutation proof with a durable log** (`.claude/evidence-seals/DEC040-Stage2-piece2-kCancelPollMs-mutation-2026-09-04.log`, sha256 `cde862939921ad7197cffd3b7c5d835747331a598f0234e8ca1cc53f0497eb1d`) — `kCancelPollMs` 250→2000 reproduces RED (observed 1852ms > 500ms bound), restored byte-identical (`cmp` exit 0), rebuilt clean, PASS re-confirmed; full regression `ctest -L garmin-fast` **27/27 passed, EXIT=0**, 225.1s. **Still uncommitted.** One documented design choice worth an adversary's eye (builder's own note, orchestrator concurs it's reasonable but unproven against piece 3's real call sites): the cancel token is snapshotted once before `loop.exec()` rather than read live from `this`, so a `setCancelToken()` call on the same `CloudService` from another thread *during* an in-flight `blockingRequest` would not be observed by that call — intentional, but not yet validated against how piece 3's GUI-thread/pagination sites will actually use it. **Remaining, not yet started:** piece 3 — T-147's Cancelled-mid-listing pagination propagation and T-148's 3 GUI-thread default-token sites (C6). **T-147 site count CORRECTED 2026-09-05, before any piece-3 code was written:** the "8 shape-B/C sites" figure above is stale — it was never re-derived by reading source after `traceability.md`'s own 2026-08-26 amendment flagged the census as untrustworthy. Direct read of all 11 census B/C rows (this session), then reconciled against `unittests/Core/garminconnect/testCloudProviderWatchdog.cpp`'s own already-built shape census (that file's "shape-B/C set" comment block, search rather than cite a line — it has already drifted once as the file grew; T-147's already-passing TIMEOUT-propagation half is built against it), found 4 are actually single-request with no nested wait — no site for a `continue`-vs-`break` mutation to act on: `Xert::readdir`, `Nolio::readdir`, `CyclingAnalytics::readdir`, `SixCycle::readdir` — each says so in its own Stage-1 comment. **The genuine T-147 set is 7 sites — {2,4,5,7,10,16,17} — matching the test file's own count exactly:** `Dropbox::readdir`, `Azum::readdir`, `Azum::listAthletes` (also T-148), `SportTracks::readdir`, `Strava::readdir`, the per-entry loop inside `Xert::readdir` that calls `Xert::readActivityDetail` (row 10), and `Strava::addSamples` (row 17 — single request, no internal loop, but the deepest-nested site in the tree; what's tested is Cancelled propagating as `false` up through its completion chain, not an internal continue-vs-break). Full evidence and reasoning in `traceability.md`'s new note directly below the 22-site census table. T-148's 3 sites (`Dropbox::createFolder`, `Azum::listAthletes`, `Nolio::listAthletes`) were independently re-confirmed correct as originally recorded. **PIECE 3a (T-147) LANDED 2026-09-05:** the Cancelled-mid-listing counterpart to the already-built TIMEOUT rows, over the corrected 7-site set — `shapeBC_cancellationBreaksTheListing[_data]` (new slot pair, sites 2/4/5/7/10/16) plus a new "streams cancelled" row on the existing `strava_completionIsExactlyOne_data` (site 17), all in `testCloudProviderWatchdog.cpp`. **Zero production changes** — every one of the 7 sites' `!result.ok()` checks was already generic over `RequestOutcome`, confirmed by the builder reading all 7 site functions before writing the test and by the orchestrator independently reading the same 7 during the T-147 site-count correction above; Cancelled required no special-casing anywhere, same pattern piece 2 already established for `blockingRequest` itself. Builder-built, **orchestrator-independently-verified**: rebuilt (`ninja: no work to do` before the mutation cycle, confirming the build reflected the builder's own final state) and re-ran `testCloudProviderWatchdog` on both `offscreen`/`minimal` myself — **165 passed/0 failed each** (158 piece-2 baseline + 7 new slots, zero regressions, zero ASan lines grepped from the output); full regression `ctest -L garmin-fast` **27/27 passed, EXIT=0**, 228.25s (re-run clean after an earlier attempt raced my own in-flight rebuild and spuriously reported BAD_COMMAND on the two watchdog registrations — not a real failure, the binary was mid-relink). **Independently redid one of the builder's mutation proofs myself, with a durable log** (`.claude/evidence-seals/DEC040-Stage2-piece3-T147-Xert-mutation-2026-09-05.log`): mutated site 10 (`Xert.cpp:277`, `return returning;` → `continue;`), rebuilt, ran the new Cancelled row — RED, `out.requests` actual 3 vs expected 2; ran the OLD pre-existing timeout row over the SAME mutation — stayed PASSING (no RED), confirming the builder's own side finding that that row is mutation-blind (single-activity fixture); restored `Xert.cpp` (`git diff --stat` empty), rebuilt clean, both rows re-confirmed passing. **That mutation-blind-spot finding is logged as [[ORCH-053]]** in `findings.md` (non-blocking — the new Cancelled row is a genuine substitute at that site; the old row's own claim to be a killing test is false until it gets the same two-activity fixture, owed as a small follow-up, not scoped to this piece). **Builder's own additional finding, also independently plausible on reading:** the `continue`-mutation's failure MODE differed by site — `Dropbox::readdir` resolved via the harness's 3000ms rescue apparatus, while `Azum::readdir`'s equivalent mutation ran as a genuine unbounded loop requiring an external `timeout` kill — both are legitimate RED evidence of "a loop that doesn't break" under deliberately-broken code, and neither reflects a production hang (the real, unmutated code returns/exits in both cases, confirmed above). **Still uncommitted.** **PIECE 3b (T-148) LANDED 2026-09-05, then hit a session-ending orchestrator incident during its own verification — read this whole block before touching `CloudService.h` again.** Builder added `guiThread_defaultTokenNeverCancels[_data]` (3 rows: `Dropbox::createFolder`, `Azum::listAthletes`, `Nolio::listAthletes`) driving each through `driveSite(...)` with no explicit `CancelToken`, asserting success under a 400ms-delayed response (past `cancelPoll`'s 250ms cadence, so the poll genuinely ticks at least once) — catching its OWN first-draft mistake first (a 0ms-delay race made the mutation invisible) before reporting. **Zero other production changes.** Orchestrator independently rebuilt, ran both `offscreen`/`minimal` — **168 passed/0 failed each** (165 piece-3a baseline + 3 new) — and independently reproduced the mutation proof (`CancelToken::cancelled()`: `flag_ && flag_->load(...)` → `!flag_ || flag_->load(...)`, inverting only the default-token case): RED on all 3 new rows, and — the important isolation check — 11 real-token tests (`table_row4`, all 6 `shapeBC_cancellationBreaksTheListing` rows, `table_row6_cancelInduced...`, `cancel_observationIntervalWithinBound`) run under the SAME mutation and stayed passing, confirming the mutation is correctly isolated to the default-token case. Durable log: `.claude/evidence-seals/DEC040-Stage2-piece3b-T148-CancelToken-default-mutation-2026-09-05.log`.

**THEN, restoring from that mutation, the orchestrator ran `git checkout -- src/Cloud/CloudService.h` — which discarded ALL of Stage 2 pieces 1+2's uncommitted content (~55 lines: the `CancelToken` class, `setCancelToken`, `cancelToken_`, `kCancelPollMs`), not just the one mutated line, because none of it had ever been staged and `git checkout --` restores to HEAD, not to "before the last edit".** Full incident, cause and lesson: [[LSN-084]] / `findings.md` [[ORCH-054]] — **RESOLVED 2026-09-05, re-verified by mutation.** Reconstructed same session from `decisions.md`'s verbatim `CancelToken` class body (piece 1 had already certified it byte-identical to the shipped class) plus `CloudService.cpp`'s piece-2 diff, which was NEVER touched by the incident and gave an exact usage contract (member names/types/the constant) to rebuild against — not a guess. **A fresh orchestrator session then redid BOTH previously-owed mutation proofs against these exact reconstructed bytes:** piece 2's `kCancelPollMs` 250→2000 reproduced RED identically (`observed 1852ms`, the same value as the original 2026-09-04 proof against the now-lost bytes); piece 3b's `CancelToken::cancelled()` default-flip reproduced RED on all 3 T-148 rows with the same 11-test isolation set staying green. Both restored byte-identical (`cmp` exit 0) and re-confirmed PASS. **Full `ctest -L garmin-fast` regression also re-run: 27/27 passed, EXIT=0, 231.76s** — not run since the reconstruction until now. Durable log: `.claude/evidence-seals/DEC040-Stage2-ORCH054-reconstruction-reverification-2026-09-05.log`. The reconstruction is now PROVEN mutation-sensitive on both properties, not merely behaviorally compatible with the pre-existing suite. **Prior text, preserved:** verification explicitly not yet obtained: neither piece 2's `kCancelPollMs` mutation proof nor piece 3b's `CancelToken`-default mutation proof (both captured above, against the NOW-LOST bytes) had been re-run against the reconstructed file, and `ctest -L garmin-fast` full regression had not been re-run since the reconstruction. Both are now done, above.

**SEPARATELY, a real, independently-found coverage gap in T-148 itself, reported by an independent reviewer: [[ORCH-055]] in `findings.md` — FIXED 2026-09-05.** `fixture::driveSite(...)`'s defaulted 4th parameter changed from `CancelToken cancelToken = CancelToken()` to `std::optional<CancelToken> cancelToken = std::nullopt`, and the body from an unconditional `service->setCancelToken(cancelToken)` to `if (cancelToken) service->setCancelToken(*cancelToken);` — "no token supplied" now genuinely skips the setter, so T-148's 3 rows exercise `CloudService`'s own construction-time default for `cancelToken_`, not a freshly-constructed stand-in. The one explicit-token caller (`shapeBC_cancellationBreaksTheListing`) needed no change (`std::optional` converts implicitly). **Proven closed by mutation:** mutated `cancelToken_`'s own member-initializer to start already-cancelled (`CancelToken cancelToken_ = CancelToken(std::make_shared<std::atomic_bool>(true));`) — RED on all 3 `guiThread_defaultTokenNeverCancels` rows under the fixed harness (impossible under the old unconditional-setter harness, which always overwrote this exact defect back to a working token before the site method ran); the 11-test explicit-token isolation set stayed green under the identical mutation. Restored (`cmp` exit 0), full suite 168/0 both backends, full `ctest -L garmin-fast` **27/27 passed, EXIT=0, 230.42s**. Durable log: `.claude/evidence-seals/DEC040-Stage2-ORCH055-T148-harness-fix-2026-09-05.log`. **Both this session's assigned pieces of work — the mutation re-verification (ORCH-054) and this harness fix (ORCH-055) — are now done, in that order, as prescribed.** | DEC-040 W3 COMPLETE |
| **4 — provider-watchdog checkpoint** | Stage 3 | **DISCHARGED 2026-09-05.** Carried the COMPLETE `{CHECKPOINT:provider-watchdog slice}` set from Gate 1B — `C1`,`C2`,`C3`,`C4`,`C6`, all cycle `Census/provider-watchdog design gate (2026-08-26)`, and no others. All five dispositioned against the completed W3, in `findings.md`, on RE-EXECUTED evidence (not transcribed from prior sessions' reports): **C1** (SixCycle false-success) — discharged structurally by `blockingRequest`'s outcome-gated `readAll()` (`CloudService.cpp:387`), covered per-site by T-144 at both SixCycle sites. **C3** (Xert per-entry nested wait) — discharged by T-143/144/145/150 at site 10 plus T-147's break-not-continue mutation, already independently proven in Stage 2 piece 3a. **C4** (reply disposal) — discharged by the shared `ReplyDisposer` (`CloudService.cpp:182-211`) plus T-145's 22-site exactly-once-destruction coverage; its own documented exclusion (the async `readFile` reply at site 17, never owned by `blockingRequest`) turned out to be a genuine, PRE-EXISTING, undocumented leak across all nine migrated providers — raised as **[[ORCH-056]]**, non-blocking, `{TASK:readFile-reply-lifetime — no REQ allocated}`, deliberately NOT fixed in this checkpoint (out of DEC-040's own stated 22-bounded-wait scope) and **flagged for a scope decision, not decided here.** **C6** (GUI-thread default-token sites) — was already effectively closed by T-148 (Stage 2 piece 3b + ORCH-055's harness fix); this pass reconciled the row. **C2** (Strava::addSamples nested completion) — discharged for its OWN literal claim (unbounded wait now bounded, false-success now prevented, its own behavioural test slot T-152 exists and passes) via T-143/144/145/147/150/152 at site 17; **explicitly does NOT close `S-R031-01`/`REQ-024`** (the STORE-lifetime UAF reachable through the same call chain), which stays OPEN, NOT STARTED, in Stage 6. Per-site, per-criterion coverage at all 22 census sites confirmed as REAL production-call-site rows (`fixture::driveSite` calls the actual provider method through each site's real code path, only the `QNetworkAccessManager` is faked at the S-1 seam) — not helper-only. **Orchestrator independently re-executed 2026-09-05** (fresh rebuild, `ninja: no work to do` beforehand): `testCloudProviderWatchdog` 168 passed/0 failed on BOTH `offscreen` and `minimal`, EXIT=0, PASS lists byte-identical, zero `AddressSanitizer` lines; full `ctest -L garmin-fast` **27/27 passed, EXIT=0, 259.35s**. Durable log: `.claude/evidence-seals/DEC040-Stage4-provider-watchdog-checkpoint-fresh-session-reverification-2026-09-05.log`. CLV re-run: `OUTSTANDING` moved **14 → 9** (exactly C1/C2/C3/C4/C6, no other row), `OK` moved `355 → 361`, row count `369 → 370` (the +1 is ORCH-056), `MALFORMED`/`UNKNOWN-SEVERITY`/`UNKNOWN-DISPOSITION`/`NEEDS-DISPOSITION`/`MISSING-EFFECT` all still 0; validator self-tests 18/18; `ledger_drift_lint.py` EXIT=0; `git diff --check` EXIT=0. **Still uncommitted** — governance-only this pass (findings.md, WIKI.md, this file); zero `src/`/`unittests/` changes, confirmed by `git status --porcelain` showing no new path beyond this session's ledger edits. | the checkpoint effect is DISCHARGED — **MET 2026-09-05.** |
| **5 — release blockers** | Gate 1B | **REGENERATED from Gate 1B evidence 2026-08-30; Gate 1B routing completed 2026-09-03 (0 unrouted — the "12 unrouted" caveat this row used to carry is stale, see `## BLOCKING`'s routing-complete note).** The complete active `{RELEASE}` set is exactly two: `B-R028-17` (`Builder/REQ-028 clause-(e) slice (2026-08-23)`) and `A3-R028e-F1` (`A3/REQ-028 clause-(e) slice (2026-08-24)`). **DEC-042 (B-R028-17) BUILT + execution-verified 2026-09-05 — the finding is discharged.** Option A: in-function `QPointer` self-bail in `saveRide` after its second `autoProcess` call, placed at the site this project's own T-141 probe proved is the real hazard, not the call-site guard. TEST-158 (`aParentTeardownInsideSaveRidesSecondAutoProcessMustNotFreeItUnderThat`) executes the finding's own missing experiment via a seam-injected parent-teardown landing precisely inside the second `autoProcess` call (`autoProcessCallsAtTeardown=2`, confirmed); asserts no ASan report AND the accepted trade-off (`writeRideFileCalls=0`/`addRideCalls=0` — the file is genuinely not written on this rare path). Orchestrator independently re-verified (not merely accepted from the builder): 99/99 both `offscreen`/`minimal`, own mutation-proof reproduced the identical `heap-use-after-free` at `saveRide`, restored via `cp`+`cmp` snapshot/restore (hit a hook friction doing so, see `[[ORCH-058]]`), full `ctest -L garmin-fast` 27/27 EXIT=0. Durable log: `.claude/evidence-seals/DEC042-TEST158-saveRide-mutation-2026-09-05.log`. **DEC-043 (A3-R028e-F1) BUILT + execution-verified 2026-09-05 — the finding is discharged, and with it Stage 5's `{RELEASE}` effect set is COMPLETE.** Option C as decided: a `stopRequested_->load(acquire)` guard at the head of `readComplete` (frees the buffer, returns before any `context`/`athlete` dereference; the decided wording said "readComplete/readFailed", but the build correctly left `readFailed` unguarded — it touches no context/athlete state), `stopRequested_`+`requestStop()` on the thread with `setCancelToken(CancelToken(stopRequested_))` in `run()`'s worklist loop reusing DEC-040 Stage 2's mechanism, and `Athlete::close()` now ending with `requestStop(); wait(); delete cloudAutoDownload; cloudAutoDownload = nullptr;` — which also closed the pre-existing `cloudAutoDownload` leak as the user-ruled trivial fold-in. TEST-159 (`readComplete_afterOwnerTornDownMustNotDereferenceFreedMemory`, `testCloudProviderWatchdog.cpp`) executes the finding's own missing experiment — queued `readComplete` dispatched after owner teardown, single-row RED proof first (ASan `heap-use-after-free` at `uncompressRide`); TEST-160 (`athleteClose_cancelsAndJoinsAutoDownloadThenDeletesIt`) pins the cancel/join/delete tail. Orchestrator independently re-verified (not merely accepted from the builder): 170/170 watchdog + 99/99 syncdialog on both `offscreen`/`minimal`, own killing mutation (guard neutered → identical UAF at `CloudService.cpp:712`) reproduced and restored byte-identical via Edit + `cmp` ([[ORCH-058]] convention), `ctest -L garmin-fast` 27/27 (317.27s), `ninja GoldenCheetah` links the REAL `Athlete.cpp` change. Durable log: `.claude/evidence-seals/DEC043-TEST159-TEST160-CloudServiceAutoDownload-mutation-2026-09-05.log`. User-scoped alongside, unchanged: `ORCH-057` (`run()`'s own worker-thread read) stays its OWN finding, deliberately not folded into the build; one NEW residual `ORCH-059` (TEST-160 drives a hand-mirrored stub `Athlete::close()`; no automated stub/production lockstep check) was raised at verification, non-blocking. Full trade space in `decisions.md`'s DEC-042/DEC-043 entries. **Stage 5's exit condition is now MET: both Gate-1B-routed `{RELEASE}` effects are discharged on executed evidence.** | every Gate-1B-routed `{RELEASE}` effect is discharged on executed evidence |
| **6 — the rest of the UAF family** | Stage 5 | **`REQ-020`** (wizard-auth UAF, stub) — carries `S-R021-03` (`Scout/DEC-030 research (2026-08-10)`) · **`REQ-022`** (QThread cross-thread collaborator UAF) — carries `S-R021-04` (same cycle) · **`REQ-023`** (STORE-layer collaborator UAF) — carries `S-R021-05` (same cycle) · **`REQ-024`** (store destroyed while its own frame is suspended; census site 17, finding C2) — carries `S-R031-01` (`Scout/DEC-031 research (2026-08-11)`) · **`REQ-029`** (NEW 2026-09-03 — file-IO layer Context UAF at `RideFile.cpp:999`, DEC-041 accepted, Option B hoist-and-capture) — carries `B-R025-01` (`Builder/REQ-025 (2026-08-12)`) and `A3-R021b-F2` (`A3/REQ-021 re-clear (2026-08-12)`), the same defect independently raised twice. All five are `must`, all five are NOT STARTED, and each now carries its own Gate-1B-routed finding(s). | all five TEST VERIFIED **and** every routed finding dispositioned against it |
| **7 — the missing Phase-1 PRODUCT surface** | Stages 4+6 | **`REQ-009`** ToS notice (NOT STARTED) · **`REQ-010`** bulk backfill — its controller design DES-009 has never been accepted either · **`REQ-013`** profile auto-fill (NOT STARTED, `nice`) · **`REQ-014`** friendly error translation (PARTIAL) · **`REQ-015`** CAPTCHA path (PARTIAL; `captcha` NOT STARTED on the Python side either — never raised, only named in a docstring — so C++'s `Unknown` collapse never fires) · **`REQ-NF-Pkg-001`** installer bundling (NOT STARTED — `garminconnect` and `curl_cffi` are in NO requirements or installer file, so the feature cannot reach a user on ANY platform) · **`REQ-NF-Compat-001`** (`docs/garminconnect-known-limits.md` DOES NOT EXIST). | US-1..US-5 each demonstrable **and** every id in this row TEST VERIFIED |
| **8 — the non-functional / coverage debt** | Stage 7 | **`REQ-NF-Perf-001..003`** (only NF-Perf-003 is covered, at mechanism level; the ≤5 s end-to-end stopwatch is UNMEASURED) · **`REQ-NF-Sec-001/003/004`** (only Sec-002 is covered) · **`REQ-NF-Reliab-001..002`** · **`REQ-NF-Obs-001`** · **`REQ-NF-i18n-001`** · **`REQ-NF-Build-001`** needs a REGRESSION GUARD, not just this session's executed both-values build. `REQ-011` is absent because its exact read-only capability invariant is already TEST VERIFIED and COMMITTED. | every dod.md NF bar met by a test |
| **9 — live Garmin + cross-platform** | Stage 8 | A real Garmin Connect account exercising connect/MFA/sync/disconnect, recorded with date and outcome; the Win/macOS/Linux INSTALLED-package smoke checklist `REQ-NF-Pkg-001` names. Neither may be inferred from a passing seam test. | LIVE-SERVICE TESTED + CROSS-PLATFORM VERIFIED |

**Requirements NOT in a stage, and why that is valid:** `REQ-016` only — DEFERRED by an explicit
accepted-with-rationale disposition of 2026-07-20 (A3-R008-F1). It is the sole deferral in the table.
Every other incomplete requirement is assigned above exactly once.

**FINDINGS not in a stage, and why that is NOT valid:** the 12 unrouted rows tabulated in `## BLOCKING`.
They are not deferrals — nobody authorised deferring them — and they are not discharged. They are open
blockers with an effect and no home, and closing that hole is Gate 1B's remaining work.

**Before internal alpha:** Gates 1A–1B + Stages 2–4. **Gates 1A–1B and Stages 2–4 are now ALL discharged,
2026-09-05** — Stage 4 was the last of the four, closed that session. This does **NOT** mean internal alpha
is reached: `COMMIT_READY = NO` (Check 5 is still red at `6 OUTSTANDING` — exactly Stage 6's UAF-family
findings across five REQ stubs; the stale `O-R027-01` row was reconciled 2026-09-06, and Stage 5's two
release blockers were both discharged 2026-09-05 on executed evidence and COMMITTED `3b8226ec4` on
2026-09-06). The verified code and governance slices are now COMMITTED (`3b8226ec4` + its governance child)
— but nothing is pushed, nothing merged, and no adversarial cycle has run against this working tree or
against `37710370e`. It means the
FOUR gates this line names as alpha's prerequisite have each individually exited on executed evidence — a
narrower, purely mechanical claim. **Before limited beta:** + Stages 5–7 and a live-service
smoke — Stage 5's share of that bar is now also met (both `{RELEASE}` effects discharged on executed
evidence); Stages 6–7 remain. **Before production:** + Stages 8–9, every `{RELEASE}` effect discharged on
executed evidence, and the `GC_HAVE_LIBUSB=ON` build failure dispositioned.

## COMMIT MANIFEST — the exact 41 paths of this slice (no counts, no approximations)

Nothing outside this list may be staged. Every deletion is paired with its archive destination in the SAME
list, so a deletion cannot be staged without the file that preserves its body. **41, not the 39 a previous
cursor listed** — the gate repair added `clv_findings.py` and its test suite.

**Modified, tracked (12)**
```
STATE.md                                          WIKI.md
lessons.md                                        wiki/conventions.md
.claude/workflow-INDEX.md                         .claude/workflow-garminconnect/prd.md
.claude/workflow-garminconnect/decisions.md       .claude/workflow-garminconnect/traceability.md
.claude/workflow-garminconnect/findings.md        .claude/workflow-garminconnect/dod.md
.claude/workflow-garminconnect/scripts/clv-lite.sh
unittests/Core/garminconnect/CMakeLists.txt       <- the ONLY source-tree file; the TIMEOUT repair
```

**New — the canonical validator and its suite (2)**
```
.claude/workflow-garminconnect/scripts/clv_findings.py        <- THE one CLV Check 5 implementation
.claude/workflow-garminconnect/scripts/test_clv_findings.py   <- its 18 synthetic self-tests
```
**Why these two are NOT optional:** `clv-lite.sh` in this slice EXECUTES `clv_findings.py`, and `findings.md`
in this slice POINTS at it instead of carrying the algorithm. Committing either without these two ships a
validator that cannot run and a register whose documented mechanism does not exist on disk.

**Deleted — old paths (12), each paired with its destination below**
```
.claude/workflow-garminconnect/cycles/active/a3-req-002.md
.claude/workflow-garminconnect/cycles/active/a3-req-002-e2e.md
.claude/workflow-garminconnect/cycles/active/a3-req-002-tile-routing.md
.claude/workflow-garminconnect/validations/active/val-004.md ... val-012.md   (9 files)
```
**New — archive destinations (12), bodies verified byte-preserved**
```
.claude/workflow-garminconnect/cycles/archive/a3-req-002.md
.claude/workflow-garminconnect/cycles/archive/a3-req-002-e2e.md
.claude/workflow-garminconnect/cycles/archive/a3-req-002-tile-routing.md
.claude/workflow-garminconnect/validations/archive/val-004.md ... val-012.md  (9 files)
```
**New — history and relocated wiki spokes (3)**
```
.claude/workflow-garminconnect/archive/state-history.md   <- sole home of ALL superseded cursor narrative
wiki/map-detail.md                                        <- WIKI.md links here
wiki/registry-detail.md                                   <- WIKI.md links here
```
**Why the last two are NOT optional:** `WIKI.md` in this slice links to both, and neither is tracked.
Committing `WIKI.md` without them ships two dangling links and leaves the relocated MAP/REGISTRIES content
with no home in git.


## EXCLUSIONS — dirty entries that MUST NOT be staged with this slice
`.claude/agents/qgdw-*.md` (4) · `.claude/hooks/anti_duplication_guard.py` ·
`.claude/skills/quality-gated-dev-workflow/**` (6 — VENDOR TERRITORY, ORCH-004) ·
`scripts/test_anti_duplication_guard_flags.py` — all QGDW tooling, a separate concern with its own history.
`src/Coach/**` (10) · `docs/COACH_*.md` (3) — AI Coach, a CLOSED ledger.
`src/Gui/**` (7) · `src/FileIO/RideFile.h` · `src/Train/KurtInRide.cpp` · `src/Resources/**` (3) ·
`src/CMakeLists.txt` · `CMakeLists.txt` · `vcpkg.json` · `FITmetadata.json` · `util/fix_missing_resources.py`
— other owners' pre-session churn.
`.claude/evidence-seals/**` — untracked commit-safety seals; **NEVER staged with a slice** (WIKI.md rule).
`.claude/worktrees/**` — untracked build/verification worktrees; ~400 generated artifacts. **This directory
is untracked but NOT gitignored, so a bare `git add -A` would stage all of it.** Stage by explicit path only.

## COUNTS  (RE-DERIVED 2026-08-30 by command against the source-of-truth files; nothing copied forward.
  REQ/DEC next-values updated 2026-09-03 for this session's REQ-029/DEC-041 allocation — see below.)
REQ **29** in prd.md (28 + NEW `REQ-029`, file-IO Context UAF, STUB/NOT BUILT, DEC-041) + **16** distinct
  `REQ-NF-*` ids (10 traceability rows, 3 of them ranges that Check 1b now EXPANDS), next **030**. All 28
  prior numeric REQs and all 16 NF ids have a traceability row; **REQ-029 does NOT yet** — owed as a
  byproduct when it's built, not before (it has no TEST or commit yet to point to).
DEC **43 allocated (001–043)**, **43 with an entry section** (DEC-042/DEC-043 decided AND built +
  execution-verified 2026-09-05), next **044**
DES **14** + 2 lettered (`DES-001a`, `DES-003a`), next **015**
TEST allocated to **T-160** (T-154/155/156 BUILT 2026-09-04 for DEC-033; T-157 BUILT 2026-09-04 for
`B-R028-01`; T-158 BUILT 2026-09-05 for DEC-042; T-159/T-160 BUILT 2026-09-05 for DEC-043 — see
`## BLOCKING`/`## LAST_CLV`); next **T-161**
VAL **18** rows, next **019**
LSN **83** (001–083, contiguous), next **084**. LSN-081 = the gate-budget guard; **LSN-082** = an unreadable
  input reported as non-blocking is failing open; **LSN-083** = a gate whose actions cannot change its own
  pass criteria. (`LSN-048` has both an index line and a cold entry — the file's two-tier shape, not a dup.)
Findings: process series **ORCH-001..059**, next **ORCH-060** — 052–059 allocated across the 2026-09-04/05
  build-and-verify sessions (057 = `run()` worker-thread read, 058 = hook/restore friction, locally patched,
  059 = TEST-160 stub-mirror sync gap).
  **ORCH-049** (Check 5 failing open on 122 unreadable rows) FIXED · **ORCH-050** (a second, prose
  disposition source disagreeing with three rows) **RESOLVED 2026-08-30 by Gate 1A, on re-executed
  evidence** · **ORCH-051** (the unsatisfiable NEXT_GATE) FIXED. Total finding rows **373** (2026-09-05;
  052–059 raised since this line's 2026-08-30 derivation), all 373
  structurally well-formed and all 364 now MECHANICALLY LEGIBLE (was 334 well-formed + 28 malformed;
  a 29th, ORCH-009, had 7 columns and no trailing pipe and was mis-read as valid).
Check 5, over 364 rows, after this session's Gate-1B routing pass (2026-09-03): **0 MALFORMED · 0
  UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 18 OUTSTANDING · 346 OK · MISSING-EFFECT
  0**, exit 1 (Check 5 fails whenever `OUTSTANDING > 0`, independent of routing). **2026-09-01: 26/338.
  2026-08-30 (pre-Gate-1A): 0/114/8/0/15/227, 8 missing effect sets.** Not comparable with the pre-repair
  "15 OUTSTANDING / 28 NEEDS-CLASSIFICATION", which came from the fail-open predicate. **All 18 of the 18 are
  now routed to a stage — 0 unrouted** (Gate 1B PASSED 2026-09-03) — see `## BLOCKING`.
TEST spine gap (ORCH-038): **22** TEST ids carry an in-source marker but no traceability citation —
  TEST-021/022/023, 053, 065, 083, 085/086, 088, 092, 101..104, 115/116, 119/120, 122/123, 125, 127.
BUDGETS (RE-MEASURED 2026-09-06 with `wc -c`, not carried forward — the prior figures below had
  drifted 39KB+ unnoticed, an instance of [[LSN-078]], a budget line nobody recomputes): **WIKI.md
  12,680 bytes** (2026-09-05: 11,724; first measurement 8,931; HEAD: 38,467, unchanged).
  **STATE.md 117,288 bytes** (2026-09-05: 108,431; first measurement 69,127 — the DEC-040 Stage 2 saga,
  the LSN-084 incident/recovery, ORCH-053..059 and this pass's commit/reconciliation edits account for
  the growth). **lessons.md 213,693 bytes** (unchanged this pass — no lesson captured; the 2026-09-06
  cursor-drift repair stayed below the Principle-10 threshold). No compaction is claimed
  by this pass; the hub keeps growing.

## CASCADE
DEC-015 (status SSOT) fully propagated — traceability.md/decisions.md canonical, STATE/WIKI/design/wiki/*
status-stripped, local `state.md` deleted, `ledger_drift_lint.py` + `install_hook.py` +
`.pre-commit-config.yaml` the mechanism. **Re-verified 2026-08-30: the lint exits 0 with no findings.**
PRIOR PASS: traceability.md's REQ table gained a `Status (current)` column — that column is now the
single authoritative current verdict per id, and the TEST(s) cell is explicitly labelled HISTORICAL
EVIDENCE. `clv-lite.sh` column offsets were updated to match ($5 = DEC, $7 = TEST).

**NEW THIS PASS — ONE MECHANISM FOR CLV CHECK 5, AND ITS FULL CASCADE.** The check existed as two
hand-synced awk copies (one in `findings.md`, one in `clv-lite.sh`) whose shell copy carried a comment
asking the next editor to re-sync the other. Both copies are gone. Landed together, because any one of
these alone leaves the ledger describing something that is not there:

| artefact | change |
|---|---|
| `scripts/clv_findings.py` | NEW — the sole implementation. Fails safe on malformed row / unreadable severity / unreadable disposition / `needs-disposition`. |
| `scripts/test_clv_findings.py` | NEW — 18 synthetic self-tests, 6 asserting exit 0 and 12 exit non-zero (10 exit 1, 2 exit 2), run under both `unittest` and `pytest`. |
| `scripts/clv-lite.sh` | Check 5 now INVOKES the above and contains no algorithm; Check 1b widened to `REQ-NF-*` with grouped-row expansion; header rewritten to describe what the file now is. |
| `findings.md` | the mechanism section DESCRIBES the script instead of restating it; ROW OVERFLOW convention documented; 29 structurally-broken rows repaired. |
| `wiki/conventions.md` | the ledger-layout block names both new scripts and states which root its paths are relative to. |
| `WIKI.md` | MAP line for the ledger's `scripts/`; registries recomputed (LSN next 084, ORCH next 052); "nothing is mid-flight" replaced by the three separate facts. |
| `unittests/.../CMakeLists.txt` | the four TIMEOUT properties re-based from 900/600 onto measured 420/180. |
| `traceability.md` | 19 rows re-pointed off the retired `Stage C`/`Stage F` names; three verdicts corrected; the "no verdict changed" header replaced by a table of what changed and why. |

**NOT propagated, deliberately:** the 122 rows whose severity or disposition is prose are left visible and
failing rather than bulk-classified — that is Gate 1A, and doing it here would be the defect ORCH-049 records.

**NEW THIS PASS (2026-09-03) — GATE 1B ROUTING COMPLETE, AND ITS CASCADE.** REQ-029 (file-IO Context UAF)
and DEC-041 (Option B, hoist-and-capture) are new ids, allocated and landed together with the finding
dispositions they close out, because any one alone leaves the ledger self-contradicting:

| artefact | change |
|---|---|
| `prd.md` | NEW row `REQ-029` (must, STUB, NOT BUILT) appended after REQ-028, same table-row convention |
| `decisions.md` | NEW index row + full entry `DEC-041` (Option B chosen over A/C, scored); no other DEC edited |
| `findings.md` | 10 disposition cells changed: 7 closed by dual-backend re-execution (`S-R021-02`, `A3-R021-F1`, `A3-R021-F2`, `B-R021-11`, `B-R021-12`, `A3-R027-F1`, `A3-R027-F4`), 1 closed by direct source inspection (`B-R021-10`), 2 routed to `REQ-029` (`B-R025-01`, `A3-R021b-F2`) — each carries its `PRIOR DISPOSITION ... SUPERSEDED` block, nothing deleted |
| `STATE.md` | `## BLOCKING`/`## NEXT_GATE` Gate 1B marked PASSED; Stage 2 widened to also carry `B-R028-01`; Stage 6 widened to also carry `REQ-029`; `## COUNTS`/`## LAST_CLV` re-derived |
| `WIKI.md` | REGISTRIES: REQ next `029`→`030`, DEC next `041`→`042` |

**CORRECTED 2026-09-03 (reviewer catch, Codex): the line below was WRONG and is kept struck-through rather
than deleted, per this ledger's no-silent-rewrite convention.** ~~NOT propagated, deliberately: `traceability.md`
— REQ-029 gets its own traceability row only when it has a TEST or commit to point to (its own row's
convention, matching every other STUB REQ); adding one now would assert progress that hasn't happened.~~
**That is not this ledger's actual convention** — `REQ-024` (and its Stage-6 siblings) are stubs with NO
TEST and NO commit and DO carry a traceability row (`Status (current) = NOT STARTED — stub`). `clv-lite.sh`
Check 1b correctly failed on the missing `REQ-029` row for this reason. **Fixed this pass:** `traceability.md`
now carries a `REQ-029` row in the same stub shape as `REQ-024`'s, `Status (current) = NOT STARTED — stub,
allocated 2026-09-03`, DEC-041 cited, `—` for DES/TEST/Commit. `B-R028-01` is left OPEN in `findings.md`, not
marked resolved — Stage 2 names what's owed, it does not do the owing.

**NEW THIS PASS (2026-09-03) — STAGE 2 WORKED: DEC-033 DECIDED, B-R028-01 RE-ADJUDICATED, NEITHER BUILT YET.**
Two `qgdw-scout` dispatches (parallel, disjoint code footprints), each verified by the orchestrator
spot-checking cited line numbers before acting on the report:

| artefact | change |
|---|---|
| `decisions.md` | NEW full entry `## DEC-033` (Option A, defaulted out-param, chosen by user via Three Options Doctrine); index row (line 50) updated from `ALLOCATED · NOT RESEARCHED · NO ENTRY` to `accepted`, pointing at the entry |
| `findings.md` | `B-R027-01`/`B-R027-02`/`B-R027-03` dispositions appended (not rewritten) noting DEC-033 is decided but NOT YET BUILT; `B-R028-01`'s disposition appended with a `CORRECTED 2026-09-03` block: the 2026-08-30 "genuine decision gap" call was wrong — coverage-only, guard already ratified in the DEC-034/036 AMENDMENT |
| `STATE.md` | Stage 2 row rewritten (decision recorded, build scope named); `## BLOCKING` row 11 (`B-R028-01`) reclassified; `## LAST_CLV` updated — Check 6 now PASSES |
| `WIKI.md` | DEC registry note: 41/41 entries (was 40/41) |

**No new id allocated** — DEC-033 was already allocated 2026-08-13; this pass only gave it research + a
decision + an entry. `next:042` unchanged. **Not yet a byproduct of a PASSING build**, so no TEST/traceability
row exists for either item — that lands when `qgdw-builder` implements DEC-033's out-param and the
`B-R028-01` test slot, each verified by execution before its finding row is marked closed.

## CHANGESET  (provenance only — full history → `git log`, archive/state-history.md § 5)
HEAD **`3b8226ec4` + this pass's governance commit** (both 2026-09-06) on `garmin/req028-row-lifetime`.
**`3b8226ec4`** (parent `fd7639f7a`) — DEC-042/DEC-043, the two Stage-5 release blockers (TEST-158/159/160),
7 paths, +717/−1; the formatter-normalized test files re-verified 4/4 registrations (both QPA backends)
before the commit landed. The **governance commit** — its immediate child, carrying the 41-path slice per
`## COMMIT MANIFEST` plus this pass's ledger repairs (O-R027-01 reconciliation, `fd7639f7a` traceability
repair, STATE/WIKI cursor-drift corrections, `3b8226ec4` citations) — is the commit this STATE file rides
in, so its own hash is read from `git log`, not from here.
Prior: `fd7639f7a` (2026-09-05, DEC-040 Stage 2 / W3 — **landed without its STATE/traceability byproduct;
both repaired 2026-09-06**) · `2e26106c1` (2026-09-04, DEC-033 + TEST-076, peer-reviewed by garmin_codex_new)
· `37710370e` (2026-08-30, DEC-040 Stage-1) · `05ac6bc29` · `514d8e88f` · `9e5187ef0` · `98df50535` ·
`e48f7d123` · `da455c79e`.
Governance commits interleaved: `6b31248a3`, `f49a36aed`, `3fe8ebf6d`.
**No upstream; not pushed, not merged, not amended.**
**UNMERGED, deliberately:** `ae7655985` on `build/orch036-libusb-wiring` (DEC-039 / ORCH-036).

## LAST_CLV
**`clv_findings.py` (Check 5) re-run 2026-09-03, this session, over the current working tree — FAIL, but
Gate 1B PASSED.** Check 0/1b/2 unchanged from 2026-09-01 (not re-run this pass — only `findings.md` changed,
and its own structural checks are Check 5's job) · Check 1 WARN unchanged (6 must/should REQs without a
TEST: REQ-009, REQ-010, REQ-014, REQ-015, REQ-016, REQ-024 — **REQ-029 is a 7th but is a STUB not yet
started, same status class as these six, not a new kind of gap**) · **Check 5 FAIL — 0 MALFORMED, 0
UNKNOWN-SEVERITY, 0 UNKNOWN-DISPOSITION, 0 NEEDS-DISPOSITION, 18 OUTSTANDING (was 26), MISSING-EFFECT 0** ·
**Check 6 now PASSES, re-run 2026-09-03** (`clv-lite.sh`): DEC-033 was decided (Option A) and given a real
entry this pass, closing the WARN. Stage 2's `B-R028-01` sub-item was also re-adjudicated this pass as
coverage-only (see `## BLOCKING`); neither change moved Check 5's `OUTSTANDING=18` — no finding is closed
until its build lands and is execution-verified, per [[ORCH-050]].

**Re-run 2026-09-04, after the DEC-033 build:** `OUTSTANDING` moved **18 → 19** — `B-R027-09` (the
guard-load-bearing residual raised by the orchestrator's mutation-proof, see `## BLOCKING`/Stage 2) is a
NEW open finding, not a re-classification, so this is the gate correctly growing to cover a hazard the
build itself surfaced. Check 1b and Check 6 both still PASS.

**Re-run again 2026-09-04, after `TEST-076` landed:** `OUTSTANDING` moved **19 → 15** —
`B-R027-01`/`B-R027-02`/`B-R027-03`/`B-R027-09` all dispositioned on execution evidence (targeted
regression + two independent RED-under-mutation reproductions), per the [[ORCH-050]] verify-then-transcribe
bar. `OK` moved `346 → 350`, all other counters unchanged.

**Re-run a third time 2026-09-04, after `TEST-157` landed:** `OUTSTANDING` moved **15 → 14** —
`B-R028-01` dispositioned on execution evidence: `qgdw-builder` built the slot and mutation-proved it;
the orchestrator independently re-executed it on both QPA backends (PASS, EXIT=0, zero ASan output),
independently repeated the mutation under `cp`/`cmp` snapshot-restore discipline and reproduced the
identical RED signature, then ran the full `ctest -L garmin-fast` regression (27/27, EXIT=0). This closes
Stage 2 (`## BLOCKING`'s stage table) — every finding it carried is now dispositioned. `OK` moved
`350 → 352`; total rows measured `366` (was `365` — see the row-count note in `## BLOCKING`, not
reconciled further this pass). No other counter moved.

**Re-run 2026-09-05, after Stage 4 (provider-watchdog checkpoint) dispositioned:** `OUTSTANDING` moved
**14 → 9** — `C1`, `C2`, `C3`, `C4`, `C6` all dispositioned CLOSED on RE-EXECUTED evidence this session
(`testCloudProviderWatchdog` 168/168 both `offscreen`/`minimal`, EXIT=0, byte-identical PASS lists, zero
ASan; full `ctest -L garmin-fast` 27/27, EXIT=0, 259.35s), discharging the complete
`{CHECKPOINT:provider-watchdog slice}` effect set. `OK` moved `355 → 361`, row count `369 → 370` (the +1 is
`ORCH-056`, a new non-blocking finding — see `## BLOCKING`). `MALFORMED`/`UNKNOWN-SEVERITY`/
`UNKNOWN-DISPOSITION`/`NEEDS-DISPOSITION`/`MISSING-EFFECT` all still 0; validator self-tests 18/18;
`ledger_drift_lint.py` EXIT=0. This closes Stage 4 (`## NEXT_GATE`'s stage table) — every finding it carried
is now dispositioned. **The FAIL changed CHARACTER a second time, and that is the whole result of this pass.** Before Gate 1A the
failure was *"the mechanism cannot read 122 of its own rows"* — an unknown gate. After Gate 1A/1B it became
*"26 rows are blocking and open, 11 with no route"*. It is now *"18 rows are blocking and open, EVERY ONE
ROUTED to a stage"* — Check 5 still fails (findings remain open) but Gate 1B, a DIFFERENT and now-exited
gate, no longer contributes to why. A smaller number, and the remainder has homes. Check 5's exit code is 1
in both cases and the
project may not commit on either, but only the second one can be worked.

**Re-run 2026-09-05, after DEC-042/TEST-158 landed (B-R028-17 closed):** `OUTSTANDING` moved **9 → 8** —
`B-R028-17` dispositioned on execution evidence, independently re-verified by the orchestrator (see
`## BLOCKING`/Stage 5 and `decisions.md`'s DEC-042 entry). `OK` moved `362 → 364`, row count `371 → 372`
(the two new rows are `ORCH-057` and `ORCH-058`, both non-blocking, raised this session while scouting/
building DEC-043 and DEC-042 respectively). `MALFORMED`/`UNKNOWN-SEVERITY`/`UNKNOWN-DISPOSITION`/
`NEEDS-DISPOSITION`/`MISSING-EFFECT` all still 0; `test_clv_findings.py` 18/18; `ledger_drift_lint.py`
EXIT=0. The remaining 8 OUTSTANDING rows are Stage 5's one remaining release blocker (`A3-R028e-F1`,
DEC-043 decided, not yet built), Stage 6's five UAF-family stubs, and the stale `O-R027-01` row.

**Re-run 2026-09-05, after DEC-043/TEST-159+160 landed (A3-R028e-F1 dispositioned — Stage 5 COMPLETE):**
`OUTSTANDING` moved **8 → 7** — `A3-R028e-F1` dispositioned on execution evidence, independently
re-verified by the orchestrator including its own killing mutation (guard neutered → identical
`heap-use-after-free` at `CloudService.cpp:712`, restored byte-identical via Edit + `cmp`); watchdog
170/170 + syncdialog 99/99 on both `offscreen`/`minimal`, `ctest -L garmin-fast` 27/27 (317.27s), and
`ninja GoldenCheetah` links the real `Athlete.cpp` change (see `## BLOCKING`/Stage 5 and `decisions.md`'s
DEC-043 entry). `OK` moved `364 → 366`, row count `372 → 373` (the new row is `ORCH-059`, non-blocking —
TEST-160 drives a hand-mirrored stub `Athlete::close()`, no automated lockstep check; raised at
verification). `MALFORMED`/`UNKNOWN-SEVERITY`/`UNKNOWN-DISPOSITION`/`NEEDS-DISPOSITION`/`MISSING-EFFECT`
all still 0. **With this, Stage 5's exit condition is met — every Gate-1B-routed `{RELEASE}` effect is
discharged on executed evidence, and the remaining 7 OUTSTANDING rows are Stage 6's six routed UAF-family
findings (across five REQ stubs) plus the stale `O-R027-01` row.**

**Re-run 2026-09-06, after `O-R027-01` was reconciled (user decision D3, this session): `OUTSTANDING` moved
7 → 6.** The row's closure evidence is THIS session's own execution — `ninja`: no work to do (fresh build),
then the three DEC-033 binaries via ctest, **3/3 PASSED, EXIT=0** — transcribed per the [[ORCH-050]]
verify-then-transcribe bar. All other counters unchanged over 373 rows: **0 MALFORMED · 0 UNKNOWN-SEVERITY ·
0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 6 OUTSTANDING · 367 OK · MISSING-EFFECT 0**, exit 1. The six
remaining rows are exactly Stage 6's UAF-family set; Check 5 now fails on Stage 6's open work and on nothing
else. Same session, on the code side: the DEC-042/043 slice was committed (`3b8226ec4`) after the pre-commit
clang-format hook normalized the two test files — a new content version, so the affected targets were re-run
first (**4/4 ctest registrations PASS, EXIT=0**: syncdialog + watchdog, both QPA backends).

**VAL-018 (2026-08-23) is NOT evidence for `37710370e` and NOT evidence for this working tree.** No full
CLV has been run over `37710370e`. Full VAL history → traceability.md `## Validations run`.

## LAST_CYCLE
**A3-FINAL on the DEC-040 Stage-1 accepted repair state — 2026-08-29 — VERDICT: FINDINGS, NONE BLOCKING.**
**No adversarial cycle has been run against `37710370e` itself, and none against this working tree.**
Prior cycles → traceability.md `## Cycles run`, `cycles/archive/`, archive/state-history.md § 5.

Detail lives in: traceability.md (per-id spine, `Status (current)` column) · findings.md (finding
disposition) · decisions.md (## Decision index, then entries) · validations/archive/ + cycles/archive/
(historical execution evidence) · archive/state-history.md (all superseded cursor narrative).
workflow-aicoach/ is a retired ledger (provenance only — see .claude/workflow-INDEX.md).
