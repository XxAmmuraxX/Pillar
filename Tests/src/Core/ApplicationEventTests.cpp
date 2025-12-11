#include <gtest/gtest.h>
#include "Pillar/Events/ApplicationEvent.h"

using namespace Pillar;

// ============================================================================
// WindowResizeEvent Tests
// ============================================================================

TEST(WindowResizeEventTests, Construction) {
    WindowResizeEvent event(1920, 1080);
    
    EXPECT_EQ(event.GetWidth(), 1920);
    EXPECT_EQ(event.GetHeight(), 1080);
    EXPECT_EQ(event.GetEventType(), EventType::WindowResize);
    EXPECT_STREQ(event.GetName(), "WindowResize");
}

TEST(WindowResizeEventTests, ToString) {
    WindowResizeEvent event(800, 600);
    std::string str = event.ToString();
    
    EXPECT_NE(str.find("800"), std::string::npos);
    EXPECT_NE(str.find("600"), std::string::npos);
}

TEST(WindowResizeEventTests, CategoryFlags) {
    WindowResizeEvent event(1024, 768);
    
    EXPECT_TRUE(event.IsInCategory(EventCategory::EventCategoryApplication));
    EXPECT_FALSE(event.IsInCategory(EventCategory::EventCategoryInput));
}

// ============================================================================
// WindowCloseEvent Tests
// ============================================================================

TEST(WindowCloseEventTests, Construction) {
    WindowCloseEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowClose);
    EXPECT_STREQ(event.GetName(), "WindowClose");
}

TEST(WindowCloseEventTests, CategoryFlags) {
    WindowCloseEvent event;
    
    EXPECT_TRUE(event.IsInCategory(EventCategory::EventCategoryApplication));
}

// ============================================================================
// WindowFocusEvent Tests
// ============================================================================

TEST(WindowFocusEventTests, Construction) {
    WindowFocusEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowFocus);
}

// ============================================================================
// WindowLostFocusEvent Tests
// ============================================================================

TEST(WindowLostFocusEventTests, Construction) {
    WindowLostFocusEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowLostFocus);
}

// ============================================================================
// WindowMovedEvent Tests
// ============================================================================

TEST(WindowMovedEventTests, Construction) {
    WindowMovedEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::WindowMoved);
    EXPECT_STREQ(event.GetName(), "WindowMoved");
    EXPECT_TRUE(event.IsInCategory(EventCategoryApplication));
}

// ============================================================================
// AppTickEvent Tests
// ============================================================================

TEST(AppTickEventTests, Construction) {
    AppTickEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::AppTick);
}

// ============================================================================
// AppUpdateEvent Tests
// ============================================================================

TEST(AppUpdateEventTests, Construction) {
    AppUpdateEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::AppUpdate);
}

// ============================================================================
// AppRenderEvent Tests
// ============================================================================

TEST(AppRenderEventTests, Construction) {
    AppRenderEvent event;
    
    EXPECT_EQ(event.GetEventType(), EventType::AppRender);
}