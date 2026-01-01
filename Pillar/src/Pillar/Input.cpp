#include "Pillar/Input.h"
#include "Pillar/Application.h"
#include <GLFW/glfw3.h>
#include <array>
#include <unordered_map>
#include <vector>

namespace Pillar {

namespace
{
    constexpr int MaxKeys = GLFW_KEY_LAST + 1;
    constexpr int MaxMouseButtons = GLFW_MOUSE_BUTTON_LAST + 1;

    struct ActionBinding
    {
        std::vector<int> Keys;
        std::vector<int> MouseButtons;
    };

    std::array<bool, MaxKeys> s_KeyCurrent{};
    std::array<bool, MaxKeys> s_KeyPrevious{};
    std::array<bool, MaxMouseButtons> s_MouseCurrent{};
    std::array<bool, MaxMouseButtons> s_MousePrevious{};

    std::pair<float, float> s_MousePosition{ 0.0f, 0.0f };
    std::pair<float, float> s_LastMousePosition{ 0.0f, 0.0f };
    std::pair<float, float> s_MouseDelta{ 0.0f, 0.0f };
    std::pair<float, float> s_ScrollDelta{ 0.0f, 0.0f };
    std::pair<float, float> s_PendingScrollDelta{ 0.0f, 0.0f };
    bool s_MouseInitialized = false;

    CursorMode s_CursorMode = CursorMode::Normal;
    std::unordered_map<std::string, ActionBinding> s_ActionBindings;

    GLFWwindow* GetGLFWWindow()
    {
        return static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    }

    bool IsValidKey(int keycode)
    {
        return keycode >= 0 && keycode < MaxKeys;
    }

    bool IsValidMouseButton(int button)
    {
        return button >= 0 && button < MaxMouseButtons;
    }
}

void Input::OnUpdate()
{
    GLFWwindow* window = GetGLFWWindow();

    s_KeyPrevious = s_KeyCurrent;
    s_MousePrevious = s_MouseCurrent;

    s_KeyCurrent.fill(false);
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
    {
        int state = glfwGetKey(window, key);
        s_KeyCurrent[key] = state == GLFW_PRESS || state == GLFW_REPEAT;
    }

    for (int button = 0; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
    {
        int state = glfwGetMouseButton(window, button);
        s_MouseCurrent[button] = state == GLFW_PRESS;
    }

    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    std::pair<float, float> currentPos{ static_cast<float>(x), static_cast<float>(y) };
    if (!s_MouseInitialized)
    {
        s_MouseInitialized = true;
        s_LastMousePosition = currentPos;
    }
    s_MouseDelta = { currentPos.first - s_LastMousePosition.first, currentPos.second - s_LastMousePosition.second };
    s_LastMousePosition = currentPos;
    s_MousePosition = currentPos;

    s_ScrollDelta = s_PendingScrollDelta;
    s_PendingScrollDelta = { 0.0f, 0.0f };
}

bool Input::IsKeyDown(int keycode)
{
    return IsValidKey(keycode) ? s_KeyCurrent[keycode] : false;
}

bool Input::IsKeyPressed(int keycode)
{
    return IsKeyDown(keycode);
}

bool Input::IsKeyJustPressed(int keycode)
{
    return IsValidKey(keycode) ? (s_KeyCurrent[keycode] && !s_KeyPrevious[keycode]) : false;
}

bool Input::IsKeyJustReleased(int keycode)
{
    return IsValidKey(keycode) ? (!s_KeyCurrent[keycode] && s_KeyPrevious[keycode]) : false;
}

bool Input::IsMouseButtonDown(int button)
{
    return IsValidMouseButton(button) ? s_MouseCurrent[button] : false;
}

bool Input::IsMouseButtonPressed(int button)
{
    return IsMouseButtonDown(button);
}

bool Input::IsMouseButtonJustPressed(int button)
{
    return IsValidMouseButton(button) ? (s_MouseCurrent[button] && !s_MousePrevious[button]) : false;
}

bool Input::IsMouseButtonJustReleased(int button)
{
    return IsValidMouseButton(button) ? (!s_MouseCurrent[button] && s_MousePrevious[button]) : false;
}

std::pair<float, float> Input::GetMousePosition()
{
    return s_MousePosition;
}

std::pair<float, float> Input::GetMouseDelta()
{
    return s_MouseDelta;
}

std::pair<float, float> Input::GetScrollDelta()
{
    return s_ScrollDelta;
}

float Input::GetMouseX()
{
    return s_MousePosition.first;
}

float Input::GetMouseY()
{
    return s_MousePosition.second;
}

void Input::SetCursorMode(CursorMode mode)
{
    GLFWwindow* window = GetGLFWWindow();
    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode)
    {
    case CursorMode::Hidden:
        glfwMode = GLFW_CURSOR_HIDDEN;
        break;
    case CursorMode::Locked:
        glfwMode = GLFW_CURSOR_DISABLED;
        break;
    default:
        glfwMode = GLFW_CURSOR_NORMAL;
        break;
    }
    glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
    s_CursorMode = mode;
}

CursorMode Input::GetCursorMode()
{
    return s_CursorMode;
}

void Input::SetMousePosition(float x, float y)
{
    GLFWwindow* window = GetGLFWWindow();
    glfwSetCursorPos(window, x, y);
    s_MousePosition = { x, y };
    s_LastMousePosition = s_MousePosition;
    s_MouseDelta = { 0.0f, 0.0f };
    s_MouseInitialized = true;
}

void Input::BindAction(const std::string& actionName, std::initializer_list<int> keys, std::initializer_list<int> mouseButtons)
{
    ActionBinding binding;
    binding.Keys.assign(keys);
    binding.MouseButtons.assign(mouseButtons);
    s_ActionBindings[actionName] = std::move(binding);
}

void Input::UnbindAction(const std::string& actionName)
{
    s_ActionBindings.erase(actionName);
}

bool Input::IsActionDown(const std::string& actionName)
{
    auto it = s_ActionBindings.find(actionName);
    if (it == s_ActionBindings.end())
        return false;
    for (int key : it->second.Keys)
    {
        if (IsKeyDown(key))
            return true;
    }
    for (int button : it->second.MouseButtons)
    {
        if (IsMouseButtonDown(button))
            return true;
    }
    return false;
}

bool Input::IsActionPressed(const std::string& actionName)
{
    auto it = s_ActionBindings.find(actionName);
    if (it == s_ActionBindings.end())
        return false;
    for (int key : it->second.Keys)
    {
        if (IsKeyJustPressed(key))
            return true;
    }
    for (int button : it->second.MouseButtons)
    {
        if (IsMouseButtonJustPressed(button))
            return true;
    }
    return false;
}

bool Input::IsActionReleased(const std::string& actionName)
{
    auto it = s_ActionBindings.find(actionName);
    if (it == s_ActionBindings.end())
        return false;
    for (int key : it->second.Keys)
    {
        if (IsKeyJustReleased(key))
            return true;
    }
    for (int button : it->second.MouseButtons)
    {
        if (IsMouseButtonJustReleased(button))
            return true;
    }
    return false;
}

void Input::OnScroll(float xOffset, float yOffset)
{
    s_PendingScrollDelta.first += xOffset;
    s_PendingScrollDelta.second += yOffset;
}

}