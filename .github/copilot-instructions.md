# Pillar Engine - Copilot Agent Instructions

## Repository Overview

**Pillar Engine** is a C++ game engine framework in active development, implementing a custom event system, layer-based architecture, windowing abstraction, and OpenGL rendering system. The project consists of three main components:
- **Pillar**: Core engine library (static library on Windows)
- **Sandbox**: Example application demonstrating engine usage with 2D rendering
- **Tests**: Unit tests using Google Test framework

**Tech Stack:**
- Language: C++17
- Build System: CMake 3.5+ (using Ninja generator)
- Dependencies: GLFW 3.4, spdlog 1.13.0, Dear ImGui (docking branch), GLAD2 v2.0.8 (OpenGL 4.6), GLM 1.0.1, stb_image (master)
- Testing: Google Test 1.14.0
- Platform: Windows (with cross-platform foundation via GLFW)
- Graphics API: OpenGL 4.6 (via GLAD2)
- Size: ~44 source files in Pillar (26 headers, 18 cpp) excluding vendored dependencies, plus 7 test files

## Build Instructions

### Prerequisites
- Visual Studio 2022 with C++ development tools
- CMake 3.5+ (tested with 3.31.6-msvc6)
- Ninja 1.12.1+
- Python 3.x with jinja2 package (required for GLAD2 code generation)
- Developer PowerShell for VS 2022

### Python Setup
GLAD2 requires Python with jinja2. Install it with:
```powershell
python -m pip install jinja2
```

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
- Pillar.lib → `bin/Debug-x64/Pillar/`
- SandboxApp.exe → `bin/Debug-x64/Sandbox/`
- PillarTests.exe → `bin/Debug-x64/Tests/`
- Pillar is now a static library (no DLL copying needed)

**Build time:** ~15-30 seconds for incremental, ~2-3 minutes for clean build (depends on FetchContent cache)

### Running the Application
```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```
The application opens a window displaying multiple textured and colored 2D quads, demonstrating the Renderer2D API with texture loading, animated scaling, and color tinting. The camera is controllable via:
- **WASD**: Move camera
- **Q/E**: Rotate camera
- **Mouse Wheel**: Zoom in/out
The ImGui panel "Renderer2D Test" shows camera stats and allows real-time adjustment of camera settings. Press ESC or close window to exit.

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

**Issue 1: Missing Python or jinja2**
- **Symptom:** CMake configure fails with "Python package 'jinja2' is required but not found!"
- **Fix:** Install Python 3.x and run `python -m pip install jinja2`
- **Reason:** GLAD2 uses Python code generation to create OpenGL loaders

**Issue 2: Missing forward declarations cause C2061 errors**
- **Symptom:** `error C2061: syntax error: identifier 'WindowCloseEvent'`
- **Fix:** Always include event headers when using event types in header files
- **Example:** In `Application.h`, add `#include "Pillar/Events/ApplicationEvent.h"` before using `WindowCloseEvent`

**Issue 3: EventDispatcher template parameter mismatch**
- **Symptom:** `error C2664: cannot convert argument 1 from 'std::_Binder<...>'`
- **Fix:** The `EventDispatcher::Dispatch<T>()` casts `Event&` to specific event type `T&` using `static_cast<T&>`
- **Pattern:** Event handler functions should take specific event types: `bool OnEvent(WindowCloseEvent& e)` not `Event& e`

**Issue 4: First-time build may take longer**
- FetchContent downloads dependencies (GLFW, spdlog, GLAD2, GLM, stb, ImGui, GoogleTest) on first configure
- GLAD2 generates OpenGL loader code using Python during configuration
- Subsequent builds use cached dependencies from `out/build/x64-Debug/_deps/`

**Issue 5: OpenGL shader version**
- Shaders use `#version 410 core` (OpenGL 4.1) for compatibility
- GLAD2 is configured for OpenGL 4.6 but actual version depends on system

**Issue 6: Texture loading fails**
- **Symptom:** Application runs but texture appears as white or black square
- **Fix:** Ensure texture files are in `Sandbox/assets/textures/` folder or next to executable
- **AssetManager:** Automatically searches multiple locations (development and distribution paths)

