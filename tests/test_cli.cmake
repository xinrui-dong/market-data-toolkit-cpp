# This test writes only to the build-local WORK_DIR supplied by CTest.
foreach(required APP SOURCE_DIR WORK_DIR)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "Missing required test variable: ${required}")
    endif()
endforeach()
file(MAKE_DIRECTORY "${WORK_DIR}")
set(sample "${SOURCE_DIR}/data/sample_prices.csv")
set(duplicate "${SOURCE_DIR}/data/invalid_duplicate.csv")
if(NOT EXISTS "${sample}" OR NOT EXISTS "${duplicate}")
    message(FATAL_ERROR "The documented sample CSV fixtures are missing")
endif()

function(expect_success label)
    execute_process(
        COMMAND "${APP}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT "${result}" STREQUAL "0")
        message(FATAL_ERROR "${label}: expected success, got ${result}\n${error}")
    endif()
    set(last_stdout "${output}" PARENT_SCOPE)
endfunction()

function(expect_failure label)
    execute_process(
        COMMAND "${APP}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if("${result}" STREQUAL "0")
        message(FATAL_ERROR "${label}: invalid invocation unexpectedly succeeded\n${output}")
    endif()
endfunction()

function(check_sample_csv text)
    string(REPLACE "\r\n" "\n" normalized "${text}")
    # Numeric details are checked independently in doctest. These expressions
    # allow round-trip decimal tails while verifying CLI alignment and content.
    set(number "[-+]?[0-9]+(\\.[0-9]+)?([eE][-+]?[0-9]+)?")
    set(expected
        "^date,close,simple_return,sma\n2026-10-01,100(\\.0+)?,,\n2026-10-02,110(\\.0+)?,${number},\n2026-10-03,99(\\.0+)?,${number},103(\\.0+)?\n2026-10-04,108(\\.0+)?,${number},105\\.666666[0-9]*\n$")
    if(NOT "${normalized}" MATCHES "${expected}")
        message(FATAL_ERROR "Unexpected CLI CSV output:\n${normalized}")
    endif()
endfunction()

expect_success("help" --help)
if(NOT "${last_stdout}" MATCHES "--input" OR NOT "${last_stdout}" MATCHES "--window")
    message(FATAL_ERROR "--help must describe the required input and window options")
endif()
expect_failure("no arguments")
expect_failure("missing window" --input "${sample}")
expect_failure("missing input" --window 3)
expect_failure("missing option value" --input)
expect_failure("unknown option" --input "${sample}" --window 3 --unknown)
expect_failure("duplicate input" --input "${sample}" --input "${sample}" --window 3)
expect_failure("duplicate window" --input "${sample}" --window 3 --window 2)
expect_failure("duplicate algorithm" --input "${sample}" --window 3
               --algorithm naive --algorithm rolling)
expect_failure("unknown algorithm" --input "${sample}" --window 3 --algorithm fast)
foreach(window "0" "-1" "1.5" "2abc" "999999999999999999999999999999")
    expect_failure("invalid window ${window}" --input "${sample}" --window "${window}")
endforeach()

expect_success("default algorithm to stdout" --input "${sample}" --window 3)
check_sample_csv("${last_stdout}")
foreach(algorithm naive rolling)
    set(output "${WORK_DIR}/${algorithm}.csv")
    file(REMOVE "${output}")
    expect_success("${algorithm} output file" --input "${sample}" --window 3
                   --algorithm "${algorithm}" --output "${output}")
    if(NOT EXISTS "${output}")
        message(FATAL_ERROR "The requested output file was not created")
    endif()
    file(READ "${output}" csv)
    check_sample_csv("${csv}")
    expect_success("${algorithm} stdout" --input "${sample}" --window 3
                   --algorithm "${algorithm}")
    check_sample_csv("${last_stdout}")
endforeach()

set(rejected_output "${WORK_DIR}/must-not-be-created.csv")
file(REMOVE "${rejected_output}")
expect_failure("duplicate output" --input "${sample}" --window 3
               --output "${rejected_output}" --output "${rejected_output}")
if(EXISTS "${rejected_output}")
    message(FATAL_ERROR "Invalid arguments must not create an output file")
endif()

set(bad_price "${WORK_DIR}/invalid-price.csv")
file(WRITE "${bad_price}" "date,close\n2026-10-01,100\n2026-10-02,0\n")
set(overflow "${WORK_DIR}/overflow.csv")
file(WRITE "${overflow}"
    "date,close\n2026-10-01,2.2250738585072014e-308\n2026-10-02,1.7976931348623157e308\n")
foreach(input "${duplicate}" "${bad_price}" "${overflow}" "${WORK_DIR}/missing.csv")
    file(REMOVE "${rejected_output}")
    expect_failure("invalid or missing input ${input}" --input "${input}" --window 1
                   --output "${rejected_output}")
    if(EXISTS "${rejected_output}")
        message(FATAL_ERROR "Invalid input or calculation must not create an output file")
    endif()
endforeach()

file(REMOVE "${rejected_output}")
expect_failure("window larger than input" --input "${sample}" --window 5
               --output "${rejected_output}")
if(EXISTS "${rejected_output}")
    message(FATAL_ERROR "An invalid calculation must not create an output file")
endif()

# Use a build-local copy so a regression cannot destroy the source fixture.
set(same_path "${WORK_DIR}/input-and-output.csv")
file(READ "${sample}" original)
file(WRITE "${same_path}" "${original}")
expect_failure("input equals output" --input "${same_path}" --window 3
               --output "${same_path}")
file(READ "${same_path}" after)
if(NOT "${original}" STREQUAL "${after}")
    message(FATAL_ERROR "Rejecting input == output must preserve the input file")
endif()

set(missing_parent "${WORK_DIR}/missing-parent-for-output")
if(EXISTS "${missing_parent}")
    message(FATAL_ERROR "Expected absent test directory: ${missing_parent}")
endif()
expect_failure("unwritable output path" --input "${sample}" --window 3
               --output "${missing_parent}/result.csv")

message(STATUS "CLI integration checks completed")
