#pragma once

#include "Pillar/Layer.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/Events/KeyEvent.h"
#include "SelectionContext.h"
#include "Commands/CommandHistory.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"
#include "Panels/TemplateLibraryPanel.h"
#include "Panels/AnimationManagerPanel.h"
#include "TemplateManager.h"
#include <memory>
#include <string>

namespace PillarEditor {

    enum class EditorState
    {
        Edit = 0,
        Play,
        Pause
    };

    class EditorLayer : public Pillar::Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        virtual void OnAttach() override;
        virtual void OnDetach() override;
        virtual void OnUpdate(float deltaTime) override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Pillar::Event& event) override;

    private:
        void NewScene();
        void OpenScene();
        void OpenScene(const std::string& filepath);
        void SaveScene();
        void SaveSceneAs();

        void OnPlay();
        void OnPause();
        void OnStop();

        void DrawMenuBar();
        void DrawToolbar();
        void DrawStatsPanel();

        void SetupImGuiStyle();
        void SetupDockspace();
        void CreateDefaultEntities();

        bool OnKeyPressed(Pillar::KeyPressedEvent& e);

    public:
        // Command system access for panels
        CommandHistory& GetCommandHistory() { return m_CommandHistory; }
        std::shared_ptr<Pillar::Scene> GetActiveScene() { return m_ActiveScene; }
        EditorState GetEditorState() const { return m_EditorState; }
        TemplateManager& GetTemplateManager() { return m_TemplateManager; }

    private:
        // Scene management
        std::shared_ptr<Pillar::Scene> m_ActiveScene;
        std::shared_ptr<Pillar::Scene> m_EditorScene;    // Backup for edit mode
        std::string m_CurrentScenePath;

        // Editor state
        EditorState m_EditorState = EditorState::Edit;
        SelectionContext m_SelectionContext;

        // Command history for undo/redo
        CommandHistory m_CommandHistory;

        // Panels
        std::unique_ptr<SceneHierarchyPanel> m_HierarchyPanel;
        std::unique_ptr<InspectorPanel> m_InspectorPanel;
        std::unique_ptr<ViewportPanel> m_ViewportPanel;
        std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
        std::unique_ptr<ConsolePanel> m_ConsolePanel;
        std::unique_ptr<TemplateLibraryPanel> m_TemplateLibraryPanel;
        std::unique_ptr<AnimationManagerPanel> m_AnimationManagerPanel;

        // Template system
        TemplateManager m_TemplateManager;

        // Animation system
        std::unique_ptr<Pillar::AnimationSystem> m_AnimationSystem;

        // Stats
        float m_LastFrameTime = 0.0f;
    };

}
