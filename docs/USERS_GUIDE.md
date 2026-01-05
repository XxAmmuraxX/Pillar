# Pillar Engine - User's Guide

A comprehensive guide to using the Pillar Engine for 2D game development.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Project Structure](#project-structure)
3. [Core Concepts](#core-concepts)
4. [Creating Your First Application](#creating-your-first-application)
5. [Entity Component System (ECS)](#entity-component-system-ecs)
6. [Rendering](#rendering)
7. [Input Handling](#input-handling)
8. [Audio System](#audio-system)
9. [Physics System](#physics-system)
10. [Animation System](#animation-system)
11. [Scene Management](#scene-management)
12. [Asset Management](#asset-management)
13. [ImGui Integration](#imgui-integration)
14. [Best Practices](#best-practices)
15. [API Reference](#api-reference)

---

## Getting Started

### Prerequisites

Before using Pillar Engine, ensure you have:
- Successfully built the engine (see [Installation Guide](INSTALLATION_GUIDE.md))
- Basic knowledge of C++17
- Familiarity with game development concepts

### Quick Start

1. Create a new layer class inheriting from `Pillar::Layer`
2. Override `OnAttach()`, `OnUpdate()`, `OnEvent()`, and `OnImGuiRender()`
3. Push your layer in `CreateApplication()`

---

## Project Structure

```
YourGame/
├── src/
│   ├── GameLayer.h          # Your main game layer
│   ├── GameLayer.cpp
│   └── Source.cpp           # Entry point (CreateApplication)
├── assets/
│   ├── textures/            # PNG, JPG images
│   ├── audio/               # WAV audio files
│   │   ├── sfx/             # Sound effects
│   │   └── music/           # Background music
|   |── templates/           # Entity templates
│   └── scenes/              # Serialized scene files (.json)
└── CMakeLists.txt
```

---

## Core Concepts

### Application Lifecycle

```
main() → CreateApplication() → Application::Run() → Game Loop
                                      ↓
                              ┌──────────────────┐
                              │  Process Events  │
                              │  Update Layers   │
                              │  Render Frame    │
                              │  ImGui Render    │
                              └──────────────────┘
```

### Layer System

Layers are processed in order:
- **Regular Layers:** Game logic, rendering
- **Overlay Layers:** UI, debug panels (rendered on top)

Layers are updated bottom-to-top
Events propagate top-to-bottom until handled


---

## Creating Your First Application

### Step 1: Create Your Layer

```cpp
// GameLayer.h
#pragma once
#include <Pillar.h>

class GameLayer : public Pillar::Layer
{
public:
    GameLayer() : Layer("GameLayer") {}
    
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float dt) override;
    void OnEvent(Pillar::Event& event) override;
    void OnImGuiRender() override;

private:
    Pillar::OrthographicCameraController m_CameraController;
    std::unique_ptr<Pillar::Scene> m_Scene;
};
```

### Step 2: Implement the Layer

```cpp
// GameLayer.cpp
#include "GameLayer.h"

void GameLayer::OnAttach()
{
    // Initialize camera (aspect ratio, enable rotation)
    m_CameraController = Pillar::OrthographicCameraController(16.0f / 9.0f, true);
    
    // Create scene
    m_Scene = std::make_unique<Pillar::Scene>("MainScene");
    
    // Create entities
    auto player = m_Scene->CreateEntity("Player");
    player.GetComponent<Pillar::TransformComponent>().Position = {0.0f, 0.0f};
    
    auto& sprite = player.AddComponent<Pillar::SpriteComponent>();
    sprite.Color = {0.2f, 0.8f, 0.3f, 1.0f};
    sprite.Size = {1.0f, 1.0f};
}

void GameLayer::OnDetach()
{
    m_Scene.reset();
}

void GameLayer::OnUpdate(float dt)
{
    // Update camera
    m_CameraController.OnUpdate(dt);
    
    // Clear screen
    Pillar::RenderCommand::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
    Pillar::RenderCommand::Clear();
    
    // Render scene
    // (Your rendering code here)
}

void GameLayer::OnEvent(Pillar::Event& event)
{
    m_CameraController.OnEvent(event);
}

void GameLayer::OnImGuiRender()
{
    ImGui::Begin("Game Stats");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
}
```

### Step 3: Create Entry Point

```cpp
// Source.cpp
#include <Pillar.h>
#include <Pillar/EntryPoint.h>
#include "GameLayer.h"

class GameApp : public Pillar::Application
{
public:
    GameApp()
    {
        PushLayer(new GameLayer());
    }
};

Pillar::Application* Pillar::CreateApplication()
{
    return new GameApp();
}
```

---

## Entity Component System (ECS)

### Creating Entities

```cpp
// Create entity with default components (Tag, Transform, UUID)
Pillar::Entity player = m_Scene->CreateEntity("Player");

// Create with specific UUID (for serialization)
Pillar::Entity enemy = m_Scene->CreateEntityWithUUID(12345, "Enemy");
```

### Adding Components

```cpp
// Transform (added by default)
auto& transform = player.GetComponent<Pillar::TransformComponent>();
transform.Position = {100.0f, 50.0f};
transform.Scale = {2.0f, 2.0f};
transform.Rotation = 45.0f;

// Sprite
auto& sprite = player.AddComponent<Pillar::SpriteComponent>();
sprite.Color = {1.0f, 1.0f, 1.0f, 1.0f};
sprite.Texture = Pillar::Texture2D::Create("player.png");
sprite.Size = {64.0f, 64.0f};

// Velocity
auto& velocity = player.AddComponent<Pillar::VelocityComponent>();
velocity.Velocity = {100.0f, 0.0f};

// Collider
auto collider = Pillar::ColliderComponent::Circle(0.5f);
collider.IsSensor = false;
player.AddComponent<Pillar::ColliderComponent>(collider);

// Animation
auto& anim = player.AddComponent<Pillar::AnimationComponent>();
anim.Play("idle");
```

### Querying Components

```cpp
// Check if entity has component
if (player.HasComponent<Pillar::VelocityComponent>())
{
    auto& vel = player.GetComponent<Pillar::VelocityComponent>();
    // Use velocity...
}

// Query all entities with specific components
m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>()
    .each([](auto entity, auto& transform, auto& sprite) {
        // Process each entity
    });
```

### Destroying Entities

```cpp
m_Scene->DestroyEntity(player);
```

---

## Rendering

Pillar’s recommended 2D rendering API is `Renderer2DBackend`, which uses the batch renderer internally.

### Using Renderer2DBackend

```cpp
void GameLayer::OnUpdate(float dt)
{
    // Begin scene with camera
    Pillar::Renderer2DBackend::ResetStats();
    Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());
    
    // Draw colored quad
    Pillar::Renderer2DBackend::DrawQuad({0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
    
    // Draw textured quad
    Pillar::Renderer2DBackend::DrawQuad({2.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, m_Texture);
    
    // Draw rotated quad
    Pillar::Renderer2DBackend::DrawRotatedQuad({-2.0f, 0.0f}, {1.0f, 1.0f}, glm::radians(45.0f), {0.0f, 1.0f, 0.0f, 1.0f});
    
    Pillar::Renderer2DBackend::EndScene();

    // Optional: read stats (useful for perf HUDs)
    // uint32_t drawCalls = Pillar::Renderer2DBackend::GetDrawCallCount();
    // uint32_t quads = Pillar::Renderer2DBackend::GetQuadCount();
}
```

> Note: The rotated quad APIs take rotation in **radians**.

### Debug Shapes

```cpp
Pillar::Renderer2DBackend::DrawLine({0.0f, 0.0f}, {2.0f, 1.0f}, {1, 1, 0, 1}, 2.0f);
Pillar::Renderer2DBackend::DrawRect({0.0f, 0.0f}, {2.0f, 1.0f}, {0, 1, 1, 1}, 2.0f);
Pillar::Renderer2DBackend::DrawCircle({0.0f, 0.0f}, 1.0f, {1, 0, 1, 1}, 2.0f);
```

### Loading Textures

```cpp
// Load from file (uses AssetManager for path resolution)
auto texture = Pillar::Texture2D::Create("player.png");

// Create blank texture
auto blankTexture = Pillar::Texture2D::Create(64, 64);

// Set pixel data
uint32_t whitePixel = 0xFFFFFFFF;
blankTexture->SetData(&whitePixel, sizeof(whitePixel));
```

### Camera Controls

```cpp
// Create camera controller
m_CameraController = Pillar::OrthographicCameraController(aspectRatio, enableRotation);

// In OnUpdate
m_CameraController.OnUpdate(dt);  // Handles WASD, Q/E, mouse wheel

// In OnEvent
m_CameraController.OnEvent(event);  // Handles window resize

// Manual camera access
auto& camera = m_CameraController.GetCamera();
camera.SetPosition({10.0f, 5.0f, 0.0f});
```

---

## Input Handling

### Polling Input State

```cpp
// Keyboard
if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE))
{
    Jump();
}

if (Pillar::Input::IsKeyPressed(PIL_KEY_W))
{
    MoveUp();
}

// Mouse
if (Pillar::Input::IsMouseButtonPressed(PIL_MOUSE_BUTTON_LEFT))
{
    Shoot();
}

auto [x, y] = Pillar::Input::GetMousePosition();
```

### Event-Based Input

```cpp
void GameLayer::OnEvent(Pillar::Event& event)
{
    Pillar::EventDispatcher dispatcher(event);
    
    dispatcher.Dispatch<Pillar::KeyPressedEvent>(
        [this](Pillar::KeyPressedEvent& e) {
            if (e.GetKeyCode() == PIL_KEY_ESCAPE)
            {
                // Handle escape
                return true;  // Event handled
            }
            return false;
        });
    
    dispatcher.Dispatch<Pillar::MouseButtonPressedEvent>(
        [this](Pillar::MouseButtonPressedEvent& e) {
            // Handle mouse click
            return false;
        });
}
```

### Key Codes

Common key codes defined in `KeyCodes.h`:

| Key | Code |
|-----|------|
| W, A, S, D | `PIL_KEY_W`, `PIL_KEY_A`, `PIL_KEY_S`, `PIL_KEY_D` |
| Space | `PIL_KEY_SPACE` |
| Escape | `PIL_KEY_ESCAPE` |
| Enter | `PIL_KEY_ENTER` |
| Arrow Keys | `PIL_KEY_UP`, `PIL_KEY_DOWN`, `PIL_KEY_LEFT`, `PIL_KEY_RIGHT` |
| Numbers | `PIL_KEY_0` through `PIL_KEY_9` |

---

## Audio System

### Initialization

Audio system is automatically initialized by the Application.

### Playing Sounds

```cpp
// Simple one-shot sound
auto clip = Pillar::AudioClip::Create("explosion.wav");
clip->Play();

// With settings
clip->SetVolume(0.8f);
clip->SetPitch(1.2f);
clip->Play();
```

### Background Music

```cpp
// Create buffer and source for looping music
auto musicBuffer = Pillar::AudioBuffer::Create("background.wav");
auto musicSource = Pillar::AudioSource::Create();
musicSource->SetBuffer(musicBuffer);
musicSource->SetLooping(true);
musicSource->SetVolume(0.5f);
musicSource->Play();
```

### 3D Positional Audio

```cpp
// Update listener position (usually camera)
Pillar::AudioEngine::SetListenerPosition(cameraPosition);

// Position sound source in world
soundSource->SetPosition({enemyX, enemyY, 0.0f});
soundSource->SetMinDistance(5.0f);
soundSource->SetMaxDistance(50.0f);
```

### Global Audio Control

```cpp
Pillar::AudioEngine::SetMasterVolume(0.75f);
Pillar::AudioEngine::StopAllSounds();
Pillar::AudioEngine::PauseAllSounds();
Pillar::AudioEngine::ResumeAllSounds();
```

---

## Physics System

### Adding Physics to Entities

```cpp
// Add rigidbody (makes entity a "heavy" physics object)
auto& rb = entity.AddComponent<Pillar::RigidbodyComponent>();
rb.BodyType = b2_dynamicBody;  // or b2_staticBody, b2_kinematicBody
rb.FixedRotation = true;       // Prevent rotation

// Add collider
auto collider = Pillar::ColliderComponent::Circle(0.5f);
// or: auto collider = Pillar::ColliderComponent::Box({0.5f, 0.5f});
collider.Density = 1.0f;
collider.Friction = 0.3f;
collider.Restitution = 0.1f;  // Bounciness
collider.IsSensor = false;     // true = trigger only, no physics response
entity.AddComponent<Pillar::ColliderComponent>(collider);
```

### Physics System Update

```cpp
// PhysicsSystem handles Box2D integration
m_PhysicsSystem.OnUpdate(dt, m_Scene->GetRegistry());
```

### Collision Filtering

```cpp
collider.CategoryBits = 0x0002;  // What am I?
collider.MaskBits = 0x0001;      // What do I collide with?
collider.GroupIndex = -1;        // Negative = never collide with same group
```

---

## Animation System

### Setting Up Animations

```cpp
// Add animation component
auto& anim = entity.AddComponent<Pillar::AnimationComponent>();

// Load animation clips (defined in .anim.json files)
// Animation system will read from AnimationClip assets

// Control playback
anim.Play("walk_right");
anim.Pause();
anim.Resume();
anim.Stop();

// Adjust speed
anim.PlaybackSpeed = 1.5f;  // 1.5x speed
```

### Animation Events

```cpp
anim.OnAnimationEvent = [](const std::string& eventName, entt::entity e) {
    if (eventName == "footstep")
    {
        PlayFootstepSound();
    }
};

anim.OnAnimationComplete = [](entt::entity e) {
    // Animation finished (non-looping only)
};
```

---

## Scene Management

### Creating and Switching Scenes

```cpp
auto& manager = Pillar::SceneManager::Get();

// Create scenes
manager.CreateScene("MainMenu");
manager.CreateScene("Level1");
manager.CreateScene("Level2");

// Switch scenes
manager.SetActiveScene("Level1");

// Get current scene
auto* currentScene = manager.GetActiveScene();
```

### Saving and Loading Scenes

```cpp
// Save current scene
Pillar::SceneSerializer serializer(m_Scene.get());
serializer.Serialize("scenes/level1.json");

// Load scene
auto newScene = std::make_unique<Pillar::Scene>("LoadedScene");
Pillar::SceneSerializer loader(newScene.get());
loader.Deserialize("scenes/level1.json");
```

---

## Asset Management

### Path Resolution

```cpp
// AssetManager automatically finds assets in multiple locations:
// 1. Sandbox/assets/ (development)
// 2. assets/ next to executable (distribution)

// Get texture path
std::string texPath = Pillar::AssetManager::GetTexturePath("player.png");

// Get audio path
std::string audioPath = Pillar::AssetManager::GetAudioPath("explosion.wav");

// Get SFX specifically
std::string sfxPath = Pillar::AssetManager::GetSFXPath("jump.wav");

// Get music specifically
std::string musicPath = Pillar::AssetManager::GetMusicPath("background.wav");
```

### Setting Custom Asset Directory

```cpp
Pillar::AssetManager::SetAssetsDirectory("C:/MyGame/assets/");
```

---

## ImGui Integration

### Creating Debug Panels

```cpp
void GameLayer::OnImGuiRender()
{
    ImGui::Begin("Debug Panel");
    
    // Display stats
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Entities: %d", m_Scene->GetEntityCount());
    
    // Edit values
    static float speed = 100.0f;
    ImGui::SliderFloat("Player Speed", &speed, 0.0f, 500.0f);
    
    // Buttons
    if (ImGui::Button("Spawn Enemy"))
    {
        SpawnEnemy();
    }
    
    ImGui::End();
}
```


## Best Practices

### Performance Tips

1. **Use Object Pools** for frequently created/destroyed entities (bullets, particles)
   ```cpp
   Pillar::BulletPool bulletPool(m_Scene.get(), 1000);
   auto bullet = bulletPool.Spawn(position, direction, speed, damage);
   ```

2. **Batch Draw Calls** - `Renderer2DBackend` batches by texture

3. **Minimize Component Queries** - Cache frequently accessed components

### Code Organization

1. **Separate Concerns** - One layer per major game state (Menu, Gameplay, Pause)
2. **Use Systems** - Put logic in Systems, not Components
3. **Event-Driven** - Use events for loose coupling between systems

### Memory Management

1. Use `std::unique_ptr` for owned resources
2. Use `std::shared_ptr` for shared resources (textures)
3. Let ECS manage entity/component memory

---

## API Reference

### Core Classes

| Class | Purpose |
|-------|---------|
| `Application` | Main application class, manages game loop |
| `Layer` | Base class for game layers |
| `Event` | Base class for all events |
| `Input` | Static input polling |
| `Logger` | Logging utilities (PIL_INFO, PIL_ERROR, etc.) |

### ECS Classes

| Class | Purpose |
|-------|---------|
| `Scene` | Container for entities |
| `Entity` | Wrapper around entt::entity |
| `SceneManager` | Manages multiple scenes |
| `SceneSerializer` | Save/load scenes to JSON |

### Renderer Classes

| Class | Purpose |
|-------|---------|
| `Renderer2DBackend` | Recommended batched 2D rendering API |
| `OrthographicCamera` | 2D camera |
| `OrthographicCameraController` | Camera input handling |
| `Texture2D` | 2D texture |
| `Shader` | GPU shader program |

### Audio Classes

| Class | Purpose |
|-------|---------|
| `AudioEngine` | Global audio management |
| `AudioBuffer` | Decoded audio data |
| `AudioSource` | Playback control |
| `AudioClip` | Simple one-shot sounds |

For detailed API documentation, see the header files in `Pillar/src/Pillar/`.

