#pragma once

#include "Pillar/Renderer/VertexArray.h"
#include <vector>

namespace Pillar {

    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();
        virtual ~OpenGLVertexArray();

        virtual void Bind() const override;
        virtual void Unbind() const override;

        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) override;
        virtual void SetIndexBuffer(IndexBuffer* indexBuffer) override;

        virtual IndexBuffer* GetIndexBuffer() const override { return m_IndexBuffer; }

    private:
        uint32_t m_RendererID;
        std::vector<VertexBuffer*> m_VertexBuffers;
        IndexBuffer* m_IndexBuffer = nullptr;
    };

}
