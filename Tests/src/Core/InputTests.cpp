#include <gtest/gtest.h>
// InputTests: checks Input polling API (keyboard/mouse) behavior and
// stability in a headless test environment using a mock Application.
#include "Pillar/Input.h"
#include "Pillar/KeyCodes.h"
#include "Pillar/Window.h"
#include "Pillar/Application.h"
#include <memory>
#include <cmath>

using namespace Pillar;

// ==============================
// Mock Application for Input Tests
// ==============================

class MockApplication : public Application {
public:
    MockApplication() : Application() {}
    ~MockApplication() = default;
};

// Override CreateApplication to return our mock
namespace Pillar {
    Application* CreateApplication() {
        return new MockApplication();
    }
}

// ==============================
// Input System Tests
// ==============================

class InputTest : public ::testing::Test {
protected:
    // Static application shared across all tests in this suite
    static std::unique_ptr<Application> s_Application;

    // Called once before all tests in this suite
    static void SetUpTestSuite() {
        // Create application once for all tests
        s_Application = std::unique_ptr<Application>(CreateApplication());
    }
    
    // Called once after all tests in this suite
    static void TearDownTestSuite() {
        // Clean up the shared application
        s_Application.reset();
    }
};

// Define the static member
std::unique_ptr<Application> InputTest::s_Application = nullptr;

TEST_F(InputTest, IsKeyDown_ReturnsFalseForUnpressedKey) {
    // Without simulating input, keys should be unpressed
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_W));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_A));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_S));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_D));
}

TEST_F(InputTest, IsKeyDown_WorksWithDifferentKeyCodes) {
    // Test various key code ranges
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_SPACE));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_ESCAPE));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_ENTER));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_TAB));
}

TEST_F(InputTest, IsKeyDown_WorksWithAlphanumericKeys) {
    // Test all letter keys
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_A));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_Z));

    // Test number keys
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_0));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_9));
}

TEST_F(InputTest, IsKeyDown_WorksWithFunctionKeys) {
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_F1));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_F12));
}

TEST_F(InputTest, IsKeyDown_WorksWithArrowKeys) {
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_UP));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_DOWN));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_LEFT));
    EXPECT_FALSE(Input::IsKeyDown(PIL_KEY_RIGHT));
}

TEST_F(InputTest, IsMouseButtonPressed_ReturnsFalseForUnpressedButton) {
    // Without simulating input, mouse buttons should be unpressed
    EXPECT_FALSE(Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_RIGHT));
    EXPECT_FALSE(Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_MIDDLE));
}

TEST_F(InputTest, GetMousePosition_ReturnsValidCoordinates) {
    auto [x, y] = Input::GetMousePosition();
    
    // Mouse position should be finite numbers
    EXPECT_FALSE(std::isnan(x));
    EXPECT_FALSE(std::isnan(y));
    EXPECT_FALSE(std::isinf(x));
    EXPECT_FALSE(std::isinf(y));
}

TEST_F(InputTest, GetMouseX_ReturnsValidValue) {
    float x = Input::GetMouseX();
    
    EXPECT_FALSE(std::isnan(x));
    EXPECT_FALSE(std::isinf(x));
}

TEST_F(InputTest, GetMouseY_ReturnsValidValue) {
    float y = Input::GetMouseY();
    
    EXPECT_FALSE(std::isnan(y));
    EXPECT_FALSE(std::isinf(y));
}

TEST_F(InputTest, GetMousePosition_MatchesIndividualGetters) {
    auto [posX, posY] = Input::GetMousePosition();
    float x = Input::GetMouseX();
    float y = Input::GetMouseY();
    
    // The individual getters should match the pair getter
    EXPECT_FLOAT_EQ(posX, x);
    EXPECT_FLOAT_EQ(posY, y);
}

TEST_F(InputTest, OnUpdate_ProducesFiniteDeltas) {
    // First update initializes mouse state and yields zero deltas
    EXPECT_NO_THROW(Input::OnUpdate());
    auto delta = Input::GetMouseDelta();
    auto scroll = Input::GetScrollDelta();
    EXPECT_FLOAT_EQ(delta.first, 0.0f);
    EXPECT_FLOAT_EQ(delta.second, 0.0f);
    EXPECT_FLOAT_EQ(scroll.first, 0.0f);
    EXPECT_FLOAT_EQ(scroll.second, 0.0f);
}

// ==============================
// Input Polling Consistency Tests
// ==============================

TEST_F(InputTest, IsKeyDown_ConsistentAcrossMultipleCalls) {
    // Multiple calls should return the same result
    bool firstCall = Input::IsKeyDown(PIL_KEY_W);
    bool secondCall = Input::IsKeyDown(PIL_KEY_W);
    
    EXPECT_EQ(firstCall, secondCall);
}

TEST_F(InputTest, IsMouseButtonPressed_ConsistentAcrossMultipleCalls) {
    bool firstCall = Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT);
    bool secondCall = Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT);
    
    EXPECT_EQ(firstCall, secondCall);
}

// ==============================
// Edge Case Tests
// ==============================

TEST_F(InputTest, IsKeyDown_HandlesInvalidKeyCode) {
    // Test with an invalid/out-of-range key code
    // Should not crash, just return false
    EXPECT_FALSE(Input::IsKeyDown(-1));
    EXPECT_FALSE(Input::IsKeyDown(9999));
}

TEST_F(InputTest, IsMouseButtonPressed_HandlesInvalidButton) {
    // Test with invalid mouse button codes
    EXPECT_FALSE(Input::IsMouseButtonPressed(-1));
    EXPECT_FALSE(Input::IsMouseButtonPressed(10));
}

// ==============================
// Action Binding and Cursor Mode
// ==============================

TEST_F(InputTest, ActionBinding_DefaultsToFalse) {
    Input::BindAction("Jump", { PIL_KEY_SPACE }, {});
    EXPECT_FALSE(Input::IsActionDown("Jump"));
    EXPECT_FALSE(Input::IsActionPressed("Jump"));
    EXPECT_FALSE(Input::IsActionReleased("Jump"));
    Input::UnbindAction("Jump");
}

TEST_F(InputTest, CursorMode_CanBeSetAndQueried) {
    Input::SetCursorMode(CursorMode::Normal);
    EXPECT_EQ(Input::GetCursorMode(), CursorMode::Normal);
    Input::SetCursorMode(CursorMode::Hidden);
    EXPECT_EQ(Input::GetCursorMode(), CursorMode::Hidden);
    Input::SetCursorMode(CursorMode::Locked);
    EXPECT_EQ(Input::GetCursorMode(), CursorMode::Locked);
    // Reset to normal to avoid side effects on other tests
    Input::SetCursorMode(CursorMode::Normal);
}
