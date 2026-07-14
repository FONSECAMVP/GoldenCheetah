# Decision Ledger — Garmin Connect Integration

> Decision IDs in this project start at DEC-001. Independent of the prior `workflow/decisions.md` (AI Coach project). Cross-project refs use `coach:DEC-NNN`.

Compact schema per `references/formats.md` § `decisions.md`. The `state.md ## decs` table is the recap source; this file is drilled only when creating, editing, or cascading from a specific DEC. Detailed deliberation notes from the original v1 ledger are preserved verbatim in `decisions-history-archive.md`.

---

## DEC-001 — Solution shape: delivery sequence for bidirectional sync
- Status: accepted (B — staged ship)
- Reversibility: medium (filenames + Python-deps stick once users configure)
- Decided / last-reviewed: 2026-05-17
- Serves: user verbatim ask; constrained by A-03 (embedded Python), A-04 (per-athlete), A-05 (accept ToS risk); triggered by A0.3-001 (risk-adjusted MVP = download-only)
- Dependents: DEC-002, DEC-003, DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-009, DEC-010, DEC-011; all Phase-1 REQs; future Phase 2/3 workout-upload + schedule-push DEC families

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A monolithic single ship | 2 — ToS-risky writes ship with safer reads; can't roll back partial | 3 — surface locked in v1 | 2 — single big PR; long test cycles | 2 — violates A0.3-001 |
| B staged ship (download → upload → schedule) | 5 — each phase independently rollback-able; Phase 1 lowest ToS risk | 5 — each phase a separate DEC family; Phase 4 additive | 4 — three smaller PRs, reviewable in isolation | 5 — matches `GC_WANT_*` and Strava/Dropbox read-then-write precedent |
| C thin language adapter only | 3 — no CloudService isolation; callers see GC↔Python coupling | 2 — no UX surface; not discoverable | 3 — couples every caller to Python threading model | 2 — bypasses the CloudService pattern |

### Cascade impact (per option)
- A: Phase 1 PRD covers download+upload+schedule+auth at once; touches AI Coach (`coach:DEC-013`) day one; A4 stalls whole release if write paths fail.
- B (chosen): **Phase 1 = read-only `Cloud/GarminConnect.*`** (REQ-001…N); **Phase 2 = workout upload** (extends `coach:DEC-013`); **Phase 3 = schedule push** (`Train/Season*`); Phase 4 health metrics. Forces DEC-002 to ship a clean thread-isolation primitive reusable by Phases 2/3.
- C: no `Cloud/` file (would be `Python/GarminPython.{h,cpp}`); AI Coach tool gains direct Python calls; no Add-Cloud-Service wizard entry; feature invisible until each consumer surfaces it.

### Chosen
B — strategic scope is bidirectional; risk-adjusted MVP (A0.3-001) is read-only. Staging satisfies both by treating bidirectional as the goal and download-only as the v1 ship, with each capability as a discrete, gateable increment. Aligns with `coach:DEC-011`'s feature-flag precedent.

### Alignment probe
grep -E '^(if|elseif)\(GC_WANT_GARMINCONNECT\)' src/CMakeLists.txt | wc -l    # expect ≥1 block (sources gated by flag — confirms staging primitive in place)

---

## DEC-002 — Python integration mechanism (C++ → embedded Python)
- Status: accepted (B — worker thread + mailbox)
- Reversibility: medium (public API stable; worker-thread pattern sticky once tests rely on thread-id assertions)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-002, REQ-003, REQ-007, REQ-010, REQ-NF-Threads-001; constrained by existing `PythonEmbed` in `src/Python/`
- Dependents: DEC-007 (rate limiter at queue head); DES-001, DES-009, DES-010; REQ-NF-Threads-001/Cancel-001/Perf-002/Perf-003

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A direct call from UI thread | 2 — 10s-class latencies freeze UI | 1 — single in-flight | 4 — simplest | 1 — violates NF-Threads-001 |
| B worker thread + mailbox | 5 — off-UI; cancellation is a queue op | 5 — queue depth tunable; rate limiter at queue head | 3 — needs queue + signals + lifecycle | 5 — Qt signal-slot canonical |
| C thread-per-request | 3 — thread spawn OK; GIL contention high | 3 — GIL serialises anyway | 2 — per-call bookkeeping fragile | 3 — works but overkill |

### Cascade impact
- A: disqualified (would force removing NF-Threads-001).
- B (chosen): forces `GarminWorker : QObject` in `QThread`, Qt-signal `request()`/`finished()` API. Phase 2 tests assert thread-id at every Python call site. DEC-007 sits at queue head. **Sets the template Phase 2 (uploads) and Phase 3 (schedule) reuse.**
- C: per-call thread bookkeeping; no win over B.

