#!/bin/bash
# T-202 / REQ-NF-Build-001 regression guard (Stage 8).
#
# prd.md: "Feature gated behind CMake flag GC_WANT_GARMINCONNECT, default OFF
# until A4 passes. Build with flag OFF must not require garminconnect or
# curl_cffi."  dod.md: "CI builds with GC_WANT_GARMINCONNECT=OFF and =ON; both
# must pass."
#
# Both values had been BUILD VERIFIED by hand (2026-08-30) with no automated
# assertion. This guard makes the assertion executable. For one flag value it:
#   1. configures a dedicated scratch build dir (fresh on first run, then
#      incremental — the regression IS a change, so ninja rebuilds exactly the
#      affected TUs),
#   2. builds the GoldenCheetah app target,
#   3. asserts static evidence that matches the requirement, not just an exit
#      code:
#        - OFF: NO garmin TU (src/Cloud/Garmin* or PyEmbeddedAdapter.cpp) is
#          configured (compile_commands.json) and the linked binary carries no
#          Python or Garmin symbols. These are the two regression classes seen
#          in this project: sources leaking out of the if(GC_WANT_GARMINCONNECT)
#          block (B-R010-07 broke the default flag-OFF link exactly this way).
#          GC_WANT_PYTHON is pinned OFF, so a Py symbol in the OFF binary can
#          only come from the Garmin embed path.
#        - ON: the garmin TUs ARE configured and the binary DOES reference
#          Python symbols — a positive control proving the ON leg exercised the
#          real feature build, not an accidental OFF build.
#
# Usage: garmin_flag_build_guard.sh <ON|OFF> <scratch-build-dir> <source-dir>
set -u

MODE="${1:?usage: garmin_flag_build_guard.sh <ON|OFF> <build-dir> <source-dir>}"
BUILD_DIR="${2:?usage: garmin_flag_build_guard.sh <ON|OFF> <build-dir> <source-dir>}"
SOURCE_DIR="${3:?usage: garmin_flag_build_guard.sh <ON|OFF> <build-dir> <source-dir>}"

CMAKE_BIN="${CMAKE_COMMAND:-cmake}"

fail() {
    echo "GUARD-FAIL[${MODE}]: $*" >&2
    exit 1
}

pass() {
    echo "GUARD-PASS[${MODE}]: $*"
}

if [ "${MODE}" != "ON" ] && [ "${MODE}" != "OFF" ]; then
    fail "first argument must be ON or OFF, got '${MODE}'"
fi

# ---------------------------------------------------------------- configure
# Pinned options mirror the hand-build evidence (traceability.md, 2026-08-30):
# Release, Ninja, libusb OFF (its ON branch does not configure a dependency
# check — ORCH-036), Python OFF (so any Py symbol in the binary is Garmin's),
# tests OFF (the guard builds the app target, not the suite).
CACHE_VAL=""
if [ -f "${BUILD_DIR}/CMakeCache.txt" ]; then
    CACHE_VAL="$(grep -E '^GC_WANT_GARMINCONNECT(:BOOL)?=' "${BUILD_DIR}/CMakeCache.txt" | tail -1 | cut -d= -f2)"
fi

if [ -f "${BUILD_DIR}/CMakeCache.txt" ] && [ "${CACHE_VAL}" = "${MODE}" ]; then
    echo "GUARD[${MODE}]: reusing scratch dir ${BUILD_DIR} (configured GC_WANT_GARMINCONNECT=${MODE})"
else
    if [ -d "${BUILD_DIR}" ]; then
        echo "GUARD[${MODE}]: scratch dir configured for '${CACHE_VAL:-nothing}', wiping for fresh configure"
        rm -rf "${BUILD_DIR}"
    fi
    mkdir -p "${BUILD_DIR}"
    if ! "${CMAKE_BIN}" -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
            -DBUILD_TESTS=OFF \
            -DGC_WANT_PYTHON=OFF \
            -DGC_HAVE_LIBUSB=OFF \
            -DGC_WANT_GARMINCONNECT="${MODE}" \
            -S "${SOURCE_DIR}" -B "${BUILD_DIR}" \
            > "${BUILD_DIR}/guard-configure.log" 2>&1; then
        tail -40 "${BUILD_DIR}/guard-configure.log" >&2
        fail "cmake configure with GC_WANT_GARMINCONNECT=${MODE} failed (log: ${BUILD_DIR}/guard-configure.log)"
    fi
    pass "cmake configure with GC_WANT_GARMINCONNECT=${MODE} (EXIT=0)"
fi

# -------------------------------------------------------------------- build
if ! "${CMAKE_BIN}" --build "${BUILD_DIR}" --target GoldenCheetah \
        > "${BUILD_DIR}/guard-build.log" 2>&1; then
    tail -40 "${BUILD_DIR}/guard-build.log" >&2
    fail "build of GoldenCheetah with GC_WANT_GARMINCONNECT=${MODE} failed (log: ${BUILD_DIR}/guard-build.log)"
