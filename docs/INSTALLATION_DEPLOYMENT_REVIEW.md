# Pillar Engine - Installation & Deployment System Review

**Review Date:** January 12, 2026  
**Status:** Critical issues identified; refactoring recommended

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current State Analysis](#current-state-analysis)
3. [Identified Problems](#identified-problems)
4. [Recommended Improvements](#recommended-improvements)
5. [Implementation Plan](#implementation-plan)

---

## Executive Summary

The Pillar Engine's installation and deployment system has several pain points that create friction for both engine developers and game developers:

| Issue | Severity | Impact |
|-------|----------|--------|
| VS Developer Command Prompt requirement | High | Blocks non-VS workflows |
| Inconsistent directory structure | High | Confusion, path errors |
| Multiple build directory locations | Medium | Wasted disk space, confusion |
| SDK installation complexity | High | Poor DX for game developers |
| No CMakePresets.json | Medium | Verbose commands, manual setup |
| Manual environment setup | Medium | Error-prone onboarding |

**Recommendation:** Implement CMake Presets, consolidate directories, add bootstrap scripts, and create an SDK installer.

---

## Current State Analysis

### Directory Structure (Problematic)

The current structure has **three different build/output locations**:

```
Pillar/
├── build/                          # GitHub Actions uses this
│   ├── build.ninja
│   ├── _deps/                      # Dependencies cached here
│   └── ...intermediate files...
├── out/build/x64-Debug/            # CMakeSettings.json uses this  
│   ├── _deps/                      # DUPLICATE dependencies here
│   └── ...intermediate files...
├── bin/Debug-x64/                  # Final outputs go here
│   ├── Pillar/Pillar.lib
│   ├── Sandbox/SandboxApp.exe
│   ├── PillarEditor/PillarEditor.exe
│   └── Tests/PillarTests.exe
└── PillarSDK-0.1.0-Windows-AMD64/  # CPack output (unpacked)
    ├── lib/Debug/                  # SDK libraries
    ├── include/                    # SDK headers
    ├── editor/                     # Editor binary
    └── templates/                  # Project templates
```

**Problems:**
1. `build/` and `out/build/x64-Debug/` both exist with duplicated `_deps/`
2. Documentation references `out/build/x64-Debug` but CI uses `build/`
3. Final outputs are in `bin/Debug-x64/` (separate from build dir)
4. SDK output is a separate unpacked folder cluttering the repo root
5. No `.gitignore` coverage for all locations

### Build Command Inconsistency

**CMakeSettings.json (VS Integration):**
```json
{
  "buildRoot": "${projectDir}\\out\\build\\${name}",
  "generator": "Ninja"
}
```

**GitHub Actions (CI):**
```yaml
cmake -S . -B build -G "Ninja"
```

**Documentation recommends:**
```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja"
```

**Result:** Three different build directories depending on how you build.

### VS Developer Prompt Requirement

The current system requires Ninja + MSVC toolchain, but doesn't automatically detect the compiler. Users must:

1. Open "Developer PowerShell for VS 2022" (or run `vcvarsall.bat`)
2. Manually ensure `cl.exe` is in PATH
3. Know to use the correct generator

**Error symptom:** `cl.exe not found` or `MSVC not found`

### SDK Installation Issues

**Current SDK workflow:**
1. Run `cmake --install build --prefix ./sdk-install` (undocumented!)
2. Or run `cpack -G ZIP` after build
3. Manually extract ZIP to desired location
4. Set `PILLAR_SDK_DIR` environment variable OR
5. Copy template project and manually configure `CMAKE_PREFIX_PATH`

**Problems:**
- No documented "create SDK" command
- No installer (just raw files)
- Template project requires manual path configuration
- `PILLAR_SDK_DIR` env var is optional, leading to configuration errors
- SDK contains `gtest` and `gmock` libs that consumers don't need
- SDK lib structure (`lib/Debug/`, `lib/cmake/`, `lib/EnTT/`, `lib/box2d.lib`) is inconsistent

### CMakeLists.txt Issues

**Root CMakeLists.txt observations:**
1. Uses hardcoded `Debug-x64` in output paths (line 14-16):
   ```cmake
   set(BINARY_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin/Debug-x64)
   ```
2. No support for Release builds without manual changes
3. FetchContent downloads ~15 dependencies on every fresh configure
4. No dependency caching mechanism documented

---

## Identified Problems

### Problem 1: VS Developer Prompt Lock-in

**Current Reality:**
- Ninja requires a compiler in PATH
- MSVC doesn't add itself to PATH by default
- Users must launch from VS Developer Prompt

**Impact:**
- Can't build from regular PowerShell/CMD
- Can't easily integrate with other IDEs (VS Code, CLion)
- CI works because it uses a different approach

### Problem 2: Directory Chaos

**Multiple build directories:**
| Location | Created By | Contains |
|----------|------------|----------|
| `build/` | CI workflow, some manual builds | Full build tree |
| `out/build/x64-Debug/` | CMakeSettings.json, docs | Full build tree |
| `bin/Debug-x64/` | CMake output rules | Final binaries only |

**Result:** 
- ~4GB+ wasted disk space from duplicate `_deps/`
- Confusion about which is "correct"
- `.gitignore` may not cover all locations

### Problem 3: No CMakePresets.json

Modern CMake (3.19+) supports presets that:
- Define build configurations declaratively
- Work across IDEs and command line
- Eliminate verbose command-line arguments
- Support configuration inheritance

**Current commands:**
```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

**With presets:**
```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

### Problem 4: SDK Generation & Distribution

**Missing pieces:**
1. No `build-sdk.ps1` or `build-sdk.sh` script
2. No documentation on how to create the SDK
3. SDK includes test libraries users don't need
4. No version stamping or changelog in SDK
5. No installer (MSI, NSIS, or even a self-extracting archive)
6. Template project discovery is fragile

### Problem 5: Onboarding Friction

New developers must:
1. Install VS 2022 with C++ workload
2. Install Python 3
3. Install Ninja
4. Install jinja2 via pip
5. Clone repo
6. Open correct terminal type
7. Run multi-part cmake commands
8. Know output locations

**Ideal:** Single script that validates/installs prerequisites and builds.

---

## Recommended Improvements

### Improvement 1: Add CMakePresets.json

Create a `CMakePresets.json` that defines all build configurations:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "windows-debug",
      "displayName": "Windows x64 Debug",
      "inherits": "base",
      "generator": "Ninja",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      },
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      },
      "toolchainFile": "",
      "environment": {}
    },
    {
      "name": "windows-release",
      "displayName": "Windows x64 Release",
      "inherits": "windows-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "ci",
      "displayName": "CI Build",
      "inherits": "windows-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-debug"
    },
    {
      "name": "windows-release",
      "configurePreset": "windows-release"
    },
    {
      "name": "ci",
      "configurePreset": "ci"
    }
  ],
  "testPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-debug",
      "output": {"outputOnFailure": true}
    }
  ]
}
```

**Benefits:**
- Single `cmake --preset windows-debug` command
- IDE integration (VS, VS Code, CLion all read presets)
- Consistent builds across all environments
- Self-documenting build options

### Improvement 2: Consolidate Directory Structure

**Proposed structure:**
```
Pillar/
├── build/                    # ALL builds go here
│   ├── windows-debug/        # Debug build (from preset)
│   ├── windows-release/      # Release build (from preset)
│   └── sdk/                  # SDK staging area
├── bin/                      # Final outputs (unchanged)
│   ├── Debug-x64/
│   └── Release-x64/
├── scripts/                  # NEW: Build scripts
│   ├── bootstrap.ps1
│   ├── build.ps1
│   ├── build-sdk.ps1
│   └── run-tests.ps1
└── sdk/                      # NEW: SDK output location
    └── PillarSDK-x.x.x/
