#include "Pillar/Renderer/Texture.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Utils/AssetManager.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Pillar/Logger.h"


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

}
