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

        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            std::vector<entt::entity> toDestroy;

            auto view = registry.view<Pillar::BulletComponent>();

            for (auto entity : view)
            {
                auto& bullet = view.get<Pillar::BulletComponent>(entity);

                // Update time alive
                bullet.TimeAlive += dt;

                // Check if expired
                if (bullet.TimeAlive >= bullet.Lifetime)
                {
                    toDestroy.push_back(entity);
                }

                // Check if out of hits
                if (bullet.HitsRemaining == 0)
                {
                    toDestroy.push_back(entity);
                }
            }

            // Destroy expired bullets
            for (auto entity : toDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }
        }

    private:
        Pillar::Scene* m_Scene = nullptr;
    };

} // namespace Game
