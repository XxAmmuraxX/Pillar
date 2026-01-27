#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <glm/glm.hpp>

#include "../Components/WeaponComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/EffectComponents.h"
#include "../Utilities/CollisionCategories.h"
#include "../Utilities/GameUtils.h"
#include "../Utilities/EffectFactory.h"

namespace Game {

    class WeaponSystem : public Pillar::System
    {
    public:
        WeaponSystem() = default;
        ~WeaponSystem() override = default;

        void OnAttach(Pillar::Scene* scene) override
        {
            m_Scene = scene;
        }

        void OnDetach() override
        {
            m_Scene = nullptr;
        }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<
                Pillar::TransformComponent,
                WeaponComponent,
                PlayerTagComponent
            >();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& weapon = view.get<WeaponComponent>(entity);

                // Update cooldown
                weapon.UpdateCooldown(dt);

                // Fire on left mouse button
                if (Pillar::Input::IsMouseButtonDown(PIL_MOUSE_BUTTON_LEFT) &&
                    weapon.CanFire())
                {
                    Pillar::Entity ownerEntity(entity, m_Scene);
                    Fire(ownerEntity, transform, weapon);
                }
            }
        }

    private:
        void Fire(
            Pillar::Entity owner,
            const Pillar::TransformComponent& transform,
            WeaponComponent& weapon)
        {
            // Calculate firing direction from rotation
            glm::vec2 direction(
                std::cos(transform.Rotation),
                std::sin(transform.Rotation)
            );

            // Spawn position slightly ahead of player
            glm::vec2 spawnPos = transform.Position + direction * 0.6f;

            // Apply spread for multiple bullets
            for (int i = 0; i < weapon.BulletsPerShot; ++i)
            {
                glm::vec2 bulletDir = direction;

                if (weapon.Spread > 0.0f)
                {
                    float spreadAngle = glm::radians(
                        (RandomFloat(0.0f, 1.0f) - 0.5f) * weapon.Spread
                    );
                    bulletDir = glm::vec2(
                        direction.x * std::cos(spreadAngle) - direction.y * std::sin(spreadAngle),
                        direction.x * std::sin(spreadAngle) + direction.y * std::cos(spreadAngle)
                    );
                }

                // Create bullet
                CreateBullet(owner, spawnPos, bulletDir, weapon.BulletSpeed, weapon.Damage);
            }

            // Spawn muzzle flash effect
            EffectFactory::SpawnMuzzleFlash(*m_Scene, spawnPos, direction);

            weapon.ResetCooldown();
        }

        void CreateBullet(
            Pillar::Entity owner,
            const glm::vec2& position,
            const glm::vec2& direction,
            float speed,
            float damage)
        {
            auto bullet = m_Scene->CreateEntity("Bullet");

            // Transform
            auto& transform = bullet.GetComponent<Pillar::TransformComponent>();
            transform.SetPosition(position);

            // Rotate to face direction
            float angle = std::atan2(direction.y, direction.x);
            transform.SetRotation(angle);

            // Small bright sprite
            auto& sprite = bullet.AddComponent<Pillar::SpriteComponent>();
            sprite.Size = glm::vec2(0.3f, 0.15f);
            sprite.Color = glm::vec4(1.0f, 1.0f, 0.5f, 1.0f);  // Yellow tint
            sprite.Layer = "Projectiles";
            sprite.OrderInLayer = 5;

            // Velocity-based movement (no physics body - lightweight)
            auto& velocity = bullet.AddComponent<Pillar::VelocityComponent>();
            velocity.Velocity = direction * speed;
            velocity.MaxSpeed = speed * 1.5f;

            // Bullet data
            auto& bulletComp = bullet.AddComponent<Pillar::BulletComponent>(owner, damage);
            bulletComp.Lifetime = 3.0f;
            bulletComp.Pierce = false;
            bulletComp.MaxHits = 1;
            bulletComp.HitsRemaining = 1;
        }

    private:
        Pillar::Scene* m_Scene = nullptr;
    };

} // namespace Game
