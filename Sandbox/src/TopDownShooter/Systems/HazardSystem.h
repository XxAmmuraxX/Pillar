#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/ECS/Components/Physics/ColliderComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
#include <Pillar/Logger.h>
#include <glm/glm.hpp>
#include <functional>

#include "../Components/HazardComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/EnemyComponent.h"
#include "../Utilities/CollisionCategories.h"
#include "../Utilities/EffectFactory.h"
#include "../Utilities/AudioManager.h"

namespace Game {

    /**
     * HazardSystem - Manages environmental hazards
     * 
     * Responsibilities:
     * - Update hazard timers and states
     * - Check for entities in hazard areas
     * - Apply damage and status effects
     * - Handle explosive barrel explosions
     * - Render hazard visual effects
     */
    class HazardSystem : public Pillar::System
    {
    public:
        using ExplosionCallback = std::function<void(const glm::vec2& pos, float radius, float damage)>;

        void OnAttach(Pillar::Scene* scene) override
        {
            m_Scene = scene;
        }

        void OnDetach() override
        {
            m_Scene = nullptr;
        }

        void SetExplosionCallback(ExplosionCallback callback)
        {
            m_OnExplosion = callback;
        }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            
            // Update all hazards
            auto hazardView = registry.view<
                Pillar::TransformComponent,
                HazardComponent
            >();

            for (auto entity : hazardView)
            {
                auto& transform = hazardView.get<Pillar::TransformComponent>(entity);
                auto& hazard = hazardView.get<HazardComponent>(entity);

                if (!hazard.IsActive) continue;

                hazard.Update(dt);

                // Check for entities in range
                switch (hazard.Type)
                {
                    case HazardType::SpikeTrap:
                    case HazardType::DamageZone:
                    case HazardType::PoisonPool:
                        ProcessDamageHazard(entity, transform, hazard, dt);
                        break;
                        
                    case HazardType::SlowField:
                        ProcessSlowField(entity, transform, hazard);
                        break;
                        
                    case HazardType::ExplosiveBarrel:
                        ProcessExplosiveBarrel(entity, transform, hazard);
                        break;
                }
            }

            // Update status effects on all entities
            auto statusView = registry.view<StatusEffectComponent>();
            for (auto entity : statusView)
            {
                auto& status = statusView.get<StatusEffectComponent>(entity);
                status.Update(dt);

                // Apply poison damage
                if (status.ShouldApplyPoisonDamage())
                {
                    Pillar::Entity ent(entity, m_Scene);
                    if (auto* health = ent.TryGetComponent<Pillar::HealthComponent>())
                    {
                        health->TakeDamage(status.PoisonDamage);
                    }
                }
            }

            // Clean up dead hazards
            CleanupInactiveHazards();
        }

        /**
         * Handle bullet hitting an explosive barrel
         */
        void OnBarrelHit(entt::entity barrelEntity, float damage)
        {
            auto& registry = m_Scene->GetRegistry();
            
            if (!registry.valid(barrelEntity)) return;
            if (!registry.all_of<HazardComponent>(barrelEntity)) return;

            auto& hazard = registry.get<HazardComponent>(barrelEntity);
            
            if (hazard.Type != HazardType::ExplosiveBarrel || hazard.HasExploded)
                return;

            hazard.Health -= damage;
            
            if (hazard.Health <= 0.0f)
            {
                TriggerExplosion(barrelEntity);
            }
        }

    private:
        void ProcessDamageHazard(
            entt::entity hazardEntity,
            const Pillar::TransformComponent& transform,
            HazardComponent& hazard,
            float dt)
        {
            if (!hazard.CanDamage()) return;

            auto& registry = m_Scene->GetRegistry();
            
            // Check player
            auto playerView = registry.view<Pillar::TransformComponent, PlayerTagComponent, Pillar::HealthComponent>();
            for (auto entity : playerView)
            {
                auto& playerTransform = playerView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(playerTransform.Position, transform.Position);
                
                if (dist <= hazard.DamageRadius)
                {
                    auto& health = playerView.get<Pillar::HealthComponent>(entity);
                    health.TakeDamage(hazard.Damage);
                    hazard.ResetDamageTimer();

                    // Apply poison if applicable
                    if (hazard.Type == HazardType::PoisonPool)
                    {
                        Pillar::Entity ent(entity, m_Scene);
                        auto& status = ent.GetOrAddComponent<StatusEffectComponent>();
                        status.ApplyPoison(hazard.Damage * 0.5f, hazard.EffectDuration);
                    }
                }
            }

            // Check enemies (hazards can damage enemies too!)
            auto enemyView = registry.view<Pillar::TransformComponent, EnemyComponent, Pillar::HealthComponent>();
            for (auto entity : enemyView)
            {
                auto& enemyTransform = enemyView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(enemyTransform.Position, transform.Position);
                
                if (dist <= hazard.DamageRadius)
                {
                    auto& health = enemyView.get<Pillar::HealthComponent>(entity);
                    health.TakeDamage(hazard.Damage);
                    // Don't reset timer per-entity, reset once globally
                }
            }

            hazard.ResetDamageTimer();
        }

