#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>

namespace Pillar {

    /**
     * @brief Renderer2D Backend Facade
     * 
     * Provides a unified interface to switch between different 2D rendering implementations:
     * - Basic: Original immediate-mode renderer (existing Renderer2D)
     * - Batch: High-performance batch renderer (BatchRenderer2D)
     * 
     * Usage:
     *   Renderer2DBackend::Init(Renderer2DBackend::API::Batch);
     *   Renderer2DBackend::BeginScene(camera);
     *   Renderer2DBackend::DrawQuad(...);
     *   Renderer2DBackend::EndScene();
     */
    class PIL_API Renderer2DBackend
    {
    public:
        enum class API
        {
            Basic = 0,  // Original Renderer2D (immediate mode)
            Batch = 1   // BatchRenderer2D (optimized batching)
        };

        static void Init(API api = API::Batch);
        static void Shutdown();
        static void SetAPI(API api);
        static API GetAPI() { return s_CurrentAPI; }

        // Scene management
        static void BeginScene(const OrthographicCamera& camera);
        static void EndScene();

        // Draw commands
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                           const glm::vec4& color);
        
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                           const glm::vec4& color, const std::shared_ptr<Texture2D>& texture);
        
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                           const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                           const glm::vec2& texCoordMin = glm::vec2(0.0f), 
                           const glm::vec2& texCoordMax = glm::vec2(1.0f));

        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color);
        
        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color, 
                                   const std::shared_ptr<Texture2D>& texture);

        // Statistics
        static uint32_t GetDrawCallCount();
        static uint32_t GetQuadCount();
        static void ResetStats();

    private:
        static API s_CurrentAPI;
    };

} // namespace Pillar
