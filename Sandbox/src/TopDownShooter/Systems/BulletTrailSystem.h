#pragma once

#include <Pillar/ECS/Systems/System.h>
#include <Pillar/ECS/Scene.h>
#include <Pillar/ECS/Components/Core/TransformComponent.h>
#include <Pillar/ECS/Components/Gameplay/BulletComponent.h>
#include <Pillar/Renderer/Renderer2D.h>

#include "../Components/BulletTrailComponent.h"

namespace Game {

    /**
     * BulletTrailSystem - Updates and renders bullet trails
     * 
     * This system:
     * 1. Updates trail points as bullets move
     * 2. Renders the trails as fading line segments
     */
    class BulletTrailSystem : public Pillar::System
    {
    public:
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
                Pillar::BulletComponent,
                BulletTrailComponent
            >();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& trail = view.get<BulletTrailComponent>(entity);

                // Update trail with current position
                trail.Update(dt, transform.Position);
            }
        }

        /**
         * Render all bullet trails
         * Call this BEFORE rendering bullets for proper layering
         */
        void RenderTrails()
        {
            if (!m_Scene) return;

            auto& registry = m_Scene->GetRegistry();
            auto view = registry.view<
                Pillar::TransformComponent,
                BulletTrailComponent
            >();

            for (auto entity : view)
            {
                auto& transform = view.get<Pillar::TransformComponent>(entity);
                auto& trail = view.get<BulletTrailComponent>(entity);

                RenderTrail(transform.Position, trail);
            }
        }

    private:
        void RenderTrail(const glm::vec2& currentPos, const BulletTrailComponent& trail)
        {
            if (trail.TrailPoints.size() < 2) return;

            // Render trail as a series of quads connecting points
            // Each segment fades based on its distance from the head
            for (size_t i = 0; i < trail.TrailPoints.size() - 1; ++i)
            {
                const glm::vec2& p1 = (i == 0) ? currentPos : trail.TrailPoints[i - 1];
                const glm::vec2& p2 = trail.TrailPoints[i];

                // Calculate alpha based on position in trail
                float t = static_cast<float>(i) / static_cast<float>(trail.TrailPoints.size());
                float alpha = trail.TrailFadeStart * (1.0f - t);

                // Calculate width taper
                float width = trail.TrailWidth * (1.0f - t * 0.5f);

                // Calculate perpendicular direction for width
                glm::vec2 dir = p2 - p1;
                float len = glm::length(dir);
                if (len < 0.001f) continue;
                
                dir /= len;
                glm::vec2 perp(-dir.y, dir.x);

                // Create quad vertices
                glm::vec2 v1 = p1 + perp * width;
                glm::vec2 v2 = p1 - perp * width;
                glm::vec2 v3 = p2 - perp * width;
                glm::vec2 v4 = p2 + perp * width;

                // Draw as colored quad
                glm::vec4 color = trail.TrailColor;
                color.a = alpha;

                // Calculate center and size for quad rendering
                glm::vec2 center = (p1 + p2) * 0.5f;
                float rotation = std::atan2(dir.y, dir.x);
                
                // Draw elongated quad as the trail segment
                Pillar::Renderer2D::DrawRotatedQuad(
                    glm::vec3(center, -0.1f),  // Slightly behind bullets
                    glm::vec2(len, width * 2.0f),
                    rotation,
                    color
                );
            }
        }

        Pillar::Scene* m_Scene = nullptr;
    };

} // namespace Game
