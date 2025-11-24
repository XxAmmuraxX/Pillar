# Scene Management System - Implementation Plan

**Date:** January 2025  
**Branch:** `feature/render_api/2` ? `feature/scene-management`  
**Priority:** High (Next major system after camera)  
**Prerequisites:** ? Camera System Complete, ? Static Linking Complete

---

## Overview

The Scene Management system will handle organization, lifecycle, and serialization of game objects/entities in a scene hierarchy. This is a foundational system that everything else will build upon (physics, scripting, editor, etc.).

---

## Current State Analysis

### ? What We Have

1. **Layer System** - Basic scene organization
   - Layers can contain game logic
   - Update loop exists (`OnUpdate(dt)`)
   - Event propagation works
   - **Limitation:** Layers are not entities, can't be serialized

2. **Renderer2D** - Can draw objects
   - `DrawQuad()` with position, size, color, texture
   - Camera integration
   - **Limitation:** No scene graph, everything drawn manually

3. **Camera System** - View management
   - `OrthographicCamera` for 2D
   - `OrthographicCameraController` for input
   - **Limitation:** Camera not part of scene hierarchy

4. **Asset Management** - Basic file loading
   - `AssetManager` for path resolution
   - Texture loading
   - **Limitation:** No asset lifecycle management, no caching

### ? What We Need

1. **Scene** - Container for all game objects
2. **Entity** - Individual game object with components
3. **Component System** - Modular entity behavior
4. **Transform Hierarchy** - Parent-child relationships
5. **Scene Serialization** - Save/load scenes
6. **Entity ID System** - Unique identification
7. **Scene Camera** - Camera as entity in scene

---

## Architecture Decision: ECS vs GameObject Pattern

### Option A: Entity-Component System (ECS) - **RECOMMENDED**

**Libraries:**
- **EnTT** - Header-only, fast, widely used
- **flecs** - More features, larger

**Pros:**
- ? Cache-friendly (data-oriented design)
- ? Very fast iteration (millions of entities)
- ? Flexible (add/remove components dynamically)
- ? Modern C++ patterns
- ? Industry standard (Unity DOTS, UE5 Mass)

**Cons:**
- ?? Steeper learning curve
- ?? More complex to serialize
- ?? Harder to debug (components scattered)

### Option B: GameObject Hierarchy Pattern

**Similar to:** Unity pre-DOTS, Unreal pre-5, Godot

**Pros:**
- ? Simpler mental model
- ? Easy parent-child relationships
- ? Straightforward serialization
- ? Component inspector trivial

**Cons:**
- ?? Less performant (pointer chasing)
- ?? Less flexible (components tightly coupled)
- ?? Doesn't scale to huge entity counts

### **Decision: Hybrid Approach - EnTT with Scene Graph**

Use EnTT for component storage and iteration, but add a thin scene graph layer for hierarchy. This gives us:
- ? Performance of ECS
- ? Usability of GameObject pattern
- ? Best of both worlds

---

## System Design

### Core Classes

```cpp
// Entity - Lightweight handle to EnTT entity
class Entity
{
public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    
    template<typename T, typename... Args>
    T& AddComponent(Args&&... args);
    
    template<typename T>
    T& GetComponent();
    
    template<typename T>
    bool HasComponent();
    
    template<typename T>
    void RemoveComponent();
    
    operator bool() const { return m_EntityHandle != entt::null && m_Scene != nullptr; }
    operator entt::entity() const { return m_EntityHandle; }
    operator uint32_t() const { return (uint32_t)m_EntityHandle; }
    
    bool operator==(const Entity& other) const;
    bool operator!=(const Entity& other) const;
    
private:
    entt::entity m_EntityHandle = entt::null;
    Scene* m_Scene = nullptr;
};

// Scene - Container for all entities
class Scene
{
public:
    Scene();
    ~Scene();
    
    Entity CreateEntity(const std::string& name = "Entity");
    void DestroyEntity(Entity entity);
    
    void OnUpdate(float deltaTime);
    void OnRender(OrthographicCamera& camera);
    void OnViewportResize(uint32_t width, uint32_t height);
    
    // Hierarchy
    void SetParent(Entity child, Entity parent);
    Entity GetParent(Entity entity);
    std::vector<Entity> GetChildren(Entity entity);
    
    // Queries
    template<typename... Components>
    auto GetEntitiesWith();
    
    // Serialization
    void Serialize(const std::string& filepath);
    void Deserialize(const std::string& filepath);
    
private:
    entt::registry m_Registry;
    std::string m_Name;
    uint32_t m_ViewportWidth = 1280;
    uint32_t m_ViewportHeight = 720;
    
    friend class Entity;
};
```

