 # Pillar Engine - Project Status Document

**Last Updated:** January 2025  
**Version:** Pre-Alpha 0.3  
**Branch:** `feature/render_api/2`  
**Repository:** https://github.com/XxAmmuraxX/Pillar  
**Build Type:** **STATIC LIBRARY** (converted from DLL)

---

## Executive Summary

Pillar Engine is a C++17 game engine framework in **active early development**, focusing on building a solid foundation for 2D game development with future 3D capabilities. The project has successfully implemented core engine architecture including event systems, layer management, OpenGL rendering abstraction, 2D rendering with texture support, and **camera controller system**.

**Current Development Phase:** 2D Rendering & Scene Management  
**Primary Focus:** Camera system complete, moving to scene management  
**Next Milestone:** Scene manager and entity system

**Recent Major Achievement:** ? **Converted to static linking** - eliminated all DLL boundary issues

---

## Project Architecture Overview

### Technology Stack

| Component | Technology | Version |
|-----------|-----------|---------|
| **Language** | C++ | C++17 |
| **Build System** | CMake + Ninja | 3.31.6 (min 3.16) |
| **Linking** | **Static Library** | .lib (was .dll) |
| **Window/Input** | GLFW | 3.4 |
| **Graphics API** | OpenGL (via GLAD) | 4.6 core (shaders use 4.1 for compatibility) |
| **Math Library** | GLM | 1.0.1 |
| **Logging** | spdlog | 1.13.0 |
| **UI/Debug** | Dear ImGui | docking branch |
| **Image Loading** | stb_image | latest |
| **Testing** | Google Test | 1.14.0 |
| **CI/CD** | GitHub Actions | Windows + Mesa3D |
| **Platform** | Windows | Primary (cross-platform foundation) |

### Project Structure

```
Pillar/                     # Core engine STATIC LIBRARY
??? src/
?   ??? Pillar/            # Public API
?   ?   ??? Core.h         # Platform macros (PIL_API = empty for static)
?   ?   ??? Application.h/cpp
?   ?   ??? Logger.h/cpp
?   ?   ??? Layer.h/cpp
?   ?   ??? LayerStack.h/cpp
?   ?   ??? ImGuiLayer.h/cpp
?   ?   ??? Input.h/cpp
?   ?   ??? KeyCodes.h
?   ?   ??? Events/        # Event system
?   ?   ??? Renderer/      # Rendering abstraction
?   ?   ?   ??? OrthographicCamera.h/cpp
?   ?   ?   ??? OrthographicCameraController.h/cpp  ? NEW
?   ?   ?   ??? ... other renderer files
?   ?   ??? Utils/         # Utilities (AssetManager)
?   ??? Platform/          # Platform-specific implementations
?       ??? WindowsWindow.h/cpp
?       ??? OpenGL/        # OpenGL implementations
??? CMakeLists.txt

Sandbox/                    # Example application EXE (static linked)
??? src/
?   ??? Source.cpp         # Entry point
?   ??? ExampleLayer.h     # Demo layer with camera controller
??? assets/
?   ??? textures/          # Texture assets
??? CMakeLists.txt

Tests/                      # Unit tests EXE (static linked)
??? src/
?   ??? EventTests.cpp
?   ??? LayerTests.cpp
?   ??? LoggerTests.cpp
?   ??? InputTests.cpp
?   ??? ApplicationTests.cpp
?   ??? WindowTests.cpp
?   ??? CameraTests.cpp    ? NEW
??? README.md
??? CMakeLists.txt
```

---

## Implementation Status

### ? **COMPLETED FEATURES**

#### Core Engine Systems
- **Application Framework**
  - `Application` base class with singleton pattern
  - Main game loop with delta time calculation
  - Window management integration
  - Event callback routing
  - Layer stack management
  - Entry point pattern for client applications

