# Cross-Layer Validation (CLV) — incremental

CLV detects drift across `REQ ↔ DEC ↔ DES ↔ TEST ↔ COMMIT`. The checks and their
blocking power is unchanged from the original *at the effects a failure is assigned* (see
**Effect assignment** below — a failure still blocks, it just blocks what it actually
affects); what changes is *scope*: by default CLV
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
gate (Phase 3 exit). Even here you read indexes, not every full entry. The release gate
additionally requires the clean-worktree configure+build of HEAD (orchestration Scale
discipline) — no release on a HEAD that only builds against a dirty tree.

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
   latency). For a criterion phrased END-TO-END ("imports", "syncs", "the user sees"), the
   trace row must NAME the boundary the test crosses and the consumer it exercises — a
   criterion satisfied only at an inner seam is WARN at minimum: a feature can be inert in
   production behind a fully green suite.
5. **Cycle-finding closure** — read **`findings.md` only**, and read each open finding's
   **BLOCKS effects**: FAIL only when an effect **intersects the gate being evaluated**
   (checkpoint gate ↔ `CHECKPOINT:<this slice>`; release gate ↔ `RELEASE`; deploy gate ↔
   `DEPLOY`; task gate ↔ `TASK:<this task>`). Ignore advisory entries and `TASK:`/
   `CHECKPOINT:` effects whose named scope does not intersect the gate; do **not** ignore
   `RELEASE` at a release gate or `DEPLOY` at a deploy gate. Non-blocking aging > 2 weeks →
   WARN. Malformed or legacy-unclassified entries are reported as `NEEDS-CLASSIFICATION` for
   the orchestrator — never treated as global blockers. (This is the check that used to read
   all cycle history; now it reads the tiny hot table.)
6. **Ledger integrity** — every referenced DEC exists in the index; index ↔ trace digest
   agree; no zombie or missing rows. FAIL on inconsistency. (Mechanizable: grep every cited
   `DEC-\d{3}`/`DES-\d{3}` against its index table — cheap, and catches summary tables
   silently falling behind their own prose.)
7. **Cascade hygiene** — no `needs-review` (in `STATE.CASCADE` or the index) older than the
   current phase start. FAIL on a lingering flag.
8. **Contradiction scan** — no two `accepted` DECs conflict; no DEC contradicts a
   non-deferred REQ; no DES contradicts an accepted DEC. (Reads the Decision Index; drills
   only suspected pairs.) FAIL on contradiction.
9. **Drift detection** — for `CHANGESET` IDs, was cascade considered for dependents and are
   dependents current? WARN normally; FAIL if a one-way-door DEC changed without a cascade.

## Effect assignment (what a FAIL actually blocks)

**The reason CLV was invoked does not determine the effect of what it discovers.** Assign
BLOCKS effects (`references/orchestration.md`) from the failed property itself:

| Run context | Initial effects on FAIL | WARN |
|---|---|---|
| Feature / checkpoint gate | blocks that changeset's checkpoint — `{CHECKPOINT:<changeset>}` | informational |
| Release gate | blocks release — `{RELEASE}` (add `DEPLOY` for a deploy gate) | blocks release unless explicitly accepted-with-rationale |
| After cascade | `{TASK:<cascaded ids>}`; re-loop until clean | informational |
| Diagnostic / user-requested | **not advisory by default** — inspect impact and assign effects from the actual failed property | assign the same way |

Rules that override the table:
- Any CLV finding may **gain or lose effects** when its demonstrated scope changes.
- **Never downgrade a production or release defect merely because the run was requested
  manually.** A use-after-free found by a curiosity run is still a release defect.
- Scope is proven, not assumed: unproven scope means `TASK:<active-slice>`, never a global
  block, and a finding whose effects don't intersect the gate being evaluated does not fail
  that gate.

## Output (the only thing written hot)
```
LAST_CLV: VAL-013 PASS · 1 WARN(trace-sync REQ-009)
```
Full per-check detail goes to a cold `validations/` report (archived on phase close). On
**green**, the run bumps `VAL.next` and updates `latest:` in the WIKI REGISTRIES, so the
**validation history is visible at a glance from the brain** — this is what keeps many
validation runs from disorienting the agent.

**On FAIL, write only these four things:**
1. the current verdict;
2. the evidence pointer, with the command and the exit code;
3. the BLOCKS effects and their explicit scope;
4. the next repair.

The `VAL.next` bump, the `latest:` update, registry rewrites, counts, narratives, compaction,
archive moves and any lesson evaluation **wait until the repair passes**. (A security or
data-loss discovery may additionally record the minimum warning required to prevent unsafe
use, and must not trigger an automatic governance wave.) A remediation with genuine
alternatives goes through the Three Options Doctrine; a uniquely determined repair does not —
apply it under the existing task/finding and record it in the normal test/commit report.

**A FAIL is a lesson-capture candidate, evaluated against the lesson capture threshold after
the repair passes.** (`references/lessons-memory.md` — recurrence, cross-boundary reach, a
defective workflow rule, or a material security/data-loss/irreversible risk. A
first-occurrence local implementation or harness bug is a repair, not a lesson.)

## Why this stays cheap
The original re-ingested the whole corpus (incl. all cycle history) on every run, many
times per project → O(features²) lifetime. Incremental scope + Check 5 reading only
`findings.md` + archive-on-close bound each routine run to O(changeset) and bend the
lifetime curve to roughly linear, with identical blocking guarantees at each failure's
assigned effects.
