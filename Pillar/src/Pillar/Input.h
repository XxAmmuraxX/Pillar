#pragma once
#include "Pillar/Core.h"
#include <utility>

namespace Pillar {

class PIL_API Input
{
public:
    // Keyboard
    static bool IsKeyPressed(int keycode);

    // Mouse
    static bool IsMouseButtonPressed(int button);
    static std::pair<float, float> GetMousePosition();
    static float GetMouseX();
    static float GetMouseY();
};

}