- **Event System** (Fully Functional)
  - Base `Event` class with type and category system
  - `EventDispatcher` with template-based type dispatch
  - Window events: Close, Resize, Focus, LostFocus, Moved
  - Keyboard events: KeyPressed, KeyReleased (with repeat count)
  - Mouse events: ButtonPressed, ButtonReleased, Moved, Scrolled
  - Event propagation through layer stack (top to bottom)
  - Event blocking capability (`event.Handled` flag)
  - GLFW callback integration

- **Layer System**
  - `Layer` base class with virtual lifecycle hooks
  - `LayerStack` management (layers + overlays)
  - Lifecycle methods: `OnAttach()`, `OnDetach()`, `OnUpdate(dt)`, `OnEvent()`, `OnImGuiRender()`
  - Proper iteration order (layers bottom-to-top, overlays always on top)
  - Automatic cleanup on destruction

- **Logging System**
  - spdlog wrapper with colored console output
  - Core engine logger (`PIL_CORE_*` macros)
  - Application logger (`PIL_*` macros)
  - Log levels: TRACE, INFO, WARN, ERROR
  - Format string support (fmt/spdlog syntax)

- **Input System**
  - Static polling API via `Input` class
  - Platform-agnostic key codes (`PIL_KEY_*` defines)
  - Keyboard state polling: `IsKeyPressed()`
  - Mouse state polling: `IsMouseButtonPressed()`, `GetMousePosition()`
  - GLFW implementation

- **Window Management**
  - Abstract `Window` interface
  - `WindowsWindow` implementation (GLFW-based)
  - Properties: title, width, height, VSync
  - Event callback system
  - Native window handle access
  - OpenGL context initialization

#### Rendering Systems
- **Rendering Architecture** (Platform-Agnostic)
  - `RenderAPI` abstraction layer (enum: None, OpenGL)
  - `RenderCommand` static API for render operations
  - `Renderer` high-level API with scene management
  - Factory pattern for shader/buffer creation
  - `GraphicsContext` interface for API initialization

- **OpenGL Renderer** (Primary Implementation)
  - `OpenGLContext` - Context creation and initialization
  - `OpenGLRenderAPI` - Draw commands, clear, viewport
  - `OpenGLShader` - Shader compilation, linking, uniform binding
  - `OpenGLBuffer` - Vertex/Index buffer implementations
  - `OpenGLVertexArray` - VAO management with attribute setup
  - `OpenGLTexture` - Texture loading (stb_image), binding, sampling
  - Depth testing support
  - Blending support (alpha transparency)

- **Shader System**
  - Abstract `Shader` base class
  - Factory method: `Shader::Create(vertexSrc, fragmentSrc)`
  - Uniform support: `SetInt()`, `SetFloat4()`, `SetMat4()`
  - OpenGL implementation with error checking
  - GLSL version 410 core (compatibility)

- **Buffer System**
  - `VertexBuffer` abstraction (dynamic allocation)
  - `IndexBuffer` abstraction (element count tracking)
  - `VertexArray` abstraction (VAO wrapper)
  - Factory methods for platform-specific creation
  - Automatic attribute pointer setup (currently 2D: position + texCoords)

- **Renderer2D System** ? (Recently Completed)
  - High-level 2D rendering API
  - `DrawQuad()` overloads:
    - Position (vec2/vec3), size, color
    - Position (vec2/vec3), size, texture, optional tint
  - Quad vertex/index data generation
  - Texture coordinate mapping (0,0 to 1,1)
  - Per-quad rendering (submit on each draw call)
  - Integration with OrthographicCamera
  - Scene management: `BeginScene()`, `EndScene()`

- **Texture System** ?
  - `Texture` and `Texture2D` abstractions
  - Factory method: `Texture2D::Create(path)` and `Create(width, height)`
  - OpenGL implementation with stb_image loading
  - Support for PNG, JPG, BMP formats
  - Texture binding to slots (0-31)
  - Width/height accessors
  - Smart pointer management (`std::shared_ptr`)

