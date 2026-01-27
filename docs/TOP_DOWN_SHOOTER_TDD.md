# Top-Down Shooter Framework - Technical Design Document

**Version 1.0 | Pillar Engine**

---

## Executive Summary

This Technical Design Document (TDD) specifies the architecture and implementation of a "Top-Down Shooter Framework Showcase" built on the Pillar Engine. The showcase demonstrates Pillar's core capabilities: ECS-driven entity management (EnTT), Box2D physics integration, OpenAL-Soft spatial audio, batch-optimized 2D rendering, and particle effects. The framework implements classic survivor-shooter mechanics—WASD movement, mouse-aimed shooting, wave-based enemy spawning, power-up collection, and satisfying "game feel" polish. A competent C++ developer can implement this framework in 2-3 days using the copy-paste-ready code examples and step-by-step guidance provided herein.

---

## 1. System Architecture

### 1.1 Overview

The Top-Down Shooter Framework follows Pillar Engine's layer-based architecture. A single `GameLayer` class inherits from `Pillar::Layer` and orchestrates all gameplay systems. The layer manages a `Scene` containing all game entities, coordinates system updates in the correct order, and handles rendering via `Renderer2D`.

```
┌─────────────────────────────────────────────────────────────┐
│                    Pillar::Application                       │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                      LayerStack                         │ │
│  │  ┌───────────────────────────────────────────────────┐  │ │
│  │  │                    GameLayer                      │  │ │
│  │  │  ┌─────────────────────────────────────────────┐  │  │ │
│  │  │  │                   Scene                     │  │  │ │
│  │  │  │  ┌─────────────────────────────────────┐    │  │  │ │
│  │  │  │  │   Entities + Components             │    │  │  │ │
│  │  │  │  │   (Player, Enemies, Bullets, etc.)  │    │  │  │ │
│  │  │  │  └─────────────────────────────────────┘    │  │  │ │
│  │  │  │  ┌─────────────────────────────────────┐    │  │  │ │
│  │  │  │  │   Systems (Physics, Animation,      │    │  │  │ │
│  │  │  │  │   Particles, Collision, etc.)       │    │  │  │ │
│  │  │  │  └─────────────────────────────────────┘    │  │  │ │
│  │  │  └─────────────────────────────────────────────┘  │  │ │
│  │  └───────────────────────────────────────────────────┘  │ │
│  │  ┌───────────────────────────────────────────────────┐  │ │
│  │  │               ImGuiLayer (Debug UI)               │  │ │
│  │  └───────────────────────────────────────────────────┘  │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 Pillar Engine Integration Points

| Pillar System | Usage in Shooter Framework |
|---------------|---------------------------|
| `Scene` + `Entity` | Entity lifecycle management—creating players, enemies, bullets, power-ups |
| `TransformComponent` | 2D position, rotation, scale for all game objects |
| `SpriteComponent` | Visual representation with textures, color tinting, sprite sheets |
| `RigidbodyComponent` | Box2D physics bodies for player and enemies (heavy entities) |
| `ColliderComponent` | Collision shapes, sensors, filtering categories |
| `VelocityComponent` | Lightweight movement for bullets and particles (no Box2D overhead) |
| `HealthComponent` | Damage, healing, invulnerability, death detection |
| `BulletComponent` | Projectile damage, lifetime, pierce mechanics |
| `AnimationComponent` | Sprite-sheet animations (walk, shoot, death) |
| `ParticleEmitterComponent` | Muzzle flash, blood splatter, explosion effects |
| `PhysicsSystem` | Box2D world stepping, body sync |
| `BulletCollisionSystem` | Raycast-based bullet hit detection |
| `AnimationSystem` | Frame-based sprite animation updates |
| `ParticleEmitterSystem` + `ParticleSystem` | Particle spawning and lifecycle |
| `SpriteRenderSystem` | Optimized batch rendering |
| `AudioEngine` | Sound effects, background music, 3D positional audio |
| `Input` | Keyboard/mouse polling, action bindings |
| `OrthographicCameraController` | Camera following, zoom, screen shake |
| `ObjectPool` | Entity recycling for bullets and particles |
| `AssetManager` | Texture and audio path resolution |

### 1.3 Game Loop Flow

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::OnUpdate(float dt)
{
    // 1. Input Processing
    ProcessPlayerInput(dt);
    
    // 2. Game Logic Updates
    m_WaveManager.OnUpdate(dt);
    m_PlayerController.OnUpdate(dt);
    
    // 3. Physics Step (Box2D)
    m_PhysicsSystem.OnUpdate(dt);
    
    // 4. Collision Resolution
    m_BulletCollisionSystem.OnUpdate(dt);
    m_DamageSystem.OnUpdate(dt);
    
    // 5. Animation Updates
    m_AnimationSystem.OnUpdate(dt);
    
    // 6. Particle Updates
    m_ParticleEmitterSystem.OnUpdate(dt);
    m_ParticleSystem.OnUpdate(dt);
    
    // 7. Cleanup Dead Entities
    CleanupDeadEntities();
    
    // 8. Update Camera (follow player, apply shake)
    m_CameraController.OnUpdate(dt);
    
    // 9. Audio Listener Update
    AudioEngine::SetListenerPosition(glm::vec3(m_PlayerPosition, 0.0f));
    AudioEngine::Update(dt);
    
    // 10. Rendering
    Renderer2D::SetClearColor({0.05f, 0.05f, 0.1f, 1.0f});
    Renderer2D::Clear();
    Renderer2D::BeginScene(m_CameraController.GetCamera());
    
    m_SpriteRenderSystem.OnUpdate(dt);  // Renders all sprites
    
    Renderer2D::EndScene();
}
```

---

## 2. Entity Component Design

### 2.1 Player System

**Entity Composition:**

| Component | Purpose |
|-----------|---------|
| `TransformComponent` | Position, rotation (faces mouse cursor) |
| `SpriteComponent` | Player sprite with directional animations |
| `RigidbodyComponent` | Dynamic body for physics-based movement |
| `ColliderComponent` | Circle collider for collision detection |
| `HealthComponent` | Player HP with invulnerability frames |
| `AnimationComponent` | Walk/idle/death animations |
| `WeaponComponent` | *(Custom)* Weapon stats, fire rate, ammo |
| `PlayerTagComponent` | *(Custom)* Marks entity as player |

**Component Definitions (Custom):**

```cpp
// In Sandbox/src/Components/WeaponComponent.h
#pragma once

#include <memory>
#include <string>
#include "Pillar/Audio/AudioBuffer.h"

namespace Game {

    struct WeaponComponent
    {
        std::string WeaponName = "Pistol";
        float Damage = 10.0f;
        float FireRate = 5.0f;          // Shots per second
        float BulletSpeed = 15.0f;      // Units per second
        float Spread = 2.0f;            // Degrees of random spread
        int BulletsPerShot = 1;         // For shotguns
        
        float FireCooldown = 0.0f;      // Countdown to next shot
        
        std::shared_ptr<Pillar::AudioBuffer> FireSound;
        
        bool CanFire() const { return FireCooldown <= 0.0f; }
        
        void ResetCooldown() { FireCooldown = 1.0f / FireRate; }
        
        void UpdateCooldown(float dt) 
        { 
            if (FireCooldown > 0.0f) 
                FireCooldown -= dt; 
        }
    };

} // namespace Game
```

```cpp
// In Sandbox/src/Components/PlayerTagComponent.h
#pragma once

namespace Game {

    struct PlayerTagComponent
    {
        float MoveSpeed = 5.0f;
        float DashSpeed = 15.0f;
        float DashDuration = 0.15f;
        float DashCooldown = 1.0f;
        
        bool IsDashing = false;
        float DashTimer = 0.0f;
        float DashCooldownTimer = 0.0f;
    };

} // namespace Game
```

**Entity Creation:**

```cpp
// In Sandbox/src/EntityFactory.cpp
#include "EntityFactory.h"
#include "Components/WeaponComponent.h"
#include "Components/PlayerTagComponent.h"

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Rendering/AnimationComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Physics/ColliderComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/Renderer/Texture.h>
#include <Pillar/Utils/AssetManager.h>
#include <Pillar/Audio/AudioEngine.h>

namespace Game {

    Pillar::Entity EntityFactory::CreatePlayer(Pillar::Scene& scene, const glm::vec2& position)
    {
        auto player = scene.CreateEntity("Player");
        
        // Transform
        auto& transform = player.AddComponent<Pillar::TransformComponent>();
        transform.SetPosition(position);
        transform.SetScale(1.0f, 1.0f);
        
        // Sprite
        auto& sprite = player.AddComponent<Pillar::SpriteComponent>();
        sprite.Texture = Pillar::Texture2D::Create(
            Pillar::AssetManager::GetTexturePath("player.png")
        );
        sprite.Size = { 1.0f, 1.0f };
        sprite.Layer = "Player";
        sprite.OrderInLayer = 10;
        
        // Animation
        auto& anim = player.AddComponent<Pillar::AnimationComponent>();
        anim.CurrentClipName = "player_idle";
        
        // Physics - Dynamic body with fixed rotation
        auto& rb = player.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
        rb.FixedRotation = true;
        rb.LinearDamping = 8.0f;  // Snappy stop when releasing movement keys
        
        // Collider - Circle for smooth movement around obstacles
        auto collider = Pillar::ColliderComponent::Circle(0.4f);
        collider.Density = 1.0f;
        collider.Friction = 0.0f;
        collider.CategoryBits = CollisionCategory::Player;
        collider.MaskBits = CollisionCategory::Enemy | 
                           CollisionCategory::Wall | 
                           CollisionCategory::PowerUp;
        player.AddComponent<Pillar::ColliderComponent>(collider);
        
        // Health
        auto& health = player.AddComponent<Pillar::HealthComponent>(100.0f);
        health.DestroyOnDeath = false;  // Handle death manually for game over screen
        
        // Weapon
        auto& weapon = player.AddComponent<WeaponComponent>();
        weapon.WeaponName = "Pistol";
        weapon.Damage = 10.0f;
        weapon.FireRate = 5.0f;
        weapon.BulletSpeed = 20.0f;
        weapon.FireSound = Pillar::AudioEngine::CreateBuffer(
            Pillar::AssetManager::GetSFXPath("gunshot.wav")
        );
        
        // Player tag
        player.AddComponent<PlayerTagComponent>();
        
        return player;
    }

} // namespace Game
```

