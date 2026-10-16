if(NOT DEFINED WORK_DIR OR "${WORK_DIR}" STREQUAL "")
    message(FATAL_ERROR "WORK_DIR must identify a build-local test directory")
endif()
file(MAKE_DIRECTORY "${WORK_DIR}")
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/benchmark_config.cmake")
set(validator "${CMAKE_CURRENT_LIST_DIR}/../cmake/check_benchmark.cmake")

function(make_fixture repetitions output)
    math(EXPR last_repetition "${repetitions} - 1")
    set(entries)
    foreach(workload IN LISTS COURSE_BENCHMARK_EXPECTED_WORKLOADS)
        foreach(repetition RANGE 0 ${last_repetition})
            list(APPEND entries
                "{\"run_name\":\"${workload}\",\"run_type\":\"iteration\",\"repetitions\":${repetitions},\"repetition_index\":${repetition},\"iterations\":10,\"real_time\":12.5,\"cpu_time\":10.5,\"time_unit\":\"ns\"}")
        endforeach()
    endforeach()
    list(JOIN entries "," measurements)
    set("${output}" "{\"benchmarks\":[${measurements}]}" PARENT_SCOPE)
endfunction()

function(check_case label fixture expected_repetitions should_pass)
    set(path "${WORK_DIR}/${label}.json")
    file(WRITE "${path}" "${fixture}")
    execute_process(
        COMMAND "${CMAKE_COMMAND}"
            "-DRESULTS=${path}"
            "-DEXPECTED_REPETITIONS=${expected_repetitions}"
            -P "${validator}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(should_pass AND NOT "${result}" STREQUAL "0")
        message(FATAL_ERROR "${label}: valid measurements rejected\n${output}${error}")
    elseif(NOT should_pass AND "${result}" STREQUAL "0")
        message(FATAL_ERROR "${label}: invalid measurements accepted\n${output}")
    endif()
endfunction()

make_fixture(3 valid)
check_case(valid "${valid}" 3 TRUE)
make_fixture(2 custom_repetitions)
check_case(custom_repetitions "${custom_repetitions}" 2 TRUE)

string(JSON invalid REMOVE "${valid}" benchmarks 0)
check_case(missing_measurement "${invalid}" 3 FALSE)
set(invalid "${valid}")
foreach(index 2 1 0)
    string(JSON invalid REMOVE "${invalid}" benchmarks ${index})
endforeach()
check_case(missing_workload "${invalid}" 3 FALSE)

set(aggregates)
foreach(workload IN LISTS COURSE_BENCHMARK_EXPECTED_WORKLOADS)
    list(APPEND aggregates
        "{\"run_name\":\"${workload}\",\"run_type\":\"aggregate\",\"repetitions\":3}")
endforeach()
list(JOIN aggregates "," aggregate_entries)
check_case(aggregate_only "{\"benchmarks\":[${aggregate_entries}]}" 3 FALSE)

string(JSON invalid SET "${valid}" benchmarks 1 repetition_index 0)
check_case(duplicate_index "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 repetition_index 3)
check_case(index_out_of_range "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 run_name "\"BM_SMA_Unknown/n:1024/window:1\"")
check_case(wrong_name "${invalid}" 3 FALSE)
string(JSON extra GET "${valid}" benchmarks 0)
string(JSON extra SET "${extra}" run_name "\"BM_SMA_Naive/n:999/window:1\"")
string(JSON length LENGTH "${valid}" benchmarks)
string(JSON invalid SET "${valid}" benchmarks ${length} "${extra}")
check_case(extra_workload "${invalid}" 3 FALSE)

string(JSON invalid SET "${valid}" benchmarks 0 iterations 0)
check_case(zero_iterations "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 cpu_time 0)
check_case(zero_timing "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 real_time -1)
check_case(negative_timing "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 cpu_time "\"NaN\"")
check_case(nonnumeric_timing "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 time_unit "\"ticks\"")
check_case(unknown_unit "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 time_unit "\"ms\"")
check_case(mixed_units "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 error_occurred true)
check_case(reported_error "${invalid}" 3 FALSE)
string(JSON invalid SET "${valid}" benchmarks 0 repetitions 2)
check_case(declared_repetitions "${invalid}" 3 FALSE)

message(STATUS "Benchmark JSON validation regression checks completed")
