# A2 — Design Pre-Mortem

## Failure Narrative 1: "The LLM Hallucinated My FTP Away"
Six months post-launch, an athlete reports their season was wiped. Investigation: LLM called `create_season_event` with date "2020-01-01" (past date), which passed validation because the date check had a timezone bug (QDate::currentDate() vs QDateTime). Then called `create_training_plan` with 52 weeks, applying 364 workouts.

**Mitigations:**
- Date validation: use `QDate::currentDate()` consistently, not `QDateTime::currentDateTimeUtc().date()`.
- `create_training_plan` limited to 16 weeks max in tool schema.
- Apply Plan atomicity: if any single workout write fails mid-plan, roll back by deleting already-written files.

## Failure Narrative 2: "Three Providers, Three Bugs"
Anthropic tool use works. OpenAI returns `tool_calls` array inside `message` — but our parser looks in the wrong place (legacy `function_call` format vs new `tool_calls` format). Gemini uses `functionCall` part but our code checks for `function_call`. Both OpenAI and Gemini subtly broken at launch.

**Mitigations:**
- Unit test for each client's tool-use response parsing with fixture JSON (no real API call).
- Each client: parse both legacy and new formats for resilience.

## Failure Narrative 3: "The Plan Card Crashes on Empty Week"
LLM generates a plan with `"weeks": []` (zero weeks — valid per schema, minimum not enforced). PlanPreviewCard tries to render zero rows → QTableWidget crashes with empty model.

**Mitigations:**
- Add `"minItems": 1` to weeks array in tool schema.
- PlanPreviewCard defensive check: if weeks.empty() → show error message, don't render table.

## Chaos Questions

| Question | Answer |
|----------|--------|
| What if LLM sends tool call with 500KB args? | QJsonDocument parsing caps at Qt's limit. Add explicit size check: reject tool args > 64KB. |
| What if user dismisses confirm card and then sends another message before LLM processes cancel? | GCToolExecutor queues tool calls. One active confirmation at a time. Input disabled while card shown. |
| What if workout save path has non-ASCII chars (athlete name in Chinese/Arabic)? | Use QFile with UTF-8 QString — Qt handles this. Filename sanitization via QRegularExpression strips dangerous chars. |
| What if two CoachChatWidgets open simultaneously (two athlete tabs)? | Each CoachChatWidget has its own GCToolExecutor with its own Context*. No shared state. |

## Migration Trap

Tool use requires `"anthropic-version": "2023-06-01"` header (already set). If Anthropic deprecates this version, tool use breaks silently. Mitigation: log version in use, surface deprecation warnings from API response headers.

## A2 Verdict: PASS — narrative findings → mitigations added to design. No DEC re-open triggered.

**Design updates from A2:**
- Weeks array minItems: 1
- Plan apply rollback on partial failure
- Input disabled while confirm card active
- Tool args size check (64KB max)
- Date validation uses QDate::currentDate() exclusively
