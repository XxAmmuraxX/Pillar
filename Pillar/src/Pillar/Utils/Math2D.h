#pragma once

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace Pillar::Math2D {

constexpr float kEpsilon = 1e-5f;

// Safely normalize a 2D vector; returns a fallback when the vector is too small.
inline glm::vec2 SafeNormalize(const glm::vec2& v, float epsilon = kEpsilon, const glm::vec2& fallback = glm::vec2(1.0f, 0.0f))
{
    const float lenSq = glm::dot(v, v);
    if (lenSq <= epsilon * epsilon)
        return fallback;
    return v * glm::inversesqrt(lenSq);
}

// Normalize and report whether the input was non-zero.
inline glm::vec2 NormalizeOrZero(const glm::vec2& v, float epsilon = kEpsilon)
{
    const float lenSq = glm::dot(v, v);
    if (lenSq <= epsilon * epsilon)
        return glm::vec2(0.0f);
    return v * glm::inversesqrt(lenSq);
}

// Clamp the vector's length to a maximum magnitude while preserving direction.
inline glm::vec2 ClampLength(const glm::vec2& v, float maxLength, float epsilon = kEpsilon)
{
    const float lenSq = glm::dot(v, v);
    const float maxLenSq = maxLength * maxLength;
    if (lenSq <= maxLenSq)
        return v;
    if (lenSq <= epsilon * epsilon)
        return glm::vec2(0.0f);
    const float scale = maxLength * glm::inversesqrt(lenSq);
    return v * scale;
}

// Clamp length between min and max.
inline glm::vec2 ClampLengthRange(const glm::vec2& v, float minLength, float maxLength, float epsilon = kEpsilon)
{
    const float lenSq = glm::dot(v, v);
    if (lenSq <= epsilon * epsilon)
        return glm::vec2(0.0f);
    const float len = std::sqrt(lenSq);
    const float clamped = std::clamp(len, minLength, maxLength);
    if (std::fabs(len) <= epsilon)
        return glm::vec2(0.0f);
    return v * (clamped / len);
}

// Linear interpolation between two 2D vectors.
inline glm::vec2 Lerp(const glm::vec2& a, const glm::vec2& b, float t)
{
    return a + (b - a) * t;
}

// Move a toward b by maxDelta, without overshooting.
inline glm::vec2 MoveTowards(const glm::vec2& a, const glm::vec2& b, float maxDelta)
{
    const glm::vec2 delta = b - a;
    const float distSq = glm::dot(delta, delta);
    const float maxDeltaSq = maxDelta * maxDelta;
    if (distSq <= maxDeltaSq)
        return b;
    const float dist = std::sqrt(distSq);
    return a + delta * (maxDelta / dist);
}

// Distance helpers.
inline float Distance(const glm::vec2& a, const glm::vec2& b)
{
    return std::sqrt(glm::dot(b - a, b - a));
}

inline float DistanceSq(const glm::vec2& a, const glm::vec2& b)
{
    return glm::dot(b - a, b - a);
}

// Perpendicular (left-hand) vector.
inline glm::vec2 PerpLeft(const glm::vec2& v)
{
    return glm::vec2(-v.y, v.x);
}

// Perpendicular (right-hand) vector.
inline glm::vec2 PerpRight(const glm::vec2& v)
{
    return glm::vec2(v.y, -v.x);
}

// Rotate a vector by radians around origin.
inline glm::vec2 Rotate(const glm::vec2& v, float radians)
{
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

// Project vector a onto vector b (returns zero when b is too small).
inline glm::vec2 Project(const glm::vec2& a, const glm::vec2& b, float epsilon = kEpsilon)
{
    const float denom = glm::dot(b, b);
    if (denom <= epsilon * epsilon)
        return glm::vec2(0.0f);
    return b * (glm::dot(a, b) / denom);
}

// Reflect incident across normal (normal assumed normalized, but guarded).
inline glm::vec2 Reflect(const glm::vec2& incident, const glm::vec2& normal, float epsilon = kEpsilon)
{
    const glm::vec2 n = SafeNormalize(normal, epsilon, glm::vec2(0.0f, 1.0f));
    return incident - 2.0f * glm::dot(incident, n) * n;
}

// Signed angle from a to b (radians, [-pi, pi]).
inline float SignedAngle(const glm::vec2& a, const glm::vec2& b, float epsilon = kEpsilon)
{
    const glm::vec2 na = SafeNormalize(a, epsilon, glm::vec2(1.0f, 0.0f));
    const glm::vec2 nb = SafeNormalize(b, epsilon, glm::vec2(1.0f, 0.0f));
    const float dot = std::clamp(glm::dot(na, nb), -1.0f, 1.0f);
    const float det = na.x * nb.y - na.y * nb.x; // 2D cross (z-component)
    return std::atan2(det, dot);
}

// Unsigned angle between vectors (radians, [0, pi]).
inline float AngleBetween(const glm::vec2& a, const glm::vec2& b, float epsilon = kEpsilon)
{
    const glm::vec2 na = SafeNormalize(a, epsilon, glm::vec2(1.0f, 0.0f));
    const glm::vec2 nb = SafeNormalize(b, epsilon, glm::vec2(1.0f, 0.0f));
    const float dot = std::clamp(glm::dot(na, nb), -1.0f, 1.0f);
    return std::acos(dot);
}

// Clamp each component independently.
inline glm::vec2 Clamp(const glm::vec2& v, const glm::vec2& minV, const glm::vec2& maxV)
{
    return glm::vec2(std::clamp(v.x, minV.x, maxV.x), std::clamp(v.y, minV.y, maxV.y));
}

// Component-wise multiply.
inline glm::vec2 Mul(const glm::vec2& a, const glm::vec2& b)
{
    return glm::vec2(a.x * b.x, a.y * b.y);
}

// Component-wise divide with zero guard.
inline glm::vec2 DivSafe(const glm::vec2& a, const glm::vec2& b, float epsilon = kEpsilon)
{
    return glm::vec2(
        (std::fabs(b.x) <= epsilon) ? 0.0f : a.x / b.x,
        (std::fabs(b.y) <= epsilon) ? 0.0f : a.y / b.y);
}

} // namespace Pillar::Math2D
