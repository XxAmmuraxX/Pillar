# Object Pooling System

## Overview

The object pooling system provides efficient memory management for frequently created/destroyed entities in Pillar Engine. Instead of constantly allocating and deallocating memory, entities are reused from a pre-allocated pool.

## Why Object Pooling?

### Performance Benefits
1. **Reduced Memory Allocations**: Reusing objects eliminates frequent `new`/`delete` calls
2. **Cache Efficiency**: Pooled objects stay in CPU cache, improving access times
3. **Predictable Performance**: No garbage collection spikes or memory fragmentation
4. **Faster Spawning**: Entity creation from pool is ~10-100x faster than allocation

### Use Cases
- **Bullets**: Spawn/despawn thousands per second
- **Particles**: Short-lived visual effects
- **Projectiles**: Any fast-moving temporary entities
- **XP Gems**: Collectibles that spawn frequently

## Architecture

```
???????????????????????????????????????????????????????????
?                    ObjectPool (Base)                     ?
?  - Generic reusable entity pool                         ?
?  - Init/reset callbacks for customization              ?
?  - Acquire/Release entity management                    ?
???????????????????????????????????????????????????????????
                            ?
          ?????????????????????????????????????
          ?                                   ?
  ????????????????????            ??????????????????????
  ?   BulletPool     ?            ?   ParticlePool     ?
  ?  - Bullet-specific?            ?  - Particle-specific?
  ?  - Auto-setup     ?            ?  - Visual-only     ?
  ?  - Lifetime mgmt  ?            ?  - Physics enabled ?
  ?????????????????????            ??????????????????????
```

## Quick Start

### 1. Initialize Pools

```cpp
// In your Layer's OnAttach()
void GameLayer::OnAttach()
{
    // Load textures
    m_BulletTexture = Texture2D::Create("bullet.png");
    m_ParticleTexture = Texture2D::Create("particle.png");

    // Initialize bullet pool (pre-allocate 200 bullets)
    m_BulletPool.Init(m_Scene, m_BulletTexture, 200);

    // Initialize particle pool (pre-allocate 1000 particles)
    m_ParticlePool.Init(m_Scene, m_ParticleTexture, 1000);
}
```

### 2. Spawn Entities

```cpp
// Spawn bullet from pool
Entity bullet = m_BulletPool.SpawnBullet(
    glm::vec2(0.0f, 0.0f),    // position
    glm::vec2(1.0f, 0.0f),    // direction
    10.0f,                     // speed
    playerEntity,              // owner
    25.0f,                     // damage
    5.0f                       // lifetime (seconds)
);

// Spawn particle
Entity particle = m_ParticlePool.SpawnParticle(
    glm::vec2(0.0f, 0.0f),    // position
    glm::vec2(0.0f, 5.0f),    // velocity
    glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // color (red)
    0.1f,                      // size
    2.0f                       // lifetime
);
```

### 3. Return to Pool

```cpp
// When bullet expires or hits target
m_BulletPool.ReturnBullet(bullet);

// When particle animation completes
m_ParticlePool.ReturnParticle(particle);
```

## Advanced Usage

### Custom Object Pool

Create specialized pools for your specific entity types:

