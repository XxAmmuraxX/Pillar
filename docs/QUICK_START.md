# Pillar Engine - Quick Start Guide

**Last Updated:** January 12, 2026

This guide gets you up and running with Pillar Engine in under 5 minutes.

---

## Prerequisites

Before you begin, ensure you have:

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** with "Desktop development with C++" workload
- **Git** (for cloning the repository)

That's it! The bootstrap script will handle everything else.

---

## Building from Source

### Step 1: Clone the Repository

```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
```

### Step 2: Bootstrap & Configure

```powershell
.\scripts\bootstrap.ps1
```

This automated script will:
- ✓ Verify Visual Studio 2022 installation
- ✓ Set up MSVC environment
- ✓ Install Ninja (if missing)
- ✓ Check Python and install jinja2
- ✓ Configure CMake with optimal settings

**Note:** If you see a Ninja installation prompt, accept it. The script uses `winget` to install it automatically.

### Step 3: Build

```powershell
cmake --build --preset windows-debug
```

**Build time:** 15-30 seconds (incremental), 2-3 minutes (first time)

### Step 4: Run

```powershell
# Run the Sandbox demo
.\bin\Debug-x64\Sandbox\SandboxApp.exe

# Or run the Editor
.\bin\Debug-x64\PillarEditor\PillarEditor.exe
```

**Controls in Sandbox:**
- **WASD** - Move camera
- **Q/E** - Rotate camera
- **Mouse Wheel** - Zoom in/out
- **ESC** - Exit

---

## Development Workflow

### Building

```powershell
# Debug build (default)
cmake --build --preset windows-debug

# Release build (optimized)
cmake --build --preset windows-release

# Clean build (if needed)
Remove-Item -Recurse -Force build\windows-debug
cmake --preset windows-debug
cmake --build --preset windows-debug
```

### Testing

```powershell
# Run all tests
ctest --preset windows-debug

# Run tests with detailed output
ctest --preset windows-debug --output-on-failure

# Run specific test suite
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.*
```

### Creating an SDK

To package the engine for game developers:

```powershell
.\scripts\build-sdk.ps1 -Config Release -CreateZip
```

This creates:
- **SDK folder:** `sdk/PillarSDK-0.1.0-Windows-x64/`
- **ZIP archive:** `sdk/PillarSDK-0.1.0-Windows-x64.zip`

---

## Using the SDK (Game Development)

### Option 1: Environment Variable (Recommended)

```powershell
# Set the SDK location
$env:PILLAR_SDK_DIR = "C:\path\to\sdk\PillarSDK-0.1.0-Windows-x64"
[System.Environment]::SetEnvironmentVariable("PILLAR_SDK_DIR", $env:PILLAR_SDK_DIR, "User")

# Copy a template project
Copy-Item -Recurse "$env:PILLAR_SDK_DIR\templates\EmptyProject" MyGame
cd MyGame

# Build your game
cmake --preset default
cmake --build --preset default
```

### Option 2: Manual Configuration

```powershell
# Copy template
Copy-Item -Recurse "C:\path\to\sdk\templates\EmptyProject" MyGame
cd MyGame

# Configure with explicit path
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\path\to\sdk\PillarSDK-0.1.0-Windows-x64"
cmake --build build
```

---

## Available CMake Presets

Pillar uses CMake Presets for consistent builds across environments:

| Preset | Description | Usage |
|--------|-------------|-------|
| `windows-debug` | Debug build with symbols | `cmake --preset windows-debug` |
| `windows-release` | Optimized release build | `cmake --preset windows-release` |
| `windows-relwithdebinfo` | Release with debug info | `cmake --preset windows-relwithdebinfo` |
| `ci` | CI/CD configuration | Used by GitHub Actions |

**All presets:**
- Use Ninja generator for fast parallel builds
- Build to `build/<preset-name>/` directory
- Output binaries to `bin/Debug-x64/` or `bin/Release-x64/`
- Automatically configure MSVC toolchain

---

## Troubleshooting

### "cl.exe not found" or "MSVC not found"

**Solution:** Always run from **Developer PowerShell for VS 2022** OR use `.\scripts\bootstrap.ps1` which sets up the environment automatically.

### "Python package 'jinja2' is required but not found"

**Solution:** Install jinja2:
```powershell
python -m pip install jinja2
```

Or run `.\scripts\bootstrap.ps1` which installs it automatically.

### "Ninja not found"

**Solution:** Install Ninja:
```powershell
# Via winget (recommended)
winget install Ninja-build.Ninja

# Via chocolatey
choco install ninja

# Or download from: https://github.com/ninja-build/ninja/releases
```

### Build directory exists but configure fails

**Solution:** Clean and reconfigure:
```powershell
Remove-Item -Recurse -Force build\windows-debug
.\scripts\bootstrap.ps1
```

### Tests fail with OpenGL errors

**Symptom:** Tests fail with "Failed to create OpenGL context"

**Solution:** This is expected in headless environments (CI). The CI uses Mesa3D software rendering. On your local machine, ensure you have OpenGL 4.1+ drivers installed.

---

## Next Steps

- **Read the full documentation:** [docs/INSTALLATION_GUIDE.md](INSTALLATION_GUIDE.md)
- **Explore the API:** [docs/API_REFERENCE.md](API_REFERENCE.md)
- **Learn the Editor:** [docs/PILLAR_EDITOR_GUIDE.md](PILLAR_EDITOR_GUIDE.md)
- **Understand the architecture:** [.github/copilot-instructions.md](../.github/copilot-instructions.md)

---

## Support

- **Issues:** [GitHub Issues](https://github.com/XxAmmuraxX/Pillar/issues)
- **Discussions:** [GitHub Discussions](https://github.com/XxAmmuraxX/Pillar/discussions)

---

**Happy Coding! 🚀**
