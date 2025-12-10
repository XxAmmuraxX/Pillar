#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>

namespace Pillar {

    /**
     * @brief Renderer2D Backend - High-Performance Batch Renderer
     * 
     * Provides a static API for 2D rendering using batched draw calls.
     * Accumulates quads into texture-based batches for optimal GPU performance.
     * 
     * Usage:
     *   Renderer2DBackend::Init();
     *   Renderer2DBackend::BeginScene(camera);
     *   Renderer2DBackend::DrawQuad(...);
     *   Renderer2DBackend::EndScene();
     */
    class PIL_API Renderer2DBackend
    {
    public:
        static void Init();
        static void Shutdown();

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
						 const glm::vec2& texCoordMax = glm::vec2(1.0f),
						 bool flipX = false, bool flipY = false);        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color);
        
        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color, 
                                   const std::shared_ptr<Texture2D>& texture);

        // Statistics
        static uint32_t GetDrawCallCount();
        static uint32_t GetQuadCount();
        static void ResetStats();
    };

} // namespace Pillar