## Project Architecture

### Directory Structure
```
PILLAR_/
├── .github/
│   └── workflows/build.yml          # CI pipeline (Windows, Ninja, Debug build + tests)
├── Pillar/                          # Core engine library
│   ├── src/
│   │   ├── Pillar/                  # Public engine API
│   │   │   ├── Application.h/cpp    # Application base class with event handling
│   │   │   ├── Core.h               # PIL_API macro (empty for static lib)
│   │   │   ├── EntryPoint.h         # main() function definition
│   │   │   ├── Logger.h/cpp         # spdlog wrapper (PIL_CORE_*, PIL_*)
│   │   │   ├── LayerStack.h/cpp     # Layer management
│   │   │   ├── Layer.h/cpp          # Layer base class
│   │   │   ├── ImGuiLayer.h/cpp     # ImGui integration layer
│   │   │   ├── Input.h/cpp          # Static input polling API
│   │   │   ├── KeyCodes.h           # Key code definitions (PIL_KEY_*)
│   │   │   ├── Window.h             # Window interface
│   │   │   ├── Events/              # Event system
│   │   │   │   ├── Event.h          # Base Event class, EventDispatcher
│   │   │   │   ├── ApplicationEvent.h/cpp # Window events (Close, Resize, Focus, etc.)
│   │   │   │   ├── KeyEvent.h       # Keyboard events
│   │   │   │   └── MouseEvent.h     # Mouse events
│   │   │   ├── Utils/               # Utility classes
│   │   │   │   └── AssetManager.h/cpp # Asset path resolution system
│   │   │   └── Renderer/            # Rendering abstraction
│   │   │       ├── Renderer.h/cpp   # High-level renderer API
│   │   │       ├── Renderer2D.h/cpp # 2D batch renderer with quad drawing
│   │   │       ├── RenderCommand.h/cpp # Immediate rendering commands
│   │   │       ├── RenderAPI.h/cpp  # Rendering API abstraction (enum + base class)
│   │   │       ├── GraphicsContext.h # Graphics context interface
│   │   │       ├── Shader.h/cpp     # Shader abstraction with factory
│   │   │       ├── Buffer.h/cpp     # Vertex/Index buffer abstractions with layout system
│   │   │       ├── VertexArray.h/cpp # Vertex Array Object abstraction
│   │   │       ├── Texture.h/cpp    # Texture abstraction (2D textures)
│   │   │       ├── OrthographicCamera.h/cpp # 2D camera with view-projection matrix
│   │   │       └── OrthographicCameraController.h/cpp # Camera controller with input
│   │   ├── Platform/
│   │   │   ├── WindowsWindow.h/cpp  # GLFW-based Window implementation
│   │   │   └── OpenGL/              # OpenGL-specific implementations
│   │   │       ├── OpenGLContext.h/cpp      # OpenGL initialization
│   │   │       ├── OpenGLRenderAPI.h/cpp    # OpenGL rendering commands
│   │   │       ├── OpenGLShader.h/cpp       # OpenGL shader compilation/linking
│   │   │       ├── OpenGLBuffer.h/cpp       # OpenGL VBO/IBO
│   │   │       ├── OpenGLVertexArray.h/cpp  # OpenGL VAO with flexible layout
│   │   │       └── OpenGLTexture.h/cpp      # OpenGL 2D texture loading (stb_image)
│   │   └── Pillar.h                 # Single include header
│   └── CMakeLists.txt               # Builds Pillar as STATIC library
├── Sandbox/                         # Example application
│   ├── src/
│   │   ├── Source.cpp               # Application entry point, defines CreateApplication()
│   │   └── ExampleLayer.h           # 2D rendering demo with textures and camera
│   ├── assets/                      # Asset directory (created by build)
│   │   └── textures/                # Texture files go here
│   └── CMakeLists.txt               # Builds Sandbox as executable
├── Tests/                           # Unit tests
│   ├── src/
│   │   ├── EventTests.cpp           # Event system tests
│   │   ├── LayerTests.cpp           # Layer system tests
│   │   ├── CameraTests.cpp          # Camera tests
│   │   ├── LoggerTests.cpp          # Logger tests
│   │   ├── InputTests.cpp           # Input system tests
│   │   ├── ApplicationTests.cpp     # Application tests
│   │   └── WindowTests.cpp          # Window tests
│   ├── CMakeLists.txt               # Builds PillarTests executable
│   └── README.md                    # Testing documentation
├── CMakeLists.txt                   # Root CMake (FetchContent setup)
└── .gitignore                       # Visual Studio, CMake build artifacts
```

