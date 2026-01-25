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

    class TextureAtlas;
    struct SubTexture;
    struct TransformComponent;
    struct SpriteComponent;

    /**
     * @brief Renderer2D Backend - High-Performance Batch Renderer
     * 
     * Provides a static API for 2D rendering using batched draw calls.
     * Accumulates quads into texture-based batches for optimal GPU performance.
     * 
     * Usage:
     *   Renderer2D::Init();
     *   Renderer2D::BeginScene(camera);
     *   Renderer2D::DrawQuad(...);
     *   Renderer2D::EndScene();
     */
    class PIL_API Renderer2D
    {
    public:
        static void Init();
        static void Shutdown();

        // Rendering setup (convenience methods to reduce need for RenderCommand)
        static void SetClearColor(const glm::vec4& color);
        static void Clear();
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

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

        // Texture Atlas methods
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size,
                            const glm::vec4& color,
                            const std::shared_ptr<TextureAtlas>& atlas,
                            const std::string& spriteName);

        static void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                            const glm::vec4& color,
                            const std::shared_ptr<TextureAtlas>& atlas,
                            const std::string& spriteName);

        static void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color,
                                   const std::shared_ptr<TextureAtlas>& atlas,
                                   const std::string& spriteName);

        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color,
                                   const std::shared_ptr<TextureAtlas>& atlas,
                                   const std::string& spriteName);

        // Direct SubTexture methods (for advanced usage)
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                            const glm::vec4& color,
                            const std::shared_ptr<Texture2D>& texture,
                            const SubTexture& subTexture,
                            bool flipX = false, bool flipY = false);

        static void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                   float rotation, const glm::vec4& color,
                                   const std::shared_ptr<Texture2D>& texture,
                                   const SubTexture& subTexture,
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
        struct Renderer2DStats
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;
            uint32_t VertexCount = 0;
            uint32_t BatchCount = 0;
            uint32_t TextureSwitches = 0;
            uint32_t FlushCount = 0;
            uint32_t BufferUploads = 0;
            uint32_t TotalQuadsRendered = 0;
            uint32_t TotalDrawCalls = 0;
            uint32_t PeakVertices = 0;
            uint32_t PeakQuads = 0;
            
            float GetBatchEfficiency() const { return BatchCount > 0 ? (float)QuadCount / BatchCount : 0.0f; }
            float GetAverageQuadsPerDraw() const { return DrawCalls > 0 ? (float)QuadCount / DrawCalls : 0.0f; }
            float GetAverageVerticesPerDraw() const { return DrawCalls > 0 ? (float)VertexCount / DrawCalls : 0.0f; }
        };

        static uint32_t GetDrawCallCount();
        static uint32_t GetQuadCount();
        static Renderer2DStats GetStats();
        static void ResetStats();
    };

} // namespace Pillar
