#pragma once

#include "EditorPanel.h"
#include "Pillar/Renderer/Texture.h"
#include <filesystem>
#include <unordered_map>
#include <memory>

namespace PillarEditor {

    struct AssetThumbnail
    {
        std::shared_ptr<Pillar::Texture2D> Texture;
        uint64_t LastModified = 0;
    };

    class ContentBrowserPanel : public EditorPanel
    {
    public:
        ContentBrowserPanel();

        virtual void OnImGuiRender() override;

        void SetBaseDirectory(const std::filesystem::path& path);

    private:
        void DrawBreadcrumbs();
        void DrawSearchBar();
        void DrawAssetGrid();
        void DrawAssetItem(const std::filesystem::path& path, bool isDirectory);
        void DrawContextMenu(const std::filesystem::path& path, bool isDirectory);
        
        const char* GetFileIcon(const std::filesystem::path& path, bool isDirectory);
        std::shared_ptr<Pillar::Texture2D> GetOrCreateThumbnail(const std::filesystem::path& path);
        bool MatchesSearch(const std::string& filename);
        
        void CreateFolder();
        void RenameAsset(const std::filesystem::path& path);
        void DeleteAsset(const std::filesystem::path& path);
        void RefreshDirectory();

    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
        
        // Search
        char m_SearchBuffer[256] = "";
        
        // Thumbnails
        std::unordered_map<std::string, AssetThumbnail> m_ThumbnailCache;
        std::shared_ptr<Pillar::Texture2D> m_FolderIcon;
        std::shared_ptr<Pillar::Texture2D> m_FileIcon;
        
        // UI state
        std::filesystem::path m_SelectedPath;
        bool m_ShowCreateFolderDialog = false;
        char m_NewFolderName[128] = "New Folder";
        
        float m_ThumbnailSize = 96.0f;
    };

}
