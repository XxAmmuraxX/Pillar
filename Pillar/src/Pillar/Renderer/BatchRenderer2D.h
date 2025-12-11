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
                            const glm::vec4& color, Texture2D* texture) = 0;
        
    virtual void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                        const glm::vec4& color, Texture2D* texture,
                        const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                        bool flipX = false, bool flipY = false) = 0;        virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color) = 0;
        
        virtual void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                    float rotation, const glm::vec4& color, Texture2D* texture) = 0;

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
        static BatchRenderer2D* Create();

        // IRenderer2D interface
        void BeginScene(const OrthographicCamera& camera) override;
        void EndScene() override;

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color) override;
        
        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture) override;
        
    void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                 const glm::vec4& color, Texture2D* texture,
                 const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                 bool flipX = false, bool flipY = false) override;        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color) override;
        
        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color, Texture2D* texture) override;

        // Stats
        uint32_t GetDrawCallCount() const override { return m_Stats.DrawCalls; }
        uint32_t GetQuadCount() const override { return m_Stats.QuadCount; }
        void ResetStats() override;

    protected:
        struct Stats
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;
            uint32_t VertexCount = 0;
        };

        Stats m_Stats;

        // Subclasses implement these
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Flush() = 0;  // Submit current batch to GPU
        virtual void FlushAndReset() = 0;  // Flush + prepare for next batch
    };

} // namespace Pillar
