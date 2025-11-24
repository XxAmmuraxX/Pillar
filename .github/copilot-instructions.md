# Pillar Engine - Copilot Agent Instructions

## Repository Overview

**Pillar Engine** is a C++ game engine framework in active development, implementing a custom event system, layer-based architecture, windowing abstraction, and OpenGL rendering system. The project consists of three main components:
- **Pillar**: Core engine library (shared DLL on Windows)
- **Sandbox**: Example application demonstrating engine usage with 2D rendering
- **Tests**: Unit tests using Google Test framework

**Tech Stack:**
- Language: C++17
- Build System: CMake 3.16+ (using Ninja generator)
- Dependencies: GLFW 3.4, spdlog 1.13.0, Dear ImGui (docking branch), GLAD (OpenGL 4.6), GLM 1.0.1
- Testing: Google Test 1.14.0
- Platform: Windows (with cross-platform foundation via GLFW)
- Graphics API: OpenGL 4.6 (via GLAD)
- Size: ~39 source files in Pillar (29 headers, 10 cpp) excluding vendored dependencies, plus 6 test files

## Build Instructions

### Prerequisites
- Visual Studio 2022 with C++ development tools
- CMake 3.16+ (tested with 3.31.6-msvc6)
- Ninja 1.12.1+
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
- PillarTests.exe ? `bin/Debug-x64/Tests/`
- Pillar.dll is auto-copied to both Sandbox and Tests directories via CMake POST_BUILD commands

**Build time:** ~15-30 seconds for incremental, ~2-3 minutes for clean build (depends on FetchContent cache)

### Running the Application
```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```
The application opens a window displaying an animated, rotating 2D square with color-changing effects. Use WASD keys to move the square. The ImGui panel "ExampleLayer Controls" allows real-time adjustment of all rendering parameters (position, rotation speed, scale, color animation). Press ESC or close window to exit.

### Running Tests
```powershell
# Run all tests
.\bin\Debug-x64\Tests\PillarTests.exe

# Run with verbose output and XML results
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml

# Run specific test suite
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.*

# List all tests
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_list_tests
```

See `Tests/README.md` for detailed testing information.

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
- FetchContent downloads dependencies (GLFW, spdlog, GLAD, GLM, ImGui, GoogleTest) on first configure
- Subsequent builds use cached dependencies from `out/build/x64-Debug/_deps/`

**Issue 4: OpenGL shader version**
- Shaders use `#version 410 core` (OpenGL 4.1) for compatibility
- GLAD is configured for OpenGL 4.6 but actual version depends on system

**Issue 5: ImGui duplicate context (RESOLVED)**
- **Symptom:** `Read access violation` in `ImGui::Begin()` with null `GImGui` pointer
- **Root Cause:** Dear ImGui was being linked into both Pillar.dll and SandboxApp.exe, creating duplicate static `GImGui` variables
- **Fix:** Changed `imgui` linkage to PRIVATE in `Pillar/CMakeLists.txt` (was PUBLIC)
- **Solution:** ImGui headers still exposed via PUBLIC include directories, but library linked only into Pillar
- **Verification:** Ensure `SandboxApp.exe` does not list `imgui*.obj` in link step

## Project Architecture

