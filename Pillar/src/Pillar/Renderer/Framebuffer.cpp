#include "Pillar/Renderer/Framebuffer.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Platform/OpenGL/OpenGLFramebuffer.h"
#include "Pillar/Logger.h"

namespace Pillar {

    std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return std::make_shared<OpenGLFramebuffer>(spec);
            case RendererAPI::None:
                PIL_CORE_ERROR("RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ERROR("Unknown RendererAPI!");
        return nullptr;
    }

}
