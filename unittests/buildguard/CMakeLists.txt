# T-202 / REQ-NF-Build-001 (Stage 8) — regression guard for the
# GC_WANT_GARMINCONNECT CMake flag. Registered UNCONDITIONALLY (both flag
# values) from unittests/CMakeLists.txt: the OFF leg must exist precisely when
# the feature is disabled, which is the configuration nobody builds routinely
# and the one that regressed before (B-R010-07).
#
# Each test drives a dedicated scratch build dir (one per flag value) under the
# build tree (gitignored): first run configures + builds the GoldenCheetah app
# target from scratch (slow — hence the 5400s TIMEOUT, the repo's missing-TIMEOUT
# lesson), subsequent runs are incremental ninja rebuilds that only recompile
# what a regression actually touched.
#
# These two are the ONLY tests excused from the routine gate (DEC-054). The
# gate is `ctest -LE gate-exclude`: default-include, so a newly registered test
# is gated without anyone remembering to enrol it. Opting out is therefore a
# deliberate, visible act and, per DEC-054, an opt-out without a recorded
# reason is itself a defect — so the reason and the alternate tier are carried
# as test PROPERTIES below, not merely as prose, which is what makes them
# checkable by garmin_gate_coverage_guard.py.

# Only the two flag-build guards below need bash; every other test in this file
# is pure Python. This used to be a bare `return()`, which aborts the WHOLE
# FILE — so on a host without bash, CMake emitted one WARNING and silently
# registered NONE of the static guards (sec-source, i18n-source, i18n-predicate
# and, once it was added, the gate-coverage guard itself). Configure still
# succeeded and `ctest -LE gate-exclude` then ran green having never loaded a
# single guard. That is precisely the silent-non-execution defect DEC-054
# exists to abolish, one level up, and it would have disarmed the coverage
# guard before the coverage guard could notice — GATE-SELF cannot fire from a
# test that was never registered. Scope the requirement to the two tests that
# actually have it; never let it gate the file.
find_program(BASH_EXECUTABLE bash)
if(NOT BASH_EXECUTABLE)
    message(WARNING
        "bash not found — the two T-202 REQ-NF-Build-001 flag-build guards "
        "(testGarminBuildGuardFlagOFF/ON) are NOT registered. Every other "
        "guard in unittests/buildguard/ IS registered and will run; only the "
        "shell-driven pair is missing. Install bash to restore them.")
endif()

foreach(_gc_garmin_mode OFF ON)
    if(NOT BASH_EXECUTABLE)
        continue()
    endif()
    add_test(
        NAME testGarminBuildGuardFlag${_gc_garmin_mode}
        COMMAND "${BASH_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/garmin_flag_build_guard.sh"
                "${_gc_garmin_mode}"
                "${CMAKE_BINARY_DIR}/guard/gc_want_garminconnect-${_gc_garmin_mode}"
                "${CMAKE_SOURCE_DIR}"
    )
    # REASON for the gate-exclude opt-out: each of these does a full
    # from-scratch CMake configure + app build in a scratch directory.
    # Measured, not estimated — build/Testing/Temporary/CTestCostData.txt
    # records 247.048s (OFF) and 256.081s (ON), ~503s added to every routine
    # run in the GOOD case, and the 5400s TIMEOUT above is the ceiling on a
    # hang. A gate that expensive gets bypassed, which would reintroduce the
    # opt-in defect operationally (DEC-054, option 3's ranking).
    # TIER under which they are run WHEN they are run: `ctest -L
    # garmin-build-guard`. Stated precisely because the honest version matters:
    # NO MECHANISM IN THIS REPOSITORY CURRENTLY INVOKES THAT COMMAND. There is
    # no CI workflow (DEC-054) and no scheduler, so today these two are excused
    # to a tier that runs only when a human types it. They were equally unrun
    # under the previous `ctest -L garmin-fast` gate, so this unit does not
    # create the gap — but it must not paper over it either. The missing
    # scheduling mechanism is tracked as its own ledger row.
    # What IS enforced: garmin_gate_coverage_guard.py checks that this tier
    # string is a ctest command that really SELECTS these tests, so "excused to
    # nowhere" is a finding rather than decoration. It cannot check that
    # anything runs it.
    # Excluded from the routine gate, NOT disabled: the label keeps them
    # discoverable and runnable, which DISABLED would not — and a DISABLED test
    # would stay in the gate's selection while never running, which the
    # coverage guard now treats as a finding in its own right.
    set_tests_properties(testGarminBuildGuardFlag${_gc_garmin_mode} PROPERTIES
        LABELS "garmin-build-guard;gate-exclude"
        GATE_EXCLUDE_REASON
            "Full from-scratch CMake configure + app build per test; measured \
247.048s/256.081s (CTestCostData.txt), 5400s timeout ceiling on a hang. Too \
slow for a per-unit gate; a gate that expensive gets bypassed (DEC-054)."
        GATE_EXCLUDE_TIER "ctest -L garmin-build-guard"
        TIMEOUT 5400
        ENVIRONMENT "CMAKE_COMMAND=${CMAKE_COMMAND}")
