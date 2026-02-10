# Build and startup handshake verification.
# Builds the project and runs controller_app --verify-startup to confirm
# controller_app can start algo_worker and complete the handshake.
#
# Usage (run from demo/ directory):
#   .\scripts\verify_startup.ps1
#   .\scripts\verify_startup.ps1 -BuildDir build-debug
#
# Exit code: 0 if build and handshake succeed; non-zero on build failure,
# launch failure, or handshake timeout.

param(
    [string]$BuildDir = "build-debug"
)

$ErrorActionPreference = "Stop"
$DemoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$ConfigDir = Join-Path $DemoRoot "config"
$BinDir = Join-Path $DemoRoot (Join-Path $BuildDir "bin")
$BinDebug = Join-Path $DemoRoot (Join-Path $BuildDir "bin\Debug")

if (Test-Path $BinDebug) {
    $RunDir = $BinDebug
} elseif (Test-Path $BinDir) {
    $RunDir = $BinDir
} else {
    Write-Error "Build output not found. Run from demo/ and build first, or pass -BuildDir."
}

Push-Location $DemoRoot | Out-Null
try {
    Write-Host "Building: $BuildDir"
    & cmake --build $BuildDir --config Debug
    if ($LASTEXITCODE -ne 0) {
        Pop-Location | Out-Null
        exit $LASTEXITCODE
    }

    # Copy config so controller_app finds controller_app.ini next to exe
    $ControllerIni = Join-Path $ConfigDir "controller_app.ini"
    if (Test-Path $ControllerIni) {
        Copy-Item -Path $ControllerIni -Destination $RunDir -Force
    }
    $AlgoIni = Join-Path $ConfigDir "algo_worker.ini"
    if (Test-Path $AlgoIni) {
        Copy-Item -Path $AlgoIni -Destination $RunDir -Force
    }

    Write-Host "Running controller_app --verify-startup from $RunDir"
    Push-Location $RunDir | Out-Null
    try {
        & ".\controller_app.exe" --verify-startup
        $exitCode = $LASTEXITCODE
    } finally {
        Pop-Location | Out-Null
    }
    exit $exitCode
} finally {
    Pop-Location | Out-Null
}
