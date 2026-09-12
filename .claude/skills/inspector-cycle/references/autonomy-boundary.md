# Autonomy boundary — what stops the loop and what doesn't

## The hierarchy (restated from SKILL.md)

Goal (complete feature) > Stage (a checkpoint) > Atomic decision (one REQ/DEC/fix). Only
the atomic decision is dispatched per turn; stage and goal completion are detected, never
manually announced by the user.

## GO — resolve these yourself, never stop the loop

- **Any technical/architectural/tooling decision that stays within the current REQ's
  accepted scope.** Score it against QGDW's own Three-Options axes — reliability,
  scalability, maintainability, best practices (`.claude/skills/quality-gated-dev-workflow/
  references/three-options-doctrine.md`) — and take the top-scored option. Record it as a
  normal DEC entry per QGDW convention (proposal + cascade impact), but do not block on a
  live user reply to make the choice. This does NOT cover widening acceptance criteria,
  accepting a known defect, or changing scope itself — those fall under STOP or the named
  exception below even when the chosen fix is technically simple.
- **Stage completion.** Identify and start the next stage in the same cycle.
- **A blocker resolved via a technical decision** (e.g. REQ-015's CAPTCHA dead end, DEC-046).
  Resume immediately, don't pause to announce it.
- **A "uniquely determined" fix** — one defensible repair, no materially different
  alternatives (QGDW's own carve-out for when the Three-Options Doctrine does not apply).
  Apply it directly, no DEC needed at all.

This is a deliberate, scoped loosening from earlier project practice recorded in memory
(e.g. DEC-046 and DEC-048 both paused for a live second opinion or user scope call) — it
applies specifically to this skill's autonomous-loop context, where continuity across
whole stages matters more than a live checkpoint on every architecture call. It does not
change how `quality-gated-dev-workflow` itself is documented or used elsewhere.

## STOP — the only real human-in-the-loop gate

Pause and surface to the user, plainly, when the next action requires:

- Real user credentials, secrets, or a live external account (e.g. Stage 9's "needs a real
  Garmin account + real installers" — flagged explicitly, not assumed available).
- Anything already covered by the harness's own risky-action list: force-push, `git reset
  --hard`/destructive git ops, pushing/publishing to a shared remote, spending real external
  cost, modifying CI/CD, sharing content to a third-party service.
- Anything irreversible outside the codebase itself (the "hard to reverse" / "affects shared
  systems" class — not an in-repo architecture choice).

## One named exception — still ask, but it's not a "stop"

A finding that reaches into pre-existing, foundational, shared code **outside** the current
REQ's own new files (e.g. `GarminConnect`/`CloudService`'s own raw-context handling, relied
on by several already-shipped REQs) is a genuine scope-boundary question, not a pillar-
scored technical one — whether to fix now or defer touches cost/schedule, which the three
axes don't capture. Surface it via `AskUserQuestion` (fix-now vs. defer), matching the
DEC-048 precedent, and hold only that specific fix/closure until answered — other,
independent atomic units may continue in the meantime. This is a scoped pause on one item,
not a full stop of the cycle.

## What this means in practice

Don't draft a Three-Options proposal and then wait in silence for the user to pick one.
Draft it, score it, pick the top option, log the DEC, move on — report what you decided and
why in your next status update, don't gate progress on it being read first.
