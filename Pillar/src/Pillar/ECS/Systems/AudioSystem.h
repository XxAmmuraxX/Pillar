#pragma once

#include "System.h"
#include "Pillar/Audio/AudioEngine.h"
#include <entt/entt.hpp>

namespace Pillar {

    /**
     * @brief System for managing audio sources and listener in ECS
     * 
     * Responsibilities:
     * - Initialize audio sources from AudioSourceComponent
     * - Update source positions based on TransformComponent
     * - Update listener position/orientation from AudioListenerComponent
     * - Handle PlayOnAwake functionality
     * - Apply volume, pitch, looping settings
     */
    class AudioSystem : public System
    {
    public:
        void OnUpdate(float dt) override;
        
    private:
        void UpdateListener(entt::registry& registry);
        void UpdateSources(entt::registry& registry);
        void InitializeSource(entt::registry& registry, entt::entity entity);
    };

} // namespace Pillar
