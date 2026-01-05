#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace PillarEditor {

    /**
     * @brief Manages editor settings and preferences
     * 
     * Singleton class that handles loading/saving editor configuration to JSON.
     * Settings include auto-save preferences, viewport options, recent files, etc.
     */
    class EditorSettings
    {
    public:
        // Singleton access
        static EditorSettings& Get();

        // Persistence
        void Load(const std::string& filepath = "EditorSettings.json");
        void Save(const std::string& filepath = "EditorSettings.json");

        // Auto-save settings
        bool AutoSaveEnabled = true;
        float AutoSaveInterval = 300.0f;  // 5 minutes in seconds
        float TimeSinceLastSave = 0.0f;

        // Viewport settings
        bool ShowGrid = true;
        float GridSize = 1.0f;
        float CameraSpeed = 5.0f;
        glm::vec3 CameraPosition = { 0.0f, 0.0f, 0.0f };
        float CameraZoom = 5.0f;

        // Recent files (max 10)
        std::vector<std::string> RecentFiles;
        void AddRecentFile(const std::string& filepath);
        void ClearRecentFiles() { RecentFiles.clear(); }

        // Window layout
        std::string WindowLayout = "default";
        bool RestoreWindowLayout = true;

        // Editor preferences
        bool ShowFPS = true;
        bool ShowEntityCount = true;
        bool ConfirmOnDelete = true;
        bool AutoFocusOnSelect = true;

        // Gizmo settings
        int GizmoOperation = 0; // 0 = Translate, 1 = Rotate, 2 = Scale
        int GizmoMode = 0;      // 0 = Local, 1 = World

        // Sprite settings
        float PixelsPerUnit = 100.0f; // How many pixels = 1 world unit (default: 100, Unity standard)
        bool AutoSizeSpritesOnLoad = false; // Automatically size sprites to match texture dimensions

    private:
        EditorSettings() = default;
        ~EditorSettings() = default;
        EditorSettings(const EditorSettings&) = delete;
        EditorSettings& operator=(const EditorSettings&) = delete;
    };

} // namespace PillarEditor