**Player Movement System:**

```cpp
// In Sandbox/src/Systems/PlayerMovementSystem.cpp
#include "PlayerMovementSystem.h"
#include "Components/PlayerTagComponent.h"

#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>

namespace Game {

    void PlayerMovementSystem::OnUpdate(float dt)
    {
        if (!m_Scene) return;
        
        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<
            Pillar::TransformComponent, 
            Pillar::RigidbodyComponent, 
            PlayerTagComponent
        >();
        
        for (auto entity : view)
        {
            auto& transform = view.get<Pillar::TransformComponent>(entity);
            auto& rb = view.get<Pillar::RigidbodyComponent>(entity);
            auto& player = view.get<PlayerTagComponent>(entity);
            
            if (!rb.Body) continue;
            
            // Update dash cooldown
            if (player.DashCooldownTimer > 0.0f)
                player.DashCooldownTimer -= dt;
            
            // Handle dash state
            if (player.IsDashing)
            {
                player.DashTimer -= dt;
                if (player.DashTimer <= 0.0f)
                    player.IsDashing = false;
                continue;  // Skip normal movement during dash
            }
            
            // Calculate movement direction from input
            glm::vec2 moveDir(0.0f);
            
            if (Pillar::Input::IsKeyDown(PIL_KEY_W)) moveDir.y += 1.0f;
            if (Pillar::Input::IsKeyDown(PIL_KEY_S)) moveDir.y -= 1.0f;
            if (Pillar::Input::IsKeyDown(PIL_KEY_A)) moveDir.x -= 1.0f;
            if (Pillar::Input::IsKeyDown(PIL_KEY_D)) moveDir.x += 1.0f;
            
            // Normalize to prevent diagonal speed boost
            if (glm::length(moveDir) > 0.0f)
                moveDir = glm::normalize(moveDir);
            
            // Apply velocity via Box2D
            float speed = player.MoveSpeed;
            b2Vec2 velocity(moveDir.x * speed, moveDir.y * speed);
            rb.Body->SetLinearVelocity(velocity);
            
            // Dash on Space
            if (Pillar::Input::IsKeyJustPressed(PIL_KEY_SPACE) && 
                player.DashCooldownTimer <= 0.0f &&
                glm::length(moveDir) > 0.0f)
            {
                player.IsDashing = true;
                player.DashTimer = player.DashDuration;
                player.DashCooldownTimer = player.DashCooldown;
                
                b2Vec2 dashVelocity(moveDir.x * player.DashSpeed, 
                                    moveDir.y * player.DashSpeed);
                rb.Body->SetLinearVelocity(dashVelocity);
            }
            
            // Rotate to face mouse cursor
            auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
            glm::vec2 mouseWorld = ScreenToWorld(mouseX, mouseY);
            glm::vec2 direction = mouseWorld - transform.Position;
            
            if (glm::length(direction) > 0.001f)
            {
                float angle = std::atan2(direction.y, direction.x);
                transform.SetRotation(angle);
            }
        }
    }

} // namespace Game
```

### 2.2 Enemy System

**Entity Composition:**

| Component | Purpose |
|-----------|---------|
| `TransformComponent` | Position, rotation (faces player) |
| `SpriteComponent` | Enemy visual with color variation |
| `RigidbodyComponent` | Dynamic body for physics |
| `ColliderComponent` | Circle collider with enemy category |
| `VelocityComponent` | For lightweight enemies without Box2D |
| `HealthComponent` | Enemy HP |
| `AnimationComponent` | Walk/attack/death animations |
| `EnemyComponent` | *(Custom)* AI type, damage, behavior state |

**Component Definition:**

```cpp
// In Sandbox/src/Components/EnemyComponent.h
#pragma once

#include <Pillar/ECS/Entity.h>
#include <glm/glm.hpp>

namespace Game {

    enum class EnemyType
    {
        Chaser,     // Moves directly toward player
        Wanderer,   // Random movement, occasional charge
        Shooter,    // Maintains distance, fires projectiles
        Swarm       // Lightweight, spawns in groups
    };

    enum class EnemyState
    {
        Idle,
        Chasing,
        Attacking,
        Stunned,
        Dying
    };

    struct EnemyComponent
    {
        EnemyType Type = EnemyType::Chaser;
        EnemyState State = EnemyState::Idle;
        
        float MoveSpeed = 3.0f;
        float AttackDamage = 10.0f;
        float AttackRange = 1.0f;       // Distance to trigger attack
        float AttackCooldown = 1.0f;    // Time between attacks
        float AttackTimer = 0.0f;
        
        float DetectionRange = 15.0f;   // Range to detect player
        float XPValue = 10.0f;          // XP dropped on death
        
        // AI state
        Pillar::Entity TargetEntity;
        glm::vec2 WanderDirection = { 1.0f, 0.0f };
        float WanderTimer = 0.0f;
    };

} // namespace Game
```

**Enemy Factory:**

```cpp
// In Sandbox/src/EntityFactory.cpp (continued)

Pillar::Entity EntityFactory::CreateEnemy(
    Pillar::Scene& scene, 
    const glm::vec2& position, 
    EnemyType type)
{
    auto enemy = scene.CreateEntity("Enemy");
    
    // Transform
    auto& transform = enemy.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    
    // Sprite with color variation based on type
    auto& sprite = enemy.AddComponent<Pillar::SpriteComponent>();
    sprite.Texture = Pillar::Texture2D::Create(
        Pillar::AssetManager::GetTexturePath("enemy_base.png")
    );
    sprite.Layer = "Enemies";
    
    // Color-code enemy types
    switch (type)
    {
        case EnemyType::Chaser:
            sprite.Color = { 1.0f, 0.3f, 0.3f, 1.0f };  // Red
            sprite.Size = { 0.8f, 0.8f };
            break;
        case EnemyType::Shooter:
            sprite.Color = { 0.3f, 0.3f, 1.0f, 1.0f };  // Blue
            sprite.Size = { 1.0f, 1.0f };
            break;
        case EnemyType::Swarm:
            sprite.Color = { 0.3f, 1.0f, 0.3f, 1.0f };  // Green
            sprite.Size = { 0.5f, 0.5f };
            break;
        default:
            sprite.Color = { 1.0f, 1.0f, 1.0f, 1.0f };
            sprite.Size = { 1.0f, 1.0f };
    }
    
    // Physics setup depends on enemy type
    if (type == EnemyType::Swarm)
    {
        // Lightweight enemies use VelocityComponent (no Box2D overhead)
        auto& velocity = enemy.AddComponent<Pillar::VelocityComponent>();
        velocity.Drag = 2.0f;
        velocity.MaxSpeed = 8.0f;
    }
    else
    {
        // Heavy enemies use Box2D
        auto& rb = enemy.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
        rb.FixedRotation = true;
        rb.LinearDamping = 4.0f;
        
        auto collider = Pillar::ColliderComponent::Circle(sprite.Size.x * 0.4f);
        collider.CategoryBits = CollisionCategory::Enemy;
        collider.MaskBits = CollisionCategory::Player | 
                           CollisionCategory::Wall | 
                           CollisionCategory::Bullet;
        enemy.AddComponent<Pillar::ColliderComponent>(collider);
    }
    
    // Health scaled by type
    float baseHealth = 30.0f;
    switch (type)
    {
        case EnemyType::Shooter: baseHealth = 50.0f; break;
        case EnemyType::Swarm: baseHealth = 10.0f; break;
        default: break;
    }
    auto& health = enemy.AddComponent<Pillar::HealthComponent>(baseHealth);
    health.DestroyOnDeath = false;  // Handle death effects first
    
    // Enemy behavior
    auto& enemyComp = enemy.AddComponent<EnemyComponent>();
    enemyComp.Type = type;
    enemyComp.State = EnemyState::Idle;
    
    // Configure stats by type
    switch (type)
    {
        case EnemyType::Chaser:
            enemyComp.MoveSpeed = 4.0f;
            enemyComp.AttackDamage = 15.0f;
            enemyComp.XPValue = 10.0f;
            break;
        case EnemyType::Shooter:
            enemyComp.MoveSpeed = 2.0f;
            enemyComp.AttackDamage = 8.0f;
            enemyComp.AttackRange = 10.0f;
            enemyComp.XPValue = 25.0f;
            break;
        case EnemyType::Swarm:
            enemyComp.MoveSpeed = 6.0f;
            enemyComp.AttackDamage = 5.0f;
            enemyComp.XPValue = 5.0f;
            break;
    }
    
    return enemy;
}
```

### 2.3 Projectile System

Projectiles use `VelocityComponent` for movement (lightweight, no Box2D overhead) and `BulletCollisionSystem` performs raycasts against heavy entities for hit detection. This hybrid approach allows thousands of bullets without overloading the physics engine.

**Entity Composition:**

| Component | Purpose |
|-----------|---------|
| `TransformComponent` | Position, rotation (direction of travel) |
| `SpriteComponent` | Bullet visual (small quad or texture) |
| `VelocityComponent` | Linear movement at constant speed |
| `BulletComponent` | Damage, lifetime, pierce count, owner |