- **Camera System** (Basic)
  - `OrthographicCamera` class
  - Projection matrix (left, right, bottom, top)
  - View matrix with position and rotation
  - Combined view-projection matrix
  - Getters/setters for position, rotation
  - Recalculation on transform change

- **Asset Management** ?
  - `AssetManager` utility class
  - Path resolution: searches `Sandbox/assets/` (dev) and `assets/` (dist)
  - Texture path resolver: `GetTexturePath()`
  - Generic asset path resolver: `GetAssetPath()`
  - Executable directory detection
  - Configurable assets directory override

#### Developer Tools
- **ImGui Integration**
  - `ImGuiLayer` as engine overlay
  - Dear ImGui docking branch support
  - Docking enabled, viewports enabled
  - GLFW + OpenGL3 backends
  - Frame management: `Begin()`, `End()`, `NewFrame()`
  - Context sharing across DLL boundary (current workaround: `GetImGuiContext()`)
  - Menu bar, dockspace setup
  - ImGui demo window included

- **Testing Infrastructure**
  - Google Test framework integration
  - 6 test files covering:
    - Event system (creation, dispatch, handlers)
    - Layer system (attach, detach, stack management)
    - Logger (initialization, output)
    - Input system (keyboard, mouse polling)
    - Application (creation, singleton, events)
    - Window (creation, properties, callbacks)
  - CTest integration (`gtest_discover_tests`)
  - CI/CD with GitHub Actions
  - Mesa3D software rendering for GUI tests in CI

- **Build System**
  - CMake 3.16+ with Ninja generator
  - FetchContent for all dependencies (no manual setup)
  - Output directories: `bin/Debug-x64/{Pillar,Sandbox,Tests}/`
  - Automatic DLL copying to executable directories
  - Per-configuration output overrides
  - Clean build support
  - Parallel compilation

---

### ?? **IN PROGRESS / PARTIALLY IMPLEMENTED**

#### Rendering Issues
- **ImGui DLL Boundary Issue** ??
  - **Problem:** ImGui symbols not exported from Pillar.dll
  - **Impact:** Sandbox layers calling ImGui functions get linker errors
  - **Current Workaround:** `ImGui::SetCurrentContext()` in `ExampleLayer::OnImGuiRender()`
  - **Proposed Fix:** Change ImGui from `PRIVATE` to `PUBLIC` linkage in `Pillar/CMakeLists.txt`
  - **Status:** Fix identified, needs implementation and testing

- **Vertex Layout System**
  - **Current State:** Hardcoded for 2D position (2 floats) + texCoords (2 floats)
  - **Location:** `OpenGLVertexArray::AddVertexBuffer()`
  - **Limitation:** Cannot support normals, colors, tangents, etc.
  - **Needed:** Flexible `VertexBufferLayout` system with attribute descriptors

#### Performance Optimizations
- **Batch Rendering** (Not Implemented)
  - **Current:** Each `DrawQuad()` submits immediately (high draw call count)
  - **Needed:** 
    - Quad batching buffer (collect vertices/indices)
    - Flush on texture change or buffer full
    - Single draw call per batch
    - Target: 10,000+ quads per frame
  
- **Texture Atlasing** (Not Implemented)
  - **Current:** One texture per quad (texture binding overhead)
  - **Needed:** Pack multiple sprites into texture atlas, use sub-UVs

#### Camera System Gaps
- **OrthographicCameraController** (Missing)
  - Manual camera control in `ExampleLayer` (WASD + Q/E rotation)
  - Need dedicated controller class with:
    - Input handling
    - Zoom support (mouse wheel)
    - Bounds limiting
    - Smooth movement/rotation
  
- **Perspective Camera** (Not Implemented)
  - For future 3D rendering
  - FOV, aspect ratio, near/far planes

---

### ? **NOT STARTED / PLANNED FEATURES**

#### Core Systems (High Priority)
- **Scene Management**
  - `Scene` class (entity container)
  - Scene serialization (save/load)
  - Scene switching/transitions
  
