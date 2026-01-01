#pragma once

#include "EntityTemplate.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Entity.h"
#include <filesystem>
#include <unordered_map>

namespace PillarEditor
{
    /**
     * TemplateManager - Manages entity templates (saving, loading, instantiation)
     */
    class TemplateManager
    {
    public:
        TemplateManager();
        ~TemplateManager() = default;

        // Save entity as template
        bool SaveEntityAsTemplate(const Pillar::Entity& entity, const std::string& templateName, 
                                 const std::string& description = "");

        // Instantiate template into scene
        Pillar::Entity InstantiateTemplate(const std::string& templateName, 
                                          std::shared_ptr<Pillar::Scene> scene);

        // Template management
        void LoadTemplatesFromDirectory();
        const std::vector<EntityTemplate>& GetTemplates() const { return m_Templates; }
        bool DeleteTemplate(const std::string& templateName);
        bool TemplateExists(const std::string& templateName) const;

        // Directory management
        void SetTemplatesDirectory(const std::string& path);
        const std::string& GetTemplatesDirectory() const { return m_TemplatesDirectory; }

    private:
        std::string SerializeEntity(const Pillar::Entity& entity);
        void DeserializeIntoEntity(const std::string& jsonData, Pillar::Entity& entity);
        
        std::string GetTemplateFilePath(const std::string& templateName) const;
        bool SaveTemplateToFile(const EntityTemplate& templateData);
        EntityTemplate LoadTemplateFromFile(const std::string& filePath);

    private:
        std::vector<EntityTemplate> m_Templates;
        std::string m_TemplatesDirectory;
    };
}
