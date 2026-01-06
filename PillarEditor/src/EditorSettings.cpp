#include "EditorSettings.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace PillarEditor {

    EditorSettings& EditorSettings::Get()
    {
        static EditorSettings instance;
        return instance;
    }

    void EditorSettings::Load(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cout << "[EditorSettings] Settings file not found, using defaults: " << filepath << std::endl;
            return;
        }

        try
        {
            json j;
            file >> j;

            // Auto-save settings
            if (j.contains("autoSave"))
            {
                AutoSaveEnabled = j["autoSave"].value("enabled", true);
                AutoSaveInterval = j["autoSave"].value("interval", 300.0f);
            }

            // Viewport settings
            if (j.contains("viewport"))
            {
                ShowGrid = j["viewport"].value("showGrid", true);
                GridSize = j["viewport"].value("gridSize", 1.0f);
                CameraSpeed = j["viewport"].value("cameraSpeed", 5.0f);
                CameraZoom = j["viewport"].value("cameraZoom", 5.0f);

                if (j["viewport"].contains("cameraPosition"))
                {
                    auto pos = j["viewport"]["cameraPosition"];
                    CameraPosition.x = pos.value("x", 0.0f);
                    CameraPosition.y = pos.value("y", 0.0f);
                    CameraPosition.z = pos.value("z", 0.0f);
                }
            }

            // Recent files
            if (j.contains("recentFiles"))
            {
                RecentFiles = j["recentFiles"].get<std::vector<std::string>>();
            }

            // Window layout
            if (j.contains("layout"))
            {
                WindowLayout = j["layout"].value("name", "default");
                RestoreWindowLayout = j["layout"].value("restore", true);
            }

            // Editor preferences
            if (j.contains("preferences"))
            {
                ShowFPS = j["preferences"].value("showFPS", true);
                ShowEntityCount = j["preferences"].value("showEntityCount", true);
                ConfirmOnDelete = j["preferences"].value("confirmOnDelete", true);
                AutoFocusOnSelect = j["preferences"].value("autoFocusOnSelect", true);
            }

            // Gizmo settings
            if (j.contains("gizmo"))
            {
                GizmoOperation = j["gizmo"].value("operation", 0);
                GizmoMode = j["gizmo"].value("mode", 0);
            }

            // Sprite settings
            if (j.contains("sprite"))
            {
                PixelsPerUnit = j["sprite"].value("pixelsPerUnit", 100.0f);
                AutoSizeSpritesOnLoad = j["sprite"].value("autoSizeOnLoad", false);
            }

            // Layer manager settings
            if (j.contains("layerManager"))
            {
                LayerManager::Get().LoadFromJSON(j["layerManager"]);
            }

            std::cout << "[EditorSettings] Loaded settings from: " << filepath << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorSettings] Failed to load settings: " << e.what() << std::endl;
        }
    }

    void EditorSettings::Save(const std::string& filepath)
    {
        try
        {
            json j;

            // Auto-save settings
            j["autoSave"]["enabled"] = AutoSaveEnabled;
            j["autoSave"]["interval"] = AutoSaveInterval;

            // Viewport settings
            j["viewport"]["showGrid"] = ShowGrid;
            j["viewport"]["gridSize"] = GridSize;
            j["viewport"]["cameraSpeed"] = CameraSpeed;
            j["viewport"]["cameraZoom"] = CameraZoom;
            j["viewport"]["cameraPosition"]["x"] = CameraPosition.x;
            j["viewport"]["cameraPosition"]["y"] = CameraPosition.y;
            j["viewport"]["cameraPosition"]["z"] = CameraPosition.z;

            // Recent files
            j["recentFiles"] = RecentFiles;

            // Window layout
            j["layout"]["name"] = WindowLayout;
            j["layout"]["restore"] = RestoreWindowLayout;

            // Editor preferences
            j["preferences"]["showFPS"] = ShowFPS;
            j["preferences"]["showEntityCount"] = ShowEntityCount;
            j["preferences"]["confirmOnDelete"] = ConfirmOnDelete;
            j["preferences"]["autoFocusOnSelect"] = AutoFocusOnSelect;

            // Gizmo settings
            j["gizmo"]["operation"] = GizmoOperation;
            j["gizmo"]["mode"] = GizmoMode;

            // Sprite settings
            j["sprite"]["pixelsPerUnit"] = PixelsPerUnit;
            j["sprite"]["autoSizeOnLoad"] = AutoSizeSpritesOnLoad;

            // Layer manager settings
            LayerManager::Get().SaveToJSON(j["layerManager"]);

            std::ofstream file(filepath);
            if (!file.is_open())
            {
                std::cerr << "[EditorSettings] Failed to open file for writing: " << filepath << std::endl;
                return;
            }

            file << j.dump(4); // Pretty print with 4-space indent
            std::cout << "[EditorSettings] Saved settings to: " << filepath << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "[EditorSettings] Failed to save settings: " << e.what() << std::endl;
        }
    }

    void EditorSettings::AddRecentFile(const std::string& filepath)
    {
        // Remove if already exists
        auto it = std::find(RecentFiles.begin(), RecentFiles.end(), filepath);
        if (it != RecentFiles.end())
        {
            RecentFiles.erase(it);
        }

        // Add to front
        RecentFiles.insert(RecentFiles.begin(), filepath);

        // Keep only last 10
        if (RecentFiles.size() > 10)
        {
            RecentFiles.resize(10);
        }
    }

    // ========================================================================
    // LayerManager Implementation
    // ========================================================================

    LayerManager& LayerManager::Get()
    {
        static LayerManager instance;
        return instance;
    }

    void LayerManager::InitializeDefaultLayers()
    {
        m_Layers.clear();
        
        // Initialize with sensible default layers
        AddLayer("Background", -100.0f);
        AddLayer("Terrain", -50.0f);
        AddLayer("Decoration", -10.0f);
        AddLayer("Default", 0.0f);
        AddLayer("Player", 10.0f);
        AddLayer("Enemies", 5.0f);
        AddLayer("Projectiles", 20.0f);
        AddLayer("Effects", 30.0f);
        AddLayer("UI Background", 100.0f);
        AddLayer("UI Foreground", 110.0f);
        AddLayer("UI Overlay", 120.0f);
        
        // Set some default colors for visual distinction
        GetLayer("Background")->color = glm::vec4(0.3f, 0.5f, 0.8f, 1.0f);    // Blue
        GetLayer("Terrain")->color = glm::vec4(0.5f, 0.7f, 0.3f, 1.0f);       // Green
        GetLayer("Player")->color = glm::vec4(1.0f, 0.8f, 0.2f, 1.0f);        // Yellow
        GetLayer("Enemies")->color = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f);       // Red
        GetLayer("Effects")->color = glm::vec4(0.9f, 0.5f, 1.0f, 1.0f);       // Purple
        GetLayer("UI Foreground")->color = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f); // Light gray
    }

    void LayerManager::AddLayer(const std::string& name, float zIndex)
    {
        // Don't add if already exists
        if (HasLayer(name))
        {
            std::cerr << "[LayerManager] Layer already exists: " << name << std::endl;
            return;
        }

        Layer layer;
        layer.name = name;
        layer.baseZIndex = zIndex;
        m_Layers.push_back(layer);
    }

    void LayerManager::RemoveLayer(const std::string& name)
    {
        // Don't remove "Default" layer
        if (name == "Default")
        {
            std::cerr << "[LayerManager] Cannot remove Default layer" << std::endl;
            return;
        }

        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
            [&name](const Layer& layer) { return layer.name == name; });

        if (it != m_Layers.end())
        {
            m_Layers.erase(it);
        }
    }

    LayerManager::Layer* LayerManager::GetLayer(const std::string& name)
    {
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
            [&name](const Layer& layer) { return layer.name == name; });

        return it != m_Layers.end() ? &(*it) : nullptr;
    }

    const LayerManager::Layer* LayerManager::GetLayer(const std::string& name) const
    {
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
            [&name](const Layer& layer) { return layer.name == name; });

        return it != m_Layers.end() ? &(*it) : nullptr;
    }

    bool LayerManager::HasLayer(const std::string& name) const
    {
        return GetLayer(name) != nullptr;
    }

    void LayerManager::RenameLayer(const std::string& oldName, const std::string& newName)
    {
        // Don't rename "Default" layer
        if (oldName == "Default")
        {
            std::cerr << "[LayerManager] Cannot rename Default layer" << std::endl;
            return;
        }

        // Check if new name already exists
        if (HasLayer(newName))
        {
            std::cerr << "[LayerManager] Layer already exists: " << newName << std::endl;
            return;
        }

        Layer* layer = GetLayer(oldName);
        if (layer)
        {
            layer->name = newName;
        }
    }

    void LayerManager::MoveLayer(size_t fromIndex, size_t toIndex)
    {
        if (fromIndex >= m_Layers.size() || toIndex >= m_Layers.size())
            return;

        if (fromIndex == toIndex)
            return;

        Layer temp = m_Layers[fromIndex];
        m_Layers.erase(m_Layers.begin() + fromIndex);
        m_Layers.insert(m_Layers.begin() + toIndex, temp);
    }

    void LayerManager::SaveToJSON(nlohmann::json& j) const
    {
        j["layers"] = nlohmann::json::array();
        
        for (const auto& layer : m_Layers)
        {
            nlohmann::json layerJson;
            layerJson["name"] = layer.name;
            layerJson["baseZIndex"] = layer.baseZIndex;
            layerJson["visible"] = layer.visible;
            layerJson["locked"] = layer.locked;
            layerJson["color"]["r"] = layer.color.r;
            layerJson["color"]["g"] = layer.color.g;
            layerJson["color"]["b"] = layer.color.b;
            layerJson["color"]["a"] = layer.color.a;
            
            j["layers"].push_back(layerJson);
        }
    }

    void LayerManager::LoadFromJSON(const nlohmann::json& j)
    {
        if (!j.contains("layers"))
            return;

        m_Layers.clear();
        
        for (const auto& layerJson : j["layers"])
        {
            Layer layer;
            layer.name = layerJson.value("name", "Unknown");
            layer.baseZIndex = layerJson.value("baseZIndex", 0.0f);
            layer.visible = layerJson.value("visible", true);
            layer.locked = layerJson.value("locked", false);
            
            if (layerJson.contains("color"))
            {
                layer.color.r = layerJson["color"].value("r", 1.0f);
                layer.color.g = layerJson["color"].value("g", 1.0f);
                layer.color.b = layerJson["color"].value("b", 1.0f);
                layer.color.a = layerJson["color"].value("a", 1.0f);
            }
            
            m_Layers.push_back(layer);
        }

        // Ensure we always have a Default layer
        if (!HasLayer("Default"))
        {
            AddLayer("Default", 0.0f);
        }
    }

} // namespace PillarEditor

