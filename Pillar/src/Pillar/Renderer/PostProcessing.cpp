#include "PostProcessing.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Renderer/EmbeddedShaders.h"
#include "Pillar/Logger.h"
#include <algorithm>
#include <glad/gl.h>

namespace Pillar {

    // ========================================================================
    // GrayscaleEffect
    // ========================================================================

    GrayscaleEffect::~GrayscaleEffect() = default;

    void GrayscaleEffect::Init()
    {
        // Use embedded shaders for SDK distribution (no file dependencies)
        m_Shader = Shader::Create(
            EmbeddedShaders::PostProcessVertex,
            EmbeddedShaders::GrayscaleFragment
        );
        
        if (!m_Shader)
        {
            PIL_CORE_ERROR("PostProcessing: Failed to create GrayscaleEffect shader!");
            return;
        }
        
        PIL_CORE_INFO("PostProcessing: GrayscaleEffect initialized");
    }

    void GrayscaleEffect::Apply(uint32_t inputTexture, Framebuffer* outputFB)
    {
        if (!m_Enabled || !m_Shader)
            return;

        m_Shader->Bind();
        m_Shader->SetFloat("u_Intensity", m_Intensity);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        m_Shader->SetInt("u_Texture", 0);
    }

    // ========================================================================
    // VignetteEffect
    // ========================================================================

    VignetteEffect::~VignetteEffect() = default;

    void VignetteEffect::Init()
    {
        // Use embedded shaders for SDK distribution (no file dependencies)
        m_Shader = Shader::Create(
            EmbeddedShaders::PostProcessVertex,
            EmbeddedShaders::VignetteFragment
        );
        
        if (!m_Shader)
        {
            PIL_CORE_ERROR("PostProcessing: Failed to create VignetteEffect shader!");
            return;
        }
        
        PIL_CORE_INFO("PostProcessing: VignetteEffect initialized");
    }

    void VignetteEffect::Apply(uint32_t inputTexture, Framebuffer* outputFB)
    {
        if (!m_Enabled || !m_Shader)
            return;

        m_Shader->Bind();
        m_Shader->SetFloat("u_Intensity", m_Intensity);
        m_Shader->SetFloat("u_Radius", m_Radius);
        m_Shader->SetFloat("u_Softness", m_Softness);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        m_Shader->SetInt("u_Texture", 0);
    }

    // ========================================================================
    // BloomEffect (Stubbed - Complex Implementation)
    // ========================================================================

    BloomEffect::~BloomEffect() = default;

    void BloomEffect::Init()
    {
        PIL_CORE_WARN("PostProcessing: BloomEffect::Init() - NOT IMPLEMENTED (requires multi-pass rendering)");
        // TODO: Implement bloom effect with:
        // 1. Extract bright pixels (threshold)
        // 2. Gaussian blur (multiple passes)
        // 3. Composite with original
    }

    void BloomEffect::Apply(uint32_t inputTexture, Framebuffer* outputFB)
    {
        PIL_CORE_WARN("PostProcessing: BloomEffect::Apply() - NOT IMPLEMENTED");
    }

    // ========================================================================
    // ChromaticAberrationEffect
    // ========================================================================

    ChromaticAberrationEffect::~ChromaticAberrationEffect() = default;

    void ChromaticAberrationEffect::Init()
    {
        // Use embedded shaders for SDK distribution (no file dependencies)
        m_Shader = Shader::Create(
            EmbeddedShaders::PostProcessVertex,
            EmbeddedShaders::ChromaticAberrationFragment
        );
        
        if (!m_Shader)
        {
            PIL_CORE_ERROR("PostProcessing: Failed to create ChromaticAberrationEffect shader!");
            return;
        }
        
        PIL_CORE_INFO("PostProcessing: ChromaticAberrationEffect initialized");
    }

    void ChromaticAberrationEffect::Apply(uint32_t inputTexture, Framebuffer* outputFB)
    {
        if (!m_Enabled || !m_Shader)
            return;

        m_Shader->Bind();
        m_Shader->SetFloat("u_Intensity", m_Intensity);
        m_Shader->SetFloat("u_Offset", m_Offset);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        m_Shader->SetInt("u_Texture", 0);
    }

