#pragma once

#include <glm/glm.hpp>
#include <Pillar/Renderer/OrthographicCamera.h>
#include <random>
#include <cmath>

namespace Game {

    // Random number generation
    static std::random_device s_RandomDevice;
    static std::mt19937 s_RandomEngine(s_RandomDevice());

    // Random float in range [min, max]
    inline float RandomFloat(float min, float max)
    {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(s_RandomEngine);
    }

    // Random vec2 in circle (uniform distribution)
    inline glm::vec2 RandomInCircle(float radius)
    {
        float angle = RandomFloat(0.0f, 2.0f * glm::pi<float>());
        float r = std::sqrt(RandomFloat(0.0f, 1.0f)) * radius;
        return glm::vec2(r * std::cos(angle), r * std::sin(angle));
    }

    // Random vec2 on circle edge
    inline glm::vec2 RandomOnCircle(float radius)
    {
        float angle = RandomFloat(0.0f, 2.0f * glm::pi<float>());
        return glm::vec2(radius * std::cos(angle), radius * std::sin(angle));
    }

    // Get normalized direction from angle (radians)
    inline glm::vec2 AngleToDirection(float radians)
    {
        return glm::vec2(std::cos(radians), std::sin(radians));
    }

    // Get angle from direction vector
    inline float DirectionToAngle(const glm::vec2& dir)
    {
        return std::atan2(dir.y, dir.x);
    }

    // Convert screen coordinates to world coordinates
    inline glm::vec2 ScreenToWorld(
        float screenX, float screenY,
        float windowWidth, float windowHeight,
        const Pillar::OrthographicCamera& camera)
    {
        // Normalize to [-1, 1]
        float ndcX = (2.0f * screenX / windowWidth) - 1.0f;
        float ndcY = 1.0f - (2.0f * screenY / windowHeight);  // Flip Y

        // Get inverse view-projection matrix
        glm::mat4 invVP = glm::inverse(camera.GetViewProjectionMatrix());

        // Transform to world space
        glm::vec4 worldPos = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
        return glm::vec2(worldPos.x, worldPos.y);
    }

} // namespace Game
