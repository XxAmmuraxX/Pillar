#include "Renderer2DBackend.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Logger.h"

namespace Pillar {

    // Internal state - single batch renderer
    static IRenderer2D* s_BatchRenderer = nullptr;

    void Renderer2DBackend::Init()
    {
        PIL_CORE_INFO("Initializing Renderer2DBackend (Batch Renderer)");

        if (!s_BatchRenderer)
        {
            s_BatchRenderer = BatchRenderer2D::Create();
        }

        PIL_CORE_INFO("Renderer2DBackend initialized successfully");
    }

    void Renderer2DBackend::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer2DBackend...");

        if (s_BatchRenderer)
        {
            delete s_BatchRenderer;
            s_BatchRenderer = nullptr;
        }
    }

    void Renderer2DBackend::BeginScene(const OrthographicCamera& camera)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->BeginScene(camera);
    }

    void Renderer2DBackend::EndScene()
    {
        if (s_BatchRenderer)
            s_BatchRenderer->EndScene();
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get());
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                    bool flipX, bool flipY)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawQuad(position, size, color, texture.get(), texCoordMin, texCoordMax, flipX, flipY);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture)
    {
        if (s_BatchRenderer)
            s_BatchRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get());
    }

    uint32_t Renderer2DBackend::GetDrawCallCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetDrawCallCount();
        return 0;
    }

    uint32_t Renderer2DBackend::GetQuadCount()
    {
        if (s_BatchRenderer)
            return s_BatchRenderer->GetQuadCount();
        return 0;
    }

    void Renderer2DBackend::ResetStats()
    {
        if (s_BatchRenderer)
            s_BatchRenderer->ResetStats();
    }

} // namespace Pillar
