# Intake — AI Coach Tool Use

## Verbatim Request
> "the AI coach feature is working, but at the moment does not has the option to create a new workout, create workouts is a tool that already exist in the project but the AI coach can not interact with it. Task: create a feature in the AI coach that allows to the AI Coach interact with all the tools for example create new workouts, interact with the calendar, schedule a complete training plan evaluating all different metrics already available in the project to create a tailor-made plan."

## 5-Whys Trace

**Why** does the user want AI Coach to create workouts?
→ Because the coach gives training advice but can't act on it — advice without execution.

**Why** does it matter that coach can't act?
→ Because users must manually translate coach recommendations into workouts and calendar entries — error-prone, friction-heavy.

**Why** is that a problem?
→ Because GC already has all the data (FTP, zones, history, body comp, season plan) but the coach can't close the loop between insight and action.

**Why** close the loop?
→ Because a coach that can plan, schedule, and create is meaningfully more valuable than one that just talks — it becomes an autonomous training assistant.

**Why** is autonomous training assistance the goal?
→ Because the user has 999 activities and rich metrics but no automated path from "here's what my data says" to "here's a plan in your calendar." That's the value gap.

**Root motivation:** Close the read-analyze-act loop: AI reads athlete data → analyzes it → creates workouts and schedules them — all within GC without user translation.

## Codebase State (2026-05-10)

- `src/Coach/`: AnthropicClient, OpenAIClient, GeminiClient, LLMService (abstract), CoachChatWidget, AthleteContext, PromptBuilder
- No tool/function-call support in LLMService or any client
- Workout creation: WorkoutWizard (GUI), ErgFile (.erg/.mrc), ZwoParser (.zwo)
- Calendar: Season, Seasons (newSeason/updateSeason/deleteSeason), SeasonEvent
- Seasons::newSeason(name, start, end, type) is the programmatic API
- ErgFile is the workout data model; saved as .erg/.mrc/.zwo on disk
