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
- Committed: `88d4ea402` (lint mechanism + TEST-017 + install/pre-commit wiring + gitignore) · `2520ed034` (ledger normalization migration + local state.md deletion + WIKI/STATE/conventions). VAL-012 PASS.
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
python3 scripts/ledger_drift_lint.py .   # expect exit 0 (no status token outside canonical homes) once migration lands
# Path corrected 2026-08-05 (ORCH-004): the lint's canonical source moved OUT of the skill tree to the
# project-owned `scripts/` this DEC's Dependents line specified all along; .claude/hooks/ holds the synced
# copy pre-commit + CLV invoke. The skill tree is vendor territory, replaced wholesale on every update.

---

## DEC-016 — FIT→TCX fallback-trigger contract for REQ-007 readFile (PRD Assumption-B resolution)
- Status: accepted (C — hybrid: kind-aware TCX retry + content-sniff backstop; RateLimited fails fast)
- Reversibility: cheap (internal adapter/readFile control-flow only; no persisted state, no new public interface, no schema commitment — a future DEC can revise the retry table with no migration if live Garmin testing shows different behaviour)
- Decided / last-reviewed: 2026-07-14
- Serves: REQ-007 (activity download → FIT/TCX pipeline); resolves PRD Assumption-B (Garmin's server-side signal for a FIT-less activity is unvalidated from local source — real `python-garminconnect` not installed, only stubs). Constrained by DEC-006 (FIT default, TCX fallback — non-negotiable), DEC-002 (GarminWorker is sole Python caller; trigger must be observable at the C++/adapter seam), LSN-006 (classify by exception TYPE, never leak raw library kinds).
- Dependents: DES-004 (readFile fallback decision-table + the ZIP-unwrap step — see OQ2); REQ-007 closure build (worker-in-CloudService lifecycle + readFile). Adapter `download_activity` and the `GarminDownloadFailure` kind enum are UNCHANGED under Option C (lower cascade footprint than A). Allocated build ids assigned at REQ-007 build-brief (TEST garmin-T-018+).
- Research: qgdw-scout dispatched (2026-07-14) — upstream landscape volatile (third-party lib, `garth` dep dropped + deprecated within the year), so web research per the doctrine rather than inline. Draft verified through the Verification Gate (three real options, dated sources on volatile claims, concrete cascade) before presentation.

### The problem
`readFile` requests the FIT (`ORIGINAL`) download and needs an OBSERVABLE condition to decide "no FIT here → retry as TCX." Corroborated evidence (two independent long-running export tools hitting the same Garmin endpoints) says a missing FIT original surfaces as an **HTTP 404**, which `python-garminconnect` collapses into its generic `GarminConnectConnectionError` (its public surface has only 4 exception classes — no not-found/format-specific type). So today's adapter already conflates "no FIT" (404) with "network died" under kind=`connection`/Network. The exact status→exception mapping could NOT be verified from source (real lib not installed; upstream fetch truncated) — this residual uncertainty IS Assumption B, and it decides which option is safe.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A new `NotAvailable` kind from adapter exception-cause inspection | 2 — depends on undocumented internals of a lib whose HTTP stack churned 2× in a year; silently fails to fall back if status isn't exposed | 4 — precise kind, no wasted retries | 2 — brittle; needs live-404 fixture re-validation each version bump; hard to unit-test w/o mocking lib internals | 4 — most directly implements LSN-006 IF internals cooperate |
| B C++-side content-sniff of returned bytes only | 2 — blind to the 404-EXCEPTION branch, which is the corroborated real trigger; only catches empty/wrong-content 200s | 4 — negligible CPU, no extra retries on exception branch | 5 — fully offline-testable with static byte fixtures, zero upstream-internals dependency | 2 — contradicts LSN-006 for the branch that actually fires; leaves 404 case unmet |
| C hybrid: kind-aware retry + content-sniff backstop (chosen) | 5 — every point in the (a)–(d) uncertainty space lands in a branch we already retry on; excludes RateLimited where auto-retry is harmful | 4 — bounded to one extra retry; RateLimited exclusion prevents throttle amplification at batch volume | 4 — more branches but each independently testable; retry table stated in one place | 3 — documented, bounded deviation from strict LSN-006 (Network kind stays coarse), not a silent violation |

### The decision table (what readFile does)
```
readFile(activityId):
  request ORIGINAL (FIT) via worker → await downloaded / downloadFailed
  on downloadFailed(kind):
      kind == RateLimited → FAIL fast (surface error; NO retry — anti retry-storm)
      kind == Network     → retry once as TCX
      kind == Unknown     → retry once as TCX
  on downloaded(bytes):
      unzip (ORIGINAL is ZIP-wrapped — see OQ2); sniff FIT magic (".FIT" @ offset 8)
      is FIT  → stage as garmin-<id>.fit
      not FIT → retry once as TCX
  TCX retry result → stage as garmin-<id>.tcx (or surface failure)
```
LSN-006 deviation (documented): the `Network` kind legitimately conflates a true network failure with a 404 "no original," because the library's coarse exception taxonomy makes them indistinguishable at the seam. Retrying TCX on `Network` costs at most one wasted attempt on a genuine outage (self-limiting), and is the only way to honour REQ-007's fallback under the real (coarse) upstream contract.

### Cascade impact
- Adapter `download_activity` (DES-013) — UNCHANGED (still maps connection→Network, rate_limit→RateLimited, else→Unknown).
- `GarminWorker` / `GarminDownloadFailure` kind enum (DES-001) — UNCHANGED (no new kind).
- DES-004 — gains the explicit readFile decision-table above + the ZIP-unwrap step + the LSN-006-deviation rationale.
- TEST-008/009/010 (already GREEN) — re-audit only: confirm none assert "Network kind → no TCX retry"; if any do, they update at REQ-007 build (bounded, explicit — unlike Option A's silent-failure risk).
- New build tests (garmin-T-018+) at REQ-007 build: Network→retries TCX, RateLimited→no retry, Unknown→retries TCX, content-sniff fixtures (valid zip+fit, zip+tcx/gpx, empty, HTML error page).

### Open questions / risks (carried to REQ-007 build)
- OQ1 (Assumption-B smoke, build NOTE, non-blocking) — once real credentials/network exist, smoke-test ONE manually-entered (FIT-less) activity to confirm the 404→Network path, before finalizing any TEST-008/009/010 rewrite. Cannot be done in this environment (stubs only). Option C is chosen precisely so a wrong guess here does NOT break the fallback.
- OQ2 (ZIP-unwrap, build-blocking prerequisite, independent of DEC-016) — ORIGINAL responses are ZIP-wrapped (per PyPI example `zip_data = ...ORIGINAL`), NOT bare FIT bytes. readFile must unzip before FIT-parsing/sniffing. Confirm whether GC already links a ZIP utility (exports use one) before the build; DES-004 must document the unwrap step regardless of this decision.

### Alignment probe
grep -nE 'NotAvailable|dl_fmt.*TCX|sniff|\.FIT' src/Cloud/GarminConnect.cpp | wc -l   # expect ≥1 once REQ-007 readFile fallback lands (retry table implemented)

---

## DEC-017 — REQ-008 sidecar-persistence component shape
- Status: accepted (A — dedicated GarminSidecarStore)
- Reversibility: medium (the class boundary + test surface stick once Slice C and REQ-010 backfill call it)
- Decided / last-reviewed: 2026-07-19
- Serves: REQ-008 (Slice B — Tier-1 dedup substrate), REQ-010 (backfill-state reuse), REQ-012 (sidecars preserved across Disconnect); constrained by DES-002 (per-account storage layout), DES-006 (AtomicFile), DES-008 (SidecarPermissionsRejected message key)
- Dependents: REQ-008 Slice C (sync orchestration reads/writes via this store), REQ-010 (bulk backfill resumability keys off backfill-state), DES-002 (gains a realized "sidecar store" note)

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A dedicated GarminSidecarStore.{h,cpp} (mirrors GarminTokenStore) | 5 — reuses the proven AtomicFile + perm-refusal path | 4 — both sidecar schemas in one class; a 3rd is a method | 5 — parallel to GarminTokenStore; unit-testable in isolation (garmin-fast) without the sync flow | 5 — matches the DES-002/006 seam; single-responsibility persistence boundary |
| B inline JSON into GarminConnect | 3 — same primitives but perm-check scattered in orchestration | 2 — sync + backfill both bloat GarminConnect.cpp | 2 — dedup substrate can't be tested without driving the whole flow | 2 — mixes persistence with orchestration |
| C generic JsonSidecar helper + typed wrappers | 5 — one perm-checked atomic-JSON path | 5 — any future sidecar reuses it | 4 — extra layer; GarminTokenStore isn't retrofitted, so a partial generalization | 4 — DRY but speculative; only two near-term consumers |

### Cascade impact (per option)
- A (chosen): adds `src/Cloud/GarminSidecarStore.{h,cpp}` (new files → LSN-018: wire into BOTH the unittest target AND the app's `GC_WANT_GARMINCONNECT`); DES-002 gains a realized "sidecar store owns imported-<uid>.json + backfill-state-<uid>.json" note; one new `garmin-fast` TEST group; production `GarminConnect.*` untouched until Slice C.
- B: no new files, but Slice C's sync orchestration inherits perm-check + torn-write handling inline; the dedup substrate becomes untestable in isolation; diverges from the GarminTokenStore precedent.
- C: adds `src/Cloud/JsonSidecar.{h,cpp}` PLUS the two typed wrappers (more surface than A) for no near-term payoff; the token store stays separate, so the generalization is only partial.

### Chosen
A — smallest change that keeps the Tier-1 dedup substrate independently testable and reads consistently with the existing GarminTokenStore. Generalization (C) is deferred until a third consumer outside this feature justifies it (YAGNI); inlining (B) is disqualified by the isolation-testability loss.

### Open questions / risks (carried to Slice B build)
- The perm-refusal semantics reuse REQ-006's `loadChecked` pattern verbatim (owner-only, refuse-and-report-path); confirm the DES-008 `SidecarPermissionsRejected` message key exists (it is drafted in design.md) or allocate it at build.
- Sidecar torn-write handling: parse failure → treat as absent (re-fetch), NOT a hard error, per DES-002 "fallback to last-good or trigger re-fetch". The store returns a typed absent/torn status; the caller (Slice C) decides.

### Alignment probe
grep -rln 'class GarminSidecarStore' src/Cloud | wc -l   # expect 1 once Slice B lands (dedicated component in place)

---

## DEC-018 — Where to persist garmin_user_id (the active-account producer for REQ-008 sync)
- Status: accepted (B — separate account-agnostic active-account.json)
- Reversibility: medium (the file name + resolveGarminUserId's read source stick once connect/disconnect write/clear it)
- Decided / last-reviewed: 2026-07-19
- Serves: REQ-008 Slice D (closes A3-R008-01 — the uid-producer gap), REQ-012 (disconnect clears it); constrained by REQ-006 (tokens.json perm-check schema is frozen), REQ-007 (from_tokens/loadTokens parse the raw garth blob), DES-002 (per-account sidecar paths need the uid BEFORE any account-specific read)
- Dependents: GarminConnect::resolveGarminUserId (repoint read source), the auth-success path (new writer), Disconnect/removeSettings (clear), REQ-008 Slice D end-to-end test
- Origin: A3-R008-01 (readdir resolves an empty uid in production → live sync no-ops; producer never built)

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A wrap tokens.json `{garmin_user_id, tokens:<blob>}` | 3 — one atomic write, but MUTATES the security-locked schema REQ-006 perm-checks + REQ-007 parses | 4 — envelope can hold future account metadata | 3 — open()/blockingRestore must unwrap `.tokens`; REQ-006/007 read path + tests churn | 3 — mixes identity into the secret file; touches a frozen contract |
| B separate account-agnostic `garminconnect/active-account.json = {garmin_user_id}` | 4 — tokens.json UNTOUCHED; a missing file already degrades to empty→no-op (graceful) | 4 — clean home for active-session metadata, separate from the secret | 5 — ZERO change to the locked token read path; resolveGarminUserId just reads a different file; disconnect clears it | 5 — identity ≠ secret; smallest blast radius on security-critical code |
| C parse uid out of the existing garth `dumps()` blob | 2 — depends on garth's undocumented token internals (OQ-risk; breaks on lib change) | 2 — no home for other metadata | 2 — brittle; violates the DES-012 don't-depend-on-library-internals seam | 2 — reads undocumented structure |

### Cascade impact (per option)
- A: write side saves the wrapped envelope; `GarminConnect::open()`/`blockingRestore` must extract `.tokens` before restoreSession; REQ-006 loadChecked + REQ-007 from_tokens tests may need envelope-awareness; any already-written tokens.json needs migration. resolveGarminUserId already reads `.garmin_user_id` top-level (would keep working).
- B (chosen): add a small writer at auth-success that persists `{garmin_user_id}` to `garminconnect/active-account.json` (atomic write via AtomicFile; the uid is not a secret so a tolerant read, not the strict perm-refusal tokens.json gets); repoint `GarminConnect::resolveGarminUserId()` (GarminConnect.cpp:168) from tokens.json to active-account.json; clear the file on Disconnect (REQ-012 — the token file is deleted, and the active-account pointer with it, but per-account sidecars are preserved). tokens.json schema + REQ-006/007 read path FROZEN, untouched. One new small file added to the garminconnect/ dir.
- C: no new write; resolveGarminUserId parses the loadChecked bytes for an embedded uid — but couples to garth internals, re-opening the OQ risk the DES-012 seam exists to contain.

### Chosen
B — least disturbance to the security-locked tokens.json and the REQ-006/007 read path; cleanest separation of "which account is active" from "the OAuth secret"; graceful degradation (absent file → empty uid → no-op, already the coded behaviour). The one cost — a second small file under the perm/atomic discipline — is cheap and non-security-critical (the uid is not a secret).

### Open questions / risks (carried to Slice D build)
- Consistency: active-account.json and tokens.json are written in the same auth-success step but are two files — order the writes so a crash leaves at worst a missing/stale active-account.json (→ empty uid → no-op), never a token file pointing at a wrong active account. Prefer: write tokens.json first, then active-account.json.
- Disconnect (REQ-012) must clear active-account.json alongside tokens.json deletion, but MUST preserve the per-account imported-<uid>/backfill-state sidecars (REQ-012, DES-002).
- End-to-end test (the one A3-R008-01 says is missing): connect with a REAL (non-override) uid → active-account.json written → a fresh GarminConnect (no ctor override) → resolveGarminUserId reads it → readdir resolves the account. This is the integration test that the ctor-override unit tests could not provide.

### Alignment probe
grep -rn 'active-account' src/Cloud/GarminConnect.cpp | wc -l   # expect ≥2 once Slice D lands (writer + resolveGarminUserId read source)

---

## DEC-019 — The production connect/disconnect persist TRIGGER insertion point
- Status: accepted (C — CloudService::disconnect() virtual + GarminConnect owns persist/disconnect; wizard-level finished() capture)
- Reversibility: cheap (both the wizard finished()-connection and the additive default-no-op CloudService::disconnect() virtual are additive; nothing locks a format/protocol/topology). Minor one-way risk: once other services override disconnect(), removing the virtual is a wider but mechanical revert.
- Decided / last-reviewed: 2026-07-20
- Serves: REQ-008 (closes A3-R008-01 — invokes the Slice-D producer on a real connect), REQ-012 (disconnect deletes tokens.json + active-account.json, preserves sidecars); constrained by DEC-014 (Garmin bypasses appsettings/CloudServiceFactory::saveSettings — file token store), the MFA-both-paths constraint, and the absence of any disconnect virtual in CloudService.h (D-R008-01)
- Dependents: the trigger-wiring build slice (TEST-051+), AddCloudWizard.cpp (single finished() connection), GarminConnect.{h,cpp} (persistConnectSuccess + disconnect() override), CloudService.h (new virtual), src/Gui/AthletePages.cpp (deleteClicked → generic disconnect()), DES-002 (prose correction — the "removeSettings overridden" surface is replaced by disconnect())
- Origin: A3-R008-01 (orphaned producer) + Slice D stop-and-report; research draft by qgdw-scout 2026-07-20 (three real options, all citations orchestrator-verified on disk)

### Key research findings (scout, verified)
- Both `AddGarminAuth` (pg 21) + `AddGarminMfa` (pg 22) are constructed with the SAME `garminChain->client()` (AddCloudWizard.cpp:188,195), so one wizard-level listener on that client's `finished(QUuid,GarminAuthSuccess)` catches BOTH direct + post-MFA success — the only structural fix for the MFA early-return miss.
- `GarminConnect::resolveConfigDir()` (GarminConnect.cpp:138-145) == `wizard->context`'s config dir — identical in every option.
- **REQ-012 has NO sibling idiom to copy**: the real disconnect UI `CredentialsPage::deleteClicked()` (src/Gui/AthletePages.cpp:143-161) only flips active/sync flags in appsettings; every sibling (Dropbox/Strava/Xert/Withings/…) leaves its OAuth token in QSettings forever (OAuthDialog.cpp). Garmin's "actually delete the token file" is stricter than anything shipped — must be newly designed, which is why a testable seam beats a magic-string GUI branch.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A persist inside both pages (credentials + MFA) | 2 — each re-gated by its own state machine → reproduces the MFA early-return bug class | 2 — no reuse; every future multi-step native-auth service re-copies it | 2 — ctor churn on 2 shipped widgets; pollutes 2 deliberately Context/Python-free test targets with disk I/O | 2 — contradicts the "commit once at the wizard" idiom (AddFinish::validatePage, AddCloudWizard.cpp:857-861) |
| B wizard owns one finished() connection, persists directly via GarminTokenStore | 3 — single ungated capture fixes MFA-both-paths; stale-reply dedup is an open Q | 3 — Garmin-specific inside AddCloudWizard.cpp, no reusable abstraction | 4 — smallest diff; reuses testGarminConnectWizardRouting target | 4 — matches idiom's spirit; but disconnect = untested Garmin-only string branch in shared AthletePages.cpp |
| C CloudService::disconnect() virtual + GarminConnect owns persist/disconnect (chosen) | 4 — same MFA fix as B PLUS disconnect logic unit-testable in the garmin-fast GarminConnect harness | 4 — real reusable seam (no sibling clears creds today; multi-account later has a seam) | 4 — wizard capture like B + trivial GarminConnect methods beside open()/close(); GUI shrinks to one generic line | 5 — best fit to letter+spirit; token-store knowledge colocated in GarminConnect.cpp; generic disconnect() |

### Cascade impact (chosen = C)
- CloudService.h: add `virtual void disconnect() {}` (default no-op — non-breaking for ~15 other services). **[Ratified 2026-07-20 during build:** a SYMMETRIC `virtual void persistConnectSuccess(uid, blob) {}` default-no-op was added alongside it, because the wizard dispatches through a `CloudService*` handle — the persist call is only well-typed on the base, and this keeps the wizard fully generic (no GarminConnect include/cast, no Python stack in the wizard test). Consistent with Option C's "wizard calls the service through the base handle"; both are safe no-ops. Orchestrator-accepted.]**
- GarminConnect.{h,cpp}: `persistConnectSuccess(uid, blob)` wrapping `GarminTokenStore::persistConnectSuccess(resolveConfigDir(), uid, blob)`; `disconnect()` override calling `GarminTokenStore::clearAccount(resolveConfigDir())`.
- AddCloudWizard.{h,cpp}: ONE `connect(garminChain->client(), &IGarminAuthClient::finished, …)` in ensureGarminAuthPage() (guarded by the existing `if (garminChain) return;` idempotency) → delegates to `wizard->cloudService->persistConnectSuccess(...)`.
- src/Gui/AthletePages.cpp deleteClicked(): call the service's generic `disconnect()` (pattern already used for sync-now, AddCloudWizard.cpp:868-871) — no Garmin string-compare.
- DES-002 (design.md ~489): correct the "removeSettings overridden to Disconnect" prose to the real `CloudService::disconnect()` virtual (resolves D-R008-01).
- Tests: persist covered in testGarminConnectWizardRouting (add GarminTokenStore/AtomicFile to its link list) or a new target; disconnect covered via the GarminConnect::disconnect() override in the existing garmin-fast harness (testGarminConnectConnectPersist/testGarminConnectOpen). TEST-051+.

### Chosen
C — satisfies the MFA-both-paths constraint identically to B (single ungated wizard-level finished() capture) while getting REQ-012's stricter, precedent-less disconnect behavior under automated test behind a small reusable virtual, instead of an untested Garmin-only branch in shared GUI code. The one objection — touching shared CloudService.h — is additive and safe (default no-op). B is an explicit, non-strawman fallback if minimizing shared-header churn ever outranks testable disconnect.

### Open questions / risks (carried to the build slice)
- **Stale-reply dedup at the wizard level:** the pages guard with `m_pendingId` to reject a slow/stale `finished()` from an abandoned earlier attempt (A3-R003-06). A wizard-level catch-all must NOT overwrite a fresh correct token with a late stale one — track the "current" pendingId on the wizard, or confirm GarminWorker never delivers out-of-order across distinct request UUIDs. Design call in the build slice.
- Confirm allocating a throwaway `CloudServiceFactory::newService(id, context)` in deleteClicked() purely to call disconnect() is acceptable (the sync-now path already does this), or expose disconnect() without a full clone.
- `WizardStubPreamble.h` Athlete::config() returns a hardcoded `/tmp/gc-garmin-test` (not a QTemporaryDir) — tighten to avoid parallel-CTest collisions before adding a persist-on-finished test there.

### Post-build correction (2026-07-20, A3-R008-F4 / LSN-027)
The base virtual was renamed `disconnect()` → **`disconnectService()`** during A3-R008 hardening: a bare `disconnect()` on a QObject-derived base name-hides `QObject::disconnect()` for all ~15 CloudService subclasses. So the realized surface is `CloudService::disconnectService()` (default no-op) + `GarminConnect::disconnectService() override`; `deleteClicked()` calls `disconnectService()`. `persistConnectSuccess` was unaffected (no QObject collision).

### Alignment probe
grep -rn 'void disconnectService' src/Cloud/CloudService.h src/Cloud/GarminConnect.h | wc -l   # expect ≥2 (base virtual + GarminConnect override)

---

## DEC-020 — Disposition of A3-R012-F1: a live session outliving Disconnect
- Status: accepted (C — fail-closed re-check + cheap hardening; full lifecycle work deferred to its own REQ) · IMPLEMENTED `f001c7d20` (2026-08-03)
- Reversibility: high (C is additive and local to GarminConnect; it does not foreclose A)
- Decided / last-reviewed: 2026-08-02
- Serves: REQ-012 (the "before clearing in-memory state" criterion clause), REQ-NF-Sec-* (a revoked account must stop syncing); triggered by A3-R012-F1 (blocking)
- Dependents: REQ-012 (closure), DEC-019 (its fresh-instance choice is the root cause and stays in force), REQ-016 (shares the readFile record path), a NEW follow-on REQ for instance-lifecycle/leak work (A3-R012-F10/F12)

**Problem.** `GarminConnect::disconnectService()` deletes tokens.json + active-account.json and clears no in-memory
state; nothing enumerates or shuts down live GarminConnect instances. A window-modal `CloudServiceSyncDialog`
(AddCloudWizard.cpp:889-893) can therefore keep downloading from a just-disconnected account, because the parentless
`ConfigDialog` (ConfigDialog.cpp:37-41) leaves Delete reachable concurrently and `readFile` (CloudService.cpp:1400)
re-checks no token. DEC-019 deliberately chose to mint a FRESH instance in `deleteClicked()` to avoid touching live
instances — that choice is exactly what leaves the live one running, so this is a DEC-level question, not prose.

**Options considered (Three Options Doctrine; scored R/S/M/BP 1-5).**
- **A — Full lifecycle binding.** Reopen DEC-019: invalidate live instances on disconnect (a registry of open
  services, or bind session lifetime to the account) AND fix the `CloudServiceSyncDialog` store leak. R5 S4 M4 BP5.
  Rejected FOR NOW: touches shared CloudService/GUI code owned by no current REQ, and drags the pre-existing
  instance-leak (A3-R012-F12) into REQ-012's scope.
- **B — Ticket and ship.** Commit the (safe) test slice, raise F1 as a new REQ, reopen DEC-019 later. R2 S3 M4 BP3.
  Rejected: the defect ships, and REQ-012 stays open on a security-relevant clause with no mitigation.
- **C — CHOSEN — Fail-closed re-check + cheap hardening.** `GarminConnect::readFile`/`readdir` re-check that the
  account is still connected (tokens present) and fail closed once it is not; plus the cheap non-blockers F2 (sweep
  the `<path>.tmp` siblings in `clearAccount`), F4 (pin the empty-uid write guard), F5 (assert mode + mtime, not just
  bytes, in TEST-055). R4 S3 M4 BP4. Severs the exploit path with a small, testable, local change and leaves A's
  remainder as honest, separately-scoped work.

**Cascade.** REQ-012 can close once C lands and its tests are green (the criterion clause is then MECHANISED, if not
by the literal ordering the prose describes — the prose should be re-read at that point, not before). A3-R012-F2/F4/F5
close with the slice. A3-R012-F3 (ordering invariants unobserved), F6 (discarded bool), F9 (uid unvalidated), F10
(uid re-resolved not latched) and F12 (CloudService leak) stay OPEN and non-blocking; F10+F12 are the seed of the
follow-on lifecycle REQ. DEC-019 is NOT reverted — its trigger design stands; C adds a guard on the consuming side.

### Alignment probe
grep -n 'resolveConfigDir\|loadChecked\|isConnected' src/Cloud/GarminConnect.cpp | head   # expect a token re-check reachable from readFile/readdir after this slice

---

## DEC-021 — The REQ-017 lifecycle-binding mechanism: how Disconnect invalidates a LIVE session
- Status: accepted (B — account-epoch counter latched at open(), compared in-memory)
- Reversibility: cheap (a private static hash + two free functions + two latched members, confined to `GarminAccountEpoch.{h,cpp}` + `GarminConnect.{h,cpp}`; deleting it reverts cleanly to DEC-020's status quo)
- Decided / last-reviewed: 2026-08-03
- Serves: REQ-017 (all five clauses a–e); closes the residual DEC-020 explicitly accepted; constrained by DEC-019 (its fresh-instance choice stays in force — NOT reopened), DEC-002/DES-001/REQ-NF-Threads-001 (single off-GUI worker, no `terminate()`), DEC-014 (Garmin owns a file token store), and the standing test constraint that GarminConnect cannot link into the Python-free wizard harness
- Dependents: REQ-017 build slices A + B, TEST-060..064, DES-002/DES-014 (add the epoch to the storage/lifecycle prose), A3-R012-F10 (uid latch) + F12 (CloudService leak) which close with this REQ, DEC-020 (its guard STAYS — the epoch is an independent second gate, not a replacement)
- Origin: A3-R012-F1 residual; research draft by qgdw-scout 2026-08-03 (three real options; orchestrator independently verified the central code claims — see below)

### Key research findings (scout, orchestrator-verified on disk)
- `blockingDownload()` (GarminConnect.cpp:231) runs a **nested `QEventLoop`** (:239, 60s watchdog `kDownloadTimeoutMs` at :260). The GUI event queue therefore keeps pumping during a download — which is precisely how a Disconnect click reaches us mid-flight, and why teardown must be QUEUED, never same-frame (deleting `m_client` under a live `blockingDownload()` frame is a UAF).
- **There is no cancel primitive anywhere**: `cancel|abort|interrupt|requestInterruption` matches NOTHING in `GarminWorker.{h,cpp}`, `IGarminDownloadClient.h`, `PyEmbeddedAdapter.h`. The download is a blocking Python call under the GIL and DES-001 invariant 3 forbids `terminate()`. True mid-flight cancellation is unreachable without new work — see the REQ-017(c) narrowing below.
- The queued-post idiom already exists and is documented: `m_completionContext` (GarminConnect.h:200, rationale at .cpp:545-580, the A3-R007-01 UAF guard). Any teardown reuses it rather than inventing one.
- Two ctors (:91, :93) + dtor (:100); `disconnectService()` :375; `accountStillConnected()` :165; `resolveGarminUserId()` defined :147 and re-resolved per call at :467 (readdir) and :521 (recordImport) — the F10 defect, fixed by the same open()-latch as the epoch.

### Alternatives
| Opt | Rel | Scal | Maint | BP |
|---|---|---|---|---|
| A registry of live instances + queued self-close broadcast | 4 — self-contained, but the queued-close discipline must be exactly right | 4 — per-config-dir keying, O(1) | 4 — all state in GarminConnect.{h,cpp}; register/unregister across 2 ctors + dtor | 4 — standard observer/registry, more machinery than B for the same guarantee |
| B account-epoch latched at open(), in-memory compare (chosen) | 4 — fewest moving parts; `bump()` never reaches into another instance's state, so there is NO delete-out-from-under hazard | 5 — one static hash of ints, per config dir | 5 — lowest cognitive load; one hash, two functions, two latched members | 5 — the generation/epoch token is the canonical idiom for exactly this stale-handle bug class, and matches REQ-017's own framing ("by binding, not a per-call disk re-check") |
| C route Disconnect through the live instance (Context tracks it) | 3 — most "correct" ownership, but widest shared touch; ~15 siblings' open/close bookkeeping was never designed for tracking | 4 — per-Context map, fine for a desktop app | 2 — touches Context/MainWindow/AddCloudWizard/AthletePages/CloudService.h and reopens DEC-019 | 3 — RAII-correct in the abstract, but reopens an accepted decision AND is untestable under the Python-free harness (WizardStubPreamble.h stubs Context/MainWindow) |

### Chosen
B. It satisfies clauses (a), (c), (d) most cheaply, stays entirely inside `src/Cloud/` with **zero blast radius on the ~15 sibling services and no `CloudService.h` change**, leaves DEC-019 intact, and is fully testable on `garmin-fast`. Decisively: its clause-(a) test is the strongest available — leave `tokens.json` **valid and untouched**, bump the epoch alone, and assert the live instance issues zero list/download calls. That proves the binding without any reference to DEC-020's guard, so the test cannot silently pass for the wrong reason.

**The one gap, accepted with a compensating control.** B's invalidation is LAZY: `bump()` does not reach into the other live instance, so an *idle* sync dialog keeps its worker thread + embedded-Python session alive until its next call (which now fails fast, pre-network) or until its owner closes it. Clause (b) is therefore read FUNCTIONALLY — "never outlives the owning window", not "torn down at the instant of disconnect". To make that reading honest, REQ-017(e)'s `CloudServiceSyncDialog` dtor + `MainWindow::syncCloud` leak fix is **load-bearing, not parallel**: it is the guaranteed teardown trigger. Option A remains the documented, non-strawman fallback if the literal instant-of-disconnect reading is ever required.

### REQ-017(c) narrowed at decision time (user-approved 2026-08-03)
Clause (c) originally read "cancelled **or** its result discarded". Cancellation is unreachable (see findings). The clause is narrowed to **discard-only**: a download whose result lands after the disconnect is neither staged nor recorded, implemented as a **post-download, pre-stage recheck** of the epoch immediately before `recordImport`/`postReadComplete` — not only at `readFile`/`readdir` entry. **Accepted residual, deliberately visible:** the in-flight HTTP call still runs to completion (or the 60s watchdog); we discard its result rather than stopping it. An interruptible download path in `GarminWorker`/`PyEmbeddedAdapter` is separate, larger work and is NOT in REQ-017 — if it is ever wanted it needs its own REQ + DEC. Do not let a later cycle quietly re-read (c) as though cancellation were implemented.

### Cascade impact (chosen = B)
- NEW `src/Cloud/GarminAccountEpoch.{h,cpp}` — pure Qt, **no `Python.h`** (same seam discipline as `GarminDownloadChain.h`); `static QHash<QString,quint64> s_epoch` + `current(dir)` / `bump(dir)`. Must be added to `src/CMakeLists.txt` GC_WANT_GARMINCONNECT **and** the unittest target (LSN-018 — a missed main-binary link has bitten this project before).
- `GarminConnect.{h,cpp}`: latch `m_openedEpoch` + `m_openedUserId` in `open()`; gate `readFile`/`readdir` on the epoch compare **in addition to** DEC-020's `accountStillConnected()` (the guard is NOT removed — defence in depth, and REQ-017(a) is proven by neutralising it); `disconnectService()` calls `GarminAccountEpoch::bump()` alongside `clearAccount()`; `readdir`/`readFile`/`recordImport` consume `m_openedUserId` instead of re-resolving (F10).
- `CloudService.h`/`.cpp` + `src/Gui/MainWindow.cpp` + `AddCloudWizard.cpp`: clause (e) — a real `~CloudServiceSyncDialog` that `close()`s and deletes its store, and the `syncCloud` leak. **This is the only part that leaves `src/Cloud/`**, and both `MainWindow.cpp` and `src/CMakeLists.txt` are ALREADY dirty with unrelated pre-session edits ⇒ hunk-split at commit (LSN-010/007).
- Tests TEST-060..064 (see traceability). A3-R017's FIRST probe is mandated: neutralise `accountStillConnected()` and confirm the epoch alone still stops the exploit.
- DEC-020 is NOT superseded — its guard remains as the second layer.

### Alignment probe
grep -rn 'GarminAccountEpoch\|m_openedEpoch\|m_openedUserId' src/Cloud/ | wc -l   # expect >0 after slice A; 0 means the bind was never wired
grep -n 'GarminAccountEpoch.cpp' src/CMakeLists.txt unittests/Core/garminconnect/CMakeLists.txt   # expect BOTH (LSN-018)

---

## DEC-022 — How a refused (fail-closed) readFile reports itself to the sync dialog
- Status: **PARTIALLY IMPLEMENTED 2026-08-04 — GarminConnect half accepted + green (TEST-065); the SHARED half is BLOCKED and un-decided, because this entry's original safety premise was FALSE (see the correction below). Re-decide before touching `completedRead`.**
- Reversibility: cheap (two small edits; the shared change is confined to the existing failure branch of one slot)
- Decided / last-reviewed: 2026-08-03
- Serves: REQ-017(a) ("fails with a Garmin-labelled error"), B-R017-06 (blocking); also retro-fixes REQ-012/DEC-020's shipped guard
- Dependents: TEST-065/066, `GarminConnect::readFile` both fail-closed paths, `CloudServiceSyncDialog::completedRead`, B-R017-07 (uploadCloud leak, folded in)
- Origin: B-R017-06, surfaced by the Slice-B builder's report-only ErrorBus feasibility read and CONFIRMED on disk by the orchestrator

**Problem.** `syncNext` (CloudService.cpp:1413) and `downloadNext` (:1495) discard `readFile`'s bool and wait for a `readComplete` signal to advance. `GarminConnect::readFile`'s fail-closed paths (`sessionSuperseded()` :452, `accountStillConnected()` :460) return false and post NO completion ⇒ the dialog hangs at "Downloading n of N" and the `new QByteArray` at :1412/:1494 leaks. Already shipped via DEC-020's identically-shaped guard in `f001c7d20`.

**THE TRAP that constrains every option (verified).** `completedRead` is declared `(QByteArray*, QString, QString /*message*/)` — the parameter is literally unnamed/discarded at CloudService.cpp:1527. But **a non-empty `message` does NOT mean failure today**: `GarminConnect::postReadComplete` already passes `tr("Completed.")` on SUCCESS (GarminConnect.cpp:686-691). So no option may infer "error" from "message is non-empty" — that would relabel every successful Garmin download as an error.

### CORRECTION 2026-08-04 — the premise that made Option A's shared half "safe" was FALSE
This entry (and the build briefing derived from it) asserted that the ~15 other CloudService subclasses pass an
EMPTY message, so surfacing `message` on the `ride == NULL` branch would leave them untouched. **That is wrong, and
the orchestrator confirmed it on disk.** EVERY sibling passes `tr("Completed.")` on success —
`Strava.cpp:530`, `Dropbox.cpp:321`, `SportTracks.cpp:539`, `Xert.cpp:478`, `Azum.cpp:297`, `PolarFlow.cpp:219`,
`CyclingAnalytics.cpp:433`, `SixCycle.cpp:480`, `Nolio.cpp:247`, `LocalFileStore.cpp:150`. So
`if (!message.isEmpty()) show(message)` on the failure branch would render EVERY OTHER SERVICE'S PARSE FAILURE as
**"Completed."** — the trap above, inverted: failure relabelled as success, across ~10 shipped integrations.
The builder built the seam, RED-verified it, then REMOVED it and fired the stop-and-report hatch rather than ship a
cross-service regression against a briefing it could see was wrong. That was the correct call.
**Consequence:** the GarminConnect half (post a labelled completion, still return false) is INDEPENDENTLY correct and
shipped — it closes the blocking half of B-R017-06 (loop advances, buffer freed) with no regression for anyone. Only
the *display* of the reason is unresolved: a Garmin refusal currently shows `uncompressRide`'s text instead of the
reason. Strictly better than the hang. The shared half needs a fresh decision — candidate discriminators, none
chosen: (1) branch on `data->isEmpty()` (safe except for a genuine 0-byte download from another service, which would
then read "Completed."); (2) a canonical success-label accessor on `CloudService` so the shared side can ignore it
(~15 files, and cross-`tr()`-context string comparison is locale-fragile); (3) carry failure EXPLICITLY — an extra
signal argument or a `readFailed` signal (bigger blast radius, but the only one that is not a heuristic).
**Lesson [[LSN-034]]** — an orchestrator's "this is what makes it safe" premise is a claim about the tree and must be
grepped before it is written into a DEC, not after.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — post a labelled completion; surface `message` ONLY on the existing failure branch.** GarminConnect's refusal paths call `postReadComplete(data, name, tr("Garmin Connect: …"))` and still `return false`. In `completedRead`, the sole shared change is in the branch that already runs when `ride == NULL`: show `message` when it is non-empty, else fall back to today's `errors.join(" ")`. Success path (`ride != NULL`) is untouched, so `tr("Completed.")` can never be mistaken for an error, and the ~15 other services (which pass an empty message) keep their exact current behaviour. R4 S4 M5 BP5.
- **B — new out-of-band error channel (ErrorBus).** Rejected: `ErrorBus` DOES NOT EXIST in the tree — only DES-008 prose and a TODO at GarminConnect.cpp:649. It would be a new subsystem, and being a side channel it would label the error while leaving the loop still stalled. R2 S3 M2 BP3.
- **C — make the callers honour `readFile`'s bool.** Rejected for now: `syncNext`/`downloadNext` would need to advance the loop themselves on a false return, changing control flow for all ~15 services in a function whose completion-driven design is load-bearing. Larger blast radius for the same user-visible outcome. R3 S4 M2 BP3.

**Cascade.** `GarminConnect::readFile` both fail-closed paths post a completion before returning false (the buffer is then freed by `completedRead`'s existing `delete data`, killing the leak). `CloudServiceSyncDialog::completedRead` names its `message` param and uses it in the `ride == NULL` branch. B-R017-07 (`MainWindow::uploadCloud`'s identical `db` leak) is folded in, reusing Slice B's `closeAndDeleteStore()`. REQ-017(a) then holds on BOTH `readdir` and `readFile`, so B-R017-01/06 close together. DEC-020's guard is unchanged in behaviour — it merely reports properly now.

### Alignment probe
grep -n 'QString /\*message\*/' src/Cloud/CloudService.cpp   # expect ZERO after this slice (the param must be named + used)
grep -n 'postReadComplete' src/Cloud/GarminConnect.cpp        # expect a call on BOTH fail-closed paths, not just the success path

---

## DEC-023 — Carry read FAILURE explicitly instead of inferring it from the message
- Status: accepted (option 3 of DEC-022's correction — an explicit failure channel)
- Reversibility: moderate (a new signal is additive and the ~15 siblings never emit it; unwinding it later means re-auditing whoever came to depend on it)
- Decided / last-reviewed: 2026-08-04
- Serves: REQ-017(a) (the "Garmin-labelled error" half DEC-022 could not deliver), B-R017-06 (display half), B-R017-10 (the ~4 other silent `return false` sites); supersedes DEC-022's shared half
- Dependents: TEST-068/069, `GarminConnect::readFile` (all non-posting return sites), `CloudService` signal surface, `CloudServiceSyncDialog`, `CloudServiceAutoDownload`
- Origin: DEC-022's false premise (see its 2026-08-04 correction) — every sibling passes `tr("Completed.")`, so success and failure are INDISTINGUISHABLE on the existing `message` channel

**Why explicit beats every heuristic.** Both cheaper candidates infer failure from a proxy: "message is non-empty" is wrong because all ~10 shipped siblings pass `tr("Completed.")` on success; "payload is empty" is wrong for a genuine 0-byte download, which would then render as its success label. A dedicated failure channel needs no proxy, and it is the only option that also covers B-R017-10's remaining silent paths (`:490`, `:501` RateLimit, `:512`, `:520`, `:527` TCX-failed) with ONE mechanism instead of five special cases. Cost is a wider surface — hence its own DEC rather than a build-slice improvisation.

**Cascade.** `CloudService` gains a `readFailed(QByteArray* data, QString name, QString reason)` signal alongside `readComplete`. The ~15 siblings never emit it and are otherwise untouched — that is what keeps the blast radius honest (contrast DEC-022's shared edit, which would have changed behaviour for all of them). `GarminConnect::readFile`'s non-posting `return false` sites emit it with a reason. Both consumers connect it: the sync dialog shows the reason, deletes the buffer, and advances the loop (the same three things `completedRead` does); `CloudServiceAutoDownload` does the equivalent. TEST-065's GarminConnect-half assertions stay valid — the refusal paths keep posting; DEC-023 only changes WHICH signal they post on. B-R017-11 (readFile vs readdir wording drift) is aligned in the same slice.

### Alignment probe
grep -rn 'readFailed' src/Cloud/CloudService.h src/Cloud/GarminConnect.cpp   # expect the signal + emits on every silent return-false site
grep -c 'notifyReadComplete' src/Cloud/Strava.cpp src/Cloud/Dropbox.cpp      # expect UNCHANGED — siblings must not be touched

---

## DEC-024 — Making the modeless sync dialog's self-deletion safe (A3-R017-F1)
- Status: accepted (A — `closeEvent()` guard + a blocking-call-in-flight flag, shipped with an ASan-backed test)
- Reversibility: cheap (a flag, an override, and a deferred re-close; confined to `CloudServiceSyncDialog`)
- Decided / last-reviewed: 2026-08-05
- Serves: REQ-017(e) on the wizard path, and therefore REQ-017(b) — DEC-021 leans on this teardown as its compensating control; triggered by A3-R017-F1 (BLOCKING, ASan-reproduced)
- Dependents: TEST-070/071, `CloudServiceSyncDialog` (ctor `open()`, `refreshClicked` readdir :1019, `syncNext` readFile :1417, `downloadNext` readFile :1499, `cancelClicked` :980), AddCloudWizard.cpp:899

**Problem.** REQ-017 Slice B added `setAttribute(Qt::WA_DeleteOnClose)` to the MODELESS post-connect sync dialog (AddCloudWizard.cpp:899) so it would stop leaking its store. But `GarminConnect::readFile`/`readdir` run nested `QEventLoop`s, so GUI events are processed mid-call; closing the dialog then destroys it → `~CloudServiceSyncDialog` → `closeAndDeleteStore(store)` → the GarminConnect is deleted **while `readFile` is still executing on it**, and execution resumes after the nested loop on freed memory. ASan-reproduced with two independent harnesses. `cancelClicked()` (CloudService.cpp:980) `reject()`s unconditionally — it does not consult `downloading` — so the ordinary Cancel button reaches this too. **We converted a leak into a crash**, which is strictly worse; the fix must not simply re-introduce the leak.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — `closeEvent()` guard + deferred self-close.** A `m_blockingCallActive` flag is set around every dialog→store call that can run a nested loop (ctor `open()`, `refreshClicked`'s readdir :1019, `syncNext` :1417, `downloadNext` :1499). `closeEvent()` overrides: if the flag is set, record `m_closeDeferred`, `e->ignore()`, and **also set the existing `aborted` flag (CloudService.h:427) so the sync stops promptly** rather than leaving the user's click apparently ignored; when the last blocking frame unwinds, if `m_closeDeferred` then `close()` for real. `cancelClicked()` routes through the same guard instead of an unconditional `reject()`. R4 S4 M4 BP4. Keeps clause (e) fully satisfied on the wizard path — no leak, no crash — at the cost of a genuine ASan-backed test, which is exactly the discipline A3-R017's lesson demands.
- **B — Drop `WA_DeleteOnClose`, explicit owner (stack + `exec()`).** Mirrors the pattern `MainWindow::syncCloud` already proves safe; structurally eliminates the hazard rather than guarding it. R5 S4 M5 BP4. Rejected only because it converts the wizard's post-connect sync from modeless to MODAL — a real UX change to the feature's most common first-use path. Retained as the explicit fallback if A's guard proves fragile under A4.
- **C — Revert to the leak for this call site.** R2 S3 M5 BP2. Rejected: REQ-017(e) would go unmet on the one path DEC-021 explicitly called load-bearing, so clause (b)'s functional reading would lose its guaranteed teardown trigger — we would be back to a session outliving its window, which is the whole point of REQ-017.

**Cascade.** `CloudService.h`: two bools + a `closeEvent` override on `CloudServiceSyncDialog`. `CloudService.cpp`: bracket the four call sites, rewrite `cancelClicked`, add the deferred re-close. `AddCloudWizard.cpp:899` is UNCHANGED — the attribute stays. `MainWindow::syncCloud`/`uploadCloud` are unaffected (stack-allocated, never set the attribute; it is commented out at CloudService.cpp:436 for this exact reason). A3-R017-F3 (`completedRead`'s abort branch leaking `data`, CloudService.cpp:1537-1541) is folded in — same function family, one line.

**Evidence bar (non-negotiable).** The fix ships with a test that actually EXECUTES the close-during-nested-loop sequence, not a test that reasons about it. A3-R017's lesson is that a Qt lifetime claim ("DeferredDelete is throttled to the posting loop level, so this is safe") sounded correct, was written into a design comment, and was false in practice. Run it under AddressSanitizer.

### Post-build correction (2026-08-05) — RATIFIED: the gate must be on `done(int)`, not `closeEvent()`
**This entry as originally written was insufficient, and the builder proved it rather than following it.** DEC-024
step 3 prescribed a `closeEvent()` override. Measured on Qt 6.8.2: `reject()` and the Escape key destroy a
`WA_DeleteOnClose` dialog **even when `closeEvent()` calls `e->ignore()`**, because `QDialog::closeEvent` internally
routes to `reject()` → `done()`. The real choke point that ALL of `close()`/`reject()`/`accept()`/Escape funnel
through is **`QDialog::done(int)`**, so the guard is overridden there.

Mutation-verified, by the builder and independently re-run by the orchestrator:
- **M1, remove the `done()` gate → ASan `heap-use-after-free`.** The gate is load-bearing. (Orchestrator re-ran this
  itself: snapshot → mutate → ASan UAF reproduced → restore, md5 `0cd1dfb3…`, 26/26 green.)
- **M2, remove the `closeEvent()` gate → still PASSES.** The `closeEvent()` override is therefore **NOT
  independently load-bearing** — it is belt-and-braces, kept so the fix does not silently depend on that Qt internal
  ever staying true. Recorded honestly so a later reader does not mistake it for the mechanism.

Also ratified, all beyond what this entry specified:
- **Depth COUNTER, not a bool** (`blockingCallDepth`): `QApplication::processEvents()` inside a blocking call can
  dispatch a Refresh click, nesting a `readdir` frame inside a `readFile` frame — a bool would be cleared by the
  inner frame while the outer was still live.
- **The ctor guard is WIDER than specified** — the whole ctor body, not just `store->open()`, because the ctor also
  reaches `refreshClicked()` → `readdir`, and `delete this` must not happen while the ctor is on the stack at all.
- **`cancelClicked()`** rewritten off its unconditional `reject()`.
- **A3-R017-F3 folded in**: `completedRead`'s abort branch now `delete data` like its `failedRead` sibling.

**Accepted residuals (recorded, not fixed):** `writeFile` is deliberately unguarded at CloudService.cpp:1435/:1656 —
no service in this tree runs a nested loop there (all go async via `QNetworkReply`), but **if one ever blocks in
`writeFile`, the UAF returns on the upload path**. Direct destruction (`delete dialog`, or teardown via the parent
`MainWindow`) still bypasses the gate entirely — nothing routes through `close`/`done` — which is pre-existing and
by construction. The ASan target sets `detect_leaks=0` (LSan drowns in Qt/qwt startup allocations), so
whole-process leaks are not covered by it.

**Hang risk, deliberately tested:** gating `done()` is dangerous in the modal path — `MainWindow::syncCloud` uses
`exec()`, which waits on `done()`, so a gate that failed to reopen would HANG File > Sync rather than crash it.
TEST-071 drives exactly that shape with a 5-second watchdog and, probed with the guard wedged, reports
`exec() never returned`.

### Alignment probe
grep -n 'void done' src/Cloud/CloudService.h                               # expect the done(int) override — the actual gate
grep -n 'closeEvent' src/Cloud/CloudService.h src/Cloud/CloudService.cpp   # present, but belt-and-braces (see M2)
grep -n 'WA_DeleteOnClose' src/Cloud/AddCloudWizard.cpp                    # expect it STILL PRESENT (option A keeps it)

---

## DEC-025 — Surviving PARENT TEARDOWN: guarding the unsafe operation, not the close (A3-R017b-F1/F4)
- Status: accepted (A — the destructor declines to delete the store while a blocking call is in flight, plus self-death detection on the frames that resume)
- Reversibility: cheap (a depth check in one destructor + a `QPointer` in one RAII class; confined to `CloudServiceSyncDialog`)
- Decided / last-reviewed: 2026-08-05
- Serves: REQ-017(e) on the parent-teardown path; triggered by A3-R017b-F1 (BLOCKING, Qt-semantics proven by isolated repro) with A3-R017b-F4 as the root cause
- Dependents: TEST-072/073/074, `~CloudServiceSyncDialog` (CloudService.cpp:981-987), `BlockingCall` (:1003-1019), `deferCloseIfBusy` (:1030-1038), `syncNext` (:1518-1521), `downloadNext` (:1605-1608), `refreshClicked` (:1113), `closeAndDeleteStore` (CloudService.h:279-289); context: AddCloudWizard.cpp:899, MainWindow.cpp:143

**Problem.** DEC-024's gate sits on close-INITIATION — `done(int)`, with `closeEvent()` as belt-and-braces. Qt destroys child widgets **directly** when the parent is destroyed: no `closeEvent`, no `done`, no virtual dispatch, and a destructor cannot be vetoed the way a virtual can. The dialog is parented to `context->mainWindow` (CloudService.cpp:709) and **`MainWindow` itself carries `WA_DeleteOnClose`** (MainWindow.cpp:143), so closing the athlete's main window mid-sync runs `~CloudServiceSyncDialog` → `closeAndDeleteStore(store)` while a nested `QEventLoop` is on the stack — freeing the GarminConnect that `readFile` is still executing on. Exposure is up to 60s per call (GarminConnect.cpp:41-43). Pre-Slice-B there was no destructor, so this path LEAKED; **this changeset converted it into a crash**, exactly as it did for the self-close routes. All ~16 CloudService subclasses run nested loops in `open()`/`readdir()`, so the route is not Garmin-specific.

**The second half of the hazard, found while drafting this entry (O-R025-01) — declining to delete the store is NECESSARY BUT NOT SUFFICIENT.** Parent teardown frees the *dialog* too, while the dialog's own member functions are suspended in that nested loop. Verified on disk, not reasoned about:
- `BlockingCall::~BlockingCall` writes `dialog->blockingCallDepth` (CloudService.cpp:1011) and may call `dialog->close()` (:1018) — the first thing that runs when the loop unwinds, straight into freed memory.
- `downloadNext` then resumes at `QApplication::processEvents()` (:1608) and `syncNext` at (:1521); both continue walking `rideListDown`/`progressLabel`/`listindex` — all members of a destroyed object.

So a store-only guard leaves a use-after-free on the *identical trigger*, and the ASan test this decision mandates would still trip. The guard must cover both objects the teardown frees.

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — guard the unsafe operation itself, in three parts.** (1) `~CloudServiceSyncDialog`: when `blockingCallDepth > 0`, keep `store->disconnect(this)` but **skip `closeAndDeleteStore(store)`** — deliberately leak the store, which is precisely the pre-REQ-017 behaviour on this path and strictly better than a UAF. (2) `BlockingCall` holds a `QPointer<CloudServiceSyncDialog>`; its destructor no-ops when the dialog is already gone. (3) `syncNext`/`downloadNext`/`refreshClicked` take a `QPointer` self before each blocking call and return immediately if it is null afterwards, touching no members. R4 S4 M4 BP4. Addresses A3-R017b-F4's root cause — the check moves onto the operation that is actually unsafe, so any future direct-destruction path (`delete dialog` tomorrow) is covered by construction, not by having enumerated the routes.
- **B — Reparent the dialog to `nullptr`.** A top-level, parentless dialog is never destroyed by MainWindow, so the route disappears rather than being guarded. R4 S3 M4 BP3. **Rejected:** it orphans the window — the sync dialog survives the athlete window that spawned it, and with `WA_DeleteOnClose` firing only on user close, the store is torn down only if the user happens to close the dialog. That forfeits REQ-017(b)'s functional reading ("never outlives the owning window"), which is the clause Slice B exists to satisfy.
- **C — MainWindow event filter / busy-veto deferring teardown while a child reports busy.** R2 S2 M2 BP2. **Rejected on three counts:** parent destruction is not an event, so there is nothing to filter at the moment that matters; it puts CloudService lifecycle knowledge into MainWindow, inverting the ownership direction the whole REQ is establishing; and it would block application quit for up to 60 seconds.

**Cascade.** `CloudService.h`: a `QPointer` member on `BlockingCall` (already non-copyable, :475-484). `CloudService.cpp`: the destructor's depth check, the `BlockingCall` destructor's null check, and the three resuming call sites. `closeAndDeleteStore` (CloudService.h:279-289) is UNCHANGED — the decision is *whether* to call it, not what it does, so TEST-064's contract stands. `AddCloudWizard.cpp:899` keeps `WA_DeleteOnClose`; `MainWindow.cpp:143` is not touched. The ~15 sibling services are untouched and inherit the fix, since the guard lives in the shared dialog. **A3-R017b-F2 folded in if cheap:** wrap the two `writeFile` sites (CloudService.cpp:1538 in `syncNext`, :1769 in `uploadNext`) in `BlockingCall` for symmetry — no service blocks there today (adversary verified across all 16), so this is pre-emptive consistency, not a fix, and it must be dropped rather than forced if it complicates the slice.

**Evidence bar (non-negotiable, and the reason this is not a one-line change).** The fix ships with a test that EXECUTES parent teardown mid-nested-loop — construct a parent, construct the dialog as its child, enter a blocking call, `delete parent` from inside that call, let the loop unwind — under AddressSanitizer, on the existing ASan target. Both directions proven: restoring the unconditional `closeAndDeleteStore` in the destructor must reproduce the ASan `heap-use-after-free`. DEC-024's lesson is that a Qt lifetime claim which *sounded* correct was false in practice; the same standard applies here to the QPointer half.

**Accepted residuals (recorded, not fixed).** The store LEAKS on the parent-teardown path — deliberate, user-chosen, and the pre-REQ-017 status quo; for GarminConnect that means the worker thread and its embedded-interpreter session live to process exit, but only for a sync that was in flight when the main window closed. The ASan target keeps `detect_leaks=0` (LSan drowns in Qt/qwt startup allocations), so that leak is not observable by the test either way. A deferred reaper that deletes the store once the call unwinds was considered and NOT taken: at parent-teardown time the application is already shutting down, so a reaper would be running against a dying event loop.

### Alignment probe
grep -n 'blockingCallDepth' src/Cloud/CloudService.cpp                     # expect a check in ~CloudServiceSyncDialog, not only in deferCloseIfBusy
grep -n 'QPointer' src/Cloud/CloudService.h src/Cloud/CloudService.cpp     # expect it on BlockingCall + the three resuming call sites
grep -n 'closeAndDeleteStore' src/Cloud/CloudService.h src/Cloud/CloudService.cpp  # expect the helper UNCHANGED, the CALL conditional

## DEC-026 — Two-phase init: no nested event loop under construction (closes the BLOCKING A3-R025-F1)
- Status: accepted (B — the constructor builds the widget shell only; the blocking work (`store->open()` + the initial `refreshClicked()`) moves to an explicit `start()` the caller invokes on a fully-constructed object)
- Reversibility: moderate (re-shapes one ctor + a new slot + two call sites; confined to `CloudServiceSyncDialog` and its two constructors of record)
- Decided / last-reviewed: 2026-08-06
- Serves: REQ-017(e) on the constructor-teardown path; triggered by A3-R025-F1 (BLOCKING) which escalated B-R025-01; completes the class DEC-024/DEC-025 left open (A3-R017b-F4)
- Dependents: TEST-075 (ctor route eliminated, ASan, both directions), TEST-076 (RED tests making the syncNext/downloadNext self-guards load-bearing — A3-R025-F2/B-R025-02), `CloudServiceSyncDialog::CloudServiceSyncDialog` (CloudService.cpp:709-974), new `CloudServiceSyncDialog::start()`, callers `MainWindow::syncCloud` (MainWindow.cpp:2577-2578), `AddCloudWizard` (AddCloudWizard.cpp:892-899); context: DEC-024, DEC-025

**Problem.** DEC-024 gated close-initiation; DEC-025 guarded the destructor + the resuming member frames for parent teardown. A3-R025-F1 (orchestrator-confirmed at CloudService.cpp:719/725/734/972; general Qt mechanism proven by the adversary's standalone ASan repro) showed a FOURTH live route to the identical use-after-free that neither closes: the **constructor itself**. `CloudServiceSyncDialog::CloudServiceSyncDialog` wraps its whole body in one depth-only `BlockingCall` and runs three blocking / nested-loop calls with no `QPointer` self-bail — `store->open()` (:725, `blockingRestore` 30s), `QMessageBox::exec()` (:734, UNBOUNDED user wait), the tail `refreshClicked()` (:972, readdir loop) — then resumes writing `this` members (`tabs = new QTabWidget(this)` :745…). A parent teardown (MainWindow `WA_DeleteOnClose`, MainWindow.cpp:143) landing in any of those frees the half-built dialog under its own ctor; reachable via the heap/modeless `AddCloudWizard.cpp:892`. DEC-025's three guards cannot help — there is no fully-formed object to stand down. This is the THIRD cycle of the same enumeration-miss family (DEC-024 → DEC-025 → here), recorded as a miss on [[LSN-037]], whose rule now names the constructor explicitly.

**Options considered (scored R/S/M/BP 1-5).**
- **A — Constructor self-bail.** `QPointer<CloudServiceSyncDialog> self(this)` + early-return after each of the three ctor blocking sites, touching no members if null. R2 S3 M3 BP2. **Rejected:** does NOT end the recurrence (same per-site enumeration that has missed a frame three times), and carries a CALLER-SIDE residual — after the ctor bails, `new` returns a dangling pointer and `AddCloudWizard.cpp:899` still calls `->open()` on freed memory, so A only fully closes the route if the caller is also guarded. Guards an anti-pattern (nested event loops in a constructor) rather than removing it.
- **B — CHOSEN — two-phase init.** The constructor builds only what does not depend on `open()` (title, size, member refs) and runs NO nested loop; a new `bool start()` slot does `store->open()`, the tab construction, and the initial `refreshClicked()`, returning false on open-failure. Both callers invoke `start()` on the fully-constructed object: `MainWindow::syncCloud` → `if (sync.start()) sync.exec();`; `AddCloudWizard` → `d->setAttribute(WA_DeleteOnClose); if (d->start()) d->open(); else delete d;` (open-failure cleanup preserved — today the ctor's failure path hides + queues close). R4 S4 M4 BP4. Eliminates the class structurally: no nested loop ever runs on the stack under construction, so there is no half-built frame to free; `start()` runs on a complete object where DEC-024/025's guards already apply, and as a resuming frame it takes the DEC-025 part-3 `QPointer` self-bail discipline. No caller-side residual. Blast radius is the TWO construction sites of record, not the ~15 sibling services (which inherit the shared dialog and never construct it).
- **C — Reparent the dialog to `nullptr`.** R3 S2 M2 BP2. **Rejected — same objection as DEC-025 option B:** a top-level parentless dialog is never destroyed by MainWindow, but it orphans the window and can outlive the athlete window that spawned it, forfeiting REQ-017(b)'s functional reading ("never outlives the owning window").

**Cascade.** `CloudService.h`: `CloudServiceSyncDialog` gains a public `bool start()`; the ctor's declared responsibility shrinks to shell-only. `CloudService.cpp`: split the ctor at the `store->open()` boundary into ctor + `start()`; `start()` carries the DEC-025 part-3 self-bail after each blocking call it makes. `MainWindow.cpp:2577-2578` and `AddCloudWizard.cpp:892-899`: the two callers gain the `start()` invocation + open-failure handling. DEC-024's `done()`/`closeEvent()`/`deferCloseIfBusy`/`BlockingCall` and DEC-025's dtor/`QPointer` guards are UNCHANGED and continue to protect `start()`'s loop and every post-construction frame — this decision removes the one frame they could not reach. TEST-070..073 must still pass. **Rider (A3-R025-F2 disposition, user-chosen):** add TEST-076 — RED tests that make `syncNext`'s and `downloadNext`'s `QPointer` self-guards load-bearing (today both mutations survive; only `refreshClicked`'s is caught), so no untested guard stands in front of the hazard.

**Evidence bar (non-negotiable).** TEST-075 EXECUTES parent teardown during `start()`'s nested loop under AddressSanitizer on the existing `testGarminConnectSyncDialogClose` target, and proves BOTH directions: reverting the split — moving `store->open()`/`refreshClicked()` back into the constructor body — must reproduce the ASan `heap-use-after-free`. TEST-076's two slots must each FAIL when their guard is removed. Extend the existing ASan target; no new file unless the build forces it.

**Accepted residuals (recorded, not fixed).** The deliberate busy-teardown store leak from DEC-025 is unchanged. `start()`-based init changes the construction contract for these two callers only; any future third caller of `CloudServiceSyncDialog` must call `start()` — enforced by making the ctor produce a non-functional (un-opened) dialog until `start()` runs.

### Alignment probe
grep -n 'start()' src/Cloud/CloudService.h                                 # expect a new public bool start() on CloudServiceSyncDialog
grep -n 'store->open\|refreshClicked' src/Cloud/CloudService.cpp           # expect these OUT of the ctor body, inside start()
grep -n 'CloudServiceSyncDialog' src/Gui/MainWindow.cpp src/Cloud/AddCloudWizard.cpp  # expect both callers to invoke start() before exec()/open()

## DEC-027 — Unify the sync-dialog lifetime: heap + WA_DeleteOnClose for BOTH callers (closes the BLOCKING A3-R026-F1)
- Status: accepted (A — convert `MainWindow::syncCloud` from a STACK modal dialog to the heap + `WA_DeleteOnClose` + modeless `open()` pattern `AddCloudWizard` already uses; the shared `CloudServiceSyncDialog` is unchanged)
- Reversibility: moderate (one call site changes storage class + modality; the dialog class itself is untouched)
- Decided / last-reviewed: 2026-08-06
- Serves: REQ-017(e) on the stack-caller path; triggered by A3-R026-F1 (BLOCKING); completes the class DEC-024/025/026 left open (the 5th and final route)
- Dependents: TEST-077 (A3-R026-F2 fixture-gap coverage — the 4 previously-unreachable `start()` self-bails), TEST-078 (syncCloud heap-conversion teardown, conditional), `MainWindow::syncCloud` (MainWindow.cpp:2571-2585); context: AddCloudWizard.cpp:892-910 (the reference pattern), DEC-024/025/026

**Problem.** A3-R026-F1 (orchestrator-confirmed self-parenting chain: MainWindow.cpp:143 `WA_DeleteOnClose` + `new Context(this)` :162/2038 + `sync` stack dialog parented to `context->mainWindow` :2583). `MainWindow::syncCloud` builds `CloudServiceSyncDialog` in AUTOMATIC storage parented to the very MainWindow whose teardown is the whole threat scenario. Qt's `QObjectPrivate::deleteChildren()` deletes every child UNCONDITIONALLY of the child's attribute — so parent teardown mid-sync (reachable during `start()`'s non-modal nested loops) calls `delete` on a STACK object (bad-free) then the scope double-destructs it. NO internal `self.isNull()` guard reaches this — the suspended frame is ON the freed object. This is the storage-class axis of the same family DEC-024/025/026 addressed; the prior [[A3-R017-F1]] disposition wrongly declared syncCloud "UNAFFECTED (no WA_DeleteOnClose)" — that attribute governs the CHILD's own close, not parent-initiated deletion (LSN-040).

**Options considered (scored R/S/M/BP 1-5).**
- **A — CHOSEN — heap + `WA_DeleteOnClose` + modeless `open()`.** Make syncCloud construct exactly as `AddCloudWizard` does: `new`, `setAttribute(WA_DeleteOnClose)`, `if (sync->start()) sync->open();` with NO `else delete` (open-failure posts a queued `close()` that self-deletes; an else-delete would double-free). R4 S4 M4 BP4. The dialog becomes a VALID heap child: `deleteChildren` frees it validly, DEC-025's dtor declines the store while `blockingCallDepth>0`, and `start()`'s `self.isNull()` sentinel fires (the dialog IS a child → it gets deleted → self goes null) BEFORE any frame touches the freed `context` — the exact geometry TEST-075 already proves. Unifies both callers on ONE lifetime contract. **Cost (deliberate, user-approved):** menu-triggered sync becomes MODELESS — it no longer blocks the main window — matching `AddCloudWizard`'s already-modeless sync. db ownership is preserved: the dialog's dtor still closes+deletes db (REQ-017(e)), now triggered by `WA_DeleteOnClose` deletion instead of stack-scope exit.
- **C — keep MODAL: heap + `WA_DeleteOnClose` + `QPointer`-guarded `exec()`.** R3 S3 M3 BP3. **Rejected:** the modal `exec()` frame is itself suspended ON the dialog, so it needs a self-check after `exec()` returns PLUS the dtor guard PLUS a new ASan test for the exec-suspended-frame teardown — more lifetime code and a second lifetime pattern, to dodge a hazard A removes structurally. Chosen only if modality were a hard requirement; it is not.
- **B — reparent the stack dialog to `nullptr`.** R2 S2 M2 BP2. **Rejected:** fixes the stack bad-free but the dialog is then NOT a child, so `start()`'s `self.isNull()` sentinel never fires and it proceeds to touch the freed `context` (owned by MainWindow) — a context use-after-free. Trades one UAF for another.

**Cascade.** `MainWindow::syncCloud` (MainWindow.cpp:2571-2585): stack `CloudServiceSyncDialog sync(...)` + `if (sync.start()) sync.exec();` → heap `new` + `setAttribute(WA_DeleteOnClose)` + `if (sync->start()) sync->open();`, no else-delete, db-ownership comment updated. `CloudServiceSyncDialog` itself is UNCHANGED — it already supports this pattern (AddCloudWizard uses it). DEC-024/025/026 machinery + TEST-070..075 unchanged/green. **Rider (A3-R026-F2, folded in):** extend the ASan fixture so the 4 currently-unreachable `start()` self-bails become testable — add an open-failure `BlockingStore::open()` mode (reaches :778/:782), a non-empty dirty `rideCache` (reaches :993), and a teardown armed during the readdir/`refreshClicked` frame (reaches :1023) — TEST-077, each guard RED-verified.

**Evidence bar.** TEST-077's four slots each FAIL when their guard is neutered (the F2 gap closed). The syncCloud conversion inherits TEST-075's proven parent-teardown geometry (both callers now identical); optionally TEST-078 drives a parent teardown through the syncCloud entry point specifically. Baseline: ASan target green, `ctest -R "Garmin|AtomicFile"` ≥25, full ≥26, `GoldenCheetah` links. DEC-024/025/026 byte-unchanged.

**Accepted residuals.** Menu-sync is now modeless (user-approved UX change). db lifetime is tied to `WA_DeleteOnClose` deletion, exactly as `AddCloudWizard`'s already is. A3-R026-F3 (no start()-called/re-entrancy invariant) stays a latent advisory — inert with the two known callers.

### Alignment probe
grep -n 'new CloudServiceSyncDialog\|WA_DeleteOnClose\|->start()\|->open()\|->exec()' src/Gui/MainWindow.cpp   # expect heap+WA_DeleteOnClose+open(), NO stack `sync`, NO exec()
grep -n 'CloudServiceSyncDialog sync' src/Gui/MainWindow.cpp                                                    # expect GONE (no stack construction)

## DEC-028 — Clean-checkout build repair: commit the missing wiring rather than guard around it (ORCH-001)
- Status: accepted (B — commit `unittests/Core/coach/CMakeLists.txt` + `stubs/` so committed CMake refers only to committed files)
- Reversibility: high (build-definition only; every part is a one-line revert, no product code changed)
- Decided / last-reviewed: 2026-08-07
- Serves: ORCH-001 (user dispositioned "fix it"); unblocks the mandated clean-worktree configure+build gate that has never once run in this project
- Dependents: commit `427da745b`; ORCH-005/006/007 (the three further causes the build gate exposed); the branch-merge gate for `garmin/req017-lifecycle-uaf`

**Problem.** ORCH-001 recorded that `master` does not configure from a clean checkout, from two causes. The decision point was cause 2: `unittests/CMakeLists.txt:1` unconditionally `add_subdirectory(Core/coach)` while `unittests/Core/coach/CMakeLists.txt` and its `stubs/` were never committed — even though `testCoachTools.cpp` and `coach.pro` WERE. The repair lands in files reserved for the pre-session Coach work's owner, so the scope was the user's call.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — commit the missing wiring** (`CMakeLists.txt` + 5 stub headers, 6 small files). R4 S4 M3 BP5. Root-cause fix: committed CMake then references only committed files, and the ALREADY-TRACKED `testCoachTools` actually builds. Every `src/Coach/*` source the wiring consumes (`GCToolExecutor.cpp`, `ToolConfirmCard.cpp`, `PlanPreviewCard.cpp`, `LLMService.h`) was already tracked, so nothing of the Coach feature itself is adopted. **Cost:** 6 files of the owner's WIP enter master, and the stubs may churn under their refactor.
- **A — guard `add_subdirectory` with an `EXISTS` check.** R4 S3 M4 BP2. **Rejected:** one line and zero ownership entanglement, but it leaves a TRACKED test permanently unbuilt from clean, silently — the precise "green but dead" class that produced REQ-018 and ORCH-001 itself. It masks the defect instead of fixing it, and would hide the NEXT forgotten CMakeLists.
- **C — both (commit AND guard).** R5 S4 M3 BP4. **Rejected:** the guard is cheap insurance but carries A's masking cost for future recurrences; with the files committed it protects only against someone deleting them, which git already reports.

**Cascade.** `unittests/Core/coach/{CMakeLists.txt,stubs/*.h}` become tracked (6 files) — WIKI MAP `unittests/` line updated. `unittests/CMakeLists.txt` is UNCHANGED (no guard added). Cause 1 (8 phantom refs) had no trade space and was fixed mechanically. **The repair then TRIPLED in scope**, because running the actual build exposed three further pre-existing causes invisible to configure: ORCH-005 (translations/lrelease never ported to CMake), ORCH-006 (11 tracked sources absent from the CMake lists, so HEAD could not link), ORCH-007 (`CMAKE_CXX_EXTENSIONS OFF` diverging the dialect from qmake and breaking the `QBluetoothUuid` link). All fixed in the same commit; see their findings rows.

**Evidence bar.** Verified against a clean extract of the STAGED tree (`git archive` of the index — never the working tree, which is dirty with unrelated work): configure+generate succeed, lrelease emits all 13 `.qm`, the full build links `GoldenCheetah` (27.9 MB) plus 26 test executables including `testCoachTools`, and `ctest` is **26/26**. The committed tree hash was confirmed byte-identical to the verified tree (`d8dfe490…`), so there is no gap between the evidence and the commit. Two-directional check on ORCH-006/007: with cause 5 alone reverted the link yields EXACTLY ONE undefined reference, at `KurtInRide.cpp`.

**Accepted residuals.** (i) `CMAKE_CXX_EXTENSIONS ON` now permits GNU extensions project-wide — this CONVERGES on the qmake reference build rather than diverging from it, but it does relax strict-ISO enforcement. (ii) The uncommitted `src/Train/KurtInRide.cpp` workaround in the working tree is now REDUNDANT and should be dropped by its owner rather than committed — it hand-rewrites byte-order handling to work around what ORCH-007 shows is a build-configuration defect. (iii) The `.qm` are generated into the SOURCE translations dir (matching qmake's `TS_DIR`) because `application.qrc` addresses them relative to the `.qrc`; they stay gitignored, so this does not dirty `git status`. (iv) All other pre-session Coach/libusb/Calendar work in both CMakeLists remains uncommitted and untouched.

### Alignment probe
grep -n 'DiaryWindow\|Velohero\|DiarySidebar' src/CMakeLists.txt        # expect NOTHING (8 phantom refs gone)
grep -n 'CMAKE_CXX_EXTENSIONS' CMakeLists.txt src/CMakeLists.txt        # expect ON in both
grep -n 'qt_add_translation\|LinguistTools\|AUTOGEN_TARGET_DEPENDS' src/CMakeLists.txt   # expect all three present
git ls-files unittests/Core/coach                                        # expect CMakeLists.txt + stubs/ + coach.pro + testCoachTools.cpp

---

## DEC-029 — Upload-dialog UAF fix shape: two-phase init + heap/WA_DeleteOnClose, MODAL preserved (REQ-019)
- Status: accepted (B — two-phase `start()` + heap + `WA_DeleteOnClose`, `exec()` retained, store ownership UNCHANGED)
- Reversibility: moderate — the heap/`WA_DeleteOnClose` conversion is effectively a one-way door already opened by DEC-027's stack-bad-free finding (reverting to stack allocation reopens a closed BLOCKING route). Switching B→A (async) afterward is a confined re-edit of `CloudService.{h,cpp}` + `MainWindow.cpp:2548-2565`; no on-disk artifact or schema involved.
- Decided / last-reviewed: 2026-08-07
- Serves: REQ-019 (A3-R027-F1, HIGH — live for 11 Upload-capable services); constrained by DEC-024, DEC-025, DEC-026, DEC-027 (the proven sync recipe) and by REQ-017's store-ownership contract (MainWindow.cpp:2558-2562)
- Dependents: REQ-019 build slice; TEST-079, TEST-080; ORCH-008 (test-home misnomer); REQ-020 (inherits whichever harness placement this establishes)

**Problem.** `CloudServiceUploadDialog`'s constructor (CloudService.cpp:326-412) performs every blocking operation while the object is a STACK dialog parented to the `WA_DeleteOnClose` MainWindow: `store->open()` (:346, a real nested `QEventLoop` over network I/O), an unsaved-changes `QMessageBox::exec()` (:363), `compressRide`/`writeFile` (:386/:389), and a failure `QMessageBox::exec()` (:401) — with zero `QPointer` self-bails. A parent teardown inside any of those loops resumes the ctor on freed memory, and it then touches `context->mainWindow->saveSilent` (:369) and `QWidget::hide()` (:403). The single caller `MainWindow::uploadCloud` (MainWindow.cpp:2548-2565) passes `this` as parent and then runs `closeAndDeleteStore(db)` at :2563 — a member call on a possibly-destroyed MainWindow. This is the pre-DEC-024..027 bug verbatim, in a sibling dialog.

**Two structural facts that make this NOT a straight recipe transfer** (scout-found, orchestrator-verified at the cited lines):
1. **No user-close route exists during the blocking work.** `okcancel` is created at :338 but is not connected to any slot until `completed()` fires (:431), and the dialog is never shown or `exec()`'d until after the ctor returns (`CloudService::upload` :82-83). DEC-024's `done()`/`closeEvent()`/`deferCloseIfBusy` gate exists to stop a *user-initiated* close racing a blocking call — a hazard that is structurally absent here. Transferring that machinery would add untested, unreachable code.
2. **Async conversion would force an ownership change.** `writeFile` is already asynchronous (completion arrives via `writeComplete` → `completed()`, :410/:425), and `exec()` (:415) is what waits for it. Making the dialog modeless means `uploadCloud`'s synchronous `closeAndDeleteStore(db)` at :2563 races the in-flight upload, so the store would have to move into the dialog — inverting the REQ-017 contract shipped days ago in `ae5a7a8ab`.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — two-phase init + heap/`WA_DeleteOnClose`, modal `exec()` preserved, caller keeps store ownership.** R5 S3 M5 BP5. Ctor becomes the widget shell only; a new `bool CloudServiceUploadDialog::start()` carries the body from `store->open()` onward with a `QPointer<CloudServiceUploadDialog>` self-bail after each of the four blocking calls, every result landed in a LOCAL (LSN-037). the construction site becomes heap + `WA_DeleteOnClose` + `if (start()) exec();`. **[CORRECTED 2026-08-08 post-build, O-R019-01 — this clause originally said "`uploadCloud` becomes…", naming the wrong function. `MainWindow::uploadCloud` does NOT construct the dialog; `CloudService::upload` (CloudService.cpp:78-87) does, and uploadCloud (MainWindow.cpp:2556) is only its single caller. The conversion landed in `CloudService::upload`, which is the same geometry this DEC intended and additionally avoids editing another owner's dirty file; `MainWindow::uploadCloud` took a comment-only update. The error was internal inconsistency — the correct site is cited accurately three paragraphs above in fact 1 — not an ungrepped premise.]** **Deliberately OMITS** the DEC-024 close-gate per fact 1. `closeAndDeleteStore(db)` at :2563 stays exactly where it is; the REQ-017 comment stays true. Smallest change that makes the hazard structurally impossible (user constraint #3) with zero ownership cascade (constraint #2).
- **A — full DEC-024..027 transfer: async/modeless, dialog takes store ownership.** R4 S5 M3 BP4. **Rejected:** highest precedent-fidelity and it would make uploads non-blocking, but it buys that with (i) a `writeComplete`-vs-late-teardown race that has no precedent and no test, (ii) deleting the `closeAndDeleteStore(db)` call and rewriting the REQ-017 contract comment, and (iii) making Upload non-modal for all 11 services — a separate UX decision from the one taken for sync in DEC-027, where the open-ended duration justified it. Upload is per-ride and short.
- **C — reparent the dialog to `nullptr`.** R2 S2 M2 BP2. **Rejected:** removes only the dialog-as-child free while leaving the `context`/`context->mainWindow` UAF exactly as exposed, so it still needs B's guards — strictly less protection for the same code. Also orphans a floating progress dialog that outlives its athlete window (unclear interaction with `quitOnLastWindowClosed`). This is the shape DEC-025 and DEC-027 each already examined and rejected for the structurally identical sync case.

**Blast radius.** 11 Upload-capable services reach this dialog — verified by reading each `capabilities()` override plus the subclasses that inherit the `CloudService.h:104` base default: explicit bit — RideWithGPS, CyclingAnalytics, Selfloops, SportsPlusHealth, TrainingsTageBuch, Xert; inherited default — Strava, Dropbox, SixCycle, SportTracks, LocalFileStore. **GarminConnect is NOT among them** (`GarminConnect.h:76` = `Query|Download`, read-only by DEC-005): this ledger's own subject is untouched by its own REQ. Under Option B the only observable change for all 11 is stack→heap allocation — still modal, still blocking, no UX delta.

**Evidence bar (non-negotiable, lifetime slice).** Acceptance is an EXECUTED ASan test, not reasoning about Qt semantics. **One claim in this decision is reasoned and not yet executed** and must be the FIRST RED test written: that `QDialog::exec()` self-protects when `this` is destroyed mid-loop, which is what lets `closeAndDeleteStore(db)` stay unguarded at :2563. This is the same class of assumption as DEC-024's original `closeEvent()` premise, which proved incomplete. **If it does not hold, Option B is falsified in part** and needs its own gate around `exec()` — re-open this DEC rather than patching around it.

**Test home (decided with the user).** The new slots go into the EXISTING ASan target `testGarminConnectSyncDialogClose` / its directory `unittests/Core/garminconnect/`, reusing the offscreen-QApplication + `-fsanitize=address` + blocking-store-stub harness (CMakeLists.txt:1339-1457). This is a knowing misnomer — the code under test is generic `CloudService`, not Garmin — accepted to reach executed evidence fast rather than front-loading a harness relocation. Registered as **ORCH-008** so the debt is visible; REQ-020 will inherit the same placement and strengthens the case for generalizing later.

### Alignment probe
grep -n 'bool start()' src/Cloud/CloudService.h                          # expect the new CloudServiceUploadDialog::start()
grep -c 'self.isNull()' src/Cloud/CloudService.cpp                        # expect >= 6 upload self-bails (CORRECTED 2026-08-08, O-R019-01: the original probe counted 'QPointer<CloudServiceUploadDialog>' and expected >=4, which the DEC-026 idiom this same DEC mandates makes UNSATISFIABLE — that idiom declares ONE QPointer per function and bails off it N times. Actual: 2 textual QPointer occurrences, 6 self-bails. Count the bails, not the declaration.)
grep -n 'closeAndDeleteStore(db)' src/Gui/MainWindow.cpp                  # expect STILL PRESENT in uploadCloud (~:2563) — ownership unchanged
grep -n 'deferCloseIfBusy\|closeEvent' src/Cloud/CloudService.cpp         # expect NO new upload-dialog close-gate (DEC-024 machinery is sync-only)
grep -n 'WA_DeleteOnClose' src/Gui/MainWindow.cpp                         # expect it on the new heap upload dialog

---

## DEC-030 — Collaborator-lifetime UAF fix shape: reparent to AthleteTab, PROBE-FIRST, with QPointer guards as a rider (REQ-021)
- Status: accepted (B — reparent both dialogs to `context->tab`, **conditional on an executed visibility probe**; Option A's collaborator `QPointer` guards ship alongside as a rider, NOT as an alternative)
- Reversibility: cheap but ASYMMETRIC — reverting the reparent after ship is a SECOND user-visible window-behaviour change across 17 shipped integrations, so the cost of being wrong is paid twice. This is precisely why the probe gates it. The harness work (below) is unconditionally reversible and unconditionally useful (required by every option, and by REQ-020 after this), so it is not a bet.
- Decided / last-reviewed: 2026-08-10
- Serves: REQ-021 (A3-R019-F1 BLOCKING + A3-R019-F2 live-on-master); constrained by DEC-024, DEC-025, DEC-026, DEC-027, DEC-029 (the dialog-lifetime recipe, explicitly NOT re-litigated) and by the standing evidence bar for lifetime slices (executed ASan test, guard on the layer performing the unsafe operation)
- Dependents: REQ-021 build slice; TEST-081..084; REQ-020 (inherits the harness AND the recipe); REQ-022, REQ-023 (new, this decision's sibling-scan byproducts); ORCH-008 (naming debt grows by one more non-Garmin subject); A3-R019-F3, B-R019-04, B-R019-05 (all closed by the harness half); S-R021-01 (folded in by user decision)

**Problem.** Five decisions (DEC-024/025/026/027/029) and four adversarial cycles closed a UAF class by guarding **the dialog's own lifetime** — `QPointer<T> self(this)` bails after every nested loop. That guard covers exactly ONE pointer: `this`. `context` and `item` are owned by the **AthleteTab**, which `MainWindow::removeAthleteTab` (MainWindow.cpp:2183-2185) deletes SYNCHRONOUSLY, while MainWindow's own `WA_DeleteOnClose` deletion is DEFERRED. So the dialog reliably OUTLIVES its Context, every `self.isNull()` stays false, and the resumed frames dereference freed memory. Both dialogs are affected; the sync half is already shipped on master in `ae5a7a8ab`.

**Three scout-found facts that shaped this decision** (all orchestrator spot-checked at the cited lines, 2026-08-10):
1. **`Context` and `RideItem` are BOTH `QObject`s** (`src/Core/Context.h:106`, `src/Core/RideItem.h:41`). `QPointer` works on them; no liveness registry is needed. This was flagged as check-do-not-assume in the briefing, and it is what makes Option C (a hand-rolled abandonment signal) redundant rather than merely costly.
2. **The sync dialog's collaborator surface is ~8x larger than REQ-021's stub stated** (S-R021-02). `MainWindow::syncCloud` (MainWindow.cpp:2604-2606) opens it MODELESSLY (`open()`, not `exec()`), so it outlives its caller and keeps running slots. Beyond the two `start()` sites, `context->` is dereferenced at NINE further sites in FIVE further slots: `refreshClicked` (:1318, :1371, :1443), `syncNext` (:1743, :1788), `downloadNext` (:1857), `uploadNext` (:1985), `saveRide` (:2064, :2088). **This is the decisive fact: a per-site fix closes ~18% of the surface; a structural one closes 100% for the same two lines.**
3. **A live `this`-axis gap remains on master** (S-R021-01): the sync dialog's Cancel branch runs `processEvents()` (:1107) then `invokeMethod(this,"close")` (:1108) with NO `self.isNull()` between them, while the upload dialog's identical branch DOES carry it (:451-452). Folded into REQ-021 by user decision.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — reparent both dialogs to `context->tab`.** R5 S5 M5 BP4. Two lines: `CloudService::upload` (:95) passes `context->tab` instead of `parent` (verified: exactly ONE production caller, MainWindow.cpp:2556), and the sync ctor (:825) becomes `QDialog(context->tab, Qt::Dialog)` (covers both construction sites at once). `context->tab` is an `AthleteTab*` (Context.h:135) and `AthleteTab : public QWidget` (AthleteTab.h:32). `delete tab` (:2183) precedes `delete athlete` (:2184) and `delete context` (:2185) — and `delete context` occurs EXACTLY ONCE in all of `src/` (grepped). So the dialog is destroyed by `deleteChildren()` strictly BEFORE the objects it points at, every existing `self.isNull()` bail becomes true again, and the nine modeless sync sites are covered for free. **It adds nothing to DEC-024..029; it restores the ordering precondition those decisions were designed against.**
- **A — per-site collaborator `QPointer` guards.** R4 S3 M4 BP4. **Rejected as the primary, RETAINED as a rider.** Widening ~11 bail sites to `self.isNull() || ctx.isNull() || ride.isNull()` uses the one mechanism already mutation-proven load-bearing in this exact file, with no unverified premise and no window-behaviour change. But it is per-SITE: it protects only the sites someone remembered, and every future blocking call added to shared `CloudService.cpp` owes two more checks with silent failure if forgotten. **It ships anyway as a rider** because it covers the one window B cannot — see below.
- **C — Context-driven abandonment signal.** R4 S4 M2 BP3. **Rejected.** `Context` already emits `athleteClose` (Context.h:181/288) from `Athlete::close()` (Athlete.cpp:214), called at MainWindow.cpp:2171 — twelve lines before `delete context`, on both routes — so the trigger genuinely exists. But it introduces a THIRD lifetime protocol alongside DEC-024's close-gate and DEC-025's blocking-depth decline, every future cycle must reason about all three interacting, and it needs explicit checks at ~40 sites anyway. It is the right answer in a codebase where the collaborators are NOT QObjects; here they are (fact 1). It also has a harness problem the others don't: `Athlete::close()` is stubbed in NEITHER `ImportSeamStubs.cpp` nor `SyncDialogSeamStubs.cpp`, so a test would drive the slot rather than the trigger — the exact fidelity gap that produced A3-R019-F3.

**THE GATING PROBE (this is what "probe-first" means, and it is not optional).** Option B rests on ONE Qt behaviour the scout could not verify from a primary source: **does hiding an `AthleteTab` also hide a child `QDialog` window in Qt 6.8.2?** `MainWindow::switchAthleteTab` (:2165) hides tabs on every athlete switch. Web search returned only undated forum anecdotes, no Qt source or documentation. This project's standing rule is that framework semantics are decided by execution, not reasoning — and its own history (DEC-024's `closeEvent` premise; the `deleteLater` loop-level claim in TEST-070's header) is that this exact class of plausible Qt claim is where it goes wrong. **Therefore: build the harness FIRST, run a ~20-line offscreen probe (parent a QDialog to a child QWidget, `open()` it, hide the parent, assert `isVisible()`), and only then write a production line.** If child windows follow the parent's hide, **Option B is dead on UX grounds and the fix falls back to Option A applied to ALL ELEVEN sites** (both `start()`s plus the nine modeless derefs) — pre-authorised by the user on 2026-08-10 so this does not require a second decision round.

**The rider, and why it is one decision and not a fourth option.** Option A's collaborator `QPointer`s ship in both `start()` methods regardless of the probe outcome. `MainWindow.cpp:2148-2171` runs `rideCache->cancel()` (:2148), `namedSearches->write()` (:2151), `tab->close()` (:2170) and `athlete->close()` (:2171, which runs autobackup) **BEFORE** the three deletes. A frame resuming inside any of those sees a partly-torn-down Athlete with all pointers still non-null. B fixes the destruction ORDER; it does not cover the pre-delete window. (Whether any of those four calls actually spins an event loop is UNVERIFIED — the scout read the call sites, not the implementations. The rider makes that question non-load-bearing, which is the point.)

**Blast radius.** Both dialogs live in shared `src/Cloud/CloudService.cpp`, reached by all **17** `CloudService` subclasses — enumerated by predicate (`grep -rn "public CloudService" src/Cloud/*.h`, one line per hit, membership not proximity, per LSN-034): CalDAVCloud, Dropbox, Azum, PolarFlow, CyclingAnalytics, Strava, LocalFileStore, GarminConnect, RideWithGPS, Selfloops, SportsPlusHealth, Nolio, Withings, SixCycle, SportTracks, Xert, TrainingsTageBuch. Option B changes VISIBLE window parenting on that path, which is the whole reason for the probe and for the two extra control assertions in the acceptance criterion.

**Test home.** The existing ASan target `testGarminConnectSyncDialogClose` (`unittests/Core/garminconnect/`), extended — no new target, no CMake wiring (`stubs/ImportSeamStubs.cpp` is already in its source list at CMakeLists.txt:1356). ORCH-008's naming debt grows by one more non-Garmin subject; REQ-021 is the THIRD non-Garmin lifetime slice, which is ORCH-008's own stated trigger to reconsider extraction. **Flagged, deliberately NOT acted on inside this REQ** (user-confirmed 2026-08-10).

**The harness half of this decision (mandatory, sequenced FIRST).**
1. **A purpose-built fake owner**, `FakeAthleteWindow`, living in the test `.cpp` (not the stubs): a `QWidget` holding `tabWidget` / `context` / `athlete` / `item`, with `closeAthleteTab()` doing `delete tabWidget; delete athlete; delete context;` SYNCHRONOUSLY (models MainWindow.cpp:2183-2185) and `closeWindow()` doing that for every tab then `deleteLater()` on itself (models closeEvent, MainWindow.cpp:1102-1103). The existing `killOwner` helper CANNOT be patched into shape — it deletes owner and context back-to-back, so the dialog always dies first and `self.isNull()` is already true on resume, making the collaborator axis unobservable by construction. Both production routes must be drivable: single-tab close (window survives, `AthleteView.cpp:213`) and whole-window close. The SAME class serves Option A (dialog child of the window) and Option B (dialog child of `tabWidget`), and hosts the visibility probe — which is why it is built first regardless of outcome.
2. **Member touches in `stubs/ImportSeamStubs.cpp`** so freed collaborators actually FAULT for the reason the test claims: `Context::metadataFlush()` (:212) and `RideItem::notifyRideMetadataChanged()` (:312) each load a real member; `MainWindow::saveSilent` (:383-387) can only do a QObject-level load (`isWidgetType()`) because the target's "MainWindow" is a `reinterpret_cast` of a plain QWidget (test file :1539) — **record that as a PRAGMATIC close of B-R019-05, not a strict one.** Also initialise `RideItem::isdirty` (:297), uninitialised today, which is B-R019-04 and fails as a HANG.
3. **Closes as a byproduct:** A3-R019-F3, B-R019-04, B-R019-05. Does **NOT** close A3-R019-F5 (7 of 9 slots hand-copy `CloudService::upload`), which stays open.

**Folded in by user decision (2026-08-10):** S-R021-01 — add the missing `if (self.isNull()) return false;` between CloudService.cpp:1107 and :1108.

**Alignment probes** (each RUN against the current tree before this DEC shipped, per LSN-045 — all fail NOW in the way that proves they will pass after):
```
grep -n 'class Context : public QObject' src/Core/Context.h                    # PASSES NOW (=1) - premise, not outcome
grep -n 'class RideItem : public QObject' src/Core/RideItem.h                  # PASSES NOW (=1) - premise, not outcome
grep -c 'delete context;' src/Gui/MainWindow.cpp                               # expect 1, unchanged by this DEC
grep -n 'context->tab' src/Cloud/CloudService.cpp                              # expect 0 NOW, >=2 after (Option B only)
sed -n '1107,1108p' src/Cloud/CloudService.cpp                                 # expect NO self.isNull() NOW, one after (S-R021-01)
grep -n 'FakeAthleteWindow' unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp  # expect 0 NOW, >0 after
```

---

## DEC-031 — Reopening DEC-025: a frame-counted DEFERRED REAPER replaces the deliberate store leak (REQ-021)
- Status: accepted (B — frame-counted deferred reaper, **conditional on an executed Qt loop-level probe**)
- Reversibility: cheap — one destructor branch, one `~BlockingCall` branch and a small record type, all inside `CloudService.h/.cpp`; the revert is a single-file diff and TEST-084's assertions revert with it. No subclass edits, no call-site edits, no user-visible behaviour change.
- Decided / last-reviewed: 2026-08-11
- Serves: REQ-021 (A3-R021-F4); **REOPENS DEC-025**; constrained by DEC-024 (`BlockingCall`), DEC-030 (the reparent that forced this), and REQ-017 (b)/(e) store ownership — *note: an earlier draft of this briefing miscited that contract as "DEC-017", which is actually REQ-008 sidecar-persistence shape; see O-R021-03*
- Dependents: TEST-084 (its assertions INVERT — see evidence bar), TEST-072 (inverts identically), TEST-073 (unchanged, and becomes the discriminator that the reaper did not merely move the leak); REQ-024 (independent, unaffected)

**Why DEC-025 is reopened rather than amended.** DEC-025 decided that `~CloudServiceSyncDialog` declines to delete the store while `blockingCallDepth > 0`, deliberately leaking it. Its rationale had exactly ONE load-bearing clause, stated verbatim in the code: *"Nothing is deferred: at parent-teardown time the application is already tearing down, so there is no live event loop left for a reaper to run on."* **DEC-030's reparent falsified it.** The dialog is now a child of `context->tab`, and `MainWindow::removeAthleteTab` deletes that tab on a route after which **the application keeps running with a live event loop**. A reaper is therefore not merely possible — it is the option DEC-025 said it would have taken had this fact held. Amending the comment would have left an accumulating leak blessed by a rationale that no longer exists → [[LSN-052]].

**Population, corrected (scout, orchestrator-verified).** The leak's trigger is NARROWER than A3-R021-F4 stated: with exactly one athlete tab, `MainWindow::closeAthleteTab()` takes the `closeWindow()` branch (MainWindow.cpp:2129-2130), NOT `removeAthleteTab`. The app-survives routes require **≥2 athlete tabs or ≥2 MainWindows**. The substance (routine, app keeps running, accumulating) survives.

**Leak quantified (this is what decided it).** The store is a **parentless** `QObject` (`CloudService::CloudService(Context*)` — `context` is a collaborator, not a QObject parent), so nothing in the tree can ever free it. Transitively: the subclass + its `QNetworkAccessManager`; the entire `list_` of `CloudServiceEntry`, which only `~CloudService` frees and which `newCloudServiceEntry` appends to on EVERY `readdir` (≈150–250 KB for a 12-month, ~500-activity listing, scaling with listing volume × refresh count). **For 16 services that is ~0.05–0.5 MB per occurrence and no threads. For GarminConnect it is categorically different:** `~GarminConnect` and `GarminConnect::close()` are what `quit()+wait()` the download worker **QThread** and free `m_adapter`, the **embedded CPython session** — so a leaked Garmin store means **one OS thread left running and one interpreter session resident, per occurrence, for the process lifetime**, not boundable from the code. That asymmetry, not the byte count, is the argument.

**Options considered (scored R/S/M/BP 1-5).**
- **B — CHOSEN — frame-counted deferred reaper.** R4 S4 M3 BP4. The decline branch hands `(store, depth)` to a small refcounted orphan record instead of dropping it; `~BlockingCall`, on its `dialog.isNull()` stand-down path, releases one frame; the last release runs `closeAndDeleteStore`. Entirely within `CloudService.h/.cpp` — **zero edits to any of the 17 subclasses, zero call-site edits, zero user-visible change**, which matters because DEC-030 already spent this REQ's entire behaviour-change budget on the reparent. Failure mode on an unwrapped nested loop is *today's* behaviour, never worse.
- **C — the store guards its own lifetime** (busy-count moves INTO `CloudService`; the store defers its own destruction). R5 S4 M2 BP4. **Rejected for THIS REQ, and it is the technically superior answer** — the only option that closes S-R031-01 and A3-R021-F1 *by construction*, because the guard would live in the store's own frames rather than in the dialog's call sites. Rejected because it lands a correctness obligation on ~17 subclasses inside code that **A3-R021-F8 proved has ZERO test coverage** (no test executes `syncNext`/`downloadNext`/`uploadNext`/`completedRead`/`completedWrite`/`saveRide`), inside a REQ already carrying two BLOCKING findings — which is exactly how the DEC-024→025→026 recurrence happened. It is the right REQ-scale follow-up, not the right move here.
- **A — keep the leak, re-decided and bounded** (+ wrap the two latent `compressRide`/`writeFile` pairs). R2 S2 M4 BP2. **Rejected:** smallest diff and the only option needing no test changes, but it re-blesses an accumulating leak whose justification is gone, and **it provably cannot reach S-R031-01** — see the invariant note below.
- *Rejected before scoring, so the list is not padded:* parenting the STORE to `context->tab` (`delete tab` is the same synchronous instant — the UAF returns identically); parenting it to `context->mainWindow` (the pre-REQ-017 leak-to-exit with extra steps); a plain `store->deleteLater()` (actively unsafe — see the probe).

**THE GATING PROBE (same discipline as DEC-030, which is why that decision survived contact with reality).** Qt documents that `deleteLater` performs the deletion when control returns to **the event loop from which it was called** — here that loop IS the store's own suspended nested loop, so a naive `store->deleteLater()` (or a queued `invokeMethod` posted from inside that loop) **would delete the store under its own frame**. That is why the frame-counted variant is the only sound shape. **This project has already had one `deleteLater` loop-level claim falsified** (cited in DEC-030 re TEST-070's header), and its standing rule is that framework semantics are decided by execution, not documentation. **Therefore: before any production line, run a ~20-line offscreen probe that calls `deleteLater()` and a queued `invokeMethod` on an object from inside a nested `QEventLoop` and asserts whether the object dies before that loop returns.** Ship the probe's positive control in the same slot ([[LSN-050]]).

**A specification the builder must NOT be left to discover** (scout's open question 2): when TWO sync dialogs have suspended frames on the same stack, a single static counter is WRONG. The orphan record must be **per-dialog and refcounted, created by the first `BlockingCall`**. This is the implementation's main risk and is specified here deliberately.

**The `BlockingCall` invariant — split answer, and the split is the point (scout, orchestrator-verified).**
- The two unwrapped `compressRide`/`writeFile` pairs are in **`syncNext` and `uploadNext`** (NOT `saveRide`, which makes no store calls at all — O-R021-03). They are a **separate, LATENT defect**: all 11 `writeFile` implementations were checked and **none runs a nested loop**, and `compressRide` is `QTemporaryFile` + `writeRideFile` + `ZipWriter` with no loop. Pre-emptive consistency; does not gate this DEC.
- **But the invariant as WRITTEN is unachievable**, and that IS part of this decision. `Strava::readFileCompleted` enters a nested `QEventLoop` from a **signal-delivery frame the dialog never calls** (S-R031-01, → REQ-024) — you cannot wrap a call site that does not exist. **So `blockingCallDepth` is not a complete predicate.** Option A rested on it entirely; **B rests on it only for the leak-fixing property and degrades to today's behaviour where it fails**; C would not rest on it at all. DEC-024's invariant text must be narrowed to what is true: *every store call the DIALOG makes that can run a nested loop is wrapped* — it says nothing about loops the store enters on its own.

**Evidence bar (user decision 2026-08-11): TEST-084's inversion belongs to THIS DEC, and BLOCKS REQ-021 from closing.** TEST-084's `assertSyncDiedWithItsTab` currently asserts `!storeDestroyed` / `!storeClosed` across 8 runs — i.e. the suite encodes the leak as REQUIRED. Under B those invert to *"closed and deleted exactly once, AFTER the nested loop returned"*; TEST-072 inverts identically; **TEST-073 (the idle control) stays unchanged and becomes the discriminator that the reaper did not merely move the leak**. REQ-021 cannot close while a test in its own changeset asserts behaviour this decision overturns. A new RED direction comes free: drop the `release()` call and TEST-072/084 must fail on a POSITIVE assertion — which means leak coverage stops depending on `detect_leaks`, currently disabled on this target (A3-R027-F4).

**Alignment probes** (to be RUN against the tree before the build ships, per [[LSN-045]]):
```
grep -n 'blockingCallDepth > 0' src/Cloud/CloudService.cpp        # the decline branch this DEC replaces
grep -c 'deleteLater' src/Cloud/CloudService.cpp                   # expect NO naive store->deleteLater() after
grep -n 'storeDestroyed\|storeClosed' unittests/Core/garminconnect/testGarminConnectSyncDialogClose.cpp   # assertions that must INVERT
```
