# GoldenCheetah Coach Module CMakeLists.txt

# Coach module source files (headers included for AUTOMOC)
set(COACH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Secrets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/LLMService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenAIClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/OpenAIClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AnthropicClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AnthropicClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GeminiClient.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GeminiClient.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AthleteContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AthleteContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PromptBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PromptBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CoachChatWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CoachChatWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GCToolExecutor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GCToolExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolConfirmCard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ToolConfirmCard.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanPreviewCard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/PlanPreviewCard.cpp
)

# Create Coach library (not a module/plugin, but a static library linked to main executable)
add_library(Coach STATIC ${COACH_SOURCES})

# Set C++ standard for this module
target_compile_features(Coach PRIVATE cxx_std_17)

# Link Qt Core, Network, Widgets, Sql, Concurrent and Xml for LLM API calls, UI, and Core dependencies
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Widgets Sql Concurrent Xml)
target_link_libraries(Coach PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Xml)

# Platform-specific settings
if(WIN32)
    target_compile_definitions(Coach PRIVATE GC_COACH_EXPORT)
elseif(APPLE)
    target_compile_definitions(Coach PRIVATE GC_COACH_EXPORT)
else()
    # Linux settings - no special defines needed
endif()

# Installation - for plugin-style deployment (optional)
install(
    TARGETS Coach
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