### Directory Structure
```
PILLAR_/
??? .github/
?   ??? workflows/build.yml          # CI pipeline (Windows, Ninja, Debug build + tests)
??? Pillar/                          # Core engine library
?   ??? src/
?   ?   ??? Pillar/                  # Public engine API
?   ?   ?   ??? Application.h/cpp    # Application base class with event handling
?   ?   ?   ??? Core.h               # DLL export macros (PIL_API)
?   ?   ?   ??? EntryPoint.h         # main() function definition
?   ?   ?   ??? Logger.h/cpp         # spdlog wrapper (PIL_CORE_*, PIL_*)
?   ?   ?   ??? LayerStack.h/cpp     # Layer management
?   ?   ?   ??? Layer.h/cpp          # Layer base class
?   ?   ?   ??? ImGuiLayer.h/cpp     # ImGui integration layer
?   ?   ?   ??? Input.h/cpp          # Static input polling API
?   ?   ?   ??? KeyCodes.h           # Key code definitions (PIL_KEY_*)
?   ?   ?   ??? Window.h             # Window interface
?   ?   ?   ??? Events/              # Event system
?   ?   ?   ?   ??? Event.h          # Base Event class, EventDispatcher
?   ?   ?   ?   ??? ApplicationEvent.h/cpp # Window events (Close, Resize, Focus, etc.)
?   ?   ?   ?   ??? KeyEvent.h       # Keyboard events
?   ?   ?   ?   ??? MouseEvent.h     # Mouse events
?   ?   ?   ??? Renderer/            # Rendering abstraction
?   ?   ?       ??? Renderer.h/cpp   # High-level renderer API
?   ?   ?       ??? RenderAPI.h/cpp  # Rendering API abstraction (enum + base class)
?   ?   ?       ??? GraphicsContext.h # Graphics context interface
?   ?   ?       ??? Shader.h/cpp     # Shader abstraction with factory
?   ?   ?       ??? Buffer.h/cpp     # Vertex/Index/VertexArray buffer abstractions
?   ?   ??? Platform/
?   ?   ?   ??? WindowsWindow.h/cpp  # GLFW-based Window implementation
?   ?   ?   ??? OpenGL/              # OpenGL-specific implementations
?   ?   ?       ??? OpenGLContext.h/cpp      # OpenGL initialization
?   ?   ?       ??? OpenGLRenderAPI.h/cpp    # OpenGL rendering commands
?   ?   ?       ??? OpenGLShader.h/cpp       # OpenGL shader compilation/linking
?   ?   ?       ??? OpenGLBuffer.h/cpp       # OpenGL VBO/IBO/VAO
?   ?   ??? Pillar.h                 # Single include header
?   ??? CMakeLists.txt               # Builds Pillar as SHARED library
??? Sandbox/                         # Example application
?   ??? src/
?   ?   ??? Source.cpp               # Application entry point, defines CreateApplication()
?   ?   ??? ExampleLayer.h           # 2D rendering demo with animated square
?   ??? CMakeLists.txt               # Builds Sandbox as executable
??? Tests/                           # Unit tests
?   ??? src/
?   ?   ??? EventTests.cpp           # Event system tests
?   ?   ??? LayerTests.cpp           # Layer system tests
?   ?   ??? LoggerTests.cpp          # Logger tests
?   ?   ??? InputTests.cpp           # Input system tests
?   ?   ??? ApplicationTests.cpp     # Application tests
?   ?   ??? WindowTests.cpp          # Window tests
?   ??? CMakeLists.txt               # Builds PillarTests executable
?   ??? README.md                    # Testing documentation
??? CMakeLists.txt                   # Root CMake (FetchContent setup)
??? .gitignore                       # Visual Studio, CMake build artifacts
```

### Key Architectural Patterns

**1. Event System:**
- Base `Event` class with `EventType` enum and category flags
- `EventDispatcher` dispatches events to type-specific handlers using templates
- Events flow: Platform (GLFW callbacks) ? Window ? Application ? Layers
- All events defined in `Pillar/Events/` (ApplicationEvent, KeyEvent, MouseEvent)
- Events propagate through layer stack from top to bottom until handled

**2. Layer System:**
- Layers are stacked and updated in order (bottom to top)
- Each layer can override: `OnAttach()`, `OnDetach()`, `OnUpdate(float dt)`, `OnEvent(Event& e)`, `OnImGuiRender()`
- `LayerStack` manages layer lifetime and iteration
- ImGui is integrated via `ImGuiLayer` (automatically pushed in Application)
- Example: `ExampleLayer` demonstrates 2D rendering with transform animations

**3. Rendering Architecture:**
- **Abstraction Pattern:** Platform-agnostic interfaces with factory methods
- **Renderer API:** Currently supports OpenGL (future: DirectX, Vulkan)
- **RenderAPI enum:** `None`, `OpenGL` - determines which implementation to use
- **Factory Pattern:** 
  - `Shader::Create()` ? creates platform-specific shader
  - `VertexBuffer::Create()` ? creates platform-specific VBO
  - `IndexBuffer::Create()` ? creates platform-specific IBO
  - `VertexArray::Create()` ? creates platform-specific VAO
