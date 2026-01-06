#include "Pillar/Utils/AssetManager.h"
#include "Pillar/Renderer/Texture.h"
#include "Pillar/Logger.h"
#include <vector>

#ifdef PIL_WINDOWS
    #include <windows.h>
#endif

namespace Pillar {

    std::filesystem::path AssetManager::s_AssetsDirectory = "";
    std::shared_ptr<Texture2D> AssetManager::s_MissingTexture = nullptr;

    std::filesystem::path AssetManager::GetExecutableDirectory()
    {
#ifdef PIL_WINDOWS
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::filesystem::path exePath(buffer);
        return exePath.parent_path();
#else
        // For other platforms, implement platform-specific code
        return std::filesystem::current_path();
#endif
    }

    std::string AssetManager::GetAssetPath(const std::string& relativePath)
    {
        // Initialize assets directory if not set
        if (s_AssetsDirectory.empty())
        {
            // First try to find Sandbox/assets in the workspace (for development)
            std::filesystem::path exeDir = GetExecutableDirectory();
            
            // Navigate up to find workspace root: bin/Debug-x64/Sandbox -> ../../..
            std::filesystem::path workspaceRoot = exeDir.parent_path().parent_path().parent_path();
            std::filesystem::path sandboxAssets = workspaceRoot / "Sandbox" / "assets";
            
            if (std::filesystem::exists(sandboxAssets))
            {
                s_AssetsDirectory = sandboxAssets;
                PIL_CORE_INFO("AssetManager: Using workspace assets directory: {0}", s_AssetsDirectory.string());
            }
            else
            {
                // Fallback to assets next to executable (for distribution)
                s_AssetsDirectory = exeDir / "assets";
                PIL_CORE_INFO("AssetManager: Using executable assets directory: {0}", s_AssetsDirectory.string());
            }
        }

        // First, check if the path exists as-is (absolute or already correct relative)
        std::filesystem::path directPath(relativePath);
        if (std::filesystem::exists(directPath))
        {
            PIL_CORE_TRACE("AssetManager: Found asset at direct path: {0}", directPath.string());
            return directPath.string();
        }

        // Check in assets directory
        std::filesystem::path assetsPath = s_AssetsDirectory / relativePath;
        if (std::filesystem::exists(assetsPath))
        {
            PIL_CORE_TRACE("AssetManager: Found asset at: {0}", assetsPath.string());
            return assetsPath.string();
        }

        // If it's a texture, check in textures subdirectory
        std::filesystem::path texturesPath = s_AssetsDirectory / "textures" / relativePath;
        if (std::filesystem::exists(texturesPath))
        {
            PIL_CORE_TRACE("AssetManager: Found texture at: {0}", texturesPath.string());
            return texturesPath.string();
        }

        // Asset not found, log warning and return original path
        PIL_CORE_WARN("AssetManager: Could not find asset '{0}'. Searched in:", relativePath);
        PIL_CORE_WARN("  - {0}", directPath.string());
        PIL_CORE_WARN("  - {0}", assetsPath.string());
        PIL_CORE_WARN("  - {0}", texturesPath.string());
        
        return relativePath;
    }

