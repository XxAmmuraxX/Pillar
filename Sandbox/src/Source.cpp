#include "Pillar.h"
#include "ExampleLayer.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		PushLayer(new Pillar::ExampleLayer());
		PushOverlay(new Pillar::ImGuiLayer());
	}
	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}


