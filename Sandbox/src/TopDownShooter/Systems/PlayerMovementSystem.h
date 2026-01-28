#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Rendering/AnimationComponent.h>
#include <Pillar/ECS/Components/Gameplay/HealthComponent.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <Pillar/Renderer/OrthographicCamera.h>
#include <box2d/b2_body.h>
#include <glm/glm.hpp>

#include "../Components/PlayerTagComponent.h"
#include "../Components/PowerUpComponent.h"
#include "../Utilities/GameUtils.h"
#include "../Utilities/EffectFactory.h"
#include "../Utilities/AudioManager.h"
#include "../Core/GameState.h"

namespace Game {

    class PlayerMovementSystem : public Pillar::System
    {
    public:
        PlayerMovementSystem(
            const Pillar::OrthographicCamera* camera,
            float windowWidth,
            float windowHeight)
            : m_Camera(camera)
            , m_WindowWidth(windowWidth)
            , m_WindowHeight(windowHeight)
        {
        }

        void OnUpdate(float dt) override
        {
            if (!m_Scene || !m_Camera) return;

            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<
                Pillar::TransformComponent,
                Pillar::RigidbodyComponent,
                PlayerTagComponent
            >();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& rb = view.get<Pillar::RigidbodyComponent>(entity);
                auto& player = view.get<PlayerTagComponent>(entity);

                if (!rb.Body) continue;

                auto entityWrapper = Pillar::Entity(entity, m_Scene);

                // Update dash cooldown
                if (player.DashCooldownTimer > 0.0f)
                    player.DashCooldownTimer -= dt;

                // Handle dash state
                if (player.IsDashing)
                {
                    player.DashTimer -= dt;
                    
                    // Spawn dash trail particles
                    m_DashTrailTimer -= dt;
                    if (m_DashTrailTimer <= 0.0f)
                    {
                        glm::vec4 playerColor(0.4f, 0.7f, 1.0f, 1.0f);  // Blue trail
                        EffectFactory::SpawnDashTrail(*m_Scene, transform.Position, playerColor);
                        m_DashTrailTimer = 0.02f;  // Spawn trail every 20ms during dash
                    }
                    
                    if (player.DashTimer <= 0.0f)
                    {
                        player.IsDashing = false;
                        
                        // End invulnerability after dash
                        if (auto* health = entityWrapper.TryGetComponent<Pillar::HealthComponent>())
                        {
                            health->IsInvulnerable = false;
                        }
                    }
                    continue;  // Skip normal movement during dash
                }

                // Calculate movement direction from input
                glm::vec2 moveDir(0.0f);

                if (Pillar::Input::IsKeyDown(PIL_KEY_W)) moveDir.y += 1.0f;
                if (Pillar::Input::IsKeyDown(PIL_KEY_S)) moveDir.y -= 1.0f;
                if (Pillar::Input::IsKeyDown(PIL_KEY_A)) moveDir.x -= 1.0f;
                if (Pillar::Input::IsKeyDown(PIL_KEY_D)) moveDir.x += 1.0f;

                // Normalize to prevent diagonal speed boost
                if (glm::length(moveDir) > 0.0f)
                    moveDir = glm::normalize(moveDir);

                // Apply speed multiplier from power-ups
                float speedMultiplier = 1.0f;
                if (auto* buffs = entityWrapper.TryGetComponent<PlayerBuffsComponent>())
                {
                    speedMultiplier = buffs->GetSpeedMultiplier();
                }
                // Also apply perk-based speed boost
                speedMultiplier *= GameState::Instance().GetPlayerStats().BaseMoveSpeedMultiplier;

                // Apply velocity via Box2D
                float speed = player.MoveSpeed * speedMultiplier;
                b2Vec2 velocity(moveDir.x * speed, moveDir.y * speed);
                rb.Body->SetLinearVelocity(velocity);

                // Update animation based on movement
                if (auto* anim = entityWrapper.TryGetComponent<Pillar::AnimationComponent>())
                {
                    bool isMoving = glm::length(moveDir) > 0.0f;
                    if (isMoving && anim->CurrentClipName != "Player_walk_cycle")
                    {
                        anim->Play("Player_walk_cycle");
                    }
                    else if (!isMoving && anim->CurrentClipName != "Player_standing")
                    {
                        anim->Play("Player_standing");
                    }
                }

                // Update sprite flip based on mouse direction (face left/right)
                if (auto* sprite = entityWrapper.TryGetComponent<Pillar::SpriteComponent>())
                {
                    auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
                    glm::vec2 mouseWorld = ScreenToWorld(
                        mouseX, mouseY,
                        m_WindowWidth, m_WindowHeight,
                        *m_Camera
                    );
                    glm::vec2 toMouse = mouseWorld - transform.Position;
                    sprite->FlipX = toMouse.x < 0.0f;  // Flip sprite when facing left
                }

                // Dash on Space (or Shift)
                bool dashInput = Pillar::Input::IsKeyJustPressed(PIL_KEY_SPACE) ||
                                 Pillar::Input::IsKeyJustPressed(PIL_KEY_LEFT_SHIFT);
                if (dashInput &&
                    player.DashCooldownTimer <= 0.0f &&
                    glm::length(moveDir) > 0.0f)
                {
                    player.IsDashing = true;
                    player.DashTimer = player.DashDuration;
                    player.DashCooldownTimer = player.DashCooldown;
                    m_DashTrailTimer = 0.0f;

                    b2Vec2 dashVelocity(moveDir.x * player.DashSpeed,
                                        moveDir.y * player.DashSpeed);
                    rb.Body->SetLinearVelocity(dashVelocity);
                    
                    // Make player invulnerable during dash
                    if (auto* health = entityWrapper.TryGetComponent<Pillar::HealthComponent>())
                    {
                        health->IsInvulnerable = true;
                    }
                    
                    // Play dash sound
                    AudioManager::Instance().PlaySound("pickup", 0.5f, 1.5f);  // Higher pitch whoosh
                }

                // Note: Player doesn't rotate - we use FlipX on the sprite to face left/right
            }
        }

        void SetCamera(const Pillar::OrthographicCamera* camera) { m_Camera = camera; }
        void SetWindowSize(float width, float height)
        {
            m_WindowWidth = width;
            m_WindowHeight = height;
        }

    private:
        const Pillar::OrthographicCamera* m_Camera = nullptr;
        float m_WindowWidth = 1280.0f;
        float m_WindowHeight = 720.0f;
        float m_DashTrailTimer = 0.0f;
    };

} // namespace Game
