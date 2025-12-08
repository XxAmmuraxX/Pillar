#include "Renderer2DBackend.h"
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Logger.h"

namespace Pillar {

    // Static members
    Renderer2DBackend::API Renderer2DBackend::s_CurrentAPI = API::Batch;

    // Internal state
    static IRenderer2D* s_ActiveRenderer = nullptr;
    static IRenderer2D* s_BatchRenderer = nullptr;
    static bool s_BasicRendererInitialized = false;

    // Adapter to wrap existing Renderer2D static API into IRenderer2D interface
    class BasicRenderer2DAdapter : public IRenderer2D
    {
    public:
        void BeginScene(const OrthographicCamera& camera) override
        {
            Renderer2D::BeginScene(camera);
        }

        void EndScene() override
        {
            Renderer2D::EndScene();
        }

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color) override
        {
            Renderer2D::DrawQuad(position, size, color);
        }

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture) override
        {
            if (texture)
                Renderer2D::DrawQuad(position, size, std::shared_ptr<Texture2D>(texture, [](Texture2D*){}), color);
            else
                Renderer2D::DrawQuad(position, size, color);
        }

        void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture,
                     const glm::vec2& texCoordMin, const glm::vec2& texCoordMax) override
        {
            // Basic renderer doesn't support custom tex coords, use standard DrawQuad
            if (texture)
                Renderer2D::DrawQuad(position, size, std::shared_ptr<Texture2D>(texture, [](Texture2D*){}), color);
            else
                Renderer2D::DrawQuad(position, size, color);
        }

        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color) override
        {
            // Basic renderer doesn't support rotation directly
            Renderer2D::DrawQuad(position, size, color);
        }

        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color, Texture2D* texture) override
        {
            // Basic renderer doesn't support rotation directly
            if (texture)
                Renderer2D::DrawQuad(position, size, std::shared_ptr<Texture2D>(texture, [](Texture2D*){}), color);
            else
                Renderer2D::DrawQuad(position, size, color);
        }

        uint32_t GetDrawCallCount() const override
        {
            // Basic renderer doesn't track stats
            return 0;
        }

        uint32_t GetQuadCount() const override
        {
            // Basic renderer doesn't track stats
            return 0;
        }

        void ResetStats() override
        {
            // No-op for basic renderer
        }
    };

    static BasicRenderer2DAdapter* s_BasicRendererAdapter = nullptr;

    void Renderer2DBackend::Init(API api)
    {
        PIL_CORE_INFO("Initializing Renderer2DBackend with API: {0}", 
                     api == API::Basic ? "Basic" : "Batch");

        // Initialize basic renderer adapter (wraps existing static Renderer2D)
        if (!s_BasicRendererInitialized)
        {
            s_BasicRendererAdapter = new BasicRenderer2DAdapter();
            s_BasicRendererInitialized = true;
        }

        // Initialize batch renderer
        if (!s_BatchRenderer)
        {
            s_BatchRenderer = BatchRenderer2D::Create();
        }

        // Set active API
        s_CurrentAPI = api;
        SetAPI(api);

        PIL_CORE_INFO("Renderer2DBackend initialized successfully");
    }

    void Renderer2DBackend::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer2DBackend...");

        if (s_BasicRendererInitialized)
        {
            delete s_BasicRendererAdapter;
            s_BasicRendererAdapter = nullptr;
            s_BasicRendererInitialized = false;
        }

        if (s_BatchRenderer)
        {
            // Don't call Shutdown() explicitly - destructor will do it
            delete s_BatchRenderer;
            s_BatchRenderer = nullptr;
        }

        s_ActiveRenderer = nullptr;
    }

    void Renderer2DBackend::SetAPI(API api)
    {
        s_CurrentAPI = api;

        switch (api)
        {
            case API::Basic:
                s_ActiveRenderer = s_BasicRendererAdapter;
                PIL_CORE_INFO("Switched to Basic Renderer2D");
                break;
            case API::Batch:
                s_ActiveRenderer = s_BatchRenderer;
                PIL_CORE_INFO("Switched to Batch Renderer2D");
                break;
        }
    }

    void Renderer2DBackend::BeginScene(const OrthographicCamera& camera)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->BeginScene(camera);
    }

    void Renderer2DBackend::EndScene()
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->EndScene();
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->DrawQuad(position, size, color);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->DrawQuad(position, size, color, texture.get());
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                    const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->DrawQuad(position, size, color, texture.get(), texCoordMin, texCoordMax);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, 
                                           const std::shared_ptr<Texture2D>& texture)
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->DrawRotatedQuad(position, size, rotation, color, texture.get());
    }

    uint32_t Renderer2DBackend::GetDrawCallCount()
    {
        if (s_ActiveRenderer)
            return s_ActiveRenderer->GetDrawCallCount();
        return 0;
    }

    uint32_t Renderer2DBackend::GetQuadCount()
    {
        if (s_ActiveRenderer)
            return s_ActiveRenderer->GetQuadCount();
        return 0;
    }

    void Renderer2DBackend::ResetStats()
    {
        if (s_ActiveRenderer)
            s_ActiveRenderer->ResetStats();
    }

} // namespace Pillar
