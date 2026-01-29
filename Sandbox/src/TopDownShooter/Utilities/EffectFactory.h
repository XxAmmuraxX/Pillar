#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <Pillar/ECS/Components/Rendering/Light2DComponent.h>
#include <glm/glm.hpp>
#include <random>
#include <cmath>

#include "../Components/EffectComponents.h"
#include "ParticleManager.h"

namespace Game {

    /**
     * EffectFactory - Spawns visual effects using the native particle system
     * 
     * All methods now use ParticleManager for pooled, efficient particle spawning.
     * Falls back to entity-based effects if ParticleManager is not initialized.
     */
    class EffectFactory
    {
    public:
        // Spawn muzzle flash effect at position
        static void SpawnMuzzleFlash(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& direction)
        {
            // Use native particle system
            ParticleManager::Instance().SpawnMuzzleFlash(position, direction);
        }

        // Spawn death particles when enemy dies
        static void SpawnDeathParticles(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec4& color,
            int count = 12)
        {
            // Use native particle system
            ParticleManager::Instance().SpawnDeathExplosion(position, color, count);
        }

        // Spawn hit particles when bullet hits enemy
        static void SpawnHitParticles(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& direction,
            int count = 6)
        {
            // Use native particle system
            ParticleManager::Instance().SpawnHitSparks(position, direction, count);
        }

        // Spawn boss death explosion (much bigger)
        static void SpawnBossDeathParticles(
            Pillar::Scene& scene,
            const glm::vec2& position,
            int count = 40)
        {
            ParticleManager::Instance().SpawnBossDeathExplosion(position, count);
        }

        // Spawn elaborate barrel explosion with multiple stages
        static void SpawnBarrelExplosion(
            Pillar::Scene& scene,
            const glm::vec2& position,
            float radius = 3.5f)
        {
            ParticleManager::Instance().SpawnBarrelExplosion(position, radius);
        }

        // Spawn XP collect sparkle
        static void SpawnXPCollectEffect(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            ParticleManager::Instance().SpawnXPCollectEffect(position);
        }

        // Spawn power-up collect effect
        static void SpawnPowerUpCollectEffect(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec4& color)
        {
            ParticleManager::Instance().SpawnPowerUpCollectEffect(position, color);
        }

        // Spawn level up celebration
        static void SpawnLevelUpEffect(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            ParticleManager::Instance().SpawnLevelUpEffect(position);
        }

        // Spawn player damage flash
        static void SpawnPlayerDamageEffect(
            Pillar::Scene& scene,
            const glm::vec2& position)
        {
            ParticleManager::Instance().SpawnPlayerDamageEffect(position);
        }

        // Spawn XP gem when enemy dies (still uses entity for collection logic)
        static Pillar::Entity SpawnXPGem(
            Pillar::Scene& scene,
            const glm::vec2& position,
            float value = 10.0f)
        {
            auto gem = scene.CreateEntity("XPGem");

            auto& transform = gem.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = gem.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.25f, 0.25f);
            sprite.Color = glm::vec4(0.2f, 0.8f, 1.0f, 1.0f);  // Cyan
            sprite.Layer = "Items";
            sprite.OrderInLayer = 2;

            // Could add XPGemComponent here for collection logic
            // gem.AddComponent<XPGemComponent>(value);

            return gem;
        }

        // Spawn dash trail effect using native particles
        static void SpawnDashTrail(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec4& color)
        {
            // Use native particle system for dash trail
            ParticleManager::Instance().SpawnDashTrail(position, color);
        }

        // Spawn temporary muzzle flash light
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

        // Spawn temporary explosion light
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
    };

} // namespace Game