        void ProcessSlowField(
            entt::entity hazardEntity,
            const Pillar::TransformComponent& transform,
            HazardComponent& hazard)
        {
            auto& registry = m_Scene->GetRegistry();

            // Apply slow to player
            auto playerView = registry.view<Pillar::TransformComponent, PlayerTagComponent>();
            for (auto entity : playerView)
            {
                auto& playerTransform = playerView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(playerTransform.Position, transform.Position);
                
                if (dist <= hazard.DamageRadius)
                {
                    Pillar::Entity ent(entity, m_Scene);
                    auto& status = ent.GetOrAddComponent<StatusEffectComponent>();
                    status.ApplySlow(hazard.EffectStrength, 0.2f);  // Brief slow, refreshed while in field
                }
            }

            // Apply slow to enemies
            auto enemyView = registry.view<Pillar::TransformComponent, EnemyComponent>();
            for (auto entity : enemyView)
            {
                auto& enemyTransform = enemyView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(enemyTransform.Position, transform.Position);
                
                if (dist <= hazard.DamageRadius)
                {
                    Pillar::Entity ent(entity, m_Scene);
                    auto& status = ent.GetOrAddComponent<StatusEffectComponent>();
                    status.ApplySlow(hazard.EffectStrength, 0.2f);
                }
            }
        }

        void ProcessExplosiveBarrel(
            entt::entity barrelEntity,
            const Pillar::TransformComponent& transform,
            HazardComponent& hazard)
        {
            // Explosive barrels flicker their light
            if (!hazard.HasExploded)
            {
                Pillar::Entity ent(barrelEntity, m_Scene);
                if (auto* light = ent.TryGetComponent<Pillar::Light2DComponent>())
                {
                    light->Intensity = 0.6f + 0.2f * std::sin(hazard.PulseTimer * 5.0f);
                }
            }
        }

        void TriggerExplosion(entt::entity barrelEntity)
        {
            auto& registry = m_Scene->GetRegistry();
            
            if (!registry.valid(barrelEntity)) return;

            auto& transform = registry.get<Pillar::TransformComponent>(barrelEntity);
            auto& hazard = registry.get<HazardComponent>(barrelEntity);

            if (hazard.HasExploded) return;
            hazard.HasExploded = true;
            hazard.IsActive = false;

            // Spawn elaborate multi-stage barrel explosion
            EffectFactory::SpawnBarrelExplosion(*m_Scene, transform.Position, hazard.ExplosionRadius);

            // Play explosion sound (using death sound with lower pitch for boom effect)
            AudioManager::Instance().PlaySound("death", transform.Position, 1.2f, 0.6f);

            // Damage all entities in radius
            DealAreaDamage(transform.Position, hazard.ExplosionRadius, hazard.ExplosionDamage);

            // Callback for additional effects (screen shake, etc.)
            if (m_OnExplosion)
            {
                m_OnExplosion(transform.Position, hazard.ExplosionRadius, hazard.ExplosionDamage);
            }

            // Mark for destruction
            Pillar::Entity ent(barrelEntity, m_Scene);
            m_Scene->DestroyEntity(ent);
        }

        void DealAreaDamage(const glm::vec2& center, float radius, float damage)
        {
            auto& registry = m_Scene->GetRegistry();

            // Damage player
            auto playerView = registry.view<Pillar::TransformComponent, PlayerTagComponent, Pillar::HealthComponent>();
            for (auto entity : playerView)
            {
                auto& transform = playerView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(transform.Position, center);
                
                if (dist <= radius)
                {
                    // Damage falloff based on distance
                    float falloff = 1.0f - (dist / radius);
                    auto& health = playerView.get<Pillar::HealthComponent>(entity);
                    health.TakeDamage(damage * falloff);
                }
            }

            // Damage enemies
            auto enemyView = registry.view<Pillar::TransformComponent, EnemyComponent, Pillar::HealthComponent>();
            for (auto entity : enemyView)
            {
                auto& transform = enemyView.get<Pillar::TransformComponent>(entity);
                float dist = glm::distance(transform.Position, center);
                
                if (dist <= radius)
                {
                    float falloff = 1.0f - (dist / radius);
                    auto& health = enemyView.get<Pillar::HealthComponent>(entity);
                    health.TakeDamage(damage * falloff);
                }
            }

            // Chain reaction - explode other barrels in range!
            auto barrelView = registry.view<Pillar::TransformComponent, HazardComponent>();
            std::vector<entt::entity> chainingBarrels;
            
            for (auto entity : barrelView)
            {
                auto& transform = barrelView.get<Pillar::TransformComponent>(entity);
                auto& hazard = barrelView.get<HazardComponent>(entity);
                
                if (hazard.Type == HazardType::ExplosiveBarrel && !hazard.HasExploded)
                {
                    float dist = glm::distance(transform.Position, center);
                    if (dist <= radius && dist > 0.1f)  // Don't chain to self
                    {
                        chainingBarrels.push_back(entity);
                    }
                }
            }

            // Trigger chain explosions (delayed slightly for visual effect)
            for (auto barrel : chainingBarrels)
            {
                TriggerExplosion(barrel);
            }
        }

        void CleanupInactiveHazards()
        {
            auto& registry = m_Scene->GetRegistry();
            
            std::vector<entt::entity> toDestroy;
            auto view = registry.view<HazardComponent>();
            
            for (auto entity : view)
            {
                auto& hazard = view.get<HazardComponent>(entity);
                if (!hazard.IsActive && hazard.Lifetime >= 0.0f)
                {
                    toDestroy.push_back(entity);
                }
            }

            for (auto entity : toDestroy)
            {
                Pillar::Entity ent(entity, m_Scene);
                m_Scene->DestroyEntity(ent);
            }
        }

        Pillar::Scene* m_Scene = nullptr;
        ExplosionCallback m_OnExplosion;
    };

} // namespace Game
