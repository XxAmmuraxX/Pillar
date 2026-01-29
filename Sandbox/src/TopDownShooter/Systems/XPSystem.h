#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/Logger.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <functional>

#include "../Components/XPOrbComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Core/GameState.h"
#include "../Utilities/AudioManager.h"
#include "../Utilities/EffectFactory.h"

namespace Game {

    /**
     * XPSystem - Handles XP orb movement, attraction, collection, and leveling
     */
    class XPSystem : public Pillar::System
    {
    public:
        using LevelUpCallback = std::function<void(int newLevel)>;

        void SetOnLevelUp(LevelUpCallback callback) { m_OnLevelUp = callback; }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            auto& gameState = GameState::Instance();
            auto& playerStats = gameState.GetPlayerStats();

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

            // Get player's pickup radius (base + magnet bonus)
            float pickupRadius = 1.0f + playerStats.XPPickupRadius;
            float magnetRadius = 5.0f + playerStats.XPPickupRadius * 2.0f;

            std::vector<entt::entity> toDestroy;

            // Update all XP orbs
            auto orbView = registry.view<
                Pillar::TransformComponent,
                Pillar::SpriteComponent,
                XPOrbComponent
            >();

            for (auto entity : orbView)
            {
                auto& transform = orbView.get<Pillar::TransformComponent>(entity);
                auto& sprite = orbView.get<Pillar::SpriteComponent>(entity);
                auto& orb = orbView.get<XPOrbComponent>(entity);

                // Update lifetime
                orb.Lifetime -= dt;
                if (orb.Lifetime <= 0.0f)
                {
                    toDestroy.push_back(entity);
                    continue;
                }

                // Fade out when lifetime is low
                if (orb.Lifetime < 5.0f)
                {
                    float alpha = orb.Lifetime / 5.0f;
                    sprite.Color.a = alpha;
                }

                // Calculate distance to player
                float distance = glm::distance(transform.Position, playerPos);

                // Check collection
                if (distance < pickupRadius)
                {
                    // Collect XP!
                    int previousLevel = gameState.GetStats().PlayerLevel;
                    gameState.GetStats().AddXP(orb.XPValue);
                    gameState.GetStats().Score += orb.XPValue;
                    
                    // Spawn XP collect effect
                    EffectFactory::SpawnXPCollectEffect(*m_Scene, transform.Position);
                    
                    // Check for level up
                    if (gameState.GetStats().PlayerLevel > previousLevel)
                    {
                        // Spawn level up particles at player position
                        EffectFactory::SpawnLevelUpEffect(*m_Scene, playerPos);
                        
                        // Play special level up sound
                        AudioManager::Instance().PlaySound("pickup", 1.0f, 0.5f);  // Low pitch
                        
                        // Notify UI
                        if (m_OnLevelUp)
                            m_OnLevelUp(gameState.GetStats().PlayerLevel);
                    }

                    // Play pickup sound (pitch varies with XP value)
                    float pitch = 0.8f + (orb.XPValue / 50.0f) * 0.4f;
                    AudioManager::Instance().PlaySound("pickup", 0.5f, pitch);

                    toDestroy.push_back(entity);
                    continue;
                }

                // Magnet attraction
                if (distance < magnetRadius)
                {
                    glm::vec2 toPlayer = playerPos - transform.Position;
                    if (glm::length(toPlayer) > 0.001f)
                    {
                        toPlayer = glm::normalize(toPlayer);
                        
                        // Move faster as orb gets closer
                        float speedMultiplier = 1.0f + (1.0f - distance / magnetRadius) * 2.0f;
                        glm::vec2 newPos = transform.Position + toPlayer * orb.MoveSpeed * speedMultiplier * dt;
                        transform.SetPosition(newPos);
                        orb.OriginalPosition = newPos;  // Update original for bobbing
                    }
                }
                else
                {
                    // Bobbing animation when not attracted
                    orb.BobTimer += dt * orb.BobSpeed;
                    float bobOffset = std::sin(orb.BobTimer) * orb.BobAmplitude;
                    transform.SetPosition(glm::vec2(
                        orb.OriginalPosition.x,
                        orb.OriginalPosition.y + bobOffset
                    ));
                }

                // Pulsing glow effect
                float pulse = 0.7f + 0.3f * std::sin(orb.BobTimer * 2.0f);
                glm::vec4 baseColor = XPOrbComponent::GetColorForValue(orb.XPValue);
                sprite.Color = glm::vec4(baseColor.r, baseColor.g, baseColor.b, sprite.Color.a * pulse);
            }

            // Destroy collected/expired orbs
            for (auto entity : toDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }
        }

    private:
        LevelUpCallback m_OnLevelUp;
    };

} // namespace Game
