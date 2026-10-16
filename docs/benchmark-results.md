# Local SMA benchmark results

This measured Release snapshot was recorded on 2026-10-16. The [JSON snapshot](benchmark-20261016.json) preserves all measurement entries; only the host name was removed from its context. New runs write `build/benchmark.json`. This is a local measurement, separate from the benchmark executed by GitHub Actions.

## Reproduction and scope

Run from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCOURSE_BENCHMARK_REPETITIONS=3
cmake --build build --parallel 2
cmake --build build --target run_benchmarks
```

The snapshot uses Apple Clang 21.0.0, C++17, CMake 4.4.3, Google Benchmark 1.9.5, and a Release build on macOS arm64. Each workload has three repetitions with a minimum requested time of 0.02 seconds per repetition. Data is deterministic synthetic positive prices near 100.

Both methods use the same input and include input validation, output-vector allocation/destruction, and their own algorithm overhead. CSV I/O, data generation, and the preliminary result cross-check are excluded. The table uses the median of the three CPU times, converted from nanoseconds to microseconds. The ratio is naive time divided by rolling time: below 1 favors naive; above 1 favors rolling. See the [algorithm and benchmark method](algorithms.md).

## Results, including small windows

| Observations n | Window w | Naive CPU median (µs) | Rolling CPU median (µs) | Naive / rolling |
|---:|---:|---:|---:|---:|
| 1024 | 1 | 0.412 | 0.436 | 0.943 |
| 1024 | 2 | 4.947 | 10.459 | 0.473 |
| 1024 | 3 | 6.567 | 12.671 | 0.518 |
| 1024 | 5 | 9.676 | 14.816 | 0.653 |
| 1024 | 10 | 20.450 | 17.579 | 1.163 |
| 1024 | 20 | 51.210 | 17.519 | 2.923 |
| 1024 | 100 | 391.932 | 18.145 | 21.600 |
| 16384 | 1 | 6.525 | 6.797 | 0.960 |
| 16384 | 2 | 81.291 | 176.988 | 0.459 |
| 16384 | 3 | 107.303 | 202.806 | 0.529 |
| 16384 | 5 | 157.824 | 244.078 | 0.647 |
| 16384 | 10 | 331.494 | 270.748 | 1.224 |
| 16384 | 20 | 833.515 | 289.312 | 2.881 |
| 16384 | 100 | 6938.500 | 314.109 | 22.089 |
| 65536 | 1 | 26.841 | 28.253 | 0.950 |
| 65536 | 2 | 324.506 | 704.775 | 0.460 |
| 65536 | 3 | 431.538 | 820.912 | 0.526 |
| 65536 | 5 | 627.977 | 982.621 | 0.639 |
| 65536 | 10 | 1337.400 | 1109.840 | 1.205 |
| 65536 | 20 | 3340.000 | 1200.250 | 2.783 |
| 65536 | 100 | 27440.000 | 1264.000 | 21.709 |

## What the small-window experiment shows

At all three tested sizes, naive was faster for windows 2, 3, and 5; rolling was faster for windows 10, 20, and 100. The sampled change in the winner therefore lies between windows 5 and 10 on this machine. Windows 6–9 were not measured, so this does not identify an exact crossover or a universal threshold.

For n = 65536, window 2 took 324.506 µs with naive and 704.775 µs with rolling. Maintaining the two stacks costs more than summing two prices. At window 10 the times were 1337.400 µs and 1109.840 µs; at window 100 they were 27440.000 µs and 1264.000 µs, a ratio of about 21.71. Repeated window summation increasingly dominates the naive method.

Window 1 uses the same validation-and-copy fast path in both methods. Its small timing differences do not demonstrate a queue optimization. Both implementations remain explicitly selectable; this experiment does not add an automatic method switch or a new numerical algorithm.

All 42 workloads had exactly three raw measurements: 126 iteration records plus 168 aggregate rows, 294 JSON entries in total. The strengthened validator checked exact names, unique repetition indices, counts, positive finite timings, and consistent units. Its CTest regression suite also rejected aggregate-only input, missing repetitions, duplicates, unexpected names, and invalid timings.

## Measurement limits and earlier snapshot

The run reported macOS CPU-frequency and thread-affinity warnings. Google Benchmark could not read `hw.cpufrequency`; the JSON frequency field must not be interpreted as the actual CPU frequency. The full workload families run in registration order, with three short repetitions. System load, clock behavior, order effects, compiler, and hardware can change both timings and the crossover. These results support the reported workload comparisons, not a fixed speedup on other machines.

The [2026-10-14 snapshot](benchmark-20261014.json) retains the earlier 18-workload experiment with windows 1, 20, and 100. It is historical evidence, not an additional repetition of this experiment.
