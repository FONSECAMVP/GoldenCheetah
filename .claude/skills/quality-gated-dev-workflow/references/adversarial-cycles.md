# Adversarial Cycles (A0–A5)

You are the adversary; the user reviews findings and picks dispositions. Rigor is
unchanged from the original. **Output discipline (lean):** write the full narrative to a
cold `cycles/` file, but push **only open/blocking findings** (one line each) to the hot
`findings.md`. On disposition, condense the finding to one archived line and remove it
from the hot path.

## Disposition rule (every finding ends in one of)
`fix-now` (→ link commit/decision) · `defer` (→ ticket id) · `accept` (→ written
rationale). Never "we'll get to it." Blocking findings keep the phase from exiting until
dispositioned.

---

## A0 — "Should we build this?" (end of Phase 0)
Answer in writing before DEC-001: (1) what existing tool already solves this and why it's
insufficient *for this user*; (2) smallest non-code path and why it breaks down; (3)
smallest code slice delivering 80% of value; (4) cost of doing nothing. Blocking if any
answer points to "build less" or "don't build."

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
(1) Mutation testing — flip operators / swap conditionals / delete statements; surviving
mutants mean decorative tests. (No tool? manually mutate ≥5 things in the central file.)
(2) Property/fuzz — ≥1 property per non-trivial input (round-trip, idempotence,
invariants). (3) Negative-path — invalid input, auth failures, partial failures. (4)
Boundary sweep — 0/1/max/max+1/empty/null and typed min/max. Loop until no surviving
mutants and no untested boundary for the feature.

## A4 — Pre-Production Hardening (Phase 3, before promotion)
(1) Staging soak under realistic load against the observability plan. (2) **Rollback
drill — actually roll back staging now**, time it; broken/slow rollback is blocking. (3)
Chaos pass — kill a process, drop a store connection, slow a downstream 5×; confirm
graceful degradation. (4) Alert dry-run — force each alert; confirm it fires, routes, and
carries enough to act on.

## A5 — Post-Launch Retrospective (after the success-metric window)
(1) Did metrics move as predicted? If not, update REQ/DEC. (2) Surprises not in A2/A4 →
update those prompt sets. (3) Did persona walks predict real behavior? (4) Actual vs
worst-case cost; recalibrate. A5 findings seed the next iteration's Phase 0 — the only
cycle that improves the workflow itself. **A5 is the richest lesson-harvest point:** for
each recurring or costly finding, capture or promote a guard in `lessons.md`
(`references/lessons-memory.md`) so the next iteration starts smarter.

---

## Cumulative discipline
A deferred finding stays as one line in `findings.md` and surfaces in CLV Check 5 at the
next gate until dispositioned. Deferred ≠ disappeared.

## Wiki upkeep from cycles
A finding that concerns *system structure* (a component risk, an integration hazard, a
scaling cliff) also adds or updates a one-line entry in `wiki/architecture.md`'s **Watch**
list, so the project brain reflects known live risks. This is a byproduct of dispositioning
the finding, not a separate step.
