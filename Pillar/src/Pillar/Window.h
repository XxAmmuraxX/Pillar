#pragma once


#include <string>
#include <functional>

#include "Pillar/Core.h"
#include "Pillar/Events/Event.h"

namespace Pillar
{
	struct WindowProps
	{
		std::string Title;
		unsigned int Width;
		unsigned int Height;
		bool VSync;
		bool Fullscreen;
		bool Resizable;
		WindowProps(const std::string& title = "Pillar Engine",
			unsigned int width = 1280,
			unsigned int height = 720,
			bool vsync = true,
			bool fullscreen = false,
			bool resizable = true)
			: Title(title), Width(width), Height(height), VSync(vsync), Fullscreen(fullscreen), Resizable(resizable) {
		}
	};

	class PIL_API Window
	{
		public:
		using EventCallbackFn = std::function<void(Event&)>;
		virtual ~Window() {}
		virtual void PollEvents() = 0;
		virtual void OnUpdate() = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		virtual void SetTitle(const std::string& title) = 0;
		virtual void SetIcon(const std::string& iconPath) = 0;
		virtual void SetResizable(bool resizable) = 0;
		virtual void SetFullscreen(bool fullscreen) = 0;
		virtual bool IsFullscreen() const = 0;
		virtual float GetContentScaleX() const = 0;
		virtual float GetContentScaleY() const = 0;
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void* GetNativeWindow() const = 0;
		static Window* Create(const WindowProps& props = WindowProps());
	};
}