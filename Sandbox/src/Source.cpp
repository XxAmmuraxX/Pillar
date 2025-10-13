#include "Pillar.h"

class Sandbox : public Pillar::Application
{
public:
	Sandbox()
	{
	}
	~Sandbox()
	{
	}
};

Pillar::Application* Pillar::CreateApplication()
{
	return new Sandbox();
}