### Essential Components

```cpp
// Transform Component - Position, rotation, scale
struct TransformComponent
{
    glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
    glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; // Euler angles in degrees
    glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
    
    glm::mat4 GetTransform() const
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.x), {1, 0, 0})
                           * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.y), {0, 1, 0})
                           * glm::rotate(glm::mat4(1.0f), glm::radians(Rotation.z), {0, 0, 1});
        
        return glm::translate(glm::mat4(1.0f), Position)
             * rotation
             * glm::scale(glm::mat4(1.0f), Scale);
    }
    
    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec3& position) : Position(position) {}
};

// Tag Component - Entity name/identifier
struct TagComponent
{
    std::string Tag;
    
    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

// Hierarchy Component - Parent-child relationships
struct HierarchyComponent
{
    entt::entity Parent = entt::null;
    std::vector<entt::entity> Children;
    
    HierarchyComponent() = default;
    HierarchyComponent(const HierarchyComponent&) = default;
};

// Sprite Renderer Component - 2D rendering
struct SpriteRendererComponent
{
    glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    std::shared_ptr<Texture2D> Texture = nullptr;
    float TilingFactor = 1.0f;
    
    SpriteRendererComponent() = default;
    SpriteRendererComponent(const SpriteRendererComponent&) = default;
    SpriteRendererComponent(const glm::vec4& color) : Color(color) {}
};

// Camera Component - Make camera an entity
struct CameraComponent
{
    OrthographicCamera Camera;
    bool Primary = true; // Primary camera for rendering
    bool FixedAspectRatio = false;
    
    CameraComponent() = default;
    CameraComponent(const CameraComponent&) = default;
};
```

---

## Implementation Phases

### Phase 1: Core Scene & Entity System (3-4 hours)

**Goal:** Basic scene with entities that have transform and tag components.

**Tasks:**
1. Add EnTT to project via FetchContent
2. Create `Scene.h/cpp` - Basic scene container
3. Create `Entity.h/cpp` - Entity handle wrapper
4. Create `Components.h` - Define TransformComponent, TagComponent
5. Implement `CreateEntity()`, `DestroyEntity()`
6. Implement basic `OnUpdate()` loop (iterate entities)
7. Update CMakeLists.txt

**Deliverable:** Can create entities with transform/tag components, update them in scene.

**Test:**
```cpp
Scene scene;
Entity entity = scene.CreateEntity("Player");
auto& transform = entity.GetComponent<TransformComponent>();
transform.Position = { 1.0f, 2.0f, 0.0f };
scene.OnUpdate(0.016f);
```

---

### Phase 2: Sprite Rendering Integration (2-3 hours)

**Goal:** Entities can be rendered as sprites.

**Tasks:**
1. Add `SpriteRendererComponent` to Components.h
2. Implement `Scene::OnRender(camera)`
3. Iterate entities with Transform + SpriteRenderer
4. Call `Renderer2D::DrawQuad()` for each sprite
5. Update ExampleLayer to use Scene instead of manual rendering
6. Add viewport resize handling

**Deliverable:** Scene full of sprite entities, all rendered automatically.

**Test:**
```cpp
Entity quad = scene.CreateEntity("Quad");
quad.AddComponent<SpriteRendererComponent>(glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
quad.GetComponent<TransformComponent>().Position = { 0.0f, 0.0f, 0.0f };

scene.OnRender(camera);  // Renders all entities
```

---

### Phase 3: Scene Camera System (1-2 hours)

**Goal:** Camera is an entity in the scene.

**Tasks:**
1. Add `CameraComponent` to Components.h
2. Add `Scene::GetPrimaryCamera()` method
3. Update `Scene::OnRender()` to use scene camera if available
4. Add `Scene::OnViewportResize()` to update camera aspect ratio
5. Update ExampleLayer to create camera entity

**Deliverable:** Camera exists as entity, can have multiple cameras, switch between them.

**Test:**
```cpp
Entity cameraEntity = scene.CreateEntity("Camera");
auto& camera = cameraEntity.AddComponent<CameraComponent>();
camera.Primary = true;

scene.OnRender();  // Uses scene's primary camera
```

---

### Phase 4: Scene Hierarchy (3-4 hours)

**Goal:** Entities can have parent-child relationships, transforms inherited.

**Tasks:**
1. Add `HierarchyComponent` to Components.h
2. Implement `Scene::SetParent(child, parent)`
3. Implement `Scene::GetParent(entity)`, `GetChildren(entity)`
4. Add world transform calculation (accumulate parent transforms)
5. Update `Scene::OnRender()` to use world transforms
6. Add hierarchy debug visualization (ImGui tree)

