#pragma once

#include <glm/glm.hpp>
#include <string>

namespace Pillar {

	/**
	 * @brief Represents a single frame in an animation sequence
	 * 
	 * Each frame specifies:
	 * - The texture file to display
	 * - How long to display it (in seconds)
	 * - UV coordinates for texture sampling
	 */
	struct AnimationFrame
	{
		std::string TexturePath = "";        // Path to sprite image file
		float Duration = 0.1f;               // Frame duration in seconds
		glm::vec2 UVMin = { 0.0f, 0.0f };   // UV coordinates (top-left)
		glm::vec2 UVMax = { 1.0f, 1.0f };   // UV coordinates (bottom-right)

		AnimationFrame() = default;

		AnimationFrame(const std::string& texturePath, float duration = 0.1f)
			: TexturePath(texturePath), Duration(duration) {}

		AnimationFrame(const std::string& texturePath, float duration,
			const glm::vec2& uvMin, const glm::vec2& uvMax)
			: TexturePath(texturePath), Duration(duration), UVMin(uvMin), UVMax(uvMax) {}
	};

} // namespace Pillar
