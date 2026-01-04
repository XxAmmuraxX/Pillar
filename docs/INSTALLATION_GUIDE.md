# Pillar Engine - Installation Guide

This guide covers two supported ways to get started:

1) Install the prebuilt **Pillar** ZIP from GitHub Releases
2) Build Pillar Engine from source (engine development)

## Table of Contents

1. [System Requirements](#system-requirements)
2. [Prerequisites](#prerequisites)
3. [Install Pillar (GitHub Releases)](#install-pillar-github-releases)
4. [Build From Source (Engine Development)](#build-from-source-engine-development)
5. [Troubleshooting](#troubleshooting)

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

1. **Visual Studio 2022 or later** (Community, Professional, or Enterprise)
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

4. **Python 3.8+** (only required if building Pillar from source)
   - Download: https://www.python.org/downloads/
   - Ensure "Add Python to PATH" is checked during installation
   - Verify: `python --version`

5. **Git**
   - Download: https://git-scm.com/downloads
   - Verify: `git --version`

---

## Install Pillar (GitHub Releases)

This is the recommended path if you just want to make a game with Pillar.

### Step 1: Download the ZIP

1. Go to the repository **Releases** page on GitHub.
2. Download the ZIP asset (for example: `Pillar-<version>-Windows-x64.zip`).
3. Extract it somewhere stable.

After extracting, you should see folders like:

- `include/`
- `lib/`
- `editor/`
- `templates/`
- `docs/`

### Step 2: Create a New Game Project from the Template

1. (Optional) Rename the project name inside `CMakeLists.txt`.

2. Configure and build:

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug --parallel
```
Or use Visual Studio support for CMake.

3. Run your game executable from the build output directory.

### Step 3: Launch the Editor

Run the editor executable from the SDK:

```powershell
& "<PILLAR_SDK_DIR>\editor\PillarEditor.exe"
```

---

## Build From Source (Engine Development)

Use this path if you are contributing to Pillar itself or need to rebuild the engine/editor.

### Step 1: Install Python Package (Required for GLAD2)

Open PowerShell and run:

```powershell
python -m pip install --upgrade pip
python -m pip install jinja2
```

### Step 2: Clone the Repository

HTTPS:

```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
```

### Step 3: Configure & Build

```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

Build outputs:

| Binary | Location |
|--------|----------|
| Engine Library | `bin/Debug-x64/Pillar/Pillar.lib` |
| Sandbox App | `bin/Debug-x64/Sandbox/SandboxApp.exe` |
| Unit Tests | `bin/Debug-x64/Tests/PillarTests.exe` |
| Editor | `bin/Debug-x64/PillarEditor/PillarEditor.exe` |

### Step 4: Verify

Run Sandbox:

```powershell
\.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

Run tests:

```powershell
\.\bin\Debug-x64\Tests\PillarTests.exe
```

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