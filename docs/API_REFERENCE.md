# Pillar Engine API Reference

A comprehensive guide to all public APIs available in the Pillar Engine. This document helps developers and AI agents quickly discover and understand available features.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Core Systems](#core-systems)
3. [Rendering](#rendering)
4. [Entity Component System (ECS)](#entity-component-system-ecs)
5. [Audio System](#audio-system)
6. [Input System](#input-system)
7. [Scene Management](#scene-management)
8. [Utility Classes](#utility-classes)
9. [Quick Reference Tables](#quick-reference-tables)

---

## Getting Started

### Main Include

```cpp
#include <Pillar.h>  // Includes all public headers
```

### Application Entry Point

```cpp
class MyGame : public Pillar::Application
{
public:
    MyGame()
    {
        PushLayer(new GameLayer());
    }
};

Pillar::Application* Pillar::CreateApplication()
{
    return new MyGame();
}
```

---

## Core Systems

### Application

**Header:** `Pillar/Application.h`

| Method | Description |
|--------|-------------|
| `Run()` | Main application loop (called automatically) |
| `Close()` | Request application shutdown |
| `PushLayer(Layer*)` | Add a layer to the stack |
| `PushOverlay(Layer*)` | Add an overlay (renders on top) |
| `GetWindow()` | Get the main window reference |
| `Get()` | Static accessor to application instance |

### Layer

**Header:** `Pillar/Layer.h`

Base class for game logic organization. Override these virtual methods:

| Method | Description |
|--------|-------------|
| `OnAttach()` | Called when layer is added |
| `OnDetach()` | Called when layer is removed |
| `OnUpdate(float dt)` | Called every frame with delta time |
| `OnEvent(Event& e)` | Handle input/window events |
| `OnImGuiRender()` | Render ImGui debug UI |

### Time

**Header:** `Pillar/Time.h`

```cpp
// Get delta time (scaled by time scale)
float dt = Pillar::Time::GetDeltaTime();

// Get unscaled delta time
float realDt = Pillar::Time::GetUnscaledDeltaTime();

// Slow motion / pause
Pillar::Time::SetTimeScale(0.5f);  // Half speed
Pillar::Time::SetTimeScale(0.0f);  // Paused

// Time since startup
float elapsed = Pillar::Time::GetTimeSeconds();

// Frame counter
uint64_t frame = Pillar::Time::GetFrameCount();
```

### Logger

**Header:** `Pillar/Logger.h`

```cpp
// Client application logging
PIL_TRACE("Debug info: {}", value);
PIL_INFO("Information: {}", message);
PIL_WARN("Warning: {}", warning);
PIL_ERROR("Error: {}", error);

// Engine-internal logging (for Pillar development)
PIL_CORE_TRACE("...");
PIL_CORE_INFO("...");
PIL_CORE_WARN("...");
PIL_CORE_ERROR("...");
```

---

## Rendering

### Renderer2D (Primary API)

**Header:** `Pillar/Renderer/Renderer2D.h`

High-performance batched 2D renderer. **Use this for most 2D rendering.**

```cpp
// Lifecycle (called once in OnAttach/OnDetach)
Pillar::Renderer2D::Init();
Pillar::Renderer2D::Shutdown();

// Frame rendering
Pillar::Renderer2D::SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
Pillar::Renderer2D::Clear();
Pillar::Renderer2D::BeginScene(camera);

// Draw colored quad
Pillar::Renderer2D::DrawQuad({0, 0}, {1, 1}, {1, 0, 0, 1});

// Draw textured quad
Pillar::Renderer2D::DrawQuad({2, 0}, {1, 1}, texture);

// Draw with texture + color tint
Pillar::Renderer2D::DrawQuad({4, 0}, {1, 1}, {1, 1, 1, 1}, texture);

// Draw rotated quad (rotation in radians)
Pillar::Renderer2D::DrawRotatedQuad({0, 2}, {1, 1}, glm::radians(45.0f), {0, 1, 0, 1});

// Draw from texture atlas
Pillar::Renderer2D::DrawQuad({0, 0}, {1, 1}, {1, 1, 1, 1}, atlas, "sprite_name");

// Debug drawing
Pillar::Renderer2D::DrawLine({0, 0}, {5, 5}, {1, 1, 1, 1});
Pillar::Renderer2D::DrawRect({0, 0}, {2, 2}, {0, 1, 0, 1});
Pillar::Renderer2D::DrawCircle({3, 3}, 1.0f, {0, 0, 1, 1});

// ECS convenience
Pillar::Renderer2D::DrawSprite(transform, sprite);

Pillar::Renderer2D::EndScene();
```

### Camera

**Header:** `Pillar/Renderer/OrthographicCamera.h`, `Pillar/Renderer/OrthographicCameraController.h`

```cpp
// Manual camera
Pillar::OrthographicCamera camera(-16.0f, 16.0f, -9.0f, 9.0f);
camera.SetPosition({0, 0, 0});
camera.SetRotation(0.0f);

// Camera controller with built-in input (recommended)
float aspectRatio = 16.0f / 9.0f;
Pillar::OrthographicCameraController controller(aspectRatio, true);  // true = enable rotation

// In OnUpdate:
controller.OnUpdate(deltaTime);  // Handles WASD, Q/E rotation, scroll zoom

// In OnEvent:
controller.OnEvent(event);  // Handles resize, scroll

// Get camera for rendering
Renderer2D::BeginScene(controller.GetCamera());

// Control zoom
controller.SetZoomLevel(2.0f);  // 2x zoom out
```

### Textures

**Header:** `Pillar/Renderer/Texture.h`

```cpp
// Load texture (auto-resolves path via AssetManager)
auto texture = Pillar::Texture2D::Create("my_sprite.png");

// Get dimensions
uint32_t width = texture->GetWidth();
uint32_t height = texture->GetHeight();
```

### Texture Atlas

**Header:** `Pillar/Renderer/TextureAtlas.h`

```cpp
// Create atlas from texture file
auto atlas = Pillar::TextureAtlas::Create("spritesheet.png");

// Define sprites manually
atlas->AddSubTexture("player_idle", 
    Pillar::SubTexture::CreateFromGrid(0, 0, 32, 32, 256, 256));

// Or load from JSON (TexturePacker format)
atlas->LoadFromJSON("spritesheet.json");

// Get sprite for rendering
auto sprite = atlas->GetSubTexture("player_idle");

// Direct draw
Pillar::Renderer2D::DrawQuad(pos, size, color, atlas, "player_idle");
```

### Framebuffer (Render to Texture)

**Header:** `Pillar/Renderer/Framebuffer.h`

```cpp
Pillar::FramebufferSpecification spec;
spec.Width = 1280;
spec.Height = 720;
auto framebuffer = Pillar::Framebuffer::Create(spec);

// Render to framebuffer
framebuffer->Bind();
// ... render scene ...
framebuffer->Unbind();

// Get result as texture
uint32_t textureID = framebuffer->GetColorAttachmentRendererID();

// Resize on window resize
framebuffer->Resize(newWidth, newHeight);
```

### 2D Lighting

**Header:** `Pillar/Renderer/Lighting2D.h`

```cpp
Pillar::Lighting2D::Init();

Pillar::Lighting2DSettings settings;
settings.AmbientColor = {0.1f, 0.1f, 0.15f};
settings.AmbientIntensity = 0.15f;
settings.EnableShadows = true;

// Begin lit scene
Pillar::Lighting2D::BeginScene(camera, viewportWidth, viewportHeight, settings);

// Draw your scene here with Renderer2D::DrawQuad()
// ...

// Submit lights
Pillar::Light2DSubmit light;
light.Position = {5.0f, 3.0f};
light.Color = {1.0f, 0.8f, 0.6f};
light.Radius = 10.0f;
light.Intensity = 1.5f;
Pillar::Lighting2D::SubmitLight(light);

// Submit shadow casters
Pillar::ShadowCaster2DSubmit caster;
caster.WorldPoints = { {0,0}, {1,0}, {1,1}, {0,1} };
Pillar::Lighting2D::SubmitShadowCaster(caster);

Pillar::Lighting2D::EndScene();
```

### Post-Processing

**Header:** `Pillar/Renderer/PostProcessing.h`

```cpp
auto vignette = std::make_shared<Pillar::VignetteEffect>();
vignette->Init();
vignette->SetRadius(0.8f);
vignette->SetSoftness(0.4f);

auto bloom = std::make_shared<Pillar::BloomEffect>();
bloom->Init();
bloom->SetThreshold(0.8f);

// Apply effects
vignette->Apply(inputTextureID, outputFramebuffer);
```

---

## Entity Component System (ECS)

### Scene

**Header:** `Pillar/ECS/Scene.h`

```cpp
Pillar::Scene scene("MyLevel");

// Create entity
Pillar::Entity player = scene.CreateEntity("Player");

// Find entities
Pillar::Entity e = scene.FindEntityByName("Player");
auto entities = scene.GetAllEntities();

// Destroy entity
scene.DestroyEntity(player);

// Runtime control
scene.OnRuntimeStart();
scene.OnUpdate(deltaTime);
scene.OnRender();
scene.OnRuntimeStop();

// Query entities with specific components
scene.ForEach<TransformComponent, SpriteComponent>([](auto entity, auto& transform, auto& sprite) {
    // Process entities with both Transform and Sprite
});

// Get EnTT view for advanced usage
auto view = scene.GetEntitiesWith<TransformComponent, VelocityComponent>();
```

### Entity

**Header:** `Pillar/ECS/Entity.h`

```cpp
Pillar::Entity entity = scene.CreateEntity("MyEntity");

// Add components
auto& transform = entity.AddComponent<Pillar::TransformComponent>();
auto& sprite = entity.AddComponent<Pillar::SpriteComponent>();

// Get components
auto& t = entity.GetComponent<Pillar::TransformComponent>();

// Check if has component
if (entity.HasComponent<Pillar::SpriteComponent>()) { ... }

// Safe get (returns nullptr if missing)
auto* rb = entity.TryGetComponent<Pillar::RigidbodyComponent>();

// Get or add
auto& vel = entity.GetOrAddComponent<Pillar::VelocityComponent>();

// Remove component
entity.RemoveComponent<Pillar::SpriteComponent>();

// Entity comparison
if (entity == otherEntity) { ... }
if (entity.IsValid()) { ... }
```

### Core Components

| Component | Header | Description |
|-----------|--------|-------------|
| `TransformComponent` | `ECS/Components/Core/TransformComponent.h` | Position, rotation, scale |
| `TagComponent` | `ECS/Components/Core/TagComponent.h` | Entity name |
| `UUIDComponent` | `ECS/Components/Core/UUIDComponent.h` | Unique identifier |
| `HierarchyComponent` | `ECS/Components/Core/HierarchyComponent.h` | Parent-child relationships |

#### TransformComponent

```cpp
auto& t = entity.GetComponent<Pillar::TransformComponent>();

// Position
t.SetPosition({5.0f, 3.0f});
t.Translate({1.0f, 0.0f});

// Rotation (radians)
t.SetRotation(glm::radians(45.0f));
t.SetRotationDegrees(45.0f);
t.Rotate(0.1f);

// Scale
t.SetScale({2.0f, 2.0f});
t.SetScale(1.5f);  // Uniform

// Get transform matrix
glm::mat4 matrix = t.GetTransform();

// Helper methods
glm::vec2 forward = t.GetForward();
glm::vec2 right = t.GetRight();
```

### Rendering Components

| Component | Description |
|-----------|-------------|
| `SpriteComponent` | 2D sprite with texture, color, UVs |
| `AnimationComponent` | Animation playback control |
| `CameraComponent` | In-game camera definition |
| `Light2DComponent` | 2D light source |
| `ShadowCaster2DComponent` | Shadow casting geometry |

#### SpriteComponent

```cpp
auto& sprite = entity.AddComponent<Pillar::SpriteComponent>();
sprite.Texture = Pillar::Texture2D::Create("player.png");
sprite.Color = {1, 1, 1, 1};  // Tint
sprite.Size = {1, 1};
sprite.ZIndex = 0.0f;
sprite.FlipX = false;
sprite.FlipY = false;
sprite.Layer = "Player";
sprite.OrderInLayer = 0;

// For sprite sheets
sprite.SetUVFromGrid(column, row, cellW, cellH, sheetW, sheetH);
```

#### AnimationComponent

```cpp
auto& anim = entity.AddComponent<Pillar::AnimationComponent>();
anim.Play("walk");
anim.Pause();
anim.Resume();
anim.Stop();
anim.PlaybackSpeed = 1.5f;

// Callbacks
anim.OnAnimationComplete = [](entt::entity e) { /* ... */ };
anim.OnAnimationEvent = [](const std::string& event, entt::entity e) { /* ... */ };
```

### Physics Components

| Component | Description |
|-----------|-------------|
| `RigidbodyComponent` | Physics body (Box2D integration) |
| `ColliderComponent` | Collision shape definition |
| `VelocityComponent` | Simple velocity for lightweight entities |

#### RigidbodyComponent

```cpp
auto& rb = entity.AddComponent<Pillar::RigidbodyComponent>();
rb.BodyType = b2_dynamicBody;  // or b2_kinematicBody, b2_staticBody
rb.FixedRotation = true;
rb.GravityScale = 1.0f;
rb.LinearDamping = 0.5f;
rb.IsBullet = true;  // CCD for fast objects
```

#### ColliderComponent

```cpp
// Circle collider
auto& col = entity.AddComponent<Pillar::ColliderComponent>(
    Pillar::ColliderComponent::Circle(0.5f)
);

// Box collider
auto& col = entity.AddComponent<Pillar::ColliderComponent>(
    Pillar::ColliderComponent::Box({0.5f, 0.5f})
);

// Properties
col.Density = 1.0f;
col.Friction = 0.3f;
col.Restitution = 0.2f;  // Bounciness
col.IsSensor = false;    // Trigger-only?
```

### Gameplay Components

| Component | Description |
|-----------|-------------|
| `ParticleEmitterComponent` | Particle system emitter |
| `BulletComponent` | Projectile data |
| `XPGemComponent` | Collectible item |

#### ParticleEmitterComponent

```cpp
auto& emitter = entity.AddComponent<Pillar::ParticleEmitterComponent>();
emitter.EmissionRate = 50.0f;  // particles/sec
emitter.Shape = Pillar::EmissionShape::Circle;
emitter.ShapeSize = {1.0f, 1.0f};
emitter.Speed = 5.0f;
emitter.Lifetime = 2.0f;
emitter.StartColor = {1, 0.5f, 0, 1};  // Orange
emitter.Gravity = {0, -2};
emitter.FadeOut = true;
```

### Audio Components

| Component | Description |
|-----------|-------------|
| `AudioSourceComponent` | Sound emitter on entity |
| `AudioListenerComponent` | 3D audio listener (usually on camera) |

```cpp
auto& audio = entity.AddComponent<Pillar::AudioSourceComponent>("explosion.wav");
audio.Volume = 0.8f;
audio.Pitch = 1.0f;
audio.Loop = false;
audio.Is3D = true;
audio.MinDistance = 5.0f;
audio.MaxDistance = 50.0f;
audio.PlayOnAwake = true;
```

### Systems

Systems process entities with specific components. Use the built-in systems or create custom ones.

| System | Description |
|--------|-------------|
| `PhysicsSystem` | Box2D physics simulation |
| `AnimationSystem` | Animation playback |
| `SpriteRenderSystem` | Sprite batch rendering |
| `ParticleEmitterSystem` | Particle emission |
| `ParticleSystem` | Particle simulation |
| `AudioSystem` | Audio source position updates |
| `Lighting2DSystem` | Light/shadow collection |

```cpp
// Create and attach physics system
Pillar::PhysicsSystem physics({0, -9.81f});
physics.OnAttach(&scene);

// In game loop
physics.OnUpdate(deltaTime);
```

### Object Pooling

**Header:** `Pillar/ECS/ObjectPool.h`

```cpp
Pillar::ObjectPool bulletPool;
bulletPool.Init(&scene, 100);  // Pre-allocate 100

// Get entity from pool
Pillar::Entity bullet = bulletPool.Acquire();

// When done, return to pool
bulletPool.Release(bullet);

// Stats
size_t available = bulletPool.GetAvailableCount();
size_t active = bulletPool.GetActiveCount();
```

---

## Audio System

### AudioEngine (Static API)

**Header:** `Pillar/Audio/AudioEngine.h`

```cpp
// Lifecycle (called automatically by Application)
Pillar::AudioEngine::Init();
Pillar::AudioEngine::Shutdown();

// Master volume
Pillar::AudioEngine::SetMasterVolume(0.8f);

// One-shot playback (fire and forget)
Pillar::AudioEngine::PlayOneShot("explosion.wav", 1.0f);

// With 3D position
Pillar::AudioEngine::PlayOneShot("explosion.wav", 1.0f, 1.0f, glm::vec3(5, 0, 0));

// 3D Listener (usually follows camera)
Pillar::AudioEngine::SetListenerPosition({cameraX, cameraY, 0});
Pillar::AudioEngine::SetListenerOrientation({0, 0, -1}, {0, 1, 0});

// Global control
Pillar::AudioEngine::StopAllSounds();
Pillar::AudioEngine::PauseAllSounds();
Pillar::AudioEngine::ResumeAllSounds();
```

### AudioBuffer & AudioSource (Manual Control)

```cpp
// Load audio data
auto buffer = Pillar::AudioEngine::CreateBuffer("music.wav");

// Create source
auto source = Pillar::AudioEngine::CreateSource();
source->SetBuffer(buffer);
source->SetLooping(true);
source->SetVolume(0.7f);
source->Play();

// Playback control
source->Pause();
source->Resume();
source->Stop();

// 3D positioning
source->SetPosition({x, y, z});
source->SetMinDistance(5.0f);
source->SetMaxDistance(50.0f);

// State queries
bool isPlaying = source->IsPlaying();
```

### AudioClip (Simple Wrapper)

**Header:** `Pillar/Audio/AudioClip.h`

```cpp
auto clip = Pillar::AudioClip::Create("jump.wav");
clip->SetVolume(0.5f);
clip->Play();
```

---

## Input System

**Header:** `Pillar/Input.h`, `Pillar/KeyCodes.h`

### Keyboard

```cpp
// Held this frame
if (Pillar::Input::IsKeyDown(PIL_KEY_W)) { /* move forward */ }

// Just pressed this frame (rising edge)
if (Pillar::Input::IsKeyJustPressed(PIL_KEY_SPACE)) { /* jump */ }

// Just released this frame
if (Pillar::Input::IsKeyJustReleased(PIL_KEY_E)) { /* end interaction */ }
```

### Mouse

```cpp
// Button state
if (Pillar::Input::IsMouseButtonDown(PIL_MOUSE_BUTTON_LEFT)) { /* fire */ }
if (Pillar::Input::IsMouseButtonJustPressed(PIL_MOUSE_BUTTON_RIGHT)) { /* aim */ }

// Position
auto [x, y] = Pillar::Input::GetMousePosition();
auto [dx, dy] = Pillar::Input::GetMouseDelta();
auto [scrollX, scrollY] = Pillar::Input::GetScrollDelta();

// Cursor control
Pillar::Input::SetCursorMode(Pillar::CursorMode::Locked);
Pillar::Input::SetMousePosition(400, 300);
```

### Action Bindings

```cpp
// Bind multiple keys/buttons to an action
Pillar::Input::BindAction("Jump", {PIL_KEY_SPACE, PIL_KEY_W}, {});
Pillar::Input::BindAction("Fire", {}, {PIL_MOUSE_BUTTON_LEFT});

// Check action state
if (Pillar::Input::IsActionPressed("Jump")) { /* jump */ }
if (Pillar::Input::IsActionReleased("Fire")) { /* stop firing */ }
```

### Common Key Codes

```cpp
// Letters: PIL_KEY_A through PIL_KEY_Z
// Numbers: PIL_KEY_0 through PIL_KEY_9
// Function: PIL_KEY_F1 through PIL_KEY_F12
// Arrows: PIL_KEY_LEFT, PIL_KEY_RIGHT, PIL_KEY_UP, PIL_KEY_DOWN
// Modifiers: PIL_KEY_LEFT_SHIFT, PIL_KEY_LEFT_CONTROL, PIL_KEY_LEFT_ALT
// Special: PIL_KEY_SPACE, PIL_KEY_ENTER, PIL_KEY_ESCAPE, PIL_KEY_TAB
// Mouse: PIL_MOUSE_BUTTON_LEFT, PIL_MOUSE_BUTTON_RIGHT, PIL_MOUSE_BUTTON_MIDDLE
```

---

## Scene Management

### SceneManager

**Header:** `Pillar/ECS/SceneManager.h`

```cpp
auto& sm = Pillar::SceneManager::Get();

// Create scenes
auto menuScene = sm.CreateScene("MainMenu");
auto gameScene = sm.CreateScene("Level1");

// Switch scenes
sm.SetActiveScene("Level1");

// Request scene change (safe, happens at end of frame)
sm.RequestSceneChange("GameOver");

// Load from file
sm.LoadScene("assets/scenes/level1.scene", "Level1");

// Save current scene
sm.SaveScene("assets/scenes/level1.scene");

// Callbacks
sm.SetOnSceneChangeCallback([](const std::string& from, const std::string& to) {
    PIL_INFO("Scene changed: {} -> {}", from, to);
});
```

### SceneSerializer

**Header:** `Pillar/ECS/SceneSerializer.h`

```cpp
Pillar::SceneSerializer serializer(&scene);

// Save to JSON (human-readable)
serializer.Serialize("level.scene");

// Load from JSON
serializer.Deserialize("level.scene");

// Binary (faster, for runtime)
serializer.SerializeBinary("level.bin");
serializer.DeserializeBinary("level.bin");

// String (for clipboard, network)
std::string data = serializer.SerializeToString();
serializer.DeserializeFromString(data);
```

---

## Utility Classes

### AssetManager

**Header:** `Pillar/Utils/AssetManager.h`

```cpp
// Get resolved path (searches multiple locations)
std::string path = Pillar::AssetManager::GetAssetPath("config.json");

// Specialized resolvers
std::string tex = Pillar::AssetManager::GetTexturePath("player.png");
std::string audio = Pillar::AssetManager::GetAudioPath("music.wav");
std::string sfx = Pillar::AssetManager::GetSFXPath("shoot.wav");
std::string music = Pillar::AssetManager::GetMusicPath("theme.wav");

// Manual override
Pillar::AssetManager::SetAssetsDirectory("C:/MyGame/assets");
```

### Random

**Header:** `Pillar/Utils/Random.h`

```cpp
using namespace Pillar::Random;

// Seed for deterministic runs
Seed(12345);

// Random values
float f = Float01();           // [0, 1]
float f2 = Float(-5, 5);       // [-5, 5]
float angle = AngleRadians();  // [0, 2π)
glm::vec2 dir = Direction2D(); // Unit vector
```

### Math2D

**Header:** `Pillar/Utils/Math2D.h`

Common 2D math utilities (distance, angle, lerp, etc.).

---

## Quick Reference Tables

### All Components

| Category | Component | Key Fields |
|----------|-----------|------------|
| **Core** | `TransformComponent` | Position, Rotation, Scale |
| | `TagComponent` | Name |
| | `UUIDComponent` | UUID |
| | `HierarchyComponent` | Parent, Children |
| **Rendering** | `SpriteComponent` | Texture, Color, Size, ZIndex, Layer |
| | `AnimationComponent` | CurrentClipName, PlaybackSpeed, Playing |
| | `CameraComponent` | OrthographicSize, Primary |
| | `Light2DComponent` | Color, Intensity, Radius, CastShadows |
| | `ShadowCaster2DComponent` | Points, Closed |
| **Physics** | `RigidbodyComponent` | BodyType, FixedRotation, GravityScale |
| | `ColliderComponent` | Type, Radius/HalfExtents, Density, IsSensor |
| | `VelocityComponent` | Velocity, AngularVelocity |
| **Gameplay** | `ParticleEmitterComponent` | EmissionRate, Shape, Speed, Lifetime |
| | `BulletComponent` | Damage, Speed, Lifetime |
| | `XPGemComponent` | Value |
| **Audio** | `AudioSourceComponent` | AudioFile, Volume, Loop, Is3D |
| | `AudioListenerComponent` | (marker component) |

### All Systems

| System | Processes | Description |
|--------|-----------|-------------|
| `PhysicsSystem` | Rigidbody, Collider | Box2D physics simulation |
| `PhysicsSyncSystem` | Rigidbody, Transform | Sync Box2D ↔ ECS |
| `VelocityIntegrationSystem` | Velocity, Transform | Simple physics (no Box2D) |
| `AnimationSystem` | Animation, Sprite | Animation playback |
| `SpriteRenderSystem` | Transform, Sprite | Batched sprite rendering |
| `ParticleEmitterSystem` | ParticleEmitter | Particle spawning |
| `ParticleSystem` | Particle | Particle simulation |
| `AudioSystem` | AudioSource, Transform | 3D audio positioning |
| `Lighting2DSystem` | Light2D, ShadowCaster2D | Collect lights/shadows |

### Keyboard Quick Reference

| Action | Keys |
|--------|------|
| Movement | `PIL_KEY_W`, `PIL_KEY_A`, `PIL_KEY_S`, `PIL_KEY_D` |
| Jump | `PIL_KEY_SPACE` |
| Interact | `PIL_KEY_E` |
| Escape | `PIL_KEY_ESCAPE` |
| Shift | `PIL_KEY_LEFT_SHIFT` |
| Control | `PIL_KEY_LEFT_CONTROL` |

---

## Version

This document reflects Pillar Engine as of January 2026.

For build instructions and project setup, see [USERS_GUIDE.md](USERS_GUIDE.md).
