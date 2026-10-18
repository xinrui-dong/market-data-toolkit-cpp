param(
    [string]$BuildDirectory = (Join-Path $PSScriptRoot "../build")
)

$ErrorActionPreference = "Stop"
$projectDir = Split-Path -Parent $PSScriptRoot

function Invoke-Checked {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE"
    }
}

# CMAKE_BUILD_TYPE selects Release for single-config generators. Visual Studio
# instead selects it through --config / -C on each build and test command.
Invoke-Checked -Program cmake -Arguments @(
    "-S", $projectDir, "-B", $BuildDirectory, "-DCMAKE_BUILD_TYPE=Release"
)
Invoke-Checked -Program cmake -Arguments @(
    "--build", $BuildDirectory, "--config", "Release", "--parallel", "2"
)
Invoke-Checked -Program ctest -Arguments @(
    "--test-dir", $BuildDirectory, "-C", "Release", "--verbose",
    "--output-on-failure", "--no-tests=error"
)
Invoke-Checked -Program cmake -Arguments @(
    "--build", $BuildDirectory, "--config", "Release", "--target", "run_benchmarks"
)
Invoke-Checked -Program cmake -Arguments @(
    "--build", $BuildDirectory, "--config", "Release", "--target", "docs"
)
