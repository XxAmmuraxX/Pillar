#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/RigidbodyComponent.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <Pillar/Renderer/OrthographicCamera.h>
#include <box2d/b2_body.h>
#include <glm/glm.hpp>

#include "../Components/PlayerTagComponent.h"
#include "../Utilities/GameUtils.h"

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

                // Update dash cooldown
                if (player.DashCooldownTimer > 0.0f)
                    player.DashCooldownTimer -= dt;

                // Handle dash state
                if (player.IsDashing)
                {
                    player.DashTimer -= dt;
                    if (player.DashTimer <= 0.0f)
                        player.IsDashing = false;
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

                // Apply velocity via Box2D
                float speed = player.MoveSpeed;
                b2Vec2 velocity(moveDir.x * speed, moveDir.y * speed);
                rb.Body->SetLinearVelocity(velocity);

                // Dash on Space
                if (Pillar::Input::IsKeyJustPressed(PIL_KEY_SPACE) &&
                    player.DashCooldownTimer <= 0.0f &&
                    glm::length(moveDir) > 0.0f)
                {
                    player.IsDashing = true;
                    player.DashTimer = player.DashDuration;
                    player.DashCooldownTimer = player.DashCooldown;

                    b2Vec2 dashVelocity(moveDir.x * player.DashSpeed,
                                        moveDir.y * player.DashSpeed);
                    rb.Body->SetLinearVelocity(dashVelocity);
                }

                // Rotate to face mouse cursor
                auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
                glm::vec2 mouseWorld = ScreenToWorld(
                    mouseX, mouseY,
                    m_WindowWidth, m_WindowHeight,
                    *m_Camera
                );
                glm::vec2 direction = mouseWorld - transform.Position;

                if (glm::length(direction) > 0.001f)
                {
                    float angle = std::atan2(direction.y, direction.x);
                    transform.SetRotation(angle);
                }
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
    };

} // namespace Game