- **Entity-Component System (ECS)**
  - Consider EnTT library integration
  - Components: Transform, SpriteRenderer, RigidBody, etc.
  - Systems: RenderSystem, PhysicsSystem, etc.
  - Entity lifecycle management

- **Shader Library**
  - `ShaderLibrary` class (name-based lookup)
  - Pre-compiled shader cache
  - Hot-reloading for development

- **Material System**
  - `Material` class (shader + uniforms + textures)
  - Material instances
  - Property blocks for per-instance data

#### Rendering Features
- **Particle System**
  - Emitter configurations
  - GPU-accelerated particles
  - Texture animation support

- **Sprite Animation**
  - Sprite sheet support
  - Frame-based animation
  - Animation state machine

- **Post-Processing**
  - Framebuffer abstraction
  - Post-process effect stack
  - Built-in effects: bloom, blur, color grading

- **Additional Rendering APIs**
  - DirectX 11 (Windows fallback)
  - Vulkan (modern, high-performance)
  - Metal (macOS/iOS)

#### Physics & Collision
- **2D Physics Integration**
  - Box2D library integration
  - RigidBody component
  - Collider components (Box, Circle, Polygon)
  - Physics debug rendering

#### Audio System
- **Audio Engine**
  - OpenAL or miniaudio integration
  - 2D/3D audio sources
  - Audio listener management
  - Sound effects + music streaming

#### Editor & Tools
- **Pillar Editor Application**
  - Visual scene editor
  - Entity inspector/hierarchy
  - Asset browser
  - Play/stop simulation
  - Save/load projects

- **Scripting Support**
  - Lua or C# scripting
  - Script hot-reloading
  - Entity behavior scripting
  - API exposure

#### Content Pipeline
- **Advanced Asset Management**
  - Asset database
  - Asset importing (FBX, OBJ, etc.)
  - Asset preprocessing
  - Async asset loading
  - Hot-reloading

---

## Current Development Focus

### Active Branch: `feature/render_api/2`

**Recent Accomplishments:**
1. ? Implemented `Renderer2D` with quad rendering
2. ? Integrated texture system with `OpenGLTexture` and stb_image
3. ? Added `OrthographicCamera` with manual controls
4. ? Created `AssetManager` for path resolution
5. ? Updated `ExampleLayer` to demonstrate Renderer2D capabilities
6. ? Fixed ImGui duplicate context issue (PRIVATE linkage)
7. ? Added comprehensive ImGui controls to ExampleLayer

**Known Issues:**
1. ? ImGui linker errors in Sandbox (DLL export issue) - **FIXED**
2. ?? Vertex layout hardcoded to 2D position + texCoords
3. ?? No batch rendering (performance bottleneck for many quads)

**Immediate Next Steps (This Week):**
1. ~~Fix ImGui linking~~ ? **COMPLETE** - Changed to PRIVATE linkage
2. ~~Add ImGui controls to ExampleLayer~~ ? **COMPLETE** - Full parameter panel
3. **Implement `OrthographicCameraController`** - Dedicated input handling class (2-3 hours)
4. **Basic batch rendering** - Collect quads, submit in batches (4-6 hours)
5. **Add tests for new features** - Camera controller, Renderer2D tests (1-2 hours)

---

## Build & Test Status

