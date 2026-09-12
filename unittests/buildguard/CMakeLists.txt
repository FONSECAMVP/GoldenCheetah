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
