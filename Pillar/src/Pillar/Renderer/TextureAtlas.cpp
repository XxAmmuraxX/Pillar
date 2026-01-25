#include "TextureAtlas.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace Pillar {

    // ========================================================================
    // Internal Helper Functions
    // ========================================================================
    namespace {
        /**
         * @brief Parse a SubTexture from a TexturePacker JSON frame object
         */
        SubTexture ParseTexturePackerFrame(const json& frame, float atlasWidth, float atlasHeight)
        {
            SubTexture subTex;
            
            // Parse frame rectangle
            int x = frame["frame"]["x"].get<int>();
            int y = frame["frame"]["y"].get<int>();
            int w = frame["frame"]["w"].get<int>();
            int h = frame["frame"]["h"].get<int>();
            
            // OpenGL uses bottom-left origin, TexturePacker uses top-left
            glm::vec2 pixelMin(static_cast<float>(x), atlasHeight - static_cast<float>(y + h));
            glm::vec2 pixelMax(static_cast<float>(x + w), atlasHeight - static_cast<float>(y));
            
            subTex.UVMin = pixelMin / glm::vec2(atlasWidth, atlasHeight);
            subTex.UVMax = pixelMax / glm::vec2(atlasWidth, atlasHeight);
            
            // Parse optional data
            if (frame.contains("rotated"))
                subTex.Rotated = frame["rotated"].get<bool>();
            
            if (frame.contains("trimmed"))
                subTex.Trimmed = frame["trimmed"].get<bool>();
            
            if (frame.contains("sourceSize"))
            {
                subTex.SourceSize = glm::vec2(
                    frame["sourceSize"]["w"].get<float>(),
                    frame["sourceSize"]["h"].get<float>()
                );
            }
            
            if (frame.contains("spriteSourceSize"))
            {
                subTex.SourceOffset = glm::vec2(
                    frame["spriteSourceSize"]["x"].get<float>(),
                    frame["spriteSourceSize"]["y"].get<float>()
                );
            }
            
            return subTex;
        }

        /**
         * @brief Strip file extension from sprite name
         */
        std::string StripFileExtension(const std::string& filename)
        {
            size_t dotPos = filename.find_last_of('.');
            if (dotPos != std::string::npos)
                return filename.substr(0, dotPos);
            return filename;
        }

        /**
         * @brief Common structure for atlas JSON loading result
         */
        struct AtlasJsonData
        {
            json Root;
            float AtlasWidth = 0.0f;
            float AtlasHeight = 0.0f;
            bool Success = false;
        };

        /**
         * @brief Load and validate atlas JSON file (common to TexturePacker and Aseprite)
         * @param jsonPath Path to JSON file
         * @param formatName Human-readable format name for error messages
         * @return AtlasJsonData with parsed JSON and dimensions, or Success=false on error
         */
        AtlasJsonData LoadAtlasJson(const std::string& jsonPath, const char* formatName)
        {
            AtlasJsonData result;
            
            std::string resolvedPath = AssetManager::GetAssetPath(jsonPath);
            std::ifstream file(resolvedPath);
            if (!file.is_open())
            {
                PIL_CORE_ERROR("TextureAtlas: Failed to open {} JSON: '{}'", formatName, resolvedPath);
                return result;
            }

            try
            {
                file >> result.Root;
            }
            catch (const json::exception& e)
            {
                PIL_CORE_ERROR("TextureAtlas: Failed to parse JSON: {}", e.what());
                return result;
            }

            // Get texture dimensions from meta
            if (!result.Root.contains("meta") || !result.Root["meta"].contains("size"))
            {
                PIL_CORE_ERROR("TextureAtlas: Invalid {} JSON (missing meta.size)", formatName);
                return result;
            }

            result.AtlasWidth = static_cast<float>(result.Root["meta"]["size"]["w"].get<int>());
            result.AtlasHeight = static_cast<float>(result.Root["meta"]["size"]["h"].get<int>());

            // Validate frames key
            if (!result.Root.contains("frames"))
            {
                PIL_CORE_ERROR("TextureAtlas: Invalid {} JSON (missing frames)", formatName);
                return result;
            }

            result.Success = true;
            return result;
        }

        /**
         * @brief Parse UV coordinates from a frame object (common to both formats)
         */
        SubTexture ParseFrameUVs(const json& frame, float atlasWidth, float atlasHeight)
        {
            SubTexture subTex;
            
            int x = frame["frame"]["x"].get<int>();
            int y = frame["frame"]["y"].get<int>();
            int w = frame["frame"]["w"].get<int>();
            int h = frame["frame"]["h"].get<int>();
            
            // Both formats use top-left origin, convert to OpenGL bottom-left
            glm::vec2 pixelMin(static_cast<float>(x), atlasHeight - static_cast<float>(y + h));
            glm::vec2 pixelMax(static_cast<float>(x + w), atlasHeight - static_cast<float>(y));
            
            subTex.UVMin = pixelMin / glm::vec2(atlasWidth, atlasHeight);
            subTex.UVMax = pixelMax / glm::vec2(atlasWidth, atlasHeight);
            
            return subTex;
        }
    } // anonymous namespace

    // ========================================================================
    // SubTexture Implementation
    // ========================================================================

    SubTexture::SubTexture(const glm::vec2& pixelMin, const glm::vec2& pixelMax,
                           float atlasWidth, float atlasHeight)
    {
        if (atlasWidth <= 0.0f || atlasHeight <= 0.0f)
        {
            PIL_CORE_ERROR("TextureAtlas: Invalid atlas dimensions ({}, {})", atlasWidth, atlasHeight);
            return;
        }

        glm::vec2 invSize = { 1.0f / atlasWidth, 1.0f / atlasHeight };
        UVMin = pixelMin * invSize;
        UVMax = pixelMax * invSize;
    }

    SubTexture SubTexture::CreateFromGrid(int column, int row,
                                          float cellWidth, float cellHeight,
                                          float atlasWidth, float atlasHeight,
                                          const glm::vec2& padding,
                                          const glm::vec2& spacing)
    {
        // Calculate pixel coordinates with padding and spacing
        glm::vec2 pixelMin(
            padding.x + column * (cellWidth + spacing.x),
            padding.y + row * (cellHeight + spacing.y)
        );
        glm::vec2 pixelMax = pixelMin + glm::vec2(cellWidth, cellHeight);

        return SubTexture(pixelMin, pixelMax, atlasWidth, atlasHeight);
    }

    // ========================================================================
    // TextureAtlas Implementation
    // ========================================================================

    TextureAtlas::TextureAtlas(std::shared_ptr<Texture2D> texture)
        : m_Texture(texture)
    {
    }

    std::shared_ptr<TextureAtlas> TextureAtlas::Create(const std::string& path)
    {
        auto texture = Texture2D::Create(path);
        if (!texture)
        {
            PIL_CORE_ERROR("TextureAtlas: Failed to load texture from '{}'", path);
            return nullptr;
        }

        // Note: Cannot use make_shared due to private constructor (factory pattern)
        return std::shared_ptr<TextureAtlas>(new TextureAtlas(texture));
    }

    std::shared_ptr<TextureAtlas> TextureAtlas::Create(std::shared_ptr<Texture2D> texture)
    {
        if (!texture)
        {
            PIL_CORE_ERROR("TextureAtlas: Cannot create atlas from null texture");
            return nullptr;
        }

        // Note: Cannot use make_shared due to private constructor (factory pattern)
        return std::shared_ptr<TextureAtlas>(new TextureAtlas(texture));
    }

    void TextureAtlas::AddSubTexture(const std::string& name, const SubTexture& subTexture)
    {
        if (name.empty())
        {
            PIL_CORE_WARN("TextureAtlas: Cannot add sub-texture with empty name");
            return;
        }

        if (auto it = m_SubTextures.find(name); it != m_SubTextures.end())
        {
            PIL_CORE_WARN("TextureAtlas: Sub-texture '{}' already exists, overwriting", name);
        }

        m_SubTextures[name] = subTexture;
    }

    SubTexture TextureAtlas::GetSubTexture(const std::string& name) const
    {
        if (name.empty())
        {
            PIL_CORE_ERROR("TextureAtlas::GetSubTexture() called with empty name!");
            return SubTexture(); // Return default
        }

        if (auto it = m_SubTextures.find(name); it != m_SubTextures.end())
        {
            return it->second;
        }

        PIL_CORE_WARN("TextureAtlas: Sub-texture '{}' not found, returning default", name);
        return SubTexture(); // Default covers entire texture (0,0) to (1,1)
    }

    bool TextureAtlas::HasSubTexture(const std::string& name) const
    {
        return m_SubTextures.find(name) != m_SubTextures.end();
    }

    bool TextureAtlas::RemoveSubTexture(const std::string& name)
    {
        if (auto it = m_SubTextures.find(name); it != m_SubTextures.end())
        {
            m_SubTextures.erase(it);
            return true;
        }
        return false;
    }

    std::vector<std::string> TextureAtlas::GetSubTextureNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_SubTextures.size());
        for (const auto& [name, _] : m_SubTextures)
        {
            names.push_back(name);
        }
        return names;
    }

    bool TextureAtlas::LoadFromTexturePacker(const std::string& jsonPath)
    {
        auto atlasData = LoadAtlasJson(jsonPath, "TexturePacker");
        if (!atlasData.Success)
            return false;

        const json& frames = atlasData.Root["frames"];
        
        // Handle both array and object formats
        if (frames.is_object())
        {
            // Hash format: "frames": { "sprite1.png": {...}, "sprite2.png": {...} }
            for (const auto& [frameName, frame] : frames.items())
            {
                SubTexture subTex = ParseTexturePackerFrame(frame, atlasData.AtlasWidth, atlasData.AtlasHeight);
                AddSubTexture(StripFileExtension(frameName), subTex);
            }
        }
        else if (frames.is_array())
        {
            // Array format: "frames": [ {...}, {...} ]
            for (const auto& frame : frames)
            {
                if (!frame.contains("filename"))
                    continue;
                
                std::string frameName = frame["filename"].get<std::string>();
                SubTexture subTex = ParseTexturePackerFrame(frame, atlasData.AtlasWidth, atlasData.AtlasHeight);
                AddSubTexture(StripFileExtension(frameName), subTex);
            }
        }

        PIL_CORE_INFO("TextureAtlas: Loaded {} sprites from TexturePacker JSON", m_SubTextures.size());
        return true;
    }

    bool TextureAtlas::LoadFromAseprite(const std::string& jsonPath)
    {
        auto atlasData = LoadAtlasJson(jsonPath, "Aseprite");
        if (!atlasData.Success)
            return false;

        const json& frames = atlasData.Root["frames"];
        
        if (frames.is_object())
        {
            for (auto& [frameName, frame] : frames.items())
            {
                SubTexture subTex = ParseFrameUVs(frame, atlasData.AtlasWidth, atlasData.AtlasHeight);
                
                // Aseprite includes duration for animations
                // Store source size if available
                if (frame.contains("sourceSize"))
                {
                    subTex.SourceSize = glm::vec2(
                        frame["sourceSize"]["w"].get<float>(),
                        frame["sourceSize"]["h"].get<float>()
                    );
                }
                
                AddSubTexture(frameName, subTex);
            }
        }

        PIL_CORE_INFO("TextureAtlas: Loaded {} sprites from Aseprite JSON", m_SubTextures.size());
        return true;
    }

    void TextureAtlas::LoadFromGrid(int columns, int rows,
                                    float cellWidth, float cellHeight,
                                    const std::string& namePrefix,
                                    const glm::vec2& padding,
                                    const glm::vec2& spacing)
    {
        if (columns <= 0 || rows <= 0)
        {
            PIL_CORE_ERROR("TextureAtlas: Invalid grid dimensions ({}, {})", columns, rows);
            return;
        }

        if (cellWidth <= 0.0f || cellHeight <= 0.0f)
        {
            PIL_CORE_ERROR("TextureAtlas: Invalid cell size ({}, {})", cellWidth, cellHeight);
            return;
        }

        float atlasWidth = static_cast<float>(m_Texture->GetWidth());
        float atlasHeight = static_cast<float>(m_Texture->GetHeight());

        m_SubTextures.clear();

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < columns; ++col)
            {
                std::string name = namePrefix + std::to_string(row * columns + col);
                SubTexture subTex = SubTexture::CreateFromGrid(
                    col, row,
                    cellWidth, cellHeight,
                    atlasWidth, atlasHeight,
                    padding, spacing
                );
                AddSubTexture(name, subTex);
            }
        }

        PIL_CORE_INFO("TextureAtlas: Generated {} sprites from {}x{} grid", m_SubTextures.size(), columns, rows);
    }

}
