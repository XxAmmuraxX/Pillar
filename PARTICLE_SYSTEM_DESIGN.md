# Particle System Design & Implementation Plan

**Date:** December 8, 2025  
**Status:** Exploration Phase  
**Goal:** Design a high-performance, flexible particle system for Pillar Engine

---

## Current State Analysis

### ✅ **What We Already Have**

1. **ParticlePool** (Pillar/ECS/SpecializedPools.h)
   - Pre-allocates up to 1000 particles
   - Manages Transform + Velocity components
   - Supports position, velocity, color, size, lifetime
   - Built on generic ObjectPool foundation

2. **SpriteComponent** (ECS/Components/Rendering/)
   - Texture support
   - Color tinting
   - Size/scale
   - Texture coordinates (sprite sheets)
   - Z-index sorting

3. **SpriteRenderSystem** (ECS/Systems/)
   - Batch rendering with automatic texture sorting
   - Z-order sorting for proper layering

4. **VelocityIntegrationSystem** (ECS/Systems/)
   - Updates position based on velocity
   - Supports acceleration (gravity, forces)

5. **Batch Renderer**
   - 10,000 quads per batch
   - Efficient GPU batching
   - Rotation support

### ❌ **What's Missing**

1. **ParticleComponent** - Lifetime, fade, scaling over time
2. **ParticleEmitter** - Spawn patterns, emission rates
3. **ParticleSystem** - Update lifetime, apply effects
4. **Particle Effects** - Trails, scaling, rotation, color gradients
5. **Emitter Shapes** - Point, line, circle, box, cone
6. **Particle Behaviors** - Attraction, repulsion, velocity curves

---

## Particle System Architecture

### **Component-Based Design**

```
Entity (Particle)
├── TransformComponent     (Position, Rotation, Scale) ✅
├── VelocityComponent      (Velocity, Acceleration) ✅
├── SpriteComponent        (Texture, Color, Size, ZIndex) ✅
└── ParticleComponent      (Lifetime, Age, Effects) ❌ NEW
```

### **System Flow**

```
ParticleEmitterSystem
  ↓ spawns
ParticleComponent + Transform + Velocity + Sprite
  ↓ updates
ParticleSystem (lifetime, fade, scale)
  ↓ updates
VelocityIntegrationSystem (movement)
  ↓ renders
SpriteRenderSystem (batch rendering)
```

---

## Implementation Plan

### **Phase 1: Core Particle Component (2-3 hours)**

#### 1.1 Create ParticleComponent

**File:** `Pillar/src/Pillar/ECS/Components/Gameplay/ParticleComponent.h`

```cpp
#pragma once

#include <glm/glm.hpp>

namespace Pillar {

    /**
     * @brief Component for particle-specific data and behavior
     * 
     * Manages:
     * - Lifetime and age tracking
     * - Size/scale curves over time
     * - Color fading and transitions
     * - Rotation curves
     */
    struct ParticleComponent
    {
        // Lifetime management
        float Lifetime = 1.0f;      // Total lifetime (seconds)
        float Age = 0.0f;           // Current age (seconds)
        bool Dead = false;          // Mark for cleanup

        // Visual effects (interpolation factors 0-1)
        glm::vec2 StartSize = {0.1f, 0.1f};
        glm::vec2 EndSize = {0.05f, 0.05f};
        
        glm::vec4 StartColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 EndColor = {1.0f, 1.0f, 1.0f, 0.0f};  // Fade out
        
        float StartRotation = 0.0f;
        float EndRotation = 0.0f;

        // Behavior flags
        bool FadeOut = true;         // Fade alpha over lifetime
        bool ScaleOverTime = false;  // Scale from Start to End
        bool RotateOverTime = false; // Rotate from Start to End

        ParticleComponent() = default;
        
        // Simple constructor
        ParticleComponent(float lifetime, const glm::vec4& startColor = {1,1,1,1})
            : Lifetime(lifetime), StartColor(startColor), EndColor(startColor)
        {
            EndColor.a = 0.0f; // Fade to transparent
        }

        // Get normalized age (0-1)
        float GetNormalizedAge() const 
        { 
            return (Lifetime > 0.0f) ? (Age / Lifetime) : 1.0f; 
        }

        // Check if particle should be removed
        bool ShouldRemove() const 
        { 
            return Dead || Age >= Lifetime; 
        }
    };

} // namespace Pillar
```

#### 1.2 Create ParticleSystem

**File:** `Pillar/src/Pillar/ECS/Systems/ParticleSystem.h`

