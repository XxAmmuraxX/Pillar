#pragma once

#include "Pillar/Layer.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/Events/KeyEvent.h"
#include "SelectionContext.h"
#include "Panels/SceneHierarchyPanel.h"
#include "Panels/InspectorPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/ConsolePanel.h"
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

    private:
        // Scene management
        std::shared_ptr<Pillar::Scene> m_ActiveScene;
        std::shared_ptr<Pillar::Scene> m_EditorScene;    // Backup for edit mode
        std::string m_CurrentScenePath;

        // Editor state
        EditorState m_EditorState = EditorState::Edit;
        SelectionContext m_SelectionContext;

        // Panels
        std::unique_ptr<SceneHierarchyPanel> m_HierarchyPanel;
        std::unique_ptr<InspectorPanel> m_InspectorPanel;
        std::unique_ptr<ViewportPanel> m_ViewportPanel;
        std::unique_ptr<ContentBrowserPanel> m_ContentBrowserPanel;
        std::unique_ptr<ConsolePanel> m_ConsolePanel;

        // Stats
        float m_LastFrameTime = 0.0f;
    };

}
