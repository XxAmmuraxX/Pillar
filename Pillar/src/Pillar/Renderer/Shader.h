#pragma once

#include "Pillar/Core.h"
#include <glm/glm.hpp>
#include <string>

namespace Pillar {

    // Abstract Shader interface for platform-agnostic rendering
    class PIL_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetInt(const std::string& name, int value) = 0;
        virtual void SetFloat4(const std::string& name, const glm::vec4& value) = 0;
        virtual void SetMat4(const std::string& name, const glm::mat4& value) = 0;

        static Shader* Create(const std::string& vertexSrc, const std::string& fragmentSrc);
    };

}
