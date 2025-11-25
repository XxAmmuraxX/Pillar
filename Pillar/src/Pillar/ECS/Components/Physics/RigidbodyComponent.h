#pragma once

#include <box2d/box2d.h>

namespace Pillar {

	// CRITICAL: This component means "this entity is a Heavy Entity"
	// Presence of this component = entity has a b2Body
	struct RigidbodyComponent
	{
		b2Body* Body = nullptr;     // Owned by b2World, cleaned up by on_destroy listener

		b2BodyType BodyType = b2_dynamicBody;
		bool FixedRotation = false; // Prevent rotation (useful for characters)
		float GravityScale = 1.0f;

		RigidbodyComponent() = default;
		RigidbodyComponent(b2BodyType type) : BodyType(type) {}
		RigidbodyComponent(const RigidbodyComponent&) = delete; // No copy (b2Body* is unique)
		RigidbodyComponent& operator=(const RigidbodyComponent&) = delete;
	};

} // namespace Pillar
