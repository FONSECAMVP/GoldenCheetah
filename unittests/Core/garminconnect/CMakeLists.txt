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

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Test)

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

# --- TEST-003 — REQ-002 wizard-side + REQ-005 wizard-side enforcement -------
# C++ wizard-wiring slice for REQ-002. Pure unit test of GarminCredentialsPage
# under DEC-012 Option A (inject IGarminAuthClient); does NOT drag in the
# embedded-Python worker. AUTOMOC processes the page's Q_OBJECT and the test
# file's FakeAuthClient. Pulls in Qt Widgets for QWizardPage/QLineEdit/QLabel.
#
# RED phase: src/Cloud/GarminCredentialsPage.{h,cpp} and src/Cloud/IGarminAuthClient.h
# do not exist yet — the executable's *compile* fails at the test's #include
# lines (right-reason RED). GREEN adds those files AND extends this
# add_executable() with the page .cpp + header so AUTOMOC processes the
# real Q_OBJECT and the link resolves.
add_executable(testGarminConnectCredentialsPage
    testGarminConnectCredentialsPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminCredentialsPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminCredentialsPage.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
)

set_target_properties(testGarminConnectCredentialsPage PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectCredentialsPage PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectCredentialsPage PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectCredentialsPage PRIVATE cxx_std_17)

add_test(NAME testGarminConnectCredentialsPage COMMAND testGarminConnectCredentialsPage)
set_tests_properties(testGarminConnectCredentialsPage PROPERTIES LABELS "garmin-fast")

# --- TEST-004 — REQ-002 end-to-end (worker + WorkerAuthClient) --------------
# REQ-002 closure slice: GarminWorker on a dedicated QThread + WorkerAuthClient
# adapter (IGarminAuthClient impl) bridging the page to the worker, with a
# FakePyAdapter (DEC-013 Option A — IGarminPyAdapter interface) standing in
# for the embedded-Python PyEmbeddedAdapter. The Python boundary itself is
# still tested by Python-side test_adapter_login.py — this executable stays
# Python-free under the `garmin-fast` label.
#
# RED phase: src/Cloud/IGarminPyAdapter.h, src/Cloud/GarminWorker.{h,cpp},
# and src/Cloud/WorkerAuthClient.{h,cpp} do not exist yet — the executable's
# compile fails at the #include lines (right-reason RED). GREEN adds those
# files AND extends this add_executable() with the GarminWorker +
# WorkerAuthClient .cpp + headers so AUTOMOC processes the real Q_OBJECTs
# and the link resolves.
add_executable(testGarminConnectAuthClient
    testGarminConnectAuthClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

set_target_properties(testGarminConnectAuthClient PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectAuthClient PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectAuthClient PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectAuthClient PRIVATE cxx_std_17)

add_test(NAME testGarminConnectAuthClient COMMAND testGarminConnectAuthClient)
set_tests_properties(testGarminConnectAuthClient PROPERTIES LABELS "garmin-fast")
