#pragma once

namespace Game {

    struct PlayerTagComponent
    {
        float MoveSpeed = 5.0f;
        float DashSpeed = 15.0f;
        float DashDuration = 0.15f;
        float DashCooldown = 1.0f;

        bool IsDashing = false;
        float DashTimer = 0.0f;
        float DashCooldownTimer = 0.0f;
    };

} // namespace Game
