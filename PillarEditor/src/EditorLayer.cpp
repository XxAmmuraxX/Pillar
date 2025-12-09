#include "EditorLayer.h"
#include "Panels/ConsolePanel.h"
#include "Pillar/Application.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <filesystem>

namespace PillarEditor {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }

    void EditorLayer::OnAttach()
    {
        PIL_CORE_INFO("EditorLayer attached");

        // Setup custom editor style
        SetupImGuiStyle();

        // Create panels
        m_HierarchyPanel = std::make_unique<SceneHierarchyPanel>();
        m_InspectorPanel = std::make_unique<InspectorPanel>();
        m_ViewportPanel = std::make_unique<ViewportPanel>();
        m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();
        m_ConsolePanel = std::make_unique<ConsolePanel>();

        // Create a default scene with some entities for demonstration
        NewScene();
        CreateDefaultEntities();

        ConsolePanel::Log("Pillar Editor initialized", LogLevel::Info);
        ConsolePanel::Log("Controls:", LogLevel::Info);
        ConsolePanel::Log("  - Middle Mouse: Pan viewport", LogLevel::Trace);
        ConsolePanel::Log("  - Scroll Wheel: Zoom in/out", LogLevel::Trace);
        ConsolePanel::Log("  - F: Focus on selected entity", LogLevel::Trace);
        ConsolePanel::Log("  - H: Reset camera to origin", LogLevel::Trace);
    }

    void EditorLayer::OnDetach()
    {
        PIL_CORE_INFO("EditorLayer detached");
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        m_LastFrameTime = deltaTime;

        // Always update viewport panel - it handles its own hover checks internally
        m_ViewportPanel->OnUpdate(deltaTime);

        // Update scene in play mode
        if (m_EditorState == EditorState::Play)
        {
            m_ActiveScene->OnUpdate(deltaTime);
        }

        // Render scene to viewport framebuffer
        m_ViewportPanel->RenderScene();
    }

    void EditorLayer::SetupImGuiStyle()
    {
        ImGuiIO& io = ImGui::GetIO();
        
        // Set default font size
        io.FontGlobalScale = 1.0f;

        ImGuiStyle& style = ImGui::GetStyle();
        
        // Main style settings
        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(5.0f, 4.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);
        style.ItemSpacing = ImVec2(8.0f, 4.0f);
        style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
        style.IndentSpacing = 20.0f;
        style.ScrollbarSize = 14.0f;
        style.GrabMinSize = 12.0f;

        // Borders
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;
        style.TabBorderSize = 0.0f;

        // Rounding
        style.WindowRounding = 4.0f;
        style.ChildRounding = 4.0f;
        style.FrameRounding = 3.0f;
        style.PopupRounding = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding = 3.0f;
        style.TabRounding = 4.0f;

        // Colors - Dark theme inspired by VS Code
        ImVec4* colors = style.Colors;
        
        // Backgrounds
        colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.0f);
        
        // Borders
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        
        // Frame backgrounds
        colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
        
        // Title bar
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        
        // Menu bar
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        
        // Scrollbar
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.43f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.53f, 1.0f);
        
        // Buttons
        colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.35f, 1.0f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.45f, 0.75f, 1.0f);
        
        // Headers (collapsing headers, tree nodes, etc.)
        colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.28f, 0.28f, 0.32f, 1.0f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.45f, 0.75f, 1.0f);
        
        // Tabs
        colors[ImGuiCol_Tab] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.20f, 0.45f, 0.75f, 0.8f);
        colors[ImGuiCol_TabActive] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
        
        // Docking
        colors[ImGuiCol_DockingPreview] = ImVec4(0.20f, 0.45f, 0.75f, 0.7f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
        
        // Separator
        colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.40f, 0.45f, 1.0f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.20f, 0.45f, 0.75f, 1.0f);
        
        // Resize grip
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.28f, 0.5f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.40f, 0.45f, 0.8f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.20f, 0.45f, 0.75f, 1.0f);
        
        // Text
        colors[ImGuiCol_Text] = ImVec4(0.85f, 0.85f, 0.88f, 1.0f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.53f, 1.0f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.45f, 0.75f, 0.4f);
        
        // Checkmark, slider
        colors[ImGuiCol_CheckMark] = ImVec4(0.30f, 0.65f, 1.0f, 1.0f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.30f, 0.65f, 1.0f, 1.0f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.75f, 1.0f, 1.0f);
    }

    void EditorLayer::SetupDockspace()
    {
        ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
        
        // Check if dockspace already has a layout
        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
        {
            // Clear any previous layout
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            
            // Get main viewport size
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

            // Split the dockspace
            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_left_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.2f, nullptr, &dock_main_id);
            ImGuiID dock_right_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.25f, nullptr, &dock_main_id);
            ImGuiID dock_bottom_id = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, nullptr, &dock_main_id);

            // Dock windows to their respective areas
            ImGui::DockBuilderDockWindow("Scene Hierarchy", dock_left_id);
            ImGui::DockBuilderDockWindow("Inspector", dock_right_id);
            ImGui::DockBuilderDockWindow("Stats", dock_right_id);
            ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
            ImGui::DockBuilderDockWindow("Content Browser", dock_bottom_id);
            ImGui::DockBuilderDockWindow("Console", dock_bottom_id);

            ImGui::DockBuilderFinish(dockspace_id);
        }
    }

    void EditorLayer::CreateDefaultEntities()
    {
        // Create some default entities so the editor isn't empty
        auto player = m_ActiveScene->CreateEntity("Player");
        auto& playerTransform = player.GetComponent<Pillar::TransformComponent>();
        playerTransform.Position = { 0.0f, 0.0f };
        playerTransform.Scale = { 1.0f, 1.0f };

        auto ground = m_ActiveScene->CreateEntity("Ground");
        auto& groundTransform = ground.GetComponent<Pillar::TransformComponent>();
        groundTransform.Position = { 0.0f, -3.0f };
        groundTransform.Scale = { 10.0f, 1.0f };

        auto enemy1 = m_ActiveScene->CreateEntity("Enemy");
        auto& enemy1Transform = enemy1.GetComponent<Pillar::TransformComponent>();
        enemy1Transform.Position = { 3.0f, 0.0f };
        enemy1Transform.Scale = { 0.8f, 0.8f };

        auto enemy2 = m_ActiveScene->CreateEntity("Enemy");
        auto& enemy2Transform = enemy2.GetComponent<Pillar::TransformComponent>();
        enemy2Transform.Position = { -3.0f, 1.0f };
        enemy2Transform.Scale = { 0.8f, 0.8f };

        auto camera = m_ActiveScene->CreateEntity("Camera");
        auto& cameraTransform = camera.GetComponent<Pillar::TransformComponent>();
        cameraTransform.Position = { 0.0f, 0.0f };
    }

    void EditorLayer::OnImGuiRender()
    {
        // Enable dockspace
        static bool dockspaceOpen = true;
        static bool opt_fullscreen = true;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        if (opt_fullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
            window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &dockspaceOpen, window_flags);
        ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 250.0f;

        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            
            // Setup default layout on first frame
            static bool firstFrame = true;
            if (firstFrame)
            {
                SetupDockspace();
                firstFrame = false;
            }
        }

        style.WindowMinSize.x = minWinSizeX;

        // Draw menu bar
        DrawMenuBar();

        ImGui::End();

        // Draw toolbar
        DrawToolbar();

        // Draw panels (only if visible)
        if (m_HierarchyPanel->IsVisible())
            m_HierarchyPanel->OnImGuiRender();
        
        if (m_InspectorPanel->IsVisible())
            m_InspectorPanel->OnImGuiRender();
        
        if (m_ViewportPanel->IsVisible())
            m_ViewportPanel->OnImGuiRender();
        
        if (m_ContentBrowserPanel->IsVisible())
            m_ContentBrowserPanel->OnImGuiRender();
        
        if (m_ConsolePanel->IsVisible())
            m_ConsolePanel->OnImGuiRender();

        // Draw stats panel
        DrawStatsPanel();
    }

    void EditorLayer::OnEvent(Pillar::Event& event)
    {
        // Always pass scroll events to viewport if it's hovered (for zoom)
        m_ViewportPanel->OnEvent(event);

        // Handle keyboard shortcuts
        Pillar::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<Pillar::KeyPressedEvent>(
            [this](Pillar::KeyPressedEvent& e) { return OnKeyPressed(e); }
        );
    }

    bool EditorLayer::OnKeyPressed(Pillar::KeyPressedEvent& e)
    {
        // Shortcuts - only process if not typing in a text field
        if (e.GetRepeatCount() > 0)
            return false;

        // Don't handle shortcuts if ImGui wants keyboard input (text fields)
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantTextInput)
            return false;

        bool control = Pillar::Input::IsKeyPressed(PIL_KEY_LEFT_CONTROL) || 
                       Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT_CONTROL);
        bool shift = Pillar::Input::IsKeyPressed(PIL_KEY_LEFT_SHIFT) || 
                     Pillar::Input::IsKeyPressed(PIL_KEY_RIGHT_SHIFT);

        switch (e.GetKeyCode())
        {
            case PIL_KEY_N:
                if (control)
                {
                    NewScene();
                    CreateDefaultEntities();
                }
                break;
            case PIL_KEY_O:
                if (control)
                    OpenScene();
                break;
            case PIL_KEY_S:
                if (control && shift)
                    SaveSceneAs();
                else if (control)
                    SaveScene();
                break;
            case PIL_KEY_D:
                if (control)
                {
                    // Duplicate selected entity
                    if (m_SelectionContext.HasSelection())
                    {
                        auto selected = m_SelectionContext.GetPrimarySelection();
                        if (selected)
                        {
                            auto duplicated = m_ActiveScene->DuplicateEntity(selected);
                            m_SelectionContext.Select(duplicated);
                            ConsolePanel::Log("Entity duplicated", LogLevel::Info);
                        }
                    }
                }
                break;
            case PIL_KEY_DELETE:
                // Delete selected entity
                if (m_SelectionContext.HasSelection())
                {
                    auto selected = m_SelectionContext.GetPrimarySelection();
                    if (selected)
                    {
                        std::string name = selected.GetComponent<Pillar::TagComponent>().Tag;
                        m_ActiveScene->DestroyEntity(selected);
                        m_SelectionContext.ClearSelection();
                        ConsolePanel::Log("Deleted entity: " + name, LogLevel::Info);
                    }
                }
                break;
            case PIL_KEY_F:
                // Focus on selected entity
                if (m_SelectionContext.HasSelection())
                {
                    auto selected = m_SelectionContext.GetPrimarySelection();
                    if (selected && selected.HasComponent<Pillar::TransformComponent>())
                    {
                        auto& transform = selected.GetComponent<Pillar::TransformComponent>();
                        m_ViewportPanel->GetCamera().FocusOnPosition(transform.Position);
                        ConsolePanel::Log("Focused on: " + selected.GetComponent<Pillar::TagComponent>().Tag, LogLevel::Trace);
                    }
                }
                break;
            case PIL_KEY_H:
                // Reset camera to origin
                m_ViewportPanel->ResetCamera();
                ConsolePanel::Log("Camera reset to origin", LogLevel::Trace);
                break;
            case PIL_KEY_ESCAPE:
                // Deselect all
                m_SelectionContext.ClearSelection();
                break;
        }

        return false;
    }

    void EditorLayer::DrawMenuBar()
    {
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New Scene", "Ctrl+N"))
                {
                    NewScene();
                    CreateDefaultEntities();
                }
                
                if (ImGui::MenuItem("Open Scene...", "Ctrl+O"))
                    OpenScene();

                ImGui::Separator();

                if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
                    SaveScene();
                
                if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
                    SaveSceneAs();

                ImGui::Separator();

                if (ImGui::MenuItem("Exit"))
                {
                    Pillar::WindowCloseEvent closeEvent;
                    Pillar::Application::Get().OnEvent(closeEvent);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z", false, false)) {}  // Placeholder
                if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {}  // Placeholder
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Duplicate", "Ctrl+D", false, m_SelectionContext.HasSelection()))
                {
                    auto selected = m_SelectionContext.GetPrimarySelection();
                    if (selected)
                    {
                        auto duplicated = m_ActiveScene->DuplicateEntity(selected);
                        m_SelectionContext.Select(duplicated);
                    }
                }
                
                if (ImGui::MenuItem("Delete", "Delete", false, m_SelectionContext.HasSelection()))
                {
                    auto selected = m_SelectionContext.GetPrimarySelection();
                    if (selected)
                    {
                        m_ActiveScene->DestroyEntity(selected);
                        m_SelectionContext.ClearSelection();
                    }
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Deselect All", "Escape"))
                {
                    m_SelectionContext.ClearSelection();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Entity"))
            {
                if (ImGui::MenuItem("Create Empty Entity"))
                {
                    auto entity = m_ActiveScene->CreateEntity("New Entity");
                    m_SelectionContext.Select(entity);
                    ConsolePanel::Log("Created new entity", LogLevel::Info);
                }
                
                ImGui::Separator();

                if (ImGui::BeginMenu("Create..."))
                {
                    if (ImGui::MenuItem("Player"))
                    {
                        auto entity = m_ActiveScene->CreateEntity("Player");
                        m_SelectionContext.Select(entity);
                    }
                    if (ImGui::MenuItem("Enemy"))
                    {
                        auto entity = m_ActiveScene->CreateEntity("Enemy");
                        m_SelectionContext.Select(entity);
                    }
                    if (ImGui::MenuItem("Ground"))
                    {
                        auto entity = m_ActiveScene->CreateEntity("Ground");
                        auto& transform = entity.GetComponent<Pillar::TransformComponent>();
                        transform.Scale = { 10.0f, 1.0f };
                        transform.Position.y = -3.0f;
                        m_SelectionContext.Select(entity);
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                bool hierarchyVisible = m_HierarchyPanel->IsVisible();
                if (ImGui::MenuItem("Hierarchy", nullptr, hierarchyVisible))
                {
                    m_HierarchyPanel->SetVisible(!hierarchyVisible);
                }
                
                bool inspectorVisible = m_InspectorPanel->IsVisible();
                if (ImGui::MenuItem("Inspector", nullptr, inspectorVisible))
                {
                    m_InspectorPanel->SetVisible(!inspectorVisible);
                }
                
                bool contentBrowserVisible = m_ContentBrowserPanel->IsVisible();
                if (ImGui::MenuItem("Content Browser", nullptr, contentBrowserVisible))
                {
                    m_ContentBrowserPanel->SetVisible(!contentBrowserVisible);
                }
                
                bool consoleVisible = m_ConsolePanel->IsVisible();
                if (ImGui::MenuItem("Console", nullptr, consoleVisible))
                {
                    m_ConsolePanel->SetVisible(!consoleVisible);
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Reset Camera", "H"))
                {
                    m_ViewportPanel->ResetCamera();
                }
                
                if (ImGui::MenuItem("Reset Layout"))
                {
                    // Force layout reset on next frame
                    ImGui::DockBuilderRemoveNode(ImGui::GetID("EditorDockSpace"));
                    ConsolePanel::Log("Layout will be reset", LogLevel::Info);
                }
                
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("About Pillar Editor"))
                {
                    ConsolePanel::Log("Pillar Engine Editor v0.1", LogLevel::Info);
                    ConsolePanel::Log("A 2D game engine editor", LogLevel::Info);
                }
                
                ImGui::Separator();
                
                ImGui::TextDisabled("Viewport Controls:");
                ImGui::BulletText("Middle Mouse: Pan");
                ImGui::BulletText("Scroll Wheel: Zoom");
                ImGui::BulletText("H: Reset camera");
                ImGui::Separator();
                ImGui::TextDisabled("Entity Controls:");
                ImGui::BulletText("F: Focus on selection");
                ImGui::BulletText("Delete: Delete selection");
                ImGui::BulletText("Ctrl+D: Duplicate");
                
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }
    }

    void EditorLayer::DrawToolbar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(4, 4));

        ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration | 
                                        ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::Begin("##toolbar", nullptr, toolbarFlags);

        float buttonHeight = 28.0f;
        float buttonWidth = 60.0f;

        // Play button
        bool isPlaying = m_EditorState == EditorState::Play;
        if (isPlaying)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.5f, 0.1f, 1.0f));
        
        if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(buttonWidth, buttonHeight)))
        {
            if (m_EditorState == EditorState::Edit)
                OnPlay();
            else
                OnStop();
        }
        
        if (isPlaying)
            ImGui::PopStyleColor();

        ImGui::SameLine();

        // Pause button
        bool isPaused = m_EditorState == EditorState::Pause;
        bool canPause = m_EditorState == EditorState::Play || m_EditorState == EditorState::Pause;
        
        if (!canPause)
            ImGui::BeginDisabled();
        
        if (isPaused)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.1f, 1.0f));
        
        if (ImGui::Button(isPaused ? "Resume" : "Pause", ImVec2(buttonWidth, buttonHeight)))
        {
            if (m_EditorState == EditorState::Play)
                OnPause();
            else if (m_EditorState == EditorState::Pause)
                OnPlay();
        }
        
        if (isPaused)
            ImGui::PopStyleColor();
        
        if (!canPause)
            ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        // Editor state indicator
        const char* stateText = "EDIT";
        ImVec4 stateColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
        
        switch (m_EditorState)
        {
            case EditorState::Edit:
                stateText = "EDIT MODE";
                stateColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
                break;
            case EditorState::Play:
                stateText = "PLAYING";
                stateColor = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
                break;
            case EditorState::Pause:
                stateText = "PAUSED";
                stateColor = ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
                break;
        }

        ImGui::TextColored(stateColor, "%s", stateText);

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    void EditorLayer::DrawStatsPanel()
    {
        ImGui::Begin("Stats");

        // Frame stats
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Performance");
        ImGui::Separator();
        
        float fps = m_LastFrameTime > 0.0f ? 1.0f / m_LastFrameTime : 0.0f;
        ImGui::Text("FPS: %.1f", fps);
        ImGui::Text("Frame Time: %.3f ms", m_LastFrameTime * 1000.0f);
        
        ImGui::Spacing();

        // Scene stats
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Scene");
        ImGui::Separator();
        
        if (m_ActiveScene)
        {
            ImGui::Text("Name: %s", m_ActiveScene->GetName().c_str());
            ImGui::Text("Entities: %zu", m_ActiveScene->GetEntityCount());
            ImGui::Text("State: %s", m_ActiveScene->IsPlaying() ? "Playing" : "Edit");
        }
        else
        {
            ImGui::TextDisabled("No scene loaded");
        }
        
        ImGui::Spacing();

        // Camera stats
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Camera");
        ImGui::Separator();
        
        auto& cam = m_ViewportPanel->GetCamera();
        glm::vec3 pos = cam.GetPosition();
        ImGui::Text("Position: (%.2f, %.2f)", pos.x, pos.y);
        ImGui::Text("Zoom: %.2fx", 1.0f / cam.GetZoomLevel());
        
        ImGui::Spacing();

        // Selection stats
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Selection");
        ImGui::Separator();
        
        size_t selectionCount = m_SelectionContext.GetSelectionCount();
        ImGui::Text("Selected: %zu entities", selectionCount);
        
        if (selectionCount > 0)
        {
            auto primary = m_SelectionContext.GetPrimarySelection();
            if (primary && primary.HasComponent<Pillar::TagComponent>())
            {
                ImGui::Text("Primary: %s", primary.GetComponent<Pillar::TagComponent>().Tag.c_str());
            }
        }

        ImGui::End();
    }

    void EditorLayer::NewScene()
    {
        m_ActiveScene = std::make_shared<Pillar::Scene>("Untitled");
        m_CurrentScenePath.clear();
        
        // Update panel contexts
        m_HierarchyPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_InspectorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_ViewportPanel->SetContext(m_ActiveScene, &m_SelectionContext);

        m_SelectionContext.ClearSelection();
        
        // Reset camera to origin when creating new scene
        m_ViewportPanel->ResetCamera();
        
        ConsolePanel::Log("Created new scene", LogLevel::Info);
    }

    void EditorLayer::OpenScene()
    {
        // Try multiple paths for the demo scene
        std::vector<std::string> paths = {
            "assets/scenes/demo.scene.json",
            "Sandbox/assets/scenes/demo.scene.json",
            "../Sandbox/assets/scenes/demo.scene.json"
        };
        
        for (const auto& path : paths)
        {
            if (std::filesystem::exists(path))
            {
                OpenScene(path);
                return;
            }
        }
        
        ConsolePanel::Log("No demo scene found. Create entities manually.", LogLevel::Warn);
    }

    void EditorLayer::OpenScene(const std::string& filepath)
    {
        if (!std::filesystem::exists(filepath))
        {
            ConsolePanel::Log("File not found: " + filepath, LogLevel::Error);
            return;
        }

        m_ActiveScene = std::make_shared<Pillar::Scene>();
        Pillar::SceneSerializer serializer(m_ActiveScene.get());
        
        if (serializer.Deserialize(filepath))
        {
            m_CurrentScenePath = filepath;
            
            // Update panel contexts
            m_HierarchyPanel->SetContext(m_ActiveScene, &m_SelectionContext);
            m_InspectorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
            m_ViewportPanel->SetContext(m_ActiveScene, &m_SelectionContext);

            m_SelectionContext.ClearSelection();
            
            // Reset camera when loading scene
            m_ViewportPanel->ResetCamera();
            
            ConsolePanel::Log("Opened scene: " + filepath, LogLevel::Info);
        }
        else
        {
            ConsolePanel::Log("Failed to parse scene: " + filepath, LogLevel::Error);
            NewScene();
        }
    }

    void EditorLayer::SaveScene()
    {
        if (!m_CurrentScenePath.empty())
        {
            Pillar::SceneSerializer serializer(m_ActiveScene.get());
            if (serializer.Serialize(m_CurrentScenePath))
            {
                ConsolePanel::Log("Saved scene: " + m_CurrentScenePath, LogLevel::Info);
            }
            else
            {
                ConsolePanel::Log("Failed to save scene", LogLevel::Error);
            }
        }
        else
        {
            SaveSceneAs();
        }
    }

    void EditorLayer::SaveSceneAs()
    {
        // Ensure directory exists
        std::filesystem::create_directories("assets/scenes");
        
        std::string filepath = "assets/scenes/" + m_ActiveScene->GetName() + ".scene.json";
        m_CurrentScenePath = filepath;
        
        Pillar::SceneSerializer serializer(m_ActiveScene.get());
        if (serializer.Serialize(filepath))
        {
            ConsolePanel::Log("Saved scene as: " + filepath, LogLevel::Info);
        }
        else
        {
            ConsolePanel::Log("Failed to save scene", LogLevel::Error);
        }
    }

    void EditorLayer::OnPlay()
    {
        if (m_EditorState == EditorState::Pause)
        {
            m_EditorState = EditorState::Play;
            ConsolePanel::Log("Resumed play mode", LogLevel::Info);
            return;
        }

        m_EditorState = EditorState::Play;
        
        // Backup the editor scene
        m_EditorScene = Pillar::Scene::Copy(m_ActiveScene);
        
        // Start runtime
        m_ActiveScene->OnRuntimeStart();
        
        ConsolePanel::Log("Entered Play mode", LogLevel::Info);
    }

    void EditorLayer::OnPause()
    {
        if (m_EditorState != EditorState::Play)
            return;
            
        m_EditorState = EditorState::Pause;
        ConsolePanel::Log("Paused", LogLevel::Info);
    }

    void EditorLayer::OnStop()
    {
        if (m_EditorState == EditorState::Edit)
            return;

        m_EditorState = EditorState::Edit;
        
        // Stop runtime
        m_ActiveScene->OnRuntimeStop();
        
        // Restore editor scene
        m_ActiveScene = m_EditorScene;
        m_EditorScene = nullptr;
        
        // Update panel contexts
        m_HierarchyPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_InspectorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_ViewportPanel->SetContext(m_ActiveScene, &m_SelectionContext);

        m_SelectionContext.ClearSelection();
        
        ConsolePanel::Log("Stopped - Returned to Edit mode", LogLevel::Info);
    }

}
