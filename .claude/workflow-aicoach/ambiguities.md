# Ambiguities

| ID | Term / Assumption | Resolution |
|----|-------------------|------------|
| AMB-001 | "all the tools" — which tools exactly? | Scoped to: (1) create workout, (2) schedule workout to calendar, (3) create season/plan, (4) add season event. Additional tools deferred. |
| AMB-002 | Workout format — .erg, .mrc, or .zwo? | ZWO (Zwift format) preferred: structured, XML, widely used. ErgFile supports it. Confirm with user. |
| AMB-003 | "tailor-made plan" — what time horizon? | Deferred until DEC-001. Could be a single week, a mesocycle (4-6 weeks), or a full season. |
| AMB-004 | LLM provider scope — Anthropic only or all? | All three clients (Anthropic, OpenAI, Gemini) support tool use. Must be provider-agnostic via LLMService. |
| AMB-005 | UI interaction model — does coach ask confirmation before creating? | Assumed yes: coach proposes → user approves → coach executes. Safety first. Needs confirmation. |
| AMB-006 | "interact with calendar" — read-only already works (PromptBuilder sends season data). Write = create season events? | Write = add SeasonEvent (races, A/B/C events) + assign workouts to dates. |
| AMB-007 | Multi-LLM tool format — each provider has different tool schema (Anthropic tools vs OpenAI functions vs Gemini functionDeclarations) | Must abstract behind LLMService interface. Each client marshals to its own format. |
