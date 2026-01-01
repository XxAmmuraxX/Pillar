#include "Pillar/Renderer/Shader.h"
#include "Platform/OpenGL/OpenGLShader.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Pillar/Logger.h"
#include "Pillar/Utils/AssetManager.h"
#include <fstream>
#include <sstream>


namespace Pillar {

    static std::string ReadFileToString(const std::string& filepath)
    {
        std::string result;
        std::ifstream in(filepath, std::ios::in | std::ios::binary);
        if (in)
        {
            in.seekg(0, std::ios::end);
            size_t size = in.tellg();
            if (size != -1)
            {
                result.resize(size);
                in.seekg(0, std::ios::beg);
                in.read(&result[0], size);
            }
            else
            {
                PIL_CORE_ERROR("Could not read from file '{0}'", filepath);
            }
        }
        else
        {
            PIL_CORE_ERROR("Could not open file '{0}'", filepath);
        }
        return result;
    }

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

    Shader* Shader::CreateFromFile(const std::string& vertexPath, const std::string& fragmentPath)
    {
        // Resolve paths using AssetManager
        std::string vertexFullPath = AssetManager::GetAssetPath(vertexPath);
        std::string fragmentFullPath = AssetManager::GetAssetPath(fragmentPath);

        PIL_CORE_INFO("Loading vertex shader from: {0}", vertexFullPath);
        PIL_CORE_INFO("Loading fragment shader from: {0}", fragmentFullPath);

        std::string vertexSrc = ReadFileToString(vertexFullPath);
        std::string fragmentSrc = ReadFileToString(fragmentFullPath);

        if (vertexSrc.empty() || fragmentSrc.empty())
        {
            PIL_CORE_ERROR("Failed to load shader files!");
            return nullptr;
        }

        return Create(vertexSrc, fragmentSrc);
    }

}
