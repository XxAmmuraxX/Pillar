#include <gtest/gtest.h>
#include "Pillar/Events/Event.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"
#include "Pillar/Events/ApplicationEvent.h"

using namespace Pillar;

// ==============================
// Event Type Tests
// ==============================

TEST(EventTests, KeyPressedEvent_Creation) {
    KeyPressedEvent event(65, 0); // 'A' key, no repeats
    
    EXPECT_EQ(event.GetEventType(), EventType::KeyPressed);
    EXPECT_EQ(event.GetKeyCode(), 65);
    EXPECT_EQ(event.GetRepeatCount(), 0);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryKeyboard)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput)));
}

TEST(EventTests, KeyReleasedEvent_Creation) {
    KeyReleasedEvent event(83); // 'S' key
    
    EXPECT_EQ(event.GetEventType(), EventType::KeyReleased);
    EXPECT_EQ(event.GetKeyCode(), 83);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryKeyboard)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput)));
}

TEST(EventTests, MouseButtonPressed_Creation) {
    MouseButtonPressedEvent event(0); // Left mouse button
    
    EXPECT_EQ(event.GetEventType(), EventType::MouseButtonPressed);
    EXPECT_EQ(event.GetMouseButton(), 0);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouseButton)));
}

TEST(EventTests, MouseButtonReleased_Creation) {
    MouseButtonReleasedEvent event(1); // Right mouse button
    
    EXPECT_EQ(event.GetEventType(), EventType::MouseButtonReleased);
    EXPECT_EQ(event.GetMouseButton(), 1);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouseButton)));
}

TEST(EventTests, MouseMoved_Creation) {
    MouseMovedEvent event(100.0f, 200.0f);
    
    EXPECT_EQ(event.GetEventType(), EventType::MouseMoved);
    EXPECT_FLOAT_EQ(event.GetX(), 100.0f);
    EXPECT_FLOAT_EQ(event.GetY(), 200.0f);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
}

TEST(EventTests, MouseScrolled_Creation) {
    MouseScrolledEvent event(1.0f, -1.0f);
    
    EXPECT_EQ(event.GetEventType(), EventType::MouseScrolled);
    EXPECT_FLOAT_EQ(event.GetXOffset(), 1.0f);
    EXPECT_FLOAT_EQ(event.GetYOffset(), -1.0f);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput)));
}

TEST(EventTests, WindowResize_Creation) {
    WindowResizeEvent event(1920, 1080);
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowResize);
    EXPECT_EQ(event.GetWidth(), 1920);
    EXPECT_EQ(event.GetHeight(), 1080);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryApplication)));
}

TEST(EventTests, WindowClose_Creation) {
    WindowCloseEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowClose);
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryApplication)));
}

// ==============================
// Event Dispatcher Tests
// ==============================

TEST(EventDispatcherTests, Dispatch_CorrectType_HandlerCalled) {
    KeyPressedEvent event(65, 0);
    bool handlerCalled = false;
    
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>([&handlerCalled](KeyPressedEvent& e) {
        handlerCalled = true;
        EXPECT_EQ(e.GetKeyCode(), 65);
        return true;
    });
    
    EXPECT_TRUE(handlerCalled);
    EXPECT_TRUE(event.Handled);
}

TEST(EventDispatcherTests, Dispatch_WrongType_HandlerNotCalled) {
    KeyPressedEvent event(65, 0);
    bool handlerCalled = false;
    
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<MouseMovedEvent>([&handlerCalled](MouseMovedEvent& e) {
        handlerCalled = true;
        return true;
    });
    
    EXPECT_FALSE(handlerCalled);
    EXPECT_FALSE(event.Handled);
}

TEST(EventDispatcherTests, Dispatch_HandlerReturnsFalse_EventNotHandled) {
    KeyPressedEvent event(65, 0);
    
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>([](KeyPressedEvent& e) {
        return false; // Don't mark as handled
    });
    
    EXPECT_FALSE(event.Handled);
}

TEST(EventDispatcherTests, Dispatch_MultipleDispatches_OnlyFirstHandles) {
    KeyPressedEvent event(65, 0);
    int callCount = 0;

    // Create separate dispatchers (like in real app)
    EventDispatcher dispatcher1(event);
    dispatcher1.Dispatch<KeyPressedEvent>([&callCount](KeyPressedEvent& e) {
        callCount++;
        return true;
        });

    // Second dispatcher should check if event already handled
    if (!event.Handled) {
        EventDispatcher dispatcher2(event);
        dispatcher2.Dispatch<KeyPressedEvent>([&callCount](KeyPressedEvent& e) {
            callCount++;
            return true;
            });
    }

    EXPECT_EQ(callCount, 1);
    EXPECT_TRUE(event.Handled);
}

// ==============================
// Event Category Tests
// ==============================

TEST(EventTests, EventCategory_KeyboardIsInput) {
    KeyPressedEvent event(65, 0);
    
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryKeyboard)));
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryApplication)));
}

TEST(EventTests, EventCategory_MouseIsInput) {
    MouseMovedEvent event(0.0f, 0.0f);
    
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput))); // MouseMovedEvent only has Mouse category
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryKeyboard)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryApplication)));
}

TEST(EventTests, EventCategory_ApplicationEventsNotInput) {
    WindowCloseEvent event;
    
    EXPECT_TRUE(event.IsInCategory(static_cast<EventCategory>(EventCategoryApplication)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryInput)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryKeyboard)));
    EXPECT_FALSE(event.IsInCategory(static_cast<EventCategory>(EventCategoryMouse)));
}