### Key Architectural Patterns

**1. Event System:**
- Base `Event` class with `EventType` enum and category flags
- `EventDispatcher` dispatches events to type-specific handlers using templates
- Events flow: Platform (GLFW callbacks) → Window → Application → Layers
- All events defined in `Pillar/Events/` (ApplicationEvent, KeyEvent, MouseEvent)
- Events propagate through layer stack from top to bottom until handled

**2. Layer System:**
- Layers are stacked and updated in order (bottom to top)
- Each layer can override: `OnAttach()`, `OnDetach()`, `OnUpdate(float dt)`, `OnEvent(Event& e)`, `OnImGuiRender()`
- `LayerStack` manages layer lifetime and iteration
- ImGui is integrated via `ImGuiLayer` (automatically pushed in Application)
- Example: `ExampleLayer` demonstrates 2D rendering with textures, camera control, and animations

**3. Rendering Architecture:**
- **Abstraction Pattern:** Platform-agnostic interfaces with factory methods
- **Renderer API:** Currently supports OpenGL (future: DirectX, Vulkan)
- **RenderAPI enum:** `None`, `OpenGL` - determines which implementation to use
- **Factory Pattern:** 
  - `Shader::Create()` → creates platform-specific shader
  - `VertexBuffer::Create()` → creates platform-specific VBO with layout
  - `IndexBuffer::Create()` → creates platform-specific IBO
  - `VertexArray::Create()` → creates platform-specific VAO
  - `Texture2D::Create()` → creates platform-specific texture
- **Renderer class:** Static API for high-level renderer operations
  - `Init()`, `Shutdown()`, `BeginScene()`, `EndScene()`
- **RenderCommand class:** Immediate rendering commands
  - `SetClearColor()`, `Clear()`, `SetViewport()`
  - `DrawIndexed()` - draws indexed geometry
- **Renderer2D class:** High-level 2D rendering API (new!)
  - `Init()`, `Shutdown()`, `BeginScene(camera)`, `EndScene()`
  - `DrawQuad()` - multiple overloads for colored and textured quads
  - Supports position (2D/3D), size, color, texture, and tint color
- **Buffer Layout System:** Flexible vertex attribute definition
  - `BufferLayout` - defines structure of vertex data
  - `BufferElement` - single attribute (position, color, texcoord, etc.)
  - `ShaderDataType` enum - Float, Float2, Float3, Float4, Mat3, Mat4, Int, etc.
  - OpenGL implementation automatically configures `glVertexAttribPointer` from layout
- **Texture System:**
  - `Texture2D` interface with factory method
  - OpenGL implementation uses stb_image for loading (PNG, JPG, etc.)
  - Supports binding to texture slots (0-31)
  - Provides width, height, and renderer ID

**4. Camera System (NEW):**
- **OrthographicCamera:** 2D camera with view and projection matrices
  - `SetPosition()`, `SetRotation()`
  - `GetViewProjectionMatrix()` - combined matrix for shaders
- **OrthographicCameraController:** Input-driven camera control
  - Handles WASD movement, Q/E rotation, mouse wheel zoom
  - Receives events via `OnEvent()`, updates via `OnUpdate()`
  - Configurable translation speed, rotation speed, zoom speed
  - Automatically handles window resize events

