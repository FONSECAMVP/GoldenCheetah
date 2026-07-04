# Decision Ledger

## DEC-001 — Solution shape: AI Coach action execution model
**Question:** How should the AI Coach execute write actions?
| Criterion | A: Tool use (native API) | B: Structured output + parser | C: Hybrid (tool use + plan export) |
|-----------|--------------------------|-------------------------------|-------------------------------------|
| Reliability | 4 | 2 | 4 |
| Scalability | 5 | 3 | 4 |
| Maintainability | 3 | 2 | 4 |
| Best Practices | 5 | 2 | 5 |

**Choice:** C — Hybrid  
**Rationale:** Atomic actions via native tool use; complex multi-week plans generate an in-app preview card user must approve before GC applies.  
**Reversibility:** Medium  
**Status:** accepted  
**Dependents:** DEC-002, DEC-003, DEC-004, DEC-005

---

## DEC-002 — LLMService tool interface design
**Question:** How to expose tool use to all three LLM clients without breaking existing chat?

| Criterion | A: New virtual methods | B: Separate ToolAgent | C: CoachChatWidget handles directly |
|-----------|------------------------|----------------------|--------------------------------------|
| Reliability | 5 | 4 | 2 |
| Scalability | 5 | 4 | 1 |
| Maintainability | 4 | 3 | 1 |
| Best Practices | 5 | 4 | 1 |

**Choice:** A — New virtual methods on LLMService  
**Rationale:** Additive to existing interface. Each client marshals ToolDef to its own JSON schema. Backward-compatible default no-op for providers that don't support tool use.  
**Reversibility:** High  
**Status:** accepted  
**Dependents:** DEC-003, DEC-004, DEC-005, GCToolExecutor implementation

---

## DEC-003 — Workout file format for AI-generated workouts
**Question:** Which format for AI-generated workout files?

| Criterion | A: .zwo (ZWO XML) | B: .erg | C: Both |
|-----------|-------------------|---------|---------|
| Reliability | 4 | 3 | 3 |
| Scalability | 5 | 3 | 3 |
| Maintainability | 4 | 4 | 2 |
| Best Practices | 5 | 3 | 3 |

**Choice:** A — .zwo (ZWO XML)  
**Rationale:** GC already has ZwoParser. ZWO has native warmup/interval/cooldown/ramp semantics. Coach tool schema maps directly to ZWO elements. Most popular structured workout format.  
**Reversibility:** High — file format choice doesn't affect tool API  
**Status:** accepted  
**Dependents:** GCWorkoutTool implementation, workout save path

---

## DEC-004 — Training plan review UI
**Question:** How to show multi-week plan for user review before applying?

| Criterion | A: Inline plan card | B: QDialog | C: HTML file |
|-----------|---------------------|------------|--------------|
| Reliability | 4 | 5 | 2 |
| Scalability | 4 | 4 | 1 |
| Maintainability | 4 | 3 | 2 |
| Best Practices | 5 | 4 | 1 |

**Choice:** A — Inline plan preview card in CoachChatWidget  
**Rationale:** Keeps UX in chat context. Custom QWidget with week table + "Apply Plan" button. No modal disruption. Consistent with chat-first design.  
**Reversibility:** High  
**Status:** accepted  
**Dependents:** CoachChatWidget UI, GCToolExecutor::executePlan()

---

## DEC-006 — Workout conflict handling (REQ-006)
**Question:** How to handle date conflicts in schedule_workout?

**Choice:** Return conflict result to LLM; let LLM inform athlete; re-call with force:true if athlete confirms.  
**Rationale:** Keeps tool API self-contained. LLM handles the user conversation naturally.  
**Reversibility:** High  
**Status:** accepted  
**Dependents:** executeScheduleWorkout implementation

---

## DEC-011 — Rollout strategy
**Question:** How to ship AI Coach tool use to users?

| Criterion | A: Direct merge | B: Feature flag | C: Beta branch |
|-----------|----------------|-----------------|----------------|
| Reliability | 3 | 5 | 4 |
| Scalability | 5 | 5 | 3 |
| Maintainability | 4 | 5 | 2 |
| Best Practices | 3 | 5 | 4 |

**Choice:** B — Feature flag via existing `GC_WANT_COACH` CMake option  
**Rationale:** Flag already exists, gates entire Coach module, GC convention, instant kill-switch, zero new infra.  
**Reversibility:** High — `cmake -DGC_WANT_COACH=OFF` removes feature from binary  
**Status:** accepted  
**Dependents:** none

---

## DEC-005 — Confirmation gate for single tool calls
**Question:** How to confirm before single tool writes?

| Criterion | A: Inline chat card | B: QMessageBox | C: Always-allow + undo |
|-----------|---------------------|----------------|------------------------|
| Reliability | 5 | 4 | 1 — violates REQ-015 |
| Scalability | 5 | 3 | 1 |
| Maintainability | 4 | 4 | 2 |
| Best Practices | 5 | 4 | 1 |

**Choice:** A — Inline chat confirmation card  
**Rationale:** Inline card in chat shows tool name, key params, confirm/cancel. Consistent with plan preview style. No modals.  
**Reversibility:** High  
**Status:** accepted  
**Dependents:** CoachChatWidget UI, ToolConfirmCard widget

---

## DEC-012 — Seasons::writeSeasons() failure propagation (A4-001/A4-002 fix)
**Question:** How to surface disk-write failure from Seasons::writeSeasons() to GCToolExecutor?

| Criterion | A: Return bool from writeSeasons() | B: Post-write file check | C: Emit writeFailed signal |
|-----------|-------------------------------------|--------------------------|---------------------------|
| Reliability | 5 | 2 | 4 |
| Scalability | 5 | 3 | 3 |
| Maintainability | 5 | 2 | 3 |
| Best Practices | 5 | 1 | 3 |

**Choice:** A — Change `Seasons::writeSeasons()` to return `bool`  
**Rationale:** `SeasonParser::serialize()` already returns `bool`; `writeSeasons()` was discarding it. Surface the existing result. Only call `seasonsChanged()` on success. GCToolExecutor checks return value and emits error `actionMessage` instead of success on failure.  
**Reversibility:** Medium — API change in GC core; all 3 internal callers in Seasons.cpp silently ignore return (acceptable; they already show QMessageBox via serialize).  
**Status:** accepted  
**Dependents:** executeScheduleWorkout, executeCreateSeasonEvent, executeRemoveCalendarEvent

---

## DEC-013 — Workout undo: remove AI-created calendar events
**Question:** How should the AI Coach remove events from the athlete's calendar?

| Criterion | A: remove_calendar_event tool (name+date) | B: undo_last_write session stack | C: Two tools (remove file + remove event) |
|-----------|--------------------------------------------|----------------------------------|-------------------------------------------|
| Reliability | 4 | 3 | 4 |
| Scalability | 5 | 2 | 4 |
| Maintainability | 5 | 2 | 3 |
| Best Practices | 5 | 2 | 4 |

**Choice:** A — `remove_calendar_event` tool with `{name, date}` required fields  
**Rationale:** LLM knows name+date from prior tool results. Single tool, self-contained logic. Consistent with create tools. Scope: calendar events only (no .zwo file deletion — v3 scope).  
**Reversibility:** High — additive tool  
**Status:** accepted  
**Dependents:** v1Tools(), GCToolExecutor::executeRemoveCalendarEvent, ToolConfirmCard::buildSummary
