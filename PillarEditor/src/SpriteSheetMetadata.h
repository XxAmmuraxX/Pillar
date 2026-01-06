#pragma once

#include <string>
#include <glm/glm.hpp>

namespace PillarEditor {

    /**
     * @brief Stores grid configuration for sprite sheet textures
     * 
     * This metadata is saved per-texture as .spritesheet.json files
     * and auto-loaded when the texture is opened in the sprite sheet editor.
     */
    struct SpriteSheetMetadata
    {
        // Grid dimensions
        int Columns = 1;
        int Rows = 1;

        // Cell size in pixels
        glm::vec2 CellSize = { 32.0f, 32.0f };

        // Padding (outer border) in pixels
        glm::vec2 Padding = { 0.0f, 0.0f };

        // Spacing (gap between cells) in pixels
        glm::vec2 Spacing = { 0.0f, 0.0f };

        // Original texture dimensions (for validation)
        glm::vec2 TextureSize = { 0.0f, 0.0f };

        // Version for future format changes
        int Version = 1;

        SpriteSheetMetadata() = default;

        /**
         * @brief Save metadata to JSON file
         * @param filePath Path to .spritesheet.json file
         * @return True if save succeeded
         */
        bool SaveToFile(const std::string& filePath) const;

        /**
         * @brief Load metadata from JSON file
         * @param filePath Path to .spritesheet.json file
         * @return True if load succeeded
         */
        bool LoadFromFile(const std::string& filePath);

        /**
         * @brief Validate grid configuration against texture size
         * @return True if configuration is valid
         */
        bool IsValid() const;

        /**
         * @brief Get the metadata file path for a texture
         * @param texturePath Path to texture file
         * @return Path to .spritesheet.json file
         */
        static std::string GetMetadataPath(const std::string& texturePath);
    };

}
