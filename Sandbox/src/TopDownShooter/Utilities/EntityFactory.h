#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Rendering/AnimationComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Physics/ColliderComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h>
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
#include <Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h>
#include <Pillar/Renderer/Texture.h>
#include <Pillar/Utils/AssetManager.h>
#include <box2d/b2_body.h>

#include "../Components/PlayerTagComponent.h"
#include "../Components/WeaponComponent.h"
#include "../Components/EnemyComponent.h"
#include "../Components/PowerUpComponent.h"
#include "../Components/XPOrbComponent.h"
#include "../Components/BossComponent.h"
#include "../Components/HazardComponent.h"
#include "CollisionCategories.h"

#include <Pillar/ECS/Components/Physics/VelocityComponent.h>

namespace Game {

    class EntityFactory
    {
    public:
        // Create player entity at specified position
        static Pillar::Entity CreatePlayer(Pillar::Scene& scene, const glm::vec2& position)
        {
            auto player = scene.CreateEntity("Player");

            // Transform
            auto& transform = player.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);
            transform.SetScale(glm::vec2(1.5f, 1.5f));  // Larger player for visibility

            // Sprite - Red mage player character
            auto& sprite = player.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(1.5f, 1.5f);  // Larger player for visibility
            sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("red_mage_wearing_a_hood/rotations/south.png"));
            sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White tint (no color change)
            sprite.Layer = "Player";
            sprite.OrderInLayer = 10;

            // Physics - Dynamic body with fixed rotation
            auto& rb = player.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
            rb.FixedRotation = true;
            rb.LinearDamping = 8.0f;  // Snappy stop when releasing movement keys

            // Collider - Circle for smooth movement around obstacles (scaled with player)
            auto collider = Pillar::ColliderComponent::Circle(0.6f);
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

            // Player tag
            player.AddComponent<PlayerTagComponent>();

            // Animation - start with red mage run animation
            auto& anim = player.AddComponent<Pillar::AnimationComponent>();
            anim.Play("red_mage_idle_south");

            // Light - player casts a blue-white pool of light
            auto& light = player.AddComponent<Pillar::Light2DComponent>();
            light.Type = Pillar::Light2DType::Point;
            light.Color = glm::vec3(0.6f, 0.7f, 1.0f);
            light.Intensity = 1.2f;
            light.Radius = 6.0f;
            light.CastShadows = true;

