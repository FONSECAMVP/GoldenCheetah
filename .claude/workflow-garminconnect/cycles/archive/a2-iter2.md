# Adversarial Cycle A2 — Design Pre-Mortem (Iteration 2 — re-confirmation)

**Subject:** `design.md` after A2 iter-1 fix-now remediations applied.
**Run:** 2026-05-17.
**Iteration:** 2.
**Phase:** 1.

This iteration re-walks the five A2 prompts only against the areas modified by iter-1 remediations. The exhaustive narratives in iter-1 stand for the unaffected surface.

---

## Remediations applied (recap)

| Finding | Disposition | Where it landed |
|---------|-------------|-----------------|
| A2-001 sub-interp wedge | defer-with-ticket → Phase 1.5 | Documented in DES-001 Failure modes + Phase 1.5 backlog. |
| A2-002 library response time | accept-with-rationale | Added to REQ-NF-Compat-001 (point c). |
| A2-003 sidecar growth | fix-now (partial) + defer (sqlite) | DES-008 observability gains 2 signals (sidecar-read-time + sidecar-size); sqlite migration in Phase 1.5 backlog with named threshold. |
| A2-004 migration trap | fix-now | **New DES-012** (`garmin_client.py` adapter); DES-001/005/008 re-routed through it; DEC re-open not required. |
| A2-005 token-permission UX | fix-now | New `TokenPermissionsRejected` + `SidecarPermissionsRejected` error kinds in DES-008 switch table with file-path-aware messages. |
| A2-006 cross-account collision | fix-now (Option C, user-chosen) | Per-account sidecar files `imported-<uid>.json` and `backfill-state-<uid>.json`; DES-002/009/010 updated; REQ-008/012 clarified; REQ-NF-Compat-001 (point a) clarified. |
| A2-007 parse cancellation latency | accept-with-rationale | Documented in DES-009. |
| A2-008 wizard phishing | accept-with-rationale | No-op; same posture as other CloudServices. |

---

## A2.1 — Re-walk failure narratives against remediated design

### Narrative 1 (silent sidecar) — re-check
Sidecar still JSON in Phase 1, but the *trigger* for swapping to sqlite is now a beta-observable signal (DES-008 emits read-time + size per sync). Beta will catch the cliff before users get hurt. **Risk acceptably reduced.**

### Narrative 2 (platform/curl_cffi) — re-check
Unchanged in design. DES-012 doesn't help here (the issue is at the HTTPS layer below the library). REQ-NF-Compat-001 now explicitly warns users about library-tracked-SSO-style outages, so the failure narrative shifts from "where did this come from?" to "this was a known risk we communicated." **No further design fix in Phase 1.**

### Narrative 3 (unmaintained dep) — re-check
**Materially mitigated.** Swap cost dropped from multi-file refactor to single-file (DES-012). The library could be replaced with `garth` or a fork in roughly a day of work plus tests. The Narrative-3 trajectory ("we delay, users get frustrated") is now budget-bound by adapter-swap time, not by codebase coupling.

### Narrative 4 (silent permission widening) — re-check
**Resolved.** `TokenPermissionsRejected` now produces a user-facing message that names the offending file path and expected mode; the user can identify the backup tool causing the issue. The infinite-relogin loop is broken because the user can now act on the cause.

### Narrative 5 (orphaned reconnect) — re-check
**Resolved.** Per-account sidecar files (`imported-<uid>.json`) make cross-account `garmin_activity_id` collisions structurally impossible. The active session reads only the active account's sidecar; prior-account sidecars are inert. New activities cannot be silently skipped.

---

## A2.2 — Chaos questions: re-check on changed surface

- **Sub-interp wedge:** still no auto-recovery in Phase 1; documented as a known limitation; Phase 1.5 has named follow-up. Accepted.
- **Account-different reconnect:** previously a silent data-loss path; now a clean separation with per-account files.
- **Backup tool widens token-file permissions:** previously a silent re-login loop; now a diagnosable error with actionable message.

All other chaos answers from iter-1 stand unchanged.

---

## A2.3 — Cost worst-case: re-check
The sidecar cliff (~10–50 MB) is unchanged, but **detectable** in Phase 1 via the new observability signals; sqlite migration is a Phase 1.5 follow-up with a measured trigger threshold rather than a guessed one. No new monetary cost; resource costs unchanged.

---

## A2.4 — Migration trap: re-check
Per A2.4 (iter-1), `python-garminconnect` was the most-likely-to-need-swapping component. After DES-012, the swap cost is structurally bounded: rewriting `_EXCEPTION_MAP` + the adapter method bodies in `garmin_client.py`. No C++ changes, no test-assertion churn against library-specific class names. **The migration trap is closed for Phase 1.**

---

## New findings introduced by iter-1 remediations

None. The remediations are additive (new DES-012, new observability fields, new error codes, refined paths). No new contradictions, no new coverage gaps, no new threat vectors.

One thing worth noting (not a finding): DES-012's `_EXCEPTION_MAP` will need to be kept in step with `python-garminconnect` upstream changes. Mitigated by the introspection test described in DES-012's "Tests (anticipated)" — that test will fail loudly if the library adds an exception subclass we don't translate. Good signal.

---

## Loop status

- [x] **Clean pass — Phase 1.5 may exit; ready for Phase 1 exit CLV.**
- [ ] Findings remain

All iter-1 findings have explicit dispositions. Iter-2 surfaced no new findings. Proceed to validation-002 (Phase 1 exit CLV).
