#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/RenderAPI.h"

namespace Pillar {

    class VertexArray;

    class PIL_API RenderCommand
    {
    public:
        static void Init()
        {
            s_RenderAPI->Init();
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RenderAPI->SetViewport(x, y, width, height);
        }

        static void SetClearColor(const glm::vec4& color)
        {
            s_RenderAPI->SetClearColor(color);
        }

        static void Clear()
        {
            s_RenderAPI->Clear();
        }

        static void DrawIndexed(const VertexArray& vertexArray)
        {
            s_RenderAPI->DrawIndexed(&vertexArray);
        }

        static void SetAPI(RenderAPI* api) { s_RenderAPI = api; }

    private:
        static RenderAPI* s_RenderAPI;
    };

}
