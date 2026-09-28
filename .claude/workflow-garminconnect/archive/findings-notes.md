# Findings — preamble and historical notes (moved verbatim 2026-09-28)


One row per finding ever raised by any cycle. The source of truth for CLV Check 5. Cycle files hold the *why*; this file holds the *closure*.

### CLV Check 5 — the mechanism (POINTER, not a copy — repaired 2026-08-30)

**The mechanism is `scripts/clv_findings.py`. This section describes it and does not restate it.**

```sh
scripts/clv_findings.py                 # human-readable report; exit 0 = pass, 1 = fail, 2 = bad input
scripts/clv_findings.py --limit 0       # every row of every failing bucket
scripts/clv_findings.py --json          # machine-readable
python3 -m unittest discover -s scripts -p 'test_clv_findings.py'   # its 18 synthetic self-tests
```

**Why this section no longer contains the algorithm.** It used to hold an awk one-liner, and
`scripts/clv-lite.sh` held a hand-made copy of the same one-liner, with a comment in the shell
copy asking whoever edited this block to remember to re-sync the other. Two copies of a predicate
are two predicates, and the gate's verdict depended on which one you happened to run. There is now
exactly one implementation, it lives in `scripts/`, and — closing the "Known residual" this section
used to carry — it has a companion test file the way `ledger_drift_lint.py`/TEST-017 do.

**The five concepts, kept apart on purpose.** Conflating any two of them is how this register
produced a clean gate over a live blocker:

| concept | question it answers | unreadable ⇒ |
|---|---|---|
| malformed row | can the row be read at all? | **FAIL** |
| severity | how bad is it? | **FAIL** |
| disposition | is work still owed? | **FAIL** |
| effect set (`BLOCKS: {…}`) | what does it block? | an INDEPENDENT axis |
| outstanding | blocking severity **and** no closing disposition | **FAIL** |

`OUTSTANDING` is computed from severity and disposition ONLY. **Writing a `BLOCKS: {…}` set onto a
row closes nothing and moves nothing out of `OUTSTANDING`** — an effect set says what a finding
blocks, a disposition says whether anyone still owes work on it, and they are separately true or
false. A gate whose actions cannot change its own pass criteria is not a gate, and one previous
NEXT_GATE was written on the opposite belief.

**What changed, and in which direction.** Every change makes MORE things fail, never fewer:

- *Malformed rows were fail-OPEN.* 28 rows split into the wrong number of cells and were counted
  `NEEDS-CLASSIFICATION` — "reported, not blocking" — while several of them literally begin
  `**BLOCKING — …**`. A 29th was worse: `ORCH-009` had seven columns AND no trailing pipe, which
  splits into exactly the field count a well-formed six-column row produces, so the old predicate
  read it as valid and took its severity from the wrong cell. All 29 are repaired (see ROW
  OVERFLOW below) and an unreadable row is now a hard failure.
- *Prose severities were fail-OPEN.* The old predicate was "if the severity cell does not start
  with `blocking`, skip this row". A cell reading `The three completion slots' TAIL …` does not
  start with `blocking`, so the row left the check entirely. That is not a finding that the row is
  harmless; it is the absence of any finding. Such rows are now `UNKNOWN-SEVERITY` and they fail.
- *Prose dispositions were read by accident*, as open or closed depending only on which word they
  happened to begin with. They are now `UNKNOWN-DISPOSITION` and they fail.
- *Vocabulary*, unchanged from VAL-018's ruling and carried across deliberately: matching is
  case-insensitive on the cell's LEADING token, so `FIXED 2026-08-03 (mitigated per DEC-020…)`
  counts as dispositioned, and `blocking-for-clean-build` / `was-blocking-for-production` count as
  blocking. `fix-now` remains a CLOSURE — its convention here is `fix-now` plus the actual
  resolution in `resolved-by` — because re-reading it as open would silently re-open 36 historical
  rows on a vocabulary opinion rather than on evidence.
- *`NEEDS-DISPOSITION`* (added 2026-08-23 when B-R028-17 exposed the hole: a finding that may well
  be blocking, but whose severity the orchestrator declined to set unilaterally, was invisible)
  now FAILS rather than being reported-and-passed. Declining to classify must be visible, and an
  undetermined severity is an undetermined gate. This also retires ORCH-041, whose defect was that
  the old branch short-circuited before the disposition check so the bucket could never self-clear.

### ROW OVERFLOW — what `**[overflow col N …]**` means

The register schema has SIX columns. Twenty-three rows had been written with a seventh (two with an
eighth and ninth) holding extra narrative. **Every Markdown renderer DROPS a cell beyond the header
count**, so that text was already invisible in any rendered view of this file and unaddressable by
any positional mechanism. Those cells were merged into `resolved-by`, each introduced by
`**[overflow col N — preserved verbatim …]**`. Nothing was deleted, rewritten or summarised: the
repair was verified by a content-stream invariant — every non-whitespace character of the original
row is present in the repaired row — on all 29 rows. Six further rows carried an unescaped `|`
inside a cell (mostly a C++ `||` inside backticks; GFM does not let a code span protect a pipe, so
`\|` is the only escape) and those pipes are now escaped.

