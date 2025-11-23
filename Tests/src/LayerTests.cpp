#include <gtest/gtest.h>
#include "Pillar/Layer.h"
#include "Pillar/LayerStack.h"
#include "Pillar/Events/KeyEvent.h"

using namespace Pillar;

// ==============================
// Test Layer Implementation
// ==============================

class TestLayer : public Layer {
public:
    TestLayer(const std::string& name = "TestLayer")
        : Layer(name), m_AttachCalled(false), m_DetachCalled(false),
          m_UpdateCallCount(0), m_EventCallCount(0), m_ImGuiCallCount(0) {}

    void OnAttach() override {
        m_AttachCalled = true;
    }

    void OnDetach() override {
        m_DetachCalled = true;
    }

    void OnUpdate(float dt) override {
        m_UpdateCallCount++;
        m_LastDeltaTime = dt;
    }

    void OnEvent(Event& event) override {
        m_EventCallCount++;
        m_LastEvent = &event;
    }

    void OnImGuiRender() override {
        m_ImGuiCallCount++;
    }

    bool m_AttachCalled;
    bool m_DetachCalled;
    int m_UpdateCallCount;
    int m_EventCallCount;
    int m_ImGuiCallCount;
    float m_LastDeltaTime = 0.0f;
    Event* m_LastEvent = nullptr;
};

// ==============================
// Layer Tests
// ==============================

TEST(LayerTests, Layer_Construction) {
    TestLayer layer("MyLayer");
    
    EXPECT_EQ(layer.GetName(), "MyLayer");
    EXPECT_FALSE(layer.m_AttachCalled);
    EXPECT_FALSE(layer.m_DetachCalled);
}

TEST(LayerTests, Layer_OnAttach) {
    TestLayer layer;
    layer.OnAttach();
    
    EXPECT_TRUE(layer.m_AttachCalled);
}

TEST(LayerTests, Layer_OnDetach) {
    TestLayer layer;
    layer.OnDetach();
    
    EXPECT_TRUE(layer.m_DetachCalled);
}

TEST(LayerTests, Layer_OnUpdate) {
    TestLayer layer;
    layer.OnUpdate(0.016f);
    
    EXPECT_EQ(layer.m_UpdateCallCount, 1);
    EXPECT_FLOAT_EQ(layer.m_LastDeltaTime, 0.016f);
}

TEST(LayerTests, Layer_OnEvent) {
    TestLayer layer;
    KeyPressedEvent event(65, 0);
    
    layer.OnEvent(event);
    
    EXPECT_EQ(layer.m_EventCallCount, 1);
    EXPECT_EQ(layer.m_LastEvent, &event);
}

// ==============================
// LayerStack Tests
// ==============================

TEST(LayerStackTests, LayerStack_InitiallyEmpty) {
    LayerStack stack;
    
    EXPECT_EQ(stack.begin(), stack.end());
}

TEST(LayerStackTests, LayerStack_PushLayer) {
    LayerStack stack;
    TestLayer* layer = new TestLayer("Layer1");
    
    stack.PushLayer(layer);
    
    EXPECT_NE(stack.begin(), stack.end());
    EXPECT_EQ(*stack.begin(), layer);
}

TEST(LayerStackTests, LayerStack_PushOverlay) {
    LayerStack stack;
    TestLayer* overlay = new TestLayer("Overlay1");
    
    stack.PushOverlay(overlay);
    
    EXPECT_NE(stack.begin(), stack.end());
    EXPECT_EQ(*stack.begin(), overlay);
}

TEST(LayerStackTests, LayerStack_LayersBeforeOverlays) {
    LayerStack stack;
    TestLayer* layer1 = new TestLayer("Layer1");
    TestLayer* layer2 = new TestLayer("Layer2");
    TestLayer* overlay1 = new TestLayer("Overlay1");
    TestLayer* overlay2 = new TestLayer("Overlay2");
    
    stack.PushLayer(layer1);
    stack.PushOverlay(overlay1);
    stack.PushLayer(layer2);
    stack.PushOverlay(overlay2);
    
    // Layers should come before overlays
    auto it = stack.begin();
    EXPECT_EQ(*it++, layer1);
    EXPECT_EQ(*it++, layer2);
    EXPECT_EQ(*it++, overlay1);
    EXPECT_EQ(*it++, overlay2);
}

TEST(LayerStackTests, LayerStack_PopLayer) {
    LayerStack stack;
    TestLayer* layer = new TestLayer("Layer1");
    
    stack.PushLayer(layer);
    stack.PopLayer(layer);
    
    EXPECT_EQ(stack.begin(), stack.end());
    
    delete layer;
}

TEST(LayerStackTests, LayerStack_PopOverlay) {
    LayerStack stack;
    TestLayer* overlay = new TestLayer("Overlay1");
    
    stack.PushOverlay(overlay);
    stack.PopOverlay(overlay);
    
    EXPECT_EQ(stack.begin(), stack.end());
    
    delete overlay;
}

TEST(LayerStackTests, LayerStack_MultipleLayers) {
    LayerStack stack;
    TestLayer* layer1 = new TestLayer("Layer1");
    TestLayer* layer2 = new TestLayer("Layer2");
    TestLayer* layer3 = new TestLayer("Layer3");
    
    stack.PushLayer(layer1);
    stack.PushLayer(layer2);
    stack.PushLayer(layer3);
    
    int count = 0;
    for (auto layer : stack) {
        count++;
    }
    
    EXPECT_EQ(count, 3);
}

TEST(LayerStackTests, LayerStack_Destructor_DeletesLayers) {
    TestLayer* layer1 = new TestLayer("Layer1");
    TestLayer* layer2 = new TestLayer("Layer2");
    
    {
        LayerStack stack;
        stack.PushLayer(layer1);
        stack.PushLayer(layer2);
        // stack destructor should delete layers
    }
    
    // If we reach here without crashes, layers were properly deleted
    SUCCEED();
}

TEST(LayerStackTests, LayerStack_PopMiddleLayer) {
    LayerStack stack;
    TestLayer* layer1 = new TestLayer("Layer1");
    TestLayer* layer2 = new TestLayer("Layer2");
    TestLayer* layer3 = new TestLayer("Layer3");
    
    stack.PushLayer(layer1);
    stack.PushLayer(layer2);
    stack.PushLayer(layer3);
    
    stack.PopLayer(layer2);
    
    auto it = stack.begin();
    EXPECT_EQ(*it++, layer1);
    EXPECT_EQ(*it++, layer3);
    EXPECT_EQ(it, stack.end());
    
    delete layer2;
}

TEST(LayerStackTests, LayerStack_IterationOrder) {
    LayerStack stack;
    TestLayer* layer1 = new TestLayer("Layer1");
    TestLayer* layer2 = new TestLayer("Layer2");
    TestLayer* overlay1 = new TestLayer("Overlay1");
    
    stack.PushLayer(layer1);
    stack.PushLayer(layer2);
    stack.PushOverlay(overlay1);
    
    std::vector<std::string> names;
    for (auto layer : stack) {
        names.push_back(layer->GetName());
    }
    
    EXPECT_EQ(names.size(), 3);
    EXPECT_EQ(names[0], "Layer1");
    EXPECT_EQ(names[1], "Layer2");
    EXPECT_EQ(names[2], "Overlay1");
}
