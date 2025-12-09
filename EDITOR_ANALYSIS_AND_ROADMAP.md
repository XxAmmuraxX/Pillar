# Pillar Editor - Comprehensive Analysis & Improvement Roadmap

## Overview

This document provides a comprehensive analysis of the current Pillar Engine Editor implementation, identifies bugs and missing features, and outlines a prioritized roadmap for improvements from the perspective of a game developer user.

**Analysis Date:** December 2024  
**Current Editor Version:** v0.1 (Early Development)  
**Engine Version:** Pillar Engine (Feature/Scene Serialization Branch)

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Current Implementation Analysis](#current-implementation-analysis)
3. [Critical Bugs to Fix](#critical-bugs-to-fix)
4. [Missing Essential Features](#missing-essential-features)
5. [Quality of Life Improvements](#quality-of-life-improvements)
6. [Advanced Features Wishlist](#advanced-features-wishlist)
7. [Prioritized Implementation Roadmap](#prioritized-implementation-roadmap)
8. [Technical Debt](#technical-debt)

---

## Executive Summary

### Current State
The Pillar Editor has a solid foundation with functional core panels (Viewport, Hierarchy, Inspector, Content Browser, Console), basic play/pause/stop functionality, and scene serialization. However, several critical features are missing that would significantly improve the user experience for game developers.

### Top 5 Priority Items
1. **Transform Gizmos** - Visual manipulation of entities in viewport
2. **Undo/Redo System** - Essential for any editor workflow
3. **Entity Picking in Viewport** - Click to select entities in scene view
4. **Native File Dialogs** - Proper Open/Save As dialogs
5. **Multi-Component Inspector Support** - More components editable in inspector

---

## Current Implementation Analysis

### ✅ What Works Well

| Feature | Implementation Quality | Notes |
|---------|----------------------|-------|
| **Dockspace Layout** | ⭐⭐⭐⭐ | Good default layout, resizable panels |
| **Scene Hierarchy Panel** | ⭐⭐⭐⭐ | Tree view, multi-select, context menus |
| **Inspector Panel** | ⭐⭐⭐ | Transform, Velocity, Collider editing |
| **Viewport Rendering** | ⭐⭐⭐⭐ | Framebuffer rendering, grid overlay |
| **Editor Camera** | ⭐⭐⭐⭐ | Pan (MMB), zoom (scroll), focus (F) |
| **Play/Pause/Stop** | ⭐⭐⭐⭐ | Scene backup/restore works correctly |
| **Scene Serialization** | ⭐⭐⭐⭐ | JSON format, save/load functional |
| **Console Panel** | ⭐⭐⭐ | Log filtering, auto-scroll |
| **Content Browser** | ⭐⭐⭐ | Directory navigation, drag-drop source |
| **Dark Theme** | ⭐⭐⭐⭐ | Professional VS Code-inspired styling |

### ⚠️ Partial Implementations

| Feature | Current State | What's Missing |
|---------|--------------|----------------|
| **Entity Selection** | Hierarchy only | No viewport picking |
| **Component Editing** | 5 components | Missing: Bullet, XPGem, Hierarchy, Script |
| **File Operations** | Hardcoded paths | No native file dialogs |
| **Keyboard Shortcuts** | Basic set | Missing common editor shortcuts |
| **Asset Import** | Manual folder copy | No import/conversion pipeline |

### ❌ Not Implemented

| Feature | Priority | Complexity |
|---------|----------|------------|
| Transform Gizmos | 🔴 Critical | Medium |
| Undo/Redo | 🔴 Critical | High |
| Viewport Entity Picking | 🔴 Critical | Medium |
| Sprite/Texture Component | 🟠 High | Medium |
| Script Component | 🟠 High | High |
| Prefab System | 🟡 Medium | High |
| Animation System | 🟡 Medium | High |
| Audio Integration | 🟡 Medium | Medium |
| Tilemaps | 🟡 Medium | High |

---

## Critical Bugs to Fix

### 🔴 BUG-001: No Visual Feedback for Selected Entity in Viewport
**Severity:** High  
**Location:** `ViewportPanel.cpp`

**Current Behavior:**  
Selection highlight (orange outline) exists but is subtle and only renders behind entities.

**Expected Behavior:**  
- Clear visual selection indicator (handles/outline around entity)
- Selection should be visible regardless of entity color

**Fix Required:**
```cpp
// Draw selection outline AROUND the entity (not behind)
if (isSelected)
{
    // Draw 4 line segments forming a box around entity
    DrawSelectionOutline(transform.Position, size, outlineColor);
}
```

---

### 🔴 BUG-002: Missing Components in Inspector
**Severity:** High  
**Location:** `InspectorPanel.cpp`

**Current Behavior:**  
Only 5 component types can be edited (Tag, Transform, Velocity, Rigidbody, Collider).

**Missing Components:**
- `BulletComponent`
- `XPGemComponent`
- `HierarchyComponent`
- Any future custom components

**Fix Required:**  
Add `DrawBulletComponent()`, `DrawXPGemComponent()`, and `DrawHierarchyComponent()` methods.

---

### 🔴 BUG-003: Add Component Menu Incomplete
**Severity:** High  
**Location:** `InspectorPanel.cpp:DrawAddComponentButton()`

**Current Behavior:**  
Only Velocity and Collider (Box/Circle) can be added.

**Missing Addable Components:**
- RigidbodyComponent
- BulletComponent
- XPGemComponent
- HierarchyComponent

**Fix Required:**
```cpp
if (!entity.HasComponent<Pillar::RigidbodyComponent>())
{
    if (ImGui::Selectable("Rigidbody"))
    {
        entity.AddComponent<Pillar::RigidbodyComponent>();
        ImGui::CloseCurrentPopup();
    }
}
// Add similar for other components
```

---

### 🟠 BUG-004: Content Browser Doesn't Update on External Changes
**Severity:** Medium  
**Location:** `ContentBrowserPanel.cpp`

**Current Behavior:**  
Directory listing is not refreshed when files are added/removed externally.

**Fix Required:**  
Add a refresh button and/or implement filesystem watching.

---

### 🟠 BUG-005: Camera Reset Position Inconsistent
**Severity:** Medium  
**Location:** `EditorCamera.cpp`

**Current Behavior:**  
`ResetCamera()` only resets position and zoom, not any pan offset accumulated.

**Fix Required:**  
Ensure all camera state is properly reset.

---

### 🟡 BUG-006: Entity Name Duplication Not Handled
**Severity:** Low  
**Location:** `Scene.cpp:CreateEntity()`

**Current Behavior:**  
Creating multiple entities with same name (e.g., "Enemy") results in identical names.

**Expected Behavior:**  
Auto-append numbers: "Enemy", "Enemy (1)", "Enemy (2)"

---

### 🟡 BUG-007: Console Panel Doesn't Show Engine Core Logs
**Severity:** Low  
**Location:** `ConsolePanel.cpp`

**Current Behavior:**  
Only editor-specific `ConsolePanel::Log()` calls are shown.

**Expected Behavior:**  
Engine `PIL_CORE_*` and `PIL_*` logs should also appear in console.

---

## Missing Essential Features

### 🔴 FEAT-001: Transform Gizmos (Critical)
**Priority:** P0 - Critical  
**Complexity:** Medium  
**Estimated Effort:** 2-3 days

**Description:**  
Visual handles for moving, rotating, and scaling entities directly in the viewport.

**Requirements:**
- [ ] Integrate ImGuizmo library
- [ ] Translate gizmo (arrows for X/Y movement)
- [ ] Rotate gizmo (circle for 2D rotation)
- [ ] Scale gizmo (squares for X/Y/uniform scaling)
- [ ] Keyboard shortcuts: W (translate), E (rotate), R (scale)
- [ ] Gizmo snapping (hold Ctrl for grid snap)

**Implementation Notes:**
```cmake
# CMakeLists.txt
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG master
)
```

---

### 🔴 FEAT-002: Undo/Redo System (Critical)
**Priority:** P0 - Critical  
**Complexity:** High  
**Estimated Effort:** 4-5 days

**Description:**  
Command pattern implementation for undoable editor actions.

**Requirements:**
- [ ] Command base class with Execute/Undo
- [ ] CommandHistory stack management
- [ ] TransformCommand (position, rotation, scale changes)
- [ ] ComponentAddCommand / ComponentRemoveCommand
- [ ] EntityCreateCommand / EntityDeleteCommand
- [ ] Ctrl+Z (Undo), Ctrl+Y (Redo) shortcuts
- [ ] Edit menu showing undo/redo with action names

**Affected Operations:**
- Transform changes
- Component add/remove
- Entity create/delete/duplicate
- Property value changes

---

### 🔴 FEAT-003: Viewport Entity Picking (Critical)
**Priority:** P0 - Critical  
**Complexity:** Medium  
**Estimated Effort:** 2 days

**Description:**  
Click on entities in the viewport to select them.

**Requirements:**
- [ ] Mouse click detection in viewport
- [ ] World coordinate conversion (screen to world)
- [ ] AABB intersection test with entities
- [ ] Select topmost entity under cursor
- [ ] Shift+Click for multi-select
- [ ] Click empty space to deselect

**Implementation Approach:**
```cpp
glm::vec2 ViewportPanel::ScreenToWorld(const glm::vec2& screenPos)
{
    // Convert screen position to normalized device coordinates
    // Then to world coordinates using inverse view-projection matrix
}

Entity ViewportPanel::PickEntity(const glm::vec2& worldPos)
{
    // Iterate entities, check bounds intersection
    // Return topmost entity (highest Z or last rendered)
}
```

---

### 🔴 FEAT-004: Native File Dialogs (Critical)
**Priority:** P0 - Critical  
**Complexity:** Low  
**Estimated Effort:** 1 day

**Description:**  
Proper Open/Save dialogs instead of hardcoded paths.

**Requirements:**
- [ ] Integrate nativefiledialog or tinyfiledialogs
- [ ] Open Scene dialog (*.scene.json filter)
- [ ] Save Scene As dialog
- [ ] Remember last used directory

**Implementation:**
```cmake
FetchContent_Declare(
  nfd
  GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
  GIT_TAG master
)
```

---

### 🟠 FEAT-005: Sprite/Render Component
**Priority:** P1 - High  
**Complexity:** Medium  
**Estimated Effort:** 2-3 days

**Description:**  
Component for entity visual appearance (texture, color, layer).

**Requirements:**
- [ ] Create `SpriteComponent` struct
- [ ] Texture path, tint color, flip X/Y, layer order
- [ ] Inspector UI for sprite editing
- [ ] Texture picker from Content Browser (drag-drop)
- [ ] Render sprites in viewport

**Component Design:**
```cpp
struct SpriteComponent
{
    std::string TexturePath;
    glm::vec4 TintColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec2 TextureOffset = { 0.0f, 0.0f };
    glm::vec2 TextureTiling = { 1.0f, 1.0f };
    int SortingLayer = 0;
    int OrderInLayer = 0;
    bool FlipX = false;
    bool FlipY = false;
};
```

---

### 🟠 FEAT-006: Camera Component & Runtime Camera
**Priority:** P1 - High  
**Complexity:** Medium  
**Estimated Effort:** 2 days

**Description:**  
Camera component for in-game camera (separate from editor camera).

**Requirements:**
- [ ] Create `CameraComponent` struct
- [ ] Orthographic projection settings
- [ ] Primary camera flag
- [ ] Follow target entity option
- [ ] Camera bounds/constraints
- [ ] Switch to game camera in Play mode

---

### 🟠 FEAT-007: Entity Templates/Presets
**Priority:** P1 - High  
**Complexity:** Low  
**Estimated Effort:** 1 day

**Description:**  
Quick-create common entity configurations.

**Requirements:**
- [ ] Entity > Create menu with preset types
- [ ] 2D Sprite (Transform + Sprite)
- [ ] Physics Object (Transform + Rigidbody + Collider)
- [ ] Trigger Zone (Transform + Collider as sensor)
- [ ] Custom template save/load

---

### 🟡 FEAT-008: Scene Settings Panel
**Priority:** P2 - Medium  
**Complexity:** Low  
**Estimated Effort:** 1 day

**Description:**  
Global scene settings configuration.

**Requirements:**
- [ ] Physics gravity settings
- [ ] Background color
- [ ] Scene bounds/camera limits
- [ ] Default layer setup
- [ ] Scene metadata (author, version)

---

### 🟡 FEAT-009: Grid Snapping & Alignment Tools
**Priority:** P2 - Medium  
**Complexity:** Medium  
**Estimated Effort:** 2 days

**Description:**  
Snap entities to grid, align multiple entities.

**Requirements:**
- [ ] Configurable grid size
- [ ] Show/hide grid toggle
- [ ] Snap on move (Ctrl held)
- [ ] Align selected: Top, Bottom, Left, Right, Center
- [ ] Distribute evenly (horizontal/vertical)

---

### 🟡 FEAT-010: Copy/Paste Entities
**Priority:** P2 - Medium  
**Complexity:** Medium  
**Estimated Effort:** 1 day

**Description:**  
Standard clipboard operations for entities.

**Requirements:**
- [ ] Ctrl+C: Copy selected entities to clipboard
- [ ] Ctrl+V: Paste at cursor position
- [ ] Ctrl+X: Cut (copy + delete)
- [ ] Cross-scene copy/paste support

---

## Quality of Life Improvements

### QOL-001: Improved Viewport Controls
- [ ] Right-click drag to pan (alternative to MMB)
- [ ] Arrow keys to nudge selected entity
- [ ] Double-click to frame entity
- [ ] Viewport overlays (entity names, collider bounds)
- [ ] Toggle collider visualization

### QOL-002: Inspector Improvements
- [ ] Component collapse/expand persistence
- [ ] Reset button for each property
- [ ] Color picker preview square
- [ ] Slider min/max customization
- [ ] Copy/paste component values

### QOL-003: Hierarchy Improvements
- [ ] Search/filter entities by name
- [ ] Entity visibility toggle (eye icon)
- [ ] Entity lock toggle (prevent selection)
- [ ] Drag-drop reordering
- [ ] Expand/collapse all button

### QOL-004: Content Browser Improvements
- [ ] Asset thumbnails (texture previews)
- [ ] Search filter
- [ ] Create folder button
- [ ] Rename files in place
- [ ] Delete confirmation dialog
- [ ] Refresh button

### QOL-005: Console Improvements
- [ ] Command input field
- [ ] Search/filter logs
- [ ] Log timestamps
- [ ] Copy log text
- [ ] Clickable stack traces (future)

### QOL-006: Editor Preferences
- [ ] Camera speed settings
- [ ] Grid size settings
- [ ] Theme selection (future)
- [ ] Keyboard shortcut customization
- [ ] Auto-save interval

### QOL-007: Status Bar
- [ ] Display at bottom of editor
- [ ] Show: FPS, entity count, current tool, zoom level
- [ ] Notification area for operations

---

## Advanced Features Wishlist

### Future Phase - Scripting
- [ ] Visual scripting nodes (future)
- [ ] C++ hot-reload support (complex)
- [ ] Lua/Python scripting integration

### Future Phase - Animation
- [ ] Sprite animation component
- [ ] Animation timeline editor
- [ ] Bone/skeletal animation (2D)
- [ ] Animation state machine

### Future Phase - Audio
- [ ] Audio component
- [ ] Audio listener
- [ ] Sound effect triggers
- [ ] Background music management

### Future Phase - Tilemaps
- [ ] Tilemap component
- [ ] Tile palette panel
- [ ] Brush tools (paint, erase, fill)
- [ ] Auto-tiling rules

### Future Phase - Prefabs
- [ ] Save entity as prefab asset
- [ ] Prefab instantiation
- [ ] Prefab overrides
- [ ] Nested prefabs

### Future Phase - Multi-View
- [ ] Multiple viewport panels
- [ ] Split view (horizontal/vertical)
- [ ] Game preview window

---

## Prioritized Implementation Roadmap

### Sprint 1: Critical Foundation (1-2 weeks)
**Goal:** Make the editor actually usable for game development

| Task | Priority | Est. Days | Dependencies |
|------|----------|-----------|--------------|
| FEAT-003: Viewport Entity Picking | P0 | 2 | None |
| FEAT-004: Native File Dialogs | P0 | 1 | None |
| BUG-002: Complete Inspector Components | P0 | 1 | None |
| BUG-003: Complete Add Component Menu | P0 | 0.5 | None |
| BUG-001: Selection Visual Improvement | P0 | 0.5 | None |

### Sprint 2: Transform Tools (1-2 weeks)
**Goal:** Visual entity manipulation

| Task | Priority | Est. Days | Dependencies |
|------|----------|-----------|--------------|
| FEAT-001: ImGuizmo Integration | P0 | 1 | None |
| FEAT-001: Translate Gizmo | P0 | 1 | ImGuizmo |
| FEAT-001: Rotate Gizmo | P0 | 0.5 | ImGuizmo |
| FEAT-001: Scale Gizmo | P0 | 0.5 | ImGuizmo |
| FEAT-009: Grid Snapping | P1 | 1 | Gizmos |

### Sprint 3: Undo/Redo (1-2 weeks)
**Goal:** Safe editing with rollback

| Task | Priority | Est. Days | Dependencies |
|------|----------|-----------|--------------|
| FEAT-002: Command Pattern Infrastructure | P0 | 2 | None |
| FEAT-002: Transform Commands | P0 | 1 | Command Pattern |
| FEAT-002: Entity Create/Delete Commands | P0 | 1 | Command Pattern |
| FEAT-002: Component Add/Remove Commands | P0 | 1 | Command Pattern |

### Sprint 4: Visual Components (1-2 weeks)
**Goal:** Entities can have visual appearance

| Task | Priority | Est. Days | Dependencies |
|------|----------|-----------|--------------|
| FEAT-005: SpriteComponent | P1 | 2 | None |
| FEAT-005: Sprite Inspector UI | P1 | 1 | SpriteComponent |
| FEAT-006: CameraComponent | P1 | 2 | None |
| QOL-001: Collider Visualization | P2 | 1 | None |

### Sprint 5: Polish & QOL (1-2 weeks)
**Goal:** Professional feel

| Task | Priority | Est. Days | Dependencies |
|------|----------|-----------|--------------|
| QOL-003: Hierarchy Search | P2 | 0.5 | None |
| QOL-004: Content Browser Thumbnails | P2 | 2 | None |
| QOL-006: Editor Preferences Panel | P2 | 1 | None |
| QOL-007: Status Bar | P2 | 0.5 | None |
| FEAT-010: Copy/Paste Entities | P2 | 1 | None |

---

## Technical Debt

### Code Quality Issues
1. **Magic Numbers:** Many hardcoded values in UI code should be constants
2. **ImGui ID Conflicts:** Some panels might have ID conflicts
3. **Resource Management:** Framebuffer resize could be optimized
4. **Event System:** Some events not properly consumed
5. **Selection Validation:** Dead entities could remain in selection

### Architecture Improvements Needed
1. **Panel System:** Consider more generic panel registration
2. **Command System:** Design for extensibility
3. **Asset System:** Proper asset database with UUIDs
4. **Settings Persistence:** Save/load editor preferences

### Missing Tests
1. Selection context unit tests
2. Command undo/redo tests
3. Scene serialization edge cases
4. Viewport coordinate conversion tests

---

## Conclusion

The Pillar Editor has a strong foundation but requires several critical features before it can be considered production-ready for game development. The prioritized roadmap focuses on:

1. **Immediate:** Make entities selectable in viewport, fix file dialogs
2. **Short-term:** Add transform gizmos for visual editing
3. **Medium-term:** Implement undo/redo for safe workflow
4. **Long-term:** Add visual components and polish

Following this roadmap will transform the editor from a functional prototype into a usable game development tool.

---

## Appendix: Reference Implementation Links

- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) - Transform gizmo library
- [nativefiledialog-extended](https://github.com/btzy/nativefiledialog-extended) - Cross-platform file dialogs
- [Hazel Engine](https://github.com/TheCherno/Hazel) - Reference implementation
- [Godot Editor](https://github.com/godotengine/godot) - Feature reference
