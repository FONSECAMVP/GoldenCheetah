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

# --- T-006 — VAL-007 second half: GarminAuthChain RAII assembly --------------
# The chain AddCloudWizard uses to stand up the production auth stack: a
# dedicated QThread + GarminWorker (moved onto it) + WorkerAuthClient, around
# an injected IGarminPyAdapter* it does NOT own (DES-001a lifecycle: wizard >
# chain(worker) > adapter). Teardown per DES-001 invariant 3: quit()+wait(),
# bounded. FakePyAdapter substitutes for PyEmbeddedAdapter (DEC-013), so this
# executable stays Python-free under the `garmin-fast` label.
#
# RED phase: src/Cloud/GarminAuthChain.{h,cpp} do not exist yet — the
# executable's compile fails at the test's #include "GarminAuthChain.h" line
# (right-reason RED). GREEN adds those files AND extends this
# add_executable() with the chain .cpp + header so AUTOMOC processes the real
# Q_OBJECTs and the link resolves.
add_executable(testGarminConnectAuthChain
    testGarminConnectAuthChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAuthChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAuthChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

set_target_properties(testGarminConnectAuthChain PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectAuthChain PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectAuthChain PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectAuthChain PRIVATE cxx_std_17)

add_test(NAME testGarminConnectAuthChain COMMAND testGarminConnectAuthChain)
set_tests_properties(testGarminConnectAuthChain PROPERTIES LABELS "garmin-fast")

# --- T-007 — REQ-002 / A3-R002-TR-01: AddCloudWizard routing + lifecycle -----
# Closes finding A3-R002-TR-01. Brings the REAL src/Cloud/AddCloudWizard.cpp
# under test: AddService/AddConsent nextId() Garmin routing (→ page 21),
# ensureGarminAuthPage() idempotency (the `if (garminChain) return;` guard) and
# ~AddCloudWizard() DES-001a teardown order (chain before adapter).
#
# The heavyweight GC GUI headers (MainWindow/Athlete/Context/Settings/Colors/
# CloudService/OAuthDialog/RideMetadata/GoldenCheetah/Serial, plus RideItem/
# Perspective which moc drags into moc_AddCloudWizard.cpp) are short-circuited
# by the force-included stubs/WizardStubPreamble.h — same pattern as
# testGarminConnectTile's GCStubPreamble (a force-include is required because the
# co-located src/Cloud headers cannot be shadowed on the include path). The REAL
# Python-free Garmin auth stack (GarminAuthChain/GarminWorker/WorkerAuthClient/
# GarminCredentialsPage) is linked in; PyEmbeddedAdapter is replaced by a
# Python-free fake in the preamble, so this target stays on `garmin-fast`.
#
# GC_WANT_GARMINCONNECT must be defined for the target so the wizard's Garmin
# routing/lifecycle branches compile; GARMIN_PY_MODULE_DIR mirrors the src build
# (used verbatim by ensureGarminAuthPage()). <QRegExp> in AddCloudWizard.cpp
# resolves via Qt6::Core5Compat, matching the src target. AUTOMOC (global) mocs
# AddCloudWizard.h's Q_OBJECT pages and this file's test class.
add_executable(testGarminConnectWizardRouting
    testGarminConnectWizardRouting.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AddCloudWizard.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AddCloudWizard.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAuthChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAuthChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/WorkerAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminCredentialsPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminCredentialsPage.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

target_include_directories(testGarminConnectWizardRouting PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_compile_definitions(testGarminConnectWizardRouting PRIVATE
    GC_WANT_GARMINCONNECT
    "GARMIN_PY_MODULE_DIR=\"${CMAKE_SOURCE_DIR}/src/Python/garminconnect\""
)

target_compile_options(testGarminConnectWizardRouting PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/WizardStubPreamble.h"
)

target_link_libraries(testGarminConnectWizardRouting PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core5Compat
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectWizardRouting PRIVATE cxx_std_17)

add_test(NAME testGarminConnectWizardRouting COMMAND testGarminConnectWizardRouting)
set_tests_properties(testGarminConnectWizardRouting PROPERTIES
    LABELS "garmin-fast"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# --- TEST-005 — DES-013 PyEmbeddedAdapter (embedded-CPython bridge) ----------
# Production side of the DEC-013 seam: PyEmbeddedAdapter implements
# IGarminPyAdapter against a REAL linked CPython (Python3::Python,
# Development.Embed). This target therefore carries the `garmin-py` CTest
# label and must NEVER carry `garmin-fast` — that label is Python-free by
# invariant (see testGarminConnectAuthClient above, which fakes this seam).
#
# The test drives a scriptable stub module (pystubs/garmin_client.py) whose
# directory is handed in via the GARMIN_PYSTUBS_DIR compile definition so the
# test can build the adapter's modulePath without env-var plumbing.
#
# RED phase: src/Cloud/PyEmbeddedAdapter.{h,cpp} do not exist yet — the
# executable's compile fails at the test's #include "PyEmbeddedAdapter.h"
# line (right-reason RED). GREEN adds those files AND extends this
# add_executable() with the adapter .cpp + header so the link resolves.
find_package(Python3 REQUIRED COMPONENTS Development.Embed)

add_executable(testGarminConnectPyAdapter
    testGarminConnectPyAdapter.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/PyEmbeddedAdapter.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/PyEmbeddedAdapter.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

target_include_directories(testGarminConnectPyAdapter PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_compile_definitions(testGarminConnectPyAdapter PRIVATE
    "GARMIN_PYSTUBS_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/pystubs\""
)

target_link_libraries(testGarminConnectPyAdapter PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Python3::Python
)
target_compile_features(testGarminConnectPyAdapter PRIVATE cxx_std_17)

add_test(NAME testGarminConnectPyAdapter COMMAND testGarminConnectPyAdapter)
set_tests_properties(testGarminConnectPyAdapter PROPERTIES LABELS "garmin-py")

# --- TEST-010 — REQ-007 (slice 3/3): GarminWorker DownloadActivity op ---------
# The worker-side download op, symmetric to TEST-004's authenticate coverage:
# maps PyDownloadOutcome kinds to downloaded()/downloadFailed() signals and runs
# the adapter off the GUI thread (REQ-NF-Threads-001). FakeDownloadPyAdapter
# stands in for PyEmbeddedAdapter (DEC-013 Option A), so this stays Python-free
# under the `garmin-fast` label (never garmin-py). AUTOMOC processes the new
# GarminWorker signals and the test class.
#
# RED phase: GarminWorker has no downloadActivity slot / downloaded /
# downloadFailed signals and GarminDownloadFailure does not exist yet — the
# compile fails at those references (right-reason RED). GREEN adds them to
# src/Cloud/GarminWorker.{h,cpp}.
add_executable(testGarminConnectDownloadWorker
    testGarminConnectDownloadWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

set_target_properties(testGarminConnectDownloadWorker PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectDownloadWorker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectDownloadWorker PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectDownloadWorker PRIVATE cxx_std_17)

add_test(NAME testGarminConnectDownloadWorker COMMAND testGarminConnectDownloadWorker)
set_tests_properties(testGarminConnectDownloadWorker PROPERTIES LABELS "garmin-fast")

# --- T-011 — REQ-004 / DES-006: AtomicFile helper (tmp+rename, owner-only) -----
# Pure-Qt helper (no Python.h) — Python-free `garmin-fast` label. Links only the
# generic AtomicFile TU; nothing GC-specific, so no stub preamble is needed.
#
# RED phase: src/Cloud/AtomicFile.{h,cpp} do not exist yet — the executable's
# compile fails at the test's #include "AtomicFile.h" (right-reason RED). GREEN
# adds those files AND extends this add_executable() so the link resolves.
add_executable(testAtomicFile
    testAtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
)

target_include_directories(testAtomicFile PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testAtomicFile PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testAtomicFile PRIVATE cxx_std_17)

add_test(NAME testAtomicFile COMMAND testAtomicFile)
set_tests_properties(testAtomicFile PROPERTIES LABELS "garmin-fast")

# --- T-012 — REQ-004 write path (DES-002): GarminTokenStore -------------------
# Per-athlete token storage layer: <athlete>/garminconnect/ (0700 on create) +
# atomic 0600 tokens.json via AtomicFile. Pure-Qt (no Python.h) — `garmin-fast`
# label. Links AtomicFile + GarminTokenStore TUs.
#
# RED phase: src/Cloud/GarminTokenStore.{h,cpp} do not exist yet — the compile
# fails at the test's #include "GarminTokenStore.h" (right-reason RED). GREEN
# adds those files AND extends this add_executable() so the link resolves.
add_executable(testGarminTokenStore
    testGarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
)

target_include_directories(testGarminTokenStore PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminTokenStore PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminTokenStore PRIVATE cxx_std_17)

add_test(NAME testGarminTokenStore COMMAND testGarminTokenStore)
set_tests_properties(testGarminTokenStore PROPERTIES LABELS "garmin-fast")
