# Definition of Done — Garmin Connect Integration (Phase 1)

Published as part of Phase 2.1 bootstrap (per the quality-gated-dev-workflow skill).
Each requirement category below states the minimum a TEST-NNN / COMMIT-NNN must satisfy
to be considered "done." Phase 2 exit CLV (Check 1, Check 4) will check against this.

---

## All requirements (universal floor)

A requirement is not "done" until **every** item below is true:

- [ ] One or more `TEST-NNN` files cover the acceptance criterion (RED → GREEN → REFACTOR).
- [ ] The test is REGISTERED with CTest (`add_test`), or lives under
      `src/Python/garminconnect/tests/` (Python). Registration is now sufficient:
      per **DEC-054** the routine gate is `ctest -LE gate-exclude`, which is
      DEFAULT-INCLUDE, so a registered test is gated whether or not anyone
      remembers to label it. A label is still useful for running a suite in
      isolation; it is no longer what puts the test in the gate.
      **This item used to read "registered under the `garmin-fast` CTest label
      ... so pre-commit and CI pick it up", and that was the defect, written
      down as a rule.** It made enrolment the verification step: a test that
      was registered but unlabelled was invisible to the habitual gate and its
      absence was silent, which is how `testGarminI18nSourceGuard` sat RED in
      HEAD from `c1948b513` (2026-09-13) to 2026-09-15 across two Inspectors'
      own independent re-runs. The "so pre-commit and CI pick it up" clause was
      additionally false in both halves: `.pre-commit-config.yaml` contains no
      ctest hook (DEC-054 ratifies that, superseding DEC-010's cascade prose),
      and this repository contains no checked-in CI workflow.
- [ ] The routine gate `ctest -LE gate-exclude` is GREEN, and evidence lines
      cite THAT command. A bare `ctest` remains the exhaustive audit and is not
      weakened; `ctest -L garmin-build-guard` is the slower tier under which the
      two flag-build guards are run WHEN they are run (~503s of scratch
      configure+build, measured — DEC-054).
      **Say it precisely: nothing in this repository invokes that command.**
      There is no CI workflow and no scheduler, so those two tests run only when
      a human types it. They were equally unrun under the previous
      `ctest -L garmin-fast` gate, so this is not a regression introduced here —
      but calling it a "scheduled" tier, as an earlier draft of this line did,
      would be a claim nothing backs. The missing scheduling mechanism has its
      own ledger row.
      Excusing a test from the routine gate means giving it the `gate-exclude`
      label PLUS `GATE_EXCLUDE_REASON` and `GATE_EXCLUDE_TIER` test properties;
      an opt-out without a recorded reason is itself a defect, and
      `testGarminGateCoverageGuard` enforces exactly that.
      **Caveat, stated rather than implied:** enforcement today is this
      documented command plus the coverage guard. There is no CI job requiring
      it, so "required merge validation" is an aspiration, not a mechanism.
- [ ] `traceability.md` REQ row has TEST and COMMIT columns populated.
- [ ] Commit message cites `REQ-NNN`, `TEST-NNN`, and the relevant `DEC-NNN`(s)
      (convention, not hook-enforced — see `.pre-commit-config.yaml` rationale).
- [ ] All hooks in `.pre-commit-config.yaml` pass on the staged files
      (clang-format, ruff, ruff-format, mypy --strict).
- [x] The default build (`GC_WANT_GARMINCONNECT=OFF`) still builds clean —
      Garmin code is gated and must not regress the baseline.
      **EXECUTED 2026-08-30 at `37710370e`** on a clean worktree: configure EXIT=0, build EXIT=0,
      `src/GoldenCheetah` linked (581 targets). This is an executed MEASUREMENT, not a regression
      guard — nothing re-runs it automatically. The guard is owed (STATE.md Stage 8).

## Must-have requirements (`must`)

Additional bar:

- [ ] Acceptance criterion's *negative* path is also tested
      (e.g. REQ-002 SSO: test both success and bad-credentials rejection).
- [ ] If the REQ touches DES-001 (worker thread): test asserts the call ran
      off the UI thread (`QSignalSpy` + thread-id check).
- [ ] If the REQ touches DES-002 (storage): test verifies tmp-and-rename
      atomicity and the 0600 ACL on token files.
- [ ] A3 adversarial cycle dispositioned the REQ's mutation / property /
      boundary findings.

## Should-have requirements (`should`)

- [ ] Same as universal floor.
- [ ] Negative-path test if the failure mode is observable to the user
      (e.g. REQ-014 friendly-error translation: assert the user-facing string
      is non-empty and `tr()`-wrapped).

## Nice-to-have requirements (`nice`)

- [ ] One happy-path test is sufficient.
- [ ] If skipped from Phase 1 due to scope, the REQ row in `traceability.md`
      is annotated with `deferred → Phase 2/3` and a one-line rationale.

## Non-functional requirements (`REQ-NF-*`)

NF requirements rarely have a single acceptance check; each gets its own bar:

| NF area | "Done" means |
|---------|--------------|
| `REQ-NF-Perf-*` | Bench test (or QElapsedTimer assertion) demonstrates the budget on the developer machine; CI artifact records the number. |
| `REQ-NF-Threads-001` / `REQ-NF-Cancel-001` | Thread-id assertion at every Python call site; cancel test interrupts a mid-flight worker request and asserts no leak/deadlock within 1 s. |
| `REQ-NF-Sec-001..004` | File-permission test asserts 0600 on token files; secret-scanner clean (once enabled in Phase 2.2); password never written to any file (grep test). |
| `REQ-NF-Reliab-001..002` | Retry test uses fake clock to assert exponential schedule; resumable test crashes mid-backfill and asserts state on restart. |
| `REQ-NF-Obs-001` | A structured-log unit test asserts the JSON shape of one event from each category. |
| `REQ-NF-i18n-001` | Lupdate-equivalent extraction test (or grep for bare `QString("…")` in user-facing strings) returns zero. |
| `REQ-NF-Build-001` | CI builds with `GC_WANT_GARMINCONNECT=OFF` and `=ON`; both must pass. **Both values were BUILT BY HAND 2026-08-30 and both linked** — but there is no CI job and no test asserting it, so the bar as written ("CI builds") is NOT met. Not waived; owed at Stage 8. |
| `REQ-NF-Pkg-001` | Installer-manifest test asserts the Python wheels list contains `garminconnect` and `curl_cffi`. |
| `REQ-NF-Compat-001` | Documented in `docs/garminconnect-known-limits.md`; no automated test. **SATISFIED as of 2026-09-11, commit `0f654f4a5`** — the file now exists and covers all 3 prd.md:116 points; README.md and the connect dialog also updated per prd.md's "Documented in README + connect dialog" clause. Status → `traceability.md`. |

---

## Feature-level "done" (gate to declare a REQ done in `traceability.md`)

When marking a REQ row's TEST/COMMIT columns populated, attach a one-line note
naming which sub-checks above were satisfied and which were waived (with reason).
This is the artifact the Phase 2 exit CLV (Check 4) reads.
