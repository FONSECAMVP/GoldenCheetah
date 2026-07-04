# The Three Options Doctrine

Never let the user make an uninformed decision. When a decision is needed, present three
scored alternatives, recommend, and explain cascade impact — *before* the user chooses.
Rigor is unchanged from the original; only storage is lean (proposal is ephemeral chat;
one cold entry + one index line persist).

## When it triggers
Any choice that shapes downstream work: runtime, persistence, API style, auth, async
model, validation library, even ID format or error-response shape. Watch for "let's use",
"I'll go with", "should I do X or Y", "what about Z", "we could just" — especially when
the user is leaning toward a choice without alternatives. "What do you think?" is
permission to *recommend after* presenting the trade space, not to skip it.

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
1. Append the **full entry** to `decisions.md` below the index (cold; Tier 2).
2. Add **one line** to the Decision Index (Tier 1).
3. Patch `STATE.md` **only if** the decision adds a dependent or is a one-way door
   (so cascades and risk stay visible at Tier 0).

## Anti-patterns
- **False trio** — three genuinely plausible options, no strawman padding. If you can
  only find two, say so and ask the user to help find a third.
- **Generic cascade impact** — always name the concrete downstream step.
- **Hidden recommendation** — one clear sentence, anchored in the user's constraints.
- **Skipping "obvious" choices** — present alternatives anyway; the user may know
  constraints you don't.
- **Treating cheap and one-way decisions the same** — reversibility sets how much debate
  is warranted; tag it.
- **Treating a lean toward X as decided** — still present three; let the user pick X with
  informed conviction.