**Deliverable:** Can parent entities, child moves with parent.

**Test:**
```cpp
Entity parent = scene.CreateEntity("Parent");
Entity child = scene.CreateEntity("Child");

scene.SetParent(child, parent);

parent.GetComponent<TransformComponent>().Position.x = 5.0f;
// Child also moves 5 units right
```

---

### Phase 5: Scene Serialization (4-5 hours)

**Goal:** Save and load scenes to/from JSON files.

**Tasks:**
1. Add JSON library (nlohmann/json via FetchContent)
2. Implement `Scene::Serialize(filepath)`
   - Iterate all entities
   - Serialize each component to JSON
   - Write to file
3. Implement `Scene::Deserialize(filepath)`
   - Read JSON file
   - Create entities
   - Deserialize components
   - Restore hierarchy
4. Add scene save/load to ExampleLayer (ImGui menu)
5. Write unit tests for serialization

**Deliverable:** Can save/load complete scenes, persist game state.

**Test:**
```cpp
// Create scene
Scene scene1;
Entity entity = scene1.CreateEntity("TestEntity");
entity.GetComponent<TransformComponent>().Position = { 1.0f, 2.0f, 3.0f };

// Save
scene1.Serialize("test_scene.json");

// Load
Scene scene2;
scene2.Deserialize("test_scene.json");
// Scene2 now has entity with same transform
```

---

### Phase 6: Component Queries & Systems (2-3 hours)

**Goal:** Efficiently query entities with specific components.

**Tasks:**
1. Implement `Scene::GetEntitiesWith<Components...>()`
2. Add example "systems" (MovementSystem, RenderSystem concepts)
3. Add component iteration benchmarks
4. Update documentation with query examples

**Deliverable:** Fast component queries, foundation for game logic systems.

**Test:**
```cpp
// Get all entities with Transform and SpriteRenderer
auto view = scene.GetEntitiesWith<TransformComponent, SpriteRendererComponent>();

for (auto entity : view)
{
    auto& transform = entity.GetComponent<TransformComponent>();
    auto& sprite = entity.GetComponent<SpriteRendererComponent>();
    // Process entity
}
```

---

## File Structure

```
Pillar/src/Pillar/Scene/
??? Scene.h/cpp                    # Scene container
??? Entity.h/cpp                   # Entity handle
??? Components.h                   # All component definitions
??? SceneSerializer.h/cpp          # JSON serialization

Tests/src/
??? SceneTests.cpp                 # Scene creation, entity lifecycle
??? EntityTests.cpp                # Component add/remove/get
??? HierarchyTests.cpp             # Parent-child relationships
??? SerializationTests.cpp         # Save/load scenes
```

---

## Dependencies

### EnTT (Entity-Component-System)
```cmake
# In root CMakeLists.txt
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.12.2
)
FetchContent_MakeAvailable(entt)
```

**License:** MIT  
**Size:** Header-only (~100KB)  
**Performance:** 50M+ entities/sec on modern CPUs

### nlohmann/json (Serialization)
```cmake
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)
```

**License:** MIT  
**Size:** Header-only (~600KB)  
**Features:** Intuitive API, good error messages

---

## Example JSON Scene Format

```json
{
  "Scene": "Example Scene",
  "Entities": [
    {
      "ID": 12345,
      "TagComponent": {
        "Tag": "Player"
      },
      "TransformComponent": {
        "Position": [0.0, 0.0, 0.0],
        "Rotation": [0.0, 0.0, 0.0],
        "Scale": [1.0, 1.0, 1.0]
      },
      "SpriteRendererComponent": {
        "Color": [1.0, 1.0, 1.0, 1.0],
        "TexturePath": "assets/textures/player.png",
        "TilingFactor": 1.0
      },
      "HierarchyComponent": {
        "Parent": null,
        "Children": [67890]
      }
    },
    {
      "ID": 67890,
      "TagComponent": {
        "Tag": "PlayerWeapon"
      },
      "TransformComponent": {
        "Position": [0.5, 0.0, 0.0],
        "Rotation": [0.0, 0.0, 0.0],
        "Scale": [0.5, 0.5, 1.0]
      },
      "SpriteRendererComponent": {
        "Color": [1.0, 1.0, 1.0, 1.0],
        "TexturePath": "assets/textures/sword.png"
      },
      "HierarchyComponent": {
        "Parent": 12345,
        "Children": []
      }
    }
  ]
}
```

---

## Testing Strategy

