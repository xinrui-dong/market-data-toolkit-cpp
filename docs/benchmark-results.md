# Local SMA benchmark results

This is a measured local snapshot from 2026-10-14, not a required speedup or a remote CI result. The [JSON snapshot](benchmark-20261014.json) preserves the measured entries; only the host name was removed from its context. New runs write `build/benchmark.json` and may differ.

## Reproduction and scope

Run from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
cmake --build build --target run_benchmarks
```

The snapshot uses Apple Clang 21.0.0, C++17, CMake 4.4.3, Google Benchmark 1.9.5, and a Release build on macOS arm64. Each workload has three repetitions with a minimum requested time of 0.02 seconds per repetition. Data is deterministic synthetic positive prices, not observed market prices.

The table uses the median of the three CPU-time measurements, converted from nanoseconds to microseconds. The ratio is naive time divided by rolling time; a ratio below 1 means rolling was slightly slower. This ratio includes input validation, returned-vector allocation/destruction, and the rolling queue cost; it excludes CSV I/O, data generation, and the preliminary result cross-check. See [algorithm and benchmark method](algorithms.md).

| Observations n | Window w | Naive CPU median (µs) | Rolling CPU median (µs) | Naive / rolling |
|---:|---:|---:|---:|---:|
| 1024 | 1 | 0.448 | 0.453 | 0.989 |
| 1024 | 20 | 54.253 | 18.650 | 2.909 |
| 1024 | 100 | 409.725 | 19.105 | 21.446 |
| 16384 | 1 | 6.863 | 6.959 | 0.986 |
| 16384 | 20 | 898.581 | 296.688 | 3.029 |
| 16384 | 100 | 7335.500 | 310.733 | 23.607 |
| 65536 | 1 | 28.286 | 28.385 | 0.997 |
| 65536 | 20 | 3603.375 | 1177.304 | 3.061 |
| 65536 | 100 | 28998.000 | 1248.136 | 23.233 |

## Interpretation

For n = 65536 and w = 100, the measured CPU medians were 28.998 ms and 1.248136 ms, a ratio of about 23.23. The rolling implementation avoids recomputing each complete window. At w = 1, both methods copy the input after validation and have almost identical timing; these measurements show no rolling advantage. Window size and allocation overhead matter.

The run reported CPU-frequency metadata and thread-affinity warnings on macOS. Google Benchmark could not read `hw.cpufrequency`, so the JSON frequency field must not be treated as the actual CPU frequency. The reported durations are still measurements; load, frequency changes, scheduling, compiler, and hardware limit comparisons across machines. There is no universal performance or numerical-accuracy guarantee.

All 18 workloads produced successful measurement entries. The target additionally validated the JSON so `SkipWithError` or missing workloads could not silently count as a successful benchmark run.
