# Top-Down Shooter Implementation Context

**Last Updated**: 2026-01-27
**Status**: Phase 5 Complete - Polish & Effects Implemented

---

## Project Overview

### Pillar Engine
- Custom C++ game engine with ECS architecture (EnTT)
- Box2D physics integration
- OpenAL-Soft spatial audio
- Batch-optimized 2D rendering (Renderer2D)
- Particle system with object pooling
- Layer-based application structure

### Goal
Implement a complete top-down survivor-shooter game following the TDD document at `docs/TOP_DOWN_SHOOTER_TDD.md`.

---

## Current Progress

### Phase 1: Core Framework - ✅ COMPLETE
- Player entity with WASD movement
- Mouse aiming (player rotates to face cursor)
- Space bar dash with cooldown
- Arena bounds (30x16 units with wall colliders)
- Camera following player
- ImGui debug panel

### Phase 2: Combat Mechanics - ✅ COMPLETE
- WeaponSystem handles firing with cooldowns
- Bullet spawning with VelocityComponent (lightweight)
- BulletLifetimeSystem for cleanup
- Left-click to shoot
- Bullets travel in aimed direction
- Entity count visible in debug panel

### Phase 3: Enemies - ✅ COMPLETE
- EnemyComponent with types (Chaser, Shooter, Swarm)
- EnemyAISystem with state-based behavior
- CreateEnemy in EntityFactory
- DamageSystem for bullet-enemy collisions
- Heavy enemies (Box2D) and light enemies (VelocityComponent)

### Phase 4: Wave System & Power-ups - ✅ COMPLETE
- WaveManager with escalating difficulty
- Shooters introduced at wave 3, Swarm at wave 5
- PowerUpComponent (Health, SpeedBoost, FireRateUp, DamageUp, Shield, Magnet)
- PowerUpSystem for collection and buff management
- PlayerBuffsComponent for tracking active effects
- 30% power-up drop chance on enemy death

### Phase 5: Polish & Effects - ✅ COMPLETE
- CameraShake utility (small/medium/large/huge presets)
- FlashComponent and FlashSystem for damage flash
- TemporaryComponent for auto-destroy effects
- EffectFactory for spawning particles
- Muzzle flash on weapon fire
- Hit particles on bullet impact
- Death particles when enemies die
- DamageSystem callbacks for effect integration

### Phase 6: Audio - 🔲 TODO
- Add sound effects (shoot, hit, death, pickup)
- Add background music

---

## Files Structure

```
Sandbox/src/TopDownShooter/
├── GameLayer.h                    [COMPLETE] - Main game layer with all systems
├── Components/
│   ├── WeaponComponent.h          [COMPLETE] - Weapon stats, cooldown
│   ├── PlayerTagComponent.h       [COMPLETE] - Player movement stats
│   ├── EnemyComponent.h           [COMPLETE] - Enemy types, AI state
│   ├── PowerUpComponent.h         [COMPLETE] - Power-up types, buffs
│   └── EffectComponents.h         [COMPLETE] - FlashComponent, TemporaryComponent
├── Systems/
│   ├── PlayerMovementSystem.h     [COMPLETE] - WASD + mouse aim + dash
│   ├── WeaponSystem.h             [COMPLETE] - Firing, bullet spawning, muzzle flash
│   ├── BulletLifetimeSystem.h     [COMPLETE] - Bullet cleanup
│   ├── EnemyAISystem.h            [COMPLETE] - Chaser, Shooter, Swarm AI
│   ├── DamageSystem.h             [COMPLETE] - Collisions, damage, callbacks
│   ├── PowerUpSystem.h            [COMPLETE] - Collection, buff application
│   └── EffectSystems.h            [COMPLETE] - FlashSystem, TemporaryCleanupSystem
└── Utilities/
    ├── EntityFactory.h            [COMPLETE] - CreatePlayer, CreateWall, CreateEnemy, CreatePowerUp
    ├── WaveManager.h              [COMPLETE] - Wave spawning logic
    ├── CameraShake.h              [COMPLETE] - Screen shake effects
    ├── EffectFactory.h            [COMPLETE] - Particle spawning helpers
    ├── CollisionCategories.h      [COMPLETE] - Collision filtering
    └── GameUtils.h                [COMPLETE] - Utility functions
```