```cpp
#pragma once

#include "Pillar/ECS/System.h"

namespace Pillar {

    /**
     * @brief System for updating particle lifetimes and visual effects
     * 
     * Responsibilities:
     * - Age particles
     * - Interpolate size, color, rotation over time
     * - Mark dead particles for cleanup
     * - Return particles to pool
     */
    class PIL_API ParticleSystem : public System
    {
    public:
        ParticleSystem() = default;
        virtual ~ParticleSystem() = default;

        void OnUpdate(float dt) override;

        // Statistics
        uint32_t GetActiveParticleCount() const { return m_ActiveCount; }
        uint32_t GetDeadParticleCount() const { return m_DeadCount; }

    private:
        void UpdateParticle(Entity entity, float dt);
        void CleanupDeadParticles();

        uint32_t m_ActiveCount = 0;
        uint32_t m_DeadCount = 0;
    };

} // namespace Pillar
```

**File:** `Pillar/src/Pillar/ECS/Systems/ParticleSystem.cpp`

```cpp
#include "ParticleSystem.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/Logger.h"

namespace Pillar {

    void ParticleSystem::OnUpdate(float dt)
    {
        if (!m_Scene)
            return;

        m_ActiveCount = 0;
        m_DeadCount = 0;

        // Update all particles
        auto view = m_Scene->GetRegistry().view<ParticleComponent, TransformComponent, SpriteComponent>();
        
        std::vector<Entity> deadParticles;
        deadParticles.reserve(100); // Pre-allocate for performance

        for (auto entityHandle : view)
        {
            Entity entity(entityHandle, m_Scene);
            auto& particle = view.get<ParticleComponent>(entityHandle);

            // Age the particle
            particle.Age += dt;

            if (particle.ShouldRemove())
            {
                particle.Dead = true;
                deadParticles.push_back(entity);
                m_DeadCount++;
                continue;
            }

            m_ActiveCount++;

            // Update visual effects
            float t = particle.GetNormalizedAge(); // 0 to 1

            auto& transform = view.get<TransformComponent>(entityHandle);
            auto& sprite = view.get<SpriteComponent>(entityHandle);

            // Interpolate size
            if (particle.ScaleOverTime)
            {
                transform.Scale = glm::mix(particle.StartSize, particle.EndSize, t);
                transform.Dirty = true;
            }

            // Interpolate color
            if (particle.FadeOut)
            {
                sprite.Color = glm::mix(particle.StartColor, particle.EndColor, t);
            }

            // Interpolate rotation
            if (particle.RotateOverTime)
            {
                transform.Rotation = glm::mix(particle.StartRotation, particle.EndRotation, t);
                transform.Dirty = true;
            }
        }

        // Cleanup dead particles (could return to pool here)
        for (auto entity : deadParticles)
        {
            // TODO: Return to ParticlePool instead of destroying
            m_Scene->DestroyEntity(entity);
        }
    }

} // namespace Pillar
```

#### 1.3 Update ParticlePool to use ParticleComponent

**Changes to:** `Pillar/src/Pillar/ECS/SpecializedPools.cpp`

```cpp
void ParticlePool::Init(Scene* scene, uint32_t initialCapacity)
{
    // ... existing code ...

    // Set up initialization callback
    m_Pool.SetInitCallback([this](Entity entity) {
        entity.AddComponent<TransformComponent>();
        entity.AddComponent<VelocityComponent>();
        entity.AddComponent<SpriteComponent>(); // Add sprite for rendering
        entity.AddComponent<ParticleComponent>(); // NEW!
    });

    // Set up reset callback
    m_Pool.SetResetCallback([](Entity entity) {
        auto& transform = entity.GetComponent<TransformComponent>();
        transform.Position = glm::vec2(0.0f);
        transform.Scale = glm::vec2(1.0f);
        transform.Rotation = 0.0f;
        transform.Dirty = true;

        auto& velocity = entity.GetComponent<VelocityComponent>();
        velocity.Velocity = glm::vec2(0.0f);
        velocity.Acceleration = glm::vec2(0.0f);

        auto& particle = entity.GetComponent<ParticleComponent>();
        particle.Age = 0.0f;
        particle.Dead = false;

        auto& sprite = entity.GetComponent<SpriteComponent>();
        sprite.Color = glm::vec4(1.0f);
    });

    // ... rest of existing code ...
}

Entity ParticlePool::SpawnParticle(
    const glm::vec2& position,
    const glm::vec2& velocity,
    const glm::vec4& color,
    float size,
    float lifetime)
{
    Entity particle = m_Pool.Acquire();

    // Transform
    auto& transform = particle.GetComponent<TransformComponent>();
    transform.Position = position;
    transform.Scale = glm::vec2(size);
    transform.Dirty = true;

    // Velocity
    auto& vel = particle.GetComponent<VelocityComponent>();
    vel.Velocity = velocity;
    vel.Acceleration = glm::vec2(0.0f, -2.0f); // Light gravity

    // Sprite (NEW)
    auto& sprite = particle.GetComponent<SpriteComponent>();
    sprite.Color = color;
    sprite.Size = glm::vec2(size);

    // Particle (NEW)
    auto& particleComp = particle.GetComponent<ParticleComponent>();
    particleComp.Lifetime = lifetime;
    particleComp.Age = 0.0f;
    particleComp.Dead = false;
    particleComp.StartColor = color;
    particleComp.EndColor = glm::vec4(color.r, color.g, color.b, 0.0f); // Fade out
    particleComp.FadeOut = true;

    return particle;
}
```