```

**Changes:**
1. Remove `out/` directory usage entirely
2. Update `CMakeSettings.json` to use `build/` 
3. Add scripts for common operations
4. Clean SDK output location

### Improvement 3: Bootstrap Script

Create `scripts/bootstrap.ps1` that:
1. Checks for VS 2022 installation
2. Checks for/installs Ninja (via winget)
3. Checks for Python and jinja2
4. Sets up VS environment variables
5. Runs initial CMake configure

```powershell
# scripts/bootstrap.ps1
param(
    [switch]$SkipPrerequisites,
    [string]$Preset = "windows-debug"
)

$ErrorActionPreference = "Stop"

Write-Host "=== Pillar Engine Bootstrap ===" -ForegroundColor Cyan

# Find VS installation
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Error "Visual Studio not found. Please install VS 2022 with C++ workload."
    exit 1
}

$vsPath = & $vsWhere -latest -property installationPath
$vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"

if (-not (Test-Path $vcvarsall)) {
    Write-Error "vcvarsall.bat not found. Please install C++ workload."
    exit 1
}

Write-Host "Found VS at: $vsPath" -ForegroundColor Green

# Setup MSVC environment
Write-Host "Setting up MSVC environment..." -ForegroundColor Yellow
cmd /c "`"$vcvarsall`" x64 && set" | ForEach-Object {
    if ($_ -match "^(.+?)=(.*)$") {
        [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process")
    }
}

# Check Ninja
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    Write-Host "Installing Ninja..." -ForegroundColor Yellow
    winget install Ninja-build.Ninja --silent
    $env:PATH += ";$env:LOCALAPPDATA\Microsoft\WinGet\Packages\Ninja-build.Ninja_Microsoft.Winget.Source_*"
}
Write-Host "Ninja: $(ninja --version)" -ForegroundColor Green

# Check Python
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Error "Python not found. Please install Python 3.8+."
    exit 1
}
Write-Host "Python: $(python --version)" -ForegroundColor Green

# Check jinja2
python -c "import jinja2" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Installing jinja2..." -ForegroundColor Yellow
    python -m pip install jinja2
}

# Configure CMake
Write-Host "Configuring with preset: $Preset" -ForegroundColor Yellow
cmake --preset $Preset

Write-Host "`n=== Bootstrap Complete ===" -ForegroundColor Cyan
Write-Host "Run 'cmake --build --preset $Preset' to build." -ForegroundColor White
```

### Improvement 4: SDK Build & Package Script

Create `scripts/build-sdk.ps1`:

```powershell
# scripts/build-sdk.ps1
param(
    [ValidateSet("Debug", "Release")]
    [string]$Config = "Release",
    [switch]$CreateInstaller
)

