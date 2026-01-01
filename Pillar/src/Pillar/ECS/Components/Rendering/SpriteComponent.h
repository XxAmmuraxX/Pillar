#pragma once

#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

	enum class SpriteLayer : int
	{
		Background = -10,
		Gameplay = 0,
		UI = 10
	};

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
		std::string TexturePath;  // Path for serialization and editor
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec2 Size = { 1.0f, 1.0f };
		glm::vec2 TexCoordMin = { 0.0f, 0.0f };
		glm::vec2 TexCoordMax = { 1.0f, 1.0f };
		float ZIndex = 0.0f;
		bool FlipX = false;  // Flip sprite horizontally
		bool FlipY = false;  // Flip sprite vertically

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

		void SetUVRect(const glm::vec2& pxMin, const glm::vec2& pxMax, float sheetWidth, float sheetHeight)
		{
			if (sheetWidth <= 0.0f || sheetHeight <= 0.0f)
				return;

			glm::vec2 invSize = { 1.0f / sheetWidth, 1.0f / sheetHeight };
			TexCoordMin = pxMin * invSize;
			TexCoordMax = pxMax * invSize;
		}

		void SetUVFromGrid(int column, int row, float cellWidth, float cellHeight, float sheetWidth, float sheetHeight)
		{
			glm::vec2 pxMin(column * cellWidth, row * cellHeight);
			glm::vec2 pxMax = pxMin + glm::vec2(cellWidth, cellHeight);
			SetUVRect(pxMin, pxMax, sheetWidth, sheetHeight);
		}

		void SetLayer(SpriteLayer layer)
		{
			ZIndex = static_cast<float>(layer);
		}
	};

} // namespace Pillar
