#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "TopDownShooter/SwarmSlayerLayer.h"

// Demo Layers (commented out)
// #include "TopDownShooter/GameLayer.h"
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
		// SWARM SLAYER - Top-Down Shooter Game
		PushLayer(new Game::SwarmSlayerLayer());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



