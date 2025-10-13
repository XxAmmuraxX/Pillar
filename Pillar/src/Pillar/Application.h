#pragma once
#include "Core.h"
#include "Platform/WindowsWindow.h"

namespace Pillar
{

	class PIL_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

	private:
		std::unique_ptr<Window> m_Window;
		bool m_Running = true;
	};

	Application* CreateApplication();
}
