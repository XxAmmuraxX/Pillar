#pragma once

#include "Pillar/Core.h"

namespace Pillar {

    class VertexBuffer;
    class IndexBuffer;

    class PIL_API VertexArray
    {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) = 0;
        virtual void SetIndexBuffer(IndexBuffer* indexBuffer) = 0;

        virtual IndexBuffer* GetIndexBuffer() const = 0;

        static VertexArray* Create();
    };

}
