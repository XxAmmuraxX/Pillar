# Camera System Implementation Plan

**Date:** January 2025  
**Branch:** `feature/render_api/2`  
**Priority:** High (Next immediate task after ImGui fix)

---

## Current State Analysis

### ? What We Have

#### 1. **OrthographicCamera Class** (Fully Implemented)
**Location:** `Pillar/src/Pillar/Renderer/OrthographicCamera.h/cpp`

**Features:**
- Orthographic projection matrix (left, right, bottom, top bounds)
- View matrix with position (vec3) and rotation (float, Z-axis only)
- Combined view-projection matrix
- Setters: `SetPosition()`, `SetRotation()`
- Getters: `GetPosition()`, `GetRotation()`, `GetProjectionMatrix()`, `GetViewMatrix()`, `GetViewProjectionMatrix()`
- Automatic matrix recalculation on transform change

**Current Usage:**
- `ExampleLayer` creates camera: `m_Camera(-1.6f, 1.6f, -0.9f, 0.9f)` (aspect ~16:9)
- Manual input handling in `OnUpdate()`:
  - WASD keys move camera (2.0 units/sec)
  - Q/E keys rotate camera (50 deg/sec)
- Passed to `Renderer2D::BeginScene(m_Camera)`

**Limitations:**
- No zoom support
- No bounds limiting
- Manual input handling scattered in application code
- No smooth movement/damping
- No aspect ratio management
- No viewport resize handling

#### 2. **Input System** (Fully Functional)
**Location:** `Pillar/src/Pillar/Input.h/cpp`

**Available APIs:**
- `Input::IsKeyPressed(keycode)` - Keyboard polling
- `Input::IsMouseButtonPressed(button)` - Mouse button polling
- `Input::GetMousePosition()` - Mouse X/Y coordinates
- `Input::GetMouseX()`, `Input::GetMouseY()` - Individual axis
- Platform-agnostic key codes (`PIL_KEY_*` in `KeyCodes.h`)

**Missing:**
- Mouse scroll event handling (events exist but no polling API)
- Gamepad/controller support

#### 3. **Event System** (Available)
**Location:** `Pillar/Events/MouseEvent.h`

**Relevant Events:**
- `MouseScrolledEvent` - Has `GetXOffset()`, `GetYOffset()`
- `WindowResizeEvent` - Has `GetWidth()`, `GetHeight()`

**Not Currently Used for Camera:**
- Could be used for zoom (scroll) and aspect ratio updates (resize)

---

## ? What We Need

### Priority 1: OrthographicCameraController (High Priority)

**Purpose:** Encapsulate camera movement, rotation, and zoom logic separate from application layers.

**File Structure:**
```
Pillar/src/Pillar/Renderer/
??? OrthographicCamera.h/cpp          (existing)
??? OrthographicCameraController.h    (NEW)
??? OrthographicCameraController.cpp  (NEW)
```

**Responsibilities:**
1. **Input Handling:** Process keyboard/mouse input for camera control
2. **Movement:** Translate camera position based on WASD or arrow keys
3. **Rotation:** Rotate camera based on Q/E keys
4. **Zoom:** Change camera bounds based on mouse scroll
5. **Aspect Ratio:** Adjust projection on window resize
6. **Bounds:** Constrain camera to world limits (optional feature)

**Core Features:**

#### Basic Features (MVP)
- [x] Movement: WASD or Arrow keys
- [x] Rotation: Q/E keys (optional, can disable)
- [x] Zoom: Mouse wheel (change projection bounds)
- [x] Speed control: Configurable movement/rotation/zoom speeds
- [x] Delta time integration: Smooth movement regardless of frame rate
- [x] Event handling: `OnUpdate(dt)` and `OnEvent(event)`

#### Advanced Features (Future)
- [ ] Smooth camera movement (damping/lerp)
- [ ] Camera bounds (min/max X/Y position)
- [ ] Zoom limits (min/max zoom level)
- [ ] Alternative control schemes (click-drag to pan)
- [ ] Camera shake effects
- [ ] Follow target (attach to entity)
- [ ] Smooth zoom (lerp to target zoom)

---

## Implementation Plan

### Phase 1: OrthographicCameraController (This Week)

