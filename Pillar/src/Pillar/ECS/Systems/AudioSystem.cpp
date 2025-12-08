#include "AudioSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
#include "Pillar/ECS/Components/Audio/AudioListenerComponent.h"
#include "Pillar/Logger.h"

namespace Pillar {

    void AudioSystem::OnUpdate(float dt)
    {
        if (m_Scene)
        {
            entt::registry& registry = m_Scene->GetRegistry();
            UpdateListener(registry);
            UpdateSources(registry);
        }
    }
    
    void AudioSystem::UpdateListener(entt::registry& registry)
    {
        auto view = registry.view<AudioListenerComponent, TransformComponent>();
        
        // Find the first active listener
        for (auto entity : view)
        {
            auto& listener = view.get<AudioListenerComponent>(entity);
            
            if (listener.IsActive)
            {
                auto& transform = view.get<TransformComponent>(entity);
                
                // Update listener position
                AudioEngine::SetListenerPosition(glm::vec3(transform.Position, 0.0f));
                
                // Update listener orientation
                AudioEngine::SetListenerOrientation(listener.Forward, listener.Up);
                
                // Only use the first active listener
                break;
            }
        }
    }
    
    void AudioSystem::UpdateSources(entt::registry& registry)
    {
        auto view = registry.view<AudioSourceComponent>();
        
        for (auto entity : view)
        {
            auto& audioComp = view.get<AudioSourceComponent>(entity);
            
            // Initialize source if needed
            if (!audioComp.Source)
            {
                InitializeSource(registry, entity);
            }
            
            if (audioComp.Source)
            {
                // Update 3D position if entity has transform
                if (audioComp.Is3D && registry.all_of<TransformComponent>(entity))
                {
                    auto& transform = registry.get<TransformComponent>(entity);
                    audioComp.Source->SetPosition(glm::vec3(transform.Position, 0.0f));
                }
                
                // Ensure volume, pitch, and looping are up to date
                audioComp.Source->SetVolume(audioComp.Volume);
                audioComp.Source->SetPitch(audioComp.Pitch);
                audioComp.Source->SetLooping(audioComp.Loop);
                
                // Set 3D audio properties
                if (audioComp.Is3D)
                {
                    audioComp.Source->SetMinDistance(audioComp.MinDistance);
                    audioComp.Source->SetMaxDistance(audioComp.MaxDistance);
                    audioComp.Source->SetRolloffFactor(audioComp.RolloffFactor);
                }
            }
        }
    }
    
    void AudioSystem::InitializeSource(entt::registry& registry, entt::entity entity)
    {
        auto& audioComp = registry.get<AudioSourceComponent>(entity);
        
        // Create audio source
        audioComp.Source = AudioSource::Create();
        
        if (!audioComp.Source)
        {
            PIL_CORE_ERROR("AudioSystem: Failed to create audio source for entity");
            return;
        }
        
        // Load audio buffer if file path is specified
        if (!audioComp.AudioFile.empty())
        {
            auto buffer = AudioBuffer::Create(audioComp.AudioFile);
            if (buffer && buffer->IsLoaded())
            {
                audioComp.Source->SetBuffer(buffer);
                PIL_CORE_TRACE("AudioSystem: Loaded audio '{0}' for entity", audioComp.AudioFile);
            }
            else
            {
                PIL_CORE_WARN("AudioSystem: Failed to load audio file '{0}'", audioComp.AudioFile);
            }
        }
        
        // Apply initial settings
        audioComp.Source->SetVolume(audioComp.Volume);
        audioComp.Source->SetPitch(audioComp.Pitch);
        audioComp.Source->SetLooping(audioComp.Loop);
        
        // Set 3D audio properties
        if (audioComp.Is3D)
        {
            audioComp.Source->SetMinDistance(audioComp.MinDistance);
            audioComp.Source->SetMaxDistance(audioComp.MaxDistance);
            audioComp.Source->SetRolloffFactor(audioComp.RolloffFactor);
            
            // Set initial position if entity has transform
            if (registry.all_of<TransformComponent>(entity))
            {
                auto& transform = registry.get<TransformComponent>(entity);
                audioComp.Source->SetPosition(glm::vec3(transform.Position, 0.0f));
            }
        }
        
        // Play on awake if specified
        if (audioComp.PlayOnAwake)
        {
            audioComp.Source->Play();
            PIL_CORE_TRACE("AudioSystem: Playing audio on awake for entity");
        }
    }

} // namespace Pillar
