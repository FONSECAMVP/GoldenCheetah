# The Three Options Doctrine

Never let the user make an uninformed decision. When a decision is needed, present up to
three genuinely viable scored alternatives, recommend, and explain cascade impact — *before* the user chooses.
Rigor is unchanged from the original; only storage is lean (proposal is ephemeral chat;
one cold entry + one index line persist).

## When it triggers
The doctrine applies to **architectural choices, irreversible actions, public API changes,
and materially different trade-offs** — choices that shape downstream work: runtime,
persistence, API style, auth, async model, validation library, even ID format or
error-response shape. Watch for "let's use", "I'll go with", "should I do X or Y", "what
about Z", "we could just" — especially when the user is leaning toward a choice without
alternatives. "What do you think?" is permission to *recommend after* presenting the trade
space, not to skip it.

## When it does NOT apply
A **uniquely determined local bug repair** — one defensible fix, no materially different
alternatives — plus mechanical conventions and tool invocations inside an already-accepted
DEC. For those:

> Apply the uniquely determined repair under the existing task/finding and record it in the
> normal test/commit report. Do not allocate a DEC, option matrix, lesson, or new index entry
> unless the repair changes architecture, public API, an irreversible behavior, or a
> previously accepted decision.

The test is whether three genuinely viable options exist, not how much the fix cost. If you
cannot state three without inventing a strawman, the doctrine does not apply.

## Proposal template (shown in chat; not stored verbatim beyond the cold entry)
```
DECISION REQUIRED: DEC-NNN — [one-line question]

Context: [why now; which REQ/DES it serves; constraints from prior DECs (from the index)]

  Option A — [name]
    Reliability:     [1–5] — [reason]
    Scalability:     [1–5] — [reason]
    Maintainability: [1–5] — [reason]
    Best Practices:  [1–5] — [reason]
    Cascade impact:  [concrete: which Phase 2 tests / Phase 3 gates this forces; what
                      becomes hard to reverse]
  Option B — [name]   [same structure]
  Option C — [name]   [same structure]

Recommendation: [A/B/C] because [reasoning anchored in the user's stated constraints].
Reversibility: cheap | expensive | one-way door
Your call?
```

## Four-Pillars rubric (1–5, relative to the other two options)
- **Reliability** — 5 mature, failure modes understood, recovery trivial → 1 unknown
  failure modes / ad-hoc recovery.
- **Scalability** — 5 horizontal to ~100× no change → 1 re-architecture at 2–3×.
- **Maintainability** — 5 solo dev fixes/extends in ~1h, no context-loading → 1 high
  cognitive load, high regression risk.
- **Best Practices** — 5 matches consensus *and* constraints → 1 violates a stated
  constraint or consensus. When constraint and consensus conflict, the **constraint wins**
  (it's the user's project) — flag the tension in the reason cell.

## Cascade impact — the highest-value field
Must be concrete. **Bad:** "affects testing." **Good:** "a relational store forces schema
migration tests per model change, adds ~30s DB spin-up to CI, and makes rollbacks
backup-dependent — which pushes DEC-011 toward blue-green." Name the specific later step
and how it changes.

## On choice — what persists (lean)
1. Append the **full entry** to `decisions.md` below the index (cold; Tier 2), within the
   DEC shape of the ledger writing contract (`state-and-tiers.md`) — the chat proposal is
   not copied in.
2. Add **one line** to the Decision Index (Tier 1).
3. Patch `STATE.md` **only if** the decision adds a dependent or is a one-way door
   (so cascades and risk stay visible at Tier 0).

**A revisited decision gets one dated line** (what changed + pointer); a changed choice is
a new DEC that supersedes it. A full entry is written once — one that keeps growing across
sessions is carrying investigation that belongs in tests and commit messages.

## Who owns the decision — and batching

Not every decision needs the user. **User-owned** (always presented, never assumed):
one-way doors; accepting deliberate residual risk or shipping a known defect; scope or
acceptance-criterion changes; anything spending money or affecting other people's data.
**Orchestrator-owned** (decide and move on): mechanical calls with one defensible answer —
the obviously correct safety fix, a naming/location choice fully determined by conventions,
tool invocations within an already-accepted DEC. These get one index line **only when they
are genuinely decisions**; a uniquely determined repair gets no index entry at all (above). When several
user-owned decisions are coupled, **batch them into one presentation** with shared context
rather than interrupting N times. This removes interruptions without removing rigor — the
test is reversibility and residual risk, not effort.

## Anti-patterns
- **False trio** — no strawman padding. On a real decision, if only two options are
  viable, present two and say so. On a
  *uniquely determined repair* there is no trio to find: the doctrine doesn't apply — apply
  the repair and report it (see **When it does NOT apply**).
- **Generic cascade impact** — always name the concrete downstream step.
- **Hidden recommendation** — one clear sentence, anchored in the user's constraints.
- **Treating cheap and one-way decisions the same** — reversibility sets how much debate
  is warranted; tag it.
- **Treating a lean toward X as decided** — still present the viable alternatives; let the
  user pick X with informed conviction.
- **Prose as machine input** — an option whose correctness requires a machine to
  unambiguously interpret human prose (Markdown, comments) is not viable at any score — the
  viable options change where the fact lives, not how the prose is parsed.