---

## Pillar Engine Architecture (Key Learnings)

### Directory Structure
```
Pillar/
├── src/Pillar/
│   ├── ECS/
│   │   ├── Components/
│   │   │   ├── Core/ (TransformComponent, TagComponent, UUIDComponent)
│   │   │   ├── Physics/ (RigidbodyComponent, ColliderComponent, VelocityComponent)
│   │   │   ├── Rendering/ (SpriteComponent, AnimationComponent, CameraComponent)
│   │   │   ├── Gameplay/ (HealthComponent, BulletComponent, ParticleComponent, XPGemComponent)
│   │   │   └── Audio/ (AudioSourceComponent, AudioListenerComponent)
│   │   ├── Systems/
│   │   │   ├── PhysicsSystem.h - Box2D world management
│   │   │   ├── VelocityIntegrationSystem.h - Light entity physics
│   │   │   ├── BulletCollisionSystem.h - Raycast hit detection
│   │   │   ├── AnimationSystem.h - Sprite animation
│   │   │   ├── SpriteRenderSystem.h - Batch rendering
│   │   │   ├── ParticleEmitterSystem.h - Particle emission
│   │   │   └── ParticleSystem.h - Particle lifecycle
│   │   ├── Scene.h - Entity container and lifecycle
│   │   ├── Entity.h - Entity wrapper with component access
│   │   └── ObjectPool.h, SpecializedPools.h - Entity recycling
│   ├── Renderer/ - Renderer2D, OrthographicCamera, Texture2D
│   ├── Audio/ - AudioEngine, AudioBuffer, AudioSource
│   ├── Input.h - Keyboard/mouse input (includes mouse button codes)
│   └── Layer.h - Application layer base class
Sandbox/
├── src/
│   ├── Source.cpp - Entry point
│   ├── *DemoLayer.h - Various demo examples
│   └── TopDownShooter/ - OUR GAME CODE
└── assets/ - Textures, audio, scenes
```

### Key Patterns

#### Layer Pattern
All demos inherit from `Pillar::Layer`:
```cpp
class GameLayer : public Pillar::Layer {
    void OnAttach() override;    // Initialize
    void OnDetach() override;    // Cleanup
    void OnUpdate(float dt) override;  // Game loop
    void OnEvent(Event& e) override;   // Input events
    void OnImGuiRender() override;     // Debug UI
};
```

#### System Pattern
Systems inherit from `Pillar::System`:
```cpp
class MySystem : public Pillar::System {
    void OnAttach(Scene* scene);
    void OnDetach();
    void OnUpdate(float deltaTime);
};
```

#### Entity Creation Pattern
```cpp
auto entity = scene.CreateEntity("Name");
auto& transform = entity.GetComponent<TransformComponent>();  // Auto-added
entity.AddComponent<SpriteComponent>();
entity.AddComponent<RigidbodyComponent>(b2_dynamicBody);
```

#### Two Entity Types
1. **Heavy Entities**: Have `RigidbodyComponent` (b2Body) for full physics
2. **Light Entities**: Have `VelocityComponent` only (bullets, particles)

### Existing Pillar Components
| Component | Purpose |
|-----------|---------|
| TransformComponent | Position, rotation, scale (auto-added) |
| SpriteComponent | Texture, color, size, layer sorting |
| RigidbodyComponent | Box2D body (dynamic/static/kinematic) |
| ColliderComponent | Circle/Box/Polygon shapes, sensors |
| VelocityComponent | Simple velocity integration |
| HealthComponent | HP, damage, invulnerability, death |
| BulletComponent | Owner, damage, lifetime, pierce |
| AnimationComponent | Animation playback state |
| ParticleEmitterComponent | Particle spawning config |
| ParticleComponent | Individual particle data |

### Existing Pillar Systems
| System | Purpose |
|--------|---------|
| PhysicsSystem | Box2D world stepping, body creation |
| VelocityIntegrationSystem | Euler integration for light entities |
| BulletCollisionSystem | Raycast hit detection |
| AnimationSystem | Frame updates, clip management |
| SpriteRenderSystem | Batch rendering with sorting |
| ParticleEmitterSystem | Emission logic |
| ParticleSystem | Particle lifecycle |

