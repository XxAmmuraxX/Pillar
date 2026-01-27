#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "TopDownShooter/GameLayer.h"

// Demo Layers (commented out)
// #include "PhysicsDemoLayer.h"
// #include "LightEntityPerfDemo.h"
// #include "HeavyEntityPerfDemo.h"
// #include "ExampleLayer.h"
// #include "RenderingDemoLayer.h"
// #include "ParticleSystemDemo.h"
// #include "ObjectPoolDemo.h"
// #include "AudioDemoLayer.h"
// #include "SceneDemoLayer.h"
// #include "ParticleEmitterDemo.h"
// #include "AnimationDemoLayer.h"
// #include "Lighting2DDemoLayer.h"
// #include "DemoMenuLayer.h"
// #include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		// Top-Down Shooter Game
		PushLayer(new Game::GameLayer());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



