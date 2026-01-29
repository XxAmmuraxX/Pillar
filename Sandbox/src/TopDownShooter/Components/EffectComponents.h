#pragma once

#include <glm/glm.hpp>

namespace Game {

    // Flash effect for damage feedback
    struct FlashComponent
    {
        glm::vec4 OriginalColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec4 FlashColor = { 1.0f, 1.0f, 1.0f, 1.0f };    // Usually white or red
        float FlashDuration = 0.1f;
        float FlashTimer = 0.0f;
        bool IsFlashing = false;

        void TriggerFlash(const glm::vec4& color = { 1.0f, 0.3f, 0.3f, 1.0f }, float duration = 0.1f)
        {
            FlashColor = color;
            FlashDuration = duration;
            FlashTimer = duration;
            IsFlashing = true;
        }

        // Returns interpolated color between flash and original
        glm::vec4 GetCurrentColor(float dt)
        {
            if (!IsFlashing) return OriginalColor;

            FlashTimer -= dt;
            if (FlashTimer <= 0.0f)
            {
                IsFlashing = false;
                return OriginalColor;
            }

            float t = FlashTimer / FlashDuration;
            return glm::mix(OriginalColor, FlashColor, t);
        }
    };

    // Temporary entity that auto-destroys after lifetime
    struct TemporaryComponent
    {
        float Lifetime = 1.0f;
        float Timer = 0.0f;

        TemporaryComponent() = default;
        explicit TemporaryComponent(float lifetime) : Lifetime(lifetime), Timer(0.0f) {}

        bool IsExpired() const { return Timer >= Lifetime; }

        void Update(float dt) { Timer += dt; }
    };

} // namespace Game