---

## Current GameLayer Update Order

**OnAttach**:
1. Create Scene
2. Physics (zero gravity)
3. PlayerMovementSystem
4. WeaponSystem
5. VelocityIntegrationSystem
6. BulletLifetimeSystem
7. EnemyAISystem
8. DamageSystem (with callbacks for effects)
9. PowerUpSystem
10. FlashSystem
11. TemporaryCleanupSystem
12. Create arena bounds
13. Create player
14. Initialize WaveManager (with spawn callbacks)
15. Initialize CameraShake

**OnUpdate**:
1. WaveManager update (spawn enemies on timer)
2. CameraShake update (apply/decay shake)
3. PlayerMovementSystem (input + aiming)
4. WeaponSystem (firing, bullet spawning, muzzle flash)
5. EnemyAISystem (AI behavior, movement)
6. PhysicsSystem (Box2D for heavy entities)
7. VelocityIntegrationSystem (for bullets/light entities/swarm)
8. DamageSystem (collisions, damage, death + effect callbacks)
9. PowerUpSystem (collection, buff timers)
10. FlashSystem (damage flash effect)
11. TemporaryCleanupSystem (auto-destroy effects)
12. BulletLifetimeSystem (cleanup expired bullets)
13. Camera update (follow player + shake offset)
14. Render (all sprites + particles)

---

## Implemented Components Summary

### WeaponComponent
```cpp
struct WeaponComponent {
    std::string WeaponName = "Pistol";
    float Damage = 10.0f;
    float FireRate = 5.0f;          // Shots per second
    float BulletSpeed = 15.0f;      // Units per second
    float Spread = 2.0f;            // Degrees of random spread
    int BulletsPerShot = 1;         // For shotguns
    float FireCooldown = 0.0f;      // Countdown to next shot
    bool CanFire() const;
    void ResetCooldown();
    void UpdateCooldown(float dt);
};
```

### PlayerTagComponent
```cpp
struct PlayerTagComponent {
    float MoveSpeed = 5.0f;
    float DashSpeed = 15.0f;
    float DashDuration = 0.15f;
    float DashCooldown = 1.0f;
    bool IsDashing = false;
    float DashTimer = 0.0f;
    float DashCooldownTimer = 0.0f;
};
```

### EnemyComponent
```cpp
enum class EnemyType { Chaser, Shooter, Swarm };
enum class EnemyState { Idle, Chasing, Attacking, Stunned, Dying };

struct EnemyComponent {
    EnemyType Type = EnemyType::Chaser;
    EnemyState State = EnemyState::Idle;
    float MoveSpeed = 3.0f;
    float AttackDamage = 10.0f;
    float DetectionRange = 15.0f;
    float AttackRange = 1.0f;
    float AttackCooldown = 1.0f;
    float AttackTimer = 0.0f;
    float ShootRange = 8.0f;          // Shooter only
    float PreferredDistance = 6.0f;   // Shooter maintains this distance
};
```

### PowerUpComponent
```cpp
enum class PowerUpType { Health, SpeedBoost, FireRateUp, DamageUp, Shield, Magnet };

struct PowerUpComponent {
    PowerUpType Type = PowerUpType::Health;
    float Value = 20.0f;              // Amount to add/multiply
    float Duration = 5.0f;            // Buff duration (0 = instant)
    float BobbingTimer = 0.0f;        // Animation timer
};

struct PlayerBuffsComponent {
    float SpeedMultiplier = 1.0f;
    float SpeedBuffTimer = 0.0f;
    float FireRateMultiplier = 1.0f;
    float FireRateBuffTimer = 0.0f;
    float DamageMultiplier = 1.0f;
    float DamageBuffTimer = 0.0f;
    bool HasShield = false;
    float ShieldTimer = 0.0f;
    float MagnetRange = 0.0f;
    float MagnetTimer = 0.0f;
};
```

