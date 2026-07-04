# Design — AI Coach Tool Use

## Component Map

```
CoachChatWidget
  ├── LLMService (abstract, DEC-002)
  │     ├── AnthropicClient  → tools[] in request JSON
  │     ├── OpenAIClient     → tools[] / functions[] in request JSON
  │     └── GeminiClient     → functionDeclarations[] in request JSON
  │
  ├── GCToolExecutor (new, DEC-002)
  │     ├── registerTool(ToolDef)
  │     ├── onToolCallRequested(name, args) → shows ToolConfirmCard
  │     ├── executeCreateWorkout(args)  → writes .zwo (DEC-003)
  │     ├── executeScheduleWorkout(args) → Seasons API
  │     ├── executeCreateSeasonEvent(args) → Seasons API
  │     └── executeCreatePlan(args) → shows PlanPreviewCard
  │
  ├── ToolConfirmCard : QWidget (new, DEC-005)
  │     └── emits: confirmed(name, args) / cancelled()
  │
  └── PlanPreviewCard : QWidget (new, DEC-004)
        └── emits: planApplied(planJson) / planCancelled()
```

## DES-001 — LLMService tool interface additions

```cpp
// In LLMService.h

struct ToolDef {
    QString name;
    QString description;
    QJsonObject inputSchema;  // JSON Schema for parameters
};

// New virtual methods (default no-op for backward compat):
virtual void setTools(const QList<ToolDef>& tools) { Q_UNUSED(tools) }
virtual bool supportsTools() const { return false; }

// New signals:
// toolCallRequested(toolName, args) — emitted when LLM requests a tool call
// toolResultReady(toolName, result) — call this to send result back to LLM
virtual void sendToolResult(const QString& toolUseId, const QString& toolName, const QJsonObject& result) {}
```

New signals on LLMService:
- `toolCallRequested(QString callId, QString toolName, QJsonObject args)`

## DES-002 — Tool definitions (v1)

### create_workout
```json
{
  "name": "create_workout",
  "description": "Create a structured cycling workout and save to the athlete's GoldenCheetah workout library. Power values are percentages of the athlete's FTP.",
  "input_schema": {
    "type": "object",
    "properties": {
      "name": { "type": "string" },
      "description": { "type": "string" },
      "intervals": {
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "type": { "enum": ["warmup","steadystate","cooldown","intervals","ramp","free"] },
            "duration_s": { "type": "integer", "minimum": 30 },
            "power_low_pct": { "type": "number", "minimum": 0, "maximum": 300 },
            "power_high_pct": { "type": "number", "minimum": 0, "maximum": 300 },
            "repeat": { "type": "integer", "minimum": 1, "default": 1 },
            "rest_duration_s": { "type": "integer" },
            "rest_power_pct": { "type": "number" }
          },
          "required": ["type","duration_s","power_low_pct"]
        }
      }
    },
    "required": ["name","intervals"]
  }
}
```

### schedule_workout
```json
{
  "name": "schedule_workout",
  "description": "Schedule a workout on a specific date in the athlete's season calendar.",
  "input_schema": {
    "type": "object",
    "properties": {
      "workout_name": { "type": "string" },
      "date": { "type": "string", "format": "date" },
      "notes": { "type": "string" }
    },
    "required": ["workout_name","date"]
  }
}
```

### create_season_event
```json
{
  "name": "create_season_event",
  "description": "Add a race or key event to the athlete's season calendar.",
  "input_schema": {
    "type": "object",
    "properties": {
      "name": { "type": "string" },
      "date": { "type": "string", "format": "date" },
      "priority": { "enum": ["A","B","C"] },
      "description": { "type": "string" }
    },
    "required": ["name","date","priority"]
  }
}
```

### create_training_plan
```json
{
  "name": "create_training_plan",
  "description": "Design a multi-week training plan. Returns a plan for user review before applying. Each workout follows the same structure as create_workout.",
  "input_schema": {
    "type": "object",
    "properties": {
      "plan_name": { "type": "string" },
      "start_date": { "type": "string", "format": "date" },
      "weeks": {
        "type": "array",
        "items": {
          "type": "object",
          "properties": {
            "week_number": { "type": "integer" },
            "phase": { "type": "string" },
            "workouts": {
              "type": "array",
              "items": {
                "type": "object",
                "properties": {
                  "day_offset": { "type": "integer", "minimum": 0, "maximum": 6 },
                  "name": { "type": "string" },
                  "description": { "type": "string" },
                  "intervals": { "$ref": "#/definitions/intervals" }
                },
                "required": ["day_offset","name","intervals"]
              }
            }
          }
        }
      }
    },
    "required": ["plan_name","start_date","weeks"]
  }
}
```

## DES-003 — ZWO generation (DEC-003)

`GCToolExecutor::buildZwoXml(args)` → returns QString of ZWO XML

```
intervals[].type mapping:
  "warmup"      → <Warmup Duration PowerLow PowerHigh />
  "cooldown"    → <Cooldown Duration PowerLow PowerHigh />
  "steadystate" → <SteadyState Duration Power />  (power_low_pct = power)
  "intervals"   → <IntervalsT Repeat OnDuration OffDuration OnPower OffPower />
  "ramp"        → <Ramp Duration LowCadence HighCadence />  (power ramp)
  "free"        → <FreeRide Duration FlatRoad />
```

Save path: `context->athlete->home->workouts().path() + "/" + sanitizedName + ".zwo"`

## DES-004 — Anthropic tool use wire format

Request body addition:
```json
{
  "tools": [ { "name": "...", "description": "...", "input_schema": {...} } ],
  "tool_choice": { "type": "auto" }
}
```

Response parsing: if `stop_reason == "tool_use"`, extract content blocks of type `"tool_use"`:
```json
{ "type": "tool_use", "id": "toolu_xxx", "name": "create_workout", "input": {...} }
```

Then send tool result:
```json
{
  "role": "user",
  "content": [{ "type": "tool_result", "tool_use_id": "toolu_xxx", "content": "success" }]
}
```

## DES-005 — OpenAI tool use wire format

Request: `"tools": [{"type":"function","function":{"name","description","parameters"}}]`  
Response: `choices[0].message.tool_calls[{id, type:"function", function:{name, arguments}}]`  
Follow-up: role `"tool"`, `tool_call_id`, content = result

## DES-006 — Gemini tool use wire format

Request: `"tools": [{"functionDeclarations": [...]}]`  
Response: `candidates[0].content.parts[{functionCall:{name, args}}]`  
Follow-up: `functionResponse` part

## DES-007 — Failure modes

| Failure | Handling |
|---------|----------|
| LLM returns malformed tool args | Validate against ToolDef.inputSchema; send error result to LLM; show error in chat |
| Power value > 300% FTP | Reject before write; tell LLM to retry with corrected value |
| Date in the past | Reject; ask LLM to pick future date |
| Workout name collision | Append suffix (1), (2), etc. |
| User cancels confirmation | Send `{"cancelled": true}` as tool result; LLM acknowledges |
| Provider doesn't support tools | `supportsTools()` → false; show warning in chat; fall back to text advice |

## New Files

| File | Purpose |
|------|---------|
| `src/Coach/GCToolExecutor.h/.cpp` | Tool registry, execution, ZWO generation |
| `src/Coach/ToolConfirmCard.h/.cpp` | Inline confirmation widget (DEC-005) |
| `src/Coach/PlanPreviewCard.h/.cpp` | Multi-week plan preview widget (DEC-004) |
