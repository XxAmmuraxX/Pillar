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
	 * - Color tint/fill
	 * - Size/scale
	 * - Texture coordinates (for sprite sheets)
	 * - Z-index (for sorting)
	 */
	struct SpriteComponent
	{
		/// Texture to render. When nullptr, a solid color quad is drawn using Color.
		std::shared_ptr<Texture2D> Texture;
		
		/// Texture path for serialization and editor (set automatically when loading)
		std::string TexturePath;
		
		/// Color multiplier (RGBA). Behavior depends on whether Texture is set:
		/// - With Texture: Acts as a tint/modulate. White (1,1,1,1) = no tint.
		/// - Without Texture: Acts as solid fill color for the quad.
		/// Alpha component controls transparency (0 = invisible, 1 = opaque).
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		
		/// Size in world units (width, height)
		glm::vec2 Size = { 1.0f, 1.0f };
		
		/// UV coordinates for sprite sheet support (normalized 0-1)
		glm::vec2 TexCoordMin = { 0.0f, 0.0f };
		glm::vec2 TexCoordMax = { 1.0f, 1.0f };
		
		/// When true, AnimationSystem will not overwrite UVs
		bool LockUV = false;
		
		/// Depth for sorting (higher = rendered later/on top)
		float ZIndex = 0.0f;
		
		/// Flip sprite horizontally (mirrors texture UVs)
		bool FlipX = false;
		
		/// Flip sprite vertically (mirrors texture UVs)
		bool FlipY = false;
		
		/// Visibility flag (false = skip rendering)
		bool Visible = true;

		// === LAYER SYSTEM ===
		/// Named layer for render ordering (e.g., "Background", "Player", "UI")
		std::string Layer = "Default";
		
		/// Fine control within layer (-100 to 100, higher = rendered later)
		int OrderInLayer = 0;

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

		/**
		 * @brief Get final Z-index value for rendering
		 * 
		 * In the editor, ZIndex is kept in sync with the layer's baseZIndex + OrderInLayer offset.
		 * This method simply returns the cached ZIndex value.
		 * Returns: ZIndex (which already includes layer base + order offset)
		 */
		float GetFinalZIndex() const
		{
			// Return the cached ZIndex value (already computed in editor)
			// Editor updates this via RefreshAllSprites() or when layer changes
			return ZIndex;
		}

		// === PIXELS-PER-UNIT HELPERS ===
		// Note: These methods require EditorSettings to be available
		// If called from engine code (no editor), use manual PPU value

		/**
		 * @brief Set sprite size in pixels (converts to world units using PPU)
		 * @param pixelWidth Width in pixels
		 * @param pixelHeight Height in pixels
		 * @param pixelsPerUnit Pixels per unit ratio (default 100)
		 */
		void SetSizeInPixels(float pixelWidth, float pixelHeight, float pixelsPerUnit = 100.0f)
		{
			Size = glm::vec2(pixelWidth / pixelsPerUnit, pixelHeight / pixelsPerUnit);
		}

		/**
		 * @brief Get sprite size in pixels (converts from world units using PPU)
		 * @param pixelsPerUnit Pixels per unit ratio (default 100)
		 * @return Size in pixels
		 */
		glm::vec2 GetSizeInPixels(float pixelsPerUnit = 100.0f) const
		{
			return Size * pixelsPerUnit;
		}

		/**
		 * @brief Auto-size sprite to match texture dimensions
		 * @param pixelsPerUnit Pixels per unit ratio (default 100)
		 */
		void MatchTextureSize(float pixelsPerUnit = 100.0f)
		{
			if (Texture)
			{
				SetSizeInPixels(static_cast<float>(Texture->GetWidth()),
								static_cast<float>(Texture->GetHeight()),
								pixelsPerUnit);
			}
		}
	};

} // namespace Pillar
