# Implementation Plan: Integrate Unused Pillar Engine Features + Add 2D Lighting

> Context dump for implementation in a separate session.

---

## Overview

Three independent workstreams:
1. **BulletPool** - Replace direct bullet entity creation/destruction with engine's `BulletPool`
2. **ParticleEmitterComponent** - Attach continuous emitters to hazard entities and player dash
3. **Lighting2D** - Full 2D lighting with shadows on walls, point lights on entities

**Skipping SpriteRenderSystem** - The game's custom layer-based rendering (sorts by `sprite.Layer` alphabetically, then `sprite.OrderInLayer`) with hazard pulsing, bullet trail passes, and screen flash overlays is incompatible with the engine's `SpriteRenderSystem` which sorts by texture/Z-order.

---

## Architecture Context

### Game Structure
- **Main game layer**: `Sandbox/src/TopDownShooter/SwarmSlayerLayer.h` (~1218 lines) - owns the Scene, all systems, camera, manages game state machine
- **Entity creation**: `Sandbox/src/TopDownShooter/Utilities/EntityFactory.h` (~658 lines) - static factory methods for player, enemies, bosses, walls, hazards, power-ups, XP orbs
- **Particle effects**: `Sandbox/src/TopDownShooter/Utilities/ParticleManager.h` (~591 lines) - singleton wrapper that already uses engine's `ParticlePool`, `ParticleSystem`, and `ParticleEmitterSystem` internally for burst effects
- **Effect spawning**: `Sandbox/src/TopDownShooter/Utilities/EffectFactory.h` (~144 lines) - static methods delegating to ParticleManager
- **Bullet creation**: Done directly in `WeaponSystem.h` (player), `EnemyAISystem.h` (enemies), `BossSystem.h` (bosses) via `m_Scene->CreateEntity()`
- **Bullet destruction**: Done in `BulletLifetimeSystem.h` via `m_Scene->DestroyEntity()`

### Engine APIs Available

#### BulletPool (`Pillar/src/Pillar/ECS/SpecializedPools.h/.cpp`)
```cpp
class BulletPool {
    void Init(Scene* scene, uint32_t initialCapacity = 200);
    Entity SpawnBullet(const glm::vec2& position, const glm::vec2& direction,
                       float speed, Entity owner, float damage = 25.0f, float lifetime = 5.0f);
    void ReturnBullet(Entity bullet);
    size_t GetAvailableCount() const;
    size_t GetActiveCount() const;
    void Clear();
};
```
Currently adds TransformComponent, VelocityComponent, BulletComponent on init. Does NOT add SpriteComponent (has a TODO comment about it on line 24).

#### ParticleEmitterComponent (`Pillar/src/Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h`)
```cpp
struct ParticleEmitterComponent {
    bool Enabled = true;
    float EmissionRate = 10.0f;        // particles/sec
    bool BurstMode = false;
    int BurstCount = 100;

    enum class EmissionShape { Point, Circle, Box, Cone };
    EmissionShape Shape = EmissionShape::Point;
    glm::vec2 ShapeSize = glm::vec2(1.0f);

    glm::vec2 Direction = glm::vec2(0.0f, 1.0f);
    float DirectionSpread = 30.0f;     // degrees
    float Speed = 5.0f;
    float SpeedVariance = 2.0f;

    float Lifetime = 2.0f;
    float LifetimeVariance = 0.5f;
    float Size = 0.2f;
    float SizeVariance = 0.05f;
    glm::vec4 StartColor = glm::vec4(1.0f);
    glm::vec4 ColorVariance = glm::vec4(0.1f, 0.1f, 0.1f, 0.0f);

    bool FadeOut = true;
    bool ScaleOverTime = false;
    float EndScale = 0.5f;
    glm::vec2 Gravity = glm::vec2(0.0f, -2.0f);
};
```
The `ParticleEmitterSystem` already runs inside `ParticleManager::OnUpdate(dt)` (line 78 of ParticleManager.h). So any entity with a `ParticleEmitterComponent` will automatically emit particles - no additional system registration needed.

#### Lighting2D (`Pillar/src/Pillar/Renderer/Lighting2D.h`)
```cpp
class Lighting2D {
    static void Init();
    static void Shutdown();
    static void BeginScene(const OrthographicCamera& camera,
        uint32_t viewportWidth, uint32_t viewportHeight,
        const Lighting2DSettings& settings = {});
    static void SubmitLight(const Light2DSubmit& light);
    static void SubmitShadowCaster(const ShadowCaster2DSubmit& caster);
    static void EndScene();
};

struct Lighting2DSettings {
    glm::vec3 AmbientColor{ 1.0f, 1.0f, 1.0f };
    float AmbientIntensity = 0.15f;
    bool EnableShadows = true;
};
```
**Critical**: `BeginScene()` internally calls `Renderer2D::BeginScene(camera)`. `EndScene()` internally calls `Renderer2D::EndScene()`, then renders light accumulation and composites.

