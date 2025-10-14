#pragma once
#include "stdio.h"
#include "Application.h"
#include "Pillar/Logger.h"


namespace Pillar
{

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

	Application::Application()
	{
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Pillar Engine", 1280, 720)));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

	}
	Application::~Application()
	{
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));
		PIL_CORE_TRACE("{0}", e.ToString());
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}


	void Application::Run()
	{

		printf("Pillar Engine is running...\n");
		PIL_CORE_INFO("Application is running...");
		PIL_CORE_ERROR("This is an error message for demonstration purposes.");
		PIL_CORE_WARN("This is a warning message for demonstration purposes.");
		while (m_Running)
		{// Simulate application running
			m_Window->OnUpdate();

			// Here you would typically handle events, update the application state, render, etc.
			// For demonstration purposes, we will just sleep for a while.
			//std::this_thread::sleep_for(std::chrono::seconds(1));

		}
	}
}

