# Audio System Integration Guide

## Overview

The Pillar Audio System has been fully integrated into the engine with ECS support. This document explains how to use the audio system in your games.

## Automatic Initialization

The audio system is **automatically initialized** in the `Application` constructor and shut down in the destructor. You don't need to manually call `AudioEngine::Init()` or `AudioEngine::Shutdown()` unless you're creating a custom application that doesn't inherit from `Pillar::Application`.

```cpp
// Application.cpp - Already done for you!
Application::Application()
{
    // ...
    AudioEngine::Init();  // Automatic
    // ...
}

Application::~Application()
{
    // ...
    AudioEngine::Shutdown();  // Automatic
    // ...
}
```

## Using Audio in Your Layers

### Simple Sound Effects

```cpp
class MyLayer : public Pillar::Layer
{
public:
    void OnAttach() override
    {
        // Create audio clip - simple wrapper for one-shot sounds
        m_ExplosionSound = Pillar::AudioClip::Create("explosion.wav");
    }
    
    void OnUpdate(float dt) override
    {
        if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE))
        {
            m_ExplosionSound->Play();
        }
    }

private:
    std::shared_ptr<Pillar::AudioClip> m_ExplosionSound;
};
```

### Background Music with Manual Control

```cpp
void MyLayer::OnAttach()
{
    // Load audio buffer
    auto musicBuffer = Pillar::AudioBuffer::Create("background_music.wav");
    
    // Create source and configure it
    m_MusicSource = Pillar::AudioSource::Create();
    m_MusicSource->SetBuffer(musicBuffer);
    m_MusicSource->SetLooping(true);
    m_MusicSource->SetVolume(0.5f);
    m_MusicSource->Play();
}

void MyLayer::OnDetach()
{
    if (m_MusicSource)
    {
        m_MusicSource->Stop();
    }
}
```

### 3D Positional Audio

```cpp
void MyLayer::OnUpdate(float dt)
{
    // Update listener position to match camera
    glm::vec3 cameraPos = m_CameraController.GetCamera().GetPosition();
    Pillar::AudioEngine::SetListenerPosition(cameraPos);
    
    // Update enemy sound position
    m_EnemySource->SetPosition(enemyPosition);
    m_EnemySource->SetMinDistance(5.0f);   // Start attenuation at 5 units
    m_EnemySource->SetMaxDistance(50.0f);  // Silent beyond 50 units
}
```

## ECS Integration

### Adding Audio to Entities

```cpp
// Create an entity that emits sound
auto entity = m_Scene.CreateEntity("AudioEmitter");

// Add transform for positioning
auto& transform = entity.AddComponent<Pillar::TransformComponent>();
transform.Position = glm::vec2(10.0f, 5.0f);

// Add audio source component
auto& audioComp = entity.AddComponent<Pillar::AudioSourceComponent>("enemy_growl.wav");
audioComp.Loop = true;
audioComp.Is3D = true;
audioComp.PlayOnAwake = true;
audioComp.Volume = 0.8f;
audioComp.MinDistance = 5.0f;
audioComp.MaxDistance = 50.0f;
```

### Setting Up Audio Listener

```cpp
// Usually attached to the camera entity
auto cameraEntity = m_Scene.CreateEntity("Camera");
cameraEntity.AddComponent<Pillar::TransformComponent>();
cameraEntity.AddComponent<Pillar::AudioListenerComponent>();
```

### Updating the Audio System

In your game loop or scene update:

```cpp
void MyScene::OnUpdate(float dt)
{
    // Update audio system with scene registry
    m_AudioSystem.OnUpdate(dt, m_Registry);
    
    // Or if you have a systems manager:
    m_SystemsManager.Update(dt);  // AudioSystem should be registered here
}
```

## Global Audio Controls

```cpp
// Master volume control
Pillar::AudioEngine::SetMasterVolume(0.7f);
float volume = Pillar::AudioEngine::GetMasterVolume();

// Stop all currently playing sounds
Pillar::AudioEngine::StopAllSounds();

// Pause all sounds (can be resumed later)
Pillar::AudioEngine::PauseAllSounds();

// Resume all paused sounds
Pillar::AudioEngine::ResumeAllSounds();
```

