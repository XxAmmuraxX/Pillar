# Pillar Engine API Guide (Minimal)

This is a minimal “how to use the engine” guide meant to be practical: what to include, what to override, and the smallest set of APIs you need to build a game.

## 0) The one include you usually want

Most client code can include:

```cpp
#include "Pillar.h"
```

That umbrella header pulls in common engine headers (Application, Layer, Input, Renderer, Audio, etc.).

## 1) Creating an application (entry point pattern)

Pillar provides `main()` in `Pillar/EntryPoint.h`. Your app:

1. Includes the engine umbrella header
2. Includes the entry point header **in the final executable only**
3. Implements `Pillar::CreateApplication()` to return your `Pillar::Application`

Minimal example (modeled after Sandbox):

```cpp
// Source.cpp (in your executable project)
#include "Pillar.h"
#include "Pillar/EntryPoint.h"

class MyGameApp : public Pillar::Application
{
public:
    MyGameApp()
    {
        PushLayer(new MyGameLayer());
        // PushOverlay(...) is also available
    }
};

Pillar::Application* Pillar::CreateApplication()
{
    return new MyGameApp();
}
```

### What the engine already does for you

`Pillar::Application`:
- Creates a window (`Window::Create(...)`)
- Hooks GLFW callbacks to the engine event system
- Initializes audio (`AudioEngine::Init()`)
- Initializes rendering (`Renderer::Init()` + `Renderer2DBackend::Init()`)
- Pushes an ImGui overlay layer

## 2) Layers: your main extension point

Derive from `Pillar::Layer` and override what you need:

- `OnAttach()` / `OnDetach()` — setup/cleanup
- `OnUpdate(float dt)` — per-frame logic (dt in seconds)
- `OnEvent(Pillar::Event& e)` — input/window events
- `OnImGuiRender()` — draw UI (engine begins/ends the ImGui frame)

Minimal layer:

```cpp
class MyGameLayer : public Pillar::Layer
{
public:
    MyGameLayer() : Layer("MyGameLayer"), m_CameraController(16.0f/9.0f) {}

    void OnAttach() override
    {
        m_Texture = Pillar::Texture2D::Create("textures/player.png");
    }

    void OnUpdate(float dt) override
    {
        m_CameraController.OnUpdate(dt);

        Pillar::Renderer::SetClearColor({0.1f, 0.1f, 0.12f, 1.0f});
        Pillar::Renderer::Clear();

        Pillar::Renderer2DBackend::BeginScene(m_CameraController.GetCamera());
        Pillar::Renderer2DBackend::DrawQuad({0.0f, 0.0f}, {1.0f, 1.0f}, {1,1,1,1}, m_Texture);
        Pillar::Renderer2DBackend::EndScene();
    }

    void OnEvent(Pillar::Event& e) override
    {
        m_CameraController.OnEvent(e);
    }

private:
    Pillar::OrthographicCameraController m_CameraController;
    std::shared_ptr<Pillar::Texture2D> m_Texture;
};
```

## 3) Events (window / keyboard / mouse)

### How propagation works

- Events are produced by platform code (GLFW callbacks)
- `Application::OnEvent` dispatches some engine-level events (like `WindowCloseEvent`, `WindowResizeEvent`)
- Then events are sent to layers **from top → bottom** until `event.Handled` is set

### Handling a specific event type

Use `EventDispatcher`:

```cpp
void MyGameLayer::OnEvent(Pillar::Event& e)
{
    Pillar::EventDispatcher d(e);

    d.Dispatch<Pillar::KeyPressedEvent>([&](Pillar::KeyPressedEvent& ke)
    {
        if (ke.GetRepeatCount() == 0)
            PIL_INFO("Key pressed: {}", ke.GetKeyCode());
        return false; // set true to stop propagation
    });
}
```

## 4) Input polling (quick & simple)

If you don’t need event-based input, you can poll:

```cpp
if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE))
{
    // jump
}
```

Key codes live in `Pillar/KeyCodes.h` (e.g. `PIL_KEY_W`, `PIL_KEY_ESCAPE`).

## 5) Rendering

Pillar has two common rendering “levels”:

### A) High-level 2D batch renderer (`Renderer2DBackend`)

Best for sprites/quads.

Core calls:
- `Renderer2DBackend::BeginScene(camera)`
- `Renderer2DBackend::DrawQuad(...)`
- `Renderer2DBackend::EndScene()`

Examples:

```cpp
// colored quad
Pillar::Renderer2DBackend::DrawQuad({0.0f, 0.0f}, {2.0f, 2.0f}, {0.2f, 0.7f, 0.3f, 1.0f});

// textured quad (full texture)
Pillar::Renderer2DBackend::DrawQuad({1.0f, 0.0f}, {1.0f, 1.0f}, {1,1,1,1}, texture);

// textured quad (sub-rect for sprite sheets)
Pillar::Renderer2DBackend::DrawQuad(
    {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {1,1,1,1}, texture,
    {0.0f, 0.0f}, {0.5f, 0.5f},
    /*flipX=*/false, /*flipY=*/false
);
```

