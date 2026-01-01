#include "Random.h"

#include <random>
#include <chrono>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace Pillar::Random {
namespace {
    std::mt19937& Engine()
    {
        static std::mt19937 engine(static_cast<uint32_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        return engine;
    }

    std::uniform_real_distribution<float>& UnitDist()
    {
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist;
    }
}

void Seed(uint32_t seed)
{
    Engine().seed(seed);
}

float Float01()
{
    return UnitDist()(Engine());
}

float Float(float min, float max)
{
    std::uniform_real_distribution<float> dist(min, max);
    return dist(Engine());
}

float AngleRadians()
{
    return Float(0.0f, glm::two_pi<float>());
}

float AngleDegrees()
{
    return Float(0.0f, 360.0f);
}

glm::vec2 Direction2D()
{
    const float angle = AngleRadians();
    return glm::vec2(std::cos(angle), std::sin(angle));
}

} // namespace Pillar::Random
