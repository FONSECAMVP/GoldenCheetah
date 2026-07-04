# Ambiguities — Garmin Connect Integration

Each entry must be resolved or explicitly deferred before Phase 0 exit.

| # | Term / assumption | Why ambiguous | Status |
|---|-------------------|---------------|--------|
| A-01 | **"the process of import activities from Garmin Connect"** | GoldenCheetah has *no* existing Garmin Connect cloud-import process. The phrase could mean (a) build a new `CloudService` for Garmin Connect; (b) extend the USB/Garmin-Express flow; (c) add a new menu item parallel to file import. | resolved (2026-05-17): (a) — implement as a new `CloudService` subclass in `src/Cloud/`. Aligns with Strava/RWG/Dropbox pattern. |
| A-02 | **Scope: "import activities" only, or broader?** | Library exposes 131+ methods. User said "options that could improve functions" — invites scope expansion. | resolved (2026-05-17): **Bidirectional sync** — activity download + workout upload + schedule push. Health metrics (HRV/readiness/body-comp) reserved as a follow-up scope item, called out under A-06 overlap. |
| A-03 | **Integration mechanism: Python vs C++ port vs subprocess** | GC has embedded Python (FixPy). Alternatives: subprocess + JSON, or native C++ port. | resolved (2026-05-17): **Embedded Python via PythonEmbed**. Add `garminconnect` to `src/Python/requirements.txt`. GIL/threading is a known risk (deferred to DEC-design phase). |
| A-04 | **Auth & MFA UX** | Where do tokens live; single account or per-athlete. | resolved (2026-05-17): **Per-athlete**. Tokens stored under each athlete's GC config dir, matching Strava/Dropbox precedent. |
| A-05 | **Garmin ToS posture** | Mobile SSO flow is unofficial. Account-suspension risk. | resolved (2026-05-17): **Proceed; document risk to user**. First-connect dialog will explicitly say this is unofficial access and not endorsed by Garmin. |
| A-06 | **Overlap with existing features** | Body comp (Withings), workouts/calendar (AI Coach `coach:DEC-013`). | open (will be addressed in PRD non-goals + cascade-impact section of relevant DECs). Pre-decision: Garmin Connect *augments* — does not replace Withings; *complements* AI Coach (Coach authors workouts locally; new Garmin push tool can sync them upward). |
| A-07 | **Distribution / dependencies** | `garminconnect` requires `curl_cffi` which links libcurl-impersonate. This is a non-trivial wheel and needs to be present on Windows / macOS / Linux. Acceptable? Or prefer a path with no new transitive deps? | deferred to DEC-002 (mechanism decision) |
| A-08 | **"Improvements to functions"** | Could mean: enrich existing data (e.g., add Garmin HRV to RideItem), or add net-new functions (e.g., import training-readiness widget). Need direction. | open — user input needed |

## Decisions deferred (explicit)

- **A-07** is folded into the integration-mechanism decision (DEC-002 family); it is not independently resolvable until A-03 lands.
