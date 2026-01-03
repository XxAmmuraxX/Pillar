#include "stdio.h"
#include "Application.h"
#include "Pillar/Logger.h"
#include "Pillar/Renderer/Renderer.h"
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Renderer/Lighting2D.h"
#include <chrono>
#include "Pillar/Input.h"
#include "Pillar/Time.h"

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

		// Initialize Audio Engine
		AudioEngine::Init();

		// Initialize Renderer
		Renderer::Init();
		Renderer2DBackend::Init();  // Batch renderer
		Lighting2D::Init();

		// Create and push ImGui layer as an overlay
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
	}
	
	Application::~Application()
	{
		// Ensure layers are detached and destroyed via LayerStack destructor
		Lighting2D::Shutdown();
		Renderer2DBackend::Shutdown();
		Renderer::Shutdown();
		AudioEngine::Shutdown();
	}

	Application& Application::Get()
	{
		return *s_Instance;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);
		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

		// Handle window resize for renderer viewport
		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
			Renderer::SetViewport(0, 0, event.GetWidth(), event.GetHeight());
			return false;
		});

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
		Close();
		return true;
	}

	void Application::Close()
	{
		m_Running = false;
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

		auto lastTime = std::chrono::steady_clock::now();
		while (m_Running)
		{// Simulate application running
			// Delta time
			auto now = std::chrono::steady_clock::now();
			std::chrono::duration<float> dt = now - lastTime;
			lastTime = now;
			float unscaledDeltaTime = dt.count();
			Time::Tick(unscaledDeltaTime);
			float deltaTime = Time::GetDeltaTime();

			m_Window->PollEvents();
			Input::OnUpdate();

			// Clear screen
			Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
			Renderer::Clear();

			// Begin scene

			for (Layer* layer : m_LayerStack)
			{
				layer->OnUpdate(deltaTime);
			}

			// End scene

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

