#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/Renderer/Renderer2D.h>
#include <box2d/b2_body.h>
#include <glm/glm.hpp>
#include <functional>

#include "../Components/BossComponent.h"
#include "../Components/EnemyComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Utilities/AudioManager.h"
#include "../Core/GameState.h"

namespace Game {

    /**
     * BossSystem - Handles boss AI, attacks, phase transitions, and special abilities
     */
    class BossSystem : public Pillar::System
    {
    public:
        using OnBossDefeatedCallback = std::function<void(const glm::vec2& position, int xpReward, int scoreReward)>;
        using OnSpawnMinionCallback = std::function<void(const glm::vec2& position, EnemyType type)>;

        void SetOnBossDefeated(OnBossDefeatedCallback callback) { m_OnBossDefeated = callback; }
        void SetOnSpawnMinion(OnSpawnMinionCallback callback) { m_OnSpawnMinion = callback; }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();

            // Find player
            glm::vec2 playerPos(0.0f);
            auto playerView = registry.view<Pillar::TransformComponent, PlayerTagComponent>();
            for (auto entity : playerView)
            {
                playerPos = playerView.get<Pillar::TransformComponent>(entity).Position;
                break;
            }

            // Update all bosses
            auto bossView = registry.view<
                Pillar::TransformComponent,
                Pillar::HealthComponent,
                BossComponent
            >();

            for (auto entity : bossView)
            {
                auto& transform = bossView.get<Pillar::TransformComponent>(entity);
                auto& health = bossView.get<Pillar::HealthComponent>(entity);
                auto& boss = bossView.get<BossComponent>(entity);

                Pillar::Entity bossEntity(entity, m_Scene);

                // Check for death
                if (health.IsDead)
                {
                    HandleBossDeath(bossEntity, transform, boss);
                    continue;
                }

                // Update phase based on health
                float healthPercent = health.CurrentHealth / health.MaxHealth;
                boss.UpdatePhase(healthPercent);

                // Update attack timer
                if (boss.AttackTimer > 0.0f)
                    boss.AttackTimer -= dt;

                // Update special timer
                if (boss.SpecialTimer > 0.0f)
                    boss.SpecialTimer -= dt;

                // Calculate direction to player
                glm::vec2 toPlayer = playerPos - transform.Position;
                float distance = glm::length(toPlayer);
                if (distance > 0.001f)
                    toPlayer = glm::normalize(toPlayer);

                // Boss-specific AI
                switch (boss.Type)
                {
                    case BossType::Behemoth:
                        UpdateBehemoth(bossEntity, boss, transform, toPlayer, distance, dt);
                        break;
                    case BossType::Swarm_Queen:
                        UpdateSwarmQueen(bossEntity, boss, transform, toPlayer, distance, dt);
                        break;
                    case BossType::Devastator:
                        UpdateDevastator(bossEntity, boss, transform, toPlayer, distance, dt);
                        break;
                }

                // Update sprite facing
                if (auto* sprite = bossEntity.TryGetComponent<Pillar::SpriteComponent>())
                {
                    sprite->FlipX = toPlayer.x < 0.0f;

                    // Flash red in later phases
                    if (boss.Phase == BossPhase::Phase3)
                    {
                        float flash = 0.7f + 0.3f * std::sin(dt * 10.0f);
                        sprite->Color = glm::vec4(1.0f, flash * 0.5f, flash * 0.5f, 1.0f);
                    }
                    else if (boss.Phase == BossPhase::Phase2)
                    {
                        sprite->Color = glm::vec4(1.0f, 0.8f, 0.8f, 1.0f);
                    }
                }
            }
        }

        // Render boss health bars (called from GameLayer after main render)
        void RenderBossHealthBars()
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();

            auto bossView = registry.view<
                Pillar::TransformComponent,
                Pillar::HealthComponent,
                BossComponent
            >();