### Build Configuration
```powershell
# Configure
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build out/build/x64-Debug --config Debug --parallel

# Clean build
Remove-Item -Path out/build/x64-Debug -Recurse -Force
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

**Build Times:**
- Incremental: ~15-30 seconds
- Clean build: ~2-3 minutes (depends on FetchContent cache)

### Test Status
**Test Executable:** `.\bin\Debug-x64\Tests\PillarTests.exe`

| Test Suite | Status | Test Count | Notes |
|------------|--------|-----------|-------|
| EventTests | ? Passing | ~12 tests | Event creation, dispatch, all event types |
| LayerTests | ? Passing | ~8 tests | Layer lifecycle, stack management |
| LoggerTests | ? Passing | ~3 tests | Initialization, output validation |
| InputTests | ? Passing | ~6 tests | Keyboard/mouse polling, GLFW integration |
| ApplicationTests | ? Passing | ~5 tests | Singleton, window creation, events |
| WindowTests | ? Passing | ~7 tests | Window properties, callbacks, GLFW |
| CameraTests | ? Passing | ~5 tests | Camera creation, movement, properties |

**Total Tests:** ~46 tests (all passing)  
**Execution Time:** < 5 seconds

### CI/CD Status
**GitHub Actions:** `.github/workflows/build.yml`
- ? Build on `windows-latest`
- ? Mesa3D software OpenGL rendering (for GUI tests)
- ? All tests pass in CI
- ? Test results published to PR checks
- ?? Sandbox smoke test commented out (flaky in CI)

---

## Application Demo

### Sandbox Application
**Executable:** `.\bin\Debug-x64\Sandbox\SandboxApp.exe`

**Current Demo (ExampleLayer):**
- **Rendering:** Animated rotating 2D square with color-changing effects and pulsing scale
- **Movement:** WASD keys to move square
- **ImGui Controls Panel:** "ExampleLayer Controls" window with live parameter editing:
  - Position (X, Y) drag controls
  - Move speed slider
  - Rotation speed (degrees/second)
  - Base scale, pulse amplitude, pulse frequency
  - Color animation toggle
  - Animated color frequencies (R, G, B) when animation enabled
  - Static color picker when animation disabled
  - Reset to defaults button
  - Live time display
- **Background:** Subtle animated color gradient
- **ImGui Features:**
  - Dockable windows
  - ImGui demo window available
  - Menu bar with "File > Exit"

**Known Demo Issues:**
- No visual grid or axis indicators
- Movement bounds unlimited

---

## Known Issues & Limitations

### Critical Issues
1. **ImGui Duplicate Context Issue** ? **RESOLVED**
   - **Status:** Fixed as of November 2025
   - **Solution:** Changed imgui to PRIVATE linkage in Pillar/CMakeLists.txt
   - **Result:** Single Dear ImGui context, no more null GImGui crashes

### Design Limitations
2. **Hardcoded Vertex Layout**
   - **Severity:** Medium
   - **Impact:** Cannot add vertex attributes (normals, colors, etc.)
   - **Location:** `OpenGLVertexArray::AddVertexBuffer()`
   - **Fix:** Implement `VertexBufferLayout` class with attribute descriptors

3. **No Batch Rendering**
   - **Severity:** Medium (performance)
   - **Impact:** High draw call count (1 per quad)
   - **Max Quads:** ~1000 before noticeable lag
   - **Fix:** Implement batching in `Renderer2D`

4. **Single Texture Per Quad**
   - **Severity:** Low (optimization)
   - **Impact:** Texture binding overhead
   - **Fix:** Texture atlasing + array textures

### Platform Limitations
5. **Windows Only**
   - **Severity:** Low (as designed)
   - **Impact:** No macOS/Linux support yet
   - **Foundation:** GLFW provides cross-platform base
   - **Blocker:** Platform-specific implementations needed

6. **OpenGL Only**
   - **Severity:** Low (as designed)
   - **Impact:** No DirectX/Vulkan/Metal
   - **Foundation:** Abstraction layer ready for additional APIs

### Testing Gaps
7. **No Renderer Tests**
   - **Severity:** Medium
   - **Impact:** Rendering bugs harder to catch
   - **Challenge:** Requires OpenGL mocking or integration tests
   - **Future:** Consider rendering output comparison tests

8. **Limited Integration Tests**
   - **Severity:** Low
   - **Impact:** Multi-system interactions not tested
   - **Current:** Mostly unit tests
   - **Future:** Add end-to-end application tests

---

## Dependencies & Licensing

### External Libraries (via CMake FetchContent)
| Library | Version | License | Purpose |
|---------|---------|---------|---------|
| GLFW | 3.4 | zlib/libpng | Window/input management |
| GLAD | 0.1.36 (GL 4.6) | Public Domain | OpenGL loader |
| GLM | 1.0.1 | MIT | Math library (vectors, matrices) |
| spdlog | 1.13.0 | MIT | Fast logging |
| Dear ImGui | docking branch | MIT | Debug UI |
| stb_image | latest | Public Domain | Image loading |
| Google Test | 1.14.0 | BSD-3-Clause | Unit testing |

**No manual dependency installation required** - all fetched by CMake.

### Project License
**Not yet specified** - Consider adding `LICENSE` file (MIT recommended for engine projects)

---

## Performance Characteristics

### Current Performance (Debug Build)
- **Startup Time:** ~1-2 seconds
- **Frame Rate:** 60 FPS (VSync on), unlimited (VSync off)
- **Quad Rendering:** ~1000 quads at 60 FPS (unoptimized)
- **Memory Usage:** ~50 MB (minimal scene)
- **DLL Size:** Pillar.dll ~2.5 MB (Debug with symbols)

### Optimization Opportunities
1. **Batch Rendering:** Expected 10x-100x improvement in quad count
2. **Texture Atlasing:** Reduce texture binds, improve cache coherence
3. **Release Build:** ~5x faster (optimizations + no debug symbols)
4. **Shader Caching:** Reduce shader compilation overhead
5. **Asset Streaming:** Reduce memory footprint for large scenes

---

## Development Workflow

### Typical Development Cycle
1. Make code changes in Visual Studio 2022
2. Save files (CMake auto-reconfigures if `CMakeLists.txt` changed)
3. Build: `cmake --build out/build/x64-Debug --config Debug`
4. Check for errors (see Known Build Issues in copilot-instructions.md)
5. Run tests: `.\bin\Debug-x64\Tests\PillarTests.exe`
6. Run application: `.\bin\Debug-x64\Sandbox\SandboxApp.exe`
7. Commit changes: `git add .` ? `git commit -m "message"` ? `git push`
8. GitHub Actions runs CI build + tests

### Code Organization Patterns
- **DLL Export:** Use `PIL_API` macro for public engine classes
- **Factory Methods:** Static `Create()` for platform-specific implementations
- **Smart Pointers:** `std::unique_ptr` for ownership, `std::shared_ptr` for shared resources
- **Event Macros:** `EVENT_CLASS_TYPE()`, `EVENT_CLASS_CATEGORY()` for event classes
- **Logging Macros:** `PIL_CORE_*` (engine), `PIL_*` (application)
- **Key Codes:** `PIL_KEY_*` for platform-agnostic keyboard input

### Testing Strategy
- **Unit Tests:** Business logic, data structures (current focus)
- **Integration Tests:** Multi-system interactions (future)
- **Manual Tests:** Run Sandbox application, verify rendering
- **CI Tests:** Automated on every push/PR
- **Target:** < 5 seconds total test execution time

---

## Roadmap & Milestones

### Milestone 1: Foundation (CURRENT) ?
**Status:** 90% Complete  
**Goals:**
- ? Event system
- ? Layer architecture
- ? Window management
- ? Input handling
- ? Logging system
- ? Basic rendering abstraction
- ? OpenGL renderer
- ? Testing infrastructure
- ? CI/CD pipeline

**Remaining:**
- ?? Fix ImGui linking issue
- ?? Flexible vertex layout

---

### Milestone 2: 2D Rendering (IN PROGRESS) ??
**Status:** 60% Complete  
**Target:** Q1 2025  
**Goals:**
- ? Renderer2D API
- ? Texture system
- ? Basic camera
- ?? Camera controller (next up)
- ?? Batch rendering (next up)
- ? Sprite animation
- ? Particle system
- ? Render stats (draw calls, quads, FPS)

---

### Milestone 3: Scene Management
**Status:** Not Started  
**Target:** Q2 2025  
**Goals:**
- Scene graph/hierarchy
- Entity-Component System (ECS)
- Scene serialization (JSON or binary)
- Asset management improvements
- Prefab system

---

### Milestone 4: Physics & Gameplay
**Status:** Not Started  
**Target:** Q3 2025  
**Goals:**
- Box2D physics integration
- Collision detection/response
- Trigger system
- Audio system (OpenAL/miniaudio)
- Scripting support (Lua or C#)

---

### Milestone 5: Editor & Tools
**Status:** Not Started  
**Target:** Q4 2025  
**Goals:**
- Visual scene editor (ImGui-based)
- Entity inspector
- Asset browser
- Play/stop simulation mode
- Project management

---

### Milestone 6: 3D Rendering (Future)
**Status:** Not Started  
**Target:** 2026  
**Goals:**
- Perspective camera
- 3D model loading (Assimp)
- PBR materials
- Lighting system (point, directional, spot)
- Shadow mapping
- Skybox/environment maps

---

## Getting Started for New Developers

### Prerequisites
- Windows 10/11 (64-bit)
- Visual Studio 2022 with C++ Desktop Development workload
- CMake 3.16+ (bundled with VS 2022)
- Ninja 1.12.1+ (bundled with VS 2022)
- Git for cloning repository

### First-Time Setup
1. Clone repository:
   ```powershell
   git clone https://github.com/XxAmmuraxX/Pillar.git
   cd Pillar
   ```

2. Open **Developer PowerShell for VS 2022**

3. Configure project (downloads dependencies):
   ```powershell
   cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
   ```
   *First run takes ~2-3 minutes to fetch dependencies*

4. Build project:
   ```powershell
   cmake --build out/build/x64-Debug --config Debug --parallel
   ```

5. Run tests:
   ```powershell
   .\bin\Debug-x64\Tests\PillarTests.exe
   ```
   *Should see all tests pass*

6. Run demo application:
   ```powershell
   .\bin\Debug-x64\Sandbox\SandboxApp.exe
   ```
   *Should see window with colored quads and textured logo*

### Recommended Reading Order
1. `.github/copilot-instructions.md` - Comprehensive architecture guide
2. `Tests/README.md` - Testing guidelines
3. `Pillar/src/Pillar/Pillar.h` - Public API overview
4. `Sandbox/src/ExampleLayer.h` - Example usage patterns

---

## Contributing Guidelines

### Code Style
- **Indentation:** Tabs (4 spaces equivalent)
- **Naming Conventions:**
  - Classes: `PascalCase` (e.g., `Application`)
  - Methods: `PascalCase` (e.g., `OnUpdate()`)
  - Variables: `camelCase` with `m_` prefix for members (e.g., `m_Running`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `PIL_KEY_SPACE`)
  - Macros: `UPPER_SNAKE_CASE` with `PIL_` prefix (e.g., `PIL_API`)
- **Headers:** Use `#pragma once` (no header guards)
- **Includes:** Group by category (system, external, engine, local)

