# Pillar Game Project Build Script
# Automatically sets up MSVC environment and builds with Ninja for fast compilation.
#
# Usage:
#   .\build.ps1                 # Configure + build Release
#   .\build.ps1 -Config Debug   # Configure + build Debug
#   .\build.ps1 -Clean          # Clean rebuild
#   .\build.ps1 -BuildOnly      # Skip configure, just build
#
# For zero-setup builds (slower), use Visual Studio generator instead:
#   cmake --preset default
#   cmake --build --preset default

param(
    [ValidateSet("Release", "Debug", "RelWithDebInfo")]
    [string]$Config = "Release",
    [switch]$Clean,
    [switch]$BuildOnly,
    [switch]$Help
)

if ($Help) {
    Write-Host @"
Pillar Game Build Script

USAGE:
    .\build.ps1 [options]

OPTIONS:
    -Config <Release|Debug|RelWithDebInfo>   Build configuration (default: Release)
    -Clean                                   Remove build directory before building
    -BuildOnly                               Skip CMake configure, just run ninja
    -Help                                    Show this help message

EXAMPLES:
    .\build.ps1                              # Build Release
    .\build.ps1 -Config Debug                # Build Debug
    .\build.ps1 -Config RelWithDebInfo       # Build Release with debug symbols
    .\build.ps1 -Clean                       # Clean and rebuild Release
    .\build.ps1 -BuildOnly                   # Fast rebuild (skip configure)

ALTERNATIVE (no script needed, but slower builds):
    cmake --preset default          # Uses Visual Studio generator
    cmake --build --preset default --config Release

"@
    exit 0
}

$ErrorActionPreference = "Stop"

# ============================================================================
# Check PILLAR_SDK_DIR
# ============================================================================

if (-not $env:PILLAR_SDK_DIR) {
    Write-Host "[ERROR] PILLAR_SDK_DIR environment variable is not set!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Set it to your SDK location:" -ForegroundColor Yellow
    Write-Host '  $env:PILLAR_SDK_DIR = "C:\path\to\PillarSDK-0.1.0-Windows-x64"' -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Or make it permanent:" -ForegroundColor Yellow
    Write-Host '  [System.Environment]::SetEnvironmentVariable("PILLAR_SDK_DIR", "C:\path\to\SDK", "User")' -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

Write-Host ""
Write-Host "=== Pillar Game Build ===" -ForegroundColor Cyan
Write-Host "Configuration: $Config" -ForegroundColor White
Write-Host "SDK Location:  $env:PILLAR_SDK_DIR" -ForegroundColor White
Write-Host ""

# ============================================================================
# Find and setup MSVC environment
# ============================================================================

function Find-VsWhere {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        return $vswhere
    }
    return $null
}

function Setup-MSVCEnvironment {
    # Check if cl.exe is already available
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        Write-Host "[OK] MSVC compiler found: $($cl.Source)" -ForegroundColor Green
        return $true
    }

    Write-Host "Setting up MSVC environment..." -ForegroundColor Yellow

    $vswhere = Find-VsWhere
    if (-not $vswhere) {
        Write-Host "[ERROR] Visual Studio not found. Install VS 2022 with C++ workload." -ForegroundColor Red
        Write-Host "        Or use: cmake --preset default (Visual Studio generator, no setup needed)" -ForegroundColor Gray
        return $false
    }

    # Find VS installation path
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) {
        Write-Host "[ERROR] Visual Studio C++ tools not found." -ForegroundColor Red
        Write-Host "        Install 'Desktop development with C++' workload." -ForegroundColor Gray
        return $false
    }

    # Find vcvarsall.bat
    $vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path $vcvarsall)) {
        Write-Host "[ERROR] vcvarsall.bat not found at: $vcvarsall" -ForegroundColor Red
        return $false
    }

    Write-Host "Found Visual Studio at: $vsPath" -ForegroundColor Gray

    # Run vcvarsall and capture environment
    $envOutput = cmd /c "`"$vcvarsall`" x64 >nul 2>&1 && set"
    foreach ($line in $envOutput) {
        if ($line -match "^([^=]+)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }

    # Verify cl.exe is now available
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl) {
        Write-Host "[OK] MSVC environment configured" -ForegroundColor Green
        return $true
    }

    Write-Host "[ERROR] Failed to configure MSVC environment" -ForegroundColor Red
    return $false
}

# ============================================================================
# Check for Ninja
# ============================================================================

function Check-Ninja {
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninja) {
        return $true
    }

    Write-Host "[WARNING] Ninja not found in PATH." -ForegroundColor Yellow
    Write-Host "          Install via: winget install Ninja-build.Ninja" -ForegroundColor Gray
    Write-Host "          Or use Visual Studio generator: cmake --preset default" -ForegroundColor Gray
    return $false
}

# ============================================================================
# Main build logic
# ============================================================================

Write-Host ""
Write-Host "=== Pillar Game Build ===" -ForegroundColor Cyan
Write-Host "Configuration: $Config" -ForegroundColor White
Write-Host ""

# Setup MSVC
if (-not (Setup-MSVCEnvironment)) {
    exit 1
}

# Check Ninja
if (-not (Check-Ninja)) {
    exit 1
}

# Determine preset and build directory
$preset = switch ($Config) {
    "Release"       { "default" }
    "Debug"         { "debug" }
    "RelWithDebInfo" { "relwithdebinfo" }
}
$buildDir = "build/$Config"

# Clean if requested
if ($Clean -and (Test-Path $buildDir)) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Path $buildDir -Recurse -Force
}

# Configure
if (-not $BuildOnly) {
    Write-Host ""
    Write-Host "Configuring ($preset)..." -ForegroundColor Yellow
    cmake --preset $preset
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        exit 1
    }
}

# Build
Write-Host ""
Write-Host "Building..." -ForegroundColor Yellow
cmake --build --preset $preset
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Build failed!" -ForegroundColor Red
    exit 1
}

# Success
Write-Host ""
Write-Host "=== Build Complete! ===" -ForegroundColor Green

# Find and report executable
$exeName = (Get-ChildItem -Path $buildDir -Filter "*.exe" -ErrorAction SilentlyContinue | Select-Object -First 1).Name
if ($exeName) {
    Write-Host ""
    Write-Host "Run your game:" -ForegroundColor White
    Write-Host "  .\$buildDir\$exeName" -ForegroundColor Cyan
}
Write-Host ""
