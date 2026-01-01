#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <optional>
#include <memory>

// Forward declaration of GL state guard for depth control
// to keep header free of GL includes.

namespace Pillar {

    struct TransformComponent;
    struct SpriteComponent;

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
                            const glm::vec4& color);

        static void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                            const std::shared_ptr<Texture2D>& texture);

        static void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                            const glm::vec4& color, const std::shared_ptr<Texture2D>& texture,
                            const glm::vec2& texCoordMin = glm::vec2(0.0f), 
                            const glm::vec2& texCoordMax = glm::vec2(1.0f),
                            bool flipX = false, bool flipY = false);

        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                  float rotation, const glm::vec4& color);
        
        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                  float rotation, const glm::vec4& color, 
                                  const std::shared_ptr<Texture2D>& texture);

        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                  float rotation, const glm::vec4& color);

        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                  float rotation, const glm::vec4& color, 
                                  const std::shared_ptr<Texture2D>& texture,
                                  const glm::vec2& texCoordMin = glm::vec2(0.0f), 
                                  const glm::vec2& texCoordMax = glm::vec2(1.0f),
                                  bool flipX = false, bool flipY = false);

        // Debug helpers
        static void DrawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float thickness = 1.0f);
        static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float thickness = 1.0f);
        static void DrawRect(const glm::vec2& center, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f);
        static void DrawRect(const glm::vec3& center, const glm::vec2& size, const glm::vec4& color, float thickness = 1.0f);
        static void DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 24, float thickness = 1.0f);
        static void DrawCircle(const glm::vec3& center, float radius, const glm::vec4& color, int segments = 24, float thickness = 1.0f);

        // ECS convenience
        static void DrawSprite(const TransformComponent& transform, const SpriteComponent& sprite);

        // Scoped depth helper to quickly disable depth writes/tests for 2D overlays.
        class ScopedDepthState
        {
        public:
            ScopedDepthState(bool enableDepthTest, bool enableDepthWrite);
            ScopedDepthState(const ScopedDepthState&) = delete;
            ScopedDepthState& operator=(const ScopedDepthState&) = delete;
            ScopedDepthState(ScopedDepthState&&) = delete;
            ScopedDepthState& operator=(ScopedDepthState&&) = delete;
            ~ScopedDepthState();

            static ScopedDepthState DepthWriteDisabled(bool keepDepthTest = true);

        private:
            bool m_PreviousDepthTest;
            bool m_PreviousDepthWrite;
        };

        // General render state guard for depth test/write and blending
        class ScopedRenderState
        {
        public:
            ScopedRenderState(std::optional<bool> depthTestEnabled,
                              std::optional<bool> depthWriteEnabled,
                              std::optional<bool> blendingEnabled);
            ScopedRenderState(const ScopedRenderState&) = delete;
            ScopedRenderState& operator=(const ScopedRenderState&) = delete;
            ScopedRenderState(ScopedRenderState&&) = delete;
            ScopedRenderState& operator=(ScopedRenderState&&) = delete;
            ~ScopedRenderState();

            static ScopedRenderState SpritePass(bool keepDepthTest = false, bool enableBlending = true);
            static ScopedRenderState DepthOnly(bool enableDepthWrite = true);

        private:
            bool m_PreviousDepthTest = false;
            bool m_PreviousDepthWrite = true;
            bool m_PreviousBlending = false;

            bool m_ChangeDepthTest = false;
            bool m_ChangeDepthWrite = false;
            bool m_ChangeBlending = false;
        };

        // Statistics
        static uint32_t GetDrawCallCount();
        static uint32_t GetQuadCount();
        static void ResetStats();
    };

} // namespace Pillar
