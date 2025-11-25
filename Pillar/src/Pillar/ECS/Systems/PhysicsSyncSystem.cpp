#include "PhysicsSyncSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"

namespace Pillar {

	void PhysicsSyncSystem::OnUpdate(float deltaTime)
	{
		SyncTransformsFromBox2D();
	}

	void PhysicsSyncSystem::SyncTransformsFromBox2D()
	{
		// Get all entities with both Transform and Rigidbody
		auto view = m_Scene->GetRegistry().view<TransformComponent, RigidbodyComponent>();

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& rigidbody = view.get<RigidbodyComponent>(entity);

			if (rigidbody.Body)
			{
				// READ from Box2D (Source of Truth) for dynamic and kinematic bodies
				// Static bodies don't move, so no need to sync
				if (rigidbody.BodyType != b2_staticBody)
				{
					const b2Vec2& pos = rigidbody.Body->GetPosition();
					float angle = rigidbody.Body->GetAngle();

					// WRITE to EnTT (for rendering)
					transform.Position = { pos.x, pos.y };
					transform.Rotation = angle;
					transform.Dirty = true; // Mark for matrix recalc
				}
			}
		}
	}

} // namespace Pillar
