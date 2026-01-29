#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Physics/VelocityComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <Pillar/Renderer/Texture.h>
#include <Pillar/Utils/AssetManager.h>
#include <Pillar/Input.h>
#include <Pillar/KeyCodes.h>
#include <glm/glm.hpp>

#include "../Components/WeaponComponent.h"
#include "../Components/PlayerTagComponent.h"
#include "../Components/PowerUpComponent.h"
#include "../Components/EffectComponents.h"
#include "../Components/BulletTrailComponent.h"
#include "../Core/GameState.h"
#include "../Utilities/CollisionCategories.h"
#include "../Utilities/GameUtils.h"
#include "../Utilities/EffectFactory.h"
#include "../Utilities/AudioManager.h"

namespace Game {

    class WeaponSystem : public Pillar::System
    {
    public:
        WeaponSystem(
            const Pillar::OrthographicCamera* camera,
            float windowWidth,
            float windowHeight)
            : m_Camera(camera)
            , m_WindowWidth(windowWidth)
            , m_WindowHeight(windowHeight)
        {
        }
        
        ~WeaponSystem() override = default;

        void OnAttach(Pillar::Scene* scene) override
        {
            m_Scene = scene;
            // Cache bullet texture to avoid loading for every bullet
            m_BulletTexture = Pillar::Texture2D::Create(Pillar::AssetManager::GetTexturePath("Soul_Orb.png"));
        }

        void OnDetach() override
        {
            m_Scene = nullptr;
        }
        
        void SetCamera(const Pillar::OrthographicCamera* camera) { m_Camera = camera; }
        void SetWindowSize(float width, float height)
        {
            m_WindowWidth = width;
            m_WindowHeight = height;
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

            // Get perk-based fire rate multiplier
            float perkFireRateMultiplier = GameState::Instance().GetPlayerStats().BaseFireRateMultiplier;

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& weapon = view.get<WeaponComponent>(entity);
                
                Pillar::Entity ownerEntity(entity, m_Scene);
                
                // Get power-up buff multiplier
                float buffFireRateMultiplier = 1.0f;
                if (auto* buffs = ownerEntity.TryGetComponent<PlayerBuffsComponent>())
                {
                    buffFireRateMultiplier = buffs->GetFireRateMultiplier();
                }
                
                float totalFireRateMultiplier = perkFireRateMultiplier * buffFireRateMultiplier;

                // Update cooldown (faster with fire rate bonuses)
                weapon.UpdateCooldown(dt * totalFireRateMultiplier);

                // Fire on left mouse button
                if (Pillar::Input::IsMouseButtonDown(PIL_MOUSE_BUTTON_LEFT) &&
                    weapon.CanFire())
                {
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
            // Get perk multipliers
            auto& playerStats = GameState::Instance().GetPlayerStats();
            float damageMultiplier = playerStats.BaseDamageMultiplier;
            float bulletSpeedMultiplier = playerStats.BaseBulletSpeedMultiplier;
            int extraPierce = playerStats.ExtraPierce;
            
            // Get power-up buff multipliers
            if (auto* buffs = owner.TryGetComponent<PlayerBuffsComponent>())
            {
                damageMultiplier *= buffs->GetDamageMultiplier();
            }

            // Calculate firing direction towards mouse cursor
            auto [mouseX, mouseY] = Pillar::Input::GetMousePosition();
            glm::vec2 mouseWorld = ScreenToWorld(
                mouseX, mouseY,
                m_WindowWidth, m_WindowHeight,
                *m_Camera
            );
            glm::vec2 toMouse = mouseWorld - transform.Position;
            glm::vec2 direction = glm::length(toMouse) > 0.001f ? glm::normalize(toMouse) : glm::vec2(1.0f, 0.0f);

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

                // Create bullet with perk-enhanced stats
                float finalDamage = weapon.Damage * damageMultiplier;
                float finalSpeed = weapon.BulletSpeed * bulletSpeedMultiplier;
                CreateBullet(owner, spawnPos, bulletDir, finalSpeed, finalDamage, extraPierce);
            }

            // Spawn muzzle flash effect
            EffectFactory::SpawnMuzzleFlash(*m_Scene, spawnPos, direction);

            // Play shoot sound (positional)
            AudioManager::Instance().PlaySound("shoot", spawnPos, 0.8f);

            weapon.ResetCooldown();
        }

        void CreateBullet(
            Pillar::Entity owner,
            const glm::vec2& position,
            const glm::vec2& direction,
            float speed,
            float damage,
            int extraPierce = 0)
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
            sprite.Texture = m_BulletTexture; // Use cached texture
            sprite.Size = glm::vec2(0.4f, 0.4f);
            sprite.Color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // White (no tint)
            sprite.Layer = "Projectiles";
            sprite.OrderInLayer = 5;

            // Velocity-based movement (no physics body - lightweight)
            auto& velocity = bullet.AddComponent<Pillar::VelocityComponent>();
            velocity.Velocity = direction * speed;
            velocity.MaxSpeed = speed * 1.5f;

            // Bullet data
            auto& bulletComp = bullet.AddComponent<Pillar::BulletComponent>(owner, damage);
            bulletComp.Lifetime = 3.0f;
            bulletComp.Pierce = extraPierce > 0;
            bulletComp.MaxHits = 1 + extraPierce;
            bulletComp.HitsRemaining = 1 + extraPierce;

            // Bullet trail for visual effect
            glm::vec4 trailColor = GetTrailColorForWeapon();
            bullet.AddComponent<BulletTrailComponent>(trailColor, 10);
        }

        glm::vec4 GetTrailColorForWeapon()
        {
            auto currentWeapon = GameState::Instance().GetCurrentWeapon();
            switch (currentWeapon)
            {
                case WeaponType::Pistol:
                    return { 1.0f, 0.9f, 0.5f, 0.7f };  // Yellow-gold
                case WeaponType::Shotgun:
                    return { 1.0f, 0.6f, 0.3f, 0.6f };  // Orange
                case WeaponType::SMG:
                    return { 0.8f, 1.0f, 0.5f, 0.7f };  // Yellow-green
                case WeaponType::Rifle:
                    return { 0.5f, 0.8f, 1.0f, 0.8f };  // Blue
                case WeaponType::Laser:
                    return { 1.0f, 0.3f, 0.3f, 0.9f };  // Red
                default:
                    return { 1.0f, 1.0f, 1.0f, 0.6f };  // White
            }
        }

    private:
        const Pillar::OrthographicCamera* m_Camera = nullptr;
        float m_WindowWidth = 1280.0f;
        float m_WindowHeight = 720.0f;
        Pillar::Scene* m_Scene = nullptr;
        std::shared_ptr<Pillar::Texture2D> m_BulletTexture; // Cached bullet texture
    };

} // namespace Game
