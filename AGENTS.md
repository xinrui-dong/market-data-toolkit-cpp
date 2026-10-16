# Project instructions

This C++17 project validates a single historical positive-price CSV series,
calculates adjacent simple price returns and trailing SMA, and compares naive
and rolling SMA implementations. The user selected this topic after the teacher
allowed finance-related self-selected projects.

## Structure

- `include/course/`: public interfaces and Doxygen contracts.
- `src/`: CSV I/O, numeric indicators, and the command-line application.
- `src/atomic_output.cpp`: staged output and replacement after successful close.
- `tests/`: doctest, exact CLI error checks, and benchmark-validator regression tests.
- `benchmarks/`: Google Benchmark workloads.
- `third_party/`: pinned official source archives, hashes, and licenses.
- `docs/`: usage, extension, and GitHub setup notes.
- `data/`: explicitly synthetic fixtures; never describe them as real prices.
- `cmake/`: benchmark-result validation.
- `.github/workflows/ci.yml`: build, testing, benchmark, documentation, and optional Pages deployment.

## Commands

Run from the project root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure --no-tests=error
cmake --build build --target run_benchmarks
cmake --build build --target docs
```

`bash scripts/verify.sh` runs that complete sequence. Run the application with:

```sh
./build/bin/market_data --input data/sample_prices.csv --window 3 --output build/analysis.csv
```

The optional `--algorithm naive|rolling` selects the SMA implementation (default:
rolling). Benchmark JSON is `build/benchmark.json`; generated documentation
starts at `build/docs/html/index.html`.

## Editing rules

- Use C++17 and simple, readable implementations; use four-space indentation, `snake_case` for functions and variables, and the `course` namespace.
- Keep implementation shared by the application, tests, and benchmark. Document inputs, outputs, assumptions, and important numerical limits.
- Before finishing code changes, build and run relevant tests. Run the benchmark when changing measured code, and regenerate documentation when changing an interface or its documentation.
- Report actual commands and results. Preserve meaningful failures; do not invent test results, timings, remote CI runs, or published URLs.
- Do not modify CI/CD or dependency versions unless the user's task authorizes those changes. Dependency changes must update hashes, provenance, and licenses together.
- Preserve user edits. Do not change the selected project scope, create a remote repository, publish a website, or submit homework unless asked.
- Keep CSV validation strict and diagnostics actionable. Do not silently sort,
  drop, or fill records. Preserve empty warm-up cells in the output.
- Preserve existing output files when writing or replacement fails. Test both
  successful replacement and failures after partial temporary output.
- Benchmark validation must verify exact workload names and every expected
  raw repetition; aggregate rows cannot substitute for measurement records.
- Validate both SMA implementations against independent examples and numeric
  stress cases. Do not require universal bitwise equality or claim fixed speedups.
- Keep the same input, validation, output allocation, and build options when
  benchmarking the two algorithms. A benchmark error must fail verification.
- Do not commit generated build output, credentials, classroom PDFs, or private data.
- Explain nontrivial changes in terms the student can reproduce. Keep comments concise and accurate.
