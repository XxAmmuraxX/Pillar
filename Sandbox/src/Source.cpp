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
#include "Lighting2DDemoLayer.h"
#include "DemoMenuLayer.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		// CHOOSE YOUR DEMO: Uncomment one of the following lines
		// Option 0: Demo Menu
		//PushLayer(new DemoMenuLayer());
		
		// Option 1: Physics Demo (Gameplay)
		//PushLayer(new PhysicsDemoLayer());
		
		// Option 2: Light Entity Performance Demo
		//PushLayer(new LightEntityPerfDemo());

		// Option 2b: Particle System Demo
		//PushLayer(new ParticleSystemDemo());

		// Option 2c: Particle Emitter Demo
		//PushLayer(new ParticleEmitterDemo());
		

		// Option 3: Heavy Entity Performance Demo
		//PushLayer(new HeavyEntityPerfDemo());

		// Option 4: Object Pool Demo
		//PushLayer(new ObjectPoolDemo());

		// Option 5: Audio System Demo
		//PushLayer(new AudioDemoLayer());
		
		// Option 6: Scene System Demo
		//PushLayer(new SceneDemoLayer());
		
		// Option 7: Animation System Demo (NEW!)
		//PushLayer(new AnimationDemoLayer());

		// Option 8: 2D Lighting + Shadows Demo
		PushLayer(new Lighting2DDemoLayer());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