### Git Workflow
- **Branch Naming:** `feature/description`, `bugfix/description`
- **Commits:** Descriptive messages, present tense ("Add feature" not "Added feature")
- **Pull Requests:** Target `master` branch, require CI pass
- **Reviews:** One approval required (if working in team)

### Adding New Features
1. Read `.github/copilot-instructions.md` for architecture patterns
2. Create feature branch: `git checkout -b feature/my-feature`
3. Implement feature following existing patterns
4. Add unit tests in `Tests/src/`
5. Update `Pillar/CMakeLists.txt` if adding new files
6. Build and test locally
7. Run full test suite: `.\bin\Debug-x64\Tests\PillarTests.exe`
8. Update documentation (this file, copilot-instructions.md)
9. Commit and push: `git push origin feature/my-feature`
10. Create pull request on GitHub

---

## Troubleshooting

### Build Issues

**Problem:** `error C2061: syntax error: identifier 'WindowCloseEvent'`  
**Fix:** Include event headers before using event types. Add `#include "Pillar/Events/ApplicationEvent.h"` to the file.

**Problem:** `cannot convert argument 1 from 'std::_Binder<...>'`  
**Fix:** Event handler functions must take specific event type references, not base `Event&`. Use `bool OnEvent(WindowCloseEvent& e)`.