**Bullet Factory:**

```cpp
// In Sandbox/src/EntityFactory.cpp (continued)

Pillar::Entity EntityFactory::CreateBullet(
    Pillar::Scene& scene,
    Pillar::Entity owner,
    const glm::vec2& position,
    const glm::vec2& direction,
    float speed,
    float damage)
{
    auto bullet = scene.CreateEntity("Bullet");
    
    // Transform
    auto& transform = bullet.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    
    // Rotate to face direction
    float angle = std::atan2(direction.y, direction.x);
    transform.SetRotation(angle);
    
    // Small bright sprite
    auto& sprite = bullet.AddComponent<Pillar::SpriteComponent>();
    sprite.Texture = Pillar::Texture2D::Create(
        Pillar::AssetManager::GetTexturePath("bullet.png")
    );
    sprite.Size = { 0.3f, 0.15f };
    sprite.Color = { 1.0f, 1.0f, 0.5f, 1.0f };  // Yellow tint
    sprite.Layer = "Projectiles";
    sprite.OrderInLayer = 5;
    
    // Velocity-based movement (no physics body)
    auto& velocity = bullet.AddComponent<Pillar::VelocityComponent>();
    velocity.Velocity = direction * speed;
    velocity.MaxSpeed = speed * 1.5f;  // Allow slight variance
    
    // Bullet data
    auto& bulletComp = bullet.AddComponent<Pillar::BulletComponent>(owner, damage);
    bulletComp.Lifetime = 3.0f;
    bulletComp.Pierce = false;
    bulletComp.MaxHits = 1;
    bulletComp.HitsRemaining = 1;
    
    return bullet;
}
```

**Bullet Pool Setup (recommended for performance):**

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::InitializePools()
{
    m_BulletPool.Init(&m_Scene, 200);
    
    m_BulletPool.SetInitCallback([](Pillar::Entity entity) {
        entity.AddComponent<Pillar::TransformComponent>();
        entity.AddComponent<Pillar::SpriteComponent>();
        entity.AddComponent<Pillar::VelocityComponent>();
        entity.AddComponent<Pillar::BulletComponent>();
    });
    
    m_BulletPool.SetResetCallback([](Pillar::Entity entity) {
        // Reset transform
        auto& transform = entity.GetComponent<Pillar::TransformComponent>();
        transform.Reset();
        
        // Reset velocity
        auto& velocity = entity.GetComponent<Pillar::VelocityComponent>();
        velocity.Velocity = { 0.0f, 0.0f };
        
        // Reset bullet state
        auto& bullet = entity.GetComponent<Pillar::BulletComponent>();
        bullet.TimeAlive = 0.0f;
        bullet.HitsRemaining = bullet.MaxHits;
        
        // Hide sprite
        auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();
        sprite.Visible = false;
    });
}

Pillar::Entity GameLayer::SpawnBullet(
    Pillar::Entity owner,
    const glm::vec2& position,
    const glm::vec2& direction,
    float speed,
    float damage)
{
    Pillar::Entity bullet = m_BulletPool.Acquire();
    
    auto& transform = bullet.GetComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    transform.SetRotation(std::atan2(direction.y, direction.x));
    
    auto& sprite = bullet.GetComponent<Pillar::SpriteComponent>();
    sprite.Visible = true;
    
    auto& velocity = bullet.GetComponent<Pillar::VelocityComponent>();
    velocity.Velocity = direction * speed;
    
    auto& bulletComp = bullet.GetComponent<Pillar::BulletComponent>();
    bulletComp.Owner = owner;
    bulletComp.Damage = damage;
    
    return bullet;
}
```

### 2.4 Power-Up System

**Entity Composition:**

| Component | Purpose |
|-----------|---------|
| `TransformComponent` | Position (may bob up/down) |
| `SpriteComponent` | Power-up icon with glow effect |
| `ColliderComponent` | Sensor trigger (no physics response) |
| `PowerUpComponent` | *(Custom)* Type, value, duration |

**Component Definition:**

```cpp
// In Sandbox/src/Components/PowerUpComponent.h
#pragma once

namespace Game {

    enum class PowerUpType
    {
        Health,         // Restore HP
        SpeedBoost,     // Increase movement speed
        FireRateBoost,  // Increase fire rate
        DamageBoost,    // Increase damage
        Shield,         // Temporary invulnerability
        XPMultiplier    // Double XP gain
    };

    struct PowerUpComponent
    {
        PowerUpType Type = PowerUpType::Health;
        float Value = 25.0f;        // Amount/multiplier
        float Duration = 5.0f;      // For timed buffs (0 = instant)
        float BobTimer = 0.0f;      // For visual bobbing
        float BobAmplitude = 0.1f;
    };

} // namespace Game
```

**Power-Up Factory:**

```cpp
// In Sandbox/src/EntityFactory.cpp (continued)

Pillar::Entity EntityFactory::CreatePowerUp(
    Pillar::Scene& scene,
    const glm::vec2& position,
    PowerUpType type)
{
    auto powerUp = scene.CreateEntity("PowerUp");
    
    auto& transform = powerUp.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    
    auto& sprite = powerUp.AddComponent<Pillar::SpriteComponent>();
    sprite.Size = { 0.6f, 0.6f };
    sprite.Layer = "Items";
    sprite.OrderInLayer = 0;
    
    // Color and texture by type
    switch (type)
    {
        case PowerUpType::Health:
            sprite.Color = { 1.0f, 0.2f, 0.2f, 1.0f };  // Red
            sprite.Texture = Pillar::Texture2D::Create(
                Pillar::AssetManager::GetTexturePath("powerup_health.png")
            );
            break;
        case PowerUpType::SpeedBoost:
            sprite.Color = { 0.2f, 0.8f, 1.0f, 1.0f };  // Cyan
            sprite.Texture = Pillar::Texture2D::Create(
                Pillar::AssetManager::GetTexturePath("powerup_speed.png")
            );
            break;
        case PowerUpType::DamageBoost:
            sprite.Color = { 1.0f, 0.5f, 0.0f, 1.0f };  // Orange
            sprite.Texture = Pillar::Texture2D::Create(
                Pillar::AssetManager::GetTexturePath("powerup_damage.png")
            );
            break;
        // ... other types
    }
    
    // Sensor collider (triggers only, no physics push)
    auto collider = Pillar::ColliderComponent::Circle(0.3f);
    collider.IsSensor = true;
    collider.CategoryBits = CollisionCategory::PowerUp;
    collider.MaskBits = CollisionCategory::Player;
    powerUp.AddComponent<Pillar::ColliderComponent>(collider);
    
    auto& powerUpComp = powerUp.AddComponent<PowerUpComponent>();
    powerUpComp.Type = type;
    
    switch (type)
    {
        case PowerUpType::Health:
            powerUpComp.Value = 25.0f;
            powerUpComp.Duration = 0.0f;  // Instant
            break;
        case PowerUpType::SpeedBoost:
            powerUpComp.Value = 1.5f;     // 50% speed increase
            powerUpComp.Duration = 5.0f;
            break;
        case PowerUpType::DamageBoost:
            powerUpComp.Value = 2.0f;     // Double damage
            powerUpComp.Duration = 8.0f;
            break;
    }
    
    return powerUp;
}
```

### 2.5 Environment System

**Wall Entity:**

```cpp
Pillar::Entity EntityFactory::CreateWall(
    Pillar::Scene& scene,
    const glm::vec2& position,
    const glm::vec2& size)
{
    auto wall = scene.CreateEntity("Wall");
    
    auto& transform = wall.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    
    auto& sprite = wall.AddComponent<Pillar::SpriteComponent>();
    sprite.Texture = Pillar::Texture2D::Create(
        Pillar::AssetManager::GetTexturePath("wall.png")
    );
    sprite.Size = size;
    sprite.Layer = "Environment";
    sprite.OrderInLayer = -10;
    
    // Static body - does not move
    auto& rb = wall.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);
    
    // Box collider matching sprite size
    auto collider = Pillar::ColliderComponent::Box(size * 0.5f);
    collider.CategoryBits = CollisionCategory::Wall;
    collider.MaskBits = CollisionCategory::Player | 
                       CollisionCategory::Enemy | 
                       CollisionCategory::Bullet;
    wall.AddComponent<Pillar::ColliderComponent>(collider);
    
    return wall;
}
```

**Arena Bounds:**

```cpp
void GameLayer::CreateArenaBounds(float width, float height)
{
    float wallThickness = 1.0f;
    float halfWidth = width * 0.5f;
    float halfHeight = height * 0.5f;
    
    // Top
    EntityFactory::CreateWall(m_Scene, 
        { 0.0f, halfHeight + wallThickness * 0.5f }, 
        { width + wallThickness * 2, wallThickness }
    );
    
    // Bottom
    EntityFactory::CreateWall(m_Scene, 
        { 0.0f, -halfHeight - wallThickness * 0.5f }, 
        { width + wallThickness * 2, wallThickness }
    );
    
    // Left
    EntityFactory::CreateWall(m_Scene, 
        { -halfWidth - wallThickness * 0.5f, 0.0f }, 
        { wallThickness, height }
    );
    
    // Right
    EntityFactory::CreateWall(m_Scene, 
        { halfWidth + wallThickness * 0.5f, 0.0f }, 
        { wallThickness, height }
    );
}
```

---

## 3. Core Gameplay Systems

### 3.1 Combat System

**Aiming Mechanism:**

Convert screen-space mouse coordinates to world-space for accurate aiming:

```cpp
// In Sandbox/src/GameLayer.cpp
glm::vec2 GameLayer::ScreenToWorld(float screenX, float screenY) const
{
    // Get window dimensions
    auto& window = Pillar::Application::Get().GetWindow();
    float windowWidth = static_cast<float>(window.GetWidth());
    float windowHeight = static_cast<float>(window.GetHeight());
    
    // Normalize to [-1, 1]
    float ndcX = (2.0f * screenX / windowWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / windowHeight);  // Flip Y
    
    // Get inverse view-projection matrix
    const auto& camera = m_CameraController.GetCamera();
    glm::mat4 invVP = glm::inverse(camera.GetViewProjectionMatrix());
    
    // Transform to world space
    glm::vec4 worldPos = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    return glm::vec2(worldPos.x, worldPos.y);
}
```

**Weapon Firing System:**

```cpp
// In Sandbox/src/Systems/WeaponSystem.cpp
#include "WeaponSystem.h"
#include "Components/WeaponComponent.h"
#include "Components/PlayerTagComponent.h"

