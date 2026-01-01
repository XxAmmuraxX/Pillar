#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "PhysicsDemoLayer.h"
#include "LightEntityPerfDemo.h"
#include "HeavyEntityPerfDemo.h"
#include "ParticleSystemDemo.h"
#include "ObjectPoolDemo.h"
#include "AudioDemoLayer.h"
#include "SceneDemoLayer.h"
#include "ParticleEmitterDemo.h"
#include "AnimationDemoLayer.h"
#include "DemoMenuLayer.h"
#include "ArenaProtocol/ArenaProtocolLayer.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		// CHOOSE YOUR DEMO: Uncomment one of the following lines
		// Option 0: Arena Protocol (Feature Showcase Game)
		PushLayer(new Pillar::ArenaProtocolLayer());
		
		// Option 1: Demo Menu
		//PushLayer(new DemoMenuLayer());
		
		// Option 2: Physics Demo (Gameplay)
		//PushLayer(new PhysicsDemoLayer());
		
		// Option 3: Light Entity Performance Demo
		//PushLayer(new LightEntityPerfDemo());

		// Option 4: Particle System Demo
		//PushLayer(new ParticleSystemDemo());

		// Option 5: Particle Emitter Demo
		//PushLayer(new ParticleEmitterDemo());

		// Option 6: Heavy Entity Performance Demo
		//PushLayer(new HeavyEntityPerfDemo());

		// Option 7: Object Pool Demo
		//PushLayer(new ObjectPoolDemo());

		// Option 8: Audio System Demo
		//PushLayer(new AudioDemoLayer());
		
		// Option 9: Scene System Demo
		//PushLayer(new SceneDemoLayer());
		
		// Option 7: Animation System Demo (NEW!)
		PushLayer(new AnimationDemoLayer());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