### B) Lower-level renderer (`Renderer`, `Shader`, `VertexArray`)

Use this when you want custom geometry/shaders.

Typical flow:

```cpp
Pillar::Renderer::BeginScene(camera);
shader->Bind();
shader->SetMat4("u_Transform", transform);
Pillar::Renderer::Submit(shader, vertexArray);
Pillar::Renderer::EndScene();
```

## 6) Cameras

### OrthographicCamera (manual)

Create one and drive it yourself:

```cpp
Pillar::OrthographicCamera camera(-1.6f, 1.6f, -0.9f, 0.9f);
camera.SetPosition({0,0,0});
camera.SetRotation(15.0f); // degrees in this API
```

### OrthographicCameraController (recommended)

Handles WASD + mouse-wheel zoom and window resize events:

```cpp
Pillar::OrthographicCameraController controller(16.0f/9.0f, /*rotation=*/true);

controller.OnUpdate(dt);
controller.OnEvent(event);

Pillar::Renderer2DBackend::BeginScene(controller.GetCamera());
```

## 7) Textures & assets

Create a texture:

```cpp
auto texture = Pillar::Texture2D::Create("textures/checkerboard.png");
```

Paths are resolved via `AssetManager`:
- During development: looks under the repository’s `Sandbox/assets/`
- In a packaged build: falls back to `assets/` next to the executable

If you need an absolute/fully-resolved path for your own loader:

```cpp
std::string fullPath = Pillar::AssetManager::GetAssetPath("scenes/level01.scene.json");
```

## 8) Audio

The engine initializes the audio engine inside `Pillar::Application`.

### Easiest: AudioClip

```cpp
auto clip = Pillar::AudioClip::Create("audio/sfx/explosion.wav");
if (clip && clip->IsLoaded())
{
    clip->SetVolume(0.8f);
    clip->Play();
}
```

### Lower-level: AudioBuffer + AudioSource

```cpp
auto buffer = Pillar::AudioBuffer::Create("audio/sfx/laser.wav");
auto source = Pillar::AudioSource::Create();
source->SetBuffer(buffer);
source->SetLooping(false);
source->Play();
```

## 9) ECS & Scenes (EnTT)

### Create a scene and entities

```cpp
auto scene = std::make_shared<Pillar::Scene>("Level01");

Pillar::Entity player = scene->CreateEntity("Player");
player.AddComponent<Pillar::SpriteComponent>(Pillar::Texture2D::Create("textures/player.png"));

auto& transform = player.GetComponent<Pillar::TransformComponent>();
transform.Position = { 2.0f, 1.0f };
transform.Dirty = true; // mark cached matrix dirty if you edit fields directly
```

### Manage scenes via SceneManager

```cpp
auto& sm = Pillar::SceneManager::Get();
sm.CreateScene("MainMenu");
sm.LoadScene("scenes/level01.scene.json", "Level01");
sm.SetActiveScene("Level01");

// per frame
sm.OnUpdate(dt);
```

### Save/load scenes (JSON)

```cpp
auto scene = Pillar::SceneManager::Get().GetActiveScene();
Pillar::SceneSerializer serializer(scene.get());
serializer.Serialize("scenes/saved.scene.json");
serializer.Deserialize("scenes/saved.scene.json");
```

### Registering your own component for serialization

```cpp
struct HealthComponent { int HP = 100; };

Pillar::ComponentRegistry::Get().Register<HealthComponent>(
    "health",
    [](Pillar::Entity e) -> Pillar::json {
        if (!e.HasComponent<HealthComponent>()) return nullptr;
        return Pillar::json{ {"hp", e.GetComponent<HealthComponent>().HP} };
    },
    [](Pillar::Entity e, const Pillar::json& j) {
        auto& c = e.AddComponent<HealthComponent>();
        c.HP = j.value("hp", 100);
    },
    [](Pillar::Entity src, Pillar::Entity dst) {
        if (!src.HasComponent<HealthComponent>()) return;
        dst.AddComponent<HealthComponent>().HP = src.GetComponent<HealthComponent>().HP;
    }
);
```

## 10) ImGui

Put UI in your layer’s `OnImGuiRender()`:

```cpp
void MyGameLayer::OnImGuiRender()
{
    ImGui::Begin("Debug");
    ImGui::Text("Hello from Pillar");
    ImGui::End();
}
```

If you want mouse/keyboard to keep going to the game while hovering UI:

```cpp
Pillar::Application::Get().GetImGuiLayer()->SetBlockEvents(false);
```

---

## Quick mental model (one frame)

1. Poll events (GLFW) → dispatch to layers
2. `Layer::OnUpdate(dt)` for each layer
3. Begin ImGui → `Layer::OnImGuiRender()` → End ImGui
4. Swap buffers

For complete working examples, see Sandbox layers like `ExampleLayer` and `SceneDemoLayer`.
