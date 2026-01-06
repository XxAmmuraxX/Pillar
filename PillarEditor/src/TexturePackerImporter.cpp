#include "TexturePackerImporter.h"
#include "Panels/ConsolePanel.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace PillarEditor {

    bool TexturePackerImporter::ParseFile(const std::string& filePath)
    {
        m_Frames.clear();
        m_ErrorMessage.clear();

        try
        {
            // Read JSON file
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                m_ErrorMessage = "Failed to open file: " + filePath;
                ConsolePanel::Log(m_ErrorMessage, LogLevel::Error);
                return false;
            }

            nlohmann::json j;
            file >> j;
            file.close();

            // Parse metadata
            if (j.contains("meta"))
            {
                const auto& meta = j["meta"];
                
                if (meta.contains("image"))
                {
                    m_Metadata.ImagePath = meta["image"].get<std::string>();
                    
                    // Resolve relative path to absolute
                    std::filesystem::path jsonPath(filePath);
                    std::filesystem::path imagePath = jsonPath.parent_path() / m_Metadata.ImagePath;
                    m_Metadata.ImagePath = imagePath.string();
                }
                
                if (meta.contains("size"))
                {
                    m_Metadata.TextureWidth = meta["size"]["w"].get<int>();
                    m_Metadata.TextureHeight = meta["size"]["h"].get<int>();
                }
                
                if (meta.contains("scale"))
                {
                    m_Metadata.Scale = meta["scale"].get<std::string>();
                }
                
                if (meta.contains("format"))
                {
                    m_Metadata.Format = meta["format"].get<std::string>();
                }
            }

            // Parse frames (hash format)
            if (j.contains("frames") && j["frames"].is_object())
            {
                for (auto& [frameName, frameData] : j["frames"].items())
                {
                    TexturePackerFrame frame;
                    frame.Name = frameName;

                    // Frame rectangle
                    if (frameData.contains("frame"))
                    {
                        const auto& frameRect = frameData["frame"];
                        frame.FrameX = frameRect["x"].get<int>();
                        frame.FrameY = frameRect["y"].get<int>();
                        frame.FrameW = frameRect["w"].get<int>();
                        frame.FrameH = frameRect["h"].get<int>();
                    }

                    // Rotation
                    if (frameData.contains("rotated"))
                    {
                        frame.Rotated = frameData["rotated"].get<bool>();
                    }

                    // Trimmed flag
                    if (frameData.contains("trimmed"))
                    {
                        frame.Trimmed = frameData["trimmed"].get<bool>();
                    }

                    // Sprite source size (offset + size in original sprite)
                    if (frameData.contains("spriteSourceSize"))
                    {
                        const auto& spriteSource = frameData["spriteSourceSize"];
                        frame.SpriteSourceX = spriteSource["x"].get<int>();
                        frame.SpriteSourceY = spriteSource["y"].get<int>();
                        frame.SpriteSourceW = spriteSource["w"].get<int>();
                        frame.SpriteSourceH = spriteSource["h"].get<int>();
                    }

                    // Original source size
                    if (frameData.contains("sourceSize"))
                    {
                        const auto& sourceSize = frameData["sourceSize"];
                        frame.SourceW = sourceSize["w"].get<int>();
                        frame.SourceH = sourceSize["h"].get<int>();
                    }

                    // Calculate UV coordinates
                    CalculateUVCoordinates(frame);

                    m_Frames.push_back(frame);
                }
            }
            // Parse frames (array format)
            else if (j.contains("frames") && j["frames"].is_array())
            {
                for (const auto& frameData : j["frames"])
                {
                    TexturePackerFrame frame;
                    
                    if (frameData.contains("filename"))
                    {
                        frame.Name = frameData["filename"].get<std::string>();
                    }

                    // Frame rectangle
                    if (frameData.contains("frame"))
                    {
                        const auto& frameRect = frameData["frame"];
                        frame.FrameX = frameRect["x"].get<int>();
                        frame.FrameY = frameRect["y"].get<int>();
                        frame.FrameW = frameRect["w"].get<int>();
                        frame.FrameH = frameRect["h"].get<int>();
                    }

                    // Rotation
                    if (frameData.contains("rotated"))
                    {
                        frame.Rotated = frameData["rotated"].get<bool>();
                    }

                    // Trimmed flag
                    if (frameData.contains("trimmed"))
                    {
                        frame.Trimmed = frameData["trimmed"].get<bool>();
                    }

                    // Sprite source size
                    if (frameData.contains("spriteSourceSize"))
                    {
                        const auto& spriteSource = frameData["spriteSourceSize"];
                        frame.SpriteSourceX = spriteSource["x"].get<int>();
                        frame.SpriteSourceY = spriteSource["y"].get<int>();
                        frame.SpriteSourceW = spriteSource["w"].get<int>();
                        frame.SpriteSourceH = spriteSource["h"].get<int>();
                    }

                    // Original source size
                    if (frameData.contains("sourceSize"))
                    {
                        const auto& sourceSize = frameData["sourceSize"];
                        frame.SourceW = sourceSize["w"].get<int>();
                        frame.SourceH = sourceSize["h"].get<int>();
                    }

                    // Calculate UV coordinates
                    CalculateUVCoordinates(frame);

                    m_Frames.push_back(frame);
                }
            }

            ConsolePanel::Log("Parsed TexturePacker file: " + std::to_string(m_Frames.size()) + 
                            " frames from " + m_Metadata.ImagePath, LogLevel::Info);
            return true;
        }
        catch (const std::exception& e)
        {
            m_ErrorMessage = "Failed to parse TexturePacker JSON: " + std::string(e.what());
            ConsolePanel::Log(m_ErrorMessage, LogLevel::Error);
            return false;
        }
    }

    void TexturePackerImporter::CalculateUVCoordinates(TexturePackerFrame& frame)
    {
        if (m_Metadata.TextureWidth == 0 || m_Metadata.TextureHeight == 0)
            return;

        float texW = (float)m_Metadata.TextureWidth;
        float texH = (float)m_Metadata.TextureHeight;

        if (frame.Rotated)
        {
            // Rotated 90° clockwise - need to adjust UV coordinates
            // Note: This is a simplified version. Full support may need more complex handling.
            float uMin = frame.FrameX / texW;
            float vMin = frame.FrameY / texH;
            float uMax = (frame.FrameX + frame.FrameH) / texW; // Width becomes height
            float vMax = (frame.FrameY + frame.FrameW) / texH; // Height becomes width

            frame.UVMin = glm::vec2(uMin, 1.0f - vMax);
            frame.UVMax = glm::vec2(uMax, 1.0f - vMin);
        }
        else
        {
            // Normal orientation
            float uMin = frame.FrameX / texW;
            float vMin = frame.FrameY / texH;
            float uMax = (frame.FrameX + frame.FrameW) / texW;
            float vMax = (frame.FrameY + frame.FrameH) / texH;

            // Flip V coordinate for OpenGL (origin bottom-left)
            frame.UVMin = glm::vec2(uMin, 1.0f - vMax);
            frame.UVMax = glm::vec2(uMax, 1.0f - vMin);
        }
    }

} // namespace PillarEditor