            return player;
        }

        // Create wall entity - SCRAPYARD SALVATION industrial style
        static Pillar::Entity CreateWall(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& size)
        {
            auto wall = scene.CreateEntity("Wall");

            // Transform
            auto& transform = wall.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite - Industrial gray with rust tint (#1A1A1E with rust hint)
            auto& sprite = wall.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = size;
            sprite.Color = glm::vec4(0.15f, 0.12f, 0.10f, 1.0f);  // Dark industrial gray-brown
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

            // Shadow caster - walls cast shadows
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

            return wall;
        }

        // Create enemy entity with wave-based difficulty scaling
        static Pillar::Entity CreateEnemy(
            Pillar::Scene& scene,
            const glm::vec2& position,
            EnemyType type,
            int waveNumber = 1)
        {
            auto enemy = scene.CreateEntity("Enemy");

            // Difficulty scaling factors per wave
            float healthScale = 1.0f + (waveNumber - 1) * 0.05f;   // +5% per wave
            float speedScale = 1.0f + (waveNumber - 1) * 0.02f;    // +2% per wave

            // Transform
            auto& transform = enemy.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite with color variation based on type
            auto& sprite = enemy.AddComponent<Pillar::SpriteComponent>();
            sprite.Layer = "Enemies";
            sprite.OrderInLayer = 5;

            // Load textures and configure size/animation based on enemy type
            // Sizes increased for better visibility
            switch (type)
            {
                case EnemyType::Chaser:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("hoodzy.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    sprite.Size = glm::vec2(1.8f, 1.8f);  // Increased from 1.0
                    break;
                case EnemyType::Shooter:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("evil_archer/rotations/south.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    sprite.Size = glm::vec2(1.8f, 1.8f);  // Increased from 1.0
                    break;
                case EnemyType::Swarm:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("goblin_with_a_sword/rotations/south.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    sprite.Size = glm::vec2(1.4f, 1.4f);  // Increased from 0.8
                    break;
                default:
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    sprite.Size = glm::vec2(1.8f, 1.8f);  // Increased from 1.0
            }

            // Add animations where available
            if (type == EnemyType::Chaser)
            {
                auto& anim = enemy.AddComponent<Pillar::AnimationComponent>();
                anim.Play("hoodzy_chaser_enemy_animation");
            }
            else if (type == EnemyType::Shooter)
            {
                auto& anim = enemy.AddComponent<Pillar::AnimationComponent>();
                anim.Play("evil_archer_run_south");
            }
            else if (type == EnemyType::Swarm)
            {
                auto& anim = enemy.AddComponent<Pillar::AnimationComponent>();
                anim.Play("goblin_with_sword_run_south");
            }

            // Physics setup depends on enemy type
            // Hitbox sizes increased proportionally with sprite sizes
            if (type == EnemyType::Swarm)
            {
                // Lightweight enemies use VelocityComponent (no Box2D overhead)
                auto& velocity = enemy.AddComponent<Pillar::VelocityComponent>();
                velocity.Drag = 2.0f;
                velocity.MaxSpeed = 8.0f * speedScale;
            }
            else
            {
                // Heavy enemies use Box2D
                auto& rb = enemy.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
                rb.FixedRotation = true;
                rb.LinearDamping = 4.0f;

                auto collider = Pillar::ColliderComponent::Circle(sprite.Size.x * 0.35f);  // Hitbox proportional to new size
                collider.CategoryBits = CollisionCategory::Enemy;
                collider.MaskBits = CollisionCategory::Player |
                                   CollisionCategory::Wall |
                                   CollisionCategory::Bullet;
                enemy.AddComponent<Pillar::ColliderComponent>(collider);
            }

            // Health scaled by type and wave
            float baseHealth = 30.0f;
            switch (type)
            {
                case EnemyType::Shooter: baseHealth = 50.0f; break;
                case EnemyType::Swarm: baseHealth = 10.0f; break;
                default: break;
            }
            auto& health = enemy.AddComponent<Pillar::HealthComponent>(baseHealth * healthScale);
            health.DestroyOnDeath = false;  // Handle death effects first

            // Enemy behavior
            auto& enemyComp = enemy.AddComponent<EnemyComponent>();
            enemyComp.Type = type;
            enemyComp.State = EnemyState::Idle;

            // Configure stats by type with wave scaling
            switch (type)
            {
                case EnemyType::Chaser:
                    enemyComp.MoveSpeed = 4.0f * speedScale;
                    enemyComp.AttackDamage = 15.0f;
                    enemyComp.XPValue = 10.0f;
                    break;
                case EnemyType::Shooter:
                    enemyComp.MoveSpeed = 2.0f * speedScale;
                    enemyComp.AttackDamage = 8.0f;
                    enemyComp.AttackRange = 10.0f;
                    enemyComp.XPValue = 25.0f;
                    break;
                case EnemyType::Swarm:
                    enemyComp.MoveSpeed = 6.0f * speedScale;
                    enemyComp.AttackDamage = 5.0f;
                    enemyComp.XPValue = 5.0f;
                    break;
            }

            return enemy;
        }

        // Create power-up entity
        static Pillar::Entity CreatePowerUp(
            Pillar::Scene& scene,
            const glm::vec2& position,
            PowerUpType type)
        {
            auto powerUp = scene.CreateEntity("PowerUp");

            // Transform
            auto& transform = powerUp.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite with color based on type - SCRAPYARD SALVATION themed
            auto& sprite = powerUp.AddComponent<Pillar::SpriteComponent>();
            float size = PowerUpComponent::GetSizeForType(type);
            sprite.Size = glm::vec2(size, size);
            
            // Use themed textures for power-ups
            switch (type)
            {
                case PowerUpType::Health:
                    // MED-STIM: Cracked syringe with glowing green fluid
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("powerups/medstim.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint, use texture colors
                    break;
                case PowerUpType::SpeedBoost:
                    // ADRENALINE SHOT: Yellow auto-injector
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("powerups/adrenaline.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint
                    break;
                case PowerUpType::Magnet:
                    // SALVAGE BEACON: Blinking antenna
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("Coins.png"));
                    sprite.Color = glm::vec4(0.6f, 0.2f, 0.8f, 1.0f);  // Toxic purple tint
                    break;
                default:
                    // Other power-ups use solid colors until we have textures
                    sprite.Color = PowerUpComponent::GetColorForType(type);
                    break;
            }
            sprite.Layer = "PowerUps";
            sprite.OrderInLayer = 3;

            // Power-up component
            auto& powerUpComp = powerUp.AddComponent<PowerUpComponent>();
            powerUpComp.Type = type;
            powerUpComp.OriginalPosition = position;

            // Configure based on type
            switch (type)
            {
                case PowerUpType::Health:
                    powerUpComp.Value = 25.0f;
                    powerUpComp.Duration = 0.0f;  // Instant effect
                    break;
                case PowerUpType::SpeedBoost:
                    powerUpComp.Value = 1.5f;     // 50% speed increase
                    powerUpComp.Duration = 5.0f;
                    break;
                case PowerUpType::FireRateUp:
                    powerUpComp.Value = 2.0f;     // Double fire rate
                    powerUpComp.Duration = 5.0f;
                    break;
                case PowerUpType::DamageUp:
                    powerUpComp.Value = 2.0f;     // Double damage
                    powerUpComp.Duration = 5.0f;
                    break;
                case PowerUpType::Shield:
                    powerUpComp.Value = 1.0f;
                    powerUpComp.Duration = 3.0f;
                    break;
                case PowerUpType::Magnet:
                    powerUpComp.Value = 5.0f;     // Magnet radius
                    powerUpComp.Duration = 10.0f;
                    break;
            }

            // Light - power-ups glow with their respective colors
            auto& light = powerUp.AddComponent<Pillar::Light2DComponent>();
            glm::vec4 c = sprite.Color;
            light.Color = glm::vec3(c.r, c.g, c.b);
            light.Intensity = 0.5f;
            light.Radius = 1.5f;
            light.CastShadows = false;

            return powerUp;
        }

        // Helper to spawn random power-up
        static Pillar::Entity CreateRandomPowerUp(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            // Weighted random selection (Health is most common)
            static const PowerUpType types[] = {
                PowerUpType::Health,
                PowerUpType::Health,
                PowerUpType::Health,
                PowerUpType::SpeedBoost,
                PowerUpType::FireRateUp,
                PowerUpType::DamageUp,
                PowerUpType::Shield
            };
            
            int index = rand() % (sizeof(types) / sizeof(types[0]));
            return CreatePowerUp(scene, position, types[index]);
        }

        // Create XP orb that can be collected by player
        static Pillar::Entity CreateXPOrb(
            Pillar::Scene& scene,
            const glm::vec2& position,
            int xpValue)
        {
            auto orb = scene.CreateEntity("XPOrb");

            // Transform
            auto& transform = orb.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite - size and color based on XP value
            auto& sprite = orb.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(XPOrbComponent::GetSizeForValue(xpValue));
            sprite.Color = XPOrbComponent::GetColorForValue(xpValue);
            sprite.Layer = "Effects";
            sprite.OrderInLayer = 2;

            // XP component
            auto& xpComp = orb.AddComponent<XPOrbComponent>();
            xpComp.XPValue = xpValue;
            xpComp.OriginalPosition = position;

            // Light - XP orbs emit a subtle cyan glow
            auto& light = orb.AddComponent<Pillar::Light2DComponent>();
            light.Color = glm::vec3(0.3f, 0.8f, 1.0f);
            light.Intensity = 0.3f;
            light.Radius = 1.0f;
            light.CastShadows = false;

            return orb;
        }

        // Create Boss entity
        static Pillar::Entity CreateBoss(
            Pillar::Scene& scene,
            const glm::vec2& position,
            BossType type,
            int waveNumber)
        {
            auto boss = scene.CreateEntity("Boss");

            // Transform
            auto& transform = boss.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Create boss component based on type
            BossComponent bossComp;
            switch (type)
            {
                case BossType::Behemoth:
                    bossComp = BossComponent::CreateBehemoth(waveNumber);
                    break;
                case BossType::Swarm_Queen:
                    bossComp = BossComponent::CreateSwarmQueen(waveNumber);
                    break;
                case BossType::Devastator:
                    bossComp = BossComponent::CreateDevastator(waveNumber);
                    break;
            }
            boss.AddComponent<BossComponent>(bossComp);

            // Sprite - large, menacing
            auto& sprite = boss.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(2.5f, 2.5f);  // Much bigger than regular enemies
            
            // Use existing textures with tinting for now
            switch (type)
            {
                case BossType::Behemoth:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("Large_Behemoth/rotations/south.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint needed
                    break;
                case BossType::Swarm_Queen:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("goblin_queen_with_a_staff/rotations/south.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint needed
                    break;
                case BossType::Devastator:
                    sprite.Texture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("monster_with_a_bow/rotations/south.png"));
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // No tint needed
                    break;
            }
            sprite.Layer = "Bosses";
            sprite.OrderInLayer = 20;

            // Add boss animations
            auto& anim = boss.AddComponent<Pillar::AnimationComponent>();
            switch (type)
            {
                case BossType::Behemoth:
                    anim.Play("large_behemoth_walk_south");
                    break;
                case BossType::Swarm_Queen:
                    anim.Play("goblin_queen_walk_south");
                    break;
                case BossType::Devastator:
                    anim.Play("monster_with_bow_run_south");
                    break;
            }

            // Physics - Dynamic body, heavy
            auto& rb = boss.AddComponent<Pillar::RigidbodyComponent>(b2_dynamicBody);
            rb.FixedRotation = true;
            rb.LinearDamping = 3.0f;

            // Large collider
            auto collider = Pillar::ColliderComponent::Circle(1.0f);
            collider.CategoryBits = CollisionCategory::Enemy;
            collider.MaskBits = CollisionCategory::Player |
                               CollisionCategory::Wall |
                               CollisionCategory::Bullet;
            boss.AddComponent<Pillar::ColliderComponent>(collider);

            // Health - use boss's max health
            auto& health = boss.AddComponent<Pillar::HealthComponent>(bossComp.MaxHealth);
            health.DestroyOnDeath = false;  // Handle death effects first

            // Also add enemy component for compatibility with damage system
            auto& enemy = boss.AddComponent<EnemyComponent>();
            enemy.Type = EnemyType::Chaser;  // Base type for collision
            enemy.MoveSpeed = bossComp.MoveSpeed;
            enemy.AttackDamage = bossComp.AttackDamage;
            enemy.XPValue = static_cast<float>(bossComp.XPReward);

            return boss;
        }

        // Helper to get random boss type
        static BossType GetRandomBossType()
        {
            static const BossType types[] = {
                BossType::Behemoth,
                BossType::Swarm_Queen,
                BossType::Devastator
            };
            return types[rand() % 3];
        }

        // ===========================================
        // ENVIRONMENTAL HAZARDS
        // ===========================================

        /**
         * Create an explosive barrel that explodes when shot
         */
        static Pillar::Entity CreateExplosiveBarrel(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            auto barrel = scene.CreateEntity("ExplosiveBarrel");

            // Transform
            auto& transform = barrel.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite - red barrel
            auto& sprite = barrel.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.8f, 1.0f);
            sprite.Color = glm::vec4(0.8f, 0.2f, 0.1f, 1.0f);  // Red
            sprite.Layer = "Environment";
            sprite.OrderInLayer = 0;

            // Hazard component
            auto& hazard = barrel.AddComponent<HazardComponent>(HazardType::ExplosiveBarrel);

            // Particle emitter - subtle orange smoke/embers
            auto& emitter = barrel.AddComponent<Pillar::ParticleEmitterComponent>();
            emitter.EmissionRate = 3.0f;
            emitter.Shape = Pillar::EmissionShape::Circle;
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

            // Light - barrels have a flickering orange glow
            auto& light = barrel.AddComponent<Pillar::Light2DComponent>();
            light.Color = glm::vec3(1.0f, 0.5f, 0.15f);
            light.Intensity = 0.6f;
            light.Radius = 2.5f;
            light.CastShadows = false;

            // Static physics body so bullets can detect it
            auto& rb = barrel.AddComponent<Pillar::RigidbodyComponent>(b2_staticBody);

            auto collider = Pillar::ColliderComponent::Box(glm::vec2(0.35f, 0.45f));
            collider.CategoryBits = CollisionCategory::Wall;  // Treated as obstacle
            collider.MaskBits = CollisionCategory::Bullet | CollisionCategory::Player | CollisionCategory::Enemy;
            barrel.AddComponent<Pillar::ColliderComponent>(collider);

            return barrel;
        }

        /**
         * Create a spike trap that periodically damages entities
         */
        static Pillar::Entity CreateSpikeTrap(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            auto trap = scene.CreateEntity("SpikeTrap");

            auto& transform = trap.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = trap.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(1.0f, 1.0f);
            sprite.Color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray metal
            sprite.Layer = "Ground";
            sprite.OrderInLayer = -5;

            trap.AddComponent<HazardComponent>(HazardType::SpikeTrap);

            return trap;
        }

        /**
         * Create a slow field that reduces movement speed
         */
        static Pillar::Entity CreateSlowField(
            Pillar::Scene& scene,
            const glm::vec2& position,
            float radius = 2.0f)
        {
            auto field = scene.CreateEntity("SlowField");

            auto& transform = field.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = field.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(radius * 2.0f, radius * 2.0f);
            sprite.Color = glm::vec4(0.3f, 0.3f, 0.8f, 0.4f);  // Blue translucent
            sprite.Layer = "Ground";
            sprite.OrderInLayer = -5;

            auto& hazard = field.AddComponent<HazardComponent>(HazardType::SlowField);
            hazard.DamageRadius = radius;

            return field;
        }

        /**
         * Create a poison pool that damages over time
         */
        static Pillar::Entity CreatePoisonPool(
            Pillar::Scene& scene,
            const glm::vec2& position,
            float radius = 1.5f,
            float lifetime = -1.0f)
        {
            auto pool = scene.CreateEntity("PoisonPool");

            auto& transform = pool.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = pool.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(radius * 2.0f, radius * 2.0f);
            sprite.Color = glm::vec4(0.2f, 0.7f, 0.2f, 0.5f);  // Green translucent
            sprite.Layer = "Ground";
            sprite.OrderInLayer = -5;

            auto& hazard = pool.AddComponent<HazardComponent>(HazardType::PoisonPool);
            hazard.DamageRadius = radius;
            hazard.Lifetime = lifetime;

            // Particle emitter - green bubbles
            auto& emitter = pool.AddComponent<Pillar::ParticleEmitterComponent>();
            emitter.EmissionRate = 5.0f;
            emitter.Shape = Pillar::EmissionShape::Circle;
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

            // Light - poison pools glow green
            auto& light = pool.AddComponent<Pillar::Light2DComponent>();
            light.Color = glm::vec3(0.2f, 0.9f, 0.2f);
            light.Intensity = 0.5f;
            light.Radius = radius * 1.5f;
            light.CastShadows = false;

            return pool;
        }

        /**
         * Create a damage zone (fire, lava, etc.)
         */
        static Pillar::Entity CreateDamageZone(
            Pillar::Scene& scene,
            const glm::vec2& position,
            float radius = 1.5f,
            float damage = 8.0f)
        {
            auto zone = scene.CreateEntity("DamageZone");

            auto& transform = zone.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = zone.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(radius * 2.0f, radius * 2.0f);
            sprite.Color = glm::vec4(1.0f, 0.4f, 0.1f, 0.6f);  // Orange/fire
            sprite.Layer = "Ground";
            sprite.OrderInLayer = -5;

            auto& hazard = zone.AddComponent<HazardComponent>(HazardType::DamageZone);
            hazard.DamageRadius = radius;
            hazard.Damage = damage;

            // Particle emitter - fire particles
            auto& emitter = zone.AddComponent<Pillar::ParticleEmitterComponent>();
            emitter.EmissionRate = 8.0f;
            emitter.Shape = Pillar::EmissionShape::Circle;
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

            // Light - damage zones glow orange (fire)
            auto& light = zone.AddComponent<Pillar::Light2DComponent>();
            light.Color = glm::vec3(1.0f, 0.4f, 0.1f);
            light.Intensity = 0.7f;
            light.Radius = radius * 1.3f;
            light.CastShadows = false;

            return zone;
        }

        /**
         * Spawn random hazards in the arena for variety
         */
        static void SpawnRandomHazards(
            Pillar::Scene& scene,
            float arenaWidth,
            float arenaHeight,
            int count)
        {
            float halfW = arenaWidth * 0.4f;  // Keep away from edges
            float halfH = arenaHeight * 0.4f;

            for (int i = 0; i < count; ++i)
            {
                float x = (static_cast<float>(rand()) / RAND_MAX) * halfW * 2.0f - halfW;
                float y = (static_cast<float>(rand()) / RAND_MAX) * halfH * 2.0f - halfH;
                glm::vec2 pos(x, y);

                // Random hazard type (weighted towards barrels and spikes)
                int hazardType = rand() % 10;
                if (hazardType < 4)
                {
                    CreateExplosiveBarrel(scene, pos);
                }
                else if (hazardType < 7)
                {
                    CreateSpikeTrap(scene, pos);
                }
                else if (hazardType < 9)
                {
                    CreateSlowField(scene, pos, 1.5f);
                }
                else
                {
                    CreateDamageZone(scene, pos, 1.2f, 5.0f);
                }
            }
        }
    };

} // namespace Game
