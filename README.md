# Market Data Toolkit

A C++17 command-line tool for validating a historical close-price CSV, calculating adjacent simple price returns and trailing simple moving averages (SMA), and exporting aligned results. It includes two SMA algorithms, doctest tests, Google Benchmark comparisons, and Doxygen documentation.

[Repository](https://github.com/xinrui-dong/market-data-toolkit-cpp) · [Published documentation](https://xinrui-dong.github.io/market-data-toolkit-cpp/) · [GitHub Actions](https://github.com/xinrui-dong/market-data-toolkit-cpp/actions)

**Homework submission:** [one-page acceptance index](docs/acceptance-index.md) · [fixed version v0.3.4](https://github.com/xinrui-dong/market-data-toolkit-cpp/releases/tag/v0.3.4). The release records the exact submission commit and its successful CI run, with downloadable source and CI evidence. Start there when reviewing the six assignment requirements.

The tool processes one asset, quotation, and consistent price basis at a time.
Small teaching fixtures and benchmark inputs are synthetic. A separate
[real-data example](docs/market-example.md) uses 22 official ECB EUR/USD
reference rates, with original data, provenance and independent result checks.
The tool computes indicators; it does not place orders, model trading costs,
or establish a profitable strategy.

## Requirements and build

- A C++17 compiler and a build tool such as Make.
- CMake 3.24 or newer.
- Python 3.9 or newer for the real-data test and documentation checks (standard library only), and Doxygen for documentation generation.

CI checks Ubuntu/GCC, macOS/Apple Clang and Windows/MSVC. Windows users can
run `./scripts/verify.ps1` in PowerShell; Linux and macOS users can run
`bash scripts/verify.sh`. See [platform commands and tested scope](docs/platforms.md).
All configurations place application executables in `build/bin/`; Windows uses
`market_data.exe`.

doctest 2.5.3 and Google Benchmark 1.9.5 are included as official source archives. CMake verifies their SHA256 hashes and builds them locally, without downloading dependencies during configuration. See [dependency provenance and licenses](third_party/README.md).

Run from the project root:

~~~sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
./build/bin/market_data --input data/sample_prices.csv --window 3 --output build/analysis.csv
~~~

The default SMA algorithm is rolling. To select the reference implementation:

~~~sh
./build/bin/market_data --input data/sample_prices.csv --window 3 --algorithm naive
~~~

Without --output, the CSV is written to stdout. --help explains all options. Exit codes are 0 for success, 2 for invalid arguments, and 1 for input, calculation, or output errors. The program refuses to overwrite its input file.

For --output, the program writes to a temporary directory beside the destination
and replaces the requested file only after writing and closing succeed. A
reported write or replacement failure preserves an existing result. POSIX file
permission bits are retained on Linux/macOS; Windows uses native same-volume
replacement and the directory's access-control rules. Output directories and
symbolic links are rejected. See the [platform notes](docs/platforms.md) for
filesystem behavior and test coverage.

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

[Algorithm details](docs/algorithms.md) describe the formulas, alignment, numerical limitations, and benchmark method. The [user guide](docs/usage.md) explains the program with the same small example.
The [implementation walkthrough](docs/algorithm-walkthrough.md) traces every
stack operation, explains the rounding correction, and answers reviewer questions.

## Tests

~~~sh
ctest --test-dir build -C Release --output-on-failure --no-tests=error
~~~

doctest cases cover CSV and calendar rules, independent hand-computed values, invalid prices/windows, arithmetic overflow, mixed numerical scales, algorithm agreement, output alignment, and safe file replacement. Command-line tests check exact exit codes, diagnostic text, empty stdout on failure, and preservation of existing input/output files. Benchmark-validator regression tests reject incomplete or malformed measurement sets. An empty test configuration is an error.

The real-data integration test validates the frozen ECB source and normalization,
then checks both CLI algorithms against 50-digit Decimal calculations for all
22 observations, including warm-up alignment. It needs no network access.

## Fair performance comparison

~~~sh
cmake --build build --config Release --target run_benchmarks
~~~

Google Benchmark compares both SMA implementations for n = 1024, 16384, 65536 and window = 1, 2, 3, 5, 10, 20, 100: 42 workloads, each repeated three times by default. Windows 2–10 expose the cost of maintaining the rolling queue when a direct sum is short.

Set `-DCOURSE_BENCHMARK_REPETITIONS=5` during CMake configuration to collect more repetitions. The measurement command and JSON validator use the same configured count.

Both algorithms receive the same deterministic synthetic input and include validation plus result-vector allocation/destruction. CSV I/O and input generation are outside timing. Results are cross-checked before measurement. The JSON validator requires the exact workload names and the configured number of distinct, valid iteration records per workload; aggregate statistics cannot substitute for raw measurements. Benchmark errors, missing cases, duplicate repetitions, or invalid timings fail the target.

Results appear in the terminal and build/benchmark.json, including individual repetitions and aggregates. Compare matching workloads and CPU-time units. The small-window case is retained even if neither algorithm wins clearly; no fixed speedup is required on shared CI machines. See the [measured local snapshot](docs/benchmark-results.md) for results and limitations.

## Documentation and complete local verification

~~~sh
cmake --build build --config Release --target docs
bash scripts/verify.sh
~~~

Doxygen creates build/docs/html/index.html and API pages. Static menus keep member navigation usable across Doxygen versions, including 1.18.0's short-member-list case. The docs target then checks every generated HTML page's local href/src references and target HTML anchors; a missing file or anchor fails the build. The checker is read-only and does not rewrite generated pages or request external websites.

The verification scripts run configuration, build, 32 CTest entries, performance comparison, JSON checks, and documentation with link validation, stopping on failure. The documentation-checker regression test rejects missing assets and anchors, including the short-member-list failure. To check an existing HTML directory separately, run `python3 scripts/check_docs.py build/docs/html`.

**Published documentation:** [Market Data Toolkit API and project guide](https://xinrui-dong.github.io/market-data-toolkit-cpp/).

## Structure and automation

~~~text
include/course/       Price record and public API contracts
src/core.cpp          CSV validation and output
src/indicators.cpp    Returns and both SMA algorithms
src/atomic_output.cpp Safe replacement of output files
src/main.cpp          Command-line application
tests/                doctest, command-line and benchmark-validator tests
benchmarks/           Fair SMA performance comparison
data/                 Synthetic fixtures and frozen ECB data with provenance
cmake/                Benchmark JSON validation
docs/                 Algorithms, usage, and GitHub setup
third_party/          Pinned archives, hashes, and licenses
scripts/verify.sh     Complete local verification
scripts/verify.ps1    Complete Windows PowerShell verification
scripts/check_market_example.py  Independent real-data result checks
scripts/check_docs.py Local HTML file and anchor validation
.github/workflows/    Build, test, benchmark, docs, optional Pages
AGENTS.md             Actual commands and coding-agent constraints
~~~

GitHub Actions runs on pushes, pull requests, and manual runs. Each platform
saves its own benchmark JSON and HTML artifacts. Pages uses Ubuntu-generated
documentation after all three platform jobs pass. Publication is enabled in
this repository with the ENABLE_PAGES repository variable. See
[GitHub setup and the initial verification record](docs/github-setup.md).

The [first verified push run](https://github.com/xinrui-dong/market-data-toolkit-cpp/actions/runs/38650139416) completed the build, tests, benchmark, Doxygen generation, and Pages deployment for commit [d63f6d4](https://github.com/xinrui-dong/market-data-toolkit-cpp/commit/d63f6d43063192c686cc360565a17de514eeacfc) on 2026-10-14. Check the corresponding Actions run and deployment for later commits before treating them as verified.

See [extension notes](docs/extension.md) for future changes to data adapters or indicators.
