#include "ContentBrowserPanel.h"
#include "ConsolePanel.h"
#include <imgui.h>
#include <algorithm>
#include <fstream>

#ifdef _WIN32
#define NOMINMAX  // Prevent Windows.h from defining min/max macros
#include <windows.h>
#include <shellapi.h>
#endif

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

        // Initialize placeholder icons (white 1x1 pixel textures)
        // In a production editor, you'd load actual icon images here
        m_FolderIcon = nullptr;  // Will use text icon for now
        m_FileIcon = nullptr;
    }

    void ContentBrowserPanel::SetBaseDirectory(const std::filesystem::path& path)
    {
        m_BaseDirectory = path;
        m_CurrentDirectory = path;
        m_ThumbnailCache.clear();  // Clear cache when changing base directory
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        ImGui::Begin("Content Browser");

        // Toolbar
        ImGui::BeginGroup();
        
        // Back button
        ImGui::BeginDisabled(m_CurrentDirectory == m_BaseDirectory);
        if (ImGui::Button("<- Back"))
        {
            m_CurrentDirectory = m_CurrentDirectory.parent_path();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        // Forward/Up button
        if (ImGui::Button("^ Up"))
        {
            if (m_CurrentDirectory != m_BaseDirectory)
                m_CurrentDirectory = m_CurrentDirectory.parent_path();
        }

        ImGui::SameLine();

        // Refresh button (F5)
        if (ImGui::Button("Refresh") || (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_F5)))
        {
            RefreshDirectory();
        }

        ImGui::SameLine();

        // Create folder button
        if (ImGui::Button("+ New Folder"))
        {
            m_ShowCreateFolderDialog = true;
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();

        // Thumbnail size slider
        ImGui::SetNextItemWidth(120);
        ImGui::SliderFloat("##ThumbnailSize", &m_ThumbnailSize, 64.0f, 196.0f, "%.0f");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Thumbnail Size");

        ImGui::EndGroup();

        // Breadcrumb navigation
        DrawBreadcrumbs();

        ImGui::Separator();

        // Search bar
        DrawSearchBar();

        ImGui::Separator();
        ImGui::Spacing();

        // Asset grid
        DrawAssetGrid();

        // Create folder dialog
        if (m_ShowCreateFolderDialog)
        {
            ImGui::OpenPopup("Create Folder");
            m_ShowCreateFolderDialog = false;
        }

        if (ImGui::BeginPopupModal("Create Folder", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Enter folder name:");
            ImGui::SetKeyboardFocusHere();
            bool enterPressed = ImGui::InputText("##FolderName", m_NewFolderName, sizeof(m_NewFolderName), ImGuiInputTextFlags_EnterReturnsTrue);
            
            ImGui::Spacing();
            
            if (ImGui::Button("Create", ImVec2(120, 0)) || enterPressed)
            {
                CreateFolder();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }

        ImGui::End();
    }

    void ContentBrowserPanel::DrawBreadcrumbs()
    {
        // Build breadcrumb path
        std::vector<std::filesystem::path> pathSegments;
        std::filesystem::path tempPath = m_CurrentDirectory;
        
        while (tempPath != m_BaseDirectory && !tempPath.empty())
        {
            pathSegments.push_back(tempPath);
            tempPath = tempPath.parent_path();
        }
        pathSegments.push_back(m_BaseDirectory);
        
        std::reverse(pathSegments.begin(), pathSegments.end());

        // Draw clickable breadcrumbs
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Assets");
        
        for (size_t i = 1; i < pathSegments.size(); i++)
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "/");
            ImGui::SameLine();
            
            std::string segmentName = pathSegments[i].filename().string();
            ImGui::PushID((int)i);
            
            if (ImGui::Button(segmentName.c_str()))
            {
                m_CurrentDirectory = pathSegments[i];
            }
            
            ImGui::PopID();
        }
    }

    void ContentBrowserPanel::DrawSearchBar()
    {
        ImGui::SetNextItemWidth(-1);
        bool searchChanged = ImGui::InputTextWithHint("##Search", "Search assets...", m_SearchBuffer, sizeof(m_SearchBuffer));
        
        if (ImGui::IsItemFocused() && ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_SearchBuffer[0] = '\0';
        }
        
        // Ctrl+F to focus search
        if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && 
            ImGui::IsKeyPressed(ImGuiKey_F) && ImGui::GetIO().KeyCtrl)
        {
            ImGui::SetKeyboardFocusHere(-1);
        }
    }

    void ContentBrowserPanel::DrawAssetGrid()
    {
        // Calculate grid layout
        float padding = 16.0f;
        float cellSize = m_ThumbnailSize + padding;

        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnCount = std::max(1, (int)(panelWidth / cellSize));

        ImGui::Columns(columnCount, nullptr, false);

        // Iterate directory contents
        if (!std::filesystem::exists(m_CurrentDirectory))
        {
            ImGui::TextDisabled("Directory not found");
            if (ImGui::Button("Create assets folder"))
            {
                std::filesystem::create_directories(m_BaseDirectory);
                m_CurrentDirectory = m_BaseDirectory;
            }
            ImGui::Columns(1);
            return;
        }

        // Collect and sort entries
        std::vector<std::filesystem::directory_entry> entries;
        try
        {
            for (auto& entry : std::filesystem::directory_iterator(m_CurrentDirectory))
            {
                std::string filename = entry.path().filename().string();
                
                // Skip hidden files
                if (filename[0] == '.')
                    continue;
                
                // Apply search filter
                if (m_SearchBuffer[0] != '\0' && !MatchesSearch(filename))
                    continue;
                
                entries.push_back(entry);
            }
        }
        catch (const std::exception& e)
        {
            ImGui::TextDisabled("Error reading directory");
        }

        // Sort: directories first, then alphabetically
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) {
            if (a.is_directory() != b.is_directory())
                return a.is_directory();
            return a.path().filename() < b.path().filename();
        });

        // Draw items
        for (const auto& entry : entries)
        {
            DrawAssetItem(entry.path(), entry.is_directory());
        }

        ImGui::Columns(1);
    }

    void ContentBrowserPanel::DrawAssetItem(const std::filesystem::path& path, bool isDirectory)
    {
        std::string filenameString = path.filename().string();
        
        ImGui::PushID(filenameString.c_str());
        ImGui::BeginGroup();

        // Button with thumbnail/icon
        ImVec4 buttonColor = isDirectory ? 
            ImVec4(0.25f, 0.35f, 0.50f, 1.0f) :  // Blue for folders
            ImVec4(0.30f, 0.30f, 0.32f, 1.0f);   // Gray for files

        bool isSelected = (m_SelectedPath == path);
        if (isSelected)
        {
            buttonColor = ImVec4(0.2f, 0.5f, 0.8f, 1.0f);  // Highlight selected
        }

        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(buttonColor.x + 0.1f, buttonColor.y + 0.1f, buttonColor.z + 0.1f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(buttonColor.x + 0.15f, buttonColor.y + 0.15f, buttonColor.z + 0.15f, 1.0f));

        // Try to get thumbnail for images
        std::shared_ptr<Pillar::Texture2D> thumbnail = nullptr;
        if (!isDirectory)
        {
            thumbnail = GetOrCreateThumbnail(path);
        }

        // Draw button
        if (thumbnail)
        {
            // Draw image thumbnail (flip V coordinate for OpenGL)
            ImGui::ImageButton(filenameString.c_str(), (ImTextureID)(uint64_t)thumbnail->GetRendererID(), 
                              ImVec2(m_ThumbnailSize, m_ThumbnailSize),
                              ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            // Draw text icon
            const char* icon = GetFileIcon(path, isDirectory);
            ImGui::Button(icon, ImVec2(m_ThumbnailSize, m_ThumbnailSize));
        }

        ImGui::PopStyleColor(3);

        // Drag source for assets
        if (!isDirectory && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            std::string pathStr = path.string();
            ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", pathStr.c_str(), pathStr.size() + 1);
            ImGui::Text("%s", filenameString.c_str());
            ImGui::EndDragDropSource();
        }

        // Click to select
        if (ImGui::IsItemClicked())
        {
            m_SelectedPath = path;
        }

        // Double-click handling
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (isDirectory)
            {
                m_CurrentDirectory = path;
                m_SelectedPath.clear();
            }
            else
            {
                // TODO: Open scene files in editor
                if (path.extension() == ".scene" || path.filename().string().find(".scene.json") != std::string::npos)
                {
                    ConsolePanel::Log("Double-clicked scene: " + path.filename().string(), LogLevel::Info);
                }
            }
        }

        // Context menu
        DrawContextMenu(path, isDirectory);

        // Tooltip
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", filenameString.c_str());
            
            if (!isDirectory)
            {
                try
                {
                    auto fileSize = std::filesystem::file_size(path);
                    if (fileSize < 1024)
                        ImGui::TextDisabled("%llu bytes", fileSize);
                    else if (fileSize < 1024 * 1024)
                        ImGui::TextDisabled("%.1f KB", fileSize / 1024.0f);
                    else
                        ImGui::TextDisabled("%.1f MB", fileSize / (1024.0f * 1024.0f));
                        
                    auto lastWrite = std::filesystem::last_write_time(path);
                    ImGui::TextDisabled("Modified: %s", "Recently");  // Simplified for now
                }
                catch (...) {}
            }
            
            ImGui::EndTooltip();
        }

        // Filename label
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + m_ThumbnailSize);
        ImGui::TextWrapped("%s", filenameString.c_str());
        ImGui::PopTextWrapPos();

        ImGui::EndGroup();
        ImGui::NextColumn();
        ImGui::PopID();
    }

    void ContentBrowserPanel::DrawContextMenu(const std::filesystem::path& path, bool isDirectory)
    {
        if (ImGui::BeginPopupContextItem())
        {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", path.filename().string().c_str());
            ImGui::Separator();

            if (ImGui::MenuItem("Rename", "F2"))
            {
                RenameAsset(path);
            }

            if (ImGui::MenuItem("Delete", "Del"))
            {
                DeleteAsset(path);
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Copy Path"))
            {
                // Copy to clipboard
#ifdef _WIN32
                if (OpenClipboard(nullptr))
                {
                    EmptyClipboard();
                    std::string pathStr = path.string();
                    HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, pathStr.size() + 1);
                    if (hg)
                    {
                        memcpy(GlobalLock(hg), pathStr.c_str(), pathStr.size() + 1);
                        GlobalUnlock(hg);
                        SetClipboardData(CF_TEXT, hg);
                    }
                    CloseClipboard();
                    ConsolePanel::Log("Copied path to clipboard", LogLevel::Info);
                }
#endif
            }

            if (ImGui::MenuItem("Show in Explorer"))
            {
#ifdef _WIN32
                std::string command = "explorer /select," + path.string();
                system(command.c_str());
#endif
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Refresh", "F5"))
            {
                RefreshDirectory();
            }

            ImGui::EndPopup();
        }
    }

    std::shared_ptr<Pillar::Texture2D> ContentBrowserPanel::GetOrCreateThumbnail(const std::filesystem::path& path)
    {
        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        // Only generate thumbnails for images
        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" && 
            extension != ".bmp" && extension != ".tga")
        {
            return nullptr;
        }

        // Check cache
        std::string pathStr = path.string();
        auto it = m_ThumbnailCache.find(pathStr);
        
        if (it != m_ThumbnailCache.end())
        {
            // Check if file has been modified since caching
            try
            {
                auto lastWrite = std::filesystem::last_write_time(path);
                auto lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
                    lastWrite.time_since_epoch()).count();
                
                if (it->second.LastModified == (uint64_t)lastWriteTime)
                {
                    return it->second.Texture;
                }
            }
            catch (...) {}
        }

        // Load texture
        try
        {
            auto texture = Pillar::Texture2D::Create(pathStr);
            if (texture)
            {
                // Cache it
                auto lastWrite = std::filesystem::last_write_time(path);
                auto lastWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
                    lastWrite.time_since_epoch()).count();
                
                AssetThumbnail thumbnail;
                thumbnail.Texture = texture;
                thumbnail.LastModified = (uint64_t)lastWriteTime;
                m_ThumbnailCache[pathStr] = thumbnail;
                
                return texture;
            }
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to load thumbnail: " + pathStr, LogLevel::Warn);
        }

        return nullptr;
    }

    bool ContentBrowserPanel::MatchesSearch(const std::string& filename)
    {
        if (m_SearchBuffer[0] == '\0')
            return true;

        std::string lowerFilename = filename;
        std::string lowerSearch = m_SearchBuffer;
        
        std::transform(lowerFilename.begin(), lowerFilename.end(), lowerFilename.begin(), ::tolower);
        std::transform(lowerSearch.begin(), lowerSearch.end(), lowerSearch.begin(), ::tolower);
        
        return lowerFilename.find(lowerSearch) != std::string::npos;
    }

    void ContentBrowserPanel::CreateFolder()
    {
        if (m_NewFolderName[0] == '\0')
            return;

        std::filesystem::path newPath = m_CurrentDirectory / m_NewFolderName;
        
        if (std::filesystem::exists(newPath))
        {
            ConsolePanel::Log("Folder already exists: " + std::string(m_NewFolderName), LogLevel::Warn);
            return;
        }

        try
        {
            std::filesystem::create_directory(newPath);
            ConsolePanel::Log("Created folder: " + std::string(m_NewFolderName), LogLevel::Info);
            
            // Reset dialog
            strcpy_s(m_NewFolderName, sizeof(m_NewFolderName), "New Folder");
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to create folder: " + std::string(e.what()), LogLevel::Error);
        }
    }

    void ContentBrowserPanel::RenameAsset(const std::filesystem::path& path)
    {
        // TODO: Show rename dialog with InputText
        // For now, just log
        ConsolePanel::Log("Rename not yet implemented for: " + path.filename().string(), LogLevel::Info);
    }

    void ContentBrowserPanel::DeleteAsset(const std::filesystem::path& path)
    {
        // TODO: Show confirmation dialog
        try
        {
            if (std::filesystem::is_directory(path))
            {
                std::filesystem::remove_all(path);
                ConsolePanel::Log("Deleted folder: " + path.filename().string(), LogLevel::Info);
            }
            else
            {
                std::filesystem::remove(path);
                ConsolePanel::Log("Deleted file: " + path.filename().string(), LogLevel::Info);
            }
            
            // Clear from cache if it was there
            m_ThumbnailCache.erase(path.string());
            
            if (m_SelectedPath == path)
            {
                m_SelectedPath.clear();
            }
        }
        catch (const std::exception& e)
        {
            ConsolePanel::Log("Failed to delete: " + std::string(e.what()), LogLevel::Error);
        }
    }

    void ContentBrowserPanel::RefreshDirectory()
    {
        m_ThumbnailCache.clear();
        ConsolePanel::Log("Refreshed content browser", LogLevel::Info);
    }

    const char* ContentBrowserPanel::GetFileIcon(const std::filesystem::path& path, bool isDirectory)
    {
        if (isDirectory)
            return "📁";  // Folder emoji

        std::string extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || 
            extension == ".bmp" || extension == ".tga")
            return "🖼️";  // Image emoji
        
        if (extension == ".json")
            return "📄";
        
        if (path.filename().string().find(".scene") != std::string::npos)
            return "🎬";  // Scene emoji
        
        if (extension == ".glsl" || extension == ".hlsl" || extension == ".shader")
            return "📜";
        
        if (extension == ".cpp" || extension == ".h" || extension == ".hpp")
            return "📝";
        
        if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
            return "🔊";  // Audio emoji
        
        if (extension == ".txt" || extension == ".md")
            return "📃";

        return "📄";  // Generic file
    }

}
