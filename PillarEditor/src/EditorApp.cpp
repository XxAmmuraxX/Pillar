#include "Pillar.h"
#include "Pillar/Renderer/Lighting2D.h"
#include "Pillar/EntryPoint.h"
#include "EditorLayer.h"

class PillarEditorApp : public Pillar::Application
{
public:
    PillarEditorApp()
    {
        // Disable ImGui event blocking so viewport can receive scroll events
        // The EditorLayer handles event routing properly based on viewport hover state
        GetImGuiLayer()->SetBlockEvents(false);

        Pillar::Lighting2D::Init();
        
        PushLayer(new PillarEditor::EditorLayer());
    }

    ~PillarEditorApp()
    {
        Pillar::Lighting2D::Shutdown();
    }
};

Pillar::Application* Pillar::CreateApplication()
{
    return new PillarEditorApp();
}
