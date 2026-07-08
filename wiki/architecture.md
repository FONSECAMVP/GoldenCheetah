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
G1 AddCloudWizard          | UI entry: "Garmin Connect" tile (page 21) + credentials/MFA pages | gov:DEC-001,004,011,012 | code:src/Cloud/AddCloudWizard.{h,cpp}, src/Cloud/GarminCredentialsPage.{h,cpp}
G2 GarminConnect           | CloudService subclass (Query\|Download capabilities)    | gov:DEC-005,006 | code:src/Cloud/GarminConnect.{h,cpp}
G3 GarminWorker            | QObject in QThread; sole adapter caller. Ops: authenticate (finished/failed) + downloadActivity (REQ-007: downloaded(id,bytes)/downloadFailed(id,GarminDownloadFailure)), each mapping the adapter outcome→signals off the GUI thread | gov:DEC-002,013 | code:src/Cloud/GarminWorker.{h,cpp}
G4 IGarminAuthClient       | pure-virtual auth-dispatcher seam (credentials page ↔ SSO) | gov:DEC-012 | code:src/Cloud/IGarminAuthClient.h
G5 IGarminPyAdapter        | pure-virtual worker ↔ Python seam: authenticate() + downloadActivity() (REQ-007) GREEN; PyAuthOutcome + PyDownloadOutcome value types | gov:DEC-013 | code:src/Cloud/IGarminPyAdapter.h
G5a PyEmbeddedAdapter      | production IGarminPyAdapter over embedded CPython (GIL RAII, type-then-kind classification via shared takeRaisedException). RETAINS the authenticated GarminClient (m_client) across authenticate→downloadActivity — REQ-005 session model (password not kept); bytes marshalled binary-exact via PyBytes_AsStringAndSize; dtor DECREFs under GIL only if Py_IsInitialized | gov:DES-013 | code:src/Cloud/PyEmbeddedAdapter.{h,cpp}
G5b GarminAuthChain        | RAII assembly: QThread+GarminWorker+WorkerAuthClient around a non-owned IGarminPyAdapter* | gov:DES-001,001a (impl. note under DES-003) | code:src/Cloud/GarminAuthChain.{h,cpp}
G6 garmin_client.py (DES-012) | sole module importing `garminconnect`; stable adapter, swap point | gov:DEC-002,DES-012 | code:src/Python/garminconnect/garmin_client.py
G7 gc_rate.py              | rate-limit + backoff decorator around library calls (planned) | gov:DEC-007,DES-005 | code:src/Python (not yet landed)
G8 Persistence (atomic writer) | tokens.json / imported-<uid>.json / backfill-state-<uid>.json under per-athlete config dir | gov:DEC-003,DES-002,006 | code:(pending, DES-002)

## Data flow — Garmin Authenticate slice (REQ-002, VAL-007 code-complete)
AddCloudWizard(G1, page 21) → GarminCredentialsPage → IGarminAuthClient(G4) → GarminAuthChain(G5b)
  → GarminWorker(G3) [Qt signal request(payload), off-GUI-thread] → IGarminPyAdapter(G5)
  → PyEmbeddedAdapter(G5a) → garmin_client.py(G6, DES-012)
  → garminconnect / curl_cffi (vendor lib) → HTTPS (OS trust store) → connect.garmin.com
Result returns via Qt finished(payload) signal delivered to GUI thread event loop.
Today (2026-07-05): the full production chain is wired end-to-end behind `GC_WANT_GARMINCONNECT`
— PyEmbeddedAdapter (TEST-005 GREEN, `garmin-py` label) and GarminAuthChain (TEST-006 GREEN,
`garmin-fast` label, FakePyAdapter) both land in commits on master; AddCloudWizard routes the
Garmin tile to page 21 and owns adapter+chain lifecycle (DES-001a order). VAL-007 CLV pass is
the immediate next gate — code is complete but not yet cross-layer validated. No automated test
covers the wizard routing itself (only the chain in isolation) — flagged for A3.

## Integration points / contracts
G1↔G4: AddCloudWizard/GarminCredentialsPage inject an IGarminAuthClient; production impl
        dispatches to G3 (DEC-012, DES-003/003a).
