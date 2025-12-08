#pragma once

#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

	/**
	 * @brief Component for rendering 2D sprites with batch support
	 * 
	 * Contains all data needed for efficient sprite rendering:
	 * - Texture reference (for texture batching)
	 * - Color tint
	 * - Size/scale
	 * - Texture coordinates (for sprite sheets)
	 * - Z-index (for sorting)
	 */
	struct SpriteComponent
	{
		std::shared_ptr<Texture2D> Texture;
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec2 Size = { 1.0f, 1.0f };
		glm::vec2 TexCoordMin = { 0.0f, 0.0f };
		glm::vec2 TexCoordMax = { 1.0f, 1.0f };
		float ZIndex = 0.0f;

		SpriteComponent() = default;
		SpriteComponent(const SpriteComponent&) = default;

		// Constructor with just color (no texture)
		SpriteComponent(const glm::vec4& color)
			: Color(color) {}

		// Constructor with texture
		SpriteComponent(std::shared_ptr<Texture2D> texture)
			: Texture(texture) {}

		// Constructor with texture and color tint
		SpriteComponent(std::shared_ptr<Texture2D> texture, const glm::vec4& color)
			: Texture(texture), Color(color) {}
	};

} // namespace Pillar
