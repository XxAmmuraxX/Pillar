#pragma once

#include "Pillar/Core.h"
#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace Pillar {

	struct ColliderComponent;

	class PIL_API Box2DBodyFactory
	{
	public:
		// Create a Box2D body from component data
		static b2Body* CreateBody(
			b2World* world,
			const glm::vec2& position,
			float rotation,
			b2BodyType bodyType,
			bool fixedRotation = false,
			float gravityScale = 1.0f,
			float linearDamping = 0.0f,
			float angularDamping = 0.0f,
			bool isBullet = false,
			bool isEnabled = true
		);

		// Create fixtures from ColliderComponent
		static b2Fixture* CreateFixture(b2Body* body, const ColliderComponent& collider);

	private:
		static b2FixtureDef CreateFixtureDef(const ColliderComponent& collider);
		static b2CircleShape CreateCircleShape(const ColliderComponent& collider);
		static b2PolygonShape CreateBoxShape(const ColliderComponent& collider);
		static b2PolygonShape CreatePolygonShape(const ColliderComponent& collider);
	};

} // namespace Pillar