```cpp
class XPGemPool
{
public:
    void Init(Scene* scene, std::shared_ptr<Texture2D> texture, uint32_t capacity)
    {
        m_Scene = scene;
        m_Texture = texture;

        // Setup initialization callback
        m_Pool.SetInitCallback([this](Entity entity) {
            entity.AddComponent<TransformComponent>();
            entity.AddComponent<VelocityComponent>();
            entity.AddComponent<XPGemComponent>();
            
            auto& sprite = entity.AddComponent<SpriteComponent>();
            sprite.Texture = m_Texture;
            sprite.Size = glm::vec2(0.3f);
        });

        // Setup reset callback
        m_Pool.SetResetCallback([](Entity entity) {
            auto& transform = entity.GetComponent<TransformComponent>();
            transform.Position = glm::vec2(0.0f);
            
            auto& gem = entity.GetComponent<XPGemComponent>();
            gem.IsAttracted = false;
        });

        m_Pool.Init(scene, capacity);
    }

    Entity SpawnGem(const glm::vec2& position, int xpValue)
    {
        Entity gem = m_Pool.Acquire();
        
        auto& transform = gem.GetComponent<TransformComponent>();
        transform.Position = position;
        
        auto& gemComp = gem.GetComponent<XPGemComponent>();
        gemComp.XPValue = xpValue;
        
        return gem;
    }

    void ReturnGem(Entity gem)
    {
        m_Pool.Release(gem);
    }

private:
    ObjectPool m_Pool;
    Scene* m_Scene = nullptr;
    std::shared_ptr<Texture2D> m_Texture;
};
```

### Automatic Lifetime Management

Create a system to automatically return entities to pools:

```cpp
class BulletLifetimeSystem : public System
{
public:
    BulletLifetimeSystem(BulletPool* pool) : m_Pool(pool) {}

    void OnUpdate(float deltaTime) override
    {
        auto view = m_Scene->GetRegistry().view<BulletComponent>();
        std::vector<Entity> expired;

        for (auto entity : view)
        {
            auto& bullet = view.get<BulletComponent>(entity);
            bullet.TimeAlive += deltaTime;

            if (bullet.TimeAlive >= bullet.Lifetime || bullet.HitsRemaining == 0)
            {
                expired.push_back(Entity(entity, m_Scene));
            }
        }

        // Return all expired bullets to pool
        for (auto& bullet : expired)
        {
            m_Pool->ReturnBullet(bullet);
        }
    }

private:
    BulletPool* m_Pool;
};
```

## Pool Statistics

Monitor pool usage for optimization:

```cpp
void OnImGuiRender()
{
    ImGui::Begin("Pool Stats");
    
    // Bullet pool stats
    ImGui::Text("Bullets:");
    ImGui::Text("  Active: %zu", m_BulletPool.GetActiveCount());
    ImGui::Text("  Available: %zu", m_BulletPool.GetAvailableCount());
    ImGui::Text("  Total: %zu", m_BulletPool.GetTotalCount());
    
    // Calculate usage percentage
    float usage = (float)m_BulletPool.GetActiveCount() / 
                  (float)m_BulletPool.GetTotalCount() * 100.0f;
    ImGui::ProgressBar(usage / 100.0f, ImVec2(0.0f, 0.0f), 
                       std::to_string((int)usage).append("%").c_str());
    
    ImGui::End();
}
```

## Performance Tips

### 1. Pre-Allocate Generously
```cpp
// Good: Pre-allocate based on maximum expected usage
m_BulletPool.Init(scene, texture, 500);  // Enough for intense combat

// Bad: Too small, causes dynamic allocations during gameplay
m_BulletPool.Init(scene, texture, 10);   // Will expand frequently
```

### 2. Return Entities Promptly
```cpp
// Good: Return immediately when no longer needed
if (bullet.TimeAlive >= bullet.Lifetime)
{
    m_BulletPool.ReturnBullet(bullet);
}

// Bad: Holding onto entities unnecessarily
// (keeps them in "active" state, reducing available pool)
```

### 3. Monitor Pool Expansion
```cpp
// Warning if pool expands beyond initial capacity
if (m_BulletPool.GetTotalCount() > initialCapacity)
{
    PIL_WARN("Bullet pool expanded to {0} entities!", 
             m_BulletPool.GetTotalCount());
}
```

### 4. Batch Operations
```cpp
// Good: Spawn multiple entities in batch
for (int i = 0; i < 10; i++)
{
    m_ParticlePool.SpawnParticle(position, velocity, color);
}

// Better: Reserve space if possible
m_ActiveParticles.reserve(10);
for (int i = 0; i < 10; i++)
{
    Entity particle = m_ParticlePool.SpawnParticle(...);
    m_ActiveParticles.push_back(particle);
}
```

