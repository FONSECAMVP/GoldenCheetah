# B-STAGE9-12 -- stderr-buffering fix behind REQ-NF-Obs-001's gc_obs traces:
# setvbuf(stderr, nullptr, _IOLBF, 0) right after nostderr()'s freopen in
# src/Core/main.cpp (plus an _IONBF companion in its WIN32 branch, since
# MSVC documents _IOLBF as equivalent to full buffering for file streams).
#
# main.cpp cannot be linked into a test binary (it owns `main()`, a
# QApplication, and the whole GUI dependency graph), so testGarminStderrBuffering
# exercises a faithful POSIX mirror of nostderr() instead (nostderr_mirror.cpp,
# Unix-only); test_main_cpp_contract.py separately pins main.cpp's own text so
# the mirror cannot drift from an unfixed production function unnoticed.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(
    NAME testMainCppStderrContract
    COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/test_main_cpp_contract.py"
)
set_tests_properties(testMainCppStderrContract PROPERTIES
    LABELS "garmin-stderr-buf"
    TIMEOUT 30)

if(UNIX)
    add_executable(nostderr_mirror nostderr_mirror.cpp)
    target_compile_features(nostderr_mirror PRIVATE cxx_std_17)

    add_test(
        NAME testGarminStderrBuffering
        COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/test_stderr_buffering.py"
                "$<TARGET_FILE:nostderr_mirror>"
    )
    set_tests_properties(testGarminStderrBuffering PROPERTIES
        LABELS "garmin-stderr-buf"
        TIMEOUT 30)
endif()
