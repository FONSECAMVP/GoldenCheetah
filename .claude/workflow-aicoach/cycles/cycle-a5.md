# A5 — Post-Launch Retrospective

Date: 2026-05-11  
Commit: ede5df406

---

## What shipped

4-tool AI Coach action layer for GoldenCheetah:
- create_workout → .zwo file in athlete library
- schedule_workout → SeasonEvent on calendar (with conflict detection)
- create_season_event → races / key events with A/B/C priority
- create_training_plan → multi-week plan with preview card before apply

All three LLM providers supported (Anthropic, OpenAI, Gemini).
Inline confirmation for all writes. Atomic rollback on plan apply failure.

---

## What went well

- Native tool use API on all 3 providers required less abstraction than expected.
  LLMService virtual interface + per-client override was the right call (DEC-002).
- ZWO format was a good choice (DEC-003) — maps directly to 6 interval types,
  parser already existed, format is widely supported.
- Inline confirm/plan cards (DEC-004, DEC-005) kept UX in chat context with no
  modal disruption. Pattern works well.
- Adaptive system prompt (buildToolUseSection) improved LLM tool invocation quality
  significantly vs. relying solely on tool schema descriptions.
- Atomic rollback (A2 mitigation, F2 fix) was correctly identified as critical early.

---

## What was harder than expected

- MOC and static methods: Qt moc parses access sections strictly — placing static
  methods inside `signals:` block causes moc to generate signal activation code for
  them. Lesson: always place statics in a separate `public:` block before `signals:`.
- Gemini tool use required more conversation-state management than Anthropic/OpenAI:
  no per-call IDs means function name is used as correlation key, which breaks if the
  same tool is called twice in one response.
- GeminiClient had a pre-existing message-duplication bug (content added to both
  conversation_ and passed as extra arg to buildRequest). Worked around in tool-use
  path without fixing root cause to minimize scope.

---

## Known risks / next iteration

| Item | Priority | Status | Notes |
|------|----------|--------|-------|
| Seasons::writeSeasons() silent failure (A4-001) | MEDIUM | **RESOLVED** | DEC-012: writeSeasons() returns bool; seasonsChanged() only on success; executors check return (2026-05-17) |
| Integration test coverage (REQ-003, REQ-010, REQ-015) | HIGH | **RESOLVED** | TEST-005..010: 16 tests added covering confirmation gate, plan gate, null-ctx, payload guard (2026-05-17) |
| Workout undo (delete AI-created events from calendar) | MEDIUM | **RESOLVED** | DEC-013: remove_calendar_event tool added; TEST-011..015 (2026-05-17) |
| Gemini same-tool-twice edge case | LOW | **RESOLVED** | `PendingToolCall.callId` = `name_N` using per-session counter; unique across same-tool repetitions (2026-05-17) |
| GeminiClient buildRequest duplication bug | LOW | **RESOLVED** | Removed trailing content append from `buildRequest`; message already in `conversation_` before call (2026-05-17) |
| Push workouts to Strava/Garmin/TrainerDay | LOW | Non-goal | Explicit non-goal in v1 PRD |

---

| Integration test coverage (REQ-006, REQ-014, REQ-016, REQ-017) | MEDIUM | **RESOLVED** | TEST-016..020 added (2026-05-17) |
| REQ-018 `buildToolUseSection` content | NICE | **Verified by inspection** | PromptBuilder.cpp:1031 static string; gating at CoachChatWidget.cpp:415 |

---

## Metrics target (from PRD REQ-017)

Round-trip time (LLM → GC executes → confirm dialog): < 5s excluding LLM latency.
TEST-020 proves GC-side synchronous work completes in < 50ms (well under budget).
