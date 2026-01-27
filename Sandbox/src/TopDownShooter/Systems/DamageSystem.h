#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <Pillar/Logger.h>

#include "../Components/EnemyComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/PowerUpComponent.h"
#include "../Utilities/EntityFactory.h"

#include <vector>
#include <random>
#include <functional>

namespace Game {

    class DamageSystem : public Pillar::System
    {
    public:
        using OnEnemyHitCallback = std::function<void(const glm::vec2& position, const glm::vec2& bulletDirection)>;
        using OnEnemyKilledCallback = std::function<void(const glm::vec2& position, const glm::vec4& color)>;
        using OnPlayerHitCallback = std::function<void()>;
        using OnGameOverCallback = std::function<void()>;

        void SetOnEnemyHit(OnEnemyHitCallback callback) { m_OnEnemyHit = callback; }
        void SetOnEnemyKilled(OnEnemyKilledCallback callback) { m_OnEnemyKilled = callback; }
        void SetOnPlayerHit(OnPlayerHitCallback callback) { m_OnPlayerHit = callback; }
        void SetOnGameOver(OnGameOverCallback callback) { m_OnGameOver = callback; }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            // 1. Check bullet-enemy collisions
            ProcessBulletCollisions();

            // 2. Update invulnerability timers
            UpdateInvulnerabilityTimers(dt);

            // 3. Handle deaths
            ProcessDeaths();
        }

    private:
        OnEnemyHitCallback m_OnEnemyHit;
        OnEnemyKilledCallback m_OnEnemyKilled;
        OnPlayerHitCallback m_OnPlayerHit;
        OnGameOverCallback m_OnGameOver;

        void ProcessBulletCollisions()
        {
            auto& registry = m_Scene->GetRegistry();

            // Get all bullets
            auto bulletView = registry.view<
                Pillar::TransformComponent,
                Pillar::BulletComponent
            >();

            // Get all enemies
            auto enemyView = registry.view<
                Pillar::TransformComponent,
                Pillar::SpriteComponent,
                Pillar::HealthComponent,
                EnemyComponent
            >();

            std::vector<entt::entity> bulletsToDestroy;

            // Check each bullet against each enemy
            for (auto bulletEntity : bulletView)
            {
                auto& bulletTransform = bulletView.get<Pillar::TransformComponent>(bulletEntity);
                auto& bulletComp = bulletView.get<Pillar::BulletComponent>(bulletEntity);

                if (bulletComp.HitsRemaining <= 0)
                {
                    bulletsToDestroy.push_back(bulletEntity);
                    continue;
                }

                // Check against all enemies
                for (auto enemyEntity : enemyView)
                {
                    auto& enemyTransform = enemyView.get<Pillar::TransformComponent>(enemyEntity);
                    auto& enemySprite = enemyView.get<Pillar::SpriteComponent>(enemyEntity);
                    auto& enemyHealth = enemyView.get<Pillar::HealthComponent>(enemyEntity);

                    // Simple circle-circle collision
                    float bulletRadius = 0.15f;
                    float enemyRadius = enemySprite.Size.x * 0.4f;
                    float distanceSquared = glm::distance2(bulletTransform.Position, enemyTransform.Position);
                    float radiusSum = bulletRadius + enemyRadius;

                    if (distanceSquared < radiusSum * radiusSum)
                    {
                        // Collision detected!
                        // Apply damage
                        float damageDealt = enemyHealth.TakeDamage(bulletComp.Damage);

                        if (damageDealt > 0.0f)
                        {
                            PIL_INFO("Bullet hit enemy! Damage: {}, Enemy Health: {}/{}",
                                damageDealt, enemyHealth.CurrentHealth, enemyHealth.MaxHealth);

                            // Trigger hit effect callback
                            if (m_OnEnemyHit)
                            {
                                // Get bullet direction from velocity if available
                                glm::vec2 bulletDir(1.0f, 0.0f);
                                Pillar::Entity bulletE(bulletEntity, m_Scene);
                                if (auto* vel = bulletE.TryGetComponent<Pillar::VelocityComponent>())
                                {
                                    if (glm::length(vel->Velocity) > 0.01f)
                                        bulletDir = glm::normalize(vel->Velocity);
                                }
                                m_OnEnemyHit(enemyTransform.Position, bulletDir);
                            }
                        }

                        // Decrement bullet hits
                        bulletComp.HitsRemaining--;

                        if (bulletComp.HitsRemaining <= 0)
                        {
                            bulletsToDestroy.push_back(bulletEntity);
                            break;  // Stop checking this bullet
                        }
                    }
                }
            }

            // Destroy bullets that hit
            for (auto entity : bulletsToDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }
        }

        void UpdateInvulnerabilityTimers(float dt)
        {
            auto& registry = m_Scene->GetRegistry();
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
        }

        void ProcessDeaths()
        {
            auto& registry = m_Scene->GetRegistry();
            std::vector<entt::entity> toDestroy;
            std::vector<glm::vec2> powerUpSpawnPositions;

            auto view = registry.view<Pillar::HealthComponent>();
            for (auto entity : view)
            {
                auto& health = view.get<Pillar::HealthComponent>(entity);

                if (!health.IsDead) continue;

                Pillar::Entity e(entity, m_Scene);

                // Check if it's an enemy
                if (auto* enemy = e.TryGetComponent<EnemyComponent>())
                {
                    auto& transform = e.GetComponent<Pillar::TransformComponent>();
                    auto& sprite = e.GetComponent<Pillar::SpriteComponent>();
                    PIL_INFO("Enemy killed at ({:.1f}, {:.1f})!", transform.Position.x, transform.Position.y);
                    
                    // Trigger death effect callback
                    if (m_OnEnemyKilled)
                    {
                        m_OnEnemyKilled(transform.Position, sprite.Color);
                    }

                    // Chance to drop power-up (30% chance)
                    if (ShouldDropPowerUp())
                    {
                        powerUpSpawnPositions.push_back(transform.Position);
                    }

                    toDestroy.push_back(entity);
                }

                // Check if it's the player
                if (e.HasComponent<PlayerTagComponent>())
                {
                    PIL_WARN("Player died! Game Over");
                    if (m_OnPlayerHit)
                    {
                        m_OnPlayerHit();
                    }
                    if (m_OnGameOver)
                    {
                        m_OnGameOver();
                    }
                }
            }

            // Destroy marked entities first
            for (auto entity : toDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }

            // Spawn power-ups after destroying entities
            for (const auto& pos : powerUpSpawnPositions)
            {
                EntityFactory::CreateRandomPowerUp(*m_Scene, pos);
            }
        }

        bool ShouldDropPowerUp()
        {
            static std::mt19937 rng{ std::random_device{}() };
            static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            return dist(rng) < 0.30f;  // 30% drop chance
        }
    };

} // namespace Game
