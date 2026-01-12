#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Pillar {

    /**
     * @brief Represents a sub-region of a texture atlas
     * 
     * Contains UV coordinates and metadata for a single sprite
     * within a larger texture atlas (sprite sheet).
     */
    struct PIL_API SubTexture
    {
        glm::vec2 UVMin = { 0.0f, 0.0f };   // Bottom-left UV coordinate
        glm::vec2 UVMax = { 1.0f, 1.0f };   // Top-right UV coordinate
        
        // Optional: Original sprite dimensions (before packing)
        glm::vec2 SourceSize = { 0.0f, 0.0f };
        
        // Optional: Offset in original sprite (for trimmed sprites)
        glm::vec2 SourceOffset = { 0.0f, 0.0f };
        
        // Rotation flag (TexturePacker can rotate sprites 90° CW for better packing)
        bool Rotated = false;
        
        // Whether sprite was trimmed (transparent pixels removed)
        bool Trimmed = false;

        SubTexture() = default;

        /**
         * @brief Create a SubTexture from pixel coordinates
         * @param pixelMin Bottom-left corner in pixels
         * @param pixelMax Top-right corner in pixels
         * @param atlasWidth Atlas texture width in pixels
         * @param atlasHeight Atlas texture height in pixels
         */
        SubTexture(const glm::vec2& pixelMin, const glm::vec2& pixelMax,
                   float atlasWidth, float atlasHeight);

        /**
         * @brief Create a SubTexture from a grid cell
         * @param column Grid column (0-based)
         * @param row Grid row (0-based)
         * @param cellWidth Cell width in pixels
         * @param cellHeight Cell height in pixels
         * @param atlasWidth Atlas texture width in pixels
         * @param atlasHeight Atlas texture height in pixels
         * @param padding Optional padding in pixels
         * @param spacing Optional spacing between cells in pixels
         */
        static SubTexture CreateFromGrid(int column, int row,
                                         float cellWidth, float cellHeight,
                                         float atlasWidth, float atlasHeight,
                                         const glm::vec2& padding = { 0.0f, 0.0f },
                                         const glm::vec2& spacing = { 0.0f, 0.0f });

        /**
         * @brief Get the size of this sub-texture in atlas UV space
         */
        glm::vec2 GetUVSize() const { return UVMax - UVMin; }
    };

    /**
     * @brief Texture atlas containing multiple sprites in a single texture
     * 
     * A texture atlas (also called sprite sheet) packs multiple sprites
     * into a single texture to reduce draw calls and texture switches.
     * This is critical for performance in 2D games.
     * 
     * Usage:
     * ```cpp
     * auto atlas = TextureAtlas::Create("spritesheet.png");
     * atlas->AddSubTexture("player_idle", SubTexture::CreateFromGrid(0, 0, 32, 32, 512, 512));
     * atlas->AddSubTexture("player_walk", SubTexture::CreateFromGrid(1, 0, 32, 32, 512, 512));
     * 
     * // Draw sprite from atlas
     * auto region = atlas->GetSubTexture("player_idle");
     * Renderer2D::DrawQuad(position, size, atlas->GetTexture(), region.UVMin, region.UVMax);
     * ```
     */
    class PIL_API TextureAtlas
    {
    public:
        /**
         * @brief Create a texture atlas from an image file
         * @param path Path to atlas texture image
         */
        static std::shared_ptr<TextureAtlas> Create(const std::string& path);

        /**
         * @brief Create a texture atlas from an existing Texture2D
         * @param texture Shared pointer to texture
         */
        static std::shared_ptr<TextureAtlas> Create(std::shared_ptr<Texture2D> texture);

        /**
         * @brief Add a named sub-texture region to the atlas
         * @param name Unique identifier for this sprite
         * @param subTexture Sub-texture coordinates and metadata
         */
        void AddSubTexture(const std::string& name, const SubTexture& subTexture);

        /**
         * @brief Get a sub-texture by name
         * @param name Sprite identifier
         * @return SubTexture if found, or default SubTexture covering entire atlas
         */
        SubTexture GetSubTexture(const std::string& name) const;

        /**
         * @brief Check if atlas contains a named sub-texture
         * @param name Sprite identifier
         * @return True if sub-texture exists
         */
        bool HasSubTexture(const std::string& name) const;

        /**
         * @brief Remove a sub-texture by name
         * @param name Sprite identifier
         * @return True if sub-texture was removed
         */
        bool RemoveSubTexture(const std::string& name);

        /**
         * @brief Get all sub-texture names
         */
        std::vector<std::string> GetSubTextureNames() const;

        /**
         * @brief Get number of sub-textures in atlas
         */
        size_t GetSubTextureCount() const { return m_SubTextures.size(); }

        /**
         * @brief Get the underlying texture
         */
        std::shared_ptr<Texture2D> GetTexture() const { return m_Texture; }

        /**
         * @brief Load atlas from TexturePacker JSON format
         * @param jsonPath Path to TexturePacker JSON file
         * @return True if load succeeded
         */
        bool LoadFromTexturePacker(const std::string& jsonPath);

        /**
         * @brief Load atlas from Aseprite JSON format
         * @param jsonPath Path to Aseprite JSON export
         * @return True if load succeeded
         */
        bool LoadFromAseprite(const std::string& jsonPath);

        /**
         * @brief Load atlas from grid configuration
         * @param columns Number of columns
         * @param rows Number of rows
         * @param cellWidth Cell width in pixels
         * @param cellHeight Cell height in pixels
         * @param namePrefix Prefix for auto-generated sprite names (e.g., "frame_")
         * @param padding Optional padding in pixels
         * @param spacing Optional spacing between cells
         */
        void LoadFromGrid(int columns, int rows,
                         float cellWidth, float cellHeight,
                         const std::string& namePrefix = "sprite_",
                         const glm::vec2& padding = { 0.0f, 0.0f },
                         const glm::vec2& spacing = { 0.0f, 0.0f });

    private:
        TextureAtlas(std::shared_ptr<Texture2D> texture);

        std::shared_ptr<Texture2D> m_Texture;
        std::unordered_map<std::string, SubTexture> m_SubTextures;
    };

}