    // ========================================================================
    // PostProcessStack
    // ========================================================================

    void PostProcessStack::Init(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        // Create ping-pong framebuffers for multi-pass effects
        FramebufferSpecification spec;
        spec.Width = width;
        spec.Height = height;
        spec.Samples = 1;
        spec.SwapChainTarget = false;

        m_PingPongFB[0] = Framebuffer::Create(spec);
        m_PingPongFB[1] = Framebuffer::Create(spec);

        // Create fullscreen quad
        float quadVertices[] = {
            // Positions    // TexCoords
            -1.0f,  1.0f,   0.0f, 1.0f,
            -1.0f, -1.0f,   0.0f, 0.0f,
             1.0f, -1.0f,   1.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 1.0f
        };

        glGenVertexArrays(1, &m_QuadVAO);
        glGenBuffers(1, &m_QuadVBO);
        glBindVertexArray(m_QuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0);

        PIL_CORE_INFO("PostProcessStack: Initialized ({}x{})", width, height);
    }

    void PostProcessStack::Shutdown()
    {
        if (m_QuadVAO)
        {
            glDeleteVertexArrays(1, &m_QuadVAO);
            m_QuadVAO = 0;
        }
        if (m_QuadVBO)
        {
            glDeleteBuffers(1, &m_QuadVBO);
            m_QuadVBO = 0;
        }

        m_PingPongFB[0].reset();
        m_PingPongFB[1].reset();
        m_Effects.clear();
    }

    void PostProcessStack::AddEffect(std::shared_ptr<PostProcessEffect> effect)
    {
        if (effect)
        {
            effect->Init();
            m_Effects.push_back(effect);
        }
    }

    void PostProcessStack::RemoveEffect(std::shared_ptr<PostProcessEffect> effect)
    {
        auto it = std::find(m_Effects.begin(), m_Effects.end(), effect);
        if (it != m_Effects.end())
            m_Effects.erase(it);
    }

    void PostProcessStack::ClearEffects()
    {
        m_Effects.clear();
    }

    void PostProcessStack::Process(uint32_t inputTexture)
    {
        if (!m_Enabled || m_Effects.empty())
            return;

        // Find the last enabled effect to determine when to render to screen
        int lastEnabledEffectIndex = -1;
        for (int i = (int)m_Effects.size() - 1; i >= 0; --i)
        {
            if (m_Effects[i]->IsEnabled())
            {
                lastEnabledEffectIndex = i;
                break;
            }
        }

        if (lastEnabledEffectIndex == -1)
            return; // No enabled effects

        // Disable depth testing for post-processing
        glDisable(GL_DEPTH_TEST);

        // Ping-pong between framebuffers for multi-pass effects
        uint32_t currentTexture = inputTexture;
        bool pingPong = false;

        for (size_t i = 0; i < m_Effects.size(); ++i)
        {
            auto& effect = m_Effects[i];
            if (!effect->IsEnabled())
                continue;

            // Last enabled effect renders to screen (null framebuffer)
            Framebuffer* outputFB = (i == lastEnabledEffectIndex) ? nullptr : m_PingPongFB[pingPong ? 1 : 0].get();

            if (outputFB)
            {
                outputFB->Bind();
                glViewport(0, 0, m_Width, m_Height);
                glClear(GL_COLOR_BUFFER_BIT);
            }

            effect->Apply(currentTexture, outputFB);
            RenderFullscreenQuad();

            if (outputFB)
            {
                outputFB->Unbind();
                currentTexture = outputFB->GetColorAttachmentRendererID();
            }

            pingPong = !pingPong;
        }

        // Re-enable depth testing
        glEnable(GL_DEPTH_TEST);
    }

    void PostProcessStack::Resize(uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;

        if (m_PingPongFB[0])
            m_PingPongFB[0]->Resize(width, height);
        if (m_PingPongFB[1])
            m_PingPongFB[1]->Resize(width, height);
    }

    void PostProcessStack::RenderFullscreenQuad()
    {
        glBindVertexArray(m_QuadVAO);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);
    }

}