## Best Practices

### DO ?
- Pre-allocate pools based on expected maximum usage
- Use specialized pools for different entity types
- Monitor pool statistics during development
- Return entities to pool as soon as they're inactive
- Reset entity state in reset callbacks

### DON'T ?
- Create pools without sufficient capacity
- Hold references to pooled entities after returning them
- Forget to implement reset callbacks (causes state pollution)
- Mix different entity types in same pool
- Return same entity to pool multiple times

## Common Pitfalls

### 1. Double Release
```cpp
// BAD: Returning entity to pool twice
m_BulletPool.ReturnBullet(bullet);
m_BulletPool.ReturnBullet(bullet);  // ERROR!

// GOOD: Check if entity is still valid
if (bullet && !m_BulletPool.IsInPool(bullet))
{
    m_BulletPool.ReturnBullet(bullet);
}
```

### 2. Incomplete Reset
```cpp
// BAD: Forgetting to reset all component state
m_Pool.SetResetCallback([](Entity entity) {
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f);
    // FORGOT to reset other components!
});

// GOOD: Reset ALL relevant component state
m_Pool.SetResetCallback([](Entity entity) {
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec2(0.0f);
    transform.Rotation = 0.0f;
    transform.Dirty = true;
    
    auto& velocity = entity.GetComponent<VelocityComponent>();
    velocity.Velocity = glm::vec2(0.0f);
    
    // ... reset all other components
});
```

### 3. Holding Dead References
```cpp
// BAD: Keeping reference after return
Entity bullet = m_BulletPool.SpawnBullet(...);
m_BulletPool.ReturnBullet(bullet);
bullet.GetComponent<TransformComponent>(); // INVALID!

// GOOD: Clear references after return
Entity bullet = m_BulletPool.SpawnBullet(...);
m_ActiveBullets.push_back(bullet);
// ... later ...
m_BulletPool.ReturnBullet(bullet);
m_ActiveBullets.erase(...);  // Remove reference
```

## Testing Object Pools

### Unit Test Example
```cpp
TEST(ObjectPoolTests, AcquireRelease)
{
    Scene scene;
    ObjectPool pool;
    pool.Init(&scene, 10);

    // Acquire entity
    Entity entity = pool.Acquire();
    EXPECT_TRUE(entity);
    EXPECT_EQ(pool.GetActiveCount(), 1);
    EXPECT_EQ(pool.GetAvailableCount(), 9);

    // Release entity
    pool.Release(entity);
    EXPECT_EQ(pool.GetActiveCount(), 0);
    EXPECT_EQ(pool.GetAvailableCount(), 10);

    // Reacquire should give same entity (reuse)
    Entity reused = pool.Acquire();
    EXPECT_EQ(entity, reused);
}
```

## Integration with Existing Systems

### With BulletCollisionSystem
```cpp
class BulletCollisionSystem : public System
{
public:
    BulletCollisionSystem(PhysicsSystem* physics, BulletPool* pool)
        : m_PhysicsSystem(physics), m_BulletPool(pool) {}

    void OnUpdate(float deltaTime) override
    {
        // ... collision detection ...
        
        if (bulletHit)
        {
            // Apply damage to target
            // ...
            
            // Return bullet to pool instead of destroying
            m_BulletPool->ReturnBullet(bullet);
        }
    }

private:
    BulletPool* m_BulletPool;
    PhysicsSystem* m_PhysicsSystem;
};
```

## References

- [Object Pool Pattern (Wikipedia)](https://en.wikipedia.org/wiki/Object_pool_pattern)
- [Game Programming Patterns: Object Pool](https://gameprogrammingpatterns.com/object-pool.html)
- [EnTT Documentation](https://github.com/skypjack/entt)
- Phase 4 Summary: Performance Optimizations for Light Entities
