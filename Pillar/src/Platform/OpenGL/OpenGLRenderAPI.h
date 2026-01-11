#pragma once

#include "Pillar/Renderer/RenderAPI.h"

namespace Pillar {

    class OpenGLRenderAPI : public RenderAPI
    {
    public:
        virtual void Init() override;
        virtual void SetClearColor(const glm::vec4& color) override;
        virtual void Clear() override;
        virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
        virtual void DrawIndexed(const VertexArray* vertexArray) override;

        // State management
        virtual bool GetDepthTest() const override;
        virtual bool GetDepthWrite() const override;
        virtual bool GetBlending() const override;
        virtual void SetDepthTest(bool enable) override;
        virtual void SetDepthWrite(bool enable) override;
        virtual void SetBlending(bool enable) override;
    };

}
