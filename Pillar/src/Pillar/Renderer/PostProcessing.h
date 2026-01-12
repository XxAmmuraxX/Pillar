#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Framebuffer.h"
#include "Pillar/Renderer/Shader.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>

namespace Pillar {

    /**
     * @brief Post-processing effect base class
     * 
     * Effects process the rendered scene texture through a fullscreen quad
     * with custom shader effects. Multiple effects can be chained together.
     */
    class PIL_API PostProcessEffect
    {
    public:
        virtual ~PostProcessEffect() = default;

        /**
         * @brief Initialize effect resources (shaders, buffers, etc.)
         */
        virtual void Init() = 0;

        /**
         * @brief Apply effect to input texture, render to output framebuffer
         * @param inputTexture Source texture ID
         * @param outputFB Destination framebuffer (or null for screen)
         */
        virtual void Apply(uint32_t inputTexture, Framebuffer* outputFB) = 0;

        /**
         * @brief Enable/disable this effect
         */
        void SetEnabled(bool enabled) { m_Enabled = enabled; }
        bool IsEnabled() const { return m_Enabled; }

        /**
         * @brief Set effect intensity (0.0 = off, 1.0 = full)
         */
        void SetIntensity(float intensity) { m_Intensity = glm::clamp(intensity, 0.0f, 1.0f); }
        float GetIntensity() const { return m_Intensity; }

    protected:
        bool m_Enabled = true;
        float m_Intensity = 1.0f;
    };

    /**
     * @brief Simple grayscale post-processing effect
     */
    class PIL_API GrayscaleEffect : public PostProcessEffect
    {
    public:
        ~GrayscaleEffect() override;
        void Init() override;
        void Apply(uint32_t inputTexture, Framebuffer* outputFB) override;

    private:
        std::shared_ptr<Shader> m_Shader = nullptr;
    };

    /**
     * @brief Vignette effect (darkens edges)
     */
    class PIL_API VignetteEffect : public PostProcessEffect
    {
    public:
        ~VignetteEffect() override;
        void Init() override;
        void Apply(uint32_t inputTexture, Framebuffer* outputFB) override;

        void SetRadius(float radius) { m_Radius = radius; }
        void SetSoftness(float softness) { m_Softness = softness; }

    private:
        std::shared_ptr<Shader> m_Shader = nullptr;
        float m_Radius = 0.75f;
        float m_Softness = 0.45f;
    };

    /**
     * @brief Bloom effect (glowing bright areas)
     */
    class PIL_API BloomEffect : public PostProcessEffect
    {
    public:
        ~BloomEffect() override;
        void Init() override;
        void Apply(uint32_t inputTexture, Framebuffer* outputFB) override;

        void SetThreshold(float threshold) { m_Threshold = threshold; }
        void SetBlurPasses(int passes) { m_BlurPasses = passes; }

    private:
        std::shared_ptr<Shader> m_ExtractBrightShader = nullptr;
        std::shared_ptr<Shader> m_BlurShader = nullptr;
        std::shared_ptr<Shader> m_CompositeShader = nullptr;
        std::shared_ptr<Framebuffer> m_BrightFB;
        std::shared_ptr<Framebuffer> m_BlurFB1;
        std::shared_ptr<Framebuffer> m_BlurFB2;
        
        float m_Threshold = 1.0f;
        int m_BlurPasses = 2;
    };

    /**
     * @brief Chromatic aberration effect (color fringing)
     */
    class PIL_API ChromaticAberrationEffect : public PostProcessEffect
    {
    public:
        ~ChromaticAberrationEffect() override;
        void Init() override;
        void Apply(uint32_t inputTexture, Framebuffer* outputFB) override;

        void SetOffset(float offset) { m_Offset = offset; }

    private:
        std::shared_ptr<Shader> m_Shader = nullptr;
        float m_Offset = 0.01f;
    };

    /**
     * @brief Post-processing stack for chaining multiple effects
     * 
     * Usage:
     * ```cpp
     * PostProcessStack stack;
     * stack.Init(1920, 1080);
     * stack.AddEffect(std::make_shared<VignetteEffect>());
     * stack.AddEffect(std::make_shared<BloomEffect>());
     * 
     * // In render loop:
     * Renderer2D::BeginScene(camera);
     * // ... draw scene ...
     * Renderer2D::EndScene();
     * 
     * stack.Process(sceneTexture);
     * ```
     */
    class PIL_API PostProcessStack
    {
    public:
        /**
         * @brief Initialize post-processing stack
         * @param width Framebuffer width
         * @param height Framebuffer height
         */
        void Init(uint32_t width, uint32_t height);

        /**
         * @brief Shutdown and clean up resources
         */
        void Shutdown();

        /**
         * @brief Add an effect to the stack
         */
        void AddEffect(std::shared_ptr<PostProcessEffect> effect);

        /**
         * @brief Remove an effect from the stack
         */
        void RemoveEffect(std::shared_ptr<PostProcessEffect> effect);

        /**
         * @brief Clear all effects
         */
        void ClearEffects();

        /**
         * @brief Process input texture through all enabled effects
         * @param inputTexture Scene texture to process
         */
        void Process(uint32_t inputTexture);

        /**
         * @brief Resize framebuffers
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief Enable/disable entire stack
         */
        void SetEnabled(bool enabled) { m_Enabled = enabled; }
        bool IsEnabled() const { return m_Enabled; }

        /**
         * @brief Get all effects in stack
         */
        const std::vector<std::shared_ptr<PostProcessEffect>>& GetEffects() const { return m_Effects; }

    private:
        void RenderFullscreenQuad();

        std::vector<std::shared_ptr<PostProcessEffect>> m_Effects;
        std::shared_ptr<Framebuffer> m_PingPongFB[2]; // For multi-pass effects
        bool m_Enabled = true;
        uint32_t m_Width = 1920;
        uint32_t m_Height = 1080;
        
        // Fullscreen quad data
        uint32_t m_QuadVAO = 0;
        uint32_t m_QuadVBO = 0;
    };

}
