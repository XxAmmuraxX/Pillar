#include "Pillar/Renderer/Renderer2D.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Platform/OpenGL/OpenGLRenderAPI.h"
#include "Pillar.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Pillar {

    struct Renderer2DStorage
    {
        std::shared_ptr<VertexArray> QuadVertexArray;
        std::shared_ptr<VertexBuffer> QuadVertexBuffer;
        std::shared_ptr<IndexBuffer> QuadIndexBuffer;
        std::shared_ptr<Shader> TextureShader;
        std::shared_ptr<Texture2D> WhiteTexture;
    };

    static Renderer2DStorage* s_Data = nullptr;

    void Renderer2D::Init()
    {
        PIL_CORE_INFO("Initializing Renderer2D...");
        
        s_Data = new Renderer2DStorage();

        // Create quad vertex array
        s_Data->QuadVertexArray = std::shared_ptr<VertexArray>(VertexArray::Create());

        float quadVertices[5 * 4] = {
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
             0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f, 0.0f, 1.0f
        };

        s_Data->QuadVertexBuffer = std::shared_ptr<VertexBuffer>(VertexBuffer::Create(quadVertices, sizeof(quadVertices)));
        s_Data->QuadVertexBuffer->SetLayout({
            { ShaderDataType::Float3, "a_Position" },
            { ShaderDataType::Float2, "a_TexCoord" }
        });
        s_Data->QuadVertexArray->AddVertexBuffer(s_Data->QuadVertexBuffer.get());

        uint32_t quadIndices[6] = { 0, 1, 2, 2, 3, 0 };
        s_Data->QuadIndexBuffer = std::shared_ptr<IndexBuffer>(IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t)));
        s_Data->QuadVertexArray->SetIndexBuffer(s_Data->QuadIndexBuffer.get());

        // Create white texture
        s_Data->WhiteTexture = Texture2D::Create(1, 1);
        uint32_t whiteTextureData = 0xffffffff;
        // Need to bind and upload data properly through OpenGL
        s_Data->WhiteTexture->Bind();
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &whiteTextureData);

        // Create texture shader
        std::string vertexSrc = R"(
            #version 410 core
            
            layout(location = 0) in vec3 a_Position;
            layout(location = 1) in vec2 a_TexCoord;
            
            uniform mat4 u_ViewProjection;
            uniform mat4 u_Transform;
            
            out vec2 v_TexCoord;
            
            void main()
            {
                v_TexCoord = a_TexCoord;
                gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0);
            }
        )";

        std::string fragmentSrc = R"(
            #version 410 core
            
            layout(location = 0) out vec4 color;
            
            in vec2 v_TexCoord;
            
            uniform sampler2D u_Texture;
            uniform vec4 u_Color;
            
            void main()
            {
                color = texture(u_Texture, v_TexCoord) * u_Color;
            }
        )";

        s_Data->TextureShader = std::shared_ptr<Shader>(Shader::Create(vertexSrc, fragmentSrc));
        s_Data->TextureShader->Bind();
        s_Data->TextureShader->SetInt("u_Texture", 0);

        PIL_CORE_INFO("Renderer2D initialized successfully");
    }

    void Renderer2D::Shutdown()
    {
        PIL_CORE_INFO("Shutting down Renderer2D...");
        delete s_Data;
    }

    void Renderer2D::BeginScene(const OrthographicCamera& camera)
    {
        s_Data->TextureShader->Bind();
        s_Data->TextureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());
    }

    void Renderer2D::EndScene()
    {
    }

    // Primitives
    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, color);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color)
    {
        s_Data->TextureShader->Bind();
        s_Data->TextureShader->SetFloat4("u_Color", color);

        s_Data->WhiteTexture->Bind();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        s_Data->TextureShader->SetMat4("u_Transform", transform);

        s_Data->QuadVertexArray->Bind();
        RenderCommand::DrawIndexed(*s_Data->QuadVertexArray);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor)
    {
        DrawQuad({ position.x, position.y, 0.0f }, size, texture, tintColor);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const std::shared_ptr<Texture2D>& texture, const glm::vec4& tintColor)
    {
        s_Data->TextureShader->Bind();
        s_Data->TextureShader->SetFloat4("u_Color", tintColor);

        texture->Bind();

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
            * glm::scale(glm::mat4(1.0f), { size.x, size.y, 1.0f });
        s_Data->TextureShader->SetMat4("u_Transform", transform);

        s_Data->QuadVertexArray->Bind();
        RenderCommand::DrawIndexed(*s_Data->QuadVertexArray);
    }

}
