---
applyTo: '**'
---
# Pillar Engine - Copilot Agent Instructions

## Repository Overview

**Pillar Engine** is a C++ game engine framework in early development, implementing a custom event system, layer-based architecture, and windowing abstraction. The project consists of two main components:
- **Pillar**: Core engine library (shared DLL on Windows)
- **Sandbox**: Example application demonstrating engine usage

**Tech Stack:**
- Language: C++17
- Build System: CMake 3.16+ (using Ninja generator)
- Dependencies: GLFW 3.4, spdlog 1.13.0, Dear ImGui (docking branch), GLAD
- Platform: Windows (with cross-platform foundation via GLFW)
- Size: ~50 source files (excluding vendored dependencies)

## Build Instructions

### Prerequisites
- Visual Studio 2022 with C++ development tools
- CMake 4.1.0-rc1 (or 3.16+)
- Ninja 1.12.1
- Developer PowerShell for VS 2022

### Building the Project

**ALWAYS use these exact commands in order:**

1. **Configure** (first time or after CMakeLists.txt changes):
```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
```

2. **Build** (incremental builds):
```powershell
cmake --build out/build/x64-Debug --config Debug --parallel
```

3. **Full Clean Build** (when needed):
```powershell
Remove-Item -Path out/build/x64-Debug -Recurse -Force -ErrorAction SilentlyContinue
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

**Build outputs:**
- Pillar.dll ? `bin/Debug-x64/Pillar/`
- SandboxApp.exe ? `bin/Debug-x64/Sandbox/`
- Pillar.dll is auto-copied to Sandbox directory via CMake POST_BUILD command

**Build time:** ~15-30 seconds for incremental, ~2-3 minutes for clean build (depends on FetchContent cache)

### Running the Application
```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```
The application opens a window displaying "Pillar Engine". Press ESC or close window to exit.

### Known Build Issues & Workarounds

**Issue 1: Missing forward declarations cause C2061 errors**
- **Symptom:** `error C2061: syntax error: identifier 'WindowCloseEvent'`
- **Fix:** Always include event headers when using event types in header files
- **Example:** In `Application.h`, add `#include "Pillar/Events/ApplicationEvent.h"` before using `WindowCloseEvent`

**Issue 2: EventDispatcher template parameter mismatch**
- **Symptom:** `error C2664: cannot convert argument 1 from 'std::_Binder<...>'`
- **Fix:** The `EventDispatcher::Dispatch<T>()` casts `Event&` to specific event type `T&` using `static_cast<T&>`
- **Pattern:** Event handler functions should take specific event types: `bool OnEvent(WindowCloseEvent& e)` not `Event& e`

**Issue 3: First-time build may take longer**
- FetchContent downloads dependencies (GLFW, spdlog, GLAD, ImGui) on first configure
- Subsequent builds use cached dependencies from `out/build/x64-Debug/_deps/`

## Project Architecture

### Directory Structure
```
PILLAR_/
??? .github/
?   ??? workflows/build.yml          # CI pipeline (Windows, Ninja, Debug build)
??? Pillar/                          # Core engine library
?   ??? src/
?   ?   ??? Pillar/                  # Public engine API
?   ?   ?   ??? Application.h/cpp    # Application base class with event handling
?   ?   ?   ??? Core.h               # DLL export macros (PIL_API)
?   ?   ?   ??? EntryPoint.h         # main() function definition
?   ?   ?   ??? Logger.h/cpp         # spdlog wrapper (PIL_CORE_*, PIL_*)
?   ?   ?   ??? LayerStack.h/cpp     # Layer management
?   ?   ?   ??? Window.h             # Window interface
?   ?   ?   ??? Events/              # Event system
?   ?   ?       ??? Event.h          # Base Event class, EventDispatcher
?   ?   ?       ??? ApplicationEvent.h # Window events (Close, Resize, Focus, etc.)
?   ?   ?       ??? KeyEvent.h       # Keyboard events
?   ?   ?       ??? MouseEvent.h     # Mouse events
?   ?   ??? Platform/
?   ?   ?   ??? WindowsWindow.h/cpp  # GLFW-based Window implementation
?   ?   ??? Layer.h/cpp              # Layer base class
?   ?   ??? Pillar.h                 # Single include header
?   ??? CMakeLists.txt               # Builds Pillar as SHARED library
??? Sandbox/                         # Example application
?   ??? src/
?   ?   ??? Source.cpp               # Application entry point, defines CreateApplication()
?   ?   ??? ExampleLayer.h           # Sample layer implementation
?   ??? CMakeLists.txt               # Builds Sandbox as executable
??? CMakeLists.txt                   # Root CMake (FetchContent setup)
??? .gitignore                       # Visual Studio, CMake build artifacts
```

### Key Architectural Patterns

**1. Event System:**
- Base `Event` class with `EventType` enum and category flags
- `EventDispatcher` dispatches events to type-specific handlers using templates
- Events flow: Platform (GLFW callbacks) ? Window ? Application ? Layers
- All events defined in `Pillar/Events/` (ApplicationEvent, KeyEvent, MouseEvent)

