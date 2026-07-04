# Cross-Layer Validation (CLV) — incremental

CLV detects drift across `REQ ↔ DEC ↔ DES ↔ TEST ↔ COMMIT`. The checks and their
blocking power are unchanged from the original; what changes is *scope*: by default CLV
validates only what changed, against the Tier-1 indexes plus targeted Tier-2 drills —
never the whole corpus.

## Two modes

**Incremental (default).** Inputs: `STATE.CHANGESET` + the three indexes (Tier 1). For
each changed ID, run the relevant checks; drill (Tier 2) **only** IDs that look
mismatched. Output: one verdict line into `STATE.LAST_CLV`; append any new findings to
`findings.md`; on green, **clear `CHANGESET`**. Cost ≈ O(changeset), flat as the project
grows. Run at: every phase/feature gate, after a cascade (over the touched IDs), on
suspicion of drift.

**Full walk.** Inputs: indexes first, then Tier-2 drills of mismatches only. All 9 checks
across all current (non-archived) IDs. Run **only** on explicit request or the release
gate (Phase 3 exit). Even here you read indexes, not every full entry.

## The nine checks
1. **Coverage (top-down)** — every must/should `REQ` has ≥1 DEC, ≥1 DES, ≥1 TEST. FAIL on
   gaps for must/should; WARN for nice-to-have. (Reads Trace Digest + Decision Index.)
2. **Provenance (bottom-up)** — every TEST→REQ, every COMMIT cites REQ/TEST/DEC, every DEC
   cites a parent, every DES cites a DEC. FAIL on orphans.
3. **Decision-implementation alignment** — each `accepted` DEC's choice is reflected in
   design/code (drill the DEC + its DES on mismatch). FAIL on divergence; WARN if only
   inferred.
4. **Acceptance-test alignment** — each REQ's acceptance criterion is objectively encoded
   in a TEST. FAIL if absent; WARN if partial (e.g. correctness tested but not the stated
   latency).
5. **Cycle-finding closure** — read **`findings.md` only**: any open blocking finding from
   a closed phase → FAIL; non-blocking aging > 2 weeks → WARN. (This is the check that
   used to read all cycle history; now it reads the tiny hot table.)
6. **Ledger integrity** — every referenced DEC exists in the index; index ↔ trace digest
   agree; no zombie or missing rows. FAIL on inconsistency.
7. **Cascade hygiene** — no `needs-review` (in `STATE.CASCADE` or the index) older than the
   current phase start. FAIL on a lingering flag.
8. **Contradiction scan** — no two `accepted` DECs conflict; no DEC contradicts a
   non-deferred REQ; no DES contradicts an accepted DEC. (Reads the Decision Index; drills
   only suspected pairs.) FAIL on contradiction.
9. **Drift detection** — for `CHANGESET` IDs, was cascade considered for dependents and are
   dependents current? WARN normally; FAIL if a one-way-door DEC changed without a cascade.

## Block vs. warn
| Context | FAIL | WARN |
|---|---|---|
| Phase/feature gate | blocks exit; remediate & re-run | informational |
| Pre-deploy (release) | blocks deploy | blocks deploy unless explicitly accepted-with-rationale |
| After cascade | re-loop until clean | informational |
| User request / suspicion | informational | informational |

## Output (the only thing written hot)
```
LAST_CLV: VAL-013 PASS · 1 WARN(trace-sync REQ-009)
```
Full per-check detail goes to a cold `validations/` report (archived on phase close). Each
run also bumps `VAL.next` and updates `latest:` in the WIKI REGISTRIES, so the **validation
history is visible at a glance from the brain** — this is what keeps many validation runs
from disorienting the agent. A FAIL lists the mismatched IDs and a proposed remediation —
usually a Three Options Doctrine when the fix has alternatives. **A FAIL is also a
lesson-capture trigger:** record (or increment) a checkable rule in `lessons.md` keyed to
the failure class, so the same drift is guarded against next time
(`references/lessons-memory.md`).

## Why this stays cheap
The original re-ingested the whole corpus (incl. all cycle history) on every run, many
times per project → O(features²) lifetime. Incremental scope + Check 5 reading only
`findings.md` + archive-on-close bound each routine run to O(changeset) and bend the
lifetime curve to roughly linear, with identical blocking guarantees.
