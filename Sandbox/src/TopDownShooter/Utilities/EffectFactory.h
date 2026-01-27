#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <glm/glm.hpp>
#include <random>
#include <cmath>

#include "../Components/EffectComponents.h"

namespace Game {

    class EffectFactory
    {
    public:
        // Spawn muzzle flash effect at position
        static void SpawnMuzzleFlash(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& direction)
        {
            auto flash = scene.CreateEntity("MuzzleFlash");

            auto& transform = flash.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position + direction * 0.3f);
            transform.SetRotation(std::atan2(direction.y, direction.x));

            auto& sprite = flash.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.4f, 0.25f);
            sprite.Color = glm::vec4(1.0f, 0.9f, 0.3f, 1.0f);  // Bright yellow
            sprite.Layer = "Effects";
            sprite.OrderInLayer = 100;

            flash.AddComponent<TemporaryComponent>(0.05f);  // Very short-lived
        }

        // Spawn death particles when enemy dies
        static void SpawnDeathParticles(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec4& color,
            int count = 8)
        {
            static std::mt19937 rng{ std::random_device{}() };
            std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);
            std::uniform_real_distribution<float> speedDist(2.0f, 6.0f);
            std::uniform_real_distribution<float> sizeDist(0.1f, 0.25f);
            std::uniform_real_distribution<float> lifeDist(0.3f, 0.6f);

            for (int i = 0; i < count; i++)
            {
                auto particle = scene.CreateEntity("DeathParticle");

                auto& transform = particle.GetComponent<Pillar::TransformComponent>();
                transform.SetPosition(position);

                auto& sprite = particle.AddComponent<Pillar::SpriteComponent>();
                float size = sizeDist(rng);
                sprite.Size = glm::vec2(size, size);
                sprite.Color = color;
                sprite.Layer = "Effects";
                sprite.OrderInLayer = 50;

                // Random velocity outward
                float angle = angleDist(rng);
                float speed = speedDist(rng);
                auto& velocity = particle.AddComponent<Pillar::VelocityComponent>();
                velocity.Velocity = glm::vec2(std::cos(angle), std::sin(angle)) * speed;
                velocity.Drag = 5.0f;

                particle.AddComponent<TemporaryComponent>(lifeDist(rng));
            }
        }

        // Spawn hit particles when bullet hits enemy
        static void SpawnHitParticles(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec2& direction,
            int count = 4)
        {
            static std::mt19937 rng{ std::random_device{}() };
            std::uniform_real_distribution<float> spreadDist(-0.5f, 0.5f);
            std::uniform_real_distribution<float> speedDist(3.0f, 5.0f);

            for (int i = 0; i < count; i++)
            {
                auto particle = scene.CreateEntity("HitParticle");

                auto& transform = particle.GetComponent<Pillar::TransformComponent>();
                transform.SetPosition(position);

                auto& sprite = particle.AddComponent<Pillar::SpriteComponent>();
                sprite.Size = glm::vec2(0.08f, 0.08f);
                sprite.Color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);  // Orange sparks
                sprite.Layer = "Effects";
                sprite.OrderInLayer = 60;

                // Velocity in opposite direction of bullet with spread
                glm::vec2 dir = glm::normalize(-direction + glm::vec2(spreadDist(rng), spreadDist(rng)));
                float speed = speedDist(rng);
                auto& velocity = particle.AddComponent<Pillar::VelocityComponent>();
                velocity.Velocity = dir * speed;
                velocity.Drag = 8.0f;

                particle.AddComponent<TemporaryComponent>(0.15f);
            }
        }

        // Spawn XP gem when enemy dies
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

        // Spawn dash trail effect
        static void SpawnDashTrail(
            Pillar::Scene& scene,
            const glm::vec2& position,
            const glm::vec4& color)
        {
            auto trail = scene.CreateEntity("DashTrail");

            auto& transform = trail.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            auto& sprite = trail.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.6f, 0.6f);
            sprite.Color = glm::vec4(color.r, color.g, color.b, 0.5f);  // Semi-transparent
            sprite.Layer = "Effects";
            sprite.OrderInLayer = -5;

            trail.AddComponent<TemporaryComponent>(0.2f);
        }
    };

} // namespace Game
