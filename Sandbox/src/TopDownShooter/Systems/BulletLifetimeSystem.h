#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <vector>

namespace Game {

    class BulletLifetimeSystem : public Pillar::System
    {
    public:
        BulletLifetimeSystem() = default;
        ~BulletLifetimeSystem() override = default;

        void OnAttach(Pillar::Scene* scene) override
        {
            m_Scene = scene;
        }

        void OnDetach() override
        {
            m_Scene = nullptr;
        }

        void SetBulletPool(Pillar::BulletPool* pool) { m_BulletPool = pool; }

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            std::vector<entt::entity> toDestroy;
            toDestroy.reserve(64);  // Pre-allocate to avoid reallocation

            auto view = registry.view<Pillar::BulletComponent>();

            for (auto entity : view)
            {
                auto& bullet = view.get<Pillar::BulletComponent>(entity);

                // Update time alive
                bullet.TimeAlive += dt;

                // Check if expired OR out of hits (use single condition to prevent double-push)
                if (bullet.TimeAlive >= bullet.Lifetime || bullet.HitsRemaining <= 0)
                {
                    toDestroy.push_back(entity);
                }
            }

            // Return expired bullets to pool
            for (auto entity : toDestroy)
            {
                Pillar::Entity e(entity, m_Scene);
                if (m_BulletPool)
                {
                    if (auto* sprite = e.TryGetComponent<Pillar::SpriteComponent>())
                        sprite->Visible = false;
                    m_BulletPool->ReturnBullet(e);
                }
                else
                {
                    m_Scene->DestroyEntity(e);
                }
            }
        }

    private:
        Pillar::Scene* m_Scene = nullptr;
        Pillar::BulletPool* m_BulletPool = nullptr;
    };

} // namespace Game
