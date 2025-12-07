#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

	/**
	 * @brief Component for rendering 2D sprites
	 * 
	 * Supports both basic rendering and batch rendering systems.
	 * Can be used with or without animation.
	 */
	struct PIL_API SpriteComponent
	{
		std::shared_ptr<Texture2D> Texture = nullptr;
		glm::vec4 Color = glm::vec4(1.0f); // White by default
		glm::vec2 Size = glm::vec2(1.0f);  // 1x1 unit quad

		// Texture coordinates (for sprite sheets / atlases)
		glm::vec2 TexCoordMin = glm::vec2(0.0f, 0.0f);
		glm::vec2 TexCoordMax = glm::vec2(1.0f, 1.0f);

		// Sorting/layering (for batch renderer sorting)
		int SortingLayer = 0;
		float ZIndex = 0.0f;

		// Animation state (managed by AnimationSystem)
		bool Animated = false;
		int CurrentFrame = 0;
		float FrameTime = 0.0f;

		SpriteComponent() = default;
		SpriteComponent(const SpriteComponent&) = default;
		
		SpriteComponent(std::shared_ptr<Texture2D> texture, 
		               const glm::vec4& color = glm::vec4(1.0f),
		               const glm::vec2& size = glm::vec2(1.0f))
			: Texture(texture), Color(color), Size(size)
		{
		}
	};

} // namespace Pillar
