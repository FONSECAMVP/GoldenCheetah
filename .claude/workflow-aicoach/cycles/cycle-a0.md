# Adversarial Cycle A0 — Should We Build This?

## Challenge Questions

**Q1: Does a simpler alternative exist?**
GC already integrates TrainerDay (TrainerDayAPIDialog, TrainerDayDownloadDialog). User could: ask coach → manually search TrainerDay → import. This avoids building tool use entirely.
**Finding:** Viable workaround exists BUT requires leaving the app and manual translation. The value gap is real. PASS — build is justified.

**Q2: Is the blast radius acceptable?**
Tool use adds LLM-controlled write paths to: workout files on disk, Seasons XML. If the LLM hallucinates bad values (e.g., 2000W FTP, season start 1970), corrupt data could result.
**Finding:** BLOCKING RISK — requires confirmation gate before any write. Mitigated by: (1) propose-then-confirm UI, (2) value validation before write. Must be in requirements. PASS with constraint.

**Q3: Is this reversible?**
Created workouts: deletable files. Season events: deletable via SeasonDialogs. Season itself: deletable.
**Finding:** All writes reversible. PASS.

**Q4: Technical feasibility — can LLMService be extended for tool use without breaking existing chat?**
LLMService is abstract. Adding tool registration + dispatch can be additive (new virtual methods with default no-op). Existing chat flow unchanged.
**Finding:** PASS. Additive, backward-compatible.

**Q5: Scope risk — "all the tools" could be unbounded.**
If scope is not fixed, this becomes an ever-expanding agent framework.
**Finding:** WARN — must fix scope in requirements to a concrete v1 tool set. Deferred tools go in backlog.

## Disposition

| Finding | Status |
|---------|--------|
| Simpler alternative exists but inferior | accept-with-rationale |
| Confirmation gate required before any write | fix-in-requirements (REQ must include it) |
| All writes reversible | pass |
| Additive LLMService extension feasible | pass |
| Scope must be bounded to v1 tool set | fix-in-requirements |

## Verdict: PROCEED — with confirmation gate and bounded scope as hard requirements.