### Unit Tests
```cpp
// Scene creation
TEST(SceneTests, CreateScene_InitializesEmpty)
TEST(SceneTests, CreateEntity_ReturnsValidEntity)
TEST(SceneTests, DestroyEntity_RemovesEntity)

// Components
TEST(EntityTests, AddComponent_ComponentExists)
TEST(EntityTests, GetComponent_ReturnsReference)
TEST(EntityTests, HasComponent_ReturnsTrue)
TEST(EntityTests, RemoveComponent_ComponentGone)

// Hierarchy
TEST(HierarchyTests, SetParent_CreatesRelationship)
TEST(HierarchyTests, GetParent_ReturnsParent)
TEST(HierarchyTests, GetChildren_ReturnsChildren)
TEST(HierarchyTests, WorldTransform_InheritsParent)

// Serialization
TEST(SerializationTests, Serialize_CreatesFile)
TEST(SerializationTests, Deserialize_LoadsScene)
TEST(SerializationTests, RoundTrip_PreservesData)
```

### Integration Tests
- Create complex scene, render, verify output
- Save scene, reload, verify identical
- Hierarchy transform propagation visual test

---

## Performance Targets

| Operation | Target | Notes |
|-----------|--------|-------|
| Create Entity | < 1 µs | EnTT is very fast |
| Add Component | < 0.5 µs | Pool allocation |
| Get Component | < 0.1 µs | Direct lookup |
| Iterate 10k entities | < 1 ms | Cache-friendly |
| Serialize scene | < 100 ms | For typical scene |
| Deserialize scene | < 150 ms | Includes asset loading |

---

## Migration Plan from Current System

### Current ExampleLayer Pattern:
```cpp
// Manual rendering
Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, texture);
```

### New Scene-Based Pattern:
```cpp
// Entity-based rendering
Entity quad = scene.CreateEntity("Quad");
quad.AddComponent<SpriteRendererComponent>(texture);
scene.OnRender(camera);  // Automatic
```

**Benefits:**
- ? Entities can be queried, modified, serialized
- ? Hierarchy support
- ? Less boilerplate in application code
- ? Foundation for editor, scripting, physics

---

## Known Challenges & Solutions

### Challenge 1: Component Serialization Boilerplate
**Problem:** Each component needs custom serialize/deserialize code  
**Solution:** Use macros or reflection library (rttr, refl-cpp)

### Challenge 2: Asset Reference Serialization
**Problem:** Textures stored as pointers, need to serialize paths  
**Solution:** Asset UUID system (future), for now use paths

### Challenge 3: Entity ID Persistence
**Problem:** EnTT entity IDs not stable across sessions  
**Solution:** Add UUIDComponent, map UUID ? entity on load

### Challenge 4: Circular Dependencies
**Problem:** Entity needs Scene, Scene needs Entity  
**Solution:** Forward declarations, implementation in .cpp

---

## Future Enhancements (Not in Initial Implementation)

- **Prefab System:** Reusable entity templates
- **Scene Layers:** Separate update/render layers within scene
- **Scene State:** Pause, play, step frame
- **Multi-Scene:** Multiple scenes loaded simultaneously
- **Scene Transitions:** Fade, slide, custom effects
- **Runtime Component Add/Remove:** Hot-reloading components
- **Component Inspector:** ImGui panels for editing
- **Scene Viewport:** Embedded game view in editor

---

## Success Criteria

### Phase 1 Complete When:
- [x] Can create scene with entities
- [x] Entities have Transform + Tag components
- [x] Scene updates entities
- [x] Unit tests pass

### Full System Complete When:
- [x] All 6 phases implemented
- [x] 50+ unit tests passing
- [x] ExampleLayer uses scene system
- [x] Can save/load scenes
- [x] Hierarchy works correctly
- [x] Documentation updated
- [x] Performance targets met

---

## Timeline Estimate

| Phase | Time | Cumulative |
|-------|------|------------|
| Phase 1: Core System | 3-4 hours | 4 hours |
| Phase 2: Sprite Rendering | 2-3 hours | 7 hours |
| Phase 3: Scene Camera | 1-2 hours | 9 hours |
| Phase 4: Hierarchy | 3-4 hours | 13 hours |
| Phase 5: Serialization | 4-5 hours | 18 hours |
| Phase 6: Queries & Systems | 2-3 hours | 21 hours |
| **Total** | **~21 hours** | **3-4 work days** |

---

## Next Steps

1. ? Review this plan
2. ? Confirm EnTT + nlohmann/json dependencies
3. ? Create feature branch: `feature/scene-management`
4. ?? Start Phase 1: Core Scene & Entity System
5. ?? Update PROJECT_STATUS.md after each phase

---

**Ready to proceed?** Let's build the scene management system! ??

