#include "SpriteSheetMetadata.h"
#include "Panels/ConsolePanel.h"
#include <fstream>
#include <filesystem>

// Use nlohmann/json for JSON parsing (assuming it's available in the project)
#include <nlohmann/json.hpp>

namespace PillarEditor {

    bool SpriteSheetMetadata::SaveToFile(const std::string& filePath) const
    {
        try
        {
            nlohmann::json j;
            j["version"] = Version;
            j["columns"] = Columns;
            j["rows"] = Rows;
            j["cellSize"] = { CellSize.x, CellSize.y };
            j["padding"] = { Padding.x, Padding.y };
            j["spacing"] = { Spacing.x, Spacing.y };
            j["textureSize"] = { TextureSize.x, TextureSize.y };

            std::ofstream file(filePath);
            if (!file.is_open())
            {
                ConsolePanel::Log("Failed to open file for writing: " + filePath, LogLevel::Error);
                return false;
            }

            file << j.dump(4); // Pretty print with 4-space indent
            file.close();

            ConsolePanel::Log("Saved sprite sheet metadata: " + filePath, LogLevel::Info);
            return true;
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to save sprite sheet metadata: " + std::string(e.what()), LogLevel::Error);
            return false;
        }
    }

    bool SpriteSheetMetadata::LoadFromFile(const std::string& filePath)
    {
        try
        {
            std::ifstream file(filePath);
            if (!file.is_open())
            {
                // Not an error - file might not exist yet
                return false;
            }

            nlohmann::json j;
            file >> j;
            file.close();

            // Load values with validation
            Version = j.value("version", 1);
            Columns = j.value("columns", 1);
            Rows = j.value("rows", 1);

            if (j.contains("cellSize") && j["cellSize"].is_array() && j["cellSize"].size() == 2)
            {
                CellSize.x = j["cellSize"][0];
                CellSize.y = j["cellSize"][1];
            }

            if (j.contains("padding") && j["padding"].is_array() && j["padding"].size() == 2)
            {
                Padding.x = j["padding"][0];
                Padding.y = j["padding"][1];
            }

            if (j.contains("spacing") && j["spacing"].is_array() && j["spacing"].size() == 2)
            {
                Spacing.x = j["spacing"][0];
                Spacing.y = j["spacing"][1];
            }

            if (j.contains("textureSize") && j["textureSize"].is_array() && j["textureSize"].size() == 2)
            {
                TextureSize.x = j["textureSize"][0];
                TextureSize.y = j["textureSize"][1];
            }

            ConsolePanel::Log("Loaded sprite sheet metadata: " + filePath, LogLevel::Info);
            return true;
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to load sprite sheet metadata: " + std::string(e.what()), LogLevel::Error);
            return false;
        }
    }

    bool SpriteSheetMetadata::IsValid() const
    {
        // Check for positive dimensions
        if (Columns <= 0 || Rows <= 0)
            return false;

        if (CellSize.x <= 0.0f || CellSize.y <= 0.0f)
            return false;

        // Check if grid fits within texture (if texture size is set)
        if (TextureSize.x > 0.0f && TextureSize.y > 0.0f)
        {
            float totalWidth = Padding.x * 2 + Columns * CellSize.x + (Columns - 1) * Spacing.x;
            float totalHeight = Padding.y * 2 + Rows * CellSize.y + (Rows - 1) * Spacing.y;

            if (totalWidth > TextureSize.x + 0.01f || totalHeight > TextureSize.y + 0.01f)
                return false;
        }

        return true;
    }

    std::string SpriteSheetMetadata::GetMetadataPath(const std::string& texturePath)
    {
        std::filesystem::path path(texturePath);
        std::string metadataPath = path.parent_path().string() + "/" + path.stem().string() + ".spritesheet.json";
        return metadataPath;
    }

}