## Audio File Requirements

### Supported Formats

Currently, only **WAV files** are supported:
- 8-bit or 16-bit PCM
- Mono or Stereo
- Any sample rate (44100 Hz recommended)

### File Organization

Place audio files in:
```
Sandbox/assets/audio/
├── sfx/           # Sound effects
│   ├── explosion.wav
│   ├── jump.wav
│   └── collect.wav
├── music/         # Background music
│   ├── menu_theme.wav
│   └── level1_music.wav
└── test_sound.wav # Test files
```

### AssetManager Integration

The `AssetManager` automatically resolves audio paths:

```cpp
// These all work:
auto buffer1 = Pillar::AudioBuffer::Create("explosion.wav");
auto buffer2 = Pillar::AudioBuffer::Create("sfx/jump.wav");
auto buffer3 = Pillar::AudioBuffer::Create("music/menu_theme.wav");
```

## Common Patterns

### Play Sound on Event

```cpp
void MyLayer::OnEvent(Pillar::Event& event)
{
    Pillar::EventDispatcher dispatcher(event);
    
    dispatcher.Dispatch<Pillar::KeyPressedEvent>([this](Pillar::KeyPressedEvent& e) {
        if (e.GetKeyCode() == PIL_KEY_SPACE)
        {
            m_JumpSound->Play();
            return true;
        }
        return false;
    });
}
```

### Fade Audio Volume

```cpp
void MyLayer::FadeOut(std::shared_ptr<Pillar::AudioSource> source, float duration)
{
    m_FadeTimer = 0.0f;
    m_FadeDuration = duration;
    m_FadeSource = source;
}

void MyLayer::OnUpdate(float dt)
{
    if (m_FadeSource && m_FadeTimer < m_FadeDuration)
    {
        m_FadeTimer += dt;
        float t = m_FadeTimer / m_FadeDuration;
        m_FadeSource->SetVolume(1.0f - t);
        
        if (t >= 1.0f)
        {
            m_FadeSource->Stop();
            m_FadeSource = nullptr;
        }
    }
}
```

### Random Sound Variations

```cpp
void MyLayer::PlayRandomFootstep()
{
    int index = rand() % m_FootstepSounds.size();
    m_FootstepSounds[index]->Play();
}
```

## Debugging Audio Issues

### Check Initialization

```cpp
if (!Pillar::AudioEngine::IsInitialized())
{
    PIL_ERROR("Audio engine not initialized!");
}
```

### Verify Audio File Loading

```cpp
auto buffer = Pillar::AudioBuffer::Create("my_sound.wav");
if (!buffer || !buffer->IsLoaded())
{
    PIL_ERROR("Failed to load audio: my_sound.wav");
}
else
{
    PIL_INFO("Loaded audio: {0}s, {1} Hz, {2} channels", 
        buffer->GetDuration(),
        buffer->GetSampleRate(),
        buffer->GetChannels());
}
```

### Check Source State

```cpp
if (m_MusicSource->IsPlaying())
{
    PIL_INFO("Music is playing");
}
else if (m_MusicSource->IsPaused())
{
    PIL_INFO("Music is paused");
}
else
{
    PIL_INFO("Music is stopped");
}
```

## Performance Tips

1. **Reuse AudioBuffers**: Load once, share between multiple sources
2. **Limit Active Sources**: OpenAL typically supports 32-256 concurrent sources
3. **Use 3D Audio Wisely**: 2D sounds are cheaper (set `Is3D = false` in components)
4. **Stream Large Files**: For music > 1MB, consider streaming (future feature)
5. **Pool AudioSources**: Reuse sources for frequently played sounds

## Future Enhancements

Planned features:
- OGG Vorbis support
- MP3 support
- Audio streaming for large files
- Audio effects (reverb, echo, filters)
- Audio mixing channels
- Crossfade between tracks

## See Also

- `docs/AUDIO_SYSTEM_PLAN.md` - Full architecture documentation
- `Sandbox/src/AudioDemoLayer.h` - Complete working example
- `Tests/src/AudioTests.cpp` - Unit tests
