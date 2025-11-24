# Pillar Engine - Selective Hybrid ECS + Box2D Implementation Plan

**Date:** November 24, 2025  
**Branch:** `feature/render_api/2` → `feature/ecs-physics-hybrid`  
**Architecture Pattern:** Selective Hybrid (EnTT + Box2D)  
**Priority:** High - Foundation for gameplay systems  
**Prerequisites:** ✅ Static linking complete, ✅ ImGui integration fixed

---

## Table of Contents
1. [Architecture Overview](#architecture-overview)
2. [Entity Classification](#entity-classification)
3. [File & Folder Structure](#file--folder-structure)
4. [Component Definitions](#component-definitions)
5. [System Definitions](#system-definitions)
6. [Game Loop & Update Order](#game-loop--update-order)
7. [Implementation Phases](#implementation-phases)
8. [Memory Management Strategy](#memory-management-strategy)
9. [Performance Considerations](#performance-considerations)
10. [Testing Strategy](#testing-strategy)

---

## Architecture Overview

### Core Principle: "Selective Hybrid"

**Heavy Entities** (Player, Enemies, Walls, Bosses):
- Managed by EnTT registry
- Own a `b2Body*` (Box2D physics body)
- **Source of Truth:** Box2D position/rotation
- **Sync Direction:** Box2D → EnTT (read-only from EnTT perspective)
- **Collision:** Box2D fixtures and callbacks
- **Lifecycle:** EnTT `on_destroy` listener cleans up `b2Body`

**Light Entities** (Bullets, Particles, XP Gems, VFX):
- Pure EnTT entities (no Box2D bodies)
- **Source of Truth:** EnTT `TransformComponent`
- **Physics:** Custom velocity integration (`pos += vel * dt`)
- **Collision Detection:**
  - **Fast projectiles:** Box2D Raycasts against Heavy Entities
  - **Simple overlap:** Spatial Hash Grid (AABB checks)
- **Rendering:** Instanced rendering (batch thousands)

**Why This Hybrid?**
- ✅ **Performance:** Avoid Box2D overhead for thousands of bullets/particles
- ✅ **Flexibility:** ECS allows fast iteration on Light Entity behavior
- ✅ **Physics Accuracy:** Box2D handles complex collisions for important objects
- ✅ **Scalability:** Can spawn 10,000+ light entities without Box2D bottleneck

---

## Entity Classification

### Heavy Entities (Box2D Bodies)

| Entity Type | b2BodyType | Fixtures | Notes |
|-------------|-----------|----------|-------|
| Player | `b2_dynamicBody` | Circle or Polygon | Continuous collision detection |
| Enemy (Ground) | `b2_dynamicBody` | Circle or Box | Pathfinding AI |
| Enemy (Flying) | `b2_dynamicBody` | Circle | No gravity, custom movement |
| Boss | `b2_dynamicBody` | Complex shape (multiple fixtures) | Large hitbox |
| Wall | `b2_staticBody` | Polygon chain | Immovable |
| Platform | `b2_kinematicBody` | Box | Moving platforms |
| Destructible Object | `b2_dynamicBody` | Box | Breaks into particles on death |

**Total Expected:** ~500-2000 heavy entities per scene

### Light Entities (Pure ECS)

| Entity Type | Physics | Collision Method | Rendering |
|-------------|---------|------------------|-----------|
| Bullet | `pos += vel * dt` | Raycast to Heavy Entities | Instanced (sprite) |
| Particle | `pos += vel * dt`, `vel.y += gravity * dt` | None (visual only) | Instanced (point sprite) |
| XP Gem | `pos += vel * dt` (moves to player) | Spatial Hash vs Player AABB | Instanced (sprite) |
| Damage Number | `pos.y += speed * dt` (floats up) | None (visual only) | Instanced (text) |
| VFX (explosion) | Animated sprite | None | Instanced (sprite) |
| Tracer | Line segment | None | Custom line renderer |

**Total Expected:** ~10,000-50,000 light entities per frame (mostly particles)

---

## File & Folder Structure

```
Pillar/src/Pillar/
├── ECS/                        # NEW: ECS subsystem
│   ├── Entity.h/cpp            # Entity handle wrapper
│   ├── Scene.h/cpp             # Scene container (EnTT registry wrapper)
│   ├── Components/             # All component definitions
│   │   ├── Core/               # Core components
│   │   │   ├── TagComponent.h
│   │   │   ├── TransformComponent.h
│   │   │   ├── HierarchyComponent.h
│   │   │   └── UUIDComponent.h
│   │   ├── Physics/            # Physics-related components
│   │   │   ├── RigidbodyComponent.h      # Wrapper for b2Body*
│   │   │   ├── ColliderComponent.h       # Data-only (shape, size, offset)
│   │   │   ├── VelocityComponent.h       # For Light Entities
│   │   │   └── PhysicsMaterialComponent.h # Friction, restitution, density
│   │   ├── Rendering/          # Rendering components
│   │   │   ├── SpriteComponent.h
│   │   │   ├── AnimatedSpriteComponent.h
│   │   │   ├── ParticleEmitterComponent.h
│   │   │   └── CameraComponent.h
│   │   └── Gameplay/           # Game-specific components
│   │       ├── HealthComponent.h
│   │       ├── DamageComponent.h
│   │       ├── BulletComponent.h         # Bullet metadata (owner, damage)
│   │       ├── XPGemComponent.h          # XP value, attraction radius
│   │       └── AIComponent.h             # State machine, target
│   ├── Systems/                # All system implementations
│   │   ├── PhysicsSystem.h/cpp           # Box2D step + setup
│   │   ├── PhysicsSyncSystem.h/cpp       # Box2D → EnTT sync
│   │   ├── VelocityIntegrationSystem.h/cpp # Light Entity movement
│   │   ├── BulletCollisionSystem.h/cpp   # Raycasts for bullets
│   │   ├── XPCollectionSystem.h/cpp      # Spatial hash for XP gems
│   │   ├── SpriteRenderSystem.h/cpp      # Instanced rendering
│   │   ├── ParticleSystem.h/cpp          # Particle update + render
│   │   └── HealthSystem.h/cpp            # Damage application, death
│   └── Physics/                # Physics integration layer
│       ├── Box2DWorld.h/cpp              # b2World wrapper
│       ├── Box2DBodyFactory.h/cpp        # Create b2Body with fixtures
│       ├── Box2DContactListener.h/cpp    # Collision callbacks → ECS events
│       ├── Box2DDebugDraw.h/cpp          # Debug rendering
│       └── SpatialHashGrid.h/cpp         # AABB broadphase for Light Entities
├── Renderer/                   # EXISTING: Renderer abstraction (keep)
│   └── ... (no changes)
└── ... (rest of engine)

Dependencies (FetchContent):
├── entt (v3.12.2)              # Header-only ECS
├── box2d (v2.4.1)              # 2D physics engine
└── ... (existing deps)
```

---

## Component Definitions

### Core Components

```cpp
// Pillar/src/Pillar/ECS/Components/Core/TagComponent.h
#pragma once
#include <string>

namespace Pillar {

struct TagComponent
{
    std::string Tag;
    
    TagComponent() = default;
    TagComponent(const TagComponent&) = default;
    TagComponent(const std::string& tag) : Tag(tag) {}
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Components/Core/TransformComponent.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Pillar {

struct TransformComponent
{
    glm::vec2 Position = { 0.0f, 0.0f };
    float Rotation = 0.0f;      // Radians
    glm::vec2 Scale = { 1.0f, 1.0f };
    
    // Cached for performance (updated by PhysicsSyncSystem or manually)
    glm::mat4 Transform = glm::mat4(1.0f);
    bool Dirty = true;
    
    glm::mat4 GetTransform() const
    {
        if (Dirty)
        {
            // Recompute (const_cast is a hack, should be in system)
            const_cast<TransformComponent*>(this)->UpdateTransform();
        }
        return Transform;
    }
    
    void UpdateTransform()
    {
        Transform = glm::translate(glm::mat4(1.0f), glm::vec3(Position, 0.0f))
                  * glm::rotate(glm::mat4(1.0f), Rotation, glm::vec3(0, 0, 1))
                  * glm::scale(glm::mat4(1.0f), glm::vec3(Scale, 1.0f));
        Dirty = false;
    }
    
    TransformComponent() = default;
    TransformComponent(const TransformComponent&) = default;
    TransformComponent(const glm::vec2& position) : Position(position) {}
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Components/Core/UUIDComponent.h
#pragma once
#include <cstdint>

namespace Pillar {

struct UUIDComponent
{
    uint64_t UUID;
    
    UUIDComponent() : UUID(GenerateUUID()) {}
    UUIDComponent(uint64_t uuid) : UUID(uuid) {}
    UUIDComponent(const UUIDComponent&) = default;
    
    operator uint64_t() const { return UUID; }
    
private:
    static uint64_t GenerateUUID(); // Implemented in .cpp (use random or counter)
};

} // namespace Pillar
```

### Physics Components (Heavy Entities)

```cpp
// Pillar/src/Pillar/ECS/Components/Physics/RigidbodyComponent.h
#pragma once
#include <box2d/box2d.h>

namespace Pillar {

// CRITICAL: This component means "this entity is a Heavy Entity"
// Presence of this component = entity has a b2Body
struct RigidbodyComponent
{
    b2Body* Body = nullptr;     // Owned by b2World, cleaned up by on_destroy listener
    
    b2BodyType BodyType = b2_dynamicBody;
    bool FixedRotation = false; // Prevent rotation (useful for characters)
    float GravityScale = 1.0f;
    
    RigidbodyComponent() = default;
    RigidbodyComponent(const RigidbodyComponent&) = delete; // No copy (b2Body* is unique)
    RigidbodyComponent& operator=(const RigidbodyComponent&) = delete;
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Components/Physics/ColliderComponent.h
#pragma once
#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

enum class ColliderType
{
    Circle,
    Box,
    Polygon
};

// Data-only component (actual fixtures created by Box2DBodyFactory)
struct ColliderComponent
{
    ColliderType Type = ColliderType::Circle;
    
    // Shape parameters
    glm::vec2 Offset = { 0.0f, 0.0f };      // Local offset from body origin
    union {
        float Radius;                        // For Circle
        glm::vec2 HalfExtents;              // For Box
    };
    std::vector<glm::vec2> Vertices;        // For Polygon (optional)
    
    // Material
    float Density = 1.0f;
    float Friction = 0.3f;
    float Restitution = 0.0f;               // Bounciness
    
    // Collision filtering
    uint16_t CategoryBits = 0x0001;         // What am I?
    uint16_t MaskBits = 0xFFFF;             // What do I collide with?
    int16_t GroupIndex = 0;                 // Negative = never collide
    
    // Sensor flag
    bool IsSensor = false;                  // Trigger collisions only (no physics response)
    
    ColliderComponent() = default;
    ColliderComponent(const ColliderComponent&) = default;
    
    // Convenience constructors
    static ColliderComponent Circle(float radius)
    {
        ColliderComponent c;
        c.Type = ColliderType::Circle;
        c.Radius = radius;
        return c;
    }
    
    static ColliderComponent Box(glm::vec2 halfExtents)
    {
        ColliderComponent c;
        c.Type = ColliderType::Box;
        c.HalfExtents = halfExtents;
        return c;
    }
};

} // namespace Pillar
```

### Physics Components (Light Entities)

```cpp
// Pillar/src/Pillar/ECS/Components/Physics/VelocityComponent.h
#pragma once
#include <glm/glm.hpp>

namespace Pillar {

// For Light Entities (no b2Body)
// Movement is simple: pos += vel * dt
struct VelocityComponent
{
    glm::vec2 Velocity = { 0.0f, 0.0f };    // Units per second
    glm::vec2 Acceleration = { 0.0f, 0.0f }; // Gravity, etc.
    
    float Drag = 0.0f;                      // Linear damping (0 = no drag)
    float MaxSpeed = 1000.0f;               // Clamp velocity magnitude
    
    VelocityComponent() = default;
    VelocityComponent(const VelocityComponent&) = default;
    VelocityComponent(const glm::vec2& velocity) : Velocity(velocity) {}
};

} // namespace Pillar
```

### Gameplay Components

```cpp
// Pillar/src/Pillar/ECS/Components/Gameplay/BulletComponent.h
#pragma once
#include <cstdint>
#include "Pillar/ECS/Entity.h"

namespace Pillar {

struct BulletComponent
{
    Entity Owner;               // Who shot this bullet (for damage attribution)
    float Damage = 10.0f;
    float Lifetime = 5.0f;      // Auto-destroy after this many seconds
    float TimeAlive = 0.0f;
    
    bool Pierce = false;        // Penetrate through enemies
    uint32_t MaxHits = 1;       // How many targets to hit before destruction
    uint32_t HitsRemaining = 1;
    
    BulletComponent() = default;
    BulletComponent(const BulletComponent&) = default;
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Components/Gameplay/XPGemComponent.h
#pragma once

namespace Pillar {

struct XPGemComponent
{
    int XPValue = 1;
    float AttractionRadius = 3.0f;  // Distance at which gem moves toward player
    float MoveSpeed = 10.0f;        // Speed when attracted
    
    bool IsAttracted = false;       // State flag
    
    XPGemComponent() = default;
    XPGemComponent(const XPGemComponent&) = default;
    XPGemComponent(int xp) : XPValue(xp) {}
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Components/Gameplay/HealthComponent.h
#pragma once

namespace Pillar {

struct HealthComponent
{
    float MaxHealth = 100.0f;
    float CurrentHealth = 100.0f;
    
    bool Invulnerable = false;
    float InvulnerabilityTimer = 0.0f;  // Iframe duration after hit
    
    bool IsAlive() const { return CurrentHealth > 0.0f; }
    float GetHealthPercent() const { return CurrentHealth / MaxHealth; }
    
    void TakeDamage(float damage)
    {
        if (!Invulnerable)
        {
            CurrentHealth -= damage;
            if (CurrentHealth < 0.0f) CurrentHealth = 0.0f;
        }
    }
    
    void Heal(float amount)
    {
        CurrentHealth += amount;
        if (CurrentHealth > MaxHealth) CurrentHealth = MaxHealth;
    }
    
    HealthComponent() = default;
    HealthComponent(const HealthComponent&) = default;
    HealthComponent(float maxHealth) : MaxHealth(maxHealth), CurrentHealth(maxHealth) {}
};

} // namespace Pillar
```

### Rendering Components

```cpp
// Pillar/src/Pillar/ECS/Components/Rendering/SpriteComponent.h
#pragma once
#include <glm/glm.hpp>
#include <memory>
#include "Pillar/Renderer/Texture.h"

namespace Pillar {

struct SpriteComponent
{
    std::shared_ptr<Texture2D> Texture = nullptr;
    glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec2 Size = { 1.0f, 1.0f };        // Sprite size in world units
    
    // UV coordinates (for sprite sheets)
    glm::vec2 UVMin = { 0.0f, 0.0f };
    glm::vec2 UVMax = { 1.0f, 1.0f };
    
    // Rendering order
    int ZOrder = 0;                         // Higher = drawn on top
    
    SpriteComponent() = default;
    SpriteComponent(const SpriteComponent&) = default;
    SpriteComponent(std::shared_ptr<Texture2D> texture) : Texture(texture) {}
};

} // namespace Pillar
```

---

## System Definitions

### System Base Class

```cpp
// Pillar/src/Pillar/ECS/Systems/System.h
#pragma once

namespace Pillar {

class Scene; // Forward declaration

class System
{
public:
    virtual ~System() = default;
    
    virtual void OnAttach(Scene* scene) {}
    virtual void OnDetach() {}
    virtual void OnUpdate(float deltaTime) = 0;
    
protected:
    Scene* m_Scene = nullptr;
};

} // namespace Pillar
```

### PhysicsSystem (Box2D Step)

```cpp
// Pillar/src/Pillar/ECS/Systems/PhysicsSystem.h
#pragma once
#include "System.h"
#include "Pillar/ECS/Physics/Box2DWorld.h"

namespace Pillar {

// Responsible for:
// 1. Creating/destroying b2Bodies for entities with RigidbodyComponent
// 2. Stepping the Box2D world
// 3. Applying forces/impulses from ECS to Box2D
class PhysicsSystem : public System
{
public:
    PhysicsSystem(const glm::vec2& gravity = { 0.0f, -9.81f });
    ~PhysicsSystem() override;
    
    void OnAttach(Scene* scene) override;
    void OnDetach() override;
    void OnUpdate(float deltaTime) override;
    
    // Access to Box2D world (for raycasts, queries)
    b2World* GetWorld() { return m_World->GetWorld(); }
    Box2DWorld* GetBox2DWorld() { return m_World.get(); }
    
private:
    std::unique_ptr<Box2DWorld> m_World;
    float m_Accumulator = 0.0f;
    const float m_FixedTimeStep = 1.0f / 60.0f; // 60 Hz physics
    
    void FixedUpdate(float fixedDeltaTime);
    void CreatePhysicsBodies();  // For new entities with Rigidbody
    void ApplyForcesFromECS();   // Read ECS components, apply to b2Bodies
};

} // namespace Pillar
```

### PhysicsSyncSystem (Box2D → EnTT) - CRITICAL

```cpp
// Pillar/src/Pillar/ECS/Systems/PhysicsSyncSystem.h
#pragma once
#include "System.h"

namespace Pillar {

// CRITICAL SYSTEM: Reads b2Body positions and writes to TransformComponent
// MUST run AFTER PhysicsSystem and BEFORE rendering
// This is the "Source of Truth" sync: Box2D -> ECS (one-way)
class PhysicsSyncSystem : public System
{
public:
    void OnUpdate(float deltaTime) override;
    
private:
    void SyncTransformsFromBox2D();
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Systems/PhysicsSyncSystem.cpp
#include "PhysicsSyncSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"

namespace Pillar {

void PhysicsSyncSystem::OnUpdate(float deltaTime)
{
    SyncTransformsFromBox2D();
}

void PhysicsSyncSystem::SyncTransformsFromBox2D()
{
    // Get all entities with both Transform and Rigidbody
    auto view = m_Scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();
    
    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rigidbody = view.get<RigidbodyComponent>(entity);
        
        if (rigidbody.Body)
        {
            // READ from Box2D (Source of Truth)
            const b2Vec2& pos = rigidbody.Body->GetPosition();
            float angle = rigidbody.Body->GetAngle();
            
            // WRITE to EnTT (for rendering)
            transform.Position = { pos.x, pos.y };
            transform.Rotation = angle;
            transform.Dirty = true; // Mark for matrix recalc
        }
    }
}

} // namespace Pillar
```

### VelocityIntegrationSystem (Light Entities)

```cpp
// Pillar/src/Pillar/ECS/Systems/VelocityIntegrationSystem.h
#pragma once
#include "System.h"

namespace Pillar {

// Custom physics for Light Entities (no Box2D)
// Simple Euler integration: pos += vel * dt
class VelocityIntegrationSystem : public System
{
public:
    void OnUpdate(float deltaTime) override;
    
private:
    void IntegrateVelocity(float deltaTime);
};

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/ECS/Systems/VelocityIntegrationSystem.cpp
#include "VelocityIntegrationSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"

namespace Pillar {

void VelocityIntegrationSystem::OnUpdate(float deltaTime)
{
    IntegrateVelocity(deltaTime);
}

void VelocityIntegrationSystem::IntegrateVelocity(float deltaTime)
{
    auto view = m_Scene->GetRegistry().view<TransformComponent, VelocityComponent>();
    
    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        auto& velocity = view.get<VelocityComponent>(entity);
        
        // Apply acceleration (gravity, wind, etc.)
        velocity.Velocity += velocity.Acceleration * deltaTime;
        
        // Apply drag
        if (velocity.Drag > 0.0f)
        {
            float dragFactor = 1.0f - (velocity.Drag * deltaTime);
            if (dragFactor < 0.0f) dragFactor = 0.0f;
            velocity.Velocity *= dragFactor;
        }
        
        // Clamp to max speed
        float speedSq = glm::dot(velocity.Velocity, velocity.Velocity);
        if (speedSq > velocity.MaxSpeed * velocity.MaxSpeed)
        {
            velocity.Velocity = glm::normalize(velocity.Velocity) * velocity.MaxSpeed;
        }
        
        // Integrate position
        transform.Position += velocity.Velocity * deltaTime;
        transform.Dirty = true;
    }
}

} // namespace Pillar
```

### BulletCollisionSystem (Raycasts)

```cpp
// Pillar/src/Pillar/ECS/Systems/BulletCollisionSystem.h
#pragma once
#include "System.h"

namespace Pillar {

class PhysicsSystem; // Forward declaration

// Uses Box2D Raycasts to detect bullet hits against Heavy Entities
// Does NOT use b2Bodies for bullets (they're Light Entities)
class BulletCollisionSystem : public System
{
public:
    BulletCollisionSystem(PhysicsSystem* physicsSystem);
    
    void OnUpdate(float deltaTime) override;
    
private:
    PhysicsSystem* m_PhysicsSystem;
    
    void ProcessBullets(float deltaTime);
    void RaycastBullet(Entity bulletEntity, const glm::vec2& start, const glm::vec2& end);
};

} // namespace Pillar
```

### XPCollectionSystem (Spatial Hash)

```cpp
// Pillar/src/Pillar/ECS/Systems/XPCollectionSystem.h
#pragma once
#include "System.h"
#include "Pillar/ECS/Physics/SpatialHashGrid.h"

namespace Pillar {

// Uses Spatial Hash Grid for fast AABB checks
// Finds XP gems near player, applies attraction
class XPCollectionSystem : public System
{
public:
    XPCollectionSystem();
    
    void OnUpdate(float deltaTime) override;
    
private:
    std::unique_ptr<SpatialHashGrid> m_SpatialGrid;
    
    void UpdateSpatialGrid();
    void ProcessGemAttraction(float deltaTime);
};

} // namespace Pillar
```

### SpriteRenderSystem (Instanced Rendering)

```cpp
// Pillar/src/Pillar/ECS/Systems/SpriteRenderSystem.h
#pragma once
#include "System.h"
#include <unordered_map>

namespace Pillar {

// Batches sprites by texture, renders using instanced draw calls
class SpriteRenderSystem : public System
{
public:
    void OnUpdate(float deltaTime) override; // deltaTime unused, but required by interface
    void OnRender(const glm::mat4& viewProjection);
    
private:
    struct InstanceData
    {
        glm::mat4 Transform;
        glm::vec4 Color;
        glm::vec4 UVRect; // (minX, minY, maxX, maxY)
    };
    
    // Batch by texture
    std::unordered_map<std::shared_ptr<Texture2D>, std::vector<InstanceData>> m_Batches;
    
    void CollectSprites();
    void RenderBatches(const glm::mat4& viewProjection);
};

} // namespace Pillar
```

---

## Game Loop & Update Order

### Critical Rule: Avoid Frame Jitter

**Frame Jitter** occurs when rendering reads positions that are "mid-update" (e.g., reading from Box2D before sync).

**Solution:** Strict system execution order:

```
FRAME START
│
├─ 1. INPUT HANDLING
│   └─ Read GLFW input, populate input events
│
├─ 2. GAME LOGIC SYSTEMS (can read/write ECS components freely)
│   ├─ PlayerControllerSystem (read input, set desired velocity)
│   ├─ AISystem (pathfinding, target selection)
│   ├─ HealthSystem (apply damage, check death)
│   └─ XPCollectionSystem (spatial hash, gem attraction)
│
├─ 3. PRE-PHYSICS (apply ECS state to Box2D)
│   └─ PhysicsSystem::ApplyForcesFromECS()
│       └─ Read VelocityComponent, call b2Body->SetLinearVelocity()
│
├─ 4. PHYSICS STEP (Box2D simulation)
│   └─ PhysicsSystem::Step(fixedDeltaTime)
│       ├─ b2World->Step(dt, velocityIterations, positionIterations)
│       └─ Box2D updates all b2Body positions/rotations
│
├─ 5. POST-PHYSICS SYNC (Box2D → EnTT) ⚠️ CRITICAL
│   └─ PhysicsSyncSystem::OnUpdate()
│       └─ Read b2Body->GetPosition(), write to TransformComponent
│
├─ 6. LIGHT ENTITY PHYSICS (custom integration)
│   ├─ VelocityIntegrationSystem (pos += vel * dt for bullets, particles)
│   └─ BulletCollisionSystem (raycasts against Heavy Entities)
│
├─ 7. RENDER PREPARATION
│   ├─ CameraSystem (update view matrix)
│   ├─ AnimationSystem (update sprite frames)
│   └─ ParticleSystem (update particle lifetime, emit new)
│
└─ 8. RENDERING
    ├─ Renderer2D::BeginScene(camera)
    ├─ SpriteRenderSystem::OnRender() (instanced draws)
    ├─ ParticleRenderSystem::OnRender() (point sprites)
    ├─ DebugRenderSystem::OnRender() (Box2D debug draw)
    └─ Renderer2D::EndScene()

FRAME END
```

### Implementation in Application.cpp

```cpp
// Pillar/src/Pillar/Application.cpp (simplified)

void Application::Run()
{
    while (m_Running)
    {
        float currentTime = GetTime();
        float deltaTime = currentTime - m_LastFrameTime;
        m_LastFrameTime = currentTime;
        
        // 1. Input Handling
        Input::Update(); // Poll GLFW events
        
        // 2-6. Layer updates (systems run inside layers)
        for (Layer* layer : m_LayerStack)
        {
            layer->OnUpdate(deltaTime);
        }
        
        // 7-8. Rendering
        m_ImGuiLayer->Begin();
        for (Layer* layer : m_LayerStack)
        {
            layer->OnImGuiRender();
        }
        m_ImGuiLayer->End();
        
        m_Window->OnUpdate();
    }
}
```

### Scene Update Order (inside GameLayer::OnUpdate)

```cpp
// Sandbox/src/GameLayer.cpp (example)

void GameLayer::OnUpdate(float deltaTime)
{
    // Order matters! Follow the frame pipeline
    
    // 2. Game Logic
    m_XPCollectionSystem->OnUpdate(deltaTime);
    m_HealthSystem->OnUpdate(deltaTime);
    
    // 3-4. Physics (fixed timestep)
    m_PhysicsSystem->OnUpdate(deltaTime); // Handles fixed timestep internally
    
    // 5. Sync Box2D -> ECS (CRITICAL)
    m_PhysicsSyncSystem->OnUpdate(deltaTime);
    
    // 6. Light Entity Physics
    m_VelocityIntegrationSystem->OnUpdate(deltaTime);
    m_BulletCollisionSystem->OnUpdate(deltaTime);
    
    // 7. Render Prep
    m_CameraController->OnUpdate(deltaTime);
    
    // 8. Rendering
    Renderer2D::BeginScene(m_Camera);
    m_SpriteRenderSystem->OnRender(m_Camera.GetViewProjectionMatrix());
    Renderer2D::EndScene();
}
```

---

## Implementation Phases

### Phase 1: Core ECS Infrastructure (4-6 hours)

**Goal:** Basic Scene + Entity + EnTT integration

**Tasks:**
1. Add EnTT to CMakeLists.txt (FetchContent)
2. Create `ECS/` folder structure
3. Implement `Scene` class (EnTT registry wrapper)
4. Implement `Entity` class (handle wrapper)
5. Define core components (Tag, Transform, UUID, Hierarchy)
6. Write unit tests for Scene and Entity

**Deliverable:**
```cpp
Scene scene;
Entity entity = scene.CreateEntity("TestEntity");
auto& transform = entity.AddComponent<TransformComponent>(glm::vec2(0, 0));
transform.Position.x = 10.0f;
scene.DestroyEntity(entity);
```

**Files:**
- `ECS/Scene.h/cpp`
- `ECS/Entity.h/cpp`
- `ECS/Components/Core/*.h`
- `Tests/src/SceneTests.cpp`
- `Tests/src/EntityTests.cpp`

---

### Phase 2: Box2D Integration (6-8 hours)

**Goal:** Heavy Entities with Box2D bodies

**Tasks:**
1. Add Box2D to CMakeLists.txt (FetchContent)
2. Create `Box2DWorld` wrapper class
3. Create `Box2DBodyFactory` (create b2Bodies with fixtures)
4. Define physics components (Rigidbody, Collider, PhysicsMaterial)
5. Implement `PhysicsSystem` (step Box2D world)
6. Implement `PhysicsSyncSystem` (Box2D → EnTT)
7. Set up EnTT `on_destroy` listener to clean up b2Bodies
8. Create `Box2DContactListener` for collision callbacks
9. Write tests for physics integration

**Deliverable:**
```cpp
// Create a Heavy Entity (player)
Entity player = scene.CreateEntity("Player");
player.AddComponent<TransformComponent>(glm::vec2(0, 0));
player.AddComponent<RigidbodyComponent>(b2_dynamicBody);
player.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));

// Physics system creates b2Body automatically
physicsSystem->OnUpdate(dt); // Step physics
physicsSyncSystem->OnUpdate(dt); // Sync positions

// Transform now has Box2D position
auto& transform = player.GetComponent<TransformComponent>();
assert(transform.Position == rigidbody.Body->GetPosition());
```

**Files:**
- `ECS/Physics/Box2DWorld.h/cpp`
- `ECS/Physics/Box2DBodyFactory.h/cpp`
- `ECS/Physics/Box2DContactListener.h/cpp`
- `ECS/Components/Physics/*.h`
- `ECS/Systems/PhysicsSystem.h/cpp`
- `ECS/Systems/PhysicsSyncSystem.h/cpp`
- `Tests/src/PhysicsSystemTests.cpp`

---

### Phase 3: Light Entity Physics (4-5 hours)

**Goal:** Velocity integration for bullets, particles

**Tasks:**
1. Define `VelocityComponent`
2. Implement `VelocityIntegrationSystem`
3. Define `BulletComponent`
4. Implement `BulletCollisionSystem` (Box2D raycasts)
5. Create bullet lifetime system (auto-destroy)
6. Write tests for velocity integration

**Deliverable:**
```cpp
// Create a Light Entity (bullet)
Entity bullet = scene.CreateEntity("Bullet");
bullet.AddComponent<TransformComponent>(glm::vec2(0, 0));
bullet.AddComponent<VelocityComponent>(glm::vec2(10, 0)); // Move right at 10 m/s
bullet.AddComponent<BulletComponent>(player, 25.0f); // Damage 25
bullet.AddComponent<SpriteComponent>(bulletTexture);

// Velocity system moves bullet
velocityIntegrationSystem->OnUpdate(dt);

// Bullet collision system raycasts
bulletCollisionSystem->OnUpdate(dt);
// If hit enemy, apply damage and destroy bullet
```

**Files:**
- `ECS/Components/Physics/VelocityComponent.h`
- `ECS/Components/Gameplay/BulletComponent.h`
- `ECS/Systems/VelocityIntegrationSystem.h/cpp`
- `ECS/Systems/BulletCollisionSystem.h/cpp`
- `Tests/src/VelocityIntegrationTests.cpp`
- `Tests/src/BulletCollisionTests.cpp`

---

### Phase 4: Spatial Hash Grid (3-4 hours)

**Goal:** Fast AABB checks for XP gems

**Tasks:**
1. Implement `SpatialHashGrid` class
2. Define `XPGemComponent`
3. Implement `XPCollectionSystem`
4. Test spatial queries (insert, query, remove)
5. Benchmark performance (10,000+ entities)

**Deliverable:**
```cpp
// Create XP gems
for (int i = 0; i < 10000; ++i)
{
    Entity gem = scene.CreateEntity("XPGem");
    gem.AddComponent<TransformComponent>(RandomPosition());
    gem.AddComponent<VelocityComponent>();
    gem.AddComponent<XPGemComponent>(1); // 1 XP
    gem.AddComponent<SpriteComponent>(gemTexture);
}

// XP collection system uses spatial hash
xpCollectionSystem->OnUpdate(dt);
// Gems near player are attracted and move toward them
```

**Files:**
- `ECS/Physics/SpatialHashGrid.h/cpp`
- `ECS/Components/Gameplay/XPGemComponent.h`
- `ECS/Systems/XPCollectionSystem.h/cpp`
- `Tests/src/SpatialHashGridTests.cpp`

---

### Phase 5: Instanced Rendering (5-6 hours)

**Goal:** Batch render 10,000+ sprites

**Tasks:**
1. Define `SpriteComponent`
2. Implement `SpriteRenderSystem` with instancing
3. Create instanced shader (per-instance transform + color)
4. Batch by texture (minimize state changes)
5. Sort by Z-order for correct layering
6. Benchmark performance (target: 60 FPS with 50,000 sprites)

**Deliverable:**
```cpp
// Create 50,000 particles
for (int i = 0; i < 50000; ++i)
{
    Entity particle = scene.CreateEntity("Particle");
    particle.AddComponent<TransformComponent>(RandomPosition());
    particle.AddComponent<VelocityComponent>(RandomVelocity());
    particle.AddComponent<SpriteComponent>(particleTexture);
}

// Render all in one batched draw call per texture
spriteRenderSystem->OnRender(camera.GetViewProjectionMatrix());
// 60+ FPS with 50,000 sprites
```

**Files:**
- `ECS/Components/Rendering/SpriteComponent.h`
- `ECS/Systems/SpriteRenderSystem.h/cpp`
- `Renderer/Shaders/InstancedSprite.glsl`

---

### Phase 6: Health & Damage System (2-3 hours)

**Goal:** Damage application and entity death

**Tasks:**
1. Define `HealthComponent` and `DamageComponent`
2. Implement `HealthSystem`
3. Integrate with `BulletCollisionSystem`
4. Emit death events (for VFX, loot drops)
5. Test damage application

**Deliverable:**
```cpp
Entity enemy = scene.CreateEntity("Enemy");
enemy.AddComponent<HealthComponent>(100.0f);
enemy.AddComponent<RigidbodyComponent>();
enemy.AddComponent<ColliderComponent>();

// Bullet hits enemy
healthSystem->ApplyDamage(enemy, 25.0f);
assert(enemy.GetComponent<HealthComponent>().CurrentHealth == 75.0f);

// Kill enemy
healthSystem->ApplyDamage(enemy, 100.0f);
// Health system destroys entity, emits death event
```

**Files:**
- `ECS/Components/Gameplay/HealthComponent.h`
- `ECS/Components/Gameplay/DamageComponent.h`
- `ECS/Systems/HealthSystem.h/cpp`
- `Tests/src/HealthSystemTests.cpp`

---

### Phase 7: Box2D Debug Rendering (2-3 hours)

**Goal:** Visualize physics shapes, contacts

**Tasks:**
1. Implement `Box2DDebugDraw` (subclass `b2Draw`)
2. Render wireframe shapes, contact points, AABBs
3. Toggle via ImGui
4. Integrate with `DebugRenderSystem`

**Deliverable:**
```cpp
// Press F3 to toggle physics debug view
if (Input::IsKeyPressed(PIL_KEY_F3))
{
    m_ShowPhysicsDebug = !m_ShowPhysicsDebug;
}

if (m_ShowPhysicsDebug)
{
    box2DDebugDraw->DrawDebugData(m_PhysicsSystem->GetWorld());
}
```

**Files:**
- `ECS/Physics/Box2DDebugDraw.h/cpp`
- `ECS/Systems/DebugRenderSystem.h/cpp`

---

## Memory Management Strategy

### Box2D Body Lifecycle

**Problem:** b2Bodies are owned by b2World, not EnTT. If we destroy an EnTT entity, the b2Body leaks.

**Solution:** EnTT `on_destroy` listener

```cpp
// In Scene constructor
m_Registry.on_destroy<RigidbodyComponent>().connect<&Scene::OnRigidbodyDestroyed>(this);

// Callback
void Scene::OnRigidbodyDestroyed(entt::registry& registry, entt::entity entity)
{
    auto& rigidbody = registry.get<RigidbodyComponent>(entity);
    if (rigidbody.Body)
    {
        m_PhysicsSystem->GetWorld()->DestroyBody(rigidbody.Body);
        rigidbody.Body = nullptr;
    }
}
```

### Spatial Hash Grid Updates

**Problem:** Entities move every frame. Spatial grid becomes stale.

**Solution:** Rebuild grid every frame (fast for 10,000 entities) or use dirty flags.

```cpp
void XPCollectionSystem::OnUpdate(float deltaTime)
{
    // Option 1: Rebuild every frame (simple, fast enough)
    m_SpatialGrid->Clear();
    auto view = m_Scene->GetRegistry().view<TransformComponent, XPGemComponent>();
    for (auto entity : view)
    {
        auto& transform = view.get<TransformComponent>(entity);
        m_SpatialGrid->Insert(entity, transform.Position);
    }
    
    // Option 2: Update only moved entities (more complex, faster)
    // Check TransformComponent.Dirty flag
}
```

---

## Performance Considerations

### Target Performance Metrics

| Metric | Target | Notes |
|--------|--------|-------|
| Heavy Entities | 2000 active | Box2D can handle this |
| Light Entities | 50,000+ | With instanced rendering |
| Physics Step | < 5 ms | 60 Hz fixed timestep |
| Rendering | < 10 ms | 1-2 draw calls per texture |
| Total Frame | < 16.67 ms | 60 FPS |

### Optimization Strategies

1. **Fixed Timestep Physics**
   - Physics runs at 60 Hz regardless of frame rate
   - Accumulator pattern prevents spiral of death
   ```cpp
   m_Accumulator += deltaTime;
   while (m_Accumulator >= m_FixedTimeStep)
   {
       FixedUpdate(m_FixedTimeStep);
       m_Accumulator -= m_FixedTimeStep;
   }
   ```

2. **Spatial Partitioning**
   - Spatial hash grid: O(1) insert, O(k) query (k = nearby entities)
   - Cell size = 2x average entity size
   - Hash function: `hash = (x / cellSize) * prime + (y / cellSize)`

3. **Instanced Rendering**
   - One draw call per texture
   - Upload transform matrices to GPU (UBO or instanced attributes)
   - Target: 10,000 sprites per draw call

4. **Object Pooling**
   - Reuse dead entities (especially bullets, particles)
   - Avoid allocations during gameplay
   ```cpp
   Entity CreateBullet()
   {
       if (!m_BulletPool.empty())
       {
           Entity bullet = m_BulletPool.back();
           m_BulletPool.pop_back();
           return bullet;
       }
       return scene.CreateEntity("Bullet");
   }
   ```

5. **Cache-Friendly Iteration**
   - EnTT stores components in contiguous arrays
   - Iterate `view<Transform, Velocity>` is cache-friendly
   - Avoid random access patterns

---

## Testing Strategy

### Unit Tests (Google Test)

```cpp
// Tests/src/ECS/SceneTests.cpp
TEST(SceneTests, CreateEntity_ReturnsValidEntity)
{
    Scene scene;
    Entity entity = scene.CreateEntity("TestEntity");
    EXPECT_TRUE(entity);
    EXPECT_EQ(entity.GetComponent<TagComponent>().Tag, "TestEntity");
}

TEST(SceneTests, DestroyEntity_RemovesEntity)
{
    Scene scene;
    Entity entity = scene.CreateEntity();
    scene.DestroyEntity(entity);
    EXPECT_FALSE(entity); // Entity handle is now invalid
}

// Tests/src/ECS/PhysicsSystemTests.cpp
TEST(PhysicsSystemTests, SyncTransform_UpdatesFromBox2D)
{
    Scene scene;
    PhysicsSystem physicsSystem;
    PhysicsSyncSystem syncSystem;
    
    Entity entity = scene.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(0, 0));
    auto& rb = entity.AddComponent<RigidbodyComponent>();
    
    physicsSystem.OnAttach(&scene);
    physicsSystem.OnUpdate(0.016f); // Create b2Body
    
    // Move Box2D body
    rb.Body->SetTransform(b2Vec2(5.0f, 10.0f), 0.0f);
    
    syncSystem.OnAttach(&scene);
    syncSystem.OnUpdate(0.016f); // Sync
    
    auto& transform = entity.GetComponent<TransformComponent>();
    EXPECT_NEAR(transform.Position.x, 5.0f, 0.001f);
    EXPECT_NEAR(transform.Position.y, 10.0f, 0.001f);
}

// Tests/src/ECS/VelocityIntegrationTests.cpp
TEST(VelocityIntegrationTests, IntegrateVelocity_UpdatesPosition)
{
    Scene scene;
    VelocityIntegrationSystem system;
    
    Entity entity = scene.CreateEntity();
    entity.AddComponent<TransformComponent>(glm::vec2(0, 0));
    entity.AddComponent<VelocityComponent>(glm::vec2(10, 0)); // 10 m/s right
    
    system.OnAttach(&scene);
    system.OnUpdate(1.0f); // 1 second
    
    auto& transform = entity.GetComponent<TransformComponent>();
    EXPECT_NEAR(transform.Position.x, 10.0f, 0.001f);
}
```

### Integration Tests

```cpp
// Tests/src/ECS/PhysicsIntegrationTests.cpp
TEST(PhysicsIntegrationTests, BulletHitsEnemy_AppliesDamage)
{
    Scene scene;
    PhysicsSystem physicsSystem;
    PhysicsSyncSystem syncSystem;
    VelocityIntegrationSystem velocitySystem;
    BulletCollisionSystem bulletSystem(&physicsSystem);
    HealthSystem healthSystem;
    
    // Create enemy (Heavy Entity)
    Entity enemy = scene.CreateEntity("Enemy");
    enemy.AddComponent<TransformComponent>(glm::vec2(10, 0));
    enemy.AddComponent<RigidbodyComponent>();
    enemy.AddComponent<ColliderComponent>(ColliderComponent::Circle(1.0f));
    enemy.AddComponent<HealthComponent>(100.0f);
    
    // Create bullet (Light Entity)
    Entity bullet = scene.CreateEntity("Bullet");
    bullet.AddComponent<TransformComponent>(glm::vec2(0, 0));
    bullet.AddComponent<VelocityComponent>(glm::vec2(20, 0)); // Moving toward enemy
    bullet.AddComponent<BulletComponent>(Entity(), 25.0f);
    
    // Simulate one frame
    physicsSystem.OnUpdate(0.016f);
    syncSystem.OnUpdate(0.016f);
    velocitySystem.OnUpdate(0.5f); // Move bullet to enemy position
    bulletSystem.OnUpdate(0.016f); // Detect hit
    healthSystem.OnUpdate(0.016f); // Apply damage
    
    auto& health = enemy.GetComponent<HealthComponent>();
    EXPECT_EQ(health.CurrentHealth, 75.0f); // 100 - 25 damage
}
```

### Performance Benchmarks

```cpp
// Tests/src/ECS/PerformanceBenchmarks.cpp
TEST(PerformanceBenchmarks, SpriteRendering_50000Entities_60FPS)
{
    Scene scene;
    SpriteRenderSystem renderSystem;
    
    // Create 50,000 sprite entities
    for (int i = 0; i < 50000; ++i)
    {
        Entity entity = scene.CreateEntity();
        entity.AddComponent<TransformComponent>(RandomPosition());
        entity.AddComponent<SpriteComponent>(testTexture);
    }
    
    renderSystem.OnAttach(&scene);
    
    // Measure rendering time
    auto start = std::chrono::high_resolution_clock::now();
    renderSystem.OnRender(glm::mat4(1.0f));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    EXPECT_LT(duration.count(), 16); // < 16 ms = 60+ FPS
}
```

---

## CMakeLists.txt Integration

```cmake
# Root CMakeLists.txt additions

# EnTT (header-only ECS)
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.12.2
)

# Box2D (2D physics engine)
FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v2.4.1
)

FetchContent_MakeAvailable(entt box2d)
```

```cmake
# Pillar/CMakeLists.txt additions

# Add ECS source files
set(ECS_SOURCES
    src/Pillar/ECS/Scene.cpp
    src/Pillar/ECS/Entity.cpp
    src/Pillar/ECS/Components/Core/UUIDComponent.cpp
    src/Pillar/ECS/Physics/Box2DWorld.cpp
    src/Pillar/ECS/Physics/Box2DBodyFactory.cpp
    src/Pillar/ECS/Physics/Box2DContactListener.cpp
    src/Pillar/ECS/Physics/Box2DDebugDraw.cpp
    src/Pillar/ECS/Physics/SpatialHashGrid.cpp
    src/Pillar/ECS/Systems/PhysicsSystem.cpp
    src/Pillar/ECS/Systems/PhysicsSyncSystem.cpp
    src/Pillar/ECS/Systems/VelocityIntegrationSystem.cpp
    src/Pillar/ECS/Systems/BulletCollisionSystem.cpp
    src/Pillar/ECS/Systems/XPCollectionSystem.cpp
    src/Pillar/ECS/Systems/SpriteRenderSystem.cpp
    src/Pillar/ECS/Systems/HealthSystem.cpp
)

# Add to Pillar library
add_library(Pillar STATIC
    # ... existing sources ...
    ${ECS_SOURCES}
)

# Link EnTT and Box2D
target_link_libraries(Pillar 
    PUBLIC 
        # ... existing libs ...
        EnTT::EnTT
        box2d::box2d
)
```

---

## Success Criteria

### Phase Completion Checklist

- [ ] **Phase 1:** Scene creates/destroys entities with components
- [ ] **Phase 2:** Heavy Entities sync correctly from Box2D
- [ ] **Phase 3:** Bullets move and detect collisions via raycast
- [ ] **Phase 4:** 10,000 XP gems query spatial hash in < 1 ms
- [ ] **Phase 5:** 50,000 sprites render at 60+ FPS
- [ ] **Phase 6:** Damage system correctly reduces health and destroys entities
- [ ] **Phase 7:** Physics debug view shows all colliders and contacts

### Final Integration Test

```cpp
// Sandbox/src/GameLayer.cpp (full game loop)

void GameLayer::OnAttach()
{
    // Create systems
    m_PhysicsSystem = new PhysicsSystem({ 0.0f, 0.0f }); // No gravity (top-down)
    m_PhysicsSyncSystem = new PhysicsSyncSystem();
    m_VelocityIntegrationSystem = new VelocityIntegrationSystem();
    m_BulletCollisionSystem = new BulletCollisionSystem(m_PhysicsSystem);
    m_XPCollectionSystem = new XPCollectionSystem();
    m_SpriteRenderSystem = new SpriteRenderSystem();
    m_HealthSystem = new HealthSystem();
    
    // Create player (Heavy Entity)
    m_Player = m_Scene->CreateEntity("Player");
    m_Player.AddComponent<TransformComponent>(glm::vec2(0, 0));
    m_Player.AddComponent<RigidbodyComponent>(b2_dynamicBody);
    m_Player.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));
    m_Player.AddComponent<HealthComponent>(100.0f);
    m_Player.AddComponent<SpriteComponent>(m_PlayerTexture);
    
    // Spawn 100 enemies (Heavy Entities)
    for (int i = 0; i < 100; ++i)
    {
        Entity enemy = m_Scene->CreateEntity("Enemy");
        enemy.AddComponent<TransformComponent>(RandomPosition());
        enemy.AddComponent<RigidbodyComponent>(b2_dynamicBody);
        enemy.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.4f));
        enemy.AddComponent<HealthComponent>(50.0f);
        enemy.AddComponent<SpriteComponent>(m_EnemyTexture);
    }
    
    // Spawn 10,000 XP gems (Light Entities)
    for (int i = 0; i < 10000; ++i)
    {
        Entity gem = m_Scene->CreateEntity("XPGem");
        gem.AddComponent<TransformComponent>(RandomPosition());
        gem.AddComponent<VelocityComponent>();
        gem.AddComponent<XPGemComponent>(1);
        gem.AddComponent<SpriteComponent>(m_GemTexture);
    }
}

void GameLayer::OnUpdate(float deltaTime)
{
    // Order matters! Follow the frame pipeline
    
    // 2. Game Logic
    m_XPCollectionSystem->OnUpdate(deltaTime);
    m_HealthSystem->OnUpdate(deltaTime);
    
    // 3-4. Physics (fixed timestep)
    m_PhysicsSystem->OnUpdate(deltaTime); // Handles fixed timestep internally
    
    // 5. Sync Box2D -> ECS (CRITICAL)
    m_PhysicsSyncSystem->OnUpdate(deltaTime);
    
    // 6. Light Entity Physics
    m_VelocityIntegrationSystem->OnUpdate(deltaTime);
    m_BulletCollisionSystem->OnUpdate(deltaTime);
    
    // 7. Health
    m_HealthSystem->OnUpdate(deltaTime);
    
    // 8. Render
    Renderer2D::BeginScene(m_Camera);
    m_SpriteRenderSystem->OnRender(m_Camera.GetViewProjectionMatrix());
    Renderer2D::EndScene();
}

void GameLayer::HandlePlayerInput(float deltaTime)
{
    // Read input, set velocity on player's b2Body
    glm::vec2 inputDir(0, 0);
    if (Input::IsKeyPressed(PIL_KEY_W)) inputDir.y += 1.0f;
    if (Input::IsKeyPressed(PIL_KEY_S)) inputDir.y -= 1.0f;
    if (Input::IsKeyPressed(PIL_KEY_A)) inputDir.x -= 1.0f;
    if (Input::IsKeyPressed(PIL_KEY_D)) inputDir.x += 1.0f;
    
    if (glm::length(inputDir) > 0.0f)
    {
        inputDir = glm::normalize(inputDir);
        auto& rb = m_Player.GetComponent<RigidbodyComponent>();
        rb.Body->SetLinearVelocity(b2Vec2(inputDir.x * m_PlayerSpeed, inputDir.y * m_PlayerSpeed));
    }
    
    // Shoot bullet on mouse click
    if (Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT))
    {
        ShootBullet();
    }
}

void GameLayer::ShootBullet()
{
    // Get player position
    auto& playerTransform = m_Player.GetComponent<TransformComponent>();
    glm::vec2 bulletDir = GetMouseWorldPosition() - playerTransform.Position;
    bulletDir = glm::normalize(bulletDir);
    
    // Create bullet (Light Entity)
    Entity bullet = m_Scene->CreateEntity("Bullet");
    bullet.AddComponent<TransformComponent>(playerTransform.Position);
    bullet.AddComponent<VelocityComponent>(bulletDir * 50.0f); // 50 m/s
    bullet.AddComponent<BulletComponent>(m_Player, 25.0f);
    bullet.AddComponent<SpriteComponent>(m_BulletTexture);
}
```

**Expected Result:**
- Player moves smoothly (Box2D velocity control)
- 100 enemies collide with player and walls
- 10,000 XP gems are attracted to player when nearby
- Bullets raycast and hit enemies (apply damage)
- Dead enemies are destroyed
- Rendering at 60+ FPS with all entities

---

## Timeline Estimate

| Phase | Time | Cumulative |
|-------|------|------------|
| Phase 1: Core ECS | 4-6 hours | 6 hours |
| Phase 2: Box2D Integration | 6-8 hours | 14 hours |
| Phase 3: Light Entity Physics | 4-5 hours | 19 hours |
| Phase 4: Spatial Hash Grid | 3-4 hours | 23 hours |
| Phase 5: Instanced Rendering | 5-6 hours | 29 hours |
| Phase 6: Health & Damage | 2-3 hours | 32 hours |
| Phase 7: Debug Rendering | 2-3 hours | 35 hours |
| **Total** | **~35 hours** | **~5 work days** |

---

## Next Steps

1. ✅ Review this implementation plan
2. ✅ Confirm architecture decisions (Selective Hybrid approach)
3. ⏳ Create feature branch: `git checkout -b feature/ecs-physics-hybrid`
4. ⏳ Start Phase 1: Core ECS Infrastructure
5. ⏳ Update PROJECT_STATUS.md after each phase

---

**Ready to implement the Selective Hybrid ECS + Box2D system!** 🚀