#include <Pillar/Input.h>
#include <Pillar/MouseButtonCodes.h>
#include <Pillar/Audio/AudioEngine.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>

namespace Game {

    void WeaponSystem::OnUpdate(float dt)
    {
        if (!m_Scene) return;
        
        auto& registry = m_Scene->GetRegistry();
        auto view = registry.view<
            Pillar::TransformComponent, 
            WeaponComponent, 
            PlayerTagComponent
        >();
        
        for (auto entity : view)
        {
            auto& transform = view.get<Pillar::TransformComponent>(entity);
            auto& weapon = view.get<WeaponComponent>(entity);
            
            // Update cooldown
            weapon.UpdateCooldown(dt);
            
            // Fire on left mouse button
            if (Pillar::Input::IsMouseButtonDown(PIL_MOUSE_BUTTON_LEFT) && 
                weapon.CanFire())
            {
                Fire(Pillar::Entity(entity, m_Scene), transform, weapon);
            }
        }
    }
    
    void WeaponSystem::Fire(
        Pillar::Entity owner, 
        const Pillar::TransformComponent& transform, 
        WeaponComponent& weapon)
    {
        // Calculate firing direction
        glm::vec2 direction(
            std::cos(transform.Rotation),
            std::sin(transform.Rotation)
        );
        
        // Spawn position slightly ahead of player
        glm::vec2 spawnPos = transform.Position + direction * 0.6f;
        
        // Apply spread for multiple bullets
        for (int i = 0; i < weapon.BulletsPerShot; ++i)
        {
            glm::vec2 bulletDir = direction;
            
            if (weapon.Spread > 0.0f)
            {
                float spreadAngle = glm::radians(
                    (static_cast<float>(rand()) / RAND_MAX - 0.5f) * weapon.Spread
                );
                bulletDir = glm::vec2(
                    direction.x * std::cos(spreadAngle) - direction.y * std::sin(spreadAngle),
                    direction.x * std::sin(spreadAngle) + direction.y * std::cos(spreadAngle)
                );
            }
            
            // Spawn bullet (use pool if available)
            if (m_BulletPool)
            {
                m_GameLayer->SpawnBullet(
                    owner, spawnPos, bulletDir, 
                    weapon.BulletSpeed, weapon.Damage
                );
            }
        }
        
        // Play fire sound
        if (weapon.FireSound)
        {
            Pillar::AudioEngine::PlayOneShot(
                weapon.FireSound->GetFilePath(),
                0.5f,  // Volume
                1.0f,  // Pitch
                glm::vec3(transform.Position, 0.0f),  // 3D position
                Pillar::AudioEngine::AudioBus::SFX
            );
        }
        
        // Spawn muzzle flash particle
        if (m_GameLayer)
        {
            m_GameLayer->SpawnMuzzleFlash(spawnPos, direction);
        }
        
        weapon.ResetCooldown();
    }

} // namespace Game
```

### 3.2 Movement & Physics

**Collision Categories:**

```cpp
// In Sandbox/src/CollisionCategories.h
#pragma once

#include <cstdint>

namespace Game {

    namespace CollisionCategory {
        constexpr uint16_t None     = 0x0000;
        constexpr uint16_t Player   = 0x0001;
        constexpr uint16_t Enemy    = 0x0002;
        constexpr uint16_t Bullet   = 0x0004;
        constexpr uint16_t Wall     = 0x0008;
        constexpr uint16_t PowerUp  = 0x0010;
        constexpr uint16_t Trigger  = 0x0020;
        constexpr uint16_t All      = 0xFFFF;
    }

} // namespace Game
```

**Physics Configuration:**

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::OnAttach()
{
    // Initialize physics with zero gravity (top-down view)
    m_PhysicsSystem = std::make_unique<Pillar::PhysicsSystem>(glm::vec2(0.0f, 0.0f));
    m_PhysicsSystem->OnAttach(&m_Scene);
    m_Scene.SetPhysicsSystem(m_PhysicsSystem.get());
    
    // Bullet collision uses raycasts
    m_BulletCollisionSystem = std::make_unique<Pillar::BulletCollisionSystem>(
        m_PhysicsSystem.get()
    );
    m_BulletCollisionSystem->OnAttach(&m_Scene);
    
    // Velocity integration for lightweight entities
    m_VelocitySystem = std::make_unique<Pillar::VelocityIntegrationSystem>();
    m_VelocitySystem->OnAttach(&m_Scene);
}
```

**Velocity Integration System** (for bullets and lightweight enemies):

Pillar includes `VelocityIntegrationSystem` which automatically updates `TransformComponent.Position` based on `VelocityComponent.Velocity`. This runs separately from Box2D.

```cpp
// System already implemented in Pillar - usage:
m_VelocitySystem.OnUpdate(dt);
```

### 3.3 AI & Wave Management

**Enemy AI System:**

```cpp
// In Sandbox/src/Systems/EnemyAISystem.cpp
#include "EnemyAISystem.h"
#include "Components/EnemyComponent.h"
#include "Components/PlayerTagComponent.h"

namespace Game {

    void EnemyAISystem::OnUpdate(float dt)
    {
        if (!m_Scene) return;
        
        // Find player entity
        Pillar::Entity playerEntity;
        glm::vec2 playerPos(0.0f);
        
        auto& registry = m_Scene->GetRegistry();
        auto playerView = registry.view<Pillar::TransformComponent, PlayerTagComponent>();
        for (auto entity : playerView)
        {
            playerEntity = Pillar::Entity(entity, m_Scene);
            playerPos = playerView.get<Pillar::TransformComponent>(entity).Position;
            break;
        }
        
        if (!playerEntity.IsValid()) return;
        
        // Update all enemies
        auto enemyView = registry.view<
            Pillar::TransformComponent, 
            EnemyComponent
        >();
        
        for (auto entity : enemyView)
        {
            auto& transform = enemyView.get<Pillar::TransformComponent>(entity);
            auto& enemy = enemyView.get<EnemyComponent>(entity);
            
            enemy.TargetEntity = playerEntity;
            
            // Calculate direction to player
            glm::vec2 toPlayer = playerPos - transform.Position;
            float distanceToPlayer = glm::length(toPlayer);
            
            if (distanceToPlayer > 0.001f)
                toPlayer = glm::normalize(toPlayer);
            
            // Update attack timer
            if (enemy.AttackTimer > 0.0f)
                enemy.AttackTimer -= dt;
            
            // State machine
            switch (enemy.Type)
            {
                case EnemyType::Chaser:
                    UpdateChaser(Pillar::Entity(entity, m_Scene), enemy, transform, 
                                 toPlayer, distanceToPlayer, dt);
                    break;
                    
                case EnemyType::Shooter:
                    UpdateShooter(Pillar::Entity(entity, m_Scene), enemy, transform,
                                  toPlayer, distanceToPlayer, dt);
                    break;
                    
                case EnemyType::Swarm:
                    UpdateSwarm(Pillar::Entity(entity, m_Scene), enemy, transform,
                                toPlayer, distanceToPlayer, dt);
                    break;
            }
            
            // Face movement direction
            if (distanceToPlayer > 0.1f)
            {
                float angle = std::atan2(toPlayer.y, toPlayer.x);
                transform.SetRotation(angle);
            }
        }
    }
    
    void EnemyAISystem::UpdateChaser(
        Pillar::Entity entity, 
        EnemyComponent& enemy,
        Pillar::TransformComponent& transform,
        const glm::vec2& toPlayer,
        float distance,
        float dt)
    {
        // Always chase player
        if (distance > enemy.AttackRange)
        {
            enemy.State = EnemyState::Chasing;
            
            // Apply movement via Box2D
            if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
            {
                if (rb->Body)
                {
                    b2Vec2 velocity(
                        toPlayer.x * enemy.MoveSpeed,
                        toPlayer.y * enemy.MoveSpeed
                    );
                    rb->Body->SetLinearVelocity(velocity);
                }
            }
        }
        else
        {
            // In attack range - deal damage
            enemy.State = EnemyState::Attacking;
            
            if (enemy.AttackTimer <= 0.0f)
            {
                // Damage player
                if (auto* health = enemy.TargetEntity.TryGetComponent<Pillar::HealthComponent>())
                {
                    health->TakeDamage(enemy.AttackDamage);
                }
                enemy.AttackTimer = enemy.AttackCooldown;
            }
        }
    }
    
    void EnemyAISystem::UpdateSwarm(
        Pillar::Entity entity,
        EnemyComponent& enemy,
        Pillar::TransformComponent& transform,
        const glm::vec2& toPlayer,
        float distance,
        float dt)
    {
        // Swarm enemies use VelocityComponent (lightweight)
        if (auto* velocity = entity.TryGetComponent<Pillar::VelocityComponent>())
        {
            if (distance > enemy.AttackRange)
            {
                velocity->Velocity = toPlayer * enemy.MoveSpeed;
            }
            else
            {
                velocity->Velocity = glm::vec2(0.0f);
                
                // Contact damage
                if (enemy.AttackTimer <= 0.0f)
                {
                    if (auto* health = enemy.TargetEntity.TryGetComponent<Pillar::HealthComponent>())
                    {
                        health->TakeDamage(enemy.AttackDamage);
                    }
                    enemy.AttackTimer = enemy.AttackCooldown;
                }
            }
        }
    }

} // namespace Game
```

