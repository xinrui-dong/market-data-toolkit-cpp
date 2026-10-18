#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-$project_dir/build}"
cmake -S "$project_dir" -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release --parallel 2
ctest --test-dir "$build_dir" -C Release --output-on-failure --no-tests=error
cmake --build "$build_dir" --config Release --target run_benchmarks
cmake --build "$build_dir" --config Release --target docs
