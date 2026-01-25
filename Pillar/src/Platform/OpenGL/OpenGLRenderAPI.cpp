#include "Platform/OpenGL/OpenGLRenderAPI.h"
#include "Pillar/Renderer/Buffer.h"
#include "Pillar/Renderer/VertexArray.h" // Added include for complete type
#include "Pillar/Logger.h"

#include <glad/gl.h>

namespace Pillar {

    void OpenGLRenderAPI::Init()
    {
        PIL_CORE_INFO("Initializing OpenGL Renderer API");
        
        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Enable depth testing
        glEnable(GL_DEPTH_TEST);

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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    bool OpenGLRenderAPI::GetDepthTest() const
    {
        return glIsEnabled(GL_DEPTH_TEST) == GL_TRUE;
    }

    bool OpenGLRenderAPI::GetDepthWrite() const
    {
        GLboolean depthMask = GL_TRUE;
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
        return depthMask == GL_TRUE;
    }

    bool OpenGLRenderAPI::GetBlending() const
    {
        return glIsEnabled(GL_BLEND) == GL_TRUE;
    }

    void OpenGLRenderAPI::SetDepthTest(bool enable)
    {
        if (enable)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void OpenGLRenderAPI::SetDepthWrite(bool enable)
    {
        glDepthMask(enable ? GL_TRUE : GL_FALSE);
    }

    void OpenGLRenderAPI::SetBlending(bool enable)
    {
        if (enable)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }

}
