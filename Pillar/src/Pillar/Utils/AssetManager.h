#pragma once

#include "Pillar/Core.h"
#include <string>
#include <filesystem>
#include <memory>

namespace Pillar {

    // Forward declaration
    class Texture2D;

    /**
     * @brief AssetManager handles asset path resolution and file loading
     *
     * Searches for assets in Sandbox/assets/ folder in the workspace (for development),
     * falling back to assets/ folder next to the executable (for distribution).
     * For textures, it specifically checks assets/textures/ subdirectory.
     * For audio, it specifically checks assets/audio/ subdirectory.
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
         * @brief Resolves an audio asset path
         * @param audioName The audio filename (e.g., "explosion.wav")
         * @return Full path to the audio file if found in assets/audio/, otherwise returns original
         */
        static std::string GetAudioPath(const std::string& audioName);

        /**
         * @brief Resolves a sound effect path (assets/audio/sfx/)
         * @param sfxName The sound effect filename
         * @return Full path to the SFX file if found
         */
        static std::string GetSFXPath(const std::string& sfxName);

        /**
         * @brief Resolves a music path (assets/audio/music/)
         * @param musicName The music filename
         * @return Full path to the music file if found
         */
        static std::string GetMusicPath(const std::string& musicName);

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

        /**
         * @brief Get the missing texture placeholder (pink checkerboard)
         * @return Shared pointer to missing texture (created on first call)
         */
        static std::shared_ptr<Texture2D> GetMissingTexture();

        /**
         * @brief Initialize the missing texture (called automatically)
         */
        static void InitializeMissingTexture();

    private:
        static std::filesystem::path s_AssetsDirectory;
        static std::shared_ptr<Texture2D> s_MissingTexture;
    };

}