**Problem:** `GLFW Error: Failed to create window`  
**Fix:** Check graphics drivers, ensure OpenGL 4.1+ support. Mesa3D software rendering available for CI.

**Problem:** Linker errors with ImGui functions  
**Fix:** ? **RESOLVED** - ImGui now linked PRIVATE in Pillar/CMakeLists.txt to prevent duplicate context

### Runtime Issues

**Problem:** Crash on startup in `ImGui::Begin()` with null GImGui  
**Fix:** ? **RESOLVED** - Fixed by ensuring single Dear ImGui instance via PRIVATE linkage

**Problem:** Texture not loading (white square instead)  
**Fix:** Check asset path with `AssetManager::GetTexturePath()`. Ensure texture in `Sandbox/assets/textures/`. Check console for error messages.

**Problem:** Black screen, no rendering  
**Fix:** Verify shader compilation (check console logs). Ensure `Renderer::Init()` called before rendering. Check OpenGL context creation.

### Test Issues

**Problem:** Tests fail to run (DLL not found)  
**Fix:** Check `Pillar.dll` copied to `bin/Debug-x64/Tests/`. Run build command again. Verify POST_BUILD commands in CMakeLists.txt.

**Problem:** Window tests hang in CI  
**Fix:** Known issue with CI environment. Mesa3D should handle this. Verify `.github/workflows/build.yml` has Mesa3D setup step.

