#pragma once
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Logger.h"
#ifdef PIL_WINDOWS

extern Pillar::Application* Pillar::CreateApplication();



int main(int argc, char** argv)
{
	Pillar::Logger::Init();
	auto app = Pillar::CreateApplication();
	app->Run();
	delete app;
	return 0;
}

#endif