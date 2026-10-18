# Platforms and build commands

The CI matrix builds the same C++17 source with GCC on Ubuntu 24.04, Apple Clang
on macOS 26, and MSVC on Windows Server 2022. Each job runs the unit and CLI
tests, frozen ECB example, SMA benchmark, and Doxygen with local-link checks.
Inspect the exact release's successful Actions run for actual validation.
This matrix establishes coverage of those environments, not all operating
systems, filesystems or compiler versions.

## Linux and macOS

Install a C++17 compiler, CMake 3.24 or newer, a build tool, Python 3.9 or newer,
and Doxygen. The project uses floating-point `std::from_chars`; the tested
Apple Clang 21/libc++ environment supports that C++17 API. An older compiler
claiming general C++17 support may still lack the library implementation.

Run from the directory containing `CMakeLists.txt`:

~~~sh
bash scripts/verify.sh
./build/bin/market_data --input data/ecb_eurusd_2025_01.csv --window 5 --output build/ecb_analysis.csv
~~~

## Windows

Install Visual Studio 2022 Build Tools with Desktop development with C++,
CMake 3.24 or newer, Python 3.9 or newer, and Doxygen. Make Python, CMake,
and Doxygen available on `PATH`. In PowerShell at the repository root:

~~~powershell
./scripts/verify.ps1
./build/bin/market_data.exe --input data/ecb_eurusd_2025_01.csv --window 5 --output build/ecb_analysis.csv
~~~

The equivalent individual commands are:

~~~powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure --no-tests=error
cmake --build build --config Release --target run_benchmarks
cmake --build build --config Release --target docs
~~~

Visual Studio is a multi-configuration generator: `--config Release` selects
the build configuration and `-C Release` selects it for CTest. Setting only
`CMAKE_BUILD_TYPE` does not select a Visual Studio build configuration. CMake
sets `build/bin/` as the executable directory for all configurations, including
the `.exe` extension on Windows. Do not run two configurations concurrently
in the same build directory.

## Output replacement and filesystem tests

Results are written to a temporary file on the destination's filesystem. The
program completes and closes that file before replacing the requested target.
There is no delete-old-file step or copy fallback before replacement.

- Linux/macOS use filesystem rename and preserve existing POSIX permission bits.
- Windows uses `MoveFileExW` with `MOVEFILE_REPLACE_EXISTING`; Unicode filesystem
  paths reach the native API as wide strings. The Windows directory's access
  controls apply; POSIX permission bits are not treated as Windows ACLs.
- Partial writes, failed stream state, failed replacement, existing output
  preservation and temporary cleanup have tests. Windows tests a replacement
  failure by holding a destination handle that denies deletion; POSIX tests
  replace the temporary file path with a directory before the attempted rename.
- Symbolic-link rejection is tested when the environment permits creating
  symlinks. If that capability is unavailable, the test reports the reason;
  ordinary file creation/replacement and failure protection still run.

These checks address normal-process failure handling. They do not test power-loss
durability, every network filesystem, or coordination with external concurrent
writers. The verification log records the platform and actual tool versions.
