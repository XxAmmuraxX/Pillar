#include "VelocityIntegrationSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include <glm/glm.hpp>

namespace Pillar {

	void VelocityIntegrationSystem::OnUpdate(float deltaTime)
	{
		IntegrateVelocity(deltaTime);
	}

	void VelocityIntegrationSystem::IntegrateVelocity(float deltaTime)
	{
		auto view = m_Scene->GetRegistry().view<TransformComponent, VelocityComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& velocity = view.get<VelocityComponent>(entity);

			// Apply acceleration (gravity, wind, etc.)
			velocity.Velocity += velocity.Acceleration * deltaTime;

			// Apply drag
			if (velocity.Drag > 0.0f)
			{
				float dragFactor = 1.0f - (velocity.Drag * deltaTime);
				if (dragFactor < 0.0f) dragFactor = 0.0f;
				velocity.Velocity *= dragFactor;
			}

			// Clamp to max speed
			float speedSq = glm::dot(velocity.Velocity, velocity.Velocity);
			if (speedSq > velocity.MaxSpeed * velocity.MaxSpeed)
			{
				velocity.Velocity = glm::normalize(velocity.Velocity) * velocity.MaxSpeed;
			}

			// Integrate position
			transform.Position += velocity.Velocity * deltaTime;
			transform.Dirty = true;
		}
	}

} // namespace Pillar
