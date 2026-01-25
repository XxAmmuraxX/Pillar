#pragma once

#include "Pillar/Renderer/Shader.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Pillar {

    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& vertexSrc, const std::string& fragmentSrc);
        ~OpenGLShader() override;

        void Bind() const override;
        void Unbind() const override;

        void SetInt(const std::string& name, int value) override;
        void SetIntArray(const std::string& name, int* values, uint32_t count) override;
        void SetFloat(const std::string& name, float value) override;
        void SetFloat4(const std::string& name, const glm::vec4& value) override;
        void SetMat4(const std::string& name, const glm::mat4& value) override;

    private:
        int GetUniformLocation(const std::string& name) const;
        
        uint32_t m_RendererID;
        mutable std::unordered_map<std::string, int> m_UniformLocationCache;
    };

}