$ErrorActionPreference = "Stop"
$sdkVersion = "0.1.0"  # Read from CMakeLists.txt ideally

Write-Host "=== Building Pillar SDK $sdkVersion ===" -ForegroundColor Cyan

# Build Release
$preset = "windows-$($Config.ToLower())"
cmake --preset $preset
cmake --build --preset $preset --parallel

# Create SDK package
Write-Host "Packaging SDK..." -ForegroundColor Yellow
$buildDir = "build/$preset"
cmake --install $buildDir --prefix "sdk/PillarSDK-$sdkVersion"

# Create ZIP
Write-Host "Creating ZIP archive..." -ForegroundColor Yellow
$zipName = "PillarSDK-$sdkVersion-Windows-x64.zip"
Compress-Archive -Path "sdk/PillarSDK-$sdkVersion/*" -DestinationPath "sdk/$zipName" -Force

Write-Host "`n=== SDK Created ===" -ForegroundColor Cyan
Write-Host "Location: sdk/PillarSDK-$sdkVersion" -ForegroundColor White
Write-Host "Archive:  sdk/$zipName" -ForegroundColor White
```

### Improvement 5: Fix SDK Content

Modify root `CMakeLists.txt` to exclude test libraries from SDK:

```cmake
# Exclude GTest from SDK install
if(TARGET gtest)
  set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )
endif()
```

Organize SDK lib structure consistently:
```
sdk/PillarSDK-x.x.x/
├── include/
│   ├── Pillar/         # Engine headers
│   ├── Pillar.h        # Main include
│   └── thirdparty/     # GLM, EnTT, spdlog, etc.
├── lib/
│   ├── Debug/          # Debug libraries
│   │   ├── Pillar.lib
│   │   └── ...deps...
│   ├── Release/        # Release libraries
│   │   └── ...
│   └── cmake/          # CMake package config
│       └── Pillar/
├── editor/
│   └── PillarEditor.exe
├── templates/
│   └── EmptyProject/
└── docs/
```

### Improvement 6: Update GitHub Actions

Update `.github/workflows/build.yml` to use presets:

```yaml
- name: Configure CMake
  run: cmake --preset ci

- name: Build
  run: cmake --build --preset ci

- name: Test
  run: ctest --preset ci
