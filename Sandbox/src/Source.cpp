#include "Pillar.h"
#include "ExampleLayer.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
		PushLayer(new Pillar::ExampleLayer());
	}
	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}


