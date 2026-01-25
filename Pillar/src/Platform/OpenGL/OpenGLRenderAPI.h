#pragma once

#include "Pillar/Renderer/RenderAPI.h"

namespace Pillar {

    class OpenGLRenderAPI : public RenderAPI
    {
    public:
        void Init() override;
        void SetClearColor(const glm::vec4& color) override;
        void Clear() override;
        void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        void DrawIndexed(const VertexArray* vertexArray) override;

        // State management
        bool GetDepthTest() const override;
        bool GetDepthWrite() const override;
        bool GetBlending() const override;
        void SetDepthTest(bool enable) override;
        void SetDepthWrite(bool enable) override;
        void SetBlending(bool enable) override;
    };

}
