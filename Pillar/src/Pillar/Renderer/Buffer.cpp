#include "Pillar/Renderer/Buffer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Logger.h"


namespace Pillar {

    VertexBuffer* VertexBuffer::Create(float* vertices, uint32_t size)
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return new OpenGLVertexBuffer(vertices, size);
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    IndexBuffer* IndexBuffer::Create(uint32_t* indices, uint32_t count)
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return new OpenGLIndexBuffer(indices, count);
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}
