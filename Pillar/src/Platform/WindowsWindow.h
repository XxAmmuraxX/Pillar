#pragma once
#include <string>
#include <memory>
#include "Pillar/Window.h"
#include "Pillar/Renderer/GraphicsContext.h"

struct GLFWwindow; // forward declaration to avoid including GLFW in header

namespace Pillar
{
	class PIL_API WindowsWindow : public Window
	{
	public:
		WindowsWindow(const WindowProps& props);
		virtual ~WindowsWindow();
		void PollEvents() override;
		void OnUpdate() override;
		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }
		void SetTitle(const std::string& title) override;
		void SetIcon(const std::string& iconPath) override;
		void SetResizable(bool resizable) override;
		void SetFullscreen(bool fullscreen) override;
		bool IsFullscreen() const override { return m_Data.Fullscreen; }
		float GetContentScaleX() const override;
		float GetContentScaleY() const override;
		void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;
		inline virtual void* GetNativeWindow() const override { return m_Window; }
	private:
		virtual void Init(const WindowProps& props);
		virtual void Shutdown();
		
		GLFWwindow* m_Window;
		std::unique_ptr<GraphicsContext> m_Context;
		
		struct WindowData
		{
			std::string Title;
			unsigned int Width, Height;
			int WindowPosX = 0, WindowPosY = 0;
			int WindowedWidth = 0, WindowedHeight = 0;
			bool VSync;
			bool Fullscreen = false;
			bool Resizable = true;
			EventCallbackFn EventCallback;
		};
		WindowData m_Data;
	};
}