#### Step 1.1: Create OrthographicCameraController Header
**File:** `Pillar/src/Pillar/Renderer/OrthographicCameraController.h`

**Class Design:**
```cpp
namespace Pillar {
    class PIL_API OrthographicCameraController
    {
    public:
        OrthographicCameraController(float aspectRatio, bool rotation = false);

        void OnUpdate(float deltaTime);
        void OnEvent(Event& e);

        OrthographicCamera& GetCamera() { return m_Camera; }
        const OrthographicCamera& GetCamera() const { return m_Camera; }

        float GetZoomLevel() const { return m_ZoomLevel; }
        void SetZoomLevel(float level);

    private:
        bool OnMouseScrolled(MouseScrolledEvent& e);
        bool OnWindowResized(WindowResizeEvent& e);

    private:
        float m_AspectRatio;
        float m_ZoomLevel = 1.0f;
        OrthographicCamera m_Camera;

        bool m_Rotation;

        glm::vec3 m_CameraPosition = { 0.0f, 0.0f, 0.0f };
        float m_CameraRotation = 0.0f;

        float m_CameraTranslationSpeed = 5.0f;
        float m_CameraRotationSpeed = 180.0f;
    };
}
```

**Key Design Decisions:**
- Owns an `OrthographicCamera` instance (composition)
- Aspect ratio stored to recalculate projection on zoom/resize
- Zoom level as multiplier (1.0 = normal, 2.0 = zoomed out 2x)
- Rotation optional (some games don't need camera rotation)
- Speed configurable but with sensible defaults

#### Step 1.2: Implement OrthographicCameraController
**File:** `Pillar/src/Pillar/Renderer/OrthographicCameraController.cpp`

**Constructor:**
```cpp
OrthographicCameraController(float aspectRatio, bool rotation)
    : m_AspectRatio(aspectRatio), 
      m_Camera(-aspectRatio * m_ZoomLevel, aspectRatio * m_ZoomLevel, 
               -m_ZoomLevel, m_ZoomLevel),
      m_Rotation(rotation)
{
}
```

**OnUpdate(deltaTime):**
- Poll input for WASD/Arrow keys
- Update `m_CameraPosition` based on input and speed
- If rotation enabled, poll Q/E keys
- Update `m_CameraRotation` based on input and rotation speed
- Apply position/rotation to `m_Camera`

**OnEvent(event):**
- Dispatch `MouseScrolledEvent` ? `OnMouseScrolled()`
- Dispatch `WindowResizeEvent` ? `OnWindowResized()`

**OnMouseScrolled(event):**
- Adjust `m_ZoomLevel` based on scroll offset
- Clamp zoom to reasonable range (e.g., 0.25 to 10.0)
- Recalculate camera projection bounds
- Update `m_Camera` projection

**OnWindowResized(event):**
- Calculate new aspect ratio from width/height
- Update `m_AspectRatio`
- Recalculate camera projection bounds
- Update `m_Camera` projection

#### Step 1.3: Update CMakeLists.txt
**File:** `Pillar/CMakeLists.txt`

Add new files to `add_library(Pillar SHARED ...)`:
```cmake
src/Pillar/Renderer/OrthographicCameraController.cpp
```

#### Step 1.4: Update ExampleLayer to Use Controller
**File:** `Sandbox/src/ExampleLayer.h`

**Changes:**
- Replace `Pillar::OrthographicCamera m_Camera;` with `Pillar::OrthographicCameraController m_CameraController;`
- Remove manual input handling code (WASD, Q/E)
- Initialize controller with aspect ratio: `m_CameraController(16.0f / 9.0f, true)`
- In `OnUpdate()`: Call `m_CameraController.OnUpdate(dt);`
- In `OnEvent()`: Call `m_CameraController.OnEvent(event);`
- Pass camera to renderer: `Renderer2D::BeginScene(m_CameraController.GetCamera());`

**Benefits:**
- Cleaner application code (separation of concerns)
- Reusable camera controller for other layers
- Standardized camera control scheme

#### Step 1.5: Add Unit Tests
**File:** `Tests/src/CameraTests.cpp` (NEW)

**Test Coverage:**
```cpp
// OrthographicCamera Tests
- TEST(OrthographicCameraTests, Constructor_SetsProjectionMatrix)
- TEST(OrthographicCameraTests, SetPosition_UpdatesViewMatrix)
- TEST(OrthographicCameraTests, SetRotation_UpdatesViewMatrix)
- TEST(OrthographicCameraTests, GetViewProjectionMatrix_CombinesMatrices)

// OrthographicCameraController Tests
- TEST(CameraControllerTests, Constructor_InitializesWithAspectRatio)
- TEST(CameraControllerTests, OnUpdate_MovesCamera_WithWASD)
- TEST(CameraControllerTests, OnUpdate_RotatesCamera_WithQE)
- TEST(CameraControllerTests, OnMouseScrolled_ChangesZoomLevel)
- TEST(CameraControllerTests, OnWindowResized_UpdatesAspectRatio)
- TEST(CameraControllerTests, SetZoomLevel_RecalculatesProjection)
- TEST(CameraControllerTests, GetCamera_ReturnsInternalCamera)
```

**Add to CMakeLists.txt:**
```cmake
# In Tests/CMakeLists.txt
add_executable(PillarTests
    # ...existing files...
    src/CameraTests.cpp
)
```

#### Step 1.6: Add ImGui Debug Panel (Optional)
**Location:** `ExampleLayer::OnImGuiRender()`

**Display:**
- Camera position (XYZ)
- Camera rotation
- Zoom level
- Movement speed slider
- Rotation speed slider
- Zoom speed slider
- Reset camera button

---

### Phase 2: Advanced Camera Features (Next Sprint)

#### 2.1: Smooth Camera Movement (Damping)
**Goal:** Camera position/rotation lerps to target over time instead of instant movement.

**Implementation:**
- Add `m_TargetPosition` and `m_TargetRotation`
- Input updates targets, not actual values
- `OnUpdate()` lerps current ? target using `glm::mix()` or custom lerp
- Configurable damping factor (e.g., 0.1 = slow, 1.0 = instant)

**Benefits:**
- More polished feel
- Reduces jarring camera snapping
- Better for action games

#### 2.2: Camera Bounds
**Goal:** Constrain camera to world boundaries.

**Implementation:**
- Add `SetBounds(minX, maxX, minY, maxY)`
- In `OnUpdate()`, clamp position to bounds
- Consider zoom level (wider view = tighter bounds)

**Use Cases:**
- Level boundaries
- Prevent showing empty space
- Minimap-style cameras

#### 2.3: Camera Follow Target
**Goal:** Camera automatically follows a target entity.

**Implementation:**
- Add `SetTarget(glm::vec3* target)` or `SetTarget(Entity* entity)`
- In `OnUpdate()`, move camera toward target position
- Optional: Dead zone (only move if target outside center box)
- Optional: Look-ahead (offset based on target velocity)

**Use Cases:**
- Player-following camera
- Cutscene cameras
- Boss fight cameras

#### 2.4: Alternative Control Schemes
**Options:**
- **Click-Drag Pan:** Middle mouse button + drag moves camera
- **Edge Scrolling:** Mouse at screen edge moves camera (RTS-style)
- **Keyboard Only:** Arrow keys instead of WASD
- **Gamepad:** Left stick moves, right stick rotates

#### 2.5: Camera Shake
**Goal:** Screen shake effect for impacts, explosions, etc.

**Implementation:**
- Add `Shake(intensity, duration)`
- Store shake parameters
- In `OnUpdate()`, add random offset to camera position
- Decay intensity over time
- Remove offset when shake expires

**Use Cases:**
- Weapon recoil
- Explosion effects
- Impact feedback

---

### Phase 3: Perspective Camera (Future - 3D Support)

**When Needed:** When adding 3D rendering support (Milestone 6)

**Files:**
```
Pillar/src/Pillar/Renderer/
??? PerspectiveCamera.h/cpp              (NEW)
??? PerspectiveCameraController.h/cpp    (NEW)
```

**Features:**
- Field of view (FOV)
- Aspect ratio
- Near/far clip planes
- First-person controls (mouse look)
- Third-person controls (orbit around target)
- Collision detection (prevent clipping into walls)

---

## Testing Strategy

### Unit Tests (High Priority)
**File:** `Tests/src/CameraTests.cpp`

**Coverage:**
- Camera matrix calculations
- Controller input handling (mock input)
- Zoom/resize calculations
- Bounds clamping

**Challenges:**
- Input polling requires GLFW window (use test fixture from `InputTests.cpp`)
- Event handling testable via direct event dispatch

### Integration Tests (Medium Priority)
**Goal:** Test camera with actual rendering

**Approach:**
- Use `ExampleLayer` as integration test
- Visual verification (manual)
- Automated: Capture frame buffer, compare pixels

### Manual Testing (Essential)
**Checklist:**
- [ ] WASD movement smooth and responsive
- [ ] Q/E rotation works (if enabled)
- [ ] Mouse wheel zoom works
- [ ] Window resize updates camera correctly
- [ ] Camera doesn't jitter or snap unexpectedly
- [ ] Controls feel natural at 60 FPS and 144 FPS
- [ ] No drift (camera shouldn't move when no input)

---

## Performance Considerations

### Current Performance: ? Acceptable
- Camera updates once per frame (negligible cost)
- Matrix calculations cached until transform changes
- Input polling lightweight (GLFW direct state query)

### Potential Issues:
- **Smooth movement:** Adds lerp calculations (still negligible)
- **Camera shake:** Random number generation per frame (use fast RNG)
- **Bounds checking:** Simple float comparisons (negligible)

### Optimization Opportunities:
- Cache projection matrix (only recalculate on zoom/resize)
- Use fast math functions (GLM already optimized)
- Avoid unnecessary camera updates if position unchanged

---

## API Design Considerations

### Design Principles:
1. **Separation of Concerns:** Controller handles input, Camera handles matrices
2. **Composition over Inheritance:** Controller owns Camera, doesn't inherit
3. **Event-Driven:** Use event system for zoom/resize, not polling
4. **Configurability:** Expose speed settings, toggles for features
5. **Simplicity:** Default behavior works out-of-box, advanced features opt-in

### Public API for OrthographicCameraController:
```cpp
// Construction
OrthographicCameraController(float aspectRatio, bool rotation = false);

// Core Updates
void OnUpdate(float deltaTime);
void OnEvent(Event& e);

// Camera Access
OrthographicCamera& GetCamera();
const OrthographicCamera& GetCamera() const;

// Zoom Control
float GetZoomLevel() const;
void SetZoomLevel(float level);

// Speed Configuration
void SetTranslationSpeed(float speed);
void SetRotationSpeed(float speed);
void SetZoomSpeed(float speed);

// Optional: Bounds (Phase 2)
void SetBounds(float minX, float maxX, float minY, float maxY);
void ClearBounds();

// Optional: Target Following (Phase 2)
void SetTarget(const glm::vec3* target);
void ClearTarget();

// Optional: Shake (Phase 2)
void Shake(float intensity, float duration);
```

---

## Documentation Requirements

### Code Comments:
- Header file: Brief description of each method
- Implementation: Explain complex calculations (projection bounds)
- Examples: Show typical usage patterns

### Update Existing Docs:
1. **`.github/copilot-instructions.md`:**
   - Add "OrthographicCameraController" to Common Development Tasks
   - Update "Creating Rendering Code" section with camera controller usage
   - Add to "Implemented Features" list

2. **`PROJECT_STATUS.md`:**
   - Move "Camera System" from "In Progress" to "Completed" (after implementation)
   - Update "Current Demo (ExampleLayer)" description
   - Add camera controller to feature list

3. **`Tests/README.md`:**
   - Add `CameraTests.cpp` to test organization section
   - Example camera test patterns

---

## Dependencies & Integration Points

### Required Classes (Already Exist):
- ? `OrthographicCamera` - Core camera math
- ? `Input` - Keyboard/mouse polling
- ? `Event`, `MouseScrolledEvent`, `WindowResizeEvent` - Event handling
- ? `Renderer2D` - Accepts camera in `BeginScene()`

### Modified Classes:
- **`ExampleLayer`** - Use controller instead of manual input
- **`Pillar.h`** - Add `#include "Pillar/Renderer/OrthographicCameraController.h"`

### No Breaking Changes:
- `OrthographicCamera` API unchanged (backward compatible)
- Existing code using `OrthographicCamera` directly still works
- Controller is opt-in, not required

---

## Risk Assessment

### Low Risk:
- ? Well-defined problem (camera control is common in engines)
- ? Simple math (projection/view matrices)
- ? Isolated component (doesn't affect rendering)
- ? Easy to test (unit tests + visual verification)

### Potential Issues:
1. **Input Lag:** If update rate too low
   - **Mitigation:** Delta time ensures consistent speed
2. **Zoom Issues:** Projection bounds calculation errors
   - **Mitigation:** Unit tests for edge cases (zero zoom, negative aspect)
3. **Event Ordering:** Resize event before render
   - **Mitigation:** Events processed before `OnUpdate()` in app loop

---

## Success Criteria

### Phase 1 Complete When:
- [x] `OrthographicCameraController.h/cpp` implemented
- [x] Unit tests written and passing (>80% coverage)
- [x] `ExampleLayer` refactored to use controller
- [x] WASD movement works smoothly
- [x] Q/E rotation works (if enabled)
- [x] Mouse wheel zoom works
- [x] Window resize updates camera projection
- [x] Documentation updated (copilot-instructions, PROJECT_STATUS)
- [x] All existing tests still pass
- [x] Application runs without crashes or visual glitches

### Phase 2 Complete When:
- [x] At least 2 advanced features implemented (damping, bounds, or follow)
- [x] Additional unit tests for new features
- [x] Performance profiling shows <1% frame time
- [x] User-facing configuration (ImGui sliders)

---

## Timeline Estimate

### Phase 1: Basic Controller (2-4 hours)
- Step 1.1: Header design (30 min)
- Step 1.2: Implementation (1.5 hours)
- Step 1.3: CMakeLists update (5 min)
- Step 1.4: ExampleLayer refactor (30 min)
- Step 1.5: Unit tests (1 hour)
- Step 1.6: ImGui panel (30 min, optional)

**Total: ~4 hours** (can be done in single session)

### Phase 2: Advanced Features (4-6 hours)
- Per feature: ~1-2 hours each
- Testing: 1 hour
- Polish/tuning: 1 hour

**Total: ~6 hours** (next sprint)

### Phase 3: Perspective Camera (8-12 hours)
- Design: 1 hour
- Implementation: 4 hours
- Testing: 2 hours
- 3D scene setup: 3 hours
- Polish: 2 hours

**Total: ~12 hours** (future milestone)

---

## Next Immediate Steps (This Week)

1. **Fix ImGui Linking Issue** (30 min) - Critical blocker
2. **Implement `OrthographicCameraController`** (4 hours) - This document
3. **Write Camera Tests** (1 hour)
4. **Update Documentation** (30 min)
5. **Manual Testing** (30 min)

**Total Time for Camera System Phase 1: ~6.5 hours**

---

## Questions to Consider

### Before Implementation:
- [x] Should rotation be enabled by default? **Decision:** No, opt-in via constructor parameter
- [x] What should default speeds be? **Decision:** 5.0 units/sec movement, 180 deg/sec rotation
- [x] Should zoom be centered or mouse-position-based? **Decision:** Centered (simpler for Phase 1)
- [x] How to handle aspect ratio on first frame? **Decision:** Pass to constructor, update on resize event
- [x] Should controller handle multiple control schemes? **Decision:** No, one scheme per controller (WASD + QE + scroll)

### For Phase 2:
- [ ] How to expose control configuration (code vs. config file)?
- [ ] Should controller support hot-swapping controls at runtime?
- [ ] How to handle gamepad input (separate controller class)?
- [ ] Should camera have animation system (keyframes)?

---

## References & Resources

### Internal Docs:
- `.github/copilot-instructions.md` - Architecture patterns
- `PROJECT_STATUS.md` - Current project state
- `Tests/README.md` - Testing guidelines

### External Resources:
- [GLM Documentation](https://glm.g-truc.net/0.9.9/api/a00247.html) - Matrix math
- [LearnOpenGL: Camera](https://learnopengl.com/Getting-started/Camera) - Camera systems
- [Game Programming Patterns: Update Method](http://gameprogrammingpatterns.com/update-method.html) - Delta time

### Similar Implementations:
- Unity: `Cinemachine` package
- Unreal: `APlayerController` + `ACameraActor`
- Godot: `Camera2D` node
- SDL2: Manual camera implementation

---

**End of Camera System Plan**

This document should be updated as implementation progresses. Mark checkboxes when features are complete.
