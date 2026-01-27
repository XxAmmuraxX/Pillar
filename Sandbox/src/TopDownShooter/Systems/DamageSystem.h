#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Systems/BulletCollisionSystem.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/Logger.h>

#include "../Components/EnemyComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/PowerUpComponent.h"
#include "../Utilities/EntityFactory.h"

#include <vector>
#include <random>
#include <functional>

namespace Game {

    /**
     * DamageSystem - Handles health updates, invulnerability, and death processing
     * Now uses BulletCollisionSystem callbacks instead of manual collision checks
     */
    class DamageSystem : public Pillar::System
    {
    public:
        using OnEnemyHitCallback = std::function<void(const glm::vec2& position, const glm::vec2& bulletDirection)>;
        using OnEnemyKilledCallback = std::function<void(const glm::vec2& position, const glm::vec4& color)>;
        using OnPlayerHitCallback = std::function<void()>;
        using OnGameOverCallback = std::function<void()>;

        DamageSystem(Pillar::BulletCollisionSystem* bulletCollisionSystem)
            : m_BulletCollisionSystem(bulletCollisionSystem)
        {
            // Hook into bullet collision system
            if (m_BulletCollisionSystem)
            {
                m_BulletCollisionSystem->SetOnBulletHit(
                    [this](Pillar::Entity bullet, Pillar::Entity target, float damage, const glm::vec2& hitPos) {
                        OnBulletHit(bullet, target, damage, hitPos);
                    }
                );
            }
        }

        void SetOnEnemyHit(OnEnemyHitCallback callback) { m_OnEnemyHit = callback; }
        void SetOnEnemyKilled(OnEnemyKilledCallback callback) { m_OnEnemyKilled = callback; }
        void SetOnPlayerHit(OnPlayerHitCallback callback) { m_OnPlayerHit = callback; }
        void SetOnGameOver(OnGameOverCallback callback) { m_OnGameOver = callback; }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            // 1. Update invulnerability timers
            UpdateInvulnerabilityTimers(dt);

            // 2. Handle deaths
            ProcessDeaths();
        }

    private:
        Pillar::BulletCollisionSystem* m_BulletCollisionSystem = nullptr;

        OnEnemyHitCallback m_OnEnemyHit;
        OnEnemyKilledCallback m_OnEnemyKilled;
        OnPlayerHitCallback m_OnPlayerHit;
        OnGameOverCallback m_OnGameOver;

        void OnBulletHit(Pillar::Entity bullet, Pillar::Entity target, float damage, const glm::vec2& hitPos)
        {
            // Apply damage to target
            if (auto* health = target.TryGetComponent<Pillar::HealthComponent>())
            {
                float damageDealt = health->TakeDamage(damage);

                if (damageDealt > 0.0f)
                {
                    // Check if target is an enemy
                    if (target.HasComponent<EnemyComponent>())
                    {
                        PIL_INFO("Bullet hit enemy! Damage: {}, Enemy Health: {}/{}",
                            damageDealt, health->CurrentHealth, health->MaxHealth);

                        // Trigger hit effect callback
                        if (m_OnEnemyHit)
                        {
                            // Get bullet direction from velocity if available
                            glm::vec2 bulletDir(1.0f, 0.0f);
                            if (auto* vel = bullet.TryGetComponent<Pillar::VelocityComponent>())
                            {
                                if (glm::length(vel->Velocity) > 0.01f)
                                    bulletDir = glm::normalize(vel->Velocity);
                            }
                            m_OnEnemyHit(hitPos, bulletDir);
                        }
                    }
                    // Check if target is player
                    else if (target.HasComponent<PlayerTagComponent>())
                    {
                        PIL_WARN("Player hit! Damage: {}, Player Health: {}/{}",
                            damageDealt, health->CurrentHealth, health->MaxHealth);

                        // Trigger player hit callback
                        if (m_OnPlayerHit)
                        {
                            m_OnPlayerHit();
                        }
                    }
                }
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