---

## Resources & Documentation

### Internal Documentation
- `.github/copilot-instructions.md` - **Primary reference** for architecture and patterns
- `Tests/README.md` - Testing guidelines and examples
- `PROJECT_STATUS.md` (this file) - Current project state

### External Resources
- [GLFW Documentation](https://www.glfw.org/documentation.html) - Window and input
- [OpenGL Reference](https://www.khronos.org/opengl/wiki/) - Graphics API
- [Dear ImGui Wiki](https://github.com/ocornut/imgui/wiki) - UI framework
- [GLM Documentation](https://glm.g-truc.net/0.9.9/index.html) - Math library
- [spdlog GitHub](https://github.com/gabime/spdlog) - Logging library
- [Google Test Primer](https://google.github.io/googletest/primer.html) - Testing framework

### Learning Resources
- [LearnOpenGL](https://learnopengl.com/) - OpenGL tutorials
- [Game Engine Architecture (Book)](https://www.gameenginebook.com/) - Engine design patterns
- [The Cherno Game Engine Series](https://www.youtube.com/playlist?list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT) - YouTube tutorial series

---

## Contact & Support

**Repository:** https://github.com/XxAmmuraxX/Pillar  
**Primary Developer:** XxAmmuraxX  
**Current Branch:** `feature/render_api/2`  
**Issues:** GitHub Issues (create issue for bugs/features)

---

## Changelog Summary

### Recent Changes (Last Sprint)
- ? Implemented `Renderer2D` with `DrawQuad()` API
- ? Added `Texture2D` system with OpenGL implementation
- ? Integrated `stb_image` for texture loading
- ? Created `AssetManager` for path resolution
- ? Added `OrthographicCamera` with manual controls
- ? Updated `ExampleLayer` to demonstrate Renderer2D
- ? Verified all unit tests pass in CI/CD

### Upcoming Changes (Next Sprint)
- ?? Fix ImGui DLL linkage issue
- ?? Implement `OrthographicCameraController`
- ?? Add batch rendering to `Renderer2D`
- ?? Write tests for camera controller
- ?? Add render stats overlay (FPS, draw calls, quad count)

---

**End of Project Status Document**  
*This document should be updated at the end of each development sprint or when major milestones are reached.*