**5. Asset Management (NEW):**
- **AssetManager:** Centralized asset path resolution
  - `GetAssetPath()` - resolves generic asset paths
  - `GetTexturePath()` - specifically for textures (checks assets/textures/)
  - Searches multiple locations:
    1. `Sandbox/assets/` (for development)
    2. `assets/` next to executable (for distribution)
  - `SetAssetsDirectory()` - manual override for custom paths
  - Used by `Texture2D::Create()` automatically

**6. Input System:**
- Static polling API via `Input` class
- Platform-agnostic key codes defined in `KeyCodes.h` (e.g., `PIL_KEY_W`, `PIL_KEY_SPACE`)
- Supports keyboard and mouse input
- Methods: `IsKeyPressed()`, `IsMouseButtonPressed()`, `GetMousePosition()`
- Implementation uses GLFW state queries

**7. Static Library Pattern (UPDATED):**
- Pillar is now a static library (.lib)
- `PIL_API` macro: Empty for static library (no import/export needed)
- `PIL_STATIC_LIB` define set by CMake
- All linking is PUBLIC in Pillar, transitively available to client apps
- No DLL copying steps needed

**8. Entry Point Pattern:**
- Sandbox defines `Pillar::CreateApplication()` returning a new `Application*`
- `EntryPoint.h` defines `main()` which calls `CreateApplication()`, then `app->Run()`
- Only include `EntryPoint.h` in the final application, not in the engine library

### Configuration Files

**CMakeLists.txt (root):**
- Sets C++17 standard
- Defines `PIL_WINDOWS` on Windows
- Uses FetchContent for: spdlog, GLFW, GLAD2 (OpenGL 4.6 core profile), GLM, stb_image, ImGui, GoogleTest
- Configures output directories: `bin/Debug-x64/{Pillar,Sandbox,Tests}/`
- Custom ImGui library target (static) with GLFW and OpenGL3 backends
- GLAD2 configured using `glad_add_library()` with reproducible build
- GLM configured with tests disabled
- GoogleTest configured with `gtest_force_shared_crt` for Windows compatibility
- Python check at configure time (required for GLAD2 code generation)

**Pillar/CMakeLists.txt:**
- Builds `Pillar` as STATIC library (changed from SHARED)
- Defines `PIL_STATIC_LIB` publicly
- Source files include:
  - Core: Application, Logger, Layer, LayerStack, ImGuiLayer, Input, KeyCodes.h
  - Events: ApplicationEvent (cpp), Event.h, KeyEvent.h, MouseEvent.h
  - Utils: AssetManager
  - Renderer: RenderAPI, Renderer, Renderer2D, RenderCommand, Shader, Buffer, VertexArray, Texture, OrthographicCamera, OrthographicCameraController
  - Platform: WindowsWindow, OpenGLContext, OpenGLRenderAPI, OpenGLShader, OpenGLBuffer, OpenGLVertexArray, OpenGLTexture
- Links PUBLIC: spdlog, glfw, glad_gl_core_46, glm, imgui
- Includes PUBLIC: Pillar/src, ImGui headers, stb headers
- Opengl32 linked on Windows
- Per-configuration output directory overrides for Debug/Release/RelWithDebInfo/MinSizeRel

**Sandbox/CMakeLists.txt:**
- Builds `Sandbox` as executable (output name: `SandboxApp`)
- Links PRIVATE to Pillar (gets all dependencies transitively)
- Defines `PIL_STATIC_LIB` privately
- POST_BUILD: Creates `assets/` and `assets/textures/` directories in Sandbox source folder
- Per-configuration output directory overrides

**Tests/CMakeLists.txt:**
- Builds `PillarTests` as executable
- Uses FetchContent to fetch GoogleTest 1.14.0
- Source files: EventTests, LayerTests, CameraTests, LoggerTests, InputTests, ApplicationTests, WindowTests
- Links PRIVATE: Pillar, GTest::gtest_main, GTest::gmock
- Uses `gtest_discover_tests()` for CTest integration
- Per-configuration output directory overrides

## Continuous Integration