#### Light2DComponent (`Pillar/src/Pillar/ECS/Components/Rendering/Light2DComponent.h`)
```cpp
struct Light2DComponent {
    Light2DType Type = Light2DType::Point;  // Point or Spot
    glm::vec3 Color{ 1.0f, 0.85f, 0.6f };
    float Intensity = 1.0f;
    float Radius = 6.0f;
    bool CastShadows = true;
    float ShadowStrength = 1.0f;
    uint32_t LayerMask = 0xFFFFFFFFu;
};
```

#### ShadowCaster2DComponent (`Pillar/src/Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h`)
```cpp
struct ShadowCaster2DComponent {
    std::vector<glm::vec2> Points;  // Local-space, CCW
    bool Closed = true;
    bool TwoSided = false;
    uint32_t LayerMask = 0xFFFFFFFFu;
};
```

#### Lighting2DSystem (`Pillar/src/Pillar/ECS/Systems/Lighting2DSystem.h`)
Collects all entities with `Light2DComponent + TransformComponent` and `ShadowCaster2DComponent + TransformComponent`, transforms shadow caster points to world space, and submits them to `Lighting2D`.

### Current Rendering Pipeline (SwarmSlayerLayer.h lines 924-1058)

```
1. Clear with charred black (#0A0A0C)
2. Collect all visible sprites, sort by (Layer alphabetical, OrderInLayer numerical)
3. Render bullet trails (own BeginScene/EndScene pass)
4. For each sorted sprite:
   - On layer change: flush (EndScene) and start new scene (BeginScene)
   - Hazard entities get pulsed color via GetPulseFactor()
   - Others drawn directly with DrawSprite()
5. End final sprite scene
6. Separate scene for overlays:
   - Boss health bars
   - Screen flash (red overlay, fades)
   - Low health vignette (pulsing red)
```

### Current System Init Order (SwarmSlayerLayer.h InitializeGameWorld, lines 361-488)

```
Scene creation → PhysicsSystem → PhysicsSyncSystem → PlayerMovementSystem →
WeaponSystem → VelocityIntegrationSystem → BulletLifetimeSystem →
BulletCollisionSystem → EnemyAISystem → BossSystem → DamageSystem →
PowerUpSystem → XPSystem → FlashSystem → TemporaryCleanupSystem →
ParticleManager → BulletTrailSystem → HazardSystem → AnimationSystem →
WaveManager → Arena walls → Hazards → Player
```

### Current System Shutdown (SwarmSlayerLayer.h ShutdownGameSystems, lines 666-689)

```
ParticleManager → HazardSystem → BulletTrailSystem → AnimationSystem →
TemporaryCleanupSystem → FlashSystem → XPSystem → PowerUpSystem →
DamageSystem → BossSystem → EnemyAISystem → BulletCollisionSystem →
BulletLifetimeSystem → VelocitySystem → WeaponSystem →
PlayerMovementSystem → PhysicsSyncSystem → PhysicsSystem
```

### Current Update Loop Order (SwarmSlayerLayer.h UpdateGame, lines 695-813)

```
WeaponSwitch → ComboTimer → Regeneration → WaveManager →
PlayerMovementSystem → EnemyAISystem → BossSystem → WeaponSystem →
PhysicsSystem → PhysicsSyncSystem → VelocityIntegration →
BulletCollisionSystem → DamageSystem → PowerUpSystem → XPSystem →
FlashSystem → AnimationSystem → BulletLifetimeSystem →
TemporaryCleanupSystem → BulletTrailSystem → HazardSystem →
ParticleManager → HUD timers → ScreenFlash → KillStreak →
CameraShake → Camera → AudioListener
```

### Member Variables (SwarmSlayerLayer.h lines 1173-1215)

```cpp
std::unique_ptr<Pillar::Scene> m_Scene;
std::unique_ptr<Pillar::OrthographicCameraController> m_CameraController;

// All system pointers (raw, manually deleted in ShutdownGameSystems)
Pillar::PhysicsSystem* m_PhysicsSystem = nullptr;
// ... (16 system pointers total)

WaveManager m_WaveManager;
CameraShake m_CameraShake;
MenuRenderer m_MenuRenderer;

int m_KillStreakCount = 0;
float m_KillStreakTimer = 0.0f;
float m_ScreenFlashTimer = 0.0f;
Pillar::Entity m_PlayerEntity;
float m_WindowWidth = 1280.0f;
float m_WindowHeight = 720.0f;
```

