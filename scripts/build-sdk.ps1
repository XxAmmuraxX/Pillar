# Pillar SDK Build Script
# Builds the Pillar Engine and packages it as an SDK for game developers

param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$Config = "Release",
    [switch]$SkipBuild,
    [switch]$CreateZip,
    [string]$OutputDir = "sdk"
)

$ErrorActionPreference = "Stop"

# ============================================================================
# Configuration
# ============================================================================

# Read version from CMakeLists.txt
$cmakeLists = Get-Content "$PSScriptRoot\..\CMakeLists.txt" -Raw
if ($cmakeLists -match 'project\s*\(\s*Pillar\s+VERSION\s+([\d.]+)') {
    $sdkVersion = $matches[1]
}
else {
    Write-Host "[WARNING] Could not determine version from CMakeLists.txt, using 0.1.0" -ForegroundColor Yellow
    $sdkVersion = "0.1.0"
}

$sdkName = "PillarSDK-$sdkVersion-Windows-x64"
$preset = "windows-$($Config.ToLower())"
$buildDir = "build\$preset"
$installDir = "$OutputDir\$sdkName"

Write-Host "`n=== Building Pillar SDK $sdkVersion ===" -ForegroundColor Cyan
Write-Host "Configuration: $Config" -ForegroundColor White
Write-Host "Preset:        $preset" -ForegroundColor White
Write-Host "Output:        $installDir`n" -ForegroundColor White

# ============================================================================
# Build
# ============================================================================

if (-not $SkipBuild) {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    cmake --preset $preset -DPILLAR_BUILD_TESTS=OFF
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] CMake configuration failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host "`nBuilding Pillar ($Config)..." -ForegroundColor Yellow
    cmake --build --preset $preset --parallel
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[ERROR] Build failed!" -ForegroundColor Red
        exit 1
    }
    Write-Host "[OK] Build completed successfully" -ForegroundColor Green
}
else {
    Write-Host "Skipping build (--SkipBuild specified)" -ForegroundColor Yellow
    if (-not (Test-Path $buildDir)) {
        Write-Host "[ERROR] Build directory not found: $buildDir" -ForegroundColor Red
        Write-Host "Run without --SkipBuild first to build the project." -ForegroundColor White
        exit 1
    }
}

# ============================================================================
# Install/Package SDK
# ============================================================================

Write-Host "`nPackaging SDK..." -ForegroundColor Yellow

# Remove old SDK if exists
if (Test-Path $installDir) {
    Write-Host "Removing existing SDK directory..." -ForegroundColor Gray
    Remove-Item -Path $installDir -Recurse -Force
}

# Install to SDK directory
cmake --install $buildDir --prefix $installDir --config $Config
if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] SDK installation failed!" -ForegroundColor Red
    exit 1
}

Write-Host "[OK] SDK installed to: $installDir" -ForegroundColor Green

# ============================================================================
# Cleanup unwanted files
# ============================================================================

Write-Host "`nCleaning up SDK..." -ForegroundColor Yellow

# Remove test executables and libraries (they shouldn't be in SDK)
$testFiles = @(
    "$installDir\bin\PillarTests.exe",
    "$installDir\lib\gtest*.lib",
    "$installDir\lib\gmock*.lib",
    "$installDir\lib\Debug\gtest*.lib",
    "$installDir\lib\Debug\gmock*.lib",
    "$installDir\lib\Release\gtest*.lib",
    "$installDir\lib\Release\gmock*.lib"
)

foreach ($file in $testFiles) {
    if (Test-Path $file) {
        Remove-Item $file -Force
        Write-Host "  Removed: $file" -ForegroundColor Gray
    }
}

# Remove any CMake temporary files
Get-ChildItem -Path $installDir -Recurse -Include "cmake_install.cmake","CMakeCache.txt" | Remove-Item -Force

Write-Host "[OK] SDK cleanup complete" -ForegroundColor Green

# ============================================================================
# Create README for SDK
# ============================================================================

