# Builder briefing shape (`garmin_builder_stage9_vN`, pane w1:pM)

Cached 2026-09-15 by `garmin_inspector_v1_24` from `/tmp/builder_v7_brief_s916_partA.md` (the
shape v7 was born with), cross-checked against the repair-round briefs. This stores the SHAPE,
not a task text — the task framing is always written fresh for the new atomic unit.

The builder is restarted from zero regularly (soft-landing at 250k), so it is usually COLD with
no scrollback. Every brief must work standalone. Re-verify auto mode before the FIRST dispatch
to a newly started pane: `herdr pane read w1:pM --source visible --lines 6` must show
`auto mode on`, and `tok 0k/0k` confirms it is genuinely fresh.

## Section order (mirror this)

1. **Identity + continuity.** Name, standing role, and whether it is cold. If it inherits a
   predecessor's uncommitted work, say so plainly, name the predecessor, say nothing was lost,
   and point at the predecessor's report files as the orientation path.
2. **HARD HOLD block, in a banner, BEFORE any work description.** `src/Core/main.cpp` and any
   qmake/make against `src.pro` are frozen pending the user's live Garmin sync re-test; state
   the reason (a rebuild swaps the binary under the user and destroys evidence) and the
   freshly re-verified evidence that the test still has not run. State that CMake + ctest
   against `./build` ARE allowed.
3. **CURRENT STATE,** prefixed "verify with git status / git diff before trusting any of this."
   Enumerate which dirty paths are the builder's, which are FROZEN and whose they are, and the
   standing "ignore the unrelated in-flight work, never `git add -A`" rule.
4. **READ THESE FIRST, IN THIS ORDER** — a numbered reading path: the governing DEC in full,
   prior build/repair reports, prior reviewer findings files. Name which one is the worklist.
5. **WHAT THIS UNIT IS,** stated as a mechanism so the builder can judge the brief rather than
   follow it. Include which round it is and what the earlier rounds already did.
6. **WHAT THE INSPECTOR PROVED PERSONALLY** — measurements, probe directories left on disk,
   real command output, explicitly separated from anything relayed. Include disproved
   suspicions so the builder does not re-open them. Invite re-verification.
7. **THE UNIT, as numbered items,** each tagged BLOCKING / NON-BLOCKING, each with file:line,
   the required direction, and "MY RULING" where the Inspector has decided a design question —
   *with the reasoning, including where the Inspector changed its mind and why.*
8. **PROVE IT** clauses attached to each item: the specific mutation that must go RED where it
   previously went green, and for side-effect findings, an assertion that the side effect did
   NOT occur.
9. **WHAT IS ALREADY SETTLED — do not redo, do not re-open.** Closed findings, accepted
   enumerations, and items deliberately deferred to another ledger row.
10. **ALLOWED PATHS,** exhaustive, followed by an explicit NOT list.
11. **LESSONS THAT ARE LIVE RIGHT NOW — not boilerplate,** numbered, each tied to a real
    incident in this project (vacuous tests; verify against the artifact that actually runs;
    never `git checkout --` a file carrying uncommitted work; prove the mutation is live before
    reading the verdict; report real output and push back on wrong premises).
12. **Closing constraints:** which test subsets to run and which NOT to (the Inspector runs the
    320s full gate itself); write the full report to a `/tmp/..._report_....md` file as well as
    reporting; do NOT commit, do NOT stage; report and stop.

## Comment density — a correction to this shape, 2026-09-16

**Sections 7 and 11 above have a side effect that took four rounds to notice, and every
Inspector so far has caused it.** "MY RULING, *with the reasoning*" and "record why" are
instructions the builder faithfully applies **to the source files**, not just to its report.
Measured on this unit: `.pre-commit-config.yaml` 83 comment lines to 35 config lines;
`unittests/buildguard/CMakeLists.txt` 160 to 98; Part A's `garmin_gate_coverage_guard.py`
377 prose to 318 code — more prose than code, and already committed.

**This is not a style complaint. It manufactures the defect the guards exist to catch.**
B-STAGE9-17 is an open finding caused by precisely this: a comment claiming a ctest tier
"DOES run" when nothing ran it. A comment asserting behaviour is an unverified claim sitting
beside verified code, and nothing checks it.

So when writing sections 7 and 11, state explicitly that the reasoning goes in the BUILDER'S
REPORT and the Inspector's ledger — not into the source. Include this rule:

1. A comment stating a checkable fact about behaviour is not allowed — make it an assertion
   or a test, or delete it.
2. WHY belongs in `decisions.md` / `findings.md`; reference it from code by id in one line.
3. Project history (which commit shipped what, which round found what) never belongs in
   source; it is already in the ledgers.
4. KEEP one-line intent above non-obvious logic, and any reason string that is **runtime
   data** the tool prints or length-checks (e.g. the lint guard's `Gap(reason=...)`) — that
   is data, not commentary.
5. Target prose-to-code <= 0.3 for new or touched code.

Scope it to new/touched code each round. A retrofit pass over already-committed files is its
own ledger row, not a rider on a repair.

## Dispatch mechanics

Long briefs go to a file; prompt with
`herdr agent prompt w1:pM "You are <name>. Read <path> in full -- it is your complete briefing
and you have no scrollback... Carry out exactly what it asks."`