**Wave Manager:**

```cpp
// In Sandbox/src/WaveManager.h
#pragma once

#include <Pillar/ECS/Scene.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>

namespace Game {

    struct WaveConfig
    {
        int EnemyCount = 10;
        float SpawnDelay = 0.5f;        // Time between spawns
        float EnemyHealthMultiplier = 1.0f;
        float EnemyDamageMultiplier = 1.0f;
        
        // Enemy type distribution (probabilities summing to 1.0)
        float ChaserWeight = 0.6f;
        float ShooterWeight = 0.2f;
        float SwarmWeight = 0.2f;
    };

    class WaveManager
    {
    public:
        void Init(Pillar::Scene* scene, const glm::vec2& arenaSize);
        void OnUpdate(float dt);
        
        void StartWave(int waveNumber);
        void ForceNextWave();
        
        int GetCurrentWave() const { return m_CurrentWave; }
        int GetEnemiesRemaining() const { return m_EnemiesRemaining; }
        bool IsWaveActive() const { return m_WaveActive; }
        
        using WaveCompleteCallback = std::function<void(int waveNumber)>;
        void SetWaveCompleteCallback(WaveCompleteCallback callback) 
        { 
            m_OnWaveComplete = callback; 
        }
        
    private:
        glm::vec2 GetRandomSpawnPosition() const;
        EnemyType SelectEnemyType(const WaveConfig& config) const;
        WaveConfig GenerateWaveConfig(int waveNumber) const;
        
        Pillar::Scene* m_Scene = nullptr;
        glm::vec2 m_ArenaSize;
        
        int m_CurrentWave = 0;
        int m_EnemiesRemaining = 0;
        int m_EnemiesSpawned = 0;
        bool m_WaveActive = false;
        
        WaveConfig m_CurrentConfig;
        float m_SpawnTimer = 0.0f;
        float m_WaveEndDelay = 2.0f;
        float m_WaveEndTimer = 0.0f;
        
        WaveCompleteCallback m_OnWaveComplete;
    };

} // namespace Game
```

```cpp
// In Sandbox/src/WaveManager.cpp
#include "WaveManager.h"
#include "EntityFactory.h"
#include <random>

namespace Game {

    static std::random_device s_RandomDevice;
    static std::mt19937 s_RandomEngine(s_RandomDevice());

    void WaveManager::Init(Pillar::Scene* scene, const glm::vec2& arenaSize)
    {
        m_Scene = scene;
        m_ArenaSize = arenaSize;
    }
    
    void WaveManager::OnUpdate(float dt)
    {
        if (!m_WaveActive) return;
        
        // Spawn enemies
        if (m_EnemiesSpawned < m_CurrentConfig.EnemyCount)
        {
            m_SpawnTimer -= dt;
            
            if (m_SpawnTimer <= 0.0f)
            {
                glm::vec2 spawnPos = GetRandomSpawnPosition();
                EnemyType type = SelectEnemyType(m_CurrentConfig);
                
                auto enemy = EntityFactory::CreateEnemy(*m_Scene, spawnPos, type);
                
                // Apply wave multipliers
                if (auto* health = enemy.TryGetComponent<Pillar::HealthComponent>())
                {
                    health->MaxHealth *= m_CurrentConfig.EnemyHealthMultiplier;
                    health->CurrentHealth = health->MaxHealth;
                }
                
                if (auto* enemyComp = enemy.TryGetComponent<EnemyComponent>())
                {
                    enemyComp->AttackDamage *= m_CurrentConfig.EnemyDamageMultiplier;
                }
                
                m_EnemiesSpawned++;
                m_SpawnTimer = m_CurrentConfig.SpawnDelay;
            }
        }
        
        // Count remaining enemies
        auto& registry = m_Scene->GetRegistry();
        m_EnemiesRemaining = 0;
        
        auto view = registry.view<EnemyComponent>();
        for (auto entity : view)
        {
            auto* health = registry.try_get<Pillar::HealthComponent>(entity);
            if (health && !health->IsDead)
                m_EnemiesRemaining++;
        }
        
        // Check wave completion
        if (m_EnemiesSpawned >= m_CurrentConfig.EnemyCount && m_EnemiesRemaining == 0)
        {
            m_WaveEndTimer -= dt;
            
            if (m_WaveEndTimer <= 0.0f)
            {
                m_WaveActive = false;
                
                if (m_OnWaveComplete)
                    m_OnWaveComplete(m_CurrentWave);
            }
        }
    }
    
    void WaveManager::StartWave(int waveNumber)
    {
        m_CurrentWave = waveNumber;
        m_CurrentConfig = GenerateWaveConfig(waveNumber);
        m_EnemiesSpawned = 0;
        m_EnemiesRemaining = 0;
        m_SpawnTimer = 0.0f;
        m_WaveEndTimer = m_WaveEndDelay;
        m_WaveActive = true;
    }
    
    WaveConfig WaveManager::GenerateWaveConfig(int waveNumber) const
    {
        WaveConfig config;
        
        // Scale difficulty with wave number
        config.EnemyCount = 5 + waveNumber * 3;
        config.SpawnDelay = std::max(0.2f, 1.0f - waveNumber * 0.05f);
        config.EnemyHealthMultiplier = 1.0f + (waveNumber - 1) * 0.1f;
        config.EnemyDamageMultiplier = 1.0f + (waveNumber - 1) * 0.05f;
        
        // Adjust enemy type distribution
        if (waveNumber < 3)
        {
            config.ChaserWeight = 1.0f;
            config.ShooterWeight = 0.0f;
            config.SwarmWeight = 0.0f;
        }
        else if (waveNumber < 5)
        {
            config.ChaserWeight = 0.7f;
            config.ShooterWeight = 0.3f;
            config.SwarmWeight = 0.0f;
        }
        else
        {
            config.ChaserWeight = 0.4f;
            config.ShooterWeight = 0.3f;
            config.SwarmWeight = 0.3f;
        }
        
        return config;
    }
    
    glm::vec2 WaveManager::GetRandomSpawnPosition() const
    {
        std::uniform_real_distribution<float> distX(-m_ArenaSize.x * 0.5f + 1.0f, 
                                                     m_ArenaSize.x * 0.5f - 1.0f);
        std::uniform_real_distribution<float> distY(-m_ArenaSize.y * 0.5f + 1.0f,
                                                     m_ArenaSize.y * 0.5f - 1.0f);
        std::uniform_int_distribution<int> sideChoice(0, 3);
        
        // Spawn from arena edges
        int side = sideChoice(s_RandomEngine);
        glm::vec2 pos;
        
        switch (side)
        {
            case 0: // Top
                pos = { distX(s_RandomEngine), m_ArenaSize.y * 0.5f - 1.0f };
                break;
            case 1: // Bottom
                pos = { distX(s_RandomEngine), -m_ArenaSize.y * 0.5f + 1.0f };
                break;
            case 2: // Left
                pos = { -m_ArenaSize.x * 0.5f + 1.0f, distY(s_RandomEngine) };
                break;
            case 3: // Right
                pos = { m_ArenaSize.x * 0.5f - 1.0f, distY(s_RandomEngine) };
                break;
        }
        
        return pos;
    }
    
    EnemyType WaveManager::SelectEnemyType(const WaveConfig& config) const
    {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float roll = dist(s_RandomEngine);
        
        if (roll < config.ChaserWeight)
            return EnemyType::Chaser;
        else if (roll < config.ChaserWeight + config.ShooterWeight)
            return EnemyType::Shooter;
        else
            return EnemyType::Swarm;
    }

} // namespace Game
```

### 3.4 Collision & Damage

**Damage System:**

```cpp
// In Sandbox/src/Systems/DamageSystem.cpp
#include "DamageSystem.h"
#include "Components/EnemyComponent.h"
#include "Components/PlayerTagComponent.h"

namespace Game {

    void DamageSystem::OnUpdate(float dt)
    {
        if (!m_Scene) return;
        
        auto& registry = m_Scene->GetRegistry();
        
        // Update invulnerability timers
        auto healthView = registry.view<Pillar::HealthComponent>();
        for (auto entity : healthView)
        {
            auto& health = healthView.get<Pillar::HealthComponent>(entity);
            
            if (health.InvulnerabilityTimer > 0.0f)
            {
                health.InvulnerabilityTimer -= dt;
                
                if (health.InvulnerabilityTimer <= 0.0f)
                    health.IsInvulnerable = false;
            }
        }
        
        // Handle deaths
        ProcessDeaths();
    }
    
    void DamageSystem::ProcessDeaths()
    {
        auto& registry = m_Scene->GetRegistry();
        std::vector<entt::entity> toDestroy;
        
        auto view = registry.view<Pillar::HealthComponent>();
        for (auto entity : view)
        {
            auto& health = view.get<Pillar::HealthComponent>(entity);
            
            if (!health.IsDead) continue;
            
            Pillar::Entity e(entity, m_Scene);
            
            // Check if it's an enemy
            if (auto* enemy = e.TryGetComponent<EnemyComponent>())
            {
                // Spawn death effects
                if (m_GameLayer)
                {
                    auto& transform = e.GetComponent<Pillar::TransformComponent>();
                    m_GameLayer->SpawnDeathEffect(transform.Position);
                    m_GameLayer->SpawnXPDrop(transform.Position, enemy->XPValue);
                }
                
                // Play death sound
                Pillar::AudioEngine::PlayOneShot(
                    Pillar::AssetManager::GetSFXPath("enemy_death.wav"),
                    0.6f, 1.0f, std::nullopt,
                    Pillar::AudioEngine::AudioBus::SFX
                );
                
                if (health.DestroyOnDeath)
                    toDestroy.push_back(entity);
                else
                    enemy->State = EnemyState::Dying;
            }
            
            // Check if it's the player
            if (e.HasComponent<PlayerTagComponent>())
            {
                // Trigger game over
                if (m_GameLayer)
                    m_GameLayer->OnPlayerDeath();
            }
        }
        
        // Destroy marked entities
        for (auto entity : toDestroy)
        {
            m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
        }
    }
    
    void DamageSystem::ApplyDamage(Pillar::Entity target, float damage, Pillar::Entity source)
    {
        if (!target.IsValid()) return;
        
        auto* health = target.TryGetComponent<Pillar::HealthComponent>();
        if (!health) return;
        
        float actualDamage = health->TakeDamage(damage);
        
        if (actualDamage > 0.0f)
        {
            // Trigger damage feedback
            if (m_GameLayer)
            {
                auto& transform = target.GetComponent<Pillar::TransformComponent>();
                m_GameLayer->OnEntityDamaged(target, transform.Position, actualDamage);
            }
        }
    }

} // namespace Game
```

