#pragma once
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "Logger.h"
#ifdef PIL_WINDOWS

extern Pillar::Application* Pillar::CreateApplication();



int main(int argc, char** argv)
{
	Pillar::Logger::Init();
	PIL_CORE_INFO("Pillar Engine Initialized");
	PIL_INFO("Starting...int={0:d}, bin={0:b}, hex={0:x} ",4 );
	PIL_INFO("Starting...int={0:d}, bin={0:b}, hex={0:x} ", 4);
	PIL_INFO("Starting...int={0:d}, bin={0:b}, hex={0:x} ", 4);
	PIL_CORE_WARN("Warning: This is a warning message");
	PIL_CORE_ERROR("Error: This is an error message");
	PIL_CORE_ERROR("Error: This is an error message with a number: {0}", 42);
	PIL_CORE_INFO("Application is running");
	PIL_CORE_TRACE("Trace: This is a trace message");
	PIL_CORE_ERROR("Fatal: This is a fatal error message");
	Pillar::Event* event = new Pillar::WindowResizeEvent(1920,1080);
	PIL_CORE_TRACE("Event: {0}", event->ToString());
	

	auto app = Pillar::CreateApplication();
	app->Run();
	delete app;
	return 0;
}

#endif