#pragma once

#include <string>
#include <vector>
#include <memory>

namespace PillarEditor
{
    /**
     * EntityTemplate - Represents a reusable entity configuration
     * Templates can be saved from existing entities and instantiated multiple times
     */
    struct EntityTemplate
    {
        std::string Name;                    // Display name of the template
        std::string FilePath;                // Path to the .template file
        std::string ComponentsJSON;          // Serialized component data
        std::string IconPath;                // Optional preview icon
        
        // Metadata
        std::string Description;             // Optional template description
        std::vector<std::string> Tags;       // Tags for filtering/searching

        EntityTemplate() = default;
        EntityTemplate(const std::string& name, const std::string& componentsJSON)
            : Name(name), ComponentsJSON(componentsJSON) {}
    };
}
