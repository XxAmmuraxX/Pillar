#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Entity.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Rendering/SpriteComponent.h>

#include "../Components/EffectComponents.h"

#include <vector>

namespace Game {

    class FlashSystem : public Pillar::System
    {
    public:
        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();

            auto view = registry.view<Pillar::SpriteComponent, FlashComponent>();
            for (auto entity : view)
            {
                auto& sprite = view.get<Pillar::SpriteComponent>(entity);
                auto& flash = view.get<FlashComponent>(entity);

                if (flash.IsFlashing)
                {
                    sprite.Color = flash.GetCurrentColor(dt);
                }
            }
        }
    };

    class TemporaryCleanupSystem : public Pillar::System
    {
    public:
        void OnUpdate(float dt) override
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            std::vector<entt::entity> toDestroy;

            auto view = registry.view<TemporaryComponent>();
            for (auto entity : view)
            {
                auto& temp = view.get<TemporaryComponent>(entity);
                temp.Update(dt);

                if (temp.IsExpired())
                {
                    toDestroy.push_back(entity);
                }
            }

            for (auto entity : toDestroy)
            {
                m_Scene->DestroyEntity(Pillar::Entity(entity, m_Scene));
            }
        }
    };

} // namespace Game
