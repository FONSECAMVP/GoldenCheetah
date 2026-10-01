# Decision Ledger — Garmin Connect Integration

> Decision IDs in this project start at DEC-001. Independent of the prior `workflow/decisions.md` (AI Coach project). Cross-project refs use `coach:DEC-NNN`.

Compact schema per `references/formats.md` § `decisions.md`. **The recap source is the `## Decision index` immediately below** — the Tier-1 head every session reads instead of scrolling the entries. (It used to say "the `state.md ## decs` table is the recap source"; DEC-015 DELETED that table on 2026-07-12 and no replacement head was ever built, so this file opened straight into DEC-001's full entry for thirteen months of work. Built 2026-08-23, librarian Job 3.) Full entries below are drilled only when creating, editing, or cascading from a specific DEC. Detailed deliberation notes from the original v1 ledger are preserved verbatim in `decisions-history-archive.md`.

---

## Decision index

> **PRECEDENCE (this index is a duplication risk — LSN-014/LSN-035 — so the rule is stated, not assumed).**
> DEC status is single-homed in `decisions.md` = THIS file, so the index sits inside its own canonical home.
> **On conflict the entry's `Status:` field wins and the index row is the drift.** A status change is not done
> until BOTH carry it. One line per DEC, every id, no gaps (VAL-017's failure was an index stopping at DEC-019).
> REQ/DES/TEST/VAL status is NOT here — that is `traceability.md`.
>
> **Split rule (auditable, applied 2026-08-23):** **Dormant** iff superseded by a later DEC, OR a fully-executed
> one-shot (toolchain / rollout / recording-only / build-repair) with no open dependent. Everything else is
> **Active**. Dormancy is ordering, not closure — a dormant DEC still binds, it just cannot change.

### Active index

| DEC | Question → chosen option | Status | Date |
|-----|--------------------------|--------|------|
| DEC-001 | Delivery sequence for bidirectional sync → B staged ship (download → upload → schedule) | accepted | 2026-05-17 |
| DEC-002 | C++ → embedded Python integration mechanism → B worker thread + mailbox | accepted | 2026-05-17 |
| DEC-003 | Token + sidecar on-disk layout → B per-athlete config dir | accepted | 2026-05-17 |
| DEC-004 | Credentials + MFA dialog UX shape → B `AddCloudWizard` pages | accepted | 2026-05-17 |
| DEC-006 | Activity file format + staging path → FIT default | accepted (recording-only) | 2026-05-17 |
| DEC-007 | Rate-limit + retry placement → B Python-side decorator inside the worker thread | accepted | 2026-05-17 |
| DEC-012 | Auth-dispatcher seam, page ↔ SSO layer → A inject `IGarminAuthClient` | accepted | 2026-05-24 |
| DEC-013 | Worker ↔ Python adapter seam → A inject `IGarminPyAdapter` | accepted | 2026-05-24 |
| DEC-014 | Token persistence write-ownership → B adapter exports the blob, C++ owns the atomic 0600 write | accepted | 2026-07-11 |
| DEC-015 | Ledger status drift → C one canonical home per id-class + absence-check lint; local `state.md` deleted | accepted · MECHANISM | 2026-07-12 |
| DEC-016 | FIT→TCX fallback trigger for `readFile` → C kind-aware retry + content-sniff backstop | accepted | 2026-07-14 |
| DEC-017 | REQ-008 sidecar-persistence shape → A dedicated `GarminSidecarStore` | accepted | 2026-07-19 |
| DEC-018 | Where `garmin_user_id` persists → B separate account-agnostic `active-account.json` | accepted | 2026-07-19 |
| DEC-019 | Connect/disconnect persist trigger → C `CloudService::disconnect()` virtual + wizard `finished()` capture | accepted | 2026-07-20 |
| DEC-020 | A live session outliving Disconnect → C fail-closed re-check + cheap hardening, lifecycle work → REQ-017 | accepted · implemented `f001c7d20` | 2026-08-02 |
| DEC-021 | How Disconnect invalidates a LIVE session → B account-epoch latched at `open()`, compared in memory | accepted | 2026-08-03 |
| DEC-022 | How a fail-closed readFile reports itself → A labelled completion | PARTIALLY IMPLEMENTED | 2026-08-03 |
| DEC-023 | Carry read FAILURE explicitly → option 3, an explicit `readFailed` channel, not a `message` heuristic | accepted | 2026-08-04 |
| DEC-024 | Making the modeless sync dialog's self-deletion safe → A done(int) gate + BlockingCall depth counter | accepted | 2026-08-05 |
| DEC-026 | No nested event loop under construction → B two-phase init, ctor builds the shell, `start()` does the work | accepted | 2026-08-06 |
| DEC-027 | Unify sync-dialog lifetime → A heap + `WA_DeleteOnClose` + modeless `open()` for BOTH callers | accepted | 2026-08-06 |
| DEC-029 | Upload-dialog UAF fix shape → B two-phase `start()` + heap/`WA_DeleteOnClose`, `exec()`/modal PRESERVED | accepted | 2026-08-07 |
| DEC-030 | Collaborator-lifetime UAF fix shape → B reparent both dialogs to `context->tab`, probe-first, QPointer rider | accepted | 2026-08-10 |
| DEC-031 | Reopening DEC-025 → B frame-counted DEFERRED REAPER replaces the deliberate store leak, probe-first | accepted | 2026-08-11 |
| DEC-032 | Closing the silent stall → A in-loop `continue` in the branch that arms nothing, adopting `uploadNext`'s shape | accepted | 2026-08-13 |
| DEC-033 | The readFile bool contract — false does NOT mean "armed nothing" | accepted | decided 2026-09-03 |
| DEC-034 | Who owns a `QTreeWidgetItem` across a nested event loop → C a `listGeneration` counter bumped by `refreshClicked` | accepted · NOT reopened | 2026-08-14 |
| DEC-034/036 AMENDMENT | Completion slots never got the drivers' guard (A3-R028c-F2) → snapshot `batchGeneration` there | accepted, folded into DEC-036's Option C build | 2026-08-21 |
| DEC-035 | Where the abort guard lives, `processEvents()` being no guaranteed drain → B co-locate it with the irreversible call | accepted | 2026-08-15 |
| DEC-036 | Correlating a completion with the transfer that asked for it → C explicit per-operation write identity | accepted + BUILT on formal reopen | 2026-08-18; re-decided 2026-08-22 |
| DEC-038 | The third list mutator, SORTING → A sorting is OFF for the batch's duration, as a DEC-034 AMENDMENT | accepted · BUILT + committed `514d8e88f` | 2026-08-22 |
| DEC-039 | libusb dependency wiring (ORCH-036) → B complete find_path/find_library for BOTH APIs | accepted | 2026-08-23 |
| DEC-040 | Bounding the 22 unbounded provider waits (S-R028f-F1/F3, C1–C6) → W3: shared bounded-request outcome contract | accepted | 2026-08-26 · amended 2026-08-30 |
| DEC-041 | File-IO layer Context UAF at RideFile.cpp:999 (B-R025-01/A3-R021b-F2) → Option B, hoist-and-capture | accepted | 2026-09-03 · amended 2026-09-08 |
| DEC-042 | saveRide's post-autoProcess dereferences vs. parent-teardown UAF (B-R028-17) → Option A, QPointer self-bail | accepted | 2026-09-05 |
| DEC-043 | CloudServiceAutoDownload cross-thread UAF vs. teardown (A3-R028e-F1) → Option C, guards + cooperative cancel | accepted | 2026-09-05 |
| DEC-044 | RideFile::appendOrUpdatePoint reads a deleted RideFilePoint* (B-R029-01) → Option A, alias surviving point | accepted | 2026-09-08 |
| DEC-045 | REQ-014 translation keying → keys GarminErrors::translate() on GarminAuthFailure::Kind, not raw exception name | accepted | 2026-09-10 (build 2026-09-08) |
| DEC-046 | REQ-015 CAPTCHA detection → defer, no structured signal survives the real `garminconnect` dependency | accepted (user decision, option 2 of 3) · no code written | 2026-09-10 |
| DEC-047 | REQ-010/DES-009 pagination model → per-activity pacing/checkpointing, not per-page, against real garminconnect | accepted | 2026-09-10 |
| DEC-048 | B-R010-04 UI wiring scope split → fix backfill-local exposure now, defer GarminConnect's context gap | accepted | 2026-09-10/11 |
| DEC-049 | REQ-NF-Pkg-001 scope expansion → port Garmin sources into qmake release before installer work | accepted | 2026-09-11 |
| DEC-050 | REQ-013 scope → narrow the first slice to DOB/weight/height, defer HR-max/FTP | accepted (user decision, option 1 of 3) · **BUILT + committed `e17262a0b`** | 2026-09-12 |
| DEC-051 | REQ-NF-Obs-001's "ErrorBus" clause → satisfied by existing error channels; build missing qDebug trace logging | accepted | 2026-09-12 |
| DEC-052 | B-STAGE9-01 fix → shared CPython bootstrap owned by neither PythonEmbed nor PyEmbeddedAdapter alone | accepted | 2026-09-13 |
| DEC-053 | B-STAGE9-15 remedy → developer-trace literals exempt from i18n guard as a CATEGORY; guard's heuristic is wrong | accepted | 2026-09-15 |
| DEC-054 | B-STAGE9-16 remedy → default ctest gate becomes FAIL-SAFE: ctest -LE gate-exclude replaces opt-in garmin-fast | accepted | 2026-09-15 |
| DEC-055 | B-STAGE9-26 remedy → response_invalid contract covers EVERY required key incl. activityId | accepted | 2026-09-19 |
| DEC-056 | B-STAGE9-29 remedy → listing entries named from activity's LOCAL start time, from startTimeLocal | accepted | 2026-09-19 |
| DEC-057 | B-STAGE9-28 remedy → an EMPTY managed root must be DECLARED pre-armed or the lint-ownership guard goes RED | accepted | 2026-09-19 |
| DEC-058 | B-STAGE9-38 remedy → Garmin adapter ships as installable distribution under new import namespace | accepted | 2026-09-19 |
| DEC-059 | B-STAGE9-36 round 3 → a DEC arms a root via a machine-readable Arms: bullet naming patterns globs | accepted | 2026-09-19 |
| DEC-060 | B-STAGE9-36 round 8 → Arms declaration names its own DEC; mismatched id is a hard parse error | accepted | 2026-09-19 |
| DEC-061 | B-STAGE9-40 remedy → main.cpp:552 consumes ensureInitialized()'s failure Result nonfatally | accepted | 2026-09-20 |
| DEC-062 | B-STAGE9-39 remedy → Qt-side deployment locator returns {home, programName} set as explicit PyConfig fields | accepted | 2026-09-20 |
| DEC-063 | B-STAGE9-41 remedy → CMake defines GC_WANT_PYTHON target-locally; GC_HAVE_PYTHON is DELETED, not aliased | accepted | 2026-09-20 |
| DEC-064 | B-STAGE9-36 remedy → Arms authorization moves from Markdown bullets into a structured fail-closed record | ACCEPTED | 2026-09-20 |
| DEC-065 | Stage 9's installer evidence is earned on the AppVeyor pipeline, not on a local build | accepted | 2026-09-20 |
| DEC-066 | B-STAGE9-45 remedy → adapter-module provenance becomes a C++-owned per-interpreter identity ledger | accepted | 2026-09-20 |
| DEC-067 | B-STAGE9-59's smoke-assert class terminated → final strengthening, residual accepted as false negative | accepted | 2026-09-20 |
| DEC-068 | B-STAGE9-66's premise refuted → returned-object compare guards nothing under PyImport_Import semantics | accepted | 2026-09-24 |
| DEC-069 | B-STAGE9-71 remedy → all three platforms; Windows and macOS gain assertion against PRODUCED artifact | accepted | 2026-09-24 |
| DEC-070 | B-STAGE9-78 remedy → stage under payload's TRUE extension, sniffed from magic bytes | accepted | 2026-09-26 |
| DEC-071 | B-STAGE9-79 remedy → split overloaded record into pending manifest plus completion-only imported map | accepted | 2026-09-26 |
| DEC-072 | B-STAGE9-83 remedy → inflate gzip in controller at stage time with zlib, re-sniff, stage true extension | accepted | 2026-09-26 |
| DEC-073 | B-STAGE9-86 remedy → invert predicate: accept ONE complete single-member gzip, FIT-or-ZIP only | accepted | 2026-09-26 |
| DEC-074 | findings.md row-size budget → the CAP is the defect: replace 200B with ~600B soft / ~1,200B hard, per-register | accepted | 2026-09-26 |
| DEC-075 | DEC-071 slice 1 write contract → the STORE holds it: pending becomes single-writer via saveBackfillState | accepted | 2026-09-27 |
| DEC-076 | B-STAGE9-108 remedy → a LOCK: GarminSidecarStore serializes on resolved sidecar path across load-to-writeOver | accepted | 2026-09-27 |
| DEC-077 | B-STAGE9-116 in-process() UAF → FROZEN as upstream: RideImportWizard's raw Context* shared by non-Garmin callers | accepted | 2026-09-27 |
| DEC-078 | B-STAGE9-112 remedy → REMOVE IT: one variable serves as both exclusive marker and inclusive range bound | accepted | 2026-09-27 |
| DEC-079 | B-STAGE9-79 slice 3 → DIALOG PRE-START SELF-CLASSIFICATION: absent schema_version loads as version 0 | accepted | 2026-09-27 |
| DEC-080 | B-STAGE9-111 vs DEC-077 freeze → NARROW THE FREEZE: stays absolute for repairs, permits additive no-op virtual | accepted | 2026-09-27 |
| DEC-081 | B-STAGE9-128 timestamp TEXT compares → CANONICALISE AT ADAPTER BOUNDARY: normalise to one UTC spelling | accepted | 2026-09-27 |
| DEC-082 | B-STAGE9-130 crash-safe migration → THREE-PHASE v0-PRESERVING TRANSACTION: fixed-order locks, three writes | accepted | 2026-09-27 |
| DEC-083 | lastSuccessStartTimeGMT contradiction → CURSOR BECOMES COMPLETENESS WATERMARK: recordImport stops writing it | accepted | 2026-09-27 |
| DEC-084 | startTimeGMT setTimeSpec REINTERPRETS offset → ONE OFFSET-CORRECT INSTANT PRIMITIVE, GarminTime.h | accepted | 2026-09-27 |
| DEC-086 | B-STAGE9-156 builder runtime → Claude Code builder; Codex only in read-only roles | accepted | 2026-09-28 |
| DEC-087 | B-STAGE9-154 v0 stamp class → WRITERS PRESERVE v0: only migration phase 3 stamps v1 | accepted | 2026-09-28 |
| DEC-088 | D2XX CI fetch completeness → MARKER after upstream-recipe copy; malformed-vendor-archive residual PINNED | accepted | 2026-09-29 |
| DEC-089 | Fork CI recipe lag behind upstream → SYNC appveyor.yml + appveyor/** to upstream/master, re-apply fork deltas | accepted | 2026-09-30 |

### Dormant index

*Superseded, or a fully-executed one-shot with no open dependent. Still binding; still one line per DEC.*

| DEC | Question → chosen option | Status | Date |
|-----|--------------------------|--------|------|
| DEC-085 | B-STAGE9-156 worktree mandate vs inspector-cycle → WORKTREE MANDATE DOES NOT BIND a supervised round | superseded 2026-09-28 by DEC-086 | 2026-09-28 |
| DEC-005 | Phase-1 CloudService capabilities (Query\|Download) | accepted (recording-only; any other value contradicts REQ-011) | 2026-05-17 |
| DEC-008 | Testing toolchain → A extend the incumbent (QTest+CTest C++ / pytest+coverage.py Python) | accepted · one-shot, executed | 2026-05-17 |
| DEC-009 | Style/quality toolchain → A existing clang-format/clang-tidy + `ruff` + `mypy --strict` on new Python | accepted · one-shot, executed | 2026-05-17 |
| DEC-010 | Pre-commit automation → A the `pre-commit` framework, scoped to new Garmin paths | accepted · one-shot, executed | 2026-05-17 |
| DEC-011 | Phase-1 rollout strategy (CMake flag `GC_WANT_GARMINCONNECT`, default OFF) | accepted (recording-only) | 2026-05-17 |
| DEC-025 | Surviving PARENT TEARDOWN → A destructor declines to delete the store while busy (a deliberate leak) | SUPERSEDED by DEC-031 | 2026-08-05 |
| DEC-028 | Clean-checkout build repair → B commit the missing CMake wiring rather than guard around it (ORCH-001) | accepted | 2026-08-07 |
| DEC-037 | Separating an abandoned write from the live one → A a retired-ticket set | SUPERSEDED 2026-08-22 by DEC-036 Option C | 2026-08-19 |

**Index measurements 2026-08-23 (librarian Job 3).** 39 ids, 39 rows, 0 gaps · Active 31 + 1 amendment row,
Dormant 8. The skill's split trigger is ~20 active lines; 31 exceeds it and the surplus was NOT forced into
Dormant — the remainder is the Garmin dialog-lifetime family and every one still governs live
`src/Cloud/CloudService.cpp` behaviour. A false dormant is worse than a long active list.

---

## DEC-001 — Solution shape: delivery sequence for bidirectional sync
- Status: accepted (B — staged ship)
- Chose: B — staged ship: Phase 1 read-only download; Phase 2 upload; Phase 3 schedule push; Phase 4 health metrics.
- Binding:
  1. Phase 1 = read-only Cloud/GarminConnect.* (REQ-001 through N)
  2. Phase 2 = workout upload (extends coach:DEC-013)
  3. Phase 3 = schedule push (Train/Season*)
  4. Phase 4 = health metrics
  5. Forces DEC-002 to ship a clean thread-isolation primitive reusable by Phases 2/3
- Dependents: DEC-002, DEC-003, DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-009, DEC-010, DEC-011
- Full record: archive/decisions-full.md

## DEC-002 — Python integration mechanism (C++ → embedded Python)
- Status: accepted (B — worker thread + mailbox)
- Chose: B — worker thread + mailbox: GarminWorker : QObject in QThread, Qt signal request()/finished() API.
- Binding:
  1. GarminWorker : QObject in QThread
  2. Qt-signal request()/finished() API
  3. Phase 2 tests assert thread-id at every Python call site
  4. DEC-007 rate limiter sits at queue head
  5. Sets the template Phase 2/3 reuse
- Dependents: DEC-007, DES-001, DES-009, DES-010, REQ-NF-Threads-001, REQ-NF-Cancel-001, REQ-NF-Perf-002, REQ-NF-Perf-003
- Full record: archive/decisions-full.md

## DEC-003 — Token + sidecar on-disk layout
- Status: accepted (B — per-athlete config dir)
- Chose: B — per-athlete dir: tokens.json singular; sidecars partitioned per Garmin account ID; JSON now, sqlite cascade if needed.
- Binding:
  1. Token path: athlete-dir/garminconnect/tokens.json
  2. Sidecar dedup: imported-<garmin_user_id>.json
  3. Resumable backfill state: backfill-state-<garmin_user_id>.json
  4. All writes go via tmp+rename helper from DEC-002's worker
  5. Sidecar format is JSON in Phase 1
  6. Cascade to sqlite if sidecar-read-time exceeds 500ms or entry-count exceeds 10,000
- Dependents: REQ-004, REQ-006, REQ-008, REQ-010, REQ-012, DEC-002, DES-002, DES-006, DES-009, DES-010
- Full record: archive/decisions-full.md

## DEC-004 — Credentials + MFA dialog UX shape
- Status: accepted (B — AddCloudWizard pages)
- Chose: B — AddCloudWizard pages: two new QWizardPage subclasses; CAPTCHA page only on detection.
- Binding:
  1. Two new QWizardPage subclasses: GarminCredentialsPage + GarminMfaPage
  2. CAPTCHA page surfaced only on detection
  3. Reuses AddCloudWizard Back/Next/Cancel/Help machinery
- Dependents: REQ-002, REQ-003, REQ-009, REQ-014, REQ-015, DES-003, DES-008, DES-011
- Full record: archive/decisions-full.md

## DEC-005 — Phase-1 CloudService capabilities
- Status: accepted (recording-only)
- Chose: Query and Download only. No Upload, Sync, OAuth (SSO used, not OAuthDialog), no Delete.
- Binding:
  1. capabilities() returns Query and Download only
  2. No Upload
  3. No Sync
  4. No OAuth (library SSO used, not GC's generic OAuthDialog)
  5. No Delete
- Dependents: REQ-011, DES-004
- Full record: archive/decisions-full.md

## DEC-006 — Activity file format + staging path
- Status: accepted (recording-only)
- Chose: FIT preferred via download_activity(ORIGINAL); TCX fallback only if Garmin returns a non-FIT original.
- Binding:
  1. FIT preferred: download_activity(activity_id, dl_fmt=ORIGINAL)
  2. TCX fallback only if Garmin returns a non-FIT original
  3. Staging path: GC's existing import staging dir
  4. Filename pattern: garmin-<activity_id>.<ext>
- Dependents: REQ-007, REQ-008, DES-004, DES-010
- Full record: archive/decisions-full.md

## DEC-007 — Rate-limit + retry placement
- Status: accepted (B — Python-side decorator inside worker thread)
- Chose: B — Python-side rate-limit/retry decorator, single chokepoint at the Python boundary, applied by DEC-002's worker.
- Binding:
  1. gc_rate.py module installed alongside the library
  2. Exposes decorator plus retry helper
  3. Worker (DEC-002) imports and applies it to each library method
  4. Tests assert call timing via fake clock
- Dependents: REQ-NF-Perf-002, REQ-NF-Reliab-001, DES-005, DES-012
- Full record: archive/decisions-full.md

## DEC-008 — Testing toolchain
- Status: accepted (A — extend incumbent)
- Chose: A — extend incumbent toolchain: QTest+CTest for C++, pytest+coverage.py for Python.
- Binding:
  1. C++ REQs use QTest macros plus QSignalSpy in unittests/Core/garminconnect/test*.cpp
  2. Python REQs use pytest, freezegun, pytest-cov in src/Python/garminconnect/tests/test_*.py
  3. Coverage: gcov/lcov for C++, coverage.py for Python
  4. CI fails on coverage-delta-drop for changed files only
  5. Tests gated behind GC_WANT_GARMINCONNECT
- Dependents: DES-001, DES-005, DES-006, DES-012, DEC-009, DEC-010
- Full record: archive/decisions-full.md

## DEC-009 — Style/quality toolchain
- Status: accepted (A — clang-format/-tidy plus ruff plus mypy --strict)
- Chose: A — clang-format/-tidy unchanged for C++; add ruff plus mypy --strict scoped to src/Python/garminconnect/.
- Binding:
  1. New pyproject.toml scoped to src/Python/garminconnect/
  2. ruff configured (default plus B plus E/F/I)
  3. mypy --strict on the same path
  4. C++: zero new config, existing clang-format/clang-tidy pick up new files
  5. CI runs the tools on changed files only
  6. Scope does not retrofit legacy src/Python/ chart scripts
- Dependents: DES-005, DES-012, DEC-010
- Full record: archive/decisions-full.md

## DEC-010 — Pre-commit automation
- Status: accepted (A — pre-commit framework, scoped)
- Chose: A — pre-commit framework via .pre-commit-config.yaml, scoped to staged files, same config reused in CI.
- Binding:
  1. Runs on staged files only: clang-format --dry-run --Werror
  2. ruff check --fix plus ruff format
  3. mypy on touched files
  4. detect-secrets
  5. Fast unit-test subset: CTest label garmin-fast plus the Python tests
  6. Onboarding: pre-commit install once
  7. CI runs pre-commit run --all-files for the new paths
  8. Hook does not touch legacy code, scoped via files regex
- Dependents: DES-007, DEC-003
- Full record: archive/decisions-full.md

## DEC-011 — Phase-1 rollout strategy
- Status: accepted (recording-only)
- Chose: New CMake option GC_WANT_GARMINCONNECT, default OFF until A4 passes; mirrors GC_WANT_COACH.
- Binding:
  1. GC_WANT_GARMINCONNECT default OFF until A4 passes
  2. When ON: build adds src/Cloud/GarminConnect.h/.cpp plus Python worker; installer bundles garminconnect plus curl_cffi
  3. When OFF: no Python network deps required
- Dependents: REQ-NF-Build-001, REQ-NF-Pkg-001, DES-007
- Full record: archive/decisions-full.md

## DEC-012 — Auth-dispatcher seam (page ↔ SSO layer)
- Status: accepted (A — IGarminAuthClient interface via ctor)
- Chose: A — inject IGarminAuthClient interface, a compile-enforced DI seam for the credentials/MFA pages.
- Binding:
  1. New file src/Cloud/IGarminAuthClient.h, compiles with GC_WANT_GARMINCONNECT=OFF, header-only
  2. Worker ships WorkerAuthClient : IGarminAuthClient adapter forwarding authenticate(...) to enqueue(Authenticate{...})
  3. Interface gains mfaRequired(QUuid) and captchaDetected(QUuid) when those slices land
  4. No change to DEC-002, DEC-005, or DEC-008
- Dependents: TEST-003, DES-001, DES-003, DES-003a, REQ-003, REQ-015, src/Cloud/IGarminAuthClient.h, unittests/Core/garminconnect/CMakeLists.txt
- Full record: archive/decisions-full.md

## DEC-013 — Worker ↔ Python adapter seam (page-side mirror of DEC-012, one layer down)
- Status: accepted (A — IGarminPyAdapter interface)
- Chose: A — inject IGarminPyAdapter interface, mirroring DEC-012; GarminWorker takes it via constructor.
- Binding:
  1. New src/Cloud/IGarminPyAdapter.h, pure-virtual, header-only, compiles with GC_WANT_GARMINCONNECT=OFF
  2. GarminWorker(IGarminPyAdapter* py, QObject* parent=nullptr); worker no longer owns the sub-interpreter directly
  3. Concrete PyEmbeddedAdapter : IGarminPyAdapter is production; FakePyAdapter drives C++ tests
  4. No change to DEC-002, DEC-004, or DEC-012
- Dependents: TEST-004, DES-001, DES-001a, REQ-002, src/Cloud/IGarminPyAdapter.h, src/Cloud/GarminWorker.h, src/Cloud/GarminWorker.cpp
- Full record: archive/decisions-full.md

## DEC-014 — Token persistence: write-ownership + permission-enforcement seam
- Status: accepted (B — adapter exports blob; C++ owns write)
- Chose: B — adapter exports the token blob via dumps()/loads(); C++ owns the atomic 0600 tmp+fsync+rename write.
- Binding:
  1. DES-002 needs NO amendment: singular tokens.json, atomic, 0600, lib-default-path-never-used all hold
  2. garmin_client.py gains dump_tokens() returning str and load_tokens(token_str: str)
  3. login() stops forwarding a tokenstore path, auth-only in-memory session
  4. PyEmbeddedAdapter stops forwarding tokenstorePath; surfaces the blob post-login via PyAuthOutcome.tokenBlob
  5. load_tokens(blob) on resume replaces password login
  6. REQ-006 load-side mode check gates the single load_tokens() call
  7. garmin-fast stays Python-free; the 2 new pytests run in the Python lane
  8. OQ2: load_tokens() session-expired case routes to REQ-NF-Compat-001(b) with a distinct session_expired error kind
- Dependents: DES-002, DES-006, DES-012, DES-013, REQ-004, REQ-006
- Full record: archive/decisions-full.md

## DEC-015 — Ledger status: single canonical source + absence-check drift lint (LSN-008 promotion-to-mechanism)
- Status: accepted (C — normalize + absence-check lint)
- Chose: C — normalize per-id status to ONE canonical home per id-class, plus a thin absence-check lint backstop; delete local state.md.
- Binding:
  1. Canonical homes: traceability.md for REQ/DES/TEST/VAL status, decisions.md for DEC status
  2. traceability.md DEC-index Status column DROPPED
  3. Appendix artifact tables marked provenance-dated, excluded from status-lint
  4. Local .claude/workflow-garminconnect/state.md DELETED; pointers repoint to root STATE.md
  5. Root STATE.md becomes lean cursor: phase, TEAM, RIGOR, gate/focus, open findings by id, CHANGESET, pointers only
  6. design.md status assertions stripped to design-intent tense
  7. WIKI.md REGISTRIES reduced to ranges plus next only
  8. Lint enforces status-absence only in the non-canonical set: STATE.md, WIKI.md, wiki/*.md, design.md; canonical ledgers fully exempt
  9. Lint excludes archive/, cycles/, validations/, lessons.md, findings.md, and blocks after provenance markers
  10. Runs pre-commit plus as a CLI the CLV step invokes; installed via install_hook.py
  11. Vocabulary: drafted to GREEN to committed to CLOSED, with deferred as an orthogonal tag
  12. Acceptance requires the lint to pass tree-wide before closing LSN-008
- Dependents: traceability.md, decisions.md, design.md, STATE.md, WIKI.md, wiki/conventions.md, scripts/ledger_drift_lint.py, install_hook.py, lessons.md, TEST-017
- Full record: archive/decisions-full.md

## DEC-016 — FIT→TCX fallback-trigger contract for REQ-007 readFile (PRD Assumption-B resolution)
- Status: accepted (C — hybrid kind-aware retry plus sniff backstop)
- Chose: C — hybrid: kind-aware TCX retry plus content-sniff backstop; RateLimited fails fast with no retry.
- Binding:
  1. RateLimited kind: fail fast, no retry, to avoid a retry-storm
  2. Network kind: retry once as TCX
  3. Unknown kind: retry once as TCX
  4. On downloaded bytes: unzip, sniff FIT magic at offset 8; not FIT means retry once as TCX
  5. TCX retry result stages as garmin-<id>.tcx or surfaces failure
  6. Adapter download_activity and the GarminDownloadFailure kind enum stay UNCHANGED
  7. LSN-006 deviation documented: Network kind legitimately conflates a true failure with a 404 no-original case
  8. OQ2, the ZIP-unwrap step, is a build-blocking prerequisite independent of this decision
- Dependents: DES-004, REQ-007, TEST-008, TEST-009, TEST-010
- Full record: archive/decisions-full.md

## DEC-017 — REQ-008 sidecar-persistence component shape
- Status: accepted (A — dedicated GarminSidecarStore)
- Chose: A — dedicated GarminSidecarStore.h/.cpp mirroring GarminTokenStore, independently unit-testable on garmin-fast.
- Binding:
  1. New src/Cloud/GarminSidecarStore.h/.cpp
  2. Must be wired into BOTH the unittest target and the app's GC_WANT_GARMINCONNECT build (LSN-018)
  3. DES-002 gains a realized sidecar-store note owning imported-<uid>.json and backfill-state-<uid>.json
  4. Perm-refusal semantics reuse REQ-006's loadChecked pattern: owner-only, refuse-and-report-path
  5. Torn-write handling: parse failure treated as absent, triggering re-fetch, not a hard error; caller decides
  6. Generalization to a generic JsonSidecar helper deferred until a third consumer justifies it
- Dependents: REQ-008, REQ-010, DES-002, DES-006, DES-008
- Full record: archive/decisions-full.md

## DEC-018 — Where to persist garmin_user_id (the active-account producer for REQ-008 sync)
- Status: accepted (B — separate account-agnostic active-account.json)
- Chose: B — separate garminconnect/active-account.json holding garmin_user_id; tokens.json stays untouched.
- Binding:
  1. New small writer at auth-success persists garmin_user_id to garminconnect/active-account.json via atomic write
  2. GarminConnect::resolveGarminUserId() repointed from tokens.json to active-account.json
  3. Disconnect clears active-account.json alongside tokens.json deletion
  4. Per-account sidecars (imported-<uid>, backfill-state) preserved across Disconnect
  5. tokens.json schema and the REQ-006/REQ-007 read path stay FROZEN, untouched
  6. Write order: tokens.json first, then active-account.json, so a crash leaves at worst a missing pointer
- Dependents: REQ-008, REQ-012, GarminConnect::resolveGarminUserId, GarminConnect.cpp
- Full record: archive/decisions-full.md

## DEC-019 — The production connect/disconnect persist TRIGGER insertion point
- Status: accepted (C — CloudService::disconnect() virtual)
- Chose: C — CloudService::disconnectService() virtual (renamed post-build) plus GarminConnect owns persist/disconnect; wizard-level finished() capture.
- Binding:
  1. CloudService.h gains virtual void disconnectService() {} default no-op, renamed from disconnect() to avoid QObject::disconnect() name-hiding
  2. Symmetric virtual void persistConnectSuccess(uid, blob) {} default no-op added on the base
  3. GarminConnect::persistConnectSuccess wraps GarminTokenStore::persistConnectSuccess(resolveConfigDir(), uid, blob)
  4. GarminConnect::disconnectService() override calls GarminTokenStore::clearAccount(resolveConfigDir())
  5. ONE wizard-level connect() on garminChain client's finished(QUuid,GarminAuthSuccess) catches both direct and post-MFA success
  6. AthletePages.cpp deleteClicked() calls the service's generic disconnectService(), no Garmin string-compare
  7. DES-002 prose corrected to disconnectService() virtual, not removeSettings overridden
  8. Stale-reply dedup and WizardStubPreamble.h's hardcoded /tmp path carried as open questions to the build slice
- Dependents: REQ-008, REQ-012, CloudService.h, GarminConnect.h, GarminConnect.cpp, AddCloudWizard.cpp, src/Gui/AthletePages.cpp, DES-002
- Full record: archive/decisions-full.md

## DEC-020 — Disposition of A3-R012-F1: a live session outliving Disconnect
- Status: accepted (C — fail-closed re-check plus cheap hardening)
- Chose: C — GarminConnect::readFile/readdir re-check that the account is still connected and fail closed if not.
- Binding:
  1. readFile/readdir re-check tokens present; fail closed once the account is not connected
  2. F2: sweep the <path>.tmp siblings in clearAccount
  3. F4: pin the empty-uid write guard
  4. F5: assert mode and mtime, not just bytes, in TEST-055
  5. DEC-019 is NOT reverted; its trigger design stands, C adds a guard on the consuming side
  6. Full lifecycle binding (Option A) deferred to a new follow-on REQ, seeded by F10/F12
  7. A3-R012-F3, F6, F9, F10, F12 stay OPEN and non-blocking
- Dependents: REQ-012, REQ-016, DEC-019, GarminConnect.cpp, TEST-055
- Full record: archive/decisions-full.md

## DEC-021 — The REQ-017 lifecycle-binding mechanism: how Disconnect invalidates a LIVE session
- Status: accepted (B — account-epoch counter latched at open())
- Chose: B — account-epoch counter: latch an epoch at open(), compare in-memory on every readFile/readdir call.
- Binding:
  1. New src/Cloud/GarminAccountEpoch.h/.cpp: static QHash<QString,quint64> s_epoch; current(dir)/bump(dir); no Python.h
  2. Must be added to src/CMakeLists.txt GC_WANT_GARMINCONNECT AND the unittest target (LSN-018)
  3. GarminConnect latches m_openedEpoch plus m_openedUserId in open()
  4. readFile/readdir gate on the epoch compare IN ADDITION to DEC-020's accountStillConnected() for defence in depth
  5. disconnectService() calls GarminAccountEpoch::bump() alongside clearAccount()
  6. readdir/readFile/recordImport consume m_openedUserId instead of re-resolving, fixing F10
  7. REQ-017(c) narrowed to discard-only: cancellation unreachable; a post-download pre-stage epoch recheck runs before recordImport/postReadComplete
  8. DEC-019's fresh-instance choice stays in force, NOT reopened
  9. DEC-020's guard is NOT superseded; it stays as a second layer
  10. Clause (e), the CloudServiceSyncDialog dtor plus MainWindow::syncCloud leak fix, is load-bearing, not parallel
- Dependents: REQ-017, DEC-019, DEC-020, DES-002, DES-014, TEST-060..064
- Full record: archive/decisions-full.md

## DEC-022 — How a refused (fail-closed) readFile reports itself to the sync dialog
- Status: PARTIALLY IMPLEMENTED 2026-08-04
- Chose: A — GarminConnect posts a labelled completion on refusal, still returns false; shared display-side fix left undecided, superseded by DEC-023.
- Binding:
  1. GarminConnect::readFile's fail-closed paths call postReadComplete(data, name, a Garmin-labelled message) then still return false
  2. completedRead's ride-is-NULL branch shows message when non-empty, else falls back to errors.join
  3. Success path (ride not NULL) is untouched, so a completed label can never be mistaken for an error
  4. The roughly 15 other services, which pass an empty message, keep their exact current behaviour
  5. CORRECTION 2026-08-04: the shared-half premise was FALSE, every sibling passes a completed label on success, so the shared change would relabel failures as success; REMOVED, re-decided as DEC-023
  6. GarminConnect half is independently correct and shipped, closing the blocking half of B-R017-06
- Dependents: TEST-065, TEST-066, GarminConnect::readFile, CloudServiceSyncDialog::completedRead, DEC-023
- Full record: archive/decisions-full.md

## DEC-023 — Carry read FAILURE explicitly instead of inferring it from the message
- Status: accepted (option 3, an explicit failure channel)
- Chose: Option 3 — explicit failure channel: add CloudService::readFailed(data, name, reason) signal alongside readComplete.
- Binding:
  1. CloudService gains a readFailed(QByteArray* data, QString name, QString reason) signal
  2. The roughly 15 siblings never emit it and are otherwise untouched
  3. GarminConnect::readFile's non-posting return-false sites emit it with a reason
  4. Sync dialog and CloudServiceAutoDownload both connect it: show reason, delete buffer, advance loop
  5. TEST-065's GarminConnect-half assertions stay valid; only which signal they post on changes
  6. B-R017-11, readFile vs readdir wording drift, is aligned in the same slice
  7. Supersedes DEC-022's shared half
- Dependents: TEST-068, TEST-069, GarminConnect::readFile, CloudService.h, CloudServiceSyncDialog, CloudServiceAutoDownload
- Full record: archive/decisions-full.md

## DEC-024 — Making the modeless sync dialog's self-deletion safe (A3-R017-F1)
- Status: accepted (A — closeEvent() guard plus in-flight flag)
- Chose: A, corrected post-build to gate on QDialog::done(int), the real choke point, instead of closeEvent(); a depth counter, not a bool.
- Binding:
  1. Gate is on done(int), not closeEvent(); close(), reject(), accept(), and Escape all funnel through done()
  2. closeEvent() override kept only as belt-and-braces, not independently load-bearing
  3. blockingCallDepth is a COUNTER, not a bool, because nested blocking frames occur via processEvents()
  4. Ctor guard covers the whole ctor body, not just store->open(), since the ctor also reaches refreshClicked to readdir
  5. cancelClicked() rewritten off its unconditional reject()
  6. A3-R017-F3 folded in: completedRead's abort branch now deletes data like failedRead does
  7. writeFile stays deliberately unguarded as an accepted residual, needing the same guard only if a service ever blocks there
  8. ASan target keeps detect_leaks=0; whole-process leaks are not covered by it
  9. TEST-071 must drive the modal exec() hang case with a five-second watchdog
  10. Removing the done() gate must reproduce an ASan heap-use-after-free
- Dependents: TEST-070, TEST-071, CloudServiceSyncDialog, AddCloudWizard.cpp
- Full record: archive/decisions-full.md

## DEC-025 — Surviving PARENT TEARDOWN: guarding the unsafe operation, not the close (A3-R017b-F1/F4)
- Status: accepted (A — destructor declines to delete the store)
- Chose: A — three-part guard: skip closeAndDeleteStore while busy; QPointer on BlockingCall; QPointer self-check in resuming call sites.
- Binding:
  1. ~CloudServiceSyncDialog: when blockingCallDepth is greater than 0, keep store->disconnect(this) but SKIP closeAndDeleteStore(store), a deliberate leak
  2. BlockingCall holds a QPointer<CloudServiceSyncDialog>; its destructor no-ops when the dialog is already gone
  3. syncNext/downloadNext/refreshClicked take a QPointer to self before each blocking call, returning immediately if null afterward
  4. closeAndDeleteStore stays UNCHANGED; only whether it is called changes
  5. AddCloudWizard.cpp keeps WA_DeleteOnClose; MainWindow.cpp is not touched
  6. A3-R017b-F2: wrap the two writeFile sites in BlockingCall for symmetry if cheap, dropped if it complicates the slice
  7. Fix ships with an ASan test executing parent-teardown mid-nested-loop in both directions
  8. Accepted residual: the store LEAKS on the parent-teardown path, deliberate, the pre-REQ-017 status quo
  9. A deferred reaper was considered and NOT taken, since the app is already shutting down at parent-teardown time
- Dependents: TEST-072, TEST-073, TEST-074, CloudService.cpp, CloudService.h, AddCloudWizard.cpp, MainWindow.cpp
- Full record: archive/decisions-full.md

## DEC-026 — Two-phase init: no nested event loop under construction (closes the BLOCKING A3-R025-F1)
- Status: accepted (B — the constructor builds the widget shell only
- Chose: Split CloudServiceSyncDialog's constructor into a shell-only ctor plus a new start() slot; start() runs store->open(), tab construction, and the initial refreshClicked(). Both callers invoke start() before exec()/open().
- Binding:
  1. Constructor must build widget shell only; no nested event loop runs in the constructor.
  2. New public bool start() on CloudServiceSyncDialog carries store->open(), tab construction, initial refreshClicked(); returns false on open-failure.
  3. start() runs the DEC-025 part-3 QPointer self-bail after each blocking call it makes.
  4. MainWindow::syncCloud must call if (sync.start()) sync.exec(); (MainWindow.cpp:2577-2578).
  5. AddCloudWizard must call if (d->start()) d->open(); else delete d; (AddCloudWizard.cpp:892-899), preserving open-failure cleanup.
  6. DEC-024's done()/closeEvent()/deferCloseIfBusy/BlockingCall and DEC-025's dtor/QPointer guards remain unchanged.
  7. TEST-070..073 must still pass.
  8. Rider: add TEST-076, RED tests making syncNext's and downloadNext's QPointer self-guards load-bearing.
  9. TEST-075 must execute parent teardown during start()'s nested loop under ASan on testGarminConnectSyncDialogClose, both directions (revert must reproduce heap-use-after-free).
  10. Any future third caller of CloudServiceSyncDialog must call start() (ctor produces a non-functional dialog until then).
  11. The deliberate busy-teardown store leak from DEC-025 remains unchanged.
- Dependents: TEST-075, TEST-076, CloudService.cpp:709-974, MainWindow.cpp:2577-2578, AddCloudWizard.cpp:892-899, DEC-024, DEC-025
- Full record: archive/decisions-full.md

---

## DEC-027 — Unify the sync-dialog lifetime: heap + WA_DeleteOnClose for BOTH callers (closes the BLOCKING A3-R026-F1)
- Status: accepted (A — convert MainWindow::syncCloud from a STACK
- Chose: Convert MainWindow::syncCloud to heap + WA_DeleteOnClose + modeless open(), matching AddCloudWizard's pattern; CloudServiceSyncDialog itself is unchanged.
- Binding:
  1. MainWindow::syncCloud must construct the dialog via new, setAttribute(WA_DeleteOnClose), if (sync->start()) sync->open(); with NO else-delete.
  2. Menu-triggered sync becomes modeless (user-approved UX change).
  3. db ownership unchanged: dialog's dtor still closes+deletes db, now triggered by WA_DeleteOnClose deletion.
  4. CloudServiceSyncDialog class itself must remain unchanged.
  5. DEC-024/025/026 machinery and TEST-070..075 must remain unchanged/green.
  6. Rider (A3-R026-F2): extend the ASan fixture so the 4 unreachable start() self-bails become testable (open-failure BlockingStore::open() mode, non-empty dirty rideCache, teardown armed during readdir/refreshClicked) as TEST-077, each guard RED-verified.
  7. TEST-077's four slots must each FAIL when their guard is neutered.
  8. Baseline: ASan target green; ctest -R Garmin|AtomicFile >=25, full >=26, GoldenCheetah links.
  9. A3-R026-F3 (no start()-called/re-entrancy invariant) stays a latent advisory, inert with the two known callers.
- Dependents: TEST-077, TEST-078, MainWindow.cpp:2571-2585, AddCloudWizard.cpp:892-910, DEC-024, DEC-025, DEC-026
- Full record: archive/decisions-full.md

---

## DEC-028 — Clean-checkout build repair: commit the missing wiring rather than guard around it (ORCH-001)
- Status: accepted (B — commit unittests/Core/coach/CMakeLists.txt
- Chose: Commit unittests/Core/coach/CMakeLists.txt plus stubs/ (6 files) so committed CMake references only committed files, fixing the clean-checkout build.
- Binding:
  1. unittests/Core/coach/CMakeLists.txt and stubs/*.h (6 files) become tracked.
  2. unittests/CMakeLists.txt stays unchanged — no EXISTS guard added.
  3. WIKI MAP unittests/ line must be updated.
  4. Cause 1 (8 phantom refs in src/CMakeLists.txt: DiaryWindow/Velohero/DiarySidebar) fixed mechanically.
  5. ORCH-005 (translations/lrelease), ORCH-006 (11 tracked sources absent from CMake lists), ORCH-007 (CMAKE_CXX_EXTENSIONS OFF breaking the QBluetoothUuid link) all fixed in the same commit.
  6. CMAKE_CXX_EXTENSIONS set ON project-wide (converges on the qmake reference build).
  7. The uncommitted src/Train/KurtInRide.cpp workaround is now redundant and should be dropped by its owner, not committed.
  8. .qm files generated into the source translations dir (matches qmake TS_DIR), stay gitignored.
  9. All other pre-session Coach/libusb/Calendar work in both CMakeLists remains uncommitted and untouched.
  10. Verification must use a clean extract of the staged tree (git archive of the index), never the working tree.
- Dependents: commit 427da745b, ORCH-005, ORCH-006, ORCH-007, branch garmin/req017-lifecycle-uaf
- Full record: archive/decisions-full.md

---

## DEC-029 — Upload-dialog UAF fix shape: two-phase init + heap/WA_DeleteOnClose, MODAL preserved (REQ-019)
- Status: accepted (B — two-phase start() + heap + WA_DeleteOnClose
- Chose: Two-phase start() plus heap/WA_DeleteOnClose for CloudServiceUploadDialog; modal exec() and the caller's store ownership are preserved.
- Binding:
  1. Ctor becomes widget shell only; new bool CloudServiceUploadDialog::start() carries store->open() onward, with a QPointer self-bail after each of the four blocking calls.
  2. Construction site becomes heap + WA_DeleteOnClose + if (start()) exec();.
  3. The conversion lands in CloudService::upload (CloudService.cpp:78-87), not MainWindow::uploadCloud.
  4. DEC-024's close-gate is deliberately NOT added to the upload dialog (no user-close route exists during the blocking work).
  5. closeAndDeleteStore(db) at MainWindow.cpp:2563 stays exactly where it is; store ownership unchanged.
  6. FIRST RED test must prove QDialog::exec() self-protects when this is destroyed mid-loop; if it does not hold, Option B is falsified in part and must be reopened rather than patched around.
  7. New test slots go into the existing ASan target testGarminConnectSyncDialogClose / unittests/Core/garminconnect/ (registered as ORCH-008 debt).
  8. GarminConnect is NOT among the 11 Upload-capable services affected (read-only, DEC-005).
  9. Under Option B the only observable change for all 11 services is stack to heap allocation — still modal, still blocking, no UX delta.
- Dependents: REQ-019, TEST-079, TEST-080, ORCH-008, REQ-020, DEC-024, DEC-025, DEC-026, DEC-027
- Full record: archive/decisions-full.md

---

## DEC-030 — Collaborator-lifetime UAF fix shape: reparent to AthleteTab, PROBE-FIRST, with QPointer guards as a rider (REQ-021)
- Status: accepted (B — reparent both dialogs to context->tab
- Chose: Reparent both dialogs to context->tab, conditional on an executed Qt visibility probe; Option A's QPointer collaborator guards ship alongside as a mandatory rider.
- Binding:
  1. GATING PROBE required before any production line: build the harness first, run a ~20-line offscreen probe (parent a QDialog to a child QWidget, open() it, hide the parent, assert isVisible()).
  2. If child windows follow the parent's hide, Option B is dead on UX grounds and the fix falls back to Option A applied to ALL ELEVEN sites (pre-authorised, no second decision round).
  3. If the probe passes: CloudService::upload (:95) passes context->tab instead of parent; the sync ctor (:825) becomes QDialog(context->tab, Qt::Dialog).
  4. Rider ships regardless of probe outcome: collaborator QPointer guards (self/ctx/ride null checks) added in both start() methods.
  5. S-R021-01 folded in: add the missing if (self.isNull()) return false; between CloudService.cpp:1107 and :1108.
  6. Build a purpose-built FakeAthleteWindow test class (not in stubs) modeling MainWindow.cpp:2183-2185 and closeEvent (:1102-1103); both single-tab-close and whole-window-close routes must be drivable.
  7. stubs/ImportSeamStubs.cpp: Context::metadataFlush() and RideItem::notifyRideMetadataChanged() must each load a real member; initialise RideItem::isdirty (:297).
  8. Test home: existing ASan target testGarminConnectSyncDialogClose, extended — no new target, no new CMake wiring.
  9. ORCH-008 naming debt grows by one more non-Garmin subject — flagged, deliberately not acted on inside this REQ.
  10. Does NOT close A3-R019-F5 (7 of 9 slots hand-copy CloudService::upload) — stays open.
- Dependents: REQ-021, TEST-081..084, REQ-020, REQ-022, REQ-023, ORCH-008, A3-R019-F3, B-R019-04, B-R019-05, S-R021-01
- Full record: archive/decisions-full.md

---

## DEC-031 — Reopening DEC-025: a frame-counted DEFERRED REAPER replaces the deliberate store leak (REQ-021)
- Status: accepted (B — frame-counted deferred reaper, conditional on
- Chose: A frame-counted deferred reaper replaces DEC-025's deliberate store leak, conditional on an executed Qt loop-level probe of deleteLater/queued invokeMethod inside a nested loop.
- Binding:
  1. REOPENS DEC-025 (not amended) — its no-live-event-loop rationale was falsified by DEC-030's reparent.
  2. GATING PROBE required before any production line: a ~20-line offscreen probe calling deleteLater() and a queued invokeMethod on an object from inside a nested QEventLoop, asserting whether the object dies before the loop returns; ship the probe's positive control in the same slot.
  3. The decline branch hands (store, depth) to a small refcounted orphan record instead of dropping it; ~BlockingCall releases one frame on its dialog.isNull() stand-down path; the last release runs closeAndDeleteStore.
  4. Change confined entirely to CloudService.h/.cpp — zero edits to any of the 17 subclasses, zero call-site edits, zero user-visible change.
  5. The orphan record must be per-dialog and refcounted, created by the first BlockingCall — a single static counter is wrong.
  6. A naive store->deleteLater() or a queued invokeMethod posted from inside the store's own suspended loop must NOT be used — it would delete the store under its own frame.
  7. DEC-024's blockingCallDepth invariant text must be narrowed to: every store call the dialog makes that can run a nested loop is wrapped — it says nothing about loops the store enters on its own.
  8. TEST-084's assertions must INVERT (closed+deleted exactly once, after the nested loop returns); TEST-072 inverts identically; TEST-073 stays unchanged as the discriminator.
  9. REQ-021 cannot close while a test in its own changeset still asserts the overturned leak behaviour.
  10. New RED direction required: dropping the release() call must make TEST-072/084 fail on a positive assertion — leak coverage must not depend on detect_leaks, currently disabled on this target.
- Dependents: REQ-021, TEST-084, TEST-072, TEST-073, REQ-024, DEC-024, DEC-025, DEC-030
- Full record: archive/decisions-full.md

---

## DEC-032 — Closing the silent stall: an in-loop `continue` in the branch that arms nothing (REQ-027)
- Status: accepted (A — in-loop continue, adopting uploadNext's
- Chose: In-loop continue in syncNext's parse-failure branch, converging with uploadNext's existing shape, plus an S-R027-01 abort re-read and a batch-generation rider.
- Binding:
  1. syncNext's upload-side parse-failure branch (:2067-2071) gets an in-loop continue instead of falling through to return true, matching uploadNext's shape.
  2. S-R027-01: add an aborted re-read at each site between openRideFile's nested loop and compressRide/writeFile, in both syncNext and uploadNext.
  3. S-R027-04: advance the progress bar in the branch that already continues.
  4. Batch-generation rider: add an int batchGeneration member, incremented in downloadClicked's start branch, snapshotted at loop entry, compared before each continue (closes the existing uploadNext instance too).
  5. O-R027-01 is NOT closed by this DEC (superseded claim) — routed to DEC-033; only B-R026-01, S-R027-01, S-R027-04 close here.
  6. B-R025-03 (four uncounted processEvents()) stays its own queued decision, out of scope.
  7. No completion-tail string change ("N failed") — visibility rests on the per-row status cell, the progress bar, and successful < downloadtotal.
  8. AMENDMENT: scope item 3 (readFile bool contract) is removed entirely from DEC-032, routed to DEC-033, together with B-R027-02 (buffer leak) and B-R027-03 (fixture honesty).
  9. B-R027-04 rider: uploadNext's parse-failure branch gets the same ++downloadcounter as syncNext's.
  10. B-R027-05 rider: criterion (a)'s downloading==false check is replaced by tail-exclusive proxies (progressLabel, downloadButton, checkboxes), recorded as a proxy.
- Dependents: TEST-095..100, DEC-033, REQ-027, B-R025-03
- Full record: archive/decisions-full.md

---

## DEC-033 — The `readFile` bool contract: `false` does not mean "armed nothing" (REQ-027 (e))
- Status: accepted (Option A — widen the contract via a defaulted
- Chose: Widen readFile's contract with a defaulted out-param (ArmedNothing/ArmedCompletion enum) so callers can distinguish a truly silent refusal from one that already queued a completion.
- Binding:
  1. Base virtual readFile (CloudService.h:263-265) gains a defaulted out-param, enum ArmedNothing or ArmedCompletion (not a narrow bool).
  2. 10 non-GarminConnect overrides get signature-only edits, default value, no logic change.
  3. GarminConnect.cpp's 7 "armed" return-false sites (:474,493,519,540,557,572,588) set ArmedCompletion; :452 stays ArmedNothing (default).
  4. syncNext/downloadNext must read the out-param instead of discarding the bool: on ArmedNothing, label+advance+continue and delete data (closes B-R027-02 leak); on ArmedCompletion, do nothing.
  5. Zero existing TEST assertions may change value (TEST-020..026, TEST-065(a)/(b), all 6 driveRefusal() sites).
  6. The fake store fixture must implement TWO distinguishable modes: Mode 1 (ArmedNothing, returns false, emits nothing) and Mode 2 (ArmedCompletion, returns false AFTER queueing a real completion via Qt::QueuedConnection) — building Mode 2 as "false + emits nothing" is forbidden.
  7. CloudServiceAutoDownload::run is confirmed out of scope — it blocks on both signals with its own watchdog and never branches on the bool.
  8. Rejected: Option B (flip GarminConnect's 7 sites to return true) inverts a deliberately-designed test doctrine; Option C (deferred watchdog timer) cannot close B-R027-01 standing alone.
- Dependents: REQ-027(e), B-R027-01, B-R027-02, B-R027-03, DEC-032, CloudService.h:263-265, GarminConnect.cpp
- Full record: archive/decisions-full.md

---

## DEC-035 — Where the abort guard lives: co-locate it with the irreversible call (REQ-027)
- Status: accepted (B — guard the irreversible call)
- Chose: Co-locate the abort-read with the irreversible call in syncNext and downloadNext, converging on DEC-032's shape rather than a new idiom.
- Binding:
  1. Add an abort read immediately before the irreversible call in syncNext (:2011, before readFile) — no read exists there today.
  2. Add an abort read immediately before the irreversible call in downloadNext (:2225) — downloadNext currently has ZERO abort reads in the function.
  3. Additive only — does not replace DEC-032's continue/batchGeneration logic, and does not touch DEC-030/031/025's lifetime machinery or REQ-026's re-entrant abort logic.
  4. Requires TEST-105 and TEST-106, mutation-proof, so the guards cannot be mistaken for redundant with the nearby completion-slot checks.
  5. Does NOT stop an abort delivered a microsecond later and does NOT close the processEvents delivery gap — must not be claimed as more than locality.
  6. Interrupting the in-flight request itself (a cancel-capable contract across ~16 siblings) is explicitly out of scope, a deliberate omission.
  7. The scout's proposed REQ-027(d) rewording was checked and NOT made; a new clause (f) is added for the newly-guarded download path.
- Dependents: REQ-027(d)(f), TEST-105, TEST-106, DEC-032, DEC-030, DEC-031, DEC-025
- Full record: archive/decisions-full.md

---

## DEC-034 — Who owns a row across a nested event loop: make the REBUILD observable (REQ-028)
- Status: accepted (C — list generation counter)
- Chose: Add a list-generation counter (Option C) so drivers can detect that refreshClicked rebuilt the tree underneath a suspended frame, validating the container rather than the element.
- Binding:
  1. Add a listGeneration-style member incremented whenever refreshClicked rebuilds the tree; six compares added across the file.
  2. No new suspension point, no new tr() string, no subclass API change; guards placed above the irreversible writes per DEC-035.
  3. refreshClicked must still run and still enter readdir, still nest the frame — TEST-091's geometry must be preserved, not refused.
  4. Does NOT close S-R028-01 (the sort route/reordering) — tracked as an open finding, not folded in.
  5. Validates the CONTAINER only, not the ELEMENT — sound only while whole-list rebuild is the sole way a row dies (:1532/:1539/:1544 are the only tree-item deletes today).
  6. Build must run probe-first on QTreeWidgetItem::child(int) out-of-range behaviour (unspecified in Qt docs) before assuming a guard shape.
  7. Suspension set is now: this (QPointer, DEC-025), context (QPointer, DEC-030), store (StoreReaper, DEC-031), intent (aborted, DEC-032/035), batch (batchGeneration, DEC-032), LIST (listGeneration, this DEC).
- Dependents: REQ-028, TEST-107, TEST-108, TEST-109, TEST-110, S-R028-01, DEC-032, DEC-031, DEC-035, DEC-030, DEC-025
- Full record: archive/decisions-full.md

---

## DEC-036 — Correlating a completion with the transfer that asked for it: the in-flight ticket (REQ-028 clause (c))
- Status: accepted and built on formal reopen (C — explicit operation
- Chose: Original in-flight ticket correlated completions by transfer identity; formally reopened 2026-08-22 and superseded on the write side by explicit write-operation identity (Option C) after two writes were measured outstanding at once.
- Binding:
  1. FORMAL REOPEN (2026-08-22): the write channel uses Option C — an opaque operation id allocated at every write dispatch, carried through writeFile, returned with writeComplete; the dialog's write records are keyed by that id.
  2. The read channel keeps its existing buffer-pointer identity — no API migration needed there.
  3. Refresh makes an outstanding write record row-free but preserves its id until completion; a late result must be consumed without touching a destroyed row.
  4. No correctness-degrading eviction cap is permitted — records live until completion or dialog destruction.
  5. Common requirement under every option: snapshot batchGeneration when each completion is admitted, re-check it after every suspension and before using a saved row, counter, or re-drive.
  6. Supersedes DEC-037's receiver-side name matching, cap, and accepted result-swap residual.
  7. TEST-124 must require exact result-to-producing-row association in both arrival orders.
  8. Lifecycle rule (from the 2026-08-18 invalidation amendment, still binding): any state ARMED at N sites and CONSUMED at M sites must have its INVALIDATION sites enumerated in the same entry.
  9. Contract to write down: readComplete must return the pointer readFile was given (add beside the existing readFailed contract at CloudService.h:145).
  10. REQ-028 clause (c) is CLOSED, not partial, as of DEC-038 landing the sort slice.
- Dependents: REQ-028(c), TEST-107, TEST-111, TEST-112, TEST-113, TEST-114, TEST-115, TEST-116, TEST-124, DEC-034, DEC-035, DEC-037, DEC-038, B-R028-04
- Full record: archive/decisions-full.md

## DEC-037 — Separating an abandoned write from the live one when the wire carries no identity (REQ-028 (c), A3-R028b-F3)
- Status: superseded by DEC-036's built and verified 2026-08-22
- Chose: Superseded. Originally chose a retired-ticket set to separate abandoned writes from live ones by name when the wire carries no identity; later fully superseded by DEC-036's explicit operation-id mechanism.
- Binding:
  1. STATUS: superseded — the retired-name/capped-ticket mechanism is NOT production behavior; DEC-036's Option C (opaque operation id) is current.
  2. Scope fact retained by later decisions: the READ channel already has a unique per-transfer token (buffer pointer) and needed no fix — only WRITES were ambiguous (user decision 2026-08-19).
  3. AMENDMENT 2026-08-21: refreshClicked must retire (not merely clear) an outstanding write's identity as a row-free tombstone — naively appending to inflight at :1559 would re-manufacture a use-after-free; superseded by DEC-036's row-free record mechanism.
  4. Generalised lesson retained: an invalidation site reasoned purely on memory safety can still be the defect — cost analysis must also be checked.
  5. Original mechanism (superseded): a retiredWrites list in CloudService.h; the START branch retired an armed WRITE ticket before disarming; refreshClicked cleared retiredWrites; a completion scanned retiredWrites oldest-first before the live compare.
  6. Accepted residual (historical, no longer applicable): in the [live, stale] arrival order the two result strings could land on opposite rows — cosmetic and bounded.
- Dependents: DEC-036 (successor), TEST-124, TEST-125, TEST-115
- Full record: archive/decisions-full.md

---

## DEC-034 / DEC-036 AMENDMENT 2026-08-21 (A3-R028c-F2, BLOCKING) — the completion slots never got the guard the drivers have
- Status: accepted; folded into DEC-036's Option C build
- Chose: Extend DEC-034's batchGeneration guard (already on the drivers) to the three completion slots (completedRead, failedRead, completedWrite) that never received it, since a downloadClicked delivered into their processEvents() suspension can re-drive or double-count.
- Binding:
  1. All three completion slots (completedRead, failedRead, completedWrite) must take a batchGeneration snapshot at slot ENTRY and compare it before the tail's re-drive.
  2. In scope: all three slots; the shape is line-identical on the read path (:2986-3016) and failedRead (:3103), both on the GarminConnect route — the write path is not on that route.
  3. Also required: an aborted/batchGeneration re-read between saveRide (:2966) and successful++ (:2978), so an abort+restart delivered inside autoProcess cannot increment the new batch's counter on the old batch's behalf.
  4. Not a new decision — reuses DEC-034's existing mechanism, applied to the sites that never received it.
- Dependents: DEC-034, DEC-036, commit e48f7d123, gate 98df50535
- Full record: archive/decisions-full.md

---

## DEC-038 = DEC-034 AMENDMENT 2026-08-22 (S-R028-01 / S-R028-02) — the sort route: sorting is OFF for the batch's duration
- Status: accepted (A — sorting disabled for the batch's duration)
- Chose: Disable sorting on all three lists for the batch's duration (Option A), restoring the user's sort column/order only at true batch termination.
- Binding:
  1. Sorting must be disabled BEFORE the batch's first setText — placement, not mere existence, is load-bearing.
  2. Disable must be restored on EVERY termination path, not only the three completion tails and the abort branch — the builder must re-derive the full census of early-return sites by grepping, not assume this entry's list is complete.
  3. A batch is NOT one stack frame: an RAII scope guard around a single driver call is forbidden — it would re-enable sorting between rows. The disable must persist across the whole batch, released only at true batch termination.
  4. On self.isNull() exits: the dialog is already gone — the restore must be structured so it cannot touch a dead dialog.
  5. The restore must preserve the user's pre-batch selected sort column and order (setSortingEnabled(true) alone defaults to section 0, descending).
  6. CloudService.h:733-742 and :795-803's production comments stating the sort route is not closed must be rewritten.
  7. DEC-036/DEC-037 ticket mechanisms are untouched — this amendment lives only in the sorting toggles and driver exits.
  8. TEST-131's Q5c pins are measured-not-desired and must go RED when this fix lands, and the block must say so; TEST-112 stays as the historical pre-fix measurement.
  9. Accepted residual: a future caller that re-enables sorting mid-batch reintroduces the defect with no architectural guard against it.
- Dependents: S-R028-01, S-R028-02, DEC-034, DEC-036, TEST-112, TEST-131, commit 514d8e88f
- Full record: archive/decisions-full.md

---

## DEC-039 — libusb dependency wiring: complete find_path/find_library for BOTH APIs, on an isolated unmerged branch (ORCH-036)
- Status: accepted (B — complete find_path/find_library include+link
- Chose: Complete find_path/find_library include and link wiring for both libusb-1.0 and legacy libusb-0.1, built on an isolated, deliberately unmerged branch.
- Binding:
  1. Implementation lives on branch build/orch036-libusb-wiring, in a separate worktree from 514d8e88f; the main worktree and the cleanhead evidence worktree must NOT be modified.
  2. Must NOT be merged or cherry-picked until the existing owner of the uncommitted src/CMakeLists.txt changes has reconciled the overlap.
  3. find_path/find_library wiring is required for BOTH libusb-1.0 and legacy libusb-0.1 branches, include dirs AND link libraries.
  4. A missing dependency must fail at CONFIGURE time, not several minutes into a link.
  5. GC_LIBUSB_* variables must be set inside the if(GC_HAVE_LIBUSB) block and consumed under the same guard in the link section.
  6. ORCH-036 stays ADVISORY and OPEN until a reconciled change lands.
  7. Accepted residuals, explicitly unexecuted: legacy libusb-0.1 successful build, macOS/Homebrew, Windows/vcpkg — none executed, must not be described as multi-platform testing.
- Dependents: ORCH-036, branch build/orch036-libusb-wiring, DEC-028
- Full record: archive/decisions-full.md

---

## DEC-040 — Bounding every blocking provider wait: a shared bounded-request outcome contract, then generic cooperative cancellation (W3), on an S-1 base-owned injectable QNetworkAccessManager
- Status: accepted 2026-08-26. NOT BUILT.
- Chose: Adopt W3 (W2 then cooperative cancellation) as one decision and release unit: a shared blockingRequest outcome contract migrated to all providers, then generic CancelToken-based cancellation; base CloudService becomes sole owner of an injectable QNetworkAccessManager (S-1).
- Binding:
  1. Sequenced as ONE decision/release unit: Stage 1 (outcome contract + migration) then Stage 2 (cancellation) — Stage 1 alone does NOT unblock the parked auto-downloader teardown draft.
  2. RequestResult CloudService::blockingRequest(QNetworkReply*, int timeoutMs, const CancelToken& cancel = CancelToken()); ownership of the reply is transferred.
  3. Strict ordering: reconcile outcome, then snapshot fields, then dispose; abort() is never called before the outcome is fixed; reply->isFinished() is NEVER consulted for outcome — only naturalFinish_ is.
  4. Disposal exactly once on every path: snapshot fields, then abortIssued_ = true; abort if not finished; deleteLater(); RAII-guarded.
  5. Cancellation: kCancelPollMs = 250; maximum cancellation-observation interval no more than 300 ms.
  6. CancelToken is plumbed as a MEMBER (setCancelToken), never as a parameter — open()/readdir() virtual signatures stay unchanged; CloudService.h carries no include, forward declaration, or friendship of CloudServiceAutoDownload.
  7. S-1: base becomes sole owner of the manager, not a virtual factory — explicit constructor injection or default construction only.
  8. All ten "if (context) delete nam;" bodies are REMOVED; base owns and destroys the manager exactly once.
  9. Timeout override and manager injection must compile in the SAME production code version under test — no test-only preprocessor branch.
  10. Dropbox's absence of an onSslErrors slot must be PRESERVED; the sslErrors connect becomes unconditional rather than if(context)-guarded.
  11. Timeout policy: generic 30s open/auth, 60s listing; SixCycle KEEPS its provider-specific 5s open / 10s readdir.
  12. This DEC id belongs ONLY to the provider-watchdog decision — the parked auto-downloader teardown draft carries NO id and must never be cited as DEC-040.
- Dependents: DES-001 invariant 3, T-143..T-151, S-R028f-F1, S-R028f-F3, C1..C6, ORCH-036, ORCH-042, S-R028f-F2
- Full record: archive/decisions-full.md

---

## DEC-040 STAGE-1 AMENDMENT & STATUS — 2026-08-30 (reconciled against repository truth)
- Status: Stage 1 is BUILT, its Verification Gate PASSED, and it is
- Chose: Reconciles Stage 1 as-built against the accepted DEC-040 text: lazy null-or-valid manager construction (not eager), delete nam is not zero, no cancellation exists yet, and blockingRequest carries no CancelToken parameter.
- Binding:
  1. Stage 1 is BUILT, Verification Gate PASSED, COMMITTED as 37710370e on branch garmin/req028-row-lifetime — NOT pushed, NOT merged.
  2. DEC-040 is NOT COMPLETE: Stage 2 (cancellation) is UNBUILT; a green Stage-1 gate does NOT discharge C1..C6 and does NOT release the parked auto-downloader teardown draft.
  3. 22 migrated bounded-request sites across TEN providers (Azum 3, Dropbox 2, PolarFlow 1, CyclingAnalytics 1, Xert 3, Strava 3, SportTracks 2, TrainingsTageBuch 2, Nolio 3, SixCycle 2).
  4. THIRTEEN providers participate in base manager ownership; RideWithGPS/Selfloops/SportsPlusHealth are manager-only siblings with NO watchdog rows, appearing only in T-153.
  5. S-1 AMENDED to LAZY, null-or-valid: no manager exists before QCoreApplication; first real use creates exactly one, in the provider's own affinity thread; nam() refuses to initialise off-affinity.
  6. An injected manager is adopted EXACTLY AS SUPPLIED, conditional on affinity — no cross-thread setParent(), no relocation, no default built alongside it.
  7. Provider sslErrors wiring runs EXACTLY ONCE, when the manager first comes into being.
  8. ACCEPTANCE CONDITION CHANGED: delete nam count is 5, not 0 — all five legitimately in OpenData.cpp deleting local managers, outside this slice; migrated providers keep the idiom only in comments.
  9. Stage 1 has NO CancelToken type, no cancellation poll, no reachable Cancelled outcome.
  10. Final blockingRequest signature is unchanged with NO token parameter; cancellation is member-plumbed via setCancelToken in Stage 2.
  11. T-148/T-149 are ALLOCATED-UNBUILT — no CancelToken exists in Stage 1, neither is drivable.
  12. Alignment probes must count live statements only, excluding comments (P1-P6 corrected to be comment-blind).
- Dependents: DEC-040, commit 37710370e, branch garmin/req028-row-lifetime, T-143..T-153
- Full record: archive/decisions-full.md

---

## DEC-041 — File-IO layer Context UAF at RideFile.cpp:999: hoist-and-capture, not guard-and-decline
- Status: accepted (B — hoist-and-capture)
- Chose: Hoist the Context-dependent read (the Athlete tag) to before the suspending file-format reader call, rather than guard-and-decline at the deref point; later amended with a supplementary liveness bail for a second, unhoistable call.
- Binding:
  1. Move the context->athlete->cyclist tag-set (RideFile.cpp:999) to BEFORE reader->openRideFile(...) — eliminate the race, don't detect it.
  2. Acceptance test must be stronger than a guard-and-decline fix: it must assert the Athlete tag is correctly set even when the Context dies mid-parse.
  3. REQ scope: allocate new REQ-029 (file-IO axis); do not fold into REQ-023, which stays scoped to the cloud STORE layer.
  4. B-R021-10 is reconciled separately (already discharged by commit 37710370e/TEST-091/TEST-092) — its own finding disposition, not folded into DEC-041.
  5. AMENDMENT 2026-09-08: census corrected — openRideFile spans :847-:1133 with FOUR Context touches (:906, :928, :999, :1055), not two.
  6. :1055 (result->recalculateDerivedSeries()) CANNOT be hoisted — it needs the parsed ride, and no return exists between :999 and :1055.
  7. Amended mechanism: add a supplementary QPointer<Context> liveness bail captured at factory entry, consulted before the :1055 call; on a dead Context, skip the derived-series recalculation.
  8. The :999 hoist stands exactly as originally decided — the amendment is additive only.
  9. The test target must assert BOTH: no fault at either tail site with a mid-loop Context death, and tag plus derived series correct when the Context survives.
- Dependents: REQ-029, DEC-030, RideFile.cpp
- Full record: archive/decisions-full.md

---

## DEC-042 — `saveRide`'s post-`autoProcess` member dereferences vs. parent-teardown UAF: guard the proven hazard site, not the call site
- Status: accepted (A — in-function QPointer self-bail after saveRide
- Chose: In-function QPointer self-bail placed after saveRide's own second autoProcess call, the proven hazard site, rather than at the completedRead call site.
- Binding:
  1. Hold QPointer<CloudServiceSyncDialog> self(this) before the first autoProcess call in saveRide; return false immediately if self.isNull() after the second autoProcess call.
  2. One check suffices — the only statement between the two calls, ride->recalculateDerivedSeries(), touches ride, not this.
  3. Accepted trade-off: on a rare teardown-mid-autoProcess path the ride's JSON file is never written at all.
  4. The new ASan test slot must arm gcstub::autoProcessAction to deliver a close from inside the second autoProcess call; assert no ASan report AND that saveRide returns without touching context/rideFiles.
  5. Reverting the guard must reproduce the exact heap-use-after-free stack the T-141 probe already captured.
  6. Closes B-R028-17.
  7. Build evidence required: RED via mutation reproducing the exact ASan UAF; GREEN 99/99 on both offscreen/minimal backends; full ctest -L garmin-fast 27/27.
- Dependents: B-R028-17, TEST-158, DEC-025, CloudService.cpp, testGarminConnectSyncDialogClose.cpp
- Full record: archive/decisions-full.md

---

## DEC-043 — `CloudServiceAutoDownload` cross-thread lifetime UAF: guard the completion slots AND cooperatively cancel-and-wait the thread at teardown
- Status: accepted (C — Option A's guards plus Option B's cooperative
- Chose: Combine Option A (lifetime guards in the completion slots) with Option B (cooperative cancel plus bounded wait() at teardown) so both the queued-delivery race and the worker-thread ownership gap close together.
- Binding:
  1. Add a requestStop() API on CloudServiceAutoDownload; call setCancelToken in run()'s worklist loop.
  2. Athlete::close()/~Athlete() teardown must call wait() on the thread BEFORE delete athlete/delete context — this ordering must hold.
  3. The built guard landed in readComplete ONLY (readFailed touches no context/athlete state): a stopRequested_ acquire-load check at the head, freeing the buffer and returning.
  4. The cloudAutoDownload leak fix (delete cloudAutoDownload in ~Athlete()) is safe ONLY once wait() guarantees run() has exited — must not be added before the wait() mechanism exists.
  5. ORCH-057 (run()'s own worker-thread read of context->athlete->rideCache->rides()) stays EXPLICITLY OUT OF SCOPE — tracked separately, not fixed here.
  6. Deleting either mechanism (guard or cancel+wait) later silently reopens the hole the other was covering — an explicit comment fence is required.
  7. The roughly 250ms-per-remaining-call bound on athlete-tab-close latency from wait() is accepted as-is, no stricter SLA required.
  8. Two independent new test categories required: a pure-slot unit test for the guard, and an Athlete/Context-fixture-driven teardown-ordering test for cancel+wait.
- Dependents: A3-R028e-F1, TEST-159, TEST-160, DEC-040, Athlete::close(), ORCH-057, ORCH-059
- Full record: archive/decisions-full.md

---

## DEC-044 — `RideFile::appendOrUpdatePoint` reads a deleted point: alias the survivor, don't detect the fault
- Status: accepted (A — alias the surviving point)
- Chose: Alias the surviving point with a new local pointing at the retained dataPoints_ slot, instead of reading the freed point after delete in updateMin/Max/Avg.
- Binding:
  1. Add a local RideFilePoint *newest = point;, reassigned to dataPoints_.at(idx) on the duplicate-timestamp (delete) branch.
  2. The three tail calls (updateMin, updateMax, updateAvg at :1709-1711) must take the alias, not point.
  3. No other call site, header, or signature changes are permitted — the fix is confined to inside appendOrUpdatePoint.
  4. Must be fixed before resuming REQ-029/DEC-041's build — T-173 cannot exercise its own intended assertions while this unrelated fault aborts the process first.
  5. Rejected: triplicating the update-call block per branch, and restructuring delete-ownership/control-flow — both add maintenance or regression surface beyond what the fault requires.
- Dependents: REQ-030, B-R029-01, REQ-029, DEC-041, T-173, RideFile.cpp
- Full record: archive/decisions-full.md

---

## DEC-045 — REQ-014 error-translation locus + keying: confirm DES-008's page-layer design, key on `GarminAuthFailure::Kind` not the raw exception class name
- Status: accepted (recording-only — the builder already implemented
- Chose: Confirm DES-008's page-layer translation locus; key GarminErrors::translate() on GarminAuthFailure::Kind (Auth/Network/RateLimit/Unknown), not the raw Python exception class name DES-008's sample used.
- Binding:
  1. Worker/adapter (GarminWorker.cpp) NEVER translates; GarminCredentialsPage/GarminMfaPage are the ONLY call sites for GarminErrors::translate().
  2. PyAuthOutcome::rawMessage and GarminAuthFailure::translatedMessage carry the raw library message verbatim, per IGarminPyAdapter.h.
  3. GarminErrors::translate() takes a GarminAuthFailure::Kind parameter, not a raw exception-class-name string — a deliberate deviation from DES-008's literal sample.
  4. testGarminConnectAuthClient.cpp is corrected to assert raw passthrough, matching the doc, not translated text.
  5. This entry documents an already-committed resolution (ac1fa40ba) — no further build action required.
- Dependents: REQ-014, GarminErrors.h, GarminCredentialsPage.cpp, GarminMfaPage.cpp, DES-008
- Full record: archive/decisions-full.md

---

## DEC-046 — REQ-015 CAPTCHA detection: defer, no structured signal survives in the real dependency
- Status: accepted (user decision 2026-09-10, option 2 of 3 presented)
- Chose: Defer REQ-015 entirely — do not build CAPTCHA detection and do not add message-substring matching as a workaround, since no structured signal survives to the adapter's call site in garminconnect==0.3.13.
- Binding:
  1. Do NOT build CAPTCHA detection now.
  2. Do NOT add message-substring or text matching to work around the gap — this would violate LSN-006.
  3. A CAPTCHA-triggered login continues to fall through as today's generic Unknown/Auth failure (REQ-014's existing translation) until the dependency changes upstream or a separately-scoped dependency-level fix is decided and built.
  4. Revisit only if garminconnect ships a fix upstream, or a dependency-level fork/patch fix is separately proposed and scoped.
  5. traceability.md's REQ-015 row must cite this DEC and record the research conclusion in place of the prior "not started" note.
- Dependents: REQ-015, LSN-006, REQ-014, REQ-NF-Pkg-001
- Full record: archive/decisions-full.md

## DEC-047 — REQ-010/DES-009 pagination model: per-activity pacing/checkpointing, not per-page, against the real `garminconnect` dependency
- Status: accepted (recording-only)
- Chose: Per-activity pacing/checkpointing, not per-page, matching what the real garminconnect dependency exposes (no page-cursor seam).
- Binding:
  1. Pacing locus: gc_rate.py's rate_limited/with_retry wrap list_activities_since() and download_activity() per call, never per page.
  2. Checkpointing locus: GarminBackfillController checkpoints resume cursor + dedup per-activity, finer-grained than DES-009's per-page sketch.
  3. Accepted residual: the listing call's own internal multi-page fetch is unrate-limited internally; tracked as B-R010-03, accepted with note, not fixed.
  4. design.md's DES-009 section owes a dated addendum noting the per-activity-not-per-page reality (not yet written).
- Dependents: REQ-010, DES-009, DES-005, REQ-NF-Reliab-001, REQ-NF-Reliab-002, GarminBackfillController.{h,cpp}, gc_rate.py
- Full record: archive/decisions-full.md

## DEC-048 — B-R010-04 UI wiring: fix the new backfill-local exposure now, defer GarminConnect's own internal context-handling gap
- Status: accepted (recording-only)
- Chose: Fix the narrow dialog-local SessionCheck gap now; defer GarminConnect/CloudService's internal raw-context dereference (B-R010-10) to a separate future REQ/DES.
- Binding:
  1. SessionCheck lambda in GarminBackfillDialog::startClicked() short-circuits on !context.isNull() before calling backfillSessionStillValid().
  2. GarminConnect.cpp/CloudService.cpp stay untouched by this fix.
  3. B-R010-10 (the foundational context-handling gap) is recorded as a new, non-blocking finding with no REQ allocated yet; not fixed here.
- Dependents: GarminBackfillDialog.cpp, GarminConnect.cpp, CloudService.cpp, REQ-010, DES-009, B-R010-10, T-196, commit 2b8cedae3
- Full record: archive/decisions-full.md

## DEC-049 — REQ-NF-Pkg-001 scope expansion: port Garmin sources into the qmake release build before any installer/packaging work
- Status: accepted (user decision, 2026-09-11, live during session)
- Chose: Split REQ-NF-Pkg-001 into two sequenced pieces under the same REQ: port Garmin sources into src/src.pro's qmake build first, then defer the originally-scoped Python packaging work.
- Binding:
  1. Step 1 (this cycle): add a contains(DEFINES, GC_WANT_GARMINCONNECT) block to src/src.pro mirroring GC_WANT_PYTHON's shape; add the toggle to src/gcconfig.pri.in; add enable-flag lines to all 3 appveyor/*/before_build scripts.
  2. Verify step 1 with an actual qmake6 && make build, not source-diff review alone.
  3. Step 2 (deferred, same REQ): src/Python/requirements.txt + installer-manifest deltas, meaningful only once step 1 lands.
  4. unittests/unittests.pro (qmake test project) stays deliberately out of scope; Garmin tests keep running only via the CMake/ctest garmin-fast path.
  5. Cloud/PyEmbeddedAdapter.cpp must include Python.h before any Qt header (Qt's slots macro collides with CPython's own slots field); use qmake's built-in NO_PCH_SOURCES mechanism, not CONFIG -= precompile_header (a no-op for .cpp under qmake's PCH generator) and not a hand-rolled QMAKE_EXTRA_COMPILERS rule (loses header-dependency tracking).
- Dependents: src/src.pro, src/gcconfig.pri.in, appveyor/linux/before_build.sh, appveyor/macos/before_build.sh, appveyor/windows/before_build.ps1, src/Python/requirements.txt, REQ-NF-Pkg-001, src/Cloud/PyEmbeddedAdapter.cpp
- Full record: archive/decisions-full.md

## DEC-050 — REQ-013 scope: narrow the first slice to DOB/weight/height, defer HR-max/FTP
- Status: accepted (user decision 2026-09-12, option 1 of 3 presented)
- Chose: Build REQ-013's first slice against dob/weight_kg/height_cm only, via existing per-cyclist app-settings scalars; defer hr_max/ftp_w.
- Binding:
  1. Use defensive, try-multiple-candidate-keys, skip-if-absent parsing of the real library's unverified JSON shape for all three fields.
  2. hr_max/ftp_w are deferred to a follow-up slice pending a separate Zones-system interaction design, not silently dropped.
  3. design.md's DES-011 gets narrowed to the first-slice return shape; hr_max/ftp_w recorded as an explicit follow-up.
  4. traceability.md's REQ-013 row must cite this DEC.
- Dependents: REQ-013, DES-011
- Full record: archive/decisions-full.md

## DEC-051 — REQ-NF-Obs-001's "ErrorBus" clause: satisfied by the project's real error channels; build only the missing structured trace logging
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Score REQ-NF-Obs-001's user-facing-errors clause as MET via the already-built readFailed signal (DEC-023) plus the errors out-param convention; build only the missing structured qDebug developer-trace logging.
- Binding:
  1. No new ErrorBus subsystem is built; DEC-022's rejection of that option stands.
  2. Structured qDebug logging is added at each sync op boundary (open/readdir/readFile/backfill), carrying op name, duration, activity count, Garmin error code.
  3. service/severity fields fold into the op-name prefix; success/failure is implicit in error-code presence, since no severity enum exists elsewhere in this codebase.
  4. design.md's DES-008 gets a dated addendum marking the ErrorBus sample superseded-in-practice by DEC-022/023, not deleted.
  5. traceability.md's REQ-NF-Obs-001 row updated to cite this DEC and split into its two separately-scored halves.
- Dependents: REQ-NF-Obs-001, DEC-022, DEC-023, GarminConnect.{h,cpp}
- Full record: archive/decisions-full.md

## DEC-052 — B-STAGE9-01 fix: shared process-level CPython bootstrap, owned by neither feature alone
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: One shared, process-level CPython bootstrap function, compiled whenever GC_WANT_PYTHON OR GC_WANT_GARMINCONNECT is ON, called once from main.cpp before any Garmin worker starts.
- Binding:
  1. New bootstrap has a Python-header-free public interface, matching PyEmbeddedAdapter.h's no-Python.h-in-headers invariant.
  2. Uses Py_InitializeFromConfig() with a checked PyStatus (not bare Py_InitializeEx(0)), so startup failure is reportable, not an abort.
  3. Signal-handler installation disabled; the user site-directory (~/.local/lib/python3.13/site-packages) explicitly preserved.
  4. PythonEmbed's PyImport_AppendInittab("goldencheetah", ...) must run before the shared bootstrap's first Py_Initialize call (CPython's inittab contract); PythonEmbed attaches to the running interpreter under the GIL rather than re-initializing it.
  5. Builder owns exact call sequencing/ordering guard and must cover: concurrent first-use races, PyEval_SaveThread() only on the initializing path, worker-teardown QThread::terminate() interaction, restart-loop PythonEmbed re-construction.
  6. Builder-owned test plan: Garmin-ON/Python-OFF, both-ON with scripting active/disabled, concurrent first use, token restoration as first op, worker teardown/recreation; existing seam tests' fail-safe assertions preserved.
- Dependents: src/Core/main.cpp, src/Python/PythonEmbed.{h,cpp}, src/Cloud/PyEmbeddedAdapter.cpp, src/src.pro, src/CMakeLists.txt, testGarminConnectPyAdapter.cpp, B-STAGE9-01
- Full record: archive/decisions-full.md

## DEC-053 — B-STAGE9-15 remedy: developer-trace literals are exempt from the i18n guard as a CATEGORY; the three flagged call sites are correct as written
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Extend is_technical() so developer-trace literals are exempt as a category (key=value tokens plus one optional leading bare event-name token), not by placeholder-count arithmetic; leave the three call sites unchanged.
- Binding:
  1. A literal is exempt when it consists of key=value tokens optionally preceded by a SINGLE leading bare snake_case identifier token, with at least one key=value token present.
  2. The repair is not complete without a RED-first test proving the new predicate still REJECTS a prose literal that merely contains an equals sign.
  3. The three call sites (garmin_auth_unknown exception_type=%1) are left unchanged; not wrapped in tr().
  4. Defect (1), the gate-coverage gap (garmin-i18n-guard not part of garmin-fast), is NOT fixed by this DEC and remains open under B-STAGE9-15.
  5. Independently verified before recording by a fresh unbriefed second-opinion agent given only the narrow question.
- Dependents: unittests/buildguard/garmin_i18n_source_guard.py, T-208, src/Cloud/GarminCredentialsPage.cpp, src/Cloud/GarminMfaPage.cpp, REQ-NF-i18n-001, B-STAGE9-15
- Full record: archive/decisions-full.md

## DEC-054 — B-STAGE9-16 remedy: the default verification scope becomes fail-safe (default-include ctest gate with an explicit opt-out label; lint scope widened to every new Garmin-owned path; two coverage guards)
<!-- gc-arms/v1 {"dec": "DEC-054", "patterns": ["unittests/Core/stderrbuf/*"]} -->
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Routine gate becomes ctest -LE gate-exclude (default-include, explicit opt-out); lint regex enumeration widened to every new Garmin-owned path; two new coverage guards added.
- Binding:
  1. Two flag-build guards get LABELS "garmin-build-guard;gate-exclude", each with an adjacent comment giving the exclusion REASON and the alternate TIER that runs them (release/integration tier, scheduled, not per-unit).
  2. The 11 tests newly entering the gate must each be proven green, or deliberately excluded with a recorded reason, before the new gate is declared the gate.
  3. Lint enumeration is widened, not made whole-tree: unittests/buildguard/ (ruff, ruff-format; mypy only if demonstrated clean) and unittests/Core/stderrbuf/ (clang-format) added to .pre-commit-config.yaml's files regexes.
  4. Two new static guards added: a GATE-COVERAGE guard (every registered ctest test is selected unless gate-exclude plus reason plus alternate tier) and a LINT-OWNERSHIP guard (every file under declared managed roots matches a hook regex); neither guard may itself carry gate-exclude.
  5. ctest stays OUT of pre-commit; the gate lives in the documented acceptance command and required merge validation instead.
  6. B-STAGE9-12's stderr-buffering test may not land RED into the default gate; it must either land its fix or register with gate-exclude plus a reason naming the hold.
  7. Corrects DEC-010's cascade prose: the live .pre-commit-config.yaml has no ctest hook at all; ratifies the config (no ctest in pre-commit) as intended, superseding that clause of DEC-010.
- Dependents: unittests/buildguard/CMakeLists.txt, .pre-commit-config.yaml, DEC-010, B-STAGE9-12, B-STAGE9-16, garmin_gate_coverage_guard.py, garmin_lint_ownership_guard.py
- Full record: archive/decisions-full.md

## DEC-055 — B-STAGE9-26 remedy: one typed contract for every key the summary shape requires
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Extend the typed contract to every required key in the summary shape — validate activityId the same way startTimeGMT is validated, raising a typed GarminError instead of a bare KeyError.
- Binding:
  1. garmin_client.py's _list_activities_since_impl raises GarminError("response_invalid", ...) naming the missing key when activityId is absent, matching startTimeGMT's existing _as_utc_instant validation.
  2. The fakes/pystub listing double must reject a record missing activityId, or the suite stays green while the real contract differs.
  3. A code comment asserting a checkable claim about behaviour is not accepted as evidence for that claim.
- Dependents: src/Python/garminconnect/garmin_client.py, B-STAGE9-26, classifyListException (src/Cloud/PyEmbeddedAdapter.cpp), REQ-002 traceability row
- Full record: archive/decisions-full.md

## DEC-056 — B-STAGE9-29 remedy: name Garmin entries from the activity's own local start time
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Marshal startTimeLocal and name Garmin entries from it (matching every peer service), replacing the current garmin-<id>.fit naming from startTimeGMT.
- Binding:
  1. GarminConnect.cpp:869-878 constructs entry names from startTimeLocal, not the activity id.
  2. garmin_client.py marshals startTimeLocal (declared optional str|None in the wheel) and falls back gracefully (never fails the listing) when absent.
  3. The pystub listing double must carry startTimeLocal or the C++ suite proves nothing about the new field.
  4. design.md's listing prose naming entries garmin-<id>.fit must be corrected.
  5. Download and dedup keys (readFile by remoteid, recordImport by the same id) do NOT move; only the staged/entry name changes.
- Dependents: GarminConnect.cpp, garmin_client.py, GarminActivitySummary, B-STAGE9-29, REQ-002, REQ-012, design.md
- Full record: archive/decisions-full.md

## DEC-057 — B-STAGE9-28 remedy: an empty managed root must be DECLARED empty, not silently passed
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: An empty managed root may report EMPTY only if its record DECLARES it pre-armed and names the DEC that armed its regexes; an undeclared empty root is a finding.
- Binding:
  1. B-STAGE9-28's own proposed option A (teach the guard to read git status/diff instead of git ls-files) is VOID — the guard already reads the index via git ls-files and already sees staged-but-uncommitted files; not adopted.
  2. unittests/Core/stderrbuf/ is the legitimate pre-armed case and must carry the declaration, not be silently exempted.
  3. test_an_empty_root_does_not_crash_and_does_not_go_red (test_garmin_lint_ownership_guard.py:702-711), which currently pins the silent-green contract, must be amended.
- Dependents: unittests/buildguard/garmin_lint_ownership_guard.py, test_garmin_lint_ownership_guard.py, B-STAGE9-28, LSN-083
- Full record: archive/decisions-full.md

## DEC-065 — Stage 9's installer evidence is earned on AppVeyor, not on a local build
- Status: ACCEPTED 2026-09-20 — USER decision
- Chose: Earn Stage 9's installer evidence via a push to AppVeyor CI, not a local build — the only route that exercises the macOS and Windows legs at all on this Linux-only host.
- Binding:
  1. The push is gated on a clean separation first: only the Garmin file set goes; git add -A is forbidden and each file's own diff is read before staging.
  2. The code queue is NOT blocked by this decision: B-STAGE9-36, the B-STAGE9-38 u3 repair, DEC-063, B-STAGE9-42 and DEC-062 all continue independently and must land before the push is worth making.
  3. B-STAGE9-42's pin-and-verify fix (appveyor/linux/after_build.sh:5, 3.11.16 + SHA-256) MUST be in the pushed set, or the Linux leg fetches a 404.
  4. APPIMAGE_EXTRACT_AND_RUN is NOT needed for this route and must not be carried into the recipe (it was only the local no-/dev/fuse workaround).
- Dependents: appveyor.yml, appveyor/linux/{install,before_build,after_build}.sh, B-STAGE9-48, B-STAGE9-54, B-STAGE9-57
- Full record: archive/decisions-full.md

## DEC-058 — B-STAGE9-38 remedy: the Garmin adapter ships as an installable distribution, not as a compiled-in source path
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Package the Garmin adapter as a real installable Python distribution under a new import namespace (gc_garmin_adapter), installed via each platform's existing pip step, removing the compiled-in absolute source path.
- Binding:
  1. Distribution root is src/Python/garminconnect/; package becomes src/Python/garminconnect/gc_garmin_adapter/ (no new src/ nesting level), discovered via where=["."] + include=["gc_garmin_adapter*"]; pyproject.toml gains [build-system] setuptools>=61.
  2. Import namespace changes to gc_garmin_adapter to avoid collision with the upstream garminconnect wheel; all eleven test files' imports, garmin_client.py's own "from gc_rate" (made relative), and pyproject.toml's stale top-level comments migrate in the same commit.
  3. Adapter is installed by its OWN explicit pip step per recipe, not an entry in requirements.txt (a repo-relative path resolves differently per leg's CWD): Linux "pip install -q --no-deps ./Python/garminconnect" after after_build.sh:50; macOS/Windows "./src/Python/garminconnect" from repo root, Windows naming C:\Python\python.exe -m pip explicitly (the bare python on PATH is the wrong interpreter), macOS/Windows adding an explicit repo-root cd.
  4. Each adapter pip step needs --no-build-isolation or network access for the setuptools build backend, and must run AFTER the -r requirements.txt install on every leg, or the adapter installs importable-but-non-functional (garmin_client.py swallows its own ImportError).
  5. GC_GARMIN_PYPATH stays the FIRST override in every mode; precedence/cache-safety repaired: sys.path override-dir entries deduplicated to index 0 without canonicalizing paths; sys.modules cache reused ONLY for entries proven loaded from the explicit override directory, never evicted, never blanket-rejected; a failed list op or non-list sys.path FAILs CLOSED.
  6. Linux CI smoke (appveyor.yml:252) becomes an extracted-bundle import assertion that exits nonzero on failure, not "AppImage --version".
  7. The rename reaches C++: all six PyImport_ImportModule("garmin_client") call sites, the exception-module allowlist/foreign_exception reject, the sys.modules type check, and the C++ test fixture pystubs/garmin_client.py (becomes pystubs/gc_garmin_adapter/garmin_client.py) must ALL move in the same commit — a single missed literal silently resolves the OLD module and passes.
  8. src/src.pro and src/CMakeLists.txt's production path defaults, and GC_GARMIN_PYPATH's own default, must name the directory CONTAINING gc_garmin_adapter/, not the package itself.
  9. main.cpp's CPython initialization on the Garmin-only path gets its own separate finding/unit (DEC-061/062), not folded into this one; the earlier-cited hold on main.cpp was RELEASED 2026-09-16.
- Dependents: src/src.pro, src/Python/garminconnect/pyproject.toml, src/Python/requirements.txt, src/Cloud/PyEmbeddedAdapter.cpp, src/Cloud/AddCloudWizard.cpp, appveyor.yml, appveyor/linux/after_build.sh, appveyor/macos/install.sh, src/CMakeLists.txt, B-STAGE9-38, REQ-NF-Pkg-001
- Full record: archive/decisions-full.md

## DEC-059 — B-STAGE9-36 round 3: a DEC arms a managed root by an explicit `Arms:` declaration, never by prose a guard greps
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: A DEC entry carries an explicit machine-readable "- Arms:" bullet listing the exact patterns globs it armed; the guard requires exact set membership, never a substring match over prose.
- Binding:
  1. "- Arms:" is matched on a line-anchored bullet inside the resolved entry body only, never anywhere else in the file.
  2. A root's patterns must match as whole entries against the parsed Arms list, not as substrings (pkg/* must not be satisfied by an Arms naming pkg/sub/*).
  3. An entry with no "- Arms:" bullet is its own finding — silence on the distinction is not permitted.
  4. _pattern_directory_prefixes's empty-tuple special case is superseded; matching is against patterns themselves.
  5. Amendment: the "- Arms:" line must be a glob list and nothing else — any non-whitespace outside backtick-quoted tokens is a finding, not a silent harvest; commentary goes on a separate "- Arms-note:" line the guard never reads.
  6. Amendment: more than one "- Arms:" bullet in a single entry is a finding; silently honouring only the first is not permitted.
- Dependents: unittests/buildguard/garmin_lint_ownership_guard.py, test_garmin_lint_ownership_guard.py, DEC-054, DEC-057, B-STAGE9-36
- Full record: archive/decisions-full.md

## DEC-060 — B-STAGE9-36 round 8: an Arms declaration names its own DEC, and ambiguity is a hard error, so Markdown block context stops being the guard's problem
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: A real Arms declaration must name its own enclosing DEC id ("- Arms DEC-054: `glob`") and is honoured only inside that DEC's own entry; any Arms-shaped line whose id doesn't match its enclosing entry is a hard error (ArmsBulletMalformed), never a silent skip.
- Binding:
  1. The guard stops trying to resolve Markdown block/container context (fences, HTML comments, divs); no container can defeat or fake a self-id'd declaration.
  2. An Arms-shaped line inside a fence/comment/div WITHIN its own named DEC's entry is still honoured — accepted as an irreducible false positive, same footing as DEC-053's pinned exemption.
  3. DEC-059's constraint 6 (exactly one bullet) is subsumed: two bullets naming this entry's own id is also ArmsBulletMalformed.
  4. Amendment (round 9): recognition is deliberately LOOSE (any of 5 measured near-miss spellings is Arms-SHAPED) but honouring stays STRICT — anything Arms-shaped that isn't exactly canonical raises ArmsBulletMalformed naming what it should have said; under-reach (silent skip) is never acceptable, over-reach (loud refusal) is.
  5. Amendment (round 9): reserved illustration lines must be spelled "Arms-<word>:" to stay excluded at the shape level.
  6. Amendment (round 11): the leading-indentation cap is retired; a line is Arms-shaped after any leading run of Unicode categories Zs/Zl/Zp/Cc/Cf, regardless of indentation width or Markdown block context.
  7. Amendment (round 11): accepted false negative, pinned — characters that render blank but are real printing characters (U+115F, U+3164, U+FFA0, U+2800) stay out of recognition scope; not to be re-filed.
  8. check_empty_roots_are_declared needs a test that reaches it with a genuinely empty managed root (the live DEC-054 pair no longer exercises that path).
- Dependents: unittests/buildguard/garmin_lint_ownership_guard.py, test_garmin_lint_ownership_guard.py, DEC-054, DEC-059, DEC-057, B-STAGE9-36
- Full record: archive/decisions-full.md

## DEC-061 — B-STAGE9-40 remedy: the startup path consumes `ensureInitialized()`'s failure `Result` nonfatally, at the call site that discards it
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Consume the bootstrap Result at main.cpp:552 nonfatally — retain the failure state, log it, and continue startup — rather than leave it discarded or treat it as fatal.
- Binding:
  1. The standing "main.cpp is under a hard hold" premise is FALSE: the user RELEASED it 2026-09-16 (STATE.md:750,758-760); corrected wherever carried forward (STATE.md, findings.md:573-574, DEC-058 entry) in the same pass.
  2. The assertable predicate is PyProcessBootstrap::isInitialized(), observed after main.cpp:552.
  3. No new error channel: use the existing log route, not a parallel ErrorBus (DEC-051 stands).
  4. --version cannot host the assertion — argument parsing exits before initialisation and --version is exactly what CI runs on Linux/macOS.
  5. A startup record emitted before stderr redirection (main.cpp:610-614) cannot rely on B-STAGE9-12's redirected-file flushing.
  6. src/Core/main.cpp still gets its own unit and its own commit, because B-STAGE9-39's fix touches the same region, not because of a hold.
- Dependents: src/Core/main.cpp:552, src/Python/PyProcessBootstrap.cpp, STATE.md, findings.md, B-STAGE9-40, B-STAGE9-39
- Full record: archive/decisions-full.md

## DEC-062 — B-STAGE9-39 remedy: a Qt-side deployment locator feeds explicit `PyConfig.home`/`program_name` before `PyConfig_Read`; no `PYTHONHOME` mutation, no `Py_SetProgramName`
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Extract PythonEmbed's existing deployment-detection logic into a shared Qt-side locator returning {home, program_name}, passed as explicit PyConfig fields set before PyConfig_Read; retire qputenv("PYTHONHOME") and the deprecated Py_SetProgramName.
- Binding:
  1. Call order is fixed: PyConfig_InitPythonConfig, then PyConfig_SetString(home), then PyConfig_SetString(program_name), then PyConfig_Read, then Py_InitializeFromConfig; measured to win even against a hostile inherited PYTHONHOME.
  2. home/program_name fields must be OPTIONAL: when the locator selects no deployed home, do NOT set an empty config.home (destroys normal PATH/env discovery); check every PyStatus and fail closed before PyConfig_Read.
  3. The locator PREFERS a deployed payload over the host stdlib when both exist (today's behaviour is the opposite and reproduces the shipped-bundle defect).
  4. The locator lives outside PyProcessBootstrap (Qt-side application policy); PyProcessBootstrap only sets what it is handed and stays settings-free/layout-free.
  5. PythonEmbed.cpp must preserve its existing fallback to an inherited, user-supplied PYTHONHOME; the extraction must be behaviour-preserving for the full existing selection/validation path, not just the deployed-path arithmetic.
  6. PythonEmbed::pythonInstalled() cannot simply be deleted — it is called by src/Gui/Pages.cpp; keep it as a forwarding wrapper or update that call site in the same change.
  7. The new locator must compile in the OR-bootstrap block of BOTH build systems; DEC-063 (CMake's GC_WANT_PYTHON fix) must land first, since CMake currently defines GC_HAVE_PYTHON, which nothing reads.
  8. program_name is derived from the detected version, never hardcoded.
  9. Every Py_SetProgramName call site (PyProcessBootstrap.cpp, PythonEmbed.cpp wrapper, testPythonProgramNameLifetime.cpp plus its CMake registration) must be removed or replaced together — DEC-064 settles this: delete the lifetime test, since PyConfig_SetString's copying contract removes the hazard it tested.
  10. PySys_SetPath (PythonEmbed.cpp) stays dead; no option revives it.
- Dependents: PyProcessBootstrap.{h,cpp}, PythonEmbed.cpp, src/CMakeLists.txt, src/src.pro, src/Gui/Pages.cpp, unittests/Core/garminconnect/CMakeLists.txt, B-STAGE9-39, DEC-061, DEC-063, DEC-064
- Full record: archive/decisions-full.md

## DEC-063 — B-STAGE9-41 remedy: CMake defines `GC_WANT_PYTHON` target-locally; the unconsumed `GC_HAVE_PYTHON` spelling is deleted
- Status: accepted (Inspector, Three-Options Doctrine)
- Chose: Replace CMake's directory-scoped add_definitions(-DGC_HAVE_PYTHON) with a target-local GC_WANT_PYTHON definition on the GoldenCheetah target, matching the existing GC_WANT_GARMINCONNECT convention; delete GC_HAVE_PYTHON entirely.
- Binding:
  1. The change is two edits, not one: DELETE the add_definitions at :1085 and ADD the target-local definition after add_executable(GoldenCheetah ...) at :1398 — the target does not exist yet at :1085.
  2. Acceptance check for "GC_HAVE_PYTHON removed" is scoped to LIVE BUILD INPUTS only (.cpp/.h/.pri/.pro/.cmake/.py, shell, CI YAML), not whole-repo text.
  3. GC_HAVE_PYTHON is deleted, not aliased; .kilo/worktrees/ is out of scope and must not be touched.
  4. Behaviour under GC_WANT_PYTHON=OFF must be byte-identical: no macro defined, no sources selected; only the ON leg changes.
  5. The unit must actually configure and build the CMake Python-on leg (previously unmeasured), not just assert the macro's presence; any compile errors surfaced are this repair's findings to report.
  6. The existing flag-build guard's passing state (which pins Python OFF) is not evidence for this repair and must not be cited as such.
  7. DEC-063 must land before DEC-062 (precondition for DEC-062's "both configurations reach the same home" claim).
- Dependents: src/CMakeLists.txt, DEC-062, B-STAGE9-41
- Full record: archive/decisions-full.md

## DEC-064 — B-STAGE9-36 remedy: the Arms authorization becomes a structured record, not a Markdown inference
- Status: ACCEPTED 2026-09-20 — Option C
- Chose: Replace the DEC-060 Markdown-recognizer doctrine, for the Arms field only, with a fixed-position embedded sentinel (gc-arms/v1 HTML comment) immediately following the resolved DEC heading, validated only in that exact slot.
- Binding:
  1. The sentinel REPLACES DEC-054's prose "- Arms DEC-054:" declaration in the same change; it must not duplicate a live prose bullet.
  2. Sentinel is validated in its fixed slot only (immediately after the DEC heading) — no scan of later bullets at any width; absent/malformed/id-mismatched is an error, never a skip.
  3. _is_arms_loose_shaped and _strip_arms_leading_run (DEC-060's loose-recognizer doctrine) are DELETED, not widened; DEC-060's pinned Lo/So accepted false negatives become moot.
  4. ArmsBulletMalformed is retained as the error transport, now raised for a malformed sentinel, and converted to LINT-EMPTY-BOGUS-DEC.
  5. The sentinel is read from the RAW resolved entry, ignoring the masked=True flag an HTML comment necessarily sets — rejecting masked entries would reject every valid sentinel.
  6. The "no Arms bullet" branch becomes unreachable and must be deleted or replaced with sentinel-specific error handling, not left dead; LINT-EMPTY-UNDECLARED/BOGUS-DEC/VACUOUS remain valid.
  7. Exactly one live declaration exists (decisions.md:2647); ledger_drift_lint.py does not scan decisions.md and needs no change.
  8. testPythonProgramNameLifetime.cpp and its CMakeLists.txt registration are DELETED, not migrated — DEC-062's PyConfig_SetString copies the string, so no dangling-caller-buffer hazard remains to test.
- Dependents: unittests/buildguard/garmin_lint_ownership_guard.py, test_garmin_lint_ownership_guard.py, decisions.md:2647, DEC-060, DEC-054, B-STAGE9-36, DEC-062
- Full record: archive/decisions-full.md

## DEC-066 — B-STAGE9-45 remedy: module provenance is a C++-owned identity ledger, not an attribute read off the cached module
- Status: accepted 2026-09-20 (Inspector, Three-Options Doctrine)
- Chose: Record a C++-owned (interpreter, override-dir, full-name, PyObject*) identity ledger with a strong reference for every module produced by the clean import from the hoisted override dir; later adapter construction accepts only identical pointers, reading no Python-visible attribute.
- Binding:
  1. Rejecting only pre-existing UNLEDGERED entries is insufficient: the ledger must be validated for exact cache/ledger membership and pointer identity AFTER the import returns, not only before it.
  2. The ledger is cleared under the owning interpreter's GIL BEFORE Py_FinalizeEx() — every strong reference released and every entry for that interpreter erased; no ledgered object is inspected or decref'd after finalization.
  3. No eviction of sys.modules and no blanket rejection of a cached entry; a genuine second adapter construction over the same override must still succeed.
  4. The ledger is only ever reached with the GIL held, at all current and any future import call sites.
  5. Any future lazy gc_garmin_adapter.* submodule import needs its own recording/validation rule or it arrives unledgered and is rejected.
  6. Accepted residuals: trusted override code mutating its own genuine module object, or arbitrary code running during the initial import, are not defended against.
- Dependents: src/Cloud/PyEmbeddedAdapter.cpp:178-257, testGarminConnectPyAdapter.cpp, DEC-058 c17(b), B-STAGE9-45, B-STAGE9-64
- Full record: archive/decisions-full.md

## DEC-067 — the Linux smoke assert gets one last strengthening, then its residual class is pinned
- Status: accepted 2026-09-20 (Inspector)
- Chose: Strengthen the Linux CI smoke assert to usability checks (callable(_gc.Garmin); each of three exception names is a type and a BaseException subclass) and PIN the remaining residual as accepted — no further repair round on this recognizer class is authorized.
- Binding:
  1. A bundle that satisfies every conjunct and still fails at runtime is an ACCEPTED residual, same footing as the shellcheck declared-gap and DEC-060's Lo/So fillers; detected instead by Stage 9's live-account acceptance criterion, not this smoke step.
  2. No further B-STAGE9-59 repair round is authorized; a reviewer finding of this class is recorded against this DEC and closed, not dispatched.
  3. Scope-pass shape constraints on appveyor.yml:258's Python literal: one physical line; double-quoted literals only (a literal single-quote terminates the enclosing sh argument); a missing m._gc.Garmin/exceptions attribute raises AttributeError, fail-closed via "|| exit 1", accepted; issubclass needs an isinstance(x, type) guard.
  4. The builder writes the exact specified 522-char, zero-single-quote, zero-newline source string, unchanged.
- Dependents: appveyor.yml:257-259, DEC-058 C11/C15, B-STAGE9-59
- Full record: archive/decisions-full.md

## DEC-068 — B-STAGE9-66's premise is refuted; the returned-object compare stays as a tripwire, and the CPython invariant it rests on becomes a test
- Status: accepted 2026-09-24 (Inspector, Three-Options Doctrine)
- Chose: Keep the PyEmbeddedAdapter.cpp:353-362 compare branch as an honestly-labeled tripwire, not delete it and not leave its comment's false claim uncorrected; add a test that pins the CPython invariant it depends on rather than the untestable branch itself.
- Binding:
  1. The :346-353 comment is rewritten to say the compare is REDUNDANT under CPython's current PyImport_Import semantics, retained as a tripwire, citing this DEC rather than restating an unverified claim.
  2. New test: with a hostile builtins.__import__ that writes a decoy into sys.modules and returns a DIFFERENT object, PyImport_ImportModule must hand back the sys.modules object — if this ever goes RED, this DEC is revisited.
  3. Accepted residual, pinned: the :353-362 branch itself has no RED test and cannot be given one while the invariant holds; no repair round on "the branch is untested" is authorized.
- Dependents: src/Cloud/PyEmbeddedAdapter.cpp:346-362, DEC-066 constraint 1, B-STAGE9-66, B-STAGE9-67
- Full record: archive/decisions-full.md

## DEC-069 — B-STAGE9-71 remedy: the Stage 9 payload assertion covers all three shipped legs
- Status: accepted 2026-09-24 (USER decision)
- Chose: Extend DEC-067's payload assertion to the Windows and macOS test_script arms too (both legs), not Windows-only or deferred, so all three shipped platforms are proven before the three installer findings close.
- Binding:
  1. Assert against the PRODUCED artifact, never its staging tree: Windows extracts/installs the actual .exe and runs the python.exe it installed; macOS mounts the actual .dmg and runs the bundled interpreter, then detaches.
  2. Reuse DEC-067's :258 predicates exactly; do not restate them.
  3. The stated limit is carried: this proves the installer payload contains a usable adapter and dependencies on all three legs; it does NOT prove GoldenCheetah's own embedded-startup path. B-STAGE9-48/-54/-57 close against the payload claim only.
  4. Neither new assertion is executable on the development host; both are reviewed as unrunnable code (read, not tested).
- Dependents: appveyor.yml (Windows/macOS test_script arms), DEC-067, DEC-065, B-STAGE9-71, B-STAGE9-48, B-STAGE9-54, B-STAGE9-57
- Full record: archive/decisions-full.md

## DEC-070 — B-STAGE9-78 remedy: stage under the payload's true extension, don't re-implement unzip
- Status: accepted 2026-09-26 (Inspector, Three-Options Doctrine)
- Chose: Sniff the leading bytes of the downloaded payload and stage under its true extension (.zip on PK magic, .fit otherwise), reusing RideImportWizard's existing suffix-dispatched unzip route rather than re-implementing archive handling in the controller.
- Binding:
  1. A bare FIT must still stage as .fit; the extension follows the sniffed bytes, never a hardcoded expectation about what Garmin returns.
  2. The controller is the ONE source for the staged path/extension; GarminBackfillDialog.cpp's read side must not guess or re-sniff independently.
  3. DES-006 (torn-write pause) and DES-009 (record-then-progress order) are unchanged; only the filename and its propagation move.
  4. The two already-staged live ZIP fixtures under the athlete's garminconnect/backfill/ must not be deleted or rewritten — they are the regression fixture.
  5. RideImportWizard is pre-existing shared code and is not modified for this dialog.
- Dependents: src/Cloud/GarminBackfillController.{h,cpp}, src/Cloud/GarminBackfillDialog.cpp:190, B-STAGE9-78, B-STAGE9-79
- Full record: archive/decisions-full.md

## DEC-071 — B-STAGE9-79 remedy: two records, and RideCache is the import-completion seam
- Status: accepted 2026-09-26 (Inspector, Three-Options Doctrine)
- Chose: Split the single overloaded record into two — a versioned pending[id] manifest in backfill-state (download/resume record) and imported-<uid>.json restricted to import-COMPLETION only; skip predicates consult completion only, evaluated via RideCache::getRide(startTimeGMT.toUTC()) as the completion seam.
- Binding:
  1. Evaluate completion after the wizard finishes, per activity, via getRide(startTimeGMT.toUTC()) exact match; do NOT use process()'s return value as completion.
  2. Migration self-classifies each legacy entry by evaluating it against RideCache once; do not blanket re-offer all legacy entries.
  3. Crash order is pending-then-cursor, atomically, before progress is reported; a crash between staging and completion must leave a pending entry that is safely re-offered. Do not fix this by moving recordImported after the wizard.
  4. Both routes must follow the same split: GarminConnect.cpp's incremental-sync path (874-931) writes the same two records and must not diverge from GarminBackfillController's split.
  5. No FIT pre-parse and no timestamp tolerance window may be added without new measurement — all three live sidecar entries matched RideCache to the exact second.
  6. Rename recordImported (it means "downloaded") to state what it actually holds, in the same round.
- Dependents: GarminSidecarStore.{h,cpp}, GarminBackfillController.cpp:229-235/282-324, GarminBackfillDialog.cpp, GarminConnect.cpp:874-931, REQ-008, REQ-010, REQ-016, DES-006, DES-009, DES-010, B-STAGE9-79, B-STAGE9-84, B-STAGE9-85
- Full record: archive/decisions-full.md

## DEC-072 — B-STAGE9-83 remedy: inflate gzip at stage time; there is no gzip route to reuse
- Status: accepted 2026-09-26 (Inspector, Three-Options Doctrine)
- Chose: Inflate a gzip payload in the controller itself, then re-sniff and stage the INNER decompressed payload, because Archive::dir/extract's GZIP arm is an empty stub, so DEC-070's "gzip is covered for free" premise was false.
- Binding:
  1. Sniff the INFLATED bytes, then stage: gzip-of-FIT becomes .fit, gzip-of-zip becomes .zip; no hardcoded expectation about what Garmin returns.
  2. A failed/empty/short inflate stages the original bytes verbatim under .gzip and lets import fail loudly; never fabricate a .fit name for unproven bytes.
  3. DES-006/DES-009 unchanged; only the bytes chosen for staging and their suffix move.
  4. No second ZIP implementation: DEC-070's option-B rejection still binds; ZipReader via Archive stays the only unzip route.
  5. Naming closes with it (B-STAGE9-85): comments/identifiers still saying "FIT bytes"/"staged FIT" on paths now holding arbitrary payloads are corrected in the same round.
  6. Accepted cost: this is a third copy of the gUncompress shape (alongside CloudService.cpp and RideFile.cpp), accepted rather than justifying shared-code surgery on a blocking checkpoint; the empty Archive::dir GZIP arm remains a filed upstream defect, not fixed here.
- Dependents: src/Cloud/GarminBackfillController.{h,cpp}, B-STAGE9-83, B-STAGE9-85, DEC-070
- Full record: archive/decisions-full.md

## DEC-073 — B-STAGE9-86 remedy: an undecodable payload is a download FAILURE, not a staging guess
- Status: accepted 2026-09-26 (Inspector, Three-Options Doctrine)
- Chose: Invert the payload-shape predicate: the controller either produces bytes resolvable to exactly two accepted shapes (a complete single gzip member whose output is FIT-or-ZIP, or a bare FIT/ZIP) or declares the download failed — it no longer guesses an extension for an open-ended enumeration of payload shapes.
- Binding:
  1. One inflate, never two: a single gzip member only; gzip-of-gzip is a refusal, not a second pass (RideImportWizard expands only one depth).
  2. Completeness comes from zlib's own signal, not a guess: accept inflate output only on Z_STREAM_END with avail_in == 0; Z_OK/Z_BUF_ERROR at loop exit or leftover input is a refusal. A partial member must never be staged.
  3. Refusal is a new PauseReason::UndecodablePayload — the activity is NOT recorded imported and the cursor does NOT advance past it, not a silent skip.
  4. Accepted cost: if Garmin serves that shape persistently, the backfill re-fetches it every run with no progress, same trade DES-006 already makes for a torn write, loud rather than silent; the per-activity skip-list fix belongs to DEC-071's record split, not here.
  5. ZIP keeps its existing signature sniff and real ZipReader/Archive implementation; only gzip is fully resolved here, because nothing downstream implements it.
  6. Tests must assert the refusal, not a suffix, for: truncated member, concatenated members, gzip-of-gzip, empty inflate, and a FIT-signature lie.
- Dependents: src/Cloud/GarminBackfillController.{h,cpp}, DES-006 (PauseReason set), B-STAGE9-86, B-STAGE9-87, B-STAGE9-89, B-STAGE9-90, B-STAGE9-91, DEC-072
- Full record: archive/decisions-full.md

## DEC-074 — findings.md's 200B row cap is the defect, not the rows
- Status: accepted 2026-09-26
- Chose: Retire findings.md's flat 200B row cap; set roughly 600B soft and 1,200B hard caps, routing accumulated repair-round leakage to the archive instead of the rows.
- Binding:
  1. Retire the flat 200B cap for findings.md.
  2. New caps: approximately 600B soft, 1,200B hard.
  3. Route repair-round leakage to archive/findings-detail.md, append-only.
  4. scripts/clv_findings.py stays UNCHANGED as the gate.
  5. Nothing deleted; archived content kept verbatim.
  6. BUDGETS must stay in breach honestly (not declared green) while the worst row exceeds the hard cap; follow-up owned by B-STAGE9-97.
- Dependents: STATE.md, archive/findings-detail.md, scripts/clv_findings.py
- Full record: archive/decisions-full.md

## DEC-075 — slice 1's write contract: `pending` is single-writer, and a file we cannot model is never overwritten
- Status: accepted 2026-09-27
- Chose: pending becomes single-writer inside GarminSidecarStore; saveBackfillState never overwrites it, and states the store cannot model refuse the write instead of overwriting it.
- Binding:
  1. pending is single-writer: only recordPendingBackfill and dropPendingBackfill mutate it.
  2. saveBackfillState persists only the three cursor fields and preserves the on-disk pending map verbatim, ignoring its argument's pending.
  3. LoadStatus NotFound and Torn keep self-healing by overwrite, per the existing recordImported precedent.
  4. SidecarPermissionsRejected and new PendingManifestMalformed make all three writers return false and write nothing.
  5. Slice 2 must not add a load-first workaround this makes dead.
  6. TEST ids for the three new assertions are allocated when the repair round reports.
- Dependents: src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, unittests/Core/garminconnect/testGarminSidecarStore.cpp
- Full record: archive/decisions-full.md

## DEC-076 — B-STAGE9-108 remedy: the sidecar store serializes its own load-modify-write transactions
- Status: accepted 2026-09-27
- Chose: serialize load-modify-write transactions inside GarminSidecarStore with a per-file lock held across the whole transaction, ahead of slice 2.
- Binding:
  1. Serialize on the resolved sidecar path inside GarminSidecarStore.
  2. Lock covers saveBackfillState, recordImported, recordPendingBackfill and dropPendingBackfill from load through AtomicFile::writeOver.
  3. AtomicFile keeps per-write atomicity; the lock supplies atomicity across the read-modify-write.
  4. The fix must land BEFORE slice 2, since the pending-map loss is latent until slice 2 gives it callers.
  5. An in-process lock does NOT serialize two GoldenCheetah processes sharing one athlete config directory; out of scope, not covered by this decision.
  6. TEST ids for the new assertions are allocated when the round reports.
- Dependents: src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, unittests/Core/garminconnect/testGarminSidecarStore.cpp
- Full record: archive/decisions-full.md

## DEC-077 — B-STAGE9-116 is upstream GoldenCheetah, not this branch's: frozen, not repaired
- Status: accepted 2026-09-27
- Chose: B-STAGE9-116, the RideImportWizard raw Context pointer use-after-free, is frozen as pre-existing upstream GoldenCheetah and not repaired on this branch.
- Binding:
  1. src/Gui/RideImportWizard.h, src/Gui/RideImportWizard.cpp, src/Core/Athlete.cpp and src/Core/Context.h stay on HARD HOLD for every builder round on this branch.
  2. B-STAGE9-116 stays a blocking finding against GoldenCheetah upstream; it stops blocking Stage 9.
  3. B-STAGE9-114's fix, the dialog's own post-process() sweep, stands unaffected.
  4. B-STAGE9-117's stub-fidelity residual is pinned alongside it, needing the same owning teardown and nested loop.
- Dependents: src/Gui/RideImportWizard.h, src/Gui/RideImportWizard.cpp, src/Core/Athlete.cpp, src/Core/Context.h
- Full record: archive/decisions-full.md

## DEC-078 — B-STAGE9-112 remedy: the resume cursor and the range start stop being one variable
- Status: accepted 2026-09-27
- Chose: split the resume cursor from the range start in GarminBackfillController::start(): an exclusive prior-success marker and a separately inclusive range start.
- Binding:
  1. Skip if a prior success exists and startTimeGMT is at or before lastSuccess.
  2. Skip if startTimeGMT is before rangeStartGmt.
  3. Skip if startTimeGMT is after rangeEndGmt.
  4. A cleared cursor means no prior success; rangeStart is then honoured inclusively.
  5. No greatest-surviving rewind is needed; the store keeps DEC-076's simpler transaction.
  6. B-STAGE9-122's assertions must name expected cursor values rather than inequalities, in the same round.
- Dependents: src/Cloud/GarminBackfillController.cpp, src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, unittests/Core/garminconnect/testGarminBackfillController.cpp, unittests/Core/garminconnect/testGarminSidecarStore.cpp
- Full record: archive/decisions-full.md

## DEC-079 — B-STAGE9-79 slice 3: the legacy migration classifies itself at the dialog, before start()
- Status: accepted 2026-09-27
- Chose: run a one-shot legacy migration in GarminBackfillDialog, before GarminBackfillController::start(), exact-matching legacy imported rows against RideCache.
- Binding:
  1. Gate the migration on state.isOk() and schemaVersion == 0.
  2. Exact-match every legacy imported entry against RideCache.
  3. Retain matches as completion, move misses into pending, then stamp the schema version so the pass never repeats.
  4. Must be idempotent: a crash mid-pass must leave a state a re-run can finish.
  5. NotFound no-ops; the first ordinary backfill writes v1.
  6. A torn sidecar yields no recoverable entries and must not be classified or promoted.
  7. Permission-rejected and malformed-pending states stay untouched; every cursor writer already refuses them.
  8. Amendment: for bf.isOk() with schemaVersion == 0, recordImport() keeps recordImported() but SKIPS the cursor save, so the dialog stays the sole upgrader.
  9. B-STAGE9-111's replacement pending writer takes the same guard.
  10. Scope limit: the guard applies ONLY to an existing Ok, v0 state; NotFound and torn states keep DEC-075's self-healing write.
  11. Cost accepted: a v0 athlete who never opens Backfill re-lists from the old cursor every sync, which is repeated work, not loss or duplicate download.
- Dependents: src/Cloud/GarminBackfillDialog.cpp, src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, unittests/Core/garminconnect/testGarminBackfillDialogLifetime.cpp
- Full record: archive/decisions-full.md

## DEC-080 — DEC-077's freeze is narrowed: additive no-op extension points are allowed, repairs are not
- Status: accepted 2026-09-27
- Chose: narrow DEC-077's freeze so an additive, default-no-op extension point is allowed on frozen files, while repairs stay forbidden; authorize a rideRegistrationCompleted hook.
- Binding:
  1. Authorized: a virtual rideRegistrationCompleted(remoteId) with an empty default body on CloudService.
  2. Invocation after each SUCCESSFUL registration only.
  3. GarminConnect's override performs the promotion through the DEC-075 and DEC-076 guarded order.
  4. The hook must NOT fire on any abandonment path: abort before or after parse, existing-file refusal, auto teardown, parse failure, duplicate-file refusal.
  5. Three things stay frozen and are NOT reopened: the RideImportWizard raw Context use-after-free (B-STAGE9-116), ArchiveFile.cpp's empty GZIP arm, CloudService.cpp:565's gUncompress.
  6. Any future change here that is not both additive and default-no-op for every existing provider needs its own decision.
  7. T-048's contract changes: an abandoned ride becomes pending with the cursor advanced and absent from imported; the next listing re-offers it.
  8. A null context path has no registration consumer: it persists pending and never promotes.
- Dependents: src/Cloud/CloudService.h, src/Cloud/CloudService.cpp, src/Cloud/GarminConnect.h, src/Cloud/GarminConnect.cpp, unittests/Core/garminconnect/testGarminConnectSync.cpp
- Full record: archive/decisions-full.md

## DEC-081 — Garmin backfill timestamps are canonicalised at the adapter boundary, not compared as text
- Status: accepted 2026-09-27
- Chose: canonicalise Garmin backfill timestamps in the Python adapter to one UTC spelling, rather than comparing raw text at the five C++ sites.
- Binding:
  1. Each accepted summary carries one yyyy-MM-dd HH:mm:ss UTC spelling from the adapter; the adapter keeps parsing every variant it accepts today.
  2. Canonicalisation must REJECT a non-zero fractional second rather than round or truncate it.
  3. Sidecars already on disk are rewritten once, atomically, cursor and ranges and pending and imported together.
  4. Readers accept the old spelling for as long as legacy files are supported.
  5. FakeBackfillClient::listActivities() must move to the same canonical basis or no controller test can catch a regression.
  6. Does NOT hold Stage 9 and must not displace B-STAGE9-79 slice 3 or B-STAGE9-111 in the builder queue.
  7. Regression coverage required: an equivalent-instant cursor and summary pair must neither re-download nor strand.
- Dependents: src/Python/garminconnect/gc_garmin_adapter/garmin_client.py, src/Cloud/GarminSidecarStore.cpp, src/Cloud/GarminBackfillController.cpp, src/Cloud/GarminBackfillDialog.cpp, unittests/Core/garminconnect/testGarminBackfillController.cpp, unittests/Core/garminconnect/testGarminSidecarStore.cpp
- Full record: archive/decisions-full.md

## DEC-082 — slice 3's legacy migration persists as a three-phase, v0-preserving store transaction
- Status: accepted 2026-09-27
- Chose: add a new store entry point, migrateLegacyImported(), that runs the legacy migration as one three-phase, v0-preserving transaction under both sidecar locks.
- Binding:
  1. migrateLegacyImported(configDir, uid, unmatched) acquires both resolved-path locks in one documented global order, held across the whole pass.
  2. Phase 1 writes backfill-state merging unmatched rows into pending while KEEPING schema_version 0.
  3. Phase 2 rewrites imported-uid.json without those rows.
  4. Phase 3 writes backfill-state preserving pending and stamping v1.
  5. Phase 1 needs a PRIVATE explicit-version state writer; public writers must NOT honour BackfillState::schemaVersion and stay pinned to v1.
  6. Both locks in one documented global order, held from load through phase 3.
  7. Phases 1 and 3 use unlocked private helpers; saveBackfillState, recordPendingBackfill and dropPendingBackfill each reacquire the same non-recursive mutex.
  8. Phase 1 MERGES into existing pending rather than replacing it; the prune is one atomic map rewrite; v1 is stamped only after that rewrite succeeds.
  9. DEC-075's refusals, SidecarPermissionsRejected and PendingManifestMalformed, apply before phase 1.
  10. Failure-injection coverage is required after each phase, each retried from its persisted intermediate.
  11. Narrow amendment to DEC-075: this one-shot, store-local migration is the sole addition beyond recordPendingBackfill and dropPendingBackfill as pending mutators; callers gain no pending-mutation ability.
  12. The two locks serialise WRITERS only, not readers; the mechanism must not be described as giving readers an atomic view of the pair.
- Dependents: src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, src/Cloud/GarminBackfillDialog.cpp, unittests/Core/garminconnect/testGarminSidecarStore.cpp, unittests/Core/garminconnect/testGarminBackfillDialogLifetime.cpp
- Full record: archive/decisions-full.md

## DEC-083 — the backfill cursor becomes a completeness watermark, advanced only by promotion
- Status: accepted 2026-09-27
- Chose: the cursor now means completeness and only promotion may move it; download time no longer advances it.
- Binding:
  1. recordImport() no longer writes the cursor; download time writes a PENDING row and nothing else.
  2. Promotion is its own store operation: record imported, remove the pending row, and advance the cursor to that entry's startTimeGMT ONLY when no pending row at or before that time remains.
  3. dropPendingBackfill keeps DEC-078 and B-STAGE9-126 clearing, for ABANDONMENT only.
  4. GarminBackfillController's exclusion filter drops only a startTimeGMT STRICTLY BEFORE the prior success, not at-or-before.
  5. Controller processing skips ids already PENDING.
  6. No lookback window is added here; adding one would need its own measurement.
  7. T-235, T-239 and T-241 stay as ABANDONMENT contracts and must not be rewritten to cover promotion.
- Dependents: src/Cloud/GarminSidecarStore.h, src/Cloud/GarminSidecarStore.cpp, src/Cloud/GarminConnect.cpp, src/Cloud/GarminBackfillController.cpp, src/Cloud/GarminBackfillDialog.cpp
- Full record: archive/decisions-full.md

## DEC-084 — one offset-correct instant primitive owns every Garmin timestamp comparison
- Status: accepted 2026-09-27
- Chose: a single primitive, garminInstantFromString() in new GarminTime.h, becomes the only way any site turns a startTimeGMT into an instant for comparison.
- Binding:
  1. Parse yyyy-MM-dd HH:mm:ss first, then Qt::ISODate, then branch on timeSpec(): a no-zone spelling is DECLARED UTC via setTimeSpec; a Z or explicit-offset spelling is CONVERTED via toUTC().
  2. Return an invalid QDateTime on an unparseable input; every caller must treat invalid as do-not-act, never as a zero instant.
  3. Delete the three lenient copies in GarminConnect.cpp, GarminBackfillController.cpp and GarminSidecarStore.cpp; they call the primitive instead.
  4. Every remaining text comparison site compares primitive results instead: the store's clause-2 survivor check, and the controller's clause-4 filter, range bounds, and oldest-first sort.
  5. Inclusive and exclusive boundaries are preserved exactly as DEC-083 clause 2 and clause 4 state them; only the comparison basis changes.
  6. GarminBackfillDialog.cpp is OUT of scope and keeps its own copy; its missing ISO fallback is deliberate per DEC-071.
  7. No site may rely on invalid-instant ordering; every comparison site tests validity EXPLICITLY and takes its own conservative branch.
  8. The store's clause-2 survivor guard treats an unparseable pending row as SURVIVING, protecting completeness.
  9. The controller's clause-4 filter and range bounds do NOT exclude an entry whose startTimeGMT fails to parse, protecting reachability.
  10. The oldest-first sort comparator orders on the isValid-then-instant pair, required for a strict weak ordering and safe std::sort.
  11. This does NOT normalise what is persisted, which stays DEC-081's job and remains owed; it does not revisit DEC-083's limit on late-published activities.
- Dependents: src/Cloud/GarminTime.h, src/Cloud/GarminSidecarStore.cpp, src/Cloud/GarminConnect.cpp, src/Cloud/GarminBackfillController.cpp
- Full record: archive/decisions-full.md

## DEC-085 — the `.codex/` worktree mandate does not bind an inspector-cycle-supervised round
- Status: superseded 2026-09-28 by DEC-086 (the user chose the Claude Code builder instead)
- Chose: the .codex worktree mandate does not bind a builder round supervised under inspector-cycle; such a builder implements in the integration checkout instead.
- Binding:
  1. A builder dispatched by the Inspector under inspector-cycle implements in the integration checkout.
  2. Its isolation comes from the feature branch plus the brief's PATHS and HARD HOLD lists, enforced per round by the Inspector and re-checked by the reviewer.
  3. .codex/WORKFLOW.md is otherwise untouched and still governs unsupervised Codex feature work, including its read-only allowance.
  4. .codex/ is NOT edited by this decision.
  5. A standing note goes in the roster reference so the next Inspector does not re-litigate it.
- Dependents: .claude/skills/inspector-cycle/references/agent-roster-and-dispatch.md
- Full record: archive/decisions-full.md

## DEC-086 — the inspector-cycle builder runs on Claude Code; Codex keeps read-only roles only
- Status: accepted 2026-09-28 (user decision at the B-STAGE9-156 gate; supersedes DEC-085)
- Chose: the builder pane is Claude Code (auto mode + Sonnet); reviewer and investigator stay Codex. The `.codex/WORKFLOW.md` worktree mandate stays untouched and binding on every Codex pane.
- Binding:
  1. No Codex pane writes the shared checkout: the reviewer only reads, the investigator writes only under `/tmp`.
  2. The builder launches with `--kind claude -- --permission-mode auto --model sonnet`, on every start and restart.
  3. `.codex/` is not edited by this decision.
- Dependents: `.claude/skills/inspector-cycle/references/agent-roster-and-dispatch.md`, `.claude/inspector-briefings/builder.md`, B-STAGE9-156
- Full record: this entry (written under the ledger writing contract; no archive copy)

## DEC-087 — store writers preserve an on-disk v0; only the migration's phase 3 stamps v1
- Status: accepted 2026-09-28 (Inspector technical decision; repair-round bound, B-STAGE9-154 class round 2)
- Chose: the v0 invariant moves into the store's single write choke point. Per-writer refusals are replaced, not extended.
- Binding:
  1. Every public backfill-state writer re-stamps the version it loaded when that state is Ok and v0; it never stamps v1 over it.
  2. Only migrateLegacyImported's phase 3 stamps v1.
  3. An Ok v0 athlete whose imported file is NotFound migrates as a no-op: phase 3 only, no phase-1 write.
  4. A skipped migration (null RideCache) never pauses ordinary backfill; the v0 gate retries on the next open.
  5. Amended 2026-09-28 (B-STAGE9-157 round 2): the dialog passes the exact imported-file bytes it classified; the store refuses, writing nothing, if the file under lock differs. Key-set checks are not enough.
  6. NotFound and torn states keep DEC-075's self-healing write (DEC-079 binding 10).
  7. Supersedes DEC-079 binding 8 and B-STAGE9-154's per-writer refusal remedy.
- Dependents: src/Cloud/GarminSidecarStore.{h,cpp}, src/Cloud/GarminBackfillDialog.cpp, B-STAGE9-154, B-STAGE9-157, B-STAGE9-158
- Full record: this entry

## DEC-088 — fork CI D2XX fetch: completion marker, malformed vendor archive pinned as accepted residual
- Status: accepted 2026-09-29 (Inspector technical decision; repair-round cap, B-STAGE9-172 class round 3)
- Chose: pin-DEC. The fetch copies what the upstream recipe copied and writes `D2XX/.gc-d2xx-complete` last; consumers gate on marker + version.
- Binding:
  1. A 403, partial cache or stale-version cache → refetch or build without D2XX (user-accepted 2026-09-29 for fork CI).
  2. Accepted false negative: a structurally valid FTDI archive lacking its own header is certified; the build then fails loudly at compile — same as the unmodified upstream recipe.
  3. No further content-recognition rounds on the vendor archive.
- Dependents: appveyor/{linux,macos}/{install,before_build}.sh, B-STAGE9-170, B-STAGE9-171, B-STAGE9-172
- Full record: this entry

## DEC-089 — fork CI recipe synced to upstream master; fork-only deltas re-applied on top
- Status: accepted 2026-09-30 (Inspector technical decision; architectural remedy, repair-round bound)
- Why: branch base 40db2bc8e predates 14 upstream CI commits; ci.1-ci.3 each hit a missing one (B-STAGE9-175/-176, 31b6685bf cache-on-error).
- Chose: B of 3 — (A) keep patching per CI run: unbounded runs; (C) merge all upstream/master: product blast radius. B = CI files only, recipe green upstream 2026-09-26.
- Binding:
  1. appveyor.yml and appveyor/** start from upstream/master content; only fork deltas with a ledger id are re-applied (Garmin payload DEC-058/-067/-069, DEC-088 D2XX gate, B-STAGE9-42/-56 pins, branch filter).
  2. An upstream CI step that needs source-tree files this branch lacks is adapted to this branch and reported, never silently dropped.
  3. The branch filter entry stays fork-only; revert before any upstream PR.
  4. Fork-only cache deltas (60-min hosted cap; cold Qwt build costs 14.5 min Win at -j1, 20.7 min macOS): qwt cache is the built `qwt/lib` only, content-keyed (a dependency key froze lib-less copies, B-STAGE9-186); srmio drops the `appveyor.yml` dependency; Windows Qwt stays -j1 (-j2 raced Debug/Release moc, B-STAGE9-184); SAVE_CACHE_ON_ERROR stays true until the first green run. Revert all before any upstream PR.
  5. Cold-Qwt cache priming (a hosted-cap timeout skips cache save, ci.4): if the Qwt library is absent at job start, every leg (Windows, Linux, macOS; was macOS-only until B-STAGE9-186) builds Qwt (macOS also SRMIO), prints an explicit "cache priming, intentional failure" line and fails with a non-zero command status before `sub-src` — never shell `exit`, which ends the AppVeyor session before cache save (B-STAGE9-185) — so save-on-error stores them. Fork-only; revert before any upstream PR.
- Dependents: appveyor.yml, appveyor/**, B-STAGE9-175, B-STAGE9-176, B-STAGE9-177
- Full record: this entry
