# Pillar Editor — Lighting System Integration Plan

**Author:** Engine Team  
**Date:** 2026-01-06  
**Status:** Planning  
**Branch:** `feature/editor-lighting`

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Current Lighting System Analysis](#2-current-lighting-system-analysis)
3. [Current Editor Architecture Analysis](#3-current-editor-architecture-analysis)
4. [Integration Requirements](#4-integration-requirements)
5. [Implementation Plan](#5-implementation-plan)
6. [Detailed Task Breakdown](#6-detailed-task-breakdown)
7. [Technical Specifications](#7-technical-specifications)
8. [Testing Strategy](#8-testing-strategy)
9. [Risk Assessment](#9-risk-assessment)
10. [Timeline Estimate](#10-timeline-estimate)

---

## 1. Executive Summary

This document outlines the plan to integrate Pillar's 2D lighting system into the editor. The goal is to enable level designers to author lights and shadow casters visually within the editor viewport, with real-time preview of the lit scene.

### Key Deliverables

1. **Inspector UI** for `Light2DComponent` and `ShadowCaster2DComponent`
2. **Viewport Integration** — lit scene preview in edit mode with toggleable lighting
3. **Gizmo Rendering** — visual representations of lights (radius, cone, direction)
4. **Scene Serialization** — save/load lighting components
5. **Settings Panel** — global ambient light and shadow configuration
6. **Play Mode Integration** — lighting system active during Play mode

---

## 2. Current Lighting System Analysis

### 2.1 Core Architecture

The lighting system consists of three main layers:

| Layer | Files | Purpose |
|-------|-------|---------|
| **Renderer API** | `Lighting2D.h/cpp` | Static API for lit rendering pipeline |
| **Geometry Helpers** | `Lighting2DGeometry.h/cpp` | Shadow volume triangle generation |
| **ECS Integration** | `Lighting2DSystem.h/cpp` | Collects components and submits to renderer |

### 2.2 Lighting2D Renderer API

**Location:** `Pillar/src/Pillar/Renderer/Lighting2D.h`

```cpp
namespace Pillar {
    class Lighting2D {
    public:
        static void Init();
        static void Shutdown();
        
        // Begin a lit scene - binds internal framebuffers
        static void BeginScene(const OrthographicCamera& camera,
                               uint32_t viewportWidth,
                               uint32_t viewportHeight,
                               const Lighting2DSettings& settings = {});
        
        // Overload that composites into a provided output framebuffer
        static void BeginScene(const OrthographicCamera& camera,
                               const std::shared_ptr<Framebuffer>& outputFramebuffer,
                               const Lighting2DSettings& settings = {});
        
        static void SubmitLight(const Light2DSubmit& light);
        static void SubmitShadowCaster(const ShadowCaster2DSubmit& caster);
        static void EndScene();
    };
}
```

**Critical Notes:**
- `BeginScene` internally calls `Renderer2DBackend::BeginScene()` — the editor must draw sprites **between** `Lighting2D::BeginScene` and `Lighting2D::EndScene`
- Supports an output framebuffer overload — essential for editor viewport integration
- Uses internal `SceneColorFramebuffer` + `LightAccumFramebuffer` for the multi-pass pipeline

### 2.3 ECS Components

#### Light2DComponent
**Location:** `Pillar/src/Pillar/ECS/Components/Rendering/Light2DComponent.h`

```cpp
struct Light2DComponent {
    Light2DType Type = Light2DType::Point;  // Point or Spot
    glm::vec3 Color{ 1.0f, 0.85f, 0.6f };
    float Intensity = 1.0f;
    float Radius = 6.0f;
    
    // Spot light only
    float InnerAngleRadians = 0.25f;
    float OuterAngleRadians = 0.5f;
    
    bool CastShadows = true;
    float ShadowStrength = 1.0f;
    uint32_t LayerMask = 0xFFFFFFFFu;
};
```

#### ShadowCaster2DComponent
**Location:** `Pillar/src/Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h`

```cpp
struct ShadowCaster2DComponent {
    std::vector<glm::vec2> Points;  // Local-space, CCW order
    bool Closed = true;
    bool TwoSided = false;
    uint32_t LayerMask = 0xFFFFFFFFu;
};
```

### 2.4 Lighting2DSystem

**Location:** `Pillar/src/Pillar/ECS/Systems/Lighting2DSystem.cpp`

The system iterates all entities with `Light2DComponent` and `ShadowCaster2DComponent`, transforms local points to world space, and submits them to the `Lighting2D` renderer.

**Key Pattern:**
```cpp
void Lighting2DSystem::OnUpdate(float dt) {
    // Submit lights
    auto lightView = registry.view<TransformComponent, Light2DComponent>();
    for (auto entity : lightView) {
        Light2DSubmit light;
        // ... populate from component + transform
        Lighting2D::SubmitLight(light);
    }
    
    // Submit shadow casters
    auto casterView = registry.view<TransformComponent, ShadowCaster2DComponent>();
    for (auto entity : casterView) {
        ShadowCaster2DSubmit caster;
        // ... transform points to world space
        Lighting2D::SubmitShadowCaster(caster);
    }
}
```

### 2.5 Rendering Pipeline Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    Lighting2D Pipeline                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  BeginScene(camera, outputFBO, settings)                        │
│       │                                                         │
│       ▼                                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Pass A: Scene Color                                     │   │
│  │  - Binds SceneColorFramebuffer                          │   │
│  │  - Calls Renderer2DBackend::BeginScene(camera)          │   │
│  │  - User draws sprites here                              │   │
│  └─────────────────────────────────────────────────────────┘   │
│       │                                                         │
│       │  SubmitLight(...) / SubmitShadowCaster(...)            │
│       ▼                                                         │
│  EndScene()                                                     │
│       │                                                         │
│       ▼                                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Pass B: Light Accumulation                              │   │
│  │  - Clear to ambient color                                │   │
│  │  - For each light:                                       │   │
│  │    - Build shadow volume triangles                       │   │
│  │    - Render shadow volumes to stencil                    │   │
│  │    - Render light quad with stencil test                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│       │                                                         │
│       ▼                                                         │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Pass C: Composite                                       │   │
│  │  - output = SceneColor * LightAccum                      │   │
│  │  - Writes to outputFBO (or backbuffer)                   │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Current Editor Architecture Analysis

### 3.1 Editor Layer Structure

**Main Entry:** `PillarEditor/src/EditorLayer.h/cpp`

The editor follows a panel-based architecture:

```
EditorLayer
├── SceneHierarchyPanel  — Entity tree view
├── InspectorPanel       — Component editing (KEY FOR LIGHTING)
├── ViewportPanel        — Scene rendering (KEY FOR LIGHTING)
├── ContentBrowserPanel  — Asset management
├── ConsolePanel         — Logging
├── TemplateLibraryPanel — Entity templates
├── AnimationManagerPanel— Animation editing
├── SpriteSheetEditorPanel— Sprite atlas editing
└── LayerEditorPanel     — Sprite layer management
```

### 3.2 ViewportPanel Analysis

**Location:** `PillarEditor/src/Panels/ViewportPanel.h/cpp`

**Current Rendering Flow:**
```cpp
void ViewportPanel::RenderScene() {
    m_Framebuffer->Bind();
    RenderCommand::SetClearColor(...);
    RenderCommand::Clear();
    
    Renderer2DBackend::BeginScene(*activeCamera);
    
    // Draw grid
    DrawGrid();
    
    // Draw all entities (sorted by Z-index)
    for (auto entity : sortedEntities) {
        Renderer2DBackend::DrawQuad(...) or DrawRotatedQuad(...);
    }
    
    // Draw collider/rigidbody gizmos
    if (m_ShowColliderGizmos) DrawColliderGizmos();
    if (m_ShowRigidbodyGizmos) DrawRigidbodyGizmos();
    
    Renderer2DBackend::EndScene();
    m_Framebuffer->Unbind();
}
```

**Key Members:**
- `m_Framebuffer` — Editor viewport framebuffer (displayed via ImGui)
- `m_EditorCamera` — Free-flying orthographic camera
- `m_ShowColliderGizmos`, `m_ShowRigidbodyGizmos` — Toggle flags for gizmo display

**Gizmo Rendering Pattern:**
The viewport already has infrastructure for drawing debug overlays (colliders, rigidbodies). We follow this pattern for light gizmos.

### 3.3 InspectorPanel Analysis

**Location:** `PillarEditor/src/Panels/InspectorPanel.h/cpp`

**Component Drawing Pattern:**
```cpp
void InspectorPanel::DrawComponents(Entity entity) {
    DrawTagComponent(entity);
    DrawTransformComponent(entity);
    
    if (entity.HasComponent<SpriteComponent>())
        DrawSpriteComponent(entity);
    
    if (entity.HasComponent<CameraComponent>())
        DrawCameraComponent(entity);
    
    // ... more components
    
    DrawAddComponentButton(entity);
}
```

Each component has a dedicated `Draw*Component()` method that:
1. Uses `DrawComponentHeader<T>()` for collapsible headers with remove button
2. Renders ImGui controls for each field
3. Tracks edits for undo/redo (using `CommandHistory`)

**Add Component Pattern:**
```cpp
void InspectorPanel::DrawAddComponentButton(Entity entity) {
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        if (!entity.HasComponent<SpriteComponent>()) {
            if (ImGui::Selectable("Sprite")) {
                entity.AddComponent<SpriteComponent>();
            }
        }
        // ... more components
    }
}
```

### 3.4 Scene Serialization

**Registry:** `Pillar/src/Pillar/ECS/ComponentRegistry.h`

Components register serialization functions:
```cpp
registry.Register<MyComponent>("myComponent",
    [](Entity e) -> json { /* serialize */ },
    [](Entity e, const json& j) { /* deserialize */ },
    [](Entity src, Entity dst) { /* copy */ }
);
```

**Existing Components Registered:**
- TagComponent, TransformComponent, UUIDComponent
- SpriteComponent, CameraComponent, AnimationComponent
- VelocityComponent, RigidbodyComponent, ColliderComponent
- BulletComponent, XPGemComponent, HierarchyComponent

**Light2DComponent and ShadowCaster2DComponent are NOT registered** — must be added.

### 3.5 Editor Settings

**Location:** `PillarEditor/src/EditorSettings.h/cpp`

Contains:
- `LayerManager` — sprite layer system
- `EditorSettings` — preferences (auto-save, grid, etc.)

**Relevant for Lighting:**
- Add global lighting settings (default ambient, shadow toggle)
- Persist editor lighting preview state

### 3.6 Play/Edit Mode

**EditorState Enum:**
```cpp
enum class EditorState {
    Edit = 0,
    Play,
    Pause
};
```

**Update Loop (EditorLayer::OnUpdate):**
- **Edit Mode:** Only renders scene, no system updates
- **Play Mode:** Updates all game systems (physics, animation, etc.)

**Lighting Integration Point:**
- Edit Mode: Optional lit preview (toggle)
- Play Mode: Lighting2DSystem should run

---

## 4. Integration Requirements

### 4.1 Functional Requirements

| ID | Requirement | Priority |
|----|-------------|----------|
| FR-01 | Add Light2D component via Inspector | High |
| FR-02 | Edit light properties (type, color, intensity, radius, angles) | High |
| FR-03 | Add ShadowCaster2D component via Inspector | High |
| FR-04 | Edit shadow caster points visually | Medium |
| FR-05 | Toggle lit preview in editor viewport | High |
| FR-06 | Configure global ambient light settings | High |
| FR-07 | Render light gizmos (radius circle, spot cone, direction arrow) | High |
| FR-08 | Serialize/deserialize lighting components | High |
| FR-09 | Lighting active during Play mode | High |
| FR-10 | Shadow caster polygon editing tool | Low (Phase 2) |

### 4.2 Non-Functional Requirements

| ID | Requirement |
|----|-------------|
| NFR-01 | Lit preview should not significantly impact editor responsiveness |
| NFR-02 | Light gizmos must be visible regardless of lighting state |
| NFR-03 | Consistent UX with existing component editing workflows |
| NFR-04 | Angles displayed in degrees in UI, stored as radians internally |

---

## 5. Implementation Plan

### 5.1 Phase 1: Core Integration (Essential)

**Goal:** Basic lighting authoring and preview capability.

```
Week 1-2:
├── Task 1.1: Initialize Lighting2D in EditorApp
├── Task 1.2: Register Light2DComponent serialization
├── Task 1.3: Register ShadowCaster2DComponent serialization
├── Task 1.4: Add Light2DComponent to Add Component menu
├── Task 1.5: Add ShadowCaster2DComponent to Add Component menu
├── Task 1.6: Implement DrawLight2DComponent() in InspectorPanel
├── Task 1.7: Implement DrawShadowCaster2DComponent() in InspectorPanel
└── Task 1.8: Basic viewport lighting toggle (Settings panel)
```

### 5.2 Phase 2: Viewport Integration

**Goal:** Visual feedback and lit rendering in editor.

```
Week 3:
├── Task 2.1: Modify ViewportPanel::RenderScene() to use Lighting2D pipeline
├── Task 2.2: Add Lighting2DSystem to EditorLayer (Play mode)
├── Task 2.3: Implement light gizmo rendering (point light radius)
├── Task 2.4: Implement spot light gizmo (cone + direction)
└── Task 2.5: Add lighting toggle to viewport toolbar
```

### 5.3 Phase 3: Polish & UX

**Goal:** Refined user experience and tools.

```
Week 4:
├── Task 3.1: Global ambient settings panel
├── Task 3.2: Layer mask editing UI (bitmask helper)
├── Task 3.3: Shadow caster point list editing
├── Task 3.4: "Generate from Collider" button for shadow casters
└── Task 3.5: Light icon rendering for unselected lights
```

---

## 6. Detailed Task Breakdown

### 6.1 Task 1.1: Initialize Lighting2D in EditorApp

**File:** `PillarEditor/src/EditorApp.cpp`

**Changes:**
```cpp
#include "Pillar/Renderer/Lighting2D.h"

class EditorApp : public Pillar::Application {
public:
    EditorApp() {
        // Existing initialization...
        Pillar::Lighting2D::Init();  // ADD THIS
        PushLayer(new EditorLayer());
    }
    
    ~EditorApp() {
        Pillar::Lighting2D::Shutdown();  // ADD THIS
    }
};
```

---

### 6.2 Task 1.2-1.3: Register Lighting Component Serialization

**File:** `Pillar/src/Pillar/ECS/BuiltinComponentRegistrations.cpp`

**Add Light2DComponent Registration:**
```cpp
#include "Components/Rendering/Light2DComponent.h"
#include "Components/Rendering/ShadowCaster2DComponent.h"

void RegisterBuiltinComponents() {
    auto& registry = ComponentRegistry::Get();
    
    // ... existing registrations ...
    
    // Light2DComponent
    registry.Register<Light2DComponent>("light2D",
        [](Entity e) -> json {
            if (!e.HasComponent<Light2DComponent>()) return nullptr;
            auto& c = e.GetComponent<Light2DComponent>();
            return json{
                { "type", static_cast<int>(c.Type) },
                { "color", { c.Color.r, c.Color.g, c.Color.b } },
                { "intensity", c.Intensity },
                { "radius", c.Radius },
                { "innerAngle", c.InnerAngleRadians },
                { "outerAngle", c.OuterAngleRadians },
                { "castShadows", c.CastShadows },
                { "shadowStrength", c.ShadowStrength },
                { "layerMask", c.LayerMask }
            };
        },
        [](Entity e, const json& j) {
            auto& c = e.AddComponent<Light2DComponent>();
            c.Type = static_cast<Light2DType>(j.value("type", 0));
            if (j.contains("color")) {
                c.Color = { j["color"][0], j["color"][1], j["color"][2] };
            }
            c.Intensity = j.value("intensity", 1.0f);
            c.Radius = j.value("radius", 6.0f);
            c.InnerAngleRadians = j.value("innerAngle", 0.25f);
            c.OuterAngleRadians = j.value("outerAngle", 0.5f);
            c.CastShadows = j.value("castShadows", true);
            c.ShadowStrength = j.value("shadowStrength", 1.0f);
            c.LayerMask = j.value("layerMask", 0xFFFFFFFFu);
        },
        [](Entity src, Entity dst) {
            if (!src.HasComponent<Light2DComponent>()) return;
            auto& s = src.GetComponent<Light2DComponent>();
            auto& d = dst.AddComponent<Light2DComponent>();
            d = s;  // POD-like copy
        }
    );
    
    // ShadowCaster2DComponent
    registry.Register<ShadowCaster2DComponent>("shadowCaster2D",
        [](Entity e) -> json {
            if (!e.HasComponent<ShadowCaster2DComponent>()) return nullptr;
            auto& c = e.GetComponent<ShadowCaster2DComponent>();
            json pointsArray = json::array();
            for (const auto& p : c.Points) {
                pointsArray.push_back({ p.x, p.y });
            }
            return json{
                { "points", pointsArray },
                { "closed", c.Closed },
                { "twoSided", c.TwoSided },
                { "layerMask", c.LayerMask }
            };
        },
        [](Entity e, const json& j) {
            auto& c = e.AddComponent<ShadowCaster2DComponent>();
            if (j.contains("points")) {
                for (const auto& p : j["points"]) {
                    c.Points.push_back({ p[0], p[1] });
                }
            }
            c.Closed = j.value("closed", true);
            c.TwoSided = j.value("twoSided", false);
            c.LayerMask = j.value("layerMask", 0xFFFFFFFFu);
        },
        [](Entity src, Entity dst) {
            if (!src.HasComponent<ShadowCaster2DComponent>()) return;
            auto& s = src.GetComponent<ShadowCaster2DComponent>();
            auto& d = dst.AddComponent<ShadowCaster2DComponent>();
            d = s;
        }
    );
}
```

---

### 6.3 Task 1.4-1.5: Add Components to Inspector Menu

**File:** `PillarEditor/src/Panels/InspectorPanel.cpp`

**In `DrawAddComponentButton()`:**
```cpp
void InspectorPanel::DrawAddComponentButton(Pillar::Entity entity) {
    // ... existing code ...
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        // ... existing components ...
        
        ImGui::Separator();
        ImGui::TextDisabled("Lighting:");
        
        if (!entity.HasComponent<Pillar::Light2DComponent>()) {
            if (ImGui::Selectable("Light 2D")) {
                entity.AddComponent<Pillar::Light2DComponent>();
                ImGui::CloseCurrentPopup();
            }
        }
        
        if (!entity.HasComponent<Pillar::ShadowCaster2DComponent>()) {
            if (ImGui::Selectable("Shadow Caster 2D")) {
                auto& caster = entity.AddComponent<Pillar::ShadowCaster2DComponent>();
                // Default to unit square for easier authoring
                caster.Points = {
                    { -0.5f, -0.5f },
                    {  0.5f, -0.5f },
                    {  0.5f,  0.5f },
                    { -0.5f,  0.5f }
                };
                ImGui::CloseCurrentPopup();
            }
        }
        
        ImGui::EndPopup();
    }
}
```

---

### 6.4 Task 1.6: Implement DrawLight2DComponent()

**File:** `PillarEditor/src/Panels/InspectorPanel.h`

**Add declaration:**
```cpp
void DrawLight2DComponent(Pillar::Entity entity);
```

**File:** `PillarEditor/src/Panels/InspectorPanel.cpp`

**Add include:**
```cpp
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
```

**Implementation:**
```cpp
void InspectorPanel::DrawLight2DComponent(Pillar::Entity entity)
{
    if (!entity.HasComponent<Pillar::Light2DComponent>())
        return;

    ImGui::PushID("Light2DComponent");

    bool open = DrawComponentHeader<Pillar::Light2DComponent>("Light 2D", entity);
    if (!open && !entity.HasComponent<Pillar::Light2DComponent>())
        return;

    if (open)
    {
        auto& light = entity.GetComponent<Pillar::Light2DComponent>();

        // === LIGHT TYPE ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Type");
        ImGui::NextColumn();
        
        const char* typeNames[] = { "Point", "Spot" };
        int currentType = static_cast<int>(light.Type);
        ImGui::PushItemWidth(-1);
        if (ImGui::Combo("##LightType", &currentType, typeNames, IM_ARRAYSIZE(typeNames)))
        {
            light.Type = static_cast<Pillar::Light2DType>(currentType);
        }
        ImGui::PopItemWidth();
        ImGui::Columns(1);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("💡 Appearance");
        ImGui::Separator();
        ImGui::Spacing();

        // === COLOR ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Color");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::ColorEdit3("##LightColor", &light.Color.x, ImGuiColorEditFlags_Float);
        ImGui::PopItemWidth();
        ImGui::Columns(1);

        // Color presets
        ImGui::Indent();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        if (ImGui::SmallButton("Warm")) { light.Color = { 1.0f, 0.85f, 0.6f }; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Cool")) { light.Color = { 0.7f, 0.85f, 1.0f }; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Fire")) { light.Color = { 1.0f, 0.5f, 0.2f }; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Neon")) { light.Color = { 0.4f, 1.0f, 0.8f }; }
        ImGui::PopStyleVar();
        ImGui::Unindent();

        // === INTENSITY ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Intensity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat("##LightIntensity", &light.Intensity, 0.05f, 0.0f, 10.0f, "%.2f");
        ImGui::PopItemWidth();
        ImGui::Columns(1);

        // === RADIUS ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::DragFloat("##LightRadius", &light.Radius, 0.1f, 0.1f, 100.0f, "%.1f");
        ImGui::PopItemWidth();
        ImGui::Columns(1);

        // === SPOT LIGHT PARAMETERS ===
        if (light.Type == Pillar::Light2DType::Spot)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text("🔦 Spot Light");
            ImGui::Separator();
            ImGui::Spacing();

            // Inner Angle (display in degrees)
            float innerDeg = glm::degrees(light.InnerAngleRadians);
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Inner Angle");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##InnerAngle", &innerDeg, 0.5f, 1.0f, 89.0f, "%.1f°"))
            {
                light.InnerAngleRadians = glm::radians(innerDeg);
                // Ensure inner <= outer
                if (light.InnerAngleRadians > light.OuterAngleRadians)
                    light.OuterAngleRadians = light.InnerAngleRadians;
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);

            // Outer Angle (display in degrees)
            float outerDeg = glm::degrees(light.OuterAngleRadians);
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Outer Angle");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if (ImGui::DragFloat("##OuterAngle", &outerDeg, 0.5f, 1.0f, 90.0f, "%.1f°"))
            {
                light.OuterAngleRadians = glm::radians(outerDeg);
                // Ensure outer >= inner
                if (light.OuterAngleRadians < light.InnerAngleRadians)
                    light.InnerAngleRadians = light.OuterAngleRadians;
            }
            ImGui::PopItemWidth();
            ImGui::Columns(1);
        }

        // === SHADOWS ===
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("🌑 Shadows");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Cast Shadows");
        ImGui::NextColumn();
        ImGui::Checkbox("##CastShadows", &light.CastShadows);
        ImGui::Columns(1);

        if (light.CastShadows)
        {
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
            ImGui::Text("Shadow Strength");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::SliderFloat("##ShadowStrength", &light.ShadowStrength, 0.0f, 1.0f, "%.2f");
            ImGui::PopItemWidth();
            ImGui::Columns(1);
        }

        // === LAYER MASK ===
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("🎭 Filtering");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Layer Mask");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Bitmask for filtering which sprites/casters this light affects");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputScalar("##LayerMask", ImGuiDataType_U32, &light.LayerMask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::PopItemWidth();
        ImGui::Columns(1);
    }

    ImGui::PopID();
}
```

**Call from `DrawComponents()`:**
```cpp
// In DrawComponents(), add:
if (entity.HasComponent<Pillar::Light2DComponent>())
{
    DrawLight2DComponent(entity);
}
```

---

### 6.5 Task 1.7: Implement DrawShadowCaster2DComponent()

```cpp
void InspectorPanel::DrawShadowCaster2DComponent(Pillar::Entity entity)
{
    if (!entity.HasComponent<Pillar::ShadowCaster2DComponent>())
        return;

    ImGui::PushID("ShadowCaster2DComponent");

    bool open = DrawComponentHeader<Pillar::ShadowCaster2DComponent>("Shadow Caster 2D", entity);
    if (!open && !entity.HasComponent<Pillar::ShadowCaster2DComponent>())
        return;

    if (open)
    {
        auto& caster = entity.GetComponent<Pillar::ShadowCaster2DComponent>();

        // === SHAPE OPTIONS ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Closed");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Whether the shape forms a closed polygon");
        ImGui::NextColumn();
        ImGui::Checkbox("##Closed", &caster.Closed);
        ImGui::Columns(1);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Two Sided");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Cast shadows from both sides of edges");
        ImGui::NextColumn();
        ImGui::Checkbox("##TwoSided", &caster.TwoSided);
        ImGui::Columns(1);

        // === LAYER MASK ===
        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, Inspector::COLUMN_WIDTH_LABEL);
        ImGui::Text("Layer Mask");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputScalar("##CasterLayerMask", ImGuiDataType_U32, &caster.LayerMask, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
        ImGui::PopItemWidth();
        ImGui::Columns(1);

        // === POINTS LIST ===
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("📐 Points (%zu)", caster.Points.size());
        ImGui::Separator();
        ImGui::Spacing();

        // Point editing
        int removeIndex = -1;
        for (size_t i = 0; i < caster.Points.size(); i++)
        {
            ImGui::PushID(static_cast<int>(i));
            
            ImGui::Text("%zu:", i);
            ImGui::SameLine();
            
            ImGui::PushItemWidth(60);
            ImGui::DragFloat("##X", &caster.Points[i].x, 0.01f, -100.0f, 100.0f, "%.2f");
            ImGui::SameLine();
            ImGui::DragFloat("##Y", &caster.Points[i].y, 0.01f, -100.0f, 100.0f, "%.2f");
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::SmallButton("X"))
            {
                removeIndex = static_cast<int>(i);
            }
            
            ImGui::PopID();
        }

        if (removeIndex >= 0 && caster.Points.size() > 2)
        {
            caster.Points.erase(caster.Points.begin() + removeIndex);
        }

        ImGui::Spacing();
        if (ImGui::Button("+ Add Point"))
        {
            // Add point at end, offset from last point
            glm::vec2 newPoint = caster.Points.empty() ? glm::vec2(0.0f) : caster.Points.back() + glm::vec2(0.5f, 0.0f);
            caster.Points.push_back(newPoint);
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset to Box"))
        {
            caster.Points = {
                { -0.5f, -0.5f },
                {  0.5f, -0.5f },
                {  0.5f,  0.5f },
                { -0.5f,  0.5f }
            };
        }

        // Generate from collider button
        if (entity.HasComponent<Pillar::ColliderComponent>())
        {
            ImGui::SameLine();
            if (ImGui::Button("From Collider"))
            {
                auto& collider = entity.GetComponent<Pillar::ColliderComponent>();
                if (collider.Type == Pillar::ColliderType::Box)
                {
                    glm::vec2 h = collider.HalfExtents;
                    caster.Points = {
                        { -h.x, -h.y },
                        {  h.x, -h.y },
                        {  h.x,  h.y },
                        { -h.x,  h.y }
                    };
                }
                else if (collider.Type == Pillar::ColliderType::Polygon)
                {
                    caster.Points = collider.Vertices;
                }
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Copy shape from attached collider");
        }
    }

    ImGui::PopID();
}
```

---

### 6.6 Task 2.1: Viewport Lighting Integration

**File:** `PillarEditor/src/Panels/ViewportPanel.h`

**Add members:**
```cpp
private:
    bool m_EnableLitPreview = false;
    bool m_ShowLightGizmos = true;
    Pillar::Lighting2DSettings m_LightingSettings;
    
    // New methods
    void DrawLightGizmos();
    void RenderSceneLit();
    void RenderSceneUnlit();
```

**File:** `PillarEditor/src/Panels/ViewportPanel.cpp`

**Modified RenderScene():**
```cpp
void ViewportPanel::RenderScene()
{
    if (!m_Framebuffer)
        return;

    if (m_EnableLitPreview && m_Scene)
    {
        RenderSceneLit();
    }
    else
    {
        RenderSceneUnlit();
    }
}

void ViewportPanel::RenderSceneLit()
{
    // Use Lighting2D pipeline
    Pillar::Lighting2D::BeginScene(
        m_EditorCamera.GetCamera(),
        m_Framebuffer,
        m_LightingSettings
    );

    // Draw grid (will be lit)
    DrawGrid();

    // Draw entities (same as unlit path)
    DrawAllEntities();

    // Submit lights from scene
    auto lightView = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::Light2DComponent>();
    for (auto entity : lightView)
    {
        auto& transform = lightView.get<Pillar::TransformComponent>(entity);
        auto& light = lightView.get<Pillar::Light2DComponent>(entity);

        Pillar::Light2DSubmit submit;
        submit.Type = light.Type;
        submit.Position = transform.Position;
        submit.Direction = Pillar::Transform2D::Forward(transform.Rotation);
        submit.Color = light.Color;
        submit.Intensity = light.Intensity;
        submit.Radius = light.Radius;
        submit.InnerAngleRadians = light.InnerAngleRadians;
        submit.OuterAngleRadians = light.OuterAngleRadians;
        submit.CastShadows = light.CastShadows;
        submit.ShadowStrength = light.ShadowStrength;
        submit.LayerMask = light.LayerMask;

        Pillar::Lighting2D::SubmitLight(submit);
    }

    // Submit shadow casters from scene
    auto casterView = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::ShadowCaster2DComponent>();
    for (auto entity : casterView)
    {
        auto& transform = casterView.get<Pillar::TransformComponent>(entity);
        auto& caster = casterView.get<Pillar::ShadowCaster2DComponent>(entity);

        if (caster.Points.size() < 2)
            continue;

        Pillar::ShadowCaster2DSubmit submit;
        submit.Closed = caster.Closed;
        submit.TwoSided = caster.TwoSided;
        submit.LayerMask = caster.LayerMask;
        submit.WorldPoints.reserve(caster.Points.size());

        for (const auto& local : caster.Points)
            submit.WorldPoints.push_back(transform.TransformPoint(local));

        Pillar::Lighting2D::SubmitShadowCaster(submit);
    }

    Pillar::Lighting2D::EndScene();

    // Draw gizmos ON TOP (after composite, directly to framebuffer)
    m_Framebuffer->Bind();
    Pillar::Renderer2DBackend::BeginScene(m_EditorCamera.GetCamera());
    
    if (m_ShowLightGizmos)
        DrawLightGizmos();
    if (m_ShowColliderGizmos)
        DrawColliderGizmos();
    if (m_ShowRigidbodyGizmos)
        DrawRigidbodyGizmos();
    
    Pillar::Renderer2DBackend::EndScene();
    m_Framebuffer->Unbind();
}

void ViewportPanel::RenderSceneUnlit()
{
    // Existing unlit rendering path
    m_Framebuffer->Bind();
    // ... (current implementation)
    m_Framebuffer->Unbind();
}
```

---

### 6.7 Task 2.3-2.4: Light Gizmo Rendering

```cpp
void ViewportPanel::DrawLightGizmos()
{
    if (!m_Scene)
        return;

    auto view = m_Scene->GetRegistry().view<Pillar::TransformComponent, Pillar::Light2DComponent>();

    for (auto entity : view)
    {
        auto& transform = view.get<Pillar::TransformComponent>(entity);
        auto& light = view.get<Pillar::Light2DComponent>(entity);

        glm::vec2 pos = transform.Position;
        
        // Determine gizmo color based on selection
        Pillar::Entity e(entity, m_Scene.get());
        bool isSelected = m_SelectionContext && m_SelectionContext->IsSelected(e);
        glm::vec4 gizmoColor = isSelected 
            ? glm::vec4(1.0f, 1.0f, 0.0f, 0.8f)  // Yellow when selected
            : glm::vec4(light.Color, 0.5f);       // Light color when not selected

        // Draw center marker
        Pillar::Renderer2DBackend::DrawQuad(pos, { 0.15f, 0.15f }, glm::vec4(1.0f, 0.9f, 0.3f, 1.0f));

        if (light.Type == Pillar::Light2DType::Point)
        {
            // Draw radius circle
            Pillar::Renderer2DBackend::DrawCircle(pos, light.Radius, gizmoColor, 48, 2.0f);
        }
        else if (light.Type == Pillar::Light2DType::Spot)
        {
            // Draw spot light cone
            glm::vec2 dir = Pillar::Transform2D::Forward(transform.Rotation);
            float outerAngle = light.OuterAngleRadians;
            float innerAngle = light.InnerAngleRadians;

            // Calculate cone edges
            float cosOuter = std::cos(outerAngle);
            float sinOuter = std::sin(outerAngle);
            float cosInner = std::cos(innerAngle);
            float sinInner = std::sin(innerAngle);

            // Rotate direction by angles
            glm::vec2 leftOuter = glm::vec2(
                dir.x * cosOuter - dir.y * sinOuter,
                dir.x * sinOuter + dir.y * cosOuter
            ) * light.Radius;

            glm::vec2 rightOuter = glm::vec2(
                dir.x * cosOuter + dir.y * sinOuter,
                -dir.x * sinOuter + dir.y * cosOuter
            ) * light.Radius;

            // Draw cone lines
            Pillar::Renderer2DBackend::DrawLine(pos, pos + leftOuter, gizmoColor, 2.0f);
            Pillar::Renderer2DBackend::DrawLine(pos, pos + rightOuter, gizmoColor, 2.0f);
            
            // Draw arc at radius
            DrawArc(pos, light.Radius, transform.Rotation - outerAngle, transform.Rotation + outerAngle, gizmoColor, 16);
            
            // Draw direction arrow
            glm::vec2 arrowTip = pos + dir * (light.Radius * 0.7f);
            Pillar::Renderer2DBackend::DrawLine(pos, arrowTip, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), 3.0f);
        }
    }
}
```

---

### 6.8 Task 2.5: Lighting Toggle in Viewport Toolbar

**In `ViewportPanel::OnImGuiRender()`:**

```cpp
void ViewportPanel::DrawGizmoToolbar()
{
    // ... existing gizmo mode buttons ...

    ImGui::Separator();
    
    // Lighting toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_EnableLitPreview ? ImVec4(0.3f, 0.6f, 0.3f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button(m_EnableLitPreview ? "💡 Lit" : "💡 Unlit"))
    {
        m_EnableLitPreview = !m_EnableLitPreview;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle lit scene preview (L)");

    ImGui::SameLine();
    
    // Light gizmos toggle
    ImGui::PushStyleColor(ImGuiCol_Button, m_ShowLightGizmos ? ImVec4(0.3f, 0.5f, 0.6f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    if (ImGui::Button("🔦"))
    {
        m_ShowLightGizmos = !m_ShowLightGizmos;
    }
    ImGui::PopStyleColor();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Toggle light gizmos");
}
```

---

### 6.9 Task 3.1: Global Ambient Settings Panel

**File:** `PillarEditor/src/EditorSettings.h`

**Add to EditorSettings class:**
```cpp
// Lighting preview settings
glm::vec3 DefaultAmbientColor = { 1.0f, 1.0f, 1.0f };
float DefaultAmbientIntensity = 0.15f;
bool DefaultShadowsEnabled = true;
```

**In `ViewportPanel` or create new `LightingSettingsPanel`:**

```cpp
void DrawLightingSettings()
{
    ImGui::Begin("Lighting Settings");
    
    ImGui::Text("Global Ambient");
    ImGui::ColorEdit3("Color", &m_LightingSettings.AmbientColor.x);
    ImGui::SliderFloat("Intensity", &m_LightingSettings.AmbientIntensity, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Checkbox("Enable Shadows", &m_LightingSettings.EnableShadows);
    
    ImGui::Separator();
    if (ImGui::Button("Reset to Defaults"))
    {
        auto& settings = EditorSettings::Get();
        m_LightingSettings.AmbientColor = settings.DefaultAmbientColor;
        m_LightingSettings.AmbientIntensity = settings.DefaultAmbientIntensity;
        m_LightingSettings.EnableShadows = settings.DefaultShadowsEnabled;
    }
    
    ImGui::End();
}
```

---

## 7. Technical Specifications

### 7.1 File Changes Summary

| File | Changes |
|------|---------|
| `PillarEditor/src/EditorApp.cpp` | Add `Lighting2D::Init/Shutdown` |
| `Pillar/src/Pillar/ECS/BuiltinComponentRegistrations.cpp` | Register Light2D and ShadowCaster2D |
| `PillarEditor/src/Panels/InspectorPanel.h` | Add method declarations |
| `PillarEditor/src/Panels/InspectorPanel.cpp` | Add `DrawLight2DComponent`, `DrawShadowCaster2DComponent`, update `DrawAddComponentButton` |
| `PillarEditor/src/Panels/ViewportPanel.h` | Add lighting members and methods |
| `PillarEditor/src/Panels/ViewportPanel.cpp` | Add lit rendering path, light gizmos, toolbar |
| `PillarEditor/src/EditorLayer.h` | Add `Lighting2DSystem` member |
| `PillarEditor/src/EditorLayer.cpp` | Initialize and update `Lighting2DSystem` in Play mode |
| `PillarEditor/src/EditorSettings.h/cpp` | Add default lighting settings |

### 7.2 New Includes Required

**InspectorPanel.cpp:**
```cpp
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"
```

**ViewportPanel.cpp:**
```cpp
#include "Pillar/Renderer/Lighting2D.h"
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"
```

**EditorLayer.cpp:**
```cpp
#include "Pillar/ECS/Systems/Lighting2DSystem.h"
```

### 7.3 API Considerations

**Framebuffer Compatibility:**
The `Lighting2D::BeginScene()` overload that takes an output framebuffer requires the framebuffer to have at minimum `RGBA8` color attachment. The existing editor framebuffer meets this requirement.

**OpenGL State:**
`Lighting2D` captures and restores OpenGL state. However, gizmos should be drawn **after** `Lighting2D::EndScene()` to ensure they appear unlit on top of the scene.

---

## 8. Testing Strategy

### 8.1 Unit Tests

**Add to `Tests/src/`:**

```cpp
// LightingComponentTests.cpp
TEST(LightingComponentTests, Light2DComponentDefaults)
{
    Pillar::Light2DComponent light;
    EXPECT_EQ(light.Type, Pillar::Light2DType::Point);
    EXPECT_FLOAT_EQ(light.Intensity, 1.0f);
    EXPECT_FLOAT_EQ(light.Radius, 6.0f);
    EXPECT_TRUE(light.CastShadows);
}

TEST(LightingComponentTests, ShadowCaster2DComponentDefaults)
{
    Pillar::ShadowCaster2DComponent caster;
    EXPECT_TRUE(caster.Closed);
    EXPECT_FALSE(caster.TwoSided);
    EXPECT_TRUE(caster.Points.empty());
}
```

### 8.2 Integration Tests

1. **Serialization Round-Trip:** Create scene with lights, save, load, verify values match
2. **Lit Preview Toggle:** Toggle lit mode, verify framebuffer content changes
3. **Play Mode Lighting:** Enter play mode, verify `Lighting2DSystem` is updating

### 8.3 Manual Test Cases

| Test Case | Steps | Expected Result |
|-----------|-------|-----------------|
| TC-01: Add Light2D | Create entity → Add Component → Light 2D | Component appears in Inspector |
| TC-02: Edit Light Color | Select light entity → Change color in Inspector | Color picker works, value persists |
| TC-03: Lit Preview | Add light → Toggle "Lit" in viewport | Scene renders with lighting |
| TC-04: Save/Load Light | Add light → Save scene → Reload → Verify | Light properties restored |
| TC-05: Spot Light Gizmo | Set light to Spot → View in viewport | Cone gizmo visible |
| TC-06: Shadow Caster | Add ShadowCaster2D → Enable lit preview | Shadow visible |

---

## 9. Risk Assessment

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Performance degradation in lit preview | Medium | Medium | Add option to disable shadows in preview; limit light count warning |
| Framebuffer incompatibility | High | Low | Validate framebuffer spec before enabling lit mode |
| Serialization breaking existing scenes | High | Low | Version scene files; add migration if needed |
| Gizmos not visible in lit mode | Medium | Medium | Render gizmos in separate pass after composite |
| Undo/redo for lighting properties | Medium | Medium | Reuse existing command pattern from Transform |

---

## 10. Timeline Estimate

| Phase | Duration | Dependencies |
|-------|----------|--------------|
| Phase 1: Core Integration | 1 week | None |
| Phase 2: Viewport Integration | 1 week | Phase 1 |
| Phase 3: Polish & UX | 1 week | Phase 2 |
| Testing & Bug Fixes | 3-4 days | All phases |
| **Total** | **~3.5 weeks** | |

---

## Appendix A: Reference Implementation — Lighting2DDemoLayer

See `Sandbox/src/Lighting2DDemoLayer.h` for a working example of the lighting pipeline:

```cpp
void OnUpdate(float dt) override {
    // 1. Begin lit scene
    Lighting2D::BeginScene(camera, w, h, m_Settings);
    
    // 2. Draw sprites (into internal SceneColorFramebuffer)
    DrawSceneSprites();
    
    // 3. Submit shadow casters
    SubmitShadowCasters();
    
    // 4. Submit lights
    Lighting2D::SubmitLight(m_Light);
    
    // 5. End scene (renders light accumulation, composites)
    Lighting2D::EndScene();
}
```

---

## Appendix B: Known Issues from LIGHTING_CODE_REVIEW.md

These issues should be addressed before or alongside editor integration:

1. **Uniform location caching** — Currently queries every frame; should cache after shader compilation
2. **Shadow extrusion distance** — Hard cutoff at radius boundary causes visual popping
3. **Color space** — RGBA8 may band with many bright lights; consider RGBA16F for light accumulation

---

*Document maintained by Engine Team. Last updated: 2026-01-06*
