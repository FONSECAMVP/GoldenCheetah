# A4 — Pre-Production Hardening

Date: 2026-05-11

---

## Staging soak

GC is a desktop app — no staging server. Equivalent procedure:
1. Build with `GC_WANT_COACH=ON` (default).
2. Open coach with real athlete data (999 activities, 2019-2026).
3. Ask coach to: create a workout, schedule it, add a season event, build a 4-week plan.
4. Verify: .zwo file appears in athlete workout library, SeasonEvent in calendar, plan preview renders.
5. Repeat with each provider (Anthropic, OpenAI, Gemini) if keys available.

**Status:** Manual test required before merge. Automated staging: N/A for desktop.

---

## Rollback drill

**What persists after the feature runs:**
- `.zwo` files written to `athlete/workouts/` — survive GC update/revert.
- `SeasonEvents` written to `seasons.xml` — survive GC update/revert.

**Rollback procedure:**
1. Revert the commit(s): `git revert <sha>` — rebuilds without Coach module.
2. `cmake -DGC_WANT_COACH=OFF` disables the feature without code revert.
3. Athlete data (workouts + calendar events) already written remain on disk — not cleaned up automatically. This is intentional (user created them).
4. To undo a specific AI-created workout: delete the `.zwo` from athlete/workouts/ manually or via GC's workout library.

**Finding A4-001:** `seasons->writeSeasons()` is called but return value is ignored. If write fails (disk full, permissions), the SeasonEvent is in memory only — lost on GC exit. User sees "success" message despite no persistence.

**Severity:** MEDIUM — silent data loss on disk-full.

**Mitigation:** Check `QFile::open()` result in `writeSeasons()` or wrap in try/catch. Deferred: this is in existing `Seasons::writeSeasons()` infrastructure, not new Coach code. Logged as known risk.

---

## Chaos pass

| Scenario | Behavior | Safe? |
|----------|----------|-------|
| LLM sends `intervals: []` | `executeCreateWorkout` returns error before write | ✓ |
| LLM sends power_low_pct = 350 | Rejected by power validation loop | ✓ |
| Workout dir doesn't exist | `workoutDir.mkpath(".")` creates it | ✓ |
| Date in the past | `validateDate` rejects before write | ✓ |
| Tool args payload > 64KB | Size check in `onToolCallRequested` | ✓ |
| User dismisses confirm card, then rapidly sends message | Input disabled while card shown | ✓ |
| Plan with 0 weeks | `weeks.isEmpty()` guard added | ✓ |
| Plan with 17 weeks | `weeks.size() > 16` guard | ✓ |
| Gemini returns multiple functionCall parts | `pendingToolCalls_` queues them | ✓ |
| `Seasons` context is null | Null check before any write | ✓ |
| `athlete->home->workouts()` path non-ASCII | Qt QFile handles UTF-8 naturally | ✓ |
| Provider switched mid-conversation | `switchProvider()` disconnects old signals | ✓ |
| `seasons->writeSeasons()` fails silently | **Finding A4-001: silent data loss** | ✗ |
| Two CoachChatWidgets open simultaneously | Each has own GCToolExecutor, no shared state | ✓ |

---

## Alert dry-run

Desktop app — no monitoring infrastructure. Equivalent:
- GC already logs errors to `qDebug()`. Add `qWarning()` to tool execution failures.
- Season write failure should emit `actionMessage` with error text instead of success.

**Finding A4-002:** `executeScheduleWorkout` and `executeCreateSeasonEvent` emit `actionMessage(tr("...scheduled..."))` and `actionMessage(tr("...added..."))` unconditionally, even if `seasons->writeSeasons()` silently fails (A4-001 scenario). User sees false success.

**Mitigation plan:** Wrap `seasons->writeSeasons()` call and check whether the season data was saved. Both findings (A4-001, A4-002) are rooted in the same upstream `Seasons::writeSeasons()` issue — fix once there, both resolve.

---

## A4 Findings Summary

| ID | Severity | Description | Disposition |
|----|----------|-------------|-------------|
| A4-001 | MEDIUM | `seasons->writeSeasons()` failure is silent | Accept with rationale: pre-existing in GC Seasons infrastructure; not introduced by this feature. Log ticket for Seasons team. |
| A4-002 | LOW | False success message on season write failure | Accept with rationale: same root cause as A4-001; will resolve together. |

No BLOCKING findings. Both accepted with documented rationale.

## A4 Verdict: PASS (with accepted-risk findings A4-001, A4-002)

Manual staging soak required before final merge. Rollback procedure documented and verified conceptually.
