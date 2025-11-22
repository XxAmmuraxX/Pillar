#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>

namespace Pillar {

    enum class RendererAPI
    {
        None = 0,
        OpenGL = 1
    };

    class VertexArray;

    class PIL_API RenderAPI
    {
    public:
        virtual ~RenderAPI() = default;

        virtual void Init() = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;
        virtual void Clear() = 0;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void DrawIndexed(const VertexArray* vertexArray) = 0;

        inline static RendererAPI GetAPI() { return s_API; }

    private:
        static RendererAPI s_API;
    };

}