---

## 4. Feedback & Polish ("Juice")

### 4.1 Visual Effects

**Screen Shake:**

```cpp
// In Sandbox/src/CameraShake.h
#pragma once

#include <Pillar/Renderer/OrthographicCameraController.h>
#include <glm/glm.hpp>
#include <random>

namespace Game {

    class CameraShake
    {
    public:
        void Shake(float intensity, float duration)
        {
            m_Intensity = intensity;
            m_Duration = duration;
            m_Timer = duration;
        }
        
        void Update(float dt, Pillar::OrthographicCameraController& controller)
        {
            if (m_Timer <= 0.0f) return;
            
            m_Timer -= dt;
            float t = m_Timer / m_Duration;
            float currentIntensity = m_Intensity * t;  // Fade out
            
            std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
            glm::vec2 offset(
                dist(m_Random) * currentIntensity,
                dist(m_Random) * currentIntensity
            );
            
            m_CurrentOffset = offset;
        }
        
        glm::vec2 GetOffset() const { return m_CurrentOffset; }
        
    private:
        float m_Intensity = 0.0f;
        float m_Duration = 0.0f;
        float m_Timer = 0.0f;
        glm::vec2 m_CurrentOffset{0.0f};
        
        std::mt19937 m_Random{std::random_device{}()};
    };

} // namespace Game
```

**Usage in game loop:**

```cpp
void GameLayer::OnUpdate(float dt)
{
    // ... other updates ...
    
    m_CameraShake.Update(dt, m_CameraController);
    
    // Apply shake offset when rendering
    Renderer2D::BeginScene(m_CameraController.GetCamera());
    
    // The actual camera position includes shake
    glm::vec2 shakeOffset = m_CameraShake.GetOffset();
    // Apply offset to rendered positions or camera
}

void GameLayer::OnEnemyKilled(const glm::vec2& position)
{
    m_CameraShake.Shake(0.1f, 0.1f);  // Small shake
}

void GameLayer::OnPlayerDamaged()
{
    m_CameraShake.Shake(0.3f, 0.2f);  // Bigger shake
}
```

**Damage Flash Effect:**

```cpp
// In Sandbox/src/Systems/FlashSystem.h
#pragma once

#include <Pillar/ECS/Systems/System.h>

namespace Game {

    struct FlashComponent
    {
        glm::vec4 OriginalColor;
        glm::vec4 FlashColor = { 1.0f, 0.0f, 0.0f, 1.0f };  // Red
        float Duration = 0.1f;
        float Timer = 0.0f;
    };

    class FlashSystem : public Pillar::System
    {
    public:
        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;
            
            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<FlashComponent, Pillar::SpriteComponent>();
            
            std::vector<entt::entity> toRemove;
            
            for (auto entity : view)
            {
                auto& flash = view.get<FlashComponent>(entity);
                auto& sprite = view.get<Pillar::SpriteComponent>(entity);
                
                flash.Timer -= dt;
                
                if (flash.Timer <= 0.0f)
                {
                    sprite.Color = flash.OriginalColor;
                    toRemove.push_back(entity);
                }
                else
                {
                    // Lerp back to original color
                    float t = 1.0f - (flash.Timer / flash.Duration);
                    sprite.Color = glm::mix(flash.FlashColor, flash.OriginalColor, t);
                }
            }
            
            for (auto entity : toRemove)
            {
                registry.remove<FlashComponent>(entity);
            }
        }
    };

} // namespace Game

// Usage:
void GameLayer::OnEntityDamaged(Pillar::Entity entity, const glm::vec2& position, float damage)
{
    if (auto* sprite = entity.TryGetComponent<Pillar::SpriteComponent>())
    {
        auto& flash = entity.GetOrAddComponent<FlashComponent>();
        flash.OriginalColor = sprite->Color;
        flash.FlashColor = { 1.0f, 0.3f, 0.3f, 1.0f };
        flash.Duration = 0.1f;
        flash.Timer = 0.1f;
        
        sprite->Color = flash.FlashColor;
    }
}
```

**Muzzle Flash & Particles:**

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::SpawnMuzzleFlash(const glm::vec2& position, const glm::vec2& direction)
{
    // Create temporary particle emitter entity
    auto emitter = m_Scene.CreateEntity("MuzzleFlash");
    
    auto& transform = emitter.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    transform.SetRotation(std::atan2(direction.y, direction.x));
    
    auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();
    emitterComp.BurstMode = true;
    emitterComp.BurstCount = 8;
    emitterComp.Enabled = true;
    
    emitterComp.Direction = direction;
    emitterComp.DirectionSpread = 30.0f;
    emitterComp.Speed = 8.0f;
    emitterComp.SpeedVariance = 3.0f;
    
    emitterComp.Lifetime = 0.15f;
    emitterComp.LifetimeVariance = 0.05f;
    emitterComp.Size = 0.15f;
    emitterComp.SizeVariance = 0.05f;
    
    emitterComp.StartColor = { 1.0f, 0.8f, 0.2f, 1.0f };  // Yellow-orange
    emitterComp.FadeOut = true;
    emitterComp.Gravity = { 0.0f, 0.0f };
    
    // Mark for auto-cleanup
    emitter.AddComponent<TemporaryComponent>().Lifetime = 0.2f;
}

void GameLayer::SpawnDeathEffect(const glm::vec2& position)
{
    auto emitter = m_Scene.CreateEntity("DeathEffect");
    
    auto& transform = emitter.AddComponent<Pillar::TransformComponent>();
    transform.SetPosition(position);
    
    auto& emitterComp = emitter.AddComponent<Pillar::ParticleEmitterComponent>();
    emitterComp.BurstMode = true;
    emitterComp.BurstCount = 20;
    emitterComp.Enabled = true;
    
    emitterComp.Shape = Pillar::EmissionShape::Circle;
    emitterComp.ShapeSize = { 0.5f, 0.5f };
    
    emitterComp.DirectionSpread = 360.0f;  // All directions
    emitterComp.Speed = 5.0f;
    emitterComp.SpeedVariance = 2.0f;
    
    emitterComp.Lifetime = 0.5f;
    emitterComp.LifetimeVariance = 0.2f;
    emitterComp.Size = 0.2f;
    
    emitterComp.StartColor = { 1.0f, 0.2f, 0.2f, 1.0f };  // Red
    emitterComp.FadeOut = true;
    emitterComp.ScaleOverTime = true;
    emitterComp.EndScale = 0.2f;
    emitterComp.Gravity = { 0.0f, -3.0f };
    
    emitter.AddComponent<TemporaryComponent>().Lifetime = 1.0f;
}
```

### 4.2 Audio Integration

**Audio Setup:**

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::InitializeAudio()
{
    // Pre-load frequently used sound effects
    m_Sounds.Gunshot = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("gunshot.wav")
    );
    m_Sounds.EnemyHit = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("enemy_hit.wav")
    );
    m_Sounds.EnemyDeath = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("enemy_death.wav")
    );
    m_Sounds.PlayerHit = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("player_hit.wav")
    );
    m_Sounds.PowerUp = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("powerup.wav")
    );
    m_Sounds.WaveStart = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetSFXPath("wave_start.wav")
    );
    
    // Background music (looping)
    m_MusicSource = Pillar::AudioEngine::CreateSource();
    auto musicBuffer = Pillar::AudioEngine::CreateBuffer(
        Pillar::AssetManager::GetMusicPath("gameplay_loop.wav")
    );
    m_MusicSource->SetBuffer(musicBuffer);
    m_MusicSource->SetLooping(true);
    m_MusicSource->SetVolume(0.4f);
    Pillar::AudioEngine::SetSourceBus(m_MusicSource, Pillar::AudioEngine::AudioBus::Music);
}

void GameLayer::StartBackgroundMusic()
{
    m_MusicSource->Play();
}

void GameLayer::StopBackgroundMusic()
{
    Pillar::AudioEngine::FadeOut(Pillar::AudioEngine::AudioBus::Music, 1.0f);
}
```

**3D Positional Audio:**