```

### Improvement 7: Quick Start Commands

After implementing presets and scripts, update documentation:

**Engine Development (from source):**
```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
.\scripts\bootstrap.ps1
cmake --build --preset windows-debug
```

**Game Development (from SDK):**
```powershell
# Download and extract SDK
$env:PILLAR_SDK_DIR = "C:\PillarSDK"
Copy-Item -Recurse "$env:PILLAR_SDK_DIR\templates\EmptyProject" "MyGame"
cd MyGame
cmake --preset default
cmake --build --preset default
```

---

## Implementation Plan

### Phase 1: CMake Presets (Immediate)

**Files to create/modify:**
1. Create `CMakePresets.json` (new file)
2. Update `CMakeSettings.json` to reference presets
3. Update `.gitignore` to include all build dirs

**Effort:** 2-3 hours

### Phase 2: Directory Consolidation (Short-term)

**Files to modify:**
1. Root `CMakeLists.txt` - update output paths
2. All documentation referencing `out/build/`
3. `.github/copilot-instructions.md`

**Effort:** 1-2 hours

### Phase 3: Bootstrap & Build Scripts (Short-term)

**Files to create:**
1. `scripts/bootstrap.ps1`
2. `scripts/build.ps1`
3. `scripts/build-sdk.ps1`
4. `scripts/run-tests.ps1`

**Effort:** 3-4 hours

### Phase 4: SDK Cleanup (Medium-term)

**Changes:**
1. Exclude GTest from install targets
2. Organize lib structure
3. Add version file to SDK
4. Create template project improvements

**Effort:** 2-3 hours

### Phase 5: Documentation Update (Short-term)

**Files to update:**
1. `docs/INSTALLATION_GUIDE.md` - complete rewrite
2. `.github/copilot-instructions.md` - update build section
3. `README.md` (if exists) - update quick start

**Effort:** 2-3 hours

---

## Summary of Changes

| Change | Priority | Breaking? |
|--------|----------|-----------|
| Add CMakePresets.json | High | No |
| Add bootstrap.ps1 | High | No |
| Consolidate build dirs | Medium | Soft (docs only) |
| SDK cleanup | Medium | SDK users need update |
| Update docs | High | No |
| Update CI to use presets | Low | No |

**Total estimated effort:** 12-16 hours

---

## Appendix A: Full CMakePresets.json

```json
{
  "version": 6,
  "cmakeMinimumRequired": {
    "major": 3,
    "minor": 21,
    "patch": 0
  },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": {
        "CMAKE_EXPORT_COMPILE_COMMANDS": "ON"
      }
    },
    {
      "name": "windows-base",
      "hidden": true,
      "inherits": "base",
      "generator": "Ninja",
      "condition": {
        "type": "equals",
        "lhs": "${hostSystemName}",
        "rhs": "Windows"
      },
      "vendor": {
        "microsoft.com/VisualStudioSettings/CMake/1.0": {
          "intelliSenseMode": "windows-msvc-x64"
        }
      }
    },
    {
      "name": "windows-debug",
      "displayName": "Windows x64 Debug",
      "inherits": "windows-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    },
    {
      "name": "windows-release",
      "displayName": "Windows x64 Release",
      "inherits": "windows-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release"
      }
    },
    {
      "name": "windows-relwithdebinfo",
      "displayName": "Windows x64 RelWithDebInfo",
      "inherits": "windows-base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "RelWithDebInfo"
      }
    },
    {
      "name": "ci",
      "displayName": "CI Build (Debug)",
      "inherits": "windows-debug",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug"
      }
    }
  ],
  "buildPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-debug",
      "jobs": 0
    },
    {
      "name": "windows-release",
      "configurePreset": "windows-release",
      "jobs": 0
    },
    {
      "name": "windows-relwithdebinfo",
      "configurePreset": "windows-relwithdebinfo",
      "jobs": 0
    },
    {
      "name": "ci",
      "configurePreset": "ci",
      "jobs": 0
    }
  ],
  "testPresets": [
    {
      "name": "windows-debug",
      "configurePreset": "windows-debug",
      "output": {
        "outputOnFailure": true,
        "verbosity": "default"
      },
      "execution": {
        "noTestsAction": "error"
      }
    },
    {
      "name": "ci",
      "configurePreset": "ci",
      "output": {
        "outputOnFailure": true
      },
      "filter": {
        "exclude": {
          "name": ".*Audio.*"
        }
      }
    }
  ],
  "packagePresets": [
    {
      "name": "sdk",
      "configurePreset": "windows-release",
      "generators": ["ZIP"],
      "packageDirectory": "${sourceDir}/sdk"
    }
  ]
}
```

---

## Appendix B: Updated .gitignore Additions

```gitignore
# Build directories (consolidated)
/build/
/out/
/sdk/

# SDK output (CPack)
/PillarSDK-*/

# CMake generated
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
compile_commands.json
CPackConfig.cmake
CPackSourceConfig.cmake
*.cmake.in

# Binary outputs
/bin/

# IDE specific
.vs/
.vscode/
*.user
```

---

## Appendix C: Simplified INSTALLATION_GUIDE.md Rewrite

After implementing these changes, the installation guide can be simplified to:

```markdown
# Pillar Engine - Quick Start

## Prerequisites
- Windows 10/11 (64-bit)
- Visual Studio 2022 with "Desktop development with C++"
- Git

## Build from Source

```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
.\scripts\bootstrap.ps1
cmake --build --preset windows-debug
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

## Create a Game (SDK)

1. Download `PillarSDK-x.x.x-Windows-x64.zip` from Releases
2. Extract to `C:\PillarSDK` (or preferred location)
3. Set environment variable: `PILLAR_SDK_DIR=C:\PillarSDK`
4. Copy `templates\EmptyProject` to your project folder
5. Build:
   ```powershell
   cd MyGame
   cmake --preset default
   cmake --build --preset default
   ```
```

This is dramatically simpler than the current 200+ line guide.
