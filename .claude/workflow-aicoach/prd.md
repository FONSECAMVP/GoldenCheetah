# PRD — AI Coach Tool Use (v1)

## Goals
Enable the GoldenCheetah AI Coach to execute training actions — creating workouts, scheduling them, adding calendar events, and building multi-week training plans — using athlete metrics already in GC, with user confirmation before any write.

## User Stories

### US-001 — Create workout
As an athlete, I can ask the coach to create a workout (e.g., "give me a VO2max session for today"), and the coach generates a structured workout file saved to my GC library, after I confirm.
- **REQ-001** [MUST] Coach can call `create_workout` tool with intervals defined as {duration_s, power_pct_ftp}[] and a name/description. GC saves result as .zwo to athlete workout library.
- **REQ-002** [MUST] All interval power values validated: 0–300% FTP. Values outside range rejected before write.
- **REQ-003** [MUST] Coach cannot write to disk without user confirmation (confirm dialog before every tool call).

### US-002 — Schedule workout
As an athlete, I can tell the coach to schedule a workout on a specific date, and it appears in my season calendar.
- **REQ-004** [MUST] Coach can call `schedule_workout` tool with (workout_name, date). GC adds a SeasonEvent of type "workout" on that date.
- **REQ-005** [MUST] Dates validated: must be in the future (today or later).
- **REQ-006** [SHOULD] If a workout already exists on that date, coach is informed and asks user to confirm override.

### US-003 — Add season event
As an athlete, I can ask the coach to add a race or key event to my season.
- **REQ-007** [MUST] Coach can call `create_season_event` with (name, date, priority: A/B/C, description). GC creates SeasonEvent via Seasons API.
- **REQ-008** [MUST] Priority must be one of: A, B, C (maps to SeasonEvent priority 0/1/2).

### US-004 — Build multi-week training plan
As an athlete, I can ask the coach to design a 4–12 week training plan based on my metrics, review it, then apply it.
- **REQ-009** [MUST] Coach can call `create_training_plan` tool with a structured plan: {weeks: [{week, phase, workouts: [{day_of_week, name, intervals[]}]}]}. GC generates a plan preview document.
- **REQ-010** [MUST] User sees plan preview (human-readable) and must click "Apply Plan" before any workouts or events are written.
- **REQ-011** [SHOULD] Plan generation uses athlete's current FTP, CTL, ATL, TSB (from existing AthleteContext metrics) to calibrate load.
- **REQ-012** [SHOULD] Plan respects any existing season events (A-race date) already in the calendar.

### US-005 — All providers supported
- **REQ-013** [MUST] Tool use works with Anthropic, OpenAI, and Gemini clients (each marshals tools to its own schema).
- **REQ-014** [MUST] If active provider does not support tool use, coach falls back to structured text and informs user.

## Non-Functional Requirements

- **REQ-015** [MUST] No workout or event written without explicit user confirmation.
- **REQ-016** [MUST] Tool execution does not block the UI thread.
- **REQ-017** [SHOULD] Tool call round-trip (LLM → GC executes → confirmation dialog) completes in < 5s excluding LLM latency.
- **REQ-018** [NICE] Coach explains what it's about to do in plain language before presenting confirmation.

## Explicit Non-Goals (v1)
- Modifying or deleting existing workouts or events
- Pushing workouts to Strava/Garmin/TrainerDay
- Real-time workout modification during training
- Multi-athlete / coach-athlete portal