```cpp
void GameLayer::PlaySoundAtPosition(
    const std::shared_ptr<Pillar::AudioBuffer>& buffer,
    const glm::vec2& position,
    float volume,
    float pitch)
{
    Pillar::AudioEngine::PlayOneShot(
        buffer->GetFilePath(),
        volume,
        pitch,
        glm::vec3(position, 0.0f),  // 3D position
        Pillar::AudioEngine::AudioBus::SFX
    );
}

void GameLayer::OnUpdate(float dt)
{
    // Update listener to follow player
    if (m_PlayerEntity.IsValid())
    {
        auto& playerTransform = m_PlayerEntity.GetComponent<Pillar::TransformComponent>();
        Pillar::AudioEngine::SetListenerPosition(glm::vec3(playerTransform.Position, 0.0f));
        
        // Listener faces same direction as camera (top-down, so "forward" is into screen)
        Pillar::AudioEngine::SetListenerOrientation(
            glm::vec3(0.0f, 0.0f, -1.0f),  // Forward
            glm::vec3(0.0f, 1.0f, 0.0f)    // Up
        );
    }
    
    // Update audio engine (handles fades, cleanup)
    Pillar::AudioEngine::Update(dt);
}
```

### 4.3 Camera Effects

**Camera Follow with Smoothing:**

```cpp
// In Sandbox/src/GameCamera.h
#pragma once

#include <Pillar/Renderer/OrthographicCameraController.h>
#include <glm/glm.hpp>

namespace Game {

    class GameCamera
    {
    public:
        GameCamera(float aspectRatio)
            : m_Controller(aspectRatio, false)
        {
        }
        
        void SetTarget(const glm::vec2& target) { m_Target = target; }
        
        void OnUpdate(float dt)
        {
            // Smooth follow
            glm::vec3 currentPos = m_Controller.GetCamera().GetPosition();
            glm::vec2 currentPos2D(currentPos.x, currentPos.y);
            
            glm::vec2 toTarget = m_Target - currentPos2D;
            float distance = glm::length(toTarget);
            
            if (distance > 0.01f)
            {
                float moveAmount = std::min(distance, m_FollowSpeed * dt);
                glm::vec2 newPos = currentPos2D + glm::normalize(toTarget) * moveAmount;
                m_Controller.GetCamera().SetPosition(glm::vec3(newPos, 0.0f));
            }
            
            // Apply shake offset (from external CameraShake)
            // Note: Shake is applied in rendering, not to actual camera position
        }
        
        void OnEvent(Pillar::Event& e) { m_Controller.OnEvent(e); }
        
        Pillar::OrthographicCamera& GetCamera() { return m_Controller.GetCamera(); }
        Pillar::OrthographicCameraController& GetController() { return m_Controller; }
        
        void SetFollowSpeed(float speed) { m_FollowSpeed = speed; }
        void SetZoom(float zoom) { m_Controller.SetZoomLevel(zoom); }
        
    private:
        Pillar::OrthographicCameraController m_Controller;
        glm::vec2 m_Target{0.0f};
        float m_FollowSpeed = 10.0f;
    };

} // namespace Game
```

### 4.4 UI Feedback

**Debug HUD with ImGui:**

```cpp
// In Sandbox/src/GameLayer.cpp
void GameLayer::OnImGuiRender()
{
    // Game stats panel
    ImGui::Begin("Game Stats");
    
    ImGui::Text("Wave: %d", m_WaveManager.GetCurrentWave());
    ImGui::Text("Enemies: %d", m_WaveManager.GetEnemiesRemaining());
    
    if (m_PlayerEntity.IsValid())
    {
        auto& health = m_PlayerEntity.GetComponent<Pillar::HealthComponent>();
        ImGui::ProgressBar(health.CurrentHealth / health.MaxHealth, ImVec2(-1, 0), 
                          "HP");
        
        auto& weapon = m_PlayerEntity.GetComponent<WeaponComponent>();
        ImGui::Text("Weapon: %s", weapon.WeaponName.c_str());
        ImGui::Text("Fire Rate: %.1f/s", weapon.FireRate);
    }
    
    ImGui::Separator();
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Entities: %zu", m_Scene.GetEntityCount());
    ImGui::Text("Active Particles: %u", m_ParticleSystem.GetActiveParticleCount());
    
    if (ImGui::Button("Next Wave"))
        m_WaveManager.ForceNextWave();
    
    ImGui::End();
    
    // Audio controls
    ImGui::Begin("Audio");
    
    static float masterVolume = 1.0f;
    if (ImGui::SliderFloat("Master", &masterVolume, 0.0f, 1.0f))
        Pillar::AudioEngine::SetMasterVolume(masterVolume);
    
    static float sfxVolume = 1.0f;
    if (ImGui::SliderFloat("SFX", &sfxVolume, 0.0f, 1.0f))
        Pillar::AudioEngine::SetBusVolume(Pillar::AudioEngine::AudioBus::SFX, sfxVolume);
    
    static float musicVolume = 0.4f;
    if (ImGui::SliderFloat("Music", &musicVolume, 0.0f, 1.0f))
        Pillar::AudioEngine::SetBusVolume(Pillar::AudioEngine::AudioBus::Music, musicVolume);
    
    ImGui::End();
}
```

---

## 5. Implementation Roadmap

### 5.1 Phase 1: Core Framework (Day 1) ✅ COMPLETE

**Tasks:**
1. Create `GameLayer` inheriting from `Pillar::Layer`
2. Set up `Scene` with physics system (zero gravity)
3. Create `EntityFactory` with player creation
4. Implement basic WASD movement via Box2D
5. Create arena bounds (walls)
6. Set up camera follow

**Validation Checklist:**
- [x] Player spawns in center of arena
- [x] WASD moves player smoothly
- [x] Player stops at walls
- [x] Camera follows player
- [x] ESC closes application

### 5.2 Phase 2: Combat Mechanics (Day 1-2) ✅ COMPLETE

**Tasks:**
1. Implement mouse aiming (screen-to-world conversion)
2. Create bullet spawning with object pool
3. Implement `WeaponSystem` with fire rate control
4. Add `BulletCollisionSystem` integration
5. Create basic enemy with chaser AI
6. Implement `HealthComponent` damage/death

**Validation Checklist:**
- [x] Player rotates to face mouse cursor
- [x] Left-click fires bullets at fire rate
- [x] Bullets travel in aimed direction
- [x] Bullets despawn after lifetime or on wall hit
- [x] Enemy chases player
- [x] Bullets damage and kill enemies
- [x] Enemy death removes entity

### 5.3 Phase 3: Enemy AI & Waves (Day 2) ✅ COMPLETE

**Tasks:**
1. Create `WaveManager` with escalating difficulty
2. Add multiple enemy types (Chaser, Shooter, Swarm)
3. Implement spawn position randomization
4. Add wave completion detection
5. Create power-up drops and collection

**Validation Checklist:**
- [x] Wave starts with correct enemy count
- [x] Enemies spawn from arena edges
- [x] New wave starts after all enemies killed
- [x] Wave 3+ includes Shooter enemies
- [x] Wave 5+ includes Swarm enemies
- [x] Power-ups spawn and can be collected

### 5.4 Phase 4: Polish & Effects (Day 3) ✅ COMPLETE

**Tasks:**
1. Add particle effects (muzzle flash, death, hit)
2. Implement screen shake
3. Add damage flash effect
4. Set up audio (SFX + music)
5. Create UI HUD with ImGui
6. Performance optimization (pooling, batching)

**Validation Checklist:**
- [x] Muzzle flash appears when shooting
- [x] Death particles spawn when enemy dies
- [x] Camera shakes on kills/damage
- [x] Entities flash red when hit (FlashSystem ready)
- [ ] Audio plays for all actions (TODO: Add sound effects)
- [ ] Background music loops (TODO: Add music)
- [x] HUD shows health, wave, enemies
- [x] Stable 60 FPS with 50+ enemies

---

## 6. API Reference

### 6.1 New Components

| Component | File | Purpose |
|-----------|------|---------|
| `PlayerTagComponent` | `Components/PlayerTagComponent.h` | Marks player, stores movement stats |
| `WeaponComponent` | `Components/WeaponComponent.h` | Weapon stats, cooldown, sound |
| `EnemyComponent` | `Components/EnemyComponent.h` | AI type, state, combat stats |
| `PowerUpComponent` | `Components/PowerUpComponent.h` | Power-up type, value, duration |
| `FlashComponent` | `Components/FlashComponent.h` | Damage flash effect state |
| `TemporaryComponent` | `Components/TemporaryComponent.h` | Auto-destroy after lifetime |

### 6.2 New Systems

| System | File | Purpose |
|--------|------|---------|
| `PlayerMovementSystem` | `Systems/PlayerMovementSystem.h` | WASD + dash movement |
| `WeaponSystem` | `Systems/WeaponSystem.h` | Firing, cooldown, bullet spawning |
| `EnemyAISystem` | `Systems/EnemyAISystem.h` | AI state machines |
| `DamageSystem` | `Systems/DamageSystem.h` | Damage application, death handling |
| `FlashSystem` | `Systems/FlashSystem.h` | Damage flash interpolation |
| `TemporaryCleanupSystem` | `Systems/TemporaryCleanupSystem.h` | Destroy expired entities |

### 6.3 Utility Functions

```cpp
// In Sandbox/src/GameUtils.h
#pragma once

#include <glm/glm.hpp>
#include <Pillar/Renderer/OrthographicCamera.h>

namespace Game {

    // Convert screen coordinates to world coordinates
    glm::vec2 ScreenToWorld(
        float screenX, float screenY,
        float windowWidth, float windowHeight,
        const Pillar::OrthographicCamera& camera);
    
    // Get normalized direction from angle (radians)
    inline glm::vec2 AngleToDirection(float radians)
    {
        return { std::cos(radians), std::sin(radians) };
    }
    
    // Get angle from direction vector
    inline float DirectionToAngle(const glm::vec2& dir)
    {
        return std::atan2(dir.y, dir.x);
    }
    
    // Random float in range
    float RandomFloat(float min, float max);
    
    // Random vec2 in circle
    glm::vec2 RandomInCircle(float radius);
    
    // Random vec2 on circle edge
    glm::vec2 RandomOnCircle(float radius);

} // namespace Game
```

