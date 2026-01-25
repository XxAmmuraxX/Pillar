# Build Scripts

This directory contains PowerShell scripts for building and packaging Pillar Engine.

## Scripts

### bootstrap.ps1

**Purpose:** Automated environment setup and initial configuration

**Usage:**
```powershell
.\scripts\bootstrap.ps1 [-Preset <preset-name>] [-SkipPrerequisites]
```

**What it does:**
1. Checks for Visual Studio 2022 with C++ workload
2. Sets up MSVC environment (vcvarsall.bat)
3. Verifies CMake installation
4. Installs Ninja build system (if missing)
5. Checks Python and installs jinja2 package
6. Configures CMake with the specified preset (default: windows-debug)

**Parameters:**
- `-Preset` - CMake preset to use (default: `windows-debug`)
- `-SkipPrerequisites` - Skip all prerequisite checks and go straight to CMake configure

**Examples:**
```powershell
# Standard usage (checks everything)
.\scripts\bootstrap.ps1

# Use release configuration
.\scripts\bootstrap.ps1 -Preset windows-release

# Skip checks (when you know environment is ready)
.\scripts\bootstrap.ps1 -SkipPrerequisites
```

**When to use:**
- First time setting up the project
- After fresh clone
- When environment setup fails
- To verify all prerequisites are installed

---

### build-sdk.ps1

**Purpose:** Build and package Pillar Engine as an SDK for game developers

**Usage:**
```powershell
.\scripts\build-sdk.ps1 [-Config <Debug|Release|RelWithDebInfo>] [-SkipBuild] [-CreateZip] [-OutputDir <path>]
```

**What it does:**
1. Configures CMake with the specified configuration
2. Builds Pillar Engine
3. Installs SDK files to `sdk/PillarSDK-{version}-Windows-x64/`
4. Cleans up test libraries and temporary files
5. Creates README.md for SDK users
6. Optionally creates a ZIP archive

**Parameters:**
- `-Config` - Build configuration: Debug, Release, or RelWithDebInfo (default: Release)
- `-SkipBuild` - Skip build step (use existing build artifacts)
- `-CreateZip` - Create a ZIP archive of the SDK
- `-OutputDir` - Custom output directory (default: `sdk`)

**Examples:**
```powershell
# Standard SDK build (Release with ZIP)
.\scripts\build-sdk.ps1 -Config Release -CreateZip

# Debug SDK without ZIP
.\scripts\build-sdk.ps1 -Config Debug

# Just package existing build
.\scripts\build-sdk.ps1 -SkipBuild -CreateZip

# Custom output location
.\scripts\build-sdk.ps1 -OutputDir "C:\SDKs\Pillar" -CreateZip
```

**SDK Contents:**
```
PillarSDK-{version}-Windows-x64/
├── include/           # Engine headers + third-party headers
├── lib/               # Compiled libraries (Debug or Release)
├── assets/            # Runtime assets (shaders)
│   └── shaders/       # OpenGL shaders (BatchQuad.vert/frag, etc.)
├── editor/            # PillarEditor.exe and assets
├── templates/         # Project templates
│   └── EmptyProject/  # Starter template with README, CMakePresets, example code
├── docs/              # Documentation
├── LICENSE.txt        # License file
└── README.md          # SDK usage instructions
```

**When to use:**
- Creating a distributable SDK
- Packaging for game developers
- Testing SDK integration
- Creating releases

---

## Workflow Examples

### Standard Development Workflow

```powershell
# 1. Initial setup
.\scripts\bootstrap.ps1

# 2. Development (incremental builds)
cmake --build --preset windows-debug

# 3. Run tests
ctest --preset windows-debug

# 4. Run application
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

### Creating a Release

```powershell
# 1. Build Release configuration
.\scripts\bootstrap.ps1 -Preset windows-release
cmake --build --preset windows-release

# 2. Run tests in Release
ctest --preset windows-release

# 3. Create SDK package
.\scripts\build-sdk.ps1 -Config Release -CreateZip

# SDK is now at: sdk/PillarSDK-0.1.0-Windows-x64.zip
```

### Clean Rebuild

```powershell
# 1. Remove build directory
Remove-Item -Recurse -Force build\windows-debug

# 2. Reconfigure and build
.\scripts\bootstrap.ps1
cmake --build --preset windows-debug
```

---

## Prerequisites

All scripts require:
- **Windows 10/11** (64-bit)
- **PowerShell 5.1+** (included with Windows)
- **Visual Studio 2022** with C++ workload
- **Git** (for repository operations)

The `bootstrap.ps1` script will verify and install other requirements (CMake, Ninja, Python, jinja2).

---

## Troubleshooting

### Script execution is disabled

**Error:** `cannot be loaded because running scripts is disabled on this system`

**Solution:** Enable script execution (one-time):
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### bootstrap.ps1 fails to find Visual Studio

**Error:** `Visual Studio not found!`

**Solutions:**
1. Install Visual Studio 2022 (not 2019 or earlier)
2. Include "Desktop development with C++" workload
3. Restart terminal after installation

### build-sdk.ps1 fails with "Build directory not found"

**Error:** `Build directory not found: build/windows-release`

**Solution:** Don't use `-SkipBuild` on first run. Let it build first:
```powershell
.\scripts\build-sdk.ps1 -Config Release -CreateZip
```

### Ninja installation fails

If `winget` is not available or fails:
```powershell
# Install via Chocolatey
choco install ninja

# Or download manually from:
# https://github.com/ninja-build/ninja/releases
```

---

## Future Scripts (Planned)

- `run-tests.ps1` - Enhanced test runner with filtering and reporting
- `clean.ps1` - Clean build artifacts and caches
- `package-release.ps1` - Create GitHub release artifacts
- `setup-dev-environment.ps1` - Install all development tools

---

## Contributing

When adding new scripts:
1. Follow PowerShell best practices
2. Use `param()` blocks for parameters
3. Include `$ErrorActionPreference = "Stop"`
4. Add help comments and examples
5. Update this README

---

**Need help?** See [docs/QUICK_START.md](../docs/QUICK_START.md) for full documentation.
