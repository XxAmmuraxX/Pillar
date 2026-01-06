#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Utils/AssetManager.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Pillar/Logger.h"
#include <filesystem>


namespace Pillar {

    std::shared_ptr<Texture2D> Texture2D::Create(uint32_t width, uint32_t height)
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return std::make_shared<OpenGLTexture2D>(width, height);
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    std::shared_ptr<Texture2D> Texture2D::Create(const std::string& path)
    {
        std::string resolvedPath = AssetManager::GetTexturePath(path);
        
        // Check if file exists
        if (!std::filesystem::exists(resolvedPath))
        {
            PIL_CORE_WARN("Texture not found: {0}, using missing texture placeholder", path);
            return AssetManager::GetMissingTexture();
        }
        
        // Try to load texture
        try
        {
            switch (RenderAPI::GetAPI())
            {
                case RendererAPI::OpenGL:
                    return std::make_shared<OpenGLTexture2D>(resolvedPath);
                case RendererAPI::None:
                    PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                    return nullptr;
            }

            PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
            return nullptr;
        }
        catch (const std::exception& e)
        {
            PIL_CORE_ERROR("Failed to load texture '{0}': {1}", path, e.what());
            PIL_CORE_WARN("Using missing texture placeholder");
            return AssetManager::GetMissingTexture();
        }
    }

}
