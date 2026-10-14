# Market Data Toolkit

A C++17 command-line tool for validating a historical close-price CSV, calculating adjacent simple price returns and trailing simple moving averages (SMA), and exporting aligned results. It includes two SMA algorithms, doctest tests, Google Benchmark comparisons, and Doxygen documentation.

The tool processes one asset, currency, and consistent price basis at a time. The included data is explicitly synthetic. It computes indicators; it does not place orders, model trading costs, or establish a profitable strategy.

## Requirements and build

- A C++17 compiler and a build tool such as Make.
- CMake 3.24 or newer.
- Doxygen for the documentation target.

doctest 2.5.3 and Google Benchmark 1.9.5 are included as official source archives. CMake verifies their SHA256 hashes and builds them locally, without downloading dependencies during configuration. See [dependency provenance and licenses](third_party/README.md).

Run from the project root:

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/bin/market_data --input data/sample_prices.csv --window 3 --output build/analysis.csv
~~~

The default SMA algorithm is rolling. To select the reference implementation:

~~~sh
./build/bin/market_data --input data/sample_prices.csv --window 3 --algorithm naive
~~~

Without --output, the CSV is written to stdout. --help explains all options. Exit codes are 0 for success, 2 for invalid arguments, and 1 for input, calculation, or output errors. The program refuses to overwrite its input file.

## Input contract

The restricted CSV format has exactly two columns:

~~~text
date,close
2026-10-01,100
2026-10-02,110
2026-10-03,99
2026-10-04,108
~~~

- The case-sensitive header is date,close. Surrounding spaces/tabs, CRLF, and a leading UTF-8 BOM are accepted.
- Dates are valid Gregorian YYYY-MM-DD dates, years 0001–9999, strictly increasing and unique.
- Close is a complete decimal number, optionally using scientific notation; values must be finite and strictly positive.
- At least one observation is required. Blank records, extra columns, quoted fields, missing values, numeric suffixes, invalid dates, and duplicate/unsorted dates are rejected with a line number.
- Records are never silently sorted, dropped, or filled. Date gaps can be weekends or holidays; no exchange calendar is inferred.

The positive-price contract applies to the chosen kind of series, not all financial products. For real data, document the source, asset, currency, date range, and raw/adjusted price basis. The tool does not perform split or dividend adjustments. See [sample data notes](data/README.md).

## Indicators and output alignment

The simple price return attached to record i is p[i]/p[i-1]-1. It is a fraction, so 0.10 means 10%. It spans adjacent observations, which need not be one day apart, and is not automatically a dividend-inclusive total return.

The SMA window counts observations. A valid window satisfies 1 <= window <= record count. Each SMA uses the current and previous window-1 observations. Output columns are:

~~~text
date,close,simple_return,sma
~~~

For the four sample prices, the returns are approximately 0.10, -0.10, and 0.09090909. Window 3 produces 103 and 105.66666667. The first return and first two SMA cells are empty; unknown warm-up values are not written as zero. Output preserves all input rows and uses enough significant digits to round-trip a double.

## Two SMA algorithms

- sma_naive recomputes each complete window: O((n-window+1)*window+n) total work.
- sma_rolling uses a two-stack aggregate queue with cached compensated sums: O(n) total work and O(window) extra workspace. A transfer step itself can cost O(window).

Both use compensated summation to reduce rounding loss. The queue removes old observations without subtracting a large value from the total; neither method promises exact arithmetic or a universal error bound. Nonfinite intermediate sums or results cause an error, even when an exact mathematical mean would be finite. Default compiler settings do not use fast-math.

[Algorithm details](docs/algorithms.md) describe the formulas, alignment, numerical limitations, and benchmark method. [Chinese usage notes](docs/使用说明.md) explain the program with the same small example.

## Tests

~~~sh
ctest --test-dir build --output-on-failure --no-tests=error
~~~

doctest cases cover CSV and calendar rules, independent hand-computed values, invalid prices/windows, arithmetic overflow, mixed numerical scales, algorithm agreement, and output alignment. A separate command-line test checks valid files, stdout behavior, argument errors, bad inputs, and failed output paths. An empty test configuration is an error.

## Fair performance comparison

~~~sh
cmake --build build --target run_benchmarks
~~~

Google Benchmark compares both SMA implementations for n = 1024, 16384, 65536 and window = 1, 20, 100: 18 workloads, each repeated three times.

Both algorithms receive the same deterministic synthetic input and include validation plus result-vector allocation/destruction. CSV I/O and input generation are outside timing. Results are cross-checked before measurement; benchmark errors also fail the JSON validation step.

Results appear in the terminal and build/benchmark.json, including individual repetitions and aggregates. Compare matching workloads and CPU-time units. The small-window case is retained even if neither algorithm wins clearly; no fixed speedup is required on shared CI machines. See the [measured local snapshot](docs/benchmark-results.md) for results and limitations.

## Documentation and complete local verification

~~~sh
cmake --build build --target docs
bash scripts/verify.sh
~~~

Doxygen creates build/docs/html/index.html and API pages. The verification script runs configuration, build, tests, performance comparison, JSON checks, and documentation, stopping on failure.

**Published documentation: pending the first successful GitHub Pages deployment.** After deployment, replace this sentence with the actual, verified documentation URL.

## Structure and automation

~~~text
include/course/       Price record and public API contracts
src/core.cpp          CSV validation and output
src/indicators.cpp    Returns and both SMA algorithms
src/main.cpp          Command-line application
tests/                doctest and command-line tests
benchmarks/           Fair SMA performance comparison
data/                 Synthetic fixtures and data notes
cmake/                Benchmark JSON validation
docs/                 Algorithms, usage, and GitHub setup
third_party/          Pinned archives, hashes, and licenses
scripts/verify.sh     Complete local verification
.github/workflows/    Build, test, benchmark, docs, optional Pages
AGENTS.md             Actual commands and coding-agent constraints
~~~

GitHub Actions is configured for pushes, pull requests, and manual runs. It saves benchmark JSON and HTML artifacts; Pages publication is enabled separately with the ENABLE_PAGES repository variable. See [GitHub setup](docs/github-setup.md).

Local success and workflow static validation are not remote CI or deployment evidence. A public repository, actual passing Actions run, and working Pages link still need to be established before submission.

See [extension notes](docs/extension.md) for future changes to data adapters or indicators.
