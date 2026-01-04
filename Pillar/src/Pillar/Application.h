#pragma once
#include "Core.h"
#include "Window.h"
#include "Pillar/Events/ApplicationEvent.h"
#include "Pillar/LayerStack.h"
#include "Pillar/ImGuiLayer.h"

#include <memory>

namespace Pillar
{

	class PIL_API Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();
		void OnEvent(Event& e);
		void Close();

		// Layers API
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		inline Window& GetWindow() { return *m_Window; }
		inline const LayerStack& GetLayerStack() const { return m_LayerStack; }
		inline ImGuiLayer* GetImGuiLayer() { return m_ImGuiLayer; }

		static Application& Get();

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		bool m_Running = true;
		LayerStack m_LayerStack;

		static Application* s_Instance;
	};

	Application* CreateApplication();
}
