#pragma once

#include <cstdint>

namespace Game {

    namespace CollisionCategory {
        constexpr uint16_t None     = 0x0000;
        constexpr uint16_t Player   = 0x0001;
        constexpr uint16_t Enemy    = 0x0002;
        constexpr uint16_t Bullet   = 0x0004;
        constexpr uint16_t Wall     = 0x0008;
        constexpr uint16_t PowerUp  = 0x0010;
        constexpr uint16_t Trigger  = 0x0020;
        constexpr uint16_t All      = 0xFFFF;
    }

} // namespace Game
