#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/RenderAPI.h"
#include <memory>

namespace Pillar {

    class VertexArray;

    class PIL_API Renderer
    {
    public:
        static void Init();
        static void Shutdown();

        static void BeginScene();
        static void EndScene();

        static void SetClearColor(const glm::vec4& color);
        static void Clear();
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        
        static void Submit(const VertexArray* vertexArray);

        inline static RendererAPI GetAPI() { return RenderAPI::GetAPI(); }

    private:
        static std::unique_ptr<RenderAPI> s_RenderAPI;
    };

}