---

## Files to Modify

| File | Changes |
|------|---------|
| `Pillar/src/Pillar/ECS/SpecializedPools.cpp` | Add SpriteComponent to BulletPool init/reset callbacks |
| `Sandbox/src/TopDownShooter/SwarmSlayerLayer.h` | Add BulletPool + Lighting2D members, init, shutdown, refactor RenderGame |
| `Sandbox/src/TopDownShooter/Systems/WeaponSystem.h` | Use BulletPool for player bullets |
| `Sandbox/src/TopDownShooter/Systems/EnemyAISystem.h` | Use BulletPool for enemy bullets |
| `Sandbox/src/TopDownShooter/Systems/BossSystem.h` | Use BulletPool for boss projectiles |
| `Sandbox/src/TopDownShooter/Systems/BulletLifetimeSystem.h` | Return bullets to pool instead of destroying |
| `Sandbox/src/TopDownShooter/Systems/HazardSystem.h` | Barrel light flicker effect |
| `Sandbox/src/TopDownShooter/Systems/PlayerMovementSystem.h` | Toggle dash emitter on/off |
| `Sandbox/src/TopDownShooter/Utilities/EntityFactory.h` | Add Light2D, ShadowCaster2D, ParticleEmitter components to entities |
| `Sandbox/src/TopDownShooter/Utilities/EffectFactory.h` | Add temporary light spawners (muzzle flash, explosion) |

---

## Workstream 1: BulletPool

### Step 1.1 - Add SpriteComponent to BulletPool (SpecializedPools.cpp)

In `BulletPool::Init` init callback (line 20-25), add after BulletComponent:
```cpp
auto& sprite = entity.AddComponent<SpriteComponent>();
sprite.Visible = false; // Hidden when pooled
```

In reset callback (line 28-40), add:
```cpp
auto& sprite = entity.GetComponent<SpriteComponent>();
sprite.Visible = false;
sprite.Color = glm::vec4(1.0f);
```

### Step 1.2 - Add BulletPool to SwarmSlayerLayer

- Add `#include <Pillar/ECS/SpecializedPools.h>`
- Add member `Pillar::BulletPool m_BulletPool;`
- In `InitializeGameWorld()` after scene creation (~line 364): `m_BulletPool.Init(m_Scene.get(), 300);`
- After creating each system, pass pool:
  ```cpp
  m_WeaponSystem->SetBulletPool(&m_BulletPool);
  m_EnemyAISystem->SetBulletPool(&m_BulletPool);
  m_BossSystem->SetBulletPool(&m_BulletPool);
  m_BulletLifetimeSystem->SetBulletPool(&m_BulletPool);
  ```
- In `ShutdownGameSystems()` at the top (before system deletion): `m_BulletPool.Clear();`

### Step 1.3 - WeaponSystem uses BulletPool (WeaponSystem.h)

Add member and setter:
```cpp
Pillar::BulletPool* m_BulletPool = nullptr;
void SetBulletPool(Pillar::BulletPool* pool) { m_BulletPool = pool; }
```

Replace `CreateBullet()` (line 167-208):
```cpp
void CreateBullet(Pillar::Entity owner, const glm::vec2& position,
    const glm::vec2& direction, float speed, float damage, int extraPierce = 0)
{
    Pillar::Entity bullet;
    if (m_BulletPool)
    {
        bullet = m_BulletPool->SpawnBullet(position, direction, speed, owner, damage, 3.0f);
    }
    else
    {
        bullet = m_Scene->CreateEntity("Bullet");
        auto& transform = bullet.GetComponent<Pillar::TransformComponent>();
        transform.SetPosition(position);
        float angle = std::atan2(direction.y, direction.x);
        transform.SetRotation(angle);
        bullet.AddComponent<Pillar::VelocityComponent>().Velocity = direction * speed;
        bullet.AddComponent<Pillar::BulletComponent>(owner, damage).Lifetime = 3.0f;
        bullet.AddComponent<Pillar::SpriteComponent>();
    }

    // Configure sprite
    auto& sprite = bullet.GetComponent<Pillar::SpriteComponent>();
    sprite.Texture = m_BulletTexture;
    sprite.Size = glm::vec2(0.4f, 0.4f);
    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    sprite.Layer = "Projectiles";
    sprite.OrderInLayer = 5;
    sprite.Visible = true;

    // Configure velocity max speed
    auto& velocity = bullet.GetComponent<Pillar::VelocityComponent>();
    velocity.MaxSpeed = speed * 1.5f;

    // Configure pierce
    auto& bulletComp = bullet.GetComponent<Pillar::BulletComponent>();
    bulletComp.Pierce = extraPierce > 0;
    bulletComp.MaxHits = 1 + extraPierce;
    bulletComp.HitsRemaining = 1 + extraPierce;

    // Bullet trail
    glm::vec4 trailColor = GetTrailColorForWeapon();
    if (bullet.HasComponent<BulletTrailComponent>())
        bullet.GetComponent<BulletTrailComponent>() = BulletTrailComponent(trailColor, 10);
    else
        bullet.AddComponent<BulletTrailComponent>(trailColor, 10);
}
```