- **Renderer class:** Static API for high-level rendering operations
  - `Init()`, `Shutdown()`, `BeginScene()`, `EndScene()`
  - `SetClearColor()`, `Clear()`, `SetViewport()`
  - `Submit(VertexArray*)` - submits geometry for rendering
- **Buffer System:** `VertexArray` manages both `VertexBuffer` and `IndexBuffer`
  - `AddVertexBuffer()` - adds vertex buffer with automatic attribute setup
  - `SetIndexBuffer()` - associates index buffer for indexed drawing
  - Currently hardcoded to 2D vertex layout (position only, 2 floats)

**4. Input System:**
- Static polling API via `Input` class
- Platform-agnostic key codes defined in `KeyCodes.h` (e.g., `PIL_KEY_W`, `PIL_KEY_SPACE`)
- Supports keyboard and mouse input
- Methods: `IsKeyPressed()`, `IsMouseButtonPressed()`, `GetMousePosition()`
- Implementation uses GLFW state queries

**5. DLL Export Pattern:**
- Pillar is a shared library (DLL)
- `PIL_API` macro: `__declspec(dllexport)` when building Pillar, `__declspec(dllimport)` when using it
- Controlled by `PIL_EXPORT` define (set only in Pillar CMakeLists.txt)

**6. Entry Point Pattern:**
- Sandbox defines `Pillar::CreateApplication()` returning a new `Application*`
- `EntryPoint.h` defines `main()` which calls `CreateApplication()`, then `app->Run()`
- Only include `EntryPoint.h` in the final application, not in the engine library

### Configuration Files

**CMakeLists.txt (root):**
- Sets C++17 standard
- Defines `PIL_WINDOWS` on Windows
- Uses FetchContent for: spdlog, GLFW, GLAD (OpenGL 4.6 core profile), GLM, ImGui, GoogleTest
- Configures output directories: `bin/Debug-x64/{Pillar,Sandbox,Tests}/`
- Custom ImGui library target (static) with GLFW and OpenGL3 backends
- GLAD configured for OpenGL 4.6 core profile
- GLM configured with tests disabled
- GoogleTest configured with `gtest_force_shared_crt` for Windows compatibility

**Pillar/CMakeLists.txt:**
- Builds `Pillar` as SHARED library
- Defines `PIL_EXPORT` privately
- Source files include:
  - Core: Application, Logger, Layer, LayerStack, ImGuiLayer, Input, KeyCodes.h
  - Events: ApplicationEvent (cpp), Event.h, KeyEvent.h, MouseEvent.h
  - Renderer: RenderAPI, Renderer, Shader, Buffer
  - Platform: WindowsWindow, OpenGLContext, OpenGLRenderAPI, OpenGLShader, OpenGLBuffer
- Links PUBLIC: spdlog, glfw, glad, glm
- Links PRIVATE: imgui (prevents duplicate Dear ImGui instances in client apps)
- Includes PUBLIC: ImGui headers (${IMGUI_DIR}, ${IMGUI_DIR}/backends) for client app access
- Opengl32 linked on Windows
- POST_BUILD: Copies Pillar.dll to Sandbox directory
- Per-configuration output directory overrides for Debug/Release/RelWithDebInfo/MinSizeRel

**Sandbox/CMakeLists.txt:**
- Builds `Sandbox` as executable (output name: `SandboxApp`)
- Links to Pillar
- POST_BUILD: Copies Pillar.dll to executable directory
- Per-configuration output directory overrides

**Tests/CMakeLists.txt:**
- Builds `PillarTests` as executable
- Uses FetchContent to fetch GoogleTest 1.14.0
- Source files: EventTests, LayerTests, LoggerTests, InputTests, ApplicationTests, WindowTests
- Links: Pillar, GTest::gtest_main, GTest::gmock
- POST_BUILD: Copies Pillar.dll to Tests directory
- Uses `gtest_discover_tests()` for CTest integration
- Per-configuration output directory overrides

## Continuous Integration

