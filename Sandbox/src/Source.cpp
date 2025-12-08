#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "PhysicsDemoLayer.h"
#include "LightEntityPerfDemo.h"
#include "HeavyEntityPerfDemo.h"
#include "ObjectPoolDemo.h"
#include "SceneDemoLayer.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		// CHOOSE YOUR DEMO: Uncomment one of the following lines
		
		// Option 1: Physics Demo (Gameplay)
		//PushLayer(new PhysicsDemoLayer());
		
		// Option 2: Light Entity Performance Demo
		// PushLayer(new LightEntityPerfDemo());
		
		// Option 3: Heavy Entity Performance Demo
		//PushLayer(new HeavyEntityPerfDemo());

		// Option 4: Object Pool Demo
		//PushLayer(new ObjectPoolDemo());

		// Option 5: Scene System Demo (NEW!)
		PushLayer(new SceneDemoLayer());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