### Step 1.4 - EnemyAISystem uses BulletPool (EnemyAISystem.h)

Add member and setter (same pattern). Replace `FireProjectile()` (line 248-287):
```cpp
void FireProjectile(Pillar::Entity entity, const Pillar::TransformComponent& transform,
    const glm::vec2& direction)
{
    glm::vec2 spawnPos = transform.Position + direction * 0.6f;

    Pillar::Entity bullet;
    if (m_BulletPool)
    {
        bullet = m_BulletPool->SpawnBullet(spawnPos, direction, 12.0f, entity, 10.0f, 5.0f);
    }
    else
    {
        bullet = m_Scene->CreateEntity("EnemyBullet");
        auto& bt = bullet.GetComponent<Pillar::TransformComponent>();
        bt.SetPosition(spawnPos);
        bt.SetRotation(std::atan2(direction.y, direction.x));
        bullet.AddComponent<Pillar::VelocityComponent>().Velocity = direction * 12.0f;
        auto& bc = bullet.AddComponent<Pillar::BulletComponent>(entity, 10.0f);
        bc.Lifetime = 5.0f;
        bullet.AddComponent<Pillar::SpriteComponent>();
    }

    auto& sprite = bullet.GetComponent<Pillar::SpriteComponent>();
    sprite.Size = glm::vec2(0.25f, 0.12f);
    sprite.Color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
    sprite.Layer = "Projectiles";
    sprite.OrderInLayer = 5;
    sprite.Visible = true;

    auto& velocity = bullet.GetComponent<Pillar::VelocityComponent>();
    velocity.MaxSpeed = 15.0f;

    auto& bulletComp = bullet.GetComponent<Pillar::BulletComponent>();
    bulletComp.Pierce = false;
    bulletComp.MaxHits = 1;
    bulletComp.HitsRemaining = 1;

    AudioManager::Instance().PlaySound("enemy_shoot", spawnPos, 0.6f, 1.2f);
}
```

### Step 1.5 - BossSystem uses BulletPool (BossSystem.h)

Same pattern for `SpawnBossProjectile()` (line 376-413). Dark magenta sprite (0.8, 0.2, 0.4), size 0.5x0.3, speed 10, lifetime 6s, OrderInLayer 6.

### Step 1.6 - BulletLifetimeSystem returns to pool (BulletLifetimeSystem.h)

Add member and setter:
```cpp
Pillar::BulletPool* m_BulletPool = nullptr;
void SetBulletPool(Pillar::BulletPool* pool) { m_BulletPool = pool; }
```

Replace destroy loop (line 57-60):
```cpp
for (auto entity : toDestroy)
{
    Pillar::Entity e(entity, m_Scene);
    if (m_BulletPool)
    {
        if (auto* sprite = e.TryGetComponent<Pillar::SpriteComponent>())
            sprite->Visible = false;
        m_BulletPool->ReturnBullet(e);
    }
    else
    {
        m_Scene->DestroyEntity(e);
    }
}
```

---

## Workstream 2: ParticleEmitterComponent on Entities

Add `#include <Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h>` to EntityFactory.h and PlayerMovementSystem.h.

### Step 2.1 - Explosive Barrels (EntityFactory.h, after line 511)

```cpp
auto& emitter = barrel.AddComponent<Pillar::ParticleEmitterComponent>();
emitter.EmissionRate = 3.0f;
emitter.Shape = Pillar::ParticleEmitterComponent::EmissionShape::Circle;
emitter.ShapeSize = glm::vec2(0.3f);
emitter.Direction = glm::vec2(0.0f, 1.0f);
emitter.DirectionSpread = 40.0f;
emitter.Speed = 1.0f;
emitter.SpeedVariance = 0.5f;
emitter.Lifetime = 0.8f;
emitter.LifetimeVariance = 0.3f;
emitter.Size = 0.08f;
emitter.SizeVariance = 0.03f;
emitter.StartColor = glm::vec4(1.0f, 0.5f, 0.1f, 0.6f);
emitter.FadeOut = true;
emitter.Gravity = glm::vec2(0.0f, 0.5f);
```

