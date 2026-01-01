# Pillar Engine Editor - Implementation Plan

## Overview

This document outlines the design and implementation plan for integrating a full-featured visual editor into the Pillar Engine. The editor will be built as a separate application (`PillarEditor`) that uses the core engine library, providing a professional UI for scene editing, entity management, and game development workflows.

---

## Table of Contents

1. [Current System Analysis](#current-system-analysis)
2. [Editor Architecture](#editor-architecture)
3. [Core Systems to Implement](#core-systems-to-implement)
4. [Implementation Phases](#implementation-phases)
5. [UI/UX Design](#uiux-design)
6. [File Structure](#file-structure)
7. [Dependencies](#dependencies)
8. [Technical Considerations](#technical-considerations)

---

## Current System Analysis

### Existing Infrastructure

The Pillar Engine already has several systems that the editor will leverage:

| System | Status | Editor Usage |
|--------|--------|--------------|
| **ImGui (Docking Branch)** | ? Integrated | Full UI framework with docking support |
| **ECS (EnTT)** | ? Integrated | Entity-Component system for scene data |
| **Scene System** | ? Integrated | Scene management, creation, transitions |
| **SceneSerializer** | ? Integrated | JSON/Binary scene save/load |
| **ComponentRegistry** | ? Integrated | Extensible component serialization |
| **Renderer2D** | ? Integrated | 2D rendering with quads and textures |
| **OrthographicCamera** | ? Integrated | 2D camera with controller |
| **Input System** | ? Integrated | Keyboard/Mouse polling and events |
| **Event System** | ? Integrated | Application/Input events |
| **Layer System** | ? Integrated | Layer-based update/render pipeline |
| **Framebuffer** | ? **IMPLEMENTED** | Render scene to viewport texture |

### Key Observations

1. **ImGui Docking** is already enabled with a basic dockspace setup in `ImGuiLayer.cpp`
2. **SceneDemoLayer** already demonstrates entity inspection and scene serialization UI
3. **ComponentRegistry** allows extensible component serialization (essential for inspector)
4. ~~**Missing Framebuffer**~~ - ? Now implemented!

---

## Implementation Progress

### ? Phase 1: Foundation (COMPLETED)

| Task | Status |
|------|--------|
| Create `PillarEditor` project in CMakeLists.txt | ? Done |
| Implement `Framebuffer` abstraction and OpenGL implementation | ? Done |
| Create `EditorLayer` class with dockspace setup | ? Done |
| Implement `ViewportPanel` to render scene to ImGui window | ? Done |
| Add editor camera with pan/zoom controls | ? Done |
| Basic menu bar (File > New/Open/Save) | ? Done |

### ? Phase 2: Entity Management (COMPLETED)

| Task | Status |
|------|--------|
| Implement `SceneHierarchyPanel` | ? Done |
| - Tree view of all entities | ? Done |
| - Context menu (Create, Delete, Duplicate) | ? Done |
| Implement `InspectorPanel` | ? Done |
| - Component display | ? Done |
| - Editable fields (Transform, Velocity, etc.) | ? Done |
| - Add Component button | ? Done |
| Implement `SelectionContext` | ? Done |
| - Click-to-select in hierarchy | ? Done |
| - Multi-select with Ctrl | ? Done |

### ? Additional Features Implemented

| Feature | Status |
|---------|--------|
| `ContentBrowserPanel` - Asset browser | ? Done |
| `ConsolePanel` - Log output panel | ? Done |
| Play/Pause/Stop functionality | ? Done |
| Stats panel (FPS, entity count) | ? Done |
| Keyboard shortcuts (Ctrl+N/O/S, Delete, F, Ctrl+D) | ? Done |
| Scene state backup/restore for play mode | ? Done |

---

## Editor Architecture

### High-Level Architecture

```
???????????????????????????????????????????????????????????????????????
?                         PillarEditor Application                     ?
???????????????????????????????????????????????????????????????????????
?  ???????????????????  ???????????????????  ???????????????????     ?
?  ?   EditorLayer   ?  ?  ImGuiLayer     ?  ?   Other Layers  ?     ?
?  ?  (Main Editor)  ?  ?  (Dockspace)    ?  ?   (Optional)    ?     ?
?  ???????????????????  ???????????????????  ???????????????????     ?
?           ?                    ?                    ?               ?
?           ???????????????????????????????????????????               ?
?                                ?                                    ?
?  ????????????????????????????????????????????????????????????????? ?
?  ?                        Panel System                            ? ?
?  ?  ???????????? ???????????? ???????????? ????????????         ? ?
?  ?  ? Viewport ? ?Hierarchy ? ?Inspector ? ? Content  ?  ...    ? ?
?  ?  ?  Panel   ? ?  Panel   ? ?  Panel   ? ? Browser  ?         ? ?
?  ?  ???????????? ???????????? ???????????? ????????????         ? ?
?  ????????????????????????????????????????????????????????????????? ?
???????????????????????????????????????????????????????????????????????
?                         Pillar Engine Library                        ?
?  ?????????? ?????????? ?????????? ?????????? ??????????           ?
?  ?Renderer? ?  ECS   ? ? Input  ? ? Events ? ? Scene  ?  ...      ?
?  ?????????? ?????????? ?????????? ?????????? ??????????           ?
???????????????????????????????????????????????????????????????????????
```

### Editor Modes

The editor will support multiple operational modes:

1. **Edit Mode** - Scene editing, entity manipulation
2. **Play Mode** - Run the game within the editor
3. **Pause Mode** - Paused game state for debugging

---

## Core Systems to Implement

### 1. Framebuffer System (CRITICAL - Phase 1)

**Purpose:** Render the scene to a texture for display in ImGui viewport.

**Interface:**
```cpp
// Pillar/Renderer/Framebuffer.h
namespace Pillar {

    struct FramebufferSpecification
    {
        uint32_t Width = 1280;
        uint32_t Height = 720;
        uint32_t Samples = 1;           // MSAA samples
        bool SwapChainTarget = false;   // Render to screen?
    };

    class Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void Resize(uint32_t width, uint32_t height) = 0;

        virtual uint32_t GetColorAttachmentRendererID() const = 0;
        virtual const FramebufferSpecification& GetSpecification() const = 0;

        static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);
    };

}
```

**OpenGL Implementation:**
```cpp
// Platform/OpenGL/OpenGLFramebuffer.h
class OpenGLFramebuffer : public Framebuffer
{
    // FBO, color attachment (texture), depth/stencil RBO
};
```

### 2. Editor Camera System

**Purpose:** Separate editor camera that doesn't affect game camera.

**Features:**
- Pan with middle mouse button
- Zoom with scroll wheel
- Focus on selected entity (F key)
- 2D orthographic view

### 3. Panel System

**Purpose:** Modular, dockable UI panels.

**Base Class:**
```cpp
// Editor/Panels/EditorPanel.h
class EditorPanel
{
public:
    virtual ~EditorPanel() = default;
    
    virtual void OnImGuiRender() = 0;
    virtual void OnEvent(Event& e) {}
    
    void SetContext(const std::shared_ptr<Scene>& scene) { m_Scene = scene; }
    
    const std::string& GetName() const { return m_Name; }
    bool IsVisible() const { return m_Visible; }
    void SetVisible(bool visible) { m_Visible = visible; }

protected:
    std::string m_Name = "Panel";
    bool m_Visible = true;
    std::shared_ptr<Scene> m_Scene;
};
```

### 4. Selection System

**Purpose:** Track selected entities for manipulation.

```cpp
// Editor/SelectionContext.h
class SelectionContext
{
public:
    void Select(Entity entity);
    void Deselect();
    void AddToSelection(Entity entity);  // Multi-select
    void RemoveFromSelection(Entity entity);
    
    Entity GetPrimarySelection() const;
    const std::vector<Entity>& GetSelection() const;
    bool IsSelected(Entity entity) const;
    
    // Events
    using SelectionChangedCallback = std::function<void()>;
    void SetOnSelectionChanged(SelectionChangedCallback callback);

private:
    std::vector<Entity> m_Selection;
    SelectionChangedCallback m_OnSelectionChanged;
};
```

### 5. Gizmo System (Transform Manipulation)

**Purpose:** Visual handles for moving, rotating, and scaling entities.

**Implementation Options:**
- Use **ImGuizmo** library (recommended for quick implementation)
- Custom gizmo rendering (more control, more work)

```cpp
// Editor/Gizmos/TransformGizmo.h
enum class GizmoOperation { Translate, Rotate, Scale };
enum class GizmoMode { Local, World };

class TransformGizmo
{
public:
    void SetOperation(GizmoOperation op);
    void SetMode(GizmoMode mode);
    
    void Render(Entity entity, const OrthographicCamera& camera);
    bool IsUsing() const;  // Is user currently dragging gizmo?
};
```

---

## Implementation Phases

### Phase 1: Foundation (Week 1-2)

**Goal:** Basic editor structure with scene viewport rendering.

**Tasks:**
1. [x] Create `PillarEditor` project in CMakeLists.txt
2. [x] Implement `Framebuffer` abstraction and OpenGL implementation
3. [x] Create `EditorLayer` class with basic dockspace setup
4. [x] Implement `ViewportPanel` to render scene to ImGui window
5. [x] Add editor camera with pan/zoom controls
6. [x] Basic menu bar (File > New/Open/Save, Edit > Undo/Redo placeholder)

**Deliverables:**
- Scene renders inside resizable ImGui viewport
- Editor camera independent from game camera
- Scene can be saved/loaded from menu

### Phase 2: Entity Management (Week 3-4)

**Goal:** Full entity hierarchy and component inspection.

**Tasks:**
1. [x] Implement `SceneHierarchyPanel`
   - Tree view of all entities
   - Drag-drop for parenting (if HierarchyComponent exists)
   - Context menu (Create Entity, Delete, Duplicate)
2. [x] Implement `InspectorPanel` (Properties Panel)
   - Component display using ComponentRegistry
   - Editable fields (position, rotation, scale, etc.)
   - Add Component button with component list
3. [x] Implement `SelectionContext`
   - Click-to-select in hierarchy
   - Click-to-select in viewport (entity picking)
4. [x] Entity creation with default components

**Deliverables:**
- Full entity hierarchy tree
- Component inspection and editing
- Entity selection system

### Phase 3: Content Browser & Assets (Week 5-6)

**Goal:** Asset management and project structure.

**Tasks:**
1. [ ] Implement `ContentBrowserPanel`
   - Directory tree navigation
   - File/folder icons
   - Double-click to open assets
   - Drag-drop textures onto entities
2. [ ] Asset thumbnails (textures)
3. [ ] Scene file management (`.scene.json` files)
4. [ ] Project structure concept (optional)

**Deliverables:**
- Browse project assets visually
- Drag textures to assign to components

### Phase 4: Gizmos & Manipulation (Week 7-8)

**Goal:** Visual entity manipulation in viewport.

**Tasks:**
1. [ ] Integrate ImGuizmo library (or implement custom)
2. [ ] Translate gizmo (move entities)
3. [ ] Rotate gizmo (2D rotation)
4. [ ] Scale gizmo
5. [ ] Keyboard shortcuts (W=Translate, E=Rotate, R=Scale)
6. [ ] Snap to grid option

**Deliverables:**
- Transform entities visually with gizmos
- Keyboard shortcuts for gizmo modes

### Phase 5: Play Mode & Runtime (Week 9-10)

**Goal:** Test game within editor.

**Tasks:**
1. [ ] Implement Play/Pause/Stop buttons
2. [ ] Scene state backup before play (copy scene)
3. [ ] Restore scene state on stop
4. [ ] Runtime simulation (physics, gameplay systems)
5. [ ] Play mode visual indicator (tinted viewport border)

**Deliverables:**
- Play button runs game in editor
- Stop returns to edit state

### Phase 6: Polish & Extended Features (Week 11-12)

**Goal:** Professional touches and additional features.

**Tasks:**
1. [ ] Undo/Redo system (command pattern)
2. [ ] Console/Log panel
3. [ ] Statistics panel (FPS, draw calls, entity count)
4. [ ] Custom editor theme (dark theme refinements)
5. [ ] Keyboard shortcuts documentation
6. [ ] Settings/Preferences panel
7. [ ] Layout save/restore

**Deliverables:**
- Professional, polished editor experience

---

## UI/UX Design

### Default Layout

```
???????????????????????????????????????????????????????????????????????????
?  File   Edit   View   Entity   Window   Help                            ?
???????????????????????????????????????????????????????????????????????????
?         ?                                           ?                   ?
? SCENE   ?                                           ?    INSPECTOR      ?
?HIERARCHY?              VIEWPORT                     ?                   ?
?         ?                                           ?  [TagComponent]   ?
? ? Root  ?        ???????????????????????           ?  Tag: Player      ?
?   ? Player       ?                     ?           ?                   ?
?   ? Enemy        ?   Scene Rendering   ?           ?  [Transform]      ?
?   ? Camera       ?                     ?           ?  Position: x y    ?
?   ? Ground       ?                     ?           ?  Rotation: deg    ?
?         ?        ???????????????????????           ?  Scale: x y       ?
?         ?                                           ?                   ?
?         ?  ? Play  ? Pause  ? Stop   [W][E][R]    ?  [+ Add Component]?
???????????????????????????????????????????????????????????????????????????
?                         CONTENT BROWSER                                  ?
?  ?? Assets / ?? Textures / ?? Scenes                                    ?
?  [player.png] [enemy.png] [tileset.png] [main.scene.json]              ?
???????????????????????????????????????????????????????????????????????????
?                              CONSOLE                                     ?
?  [INFO] Scene loaded: MainScene                                         ?
?  [WARN] Texture not found: missing.png                                  ?
???????????????????????????????????????????????????????????????????????????
```

### Color Scheme (Dark Theme)

| Element | Color (Hex) | Description |
|---------|-------------|-------------|
| Background | `#1E1E1E` | Main window background |
| Panel Background | `#252526` | Panel backgrounds |
| Header | `#333333` | Panel headers, menu |
| Accent | `#007ACC` | Selected items, buttons |
| Text | `#D4D4D4` | Primary text |
| Text Dim | `#808080` | Secondary/disabled text |
| Error | `#F44747` | Error messages |
| Warning | `#CCA700` | Warning messages |
| Success | `#4EC9B0` | Success/info messages |

### Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New Scene |
| `Ctrl+O` | Open Scene |
| `Ctrl+S` | Save Scene |
| `Ctrl+Shift+S` | Save Scene As |
| `Ctrl+Z` | Undo |
| `Ctrl+Y` | Redo |
| `Delete` | Delete Selected Entity |
| `Ctrl+D` | Duplicate Entity |
| `F` | Focus on Selected |
| `W` | Translate Gizmo |
| `E` | Rotate Gizmo |
| `R` | Scale Gizmo |
| `Ctrl+P` | Play/Stop |
| `Ctrl+Shift+P` | Pause |

---

## File Structure

### New Editor Project Structure

```
PillarEditor/
??? src/
?   ??? EditorApp.cpp                    # Entry point, creates EditorApplication
?   ??? EditorLayer.h                    # Main editor layer
?   ??? EditorLayer.cpp
?   ??? EditorApplication.h              # Editor-specific Application subclass
?   ??? EditorApplication.cpp
?   ??? SelectionContext.h               # Entity selection management
?   ??? SelectionContext.cpp
?   ??? EditorCamera.h                   # Editor-specific camera
?   ??? EditorCamera.cpp
?   ??? Panels/
?   ?   ??? EditorPanel.h                # Base panel class
?   ?   ??? ViewportPanel.h              # Scene viewport
?   ?   ??? ViewportPanel.cpp
?   ?   ??? SceneHierarchyPanel.h        # Entity tree
?   ?   ??? SceneHierarchyPanel.cpp
?   ?   ??? InspectorPanel.h             # Component properties
?   ?   ??? InspectorPanel.cpp
?   ?   ??? ContentBrowserPanel.h        # Asset browser
?   ?   ??? ContentBrowserPanel.cpp
?   ?   ??? ConsolePanel.h               # Log output
?   ?   ??? ConsolePanel.cpp
?   ?   ??? StatsPanel.h                 # Performance stats
?   ?       StatsPanel.cpp
?   ??? Gizmos/
?   ?   ??? TransformGizmo.h
?   ?   ??? TransformGizmo.cpp
?   ??? Commands/                        # Undo/Redo (Phase 6)
?       ??? Command.h
?       ??? CommandHistory.h
?       ??? TransformCommand.cpp
??? assets/
?   ??? icons/                           # Editor icons
?   ?   ??? folder.png
?   ?   ??? file.png
?   ?   ??? play.png
?   ?   ??? pause.png
?   ?   ??? stop.png
?   ??? fonts/                           # Custom fonts (optional)
?       ??? Roboto-Regular.ttf
??? CMakeLists.txt
```

### Engine Additions

```
Pillar/src/
??? Pillar/
?   ??? Renderer/
?       ??? Framebuffer.h                # NEW - Framebuffer interface
?       ??? Framebuffer.cpp              # NEW - Factory
??? Platform/
?   ??? OpenGL/
?       ??? OpenGLFramebuffer.h          # NEW - OpenGL FBO
?       ??? OpenGLFramebuffer.cpp        # NEW
```

---

## Dependencies

### New Dependencies Required

| Library | Purpose | Integration Method |
|---------|---------|-------------------|
| **ImGuizmo** | Transform gizmos | FetchContent or vendored |
| **nfd (Native File Dialog)** | Open/Save dialogs | FetchContent (optional) |
| **IconFontCppHeaders** | Icon fonts for UI | Header-only, vendored |

### ImGuizmo Integration (CMake)

```cmake
# In root CMakeLists.txt
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG master
)
FetchContent_MakeAvailable(imguizmo)

# Create library target
add_library(imguizmo STATIC
    ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
)
target_include_directories(imguizmo PUBLIC ${imguizmo_SOURCE_DIR})
target_link_libraries(imguizmo PUBLIC imgui)
```

---

## Technical Considerations

### 1. Framebuffer Resize Handling

When the viewport panel is resized, the framebuffer must be resized to match:

```cpp
void ViewportPanel::OnImGuiRender()
{
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    if (m_ViewportSize != glm::vec2(viewportSize.x, viewportSize.y))
    {
        m_ViewportSize = { viewportSize.x, viewportSize.y };
        m_Framebuffer->Resize((uint32_t)viewportSize.x, (uint32_t)viewportSize.y);
        m_EditorCamera.SetViewportSize(viewportSize.x, viewportSize.y);
    }
    
    // Render framebuffer texture
    uint32_t textureID = m_Framebuffer->GetColorAttachmentRendererID();
    ImGui::Image((void*)(intptr_t)textureID, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
}
```

### 2. Entity Picking (Mouse Selection in Viewport)

Two approaches:

**Option A: Color-based picking (Simple)**
- Render each entity with unique color ID to separate framebuffer
- Read pixel at mouse position to determine clicked entity

**Option B: Ray casting (More accurate for 2D)**
- Convert mouse position to world coordinates
- Check intersection with entity bounds

### 3. Component Inspector UI Generation

Leverage `ComponentRegistry` for dynamic UI:

```cpp
void InspectorPanel::DrawComponents(Entity entity)
{
    auto& registry = ComponentRegistry::Get();
    
    for (auto& [key, reg] : registry.GetRegistrations())
    {
        // Check if entity has this component via serialize returning non-null
        json data = reg.Serialize(entity);
        if (!data.is_null())
        {
            if (ImGui::CollapsingHeader(key.c_str()))
            {
                DrawComponentUI(entity, key, data, reg);
            }
        }
    }
}
```

### 4. Undo/Redo System (Command Pattern)

```cpp
class Command
{
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetName() const = 0;
};

class TransformCommand : public Command
{
public:
    TransformCommand(Entity entity, TransformComponent oldTransform, TransformComponent newTransform);
    void Execute() override;
    void Undo() override;
    
private:
    Entity m_Entity;
    TransformComponent m_OldTransform;
    TransformComponent m_NewTransform;
};

class CommandHistory
{
public:
    void Execute(std::unique_ptr<Command> command);
    void Undo();
    void Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    
private:
    std::vector<std::unique_ptr<Command>> m_UndoStack;
    std::vector<std::unique_ptr<Command>> m_RedoStack;
};
```

### 5. Play Mode Scene Backup

```cpp
class EditorLayer : public Layer
{
    void OnPlayButtonPressed()
    {
        // Deep copy scene before playing
        m_EditorScene = m_ActiveScene;
        m_RuntimeScene = Scene::Copy(m_ActiveScene);
        m_ActiveScene = m_RuntimeScene;
        m_ActiveScene->OnRuntimeStart();
        m_EditorState = EditorState::Play;
    }
    
    void OnStopButtonPressed()
    {
        m_ActiveScene->OnRuntimeStop();
        m_ActiveScene = m_EditorScene;
        m_RuntimeScene.reset();
        m_EditorState = EditorState::Edit;
    }

private:
    std::shared_ptr<Scene> m_ActiveScene;
    std::shared_ptr<Scene> m_EditorScene;    // Backup for edit mode
    std::shared_ptr<Scene> m_RuntimeScene;   // Runtime copy
    EditorState m_EditorState = EditorState::Edit;
};
```

---

## Success Criteria

### Minimum Viable Product (MVP)

1. ? Scene renders in viewport panel
2. ? Entity hierarchy panel with selection
3. ? Inspector panel showing components
4. ? Save/Load scenes from menu
5. ? Basic transform editing (position, rotation, scale sliders)

### Full Feature Set

1. All MVP features
2. Transform gizmos (translate, rotate, scale)
3. Content browser with asset thumbnails
4. Play/Pause/Stop functionality
5. Undo/Redo system
6. Console/Log panel
7. Custom dark theme
8. Keyboard shortcuts

---

## Estimated Timeline

| Phase | Duration | Key Deliverables |
|-------|----------|------------------|
| Phase 1 | 2 weeks | Framebuffer, Viewport, Editor structure |
| Phase 2 | 2 weeks | Hierarchy, Inspector, Selection |
| Phase 3 | 2 weeks | Content Browser, Asset management |
| Phase 4 | 2 weeks | Gizmos, Transform manipulation |
| Phase 5 | 2 weeks | Play mode, Runtime integration |
| Phase 6 | 2 weeks | Polish, Undo/Redo, Console |
| **Total** | **~12 weeks** | Full-featured editor |

---

## Next Steps

1. **Create Framebuffer classes** (highest priority - blocks viewport)
2. **Set up PillarEditor project** in CMake
3. **Implement ViewportPanel** with basic scene rendering
4. **Test with existing SceneDemoLayer** content

---

## References

- [ImGui Docking Documentation](https://github.com/ocornut/imgui/wiki/Docking)
- [ImGuizmo Library](https://github.com/CedricGuillemet/ImGuizmo)
- [LearnOpenGL - Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)
- [EnTT ECS Documentation](https://github.com/skypjack/entt/wiki)
- [The Cherno - Game Engine Series](https://www.youtube.com/playlist?list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT) (Hazel Engine)
