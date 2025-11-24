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
// Application Test Fixture
// ==============================

class ApplicationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a fresh application for each test
        m_App = new TestApp();
    }
    
    void TearDown() override {
        // Clean up application automatically
        if (m_App) {
            m_App->StopRunning();
            delete m_App;
            m_App = nullptr;
        }
    }
    
    TestApp* m_App = nullptr;
};

// ==============================
// Application Singleton Tests
// ==============================

TEST_F(ApplicationTest, Application_Singleton_GetInstance) {
    // Get singleton instance
    Application& instance = Application::Get();
    
    EXPECT_EQ(&instance, m_App);
}

TEST_F(ApplicationTest, Application_HasWindow) {
    Window& window = m_App->GetWindow();
    
    EXPECT_GT(window.GetWidth(), 0);
    EXPECT_GT(window.GetHeight(), 0);
}

// ==============================
// Application Window Tests
// ==============================

TEST_F(ApplicationTest, Application_GetWindow_ReturnsValidWindow) {
    Window& window = m_App->GetWindow();
    
    // Window should have valid dimensions
    EXPECT_EQ(window.GetWidth(), 1280);
    EXPECT_EQ(window.GetHeight(), 720);
}

// ==============================
// Application Layer Management Tests
// ==============================

TEST_F(ApplicationTest, Application_PushLayer_AddsLayer) {
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    
    m_App->PushLayer(layer);
    
    LayerStack& stack = const_cast<LayerStack&>(m_App->GetLayerStack());
    bool found = false;
    for (auto l : stack) {
        if (l == layer) {
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(layer->m_OnAttachCalled); // Layer should be attached
}

TEST_F(ApplicationTest, Application_PushOverlay_AddsOverlay) {
    TrackingLayer* overlay = new TrackingLayer("TestOverlay");
    
    m_App->PushOverlay(overlay);
    
    LayerStack& stack = const_cast<LayerStack&>(m_App->GetLayerStack());
    bool found = false;
    for (auto l : stack) {
        if (l == overlay) {
            found = true;
            break;
        }
    }
    
    EXPECT_TRUE(found);
    EXPECT_TRUE(overlay->m_OnAttachCalled);
}

TEST_F(ApplicationTest, Application_LayerStack_OverlaysAfterLayers) {
    TrackingLayer* layer = new TrackingLayer("Layer");
    TrackingLayer* overlay = new TrackingLayer("Overlay");
    
    m_App->PushLayer(layer);
    m_App->PushOverlay(overlay);
    
    LayerStack& stack = const_cast<LayerStack&>(m_App->GetLayerStack());
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
}

// ==============================
// Application Event Handling Tests
// ==============================

TEST_F(ApplicationTest, Application_OnEvent_DispatchesToLayers) {
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    m_App->PushLayer(layer);
    
    KeyPressedEvent event(65, 0); // 'A' key
    m_App->TestOnEvent(event);
    
    EXPECT_TRUE(layer->m_OnEventCalled);
    EXPECT_EQ(layer->m_ReceivedEventType, EventType::KeyPressed);
}

TEST_F(ApplicationTest, Application_OnEvent_WindowClose_StopsApp) {
    WindowCloseEvent event;
    m_App->TestOnEvent(event);
    
    // Event should be handled
    EXPECT_TRUE(event.Handled);
}

TEST_F(ApplicationTest, Application_OnEvent_WindowResize_NotHandled) {
    WindowResizeEvent event(1920, 1080);
    m_App->TestOnEvent(event);
    
    // Window resize event should be processed but not marked as handled
    // (returns false in lambda)
    EXPECT_FALSE(event.Handled);
}

TEST_F(ApplicationTest, Application_OnEvent_LayerCanHandleEvent) {
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
    
    m_App->PushLayer(layer1);
    m_App->PushLayer(layer2);
    
    KeyPressedEvent event(65, 0);
    m_App->TestOnEvent(event);
    
    // Event should stop propagating after first layer handles it
    EXPECT_TRUE(event.Handled);
}

TEST_F(ApplicationTest, Application_OnEvent_ReverseLayerOrder) {
    class OrderTrackingLayer : public Layer {
    public:
        OrderTrackingLayer(const std::string& name, std::vector<std::string>* order)
            : Layer(name), m_Order(order) {}
        
        void OnEvent(Event& event) override {
            if (m_Order) {
                m_Order->push_back(GetName());
            }
        }
        
        std::vector<std::string>* m_Order;
    };
    
    std::vector<std::string> order;
    OrderTrackingLayer* layer1 = new OrderTrackingLayer("Layer1", &order);
    OrderTrackingLayer* layer2 = new OrderTrackingLayer("Layer2", &order);
    OrderTrackingLayer* overlay1 = new OrderTrackingLayer("Overlay1", &order);
    
    m_App->PushLayer(layer1);
    m_App->PushLayer(layer2);
    m_App->PushOverlay(overlay1);
    
    KeyPressedEvent event(65, 0);
    m_App->TestOnEvent(event);
    
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
    
    // Clear the pointer to prevent use-after-free during cleanup
    layer1->m_Order = nullptr;
    layer2->m_Order = nullptr;
    overlay1->m_Order = nullptr;
}

// ==============================
// Application Lifecycle Tests
// ==============================

TEST_F(ApplicationTest, Application_Constructor_InitializesRenderer) {
    // If renderer is initialized, we can call renderer functions without crashing
    EXPECT_NO_THROW({
        Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    });
}

TEST_F(ApplicationTest, Application_Destructor_ShutsDownRenderer) {
    // Destructor should clean up properly (tested via TearDown)
    EXPECT_NO_THROW({
        m_App->StopRunning();
        delete m_App;
        m_App = nullptr; // Prevent double-delete in TearDown
    });
}

TEST_F(ApplicationTest, Application_GetLayerStack_ReturnsStack) {
    LayerStack& stack = const_cast<LayerStack&>(m_App->GetLayerStack());
    
    // Should at least have ImGuiLayer
    int count = 0;
    for (auto layer : stack) {
        count++;
    }
    
    EXPECT_GT(count, 0); // ImGuiLayer is auto-added
}

// ==============================
// Application ImGuiLayer Tests
// ==============================

TEST_F(ApplicationTest, Application_HasImGuiLayerByDefault) {
    LayerStack& stack = const_cast<LayerStack&>(m_App->GetLayerStack());
    bool hasImGuiLayer = false;
    
    for (auto layer : stack) {
        if (layer->GetName() == "ImGuiLayer") {
            hasImGuiLayer = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasImGuiLayer);
}

// ==============================
// Application Multiple Event Tests
// ==============================

TEST_F(ApplicationTest, Application_OnEvent_MultipleEventsHandled) {
    TrackingLayer* layer = new TrackingLayer("TestLayer");
    m_App->PushLayer(layer);
    
    KeyPressedEvent keyEvent(65, 0);
    m_App->TestOnEvent(keyEvent);
    EXPECT_TRUE(layer->m_OnEventCalled);
    
    layer->m_OnEventCalled = false; // Reset
    
    WindowResizeEvent resizeEvent(800, 600);
    m_App->TestOnEvent(resizeEvent);
    EXPECT_TRUE(layer->m_OnEventCalled);
}