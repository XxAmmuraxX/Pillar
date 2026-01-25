#include "Pillar/Renderer/VertexArray.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Logger.h"


namespace Pillar {

    std::shared_ptr<VertexArray> VertexArray::Create()
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return std::make_shared<OpenGLVertexArray>();
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}
