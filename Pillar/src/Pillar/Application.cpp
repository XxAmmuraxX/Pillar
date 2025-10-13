#pragma once
#include "stdio.h"
#include "Application.h"
#include "Pillar/Logger.h"
#include "Pillar/Events/ApplicationEvent.h"

namespace Pillar
{
	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Pillar Engine", 1280, 720)));

	}
	Application::~Application()
	{
	}

	void Application::Run()
	{

		printf("Pillar Engine is running...\n");
		while (m_Running)
		{// Simulate application running
			m_Window->OnUpdate();
			PIL_CORE_INFO("Application is running...");
			PIL_CORE_ERROR("This is an error message for demonstration purposes.");
			PIL_CORE_WARN("This is a warning message for demonstration purposes.");
			// Here you would typically handle events, update the application state, render, etc.
			// For demonstration purposes, we will just sleep for a while.
			std::this_thread::sleep_for(std::chrono::seconds(1));
			// Simulate an event
			WindowResizeEvent resizeEvent(1280, 720);
			PIL_CORE_TRACE("Event: {0}", resizeEvent.ToString());
		}
	}
}