**Estimated Time:** 2-3 hours

---

### **Phase 2: Particle Emitter System (3-4 hours)**

#### 2.1 Create ParticleEmitterComponent

**File:** `Pillar/src/Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h`

```cpp
#pragma once

#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

    // Forward declarations
    class ParticlePool;
    class Texture2D;

    /**
     * @brief Shape types for particle emission
     */
    enum class EmitterShape
    {
        Point,      // Single point
        Circle,     // Circle perimeter
        Sphere,     // Filled circle
        Box,        // Rectangle area
        Cone,       // Directional cone
        Line        // Line segment
    };

    /**
     * @brief Component for continuous particle emission
     * 
     * Emits particles at regular intervals with configurable properties
     */
    struct ParticleEmitterComponent
    {
        // Emission control
        bool Active = true;
        bool Loop = true;               // Restart after duration
        float Duration = 5.0f;          // How long to emit (-1 = infinite)
        float EmissionRate = 10.0f;     // Particles per second
        int BurstCount = 0;             // Emit this many instantly (0 = continuous)
        
        // Timing
        float TimeSinceLastEmit = 0.0f;
        float Age = 0.0f;

        // Emission shape
        EmitterShape Shape = EmitterShape::Point;
        glm::vec2 ShapeSize = {1.0f, 1.0f};  // Box size, circle radius, etc.
        float EmissionAngle = 0.0f;           // Direction (radians)
        float EmissionArc = 360.0f;           // Cone spread (degrees)

        // Particle properties
        float ParticleLifetime = 1.0f;
        glm::vec2 StartSize = {0.1f, 0.1f};
        glm::vec2 EndSize = {0.05f, 0.05f};
        glm::vec4 StartColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 EndColor = {1.0f, 1.0f, 1.0f, 0.0f};

        // Velocity
        float StartSpeed = 5.0f;
        float EndSpeed = 2.0f;
        glm::vec2 Gravity = {0.0f, -2.0f};

        // Randomization (variance)
        float SpeedVariance = 0.2f;        // ±20% speed
        float LifetimeVariance = 0.1f;     // ±10% lifetime
        float SizeVariance = 0.1f;         // ±10% size
        float AngleVariance = 15.0f;       // ±15 degrees

        // Texture (optional)
        std::shared_ptr<Texture2D> ParticleTexture;

        ParticleEmitterComponent() = default;
    };

} // namespace Pillar
```

#### 2.2 Create ParticleEmitterSystem

**File:** `Pillar/src/Pillar/ECS/Systems/ParticleEmitterSystem.h`

```cpp
#pragma once

#include "Pillar/ECS/System.h"
#include "Pillar/ECS/SpecializedPools.h"

namespace Pillar {

    /**
     * @brief System for managing particle emitters
     * 
     * Handles:
     * - Emission timing and rates
     * - Shape-based spawning
     * - Randomization
     * - Burst emissions
     */
    class PIL_API ParticleEmitterSystem : public System
    {
    public:
        ParticleEmitterSystem(ParticlePool* particlePool);
        virtual ~ParticleEmitterSystem() = default;

        void OnUpdate(float dt) override;

    private:
        void UpdateEmitter(Entity entity, float dt);
        void EmitParticle(Entity emitter);
        glm::vec2 GetRandomPositionInShape(const ParticleEmitterComponent& emitter, 
                                          const glm::vec2& emitterPos);
        glm::vec2 GetRandomVelocity(const ParticleEmitterComponent& emitter);

        ParticlePool* m_ParticlePool = nullptr;
    };

} // namespace Pillar
```

