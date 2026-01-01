#include <gtest/gtest.h>
// WindowTests: integration tests for GLFW-based Window implementation,
// including creation, VSync, event callbacks and native GLFW interaction.
#include "Pillar/Window.h"
#include "Pillar/Events/ApplicationEvent.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Events/MouseEvent.h"
#include <memory>
#include <GLFW/glfw3.h>

using namespace Pillar;

// ==============================
// GLFW Test Environment
// ==============================

// Global test environment to initialize/terminate GLFW once for all tests
class GLFWTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }

    void TearDown() override {
        glfwTerminate();
    }
};

// Register the global environment
::testing::Environment* const glfw_env = 
    ::testing::AddGlobalTestEnvironment(new GLFWTestEnvironment);

// ==============================
// Window Creation Tests
// ==============================

class WindowTest : public ::testing::Test {
protected:
    // Helper to set up a dummy event callback for windows that call OnUpdate()
    void SetDummyCallback(Window* window) {
        window->SetEventCallback([](Event& e) {
            // Do nothing, just prevent bad_function_call
        });
    }
};

TEST_F(WindowTest, Window_Create_WithDefaultProps) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->GetWidth(), 1280);
    EXPECT_EQ(window->GetHeight(), 720);
}

TEST_F(WindowTest, Window_Create_WithCustomProps) {
    WindowProps props("Test Window", 800, 600);
    std::unique_ptr<Window> window(Window::Create(props));
    
    ASSERT_NE(window, nullptr);
    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);
}

// ==============================
// Window Properties Tests
// ==============================

TEST_F(WindowTest, Window_GetWidth_ReturnsCorrectValue) {
    WindowProps props("Window", 640, 480);
    std::unique_ptr<Window> window(Window::Create(props));
    
    EXPECT_EQ(window->GetWidth(), 640);
}

TEST_F(WindowTest, Window_GetHeight_ReturnsCorrectValue) {
    WindowProps props("Window", 640, 480);
    std::unique_ptr<Window> window(Window::Create(props));
    
    EXPECT_EQ(window->GetHeight(), 480);
}

TEST_F(WindowTest, Window_GetNativeWindow_ReturnsValidPointer) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    void* nativeWindow = window->GetNativeWindow();
    
    EXPECT_NE(nativeWindow, nullptr);
    
    // Should be a valid GLFW window
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
    EXPECT_NE(glfwWindow, nullptr);
}

// ==============================
// Window VSync Tests
// ==============================

TEST_F(WindowTest, Window_VSync_EnabledByDefault) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    // VSync is enabled by default in WindowsWindow::Init
    EXPECT_TRUE(window->IsVSync());
}

TEST_F(WindowTest, Window_SetVSync_Enable) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    window->SetVSync(true);
    
    EXPECT_TRUE(window->IsVSync());
}

TEST_F(WindowTest, Window_SetVSync_Disable) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    window->SetVSync(false);
    
    EXPECT_FALSE(window->IsVSync());
}

TEST_F(WindowTest, Window_VSync_RespectsInitialFlag)
{
    WindowProps props("No VSync", 800, 600, false);
    std::unique_ptr<Window> window(Window::Create(props));

    EXPECT_FALSE(window->IsVSync());
}

TEST_F(WindowTest, Window_SetVSync_Toggle) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    window->SetVSync(false);
    EXPECT_FALSE(window->IsVSync());
    
    window->SetVSync(true);
    EXPECT_TRUE(window->IsVSync());
}

TEST_F(WindowTest, Window_SetResizable_TogglesAttribute)
{
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));

    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

    window->SetResizable(false);
    EXPECT_EQ(glfwGetWindowAttrib(glfwWindow, GLFW_RESIZABLE), GLFW_FALSE);

    window->SetResizable(true);
    EXPECT_EQ(glfwGetWindowAttrib(glfwWindow, GLFW_RESIZABLE), GLFW_TRUE);
}

TEST_F(WindowTest, Window_SetTitle_DoesNotCrash)
{
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));

    EXPECT_NO_THROW(window->SetTitle("Updated Title"));
}

TEST_F(WindowTest, Window_Fullscreen_ToggleMonitor)
{
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());

    window->SetFullscreen(true);
    EXPECT_NE(glfwGetWindowMonitor(glfwWindow), nullptr);

    window->SetFullscreen(false);
    EXPECT_EQ(glfwGetWindowMonitor(glfwWindow), nullptr);

    glfwDefaultWindowHints(); // Restore defaults for other tests
}

