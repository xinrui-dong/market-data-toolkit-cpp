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
        message(FATAL_ERROR "${label}: expected exit 0, got ${result}\n${error}")
    endif()
    if(NOT "${error}" STREQUAL "")
        message(FATAL_ERROR "${label}: successful invocation wrote to stderr\n${error}")
    endif()
    set(last_stdout "${output}" PARENT_SCOPE)
endfunction()

function(expect_failure label expected_code diagnostic)
    execute_process(
        COMMAND "${APP}" ${ARGN}
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    # RESULT_VARIABLE can also contain a crash/signal description. Comparing
    # the exact documented code prevents such failures from passing this test.
    if(NOT "${result}" STREQUAL "${expected_code}")
        message(FATAL_ERROR
            "${label}: expected exit ${expected_code}, got ${result}\n${error}")
    endif()
    if(expected_code EQUAL 2)
        set(prefix "^Argument error:")
    else()
        set(prefix "^Error:")
    endif()
    if(NOT "${error}" MATCHES "${prefix}" OR NOT "${error}" MATCHES "${diagnostic}")
        message(FATAL_ERROR "${label}: missing expected stderr diagnostic '${diagnostic}'\n${error}")
    endif()
    if(NOT "${output}" STREQUAL "")
        message(FATAL_ERROR "${label}: failed invocation must not emit successful CSV\n${output}")
    endif()
endfunction()

function(check_file_unchanged path expected label)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "${label}: existing file was removed: ${path}")
    endif()
    file(READ "${path}" actual)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "${label}: existing file contents changed: ${path}")
    endif()
endfunction()

function(directory_entries directory output_variable)
    # Include hidden names without depending on the implementation's temporary
    # directory naming scheme. This is intentionally not a recursive scan.
    file(GLOB entries RELATIVE "${directory}" LIST_DIRECTORIES true
        "${directory}/*" "${directory}/.*")
    list(SORT entries)
    set(${output_variable} "${entries}" PARENT_SCOPE)
endfunction()

function(check_directory_entries directory expected label)
    directory_entries("${directory}" actual)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR
            "${label}: output directory entries changed unexpectedly\nBefore: ${expected}\nAfter: ${actual}")
    endif()
endfunction()

function(check_sample_csv text)
    string(REPLACE "\r\n" "\n" normalized "${text}")
    # Numerical return values are independently checked in doctest; here the
    # expressions allow round-trip tails and check CLI alignment and content.
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
expect_failure("no arguments" 2 "required")
expect_failure("missing window" 2 "required" --input "${sample}")
expect_failure("missing input" 2 "required" --window 3)
expect_failure("missing option value" 2 "missing value" --input)
expect_failure("unknown option" 2 "unknown option" --input "${sample}" --window 3 --unknown)
expect_failure("duplicate input" 2 "duplicate option: --input"
               --input "${sample}" --input "${sample}" --window 3)
expect_failure("duplicate window" 2 "duplicate option: --window"
               --input "${sample}" --window 3 --window 2)
expect_failure("duplicate algorithm" 2 "duplicate option: --algorithm"
               --input "${sample}" --window 3 --algorithm naive --algorithm rolling)
expect_failure("unknown algorithm" 2 "algorithm must be naive or rolling"
               --input "${sample}" --window 3 --algorithm fast)
foreach(window "0" "-1" "1.5" "2abc" "999999999999999999999999999999")
    expect_failure("invalid window ${window}" 2 "window must be a positive integer"
                   --input "${sample}" --window "${window}")
endforeach()

expect_success("default algorithm to stdout" --input "${sample}" --window 3)
check_sample_csv("${last_stdout}")
set(output_parent "${WORK_DIR}/atomic-output")
file(REMOVE_RECURSE "${output_parent}")
file(MAKE_DIRECTORY "${output_parent}")
set(sentinel "Existing output must survive rejected invocations.\n")
foreach(algorithm naive rolling)
    set(output "${output_parent}/${algorithm}.csv")
    file(WRITE "${output}" "${sentinel}")
    directory_entries("${output_parent}" before)
    expect_success("${algorithm} overwrites existing output" --input "${sample}" --window 3
                   --algorithm "${algorithm}" --output "${output}")
    if(NOT "${last_stdout}" STREQUAL "")
        message(FATAL_ERROR "File output must leave stdout empty")
    endif()
    check_directory_entries("${output_parent}" "${before}" "successful output cleanup")
    file(READ "${output}" csv)
    check_sample_csv("${csv}")
    expect_success("${algorithm} stdout" --input "${sample}" --window 3
                   --algorithm "${algorithm}")
    check_sample_csv("${last_stdout}")
