#pragma once
#include "Core.h"
#include "Platform/WindowsWindow.h"
#include "Pillar/Events/ApplicationEvent.h"

namespace Pillar
{

	class PIL_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	Application* CreateApplication();
}