**Do not add a seventh column.** Put the narrative in `resolved-by`.

**Current run (2026-09-01, AFTER the A3-R027-F8 closure), over 364 rows:**
**0 MALFORMED · 0 UNKNOWN-SEVERITY · 0 UNKNOWN-DISPOSITION · 0 NEEDS-DISPOSITION · 27 OUTSTANDING
· 337 OK**, and **0 of the 27 outstanding rows carry no effect set** (`MISSING-EFFECT = 0`). (2026-08-30: 34/330.)
Reproduce with `scripts/clv_findings.py`; exit code is 1, because `OUTSTANDING` is non-empty.

**`OUTSTANDING` ROSE 15 → 34, and that is the gate WORKING, not a regression.** It was 15 only
because 114 rows carried a severity no mechanism could read and 8 carried an unreadable
disposition — those rows were unjudged, not harmless. Gate 1A transcribed each cell's severity and
disposition from the sentence in that row's own body that already established it, keeping the
original text after the prefix; nothing was re-adjudicated and no row's meaning was changed. The
arithmetic of the change: −3 (ORACLE-F1, A3-R028c-F1, A3-R028c-F2, adjudicated on re-executed
evidence — see [[ORCH-050]]), −1 (ORCH-050 itself, resolved by that adjudication), +23 rows that
were already blocking and open and are now legible to the mechanism.

**Effect sets are NOT dispositions.** Gate 1B wrote an explicit `BLOCKS: {…}` set onto all 27
outstanding rows that lacked one, read off each row's OWN recorded harm, with scope PROVEN rather
than assumed (`references/orchestration.md`) — so an unproven-scope row names its raising TASK, never
a global block. `OUTSTANDING` stayed at exactly 34 across that edit, which is [[LSN-083]] executed
rather than asserted.

Historical runs, kept for provenance and NOT a current status: the 2026-08-30 gate-repair pass
reported 0 / 114 / 8 / 0 / 15 / 227; 2026-08-24 reported 11 OUTSTANDING / 27 NEEDS-CLASSIFICATION;
2026-08-23 reported 8 / 27 / 1. The last two came from the fail-open predicate and are not
comparable with the ones above.



## Historical block formerly inside the register

### REQ-028 Option C disposition override — 2026-08-22  (HISTORICAL NARRATIVE — NO CURRENT AUTHORITY)
**RELABELLED 2026-08-30 by Gate 1A, closing [[ORCH-050]]. This block is NOT a disposition source and
carries no verdict.** It was written as a second disposition source for rows whose own cells said
something else, which is exactly the defect ORCH-050 records: two statements of one finding's
disposition, in one file, disagreeing. Every verdict below has been moved INTO the disposition cell
of the row it is about, with that row's original text preserved verbatim after it, and each move was
made only after the cited evidence was RE-EXECUTED at `37710370e` (TEST-128; TEST-129 across three
completion channels and both drivers; `GC_FUZZ_SEED=447` — all PASS on BOTH `offscreen` and
`minimal`) and the prescribed fixes were located in the shipped source. **Read the row, not this
block.** Kept, unedited below, as dated provenance for how those verdicts were reached:
- **A3-R028c-F1 — RESOLVED.** TEST-128 was RED on the stale completion relabelling the restarted row, then GREEN after operation-keyed records survived Refresh with a null row.
- **A3-R028c-F2 — RESOLVED.** All three completion-tail rows and both parseable-driver rows in TEST-129 were independently RED, then GREEN after batch-generation checks; seed 32 passes on both backends.
- **ORACLE-F1 — RESOLVED.** The common stale-frame generation mechanism closes the held-row route; seed 447 passes on both backends under ASan.
- **A3-R028c-F3/A3-R028c-F8 — RESOLVED.** DEC-036 Option C removed scalar/capped retirement; TEST-126 now directly enforces dispatches = matched completions + outstanding records on every generated run.
- **New A3 attack-the-fix blockers — RESOLVED before acceptance.** SixCycle now reads identity from the actual sender reply; syncNext/uploadNext recheck generation after parseable openRideFile suspension; TEST-126 consumes the diagnostic accessor; the queued test callback no longer captures a dead stack local. Full offscreen is 78/0 and `garmin-fast` 25/0.
- **A3 recheck residual — OPEN, non-blocking.** SixCycle's sender-bound fix is source-reviewed and application-build verified, but no unit test crosses its real two-`QNetworkReply` reversed-completion boundary. Add that provider-specific integration test in the Cloud-adapter hardening slice.
- **A3 boundary residual — OPEN, non-blocking.** The `quint64` allocator skips zero but has no forced-wrap/live-ID collision policy or test. This requires roughly 2^64 allocations while the colliding record remains live and is not release-blocking; define collision retry and a forced-boundary seam before changing allocator semantics.
- **A3 recheck oracle residual — RESOLVED.** TEST-126 prose now describes direct observation and completion helpers resample `outstandingTransferCount()` after production removal; focused offscreen/minimal each pass 6/0.
REQ-028(c) remains partial for the independent sorting/positional-driver slice; this override does not close that defect.