endforeach()

set(fresh_parent "${WORK_DIR}/fresh-output")
file(REMOVE_RECURSE "${fresh_parent}")
file(MAKE_DIRECTORY "${fresh_parent}")
expect_success("new output file" --input "${sample}" --window 3
               --output "${fresh_parent}/analysis.csv")
file(READ "${fresh_parent}/analysis.csv" csv)
check_sample_csv("${csv}")
check_directory_entries("${fresh_parent}" "analysis.csv" "new output cleanup")

set(rejected_output "${WORK_DIR}/must-not-be-created.csv")
file(REMOVE "${rejected_output}")
expect_failure("duplicate output" 2 "duplicate option: --output"
               --input "${sample}" --window 3
               --output "${rejected_output}" --output "${rejected_output}")
if(EXISTS "${rejected_output}")
    message(FATAL_ERROR "Invalid arguments must not create an output file")
endif()

set(bad_price "${WORK_DIR}/invalid-price.csv")
file(WRITE "${bad_price}" "date,close\n2026-10-01,100\n2026-10-02,0\n")
set(overflow "${WORK_DIR}/overflow.csv")
file(WRITE "${overflow}"
    "date,close\n2026-10-01,2.2250738585072014e-308\n2026-10-02,1.7976931348623157e308\n")
set(sma_overflow "${WORK_DIR}/sma-overflow.csv")
file(WRITE "${sma_overflow}"
    "date,close\n2026-10-01,1.7976931348623157e308\n2026-10-02,1.7976931348623157e308\n")
file(REMOVE "${WORK_DIR}/missing.csv")
set(protected_output "${output_parent}/protected.csv")
file(WRITE "${protected_output}" "${sentinel}")
directory_entries("${output_parent}" before)

function(expect_preserved_failure label diagnostic input window)
    # Check both nonexistent and existing destinations for the same rejected
    # data/calculation, so neither a new false result nor truncation is missed.
    file(REMOVE "${rejected_output}")
    expect_failure("${label} with new output" 1 "${diagnostic}"
                   --input "${input}" --window "${window}" --output "${rejected_output}")
    if(EXISTS "${rejected_output}")
        message(FATAL_ERROR "${label}: failure created an output file")
    endif()
    expect_failure("${label} preserves existing output" 1 "${diagnostic}"
                   --input "${input}" --window "${window}" --output "${protected_output}")
    check_file_unchanged("${protected_output}" "${sentinel}" "${label}")
    check_directory_entries("${output_parent}" "${before}" "${label} cleanup")
endfunction()

expect_preserved_failure("duplicate dates" "CSV line.*dates" "${duplicate}" 1)
expect_preserved_failure("invalid price" "CSV line.*positive" "${bad_price}" 1)
expect_preserved_failure("return overflow" "nonfinite simple return" "${overflow}" 1)
expect_preserved_failure("SMA overflow" "nonfinite.*SMA" "${sma_overflow}" 2)
expect_preserved_failure("missing input" "could not open input file" "${WORK_DIR}/missing.csv" 1)
expect_preserved_failure("window larger than input" "SMA window" "${sample}" 5)
expect_failure("invalid window preserves output" 2 "window must be a positive integer"
               --input "${sample}" --window 0 --output "${protected_output}")
check_file_unchanged("${protected_output}" "${sentinel}" "invalid window")
check_directory_entries("${output_parent}" "${before}" "invalid window cleanup")

# Use build-local copies so a regression cannot destroy the source fixture.
set(alias_parent "${WORK_DIR}/alias-guard")
file(REMOVE_RECURSE "${alias_parent}")
file(MAKE_DIRECTORY "${alias_parent}")
set(same_path "${alias_parent}/input.csv")
file(READ "${sample}" original)
file(WRITE "${same_path}" "${original}")
expect_failure("input equals output" 1 "input and output must be different files"
               --input "${same_path}" --window 3 --output "${same_path}")
check_file_unchanged("${same_path}" "${original}" "input equals output")
expect_failure("normalized input/output alias" 1 "input and output must be different files"
               --input "${alias_parent}/./input.csv" --window 3 --output "${same_path}")
check_file_unchanged("${same_path}" "${original}" "normalized alias")