### Step 2.2 - Poison Pools (EntityFactory.h, after line 585)

```cpp
auto& emitter = pool.AddComponent<Pillar::ParticleEmitterComponent>();
emitter.EmissionRate = 5.0f;
emitter.Shape = Pillar::ParticleEmitterComponent::EmissionShape::Circle;
emitter.ShapeSize = glm::vec2(radius * 0.8f);
emitter.Direction = glm::vec2(0.0f, 1.0f);
emitter.DirectionSpread = 60.0f;
emitter.Speed = 0.5f;
emitter.SpeedVariance = 0.3f;
emitter.Lifetime = 1.2f;
emitter.Size = 0.1f;
emitter.SizeVariance = 0.04f;
emitter.StartColor = glm::vec4(0.2f, 0.9f, 0.2f, 0.5f);
emitter.FadeOut = true;
emitter.Gravity = glm::vec2(0.0f, 0.3f);
```

### Step 2.3 - Damage Zones (EntityFactory.h, after line 612)

```cpp
auto& emitter = zone.AddComponent<Pillar::ParticleEmitterComponent>();
emitter.EmissionRate = 8.0f;
emitter.Shape = Pillar::ParticleEmitterComponent::EmissionShape::Circle;
emitter.ShapeSize = glm::vec2(radius * 0.6f);
emitter.Direction = glm::vec2(0.0f, 1.0f);
emitter.DirectionSpread = 45.0f;
emitter.Speed = 1.5f;
emitter.SpeedVariance = 0.8f;
emitter.Lifetime = 0.6f;
emitter.Size = 0.12f;
emitter.SizeVariance = 0.05f;
emitter.StartColor = glm::vec4(1.0f, 0.4f, 0.1f, 0.7f);
emitter.FadeOut = true;
emitter.Gravity = glm::vec2(0.0f, 1.0f);
```

### Step 2.4 - Player Dash Trail (PlayerMovementSystem.h)

Find where dash starts (IsDashing transitions to true). Add:
```cpp
if (auto* emitter = entity.TryGetComponent<Pillar::ParticleEmitterComponent>())
{
    emitter->Enabled = true;
}
else
{
    auto& emitter = entity.AddComponent<Pillar::ParticleEmitterComponent>();
    emitter.EmissionRate = 25.0f;
    emitter.Direction = glm::vec2(0.0f, 0.0f);
    emitter.DirectionSpread = 180.0f;
    emitter.Speed = 1.0f;
    emitter.SpeedVariance = 0.5f;
    emitter.Lifetime = 0.2f;
    emitter.LifetimeVariance = 0.05f;
    emitter.Size = 0.15f;
    emitter.SizeVariance = 0.05f;
    emitter.StartColor = glm::vec4(0.5f, 0.7f, 1.0f, 0.6f);
    emitter.FadeOut = true;
    emitter.Gravity = glm::vec2(0.0f, 0.0f);
}
```

Where dash ends:
```cpp
if (auto* emitter = entity.TryGetComponent<Pillar::ParticleEmitterComponent>())
    emitter->Enabled = false;
```

---

## Workstream 3: 2D Lighting

### Step 3.1 - Init Lighting2D (SwarmSlayerLayer.h)

Add includes:
```cpp
#include <Pillar/Renderer/Lighting2D.h>
#include <Pillar/ECS/Systems/Lighting2DSystem.h>
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
#include <Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h>
```

Add members (~line 1195):
```cpp
Pillar::Lighting2DSystem* m_Lighting2DSystem = nullptr;
Pillar::Lighting2DSettings m_LightingSettings;
```

In `OnAttach()` (after camera init, ~line 104):
```cpp
Pillar::Lighting2D::Init();
m_LightingSettings.AmbientColor = glm::vec3(0.15f, 0.12f, 0.18f);
m_LightingSettings.AmbientIntensity = 0.2f;
m_LightingSettings.EnableShadows = true;
```

In `InitializeGameWorld()` (after AnimationSystem, ~line 462):
```cpp
m_Lighting2DSystem = new Pillar::Lighting2DSystem();
m_Lighting2DSystem->OnAttach(m_Scene.get());
```

In `OnDetach()` (after ShutdownGameSystems, ~line 117):
```cpp
Pillar::Lighting2D::Shutdown();
```

In `ShutdownGameSystems()` (add before HazardSystem cleanup, ~line 672):
```cpp
if (m_Lighting2DSystem) { m_Lighting2DSystem->OnDetach(); delete m_Lighting2DSystem; m_Lighting2DSystem = nullptr; }
```