### Chosen
B — anchors NF-Threads-001 directly and gives Phase 2/3 a reusable transport.

### Alignment probe
grep -rn 'QThread\|QObject' src/Cloud/Garmin* | wc -l   # expect ≥1 (worker subclass + QThread harness)

---

## DEC-003 — Token + sidecar on-disk layout
- Status: accepted (B — per-athlete config dir)
- Reversibility: low-medium (paths sticky after users have token files)
- Decided / last-reviewed: 2026-05-17 (path-convention refinement 2026-05-17 post-A2)
- Serves: REQ-004, REQ-006, REQ-008, REQ-010, REQ-NF-Reliab-002, REQ-NF-Sec-002/004
- Dependents: REQ-004, REQ-006, REQ-008, REQ-010, REQ-012; DEC-002 (worker uses the writer); DES-002, DES-006, DES-009, DES-010

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A library default (`~/.garminconnect/tokens.json`) | 3 — global file; multi-athlete collision | 1 — broken >1 athlete | 4 — least code | 1 — violates A-04 |
| B per-athlete dir + dedicated subfolder | 5 — fully isolated; tmp+rename trivial | 5 — N athletes scale | 4 — explicit paths; easy Disconnect cleanup | 5 — matches Strava precedent |
| C inside GC's QSettings INI | 2 — INI not designed for blobs + tmp+rename | 3 — cramps the INI | 2 — values not human-greppable | 2 — mixes secrets with app settings |

### Cascade impact
- B (chosen): defines exact paths used by every subsequent REQ. Token: `<athlete-dir>/garminconnect/tokens.json`. Sidecar dedup: `imported-<garmin_user_id>.json`. Resumable backfill state: `backfill-state-<garmin_user_id>.json`. All written via tmp+rename helper from DEC-002's worker.

### Chosen
B — aligns with Strava precedent; isolates per-athlete state per A-04. Post-A2 refinement: sidecar files partitioned per Garmin account ID (A2-006 Option C); `tokens.json` singular. Sidecar **format** in Phase 1 is JSON; Phase-1.5 cascade trigger to sqlite if beta observability shows sidecar-read-time > 500ms or entry-count > 10,000.

### Alignment probe
grep -nE 'tokens\.json|imported-.*\.json|backfill-state-.*\.json' src/Cloud/Garmin* src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 (paths reference per-athlete subfolder, never library default)

---

## DEC-004 — Credentials + MFA dialog UX shape
- Status: accepted (B — `AddCloudWizard` pages)
- Reversibility: high (page replacement is local)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-002, REQ-003, REQ-009, REQ-014, REQ-015
- Dependents: REQ-002, REQ-003, REQ-009, REQ-014, REQ-015; DES-003, DES-008, DES-011; AddCloudWizard infrastructure

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A blocking modal `QDialog`s | 4 — simplest cancel | 4 — new step = new dialog | 4 — self-contained | 3 — modal stack feels clunky |
| B `AddCloudWizard` pages | 5 — wizard handles cancel/back/forward | 5 — page = subclass; framework handles flow | 5 — matches GC convention | 5 — Strava/Dropbox precedent |
| C non-modal floating widget + state machine | 2 — modeless flows bug-friendly | 3 — state machine grows fast | 2 — non-modal hard to debug | 2 — anti-pattern for credentials |

### Cascade impact
- B (chosen): two new `QWizardPage` subclasses (`GarminCredentialsPage` + `GarminMfaPage`); CAPTCHA page surfaced only on detection. Reuses `AddCloudWizard` Back/Next/Cancel/Help machinery.

### Chosen
B — matches Strava/Dropbox precedent; reuses Qt wizard infrastructure.

### Alignment probe
grep -rn 'QWizardPage' src/Cloud/Garmin* 2>/dev/null | wc -l   # expect ≥2 once Phase 2.2 wizard slice ships (credentials + MFA pages)

---

