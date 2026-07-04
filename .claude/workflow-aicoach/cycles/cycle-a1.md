# A1 — Requirements Red Team

## Lawyer Hat

**Finding L1:** REQ-001 says "saves result as .zwo to athlete workout library" but doesn't define the library path. The path `context->athlete->home->workouts().path()` may not exist or may not be writable.
→ Action: GCToolExecutor must verify path exists before write; create dir if absent. Add to DES-003.

**Finding L2:** REQ-004 uses SeasonEvent for scheduling workouts. SeasonEvent has a `name` and `date` but no `workout_file_ref`. Scheduled workout won't know which .zwo to load.
→ Action: Store workout filename in SeasonEvent `description` field. Coach must have previously created (or named) the workout before scheduling. Update DES-003.

**Finding L3:** REQ-013 says "falls back to structured text" if provider doesn't support tools. What is "structured text"? Under-specified.
→ Action: Fall back = coach states its recommendation in plain text with no action buttons. Add WARN in chat: "Tool use not supported for this model. Recommendations are text-only."

## Persona Hat (Power User / Adversarial)

**Finding P1:** Power user asks "Create a 12-week plan" → LLM may generate 84 workouts. Each workout goes through a confirmation dialog. User fatigue is extreme.
→ Action: For `create_training_plan`, the plan preview card must show ALL workouts before any confirmation. Single "Apply Plan" button executes all atomically. Already in DEC-004 / REQ-010. Confirmed adequate.

**Finding P2 (malicious):** Could a prompt injection in athlete data (e.g., a ride named `create_workout({...malicious...})`) trigger tool calls?
→ Action: Tools are only triggered by LLM-generated `tool_use` content blocks (not from text content). Anthropic/OpenAI/Gemini APIs separate tool call blocks from text. Prompt injection in athlete data would land in text, not a tool_use block. Low risk — but GCToolExecutor should only respond to tool calls from `toolCallRequested` signal (typed, not parsed from text).

## Edge Case Hat

**Finding E1:** Athlete has no FTP set (new user). Power percentages in workout are undefined.
→ Action: GCToolExecutor checks FTP before executing create_workout. If FTP=0 or unset: send error result to LLM, LLM tells user to set FTP first.

**Finding E2:** `schedule_workout` called with workout_name that doesn't exist in library (LLM hallucinated name).
→ Action: GCToolExecutor scans workout library for file. If not found: send error result, LLM acknowledges and offers to create it first.

**Finding E3:** Two tool calls in one LLM response (Anthropic allows this).
→ Action: Process tool calls sequentially. Show confirm card for each in order. Don't send results until all confirmed (or first cancelled = abort remainder).

## Contradiction Hat

**Finding C1:** REQ-011 says plan uses "CTL, ATL, TSB" — these are computed metrics (Chronic/Acute Training Load, Training Stress Balance). PromptBuilder already sends these. But `create_training_plan` tool call happens inside the LLM response — the LLM has already received CTL/ATL/TSB in the system prompt. No contradiction; LLM uses them to structure the plan JSON.

**Finding C2:** REQ-005 says schedule_workout dates "must be in the future". REQ-012 says "plan respects existing season events". These are compatible but GCToolExecutor needs access to Seasons data at execution time (not just at prompt time).
→ Action: GCToolExecutor receives Context* (has access to Seasons via context->athlete).

## Dispositions

| Finding | Action | Status |
|---------|--------|--------|
| L1: workout path may not exist | verify/create dir in GCToolExecutor | fix-in-design |
| L2: SeasonEvent has no workout_file_ref | store filename in description | fix-in-design |
| L3: "structured text" fallback under-specified | WARN message + text-only mode | fix-in-design |
| P1: 84-workout plan confirmations | already handled by plan preview card | pass |
| P2: prompt injection risk | signal-typed path, not text-parsed | pass |
| E1: FTP not set | check before execute, return error to LLM | fix-in-design |
| E2: workout_name not found | scan library, return error to LLM | fix-in-design |
| E3: multiple tool calls in one response | sequential confirm, abort-on-cancel | fix-in-design |
| C1: CTL/ATL/TSB in system prompt | no issue | pass |
| C2: GCToolExecutor needs Context* | pass Context* in constructor | fix-in-design |

**A1 verdict: PASS — all findings dispositioned.**
