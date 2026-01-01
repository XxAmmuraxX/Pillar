# Pillar Engine - Installation Guide

This guide covers the complete installation process for building and running the Pillar Engine from source.

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Prerequisites](#prerequisites)
3. [Installing Dependencies](#installing-dependencies)
4. [Cloning the Repository](#cloning-the-repository)
5. [Building the Engine](#building-the-engine)
6. [Verifying Installation](#verifying-installation)
7. [Troubleshooting](#troubleshooting)

---

## System Requirements

### Minimum Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Windows 10 (64-bit) or later |
| **CPU** | x64 processor |
| **RAM** | 4 GB |
| **GPU** | OpenGL 4.1 compatible |
| **Disk Space** | 2 GB for full build |

### Recommended Requirements

| Component | Requirement |
|-----------|-------------|
| **OS** | Windows 11 (64-bit) |
| **CPU** | Intel i5 / AMD Ryzen 5 or better |
| **RAM** | 8 GB or more |
| **GPU** | OpenGL 4.6 compatible, dedicated GPU |
| **Disk Space** | 5 GB (includes dependencies cache) |

---

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community, Professional, or Enterprise)
   - Download: https://visualstudio.microsoft.com/downloads/
   - Required workloads:
     - "Desktop development with C++"
     - Include "C++ CMake tools for Windows"

2. **CMake 3.20+**
   - Download: https://cmake.org/download/
   - Or install via Visual Studio Installer
   - Verify: `cmake --version`

3. **Ninja Build System 1.10+**
   - Download: https://ninja-build.org/
   - Or install via: `winget install Ninja-build.Ninja`
   - Verify: `ninja --version`

4. **Python 3.8+**
   - Download: https://www.python.org/downloads/
   - Ensure "Add Python to PATH" is checked during installation
   - Verify: `python --version`

5. **Git**
   - Download: https://git-scm.com/downloads
   - Verify: `git --version`

---

## Installing Dependencies

### Step 1: Install Python Package (Required for GLAD2)

Open PowerShell and run:

```powershell
python -m pip install --upgrade pip
python -m pip install jinja2
```

### Step 2: Verify Python Setup

```powershell
python -c "import jinja2; print('jinja2 installed successfully')"
```

If this fails, ensure Python is in your PATH.

---

## Cloning the Repository

### Option 1: HTTPS (Recommended)

```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
```

### Option 2: SSH

```powershell
git clone git@github.com:XxAmmuraxX/Pillar.git
cd Pillar
```

---

## Building the Engine

### Step 1: Open Developer PowerShell

Open "Developer PowerShell for VS 2022" from the Start menu, or run:

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"
```

### Step 2: Navigate to Project Directory

```powershell
cd C:\path\to\Pillar
```

### Step 3: Configure CMake (First Time)

```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
```

**What this does:**
- `-S .` - Source directory is current folder
- `-B out/build/x64-Debug` - Build output directory
- `-G "Ninja"` - Use Ninja generator (faster than MSBuild)
- `-DCMAKE_BUILD_TYPE=Debug` - Debug configuration with symbols

### Step 4: Build the Project

```powershell
cmake --build out/build/x64-Debug --config Debug --parallel
```

**Expected build time:**
- First build: 2-5 minutes (downloads dependencies)
- Incremental builds: 10-30 seconds

### Step 5: Build Output Locations

After successful build:

| Binary | Location |
|--------|----------|
| Engine Library | `bin/Debug-x64/Pillar/Pillar.lib` |
| Sandbox App | `bin/Debug-x64/Sandbox/SandboxApp.exe` |
| Unit Tests | `bin/Debug-x64/Tests/PillarTests.exe` |
| Editor | `bin/Debug-x64/PillarEditor/PillarEditor.exe` |

---

## Verifying Installation

### Run the Sandbox Application

```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

**Expected behavior:**
- Window opens with rendered 2D scene
- Camera controls: WASD (move), Q/E (rotate), Mouse wheel (zoom)
- ImGui panel visible for debugging
- Press ESC or close window to exit

### Run Unit Tests

```powershell
.\bin\Debug-x64\Tests\PillarTests.exe
```

**Expected output:**
- All tests should pass (green)
- Test count: 200+ tests
- Execution time: < 10 seconds

### Run Tests with Detailed Output

```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml
```

---

## Building Release Configuration

For optimized builds:

```powershell
# Configure Release build
cmake -S . -B out/build/x64-Release -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Build Release
cmake --build out/build/x64-Release --config Release --parallel
```

Release binaries will be in `bin/Release-x64/`.

---

## Troubleshooting

### CMake Configuration Fails

**Error:** "Python package 'jinja2' is required but not found!"

**Solution:**
```powershell
python -m pip install jinja2
```

---

**Error:** "Ninja not found"

**Solution:**
```powershell
winget install Ninja-build.Ninja
# Restart PowerShell after installation
```

---

**Error:** "cl.exe not found" or "MSVC not found"

**Solution:**
Use Developer PowerShell for VS 2022, not regular PowerShell.

---

### Build Fails

**Error:** Linker errors (LNK2019, LNK2001)

**Solution:**
```powershell
# Clean and rebuild
Remove-Item -Path out/build/x64-Debug -Recurse -Force -ErrorAction SilentlyContinue
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

---

**Error:** "fatal error C1083: Cannot open include file"

**Solution:**
Ensure CMake configure step completed successfully. Re-run configuration.

---

### Runtime Errors

**Error:** Application crashes on startup

**Possible causes:**
1. Missing OpenGL drivers - Update GPU drivers
2. Incompatible GPU - Ensure OpenGL 4.1+ support

---

**Error:** Textures appear white/black

**Solution:**
Ensure assets folder exists at `Sandbox/assets/textures/` with texture files.

---

### Performance Issues

**Slow builds:**
- Use Ninja instead of MSBuild
- Enable parallel builds: `--parallel`
- Use SSD for source and build directories

**Slow application:**
- Build in Release mode for better performance
- Check GPU driver is up to date

---

## Next Steps

After successful installation:

1. Read the [User's Guide](USERS_GUIDE.md) to learn how to use the engine
2. Explore the `Sandbox` project for example code
3. Check the `Pillar/docs/` folder for API documentation
4. Run the tests to understand available features

---

## Getting Help

- **GitHub Issues:** https://github.com/XxAmmuraxX/Pillar/issues
- **Documentation:** See `docs/` folder
- **Examples:** See `Sandbox/src/` for demo implementations

