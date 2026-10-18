# Homework acceptance index

**Market Data Toolkit · C++17 · v0.3.4**

Validates a single price-series CSV, calculates adjacent simple price returns
and trailing SMA, and benchmarks naive versus rolling SMA.

**Submission:** [v0.3.4 Release](https://github.com/xinrui-dong/market-data-toolkit-cpp/releases/tag/v0.3.4)
· [Tagged source](https://github.com/xinrui-dong/market-data-toolkit-cpp/tree/v0.3.4)
· [Doxygen](https://xinrui-dong.github.io/market-data-toolkit-cpp/)

Use the release to identify the exact commit, its successful CI run, source,
verification evidence, and a fixed documentation snapshot. Acceptance applies
to that tagged version. Live Pages follows `main`.

| Criterion | Points | Evidence |
|---|---:|---|
| 1. Public repository, clear structure, and README | 2 | [README](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/README.md); separate [src](https://github.com/xinrui-dong/market-data-toolkit-cpp/tree/v0.3.4/src), [tests](https://github.com/xinrui-dong/market-data-toolkit-cpp/tree/v0.3.4/tests), and [benchmarks](https://github.com/xinrui-dong/market-data-toolkit-cpp/tree/v0.3.4/benchmarks) directories |
| 2. CMake and automatic CI build | 4 | [CMakeLists.txt](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/CMakeLists.txt) and [CI workflow](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/.github/workflows/ci.yml); automatic Ubuntu, macOS and Windows Release builds on every push |
| 3. C++ testing framework and tests in CI | 4 | [doctest cases](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/tests/test_core.cpp) and [output tests](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/tests/test_atomic_output.cpp); 28 doctest cases plus CLI, real-data, benchmark-validator, and documentation-validator tests = 32 CTest tests |
| 4. Automatic benchmark and results in CI logs | 4 | [Google Benchmark workloads](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/benchmarks/benchmark_core.cpp); 42 configurations times 3 repetitions = 126 raw measurements per platform; CI logs and benchmark JSON available through the release |
| 5. Automatic Doxygen publication | 4 | [Doxyfile.in](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/Doxyfile.in) and [Pages](https://xinrui-dong.github.io/market-data-toolkit-cpp/); README documentation link, static navigation, and local file/anchor validation |
| 6. Agent instructions in the repository root | 2 | [AGENTS.md](https://github.com/xinrui-dong/market-data-toolkit-cpp/blob/v0.3.4/AGENTS.md): purpose, structure, commands, style, and constraints |
| **Total** | **20** | Criteria 2 through 5 require successful CI/CD execution |

## Reproduce

Install a C++17 compiler, CMake 3.24 or later, a build tool, Doxygen, and Python
3.9 or later, then run:

~~~sh
git clone --branch v0.3.4 --depth 1 https://github.com/xinrui-dong/market-data-toolkit-cpp.git
cd market-data-toolkit-cpp
bash scripts/verify.sh
./build/bin/market_data --input data/sample_prices.csv --window 3 --output build/analysis.csv
~~~

Synthetic prices: `[100, 110, 99, 108]`. Aligned returns: empty, `0.1`, `-0.1`,
`1/11`. Aligned SMA(3): empty, empty, `103`, `317/3`. Floating-point output may
show rounding.

Verification sequence: **configure -> build -> 32 tests -> benchmark and JSON
validation -> Doxygen and link validation**. Outputs:
`build/analysis.csv`, `build/benchmark.json`, and `build/docs/html/index.html`.

On each platform, with the default three repetitions, the benchmark has 42 workloads, 126 raw
measurement records, and 168 aggregate rows: 294 JSON entries in total. Timing
uses synthetic input. Release CI measurements and the
[local benchmark report](benchmark-results.md) are identified separately.
The [usage guide](usage.md) explains CSV validation, returns, the two SMA methods,
and how to interpret the results.

## Additional checks

- [Real financial data](market-example.md): frozen ECB EUR/USD reference rates,
  source attribution and independent Decimal checks of both CLI algorithms.
- [Platform verification](platforms.md): Linux/macOS commands, Windows PowerShell
  commands, native file replacement and platform-specific filesystem tests.
- [Algorithm walkthrough](algorithm-walkthrough.md): stack states, compensated
  summation, amortized complexity and measured small-window behavior.

On Windows, run `./scripts/verify.ps1` and use
`./build/bin/market_data.exe` for the application.
