#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

namespace Pillar {

	struct TransformComponent
	{
		glm::vec2 Position = { 0.0f, 0.0f };
		float Rotation = 0.0f;      // Radians
		glm::vec2 Scale = { 1.0f, 1.0f };

		// Cached transform matrix for performance
		mutable glm::mat4 CachedTransform = glm::mat4(1.0f);
		mutable bool Dirty = true;

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec2& position)
			: Position(position) {}

		glm::mat4 GetTransform() const
		{
			if (Dirty)
			{
				glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation, glm::vec3(0, 0, 1));

				CachedTransform = glm::translate(glm::mat4(1.0f), glm::vec3(Position, 0.0f))
					* rotation
					* glm::scale(glm::mat4(1.0f), glm::vec3(Scale, 1.0f));
				
				Dirty = false;
			}
			return CachedTransform;
		}
	};

} // namespace Pillar