**2. Layer System:**
- Layers are stacked and updated in order (bottom to top)
- Each layer can override: `OnAttach()`, `OnDetach()`, `OnUpdate(float dt)`, `OnEvent(Event& e)`, `OnImGuiRender()`
- `LayerStack` manages layer lifetime and iteration

**3. DLL Export Pattern:**
- Pillar is a shared library (DLL)
- `PIL_API` macro: `__declspec(dllexport)` when building Pillar, `__declspec(dllimport)` when using it
- Controlled by `PIL_EXPORT` define (set only in Pillar CMakeLists.txt)

**4. Entry Point Pattern:**
- Sandbox defines `Pillar::CreateApplication()` returning a new `Application*`
- `EntryPoint.h` defines `main()` which calls `CreateApplication()`, then `app->Run()`
- Only include `EntryPoint.h` in the final application, not in the engine library

### Configuration Files

**CMakeLists.txt (root):**
- Sets C++17 standard
- Defines `PIL_WINDOWS` on Windows
- Uses FetchContent for: spdlog, GLFW, GLAD, ImGui
- Configures output directories: `bin/Debug-x64/Pillar/` and `bin/Debug-x64/Sandbox/`
- Custom ImGui library target (static) with GLFW and OpenGL3 backends

**Pillar/CMakeLists.txt:**
- Builds `Pillar` as SHARED library
- Defines `PIL_EXPORT` privately
- Links: spdlog, glfw, glad, imgui, opengl32
- POST_BUILD: Copies Pillar.dll to Sandbox directory

**Sandbox/CMakeLists.txt:**
- Builds `Sandbox` as executable (output name: `SandboxApp`)
- Links to Pillar
- POST_BUILD: Copies Pillar.dll to executable directory

## Continuous Integration

**GitHub Actions:** `.github/workflows/build.yml`
- Triggers: Push/PR to `master` branch
- Runner: `windows-latest`
- Steps:
  1. Checkout code
  2. Install Ninja
  3. Configure: `cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
  4. Build: `cmake --build build --config Debug --parallel`
  5. List outputs: `dir "bin\Debug-x64" -Recurse`
- No automated tests (smoke test commented out as flaky)

## Common Development Tasks

### Adding a New Event Type
1. Define event class in appropriate header (`Pillar/Events/*.h`)
2. Inherit from `Event`, use `EVENT_CLASS_TYPE()` and `EVENT_CLASS_CATEGORY()` macros
3. Add enum value to `EventType` in `Event.h`
4. Set up GLFW callback in `WindowsWindow::Init()` if platform event
5. Handle in `Application::OnEvent()` using `EventDispatcher`

### Adding a New Layer
1. Create class inheriting from `Layer`
2. Override desired virtual methods (`OnAttach`, `OnUpdate`, etc.)
3. Push to application in `CreateApplication()`: `PushLayer(new MyLayer())`

### Modifying the Window System
- Interface: `Pillar/Window.h`
- Implementation: `Platform/WindowsWindow.h/cpp`
- GLFW callbacks set in `WindowsWindow::Init()`
- Always update callback when adding new event types

## Critical Implementation Details

**1. Include Order Matters:**
- `Pillar.h` ? includes Application.h, Logger.h, EntryPoint.h
- Application.h now requires: `#include "Pillar/Events/ApplicationEvent.h"` (as of latest fix)
- Always include event headers before using event types in declarations

**2. Event Callback Binding:**
```cpp
// Pattern used throughout:
m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
// Or using std::bind:
m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));
```

**3. Logger Macros:**
- Core (engine): `PIL_CORE_TRACE`, `PIL_CORE_INFO`, `PIL_CORE_WARN`, `PIL_CORE_ERROR`
- App (sandbox): `PIL_TRACE`, `PIL_INFO`, `PIL_WARN`, `PIL_ERROR`
- Format: `PIL_INFO("Message: {0}", value)` (uses fmt/spdlog syntax)

**4. Assertions (when enabled):**
```cpp
PIL_ASSERT(condition, "message");       // Application asserts
PIL_CORE_ASSERT(condition, "message");  // Engine asserts
```
Enable by defining `PIL_ENABLE_ASSERTS` (currently not defined)

## Validation Steps

**After code changes, ALWAYS:**
1. Build: `cmake --build out/build/x64-Debug --config Debug`
2. Check for compilation errors (see Known Issues section)
3. Verify DLL is copied: `Test-Path bin/Debug-x64/Sandbox/Pillar.dll` should return True
4. Run application: `.\bin\Debug-x64\Sandbox\SandboxApp.exe`
5. Window should appear with title "Pillar Engine"

**Before committing:**
1. Ensure code compiles without warnings
2. Check GitHub Actions will pass (same build commands)
3. Verify no hardcoded paths or user-specific configurations

## Trust These Instructions

These instructions are comprehensive and verified against the actual codebase state. Only perform additional exploration if:
- Instructions reference a file/path that doesn't exist
- Build commands fail with unexpected errors
- The information contradicts observed behavior

When in doubt about architecture or patterns, refer back to this document before exploring the codebase.

## Review and Update
Periodically review these instructions for accuracy as the codebase evolves. Update sections as needed to reflect new dependencies, build steps, or architectural changes.