### EffectComponents
```cpp
struct FlashComponent {
    glm::vec4 OriginalColor;
    glm::vec4 FlashColor = {1.0f, 1.0f, 1.0f, 1.0f};
    float FlashDuration = 0.1f;
    float FlashTimer = 0.0f;
};

struct TemporaryComponent {
    float Lifetime = 1.0f;
    float Timer = 0.0f;
};
```

### CollisionCategories
```cpp
namespace Game::CollisionCategory {
    constexpr uint16_t Player   = 0x0001;
    constexpr uint16_t Enemy    = 0x0002;
    constexpr uint16_t Bullet   = 0x0004;
    constexpr uint16_t Wall     = 0x0008;
    constexpr uint16_t PowerUp  = 0x0010;
}
```

---

## Utility Classes Summary

### WaveManager
```cpp
struct WaveConfig {
    int WaveNumber;
    int ChaserCount;
    int ShooterCount;
    int SwarmCount;
    float SpawnDelay;
};

class WaveManager {
    void Update(float dt);
    void StartWave(int waveNumber);
    int GetCurrentWave() const;
    bool IsWaveComplete() const;
    void SetSpawnCallback(std::function<void(EnemyType, glm::vec2)> callback);
    // Escalation: Shooters at wave 3, Swarm at wave 5
};
```

### CameraShake
```cpp
class CameraShake {
    void Update(float dt);
    void Shake(float intensity, float duration);
    void ShakeSmall();   // 0.1 intensity
    void ShakeMedium();  // 0.2 intensity
    void ShakeLarge();   // 0.4 intensity
    void ShakeHuge();    // 0.8 intensity
    glm::vec2 GetOffset() const;
};
```

### EffectFactory
```cpp
namespace EffectFactory {
    void SpawnMuzzleFlash(Scene& scene, const glm::vec2& pos, float angle);
    void SpawnDeathParticles(Scene& scene, const glm::vec2& pos, const glm::vec4& color);
    void SpawnHitParticles(Scene& scene, const glm::vec2& pos);
    void SpawnXPGem(Scene& scene, const glm::vec2& pos);
    void SpawnDashTrail(Scene& scene, const glm::vec2& pos);
}
```

---

## Next Steps (Phase 6: Audio)

1. **Add Shoot SFX**
   - Load WAV file in OnAttach
   - Play in WeaponSystem when firing

2. **Add Hit/Death SFX**
   - Play in DamageSystem callbacks

3. **Add Pickup SFX**
   - Play in PowerUpSystem on collection

4. **Add Background Music**
   - Looping music in OnAttach
   - Volume control in debug panel

---

## Code Patterns Reference

### Entity Factory Methods
```cpp
// Enemy creation (heavy with Box2D or light with VelocityComponent)
static Pillar::Entity CreateEnemy(Pillar::Scene& scene, const glm::vec2& position, EnemyType type) {
    auto enemy = scene.CreateEntity("Enemy");
    auto& transform = enemy.GetComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);

    auto& sprite = enemy.AddComponent<Pillar::SpriteComponent>();
    sprite.Size = glm::vec2(0.8f, 0.8f);
    sprite.Color = GetColorForType(type);

    if (type == EnemyType::Swarm) {
        // Light entity - no physics body
        enemy.AddComponent<Pillar::VelocityComponent>();
    } else {
        // Heavy entity - Box2D physics
        auto& rb = enemy.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
        rb.FixedRotation = true;
        auto collider = Pillar::ColliderComponent::Circle(0.35f);
        collider.CategoryBits = CollisionCategory::Enemy;
        enemy.AddComponent<Pillar::ColliderComponent>(collider);
    }

    enemy.AddComponent<Pillar::HealthComponent>(30.0f);
    enemy.AddComponent<EnemyComponent>(type);
    return enemy;
}

// Power-up creation
static Pillar::Entity CreatePowerUp(Pillar::Scene& scene, const glm::vec2& position, PowerUpType type) {
    auto powerUp = scene.CreateEntity("PowerUp");
    auto& transform = powerUp.GetComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);

    auto& sprite = powerUp.AddComponent<Pillar::SpriteComponent>();
    sprite.Size = glm::vec2(0.5f, 0.5f);
    sprite.Color = GetPowerUpColor(type);

    powerUp.AddComponent<PowerUpComponent>(type);
    return powerUp;
}
```

