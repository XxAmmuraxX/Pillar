#include "Pillar/Utils/AssetManager.h"
#include "Pillar/Logger.h"

#ifdef PIL_WINDOWS
    #include <windows.h>
#endif

namespace Pillar {

    std::filesystem::path AssetManager::s_AssetsDirectory = "";

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

}