G3↔G5: GarminWorker calls IGarminPyAdapter.authenticate() + downloadActivity() off the GUI
        thread; interface locks in `PyAuthOutcome` / `PyDownloadOutcome` shapes (DEC-013,
        DES-001/001a). REQ-007: download reuses the session authenticate() established — the
        adapter (G5a) is the sole holder of the live Python client, so the worker holds ONE
        adapter across auth+download (slice 3 wires the worker/CloudService side).
G5↔G6: PyEmbeddedAdapter (DES-013, src/Cloud/PyEmbeddedAdapter.{h,cpp}) bridges
        IGarminPyAdapter calls into garmin_client.py — GIL via RAII, classification by
        type-then-kind (LSN-006). Test-linked CPython today; app-build wiring pending (VAL-007).
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
- PyEmbeddedAdapter requires CPython ≥3.12 (`PyErr_GetRaisedException`); a 3.11-or-older
  target needs a `PyErr_Fetch`/Normalize fallback (T-005 build report, 2026-07-04).
- Include-order hazard: `Python.h` must precede Qt headers in PyEmbeddedAdapter.cpp (Qt
  `slots` macro clash). Any unity-build/TU-merge when wiring the app-build
  GC_WANT_GARMINCONNECT block can silently re-trigger this — check at the tile-routing slice.
- AddCloudWizard Garmin routing/lifecycle is now guarded by TEST-007 (A3-R002-TR-01, resolved):
  `nextId()`→page-21 routing, `ensureGarminAuthPage()` idempotency, and the `~AddCloudWizard()`
  DES-001a teardown order (`delete garminChain` before `delete garminAdapter`) each kill a
  mutant. The suite exercises the logic directly, NOT via live QWizard navigation — the
  `AddService::clicked`/back-next wiring that *calls* nextId/ensure is compiled but not driven
  (residual gap; candidate follow-up).
- REQ-007 NOT fully deployed (download chain GREEN adapter→PyEmbeddedAdapter→worker, but the
  CloudService side is deferred): `GarminConnect` is still the REQ-001 tile stub — no Q_OBJECT, no
  worker wiring, no `readFile` override. The acceptance-completing pieces (readFile staging bytes as
  garmin-<id>.<ext> → FitRideFile → RideItem; the FIT→TCX fallback per DES-004) are DEFERRED because
  (a) readFile needs a worker-in-CloudService lifecycle + loaded tokens (REQ-004/006, not started), and
  (b) the fallback trigger "FIT not available" depends on unvalidated library behaviour (PRD Assumption
  B). The worker's downloadActivity op takes `fmt` verbatim so the future fallback drives ORIGINAL→TCX
  without an API change. Revisit at REQ-004/006.
- PyEmbeddedAdapter retained-client lifecycle (REQ-007 slice 2, new): the adapter now holds a
  live PyObject* GarminClient (m_client) across authenticate→downloadActivity. Access is
  worker-thread-confined by DES-001; the dtor DECREFs under a GIL guard only when
  Py_IsInitialized() (a finalized-interpreter DECREF would be use-after-free — leak is chosen
  instead). Watch at slice 3 / A3: (a) real destruction happens on the wizard/GUI thread while the
  worker thread may have touched m_client — confirm no concurrent access; (b) a failed re-auth
  currently leaves the prior session in place (does not clear m_client) — verify that is intended
  when REQ-012 Disconnect lands; (c) TEST-009 exercises dtor-with-interpreter-up only, not the
  finalized-interpreter leak branch.
- GarminAuthChain last-resort teardown (A3-R002-TR-08, deferred → Phase 1.5 with A2-001): a
  genuinely uncancellable native busy-loop (no cancellation point) defeats `QThread::terminate()`;
  `~GarminAuthChain` then destroys a still-running `QThread` → `qFatal` abort. Only reachable via
  a pure native wedge — realistic wedges (blocking I/O, Python hitting a cancellation point)
  unwind cleanly, and the graceful `quit()+wait()` bound is now asserted tightly (LSN-009). The
  benign "Qt caught an exception thrown from an event handler" warning on `terminate()` is the
  pthread_cancel forced-unwind, not a defect. Harden the dtor (detach/leak rather than destroy a
  running thread) if Phase 1.5 wedge-recovery doesn't already remove the reachability.
