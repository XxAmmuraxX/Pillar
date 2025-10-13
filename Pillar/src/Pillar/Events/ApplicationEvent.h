#pragma once
#include "Event.h"
#include <sstream>

namespace Pillar
{

	class PIL_API WindowCloseEvent : public Event
	{
	public:

		std::string ToString() const override
		{
			return "WindowCloseEvent";
		}

		EVENT_CLASS_TYPE(WindowClose)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
		: m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }
		
		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << " x " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_TYPE(WindowResize)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	private:
		unsigned int m_Width, m_Height;
	};

	class PIL_API WindowFocusEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "WindowFocusEvent";
		}
		EVENT_CLASS_TYPE(WindowFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API WindowLostFocusEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "WindowLostFocusEvent";
		}
		EVENT_CLASS_TYPE(WindowLostFocus)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API WindowMovedEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "WindowMovedEvent";
		}
		EVENT_CLASS_TYPE(WindowMoved)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API AppTickEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "AppTickEvent";
		}
		EVENT_CLASS_TYPE(AppTick)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API AppUpdateEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "AppUpdateEvent";
		}
		EVENT_CLASS_TYPE(AppUpdate)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class PIL_API AppRenderEvent : public Event
	{
		public:
		std::string ToString() const override
		{
			return "AppRenderEvent";
		}
		EVENT_CLASS_TYPE(AppRender)
		EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

}