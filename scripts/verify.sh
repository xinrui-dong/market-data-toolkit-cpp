#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$project_dir" -B "$project_dir/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$project_dir/build" --parallel 2
ctest --test-dir "$project_dir/build" --output-on-failure --no-tests=error
cmake --build "$project_dir/build" --target run_benchmarks
cmake --build "$project_dir/build" --target docs
