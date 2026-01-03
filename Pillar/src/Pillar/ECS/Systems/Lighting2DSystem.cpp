#include "Pillar/ECS/Systems/Lighting2DSystem.h"

#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h"
#include "Pillar/Renderer/Lighting2D.h"

namespace Pillar
{
	void Lighting2DSystem::OnUpdate(float dt)
	{
		if (!m_Scene)
			return;

		auto& registry = m_Scene->GetRegistry();

		// Submit lights
		{
			auto view = registry.view<TransformComponent, Light2DComponent>();
			for (auto entity : view)
			{
				auto& transform = view.get<TransformComponent>(entity);
				auto& lightComp = view.get<Light2DComponent>(entity);

				Light2DSubmit light;
				light.Type = lightComp.Type;
				light.Position = transform.Position;
				light.Direction = Transform2D::Forward(transform.Rotation);
				light.Color = lightComp.Color;
				light.Intensity = lightComp.Intensity;
				light.Radius = lightComp.Radius;
				light.InnerAngleRadians = lightComp.InnerAngleRadians;
				light.OuterAngleRadians = lightComp.OuterAngleRadians;
				light.CastShadows = lightComp.CastShadows;
				light.ShadowStrength = lightComp.ShadowStrength;
				light.LayerMask = lightComp.LayerMask;

				Lighting2D::SubmitLight(light);
			}
		}

		// Submit shadow casters
		{
			auto view = registry.view<TransformComponent, ShadowCaster2DComponent>();
			for (auto entity : view)
			{
				auto& transform = view.get<TransformComponent>(entity);
				auto& casterComp = view.get<ShadowCaster2DComponent>(entity);
				if (casterComp.Points.size() < 2)
					continue;

				ShadowCaster2DSubmit caster;
				caster.Closed = casterComp.Closed;
				caster.TwoSided = casterComp.TwoSided;
				caster.LayerMask = casterComp.LayerMask;
				caster.WorldPoints.reserve(casterComp.Points.size());

				for (const auto& local : casterComp.Points)
					caster.WorldPoints.push_back(transform.TransformPoint(local));

				Lighting2D::SubmitShadowCaster(caster);
			}
		}
	}
}
