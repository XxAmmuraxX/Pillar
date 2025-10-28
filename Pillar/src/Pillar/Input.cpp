#include "Pillar/Input.h"
#include "Pillar/Application.h"
#include <GLFW/glfw3.h>

namespace Pillar {

static GLFWwindow* GetGLFWWindow()
{
    return static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
}

bool Input::IsKeyPressed(int keycode)
{
    GLFWwindow* window = GetGLFWWindow();

    int state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(int button)
{
    GLFWwindow* window = GetGLFWWindow();

    int state = glfwGetMouseButton(window, button);
    return state == GLFW_PRESS;
}

std::pair<float, float> Input::GetMousePosition()
{
    GLFWwindow* window = GetGLFWWindow();

    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return { static_cast<float>(x), static_cast<float>(y) };
}

float Input::GetMouseX()
{
    return GetMousePosition().first;
}

float Input::GetMouseY()
{
    return GetMousePosition().second;
}

}