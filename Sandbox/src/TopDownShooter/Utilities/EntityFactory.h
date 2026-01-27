#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Physics/ColliderComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <box2d/b2_body.h>

#include "../Components/PlayerTagComponent.h"
#include "../Components/WeaponComponent.h"
#include "../Components/EnemyComponent.h"
#include "../Components/PowerUpComponent.h"
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
            transform.SetScale(glm::vec2(1.0f, 1.0f));

            // Sprite - Use colored quad for now (no texture)
            auto& sprite = player.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(1.0f, 1.0f);
            sprite.Color = glm::vec4(0.3f, 0.7f, 1.0f, 1.0f);  // Light blue
            sprite.Layer = "Player";
            sprite.OrderInLayer = 10;

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

            // Player tag
            player.AddComponent<PlayerTagComponent>();

            return player;
        }

        // Create wall entity
        static Pillar::Entity CreateWall(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& size)
        {
            auto wall = scene.CreateEntity("Wall");

            // Transform
            auto& transform = wall.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite
            auto& sprite = wall.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = size;
            sprite.Color = glm::vec4(0.3f, 0.3f, 0.3f, 1.0f);  // Dark gray
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

        // Create enemy entity
        static Pillar::Entity CreateEnemy(
            Pillar::Scene& scene,
            const glm::vec2& position,
            EnemyType type)
        {
            auto enemy = scene.CreateEntity("Enemy");

            // Transform
            auto& transform = enemy.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Sprite with color variation based on type
            auto& sprite = enemy.AddComponent<Pillar::SpriteComponent>();
            sprite.Layer = "Enemies";
            sprite.OrderInLayer = 5;

            // Color-code enemy types
            switch (type)
            {
                case EnemyType::Chaser:
                    sprite.Color = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
                    sprite.Size = glm::vec2(0.8f, 0.8f);
                    break;
                case EnemyType::Shooter:
                    sprite.Color = glm::vec4(0.3f, 0.3f, 1.0f, 1.0f);  // Blue
                    sprite.Size = glm::vec2(1.0f, 1.0f);
                    break;
                case EnemyType::Swarm:
                    sprite.Color = glm::vec4(0.3f, 1.0f, 0.3f, 1.0f);  // Green
                    sprite.Size = glm::vec2(0.5f, 0.5f);
                    break;
                default:
                    sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    sprite.Size = glm::vec2(1.0f, 1.0f);
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

            // Sprite with color based on type
            auto& sprite = powerUp.AddComponent<Pillar::SpriteComponent>();
            float size = PowerUpComponent::GetSizeForType(type);
            sprite.Size = glm::vec2(size, size);
            sprite.Color = PowerUpComponent::GetColorForType(type);
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
    };

} // namespace Game
