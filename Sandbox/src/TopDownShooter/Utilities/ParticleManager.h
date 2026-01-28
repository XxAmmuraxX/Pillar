#pragma once

#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/SpecializedPools.h>
#include <Pillar/ECS/Systems/ParticleSystem.h>
#include <Pillar/ECS/Systems/ParticleEmitterSystem.h>
#include <Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h>
#include <Pillar/Logger.h>
#include <glm/glm.hpp>

namespace Game {

    /**
     * ParticleManager - Wrapper around Pillar's native particle system
     * 
     * Provides easy-to-use methods for spawning game-specific particle effects:
     * - Muzzle flash
     * - Hit sparks
     * - Death explosions
     * - Dash trails
     * - XP pickup sparkles
     * - Boss damage effects
     */
    class ParticleManager
    {
    public:
        static ParticleManager& Instance()
        {
            static ParticleManager instance;
            return instance;
        }

        void Init(Pillar::Scene* scene, uint32_t poolCapacity = 2000)
        {
            if (m_Initialized) return;

            m_Scene = scene;
            m_ParticlePool.Init(scene, poolCapacity);

            // Create particle systems
            m_ParticleSystem = new Pillar::ParticleSystem();
            m_ParticleSystem->OnAttach(scene);
            m_ParticleSystem->SetParticlePool(&m_ParticlePool);

            m_EmitterSystem = new Pillar::ParticleEmitterSystem();
            m_EmitterSystem->OnAttach(scene);
            m_EmitterSystem->SetParticlePool(&m_ParticlePool);

            m_Initialized = true;
            PIL_INFO("ParticleManager: Initialized with pool capacity {}", poolCapacity);
        }

        void Shutdown()
        {
            if (m_EmitterSystem)
            {
                m_EmitterSystem->OnDetach();
                delete m_EmitterSystem;
                m_EmitterSystem = nullptr;
            }
            if (m_ParticleSystem)
            {
                m_ParticleSystem->OnDetach();
                delete m_ParticleSystem;
                m_ParticleSystem = nullptr;
            }
            m_ParticlePool.Clear();
            m_Scene = nullptr;
            m_Initialized = false;
        }

        void OnUpdate(float dt)
        {
            if (!m_Initialized) return;

            // Update emitter system (spawns new particles)
            m_EmitterSystem->OnUpdate(dt);

            // Update particle system (moves and ages particles)
            m_ParticleSystem->OnUpdate(dt);
        }

        // === Instant Burst Effects ===

        /**
         * Muzzle flash - bright, quick burst at gun barrel
         */
        void SpawnMuzzleFlash(const glm::vec2& position, const glm::vec2& direction)
        {
            if (!m_Initialized) return;

            // Spawn 3-5 quick bright particles
            for (int i = 0; i < 4; i++)
            {
                float spreadAngle = (RandomFloat(-15.0f, 15.0f)) * 0.0174533f; // degrees to radians
                float speed = RandomFloat(8.0f, 15.0f);
                
                glm::vec2 velocity = RotateVector(direction, spreadAngle) * speed;
                
                m_ParticlePool.SpawnParticle(
                    position + direction * 0.3f,    // Slightly in front
                    velocity,
                    glm::vec4(1.0f, 0.9f, 0.4f, 1.0f),  // Bright yellow-orange
                    RandomFloat(0.08f, 0.15f),          // Small
                    RandomFloat(0.04f, 0.08f)           // Very short lifetime
                );
            }
        }

