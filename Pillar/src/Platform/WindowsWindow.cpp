#include "WindowsWindow.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Pillar/Events/ApplicationEvent.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Logger.h"
#include "Pillar/Input.h"
#include "stb_image.h"
#include <GLFW/glfw3.h>

namespace Pillar
{
	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProps& props)
	{
		return new WindowsWindow(props);
	}

	WindowsWindow::WindowsWindow(const WindowProps& props)
	{
		Init(props);
	}

	WindowsWindow::~WindowsWindow()
	{
		Shutdown();
	}

	void WindowsWindow::Init(const WindowProps& props)
	{
		m_Data.Title = props.Title;
		m_Data.Width = props.Width;
		m_Data.Height = props.Height;
		m_Data.VSync = props.VSync;
		m_Data.Fullscreen = props.Fullscreen;
		m_Data.Resizable = props.Resizable;

		PIL_CORE_INFO("Creating window {0} ({1}, {2})", props.Title, props.Width, props.Height);

		if (!s_GLFWInitialized)
		{
			int success = glfwInit();
			PIL_CORE_ASSERT(success, "Could not initialize GLFW");
			glfwSetErrorCallback([](int error, const char* description)
				{
					PIL_CORE_ERROR("GLFW Error ({0}): {1}", error, description);
				});
			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_RESIZABLE, props.Resizable ? GLFW_TRUE : GLFW_FALSE);

		m_Window = glfwCreateWindow((int)m_Data.Width, (int)m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
		if (!m_Window)
		{
			glfwTerminate();
			PIL_CORE_ERROR("Failed to create GLFW window");
			return;
		}

		// Create and initialize graphics context
		m_Context = std::make_unique<OpenGLContext>(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		m_Data.EventCallback = [](Event&) {};
		SetVSync(props.VSync);

		glfwGetWindowPos(m_Window, &m_Data.WindowPosX, &m_Data.WindowPosY);
		m_Data.WindowedWidth = static_cast<int>(m_Data.Width);
		m_Data.WindowedHeight = static_cast<int>(m_Data.Height);

		if (props.Fullscreen)
		{
			SetFullscreen(true);
		}

		// Setting up GLFW callbacks
		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				data.Width = width;
				data.Height = height;
				WindowResizeEvent event(width, height);
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				switch (action)
				{
					case GLFW_PRESS:
					{
						KeyPressedEvent event(key, 0);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						KeyReleasedEvent event(key);
						data.EventCallback(event);
						break;
					}
					case GLFW_REPEAT:
					{
						KeyPressedEvent event(key, 1);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int keycode)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				// Character callback can be used for text input if needed
				// For now, we'll just log it via the key events
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				switch (action)
				{
					case GLFW_PRESS:
					{
						MouseButtonPressedEvent event(button);
						data.EventCallback(event);
						break;
					}
					case GLFW_RELEASE:
					{
						MouseButtonReleasedEvent event(button);
						data.EventCallback(event);
						break;
					}
				}
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				MouseMovedEvent event((float)xPos, (float)yPos);
				data.EventCallback(event);
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				MouseScrolledEvent event((float)xOffset, (float)yOffset);
				Input::OnScroll((float)xOffset, (float)yOffset);
				data.EventCallback(event);
			});

		glfwSetWindowFocusCallback(m_Window, [](GLFWwindow* window, int focused)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				if (focused)
				{
					WindowFocusEvent event;
					data.EventCallback(event);
				}
				else
				{
					WindowLostFocusEvent event;
					data.EventCallback(event);
				}
			});

		glfwSetWindowPosCallback(m_Window, [](GLFWwindow* window, int xPos, int yPos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowMovedEvent event;
				data.WindowPosX = xPos;
				data.WindowPosY = yPos;
				data.EventCallback(event);
			});

	}

	void WindowsWindow::Shutdown()
	{
		if (m_Window)
		{
			glfwDestroyWindow(m_Window);
		}
	}

	void WindowsWindow::OnUpdate()
	{
		m_Context->SwapBuffers();
	}

	void WindowsWindow::SetTitle(const std::string& title)
	{
		m_Data.Title = title;
		if (m_Window)
		{
			glfwSetWindowTitle(m_Window, m_Data.Title.c_str());
		}
	}

	void WindowsWindow::SetIcon(const std::string& iconPath)
	{
		int width = 0, height = 0, channels = 0;
		stbi_uc* pixels = stbi_load(iconPath.c_str(), &width, &height, &channels, 4);
		if (!pixels)
		{
			PIL_CORE_WARN("Failed to load window icon: {0}", iconPath);
			return;
		}

		GLFWimage image;
		image.width = width;
		image.height = height;
		image.pixels = pixels;
		glfwSetWindowIcon(m_Window, 1, &image);
		stbi_image_free(pixels);
	}

	void WindowsWindow::SetResizable(bool resizable)
	{
		m_Data.Resizable = resizable;
		glfwSetWindowAttrib(m_Window, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
	}

	void WindowsWindow::SetFullscreen(bool fullscreen)
	{
		if (fullscreen == m_Data.Fullscreen)
		{
			return;
		}

		if (fullscreen)
		{
			glfwGetWindowPos(m_Window, &m_Data.WindowPosX, &m_Data.WindowPosY);
			glfwGetWindowSize(m_Window, &m_Data.WindowedWidth, &m_Data.WindowedHeight);
			GLFWmonitor* monitor = glfwGetPrimaryMonitor();
			const GLFWvidmode* mode = glfwGetVideoMode(monitor);
			glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			m_Data.Width = mode->width;
			m_Data.Height = mode->height;
		}
		else
		{
			glfwSetWindowMonitor(m_Window, nullptr, m_Data.WindowPosX, m_Data.WindowPosY, m_Data.WindowedWidth, m_Data.WindowedHeight, 0);
			m_Data.Width = static_cast<unsigned int>(m_Data.WindowedWidth);
			m_Data.Height = static_cast<unsigned int>(m_Data.WindowedHeight);
		}

		m_Data.Fullscreen = fullscreen;
	}

	float WindowsWindow::GetContentScaleX() const
	{
		float scaleX = 1.0f, scaleY = 1.0f;
		glfwGetWindowContentScale(m_Window, &scaleX, &scaleY);
		return scaleX;
	}

	float WindowsWindow::GetContentScaleY() const
	{
		float scaleX = 1.0f, scaleY = 1.0f;
		glfwGetWindowContentScale(m_Window, &scaleX, &scaleY);
		return scaleY;
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
			glfwSwapInterval(1);
		else
			glfwSwapInterval(0);

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const
	{
		return m_Data.VSync;
	}

	void WindowsWindow::PollEvents()
	{
		glfwPollEvents();
	}
}