---

## 7. Testing Strategy

### 7.1 Unit Tests

Add tests in `Tests/src/`:

```cpp
// In Tests/src/ShooterComponentTests.cpp
#include <gtest/gtest.h>
#include "Components/WeaponComponent.h"
#include "Components/EnemyComponent.h"

TEST(WeaponComponentTest, FireCooldown)
{
    Game::WeaponComponent weapon;
    weapon.FireRate = 5.0f;  // 5 shots/sec = 0.2s cooldown
    
    EXPECT_TRUE(weapon.CanFire());
    
    weapon.ResetCooldown();
    EXPECT_FALSE(weapon.CanFire());
    EXPECT_FLOAT_EQ(weapon.FireCooldown, 0.2f);
    
    weapon.UpdateCooldown(0.1f);
    EXPECT_FALSE(weapon.CanFire());
    
    weapon.UpdateCooldown(0.15f);
    EXPECT_TRUE(weapon.CanFire());
}

TEST(HealthComponentTest, Damage)
{
    Pillar::HealthComponent health(100.0f);
    
    EXPECT_FALSE(health.IsDead);
    EXPECT_FLOAT_EQ(health.CurrentHealth, 100.0f);
    
    float damage = health.TakeDamage(30.0f);
    EXPECT_FLOAT_EQ(damage, 30.0f);
    EXPECT_FLOAT_EQ(health.CurrentHealth, 70.0f);
    EXPECT_FALSE(health.IsDead);
    
    health.TakeDamage(70.0f);
    EXPECT_FLOAT_EQ(health.CurrentHealth, 0.0f);
    EXPECT_TRUE(health.IsDead);
}

TEST(HealthComponentTest, Invulnerability)
{
    Pillar::HealthComponent health(100.0f);
    health.IsInvulnerable = true;
    
    float damage = health.TakeDamage(50.0f);
    EXPECT_FLOAT_EQ(damage, 0.0f);
    EXPECT_FLOAT_EQ(health.CurrentHealth, 100.0f);
}
```

### 7.2 Integration Tests

```cpp
// In Tests/src/ShooterSystemTests.cpp
#include <gtest/gtest.h>
#include <Pillar/ECS/Scene.h>
#include "EntityFactory.h"
#include "Systems/EnemyAISystem.h"

class ShooterSystemTest : public ::testing::Test
{
protected:
    Pillar::Scene m_Scene;
    
    void SetUp() override
    {
        // Initialize scene
    }
};

TEST_F(ShooterSystemTest, EnemyChasesPlayer)
{
    auto player = Game::EntityFactory::CreatePlayer(m_Scene, { 0.0f, 0.0f });
    auto enemy = Game::EntityFactory::CreateEnemy(m_Scene, { 5.0f, 0.0f }, 
                                                  Game::EnemyType::Chaser);
    
    Game::EnemyAISystem aiSystem;
    aiSystem.OnAttach(&m_Scene);
    
    // Simulate several frames
    for (int i = 0; i < 60; ++i)
    {
        aiSystem.OnUpdate(1.0f / 60.0f);
    }
    
    auto& transform = enemy.GetComponent<Pillar::TransformComponent>();
    
    // Enemy should have moved toward player (x should be less than 5)
    EXPECT_LT(transform.Position.x, 5.0f);
}

TEST_F(ShooterSystemTest, BulletDamagesEnemy)
{
    auto player = Game::EntityFactory::CreatePlayer(m_Scene, { 0.0f, 0.0f });
    auto enemy = Game::EntityFactory::CreateEnemy(m_Scene, { 1.0f, 0.0f }, 
                                                  Game::EnemyType::Chaser);
    
    auto& enemyHealth = enemy.GetComponent<Pillar::HealthComponent>();
    float initialHealth = enemyHealth.CurrentHealth;
    
    // Spawn bullet aimed at enemy
    auto bullet = Game::EntityFactory::CreateBullet(
        m_Scene, player, { 0.1f, 0.0f }, { 1.0f, 0.0f }, 20.0f, 10.0f
    );
    
    // ... simulate bullet collision ...
    
    EXPECT_LT(enemyHealth.CurrentHealth, initialHealth);
}
```

### 7.3 Performance Benchmarks

```cpp
// In Tests/src/ShooterPerfTests.cpp
#include <gtest/gtest.h>
#include <chrono>

TEST(PerformanceTest, SpawnManyEnemies)
{
    Pillar::Scene scene;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 500; ++i)
    {
        float x = (i % 50) * 1.0f - 25.0f;
        float y = (i / 50) * 1.0f - 5.0f;
        Game::EntityFactory::CreateEnemy(scene, { x, y }, Game::EnemyType::Swarm);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);  // Should complete in < 100ms
    EXPECT_EQ(scene.GetEntityCount(), 500);
}

TEST(PerformanceTest, UpdateManyEntities)
{
    Pillar::Scene scene;
    
    // Spawn 200 enemies
    for (int i = 0; i < 200; ++i)
    {
        Game::EntityFactory::CreateEnemy(scene, { 0.0f, 0.0f }, Game::EnemyType::Swarm);
    }
    
    Game::EnemyAISystem aiSystem;
    aiSystem.OnAttach(&scene);
    
    // Measure update time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int frame = 0; frame < 60; ++frame)
    {
        aiSystem.OnUpdate(1.0f / 60.0f);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 60 frames should take ~1 second in real-time, but processing should be < 500ms
    EXPECT_LT(duration.count(), 500);
}
```

---

## Appendix A: Asset Requirements

### Directory Structure

```
Sandbox/
└── assets/
    ├── textures/
    │   ├── player.png          # 64x64 character sprite
    │   ├── enemy_base.png      # 64x64 enemy sprite (tinted per type)
    │   ├── bullet.png          # 16x8 bullet sprite
    │   ├── wall.png            # 64x64 tileable wall texture
    │   ├── powerup_health.png  # 32x32 health pickup
    │   ├── powerup_speed.png   # 32x32 speed pickup
    │   ├── powerup_damage.png  # 32x32 damage pickup
    │   └── particle.png        # 8x8 particle texture (optional)
    │
    ├── audio/
    │   ├── sfx/
    │   │   ├── gunshot.wav     # 8-bit/16-bit mono, < 0.5s
    │   │   ├── enemy_hit.wav   # Impact sound
    │   │   ├── enemy_death.wav # Death sound
    │   │   ├── player_hit.wav  # Player damage sound
    │   │   ├── powerup.wav     # Pickup chime
    │   │   └── wave_start.wav  # Wave announcement
    │   │
    │   └── music/
    │       └── gameplay_loop.wav  # Background music (loop-ready)
    │
    └── animations/
        └── player.anim.json    # Animation definitions (optional)
```

### Asset Specifications

| Asset Type | Format | Size | Notes |
|------------|--------|------|-------|
| Sprites | PNG | 32x32 to 128x128 | RGBA, transparent background |
| Particles | PNG | 8x8 to 16x16 | Simple shapes, alpha gradient |
| Sound Effects | WAV | < 1 second | 16-bit PCM, mono, 44.1kHz |
| Music | WAV | Loop-ready | 16-bit PCM, stereo, 44.1kHz |

**Note:** stb_image supports PNG, JPG, BMP, TGA. For best quality with transparency, use PNG. OpenAL-Soft requires WAV format (8-bit or 16-bit PCM).

---

## Appendix B: Build Instructions

### Prerequisites

- Visual Studio 2022 with C++ development tools
- CMake 3.21+
- Ninja 1.12.1+
- Python 3.x with jinja2 (`pip install jinja2`)

### Build Steps

```powershell
# 1. Bootstrap (first time only)
.\scripts\bootstrap.ps1

# 2. Configure
cmake --preset windows-debug

# 3. Build
cmake --build --preset windows-debug

# 4. Run
.\bin\Debug-x64\Sandbox\SandboxApp.exe

# 5. Run tests
ctest --preset windows-debug
```

### Release Build

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
.\bin\Release-x64\Sandbox\SandboxApp.exe
```

### Adding Shooter Files to Build

Edit `Sandbox/CMakeLists.txt`:

```cmake
add_executable(Sandbox
    src/Source.cpp
    src/GameLayer.cpp
    src/EntityFactory.cpp
    src/WaveManager.cpp
    src/GameCamera.cpp
    src/Systems/PlayerMovementSystem.cpp
    src/Systems/WeaponSystem.cpp
    src/Systems/EnemyAISystem.cpp
    src/Systems/DamageSystem.cpp
    src/Systems/FlashSystem.cpp
    # Add more as needed
)
```

### Troubleshooting

**Issue: Textures not loading**
- Verify files exist in `Sandbox/assets/textures/`
- Check file extension is lowercase `.png`
- Use `AssetManager::GetTexturePath()` for path resolution

**Issue: Audio not playing**
- Verify WAV files are valid (16-bit PCM, mono or stereo)
- Check `AudioEngine::IsInitialized()` returns true
- Ensure volume is not 0 (check master and bus volumes)

**Issue: Physics bodies not moving**
- Verify `PhysicsSystem::OnUpdate()` is called every frame
- Check body type is `b2_dynamicBody` for moving entities
- Ensure gravity is set to zero for top-down (default is -9.81)

**Issue: Bullets not hitting enemies**
- Verify collision categories and masks are configured correctly
- Check `BulletCollisionSystem` is attached and updated
- Ensure bullets have `VelocityComponent` with non-zero velocity

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-27 | Lead Systems Architect | Initial TDD release |

---

*This document is the canonical reference for Top-Down Shooter development on Pillar Engine. For engine-specific questions, refer to [.github/copilot-instructions.md](../.github/copilot-instructions.md) and [docs/API_REFERENCE.md](API_REFERENCE.md).*
