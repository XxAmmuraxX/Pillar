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

} // namespace PillarEditor
