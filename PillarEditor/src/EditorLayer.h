#pragma once

#include "Pillar/Layer.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/SceneManager.h"
#include "Pillar/ECS/Systems/AnimationSystem.h"
#include "Pillar/ECS/Systems/VelocityIntegrationSystem.h"
#include "Pillar/ECS/Systems/PhysicsSystem.h"
#include "Pillar/ECS/Systems/PhysicsSyncSystem.h"
#include "Pillar/ECS/Systems/AudioSystem.h"
#include "Pillar/ECS/Systems/ParticleSystem.h"
#include "Pillar/ECS/Systems/ParticleEmitterSystem.h"
#include "Pillar/ECS/Systems/BulletCollisionSystem.h"
#include "Pillar/ECS/Systems/XPCollectionSystem.h"
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
#include "Panels/SpriteSheetEditorPanel.h"
#include "Panels/LayerEditorPanel.h"
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
        void DrawPreferencesWindow();
        void DrawStatusBar();

        // Setup modern, sleek ImGui theme with enhanced colors, spacing, and typography
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
        std::unique_ptr<SpriteSheetEditorPanel> m_SpriteSheetEditorPanel;
        std::unique_ptr<LayerEditorPanel> m_LayerEditorPanel;

        // Template system
        TemplateManager m_TemplateManager;

        // Game systems (updated during play mode)
        std::unique_ptr<Pillar::AnimationSystem> m_AnimationSystem;
        std::unique_ptr<Pillar::VelocityIntegrationSystem> m_VelocitySystem;
        std::unique_ptr<Pillar::PhysicsSystem> m_PhysicsSystem;
        std::unique_ptr<Pillar::PhysicsSyncSystem> m_PhysicsSyncSystem;
        std::unique_ptr<Pillar::AudioSystem> m_AudioSystem;
        std::unique_ptr<Pillar::ParticleSystem> m_ParticleSystem;
        std::unique_ptr<Pillar::ParticleEmitterSystem> m_ParticleEmitterSystem;
        std::unique_ptr<Pillar::BulletCollisionSystem> m_BulletCollisionSystem;
        std::unique_ptr<Pillar::XPCollectionSystem> m_XPCollectionSystem;

        // Stats
        float m_LastFrameTime = 0.0f;

        // UI state
        bool m_ShowPreferences = false;

        // Auto-save state
        float m_AutoSaveTimer = 0.0f;
        bool m_SceneModified = false;
        void SetSceneModified(bool modified = true) { m_SceneModified = modified; }
        void PerformAutoSave();
    };

}