        /**
         * Hit sparks - spray outward from impact point
         */
        void SpawnHitSparks(const glm::vec2& position, const glm::vec2& impactDirection, int count = 6)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < count; i++)
            {
                // Spray in opposite direction of impact with spread
                float spreadAngle = RandomFloat(-60.0f, 60.0f) * 0.0174533f;
                float speed = RandomFloat(4.0f, 8.0f);
                
                glm::vec2 velocity = RotateVector(-impactDirection, spreadAngle) * speed;
                
                m_ParticlePool.SpawnParticle(
                    position,
                    velocity,
                    glm::vec4(1.0f, 0.7f, 0.2f, 1.0f),  // Orange sparks
                    RandomFloat(0.05f, 0.1f),
                    RandomFloat(0.1f, 0.2f)
                );
            }
        }

        /**
         * Death explosion - colored burst based on enemy color
         */
        void SpawnDeathExplosion(const glm::vec2& position, const glm::vec4& baseColor, int count = 12)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < count; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);  // Full circle
                float speed = RandomFloat(3.0f, 8.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                // Vary color slightly
                glm::vec4 color = baseColor;
                color.r = glm::clamp(color.r + RandomFloat(-0.1f, 0.1f), 0.0f, 1.0f);
                color.g = glm::clamp(color.g + RandomFloat(-0.1f, 0.1f), 0.0f, 1.0f);
                color.b = glm::clamp(color.b + RandomFloat(-0.1f, 0.1f), 0.0f, 1.0f);
                
                m_ParticlePool.SpawnParticle(
                    position + glm::vec2(RandomFloat(-0.2f, 0.2f), RandomFloat(-0.2f, 0.2f)),
                    velocity,
                    color,
                    RandomFloat(0.1f, 0.25f),
                    RandomFloat(0.3f, 0.6f)
                );
            }
        }

        /**
         * Boss death - massive explosion with multiple colors
         */
        void SpawnBossDeathExplosion(const glm::vec2& position, int count = 40)
        {
            if (!m_Initialized) return;

            // Core explosion - orange/red
            SpawnDeathExplosion(position, glm::vec4(1.0f, 0.4f, 0.1f, 1.0f), count / 2);

            // Secondary ring - yellow
            for (int i = 0; i < count / 2; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);
                float speed = RandomFloat(5.0f, 12.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                m_ParticlePool.SpawnParticle(
                    position,
                    velocity,
                    glm::vec4(1.0f, 0.9f, 0.3f, 1.0f),  // Bright yellow
                    RandomFloat(0.15f, 0.35f),
                    RandomFloat(0.5f, 1.0f)
                );
            }

            // Smoke particles - gray, slower
            for (int i = 0; i < count / 4; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);
                float speed = RandomFloat(1.0f, 3.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                m_ParticlePool.SpawnParticle(
                    position + glm::vec2(RandomFloat(-0.5f, 0.5f), RandomFloat(-0.5f, 0.5f)),
                    velocity,
                    glm::vec4(0.3f, 0.3f, 0.3f, 0.7f),  // Gray smoke
                    RandomFloat(0.3f, 0.6f),
                    RandomFloat(0.8f, 1.5f)
                );
            }
        }

        /**
         * Dash trail - afterimage effect behind dashing player
         */
        void SpawnDashTrail(const glm::vec2& position, const glm::vec4& playerColor)
        {
            if (!m_Initialized) return;

            // Spawn a few particles at player's position with slight spread
            for (int i = 0; i < 3; i++)
            {
                glm::vec2 offset(RandomFloat(-0.2f, 0.2f), RandomFloat(-0.2f, 0.2f));
                glm::vec2 velocity(RandomFloat(-0.5f, 0.5f), RandomFloat(-0.5f, 0.5f));
                
                glm::vec4 color = playerColor;
                color.a = 0.5f;  // Semi-transparent
                
                m_ParticlePool.SpawnParticle(
                    position + offset,
                    velocity,
                    color,
                    RandomFloat(0.2f, 0.4f),
                    RandomFloat(0.15f, 0.25f)
                );
            }
        }

        /**
         * XP pickup sparkle - when collecting XP orb
         */
        void SpawnXPCollectEffect(const glm::vec2& position)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < 8; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);
                float speed = RandomFloat(2.0f, 5.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                m_ParticlePool.SpawnParticle(
                    position,
                    velocity,
                    glm::vec4(0.3f, 0.8f, 1.0f, 1.0f),  // Cyan sparkles
                    RandomFloat(0.05f, 0.12f),
                    RandomFloat(0.2f, 0.4f)
                );
            }
        }

        /**
         * Power-up pickup effect - burst in power-up's color
         */
        void SpawnPowerUpCollectEffect(const glm::vec2& position, const glm::vec4& color)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < 10; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);
                float speed = RandomFloat(3.0f, 6.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                m_ParticlePool.SpawnParticle(
                    position,
                    velocity,
                    color,
                    RandomFloat(0.08f, 0.15f),
                    RandomFloat(0.25f, 0.5f)
                );
            }
        }

        /**
         * Level up effect - upward golden particles
         */
        void SpawnLevelUpEffect(const glm::vec2& position)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < 20; i++)
            {
                float angle = RandomFloat(-0.5f, 0.5f);  // Mostly upward
                float speed = RandomFloat(4.0f, 8.0f);
                
                glm::vec2 velocity(std::sin(angle) * speed * 0.3f, speed);  // Upward bias
                
                m_ParticlePool.SpawnParticle(
                    position + glm::vec2(RandomFloat(-0.5f, 0.5f), 0.0f),
                    velocity,
                    glm::vec4(1.0f, 0.85f, 0.2f, 1.0f),  // Gold
                    RandomFloat(0.1f, 0.2f),
                    RandomFloat(0.5f, 1.0f)
                );
            }
        }

        /**
         * Damage flash - red particles when player takes damage
         */
        void SpawnPlayerDamageEffect(const glm::vec2& position)
        {
            if (!m_Initialized) return;

            for (int i = 0; i < 8; i++)
            {
                float angle = RandomFloat(0.0f, 6.28318f);
                float speed = RandomFloat(2.0f, 5.0f);
                
                glm::vec2 velocity(std::cos(angle) * speed, std::sin(angle) * speed);
                
                m_ParticlePool.SpawnParticle(
                    position,
                    velocity,
                    glm::vec4(1.0f, 0.2f, 0.2f, 0.8f),  // Red
                    RandomFloat(0.1f, 0.2f),
                    RandomFloat(0.15f, 0.3f)
                );
            }
        }

        // === Stats ===
        uint32_t GetActiveParticleCount() const 
        { 
            return static_cast<uint32_t>(m_ParticlePool.GetActiveCount()); 
        }

    private:
        ParticleManager() = default;
        ~ParticleManager() { Shutdown(); }

        // Disable copy
        ParticleManager(const ParticleManager&) = delete;
        ParticleManager& operator=(const ParticleManager&) = delete;

        float RandomFloat(float min, float max)
        {
            return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
        }

        glm::vec2 RotateVector(const glm::vec2& v, float angleRadians)
        {
            float c = std::cos(angleRadians);
            float s = std::sin(angleRadians);
            return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
        }

    private:
        bool m_Initialized = false;
        Pillar::Scene* m_Scene = nullptr;
        Pillar::ParticlePool m_ParticlePool;
        Pillar::ParticleSystem* m_ParticleSystem = nullptr;
        Pillar::ParticleEmitterSystem* m_EmitterSystem = nullptr;
    };

} // namespace Game