$readmeContent = @"
# Pillar Engine SDK v$sdkVersion

This SDK contains everything you need to create games with Pillar Engine.

## Contents

- **include/** - Engine headers and third-party library headers
- **lib/** - Engine libraries ($Config configuration)
- **editor/** - PillarEditor executable
- **templates/** - Project templates to get started quickly
- **docs/** - API reference and user guides

## Quick Start

### Option 1: Using Environment Variable (Recommended)

``````powershell
# Set PILLAR_SDK_DIR to this SDK location
`$env:PILLAR_SDK_DIR = "$((Get-Location).Path)\$sdkName"
[System.Environment]::SetEnvironmentVariable("PILLAR_SDK_DIR", `$env:PILLAR_SDK_DIR, "User")

# Copy template project
Copy-Item -Recurse "`$env:PILLAR_SDK_DIR\templates\EmptyProject" "MyGame"
cd MyGame

# Build your game
cmake --preset default
cmake --build --preset default
``````

### Option 2: Manual Configuration

``````powershell
# Copy template
Copy-Item -Recurse "$sdkName\templates\EmptyProject" "MyGame"
cd MyGame

# Configure with explicit SDK path
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\path\to\$sdkName"
cmake --build build
``````

## Documentation

- User Guide: docs/USERS_GUIDE.md
- API Reference: docs/API_REFERENCE.md
- Editor Guide: docs/PILLAR_EDITOR_GUIDE.md

## System Requirements

- Windows 10/11 (64-bit)
- Visual Studio 2022 or compatible C++17 compiler
- CMake 3.21+
- OpenGL 4.1+ compatible graphics card

## Support

- Documentation: See docs/ folder
- Issues: https://github.com/XxAmmuraxX/Pillar/issues
- License: See LICENSE.txt

---
Built with Pillar Engine ($Config configuration)
Generated: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
"@

Set-Content -Path "$installDir\README.md" -Value $readmeContent
Write-Host "[OK] Created SDK README.md" -ForegroundColor Green

# ============================================================================
# Create ZIP Archive
# ============================================================================

if ($CreateZip) {
    Write-Host "`nCreating ZIP archive..." -ForegroundColor Yellow
    
    $zipPath = "$OutputDir\$sdkName.zip"
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    
    # Use PowerShell's Compress-Archive
    Compress-Archive -Path "$installDir\*" -DestinationPath $zipPath -CompressionLevel Optimal
    
    if (Test-Path $zipPath) {
        $zipSize = [math]::Round((Get-Item $zipPath).Length / 1MB, 2)
        Write-Host "[OK] Created ZIP archive: $zipPath ($zipSize MB)" -ForegroundColor Green
    }
    else {
        Write-Host "[WARNING] Failed to create ZIP archive" -ForegroundColor Yellow
    }
}

# ============================================================================
# Success
# ============================================================================

Write-Host "`n=== SDK Build Complete! ===" -ForegroundColor Cyan
Write-Host "`nSDK Location:" -ForegroundColor White
Write-Host "  $installDir" -ForegroundColor Green

if ($CreateZip) {
    Write-Host "`nZIP Archive:" -ForegroundColor White
    Write-Host "  $OutputDir\$sdkName.zip" -ForegroundColor Green
}

Write-Host "`nNext Steps:" -ForegroundColor White
Write-Host "  1. Set environment variable:" -ForegroundColor Gray
Write-Host "     `$env:PILLAR_SDK_DIR = `"$(Resolve-Path $installDir)`"" -ForegroundColor Cyan
Write-Host "  2. Copy a template project:" -ForegroundColor Gray
Write-Host "     Copy-Item -Recurse `"$installDir\templates\EmptyProject`" MyGame" -ForegroundColor Cyan
Write-Host "  3. Build your game:" -ForegroundColor Gray
Write-Host "     cd MyGame && cmake --preset default && cmake --build --preset default" -ForegroundColor Cyan
Write-Host ""
