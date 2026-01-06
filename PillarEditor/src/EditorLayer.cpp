#include "EditorLayer.h"
#include "EditorSettings.h"
#include "Panels/ConsolePanel.h"
#include "Panels/SpriteSheetEditorPanel.h"
#include "Utils/FileDialog.h"
#include "Pillar/Application.h"
#include "Pillar/ECS/SceneSerializer.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/Renderer/RenderCommand.h"
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>
#include <filesystem>

namespace PillarEditor {

    EditorLayer::EditorLayer()
        : Layer("EditorLayer")
    {
    }

    void EditorLayer::OnAttach()
    {
        PIL_CORE_INFO("EditorLayer attached");

        // Load editor settings
        EditorSettings::Get().Load();

        // Setup custom editor style
        SetupImGuiStyle();

        // Create panels
        m_HierarchyPanel = std::make_unique<SceneHierarchyPanel>();
        m_HierarchyPanel->SetTemplateManager(&m_TemplateManager);
        
        m_InspectorPanel = std::make_unique<InspectorPanel>(this);
        m_ViewportPanel = std::make_unique<ViewportPanel>(this);
        m_ContentBrowserPanel = std::make_unique<ContentBrowserPanel>();
        m_ConsolePanel = std::make_unique<ConsolePanel>();
        m_TemplateLibraryPanel = std::make_unique<TemplateLibraryPanel>();
        m_AnimationManagerPanel = std::make_unique<AnimationManagerPanel>();
        m_SpriteSheetEditorPanel = std::make_unique<SpriteSheetEditorPanel>();
        m_LayerEditorPanel = std::make_unique<LayerEditorPanel>();
        m_AnimationEditorPanel = std::make_unique<AnimationEditorPanel>();

        // Initialize all game systems (order matters - some systems depend on others)
        m_AnimationSystem = std::make_unique<Pillar::AnimationSystem>();
        m_VelocitySystem = std::make_unique<Pillar::VelocityIntegrationSystem>();
        m_PhysicsSystem = std::make_unique<Pillar::PhysicsSystem>(glm::vec2(0.0f, -9.81f)); // Gravity
        m_PhysicsSyncSystem = std::make_unique<Pillar::PhysicsSyncSystem>();
        m_BulletCollisionSystem = std::make_unique<Pillar::BulletCollisionSystem>(m_PhysicsSystem.get()); // Needs PhysicsSystem
        m_XPCollectionSystem = std::make_unique<Pillar::XPCollectionSystem>(2.0f); // Cell size
        m_AudioSystem = std::make_unique<Pillar::AudioSystem>();
        m_ParticleSystem = std::make_unique<Pillar::ParticleSystem>();
        m_ParticleEmitterSystem = std::make_unique<Pillar::ParticleEmitterSystem>();

        // Initialize template panel
        m_TemplateLibraryPanel->SetTemplateManager(&m_TemplateManager);

        // Set up command history callback to track scene modifications
        m_CommandHistory.SetOnCommandExecuted([this]() {
            SetSceneModified(true);
        });

        // Initialize animation library manager
        m_AnimationLibraryManager.Initialize(m_AnimationSystem.get());

        // Initialize animation editor panel
        m_AnimationEditorPanel->Initialize(m_AnimationSystem.get(), &m_AnimationLibraryManager);

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
        
        // Save editor settings
        EditorSettings::Get().Save();
    }