            for (auto entity : bossView)
            {
                auto& transform = bossView.get<Pillar::TransformComponent>(entity);
                auto& health = bossView.get<Pillar::HealthComponent>(entity);
                auto& boss = bossView.get<BossComponent>(entity);

                if (!boss.ShowHealthBar || health.IsDead) continue;

                // Draw health bar above boss
                float barWidth = boss.HealthBarWidth;
                float barHeight = 0.15f;
                float healthPercent = health.CurrentHealth / health.MaxHealth;
                
                glm::vec2 barPos = transform.Position + glm::vec2(0.0f, 1.5f);

                // Background (dark red)
                Pillar::Renderer2D::DrawQuad(
                    glm::vec3(barPos, 0.5f),
                    glm::vec2(barWidth, barHeight),
                    glm::vec4(0.3f, 0.1f, 0.1f, 0.8f)
                );

                // Health (bright red)
                float healthWidth = barWidth * healthPercent;
                glm::vec2 healthBarPos = barPos - glm::vec2((barWidth - healthWidth) * 0.5f, 0.0f);
                Pillar::Renderer2D::DrawQuad(
                    glm::vec3(healthBarPos, 0.51f),
                    glm::vec2(healthWidth, barHeight * 0.8f),
                    GetHealthBarColor(boss.Phase)
                );
            }
        }

    private:
        OnBossDefeatedCallback m_OnBossDefeated;
        OnSpawnMinionCallback m_OnSpawnMinion;

        glm::vec4 GetHealthBarColor(BossPhase phase)
        {
            switch (phase)
            {
                case BossPhase::Phase3: return glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);   // Bright red
                case BossPhase::Phase2: return glm::vec4(1.0f, 0.6f, 0.2f, 1.0f);   // Orange
                default: return glm::vec4(0.8f, 0.2f, 0.2f, 1.0f);                   // Dark red
            }
        }

        void HandleBossDeath(Pillar::Entity& boss, Pillar::TransformComponent& transform, BossComponent& bossComp)
        {
            if (m_OnBossDefeated)
            {
                m_OnBossDefeated(transform.Position, bossComp.XPReward, bossComp.ScoreReward);
            }

            // Update game stats
            GameState::Instance().GetStats().BossesKilled++;

            // Play death sound
            AudioManager::Instance().PlaySound("death", transform.Position, 1.0f, 0.6f);

            // Destroy boss
            m_Scene->DestroyEntity(boss);
        }

        void UpdateBehemoth(
            Pillar::Entity entity,
            BossComponent& boss,
            Pillar::TransformComponent& transform,
            const glm::vec2& toPlayer,
            float distance,
            float dt)
        {
            float attackRange = 1.5f;

            // Move toward player
            if (distance > attackRange)
            {
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(toPlayer.x * boss.MoveSpeed, toPlayer.y * boss.MoveSpeed);
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }
            else
            {
                // Attack!
                if (boss.AttackTimer <= 0.0f)
                {
                    // Deal damage to player
                    DamagePlayer(boss.AttackDamage);
                    boss.AttackTimer = boss.AttackCooldown;
                    AudioManager::Instance().PlaySound("hit", transform.Position, 0.8f, 0.7f);
                }
            }
        }

        void UpdateSwarmQueen(
            Pillar::Entity entity,
            BossComponent& boss,
            Pillar::TransformComponent& transform,
            const glm::vec2& toPlayer,
            float distance,
            float dt)
        {
            float preferredDistance = 6.0f;

            // Maintain distance
            if (distance < preferredDistance - 1.0f)
            {
                // Move away
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(-toPlayer.x * boss.MoveSpeed, -toPlayer.y * boss.MoveSpeed);
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }
            else if (distance > preferredDistance + 2.0f)
            {
                // Move closer
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(toPlayer.x * boss.MoveSpeed, toPlayer.y * boss.MoveSpeed);
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }

            // Spawn minions periodically
            if (boss.SpecialTimer <= 0.0f && m_OnSpawnMinion)
            {
                // Spawn 2-4 swarmers around the queen
                int count = 2 + (boss.Phase == BossPhase::Phase3 ? 2 : (boss.Phase == BossPhase::Phase2 ? 1 : 0));
                for (int i = 0; i < count; i++)
                {
                    float angle = (static_cast<float>(i) / count) * 6.28318f;
                    glm::vec2 spawnOffset(std::cos(angle) * 1.5f, std::sin(angle) * 1.5f);
                    m_OnSpawnMinion(transform.Position + spawnOffset, EnemyType::Swarm);
                }
                boss.SpecialTimer = boss.SpecialCooldown;
                AudioManager::Instance().PlaySound("enemy_shoot", transform.Position, 0.6f, 0.8f);
            }
        }

        void UpdateDevastator(
            Pillar::Entity entity,
            BossComponent& boss,
            Pillar::TransformComponent& transform,
            const glm::vec2& toPlayer,
            float distance,
            float dt)
        {
            float preferredDistance = 8.0f;

            // Maintain distance
            if (distance < preferredDistance - 2.0f)
            {
                // Move away slowly
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(-toPlayer.x * boss.MoveSpeed * 0.5f, -toPlayer.y * boss.MoveSpeed * 0.5f);
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }
            else if (distance > preferredDistance + 3.0f)
            {
                // Move closer
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(toPlayer.x * boss.MoveSpeed, toPlayer.y * boss.MoveSpeed);
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }

            // Ranged attack
            if (boss.AttackTimer <= 0.0f && distance < 15.0f)
            {
                // TODO: Spawn projectile toward player
                // For now, just deal damage at range
                if (distance < 10.0f)
                {
                    DamagePlayer(boss.AttackDamage * 0.5f);  // Reduced damage for ranged
                }
                boss.AttackTimer = boss.AttackCooldown;
                AudioManager::Instance().PlaySound("enemy_shoot", transform.Position, 0.7f, 0.6f);
            }
        }

        void DamagePlayer(float damage)
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            auto playerView = registry.view<Pillar::HealthComponent, PlayerTagComponent>();

            for (auto entity : playerView)
            {
                auto& health = playerView.get<Pillar::HealthComponent>(entity);
                health.TakeDamage(damage);
                break;
            }
        }
    };

} // namespace Game