## DEC-005 — Phase-1 CloudService capabilities
- Status: accepted (recording-only; alternatives don't apply — any other value contradicts REQ-011)
- Reversibility: high (Phase 2 flips in `Upload`)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-011
- Dependents: REQ-011; AddCloudWizard UI logic; DES-004 (`capabilities()` return)

### Chosen
`Query | Download` only. No `Upload`, `Sync`, `OAuth` (library SSO is used, not GC's generic OAuthDialog), no `Delete`.

### Alignment probe
grep -nE 'capabilities.*Query.*Download' src/Cloud/GarminConnect.cpp 2>/dev/null | wc -l   # expect 1 (exact bitmask) — TEST-001 mutants #2/#3 also gate this

---

## DEC-006 — Activity file format + staging path
- Status: accepted (recording-only)
- Reversibility: medium (filename pattern sticky if anyone scripts against it)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-007, REQ-008
- Dependents: REQ-007, REQ-008; DES-004, DES-010

### Chosen
FIT preferred (`download_activity(activity_id, dl_fmt=ORIGINAL)` — gives FIT for nearly all activities). TCX fallback only if Garmin returns a non-FIT original. Staging path: GC's existing import staging dir, filename `garmin-<activity_id>.<ext>` to avoid collisions.

### Alignment probe
grep -nE "dl_fmt|ORIGINAL|garmin-.*\.fit" src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 when REQ-007 lands

---

## DEC-007 — Rate-limit + retry placement
- Status: accepted (B — Python-side decorator inside worker thread)
- Reversibility: high (limiter file is local)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-NF-Perf-002, REQ-NF-Reliab-001
- Dependents: REQ-NF-Perf-002, REQ-NF-Reliab-001; sits inside DEC-002's worker; DES-005, DES-012

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A C++ limiter wrapping Python entry-points | 3 — every site must remember to wrap | 3 — N entry points | 3 — split brain | 3 — limiter near consumer |
| B Python-side decorator | 5 — single chokepoint at Python boundary | 5 — one decorator, all calls | 4 — testable in isolation | 5 — canonical |
| C trust `python-garminconnect` | 1 — library has no rich rate limiter | N/A | 2 — opaque external dep | 1 — assumes external behaviour |

### Cascade impact
- B (chosen): a small `gc_rate.py` module installed alongside the library, exposing decorator + retry helper. Worker (DEC-002) imports and applies to each library method. Tests assert call timing via fake clock.

### Chosen
B.

### Alignment probe
grep -rn '@.*rate_limit\|@retry\|token_bucket' src/Python/garminconnect/ 2>/dev/null | wc -l   # expect ≥1 when DES-005 lands

---

## DEC-008 — Testing toolchain
- Status: accepted (A — extend incumbent; QTest+CTest C++ / pytest+coverage.py Python)
- Reversibility: high for Python; medium for C++ (QTest macros sprinkled)
- Decided / last-reviewed: 2026-05-17
- Serves: every Phase 2.2 TEST-NNN
- Dependents: every Phase 2.2 TEST-NNN; DES-001 (signal assertions), DES-005 (timing tests), DES-006 (atomic-write race tests), DES-012 (adapter contract tests); DEC-009; DEC-010

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A QTest+CTest C++ / pytest+coverage.py Python | 5 — both battle-tested; QSignalSpy native; pytest+freezegun canonical | 5 — same pattern coach tests use; Python scales by file | 5 — zero novelty for C++ devs | 5 — matches `unittests/Core/coach/` precedent + Python default |
| B GoogleTest/Catch2 C++ + pytest Python | 3 — GoogleTest lacks Qt signal/event-loop support; needs adapter | 3 — duplicate CMake helpers | 2 — first non-QTest framework here | 3 — industry-standard but not Qt-industry |
| C QTest only (subprocess pytest shims) | 2 — flaky exit-code plumbing | 2 — every Python test needs C++ harness | 2 — debug = parse stdout out of C++ log | 1 — anti-pattern |

### Cascade impact
- A (chosen): C++ REQs go in `unittests/Core/garminconnect/test*.cpp` using `QTest` macros + `QSignalSpy`; Python REQs in `src/Python/garminconnect/tests/test_*.py` using pytest + freezegun + pytest-cov. Coverage: gcov/lcov (C++), coverage.py (Python). CI fails on coverage-delta-drop for **changed files only**. Tests gated behind `GC_WANT_GARMINCONNECT`.

### Chosen
A — matches Qt convention + Python community default; zero new framework.

### Alignment probe
test -f unittests/Core/garminconnect/CMakeLists.txt && test -f src/Python/garminconnect/pyproject.toml && echo OK   # expect "OK"

---

## DEC-009 — Style/quality toolchain
- Status: accepted (A — existing `.clang-format`/`.clang-tidy` + `ruff` + `mypy --strict` scoped to new Python)
- Reversibility: high (single file config)
- Decided / last-reviewed: 2026-05-17
- Serves: every Phase 2 COMMIT
- Dependents: every Phase 2 COMMIT; DES-005 + DES-012 (type-checked); DEC-010 (which tools the pre-commit hook invokes); future i18n string-extraction

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A clang-format/-tidy + ruff + mypy --strict (scoped) | 5 — ruff covers pyflakes/pycodestyle/isort/bugbear; mypy catches DES-012 drift | 5 — ruff ~10–100× faster; matters for pre-commit | 5 — one `pyproject.toml` configures lint+format | 5 — 2025 community default |
| B clang-tools + black+flake8+isort + mypy | 4 — same coverage, three tools to align | 3 — three subprocess invocations | 2 — version-skew across three packages | 3 — established but legacy |
| C clang-tools + black only | 2 — uncaught name typos in adapter | 5 — fastest because does the least | 4 — least config; trades into bug cost | 1 — below industry hygiene bar |

### Cascade impact
- A (chosen): adds `pyproject.toml` scoped to `src/Python/garminconnect/`. Configures `ruff` (default + B + E/F/I) and `mypy --strict` on the same path. C++: zero new config — new files under `src/Cloud/Garmin*` + `unittests/Core/garminconnect/` picked up by existing `.clang-format` + `.clang-tidy`. CI runs the tools on changed files only. **Scope is narrow** — does not retrofit the rest of `src/Python/` (legacy chart scripts).

### Chosen
A.

### Alignment probe
grep -E 'strict.*=.*true' src/Python/garminconnect/pyproject.toml | wc -l   # expect 1 (mypy --strict configured)

---

## DEC-010 — Pre-commit automation
- Status: accepted (A — `pre-commit` framework, scoped)
- Reversibility: high (`.pre-commit-config.yaml` is one file)
- Decided / last-reviewed: 2026-05-17
- Serves: Phase 3.1 "fast local feedback"
- Dependents: every Phase 2 COMMIT; DES-007 (CMake flag — hook scope follows path set); CI pipeline (Phase 3.3); commit-message format

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A `pre-commit` framework + `.pre-commit-config.yaml` | 5 — versioned, pinned hook revisions; local+CI parity | 5 — adding a check = one YAML stanza | 5 — `pre-commit autoupdate`; well-documented | 5 — current OSS default for mixed-lang repos |
| B in-repo shell script via `core.hooksPath` | 3 — relies on PATH; silent version drift | 3 — script grows hand-maintained diff parsing | 3 — contributors must discover/enable | 3 — works but bespoke |
| C no local hook; CI only | 2 — bad code in history; bisect polluted | 5 — scales because does nothing | 4 — nothing to maintain locally; PR latency burns time | 1 — violates "seconds-not-minutes" |

### Cascade impact
- A (chosen): adds `.pre-commit-config.yaml` running on **staged files only**: `clang-format --dry-run --Werror`, `ruff check --fix` + `ruff format`, `mypy` (touched-files), `detect-secrets`, fast unit-test subset (CTest label `garmin-fast` + the Python tests). Onboarding: `pre-commit install` once. CI runs the same config via `pre-commit run --all-files` for the new paths. Hook **does not touch legacy** (scoped via `files:` regex).

### Chosen
A — standard tool, local/CI parity, scope-narrowing supported; catches secrets at commit time (DEC-003's threat model).

### Alignment probe
test -f .pre-commit-config.yaml && grep -E 'detect-secrets|ruff|clang-format' .pre-commit-config.yaml | wc -l   # expect ≥3

---

## DEC-011 — Phase-1 rollout strategy
- Status: accepted (recording-only)
- Reversibility: high (flag flip per-build)
- Decided / last-reviewed: 2026-05-17
- Serves: REQ-NF-Build-001, REQ-NF-Pkg-001
- Dependents: REQ-NF-Build-001, REQ-NF-Pkg-001; entire Phase-1 feature; DES-007 (CMake option + installer manifest delta)

### Chosen
New CMake option `GC_WANT_GARMINCONNECT`, default **OFF** until A4 passes. Mirrors `GC_WANT_COACH` (`coach:DEC-011`). When ON: build adds `src/Cloud/GarminConnect.{h,cpp}` + Python worker; installer bundles `garminconnect` + `curl_cffi`. When OFF: no Python network deps required.

### Alignment probe
grep -nE 'GC_WANT_GARMINCONNECT' src/CMakeLists.txt | wc -l   # expect ≥2 (option declaration + status line)

---

## DEC-012 — Auth-dispatcher seam (page ↔ SSO layer)
- Status: accepted (A — `IGarminAuthClient` interface, injected via constructor)
- Reversibility: high (replacing with signal-only shape is a constructor + WorkerAuthClient adapter refactor)
- Decided / last-reviewed: 2026-05-24
- Serves: REQ-002 (wizard-side acceptance), REQ-003 (MFA), REQ-005 (wizard-side enforcement), REQ-015 (CAPTCHA)
- Dependents: TEST-003 (encodes the interface in its fake); DES-001 (concrete `WorkerAuthClient` adapter added in REQ-002 GREEN); DES-003 (credentials-page seam); DES-003a (interface spec); REQ-003 + REQ-015 slices; `src/Cloud/IGarminAuthClient.h`; `unittests/Core/garminconnect/CMakeLists.txt`

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A inject `IGarminAuthClient` interface | 4 — compile-enforced; impossible to forget wiring | 4 — same interface extends trivially to MFA/CAPTCHA | 4 — small header; explicit seam visible at call site | 5 — mainstream DI; aligns with `CloudService` pure-virtual contracts |
| B page emits `submitRequested`; wizard glues to worker | 3 — signal/slot wiring is runtime; typo silently no-ops | 3 — MFA/CAPTCHA each need own signal taxonomy | 3 — logic spread across page + wizard glue | 4 — idiomatic Qt; matches existing `AddAuth` informal pattern |
| C page holds `GarminWorker*`; `enqueue()` virtual for tests | 2 — drags Python-embed headers into page build via `GarminWorker.h` | 2 — same coupling to every page; anti-cascade if DES-001 changes | 2 — two roles in one type; production deps leak into tests | 2 — concrete-class injection for test-only is a smell |

### Cascade impact
- A (chosen): refines DES-003 (credentials-page takes `IGarminAuthClient*` via ctor); new sub-section DES-003a specifies the pure-virtual surface + value-type result/error shapes. Refines DES-001 (worker ships `WorkerAuthClient : IGarminAuthClient` adapter forwarding `authenticate(...)` to `enqueue(Authenticate{...})` and re-emitting `finished`/`error` through the interface's signals). New file `src/Cloud/IGarminAuthClient.h` — compiles with `GC_WANT_GARMINCONNECT=OFF` (header-only contract). Unlocks REQ-003 (MFA) and REQ-015 (CAPTCHA) page tests: interface gains `mfaRequired(QUuid)` + `captchaDetected(QUuid)` when those slices land. No change to DEC-002/005/008.

### Chosen
A — only option giving compile-time guarantees AND keeping `garmin-fast` CTest Python-free AND naturally extending to MFA/CAPTCHA pages with no rework.

### Alignment probe
test -f src/Cloud/IGarminAuthClient.h && grep -nE 'class\s+IGarminAuthClient' src/Cloud/IGarminAuthClient.h | wc -l   # expect 1 when DES-003a GREEN lands

---

## DEC-013 — Worker ↔ Python adapter seam (page-side mirror of DEC-012, one layer down)
- Status: accepted (A — `IGarminPyAdapter` interface, injected into `GarminWorker`)
- Reversibility: high (swap to virtual `doAuthenticate()` or direct call is a header + worker-ctor refactor)
- Decided / last-reviewed: 2026-05-24
- Serves: REQ-002 (end-to-end Authenticate testable in C++ without spinning the embedded Python sub-interpreter); future REQ-003/006/007/010/012/014 worker-side ops (each adds a method to the same interface)
- Dependents: TEST-004 (will inject `FakePyAdapter` to drive worker behaviour); DES-001 (worker takes `IGarminPyAdapter*` via ctor); new DES-001a (interface spec, mirror of DES-003a); REQ-002 end-to-end slice; `src/Cloud/IGarminPyAdapter.h`; `src/Cloud/GarminWorker.{h,cpp}`

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A inject `IGarminPyAdapter` interface (mirror of DEC-012) | 5 — compile-enforced seam; tests cannot accidentally hit real Python | 5 — every future worker op (List, Download, Refresh, …) is one method on the same interface | 5 — small header; symmetry with DEC-012 reduces cognitive load | 5 — DI mainstream; matches the page-side pattern already accepted |
| B virtual `doAuthenticate()`; tests subclass `GarminWorker` | 3 — protected-virtual leak; tests reach into worker internals | 3 — every new op needs its own virtual + test subclass copy | 3 — two roles in one type; test-only ctor risk | 3 — viable but inferior to a real seam |
| C `#ifdef TESTING` swap real Python call for fake | 1 — production vs test code paths diverge by macro; ships test code by accident risk | 2 — every op needs its own #ifdef branch | 1 — macro pollution; review-resistant | 1 — anti-pattern |

### Cascade impact
- A (chosen): adds `src/Cloud/IGarminPyAdapter.h` — pure-virtual header-only contract compilable with `GC_WANT_GARMINCONNECT=OFF` (mirrors `IGarminAuthClient.h`). Refines DES-001: `GarminWorker(IGarminPyAdapter* py, QObject* parent=nullptr)` — worker no longer owns the embedded-Python sub-interpreter directly; the concrete `PyEmbeddedAdapter : IGarminPyAdapter` (production) does. Adds new sub-section DES-001a specifying the interface surface (mirror of DES-003a). REQ-002 GREEN uses `FakePyAdapter` in C++ tests; the real `PyEmbeddedAdapter` is exercised at the Python boundary by existing `test_adapter_login.py`. No change to DEC-002/004/012.

### Chosen
A — only option that gives compile-time guarantees, preserves the `garmin-fast` CTest Python-free invariant for the worker tests, and mirrors DEC-012 so every layer of the dispatch chain uses the same DI pattern. Future REQ slices (List/Download/Refresh/Profile/Disconnect) extend the interface additively with no test-shape rework.

### Alignment probe
test -f src/Cloud/IGarminPyAdapter.h && grep -nE 'class\s+IGarminPyAdapter' src/Cloud/IGarminPyAdapter.h | wc -l   # expect 1 when DES-001a GREEN lands

---

## DEC-014 — Token persistence: write-ownership + permission-enforcement seam
- Status: accepted (B — adapter exports blob via dumps()/loads(); C++ owns the atomic 0600 write)
- Reversibility: medium-expensive (not one-way; switching to A/C means reverting DES-013 forwarding + migrating or force-re-SSO existing athletes' blobs — no component becomes unbuildable)
- Decided / last-reviewed: 2026-07-11
- Serves: REQ-004, REQ-005, REQ-006, REQ-NF-Reliab-002, REQ-NF-Sec-002, REQ-NF-Sec-004; constrained by DEC-003 (per-athlete singular tokens.json, JSON), REQ-NF-Sec-004 (file-based Phase 1; keychain is Phase-1.5 non-goal)
- Dependents: DES-002 (confirmed literal — NO amendment), DES-006 (AtomicFile used as designed), DES-012 (+dump_tokens/load_tokens), DES-013 (stops forwarding tokenstorePath; exports blob post-login), REQ-004 + REQ-006 build slices
- Research: qgdw-scout 2026-07-11 (web-sourced; garth deprecated/unmaintained, python-garminconnect migrated to native OAuth engine, single `garmin_tokens.json`, in-memory `dumps()`/`loads()` API). Orchestrator Verification Gate: contract+goal PASS; evidence PASS with caveat — OQ1 below (library string-API method name unconfirmed against a repo pin; none exists, lib not installed in .venv).

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A library writes, GC post-hardens | 2 — 0644 race window before chmod; non-atomic first write | 3 — fixed post-login cost | 2 — two write paths; depends on lib file-count staying single | 2 — "write insecure then fix perms" anti-pattern for secrets |
| B adapter exports blob, C++ owns atomic write | 5 — single tmp+fsync+rename at 0600, no exposure window | 4 — O(1) per athlete | 3 — new adapter surface kept in sync with lib string API | 5 — textbook seam; makes DES-002 literally true |
| C directory-level enforcement only | 3 — no race but no atomicity; torn-write detect over unknown file set | 3 — enumerate-dir blast radius | 2 — on-disk shape floats under GC across lib releases | 2 — externally-controlled layout as stored contract |

### Cascade impact
- B (chosen): **DES-002 needs NO amendment** (singular tokens.json / atomic / 0600 / lib-default-path-never-used all become literally true). DES-006 AtomicFile used exactly as designed. DES-012 `garmin_client.py` gains `dump_tokens() -> str` + `load_tokens(token_str: str)`; `login()` stops forwarding a tokenstore path (auth-only, in-memory session) → +2 pytest (round-trip; reject-tampered-blob) + matching pystub methods. DES-013 `PyEmbeddedAdapter` stops forwarding `tokenstorePath` to `GarminClient(...)`; surfaces the blob post-login (new `PyAuthOutcome.tokenBlob` on Success) so the worker/CloudService persists it via AtomicFile; `load_tokens(blob)` on resume replaces password login (the mechanism that lets the retained `m_client` skip SSO). REQ-006 load-side mode check gates the single `load_tokens()` call. garmin-fast stays Python-free (C++ tests hit the pystub only; the 2 new pytests run in the Python lane).

### Open questions
- OQ1 (exact string-API method names on the *bundled* library version) — CARRIED to build as a NOTE: builder implements against the pystub/fake contract (`dump_tokens`/`load_tokens`); real-library method-name wiring is confirmed when the wheel is bundled (DES-007 / REQ-NF-Pkg-001, already deferred). NOT GREEN-blocking for the C++/fake seam.
- OQ2 (load_tokens() failure when Garmin invalidated the session server-side) — RESOLVED: routes to REQ-NF-Compat-001(b) "prompt full re-login, no silent reauth" with a DISTINCT error kind (`session_expired`) so DES-008 messages it differently from REQ-006's permission-rejection. Both end in fresh SSO; the surfaced message differs.

### Alignment probe
grep -nE 'dump_tokens|load_tokens' src/Python/garminconnect/garmin_client.py | wc -l   # expect ≥2 when REQ-004/006 GREEN lands

---

## DEC-015 — Ledger status: single canonical source + absence-check drift lint (LSN-008 promotion-to-mechanism)
- Status: accepted (C — normalize per-id status to ONE canonical home per id-class AND add a thin absence-check lint as regression backstop; sub-decision: collapse to a single live cursor by deleting the local `state.md`)
- Reversibility: medium (a schema/layout change to the governance tree; reverting means re-scattering status back across files — no artifact becomes unusable, but the migration edits every status-bearing governance file once)
- Decided / last-reviewed: 2026-07-12
- Serves: retiring LSN-008 (op:ledger-update / index-vs-detail-drift, recur:7 miss:6 — DEFINITIVE promotion trigger recorded across 7 loci); the workflow's own maintainability. Constrained by Principle 9 (update-as-byproduct), Principle 10 (promotion-to-mechanism), the wiki-first orientation contract (root STATE.md is the file read first each session), and user execution conditions (2026-07-12): lint-FIRST, explicit code-level scope, install via install_hook.py, vocabulary+state-machine into wiki/conventions.md, full CLV to close.
- Dependents: `traceability.md` (becomes canonical status ledger), `decisions.md` (canonical DEC-status), `design.md` (status stripped → intent tense), root `STATE.md` (cursor only, per-id status removed), `WIKI.md` REGISTRIES (ranges+next only), local `.claude/workflow-garminconnect/state.md` (DELETED), `wiki/conventions.md` (+vocabulary/state-machine), `scripts/ledger_drift_lint.py` + `install_hook.py` (new lint + wiring), `lessons.md` (LSN-008 promoted, LSN-014 portable capture). Allocated build id: TEST garmin-T-017 (lint's own test suite).
- Research: none dispatched — trade space is fully internal (our own ledger schema, read directly this session); constraints determine the options, so drafted inline per the Three Options Doctrine (no qgdw-scout).

### The problem
Each id's lifecycle status is ONE logical fact stored in 7–8 physical places; every byproduct update must hand-sync them and context loss between turns makes that lossy. The manual CHECK-list control was broadened 4× and still missed — enumerating loci and parsing free-prose status is the fragile part. The seven recorded loci (VAL-007…VAL-011):
1. `traceability.md` PRIMARY REQ→DEC→DES→TEST→COMMIT matrix rows (status strings + Commit column)
2. `traceability.md` DEC index + DES index Status cells (freeform status prose)
3. `traceability.md` per-slice appendix "artifacts" tables (`| Artifact | Path | Serves |`, carry "DONE/still forwards/GREEN" status language)
4. local `.claude/workflow-garminconnect/state.md` structured tables (`## reqs/des/vals/last-clv/open`)
5. root `STATE.md` body (CASCADE/NEXT_GATE/CHANGESET/COUNTS/VAL-registry) drifting vs its OWN banner (intra-file)
6. `design.md` prose (DES bodies + "DEC-NNN refinement"/"what this does NOT cover" notes + ctor code snippets + "Serves:"/"### Surface")
7. `WIKI.md` REGISTRIES per-id status strings ("in build"/status) + phase banner

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A pre-CLV/pre-commit drift lint only (no schema change) | 3 — deterministic catch but only for enumerated loci (same completeness gap that failed 4×); detects AFTER drift, repair can re-drift (VAL-011 passes 1-3) | 2 — must parse open-ended free-prose status (design.md); brittle N-way agreement grows with ledger | 3 — one isolated script, but perpetually chasing new status phrasings | 2 — band-aid over duplicated data; DRY violation intact |
| B normalize to one canonical home per id-class (strip/derive elsewhere) | 5 — status stored once CANNOT drift; bug class eliminated | 5 — new id = one row in one place; dual-cursor maintenance gone | 4 — sharp long-term drop; one-time migration is a large multi-file edit needing its own validation | 5 — textbook SSOT + design-intent/live-status separation |
| C normalize AND thin absence-check lint (chosen) | 5 — B's elimination + guards against a context-lost turn re-adding status prose (this project's exact failure mode); lint is an ABSENCE check with no vocabulary-completeness dependency | 5 — B's scaling + lint is a trivial scoped grep | 4 — B's cascade + ~30-line near-zero-upkeep lint | 5 — SSOT + a deterministic guard encoding the invariant, surviving agent/context turnover a manual CHECK-list provably does not |

### Cascade impact (per locus; the migration)
Canonical homes (the ONLY files where an id may be paired with a lifecycle-status token): **`traceability.md`** for REQ/DES/TEST/VAL status; **`decisions.md`** per-DEC `Status:` for DEC status. All other live-governance files carry references WITHOUT status tokens.
- Locus 1 → stays canonical; status strings normalized to the controlled vocabulary; Commit column = git provenance (kept).
- Locus 2 → DES index Status cells canonical, controlled vocabulary only (no freeform prose); traceability DEC-index Status column DROPPED (drill `decisions.md` for DEC status) to avoid DEC-status duplication.
- Locus 3 → appendix "artifacts" tables reclassified as immutable dated **provenance**, marked `<!-- provenance: dated; excluded from status-lint -->`; live-status verbs stripped (they describe what was built at a commit, not current state).
- Locus 4 → local `state.md` **DELETED**; all pointers (WIKI PAGES, workflow-INDEX, any "full live cursor" refs) repointed to root `STATE.md` as the sole cursor.
- Locus 5 → root `STATE.md` becomes the lean **cursor**: phase, TEAM, RIGOR, current gate/focus, open findings (by id, no status token), CHANGESET (commit hashes = provenance), pointers to traceability/validations. Per-id GREEN/committed/CLOSED enumeration removed; COUNTS rollup dropped (derivable from traceability).
- Locus 6 → `design.md` status assertions stripped → design-INTENT tense (target shape, not "DONE"/"deferred"); [[LSN-011]] sibling (design-note false-done) subsumed.
- Locus 7 → `WIKI.md` REGISTRIES = ranges + `next` only; phase banner names the phase, not per-id status.

### The invariant (what the lint enforces) — condition 1
Absence check, not agreement check. **The canonical ledgers `traceability.md` and `decisions.md` are the SOURCE OF TRUTH and are fully exempt (not scanned)** — they legitimately co-locate ids with status (the REQ→DEC→DES→TEST matrix cites a REQ's governing DECs/DES/TESTs on the same row as the REQ's status; DEC cascade-notes reference the REQ/DES a decision affects). The lint enforces status-ABSENCE only in the **non-canonical governance set**: root `STATE.md`, root `WIKI.md`, `wiki/*.md`, and each ledger's `design.md`. In those files no id token (`REQ-\d{3}`, `REQ-NF-\w+-\d{3}`, `DES-\d{3}[a-z]?`, `TEST-\d{3}`/`T-\d{3}`, `VAL-\d{3}`, `DEC-\d{3}`) may appear on the same line as a lifecycle-status token (`GREEN|CLOSED|DEFERRED|deferred|drafted|in build|_pending_|_uncommitted_`; "committed" excluded — commit-hash provenance entanglement; "in progress" dropped during migration — collides with the spec'd REQ-NF-Perf-002 "sync already in progress" message and is not in the controlled vocabulary). Explicit code-level exclusions: `archive/`, `cycles/`, `validations/`, `lessons.md`, `findings.md`, and any block after a `<!-- provenance: dated ... -->` marker (runs to the next `## ` heading). Runs pre-commit (framework entry scoped to ledger paths) + as a CLI the CLV step invokes. Installed via `install_hook.py` alongside `anti_duplication_guard.py`.
- *Refinement note (2026-07-12, discovered by running the built lint against the real tree):* the original briefing's "foreign-class catch" (flag DEC+status inside traceability.md) was dropped — it false-positived on legitimate traceability cross-citation. Correct rule = exempt the canonical ledgers entirely; the real duplication loci (STATE/WIKI/design/wiki-spokes + the deleted local state.md) are all still covered.

### Controlled vocabulary + state machine (→ wiki/conventions.md, condition 3)
Per-id lifecycle: `drafted → GREEN → committed → CLOSED`, with `deferred` as an orthogonal tag on a not-yet-built slice. (DEC-status stays `accepted (X)` in decisions.md.) One vocabulary, re-counted from source, never copied between docs.

### Execution order (user conditions 1–2, 6) & acceptance
1. Build the lint FIRST (TDD, qgdw-builder, TEST-017) with the scope above.
2. Wire + install via install_hook.py.
3. Run the migration (orchestrator = single writer of governance files); acceptance = **lint passes tree-wide**.
4. Full 9-check CLV (qgdw-validator); on PASS, close LSN-008 as promoted-to-mechanism and capture scope:portable LSN-014.

### Open questions / risks
- OQ1 — STATE.md is read first each session and must stay a *useful* cursor after status is stripped; risk it becomes too thin to orient. Mitigation: it keeps the current-gate + changeset + open-findings (the volatile cursor facts that legitimately live only here), and points to traceability for per-id status. Validate during migration that a cold session-start still orients from it.
- OQ2 — the migration edits every status-bearing file once; drift can be *introduced* during the cut (ironic). Mitigation: lint is the acceptance gate (must be GREEN tree-wide before commit) + closing full CLV.
- OQ3 (RESOLVED by the corrected invariant, see Refinement note above) — the appendix provenance tables live in `traceability.md`, which the corrected lint exempts UNCONDITIONALLY (canonical source-of-truth, never scanned). So the `<!-- provenance: dated -->` marker is NOT needed there and none were added — no false-positive risk exists for those tables. The marker mechanism still applies to any provenance block that appears in a SCANNED file (STATE.md / WIKI.md / wiki/* / design.md), where it exempts the block up to the next `## ` heading.

### Alignment probe
python3 .claude/skills/quality-gated-dev-workflow/scripts/ledger_drift_lint.py .   # expect exit 0 (no status token outside canonical homes) once migration lands