**GitHub Actions:** `.github/workflows/build.yml`
- Triggers: Push/PR to `master` branch
- Runner: `windows-latest`
- Steps:
  1. Checkout code
  2. Install Ninja
  3. Setup Mesa3D for software OpenGL rendering (allows GUI tests in CI)
  4. Configure: `cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
  5. Build: `cmake --build build --config Debug --parallel`
  6. List outputs: `dir "bin\Debug-x64" -Recurse`
  7. Run tests: `.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml`
  8. Publish test results using EnricoMi/publish-unit-test-result-action
- **Mesa3D**: Uses software OpenGL rendering (llvmpipe) to enable window/input tests without hardware GPU
- Smoke test for Sandbox is commented out (flaky in CI)

## Common Development Tasks

### Adding a New Event Type
1. Define event class in appropriate header (`Pillar/Events/*.h`)
2. Inherit from `Event`, use `EVENT_CLASS_TYPE()` and `EVENT_CLASS_CATEGORY()` macros
3. Add enum value to `EventType` in `Event.h`
4. Set up GLFW callback in `WindowsWindow::Init()` if platform event
5. Handle in `Application::OnEvent()` using `EventDispatcher`
6. Layer can override `OnEvent()` to intercept events
7. Add tests in `Tests/src/EventTests.cpp`

### Adding a New Layer
1. Create class inheriting from `Layer`
2. Override desired virtual methods (`OnAttach`, `OnUpdate`, `OnEvent`, `OnImGuiRender`)
3. Push to application in `CreateApplication()`: `PushLayer(new MyLayer())`
4. Layers receive events in reverse order (top to bottom)
5. Set `event.Handled = true` to prevent further propagation

### Creating Rendering Code
1. **Initialize in OnAttach():**
   - Create `VertexArray` using `VertexArray::Create()`
   - Create `VertexBuffer` with vertex data
   - Create `IndexBuffer` with indices
   - Add buffers to vertex array: `m_VertexArray->AddVertexBuffer(m_VertexBuffer)`
   - Set index buffer: `m_VertexArray->SetIndexBuffer(m_IndexBuffer)`
   - Create `Shader` using `Shader::Create(vertexSrc, fragmentSrc)`

2. **Render in OnUpdate():**
   - Call `Renderer::SetClearColor()` and `Renderer::Clear()`
   - Call `Renderer::BeginScene()`
   - Bind shader: `m_Shader->Bind()`
   - Set shader uniforms: `m_Shader->SetMat4()`, `m_Shader->SetFloat4()`
   - Submit geometry: `Renderer::Submit(m_VertexArray)`
   - Call `Renderer::EndScene()`

3. **Clean up in OnDetach():**
   - Delete shader, buffers, and vertex array (in that order)

### Adding Input Handling
1. In `OnUpdate()`, poll input state:
   ```cpp
   if (Pillar::Input::IsKeyPressed(PIL_KEY_W)) { /* move up */ }
   ```
2. Or handle `KeyPressedEvent` in `OnEvent()` for key press detection
3. Use key codes from `KeyCodes.h` (e.g., `PIL_KEY_A`, `PIL_KEY_ESCAPE`)
4. Mouse input: `Input::IsMouseButtonPressed()`, `Input::GetMousePosition()`

### Modifying the Window System
- Interface: `Pillar/Window.h`
- Implementation: `Platform/WindowsWindow.h/cpp`
- GLFW callbacks set in `WindowsWindow::Init()`
- Always update callback when adding new event types

### Adding a New Rendering API (e.g., DirectX)
1. Add new enum value to `RendererAPI` in `RenderAPI.h`
2. Create platform-specific implementations:
   - `Platform/DirectX/DirectXContext.h/cpp`
   - `Platform/DirectX/DirectXRenderAPI.h/cpp`
   - `Platform/DirectX/DirectXShader.h/cpp`
   - `Platform/DirectX/DirectXBuffer.h/cpp`
3. Update factory methods in `Shader::Create()`, `Buffer::Create()`, etc.
4. Set API in `RenderAPI::SetAPI()` based on platform/configuration

### Writing Unit Tests
1. Create test file in `Tests/src/` (e.g., `MyComponentTests.cpp`)
2. Include Google Test: `#include <gtest/gtest.h>`
3. Include Pillar headers to test
4. Write tests using `TEST()` macro:
   ```cpp
   TEST(MyComponentTests, TestName) {
       // Arrange
       MyComponent component;
       // Act
       component.DoSomething();
       // Assert
       EXPECT_EQ(component.GetValue(), expectedValue);
   }
   ```
