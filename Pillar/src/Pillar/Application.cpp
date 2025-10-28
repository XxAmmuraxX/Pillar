#pragma once
#include "stdio.h"
#include "Application.h"
#include "Pillar/Logger.h"
#include <chrono>
#include "Pillar/Input.h"

namespace Pillar
{

#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		PIL_CORE_ASSERT(!s_Instance, "Application already exists!");
		s_Instance = this;
		m_Window = std::unique_ptr<Window>(Window::Create(WindowProps("Pillar Engine", 1280, 720)));
		m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));

		// Create and push ImGui layer as an overlay
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}
	Application::~Application()
	{
		// Ensure layers are detached and destroyed via LayerStack destructor
	}

	Application& Application::Get()
	{
		return *s_Instance;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

		// Propagate to layers in reverse order (top-most first)
		for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it)
		{
			(*it)->OnEvent(e);
			if (e.Handled)
				break;
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		m_Running = false;
		return true;
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
	}

	void Application::Run()
	{

		printf("Pillar Engine is running...\n");
		PIL_CORE_INFO("Application is running...");
		PIL_CORE_ERROR("This is an error message for demonstration purposes.");
		PIL_CORE_WARN("This is a warning message for demonstration purposes.");

		auto lastTime = std::chrono::high_resolution_clock::now();
		while (m_Running)
		{// Simulate application running
			// Delta time
			auto now = std::chrono::high_resolution_clock::now();
			std::chrono::duration<float> dt = now - lastTime;
			lastTime = now;
			float deltaTime = dt.count();

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(deltaTime);
			}

			// Render ImGui
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

			m_Window->OnUpdate();
		}
	}
}