    std::string AssetManager::GetTexturePath(const std::string& textureName)
    {
        // Initialize assets directory if not set
        if (s_AssetsDirectory.empty())
        {
            // First try to find Sandbox/assets in the workspace (for development)
            std::filesystem::path exeDir = GetExecutableDirectory();
            
            // Navigate up to find workspace root: bin/Debug-x64/Sandbox -> ../../..
            std::filesystem::path workspaceRoot = exeDir.parent_path().parent_path().parent_path();
            std::filesystem::path sandboxAssets = workspaceRoot / "Sandbox" / "assets";
            
            if (std::filesystem::exists(sandboxAssets))
            {
                s_AssetsDirectory = sandboxAssets;
                PIL_CORE_INFO("AssetManager: Using workspace assets directory: {0}", s_AssetsDirectory.string());
            }
            else
            {
                // Fallback to assets next to executable (for distribution)
                s_AssetsDirectory = exeDir / "assets";
                PIL_CORE_INFO("AssetManager: Using executable assets directory: {0}", s_AssetsDirectory.string());
            }
        }

        // First check if the path exists as-is
        std::filesystem::path directPath(textureName);
        if (std::filesystem::exists(directPath))
        {
            PIL_CORE_TRACE("AssetManager: Found texture at direct path: {0}", directPath.string());
            return directPath.string();
        }

        // Check in assets/textures directory
        std::filesystem::path texturesPath = s_AssetsDirectory / "textures" / textureName;
        if (std::filesystem::exists(texturesPath))
        {
            PIL_CORE_TRACE("AssetManager: Found texture at: {0}", texturesPath.string());
            return texturesPath.string();
        }

        // Check in root assets directory as fallback
        std::filesystem::path assetsPath = s_AssetsDirectory / textureName;
        if (std::filesystem::exists(assetsPath))
        {
            PIL_CORE_TRACE("AssetManager: Found texture at: {0}", assetsPath.string());
            return assetsPath.string();
        }

        // Texture not found
        PIL_CORE_WARN("AssetManager: Could not find texture '{0}'. Searched in:", textureName);
        PIL_CORE_WARN("  - {0}", directPath.string());
        PIL_CORE_WARN("  - {0}", texturesPath.string());
        PIL_CORE_WARN("  - {0}", assetsPath.string());
        
        return textureName;
    }

    std::string AssetManager::GetAudioPath(const std::string& audioName)
    {
        // Initialize assets directory if not set
        if (s_AssetsDirectory.empty())
        {
            std::filesystem::path exeDir = GetExecutableDirectory();
            std::filesystem::path workspaceRoot = exeDir.parent_path().parent_path().parent_path();
            std::filesystem::path sandboxAssets = workspaceRoot / "Sandbox" / "assets";

            if (std::filesystem::exists(sandboxAssets))
            {
                s_AssetsDirectory = sandboxAssets;
                PIL_CORE_INFO("AssetManager: Using workspace assets directory: {0}", s_AssetsDirectory.string());
            }
            else
            {
                s_AssetsDirectory = exeDir / "assets";
                PIL_CORE_INFO("AssetManager: Using executable assets directory: {0}", s_AssetsDirectory.string());
            }
        }

        std::filesystem::path directPath(audioName);
        if (std::filesystem::exists(directPath))
        {
            PIL_CORE_TRACE("AssetManager: Found audio at direct path: {0}", directPath.string());
            return directPath.string();
        }

        std::filesystem::path audioPath = s_AssetsDirectory / "audio" / audioName;
        if (std::filesystem::exists(audioPath))
        {
            PIL_CORE_TRACE("AssetManager: Found audio at: {0}", audioPath.string());
            return audioPath.string();
        }

        std::filesystem::path sfxPath = s_AssetsDirectory / "audio" / "sfx" / audioName;
        if (std::filesystem::exists(sfxPath))
        {
            PIL_CORE_TRACE("AssetManager: Found audio at: {0}", sfxPath.string());
            return sfxPath.string();
        }

        std::filesystem::path musicPath = s_AssetsDirectory / "audio" / "music" / audioName;
        if (std::filesystem::exists(musicPath))
        {
            PIL_CORE_TRACE("AssetManager: Found audio at: {0}", musicPath.string());
            return musicPath.string();
        }

        std::filesystem::path assetsPath = s_AssetsDirectory / audioName;
        if (std::filesystem::exists(assetsPath))
        {
            PIL_CORE_TRACE("AssetManager: Found audio at: {0}", assetsPath.string());
            return assetsPath.string();
        }

        PIL_CORE_WARN("AssetManager: Could not find audio '{0}'. Searched in:", audioName);
        PIL_CORE_WARN("  - {0}", directPath.string());
        PIL_CORE_WARN("  - {0}", audioPath.string());
        PIL_CORE_WARN("  - {0}", sfxPath.string());
        PIL_CORE_WARN("  - {0}", musicPath.string());
        PIL_CORE_WARN("  - {0}", assetsPath.string());

        return audioName;
    }