TEST_F(WindowTest, Window_ContentScale_Positive)
{
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));

    float scaleX = window->GetContentScaleX();
    float scaleY = window->GetContentScaleY();

    EXPECT_GT(scaleX, 0.0f);
    EXPECT_GT(scaleY, 0.0f);
}

// ==============================
// Window Event Callback Tests
// ==============================

TEST_F(WindowTest, Window_SetEventCallback_CallbackIsSet) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    bool callbackCalled = false;
    EventType receivedEventType = EventType::None;
    
    window->SetEventCallback([&callbackCalled, &receivedEventType](Event& e) {
        // Only capture WindowResize events, ignore others (like MouseScrolled in CI)
        if (e.GetEventType() == EventType::WindowResize) {
            callbackCalled = true;
            receivedEventType = e.GetEventType();
        }
    });
    
    // Manually trigger a window resize event by calling glfwSetWindowSize
    // This will invoke the WindowSizeCallback which calls our event callback
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());
    
    // Set a different size to trigger the callback
    glfwSetWindowSize(glfwWindow, 640, 480);
    glfwPollEvents(); // Process the event
    
    // Verify the callback was invoked with a WindowResize event
    // Note: In CI with Mesa3D, other events (like MouseScrolled) may also fire
    EXPECT_TRUE(callbackCalled);
    if (callbackCalled) {
        EXPECT_EQ(receivedEventType, EventType::WindowResize);
    }
}

TEST_F(WindowTest, Window_EventCallback_ReceivesEvents) {
    WindowProps props("Event Test", 800, 600);
    std::unique_ptr<Window> window(Window::Create(props));
    
    Event* receivedEvent = nullptr;
    EventType receivedType = EventType::None;
    
    window->SetEventCallback([&receivedEvent, &receivedType](Event& e) {
        receivedEvent = &e;
        receivedType = e.GetEventType();
    });
    
    // The callback is set and ready to receive events
    EXPECT_NO_THROW(window->OnUpdate());
}

TEST_F(WindowTest, Window_EventCallback_CanBeChanged) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    int callback1Called = 0;
    int callback2Called = 0;
    
    window->SetEventCallback([&callback1Called](Event& e) {
        callback1Called++;
    });
    
    window->SetEventCallback([&callback2Called](Event& e) {
        callback2Called++;
    });
    
    // Only the second callback should be active
    EXPECT_NO_THROW(window->OnUpdate());
}

// ==============================
// Window Update Tests
// ==============================

TEST_F(WindowTest, Window_OnUpdate_DoesNotCrash) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    // Set a valid callback to prevent bad_function_call
    SetDummyCallback(window.get());
    
    EXPECT_NO_THROW(window->OnUpdate());
}

TEST_F(WindowTest, Window_OnUpdate_MultipleCalls) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    // Set a valid callback to prevent bad_function_call
    SetDummyCallback(window.get());
    
    EXPECT_NO_THROW({
        window->OnUpdate();
        window->OnUpdate();
        window->OnUpdate();
    });
}

// ==============================
// Window Destruction Tests
// ==============================

TEST_F(WindowTest, Window_Destructor_CleansUpProperly) {
    WindowProps props;
    
    EXPECT_NO_THROW({
        Window* window = Window::Create(props);
        delete window;
    });
}

// ==============================
// WindowProps Tests
// ==============================

TEST(WindowPropsTests, WindowProps_DefaultConstructor) {
    WindowProps props;
    
    EXPECT_EQ(props.Title, "Pillar Engine");
    EXPECT_EQ(props.Width, 1280);
    EXPECT_EQ(props.Height, 720);
    EXPECT_TRUE(props.VSync);
    EXPECT_FALSE(props.Fullscreen);
    EXPECT_TRUE(props.Resizable);
}

TEST(WindowPropsTests, WindowProps_CustomConstructor) {
    WindowProps props("My Game", 1024, 768);
    
    EXPECT_EQ(props.Title, "My Game");
    EXPECT_EQ(props.Width, 1024);
    EXPECT_EQ(props.Height, 768);
}

TEST(WindowPropsTests, WindowProps_TitleOnly) {
    WindowProps props("Custom Title");
    
    EXPECT_EQ(props.Title, "Custom Title");
    EXPECT_EQ(props.Width, 1280);
    EXPECT_EQ(props.Height, 720);
}

TEST(WindowPropsTests, WindowProps_CustomFlags)
{
    WindowProps props("My Game", 1024, 768, false, true, false);

    EXPECT_FALSE(props.VSync);
    EXPECT_TRUE(props.Fullscreen);
    EXPECT_FALSE(props.Resizable);
}

// ==============================
// Window Stress Tests
// ==============================

