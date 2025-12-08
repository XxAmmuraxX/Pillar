# Pillar Engine Audio System Plan

## Overview

This document outlines the comprehensive plan for implementing an audio system in the Pillar Engine using **OpenAL-Soft** as the backend. The audio system will follow the same architectural patterns established in the engine (factory pattern, abstraction layers, static API, event integration).

**Author:** Audio System Design Document  
**Target:** Pillar Engine v0.x  
**Backend:** OpenAL-Soft 1.24.3  
**Status:** Planning Phase

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Directory Structure](#2-directory-structure)
3. [Core Components](#3-core-components)
4. [Platform Implementation (OpenAL)](#4-platform-implementation-openal)
5. [API Design](#5-api-design)
6. [Integration Points](#6-integration-points)
7. [Asset Management](#7-asset-management)
8. [ECS Integration](#8-ecs-integration)
9. [Implementation Phases](#9-implementation-phases)
10. [Testing Strategy](#10-testing-strategy)
11. [CMake Configuration](#11-cmake-configuration)
12. [Usage Examples](#12-usage-examples)
13. [Future Enhancements](#13-future-enhancements)

---

## 1. Architecture Overview

### Design Principles

Following the established Pillar Engine patterns:

1. **Abstraction Layer**: Platform-agnostic audio interfaces with OpenAL implementation
2. **Factory Pattern**: `AudioBuffer::Create()`, `AudioSource::Create()` for platform-specific creation
3. **Static API**: `AudioEngine` class provides global audio control (similar to `Renderer`)
4. **Resource Management**: Shared pointers for audio resources, automatic cleanup
5. **Event Integration**: Audio events for playback state changes (optional)
6. **ECS Support**: `AudioSourceComponent` for entity-based audio

### High-Level Architecture

```
???????????????????????????????????????????????????????????????????
?                        Application Layer                         ?
?   ExampleLayer / Game Code                                       ?
???????????????????????????????????????????????????????????????????
                              ?
                              ?
???????????????????????????????????????????????????????????????????
?                     Pillar Audio API                             ?
?   AudioEngine (static) ? AudioBuffer ? AudioSource ? AudioClip  ?
???????????????????????????????????????????????????????????????????
                              ?
                              ?
???????????????????????????????????????????????????????????????????
?                   Platform Abstraction                           ?
?   OpenALContext ? OpenALBuffer ? OpenALSource                   ?
???????????????????????????????????????????????????????????????????
                              ?
                              ?
???????????????????????????????????????????????????????????????????
?                      OpenAL-Soft 1.24.3                          ?
???????????????????????????????????????????????????????????????????
```

---

## 2. Directory Structure

```
Pillar/
??? src/
?   ??? Pillar/
?   ?   ??? Audio/                           # Abstract audio interfaces
?   ?   ?   ??? AudioEngine.h                # Static audio API (EXISTS - needs impl)
?   ?   ?   ??? AudioEngine.cpp              # NEW
?   ?   ?   ??? AudioBuffer.h                # Abstract buffer (EXISTS - needs update)
?   ?   ?   ??? AudioBuffer.cpp              # NEW - Factory implementation
?   ?   ?   ??? AudioSource.h                # NEW - Abstract audio source
?   ?   ?   ??? AudioSource.cpp              # NEW - Factory implementation
?   ?   ?   ??? AudioClip.h                  # NEW - High-level audio clip wrapper
?   ?   ?   ??? AudioClip.cpp                # NEW
?   ?   ?   ??? AudioListener.h              # NEW - 3D audio listener
?   ?   ?   ??? AudioListener.cpp            # NEW
?   ?   ?   ??? WavLoader.h                  # NEW - WAV file parsing utility
?   ?   ?
?   ?   ??? ECS/
?   ?   ?   ??? Components/
?   ?   ?       ??? Audio/                   # NEW - Audio components
?   ?   ?           ??? AudioSourceComponent.h
?   ?   ?           ??? AudioListenerComponent.h
?   ?   ?
?   ?   ??? Events/
?   ?       ??? AudioEvent.h                 # NEW - Audio-specific events (optional)
?   ?
?   ??? Platform/
?       ??? OpenAL/                          # NEW - OpenAL implementations
?           ??? OpenALContext.h
?           ??? OpenALContext.cpp
?           ??? OpenALBuffer.h
?           ??? OpenALBuffer.cpp
?           ??? OpenALSource.h
?           ??? OpenALSource.cpp

Sandbox/
??? assets/
?   ??? audio/                               # NEW - Audio assets folder
?       ??? sfx/                             # Sound effects
?       ??? music/                           # Background music

Tests/
??? src/
    ??? AudioTests.cpp                       # NEW - Audio system tests
```

---

## 3. Core Components

### 3.1 AudioEngine (Static API)

**File:** `Pillar/src/Pillar/Audio/AudioEngine.h` (EXISTS - needs implementation)

The `AudioEngine` is the main entry point for audio functionality, providing:
- Initialization and shutdown
- Factory methods for creating audio resources
- Global audio controls (master volume)
- 3D listener management

```cpp
class PIL_API AudioEngine
{
public:
    // Lifecycle
    static void Init();
    static void Shutdown();
    static bool IsInitialized();
    
    // Factory methods
    static std::shared_ptr<AudioBuffer> CreateBuffer(const std::string& filepath);
    static std::shared_ptr<AudioSource> CreateSource();
    
    // Global controls
    static void SetMasterVolume(float volume);
    static float GetMasterVolume();
    
    // 3D Audio Listener
    static void SetListenerPosition(const glm::vec3& position);
    static void SetListenerVelocity(const glm::vec3& velocity);
    static void SetListenerOrientation(const glm::vec3& forward, const glm::vec3& up);
    static glm::vec3 GetListenerPosition();
    
    // Utility
    static void StopAllSounds();
    static void PauseAllSounds();
    static void ResumeAllSounds();
    
private:
    static bool s_Initialized;
    static float s_MasterVolume;
};
```

### 3.2 AudioBuffer (Abstract Interface)

**File:** `Pillar/src/Pillar/Audio/AudioBuffer.h` (EXISTS - needs update)

Holds decoded audio data. Can be shared between multiple sources.

```cpp
class PIL_API AudioBuffer
{
public:
    virtual ~AudioBuffer() = default;
    
    // Properties
    virtual uint32_t GetBufferID() const = 0;
    virtual float GetDuration() const = 0;
    virtual int GetSampleRate() const = 0;
    virtual int GetChannels() const = 0;
    virtual int GetBitsPerSample() const = 0;
    virtual bool IsLoaded() const = 0;
    virtual const std::string& GetFilePath() const = 0;
    
    // Factory method
    static std::shared_ptr<AudioBuffer> Create(const std::string& filepath);
};
```

### 3.3 AudioSource (NEW - Abstract Interface)

**File:** `Pillar/src/Pillar/Audio/AudioSource.h`

Controls playback of an audio buffer. Each source can play one buffer at a time.

```cpp
enum class AudioState
{
    Stopped,
    Playing,
    Paused
};

class PIL_API AudioSource
{
public:
    virtual ~AudioSource() = default;
    
    // Buffer management
    virtual void SetBuffer(const std::shared_ptr<AudioBuffer>& buffer) = 0;
    virtual std::shared_ptr<AudioBuffer> GetBuffer() const = 0;
    
    // Playback control
    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual void Rewind() = 0;
    
    // State
    virtual AudioState GetState() const = 0;
    virtual bool IsPlaying() const = 0;
    virtual bool IsPaused() const = 0;
    virtual bool IsStopped() const = 0;
    
    // Properties
    virtual void SetVolume(float volume) = 0;      // 0.0 to 1.0
    virtual float GetVolume() const = 0;
    virtual void SetPitch(float pitch) = 0;        // 0.5 to 2.0 typically
    virtual float GetPitch() const = 0;
    virtual void SetLooping(bool loop) = 0;
    virtual bool IsLooping() const = 0;
    
    // 3D Audio
    virtual void SetPosition(const glm::vec3& position) = 0;
    virtual glm::vec3 GetPosition() const = 0;
    virtual void SetVelocity(const glm::vec3& velocity) = 0;
    virtual void SetDirection(const glm::vec3& direction) = 0;
    
    // Attenuation (3D falloff)
    virtual void SetMinDistance(float distance) = 0;  // Distance at which attenuation starts
    virtual void SetMaxDistance(float distance) = 0;  // Distance at which sound is silent
    virtual void SetRolloffFactor(float factor) = 0;  // How quickly sound fades
    
    // Playback position
    virtual void SetPlaybackPosition(float seconds) = 0;
    virtual float GetPlaybackPosition() const = 0;
    
    // Internal
    virtual uint32_t GetSourceID() const = 0;
    
    // Factory method
    static std::shared_ptr<AudioSource> Create();
};
```

### 3.4 AudioClip (NEW - High-Level Wrapper)

**File:** `Pillar/src/Pillar/Audio/AudioClip.h`

A convenience wrapper combining a buffer and source for simple one-shot playback.

```cpp
class PIL_API AudioClip
{
public:
    AudioClip(const std::string& filepath);
    ~AudioClip();
    
    // Simple playback
    void Play();
    void Stop();
    void Pause();
    void Resume();
    
    // Properties
    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetLooping(bool loop);
    void SetPosition(const glm::vec3& position);
    
    // State
    bool IsPlaying() const;
    float GetDuration() const;
    
    // Access underlying objects
    std::shared_ptr<AudioBuffer> GetBuffer() const { return m_Buffer; }
    std::shared_ptr<AudioSource> GetSource() const { return m_Source; }
    
    // Factory
    static std::shared_ptr<AudioClip> Create(const std::string& filepath);
    
private:
    std::shared_ptr<AudioBuffer> m_Buffer;
    std::shared_ptr<AudioSource> m_Source;
};
```

### 3.5 WavLoader (NEW - Utility)

**File:** `Pillar/src/Pillar/Audio/WavLoader.h`

Parses WAV file format and extracts audio data.

```cpp
struct WavData
{
    std::vector<char> Data;
    int SampleRate;
    int Channels;
    int BitsPerSample;
    float Duration;
};

class WavLoader
{
public:
    static bool Load(const std::string& filepath, WavData& outData);
    static bool LoadFromMemory(const char* data, size_t size, WavData& outData);
    
private:
    static bool ParseHeader(const char* data, size_t size, WavData& outData);
};
```

---

## 4. Platform Implementation (OpenAL)

### 4.1 OpenALContext

**File:** `Pillar/src/Platform/OpenAL/OpenALContext.h`

Manages OpenAL device and context initialization.

```cpp
class OpenALContext
{
public:
    static bool Init();
    static void Shutdown();
    static bool IsInitialized();
    
    static ALCdevice* GetDevice();
    static ALCcontext* GetContext();
    
private:
    static ALCdevice* s_Device;
    static ALCcontext* s_Context;
    static bool s_Initialized;
};
```

### 4.2 OpenALBuffer

**File:** `Pillar/src/Platform/OpenAL/OpenALBuffer.h`

OpenAL implementation of AudioBuffer.

```cpp
class OpenALBuffer : public AudioBuffer
{
public:
    OpenALBuffer(const std::string& filepath);
    ~OpenALBuffer();
    
    uint32_t GetBufferID() const override { return m_BufferID; }
    float GetDuration() const override { return m_Duration; }
    int GetSampleRate() const override { return m_SampleRate; }
    int GetChannels() const override { return m_Channels; }
    int GetBitsPerSample() const override { return m_BitsPerSample; }
    bool IsLoaded() const override { return m_Loaded; }
    const std::string& GetFilePath() const override { return m_FilePath; }
    
private:
    bool LoadWAV(const std::string& filepath);
    ALenum GetALFormat() const;
    
    ALuint m_BufferID = 0;
    std::string m_FilePath;
    float m_Duration = 0.0f;
    int m_SampleRate = 0;
    int m_Channels = 0;
    int m_BitsPerSample = 0;
    bool m_Loaded = false;
};
```

### 4.3 OpenALSource

**File:** `Pillar/src/Platform/OpenAL/OpenALSource.h`

OpenAL implementation of AudioSource.

```cpp
class OpenALSource : public AudioSource
{
public:
    OpenALSource();
    ~OpenALSource();
    
    // All AudioSource interface methods implemented...
    
private:
    ALuint m_SourceID = 0;
    std::shared_ptr<AudioBuffer> m_Buffer;
    float m_Volume = 1.0f;
    float m_Pitch = 1.0f;
    bool m_Looping = false;
    glm::vec3 m_Position = { 0.0f, 0.0f, 0.0f };
};
```

---

## 5. API Design

### Audio API Enum

**File:** `Pillar/src/Pillar/Audio/AudioAPI.h`

```cpp
enum class AudioAPI
{
    None = 0,
    OpenAL = 1
    // Future: FMOD, XAudio2, etc.
};

class AudioAPISelector
{
public:
    static AudioAPI GetAPI() { return s_API; }
    static void SetAPI(AudioAPI api) { s_API = api; }
    
private:
    static AudioAPI s_API;
};
```

---

## 6. Integration Points

### 6.1 Application Integration

**File:** `Pillar/src/Pillar/Application.cpp`

```cpp
void Application::Run()
{
    // Initialize audio engine
    AudioEngine::Init();
    
    // ... existing code ...
    
    // Shutdown audio engine
    AudioEngine::Shutdown();
}
```

### 6.2 Pillar.h Integration

**File:** `Pillar/src/Pillar.h`

```cpp
// Audio
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"
#include "Pillar/Audio/AudioClip.h"
```

### 6.3 Event Integration (Optional)

**File:** `Pillar/src/Pillar/Events/AudioEvent.h`

```cpp
enum class AudioEventType
{
    SoundStarted,
    SoundFinished,
    SoundPaused,
    SoundResumed
};

class AudioPlaybackEvent : public Event
{
public:
    AudioPlaybackEvent(AudioEventType type, uint32_t sourceId);
    
    EVENT_CLASS_TYPE(AudioPlayback)
    EVENT_CLASS_CATEGORY(EventCategoryAudio)
    
    AudioEventType GetAudioEventType() const { return m_Type; }
    uint32_t GetSourceID() const { return m_SourceID; }
    
private:
    AudioEventType m_Type;
    uint32_t m_SourceID;
};
```

---

## 7. Asset Management

### AssetManager Update

**File:** `Pillar/src/Pillar/Utils/AssetManager.h`

Add audio path resolution:

```cpp
class PIL_API AssetManager
{
public:
    // Existing methods...
    
    /**
     * @brief Resolves an audio asset path
     * @param audioName The audio filename (e.g., "explosion.wav")
     * @return Full path to the audio file if found
     */
    static std::string GetAudioPath(const std::string& audioName);
    
    /**
     * @brief Resolves a sound effect path (assets/audio/sfx/)
     */
    static std::string GetSFXPath(const std::string& sfxName);
    
    /**
     * @brief Resolves a music path (assets/audio/music/)
     */
    static std::string GetMusicPath(const std::string& musicName);
};
```

### Supported Formats

**Phase 1 (MVP):**
- WAV (uncompressed PCM) - Simple parsing, no external dependencies

**Phase 2 (Future):**
- OGG Vorbis (via stb_vorbis)
- MP3 (via minimp3 or dr_mp3)
- FLAC (via dr_flac)

---

## 8. ECS Integration

### 8.1 AudioSourceComponent

**File:** `Pillar/src/Pillar/ECS/Components/Audio/AudioSourceComponent.h`

```cpp
struct AudioSourceComponent
{
    std::shared_ptr<AudioSource> Source;
    std::string AudioFile;          // Path to audio file
    float Volume = 1.0f;
    float Pitch = 1.0f;
    bool Loop = false;
    bool PlayOnAwake = false;
    bool Is3D = true;               // Use 3D spatial audio
    float MinDistance = 1.0f;
    float MaxDistance = 100.0f;
    
    AudioSourceComponent() = default;
    AudioSourceComponent(const std::string& file) : AudioFile(file) {}
};
```

### 8.2 AudioListenerComponent

**File:** `Pillar/src/Pillar/ECS/Components/Audio/AudioListenerComponent.h`

```cpp
struct AudioListenerComponent
{
    bool IsActive = true;           // Only one listener can be active
    glm::vec3 Forward = { 0.0f, 0.0f, -1.0f };
    glm::vec3 Up = { 0.0f, 1.0f, 0.0f };
    
    AudioListenerComponent() = default;
};
```

### 8.3 AudioSystem (ECS)

**File:** `Pillar/src/Pillar/ECS/Systems/AudioSystem.h`

```cpp
class AudioSystem : public System
{
public:
    void OnUpdate(float dt, entt::registry& registry) override;
    
private:
    void UpdateListener(entt::registry& registry);
    void UpdateSources(entt::registry& registry);
};
```

---

## 9. Implementation Phases

### Phase 1: Core Audio (MVP) - Week 1-2

**Goal:** Basic audio playback working

1. **OpenAL Integration**
   - [ ] Set up OpenAL-Soft in CMakeLists.txt (DONE in root CMake)
   - [ ] Implement `OpenALContext` (device/context init)
   - [ ] Test OpenAL initialization

2. **WAV Loading**
   - [ ] Implement `WavLoader` utility
   - [ ] Support 8-bit and 16-bit PCM
   - [ ] Support mono and stereo

3. **Basic Playback**
   - [ ] Implement `OpenALBuffer`
   - [ ] Implement `OpenALSource`
   - [ ] Implement `AudioEngine::Init/Shutdown`
   - [ ] Implement factory methods

4. **Integration**
   - [ ] Update `AssetManager` for audio paths
   - [ ] Update `Pillar.h` includes
   - [ ] Add to `Pillar/CMakeLists.txt`

5. **Testing**
   - [ ] Create `AudioTests.cpp`
   - [ ] Test buffer loading
   - [ ] Test source playback

### Phase 2: Enhanced Features - Week 3

**Goal:** Complete audio API

1. **Volume & Pitch**
   - [ ] Master volume control
   - [ ] Per-source volume/pitch
   - [ ] Looping support

2. **3D Audio**
   - [ ] Listener position/orientation
   - [ ] Source positioning
   - [ ] Distance attenuation

3. **Playback Control**
   - [ ] Pause/Resume
   - [ ] Seek/GetPlaybackPosition
   - [ ] Stop all sounds

4. **High-Level API**
   - [ ] Implement `AudioClip` wrapper
   - [ ] Simple one-shot playback

### Phase 3: ECS & Polish - Week 4

**Goal:** Full engine integration

1. **ECS Components**
   - [ ] `AudioSourceComponent`
   - [ ] `AudioListenerComponent`
   - [ ] `AudioSystem`

2. **Sandbox Demo**
   - [ ] Add audio demo to ExampleLayer
   - [ ] UI controls for audio testing

3. **Documentation**
   - [ ] Update copilot-instructions.md
   - [ ] API documentation
   - [ ] Usage examples

### Phase 4: Future Enhancements

1. **Additional Formats**
   - [ ] OGG Vorbis support
   - [ ] Streaming for large files

2. **Advanced Features**
   - [ ] Audio effects (reverb, echo)
   - [ ] Audio mixing/channels
   - [ ] Background music system

---

## 10. Testing Strategy

### Unit Tests

**File:** `Tests/src/AudioTests.cpp`

```cpp
#include <gtest/gtest.h>
#include "Pillar/Audio/AudioEngine.h"
#include "Pillar/Audio/AudioBuffer.h"
#include "Pillar/Audio/AudioSource.h"

class AudioTests : public ::testing::Test {
protected:
    void SetUp() override {
        Pillar::AudioEngine::Init();
    }
    
    void TearDown() override {
        Pillar::AudioEngine::Shutdown();
    }
};

// Initialization tests
TEST_F(AudioTests, AudioEngine_InitializesSuccessfully) {
    EXPECT_TRUE(Pillar::AudioEngine::IsInitialized());
}

// Buffer tests
TEST_F(AudioTests, AudioBuffer_LoadsWAVFile) {
    auto buffer = Pillar::AudioBuffer::Create("test_sound.wav");
    ASSERT_NE(buffer, nullptr);
    EXPECT_TRUE(buffer->IsLoaded());
    EXPECT_GT(buffer->GetDuration(), 0.0f);
}

TEST_F(AudioTests, AudioBuffer_FailsOnInvalidFile) {
    auto buffer = Pillar::AudioBuffer::Create("nonexistent.wav");
    EXPECT_EQ(buffer, nullptr);
}

// Source tests
TEST_F(AudioTests, AudioSource_CreatesSuccessfully) {
    auto source = Pillar::AudioSource::Create();
    ASSERT_NE(source, nullptr);
    EXPECT_TRUE(source->IsStopped());
}

TEST_F(AudioTests, AudioSource_PlaysBuffer) {
    auto buffer = Pillar::AudioBuffer::Create("test_sound.wav");
    auto source = Pillar::AudioSource::Create();
    
    source->SetBuffer(buffer);
    source->Play();
    
    EXPECT_TRUE(source->IsPlaying());
}

// Volume tests
TEST_F(AudioTests, AudioSource_VolumeClamps) {
    auto source = Pillar::AudioSource::Create();
    
    source->SetVolume(1.5f);
    EXPECT_LE(source->GetVolume(), 1.0f);
    
    source->SetVolume(-0.5f);
    EXPECT_GE(source->GetVolume(), 0.0f);
}

// 3D Audio tests
TEST_F(AudioTests, AudioEngine_ListenerPosition) {
    glm::vec3 pos = { 10.0f, 5.0f, 3.0f };
    Pillar::AudioEngine::SetListenerPosition(pos);
    
    glm::vec3 result = Pillar::AudioEngine::GetListenerPosition();
    EXPECT_FLOAT_EQ(result.x, pos.x);
    EXPECT_FLOAT_EQ(result.y, pos.y);
    EXPECT_FLOAT_EQ(result.z, pos.z);
}
```

### Integration Tests

- Test audio with rendering loop
- Test multiple simultaneous sources
- Test 3D positioning with camera movement

### CI Considerations

- Audio tests may require special handling in CI (no audio device)
- Use mock audio device or skip hardware-dependent tests
- OpenAL-Soft supports "null" device for testing

---

## 11. CMake Configuration

### Root CMakeLists.txt Updates

OpenAL-Soft is already configured in the root CMakeLists.txt:

```cmake
# OpenAL-Soft (cross-platform audio library)
FetchContent_Declare(
  openal
  GIT_REPOSITORY https://github.com/kcat/openal-soft.git
  GIT_TAG 1.24.3
)

# Configure OpenAL-Soft options
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
```

### Pillar CMakeLists.txt Updates

```cmake
# Add audio source files to Pillar library
add_library(Pillar STATIC
    # ... existing sources ...
    
    # Audio
    src/Pillar/Audio/AudioEngine.cpp
    src/Pillar/Audio/AudioBuffer.cpp
    src/Pillar/Audio/AudioSource.cpp
    src/Pillar/Audio/AudioClip.cpp
    src/Pillar/Audio/WavLoader.cpp
    
    # Platform - OpenAL
    src/Platform/OpenAL/OpenALContext.cpp
    src/Platform/OpenAL/OpenALBuffer.cpp
    src/Platform/OpenAL/OpenALSource.cpp
    
    # ECS - Audio Components (header-only)
    src/Pillar/ECS/Components/Audio/AudioSourceComponent.h
    src/Pillar/ECS/Components/Audio/AudioListenerComponent.h
    
    # ECS - Audio System
    src/Pillar/ECS/Systems/AudioSystem.cpp
)

# Link OpenAL
target_link_libraries(Pillar 
    PUBLIC 
        # ... existing libraries ...
        OpenAL::OpenAL
)
```

### Tests CMakeLists.txt Updates

```cmake
add_executable(PillarTests
    # ... existing tests ...
    src/AudioTests.cpp
)
```

---

## 12. Usage Examples

### Basic Playback

```cpp
// In layer OnAttach()
void MyLayer::OnAttach()
{
    // Load and play a sound effect
    m_ExplosionClip = Pillar::AudioClip::Create("explosion.wav");
}

void MyLayer::OnUpdate(float dt)
{
    if (/* explosion happened */)
    {
        m_ExplosionClip->Play();
    }
}
```

### Background Music

```cpp
void MyLayer::OnAttach()
{
    // Load background music
    auto musicBuffer = Pillar::AudioBuffer::Create("background_music.wav");
    m_MusicSource = Pillar::AudioSource::Create();
    m_MusicSource->SetBuffer(musicBuffer);
    m_MusicSource->SetLooping(true);
    m_MusicSource->SetVolume(0.5f);
    m_MusicSource->Play();
}

void MyLayer::OnDetach()
{
    m_MusicSource->Stop();
}
```

### 3D Positional Audio

```cpp
void MyLayer::OnUpdate(float dt)
{
    // Update listener position to match camera
    auto& camera = m_CameraController.GetCamera();
    glm::vec3 camPos = camera.GetPosition();
    Pillar::AudioEngine::SetListenerPosition(camPos);
    
    // Update sound source position
    glm::vec3 enemyPos = GetEnemyPosition();
    m_EnemySource->SetPosition(enemyPos);
}
```

### ECS-Based Audio

```cpp
// Creating an entity with audio
auto entity = m_Scene.CreateEntity("AudioEmitter");
entity.AddComponent<TransformComponent>();
entity.AddComponent<AudioSourceComponent>("enemy_growl.wav");
auto& audioComp = entity.GetComponent<AudioSourceComponent>();
audioComp.Loop = true;
audioComp.Is3D = true;
audioComp.PlayOnAwake = true;
audioComp.Volume = 0.8f;

// Audio listener on camera entity
auto cameraEntity = m_Scene.CreateEntity("Camera");
cameraEntity.AddComponent<TransformComponent>();
cameraEntity.AddComponent<AudioListenerComponent>();
```

### ImGui Audio Controls

```cpp
void MyLayer::OnImGuiRender()
{
    ImGui::Begin("Audio Controls");
    
    float masterVolume = Pillar::AudioEngine::GetMasterVolume();
    if (ImGui::SliderFloat("Master Volume", &masterVolume, 0.0f, 1.0f))
    {
        Pillar::AudioEngine::SetMasterVolume(masterVolume);
    }
    
    if (ImGui::Button("Play SFX"))
    {
        m_SFXClip->Play();
    }
    
    if (m_MusicSource->IsPlaying())
    {
        if (ImGui::Button("Pause Music"))
            m_MusicSource->Pause();
    }
    else
    {
        if (ImGui::Button("Play Music"))
            m_MusicSource->Play();
    }
    
    ImGui::End();
}
```

---

## 13. Future Enhancements

### Audio Formats
- **OGG Vorbis**: Use `stb_vorbis` (header-only, already have stb)
- **MP3**: Use `dr_mp3` or `minimp3` (header-only)
- **FLAC**: Use `dr_flac` (header-only)

### Streaming Audio
- For large music files (> 1MB)
- Double-buffered streaming
- Background loading thread

### Audio Effects
- OpenAL EFX extension support
- Reverb, Echo, Chorus
- Low-pass/High-pass filters

### Advanced Features
- Audio groups/mixer channels
- Crossfade between tracks
- Ducking (lower music when SFX plays)
- Audio occlusion (walls blocking sound)

### Platform Support
- Windows: OpenAL-Soft ?
- Linux: OpenAL-Soft
- macOS: OpenAL-Soft or Core Audio
- Consoles: Platform-specific APIs

---

## Summary

This audio system plan provides a comprehensive roadmap for adding audio capabilities to the Pillar Engine. The design follows existing engine patterns:

1. **Abstraction**: Platform-agnostic interfaces (`AudioBuffer`, `AudioSource`)
2. **Factory Pattern**: `Create()` methods for platform-specific instantiation
3. **Static API**: `AudioEngine` for global control (like `Renderer`)
4. **ECS Integration**: Components for entity-based audio
5. **Asset Management**: Integrated with `AssetManager`

The implementation is divided into clear phases, starting with basic WAV playback and progressing to 3D audio and ECS integration. OpenAL-Soft provides a solid, cross-platform foundation for audio functionality.

---

**Next Steps:**
1. Review and approve this plan
2. Begin Phase 1 implementation
3. Create test audio files in `Sandbox/assets/audio/`
4. Implement core classes following the outlined structure
