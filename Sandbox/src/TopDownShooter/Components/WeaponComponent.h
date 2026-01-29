#pragma once

#include <string>

namespace Game {

    struct WeaponComponent
    {
        std::string WeaponName = "Pistol";
        float Damage = 10.0f;
        float FireRate = 5.0f;          // Shots per second
        float BulletSpeed = 15.0f;      // Units per second
        float Spread = 2.0f;            // Degrees of random spread
        int BulletsPerShot = 1;         // For shotguns

        float FireCooldown = 0.0f;      // Countdown to next shot

        bool CanFire() const { return FireCooldown <= 0.0f; }

        void ResetCooldown() { FireCooldown = 1.0f / FireRate; }

        void UpdateCooldown(float dt)
        {
            if (FireCooldown > 0.0f)
                FireCooldown -= dt;
        }
    };

} // namespace Game
