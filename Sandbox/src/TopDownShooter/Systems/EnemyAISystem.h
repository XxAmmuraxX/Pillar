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
#include <Pillar/ECS/Components/Rendering/AnimationComponent.h>
#include <box2d/b2_body.h>
#include <string>
#include <cmath>

#include "../Components/EnemyComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Utilities/AudioManager.h"

namespace Game {

    class EnemyAISystem : public Pillar::System
    {
    public:
        void SetBulletPool(Pillar::BulletPool* pool) { m_BulletPool = pool; }

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

                // Note: Enemies don't rotate - their sprites are designed to face a specific direction
                // Update animation based on movement direction toward player
                auto entityWrapper = Pillar::Entity(entity, m_Scene);
                std::string facingDir = GetFacingDirection(toPlayer);
                
                // Update directional animation for enemies with AnimationComponent
                if (auto* anim = entityWrapper.TryGetComponent<Pillar::AnimationComponent>())
                {
                    UpdateEnemyAnimation(enemy.Type, *anim, facingDir);
                }
                
                // No longer need FlipX since we have directional sprites
                if (auto* sprite = entityWrapper.TryGetComponent<Pillar::SpriteComponent>())
                {
                    sprite->FlipX = false;
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
        
        // Determine facing direction (north, south, east, west) from a direction vector
        static std::string GetFacingDirection(const glm::vec2& direction)
        {
            if (glm::length(direction) < 0.001f)
                return "south";  // Default to south when no direction
            
            // Get angle in degrees (0 = east, 90 = north, 180/-180 = west, -90 = south)
            float angle = glm::degrees(std::atan2(direction.y, direction.x));
            
            // Determine quadrant based on angle
            if (angle >= -45.0f && angle < 45.0f)
                return "east";
            else if (angle >= 45.0f && angle < 135.0f)
                return "north";
            else if (angle >= 135.0f || angle < -135.0f)
                return "west";
            else // angle >= -135 && angle < -45
                return "south";
        }
        
        // Update enemy animation based on type and facing direction
        void UpdateEnemyAnimation(EnemyType type, Pillar::AnimationComponent& anim, const std::string& facingDir)
        {
            std::string targetAnim;
            
            switch (type)
            {
                case EnemyType::Shooter:
                    targetAnim = "evil_archer_run_" + facingDir;
                    break;
                case EnemyType::Swarm:
                    targetAnim = "goblin_with_sword_run_" + facingDir;
                    break;
                case EnemyType::Chaser:
                default:
                    // Hoodzy doesn't have directional animations yet, keep existing
                    return;
            }
            
            if (anim.CurrentClipName != targetAnim)
            {
                anim.Play(targetAnim);
            }
        }

    private:
        Pillar::BulletPool* m_BulletPool = nullptr;
    };

} // namespace Game
