#include "Platform/OpenGL/OpenGLRenderAPI.h"
#include "Pillar/Renderer/Buffer.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Logger.h"

#include <glad/gl.h>

namespace Pillar {

    void OpenGLRenderAPI::Init()
    {
        PIL_CORE_INFO("Initializing OpenGL Renderer API");
        
        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Disable depth testing for 2D rendering
        // Sprites should be sorted by Z-index and rendered back-to-front
        // Depth testing causes transparent pixels to block sprites behind them
        glDisable(GL_DEPTH_TEST);

        PIL_CORE_INFO("OpenGL Info:");
        PIL_CORE_INFO("  Vendor:   {0}", (const char*)glGetString(GL_VENDOR));
        PIL_CORE_INFO("  Renderer: {0}", (const char*)glGetString(GL_RENDERER));
        PIL_CORE_INFO("  Version:  {0}", (const char*)glGetString(GL_VERSION));
    }

    void OpenGLRenderAPI::SetClearColor(const glm::vec4& color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRenderAPI::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void OpenGLRenderAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        glViewport(x, y, width, height);
    }

    void OpenGLRenderAPI::DrawIndexed(const VertexArray* vertexArray)
    {
        // Ensure vertex array bound before draw
        vertexArray->Bind();
        glDrawElements(GL_TRIANGLES, vertexArray->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, nullptr);
    }

}
