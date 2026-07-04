# Traceability Matrix — AI Coach Tool Use

Last updated: 2026-05-17

| REQ | Description | DEC(s) | DES(s) | TEST(s) | Commit |
|-----|-------------|--------|--------|---------|--------|
| REQ-001 | create_workout tool → .zwo file | DEC-001, DEC-003 | DES-001, DES-002, DES-003 | TEST-003 | ede5df406 |
| REQ-002 | Power values validated 0–300% FTP | DEC-001 | DES-007 | TEST-003 (implicit via buildZwoXml) | ede5df406 |
| REQ-003 | No disk write without confirmation | DEC-001, DEC-005 | DES-007 | TEST-005, TEST-006, TEST-009, TEST-010 | ede5df406 |
| REQ-004 | schedule_workout → SeasonEvent | DEC-001 | DES-002 | TEST-004 (tool schema) | ede5df406 |
| REQ-005 | Dates must be future | DEC-001 | DES-007 | TEST-001 | ede5df406 |
| REQ-006 | Existing workout on date: inform coach | DEC-001 | DES-007 | TEST-016, TEST-017 | ede5df406 |
| REQ-007 | create_season_event with A/B/C priority | DEC-001 | DES-002 | TEST-004 (priority required) | ede5df406 |
| REQ-008 | Priority MUST be A/B/C | DEC-001 | DES-002 | TEST-004 (priority in required[]) | ede5df406 |
| REQ-009 | create_training_plan → preview before apply | DEC-001, DEC-004 | DES-002 | TEST-004 (weeks constraints) | ede5df406 |
| REQ-010 | User must click Apply before writes | DEC-004, DEC-005 | DES-007 | TEST-007, TEST-008 | ede5df406 |
| REQ-011 | Plan uses FTP/CTL/ATL/TSB | DEC-001 | DES-002 | — (LLM behavior, not unit testable) | ede5df406 |
| REQ-012 | Plan respects existing A-race events | DEC-001 | DES-002 | — (LLM behavior) | ede5df406 |
| REQ-013 | Tools work with Anthropic, OpenAI, Gemini | DEC-002 | DES-004, DES-005, DES-006 | TEST-004 (v1Tools schema) | ede5df406 |
| REQ-014 | No-tool provider: fallback + inform user | DEC-002 | DES-007 | TEST-018 (registerTools side); UI message in CoachChatWidget:790 by inspection | ede5df406 |
| REQ-015 | No write without confirmation | DEC-005 | DES-007 | TEST-005, TEST-006, TEST-009, TEST-010 | ede5df406 |
| REQ-016 | Tool execution non-blocking | DEC-002 | DES-001 | TEST-019 | ede5df406 |
| REQ-017 | Round-trip < 5s excl. LLM latency | DEC-001 | DES-007 | TEST-020 | ede5df406 |
| REQ-018 | Coach explains action before confirmation | DEC-005 | DES-007 | Verified by inspection: PromptBuilder.cpp:1031 (static string); CoachChatWidget.cpp:415 (gated on supportsTools) | ede5df406 |
| REQ-019 | writeSeasons() failure surfaced as error result, not silent success | DEC-012 | — | TEST-012 (null ctx), manual (disk-full) | pending |
| REQ-020 | remove_calendar_event tool removes event by name+date with confirmation | DEC-013 | — | TEST-011..TEST-015 | pending |

## Test index

| TEST | File | Covers |
|------|------|--------|
| TEST-001 | unittests/Core/coach/testCoachTools.cpp | validateDate: past/today/future/invalid-format |
| TEST-002 | unittests/Core/coach/testCoachTools.cpp | sanitizeFilename: unsafe chars, empty input |
| TEST-003 | unittests/Core/coach/testCoachTools.cpp | buildZwoXml: all 6 block types, HTML escaping |
| TEST-004 | unittests/Core/coach/testCoachTools.cpp | v1Tools: 4 tools, required fields, schema constraints |
| TEST-005 | unittests/Core/coach/testCoachTools.cpp | onCancelled: cancelled result emitted, no write (REQ-003, REQ-015) |
| TEST-006 | unittests/Core/coach/testCoachTools.cpp | Confirmation gate: result NOT emitted before user action (REQ-003, REQ-015) |
| TEST-007 | unittests/Core/coach/testCoachTools.cpp | Plan preview gate: result NOT emitted before user action (REQ-010) |
| TEST-008 | unittests/Core/coach/testCoachTools.cpp | Plan validation guards: empty/too-many weeks, invalid date (REQ-010) |
| TEST-009 | unittests/Core/coach/testCoachTools.cpp | Null context guard: error result, no disk write (REQ-003, REQ-015) |
| TEST-010 | unittests/Core/coach/testCoachTools.cpp | Payload size guard: oversized args rejected before confirmation (REQ-003, REQ-015) |
| TEST-011 | unittests/Core/coach/testCoachTools.cpp | remove_calendar_event schema: required name+date (DEC-013) |
| TEST-012 | unittests/Core/coach/testCoachTools.cpp | remove_calendar_event null context guard (DEC-013) |
| TEST-013 | unittests/Core/coach/testCoachTools.cpp | remove_calendar_event removes matching event, in-memory + result (DEC-013) |
| TEST-014 | unittests/Core/coach/testCoachTools.cpp | remove_calendar_event not_found status when no match (DEC-013) |
| TEST-015 | unittests/Core/coach/testCoachTools.cpp | remove_calendar_event invalid date rejected before context access (DEC-013) |
| TEST-016 | unittests/Core/coach/testCoachTools.cpp | schedule_workout on occupied date without force → conflict result (REQ-006) |
| TEST-017 | unittests/Core/coach/testCoachTools.cpp | schedule_workout on occupied date with force:true → success, event appended (REQ-006) |
| TEST-018 | unittests/Core/coach/testCoachTools.cpp | registerTools on no-tool provider → setTools never called (REQ-014) |
| TEST-019 | unittests/Core/coach/testCoachTools.cpp | All null-context onConfirmed paths complete < 5ms (REQ-016) |
| TEST-020 | unittests/Core/coach/testCoachTools.cpp | GC-side round-trip (no layout) completes < 50ms, well under 5s budget (REQ-017) |

## Build

Tests use CMake: `cmake -DBUILD_TESTS=ON ..; cmake --build . --target testCoachTools`  
Run: `QT_QPA_PLATFORM=offscreen ./unittests/Core/coach/testCoachTools`  
Stub preamble: `unittests/Core/coach/stubs/GCStubPreamble.h` (force-included via -include flag)

## Gaps (integration tests deferred)

REQ-006, REQ-014, REQ-016, REQ-017, REQ-018 require a live GC instance with a real Context.
These are marked for integration/manual testing.

Previously deferred REQ-003, REQ-010, REQ-015 now covered by TEST-005 through TEST-010.
