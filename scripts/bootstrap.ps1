# Pillar Engine Bootstrap Script
# Validates prerequisites, sets up environment, and configures CMake

param(
    [switch]$SkipPrerequisites,
    [string]$Preset = "windows-debug"
)

$ErrorActionPreference = "Stop"

Write-Host "`n=== Pillar Engine Bootstrap ===" -ForegroundColor Cyan
Write-Host "This script will validate prerequisites and configure the build environment.`n" -ForegroundColor White

# ============================================================================
# Prerequisite Checks
# ============================================================================

if (-not $SkipPrerequisites) {
    Write-Host "Checking prerequisites..." -ForegroundColor Yellow

    # Check for Visual Studio
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vsWhere)) {
        Write-Host "`n[ERROR] Visual Studio not found!" -ForegroundColor Red
        Write-Host "Please install Visual Studio 2022 or later with 'Desktop development with C++' workload." -ForegroundColor White
        Write-Host "Download from: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Cyan
        exit 1
    }

    # Look for VS 2022 or newer (version 17.0+)
    $vsPath = & $vsWhere -latest -property installationPath -version "[17.0,)"
    if (-not $vsPath) {
        Write-Host "`n[ERROR] Visual Studio 2022 or newer not found!" -ForegroundColor Red
        Write-Host "Please install Visual Studio 2022 or later (not 2019 or earlier)." -ForegroundColor White
        Write-Host "You may have an older version installed. This project requires VS 2022+." -ForegroundColor Yellow
        exit 1
    }

    # Get the version for display
    $vsVersion = & $vsWhere -latest -property catalog_productDisplayVersion -version "[17.0,)"
    Write-Host "[OK] Found Visual Studio $vsVersion at: $vsPath" -ForegroundColor Green

    # Check for C++ workload by looking for vcvarsall.bat
    $vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path $vcvarsall)) {
        Write-Host "`n[ERROR] C++ development tools not found!" -ForegroundColor Red
        Write-Host "Please install the 'Desktop development with C++' workload in Visual Studio." -ForegroundColor White
        Write-Host "Run Visual Studio Installer and modify your installation." -ForegroundColor Cyan
        exit 1
    }

    Write-Host "[OK] C++ development tools found" -ForegroundColor Green

    # Setup MSVC environment
    Write-Host "`nSetting up MSVC environment..." -ForegroundColor Yellow
    $tempFile = [System.IO.Path]::GetTempFileName()
    cmd /c "`"$vcvarsall`" x64 && set" > $tempFile
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match "^(.+?)=(.*)$") {
            [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
        }
    }
    Remove-Item $tempFile
    Write-Host "[OK] MSVC environment configured" -ForegroundColor Green

    # Check for CMake
    if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
        Write-Host "`n[ERROR] CMake not found!" -ForegroundColor Red
        Write-Host "CMake should be installed with Visual Studio." -ForegroundColor White
        Write-Host "If not available, download from: https://cmake.org/download/" -ForegroundColor Cyan
        exit 1
    }
    $cmakeVersion = (cmake --version | Select-Object -First 1) -replace 'cmake version ', ''
    Write-Host "[OK] CMake $cmakeVersion" -ForegroundColor Green

    # Check for Ninja
    if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
        Write-Host "`nNinja build system not found. Attempting to install..." -ForegroundColor Yellow
        
        # Try winget first
        if (Get-Command winget -ErrorAction SilentlyContinue) {
            try {
                winget install Ninja-build.Ninja --silent --accept-source-agreements --accept-package-agreements
                # Refresh PATH to include newly installed Ninja
                $env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "User")
                
                if (Get-Command ninja -ErrorAction SilentlyContinue) {
                    Write-Host "[OK] Ninja installed successfully" -ForegroundColor Green
                } else {
                    Write-Host "[WARNING] Ninja installed but not in PATH. You may need to restart your terminal." -ForegroundColor Yellow
                }
            }
            catch {
                Write-Host "[WARNING] Could not install Ninja automatically." -ForegroundColor Yellow
                Write-Host "Please install Ninja manually: https://github.com/ninja-build/ninja/releases" -ForegroundColor Cyan
            }
        }
        else {
            Write-Host "[WARNING] winget not available. Please install Ninja manually." -ForegroundColor Yellow
            Write-Host "Download from: https://github.com/ninja-build/ninja/releases" -ForegroundColor Cyan
            Write-Host "Or install via: choco install ninja" -ForegroundColor Cyan
        }
    }
    else {
        $ninjaVersion = (ninja --version)
        Write-Host "[OK] Ninja $ninjaVersion" -ForegroundColor Green
    }

    # Check for Python
    if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
        Write-Host "`n[ERROR] Python not found!" -ForegroundColor Red
        Write-Host "Python is required for GLAD2 code generation." -ForegroundColor White
        Write-Host "Install from: https://www.python.org/downloads/" -ForegroundColor Cyan
        Write-Host "Ensure 'Add Python to PATH' is checked during installation." -ForegroundColor Yellow
        exit 1
    }
    $pythonVersion = (python --version 2>&1) -replace 'Python ', ''
    Write-Host "[OK] Python $pythonVersion" -ForegroundColor Green

    # Check for jinja2
    $jinja2Check = python -c "import jinja2; print(jinja2.__version__)" 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`njinja2 package not found. Installing..." -ForegroundColor Yellow
        python -m pip install --user jinja2
        if ($LASTEXITCODE -ne 0) {
            Write-Host "[ERROR] Failed to install jinja2!" -ForegroundColor Red
            Write-Host "Try running: python -m pip install jinja2" -ForegroundColor Cyan
            exit 1
        }
        $jinja2Check = python -c "import jinja2; print(jinja2.__version__)" 2>&1
    }
    Write-Host "[OK] jinja2 $jinja2Check" -ForegroundColor Green

    Write-Host "`n[SUCCESS] All prerequisites validated!`n" -ForegroundColor Green
}
else {
    Write-Host "Skipping prerequisite checks (--SkipPrerequisites specified)`n" -ForegroundColor Yellow
}

# ============================================================================
# CMake Configuration
# ============================================================================

Write-Host "Configuring CMake with preset: $Preset" -ForegroundColor Cyan

$cmakeArgs = @("--preset", $Preset)

Write-Host "Running: cmake $($cmakeArgs -join ' ')" -ForegroundColor Gray

try {
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed with exit code $LASTEXITCODE"
    }
}
catch {
    Write-Host "`n[ERROR] CMake configuration failed!" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    Write-Host "`nTroubleshooting:" -ForegroundColor Yellow
    Write-Host "  1. Ensure you're in the Pillar root directory" -ForegroundColor White
    Write-Host "  2. Try deleting the build/ directory and running again" -ForegroundColor White
    Write-Host "  3. Check that CMakePresets.json exists in the root directory" -ForegroundColor White
    exit 1
}

# ============================================================================
# Success
# ============================================================================

Write-Host "`n=== Bootstrap Complete! ===" -ForegroundColor Cyan
Write-Host "`nNext steps:" -ForegroundColor White
Write-Host "  Build:       cmake --build --preset $Preset" -ForegroundColor Green
Write-Host "  Test:        ctest --preset $Preset" -ForegroundColor Green
Write-Host "  Run Sandbox: .\bin\Debug-x64\Sandbox\SandboxApp.exe" -ForegroundColor Green
Write-Host "  Run Editor:  .\bin\Debug-x64\PillarEditor\PillarEditor.exe" -ForegroundColor Green
Write-Host ""