endforeach()

# --- T-203 / REQ-NF-Sec-001 + REQ-NF-Sec-003: static security source guard ---
# Makes prd.md's two manual grep audits executable: no password identifier may
# touch a persistence API in the Garmin tree (Sec-001, dod.md's "grep test"),
# and no disable-verification idiom (verify=False, CERT_NONE, VerifyNone,
# ignoreSslErrors, ...) may appear (Sec-003). Comment-stripped matching; pure
# stdlib; runs in well under a second — no build involved.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME testGarminSecSourceGuard
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/garmin_sec_source_guard.py"
            "${CMAKE_SOURCE_DIR}"
)
set_tests_properties(testGarminSecSourceGuard PROPERTIES
    LABELS "garmin-sec-guard"
    TIMEOUT 300)

# --- T-208 / REQ-NF-i18n-001: static i18n source guard -----------------------
# Both halves of prd.md's "all user-facing strings use tr() and appear in .ts
# translation files": (1) every user-facing literal in the Garmin scope must
# sit inside a tr()/translate() call, and (2) every tr() literal extracted
# from that scope must appear as a <source> in every tracked gc_*.ts. Shares
# its scope globs and comment-stripping state machine with the T-203 guard
# above (imported, so the two can never drift). Same pure-stdlib, sub-second
# shape; run via `ctest -L garmin-i18n-guard`.
add_test(
    NAME testGarminI18nSourceGuard
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/garmin_i18n_source_guard.py"
            "${CMAKE_SOURCE_DIR}"
)
set_tests_properties(testGarminI18nSourceGuard PROPERTIES
    LABELS "garmin-i18n-guard"
    TIMEOUT 300)

