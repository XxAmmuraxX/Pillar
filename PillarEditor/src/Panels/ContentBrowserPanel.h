#pragma once

#include "EditorPanel.h"
#include <filesystem>

namespace PillarEditor {

    class ContentBrowserPanel : public EditorPanel
    {
    public:
        ContentBrowserPanel();

        virtual void OnImGuiRender() override;

        void SetBaseDirectory(const std::filesystem::path& path);

    private:
        const char* GetFileIcon(const std::filesystem::path& path, bool isDirectory);

    private:
        std::filesystem::path m_BaseDirectory;
        std::filesystem::path m_CurrentDirectory;
    };

}
