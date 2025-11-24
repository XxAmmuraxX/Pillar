# Pillar Engine

A C++ game engine framework with OpenGL rendering, event system, and ImGui integration.

## Prerequisites

Before building Pillar, ensure you have the following installed:

### Required Tools

1. **CMake 3.5 or higher**
   - Download from: https://cmake.org/download/
   - Or via Visual Studio Installer (included with C++ tools)

2. **Ninja Build System**
   - Included with Visual Studio 2022 (use "Developer PowerShell for VS 2022")
   - Or download from: https://ninja-build.org/

3. **Visual Studio 2022** (Windows)
   - Install "Desktop development with C++" workload
   - Includes MSVC compiler and Windows SDK

4. **Python 3.7 or higher**
   - Download from: https://www.python.org/downloads/
   - **Important**: Make sure to check "Add Python to PATH" during installation

5. **Python package: jinja2**
   - Required for GLAD2 OpenGL loader generation
   - Install with:
     ```powershell
     python -m pip install jinja2
     ```

## Quick Start

### 1. Clone the Repository

```powershell
git clone https://github.com/XxAmmuraxX/Pillar.git
cd Pillar
```

### 2. Install Python Dependencies

```powershell
python -m pip install jinja2
```

### 3. Configure the Project

Open **Developer PowerShell for VS 2022** and run:

```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
```

**Note**: If CMake cannot find Python or jinja2, you'll see a clear error message with installation instructions.

### 4. Build the Project

```powershell
cmake --build out/build/x64-Debug --config Debug --parallel
```

### 5. Run the Application

```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

## Build Outputs

After a successful build, you'll find:

- **Pillar Engine Library**: `bin/Debug-x64/Pillar/Pillar.lib`
- **Sandbox Application**: `bin/Debug-x64/Sandbox/SandboxApp.exe`
- **Tests**: `bin/Debug-x64/Tests/PillarTests.exe`

## Project Structure

```
Pillar/
??? Pillar/                 # Core engine library
?   ??? src/Pillar/         # Public API
?   ??? src/Platform/       # Platform-specific implementations
??? Sandbox/                # Example application
??? Tests/                  # Unit tests
??? CMakeLists.txt          # Root build configuration
```

## Dependencies

The following dependencies are automatically fetched by CMake:

- **GLFW 3.4**: Window management and input
- **GLAD2 v2.0.8**: OpenGL loader (requires Python + jinja2)
- **GLM 1.0.1**: Mathematics library
- **spdlog 1.13.0**: Logging
- **Dear ImGui** (docking branch): UI framework
- **stb_image**: Image loading
- **Google Test 1.14.0**: Testing framework

## Troubleshooting

### Error: "Python package 'jinja2' is required but not found"

**Solution**:
```powershell
python -m pip install jinja2
```

### Error: "Python 3 is required to build this project"

**Solution**: Install Python 3 from https://www.python.org/downloads/

Make sure to check "Add Python to PATH" during installation, or manually add it to your PATH.

### CMake can't find Python

**Solution**: Verify Python is in your PATH:
```powershell
python --version
```

If not found, add Python to your PATH or specify it explicitly:
```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DPython3_EXECUTABLE="C:/Python312/python.exe"
```

### Build fails with GLAD errors

**Solution**: Clean and reconfigure:
```powershell
Remove-Item -Path out/build/x64-Debug -Recurse -Force
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

## Running Tests

```powershell
.\bin\Debug-x64\Tests\PillarTests.exe
```

For detailed test output:
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml
```

## Development

### Clean Build

```powershell
Remove-Item -Path out/build/x64-Debug -Recurse -Force
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

### Incremental Build

```powershell
cmake --build out/build/x64-Debug --config Debug --parallel
```

## CI/CD

The project uses GitHub Actions for continuous integration. See `.github/workflows/build.yml` for the automated build pipeline.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Ensure all tests pass
5. Submit a pull request

## License

[Add your license here]

## Contact

- GitHub: https://github.com/XxAmmuraxX/Pillar
- Issues: https://github.com/XxAmmuraxX/Pillar/issues
