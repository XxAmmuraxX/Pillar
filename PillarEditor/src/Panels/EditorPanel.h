#pragma once

#include "Pillar/ECS/Scene.h"
#include "Pillar/Events/Event.h"
#include <string>
#include <memory>

namespace PillarEditor {

    class SelectionContext;

    class EditorPanel
    {
    public:
        EditorPanel(const std::string& name = "Panel")
            : m_Name(name) {}
        virtual ~EditorPanel() = default;

        virtual void OnImGuiRender() = 0;
        virtual void OnEvent(Pillar::Event& e) {}
        virtual void OnUpdate(float deltaTime) {}

        void SetContext(const std::shared_ptr<Pillar::Scene>& scene, SelectionContext* selection)
        {
            m_Scene = scene;
            m_SelectionContext = selection;
        }

        const std::string& GetName() const { return m_Name; }
        bool IsVisible() const { return m_Visible; }
        void SetVisible(bool visible) { m_Visible = visible; }

    protected:
        std::string m_Name;
        bool m_Visible = true;
        std::shared_ptr<Pillar::Scene> m_Scene;
        SelectionContext* m_SelectionContext = nullptr;
    };

}
