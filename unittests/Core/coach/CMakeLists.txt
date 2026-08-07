find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Test)

add_executable(testCoachTools
    testCoachTools.cpp
    ${CMAKE_SOURCE_DIR}/src/Coach/GCToolExecutor.cpp
    ${CMAKE_SOURCE_DIR}/src/Coach/ToolConfirmCard.cpp
    ${CMAKE_SOURCE_DIR}/src/Coach/PlanPreviewCard.cpp
    ${CMAKE_SOURCE_DIR}/src/Coach/LLMService.h  # AUTOMOC needs this to generate moc_LLMService for FakeNoToolService
)

target_include_directories(testCoachTools PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Coach
)

# Force-include the stub preamble before every .cpp so that when GCToolExecutor.cpp
# includes Context.h / Athlete.h / Season.h / Seasons.h, the real headers' guards
# are already defined and the full GC class hierarchy is skipped.
target_compile_options(testCoachTools PRIVATE
    "-include${CMAKE_CURRENT_SOURCE_DIR}/stubs/GCStubPreamble.h"
)

target_link_libraries(testCoachTools PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
)

target_compile_features(testCoachTools PRIVATE cxx_std_17)

add_test(NAME testCoachTools COMMAND testCoachTools)
