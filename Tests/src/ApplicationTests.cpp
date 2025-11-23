#include <gtest/gtest.h>
#include "Pillar/Application.h"
#include "Pillar/Layer.h"
#include "Pillar/Events/ApplicationEvent.h"
#include "Pillar/Events/KeyEvent.h"
#include "Pillar/Window.h"
#include "Pillar/Renderer/Renderer.h"
#include <memory>

using namespace Pillar;

// ==============================
// Test Application Implementation
// ==============================

class TestApp : public Application {
public:
    TestApp() : Application() {}
    ~TestApp() override {}
    
    // Expose protected methods for testing
    void TestOnEvent(Event& e) { OnEvent(e); }
    void StopRunning() { 
        // Trigger window close event to stop the app
        WindowCloseEvent event;
        OnEvent(event);
    }
};

// Test layer for tracking calls
class TrackingLayer : public Layer {
public:
    TrackingLayer(const std::string& name = "TrackingLayer")
        : Layer(name), m_OnAttachCalled(false), m_OnDetachCalled(false),
          m_OnUpdateCalled(false), m_OnEventCalled(false), m_EventReceived(nullptr) {}

    void OnAttach() override { m_OnAttachCalled = true; }
    void OnDetach() override { m_OnDetachCalled = true; }
    void OnUpdate(float dt) override { m_OnUpdateCalled = true; m_LastDeltaTime = dt; }
    void OnEvent(Event& event) override { 
        m_OnEventCalled = true; 
        m_EventReceived = &event;
        m_ReceivedEventType = event.GetEventType();
    }

    bool m_OnAttachCalled;
    bool m_OnDetachCalled;
    bool m_OnUpdateCalled;
    bool m_OnEventCalled;
    float m_LastDeltaTime = 0.0f;
    Event* m_EventReceived;
    EventType m_ReceivedEventType;
};

// ==============================
// Application Singleton Tests
// ==============================

