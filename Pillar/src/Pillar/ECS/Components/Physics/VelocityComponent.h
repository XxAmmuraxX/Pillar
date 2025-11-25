#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	// For Light Entities (no b2Body)
	// Movement is simple: pos += vel * dt
	struct VelocityComponent
	{
		glm::vec2 Velocity = { 0.0f, 0.0f };    // Units per second
		glm::vec2 Acceleration = { 0.0f, 0.0f }; // Gravity, etc.

		float Drag = 0.0f;                      // Linear damping (0 = no drag)
		float MaxSpeed = 1000.0f;               // Clamp velocity magnitude

		VelocityComponent() = default;
		VelocityComponent(const VelocityComponent&) = default;
		VelocityComponent(const glm::vec2& velocity) : Velocity(velocity) {}
	};

} // namespace Pillar
