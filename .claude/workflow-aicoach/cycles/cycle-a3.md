# A3 — Test Hardening Log

Running: 2026-05-11 (Phase 2, ongoing — one entry per feature complete)

---

## Feature: GCToolExecutor static methods (TEST-001..004)

### Mutation analysis

| Method | Critical mutations | Covered? |
|--------|-------------------|---------|
| validateDate | flip `<` to `<=` on past-date check | YES — TEST-001 uses yesterday as boundary |
| validateDate | remove invalid-format check | YES — TEST-001 explicit |
| sanitizeFilename | miss one char in invalid set | PARTIAL — tests cover `/`, `:`, `*`, `?`, `<`, `>` |
| buildZwoXml | wrong power conversion (missing /100.0) | YES — TEST-003 checks exact Power="0.750" |
| buildZwoXml | wrong element name (e.g. "SteadyState" → "Steadystate") | YES — tests check exact tag strings |
| v1Tools | missing tool | YES — TEST-004 checks count == 4 |
| v1Tools | wrong required[] | YES — TEST-004 checks each required array |

### Boundary sweep

| Boundary | Test |
|----------|------|
| date = today | TEST-001 validateDate_today_accepted |
| date = yesterday | TEST-001 validateDate_pastDate_rejected |
| empty filename | TEST-002 sanitizeFilename_emptyAfterTrim_returnsDefault |
| all-spaces filename | TEST-002 sanitizeFilename_emptyAfterTrim_returnsDefault |
| weeks minItems=1 | TEST-004 v1Tools_trainingPlan_weeksConstraints |
| weeks maxItems=16 | TEST-004 v1Tools_trainingPlan_weeksConstraints |

### Negative paths

| Scenario | Covered |
|----------|---------|
| Invalid date format (slashes) | YES |
| Empty date string | YES |
| Invalid date format (text) | YES |
| Future date | YES |

### Deferred hardening (requires Context* / integration)

- REQ-002: power > 300% rejected → executeCreateWorkout path (integration)
- REQ-003: confirm gate actually blocks write → CoachChatWidget integration
- REQ-009: plan rollback on file write failure → requires mock filesystem

### A3 status: PARTIAL PASS
Static-method coverage complete. Integration paths deferred per traceability.md gap list.
All deferred items must be verified before Phase 3 exit.
