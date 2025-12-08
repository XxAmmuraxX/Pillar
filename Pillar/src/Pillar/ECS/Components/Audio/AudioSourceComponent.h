#pragma once

#include "Pillar/Audio/AudioSource.h"
#include <memory>
#include <string>

namespace Pillar {

    /**
     * @brief Component for attaching audio sources to entities
     * 
     * Allows entities to emit sounds in 2D or 3D space.
     * The AudioSystem will automatically update the source position
     * based on the entity's TransformComponent.
     */
    struct AudioSourceComponent
    {
        std::shared_ptr<AudioSource> Source;
        std::string AudioFile;          // Path to audio file
        float Volume = 1.0f;
        float Pitch = 1.0f;
        bool Loop = false;
        bool PlayOnAwake = false;
        bool Is3D = true;               // Use 3D spatial audio
        float MinDistance = 1.0f;       // Distance at which attenuation starts
        float MaxDistance = 100.0f;     // Distance at which sound is silent
        float RolloffFactor = 1.0f;     // How quickly sound fades
        
        AudioSourceComponent() = default;
        AudioSourceComponent(const std::string& file) 
            : AudioFile(file) {}
        
        AudioSourceComponent(const AudioSourceComponent& other)
            : AudioFile(other.AudioFile),
              Volume(other.Volume),
              Pitch(other.Pitch),
              Loop(other.Loop),
              PlayOnAwake(other.PlayOnAwake),
              Is3D(other.Is3D),
              MinDistance(other.MinDistance),
              MaxDistance(other.MaxDistance),
              RolloffFactor(other.RolloffFactor)
        {
            // Note: Source is not copied, will be recreated by AudioSystem
        }
        
        AudioSourceComponent& operator=(const AudioSourceComponent& other)
        {
            if (this != &other)
            {
                AudioFile = other.AudioFile;
                Volume = other.Volume;
                Pitch = other.Pitch;
                Loop = other.Loop;
                PlayOnAwake = other.PlayOnAwake;
                Is3D = other.Is3D;
                MinDistance = other.MinDistance;
                MaxDistance = other.MaxDistance;
                RolloffFactor = other.RolloffFactor;
                // Note: Source is not copied
            }
            return *this;
        }
    };

} // namespace Pillar
