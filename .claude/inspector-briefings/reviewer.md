# Reviewer briefing shape (`garmin_codex_reviewer`, pane w1:pD)

Cached 2026-09-15 by `garmin_inspector_v1_24` from the pane's first-ever prompt
(`/tmp/reviewer_brief.md`, 2026-09-14 20:01) cross-checked against the most recent
round-1 and round-2 briefs. This stores the SHAPE, not a task text — the framing for each
new atomic unit is always written fresh.

Note: this pane's herdr AGENT NAME is empty (`herdr agent list` shows `-`); `herdr pane
rename` sets the pane LABEL, not the agent name, and does not fix it. Address it by pane id.

## Section order (mirror this)

1. **Identity + amnesia premise.** "You are garmin_codex_reviewer, the standing INDEPENDENT
   delta-check reviewer for the GoldenCheetah Garmin Connect integration (this checkout).
   Assume you remember nothing about this unit." Say who the current Inspector is when it
   has changed.
2. **Role, stated as a boundary.** Read the REAL uncommitted diff; hunt defects the
   builder's own tests would not catch. Does NOT fix, stage, or commit. Explicitly: *the
   Inspector re-runs the tests itself, so re-running them is not your job — independent
   reading is.*
3. **WHAT TO REVIEW.** The exact `git diff -- <paths>` command, path by path, with line
   counts. Always call out NEW UNTRACKED files by name, because `git diff` will not show
   them. Then the explicit ignore-list of unrelated in-flight work in this tree
   (Coach/, Qt 6.8 porting, .claude/ tooling, other builders' frozen deliverables).
4. **WHAT IT IS FIXING.** The ledger id(s) and the governing DEC, with a pointer to read
   the DEC entry itself as the authority. State the defect as a *mechanism*, not a label.
5. **GROUND TRUTH the Inspector verified personally,** flagged as such and separated from
   what the builder claims — plus an explicit invitation to re-verify and to say the
   Inspector's probe was wrong. On a repair round, also list what the Inspector has
   already closed so the reviewer does not spend time there.
6. **THE CENTRAL QUESTION** — one adversarial question, answered FIRST and EXPLICITLY.
   Typically: can this check report success without having examined anything?
7. **SPECIFIC HAZARDS TO HUNT,** numbered, each with a concrete mechanism and file:line,
   ending "not exhaustive — add anything you find." Demand a verdict, not a hedge, on the
   ones the Inspector must rule on.
8. **CONTEXT YOU SHOULD HAVE, so you do not file it as a finding** — known-expected
   failures, frozen deliverables, measured baselines.
9. **SCOPE GUARDRAILS** — read-only; what may and may not be executed this round (say when
   the Inspector is running the gate concurrently in the shared `build/` tree); the standing
   HARD HOLD on `src/Core/main.cpp` and any qmake/make against `src.pro`.
10. **DELIVERABLE** — findings list, each with file:line, a concrete failure scenario
    (inputs/state → wrong outcome), and BLOCKING / NON-BLOCKING. "If you find nothing
    blocking, say so plainly — do not manufacture findings to look thorough." "If you
    disagree with a premise in this brief, say so; the Inspector has been wrong before and
    wants to be corrected."
11. **Write the findings to a `/tmp/...findings.md` file as well as printing them,** so they
    survive a context reset. (`herdr agent read --source recent` returns a stale screen when
    the pane is scrolled back; the file is the documented recovery.)

## Repair-round additions

On a round 2+, add: a pointer to the previous round's findings file and to the builder's
repair report; "TREAT EVERY CLAIM IN THE REPORT AS A CLAIM TO BE CHECKED BY READING, not as
evidence"; the Inspector's disposition for each prior finding including any the Inspector
*overruled*, with the reasoning, and an invitation to challenge the overrule; and a request
for an explicit CLOSED/NOT-CLOSED verdict per prior finding.

## Dispatch mechanics

Long briefs go to a file; prompt with
`herdr agent prompt w1:pD "Read <path> and carry out exactly what it asks."`
