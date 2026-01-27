#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <box2d/b2_body.h>

#include "../Components/EnemyComponent.h"
#include "../Components/PlayerTagComponent.h"

namespace Game {

    class EnemyAISystem : public Pillar::System
    {
    public:
        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();

            // Find player entity
            Pillar::Entity playerEntity;
            glm::vec2 playerPos(0.0f);

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

    private:
        void UpdateChaser(
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

        void UpdateShooter(
            Pillar::Entity entity,
            EnemyComponent& enemy,
            Pillar::TransformComponent& transform,
            const glm::vec2& toPlayer,
            float distance,
            float dt)
        {
            // Maintain distance and shoot
            float idealDistance = 8.0f;

            if (distance < idealDistance - 1.0f)
            {
                // Back away
                enemy.State = EnemyState::Chasing;
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        b2Vec2 velocity(
                            -toPlayer.x * enemy.MoveSpeed,
                            -toPlayer.y * enemy.MoveSpeed
                        );
                        rb->Body->SetLinearVelocity(velocity);
                    }
                }
            }
            else if (distance > idealDistance + 2.0f)
            {
                // Move closer
                enemy.State = EnemyState::Chasing;
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
                // In ideal range - stop and shoot
                enemy.State = EnemyState::Attacking;
                if (auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>())
                {
                    if (rb->Body)
                    {
                        rb->Body->SetLinearVelocity(b2Vec2(0.0f, 0.0f));
                    }
                }

                // Fire projectiles at player
                if (enemy.AttackTimer <= 0.0f)
                {
                    FireProjectile(entity, transform, toPlayer);
                    enemy.AttackTimer = enemy.AttackCooldown;
                }
            }
        }

        void UpdateSwarm(
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

        void FireProjectile(
            Pillar::Entity entity,
            const Pillar::TransformComponent& transform,
            const glm::vec2& direction)
        {
            // Create enemy bullet
            auto bullet = m_Scene->CreateEntity("EnemyBullet");

            // Position slightly ahead of enemy
            glm::vec2 spawnPos = transform.Position + direction * 0.6f;

            auto& bulletTransform = bullet.GetComponent<Pillar::TransformComponent>();
            bulletTransform.SetPosition(spawnPos);

            // Rotate to face direction
            float angle = std::atan2(direction.y, direction.x);
            bulletTransform.SetRotation(angle);

            // Sprite (red enemy bullet)
            auto& sprite = bullet.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.25f, 0.12f);
            sprite.Color = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);  // Red
            sprite.Layer = "Projectiles";
            sprite.OrderInLayer = 5;

            // Velocity-based movement
            auto& velocity = bullet.AddComponent<Pillar::VelocityComponent>();
            velocity.Velocity = direction * 12.0f;  // Slower than player bullets
            velocity.MaxSpeed = 15.0f;

            // Bullet component (owned by enemy, damages player)
            auto& bulletComp = bullet.AddComponent<Pillar::BulletComponent>(entity, 10.0f);
            bulletComp.Lifetime = 5.0f;
            bulletComp.Pierce = false;
            bulletComp.MaxHits = 1;
            bulletComp.HitsRemaining = 1;
        }
    };

} // namespace Game