**GitHub Actions:** `.github/workflows/build.yml`
- Triggers: Push/PR to `master` branch
- Runner: `windows-latest`
- Steps:
  1. Checkout code
  2. Setup Python 3.14 with pip caching
  3. Install jinja2 via pip (required for GLAD2)
  4. Install Ninja
  5. Setup Mesa3D for software OpenGL rendering (allows GUI tests in CI)
  6. Configure: `cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
  7. Build: `cmake --build build --config Debug --parallel`
  8. List outputs: `dir "bin\Debug-x64" -Recurse`
  9. Run tests: `.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml`
  10. Publish test results using EnricoMi/publish-unit-test-result-action
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

### Creating Rendering Code with Renderer2D (Recommended)
1. **Include headers:**
   ```cpp
   #include "Pillar/Renderer/Renderer2D.h"
   #include "Pillar/Renderer/OrthographicCameraController.h"
   ```

2. **Initialize in OnAttach():**
   ```cpp
   m_CameraController = Pillar::OrthographicCameraController(aspectRatio, enableRotation);
   m_Texture = Pillar::Texture2D::Create("my_texture.png");
   ```

3. **Render in OnUpdate():**
   ```cpp
   m_CameraController.OnUpdate(dt);
   
   Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
   Pillar::Renderer::Clear();
   
   Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
   Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, m_Texture);
   Pillar::Renderer2D::EndScene();
   ```

4. **Handle events in OnEvent():**
   ```cpp
   m_CameraController.OnEvent(event);
   ```

### Creating Rendering Code with Low-Level API (Advanced)
1. **Initialize in OnAttach():**
   - Create `VertexArray` using `VertexArray::Create()`
   - Create `VertexBuffer` with vertex data and layout:
     ```cpp
     float vertices[] = { /* vertex data */ };
     m_VertexBuffer = VertexBuffer::Create(vertices, sizeof(vertices));
     m_VertexBuffer->SetLayout({
         { ShaderDataType::Float3, "a_Position" },
         { ShaderDataType::Float2, "a_TexCoord" }
     });
     ```
   - Create `IndexBuffer` with indices
   - Add buffers to vertex array: `m_VertexArray->AddVertexBuffer(m_VertexBuffer)`
   - Set index buffer: `m_VertexArray->SetIndexBuffer(m_IndexBuffer)`
   - Create `Shader` using `Shader::Create(vertexSrc, fragmentSrc)`
   - Load textures: `m_Texture = Texture2D::Create("path/to/texture.png")`

2. **Render in OnUpdate():**
   - Call `RenderCommand::SetClearColor()` and `RenderCommand::Clear()`
   - Call `Renderer::BeginScene(camera)`
   - Bind shader: `m_Shader->Bind()`
   - Set shader uniforms: `m_Shader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix())`
   - Bind textures: `m_Texture->Bind(slot)`
   - Submit geometry: `RenderCommand::DrawIndexed(m_VertexArray)`
   - Call `Renderer::EndScene()`

3. **Clean up in OnDetach():**
   - Delete shader, textures, buffers, and vertex array (smart pointers handle this automatically)

### Loading Textures
1. Place texture files in `Sandbox/assets/textures/` during development
2. Use `Texture2D::Create("filename.png")` - automatically searches correct paths
3. Supports PNG, JPG, BMP, TGA (via stb_image)
4. For distribution, include `assets/textures/` folder next to executable

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
   - `Platform/DirectX/DirectXVertexArray.h/cpp`
   - `Platform/DirectX/DirectXTexture.h/cpp`
3. Update factory methods in `Shader::Create()`, `Buffer::Create()`, `Texture2D::Create()`, etc.
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
- `Pillar.h` → includes Application.h, Logger.h, Input.h, Renderer headers, EntryPoint.h
- Application.h requires: `#include "Pillar/Events/ApplicationEvent.h"`
- Always include event headers before using event types in declarations
- Renderer headers should be included after core headers

