cmake_minimum_required(VERSION 3.24)
project(market_data_toolkit VERSION 0.3.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(CTest)
include(FetchContent)
include(cmake/benchmark_config.cmake)
option(COURSE_BUILD_BENCHMARKS "Build the SMA algorithm comparison" ON)
option(COURSE_BUILD_DOCS "Provide the Doxygen docs target" ON)
set(COURSE_BENCHMARK_REPETITIONS 3 CACHE STRING "Repetitions of each SMA workload")
if(NOT COURSE_BENCHMARK_REPETITIONS MATCHES "^[1-9][0-9]*$")
    message(FATAL_ERROR "COURSE_BENCHMARK_REPETITIONS must be a positive integer")
endif()

add_library(course_core src/core.cpp src/indicators.cpp src/atomic_output.cpp)
target_include_directories(course_core PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(MSVC)
    target_compile_options(course_core PRIVATE /W4)
else()
    target_compile_options(course_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(market_data src/main.cpp)
target_link_libraries(market_data PRIVATE course_core)

# Official release archives are included in the repository for offline builds.
if(BUILD_TESTING)
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(doctest
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/doctest-2.5.3.tar.gz"
        URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(doctest)
    add_executable(course_tests tests/test_core.cpp tests/test_atomic_output.cpp)
    target_link_libraries(course_tests PRIVATE course_core doctest::doctest)
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
    doctest_discover_tests(course_tests)
    add_test(NAME cli_end_to_end
        COMMAND "${CMAKE_COMMAND}"
            "-DAPP=$<TARGET_FILE:market_data>"
            "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/cli-test"
            -P "${PROJECT_SOURCE_DIR}/tests/test_cli.cmake"
    )
    set_tests_properties(cli_end_to_end PROPERTIES TIMEOUT 30)
    add_test(NAME benchmark_json_validation
        COMMAND "${CMAKE_COMMAND}"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/benchmark-validation-test"
            -P "${PROJECT_SOURCE_DIR}/tests/test_benchmark_validation.cmake"
    )
    set_tests_properties(benchmark_json_validation PROPERTIES TIMEOUT 30)
endif()

if(COURSE_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
        URL "${PROJECT_SOURCE_DIR}/third_party/archives/benchmark-1.9.5.tar.gz"
        URL_HASH SHA256=9631341c82bac4a288bef951f8b26b41f69021794184ece969f8473977eaa340
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(benchmark)
    add_executable(course_benchmark benchmarks/benchmark_core.cpp)
    target_link_libraries(course_benchmark PRIVATE course_core benchmark::benchmark)
    list(JOIN COURSE_BENCHMARK_COUNTS "," benchmark_counts_cpp)
    list(JOIN COURSE_BENCHMARK_WINDOWS "," benchmark_windows_cpp)
    target_compile_definitions(course_benchmark PRIVATE
        "COURSE_BENCHMARK_COUNTS=${benchmark_counts_cpp}"
        "COURSE_BENCHMARK_WINDOWS=${benchmark_windows_cpp}"
    )
    add_custom_target(run_benchmarks
        COMMAND "$<TARGET_FILE:course_benchmark>"
            --benchmark_min_time=0.02s
            "--benchmark_repetitions=${COURSE_BENCHMARK_REPETITIONS}"
            "--benchmark_out=${CMAKE_BINARY_DIR}/benchmark.json"
            --benchmark_out_format=json
        COMMAND "${CMAKE_COMMAND}"
            "-DRESULTS=${CMAKE_BINARY_DIR}/benchmark.json"
            "-DEXPECTED_REPETITIONS=${COURSE_BENCHMARK_REPETITIONS}"
            -P "${PROJECT_SOURCE_DIR}/cmake/check_benchmark.cmake"
        DEPENDS course_benchmark
        USES_TERMINAL
        VERBATIM
        COMMENT "Compare both SMA algorithms and validate benchmark.json"
    )
endif()

if(COURSE_BUILD_DOCS)
    find_package(Doxygen REQUIRED)
    find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
    if(BUILD_TESTING)
        add_test(NAME documentation_link_validation
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/tests/test_docs_validation.py"
        )
        set_tests_properties(documentation_link_validation PROPERTIES TIMEOUT 30)
    endif()
    configure_file(Doxyfile.in "${CMAKE_BINARY_DIR}/Doxyfile" @ONLY)
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
        COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/check_docs.py"
            "${CMAKE_BINARY_DIR}/docs/html"
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        USES_TERMINAL
        VERBATIM
        COMMENT "Generate HTML documentation and validate local links and anchors"
    )
endif()
