#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Renderer/Buffer.h"
#include "Platform/OpenGL/OpenGLRenderAPI.h"
#include "Pillar/Logger.h"

namespace Pillar {

    std::unique_ptr<RenderAPI> Renderer::s_RenderAPI = nullptr;

    void Renderer::Init()
    {
        PIL_CORE_INFO("Initializing Renderer...");
        
        // Create the appropriate RenderAPI based on the selected API
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                s_RenderAPI = std::make_unique<OpenGLRenderAPI>();
                break;
            case RendererAPI::None:
                PIL_CORE_ERROR("RendererAPI::None is not supported!");
                break;
        }

        if (s_RenderAPI)
        {
            s_RenderAPI->Init();
            PIL_CORE_INFO("Renderer initialized successfully");
        }
    }

    void Renderer::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer...");
        s_RenderAPI.reset();
    }

    void Renderer::BeginScene()
    {
        // In the future, this will handle camera setup, lighting, etc.
    }

    void Renderer::EndScene()
    {
        // In the future, this will handle final render pass, post-processing, etc.
    }

    void Renderer::SetClearColor(const glm::vec4& color)
    {
        s_RenderAPI->SetClearColor(color);
    }

    void Renderer::Clear()
    {
        s_RenderAPI->Clear();
    }

    void Renderer::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        s_RenderAPI->SetViewport(x, y, width, height);
    }

    void Renderer::Submit(const VertexArray* vertexArray)
    {
        vertexArray->Bind();
        s_RenderAPI->DrawIndexed(vertexArray);
    }

}