    void EditorLayer::OnUpdate(float deltaTime)
    {
        m_LastFrameTime = deltaTime;

        // Validate selection to remove any invalid/deleted entities
        m_SelectionContext.ValidateSelection();

        // Update animation library manager for file watching (hot-reload)
        m_AnimationLibraryManager.Update();

        // Always update viewport panel - it handles its own hover checks internally
        m_ViewportPanel->OnUpdate(deltaTime);

        // Update animation editor panel preview
        if (m_AnimationEditorPanel)
            m_AnimationEditorPanel->Update(deltaTime);

        // Update scene in play mode
        if (m_EditorState == EditorState::Play)
        {
            // Update all game systems in order
            auto& registry = m_ActiveScene->GetRegistry();
            
            // 1. Input & AI (future)
            
            // 2. Physics & Movement
            if (m_VelocitySystem)
                m_VelocitySystem->OnUpdate(deltaTime);
            
            if (m_PhysicsSystem)
                m_PhysicsSystem->OnUpdate(deltaTime);
            
            if (m_PhysicsSyncSystem)
                m_PhysicsSyncSystem->OnUpdate(deltaTime);
            
            // 3. Collision & Gameplay
            if (m_BulletCollisionSystem)
                m_BulletCollisionSystem->OnUpdate(deltaTime);
            
            if (m_XPCollectionSystem)
                m_XPCollectionSystem->OnUpdate(deltaTime);
            
            // 4. Particles & Effects
            if (m_ParticleEmitterSystem)
                m_ParticleEmitterSystem->OnUpdate(deltaTime);
            
            if (m_ParticleSystem)
                m_ParticleSystem->OnUpdate(deltaTime);
            
            // 5. Animation
            if (m_AnimationSystem)
                m_AnimationSystem->OnUpdate(deltaTime);
            
            // 6. Audio
            if (m_AudioSystem)
                m_AudioSystem->OnUpdate(deltaTime);
            
            // 7. Scene lifecycle
            m_ActiveScene->OnUpdate(deltaTime);
        }

        // Auto-save logic (only in edit mode)
        if (m_EditorState == EditorState::Edit && !m_CurrentScenePath.empty())
        {
            auto& settings = EditorSettings::Get();
            if (settings.AutoSaveEnabled && m_SceneModified)
            {
                m_AutoSaveTimer += deltaTime;
                if (m_AutoSaveTimer >= settings.AutoSaveInterval)
                {
                    PerformAutoSave();
                    m_AutoSaveTimer = 0.0f;
                }
            }
        }

        // Render scene to viewport framebuffer
        m_ViewportPanel->RenderScene();
    }

    void EditorLayer::SetupImGuiStyle()
    {
        ImGuiIO& io = ImGui::GetIO();
        
        // ========================================================================
        // FONT CONFIGURATION - Modern, Crisp Typography
        // ========================================================================
        
        // Try to load custom fonts for better readability
        // If fonts aren't available, ImGui will fall back to its built-in ProggyClean font
        ImFontConfig fontConfig;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 1;
        fontConfig.PixelSnapH = true;
        
        // Attempt to load common system fonts (Windows paths)
        // Try Segoe UI (Windows 10/11 default) - clean, modern, professional
        ImFont* mainFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 16.0f, &fontConfig);
        
        // If Segoe UI isn't found, try Consolas for a monospace alternative
        if (!mainFont)
            mainFont = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/consola.ttf", 15.0f, &fontConfig);
        
        // If no custom fonts loaded, ensure default font is built
        if (io.Fonts->Fonts.empty())
            io.Fonts->AddFontDefault();
        
        io.FontGlobalScale = 1.0f;

        // ========================================================================
        // STYLE CONFIGURATION - Sleek, Modern, Polished
        // ========================================================================
        
        ImGuiStyle& style = ImGui::GetStyle();
        
