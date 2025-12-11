#include "ContentBrowserPanel.h"
#include <imgui.h>
#include <algorithm>

namespace PillarEditor {

    ContentBrowserPanel::ContentBrowserPanel()
        : EditorPanel("Content Browser")
    {
        // Try multiple base directories
        std::vector<std::filesystem::path> possiblePaths = {
            std::filesystem::current_path() / "assets",
            std::filesystem::current_path() / "Sandbox" / "assets",
            std::filesystem::current_path() / "PillarEditor" / "assets",
            std::filesystem::current_path().parent_path() / "assets"
        };

        for (const auto& path : possiblePaths)
        {
            if (std::filesystem::exists(path))
            {
                m_BaseDirectory = path;
                m_CurrentDirectory = path;
                break;
            }
        }

        // Create assets directory if none found
        if (m_BaseDirectory.empty())
        {
            m_BaseDirectory = std::filesystem::current_path() / "assets";
            std::filesystem::create_directories(m_BaseDirectory);
            m_CurrentDirectory = m_BaseDirectory;
        }
    }

    void ContentBrowserPanel::SetBaseDirectory(const std::filesystem::path& path)
    {
        m_BaseDirectory = path;
        m_CurrentDirectory = path;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        // Navigation bar
        if (m_CurrentDirectory != m_BaseDirectory)
        {
            if (ImGui::Button("<- Back"))
            {
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
            }
            ImGui::SameLine();
        }

        // Show current path (clickable breadcrumbs)
        std::string relativePath;
        try
        {
            relativePath = std::filesystem::relative(m_CurrentDirectory, m_BaseDirectory).string();
            if (relativePath == ".")
                relativePath = "assets";
            else
                relativePath = "assets/" + relativePath;
        }
        catch (...)
        {
            relativePath = m_CurrentDirectory.filename().string();
        }
        
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", relativePath.c_str());

        ImGui::Separator();
        ImGui::Spacing();

        // Calculate item size for grid layout
        float padding = 12.0f;
        float thumbnailSize = 80.0f;
        float cellSize = thumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = std::max(1, (int)(panelWidth / cellSize));

        ImGui::Columns(columnCount, nullptr, false);

        // Iterate directory contents
        if (std::filesystem::exists(m_CurrentDirectory))
        {
            // Sort entries: directories first, then files
            std::vector<std::filesystem::directory_entry> entries;
            try
            {
                for (auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
                {
                    entries.push_back(entry);
                }
            }
            catch (const std::exception& e)
            {
                ImGui::TextDisabled("Error reading directory");
            }

            // Sort: directories first
            std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
                if (a.is_directory() != b.is_directory())
                    return a.is_directory();
                return a.path().filename() < b.path().filename();
            });

            for (const auto& directoryEntry : entries)
            {
                const auto& path = directoryEntry.path();
                std::string filenameString = path.filename().string();

                // Skip hidden files
                if (filenameString[0] == '.')
                    continue;

                ImGui::PushID(filenameString.c_str());

                // Style based on type
                bool isDirectory = directoryEntry.is_directory();
                ImVec4 buttonColor = isDirectory ? 
                    ImVec4(0.25f, 0.35f, 0.50f, 1.0f) :  // Blue-ish for folders
                    ImVec4(0.30f, 0.30f, 0.32f, 1.0f);   // Gray for files

                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonColor.x + 0.1f, buttonColor.y + 0.1f, buttonColor.z + 0.1f, 1.0f));
                
                // Create button with icon
                const char* icon = GetFileIcon(path, isDirectory);
                
                ImGui::Button(icon, ImVec2(thumbnailSize, thumbnailSize));

                ImGui::PopStyleColor(2);

                // Drag source for files (for drag-drop onto entities)
                if (!isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
                {
                    std::string pathStr = path.string();
                    ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
                    ImGui::Text("%s", filenameString.c_str());
                    ImGui::EndDragDropSource();
                }

                // Double-click handling
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    if (isDirectory)
                    {
                        m_CurrentDirectory = path;
                    }
                    else
                    {
                        // TODO: Open files (scenes, etc.)
                    }
                }

                // Tooltip with full path
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", filenameString.c_str());
                    if (!isDirectory)
                    {
                        auto fileSize = std::filesystem::file_size(path);
                        if (fileSize < 1024)
                            ImGui::TextDisabled("%llu bytes", fileSize);
                        else if (fileSize < 1024 * 1024)
                            ImGui::TextDisabled("%.1f KB", fileSize / 1024.0f);
                        else
                            ImGui::TextDisabled("%.1f MB", fileSize / (1024.0f * 1024.0f));
                    }
                    ImGui::EndTooltip();
                }

                // Filename label (truncated if too long)
                std::string displayName = filenameString;
                if (displayName.length() > 12)
                {
                    displayName = displayName.substr(0, 9) + "...";
                }
                
                ImGui::TextWrapped("%s", displayName.c_str());

                ImGui::NextColumn();
                ImGui::PopID();
            }
        }
        else
        {
            ImGui::TextDisabled("Directory not found");
            if (ImGui::Button("Create assets folder"))
            {
                std::filesystem::create_directories(m_BaseDirectory);
                m_CurrentDirectory = m_BaseDirectory;
            }
        }

        ImGui::Columns(1);

        ImGui::End();
    }

    const char* ContentBrowserPanel::GetFileIcon(const std::filesystem::path& path, bool isDirectory)
    {
        if (isDirectory)
            return "[DIR]";

        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
            extension == ".bmp" || extension == ".tga")
            return "[IMG]";
        
        if (extension == ".json")
            return "[JSON]";
        
        if (extension == ".scene.json" || extension == ".scene")
            return "[SCN]";
        
        if (extension == ".glsl" || extension == ".hlsl" || extension == ".shader")
            return "[SHD]";
        
        if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
            return "[C++]";
        
        if (extension == ".txt" || extension == ".md")
            return "[TXT]";

/*         if (extension == ".zip" || extension == ".rar" || extension == ".tar")
            return "[ARCHIVE]"; */

        return "[FILE]";
    }

}