### Step 3.2 - Refactor RenderGame for lit pass

Replace the entire `RenderGame()` method (lines 924-1058). The new structure:

```cpp
void RenderGame()
{
    Pillar::Renderer2D::SetClearColor(glm::vec4(0.04f, 0.04f, 0.05f, 1.0f));
    Pillar::Renderer2D::Clear();

    if (!m_Scene || !m_CameraController) return;

    auto& window = Pillar::Application::Get().GetWindow();
    uint32_t vpWidth = static_cast<uint32_t>(window.GetWidth());
    uint32_t vpHeight = static_cast<uint32_t>(window.GetHeight());

    // Collect and sort all renderable sprites (SAME AS BEFORE)
    auto& registry = m_Scene->GetRegistry();
    auto view = registry.view<Pillar::TransformComponent, Pillar::SpriteComponent>();

    std::vector<entt::entity> renderables;
    renderables.reserve(static_cast<size_t>(view.size_hint()));

    for (auto entity : view)
    {
        const auto& sprite = view.get<Pillar::SpriteComponent>(entity);
        if (!sprite.Visible) continue;
        renderables.push_back(entity);
    }

    std::sort(renderables.begin(), renderables.end(), [&](entt::entity a, entt::entity b) {
        const auto& spriteA = view.get<Pillar::SpriteComponent>(a);
        const auto& spriteB = view.get<Pillar::SpriteComponent>(b);
        if (spriteA.Layer != spriteB.Layer) return spriteA.Layer < spriteB.Layer;
        return spriteA.OrderInLayer < spriteB.OrderInLayer;
    });

    // === LIT PASS ===
    Pillar::Lighting2D::BeginScene(
        m_CameraController->GetCamera(),
        vpWidth, vpHeight,
        m_LightingSettings
    );
    // Note: Lighting2D::BeginScene already called Renderer2D::BeginScene

    // Bullet trails first
    if (m_BulletTrailSystem)
        m_BulletTrailSystem->RenderTrails();

    // Flush after trails before sprites
    Pillar::Renderer2D::EndScene();
    Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

    // Render sorted sprites with mid-scene flushes on layer changes
    std::string currentLayer = "";
    for (size_t i = 0; i < renderables.size(); ++i)
    {
        auto entity = renderables[i];
        auto& transform = view.get<Pillar::TransformComponent>(entity);
        auto& sprite = view.get<Pillar::SpriteComponent>(entity);

        if (sprite.Layer != currentLayer && !currentLayer.empty())
        {
            // Mid-scene flush: preserves draw order while staying in lit FBO
            Pillar::Renderer2D::EndScene();
            Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());
        }
        currentLayer = sprite.Layer;

        Pillar::Entity ent(entity, m_Scene.get());
        if (auto* hazard = ent.TryGetComponent<HazardComponent>())
        {
            Pillar::SpriteComponent pulsedSprite = sprite;
            float pulse = hazard->GetPulseFactor();
            pulsedSprite.Color = glm::mix(sprite.Color,
                glm::vec4(1.0f, 1.0f, 1.0f, sprite.Color.a), pulse * 0.3f);
            Pillar::Renderer2D::DrawSprite(transform, pulsedSprite);
        }
        else
        {
            Pillar::Renderer2D::DrawSprite(transform, sprite);
        }
    }

    // Flush final sprite batch
    Pillar::Renderer2D::EndScene();

    // Submit lights and shadow casters
    if (m_Lighting2DSystem)
        m_Lighting2DSystem->OnUpdate(0.0f);

    // Begin a new scene for Lighting2D to composite correctly
    Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

    // End lit pass (composites scene * lighting to screen)
    Pillar::Lighting2D::EndScene();

    // === UNLIT PASS (overlays) ===
    Pillar::Renderer2D::BeginScene(m_CameraController->GetCamera());

    if (m_BossSystem)
        m_BossSystem->RenderBossHealthBars();

    // Screen flash overlay
    if (m_ScreenFlashTimer > 0.0f)
    {
        float flashAlpha = m_ScreenFlashTimer / 0.3f * 0.3f;
        auto& camera = m_CameraController->GetCamera();
        glm::vec3 camPos = camera.GetPosition();
        float zoom = m_CameraController->GetZoomLevel();

        Pillar::TransformComponent flashTransform;
        flashTransform.SetPosition(glm::vec2(camPos.x, camPos.y));
        Pillar::SpriteComponent flashSprite;
        flashSprite.Size = glm::vec2(zoom * 4.0f, zoom * 4.0f);
        flashSprite.Color = glm::vec4(1.0f, 0.0f, 0.0f, flashAlpha);
        flashSprite.Layer = "Overlay";
        flashSprite.OrderInLayer = 100;
        Pillar::Renderer2D::DrawSprite(flashTransform, flashSprite);
    }

    // Low health vignette
    if (m_PlayerEntity.IsValid())
    {
        auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
        float healthPercent = health.CurrentHealth / health.MaxHealth;
        if (healthPercent < 0.3f && healthPercent > 0.0f)
        {
            float pulse = 0.1f + 0.1f * std::sin(GameState::Instance().GetStats().PlayTime * 6.0f);
            float intensity = (1.0f - healthPercent / 0.3f) * pulse;

            auto& camera = m_CameraController->GetCamera();
            glm::vec3 camPos = camera.GetPosition();
            float zoom = m_CameraController->GetZoomLevel();

            Pillar::TransformComponent vignetteTransform;
            vignetteTransform.SetPosition(glm::vec2(camPos.x, camPos.y));
            Pillar::SpriteComponent vignetteSprite;
            vignetteSprite.Size = glm::vec2(zoom * 4.0f, zoom * 4.0f);
            vignetteSprite.Color = glm::vec4(0.8f, 0.0f, 0.0f, intensity);
            vignetteSprite.Layer = "Overlay";
            vignetteSprite.OrderInLayer = 99;
            Pillar::Renderer2D::DrawSprite(vignetteTransform, vignetteSprite);
        }
    }

    Pillar::Renderer2D::EndScene();
}
```

