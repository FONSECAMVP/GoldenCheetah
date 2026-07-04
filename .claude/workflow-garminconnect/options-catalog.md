# Options Catalog — How python-garminconnect Could Improve Existing GC Functions

This is exploratory input for Phase 1 (PRD). It is not a decision and contains no Three-Options scoring; it is a *menu* of improvement opportunities mapped to the library's 13 method categories. Each row says: which GC feature could change, what new capability the library enables, and the integration phase the work would land in.

| # | Library category | Library capability | GC feature it touches | Improvement enabled | Likely phase |
|---|------------------|--------------------|-----------------------|----------------------|--------------|
| O-01 | Activities (36) | `get_activities()`, `download_activity()` FIT/TCX/GPX | Existing `Cloud/*` services / file import | **Direct Garmin → GC pull, no Strava round-trip; preserves structured-workout & training-effect data Strava strips.** | **Phase 1** (MVP) |
| O-02 | Activities | `delete_activity()` | None today | Cleanup tool for accidental rides — *low value, optional* | Phase 3+ |
| O-03 | Activities | `upload_activity(fit_file)` | Manual export-and-upload | Push a GC-recorded activity (e.g., from a Wahoo / Stryd) up to Garmin Connect | Phase 2 |
| O-04 | Workouts (typed) | `upload_workout(RunningWorkout/CyclingWorkout)` | AI Coach `create_workout` tool (`coach:DEC-013`) | Coach builds a .zwo → also pushes to Garmin Connect so it appears on the watch | **Phase 2** |
| O-05 | Workouts | `schedule_workout(date)` | AI Coach `schedule_workout` tool + `Train/Season` | Coach schedules locally → mirrors to Garmin calendar so watch alerts on the day | **Phase 3** |
| O-06 | Workouts | `delete_workout()`, `unschedule_workout()` | AI Coach `remove_calendar_event` tool (`coach:DEC-013`) | Remove on either side → propagate | Phase 3 |
| O-07 | Training Plans (2) | Read/write training plan | AI Coach `create_training_plan` tool (`coach:DEC-004`) | Coach exports a full plan to Garmin's plan engine; watch nags daily without GC needing to be open | Phase 3+ |
| O-08 | Advanced Health Metrics (12) | HRV, training readiness, VO2, body battery, training load | RideItem / Athlete metadata / charts | **Coach plan recommendations grounded in Garmin's own HRV/readiness signal — closes the read-loop for `coach:REQ-011`.** | Phase 4 candidate |
| O-09 | Daily Health & Activity (9) | Steps, calories, intensity minutes | Athlete daily summary | Daily wellness panel in GC | Phase 4 candidate |
| O-10 | Body Composition & Weight (8) | Weigh-in history, body-fat %, lean mass | Existing `Cloud/Withings.*` | **Overlap with Withings.** Two strategies: (a) only enable for athletes who don't use Withings; (b) merge sources with conflict resolution. Decision deferred. | Phase 4 candidate |
| O-11 | Goals & Achievements (15) | Race predictions, PRs, badges, challenges | None today | Race-prediction widget; PR-tracking surface | Phase 4+ (cosmetic) |
| O-12 | Historical Data & Trends (9) | Date-range queries, weekly aggregates | Existing summary charts | Backfill: pull 12 months on first-connect to seed GC — *one-shot importer* | Phase 1 add-on (bulk-import flag) |
| O-13 | Gear & Equipment (7) | Bike/shoe odometers, retirement dates | None today | Per-bike mileage tracking that mirrors what's on the watch | Phase 4+ |
| O-14 | Hydration & Wellness (12) | Sleep, hydration, blood pressure, menstrual cycle | None today | Sleep-quality overlay on training-load chart (sleep ↔ next-day TSS) — *high analytical value* | Phase 4 candidate |
| O-15 | Device & Technical (7) | Device list, settings, alarms | None today | Device-aware import (e.g., distinguish Edge vs Fenix activities) | Phase 4+ |
| O-16 | User & Profile (4) | Profile, settings | None today | Could auto-populate Athlete profile on first connect (DOB, weight, HRMax) | Phase 1 nice-to-have |
| O-17 | Golf (3) | Scorecards, shot-by-shot | None today | **Out of scope** (GC is cycling/running-focused) | Non-goal |

## How the catalog feeds the PRD

- Phase 1 PRD (driven by DEC-001 Option B) takes its core REQs from **O-01** (activity download) and **O-12** (bulk backfill on first-connect) and **O-16** (optional profile auto-fill).
- Phase 2 PRD picks up **O-03, O-04**.
- Phase 3 PRD picks up **O-05, O-06, O-07**.
- Phase 4 (or later) PRD picks from **O-08, O-09, O-10, O-13, O-14, O-15** — these are the "improve functions" line items the user specifically asked about.
- **O-17 is an explicit non-goal**: golf data is not aligned with GC's cycling/running focus.