**2. Event Callback Binding:**
```cpp
// Pattern used throughout:
m_Window->SetEventCallback([this](Event& e) { OnEvent(e); });
// Or using BIND_EVENT_FN macro:
m_Window->SetEventCallback(BIND_EVENT_FN(OnEvent));
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
- Use consistent naming convention: `u_Transform`, `u_Color`, `u_ViewProjection`, `u_Texture`
- Set uniforms after binding shader
- Common types: `SetMat4()`, `SetFloat4()`, `SetFloat3()`, `SetInt()`

**6. GLM Usage:**
- GLM is used for all math operations (vectors, matrices, transforms)
- Common includes: `<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`
- Functions: `glm::translate()`, `glm::rotate()`, `glm::scale()`, `glm::radians()`

**7. OpenGL Context:**
- Initialized in `WindowsWindow` via `OpenGLContext`
- Context is created and bound during window initialization
- GLAD2 loader must be called after context creation

**8. Vertex Array Layout System:**
- Use `BufferLayout` with `BufferElement` to define vertex structure
- Example:
  ```cpp
  vertexBuffer->SetLayout({
      { ShaderDataType::Float3, "a_Position" },
      { ShaderDataType::Float4, "a_Color" },
      { ShaderDataType::Float2, "a_TexCoord" }
  });
  ```
- OpenGL implementation automatically configures `glVertexAttribPointer` from layout
- Supports all common types: Float, Float2, Float3, Float4, Int, etc.

**9. Texture Loading:**
- Textures loaded via stb_image (single-header library)
- Automatically flipped vertically for OpenGL coordinate system
- Asset paths resolved via `AssetManager` - searches development and distribution paths
- Texture slot binding: `texture->Bind(slot)` where slot is 0-31

**10. Camera Matrices:**
- OrthographicCamera maintains separate view and projection matrices
- Combined into `ViewProjectionMatrix` for shaders
- Update view matrix when position/rotation changes: automatically handled internally
- Update projection matrix on resize: handled by `OrthographicCameraController::OnEvent()`

## Validation Steps

**After code changes, ALWAYS:**
1. Build: `cmake --build out/build/x64-Debug --config Debug`
2. Check for compilation errors (see Known Issues section)
3. Verify build outputs exist:
   - `Test-Path bin/Debug-x64/Pillar/Pillar.lib` should return True
   - `Test-Path bin/Debug-x64/Sandbox/SandboxApp.exe` should return True
   - `Test-Path bin/Debug-x64/Tests/PillarTests.exe` should return True
4. Run tests: `.\bin\Debug-x64\Tests\PillarTests.exe`
   - All tests should pass
   - Fix any failing tests before proceeding
5. Run application: `.\bin\Debug-x64\Sandbox\SandboxApp.exe`
   - Window should appear with textured and colored quads
   - Test camera controls: WASD movement, Q/E rotation, mouse wheel zoom
   - Test ESC to close

**Before committing:**
1. Ensure code compiles without warnings
2. All unit tests pass
3. Check GitHub Actions will pass (same build commands + tests)
4. Verify no hardcoded paths or user-specific configurations
5. Test that application runs and renders correctly
6. Update tests if you changed behavior
7. Ensure Python/jinja2 requirement is documented if modified

## Current Features & Status

**Implemented:**
- ✅ Event system (keyboard, mouse, window events)
- ✅ Layer system with update/event handling
- ✅ ImGui integration (docking branch)
- ✅ Input polling API (keyboard and mouse)
- ✅ OpenGL rendering abstraction
- ✅ Shader system with uniform support
- ✅ Vertex/Index buffer abstractions
- ✅ Vertex Array Objects (VAO) with buffer management
- ✅ Flexible vertex layout system (BufferLayout with ShaderDataType)
- ✅ 2D texture loading and binding (PNG, JPG, BMP, TGA via stb_image)
- ✅ Asset management system (AssetManager for path resolution)
- ✅ 2D rendering API (Renderer2D with quad drawing)
- ✅ Orthographic camera system with controller
- ✅ Camera controls (movement, rotation, zoom)
- ✅ Window management (GLFW)
- ✅ Logging system (spdlog)
- ✅ Unit testing framework (Google Test)
- ✅ CI/CD pipeline with automated tests
- ✅ Static library architecture (no DLL complications)




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
Periodically review these instructions for accuracy as the codebase evolves. Update sections as needed to reflect new dependencies, build steps, or architectural changes.
