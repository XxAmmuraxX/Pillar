#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "PhysicsDemoLayer.h"
#include "LightEntityPerfDemo.h"
#include "HeavyEntityPerfDemo.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		// CHOOSE YOUR DEMO: Uncomment one of the following lines
		
		// Option 1: Physics Demo (Gameplay)
		PushLayer(new PhysicsDemoLayer());
		
		// Option 2: Light Entity Performance Demo
		// PushLayer(new LightEntityPerfDemo());
		
		// Option 3: Heavy Entity Performance Demo
		// PushLayer(new HeavyEntityPerfDemo());
	}

	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}



