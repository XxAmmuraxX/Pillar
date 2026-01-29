#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Game {

    /**
     * BulletTrailComponent - Creates a visual trail behind bullets
     * 
     * Stores recent positions to render a fading trail effect.
     */
    struct BulletTrailComponent
    {
        // Trail configuration
        int MaxTrailPoints = 8;
        float TrailWidth = 0.08f;
        float TrailFadeStart = 0.3f;  // Alpha at head of trail
        float SpawnInterval = 0.01f;   // Seconds between trail points
        
        // Trail color (typically matches bullet color)
        glm::vec4 TrailColor = { 1.0f, 0.9f, 0.5f, 0.8f };
        
        // Trail state
        std::vector<glm::vec2> TrailPoints;
        float SpawnTimer = 0.0f;
        
        BulletTrailComponent() = default;
        
        explicit BulletTrailComponent(const glm::vec4& color, int maxPoints = 8)
            : MaxTrailPoints(maxPoints)
            , TrailColor(color)
        {
            TrailPoints.reserve(maxPoints);
        }
        
        void AddPoint(const glm::vec2& position)
        {
            TrailPoints.insert(TrailPoints.begin(), position);
            if (static_cast<int>(TrailPoints.size()) > MaxTrailPoints)
                TrailPoints.pop_back();
        }
        
        void Update(float dt, const glm::vec2& currentPos)
        {
            SpawnTimer += dt;
            if (SpawnTimer >= SpawnInterval)
            {
                AddPoint(currentPos);
                SpawnTimer = 0.0f;
            }
        }
        
        void Clear()
        {
            TrailPoints.clear();
            SpawnTimer = 0.0f;
        }
    };

} // namespace Game
