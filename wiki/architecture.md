# Architecture Map (spoke)            updated:2026-07-04

## Components (GC core, static by tree structure — not ledger-governed)
C1 ANT        | ANT+/ANT device protocol stack                        | code:src/ANT
C2 Charts     | analysis/train chart windows + plotting widgets       | code:src/Charts
C3 Core       | domain model: Athlete, Context, DataFilter, calendar  | code:src/Core
C4 FileIO     | ride file format parsers/writers + backup             | code:src/FileIO
C5 Gui        | main window, dialogs, wizards, sidebars               | code:src/Gui
C6 Metrics    | ride metric calculations (power/HR/pace/CP/PMC)       | code:src/Metrics
C7 Planning   | training-plan window                                 | code:src/Planning
C8 R          | embedded R integration                                | code:src/R
C9 Train      | trainer/device control                                | code:src/Train
C10 Coach     | AI Coach (LLM tool-use) — SHIPPED                     | gov:coach:DEC-001..013 | code:src/Coach
C11 Cloud     | CloudService integrations (Strava, Dropbox, TrainerDay, RideWithGPS, CyclingAnalytics, GarminConnect) | code:src/Cloud
C12 Python    | embedded CPython host (PythonEmbed, SIP bindings, garminconnect/ adapter pkg) | code:src/Python

## Garmin Connect feature — components (governed, active ledger)
G1 AddCloudWizard          | UI entry: "Garmin Connect" tile + credentials/MFA pages | gov:DEC-001,004,011,012 | code:src/Gui (AddCloudWizard), src/Cloud/GarminCredentialsPage.{h,cpp}
G2 GarminConnect           | CloudService subclass (Query\|Download capabilities)    | gov:DEC-005,006 | code:src/Cloud/GarminConnect.{h,cpp}
G3 GarminWorker            | QObject in QThread; mailbox + cancellation + sole GIL holder | gov:DEC-002,013 | code:src/Cloud/GarminWorker.{h,cpp}
G4 IGarminAuthClient       | pure-virtual auth-dispatcher seam (credentials page ↔ SSO) | gov:DEC-012 | code:src/Cloud/IGarminAuthClient.h
G5 IGarminPyAdapter        | pure-virtual worker ↔ Python seam (Auth-only surface GREEN) | gov:DEC-013 | code:src/Cloud/IGarminPyAdapter.h
G6 garmin_client.py (DES-012) | sole module importing `garminconnect`; stable adapter, swap point | gov:DEC-002,DES-012 | code:src/Python/garminconnect/garmin_client.py
G7 gc_rate.py              | rate-limit + backoff decorator around library calls (planned) | gov:DEC-007,DES-005 | code:src/Python (not yet landed)
G8 Persistence (atomic writer) | tokens.json / imported-<uid>.json / backfill-state-<uid>.json under per-athlete config dir | gov:DEC-003,DES-002,006 | code:(pending, DES-002)

## Data flow — Garmin Authenticate slice (REQ-002, VAL-006 PASS)
AddCloudWizard(G1) → GarminCredentialsPage → IGarminAuthClient(G4) → GarminWorker(G3)
  [Qt signal request(payload), off-GUI-thread] → IGarminPyAdapter(G5) → garmin_client.py(G6, DES-012)
  → garminconnect / curl_cffi (vendor lib) → HTTPS (OS trust store) → connect.garmin.com
Result returns via Qt finished(payload) signal delivered to GUI thread event loop.
Today: IGarminPyAdapter is satisfied by FakePyAdapter (test double) for REQ-002 C++ contract;
production G6 impl exists (Auth-only) but the C++-side embedded-Python adapter that calls it
(the "PyEmbeddedAdapter") is NEXT_GATE VAL-007 — not yet wired to G5.

## Integration points / contracts
G1↔G4: AddCloudWizard/GarminCredentialsPage inject an IGarminAuthClient; production impl
        dispatches to G3 (DEC-012, DES-003/003a).
G3↔G5: GarminWorker calls IGarminPyAdapter.authenticate() off the GUI thread; interface
        locks in `PyAuthOutcome` shape (DEC-013, DES-001/001a).
G5↔G6: production PyEmbeddedAdapter (pending, VAL-007) will bridge IGarminPyAdapter calls
        into src/Python/garminconnect/garmin_client.py via the existing PythonEmbed host.
G2↔G8: CloudService capability calls (Query|Download) read/write the atomic-writer
        persistence layer (DES-002/006) — not yet implemented (REQ-004/006/008 not started).
C10↔C12: AI Coach (Coach) also uses embedded Python via C12's PythonEmbed core — shared
        infrastructure between the two features; no direct Coach↔Garmin coupling today.

## Watch (live risks — from ledger open/deferred/accept-with-rationale items)
- Sub-interpreter wedge has no auto-recovery (A2-001, deferred → Phase 1.5: thread-heartbeat
  + kill-and-recreate). Reopens if a hang is observed in the field.
- Library-tracked SSO risk: python-garminconnect follows Garmin's undocumented SSO; accepted
  (A2-002) because the adapter seam (G6/DES-012) bounds swap cost — documented in
  REQ-NF-Compat-001. Reopens if upstream breaks and no fork exists.
- sqlite-sidecar migration trigger: DEC-003 (per-athlete flat-file layout) reopens if beta
  sidecar read time >500ms or >10k entries.
- Phishing-surface risk: fake-looking Garmin login dialog (A2-008, deferred — separate UX
  warning ticket).
- PEP 3134 exception chaining is cosmetic only (A3-R002-M10, accepted) — `.original` is the
  real contract; do not rely on `__cause__` for control flow.
- C++ mutation coverage: no mull-cxx/cosmic-ray-cpp yet (A3-R001-tool, deferred to Phase 3
  entry) — REQ-001/REQ-002 C++ mutation confidence today is manual/A3-cycle only.
