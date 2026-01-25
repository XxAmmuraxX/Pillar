#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Shader.h"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Pillar {

    /**
     * ShaderLibrary - Manages shader resources with caching and hot-reload support
     * 
     * This class provides:
     * - Shader loading from files with automatic path resolution
     * - Shader caching to avoid duplicate loads
     * - Named shader retrieval
     * - Hot-reload capability for development
     * 
     * Usage:
     *   auto& library = ShaderLibrary::GetInstance();
     *   library.Load("BatchQuad", "shaders/BatchQuad.vert", "shaders/BatchQuad.frag");
     *   auto shader = library.Get("BatchQuad");
     */
    class PIL_API ShaderLibrary
    {
    public:
        ShaderLibrary() = default;
        ~ShaderLibrary() = default;

        // Singleton access (optional - can also use instance-based)
        static ShaderLibrary& GetInstance();

        /**
         * Load a shader from file paths and store it with a name
         * @param name - Unique identifier for this shader
         * @param vertexPath - Path to vertex shader file (relative to assets/ or absolute)
         * @param fragmentPath - Path to fragment shader file (relative to assets/ or absolute)
         * @return Shared pointer to loaded shader, or nullptr on failure
         */
        std::shared_ptr<Shader> Load(const std::string& name, 
                                       const std::string& vertexPath, 
                                       const std::string& fragmentPath);

        /**
         * Load a shader from source strings
         * @param name - Unique identifier for this shader
         * @param vertexSrc - Vertex shader source code
         * @param fragmentSrc - Fragment shader source code
         * @return Shared pointer to loaded shader, or nullptr on failure
         */
        std::shared_ptr<Shader> LoadFromSource(const std::string& name,
                                                 const std::string& vertexSrc,
                                                 const std::string& fragmentSrc);

        /**
         * Add an existing shader to the library
         * @param name - Unique identifier for this shader
         * @param shader - Shared pointer to shader
         */
        void Add(const std::string& name, std::shared_ptr<Shader> shader);

        /**
         * Get a shader by name
         * @param name - Shader identifier
         * @return Shared pointer to shader, or nullptr if not found
         */
        std::shared_ptr<Shader> Get(const std::string& name);

        /**
         * Check if a shader exists in the library
         * @param name - Shader identifier
         * @return True if shader exists
         */
        bool Exists(const std::string& name) const;

        /**
         * Reload a shader from its stored file paths (hot-reload)
         * @param name - Shader identifier
         * @return True if reload succeeded
         */
        bool Reload(const std::string& name);

        /**
         * Reload all shaders that were loaded from files
         * @return Number of shaders successfully reloaded
         */
        uint32_t ReloadAll();

        /**
         * Remove a shader from the library
         * @param name - Shader identifier
         */
        void Remove(const std::string& name);

        /**
         * Clear all shaders from the library
         */
        void Clear();

        /**
         * Get all shader names currently in the library
         * @return Vector of shader names
         */
        std::vector<std::string> GetShaderNames() const;

    private:
        struct ShaderEntry
        {
            std::shared_ptr<Shader> Shader;
            std::string VertexPath;   // For hot-reload
            std::string FragmentPath; // For hot-reload
        };

        std::unordered_map<std::string, ShaderEntry> m_Shaders;
    };

}
