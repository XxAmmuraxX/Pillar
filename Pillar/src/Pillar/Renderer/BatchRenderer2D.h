#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace Pillar {

    class VertexArray;
    class VertexBuffer;
    class IndexBuffer;
    class Shader;

    /**
     * @brief Interface for 2D Renderer implementations
     * 
     * Defines the contract for all 2D rendering backends (Basic, Batch, etc.)
     */
    class PIL_API IRenderer2D
    {
    public:
        virtual ~IRenderer2D() = default;

        virtual void BeginScene(const OrthographicCamera& camera) = 0;
        virtual void EndScene() = 0;

        virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                            const glm::vec4& color) = 0;
        
        virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                            const glm::vec4& color, const Texture2D* texture) = 0;

        virtual void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                             const glm::vec4& color) = 0;

        virtual void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                             const Texture2D* texture) = 0;

        virtual void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                             const glm::vec4& color, const Texture2D* texture,
                             const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                             bool flipX = false, bool flipY = false) = 0;

        virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color) = 0;

        virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color, const Texture2D* texture) = 0;

        virtual void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color) = 0;

        virtual void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color, const Texture2D* texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                    bool flipX = false, bool flipY = false) = 0;

        // Stats
        virtual uint32_t GetDrawCallCount() const = 0;
        virtual uint32_t GetQuadCount() const = 0;
        virtual void ResetStats() = 0;
    };

    /**
     * @brief Batch Renderer for 2D Quads
     * 
     * Accumulates quads into texture-based batches and submits
     * them to the GPU in a single draw call per texture.
     * 
     * Performance Target:
     * - 50,000 quads at 60 FPS
     * - 1-5 draw calls per frame (depends on unique textures)
     */
    class PIL_API BatchRenderer2D : public IRenderer2D
    {
    public:
        static constexpr uint32_t MaxQuadsPerBatch = 10000;
        static constexpr uint32_t MaxVertices = MaxQuadsPerBatch * 4;
        static constexpr uint32_t MaxIndices = MaxQuadsPerBatch * 6;

        virtual ~BatchRenderer2D() = default;

        // Factory method (creates OpenGLBatchRenderer2D)
        // Returns unique_ptr for automatic memory management
        static std::unique_ptr<BatchRenderer2D> Create();

        // Stats
        uint32_t GetDrawCallCount() const override { return m_Stats.DrawCalls; }
        uint32_t GetQuadCount() const override { return m_Stats.QuadCount; }
        void ResetStats() override;

        // Make stats accessible for Renderer2D::GetStats()
        // This is intentionally public to allow the static API to access detailed stats
        struct Stats
        {
            // Per-frame counters
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;
            uint32_t VertexCount = 0;
            uint32_t BatchCount = 0;
            uint32_t TextureSwitches = 0;
            uint32_t FlushCount = 0;
            uint32_t BufferUploads = 0;
            
            // Accumulated stats
            uint32_t TotalQuadsRendered = 0;
            uint32_t TotalDrawCalls = 0;
            
            // Performance metrics
            uint32_t PeakVertices = 0;
            uint32_t PeakQuads = 0;
            
            // Efficiency ratios
            float GetBatchEfficiency() const
            {
                return BatchCount > 0 ? (float)QuadCount / BatchCount : 0.0f;
            }
            
            float GetAverageQuadsPerDraw() const
            {
                return DrawCalls > 0 ? (float)QuadCount / DrawCalls : 0.0f;
            }
            
            float GetAverageVerticesPerDraw() const
            {
                return DrawCalls > 0 ? (float)VertexCount / DrawCalls : 0.0f;
            }
        };

        Stats m_Stats;

    protected:
        // Subclasses implement these
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Flush() = 0;  // Submit current batch to GPU
        virtual void FlushAndReset() = 0;  // Flush + prepare for next batch
    };

} // namespace Pillar
