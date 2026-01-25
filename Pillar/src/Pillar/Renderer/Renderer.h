#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/RenderAPI.h"
#include <memory>

namespace Pillar {

    class VertexArray;
    class Shader;
    class OrthographicCamera;

    /**
     * @brief Legacy Renderer Class - DEPRECATED
     * 
     * @deprecated Use Renderer2D instead. This class only wraps RenderCommand.
     * 
     * Migration Guide:
     * - Renderer::SetClearColor() -> Renderer2D::SetClearColor()
     * - Renderer::Clear() -> Renderer2D::Clear()
     * - Renderer::SetViewport() -> Renderer2D::SetViewport()
     * - Renderer::BeginScene() -> Renderer2D::BeginScene()
     * - Renderer::EndScene() -> Renderer2D::EndScene()
     * 
     * This class will be removed in a future version.
     */
    class PIL_API Renderer
    {
    public:
        [[deprecated("Use Renderer2D::Init() instead")]]
        static void Init();
        
        [[deprecated("Use Renderer2D::Shutdown() instead")]]
        static void Shutdown();

        [[deprecated("Use Renderer2D::BeginScene() instead")]]
        static void BeginScene(OrthographicCamera& camera);
        
        [[deprecated("Use Renderer2D::EndScene() instead")]]
        static void EndScene();

        [[deprecated("Use Renderer2D::SetClearColor() instead")]]
        static void SetClearColor(const glm::vec4& color);
        
        [[deprecated("Use Renderer2D::Clear() instead")]]
        static void Clear();
        
        [[deprecated("Use Renderer2D::SetViewport() instead")]]
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        inline static RendererAPI GetAPI() { return RenderAPI::GetAPI(); }

    private:
        struct SceneData
        {
            glm::mat4 ViewProjectionMatrix;
        };

        static std::unique_ptr<SceneData> s_SceneData;
        static std::unique_ptr<RenderAPI> s_RenderAPI;
    };

}