**IMPORTANT NOTE**: The mid-scene flush approach (`Renderer2D::EndScene()` + `BeginScene()`) between layers should work because `Lighting2D` binds its scene-color FBO at `BeginScene`. The Renderer2D End/Begin only flushes the sprite batch to that FBO without switching the render target. However, if `Renderer2D::BeginScene()` rebinds the default framebuffer, this won't work. In that case, you'll need to check the `Lighting2D::BeginScene` source to see if it overrides the Renderer2D's FBO binding, and ensure `Renderer2D::BeginScene` doesn't unbind it. If it does, you may need to skip the mid-scene flushes and rely on submission order alone (which should work if the renderer preserves submission order within a batch).

### Step 3.3 - Entity Lights (EntityFactory.h)

Add `#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>` and `#include <Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h>`.

**Player (CreatePlayer, before `return player;` ~line 82):**
```cpp
auto& light = player.AddComponent<Pillar::Light2DComponent>();
light.Type = Pillar::Light2DType::Point;
light.Color = glm::vec3(0.6f, 0.7f, 1.0f);
light.Intensity = 1.2f;
light.Radius = 6.0f;
light.CastShadows = true;
```

**Explosive Barrel (CreateExplosiveBarrel, before `return barrel;` ~line 512):**
```cpp
auto& light = barrel.AddComponent<Pillar::Light2DComponent>();
light.Color = glm::vec3(1.0f, 0.5f, 0.15f);
light.Intensity = 0.6f;
light.Radius = 2.5f;
light.CastShadows = false;
```

**Poison Pool (CreatePoisonPool, before `return pool;` ~line 587):**
```cpp
auto& light = pool.AddComponent<Pillar::Light2DComponent>();
light.Color = glm::vec3(0.2f, 0.9f, 0.2f);
light.Intensity = 0.5f;
light.Radius = radius * 1.5f;
light.CastShadows = false;
```

**Damage Zone (CreateDamageZone, before `return zone;` ~line 614):**
```cpp
auto& light = zone.AddComponent<Pillar::Light2DComponent>();
light.Color = glm::vec3(1.0f, 0.4f, 0.1f);
light.Intensity = 0.7f;
light.Radius = radius * 1.3f;
light.CastShadows = false;
```

**XP Orb (CreateXPOrb, before return ~line 370):**
```cpp
auto& light = orb.AddComponent<Pillar::Light2DComponent>();
light.Color = glm::vec3(0.3f, 0.8f, 1.0f);
light.Intensity = 0.3f;
light.Radius = 1.0f;
light.CastShadows = false;
```

**Power-Up (CreatePowerUp, before return ~line 343):**
```cpp
auto& light = powerUp.AddComponent<Pillar::Light2DComponent>();
glm::vec4 c = sprite.Color; // use whatever color was set for the sprite
light.Color = glm::vec3(c.r, c.g, c.b);
light.Intensity = 0.5f;
light.Radius = 1.5f;
light.CastShadows = false;
```

### Step 3.4 - Shadow Casters on Walls (EntityFactory.h, CreateWall ~line 114)

