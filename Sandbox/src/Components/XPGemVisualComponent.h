#pragma once

#include <glm/glm.hpp>

namespace Pillar {

	/**
	 * @brief XP gem sizes for visual variety
	 */
	enum class GemSize
	{
		Small,   // 1 XP
		Medium,  // 5 XP
		Large    // 10 XP
	};

	/**
	 * @brief Extended XP Gem component with visual data
	 * Part of Arena Protocol showcase
	 */
	struct XPGemVisualComponent
	{
		GemSize Size = GemSize::Small;
		float BobTimer = 0.0f;         // For sine wave bobbing
		float BobSpeed = 2.0f;
		float BobAmplitude = 0.1f;
		float RotationSpeed = 90.0f;   // Degrees per second
		glm::vec2 BasePosition = glm::vec2(0.0f);  // Original position for bobbing

		XPGemVisualComponent() = default;
		XPGemVisualComponent(const XPGemVisualComponent&) = default;
		XPGemVisualComponent(GemSize size) : Size(size) 
		{
			// Set speed based on size
			switch (size)
			{
			case GemSize::Small:  RotationSpeed = 90.0f; break;
			case GemSize::Medium: RotationSpeed = 120.0f; break;
			case GemSize::Large:  RotationSpeed = 150.0f; break;
			}
		}
	};

} // namespace Pillar