5. Add file to `Tests/CMakeLists.txt` in `add_executable(PillarTests ...)`
6. Run tests locally before committing
7. See `Tests/README.md` for more testing patterns and best practices

## Critical Implementation Details

**1. Include Order Matters:**
- `Pillar.h` ? includes Application.h, Logger.h, Input.h, Renderer headers, EntryPoint.h
- Application.h requires: `#include "Pillar/Events/ApplicationEvent.h"`
- Always include event headers before using event types in declarations
- Renderer headers should be included after core headers

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

**5. Shader Uniform Naming:**
- Use consistent naming convention: `u_Transform`, `u_Color`, `u_ViewProjection`
- Set uniforms after binding shader
- Common types: `SetMat4()`, `SetFloat4()`, `SetFloat3()`, `SetInt()`

**6. GLM Usage:**
- GLM is used for all math operations (vectors, matrices, transforms)
- Common includes: `<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`
- Functions: `glm::translate()`, `glm::rotate()`, `glm::scale()`, `glm::radians()`

**7. OpenGL Context:**
- Initialized in `WindowsWindow` via `OpenGLContext`
- Context is created and bound during window initialization
- GLAD loader must be called after context creation

**8. Vertex Array Layout:**
- Currently hardcoded for 2D rendering (position only: 2 floats per vertex)
- Defined in `OpenGLVertexArray::AddVertexBuffer()`
- Future: Add flexible vertex layout system with per-attribute configuration

## Validation Steps

**After code changes, ALWAYS:**
1. Build: `cmake --build out/build/x64-Debug --config Debug`
2. Check for compilation errors (see Known Issues section)
3. Verify DLL is copied: 
   - `Test-Path bin/Debug-x64/Sandbox/Pillar.dll` should return True
   - `Test-Path bin/Debug-x64/Tests/Pillar.dll` should return True
4. Run tests: `.\bin\Debug-x64\Tests\PillarTests.exe`
   - All tests should pass
   - Fix any failing tests before proceeding
5. Run application: `.\bin\Debug-x64\Sandbox\SandboxApp.exe`
   - Window should appear with animated rotating square
   - Test WASD movement and ESC to close

**Before committing:**
1. Ensure code compiles without warnings
2. All unit tests pass
3. Check GitHub Actions will pass (same build commands + tests)
4. Verify no hardcoded paths or user-specific configurations
5. Test that application runs and renders correctly
6. Update tests if you changed behavior

## Current Features & Status

**Implemented:**
- ? Event system (keyboard, mouse, window events)
- ? Layer system with update/event handling
- ? ImGui integration (docking branch)
- ? Input polling API (keyboard and mouse)
- ? OpenGL rendering abstraction
- ? Shader system with uniform support
- ? Vertex/Index buffer abstractions
- ? Vertex Array Objects (VAO) with buffer management
- ? 2D rendering with transforms (GLM)
- ? Window management (GLFW)
- ? Logging system (spdlog)
- ? Unit testing framework (Google Test)
- ? CI/CD pipeline with automated tests

**In Development / Future:**
- ? Flexible vertex layout system (currently hardcoded for 2D)
- ? Camera system (orthographic and perspective)
- ? Texture support (2D textures with loading/binding)
- ? Batch rendering (reduce draw calls)
- ? Scene management
- ? Entity-Component System (ECS)
- ? Additional rendering APIs (DirectX, Vulkan)
- ? More comprehensive test coverage (renderer, window mocking)

## Testing Strategy

**Testing Approach:**
- Unit tests for business logic and data structures
- Integration tests for subsystem interactions
- Mock objects for GLFW/OpenGL when testing window/renderer (future)
- CI runs all tests on every commit
- Target: < 5 seconds total test execution time

**See `Tests/README.md` for detailed testing documentation.**

## Trust These Instructions

These instructions are comprehensive and verified against the actual codebase state (as of latest update). Only perform additional exploration if:
- Instructions reference a file/path that doesn't exist
- Build commands fail with unexpected errors
- The information contradicts observed behavior

When in doubt about architecture or patterns, refer back to this document before exploring the codebase.

## Review and Update
Periodically review these instructions for accuracy as the codebase evolves. Update sections as needed to reflect new dependencies, build steps, or architectural changes. Last updated: [Current Date]
