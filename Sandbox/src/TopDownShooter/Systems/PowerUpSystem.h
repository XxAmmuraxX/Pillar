#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/Logger.h>
#include <glm/glm.hpp>
#include <cmath>
#include <vector>

#include "../Components/PowerUpComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/XPOrbComponent.h"
#include "../Utilities/AudioManager.h"
#include "../Utilities/EffectFactory.h"

namespace Game {

    class PowerUpSystem : public Pillar::System
    {
    public:
        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();

            // Find player
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

            // Check if player has magnet buff active
            float magnetRange = 0.0f;
            if (auto* buffs = playerEntity.TryGetComponent<PlayerBuffsComponent>())
            {
                magnetRange = buffs->GetMagnetRange();
            }

            // Update power-up bobbing and check for collection
            std::vector<entt::entity> toDestroy;

            auto powerUpView = registry.view<
                Pillar::TransformComponent,
                Pillar::SpriteComponent,
                PowerUpComponent
            >();

            for (auto entity : powerUpView)
            {
                auto& transform = powerUpView.get<Pillar::TransformComponent>(entity);
                auto& sprite = powerUpView.get<Pillar::SpriteComponent>(entity);
                auto& powerUp = powerUpView.get<PowerUpComponent>(entity);

                // Update bobbing animation
                powerUp.BobTimer += dt * powerUp.BobSpeed;
                float bobOffset = std::sin(powerUp.BobTimer) * powerUp.BobAmplitude;
                transform.SetPosition(glm::vec2(
                    powerUp.OriginalPosition.x,
                    powerUp.OriginalPosition.y + bobOffset
                ));

                // Pulsing glow effect
                float pulse = 0.8f + 0.2f * std::sin(powerUp.BobTimer * 2.0f);
                sprite.Color.a = pulse;

                // Check collection (circle overlap)
                float pickupRadius = 0.8f;
                float distance = glm::distance(transform.Position, playerPos);

                // Magnet attraction - pull power-ups toward player
                if (magnetRange > 0.0f && distance < magnetRange && distance > pickupRadius)
                {
                    glm::vec2 toPlayer = playerPos - transform.Position;
                    glm::vec2 direction = glm::normalize(toPlayer);
                    float attractSpeed = 8.0f * dt;
                    
                    // Move original position toward player (bobbing is relative to this)
                    powerUp.OriginalPosition += direction * attractSpeed;
                    transform.SetPosition(glm::vec2(
                        powerUp.OriginalPosition.x,
                        powerUp.OriginalPosition.y + std::sin(powerUp.BobTimer) * powerUp.BobAmplitude
                    ));
                }

                if (distance < pickupRadius)
                {
                    // Collect power-up!
                    ApplyPowerUp(playerEntity, powerUp);

                    // Spawn power-up collect effect with matching color
                    glm::vec4 color = PowerUpComponent::GetColorForType(powerUp.Type);
                    EffectFactory::SpawnPowerUpCollectEffect(*m_Scene, transform.Position, color);

                    // Play pickup sound (non-positional, UI sound)
                    AudioManager::Instance().PlaySound("pickup", 0.7f);

                    toDestroy.push_back(entity);
                }
            }

            // Destroy collected power-ups
            for (auto entity : toDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }

            // Apply magnet attraction to XP orbs if magnet buff is active
            if (magnetRange > 0.0f)
            {
                auto xpView = registry.view<
                    Pillar::TransformComponent,
                    XPOrbComponent
                >();

                for (auto entity : xpView)
                {
                    auto& transform = xpView.get<Pillar::TransformComponent>(entity);
                    auto& xpOrb = xpView.get<XPOrbComponent>(entity);

                    float distance = glm::distance(transform.Position, playerPos);

                    // Override the orb's default magnet radius with our buff's range
                    if (distance < magnetRange && distance > xpOrb.PickupRadius)
                    {
                        glm::vec2 toPlayer = playerPos - transform.Position;
                        glm::vec2 direction = glm::normalize(toPlayer);
                        float attractSpeed = xpOrb.MoveSpeed * dt;

                        // Move original position toward player
                        xpOrb.OriginalPosition += direction * attractSpeed;
                        float bobOffset = std::sin(xpOrb.BobTimer) * xpOrb.BobAmplitude;
                        transform.SetPosition(glm::vec2(
                            xpOrb.OriginalPosition.x,
                            xpOrb.OriginalPosition.y + bobOffset
                        ));
                    }
                }
            }

            // Update player buff timers
            if (auto* buffs = playerEntity.TryGetComponent<PlayerBuffsComponent>())
            {
                buffs->UpdateEffects(dt);
            }
        }

    private:
        void ApplyPowerUp(Pillar::Entity player, const PowerUpComponent& powerUp)
        {
            switch (powerUp.Type)
            {
                case PowerUpType::Health:
                {
                    if (auto* health = player.TryGetComponent<Pillar::HealthComponent>())
                    {
                        float healed = health->Heal(powerUp.Value);
                        PIL_INFO("Collected Health: +{:.0f} HP", healed);
                    }
                    break;
                }

                case PowerUpType::SpeedBoost:
                case PowerUpType::FireRateUp:
                case PowerUpType::DamageUp:
                case PowerUpType::Shield:
                {
                    // Add temporary buff
                    auto* buffs = player.TryGetComponent<PlayerBuffsComponent>();
                    if (!buffs)
                    {
                        buffs = &player.AddComponent<PlayerBuffsComponent>();
                    }
                    buffs->AddEffect(powerUp.Type, powerUp.Duration, powerUp.Value);

                    const char* effectName = "";
                    switch (powerUp.Type)
                    {
                        case PowerUpType::SpeedBoost: effectName = "Speed Boost"; break;
                        case PowerUpType::FireRateUp: effectName = "Fire Rate Up"; break;
                        case PowerUpType::DamageUp: effectName = "Damage Up"; break;
                        case PowerUpType::Shield: effectName = "Shield"; break;
                        default: break;
                    }
                    PIL_INFO("Collected {}: {:.1f}s duration", effectName, powerUp.Duration);
                    break;
                }

                case PowerUpType::Magnet:
                {
                    // Add magnet buff to player
                    auto* buffs = player.TryGetComponent<PlayerBuffsComponent>();
                    if (!buffs)
                    {
                        buffs = &player.AddComponent<PlayerBuffsComponent>();
                    }
                    buffs->AddEffect(PowerUpType::Magnet, powerUp.Duration, powerUp.Value);
                    PIL_INFO("Collected Magnet: {:.1f}s duration, {:.0f} range", powerUp.Duration, powerUp.Value);
                    break;
                }
            }
        }
    };

} // namespace Game