**Implementation highlights:**
- Emission timing based on rate
- Shape-based spawning (point, circle, box, cone)
- Randomization of properties
- Burst support (one-time or repeating)
- Pool integration

**Estimated Time:** 3-4 hours

---

### **Phase 3: Advanced Features (4-6 hours)**

#### 3.1 Particle Effects & Behaviors

**Features:**
- **Velocity curves** - Accelerate/decelerate over time
- **Attraction points** - Particles drawn to positions
- **Turbulence** - Perlin noise-based movement
- **Color gradients** - Multi-key color transitions
- **Sprite animation** - Animated texture frames
- **Sub-emitters** - Particles that spawn particles
- **Trails** - Spawn particles behind moving objects

#### 3.2 Emitter Presets

**File:** `Pillar/src/Pillar/Renderer/ParticlePresets.h`

```cpp
namespace Pillar::ParticlePresets {

    // Common effect templates
    ParticleEmitterComponent CreateExplosion();
    ParticleEmitterComponent CreateSmoke();
    ParticleEmitterComponent CreateFire();
    ParticleEmitterComponent CreateSparks();
    ParticleEmitterComponent CreateMagicTrail();
    ParticleEmitterComponent CreateBlood();
    ParticleEmitterComponent CreateDust();
    ParticleEmitterComponent CreateRain();
    ParticleEmitterComponent CreateSnow();

} // namespace Pillar::ParticlePresets
```

#### 3.3 Texture Atlas Support

- Pack multiple particle sprites into one texture
- Reduces draw calls
- Allows varied particle visuals

**Estimated Time:** 4-6 hours

---

## Performance Considerations

### **CPU Performance**

| Optimization | Impact | Effort |
|-------------|--------|--------|
| Object pooling (already done) | ✅ Very High | ✅ Done |
| Batch rendering (already done) | ✅ Very High | ✅ Done |
| Pre-allocate cleanup lists | Medium | Low |
| SIMD for position updates | High | Medium |
| Multi-threaded particle updates | High | High |
| Spatial hashing for attraction | Medium | Medium |

### **GPU Performance**

| Optimization | Impact | Effort |
|-------------|--------|--------|
| Instanced rendering | Very High | High |
| Geometry shaders | High | High |
| Compute shader particles | Very High | Very High |
| Texture atlases | High | Medium |

### **Memory Usage**

**Per Particle (Current):**
```
TransformComponent:  ~32 bytes (position, scale, rotation, dirty)
VelocityComponent:   ~32 bytes (velocity, acceleration, max speed)
SpriteComponent:     ~64 bytes (texture ptr, color, size, uvs, z)
ParticleComponent:   ~80 bytes (lifetime, colors, sizes, rotations)
-----------------------------------------------------------
Total:              ~208 bytes per particle
```

**1000 particles = 203 KB**  
**10,000 particles = 2 MB**  
**100,000 particles = 20 MB** (GPU-side would be better)

---

## Usage Examples

### **Example 1: Simple Explosion Effect**

```cpp
// In GameLayer.cpp
void SpawnExplosion(glm::vec2 position)
{
    Entity emitter = m_Scene->CreateEntity("Explosion");
    
    auto& transform = emitter.GetComponent<TransformComponent>();
    transform.Position = position;
    
    auto& emitterComp = emitter.AddComponent<ParticleEmitterComponent>();
    emitterComp.Active = true;
    emitterComp.Loop = false;
    emitterComp.Duration = 0.5f;
    emitterComp.EmissionRate = 100.0f;
    emitterComp.Shape = EmitterShape::Sphere;
    emitterComp.ShapeSize = {0.5f, 0.5f};
    
    emitterComp.ParticleLifetime = 1.0f;
    emitterComp.StartSize = {0.3f, 0.3f};
    emitterComp.EndSize = {0.05f, 0.05f};
    emitterComp.StartColor = {1.0f, 0.5f, 0.0f, 1.0f}; // Orange
    emitterComp.EndColor = {0.3f, 0.1f, 0.0f, 0.0f};   // Dark fade
    
    emitterComp.StartSpeed = 8.0f;
    emitterComp.Gravity = {0.0f, -5.0f};
}
```

### **Example 2: Continuous Fire Effect**

