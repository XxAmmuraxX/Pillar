#pragma once
#include <cmath>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
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

		void MarkDirty() const { Dirty = true; }

		void Reset()
		{
			Position = { 0.0f, 0.0f };
			Rotation = 0.0f;
			Scale = { 1.0f, 1.0f };
			CachedTransform = glm::mat4(1.0f);
			Dirty = true;
		}

		void SetPosition(const glm::vec2& position)
		{
			Position = position;
			Dirty = true;
		}

		void SetPosition(float x, float y)
		{
			Position = { x, y };
			Dirty = true;
		}

		void Translate(const glm::vec2& delta)
		{
			Position += delta;
			Dirty = true;
		}

		void Translate(float dx, float dy)
		{
			Position += glm::vec2(dx, dy);
			Dirty = true;
		}

		void SetRotation(float rotationRadians)
		{
			Rotation = rotationRadians;
			Dirty = true;
		}

		void SetRotationDegrees(float rotationDegrees)
		{
			Rotation = glm::radians(rotationDegrees);
			Dirty = true;
		}

		void Rotate(float deltaRadians)
		{
			Rotation += deltaRadians;
			Dirty = true;
		}

		void RotateDegrees(float deltaDegrees)
		{
			Rotation += glm::radians(deltaDegrees);
			Dirty = true;
		}

		void SetScale(const glm::vec2& scale)
		{
			Scale = scale;
			Dirty = true;
		}

		void SetScale(float uniformScale)
		{
			Scale = glm::vec2(uniformScale);
			Dirty = true;
		}

		void SetScale(float x, float y)
		{
			Scale = glm::vec2(x, y);
			Dirty = true;
		}

		void ScaleBy(const glm::vec2& factor)
		{
			Scale *= factor;
			Dirty = true;
		}

		void ScaleBy(float factor)
		{
			Scale *= factor;
			Dirty = true;
		}

		void SetTRS(const glm::vec2& position, float rotationRadians, const glm::vec2& scale)
		{
			Position = position;
			Rotation = rotationRadians;
			Scale = scale;
			Dirty = true;
		}

		glm::vec2 TransformPoint(const glm::vec2& local) const
		{
			glm::vec4 world = GetTransform() * glm::vec4(local, 0.0f, 1.0f);
			return { world.x, world.y };
		}

		glm::vec2 TransformDirection(const glm::vec2& direction) const
		{
			glm::vec4 world = GetTransform() * glm::vec4(direction, 0.0f, 0.0f);
			return { world.x, world.y };
		}

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

namespace Pillar::Transform2D
{
	inline glm::vec2 Forward(float rotationRadians)
	{
		return glm::vec2(std::cos(rotationRadians), std::sin(rotationRadians));
	}

	inline glm::vec2 Right(float rotationRadians)
	{
		return glm::vec2(-std::sin(rotationRadians), std::cos(rotationRadians));
	}

	inline glm::vec2 RotateAround(const glm::vec2& point, const glm::vec2& pivot, float angleRadians)
	{
		glm::vec2 offset = point - pivot;
		glm::vec2 rotated = glm::rotate(offset, angleRadians);
		return rotated + pivot;
	}

	inline float LookAtAngle(const glm::vec2& from, const glm::vec2& to)
	{
		glm::vec2 dir = to - from;
		return std::atan2(dir.y, dir.x);
	}
}
