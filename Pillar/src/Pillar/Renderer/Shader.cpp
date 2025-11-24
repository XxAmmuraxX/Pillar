#include "Pillar/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Logger.h"


namespace Pillar {

    Shader* Shader::Create(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return new OpenGLShader(vertexSrc, fragmentSrc);
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

}
