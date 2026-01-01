#pragma once

#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Renderer/Texture.h"
#include <unordered_map>
#include <vector>
#include <array>

namespace Pillar {

    /**
     * @brief OpenGL-specific batch renderer
     * 
     * Implementation Details:
     * - Uses dynamic vertex buffer (GL_DYNAMIC_DRAW)
     * - Batches quads by texture (minimize texture swaps)
     * - Uploads data with glBufferSubData per flush
     * - Uses indexed rendering (6 indices per quad)
     */
    class OpenGLBatchRenderer2D : public BatchRenderer2D
    {
    public:
        OpenGLBatchRenderer2D();
        ~OpenGLBatchRenderer2D() override;

        // IRenderer2D interface
        void BeginScene(const OrthographicCamera& camera) override;
        void EndScene() override;

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color) override;
        
        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture) override;

        void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                 const glm::vec4& color) override;

        void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                 Texture2D* texture) override;

        void DrawQuad(const glm::vec3& position, const glm::vec2& size,
                 const glm::vec4& color, Texture2D* texture,
                 const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                 bool flipX = false, bool flipY = false) override;

        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                     float rotation, const glm::vec4& color) override;
        
        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                     float rotation, const glm::vec4& color, Texture2D* texture) override;

        void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                     float rotation, const glm::vec4& color) override;

        void DrawRotatedQuad(const glm::vec3& position, const glm::vec2& size,
                     float rotation, const glm::vec4& color, Texture2D* texture,
                     const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                     bool flipX = false, bool flipY = false) override;

    protected:
        void Init() override;
        void Shutdown() override;
        void Flush() override;
        void FlushAndReset() override;

    private:
        // Vertex structure (per quad corner)
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec4 Color;
            glm::vec2 TexCoord;
            float TexIndex;  // Which texture slot (0-31)
        };

        // Batch data structure (per texture)
        struct QuadBatch
        {
            Texture2D* Texture = nullptr;
            std::vector<QuadVertex> Vertices;  // 4 vertices per quad
            uint32_t QuadCount = 0;

            void Clear()
            {
                Vertices.clear();
                QuadCount = 0;
            }
        };

        // Rendering resources
        std::shared_ptr<VertexArray> m_QuadVertexArray;
        std::shared_ptr<VertexBuffer> m_QuadVertexBuffer;
        std::shared_ptr<IndexBuffer> m_QuadIndexBuffer;
        std::shared_ptr<Shader> m_BatchShader;
        std::shared_ptr<Texture2D> m_WhiteTexture;  // For colored quads

        // Batch storage (key = texture ID)
        std::unordered_map<uint32_t, QuadBatch> m_Batches;
        
        // Texture slots (OpenGL supports 32 texture units)
        static const uint32_t MaxTextureSlots = 32;
        std::array<Texture2D*, MaxTextureSlots> m_TextureSlots;
        uint32_t m_TextureSlotIndex = 1;  // 0 = white texture

        // Camera
        glm::mat4 m_ViewProjectionMatrix;

    // Helper methods
    void StartBatch();
    void NextBatch();
    uint32_t GetOrAddTextureSlot(Texture2D* texture);
    void AddQuadToBatch(const glm::vec3& position, const glm::vec2& size,
                       const glm::vec4& color, Texture2D* texture,
                       const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                       float rotation, bool flipX = false, bool flipY = false);
};} // namespace Pillar
