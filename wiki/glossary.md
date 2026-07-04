# Glossary (spoke)

ride / activity = a single recorded workout session (any sport); GC's core unit of data,
  read/written via src/FileIO parsers (FIT, TCX, GC, CSV, SRM, PWX, WKO, …)  [source: src/FileIO, RideFile.*]
athlete dir = per-athlete config/data directory holding rides, settings, and (for Garmin)
  `<athlete-dir>/garminconnect/{tokens.json,imported-<uid>.json,backfill-state-<uid>.json}`
  [source: DEC-003, design.md DES-002]
FIT / TCX = Garmin/industry binary and XML ride-file formats; FIT is the Garmin default
  download/staging format (DEC-006), TCX is the fallback  [source: REQ-007, DEC-006]
CloudService = GC's abstract base for remote-service integrations (Strava, Dropbox,
  TrainerDay, RideWithGPS, CyclingAnalytics, GarminConnect); exposes capabilities like
  Query/Download  [source: src/Cloud/CloudService.h, DEC-005]
worker / adapter seam = the injected-interface pattern used to isolate GC's C++ core from
  a volatile third-party dependency: GarminWorker (QThread) talks to Python only through
  IGarminPyAdapter (DEC-013); GarminCredentialsPage talks to auth only through
  IGarminAuthClient (DEC-012). Swapping the underlying library/impl is a one-seam change.
  [source: DEC-012, DEC-013, design.md]
ledger = a `.claude/workflow-<feature>/` directory: the full quality-gated-dev-workflow
  state for one feature (state.md, decisions.md, design.md, prd.md, traceability.md,
  findings.md, cycles/, validations/). One feature = one ledger; see workflow-INDEX.md.
  [source: .claude/workflow-INDEX.md]
slice = a vertically-scoped subset of a REQ taken through RED→GREEN→A3→VAL independently
  (e.g. REQ-002's "adapter slice", "wizard-wiring slice", "end-to-end slice") — lets a large
  REQ ship incrementally with its own VAL gate per slice.  [source: state.md ## reqs, ## vals]
A-cycle (A0/A1/A2/A3…) = an adversarial review cycle in the quality-gated-dev-workflow skill;
  each cycle produces dispositioned findings (fix-now/defer/accept/informational).
  [source: findings.md, references/adversarial-cycles.md]
VAL / CLV = a cross-layer validation gate (Validator subagent) checking REQ→DEC→DES→TEST→
  CODE→commit alignment before a slice/phase is declared done.
  [source: references/cross-layer-validation.md, validations/]
accept-with-rationale = an A-cycle finding disposition: a real gap the team chooses NOT to
  fix now, with a written reason (vs. "defer" which has a future trigger ticket).
  [source: findings.md dispositions column]
GC_WANT_GARMINCONNECT = the CMake option gating Garmin Connect sources into the build;
  default OFF (DEC-011, staged/reversible rollout).  [source: src/CMakeLists.txt:784]
IGarminAuthClient / IGarminPyAdapter = the two pure-virtual C++ interfaces (DEC-012,
  DEC-013) that bound the Garmin feature's two integration seams (credentials↔SSO,
  worker↔Python).  [source: src/Cloud/IGarminAuthClient.h, IGarminPyAdapter.h]
DES-012 (adapter) = src/Python/garminconnect/garmin_client.py — the single module in the
  codebase allowed to `import garminconnect` (the vendored Python library); everything
  else calls through it.  [source: garmin_client.py docstring]
PythonEmbed = GC's pre-existing embedded-CPython host (src/Python/PythonEmbed.{h,cpp}),
  reused by both the AI Coach and Garmin Connect features.  [source: src/Python]
qmake / .pro vs CMake = GoldenCheetah's two parallel build systems; CMakeLists.txt is the
  newer one, migration is in progress, and both must currently be kept in sync per file
  addition.  [source: root CMakeLists.txt + src/src.pro coexistence]
