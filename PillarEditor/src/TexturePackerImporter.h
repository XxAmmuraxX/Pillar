#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace PillarEditor {

    /**
     * @brief Represents a single sprite frame from TexturePacker
     * 
     * TexturePacker JSON format (hash variant):
     * {
     *   "frames": {
     *     "sprite_name.png": {
     *       "frame": { "x": 0, "y": 0, "w": 32, "h": 32 },
     *       "rotated": false,
     *       "trimmed": false,
     *       "spriteSourceSize": { "x": 0, "y": 0, "w": 32, "h": 32 },
     *       "sourceSize": { "w": 32, "h": 32 }
     *     }
     *   },
     *   "meta": {
     *     "image": "spritesheet.png",
     *     "size": { "w": 512, "h": 512 },
     *     "scale": "1"
     *   }
     * }
     */
    struct TexturePackerFrame
    {
        std::string Name;           // Frame name (sprite_name.png)
        
        // Frame rectangle in texture (pixel coordinates)
        int FrameX = 0;
        int FrameY = 0;
        int FrameW = 0;
        int FrameH = 0;
        
        // Sprite source info (for trimmed sprites)
        int SpriteSourceX = 0;      // Offset in original sprite
        int SpriteSourceY = 0;
        int SpriteSourceW = 0;      // Size in original sprite
        int SpriteSourceH = 0;
        
        // Original sprite size before trimming
        int SourceW = 0;
        int SourceH = 0;
        
        bool Rotated = false;       // If true, sprite is rotated 90° clockwise
        bool Trimmed = false;       // If true, transparent pixels were removed
        
        // Calculated UV coordinates (0-1 range)
        glm::vec2 UVMin = { 0.0f, 0.0f };
        glm::vec2 UVMax = { 1.0f, 1.0f };
    };

    /**
     * @brief Metadata from TexturePacker export
     */
    struct TexturePackerMetadata
    {
        std::string ImagePath;      // Path to texture atlas image
        int TextureWidth = 0;
        int TextureHeight = 0;
        std::string Scale = "1";
        std::string Format;
    };

    /**
     * @brief Parser for TexturePacker JSON exports
     */
    class TexturePackerImporter
    {
    public:
        /**
         * @brief Parse a TexturePacker JSON file
         * @param filePath Path to .json file
         * @return True if parsing succeeded
         */
        bool ParseFile(const std::string& filePath);

        /**
         * @brief Get parsed frames
         */
        const std::vector<TexturePackerFrame>& GetFrames() const { return m_Frames; }

        /**
         * @brief Get metadata
         */
        const TexturePackerMetadata& GetMetadata() const { return m_Metadata; }

        /**
         * @brief Get last error message
         */
        const std::string& GetError() const { return m_ErrorMessage; }

    private:
        void CalculateUVCoordinates(TexturePackerFrame& frame);

    private:
        std::vector<TexturePackerFrame> m_Frames;
        TexturePackerMetadata m_Metadata;
        std::string m_ErrorMessage;
    };

} // namespace PillarEditor
