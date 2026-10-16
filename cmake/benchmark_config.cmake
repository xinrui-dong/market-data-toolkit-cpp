# Shared by the executable's compile definitions and the JSON validator.
set(COURSE_BENCHMARK_COUNTS 1024 16384 65536)
set(COURSE_BENCHMARK_WINDOWS 1 2 3 5 10 20 100)
set(COURSE_BENCHMARK_METHODS BM_SMA_Naive BM_SMA_Rolling)

set(COURSE_BENCHMARK_EXPECTED_WORKLOADS)
foreach(method IN LISTS COURSE_BENCHMARK_METHODS)
    foreach(count IN LISTS COURSE_BENCHMARK_COUNTS)
        foreach(window IN LISTS COURSE_BENCHMARK_WINDOWS)
            list(APPEND COURSE_BENCHMARK_EXPECTED_WORKLOADS
                "${method}/n:${count}/window:${window}")
        endforeach()
    endforeach()
endforeach()