# --- B-STAGE9-15 / DEC-053: unit tests for the guard's exemption predicate ---
# testGarminI18nSourceGuard above only proves "0 findings against today's
# tree" — an outcome a VACUOUS predicate satisfies more easily than a correct
# one. DEC-053 widens the technical-literal exemption to cover developer-trace
# lines as a category, and a widened exemption is the standard way a guard
# quietly stops guarding. This entry runs the predicate's own pytest file,
# whose load-bearing half asserts REJECTION: prose containing '=', prose with
# a leading snake_case or capitalised word, and prose that drifts out of a
# key=value prefix must all still be flagged. Same label as the guard it
# defends, so the two always run together.
add_test(
    NAME testGarminI18nGuardPredicate
    COMMAND "${Python3_EXECUTABLE}" -m pytest
            "${CMAKE_CURRENT_SOURCE_DIR}/test_garmin_i18n_source_guard.py"
            -v
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(testGarminI18nGuardPredicate PROPERTIES
    LABELS "garmin-i18n-guard"
    TIMEOUT 300)

# --- B-STAGE9-16 / DEC-054: the routine gate covers every registered test ----
# DEC-054 flipped the routine gate from opt-in (`ctest -L garmin-fast`, which
# could only ever see tests someone remembered to label) to default-include
# (`ctest -LE gate-exclude`). These two are the DETECTION half of that change:
# the protection is the default itself, and a guard is only the complement.
#
# testGarminGateCoverageGuard reads the real configured registry via
# `ctest --show-only=json-v1` — the artifact that actually describes what
# ctest will do, not the CMakeLists text that merely looks like it — and
# asserts that every registered test is selected by the gate unless it carries
# `gate-exclude`, and that every exclusion records a reason and an alternate
# tier that SELECTS it.
#
# "Selects", not "runs", and the distinction is the whole point rather than
# pedantry: nothing in this repository invokes any alternate tier (see the
# comment on the two excused guards above). What the guard establishes is that
# the recorded command would reach the test if someone ran it. It cannot
# establish that anyone does.
#
# It also asserts that no unexcused test carries one of the result-masking
# properties DISABLED, SKIP_RETURN_CODE or SKIP_REGULAR_EXPRESSION. All three
# leave a test in the gate's SELECTION while removing its result from the
# verdict, and the two skip properties are worse than DISABLED: ctest COUNTS
# SKIPPED AS PASSED, so a genuinely failing test inflates the pass rate
# instead of failing the gate (measured, ctest 3.31.6). That is a bypass of
# the opt-out protocol which touches no label, records no reason and names no
# tier.
#
# NEITHER of these may ever carry `gate-exclude`: a coverage guard outside the
# gate it polices is the original bug wearing a new hat. The guard asserts
# that about its own registration (rule GATE-SELF), so an attempt to excuse it
# makes it fail rather than go quiet.
#
# Nor may either carry the masking properties above, or WILL_FAIL. The
# self-neutering case is the sharpest in this file and is measured, not
# theorised: put `SKIP_RETURN_CODE 1` on testGarminGateCoverageGuard, and the
# moment it DETECTS an exclusion defect and exits 1, ctest reports
# `***Skipped` / `100% tests passed` and exits 0. The gate stays green while
# the instrument screams into a void. WILL_FAIL is the inverse — a clean run
# fails, a detected defect passes — so it is prohibited here specifically,
# though not for ordinary tests, where it is a legitimate expected-failure
# semantic that turns red immediately rather than hiding.
add_test(
    NAME testGarminGateCoverageGuard
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/garmin_gate_coverage_guard.py"
            "${CMAKE_BINARY_DIR}"
)
set_tests_properties(testGarminGateCoverageGuard PROPERTIES
    LABELS "garmin-gate-guard"
    TIMEOUT 300)

add_test(
    NAME testGarminGateCoverageGuardUnits
    COMMAND "${Python3_EXECUTABLE}" -m pytest
            "${CMAKE_CURRENT_SOURCE_DIR}/test_garmin_gate_coverage_guard.py"
            -v
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(testGarminGateCoverageGuardUnits PROPERTIES
    LABELS "garmin-gate-guard"
    TIMEOUT 300)

# --- B-STAGE9-16 Part B / DEC-054: every managed file is linted by something --
# The LINT half of the same finding, and the same defect one layer over.
# `.pre-commit-config.yaml` scopes each hook with an enumerated `files:` regex
# that was accurate on 2026-05-17 and then never grew, so when this very
# directory appeared it was linted by NOTHING — commits d9ba4faad and 8611fb2d0
# each staged new buildguard Python and every hook printed "(no files to
# check)Skipped", ~1,950 unlinted lines in the latter. A skipped hook and a
# passing hook look identical in a terminal, which is what made four months of
# it invisible.
#
# Part A could invert the ctest default to default-include. This half CANNOT
# take that route: DEC-010 scoped pre-commit to new Garmin paths precisely so
# as not to retrofit the legacy GoldenCheetah tree, and that principle stands —
# only its ENUMERATION was stale. So the enumeration stays and this guard makes
# forgetting to grow it LOUD, asserting that every tracked file under the
# declared managed roots is matched by at least one hook's real regex.
#
# It reads the two artifacts that actually run — `.pre-commit-config.yaml`
# parsed with PyYAML and evaluated with `re`, and `git ls-files` for the
# inventory (THE INDEX, which is exactly pre-commit's input domain; a directory
# walk would sweep in the __pycache__/.mypy_cache/.pytest_cache dirs sitting in
# this directory right now). It does NOT keep a second hand-written list of
# what is covered; that would reintroduce the two-lists-that-drift defect being
# fixed.
#
# Gaps are DECLARED, not silent, and re-checked every run: mypy deliberately
# does not cover this directory (43 measured errors under --strict, DEC-054's
# own sanctioned branch, no type coverage claimed) and no shell linter exists
# for garmin_flag_build_guard.sh. A gap that quietly becomes false — someone
# widens mypy and leaves the declaration behind — fails as STALE, so the
# declaration cannot outlive its own truth.
#
# SOURCE dir AND BINARY dir are both passed, and the distinction burned v7:
# the config and the index live in the SOURCE tree, but LINT-SELF's registry
# half needs the BUILD tree, because that is where ctest's registry is. Without
# --build-dir the guard still runs and says plainly that the registry half did
# not run, rather than implying it passed.
#
# Like the gate-coverage guard above, NEITHER of these may ever carry
# `gate-exclude`: a coverage guard outside the gate it polices is the original
# bug wearing a new hat. The guard asserts that about its own registration.
add_test(
    NAME testGarminLintOwnershipGuard
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/garmin_lint_ownership_guard.py"
            "${CMAKE_SOURCE_DIR}"
            --build-dir "${CMAKE_BINARY_DIR}"
)
set_tests_properties(testGarminLintOwnershipGuard PROPERTIES
    LABELS "garmin-lint-guard"
    TIMEOUT 300)

add_test(
    NAME testGarminLintOwnershipGuardUnits
    COMMAND "${Python3_EXECUTABLE}" -m pytest
            "${CMAKE_CURRENT_SOURCE_DIR}/test_garmin_lint_ownership_guard.py"
            -v
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
set_tests_properties(testGarminLintOwnershipGuardUnits PROPERTIES
    LABELS "garmin-lint-guard"
    TIMEOUT 300)
