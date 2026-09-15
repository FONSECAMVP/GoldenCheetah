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
# what a regression actually touched. Not part of the `garmin-fast` gate label:
# run via `ctest -L garmin-build-guard`.

find_program(BASH_EXECUTABLE bash)
if(NOT BASH_EXECUTABLE)
    message(WARNING "bash not found — T-202 REQ-NF-Build-001 build-guard tests not registered")
    return()
endif()

foreach(_gc_garmin_mode OFF ON)
    add_test(
        NAME testGarminBuildGuardFlag${_gc_garmin_mode}
        COMMAND "${BASH_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/garmin_flag_build_guard.sh"
                "${_gc_garmin_mode}"
                "${CMAKE_BINARY_DIR}/guard/gc_want_garminconnect-${_gc_garmin_mode}"
                "${CMAKE_SOURCE_DIR}"
    )
    set_tests_properties(testGarminBuildGuardFlag${_gc_garmin_mode} PROPERTIES
        LABELS "garmin-build-guard"
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
