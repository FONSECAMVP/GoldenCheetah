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
# REQ-007 closure grew GarminConnect.cpp to own the download/restore host +
# adapter and implement open()/close()/readFile() (ZIP unwrap via contrib/qzip).
# The tile test still only asserts the REQ-001 tile contract, but it compiles the
# real GarminConnect.cpp, so it now links the same Python-free host stack + qzip
# and uses the ReadFileStubPreamble (superset of the old GCStubPreamble: adds the
# open/close/readFile virtuals + the Python-free PyEmbeddedAdapter fake). AUTOMOC
# is required now that the linked host TUs (GarminWorker/…Client) carry Q_OBJECT.
add_executable(testGarminConnectTile
    testGarminConnectTile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    # LSN-018 — REQ-017 Slice A: GarminConnect.cpp now calls
    # GarminAccountEpoch::current()/bump(), so EVERY target that compiles it must
    # link this TU too or the link fails with an undefined reference.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectTile PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectTile PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

# Force-include the stub preamble before every .cpp so that when GarminConnect.cpp
# pulls CloudService.h (and transitively Context/Athlete/Settings) and
# PyEmbeddedAdapter.h, the real headers' guards are already defined and only the
# lightweight (Python-free) stubs are visible.
target_compile_options(testGarminConnectTile PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectTile PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
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

# --- T-032..T-034 — REQ-003 (MFA) Slice B: GarminMfaPage ---------------------
# C++ wizard-page slice for REQ-003 (MFA), sibling to TEST-003's credentials
# page. Pure unit test of GarminMfaPage under DEC-012 Option A (inject
# IGarminAuthClient); does NOT drag in the embedded-Python worker. AUTOMOC
# processes the page's Q_OBJECT and the test file's FakeAuthClient. Pulls in Qt
# Widgets for QWizardPage/QLineEdit/QLabel.
#
# RED phase: src/Cloud/GarminMfaPage.{h,cpp} do not exist yet — the executable's
# *compile* fails at the test's #include "GarminMfaPage.h" line (right-reason
# RED). GREEN adds those files AND keeps the page .cpp + header in this
# add_executable() so AUTOMOC processes the real Q_OBJECT and the link resolves.
add_executable(testGarminConnectMfaPage
    testGarminConnectMfaPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminMfaPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminMfaPage.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
)

set_target_properties(testGarminConnectMfaPage PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectMfaPage PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectMfaPage PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectMfaPage PRIVATE cxx_std_17)

add_test(NAME testGarminConnectMfaPage COMMAND testGarminConnectMfaPage)
set_tests_properties(testGarminConnectMfaPage PROPERTIES LABELS "garmin-fast")

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
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminMfaPage.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminMfaPage.h
    # REQ-008 (T-051) — the REAL connect-success producer + atomic writer, so the
    # wizard's persist trigger writes real tokens.json/active-account.json under the
    # stubbed CloudService (LSN-018 — a test target linking a production .cpp it
    # didn't before must add it here).
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
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

# --- T-014 — REQ-006 (Slice A): GarminTokenStore load-side perms enforcement ---
# The READ-side counterpart of DES-006's owner-only guarantee (DES-002 invariant):
# on load, a tokens.json whose mode is WIDER than owner-only 0600 is REFUSED —
# loadChecked() returns a typed TokenPermissionsRejected outcome (naming the path)
# instead of the bytes, so the caller forces a fresh SSO (REQ-006). A conforming
# 0600 file loads normally; an absent file is a distinct NotFound outcome.
#
# A SEPARATE target from testGarminTokenStore keeps the REQ-004 (write) and
# REQ-006 (load) suites independently runnable. Pure-Qt (no Python.h) —
# `garmin-fast` label. Links the same GarminTokenStore + AtomicFile TUs.
add_executable(testGarminTokenStore_load
    testGarminTokenStore_load.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
)

target_include_directories(testGarminTokenStore_load PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminTokenStore_load PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminTokenStore_load PRIVATE cxx_std_17)

add_test(NAME testGarminTokenStore_load COMMAND testGarminTokenStore_load)
set_tests_properties(testGarminTokenStore_load PROPERTIES LABELS "garmin-fast")

# --- T-018 — REQ-007 closure (Slice 1/3): GarminWorker session-restore seam -----
# The worker-side restore op, symmetric to TEST-004 (authenticate) and TEST-010
# (download): maps PyLoadTokensOutcome kinds to sessionRestored()/restoreFailed()
# and runs the adapter off the GUI thread (REQ-NF-Threads-001).
# FakeRestorePyAdapter stands in for PyEmbeddedAdapter (DEC-013 Option A) → this
# stays Python-free under the `garmin-fast` label (never garmin-py). AUTOMOC
# processes the new GarminWorker signals and the test class.
#
# RED phase: GarminWorker has no restoreSession slot / sessionRestored /
# restoreFailed signals and GarminRestoreFailure + PyLoadTokensOutcome do not
# exist yet — the compile fails at those references (right-reason RED). GREEN
# adds them to src/Cloud/IGarminPyAdapter.h + src/Cloud/GarminWorker.{h,cpp}.
add_executable(testGarminConnectRestoreWorker
    testGarminConnectRestoreWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
)

set_target_properties(testGarminConnectRestoreWorker PROPERTIES
    AUTOMOC ON
)

target_include_directories(testGarminConnectRestoreWorker PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminConnectRestoreWorker PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectRestoreWorker PRIVATE cxx_std_17)

add_test(NAME testGarminConnectRestoreWorker COMMAND testGarminConnectRestoreWorker)
set_tests_properties(testGarminConnectRestoreWorker PROPERTIES LABELS "garmin-fast")

# --- T-019 — REQ-007 closure (Slice 2/3): GarminConnect open()/close() lifecycle -
# Brings the REAL src/Cloud/GarminConnect.cpp under test for the worker-in-
# CloudService lifecycle: open() resolves the config dir, loadChecked()s the
# tokens (REQ-006) and restores the session via the host; a permission-refused
# LoadResult returns false + labelled error and attempts NO download. The
# heavyweight GC headers (CloudService/Context/Athlete) are short-circuited by
# the force-included ReadFileStubPreamble.h, which ALSO replaces the embedded-
# Python PyEmbeddedAdapter with a Python-free fake so this stays `garmin-fast`.
# A fake IGarminDownloadClient is injected; the REAL (pure-Qt) GarminTokenStore +
# AtomicFile + the Python-free GarminDownloadChain/Client/Worker are linked.
#
# RED phase: GarminConnect has no open/close override, no injecting ctor, and
# IGarminDownloadClient/GarminDownloadChain do not exist yet — the compile fails
# at those references (right-reason RED). GREEN adds them.
add_executable(testGarminConnectOpen
    testGarminConnectOpen.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    # LSN-018 — REQ-017 Slice A: GarminConnect.cpp now calls
    # GarminAccountEpoch::current()/bump(), so EVERY target that compiles it must
    # link this TU too or the link fails with an undefined reference.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectOpen PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectOpen PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectOpen PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectOpen PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectOpen PRIVATE cxx_std_17)

add_test(NAME testGarminConnectOpen COMMAND testGarminConnectOpen)
set_tests_properties(testGarminConnectOpen PROPERTIES LABELS "garmin-fast")

# --- T-020..T-023 — REQ-007 closure (Slice 3/3): GarminConnect::readFile --------
# The DEC-016 retry table under test: ORIGINAL(FIT) unzip+sniff → garmin-<id>.fit;
# RateLimited fail-fast (no retry); Network/Unknown/not-FIT → retry once as TCX →
# garmin-<id>.tcx. Same Python-free stubbing as testGarminConnectOpen, PLUS the
# real contrib/qzip ZipReader/ZipWriter (zip.cpp + ZLIB) so the ZIP unwrap is
# exercised for real. A fake IGarminDownloadClient scripts per-fmt responses.
#
# RED phase: GarminConnect has no readFile override / injecting ctor — the
# compile fails at those references (right-reason RED). GREEN adds them.
add_executable(testGarminConnectReadFile
    testGarminConnectReadFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    # LSN-018 — REQ-017 Slice A: GarminConnect.cpp now calls
    # GarminAccountEpoch::current()/bump(), so EVERY target that compiles it must
    # link this TU too or the link fails with an undefined reference.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectReadFile PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectReadFile PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectReadFile PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectReadFile PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectReadFile PRIVATE cxx_std_17)

add_test(NAME testGarminConnectReadFile COMMAND testGarminConnectReadFile)
set_tests_properties(testGarminConnectReadFile PROPERTIES LABELS "garmin-fast")

# --- T-045 / T-046 — REQ-008 Slice B (DEC-017 A): GarminSidecarStore -----------
# Per-account sidecar persistence layer mirroring GarminTokenStore: owns the two
# per-account files (imported-<uid>.json Tier-1 dedup map + backfill-state-<uid>.json
# resume cursor). Pure-Qt (no Python.h) — `garmin-fast` label. Links the new
# GarminSidecarStore TU plus AtomicFile (DES-006, the atomic 0600 writer).
#
# RED phase: src/Cloud/GarminSidecarStore.{h,cpp} do not exist yet — the compile
# fails at the test's #include "GarminSidecarStore.h" (right-reason RED). GREEN
# adds those files AND keeps them in this add_executable() so the link resolves.
# LSN-018: this NEW production .cpp is registered BOTH here and in the app's
# GC_WANT_GARMINCONNECT source list (src/CMakeLists.txt).
add_executable(testGarminSidecarStore
    testGarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
)

target_include_directories(testGarminSidecarStore PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
)

target_link_libraries(testGarminSidecarStore PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminSidecarStore PRIVATE cxx_std_17)

add_test(NAME testGarminSidecarStore COMMAND testGarminSidecarStore)
set_tests_properties(testGarminSidecarStore PROPERTIES LABELS "garmin-fast")

# --- T-047 / T-048 — REQ-008 Slice C: GarminConnect incremental-sync ----------
# The DES-010 flow bound onto CloudService's readdir/readFile sync machinery:
# readdir drives the Slice-A worker list op (off the caller thread via the REAL
# GarminDownloadChain), builds one CloudServiceEntry per activity, short-circuits
# already-imported ids against imported-<uid>.json (Tier-1 dedup), and rejects a
# concurrent sync (REQ-NF-Perf-002); readFile records each successful download
# into the sidecar + advances backfill-state. Same Python-free stubbing as
# testGarminConnectReadFile (ReadFileStubPreamble + Python-free PyEmbeddedAdapter
# fake) PLUS the REAL GarminSidecarStore + AtomicFile (Slice B) and real
# contrib/qzip for the FIT unwrap. A fake IGarminPyAdapter/IGarminDownloadClient
# scripts the listing/download.
#
# RED phase: GarminConnect has no readdir override and IGarminDownloadClient has
# no listActivities op yet — the compile fails at those references (right-reason
# RED). GREEN adds them.
add_executable(testGarminConnectSync
    testGarminConnectSync.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    # LSN-018 — REQ-017 Slice A: GarminConnect.cpp now calls
    # GarminAccountEpoch::current()/bump(), so EVERY target that compiles it must
    # link this TU too or the link fails with an undefined reference.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectSync PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectSync PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectSync PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectSync PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectSync PRIVATE cxx_std_17)

add_test(NAME testGarminConnectSync COMMAND testGarminConnectSync)
set_tests_properties(testGarminConnectSync PROPERTIES LABELS "garmin-fast")

# --- T-049 / T-050 — REQ-008 Slice D: connect->persist producer wiring ---------
# Closes adversary finding A3-R008-01: the connect-success producer writes were
# never wired, so sync could never run in production (only the ctor uid-override
# masked the gap — LSN-024). This slice adds the producer: GarminAuthSuccess now
# carries the tokenBlob forward (GarminWorker), GarminTokenStore persists BOTH
# tokens.json AND the account-agnostic active-account.json (DEC-garmin-018 Option
# B), and GarminConnect::resolveGarminUserId() reads active-account.json.
#
# T-049 (producer unit) drives GarminTokenStore::persistConnectSuccess /
# loadActiveAccountUserId / clearAccount + a FRESH (no-override) GarminConnect's
# readdir. T-050 (end-to-end) drives auth-success through the REAL GarminWorker,
# persists, then a FRESH GarminConnect resolves the uid from disk and readdir
# returns the scripted entries. Same Python-free stubbing as testGarminConnectSync
# (ReadFileStubPreamble + Python-free PyEmbeddedAdapter fake) with the REAL
# GarminTokenStore + GarminSidecarStore + AtomicFile backing the disk I/O.
add_executable(testGarminConnectConnectPersist
    testGarminConnectConnectPersist.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    # LSN-018 — REQ-017 Slice A: GarminConnect.cpp now calls
    # GarminAccountEpoch::current()/bump(), so EVERY target that compiles it must
    # link this TU too or the link fails with an undefined reference.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectConnectPersist PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectConnectPersist PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectConnectPersist PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectConnectPersist PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectConnectPersist PRIVATE cxx_std_17)

add_test(NAME testGarminConnectConnectPersist COMMAND testGarminConnectConnectPersist)
set_tests_properties(testGarminConnectConnectPersist PROPERTIES LABELS "garmin-fast")

# --- TEST-060..063 — REQ-017 Slice A (DEC-garmin-021 B): account EPOCH ---------
# Binds a GarminConnect session to the account it was opened against with an
# in-memory epoch (src/Cloud/GarminAccountEpoch.{h,cpp} — pure Qt, no Python.h,
# same seam discipline as GarminDownloadChain.h) instead of relying solely on
# DEC-garmin-020's per-call disk re-check, which is deliberately KEPT as the
# second layer.
#
# A SEPARATE executable rather than more slots in testGarminConnectConnectPersist:
# the epoch map is process-global static state, so a dedicated process keeps these
# slots independent of every other suite's expectations (and testGarminConnectSync
# / …ConnectPersist stay readable).
#
# Same Python-free stubbing as testGarminConnectConnectPersist (ReadFileStubPreamble
# + Python-free PyEmbeddedAdapter fake) with the REAL GarminTokenStore +
# GarminSidecarStore + AtomicFile backing the disk I/O and real contrib/qzip for
# the FIT unwrap.
#
# LSN-018: GarminAccountEpoch.cpp is a NEW production TU — it is registered BOTH
# here (and in every other target that compiles GarminConnect.cpp) AND in the app's
# GC_WANT_GARMINCONNECT source list (src/CMakeLists.txt).
add_executable(testGarminConnectAccountEpoch
    testGarminConnectAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectAccountEpoch PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectAccountEpoch PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectAccountEpoch PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectAccountEpoch PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectAccountEpoch PRIVATE cxx_std_17)

add_test(NAME testGarminConnectAccountEpoch COMMAND testGarminConnectAccountEpoch)
set_tests_properties(testGarminConnectAccountEpoch PROPERTIES LABELS "garmin-fast")

# --- TEST-065 — REQ-017 / DEC-garmin-022: a refusal REPORTS itself ------------
# Drives GarminConnect::readFile's two fail-closed entry guards (DEC-021 session
# epoch + DEC-020 credential re-check) and asserts each posts a labelled,
# QUEUED completion carrying the caller's own (empty) buffer while still
# returning false — so CloudServiceSyncDialog's loop advances and the
# preallocated QByteArray is freed instead of leaking (finding B-R017-06).
#
# A SEPARATE executable for the same reason as testGarminConnectAccountEpoch: the
# account-epoch map is process-global static state, so a dedicated process keeps
# these slots independent of every other suite's expectations. Same Python-free
# stubbing (ReadFileStubPreamble + Python-free PyEmbeddedAdapter fake) with the
# REAL GarminTokenStore + GarminSidecarStore + AtomicFile and real contrib/qzip.
add_executable(testGarminConnectRefusalCompletion
    testGarminConnectRefusalCompletion.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectRefusalCompletion PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectRefusalCompletion PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectRefusalCompletion PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectRefusalCompletion PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectRefusalCompletion PRIVATE cxx_std_17)

add_test(NAME testGarminConnectRefusalCompletion COMMAND testGarminConnectRefusalCompletion)
set_tests_properties(testGarminConnectRefusalCompletion PROPERTIES LABELS "garmin-fast")

# --- TEST-068 — DEC-garmin-023: readFile carries FAILURE explicitly -----------
# Drives the FIVE remaining silent `return false` sites in GarminConnect::readFile
# (finding B-R017-10) and asserts each posts a QUEUED, distinguishable readFailed
# carrying the caller's own (empty) buffer while still returning false — so the
# consumers' loops advance and the preallocated QByteArray is freed instead of
# leaking, exactly as DEC-022/TEST-065 established for the two ENTRY guards.
#
# A SEPARATE executable for the same reason as testGarminConnectRefusalCompletion:
# the account-epoch map is process-global static state, and three of the five
# sites bump it mid-flight. Same Python-free stubbing (ReadFileStubPreamble +
# Python-free PyEmbeddedAdapter fake) with the REAL GarminTokenStore +
# GarminSidecarStore + AtomicFile + GarminAccountEpoch and real contrib/qzip.
add_executable(testGarminConnectReadFailure
    testGarminConnectReadFailure.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectReadFailure PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectReadFailure PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/contrib/qzip
)

target_compile_options(testGarminConnectReadFailure PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/ReadFileStubPreamble.h"
)

find_package(ZLIB REQUIRED)
target_link_libraries(testGarminConnectReadFailure PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
)
target_compile_features(testGarminConnectReadFailure PRIVATE cxx_std_17)

add_test(NAME testGarminConnectReadFailure COMMAND testGarminConnectReadFailure)
set_tests_properties(testGarminConnectReadFailure PROPERTIES LABELS "garmin-fast")

# --- TEST-064 — REQ-017 Slice B: an owner destroys what it opens --------------
# Drives the REAL src/Cloud/CloudService.h teardown contract (closeAndDeleteStore)
# against a spy store that owns a real QThread — the assertion is close()-then-
# destroy, which is what actually stops a GarminConnect worker thread and releases
# its interpreter session (DES-001 invariant 3).
#
# NO GarminConnect/CloudService .cpp is compiled here: CloudService.cpp pulls in
# RideCache/RideItem/MainWindow/JsonRideFile/CsvRideFile/DataProcessor, i.e. the
# whole application, which is precisely why DEC-garmin-021 rejected Option C. Only
# the HEADER is real; the test TU predefines the Context.h/Athlete.h guards inline
# (Settings.h and Utils.h are pulled in for real, both pure Qt) so no stub preamble
# and no -include flag is needed. Widgets is linked because CloudService.h's dialog
# declarations are QWidget-derived; Network because it declares QNetworkReply
# slots. Nothing from either module is executed.
#
# CloudService.h is deliberately NOT listed as a target source: AUTOMOC would then
# generate moc_CloudService.cpp, whose qt_static_metacall calls every slot of
# CloudServiceSyncDialog/CloudServiceDialog/CloudServiceAutoDownload — all defined
# in the un-linkable CloudService.cpp. The test only INCLUDES the header (no
# CloudService-derived object is ever instantiated), so no metaobject is needed.
add_executable(testGarminConnectOwnerTeardown
    testGarminConnectOwnerTeardown.cpp
)

set_target_properties(testGarminConnectOwnerTeardown PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectOwnerTeardown PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Charts
)

target_link_libraries(testGarminConnectOwnerTeardown PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)
target_compile_features(testGarminConnectOwnerTeardown PRIVATE cxx_std_17)

add_test(NAME testGarminConnectOwnerTeardown COMMAND testGarminConnectOwnerTeardown)
set_tests_properties(testGarminConnectOwnerTeardown PROPERTIES LABELS "garmin-fast")

# --- TEST-067 — REQ-018: a downloaded Garmin activity must actually IMPORT -----
# The ONLY garmin target that does NOT force-include stubs/ReadFileStubPreamble.h.
# That preamble replaces CloudService with a stand-in that has no uncompressRide
# at all, which is precisely why the REQ-018 defect (GarminConnect never setting
# downloadCompression, so uncompressRide's first guard rejected every
# garmin-<id>.fit/.tcx it was handed) survived a 22/22-green suite: no test ever
# crossed that boundary.
#
# So this target uses the REAL GC headers and compiles:
#   - the REAL src/Cloud/CloudService.cpp  -> the REAL uncompressRide + guard
#   - the REAL src/Cloud/GarminConnect.cpp -> the REAL staged name + compression
#   - the REAL FileIO ride readers         -> a REAL parsed RideFile
# stubs/ImportSeamStubs.cpp supplies link-level stand-ins for the application
# layers those TUs merely reference (RideCache/RideItem/RideMetric/MainWindow/
# DataProcessor/Colors/GlobalContext/...); nothing on the path under test is
# stubbed. Linking those for real would pull in 355 of the app's 439 objects.
#
# Still Python-free (the injected IGarminDownloadClient replaces the worker host
# and PyEmbeddedAdapter is a never-constructed link stub) -> `garmin-fast`.
add_executable(testGarminConnectImport
    testGarminConnectImport.cpp
    stubs/ImportSeamStubs.cpp
    # The boundary under test.
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.h
    # The producer under test.
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminConnect.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminAccountEpoch.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadChain.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminWorker.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminTokenStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/GarminSidecarStore.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/AtomicFile.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminDownloadClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminAuthClient.h
    ${CMAKE_SOURCE_DIR}/src/Cloud/IGarminPyAdapter.h
    # The REAL parsers, so "yields a parsed RideFile" means what it says.
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.h
    ${CMAKE_SOURCE_DIR}/src/FileIO/FitRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxParser.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectImport PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectImport PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ANT
    ${CMAKE_SOURCE_DIR}/src/Train
    ${CMAKE_SOURCE_DIR}/src/FileIO
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/src/Charts
    ${CMAKE_SOURCE_DIR}/src/Metrics
    ${CMAKE_SOURCE_DIR}/src/Gui
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Planning
    ${CMAKE_SOURCE_DIR}/src/Coach
    ${CMAKE_SOURCE_DIR}/qwt/src
    ${CMAKE_SOURCE_DIR}/contrib/qxt/src
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/json
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/qwtcurve
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/flowlayout
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/codeeditor
    ${CMAKE_SOURCE_DIR}/contrib/lmfit
    ${CMAKE_SOURCE_DIR}/contrib/boost
    ${CMAKE_SOURCE_DIR}/contrib/kmeans
    ${CMAKE_SOURCE_DIR}/contrib/voronoi
    ${CMAKE_SOURCE_DIR}/contrib/httpserver
    ${CMAKE_SOURCE_DIR}/contrib/qzip
    ${CMAKE_BINARY_DIR}
)

target_compile_definitions(testGarminConnectImport PRIVATE
    GC_WANT_GARMINCONNECT
    "GARMIN_PY_MODULE_DIR=\"${CMAKE_SOURCE_DIR}/src/Python/garminconnect\""
    "GC_TEST_RIDES_DIR=\"${CMAKE_SOURCE_DIR}/test/rides\""
)

# The REAL GC headers reach a long way (MainWindow.h -> QSqlDatabase,
# Context.h -> WebEngine/Charts, ...). These modules are here for their HEADERS;
# nothing from them is executed by this test.
find_package(ZLIB REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Sql Concurrent SerialPort Multimedia MultimediaWidgets PrintSupport Svg
    Bluetooth DBus Charts OpenGL OpenGLWidgets WebEngineCore WebEngineWidgets
    WebEngineQuick WebChannel Positioning Qml Quick)
target_link_libraries(testGarminConnectImport PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineCore
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineQuick
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Core5Compat
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
    # RideFile.cpp's smoothing uses QwtSplineBasis — link the REAL qwt the app
    # links rather than faking a spline.
    qwt
)
target_compile_features(testGarminConnectImport PRIVATE cxx_std_17)

add_test(NAME testGarminConnectImport COMMAND testGarminConnectImport)
set_tests_properties(testGarminConnectImport PROPERTIES
    LABELS "garmin-fast"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# --- TEST-069 — DEC-garmin-023: the CONSUMER half of the failure channel -------
# The defect DEC-023 closes is a STALL plus a LEAK in CloudServiceSyncDialog, so
# the dialog itself has to be the thing under test — "readFailed was emitted" is
# not the contract (LSN-033). Like testGarminConnectImport, and for the same
# reason, this target does NOT force-include stubs/ReadFileStubPreamble.h: that
# preamble replaces CloudService with a stand-in that has no sync dialog at all.
#
# So it compiles the REAL src/Cloud/CloudService.cpp (hence the REAL
# CloudServiceSyncDialog ctor/refreshClicked/downloadClicked/syncNext/
# completedRead/failedRead and the REAL uncompressRide the positive control is
# measured against) plus the REAL FileIO readers CloudService.cpp dispatches to.
#
# Stand-ins: stubs/ImportSeamStubs.cpp (shared verbatim with TEST-067) supplies
# the application layers those TUs merely reference, and stubs/SyncDialogSeamStubs.cpp
# adds ONLY what the dialog additionally touches (RideCache's ctor/dtor — the
# dialog walks the athlete's ride cache; the import boundary never did). No
# GarminConnect.cpp is compiled here: the contract is CloudService's, and driving
# it from a spy store is what proves the ~15 siblings are governed by the same
# code path.
#
# Python-free -> `garmin-fast`. Needs a QApplication (the dialog is a QWidget),
# hence the offscreen platform.
add_executable(testGarminConnectReadFailedConsumer
    testGarminConnectReadFailedConsumer.cpp
    stubs/ImportSeamStubs.cpp
    stubs/SyncDialogSeamStubs.cpp
    # The consumers under test.
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.h
    # The REAL parsers, so uncompressRide's verdict is the real one.
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.h
    ${CMAKE_SOURCE_DIR}/src/FileIO/FitRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxParser.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectReadFailedConsumer PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectReadFailedConsumer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ANT
    ${CMAKE_SOURCE_DIR}/src/Train
    ${CMAKE_SOURCE_DIR}/src/FileIO
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/src/Charts
    ${CMAKE_SOURCE_DIR}/src/Metrics
    ${CMAKE_SOURCE_DIR}/src/Gui
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Planning
    ${CMAKE_SOURCE_DIR}/src/Coach
    ${CMAKE_SOURCE_DIR}/qwt/src
    ${CMAKE_SOURCE_DIR}/contrib/qxt/src
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/json
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/qwtcurve
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/flowlayout
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/codeeditor
    ${CMAKE_SOURCE_DIR}/contrib/lmfit
    ${CMAKE_SOURCE_DIR}/contrib/boost
    ${CMAKE_SOURCE_DIR}/contrib/kmeans
    ${CMAKE_SOURCE_DIR}/contrib/voronoi
    ${CMAKE_SOURCE_DIR}/contrib/httpserver
    ${CMAKE_SOURCE_DIR}/contrib/qzip
    ${CMAKE_BINARY_DIR}
)

target_compile_definitions(testGarminConnectReadFailedConsumer PRIVATE
    GC_WANT_GARMINCONNECT
    "GARMIN_PY_MODULE_DIR=\"${CMAKE_SOURCE_DIR}/src/Python/garminconnect\""
)

find_package(ZLIB REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Sql Concurrent SerialPort Multimedia MultimediaWidgets PrintSupport Svg
    Bluetooth DBus Charts OpenGL OpenGLWidgets WebEngineCore WebEngineWidgets
    WebEngineQuick WebChannel Positioning Qml Quick)
target_link_libraries(testGarminConnectReadFailedConsumer PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineCore
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineQuick
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Core5Compat
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
    qwt
)
target_compile_features(testGarminConnectReadFailedConsumer PRIVATE cxx_std_17)

add_test(NAME testGarminConnectReadFailedConsumer COMMAND testGarminConnectReadFailedConsumer)
set_tests_properties(testGarminConnectReadFailedConsumer PROPERTIES
    LABELS "garmin-fast"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# --- TEST-070 / TEST-071 — DEC-garmin-024: the sync dialog's LIFETIME ---------
# A3-R017-F1 is a use-after-free that REQ-017 Slice B introduced, so the thing
# under test is the REAL CloudServiceSyncDialog and its REAL destructor (the one
# that calls closeAndDeleteStore). Same seam as TEST-067/TEST-069: compile the
# REAL src/Cloud/CloudService.cpp and satisfy the application layers it merely
# references with stubs/ImportSeamStubs.cpp + stubs/SyncDialogSeamStubs.cpp. The
# service is the only stand-in, and it stands in for GarminConnect by doing the
# one thing that matters — running a nested QEventLoop inside readFile.
#
# WHY THIS ONE TARGET IS BUILT WITH AddressSanitizer
# --------------------------------------------------
# A3-R017's standing lesson: a Qt lifetime claim must be EXECUTED, not reasoned.
# A test that "passes" because the freed store's bytes happened to survive proves
# nothing, so the memory model itself has to be the judge. -fsanitize=address is
# applied to compilation AND linking of this target only; the test source
# #errors out if it is ever compiled without it, so this wiring cannot rot
# silently. Before the fix this binary aborts with
#   AddressSanitizer: heap-use-after-free ... in BlockingStore::readFile
#
# detect_leaks=0: LeakSanitizer would report the process-lifetime allocations Qt,
# qwt and GlobalContext make on the way to main() and drown the signal. The LEAK
# half of this slice is asserted directly instead, by asking ASan's own shadow
# whether the dialog's in-flight QByteArray was released
# (__asan_region_is_poisoned) and by QPointer-observing that dialog and store are
# both destroyed — both far sharper than a whole-process leak report.
#
# Python-free -> `garmin-fast`. Needs a QApplication (the dialog is a QWidget),
# hence the offscreen platform.
add_executable(testGarminConnectSyncDialogClose
    testGarminConnectSyncDialogClose.cpp
    stubs/ImportSeamStubs.cpp
    stubs/SyncDialogSeamStubs.cpp
    # The class under test.
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.cpp
    ${CMAKE_SOURCE_DIR}/src/Cloud/CloudService.h
    # The REAL parsers CloudService.cpp dispatches to.
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/RideFile.h
    ${CMAKE_SOURCE_DIR}/src/FileIO/FitRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxRideFile.cpp
    ${CMAKE_SOURCE_DIR}/src/FileIO/TcxParser.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zip.cpp
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipreader.h
    ${CMAKE_SOURCE_DIR}/contrib/qzip/zipwriter.h
)

set_target_properties(testGarminConnectSyncDialogClose PROPERTIES AUTOMOC ON)

target_include_directories(testGarminConnectSyncDialogClose PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/ANT
    ${CMAKE_SOURCE_DIR}/src/Train
    ${CMAKE_SOURCE_DIR}/src/FileIO
    ${CMAKE_SOURCE_DIR}/src/Cloud
    ${CMAKE_SOURCE_DIR}/src/Charts
    ${CMAKE_SOURCE_DIR}/src/Metrics
    ${CMAKE_SOURCE_DIR}/src/Gui
    ${CMAKE_SOURCE_DIR}/src/Core
    ${CMAKE_SOURCE_DIR}/src/Planning
    ${CMAKE_SOURCE_DIR}/src/Coach
    ${CMAKE_SOURCE_DIR}/qwt/src
    ${CMAKE_SOURCE_DIR}/contrib/qxt/src
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/json
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/qwtcurve
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/flowlayout
    ${CMAKE_SOURCE_DIR}/contrib/qtsolutions/codeeditor
    ${CMAKE_SOURCE_DIR}/contrib/lmfit
    ${CMAKE_SOURCE_DIR}/contrib/boost
    ${CMAKE_SOURCE_DIR}/contrib/kmeans
    ${CMAKE_SOURCE_DIR}/contrib/voronoi
    ${CMAKE_SOURCE_DIR}/contrib/httpserver
    ${CMAKE_SOURCE_DIR}/contrib/qzip
    ${CMAKE_BINARY_DIR}
)

target_compile_definitions(testGarminConnectSyncDialogClose PRIVATE
    GC_WANT_GARMINCONNECT
    "GARMIN_PY_MODULE_DIR=\"${CMAKE_SOURCE_DIR}/src/Python/garminconnect\""
)

# The evidence bar for this slice, expressed in the build.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(testGarminConnectSyncDialogClose PRIVATE
        -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(testGarminConnectSyncDialogClose PRIVATE -fsanitize=address)
else()
    message(FATAL_ERROR
        "testGarminConnectSyncDialogClose (TEST-070/071) is a lifetime test and requires "
        "AddressSanitizer; no -fsanitize=address for compiler ${CMAKE_CXX_COMPILER_ID}.")
endif()

find_package(ZLIB REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Sql Concurrent SerialPort Multimedia MultimediaWidgets PrintSupport Svg
    Bluetooth DBus Charts OpenGL OpenGLWidgets WebEngineCore WebEngineWidgets
    WebEngineQuick WebChannel Positioning Qml Quick)
target_link_libraries(testGarminConnectSyncDialogClose PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Xml
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::SerialPort
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Bluetooth
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineCore
    Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    Qt${QT_VERSION_MAJOR}::WebEngineQuick
    Qt${QT_VERSION_MAJOR}::WebChannel
    Qt${QT_VERSION_MAJOR}::Positioning
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Core5Compat
    Qt${QT_VERSION_MAJOR}::Test
    ZLIB::ZLIB
    qwt
)
target_compile_features(testGarminConnectSyncDialogClose PRIVATE cxx_std_17)

add_test(NAME testGarminConnectSyncDialogClose COMMAND testGarminConnectSyncDialogClose)
set_tests_properties(testGarminConnectSyncDialogClose PROPERTIES
    LABELS "garmin-fast"
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1")

# THE SAME EXECUTABLE, REGISTERED A SECOND TIME UNDER A SECOND QPA BACKEND.
#
# WHY TWICE. Every gate in this project pinned exactly ONE QT_QPA_PLATFORM, so
# every gate in this project was a SINGLE-ENVIRONMENT gate: whatever the pinned
# backend happened to hide, the suite could not see. This target is the proof of
# that. Its completion-slot fixtures used to queue an action behind a completion
# and rely on the production QApplication::processEvents() inside the slot to
# deliver it, and a nested processEvents() is ONE non-blocking
# g_main_context_iteration - not a guaranteed drain. Under `offscreen` the queued
# action happened to be dispatched there and the suite was 49/0 green; under
# `minimal` - same dispatcher class, same machine, same binary - it was not, and
# a slot that had been COMMITTED green failed (48/1). Under an ambient wayland
# session two slots failed (47/2). One pinned backend meant that defect lived in
# the tree, committed and green, for a whole wave.
#
# So the backend is a DIMENSION of the gate, not a constant of it, and this
# second registration is the guard against the single-environment gate coming
# back. `minimal` deliberately: it is headless, ships with Qt, needs no display
# server and no compositor, so it is as CI-portable as `offscreen` is. Wayland
# (which fails differently again) is NOT registered - it needs a live compositor
# and is a developer-machine check only.
#
# Same LABELS as its sibling so `ctest -L garmin-fast` runs BOTH: a gate the
# label selection can silently drop half of is the same defect one level up.
add_test(NAME testGarminConnectSyncDialogClose_minimal COMMAND testGarminConnectSyncDialogClose)
set_tests_properties(testGarminConnectSyncDialogClose_minimal PROPERTIES
    LABELS "garmin-fast"
    ENVIRONMENT "QT_QPA_PLATFORM=minimal;ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:halt_on_error=1")
