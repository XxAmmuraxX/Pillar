#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace Pillar::Random {

// Seed the global RNG for deterministic runs.
void Seed(uint32_t seed);

// Random float in [0, 1].
float Float01();

// Random float in [min, max].
float Float(float min, float max);

// Random angle in radians within [0, 2*pi).
float AngleRadians();

// Random angle in degrees within [0, 360).
float AngleDegrees();

// Random unit-length 2D direction.
glm::vec2 Direction2D();

} // namespace Pillar::Random
