# Proportional Rigor — calibrate once, then get out of the way

Not every project deserves the same ceremony. A 100-line utility script drowned in PRDs and
pre-mortems gets worse outcomes (the process becomes the overhead); a production system run
like a script ships avoidable failures. So the FIRST decision of every project — and of any
*significant* feature added later — is the **rigor tier**. It is decided once, via the
Three Options Doctrine like everything else, recorded in `STATE.RIGOR`, and then applied
silently: the tier changes *which* mechanisms run, never *how carefully* they run.

## The calibration decision (Phase 0, right after intake)

Present three tiers as a standard scored proposal. The scoring signals are objective —
answer them from the intake, don't guess:

| Signal | pulls LIGHT | pulls FULL |
|---|---|---|
| Blast radius on failure | only me, re-runnable | other people, money, data loss |
| Users | me / a script | external users or teammates |
| Data | none / throwaway | credentials, PII, payments |
| Exposure | local only | network-facing / deployed |
| Expected lifespan | days–weeks | 6+ months, maintained |
| Expected size | < ~20 files, < ~8 REQs | > ~50 files, > ~25 REQs |
| Reversibility of mistakes | everything cheap to redo | one-way doors present |

Two+ signals in the FULL column → recommend FULL. All signals LIGHT → recommend LIGHT.
Mixed → STANDARD. State the recommendation with the signals that drove it; reversibility of
the calibration itself is **cheap** (escalation is designed in, below).

## The tier matrix — what each tier changes

| Mechanism | LIGHT (scripts, one-offs, prototypes) | STANDARD (default) | FULL (production scale) |
|---|---|---|---|
| Intake | 5-line mini-brief (goal, user, done-when, constraints, non-goals) | full intake + 5-Whys + ambiguities | same as standard |
| Requirements | done-when checklist (3–8 bullets), no formal PRD | PRD with REQ ids + acceptance criteria | standard + NFRs mandatory (perf, security, observability, cost) |
| Decisions | doctrine ONLY for one-way doors and money; everything else just decided and one-lined in the index | doctrine for every shaping choice | standard + an ADR-style DEC per component |
| Adversarial cycles | ONE combined 10-minute pre-flight: top-3 failure modes + "what existing tool does this?" + edge list | A0–A2 as designed; A3 per feature | standard + A2 threat model mandatory + A4 chaos/rollback mandatory |
| Tests | smoke test for the happy path + the one scary edge; TDD optional | per-feature TDD + A3 hardening | standard + property tests mandatory + coverage gate in CI |
| CLV | one end-of-work checklist pass (do the done-whens hold? anything undocumented?) | incremental per wave + FULL at release | standard + **incremental** CLV at every phase exit (FULL still only on explicit request or at the release gate) |
| Wiki brain | single `WIKI.md` (MAP + a 3-line registry), no spokes | full brain + spokes | full brain + `wiki/architecture.md` kept current per wave |
| Agents | optional — inline role-work is PERMITTED at LIGHT (spawning five agents for a script is overhead, not rigor) | delegation mandatory per Principle 11 | delegation mandatory + adversary/validator ALWAYS fresh-context |
| Phases | collapsed: plan → build → check | 0–3 as designed | 0–3 + staged waves with wave gates |

## Invariants — never turned off at any tier

These are cheap and prevent the expensive mistakes, so they survive even LIGHT:
1. **Check-before-create / anti-clobber** (the hook stays on — it costs nothing).
2. **One-way doors get the doctrine.** Even in a script, an irreversible choice (data
   format you'll be stuck with, a paid service) gets three options.
3. **The done-when is written before building.** However small, the goal is stated first.
4. **Honest verification when delegating.** If an agent is used at any tier, its report
   passes the Verification Gate.
5. **STATE.md exists** (even 5 lines) — so "continue my project" always works.

## Escalation (a light project that grew) — and de-escalation

RIGOR is not a cage. Re-calibrate when an objective trigger fires:
- **Escalate** when any of: file count or REQ count crosses the tier's size signal;
  a second person starts using it; it becomes network-exposed; it starts touching
  credentials/PII/money; lifespan expectation changes.
  Escalation = a cascade event: the tier DEC re-opens, the missing mechanisms are
  *backfilled at the current boundary* (write the PRD for what exists now; run one catch-up
  pre-mortem; bring tests to the new tier's floor) — never retroactive ceremony for its own
  sake.
- **De-escalate** is allowed too (a FULL experiment that settled into a personal tool),
  but requires the user's explicit sign-off since it removes protections.
- **Per-feature override:** a *significant* feature inside a LIGHT/STANDARD project (one
  that trips any FULL signal by itself — e.g., "add payments") runs at the higher tier for
  that feature only: its own mini-calibration line in STATE (`RIGOR: light (REQ-012@full)`).

## How it reads in practice

- LIGHT session start: orient (small WIKI + STATE) → "done-whens: 2/5 remaining" → build →
  end-of-work checklist. No waves, no gates, no spawns unless useful.
- STANDARD: exactly the workflow as documented everywhere else.
- FULL: the workflow + the mandatory extras above — which are *named checks*, not
  micromanagement: nothing in FULL adds approval loops, only evidence.
