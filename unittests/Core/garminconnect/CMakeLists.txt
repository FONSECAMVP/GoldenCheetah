# Phase 2.2 — REQ-001 TEST-001 build.
# Pulls the real src/Cloud/GarminConnect.cpp into the test executable and
# force-includes stubs/GCStubPreamble.h so the heavyweight GC headers
# (Context.h, Athlete.h, Settings.h, CloudService.h) are short-circuited to
# minimal stand-ins. Pattern mirrors unittests/Core/coach/.
#
# Phase 2.1's smoke test (testGarminConnectSmoke) is retained as a separate
# executable under the same `garmin-fast` CTest label so the bootstrap signal
# stays green; the smoke file itself documented that it would be superseded
# by REQ-001..015 tests in 2.2.

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Test)

# --- Phase 2.1 smoke (toolchain alive) --------------------------------------
add_executable(testGarminConnectSmoke
    testGarminConnectSmoke.cpp
)
target_link_libraries(testGarminConnectSmoke PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectSmoke PRIVATE cxx_std_17)
add_test(NAME testGarminConnectSmoke COMMAND testGarminConnectSmoke)
set_tests_properties(testGarminConnectSmoke PROPERTIES LABELS "garmin-fast")

# --- TEST-001 — REQ-001 tile contract ---------------------------------------
add_executable(testGarminConnectTile
    testGarminConnectTile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
)

target_include_directories(testGarminConnectTile PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

# Force-include the stub preamble before every .cpp so that when
# GarminConnect.cpp pulls CloudService.h (and transitively Context.h /
# Athlete.h / Settings.h), the real headers' guards are already defined
# and only the lightweight stubs are visible.
target_compile_options(testGarminConnectTile PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/GCStubPreamble.h"
)

target_link_libraries(testGarminConnectTile PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectTile PRIVATE cxx_std_17)

add_test(NAME testGarminConnectTile COMMAND testGarminConnectTile)
set_tests_properties(testGarminConnectTile PROPERTIES LABELS "garmin-fast")
