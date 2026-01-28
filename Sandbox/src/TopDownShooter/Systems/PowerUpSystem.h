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
#include "../Utilities/AudioManager.h"

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

                if (distance < pickupRadius)
                {
                    // Collect power-up!
                    ApplyPowerUp(playerEntity, powerUp);

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
                    // TODO: Implement magnet attraction
                    PIL_INFO("Collected Magnet power-up");
                    break;
                }
            }
        }
    };

} // namespace Game
