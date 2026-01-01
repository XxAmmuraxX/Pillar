#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief Component for in-game camera (orthographic projection for 2D)
	 * 
	 * Defines camera properties for rendering the scene during gameplay.
	 * Only one camera should be marked as Primary at a time.
	 */
	struct CameraComponent
	{
		float OrthographicSize = 10.0f;  // Height of the orthographic view (in world units)
		float NearClip = -1.0f;          // Near clipping plane
		float FarClip = 1.0f;            // Far clipping plane
		bool Primary = true;             // Is this the primary/active camera?
		bool FixedAspectRatio = false;   // Keep aspect ratio fixed (for pixel-perfect games)

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;

		// Constructor with size
		CameraComponent(float size)
			: OrthographicSize(size) {}

		/**
		 * @brief Get the projection matrix for this camera
		 * @param aspectRatio Width/Height ratio of the viewport
		 * @return Orthographic projection matrix
		 */
		glm::mat4 GetProjectionMatrix(float aspectRatio) const
		{
			float orthoLeft = -OrthographicSize * aspectRatio * 0.5f;
			float orthoRight = OrthographicSize * aspectRatio * 0.5f;
			float orthoBottom = -OrthographicSize * 0.5f;
			float orthoTop = OrthographicSize * 0.5f;

			return glm::ortho(orthoLeft, orthoRight, orthoBottom, orthoTop, NearClip, FarClip);
		}

		/**
		 * @brief Get the view matrix based on entity transform
		 * @param position Camera position in world space
		 * @param rotation Camera rotation in radians
		 * @return View matrix (inverse transform)
		 */
		glm::mat4 GetViewMatrix(const glm::vec2& position, float rotation) const
		{
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
			transform = glm::rotate(transform, rotation, glm::vec3(0, 0, 1));

			return glm::inverse(transform);
		}
	};

} // namespace Pillar