### DamageSystem Callback Pattern
```cpp
// In DamageSystem.h
using HitCallback = std::function<void(entt::entity enemy, const glm::vec2& pos)>;
using DeathCallback = std::function<void(entt::entity enemy, const glm::vec2& pos)>;
using PlayerHitCallback = std::function<void(float damage)>;

class DamageSystem : public Pillar::System {
    HitCallback OnEnemyHit;
    DeathCallback OnEnemyKilled;
    PlayerHitCallback OnPlayerHit;
};

// In GameLayer OnAttach
m_DamageSystem->OnEnemyHit = [this](entt::entity e, const glm::vec2& pos) {
    EffectFactory::SpawnHitParticles(m_Scene, pos);
    m_CameraShake.ShakeSmall();
};

m_DamageSystem->OnEnemyKilled = [this](entt::entity e, const glm::vec2& pos) {
    EffectFactory::SpawnDeathParticles(m_Scene, pos, {1,0,0,1});
    m_CameraShake.ShakeMedium();
    // 30% chance to spawn power-up
    if (RandomFloat(0, 1) < 0.3f) {
        EntityFactory::CreateRandomPowerUp(m_Scene, pos);
    }
};

m_DamageSystem->OnPlayerHit = [this](float damage) {
    m_CameraShake.ShakeLarge();
};
```

### WaveManager Integration Pattern
```cpp
// In GameLayer OnAttach
m_WaveManager.SetSpawnCallback([this](EnemyType type, const glm::vec2& pos) {
    EntityFactory::CreateEnemy(m_Scene, pos, type);
});
m_WaveManager.StartWave(1);

// In GameLayer OnUpdate
m_WaveManager.Update(dt);

if (m_WaveManager.IsWaveComplete()) {
    // Check if all enemies dead
    auto enemyView = m_Scene.GetRegistry().view<EnemyComponent>();
    if (enemyView.size() == 0) {
        m_WaveManager.StartWave(m_WaveManager.GetCurrentWave() + 1);
    }
}
```

---

## Build Notes

### IMPORTANT: Use bootstrap and ensure terminal is in that environment


### Build Commands
```powershell
# Configure
cmake --preset windows-debug

# Build
cmake --build --preset windows-debug

# Run
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

### Common Issues
1. **"Cannot open include file: 'string'"**: Run from VS Developer environment
2. **LNK2038 mismatch errors**: Clean rebuild (delete build folder)
3. **MinGW being used**: CMake is picking up wrong compiler, reconfigure from VS environment

---

## Reference Documents

- **TDD**: `docs/TOP_DOWN_SHOOTER_TDD.md` - Complete technical design
- **API Reference**: `docs/API_REFERENCE.md` - Pillar API docs
- **Copilot Instructions**: `.github/copilot-instructions.md` - Engine patterns

---

## Controls (Current)

| Input | Action |
|-------|--------|
| WASD | Move player |
| Mouse Move | Aim (player rotates to face cursor) |
| Left Click | Shoot |
| Space | Dash (with cooldown) |
| Scroll | Zoom camera |
| ESC | Exit game |

---

## Game Features Summary

### Enemies
| Type | Behavior | Entity Type |
|------|----------|-------------|
| Chaser | Moves directly toward player | Heavy (Box2D) |
| Shooter | Maintains distance, fires projectiles | Heavy (Box2D) |
| Swarm | Fast, numerous, simple movement | Light (VelocityComponent) |

### Power-ups
| Type | Effect | Duration |
|------|--------|----------|
| Health | Restores 20 HP | Instant |
| SpeedBoost | +50% move speed | 5 seconds |
| FireRateUp | +50% fire rate | 5 seconds |
| DamageUp | +50% damage | 5 seconds |
| Shield | Blocks one hit | Until hit |
| Magnet | Attracts power-ups | 5 seconds |

### Wave Escalation
- Wave 1-2: Chasers only
- Wave 3-4: Chasers + Shooters
- Wave 5+: Chasers + Shooters + Swarm

---

*This context file preserves implementation progress for future sessions.*
