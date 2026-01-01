#pragma once
#include "Pillar/Core.h"
#include <initializer_list>
#include <string>
#include <utility>

namespace Pillar {

enum class CursorMode
{
    Normal = 0,
    Hidden,
    Locked
};

class PIL_API Input
{
public:
    // Update input snapshots once per frame (called by Application).
    static void OnUpdate();

    // Keyboard
    static bool IsKeyDown(int keycode);          // Held this frame
    static bool IsKeyPressed(int keycode);       // Alias for IsKeyDown (legacy)
    static bool IsKeyJustPressed(int keycode);   // Rising edge
    static bool IsKeyJustReleased(int keycode);  // Falling edge

    // Mouse buttons
    static bool IsMouseButtonDown(int button);
    static bool IsMouseButtonPressed(int button);    // Alias for IsMouseButtonDown (legacy)
    static bool IsMouseButtonJustPressed(int button);
    static bool IsMouseButtonJustReleased(int button);

    // Cursor and scroll
    static std::pair<float, float> GetMousePosition();
    static std::pair<float, float> GetMouseDelta();
    static std::pair<float, float> GetScrollDelta();
    static float GetMouseX();
    static float GetMouseY();

    static void SetCursorMode(CursorMode mode);
    static CursorMode GetCursorMode();
    static void SetMousePosition(float x, float y);

    // Action bindings (keys and mouse buttons)
    static void BindAction(const std::string& actionName, std::initializer_list<int> keys, std::initializer_list<int> mouseButtons = {});
    static void UnbindAction(const std::string& actionName);
    static bool IsActionDown(const std::string& actionName);
    static bool IsActionPressed(const std::string& actionName);
    static bool IsActionReleased(const std::string& actionName);

private:
    static void OnScroll(float xOffset, float yOffset);
    friend class WindowsWindow;
};

}