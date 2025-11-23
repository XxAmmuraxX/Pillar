#include "Pillar.h"
#include "Pillar/EntryPoint.h"
#include "ExampleLayer.h"
#include "Pillar/ImGuiLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		PushLayer(new ExampleLayer());
	}
	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}


