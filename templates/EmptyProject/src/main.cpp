#include "Pillar.h"
#include "Pillar/EntryPoint.h"

#include "GameLayer.h"

class GameApp : public Pillar::Application
{
public:
    GameApp()
    {
        PushLayer(new GameLayer());
    }

    ~GameApp() override = default;
};

Pillar::Application* Pillar::CreateApplication()
{
    return new GameApp();
}