```cpp
void CreateFireEmitter(Entity entity)
{
    auto& emitter = entity.AddComponent<ParticleEmitterComponent>();
    emitter.Active = true;
    emitter.Loop = true;
    emitter.Duration = -1.0f; // Infinite
    emitter.EmissionRate = 20.0f;
    emitter.Shape = EmitterShape::Circle;
    emitter.ShapeSize = {0.2f, 0.2f};
    
    emitter.ParticleLifetime = 0.8f;
    emitter.StartColor = {1.0f, 0.8f, 0.0f, 1.0f}; // Yellow
    emitter.EndColor = {1.0f, 0.0f, 0.0f, 0.0f};   // Red fade
    emitter.StartSpeed = 2.0f;
    emitter.Gravity = {0.0f, 1.0f}; // Upward
    emitter.SpeedVariance = 0.3f;
}
```

### **Example 3: Magic Trail**

```cpp
// In PlayerController::OnUpdate()
if (isMoving)
{
    // Spawn trail particle
    glm::vec2 trailPos = playerPos - velocity * 0.1f;
    
    m_ParticlePool.SpawnParticle(
        trailPos,
        glm::vec2(0.0f), // No velocity
        glm::vec4(0.5f, 0.0f, 1.0f, 1.0f), // Purple
        0.15f,  // size
        0.5f    // lifetime
    );
}
```

---

## Integration with Existing Systems

### **With Physics**
- Particles affected by wind forces
- Collision with world geometry
- Particle-to-particle interactions

### **With Audio**
- Sound effects on emission
- 3D audio positioning

### **With Renderer**
- Additive blending for fire/magic
- Alpha blending for smoke
- Z-sorting for correct layering

---

## Testing Strategy

### **Unit Tests**
1. ParticleComponent age calculations
2. Emitter timing and emission rates
3. Shape-based spawning accuracy
4. Randomization distributions

### **Performance Tests**
1. 1,000 particles @ 60 FPS
2. 10,000 particles @ 30 FPS
3. Memory usage profiling
4. Batch rendering efficiency

### **Visual Tests (Demos)**
1. Explosion effect demo
2. Fire/smoke effect demo
3. Magic trail demo
4. Weather effects (rain/snow)
5. Impact effects (sparks, dust)

---

## Recommended Implementation Order

### **✅ Phase 1: Foundation (Recommended First)**
1. ParticleComponent
2. ParticleSystem (lifetime management)
3. Update ParticlePool integration
4. Basic demo layer

**Time:** 2-3 hours  
**Value:** Particles become visible and manageable

### **🟡 Phase 2: Emitters (Next Priority)**
1. ParticleEmitterComponent
2. ParticleEmitterSystem
3. Shape-based spawning
4. Emitter demo layer

**Time:** 3-4 hours  
**Value:** Automated particle effects

### **🟢 Phase 3: Polish (Later)**
1. Effect presets
2. Advanced behaviors
3. Texture atlas support
4. Performance optimization

**Time:** 4-6 hours  
**Value:** Production-ready effects library

---

## Alternative Approaches

### **GPU Particle Systems** (Not Recommended Yet)
- Compute shaders (OpenGL 4.3+)
- 100,000+ particles possible
- More complex to implement
- Better for Phase 6+

### **Third-Party Libraries** (Not Recommended)
- ParticleFX, Effekseer, Unity VFX Graph
- Reduces control and learning
- Increases dependencies
- Harder to integrate with ECS

---

## Success Metrics

**Performance Goals:**
- 1,000 active particles @ 60 FPS (target)
- 5,000 active particles @ 30 FPS (stretch)
- < 5 draw calls for all particles (batch by texture)
- < 2ms CPU time for particle update

**Feature Goals:**
- 5+ emitter presets (explosion, fire, smoke, etc.)
- 3+ emitter shapes (point, circle, box)
- Color/size/rotation interpolation
- Texture support

**Code Quality Goals:**
- Unit tested components
- Performance demo layer
- Documented API
- Example usage in sandbox

---

## Next Steps

1. **Choose Implementation Phase** (Phase 1 recommended)
2. **Create ParticleComponent**
3. **Create ParticleSystem**
4. **Update ParticlePool**
5. **Create demo layer**
6. **Test and iterate**

---

## References

- Unity VFX Graph
- Unreal Niagara
- "Real-Time Rendering" (4th Edition) - Chapter 13
- GDC: "Advanced Particle Systems"
- Current Implementation: `SpecializedPools.cpp`, `SpriteRenderSystem.cpp`

**Last Updated:** December 8, 2025
