# Intake — Garmin Connect Integration via python-garminconnect

## Verbatim Request
> "I want to integrate this Python library 'garminconnect' https://github.com/cyberjunky/python-garminconnect in the process of import activities from Garmin Connect, explore the library and give me some options that could improve functions in the application"

## 5-Whys Trace

**Why** integrate python-garminconnect into GoldenCheetah?
→ Because activities recorded on Garmin devices currently reach GC only via file-based paths (FIT/TCX/GPX export, USB sync via Garmin Express, or watching a folder). There is no direct cloud-pull from Garmin Connect.

**Why** is file-based import insufficient?
→ Because it requires the athlete to either (a) connect the device by USB, (b) keep Garmin Express running, or (c) manually download FIT files. Activities recorded on smart-trainers/phones never touch a Garmin USB device — they live only in Garmin Connect.

**Why** does that gap matter to this user?
→ Because GC already has cloud sync for Strava, RideWithGPS, Dropbox, SportTracks, etc., but Garmin Connect — the *primary* data source for most Garmin-device owners — is conspicuously absent from that list. Athletes are forced to round-trip through Strava (Garmin → Strava → GC) and lose the structured workout / training-effect / HRV / body-comp data Garmin provides that Strava strips.

**Why** is the python library the chosen mechanism (vs writing a C++ HTTPS client)?
→ Because Garmin Connect has no public REST API; access requires the mobile-app SSO flow plus DI OAuth bearer-token rotation. `python-garminconnect` already implements and maintains the SSO/MFA/token-refresh machinery. Re-implementing it in C++ would duplicate work that breaks every time Garmin changes the flow.

**Why** does the user say "options that could improve functions" rather than scoping a single task?
→ Because the library exposes 131+ methods across 13 categories (activities, workouts, body composition, HRV, training plans, gear, sleep, hydration, golf, etc.). Several of these overlap with features GC already has (workouts, calendar/season events, body composition via Withings, AI Coach training plans). The user wants the trade space surfaced before committing to scope.

**Root motivation:** Make Garmin Connect a first-class cloud data source for GoldenCheetah — at minimum for activity import, potentially extending to body composition, HRV/readiness, structured workout sync, and training plans — using an existing maintained library rather than re-implementing the Garmin SSO flow.

## Codebase State (2026-05-17)

### Existing Garmin-related surface
- `src/Train/GarminServiceHelper.{h,cpp}` — *only* purpose: stop the Windows "Garmin" service to free a USB device. **Not** a Garmin Connect client.
- `src/Resources/linux/51-garmin-usb.rules` — udev rules for Garmin USB devices.
- No `Cloud/GarminConnect.*` file exists. Garmin Connect is **absent** from the cloud-sync stack.

### Existing cloud-sync architecture (the integration target)
- `src/Cloud/CloudService.h/.cpp` — abstract base class for cloud services. Reimplementations in: `Strava`, `RideWithGPS`, `Dropbox`, `SportTracks`, `Xert`, `PolarFlow`, `Selfloops`, `SixCycle`, `Nolio`, `Withings`, `TrainingsTageBuch`, `Azum`, `SportsPlusHealth`, `OpenData`, `CyclingAnalytics`, `LocalFileStore`, `CalDAVCloud`. Standard ops: `open()/close()`, `readFile()`, `writeFile()`, `readdir()`, OAuth.
- `src/Cloud/AddCloudWizard.{h,cpp}` — UI for adding/configuring a new cloud-service account.
- `src/Cloud/OAuthDialog.{h,cpp}` — generic OAuth handler used by Strava/Dropbox/SportTracks etc.

### Existing Python integration (the candidate integration mechanism)
- `src/Python/PythonEmbed.{h,cpp}` — embedded CPython runtime, already used by FixPy data processors.
- `src/FileIO/FixPyDataProcessor.cpp`, `FixPyRunner.cpp`, `FixPySettings.cpp`, `FixPyScriptsDialog.cpp` — production callers.
- `src/Python/requirements.txt` — declares `sip`, `numpy`, `pandas`, `scipy`, `lmfit`, `plotly`, `importlib_metadata`. Comments mention `requests` as "nice to have" but it's not currently pulled in.
- `src/Resources/python/library.py` — bundled helper script.

### Existing file-format import (the current data path)
- `src/FileIO/FitRideFile.{h,cpp}`, `TcxRideFile.{h,cpp}`, `TcxParser.{h,cpp}`, `GpxRideFile.{h,cpp}` — Garmin-native formats already parsed natively in C++. Any downloaded activity would still need to reach one of these parsers.

### Adjacent features that might gain from Garmin Connect data
- AI Coach (DEC-001..013 in workflow/decisions.md — already shipped via `GC_WANT_COACH`): creates workouts (.zwo), schedules season events, generates training plans. **Could consume HRV / training readiness / training-effect signals from Garmin Connect** to ground its recommendations.
- Withings cloud sync (`Cloud/Withings.*`, `Cloud/WithingsDownload.*`) — already pulls body composition. **Overlap risk** if we also pull body comp from Garmin.
- Seasons / SeasonEvent (`Train/Season*`) — calendar surface. **Could be a sink** for Garmin Connect workout schedules.

### Licensing
- GoldenCheetah: GPL v2+ (per `COPYING`)
- python-garminconnect: MIT — GPL-compatible (MIT can be combined into a GPL work).

### Build / packaging considerations
- GC ships installers for Windows, macOS, Linux (AppImage). Embedded Python is already a per-platform build concern; adding one more pip package (`garminconnect`) is mechanically the same.
- Secrets handling: `Secrets.h` is already gitignored (commit `f4e6f38c1`) for API keys; a similar approach exists for token storage if needed.

## Library Capabilities (from upstream README, 2026-05-17)

- **Auth**: Mobile SSO + DI OAuth, MFA callback support, token persistence at `~/.garminconnect/garmin_tokens.json`, automatic refresh.
- **131+ methods across 13 categories**:
  - User & Profile (4)
  - Daily Health & Activity (9)
  - Advanced Health Metrics (12) — fitness age, HRV, VO2, training readiness, running tolerance
  - Historical Data & Trends (9)
  - Activities & Workouts (36) — read, upload, schedule, delete
  - Body Composition & Weight (8)
  - Goals & Achievements (15) — challenges, badges, PRs, race predictions
  - Device & Technical (7)
  - Gear & Equipment (7)
  - Hydration & Wellness (12)
  - System & Export (4)
  - Training Plans (2)
  - Golf (3)
- **Typed workouts** (optional pydantic extra `[workout]`): `RunningWorkout`, `CyclingWorkout`, `SwimmingWorkout`, `WalkingWorkout`, `HikingWorkout`, `MultiSportWorkout`, `FitnessEquipmentWorkout` plus warmup/interval/recovery/cooldown step helpers and repeat groups.
- **License**: MIT. **Runtime dep**: `curl_cffi` (required), `pydantic` (optional).
