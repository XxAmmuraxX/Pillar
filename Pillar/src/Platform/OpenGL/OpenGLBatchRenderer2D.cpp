#include "OpenGLBatchRenderer2D.h"
#include "Pillar/Renderer/Buffer.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Logger.h"
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Pillar {

    OpenGLBatchRenderer2D::OpenGLBatchRenderer2D()
    {
        Init();
    }

    OpenGLBatchRenderer2D::~OpenGLBatchRenderer2D()
    {
        Shutdown();
    }

    void OpenGLBatchRenderer2D::Init()
    {
        PIL_CORE_INFO("Initializing OpenGLBatchRenderer2D...");

        // Create white texture (1x1 white pixel for colored quads)
        uint32_t whiteTextureData = 0xffffffff;
        m_WhiteTexture = std::shared_ptr<Texture2D>(Texture2D::Create(1, 1));
        m_WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        // Initialize texture slots (slot 0 = white texture)
        m_TextureSlots.fill(nullptr);
        m_TextureSlots[0] = m_WhiteTexture.get();
        m_TextureSlotIndex = 1;

        // Create vertex array
        m_QuadVertexArray = std::shared_ptr<VertexArray>(VertexArray::Create());

        // Create vertex buffer (dynamic - will be updated each frame)
        m_QuadVertexBuffer = std::shared_ptr<VertexBuffer>(
            VertexBuffer::Create(MaxVertices * sizeof(QuadVertex))
        );

        // Set vertex buffer layout
        m_QuadVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float4, "a_Color" },
            { ShaderDataType::Float2, "a_TexCoord" },
            { ShaderDataType::Float,  "a_TexIndex" }
        });

        m_QuadVertexArray->AddVertexBuffer(m_QuadVertexBuffer.get());

        // Create index buffer (static - indices pattern repeats)
        std::vector<uint32_t> quadIndices;
        quadIndices.reserve(MaxIndices);

        uint32_t offset = 0;
        for (uint32_t i = 0; i < MaxQuadsPerBatch; ++i)
        {
            // Two triangles per quad (0,1,2) and (2,3,0)
            quadIndices.push_back(offset + 0);
            quadIndices.push_back(offset + 1);
            quadIndices.push_back(offset + 2);

            quadIndices.push_back(offset + 2);
            quadIndices.push_back(offset + 3);
            quadIndices.push_back(offset + 0);

            offset += 4;
        }

        m_QuadIndexBuffer = std::shared_ptr<IndexBuffer>(
            IndexBuffer::Create(quadIndices.data(), MaxIndices)
        );
        m_QuadVertexArray->SetIndexBuffer(m_QuadIndexBuffer.get());

        // Load batch shader from embedded source (shaders are part of engine, not assets)
        const char* vertexShaderSrc = R"(
            #version 410 core

            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec4 a_Color;
            layout(location = 2) in vec2 a_TexCoord;
            layout(location = 3) in float a_TexIndex;

            uniform mat4 u_ViewProjection;

            out vec4 v_Color;
            out vec2 v_TexCoord;
            out float v_TexIndex;

            void main()
            {
                v_Color = a_Color;
                v_TexCoord = a_TexCoord;
                v_TexIndex = a_TexIndex;
                gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            }
        )";

        const char* fragmentShaderSrc = R"(
            #version 410 core

            layout(location = 0) out vec4 color;

            in vec4 v_Color;
            in vec2 v_TexCoord;
            in float v_TexIndex;

            uniform sampler2D u_Textures[32];

            void main()
            {
                int texIndex = int(v_TexIndex);
                color = texture(u_Textures[texIndex], v_TexCoord) * v_Color;
            }
        )";

        m_BatchShader = std::shared_ptr<Shader>(Shader::Create(vertexShaderSrc, fragmentShaderSrc));

        if (!m_BatchShader)
        {
            PIL_CORE_ERROR("Failed to create batch shader!");
            return;
        }

        // Set texture samplers uniform (0-31)
        m_BatchShader->Bind();
        int samplers[MaxTextureSlots];
        for (int i = 0; i < MaxTextureSlots; ++i)
            samplers[i] = i;
        m_BatchShader->SetIntArray("u_Textures", samplers, MaxTextureSlots);

        PIL_CORE_INFO("OpenGLBatchRenderer2D initialized successfully");
    }

    void OpenGLBatchRenderer2D::Shutdown()
    {
        PIL_CORE_INFO("Shutting down OpenGLBatchRenderer2D...");
        m_Batches.clear();
        m_QuadVertexArray.reset();
        m_QuadVertexBuffer.reset();
        m_QuadIndexBuffer.reset();
        m_BatchShader.reset();
        m_WhiteTexture.reset();
    }

    void OpenGLBatchRenderer2D::BeginScene(const OrthographicCamera& camera)
    {
        m_ViewProjectionMatrix = camera.GetViewProjectionMatrix();
        StartBatch();
    }

    void OpenGLBatchRenderer2D::EndScene()
    {
        Flush();
    }

    void OpenGLBatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                        const glm::vec4& color)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color, m_WhiteTexture.get(), 
                glm::vec2(0.0f), glm::vec2(1.0f), false, false);
    }

    void OpenGLBatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                        const glm::vec4& color, Texture2D* texture)
    {
        DrawQuad(glm::vec3(position, 0.0f), size, color, texture, 
                glm::vec2(0.0f), glm::vec2(1.0f), false, false);
    }

	void OpenGLBatchRenderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
										   const glm::vec4& color, Texture2D* texture,
										   const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
										   bool flipX, bool flipY)
	{
		AddQuadToBatch(position, size, color, texture, texCoordMin, texCoordMax, 0.0f, flipX, flipY);
	}    void OpenGLBatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                               float rotation, const glm::vec4& color)
    {
        AddQuadToBatch(glm::vec3(position, 0.0f), size, color, m_WhiteTexture.get(),
                      glm::vec2(0.0f), glm::vec2(1.0f), rotation, false, false);
    }

    void OpenGLBatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                               float rotation, const glm::vec4& color, Texture2D* texture)
    {
        AddQuadToBatch(glm::vec3(position, 0.0f), size, color, texture,
                      glm::vec2(0.0f), glm::vec2(1.0f), rotation, false, false);
    }

    void OpenGLBatchRenderer2D::StartBatch()
    {
        // Clear all batches
        m_Batches.clear();
        
        // Reset texture slot index (0 is white texture)
        m_TextureSlotIndex = 1;
        
        // Clear stats
        ResetStats();
    }

    void OpenGLBatchRenderer2D::NextBatch()
    {
        Flush();
        StartBatch();
    }

    void OpenGLBatchRenderer2D::Flush()
    {
        if (m_Batches.empty())
            return;

        if (!m_BatchShader)
        {
            PIL_CORE_ERROR("Batch shader is null! Cannot render.");
            return;
        }

        // Bind shader and set view-projection matrix
        m_BatchShader->Bind();
        m_BatchShader->SetMat4("u_ViewProjection", m_ViewProjectionMatrix);

        // Bind all textures to their slots
        for (uint32_t i = 0; i < m_TextureSlotIndex; ++i)
        {
            if (m_TextureSlots[i])
            {
                m_TextureSlots[i]->Bind(i);
            }
        }

        // Render each batch
        for (auto& [textureID, batch] : m_Batches)
        {
            if (batch.QuadCount == 0)
                continue;

            // Upload vertex data to GPU
            uint32_t dataSize = static_cast<uint32_t>(batch.Vertices.size() * sizeof(QuadVertex));
            m_QuadVertexBuffer->SetData(batch.Vertices.data(), dataSize);

            // Bind vertex array and draw
            m_QuadVertexArray->Bind();
            uint32_t indexCount = batch.QuadCount * 6;
            glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);

            // Update stats
            m_Stats.DrawCalls++;
            m_Stats.QuadCount += batch.QuadCount;
            m_Stats.VertexCount += batch.Vertices.size();
        }
    }

    void OpenGLBatchRenderer2D::FlushAndReset()
    {
        Flush();
        StartBatch();
    }

    uint32_t OpenGLBatchRenderer2D::GetOrAddTextureSlot(Texture2D* texture)
    {
        if (!texture)
            return 0;  // White texture

        uint32_t textureID = texture->GetRendererID();

        // Check if texture is already in a slot
        for (uint32_t i = 1; i < m_TextureSlotIndex; ++i)
        {
            if (m_TextureSlots[i] && m_TextureSlots[i]->GetRendererID() == textureID)
            {
                return i;
            }
        }

        // Check if we have space for a new texture
        if (m_TextureSlotIndex >= MaxTextureSlots)
        {
            // No more texture slots - flush and reset
            FlushAndReset();
        }

        // Add texture to next available slot
        uint32_t slotIndex = m_TextureSlotIndex;
        m_TextureSlots[slotIndex] = texture;
        m_TextureSlotIndex++;

        return slotIndex;
    }

    void OpenGLBatchRenderer2D::AddQuadToBatch(const glm::vec3& position, const glm::vec2& size,
                                              const glm::vec4& color, Texture2D* texture,
                                              const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                                              float rotation, bool flipX, bool flipY)
    {
        // Get or assign texture slot
        uint32_t textureSlot = GetOrAddTextureSlot(texture);
        uint32_t textureID = texture ? texture->GetRendererID() : 0;

        // Find batch or create new one
        auto it = m_Batches.find(textureID);
        if (it == m_Batches.end())
        {
            // Create new batch with preallocated capacity
            QuadBatch newBatch;
            newBatch.Texture = texture;
            newBatch.Vertices.reserve(MaxVertices);  // Preallocate to prevent reallocation
            auto result = m_Batches.emplace(textureID, std::move(newBatch));
            it = result.first;
        }

        QuadBatch& batch = it->second;

        // Check if batch is full
        if (batch.QuadCount >= MaxQuadsPerBatch)
        {
            FlushAndReset();
            // After reset, re-get texture slot and create new batch
            textureSlot = GetOrAddTextureSlot(texture);
            QuadBatch newBatch;
            newBatch.Texture = texture;
            newBatch.Vertices.reserve(MaxVertices);
            auto result = m_Batches.emplace(textureID, std::move(newBatch));
            it = result.first;
        }

        QuadBatch& currentBatch = it->second;

        // Calculate quad vertices
        glm::vec3 vertices[4];
        
        if (rotation != 0.0f)
        {
            // Rotated quad
            glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                                * glm::rotate(glm::mat4(1.0f), rotation, glm::vec3(0.0f, 0.0f, 1.0f))
                                * glm::scale(glm::mat4(1.0f), glm::vec3(size, 1.0f));

            vertices[0] = transform * glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f);
            vertices[1] = transform * glm::vec4( 0.5f, -0.5f, 0.0f, 1.0f);
            vertices[2] = transform * glm::vec4( 0.5f,  0.5f, 0.0f, 1.0f);
            vertices[3] = transform * glm::vec4(-0.5f,  0.5f, 0.0f, 1.0f);
        }
        else
        {
            // Non-rotated quad (faster)
            glm::vec3 halfSize = glm::vec3(size * 0.5f, 0.0f);
            vertices[0] = position + glm::vec3(-halfSize.x, -halfSize.y, 0.0f);
            vertices[1] = position + glm::vec3( halfSize.x, -halfSize.y, 0.0f);
            vertices[2] = position + glm::vec3( halfSize.x,  halfSize.y, 0.0f);
            vertices[3] = position + glm::vec3(-halfSize.x,  halfSize.y, 0.0f);
        }

	// Texture coordinates (with optional flipping)
	float uvMinX = flipX ? texCoordMax.x : texCoordMin.x;
	float uvMaxX = flipX ? texCoordMin.x : texCoordMax.x;
	float uvMinY = flipY ? texCoordMax.y : texCoordMin.y;
	float uvMaxY = flipY ? texCoordMin.y : texCoordMax.y;
	
	glm::vec2 texCoords[4] = {
		{ uvMinX, uvMinY },  // Bottom-left
		{ uvMaxX, uvMinY },  // Bottom-right
		{ uvMaxX, uvMaxY },  // Top-right
		{ uvMinX, uvMaxY }   // Top-left
	};        // Add 4 vertices to batch
        for (int i = 0; i < 4; ++i)
        {
            QuadVertex vertex;
            vertex.Position = vertices[i];
            vertex.Color = color;
            vertex.TexCoord = texCoords[i];
            vertex.TexIndex = static_cast<float>(textureSlot);
            currentBatch.Vertices.push_back(vertex);
        }

        currentBatch.QuadCount++;
    }

} // namespace Pillar
