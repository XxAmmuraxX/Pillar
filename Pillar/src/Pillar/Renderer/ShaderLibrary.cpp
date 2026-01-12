#include "ShaderLibrary.h"
#include "Pillar/Logger.h"
#include "Pillar/Renderer/Shader.h"

namespace Pillar {

    ShaderLibrary& ShaderLibrary::GetInstance()
    {
        static ShaderLibrary instance;
        return instance;
    }

    std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name,
                                                   const std::string& vertexPath,
                                                   const std::string& fragmentPath)
    {
        // Check if shader already exists
        if (Exists(name))
        {
            PIL_CORE_WARN("Shader '{0}' already exists in library. Returning existing shader.", name);
            return Get(name);
        }

        // Load shader from files
        PIL_CORE_INFO("Loading shader '{0}' from files: {1}, {2}", name, vertexPath, fragmentPath);
        
        auto shader = Shader::CreateFromFile(vertexPath, fragmentPath);
        if (!shader)
        {
            PIL_CORE_ERROR("Failed to load shader '{0}'", name);
            return nullptr;
        }

        // Store in library with paths for hot-reload
        ShaderEntry entry;
        entry.Shader = shader;
        entry.VertexPath = vertexPath;
        entry.FragmentPath = fragmentPath;
        m_Shaders[name] = entry;

        PIL_CORE_INFO("Shader '{0}' loaded successfully", name);
        return shader;
    }

    std::shared_ptr<Shader> ShaderLibrary::LoadFromSource(const std::string& name,
                                                             const std::string& vertexSrc,
                                                             const std::string& fragmentSrc)
    {
        // Check if shader already exists
        if (Exists(name))
        {
            PIL_CORE_WARN("Shader '{0}' already exists in library. Returning existing shader.", name);
            return Get(name);
        }

        // Create shader from source
        PIL_CORE_INFO("Loading shader '{0}' from source", name);
        
        auto shader = Shader::Create(vertexSrc, fragmentSrc);
        if (!shader)
        {
            PIL_CORE_ERROR("Failed to create shader '{0}' from source", name);
            return nullptr;
        }

        // Store in library (no paths - can't hot-reload)
        ShaderEntry entry;
        entry.Shader = shader;
        entry.VertexPath = "";
        entry.FragmentPath = "";
        m_Shaders[name] = entry;

        PIL_CORE_INFO("Shader '{0}' loaded from source successfully", name);
        return shader;
    }

    void ShaderLibrary::Add(const std::string& name, std::shared_ptr<Shader> shader)
    {
        if (Exists(name))
        {
            PIL_CORE_WARN("Shader '{0}' already exists in library. Overwriting.", name);
        }

        ShaderEntry entry;
        entry.Shader = shader;
        entry.VertexPath = "";
        entry.FragmentPath = "";
        m_Shaders[name] = entry;

        PIL_CORE_INFO("Shader '{0}' added to library", name);
    }

    std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name)
    {
        auto it = m_Shaders.find(name);
        if (it != m_Shaders.end())
        {
            return it->second.Shader;
        }

        PIL_CORE_ERROR("Shader '{0}' not found in library", name);
        return nullptr;
    }

    bool ShaderLibrary::Exists(const std::string& name) const
    {
        return m_Shaders.find(name) != m_Shaders.end();
    }

    bool ShaderLibrary::Reload(const std::string& name)
    {
        auto it = m_Shaders.find(name);
        if (it == m_Shaders.end())
        {
            PIL_CORE_ERROR("Cannot reload shader '{0}' - not found in library", name);
            return false;
        }

        auto& entry = it->second;

        // Check if shader was loaded from files (has paths)
        if (entry.VertexPath.empty() || entry.FragmentPath.empty())
        {
            PIL_CORE_WARN("Cannot reload shader '{0}' - was not loaded from files", name);
            return false;
        }

        PIL_CORE_INFO("Reloading shader '{0}'...", name);

        // Load new shader from files
        auto newShader = Shader::CreateFromFile(entry.VertexPath, entry.FragmentPath);
        if (!newShader)
        {
            PIL_CORE_ERROR("Failed to reload shader '{0}'", name);
            return false;
        }

        // Replace old shader with new one
        entry.Shader = newShader;

        PIL_CORE_INFO("Shader '{0}' reloaded successfully", name);
        return true;
    }

    uint32_t ShaderLibrary::ReloadAll()
    {
        PIL_CORE_INFO("Reloading all shaders...");
        
        uint32_t successCount = 0;
        for (auto& [name, entry] : m_Shaders)
        {
            // Skip shaders not loaded from files
            if (entry.VertexPath.empty() || entry.FragmentPath.empty())
            {
                continue;
            }

            if (Reload(name))
            {
                successCount++;
            }
        }

        PIL_CORE_INFO("Reloaded {0} shaders", successCount);
        return successCount;
    }

    void ShaderLibrary::Remove(const std::string& name)
    {
        auto it = m_Shaders.find(name);
        if (it != m_Shaders.end())
        {
            m_Shaders.erase(it);
            PIL_CORE_INFO("Shader '{0}' removed from library", name);
        }
        else
        {
            PIL_CORE_WARN("Cannot remove shader '{0}' - not found in library", name);
        }
    }

    void ShaderLibrary::Clear()
    {
        PIL_CORE_INFO("Clearing shader library ({0} shaders)", m_Shaders.size());
        m_Shaders.clear();
    }

    std::vector<std::string> ShaderLibrary::GetShaderNames() const
    {
        std::vector<std::string> names;
        names.reserve(m_Shaders.size());
        
        for (const auto& [name, entry] : m_Shaders)
        {
            names.push_back(name);
        }
        
        return names;
    }

}
