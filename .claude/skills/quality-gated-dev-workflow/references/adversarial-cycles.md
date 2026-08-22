# Adversarial Cycles (A0–A5)

You are the adversary; the user reviews findings and picks dispositions. Rigor is
unchanged from the original. **Output discipline (lean):** write the full narrative to a
cold `cycles/` file, but push **only open findings** (one line each, carrying their BLOCKS
effects) to the hot `findings.md`. On disposition, condense the finding to one archived line and remove it
from the hot path.

## Disposition rule (every finding ends in one of)
`fix-now` (→ link commit/decision) · `defer` (→ ticket id) · `accept` (→ written
rationale). Never "we'll get to it."

**Every finding is also assigned a BLOCKS effect set** by the orchestrator — zero or more of
`TASK:<task-or-id>`, `CHECKPOINT:<slice>`, `RELEASE`, `DEPLOY` (`references/orchestration.md`).
The adversary reports *technical severity*; the effects are assigned at merge, and scope is
**proven, not assumed** — unproven scope means `TASK:<active-slice>`, never a global block. A
finding carrying `RELEASE` keeps the release from happening until dispositioned; a finding
carrying only `CHECKPOINT:<slice>` stops that slice's verified checkpoint and nothing else; an
empty set is advisory and stops nothing.

---

## A0 — "Should we build this?" (end of Phase 0)
Answer in writing before DEC-001: (1) what existing tool already solves this and why it's
insufficient *for this user*; (2) smallest non-code path and why it breaks down; (3)
smallest code slice delivering 80% of value; (4) cost of doing nothing. Severity blocking —
effects `{TASK:<the build itself>}`, plus `RELEASE` once there is something to release — if
any answer points to "build less" or "don't build."

## A1 — Requirements Red Team (Phase 1, after PRD)
Four hats in sequence:
- **Lawyer** — is each acceptance criterion objectively verifiable, or could two engineers
  disagree? Flag "fast", "easy", "secure" with no measure.
- **Persona** — walk three stories as novice / power user / malicious actor. The malicious
  walk is mandatory even for "trusted-user-only" tools.
- **Edge case** — per input: empty / one / max / max+1 / negative / zero / Unicode / long
  / concurrent / network-failure-mid-op / clock-skew. Each needs a *decision*, not
  necessarily a test.
- **Contradiction** — pair every REQ with every other; flag conflicts and silent deps.

## A2 — Design Pre-Mortem (Phase 1, after design)
Imagine it failed six months out. (1) Three failure narratives (technical / organizational
/ security), concrete. (2) Chaos questions: primary store down 30 min; network partition;
10× traffic; dependency yanked; 3-week unattended. (3) Cost worst-case at 10× / 100× and
where the cliff is. (4) Migration trap for the component you're least sure of.
**If a finding is rooted in a foundational DEC, re-open that DEC via cascade — fix the
root, not the symptom.**

## A3 — Test Hardening (Phase 2, per feature)
(1) Mutation analysis — you SPECIFY the mutations, the orchestrator EXECUTES them (you are
read-only; applying a mutation is a file edit you must not make). Produce a numbered
mutation list for the feature's central mechanism (operator flips, guard removals, swapped
conditionals — ≥5 when no mutation tool exists for the language) with, per mutation, the
exact edit and the test(s) expected to kill it. The orchestrator applies each under
snapshot discipline (`cp f f.orig`, restore + `cmp`; never `git checkout --` on a dirty
file), runs the suite, and returns the kill/survive record — surviving mutants mean
decorative tests. Where a real mutation tool exists, name it and the invocation instead.
(2) Property/fuzz — ≥1 property per non-trivial input (round-trip, idempotence,
invariants). (3) Negative-path — invalid input, auth failures, partial failures. (4)
Boundary sweep — 0/1/max/max+1/empty/null and typed min/max. (5) Tautology scan — grep the
changeset's tests for unconditionally-true assertions (`|| true`, `or True`,
literal-true disjunctions): they are why "covered" lines let mutants survive. (6) Standing
probe: **"what else is green but dead?"** — enumerate integration boundaries no test
crosses (a criterion phrased end-to-end satisfied only at an inner seam is not covered).
Loop until no surviving mutants and no untested boundary for the feature.

## A4 — Pre-Production Hardening (Phase 3, before promotion)
(1) Staging soak under realistic load against the observability plan. (2) **Rollback
drill — actually roll back staging now**, time it; a broken or slow rollback carries
`{RELEASE, DEPLOY}`. (3)
Chaos pass — kill a process, drop a store connection, slow a downstream 5×; confirm
graceful degradation. (4) Alert dry-run — force each alert; confirm it fires, routes, and
carries enough to act on.

## A5 — Post-Launch Retrospective (after the success-metric window)
(1) Did metrics move as predicted? If not, update REQ/DEC. (2) Surprises not in A2/A4 →
update those prompt sets. (3) Did persona walks predict real behavior? (4) Actual vs
worst-case cost; recalibrate. A5 findings seed the next iteration's Phase 0 — the only
cycle that improves the workflow itself. **A5 is the richest lesson-harvest point:** for
each **recurring or costly** finding — i.e. one that clears the capture threshold — capture or
promote a guard in `lessons.md` (`references/lessons-memory.md`) so the next iteration starts
smarter. A first-occurrence local implementation or harness bug found here is still a repair,
not a lesson.

---

## Bounded repair (harness and process defects)

A harness/scaffolding/process defect receives a **direct bounded repair**. Its evidence is the
complete affected target rerun on the final content version. It creates **no fresh A-cycle,
CLV, lesson, registry entry, or narrative wave** unless:

- production code changes;
- a distinct production hazard is exposed;
- the problem recurs; or
- the workflow mechanism itself is defective.

**An attempt counts only after both a material patch and an executed reproducer.** Hook
prompts, unavailable tools, infrastructure failures, and command typos do **not** consume an
attempt. **After two unsuccessful material attempts, return to the user** unless another
attempt is explicitly authorized — never recursively allocate more process work.

## Focused re-check after a production blocking fix ("attack the fix")
A **production** finding's fix gets a focused re-check — not a full new cycle — whose brief
is: attack the fix. (A harness repair's re-check *is* its rerun of the complete affected
target on the final content version; it does not also get an attack-the-fix pass.) Ask (1) is the defect closed or merely harder to hit (a second route to
the same hazard)? (2) **is the guard on the layer that performs the unsafe operation**, or
only the layer that initiates it (a destructor cannot be vetoed the way a virtual can)?
(3) did the fix create a new hazard (field-proven: a leak fix became a use-after-free)?
State plainly what is closed and must not be re-litigated.

## Briefing an A-cycle (orchestrator side)
A-cycle briefings must include 3–5 **mandated probes mined from the builder's NOTES** (the
builder names its own weak spots; aim the adversary at them) and must demand a **REFUTED
section** — a refutation retires an open question and is as valuable as a finding.

## Cumulative discipline
A deferred finding stays as one line in `findings.md`, **with its BLOCKS effects**, and
surfaces in CLV Check 5 at any gate its effects intersect, until dispositioned. Deferred ≠
disappeared — but a deferred finding whose effects don't touch the gate being evaluated does
not fail that gate.

## Wiki upkeep from cycles
A finding that concerns *system structure* (a component risk, an integration hazard, a
scaling cliff) also adds or updates a one-line entry in `wiki/architecture.md`'s **Watch**
list, so the project brain reflects known live risks. This is a byproduct of dispositioning
the finding, not a separate step.