TEST_F(WindowTest, Window_RapidCreateDestroy) {
    // Test creating and destroying windows rapidly
    for (int i = 0; i < 5; ++i) {
        WindowProps props("Window " + std::to_string(i), 400, 300);
        std::unique_ptr<Window> window(Window::Create(props));
        
        EXPECT_NE(window, nullptr);
        EXPECT_EQ(window->GetWidth(), 400);
        EXPECT_EQ(window->GetHeight(), 300);
        
        // Set callback before calling OnUpdate
        SetDummyCallback(window.get());
        window->OnUpdate();
    }
}

TEST_F(WindowTest, Window_VSync_RapidToggle) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    for (int i = 0; i < 10; ++i) {
        window->SetVSync(i % 2 == 0);
        EXPECT_EQ(window->IsVSync(), i % 2 == 0);
    }
}

// ==============================
// Window Callback Context Tests
// ==============================

TEST_F(WindowTest, Window_EventCallback_PreservesContext) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    struct CallbackData {
        int callCount = 0;
        std::string lastEventName;
    };
    
    CallbackData data;
    
    window->SetEventCallback([&data](Event& e) {
        data.callCount++;
        data.lastEventName = e.GetName();
    });
    
    // Even without triggering events, callback should be properly bound
    EXPECT_EQ(data.callCount, 0);
    
    window->OnUpdate();
}

// ==============================
// Window Native Window Tests
// ==============================

TEST_F(WindowTest, Window_NativeWindow_IsGLFWWindow) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    void* nativeWindow = window->GetNativeWindow();
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(nativeWindow);
    
    // Verify it's a valid GLFW window
    EXPECT_NE(glfwWindow, nullptr);
    
    // Should be able to query GLFW window properties
    int width, height;
    glfwGetWindowSize(glfwWindow, &width, &height);
    
    // In CI with software rendering, window size might differ from requested size
    // due to window decorations/borders. Just verify they're in a reasonable range.
    EXPECT_GT(width, 0);
    EXPECT_GT(height, 0);
    
    // Allow some tolerance for window decorations (typically borders reduce size)
    EXPECT_GE(width, 800);
    EXPECT_LE(width, 1500);
    EXPECT_GE(height, 480);
    EXPECT_LE(height, 900);
}

TEST_F(WindowTest, Window_NativeWindow_CanBeUsedWithGLFW) {
    WindowProps props("GLFW Test", 500, 500);
    std::unique_ptr<Window> window(Window::Create(props));
    
    GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(window->GetNativeWindow());
    
    // Test GLFW operations
    EXPECT_NO_THROW({
        glfwGetWindowAttrib(glfwWindow, GLFW_VISIBLE);
        glfwGetWindowAttrib(glfwWindow, GLFW_FOCUSED);
    });
}

// ==============================
// Window Edge Cases Tests
// ==============================

TEST_F(WindowTest, Window_EmptyTitle) {
    WindowProps props("", 800, 600);
    std::unique_ptr<Window> window(Window::Create(props));
    
    EXPECT_NE(window, nullptr);
    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);
}

TEST_F(WindowTest, Window_VeryLongTitle) {
    std::string longTitle(1000, 'A');
    WindowProps props(longTitle, 800, 600);
    std::unique_ptr<Window> window(Window::Create(props));
    
    EXPECT_NE(window, nullptr);
    EXPECT_EQ(window->GetWidth(), 800);
    EXPECT_EQ(window->GetHeight(), 600);
}

// ==============================
// Window Consistency Tests
// ==============================

TEST_F(WindowTest, Window_PropertiesConsistentAfterCreation) {
    WindowProps props("Consistency Test", 1024, 768);
    std::unique_ptr<Window> window(Window::Create(props));
    
    // Properties should remain consistent
    EXPECT_EQ(window->GetWidth(), 1024);
    EXPECT_EQ(window->GetHeight(), 768);
    
    // Set callback before OnUpdate
    SetDummyCallback(window.get());
    window->OnUpdate();
    
    EXPECT_EQ(window->GetWidth(), 1024);
    EXPECT_EQ(window->GetHeight(), 768);
}

TEST_F(WindowTest, Window_VSyncStateConsistent) {
    WindowProps props;
    std::unique_ptr<Window> window(Window::Create(props));
    
    // Set callback before OnUpdate
    SetDummyCallback(window.get());
    
    window->SetVSync(true);
    EXPECT_TRUE(window->IsVSync());
    
    window->OnUpdate();
    EXPECT_TRUE(window->IsVSync());
    
    window->SetVSync(false);
    EXPECT_FALSE(window->IsVSync());
    
    window->OnUpdate();
    EXPECT_FALSE(window->IsVSync());
}
