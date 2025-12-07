#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Pillar {

	enum class ColliderType
	{
		Circle,
		Box,
		Polygon
	};

	// Data-only component (actual fixtures created by Box2DBodyFactory)
	struct ColliderComponent
	{
		ColliderType Type = ColliderType::Circle;

		// Shape parameters
		glm::vec2 Offset = { 0.0f, 0.0f };      // Local offset from body origin
		union {
			float Radius;                        // For Circle
			glm::vec2 HalfExtents;              // For Box
		};
		std::vector<glm::vec2> Vertices;        // For Polygon (optional)

		// Material
		float Density = 1.0f;
		float Friction = 0.3f;
		float Restitution = 0.0f;               // Bounciness

		// Collision filtering
		uint16_t CategoryBits = 0x0001;         // What am I?
		uint16_t MaskBits = 0xFFFF;             // What do I collide with?
		int16_t GroupIndex = 0;                 // Negative = never collide

		// Sensor flag
		bool IsSensor = false;                  // Trigger collisions only (no physics response)

		ColliderComponent() : Radius(0.5f) {}
		ColliderComponent(const ColliderComponent&) = default;

		// Convenience constructors
		static ColliderComponent Circle(float radius)
		{
			ColliderComponent c;
			c.Type = ColliderType::Circle;
			c.Radius = radius;
			return c;
		}

		static ColliderComponent Box(glm::vec2 halfExtents)
		{
			ColliderComponent c;
			c.Type = ColliderType::Box;
			c.HalfExtents = halfExtents;
			return c;
		}
	};

} // namespace Pillar