foreach(link_kind symbolic hard)
    set(alias "${alias_parent}/${link_kind}-alias.csv")
    if(link_kind STREQUAL "symbolic")
        file(CREATE_LINK "${same_path}" "${alias}" SYMBOLIC RESULT link_result)
    else()
        file(CREATE_LINK "${same_path}" "${alias}" RESULT link_result)
    endif()
    if("${link_result}" STREQUAL "0")
        directory_entries("${alias_parent}" aliases_before)
        expect_failure("input/output ${link_kind} alias" 1 "input and output must be different files"
                       --input "${same_path}" --window 3 --output "${alias}")
        check_file_unchanged("${same_path}" "${original}" "${link_kind} alias input")
        check_file_unchanged("${alias}" "${original}" "${link_kind} alias output")
        check_directory_entries("${alias_parent}" "${aliases_before}" "${link_kind} alias cleanup")
        if(link_kind STREQUAL "symbolic" AND NOT IS_SYMLINK "${alias}")
            message(FATAL_ERROR "Rejecting a symbolic alias must preserve the link")
        endif()
    else()
        message(STATUS "Skipping ${link_kind} alias: link creation unavailable (${link_result})")
    endif()
endforeach()

set(other_target "${output_parent}/unrelated.csv")
set(output_link "${output_parent}/redirect.csv")
file(WRITE "${other_target}" "${sentinel}")
file(CREATE_LINK "${other_target}" "${output_link}" SYMBOLIC RESULT link_result)
if("${link_result}" STREQUAL "0")
    if(UNIX)
        file(CHMOD "${other_target}" PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
    endif()
    directory_entries("${output_parent}" before_link)
    expect_failure("symlink output to unrelated file" 1 "output.*symbolic link"
                   --input "${sample}" --window 3 --output "${output_link}")
    check_file_unchanged("${other_target}" "${sentinel}" "symlink target")
    if(NOT IS_SYMLINK "${output_link}")
        message(FATAL_ERROR "A rejected output symlink must not be replaced")
    endif()
    check_directory_entries("${output_parent}" "${before_link}" "symlink output cleanup")
    if(UNIX)
        file(CHMOD "${other_target}" PERMISSIONS OWNER_READ OWNER_WRITE)
    endif()
else()
    message(STATUS "Skipping output symlink: link creation unavailable (${link_result})")
endif()

set(dangling_link "${output_parent}/dangling.csv")
set(dangling_target "${output_parent}/absent-target.csv")
file(CREATE_LINK "${dangling_target}" "${dangling_link}" SYMBOLIC RESULT link_result)
if("${link_result}" STREQUAL "0")
    directory_entries("${output_parent}" before_link)
    expect_failure("dangling output symlink" 1 "output.*symbolic link"
                   --input "${sample}" --window 3 --output "${dangling_link}")
    if(NOT IS_SYMLINK "${dangling_link}" OR EXISTS "${dangling_target}")
        message(FATAL_ERROR "Rejecting a dangling output link must preserve it without creating its target")
    endif()
    check_directory_entries("${output_parent}" "${before_link}" "dangling symlink cleanup")
else()
    message(STATUS "Skipping dangling symlink: link creation unavailable (${link_result})")
endif()

# Nonregular targets are rejected before replacement. A deterministic failure
# inside rename itself is tested by the atomic-output module, without CLI races.
set(directory_output "${output_parent}/directory-target")
file(MAKE_DIRECTORY "${directory_output}")
file(WRITE "${directory_output}/keep.txt" "${sentinel}")
directory_entries("${output_parent}" before_directory)
expect_failure("directory output target" 1 "output.*regular file"
               --input "${sample}" --window 3 --output "${directory_output}")
check_file_unchanged("${directory_output}/keep.txt" "${sentinel}" "directory target")
check_directory_entries("${output_parent}" "${before_directory}" "directory target cleanup")

set(missing_parent "${WORK_DIR}/missing-parent-for-output")
if(EXISTS "${missing_parent}")
    message(FATAL_ERROR "Expected absent test directory: ${missing_parent}")
endif()
expect_failure("missing output parent" 1 "temporary output directory"
               --input "${sample}" --window 3 --output "${missing_parent}/result.csv")
if(EXISTS "${missing_parent}")
    message(FATAL_ERROR "Failed output must not create the requested parent directory")
endif()

message(STATUS "CLI integration checks completed")