TEST(ApplicationTests, Application_Singleton_GetInstance) {
    // Create an application
    TestApp* app = new TestApp();
    
    // Get singleton instance
    Application& instance = Application::Get();
    
    EXPECT_EQ(&instance, app);
    
    // Manually stop and delete to avoid infinite loop
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_HasWindow) {
    TestApp* app = new TestApp();
    
    Window& window = app->GetWindow();
    
    EXPECT_GT(window.GetWidth(), 0);
    EXPECT_GT(window.GetHeight(), 0);
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application Window Tests
// ==============================

TEST(ApplicationTests, Application_GetWindow_ReturnsValidWindow) {
    TestApp* app = new TestApp();
    
    Window& window = app->GetWindow();
    
    // Window should have valid dimensions
    EXPECT_EQ(window.GetWidth(), 1280);
    EXPECT_EQ(window.GetHeight(), 720);
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application Layer Management Tests
// ==============================

TEST(ApplicationTests, Application_PushLayer_AddsLayer) {
    TestApp* app = new TestApp();
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    
    app->PushLayer(layer);
    
    LayerStack& stack = const_cast<LayerStack&>(app->GetLayerStack());
    bool found = false;
    for (auto l : stack) {
        if (l == layer) {
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(layer->m_OnAttachCalled); // Layer should be attached
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_PushOverlay_AddsOverlay) {
    TestApp* app = new TestApp();
    TrackingLayer* overlay = new TrackingLayer("TestOverlay");
    
    app->PushOverlay(overlay);
    
    LayerStack& stack = const_cast<LayerStack&>(app->GetLayerStack());
    bool found = false;
    for (auto l : stack) {
        if (l == overlay) {
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(overlay->m_OnAttachCalled);
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_LayerStack_OverlaysAfterLayers) {
    TestApp* app = new TestApp();
    TrackingLayer* layer = new TrackingLayer("Layer");
    TrackingLayer* overlay = new TrackingLayer("Overlay");
    
    app->PushLayer(layer);
    app->PushOverlay(overlay);
    
    LayerStack& stack = const_cast<LayerStack&>(app->GetLayerStack());
    std::vector<Layer*> layers;
    for (auto l : stack) {
        // Skip ImGuiLayer that's auto-added
        if (l == layer || l == overlay) {
            layers.push_back(l);
        }
    }
    
    // Find positions
    auto layerPos = std::find(layers.begin(), layers.end(), layer);
    auto overlayPos = std::find(layers.begin(), layers.end(), overlay);
    
    EXPECT_TRUE(layerPos != layers.end());
    EXPECT_TRUE(overlayPos != layers.end());
    EXPECT_LT(layerPos - layers.begin(), overlayPos - layers.begin());
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application Event Handling Tests
// ==============================

TEST(ApplicationTests, Application_OnEvent_DispatchesToLayers) {
    TestApp* app = new TestApp();
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    app->PushLayer(layer);
    
    KeyPressedEvent event(65, 0); // 'A' key
    app->TestOnEvent(event);
    
    EXPECT_TRUE(layer->m_OnEventCalled);
    EXPECT_EQ(layer->m_ReceivedEventType, EventType::KeyPressed);
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_OnEvent_WindowClose_StopsApp) {
    TestApp* app = new TestApp();
    
    WindowCloseEvent event;
    app->TestOnEvent(event);
    
    // Event should be handled
    EXPECT_TRUE(event.Handled);
    
    delete app;
}

TEST(ApplicationTests, Application_OnEvent_WindowResize_NotHandled) {
    TestApp* app = new TestApp();
    
    WindowResizeEvent event(1920, 1080);
    app->TestOnEvent(event);
    
    // Window resize event should be processed but not marked as handled
    // (returns false in lambda)
    EXPECT_FALSE(event.Handled);
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_OnEvent_LayerCanHandleEvent) {
    TestApp* app = new TestApp();
    
    // Create a layer that handles events
    class EventHandlingLayer : public Layer {
    public:
        EventHandlingLayer() : Layer("Handler") {}
        void OnEvent(Event& event) override {
            event.Handled = true;
        }
    };
    
    EventHandlingLayer* layer1 = new EventHandlingLayer();
    TrackingLayer* layer2 = new TrackingLayer("Layer2");
    
    app->PushLayer(layer1);
    app->PushLayer(layer2);
    
    KeyPressedEvent event(65, 0);
    app->TestOnEvent(event);
    
    // Event should stop propagating after first layer handles it
    EXPECT_TRUE(event.Handled);
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_OnEvent_ReverseLayerOrder) {
    TestApp* app = new TestApp();
    
    class OrderTrackingLayer : public Layer {
    public:
        OrderTrackingLayer(const std::string& name, std::vector<std::string>& order)
            : Layer(name), m_Order(order) {}
        
        void OnEvent(Event& event) override {
            m_Order.push_back(GetName());
        }
        
        std::vector<std::string>& m_Order;
    };
    
    std::vector<std::string> order;
    OrderTrackingLayer* layer1 = new OrderTrackingLayer("Layer1", order);
    OrderTrackingLayer* layer2 = new OrderTrackingLayer("Layer2", order);
    OrderTrackingLayer* overlay1 = new OrderTrackingLayer("Overlay1", order);
    
    app->PushLayer(layer1);
    app->PushLayer(layer2);
    app->PushOverlay(overlay1);
    
    KeyPressedEvent event(65, 0);
    app->TestOnEvent(event);
    
    // Events should propagate from top to bottom (reverse order)
    // So overlays first, then layers in reverse
    ASSERT_GE(order.size(), 3);
    
    // Find our layers in the order (ImGuiLayer will also be in there)
    auto it1 = std::find(order.begin(), order.end(), "Layer1");
    auto it2 = std::find(order.begin(), order.end(), "Layer2");
    auto itOverlay = std::find(order.begin(), order.end(), "Overlay1");
    
    EXPECT_TRUE(it1 != order.end());
    EXPECT_TRUE(it2 != order.end());
    EXPECT_TRUE(itOverlay != order.end());
    
    // Overlay should come before layers
    EXPECT_LT(itOverlay - order.begin(), it2 - order.begin());
    EXPECT_LT(itOverlay - order.begin(), it1 - order.begin());
    
    // Layer2 should come before Layer1 (reverse order)
    EXPECT_LT(it2 - order.begin(), it1 - order.begin());
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application Lifecycle Tests
// ==============================

TEST(ApplicationTests, Application_Constructor_InitializesRenderer) {
    // Creating an application should initialize the renderer
    TestApp* app = new TestApp();
    
    // If renderer is initialized, we can call renderer functions
    // without crashing
    EXPECT_NO_THROW({
        Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    });
    
    app->StopRunning();
    delete app;
}

TEST(ApplicationTests, Application_Destructor_ShutsDownRenderer) {
    TestApp* app = new TestApp();
    app->StopRunning();
    
    // Destructor should clean up properly
    EXPECT_NO_THROW({
        delete app;
    });
}

TEST(ApplicationTests, Application_GetLayerStack_ReturnsStack) {
    TestApp* app = new TestApp();
    
    LayerStack& stack = const_cast<LayerStack&>(app->GetLayerStack());
    
    // Should at least have ImGuiLayer
    int count = 0;
    for (auto layer : stack) {
        count++;
    }
    
    EXPECT_GT(count, 0); // ImGuiLayer is auto-added
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application ImGuiLayer Tests
// ==============================

TEST(ApplicationTests, Application_HasImGuiLayerByDefault) {
    TestApp* app = new TestApp();
    
    LayerStack& stack = const_cast<LayerStack&>(app->GetLayerStack());
    bool hasImGuiLayer = false;
    
    for (auto layer : stack) {
        if (layer->GetName() == "ImGuiLayer") {
            hasImGuiLayer = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasImGuiLayer);
    
    app->StopRunning();
    delete app;
}

// ==============================
// Application Multiple Event Tests
// ==============================

TEST(ApplicationTests, Application_OnEvent_MultipleEventsHandled) {
    TestApp* app = new TestApp();
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    app->PushLayer(layer);
    
    KeyPressedEvent keyEvent(65, 0);
    app->TestOnEvent(keyEvent);
    EXPECT_TRUE(layer->m_OnEventCalled);
    
    layer->m_OnEventCalled = false; // Reset
    
    WindowResizeEvent resizeEvent(800, 600);
    app->TestOnEvent(resizeEvent);
    EXPECT_TRUE(layer->m_OnEventCalled);
    
    app->StopRunning();
    delete app;
}