fi
pass "build of target GoldenCheetah with GC_WANT_GARMINCONNECT=${MODE} (EXIT=0)"

APP_BIN="${BUILD_DIR}/src/GoldenCheetah"
if [ ! -x "${APP_BIN}" ]; then
    fail "expected app binary ${APP_BIN} not found/not executable after build"
fi
pass "app binary linked: ${APP_BIN}"

# ------------------------------------------------- configured-TU assertions
COMPILE_DB="${BUILD_DIR}/compile_commands.json"
if [ ! -f "${COMPILE_DB}" ]; then
    fail "compile_commands.json not found at ${COMPILE_DB} — cannot assert which TUs are configured"
fi

GARMIN_TU_COUNT="$(grep -c 'src/Cloud/Garmin\|src/Cloud/PyEmbeddedAdapter' "${COMPILE_DB}" || true)"

if [ "${MODE}" = "OFF" ]; then
    if [ "${GARMIN_TU_COUNT}" -ne 0 ]; then
        grep -o '"[^"]*src/Cloud/\(Garmin[^"]*\|PyEmbeddedAdapter[^"]*\)"' "${COMPILE_DB}" >&2
        fail "${GARMIN_TU_COUNT} Garmin/PyEmbeddedAdapter TU(s) are CONFIGURED in the GC_WANT_GARMINCONNECT=OFF build — sources leaked out of the if(GC_WANT_GARMINCONNECT) block (B-R010-07 regression class)"
    fi
    pass "no Garmin/PyEmbeddedAdapter TU configured under the OFF flag (compile_commands.json)"
else
    if [ "${GARMIN_TU_COUNT}" -eq 0 ]; then
        fail "GC_WANT_GARMINCONNECT=ON configured ZERO Garmin/PyEmbeddedAdapter TUs — the feature silently stopped being built"
    fi
    pass "Garmin/PyEmbeddedAdapter TUs configured under the ON flag (${GARMIN_TU_COUNT} entries)"
fi

# ------------------------------------------------ symbol-level assertions
# Defense in depth: the TU census above catches a configured leak even before
# the build; the symbol census catches code that reached the binary by any
# other route. Skipped (with a warning, never a failure) only when nm is not
# installed.
NM_BIN="$(command -v nm || true)"
if [ -z "${NM_BIN}" ]; then
    echo "GUARD-WARN[${MODE}]: nm not found — symbol-level assertions skipped (TU census still applies)"
else
    PY_SYMBOLS="$(nm -D --undefined-only "${APP_BIN}" 2>/dev/null | grep -c ' U Py' || true)"
    GARMIN_SYMBOLS="$(nm -C "${APP_BIN}" 2>/dev/null | grep -cE 'GarminConnect::|GarminTokenStore::|GarminAccountEpoch::|PyEmbeddedAdapter::' || true)"

    if [ "${MODE}" = "OFF" ]; then
        if [ "${PY_SYMBOLS}" -ne 0 ]; then
            nm -D --undefined-only "${APP_BIN}" 2>/dev/null | grep ' U Py' >&2
            fail "${PY_SYMBOLS} Python symbol(s) referenced by the OFF binary — GC_WANT_PYTHON is pinned OFF, so this can only be the Garmin embed path requiring Python/garminconnect/curl_cffi"
        fi
        pass "OFF binary references no Python symbols (nm -D)"
        if [ "${GARMIN_SYMBOLS}" -ne 0 ]; then
            nm -C "${APP_BIN}" 2>/dev/null | grep -E 'GarminConnect::|GarminTokenStore::|GarminAccountEpoch::|PyEmbeddedAdapter::' | head -10 >&2
            fail "${GARMIN_SYMBOLS} Garmin symbol(s) present in the OFF binary — garmin code is linked in despite the flag being OFF"
        fi
        pass "OFF binary contains no Garmin symbols (nm -C)"
    else
        if [ "${PY_SYMBOLS}" -eq 0 ]; then
            fail "ON binary references NO Python symbols — the embedded-Python link (find_package(Python3 Development.Embed)) silently dropped out of the ON build"
        fi
        pass "ON binary references Python symbols (${PY_SYMBOLS}) — embed link present"
        if [ "${GARMIN_SYMBOLS}" -eq 0 ]; then
            fail "ON binary contains NO Garmin symbols — the feature code did not link into the app target"
        fi
        pass "ON binary contains Garmin symbols (${GARMIN_SYMBOLS}) — feature linked in"
    fi
fi

pass "REQ-NF-Build-001 guard satisfied for GC_WANT_GARMINCONNECT=${MODE}"
exit 0
