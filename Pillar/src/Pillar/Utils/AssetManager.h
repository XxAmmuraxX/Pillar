#pragma once

#include "Pillar/Core.h"
#include <string>
#include <filesystem>

namespace Pillar {

    /**
     * @brief AssetManager handles asset path resolution and file loading
     * 
     * Searches for assets in Sandbox/assets/ folder in the workspace (for development),
     * falling back to assets/ folder next to the executable (for distribution).
     * For textures, it specifically checks assets/textures/ subdirectory.
     */
    class PIL_API AssetManager
    {
    public:
        /**
         * @brief Resolves an asset path, checking multiple locations
         * @param relativePath The relative path to the asset (e.g., "my_texture.png")
         * @return Full path to the asset if found, otherwise returns the original path
         */
        static std::string GetAssetPath(const std::string& relativePath);

        /**
         * @brief Resolves a texture asset path
         * @param textureName The texture filename (e.g., "my_texture.png")
         * @return Full path to the texture if found in assets/textures/, otherwise returns original
         */
        static std::string GetTexturePath(const std::string& textureName);

        /**
         * @brief Sets the base assets directory (overrides automatic detection)
         * @param path The base assets directory path
         */
        static void SetAssetsDirectory(const std::string& path);

        /**
         * @brief Gets the current base assets directory
         * @return The base assets directory path
         */
        static std::string GetAssetsDirectory();

        /**
         * @brief Gets the directory containing the executable
         * @return Path to the directory containing the running executable
         */
        static std::filesystem::path GetExecutableDirectory();

    private:
        static std::filesystem::path s_AssetsDirectory;
    };

}