        // --- SPACING & SIZING ---
        // More generous spacing for better visual breathing room
        style.WindowPadding = ImVec2(10.0f, 10.0f);
        style.FramePadding = ImVec2(8.0f, 5.0f);
        style.CellPadding = ImVec2(6.0f, 3.0f);
        style.ItemSpacing = ImVec2(10.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
        style.IndentSpacing = 22.0f;
        style.ScrollbarSize = 16.0f;
        style.GrabMinSize = 14.0f;
        
        // Align text to padding
        style.WindowTitleAlign = ImVec2(0.5f, 0.5f);  // Center window titles
        style.ButtonTextAlign = ImVec2(0.5f, 0.5f);   // Center button text
        
        // --- BORDERS ---
        // Subtle borders for definition without harshness
        style.WindowBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.FrameBorderSize = 0.0f;  // Frameless inputs for cleaner look
        style.TabBorderSize = 0.0f;
        
        // --- ROUNDING ---
        // Smooth, modern rounded corners throughout
        style.WindowRounding = 6.0f;
        style.ChildRounding = 5.0f;
        style.FrameRounding = 4.0f;
        style.PopupRounding = 5.0f;
        style.ScrollbarRounding = 9.0f;
        style.GrabRounding = 4.0f;
        style.TabRounding = 5.0f;
        
        // --- SHADOWS & ANTI-ALIASING ---
        style.AntiAliasedLines = true;
        style.AntiAliasedLinesUseTex = true;
        style.AntiAliasedFill = true;
        
        // ========================================================================
        // COLOR SCHEME - Sophisticated Dark Theme
        // ========================================================================
        // Inspired by JetBrains IDEs, VS Code, and modern design systems
        // Carefully balanced for extended coding sessions
        
        ImVec4* colors = style.Colors;
        
        // --- BACKGROUNDS ---
        // Deep, rich background with subtle variations for depth
        colors[ImGuiCol_WindowBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);      // Main window background
        colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);       // Child window background
        colors[ImGuiCol_PopupBg] = ImVec4(0.16f, 0.17f, 0.18f, 0.98f);       // Popup background (slightly lighter)
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);     // Menu bar background
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.11f, 0.12f, 0.13f, 1.00f); // Empty docking area
        
        // --- TITLE BARS ---
        // Darker title bars for visual hierarchy
        colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.11f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.11f, 0.12f, 0.75f);
        
        // --- BORDERS & SEPARATORS ---
        // Subtle but visible separation
        colors[ImGuiCol_Border] = ImVec4(0.28f, 0.29f, 0.31f, 0.60f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.29f, 0.31f, 0.60f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.62f, 0.85f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.62f, 0.85f, 1.00f);
        
        // --- INPUT FIELDS & FRAMES ---
        // Slightly elevated from background
        colors[ImGuiCol_FrameBg] = ImVec4(0.19f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.28f, 0.30f, 1.00f);
        
        // --- TEXT ---
        // High contrast for readability with pleasant warmth
        colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.91f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.51f, 0.52f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.40f, 0.62f, 0.85f, 0.35f);
        
        // --- BUTTONS ---
        // Professional, modern button styling with smooth interactions
        colors[ImGuiCol_Button] = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.32f, 0.34f, 0.37f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.62f, 0.85f, 1.00f);
        
        // --- HEADERS & COLLAPSIBLES ---
        // Tree nodes, collapsing headers, etc.
        colors[ImGuiCol_Header] = ImVec4(0.24f, 0.26f, 0.28f, 0.80f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.32f, 0.34f, 0.37f, 0.90f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.62f, 0.85f, 1.00f);
        
        // --- TABS ---
        // Clean, modern tab bar
        colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.40f, 0.62f, 0.85f, 0.90f);
        colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.19f, 0.20f, 1.00f);
        
        // --- DOCKING ---
        // Visual feedback for docking operations
        colors[ImGuiCol_DockingPreview] = ImVec4(0.40f, 0.62f, 0.85f, 0.70f);
        
        // --- SCROLLBARS ---
        // Smooth, unobtrusive scrollbars
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.11f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.32f, 0.34f, 0.36f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.42f, 0.44f, 0.46f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.52f, 0.54f, 0.56f, 1.00f);
        
        // --- SLIDERS & CHECKBOXES ---
        // Accent color for interactive elements
        colors[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.70f, 0.95f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.40f, 0.62f, 0.85f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.72f, 0.95f, 1.00f);
        
        // --- RESIZE GRIPS ---
        // Subtle resize handles
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.29f, 0.31f, 0.40f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.40f, 0.62f, 0.85f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.62f, 0.85f, 0.95f);
        
        // --- TABLE COLORS ---
        // For future table widgets
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.28f, 0.29f, 0.31f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.24f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.04f);
        
        // --- DRAG & DROP ---
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.45f, 0.70f, 0.95f, 0.90f);
        
        // --- NAVIGATION ---
        // Keyboard/gamepad navigation highlight
        colors[ImGuiCol_NavHighlight] = ImVec4(0.45f, 0.70f, 0.95f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        
        // --- MODALS ---
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
        
        PIL_CORE_INFO("✨ Modern sleek theme applied successfully!");
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
        // ImGuizmo needs this to be called every frame
        ImGuizmo::BeginFrame();

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

        if (m_TemplateLibraryPanel)
            m_TemplateLibraryPanel->OnImGuiRender();

        if (m_AnimationManagerPanel->IsVisible())
            m_AnimationManagerPanel->OnImGuiRender();

        if (m_SpriteSheetEditorPanel->IsVisible())
            m_SpriteSheetEditorPanel->OnImGuiRender();

        if (m_AnimationEditorPanel)
            m_AnimationEditorPanel->OnImGuiRender();

        if (m_LayerEditorPanel)
            m_LayerEditorPanel->OnImGuiRender();

        // Draw stats panel
        DrawStatsPanel();

        // Draw preferences window if visible
        if (m_ShowPreferences)
            DrawPreferencesWindow();

        // Draw status bar (always visible)
        DrawStatusBar();
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
            case PIL_KEY_Z:
                if (control)
                {
                    // Undo
                    if (m_CommandHistory.CanUndo())
                    {
                        std::string actionName = m_CommandHistory.GetUndoName();
                        m_CommandHistory.Undo();
                        ConsolePanel::Log("Undo: " + actionName, LogLevel::Info);
                    }
                    else
                    {
                        ConsolePanel::Log("Nothing to undo", LogLevel::Trace);
                    }
                    return true;
                }
                break;
            case PIL_KEY_Y:
                if (control)
                {
                    // Redo
                    if (m_CommandHistory.CanRedo())
                    {
                        std::string actionName = m_CommandHistory.GetRedoName();
                        m_CommandHistory.Redo();
                        ConsolePanel::Log("Redo: " + actionName, LogLevel::Info);
                    }
                    else
                    {
                        ConsolePanel::Log("Nothing to redo", LogLevel::Trace);
                    }
                    return true;
                }
                break;
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
            case PIL_KEY_A:
                if (control)
                {
                    // Select all entities
                    m_SelectionContext.ClearSelection();
                    auto view = m_ActiveScene->GetRegistry().view<Pillar::TagComponent>();
                    for (auto entity : view)
                    {
                        Pillar::Entity e(entity, m_ActiveScene.get());
                        m_SelectionContext.AddToSelection(e);
                    }
                    size_t count = m_SelectionContext.GetSelectionCount();
                    ConsolePanel::Log("Selected " + std::to_string(count) + " entities", LogLevel::Info);
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
            case PIL_KEY_ESCAPE:
                // Clear selection
                if (m_SelectionContext.HasSelection())
                {
                    m_SelectionContext.ClearSelection();
                    ConsolePanel::Log("Selection cleared", LogLevel::Trace);
                }
                break;
            case PIL_KEY_W:
                if (!control) // Only if Ctrl is not pressed (Ctrl+W might be close)
                {
                    // Translate gizmo mode
                    m_ViewportPanel->SetGizmoMode(PillarEditor::GizmoMode::Translate);
                    ConsolePanel::Log("Gizmo mode: Translate", LogLevel::Trace);
                }
                break;
            case PIL_KEY_E:
                if (!control)
                {
                    // Rotate gizmo mode
                    m_ViewportPanel->SetGizmoMode(PillarEditor::GizmoMode::Rotate);
                    ConsolePanel::Log("Gizmo mode: Rotate", LogLevel::Trace);
                }
                break;
            case PIL_KEY_R:
                if (!control)
                {
                    // Scale gizmo mode
                    m_ViewportPanel->SetGizmoMode(PillarEditor::GizmoMode::Scale);
                    ConsolePanel::Log("Gizmo mode: Scale", LogLevel::Trace);
                }
                break;
            case PIL_KEY_Q:
                // No gizmo mode
                m_ViewportPanel->SetGizmoMode(PillarEditor::GizmoMode::None);
                ConsolePanel::Log("Gizmo mode: None", LogLevel::Trace);
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
                // Undo/Redo with action names
                bool canUndo = m_CommandHistory.CanUndo();
                bool canRedo = m_CommandHistory.CanRedo();
                
                std::string undoLabel = canUndo ? "Undo " + m_CommandHistory.GetUndoName() : "Undo";
                std::string redoLabel = canRedo ? "Redo " + m_CommandHistory.GetRedoName() : "Redo";
                
                if (ImGui::MenuItem(undoLabel.c_str(), "Ctrl+Z", false, canUndo))
                {
                    m_CommandHistory.Undo();
                }
                
                if (ImGui::MenuItem(redoLabel.c_str(), "Ctrl+Y", false, canRedo))
                {
                    m_CommandHistory.Redo();
                }
                
                ImGui::Separator();
                
                if (ImGui::MenuItem("Select All", "Ctrl+A"))
                {
                    m_SelectionContext.ClearSelection();
                    auto view = m_ActiveScene->GetRegistry().view<Pillar::TagComponent>();
                    for (auto entity : view)
                    {
                        Pillar::Entity e(entity, m_ActiveScene.get());
                        m_SelectionContext.AddToSelection(e);
                    }
                    ConsolePanel::Log("Selected all entities", LogLevel::Info);
                }
                
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

                ImGui::Separator();
                
                if (ImGui::MenuItem("Preferences..."))
                {
                    m_ShowPreferences = true;
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
                
                bool spriteSheetEditorVisible = m_SpriteSheetEditorPanel->IsVisible();
                if (ImGui::MenuItem("Sprite Sheet Editor", nullptr, spriteSheetEditorVisible))
                {
                    m_SpriteSheetEditorPanel->SetVisible(!spriteSheetEditorVisible);
                }

                bool animationEditorVisible = m_AnimationEditorPanel && m_AnimationEditorPanel->IsVisible();
                if (ImGui::MenuItem("Animation Editor", nullptr, animationEditorVisible))
                {
                    m_AnimationEditorPanel->SetVisible(!animationEditorVisible);
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
                
                ImGui::TextDisabled("File Operations:");
                ImGui::BulletText("Ctrl+N: New Scene");
                ImGui::BulletText("Ctrl+O: Open Scene");
                ImGui::BulletText("Ctrl+S: Save Scene");
                ImGui::BulletText("Ctrl+Shift+S: Save Scene As");
                ImGui::Separator();
                ImGui::TextDisabled("Viewport Controls:");
                ImGui::BulletText("Left Click: Select entity");
                ImGui::BulletText("Ctrl+Click: Add to selection");
                ImGui::BulletText("Middle Mouse: Pan");
                ImGui::BulletText("Scroll Wheel: Zoom");
                ImGui::BulletText("H: Reset camera");
                ImGui::Separator();
                ImGui::TextDisabled("Gizmo Controls:");
                ImGui::BulletText("Q: No gizmo");
                ImGui::BulletText("W: Translate mode");
                ImGui::BulletText("E: Rotate mode");
                ImGui::BulletText("R: Scale mode");
                ImGui::BulletText("Hold Ctrl: Snap to grid");
                ImGui::Separator();
                ImGui::TextDisabled("Entity Controls:");
                ImGui::BulletText("F: Focus on selection");
                ImGui::BulletText("Escape: Clear selection");
                ImGui::BulletText("Delete: Delete selection");
                ImGui::BulletText("Ctrl+A: Select all");
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
        
        if (ImGui::IsItemHovered())
        {
            if (isPlaying)
                ImGui::SetTooltip("Stop playing and return to edit mode");
            else
                ImGui::SetTooltip("Start playing the scene");
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
            // Editable scene name
            ImGui::Text("Name:");
            ImGui::SameLine();
            
            static char sceneNameBuffer[256];
            std::string currentName = m_ActiveScene->GetName();
            
            // Initialize buffer with current name if different
            static std::string lastSceneName = currentName;
            if (lastSceneName != currentName)
            {
                strncpy_s(sceneNameBuffer, currentName.c_str(), sizeof(sceneNameBuffer) - 1);
                lastSceneName = currentName;
            }
            
            // First time initialization
            static bool initialized = false;
            if (!initialized)
            {
                strncpy_s(sceneNameBuffer, currentName.c_str(), sizeof(sceneNameBuffer) - 1);
                initialized = true;
            }
            
            ImGui::PushItemWidth(-1);
            if (ImGui::InputText("##SceneName", sceneNameBuffer, sizeof(sceneNameBuffer)))
            {
                if (strlen(sceneNameBuffer) > 0)
                {
                    m_ActiveScene->SetName(sceneNameBuffer);
                    lastSceneName = sceneNameBuffer;
                }
            }
            ImGui::PopItemWidth();
            
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Edit scene name");
            
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

    void EditorLayer::DrawPreferencesWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Preferences", &m_ShowPreferences))
        {
            // Auto-Save Settings
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Auto-Save Settings");
            ImGui::Separator();
            
            auto& settings = EditorSettings::Get();
            
            ImGui::Checkbox("Enable Auto-Save", &settings.AutoSaveEnabled);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Automatically save backup copies of your scene");
            
            if (settings.AutoSaveEnabled)
            {
                ImGui::Spacing();
                ImGui::Text("Auto-Save Interval:");
                
                // Convert seconds to minutes for display
                float intervalMinutes = settings.AutoSaveInterval / 60.0f;
                
                if (ImGui::SliderFloat("##AutoSaveInterval", &intervalMinutes, 1.0f, 30.0f, "%.1f min"))
                {
                    // Clamp and convert back to seconds
                    intervalMinutes = std::max(1.0f, std::min(30.0f, intervalMinutes));
                    settings.AutoSaveInterval = intervalMinutes * 60.0f;
                }
                
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("How often to auto-save (1-30 minutes)");
                
                // Show time until next auto-save
                if (!m_CurrentScenePath.empty() && m_SceneModified)
                {
                    float timeRemaining = settings.AutoSaveInterval - m_AutoSaveTimer;
                    if (timeRemaining > 0)
                    {
                        int minutes = (int)(timeRemaining / 60.0f);
                        int seconds = (int)timeRemaining % 60;
                        ImGui::Text("Next auto-save in: %dm %ds", minutes, seconds);
                    }
                }
                else if (!m_CurrentScenePath.empty() && !m_SceneModified)
                {
                    ImGui::TextDisabled("No unsaved changes");
                }
                else
                {
                    ImGui::TextDisabled("No scene loaded");
                }
            }
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // Viewport Settings
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Viewport Settings");
            ImGui::Separator();
            
            ImGui::Checkbox("Show Grid", &settings.ShowGrid);
            ImGui::SliderFloat("Grid Size", &settings.GridSize, 0.1f, 10.0f, "%.1f");
            ImGui::SliderFloat("Camera Speed", &settings.CameraSpeed, 1.0f, 20.0f, "%.1f");
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // Editor Preferences
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Editor Preferences");
            ImGui::Separator();
            
            ImGui::Checkbox("Show FPS", &settings.ShowFPS);
            ImGui::Checkbox("Show Entity Count", &settings.ShowEntityCount);
            ImGui::Checkbox("Confirm on Delete", &settings.ConfirmOnDelete);
            ImGui::Checkbox("Auto Focus on Select", &settings.AutoFocusOnSelect);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // Recent Files
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Recent Files");
            ImGui::Separator();
            
            if (ImGui::Button("Clear Recent Files"))
            {
                settings.ClearRecentFiles();
            }
            
            ImGui::Text("%zu recent file(s)", settings.RecentFiles.size());
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            // Action buttons
            ImGui::Separator();
            if (ImGui::Button("Save Settings"))
            {
                settings.Save();
                ConsolePanel::Log("Preferences saved", LogLevel::Info);
            }
            
            ImGui::SameLine();
            
            if (ImGui::Button("Close"))
            {
                m_ShowPreferences = false;
            }
        }
        
        ImGui::End();
    }

    void EditorLayer::DrawStatusBar()
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        
        // Position at bottom of main viewport
        float statusBarHeight = 26.0f;
        ImVec2 statusBarPos = ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - statusBarHeight);
        ImVec2 statusBarSize = ImVec2(viewport->Size.x, statusBarHeight);
        
        ImGui::SetNextWindowPos(statusBarPos);
        ImGui::SetNextWindowSize(statusBarSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        
        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                                 ImGuiWindowFlags_NoMove | 
                                 ImGuiWindowFlags_NoDocking |
                                 ImGuiWindowFlags_NoSavedSettings |
                                 ImGuiWindowFlags_NoFocusOnAppearing |
                                 ImGuiWindowFlags_NoNav |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        
        if (ImGui::Begin("##StatusBar", nullptr, flags))
        {
            // Left side: FPS
            float fps = m_LastFrameTime > 0.0f ? 1.0f / m_LastFrameTime : 0.0f;
            ImGui::Text("FPS: %.0f", fps);
            
            // Vertical separator
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            
            // Entity count
            ImGui::SameLine();
            if (m_ActiveScene)
            {
                size_t entityCount = m_ActiveScene->GetEntityCount();
                ImGui::Text("Entities: %zu", entityCount);
            }
            else
            {
                ImGui::TextDisabled("No scene");
            }
            
            // Selected count
            ImGui::SameLine();
            size_t selectionCount = m_SelectionContext.GetSelectionCount();
            if (selectionCount > 0)
            {
                ImGui::Text("| Selected: %zu", selectionCount);
            }
            
            // Vertical separator
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            
            // Current gizmo tool
            ImGui::SameLine();
            std::string toolName = "None";
            if (m_ViewportPanel)
            {
                switch (m_ViewportPanel->GetGizmoMode())
                {
                    case GizmoMode::Translate: toolName = "Translate (W)"; break;
                    case GizmoMode::Rotate: toolName = "Rotate (E)"; break;
                    case GizmoMode::Scale: toolName = "Scale (R)"; break;
                    default: toolName = "Select (Q)"; break;
                }
            }
            ImGui::Text("Tool: %s", toolName.c_str());
            
            // Vertical separator
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            
            // Camera zoom
            ImGui::SameLine();
            if (m_ViewportPanel)
            {
                float zoom = m_ViewportPanel->GetCamera().GetZoomLevel();
                ImGui::Text("Zoom: %.0f%%", zoom * 100.0f);
            }
            
            // Right side: Play mode indicator
            ImGui::SameLine();
            float rightOffset = 120.0f;
            float availableWidth = ImGui::GetContentRegionAvail().x;
            if (availableWidth > rightOffset)
            {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - rightOffset);
            }
            
            if (m_EditorState == EditorState::Play)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.9f, 0.2f, 1.0f));
                ImGui::Text("Playing");
                ImGui::PopStyleColor();
            }
            else if (m_EditorState == EditorState::Pause)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                ImGui::Text("Paused");
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("Stopped");
                ImGui::PopStyleColor();
            }
        }
        ImGui::End();
        
        ImGui::PopStyleVar(3);
    }

    void EditorLayer::NewScene()
    {
        m_ActiveScene = std::make_shared<Pillar::Scene>("Untitled");
        m_CurrentScenePath.clear();
        
        // Update panel contexts
        m_HierarchyPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_InspectorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_ViewportPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_SpriteSheetEditorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_TemplateLibraryPanel->SetScene(m_ActiveScene);
        m_LayerEditorPanel->SetScene(m_ActiveScene);
        
        // Set animation system
        m_AnimationSystem->OnAttach(m_ActiveScene.get());
        m_ActiveScene->SetAnimationSystem(m_AnimationSystem.get());
        m_AnimationManagerPanel->SetAnimationSystem(m_AnimationSystem.get());

        m_SelectionContext.ClearSelection();
        
        // Reset camera to origin when creating new scene
        m_ViewportPanel->ResetCamera();
        
        // Reset auto-save state
        SetSceneModified(false);
        m_AutoSaveTimer = 0.0f;
        
        ConsolePanel::Log("Created new scene", LogLevel::Info);
    }

    void EditorLayer::OpenScene()
    {
        // Open file dialog
        auto filepath = FileDialog::OpenFile("Pillar Scene (*.scene.json)\0*.scene.json\0All Files (*.*)\0*.*\0");
        
        if (filepath.has_value())
        {
            OpenScene(filepath.value());
        }
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
            m_SpriteSheetEditorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
            m_TemplateLibraryPanel->SetScene(m_ActiveScene);
            m_LayerEditorPanel->SetScene(m_ActiveScene);
            
            // Set animation system
            m_AnimationSystem->OnAttach(m_ActiveScene.get());
            m_ActiveScene->SetAnimationSystem(m_AnimationSystem.get());
            m_AnimationManagerPanel->SetAnimationSystem(m_AnimationSystem.get());

            m_SelectionContext.ClearSelection();
            
            // Reset camera when loading scene
            m_ViewportPanel->ResetCamera();
            
            // Add to recent files
            EditorSettings::Get().AddRecentFile(filepath);
            
            // Reset auto-save state
            SetSceneModified(false);
            m_AutoSaveTimer = 0.0f;
            
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
                // Reset auto-save state after successful save
                SetSceneModified(false);
                m_AutoSaveTimer = 0.0f;
                
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
        // Open save file dialog
        auto filepath = FileDialog::SaveFile("Pillar Scene (*.scene.json)\0*.scene.json\0All Files (*.*)\0*.*\0");
        
        if (filepath.has_value())
        {
            std::string path = filepath.value();
            
            // Ensure .scene.json extension
            if (path.find(".scene.json") == std::string::npos)
            {
                path += ".scene.json";
            }
            
            m_CurrentScenePath = path;
            
            Pillar::SceneSerializer serializer(m_ActiveScene.get());
            if (serializer.Serialize(path))
            {
                // Add to recent files
                EditorSettings::Get().AddRecentFile(path);
                
                // Reset auto-save state after successful save
                SetSceneModified(false);
                m_AutoSaveTimer = 0.0f;
                
                ConsolePanel::Log("Saved scene as: " + path, LogLevel::Info);
            }
            else
            {
                ConsolePanel::Log("Failed to save scene", LogLevel::Error);
            }
        }
    }

    void EditorLayer::PerformAutoSave()
    {
        if (m_CurrentScenePath.empty())
            return;

        // Create backup filename: scene.scene.json -> scene.autosave.scene.json
        std::string backupPath = m_CurrentScenePath;
        size_t dotPos = backupPath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            backupPath.insert(dotPos, ".autosave");
        }
        else
        {
            backupPath += ".autosave";
        }

        // Save to backup file
        Pillar::SceneSerializer serializer(m_ActiveScene.get());
        if (serializer.Serialize(backupPath))
        {
            ConsolePanel::Log("Auto-saved scene to: " + backupPath, LogLevel::Info);
            m_SceneModified = false;  // Reset modified flag after successful auto-save
        }
        else
        {
            ConsolePanel::Log("Failed to auto-save scene", LogLevel::Warn);
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
        
        // Attach all systems to the active scene
        if (m_AnimationSystem)
            m_AnimationSystem->OnAttach(m_ActiveScene.get());
        if (m_VelocitySystem)
            m_VelocitySystem->OnAttach(m_ActiveScene.get());
        if (m_PhysicsSystem)
            m_PhysicsSystem->OnAttach(m_ActiveScene.get());
        if (m_PhysicsSyncSystem)
            m_PhysicsSyncSystem->OnAttach(m_ActiveScene.get());
        if (m_AudioSystem)
            m_AudioSystem->OnAttach(m_ActiveScene.get());
        if (m_ParticleSystem)
            m_ParticleSystem->OnAttach(m_ActiveScene.get());
        if (m_ParticleEmitterSystem)
            m_ParticleEmitterSystem->OnAttach(m_ActiveScene.get());
        if (m_BulletCollisionSystem)
            m_BulletCollisionSystem->OnAttach(m_ActiveScene.get());
        if (m_XPCollectionSystem)
            m_XPCollectionSystem->OnAttach(m_ActiveScene.get());
        
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
        
        // Detach all systems
        if (m_AnimationSystem)
            m_AnimationSystem->OnDetach();
        if (m_VelocitySystem)
            m_VelocitySystem->OnDetach();
        if (m_PhysicsSystem)
            m_PhysicsSystem->OnDetach();
        if (m_PhysicsSyncSystem)
            m_PhysicsSyncSystem->OnDetach();
        if (m_AudioSystem)
            m_AudioSystem->OnDetach();
        if (m_ParticleSystem)
            m_ParticleSystem->OnDetach();
        if (m_ParticleEmitterSystem)
            m_ParticleEmitterSystem->OnDetach();
        if (m_BulletCollisionSystem)
            m_BulletCollisionSystem->OnDetach();
        if (m_XPCollectionSystem)
            m_XPCollectionSystem->OnDetach();
        
        // Stop runtime
        m_ActiveScene->OnRuntimeStop();
        
        // Restore editor scene
        m_ActiveScene = m_EditorScene;
        m_EditorScene = nullptr;
        
        // Update panel contexts
        m_HierarchyPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_InspectorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_ViewportPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_SpriteSheetEditorPanel->SetContext(m_ActiveScene, &m_SelectionContext);
        m_LayerEditorPanel->SetScene(m_ActiveScene);

        m_SelectionContext.ClearSelection();
        
        ConsolePanel::Log("Stopped - Returned to Edit mode", LogLevel::Info);
    }

}