Before `return wall;`:
```cpp
auto& caster = wall.AddComponent<Pillar::ShadowCaster2DComponent>();
glm::vec2 halfSize = size * 0.5f;
caster.Points = {
    glm::vec2(-halfSize.x, -halfSize.y),
    glm::vec2( halfSize.x, -halfSize.y),
    glm::vec2( halfSize.x,  halfSize.y),
    glm::vec2(-halfSize.x,  halfSize.y)
};
caster.Closed = true;
caster.TwoSided = false;
```

### Step 3.5 - Temporary Lights (EffectFactory.h)

Add includes:
```cpp
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
```

Add two new static methods:

```cpp
static void SpawnMuzzleFlashLight(Pillar::Scene& scene, const glm::vec2& position)
{
    auto flash = scene.CreateEntity("MuzzleLight");
    auto& transform = flash.GetComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);

    auto& light = flash.AddComponent<Pillar::Light2DComponent>();
    light.Color = glm::vec3(1.0f, 0.85f, 0.3f);
    light.Intensity = 3.0f;
    light.Radius = 3.0f;
    light.CastShadows = false;

    auto& temp = flash.AddComponent<TemporaryComponent>();
    temp.Lifetime = 0.08f;
}

static void SpawnExplosionLight(Pillar::Scene& scene, const glm::vec2& position, float radius)
{
    auto light = scene.CreateEntity("ExplosionLight");
    auto& transform = light.GetComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);

    auto& lightComp = light.AddComponent<Pillar::Light2DComponent>();
    lightComp.Color = glm::vec3(1.0f, 0.6f, 0.1f);
    lightComp.Intensity = 5.0f;
    lightComp.Radius = radius * 2.0f;
    lightComp.CastShadows = false;

    auto& temp = light.AddComponent<TemporaryComponent>();
    temp.Lifetime = 0.3f;
}
```

**Call sites:**
- In `WeaponSystem.h` after `EffectFactory::SpawnMuzzleFlash(...)` (line 159):
  ```cpp
  EffectFactory::SpawnMuzzleFlashLight(*m_Scene, spawnPos);
  ```
- In `SwarmSlayerLayer.h` barrel explosion callback (~line 451-457):
  ```cpp
  EffectFactory::SpawnExplosionLight(*m_Scene, pos, radius);
  ```

### Step 3.6 - Barrel Flicker (HazardSystem.h)

In the hazard update loop, for explosive barrels that haven't exploded, add:
```cpp
if (hazard.Type == HazardType::ExplosiveBarrel && !hazard.HasExploded)
{
    Pillar::Entity ent(entity, m_Scene);
    if (auto* light = ent.TryGetComponent<Pillar::Light2DComponent>())
    {
        light->Intensity = 0.6f + 0.2f * std::sin(hazard.PulseTimer * 5.0f);
    }
}
```

### Step 3.7 - Boss Projectile Lights (BossSystem.h)

In `SpawnBossProjectile()` after creating the projectile, add:
```cpp
auto& light = projectile.AddComponent<Pillar::Light2DComponent>();
light.Color = glm::vec3(0.8f, 0.2f, 0.4f);
light.Intensity = 0.6f;
light.Radius = 2.0f;
light.CastShadows = false;
```

---

## Verification

1. **Build**: Compile the project, fix any errors
2. **BulletPool**: Fire all 5 weapons - bullets should look and behave identically. Enemy and boss projectiles should work. Bullets should be recycled (pool active count should stabilize).
3. **Emitters**: Explosive barrels should have subtle orange smoke. Poison pools should have green bubbles. Damage zones should have fire particles. Dashing should leave a blue-white particle trail.
4. **Lighting**: Arena should be dark with ambient lighting. Player should cast a blue-white pool of light. Walls should cast shadows. Hazards should glow their respective colors. Muzzle flashes should briefly illuminate. Barrel lights should flicker. Overlays (screen flash, vignette, boss health bars) should render normally on top without being affected by lighting.
5. **Performance**: Should be equal or better than before. Bullet pooling reduces allocation pressure. Lighting adds GPU cost but only player light casts shadows.

## Risks

1. **Renderer2D FBO binding**: If `Renderer2D::BeginScene()` always binds the default FBO, the mid-scene flushes inside the lit pass will break. Check the source. If so, skip mid-scene flushes and rely on sprite submission order.
2. **BulletTrailComponent on pooled bullets**: When a bullet returns to the pool, its trail component persists. On re-spawn, overwrite it (already handled in Step 1.3).
3. **Many XP orb lights**: If 50+ XP orbs are on screen, that's 50+ light submissions. Since they have `CastShadows = false`, the cost is just the additive blend pass per light. If performance is an issue, remove lights from XP orbs.
