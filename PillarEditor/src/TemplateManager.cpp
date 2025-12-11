#include "TemplateManager.h"
#include "Pillar/ECS/ComponentRegistry.h"
#include "Pillar/Logger.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace PillarEditor
{
    TemplateManager::TemplateManager()
        : m_TemplatesDirectory("assets/templates")
    {
        // Ensure builtin components are registered
        Pillar::ComponentRegistry::Get().EnsureBuiltinsRegistered();
        
        // Create templates directory if it doesn't exist
        if (!std::filesystem::exists(m_TemplatesDirectory))
        {
            std::filesystem::create_directories(m_TemplatesDirectory);
        }
        
        LoadTemplatesFromDirectory();
    }

    bool TemplateManager::SaveEntityAsTemplate(const Pillar::Entity& entity, 
                                              const std::string& templateName,
                                              const std::string& description)
    {
        if (!entity.IsValid())
        {
            PIL_CORE_ERROR("Cannot save invalid entity as template");
            return false;
        }

        if (templateName.empty())
        {
            PIL_CORE_ERROR("Template name cannot be empty");
            return false;
        }

        // Serialize entity components
        std::string componentsJSON = SerializeEntity(entity);

        // Create template
        EntityTemplate templateData;
        templateData.Name = templateName;
        templateData.ComponentsJSON = componentsJSON;
        templateData.Description = description;
        templateData.FilePath = GetTemplateFilePath(templateName);

        // Save to file
        if (!SaveTemplateToFile(templateData))
        {
            PIL_CORE_ERROR("Failed to save template: {}", templateName);
            return false;
        }

        // Add to loaded templates
        m_Templates.push_back(templateData);
        PIL_CORE_INFO("Saved entity template: {}", templateName);
        return true;
    }

    Pillar::Entity TemplateManager::InstantiateTemplate(const std::string& templateName,
                                                       std::shared_ptr<Pillar::Scene> scene)
    {
        if (!scene)
        {
            PIL_CORE_ERROR("Cannot instantiate template into null scene");
            return Pillar::Entity();
        }

        // Find template
        auto it = std::find_if(m_Templates.begin(), m_Templates.end(),
            [&](const EntityTemplate& t) { return t.Name == templateName; });

        if (it == m_Templates.end())
        {
            PIL_CORE_ERROR("Template not found: {}", templateName);
            return Pillar::Entity();
        }

        // Create new entity
        Pillar::Entity newEntity = scene->CreateEntity(templateName);

        // Deserialize components into entity
        DeserializeIntoEntity(it->ComponentsJSON, newEntity);

        PIL_CORE_INFO("Instantiated template: {}", templateName);
        return newEntity;
    }

    void TemplateManager::LoadTemplatesFromDirectory()
    {
        m_Templates.clear();

        if (!std::filesystem::exists(m_TemplatesDirectory))
        {
            return;
        }

        for (const auto& entry : std::filesystem::directory_iterator(m_TemplatesDirectory))
        {
            if (entry.path().extension() == ".template")
            {
                try
                {
                    EntityTemplate templateData = LoadTemplateFromFile(entry.path().string());
                    m_Templates.push_back(templateData);
                }
                catch (const std::exception& e)
                {
                    PIL_CORE_ERROR("Failed to load template: {} - {}", 
                                 entry.path().string(), e.what());
                }
            }
        }

        PIL_CORE_INFO("Loaded {} templates", m_Templates.size());
    }

    bool TemplateManager::DeleteTemplate(const std::string& templateName)
    {
        auto it = std::find_if(m_Templates.begin(), m_Templates.end(),
            [&](const EntityTemplate& t) { return t.Name == templateName; });

        if (it == m_Templates.end())
        {
            return false;
        }

        // Delete file
        try
        {
            std::filesystem::remove(it->FilePath);
        }
        catch (const std::exception& e)
        {
            PIL_CORE_ERROR("Failed to delete template file: {}", e.what());
            return false;
        }

        // Remove from list
        m_Templates.erase(it);
        PIL_CORE_INFO("Deleted template: {}", templateName);
        return true;
    }

    bool TemplateManager::TemplateExists(const std::string& templateName) const
    {
        return std::find_if(m_Templates.begin(), m_Templates.end(),
            [&](const EntityTemplate& t) { return t.Name == templateName; }) != m_Templates.end();
    }

    void TemplateManager::SetTemplatesDirectory(const std::string& path)
    {
        m_TemplatesDirectory = path;
        
        if (!std::filesystem::exists(m_TemplatesDirectory))
        {
            std::filesystem::create_directories(m_TemplatesDirectory);
        }
        
        LoadTemplatesFromDirectory();
    }

    std::string TemplateManager::SerializeEntity(const Pillar::Entity& entity)
    {
        json entityJson;
        
        // Serialize all registered components using the component registry
        auto& componentRegistry = Pillar::ComponentRegistry::Get();
        PIL_CORE_INFO("Serializing entity template - {} registered components", 
                     componentRegistry.GetRegistrations().size());
        
        for (const auto& [key, registration] : componentRegistry.GetRegistrations())
        {
            json componentJson = registration.Serialize(entity);
            if (!componentJson.is_null())
            {
                PIL_CORE_INFO("  Serialized component: {}", key);
                entityJson[key] = componentJson;
            }
        }
        
        PIL_CORE_INFO("Final JSON size: {} bytes", entityJson.dump().size());
        return entityJson.dump(4); // Pretty print with 4 spaces
    }

    void TemplateManager::DeserializeIntoEntity(const std::string& jsonData, Pillar::Entity& entity)
    {
        try
        {
            json entityJson = json::parse(jsonData);
            
            // Deserialize all components using the component registry
            auto& componentRegistry = Pillar::ComponentRegistry::Get();
            for (const auto& [key, registration] : componentRegistry.GetRegistrations())
            {
                if (entityJson.contains(key))
                {
                    registration.Deserialize(entity, entityJson[key]);
                }
            }
        }
        catch (const std::exception& e)
        {
            PIL_CORE_ERROR("Failed to deserialize entity template: {}", e.what());
        }
    }

    std::string TemplateManager::GetTemplateFilePath(const std::string& templateName) const
    {
        std::string filename = templateName;
        
        // Replace spaces with underscores
        std::replace(filename.begin(), filename.end(), ' ', '_');
        
        // Remove invalid filename characters
        filename.erase(std::remove_if(filename.begin(), filename.end(),
            [](char c) { return !std::isalnum(c) && c != '_' && c != '-'; }), filename.end());
        
        return m_TemplatesDirectory + "/" + filename + ".template";
    }

    bool TemplateManager::SaveTemplateToFile(const EntityTemplate& templateData)
    {
        try
        {
            json j;
            j["name"] = templateData.Name;
            j["description"] = templateData.Description;
            j["components"] = json::parse(templateData.ComponentsJSON);
            j["iconPath"] = templateData.IconPath;
            j["tags"] = templateData.Tags;

            PIL_CORE_INFO("Saving template to: {}", templateData.FilePath);
            PIL_CORE_INFO("Components JSON:\n{}", templateData.ComponentsJSON);

            std::ofstream file(templateData.FilePath);
            if (!file.is_open())
            {
                PIL_CORE_ERROR("Failed to open file for writing: {}", templateData.FilePath);
                return false;
            }

            file << j.dump(4);
            file.close();
            
            PIL_CORE_INFO("Template saved successfully");
            return true;
        }
        catch (const std::exception& e)
        {
            PIL_CORE_ERROR("Failed to save template file: {}", e.what());
            return false;
        }
    }

    EntityTemplate TemplateManager::LoadTemplateFromFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open template file: " + filePath);
        }

        json j;
        file >> j;
        file.close();

        EntityTemplate templateData;
        templateData.Name = j.value("name", "Unnamed Template");
        templateData.Description = j.value("description", "");
        templateData.FilePath = filePath;
        templateData.IconPath = j.value("iconPath", "");
        
        if (j.contains("tags") && j["tags"].is_array())
        {
            for (const auto& tag : j["tags"])
            {
                templateData.Tags.push_back(tag);
            }
        }

        // Store components as JSON string
        if (j.contains("components"))
        {
            templateData.ComponentsJSON = j["components"].dump();
        }

        return templateData;
    }
}