    std::string AssetManager::GetSFXPath(const std::string& sfxName)
    {
        if (s_AssetsDirectory.empty())
        {
            GetAssetsDirectory();
        }

        std::filesystem::path directPath(sfxName);
        if (std::filesystem::exists(directPath))
        {
            return directPath.string();
        }

        std::filesystem::path sfxPath = s_AssetsDirectory / "audio" / "sfx" / sfxName;
        if (std::filesystem::exists(sfxPath))
        {
            PIL_CORE_TRACE("AssetManager: Found SFX at: {0}", sfxPath.string());
            return sfxPath.string();
        }

        return GetAudioPath(sfxName);
    }

    std::string AssetManager::GetMusicPath(const std::string& musicName)
    {
        if (s_AssetsDirectory.empty())
        {
            GetAssetsDirectory();
        }

        std::filesystem::path directPath(musicName);
        if (std::filesystem::exists(directPath))
        {
            return directPath.string();
        }

        std::filesystem::path musicPath = s_AssetsDirectory / "audio" / "music" / musicName;
        if (std::filesystem::exists(musicPath))
        {
            PIL_CORE_TRACE("AssetManager: Found music at: {0}", musicPath.string());
            return musicPath.string();
        }

        return GetAudioPath(musicName);
    }

    void AssetManager::SetAssetsDirectory(const std::string& path)
    {
        s_AssetsDirectory = path;
        PIL_CORE_INFO("AssetManager: Assets directory changed to: {0}", s_AssetsDirectory.string());
    }

    std::string AssetManager::GetAssetsDirectory()
    {
        if (s_AssetsDirectory.empty())
        {
            // First try to find Sandbox/assets in the workspace (for development)
            std::filesystem::path exeDir = GetExecutableDirectory();
            
            // Navigate up to find workspace root: bin/Debug-x64/Sandbox -> ../../..
            std::filesystem::path workspaceRoot = exeDir.parent_path().parent_path().parent_path();
            std::filesystem::path sandboxAssets = workspaceRoot / "Sandbox" / "assets";
            
            if (std::filesystem::exists(sandboxAssets))
            {
                s_AssetsDirectory = sandboxAssets;
            }
            else
            {
                // Fallback to assets next to executable
                s_AssetsDirectory = exeDir / "assets";
            }
        }
        return s_AssetsDirectory.string();
    }

    void AssetManager::InitializeMissingTexture()
    {
        if (s_MissingTexture)
            return; // Already initialized

        PIL_CORE_INFO("AssetManager: Creating missing texture placeholder...");

        // Create 64x64 pink/black checkerboard texture
        const uint32_t width = 64;
        const uint32_t height = 64;
        const uint32_t checkSize = 8; // 8x8 pixel checks
        std::vector<uint8_t> pixels(width * height * 4);

        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                // Determine if this pixel is in a "pink" or "black" check
                bool isPink = ((x / checkSize) + (y / checkSize)) % 2 == 0;

                uint32_t index = (y * width + x) * 4;
                if (isPink)
                {
                    pixels[index + 0] = 255; // R
                    pixels[index + 1] = 0;   // G
                    pixels[index + 2] = 255; // B
                    pixels[index + 3] = 255; // A
                }
                else
                {
                    pixels[index + 0] = 0;   // R
                    pixels[index + 1] = 0;   // G
                    pixels[index + 2] = 0;   // B
                    pixels[index + 3] = 255; // A
                }
            }
        }

        // Create texture from pixel data
        s_MissingTexture = Texture2D::Create(width, height);
        s_MissingTexture->SetData(pixels.data(), width * height * 4);

        PIL_CORE_INFO("AssetManager: Missing texture created (64x64 pink/black checkerboard)");
    }

    std::shared_ptr<Texture2D> AssetManager::GetMissingTexture()
    {
        if (!s_MissingTexture)
        {
            InitializeMissingTexture();
        }
        return s_MissingTexture;
    }

}
