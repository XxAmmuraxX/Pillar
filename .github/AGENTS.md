# Pillar Engine — Agent Notes

This file is for automated agents (Copilot, scripts) working in this repository.

## What this repo is

- **Pillar**: C++17 engine library (static lib) providing:
  - Application + layer system
  - Event system (window/keyboard/mouse/audio)
  - OpenGL renderer + 2D batch renderer
  - Asset path resolution
  - Audio (OpenAL)
  - ECS (EnTT) + scene manager + JSON serializer
- **Sandbox**: example/game-like app using the engine.
- **PillarEditor**: editor application.
- **Tests**: GoogleTest unit tests.

## Canonical build & test commands (Windows / Ninja)

Use these exact commands (PowerShell) from the repo root:

- Configure:
  - `cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug`
- Build:
  - `cmake --build out/build/x64-Debug --config Debug --parallel`
- Run Sandbox:
  - `./bin/Debug-x64/Sandbox/SandboxApp.exe`
- Run tests:
  - `./bin/Debug-x64/Tests/PillarTests.exe`

If configuration fails with a GLAD2/Jinja2 error:
- `python -m pip install jinja2`

## How the engine runs (high level)

- `main()` lives in `Pillar/EntryPoint.h` and calls:
  - `Pillar::Logger::Init()`
  - `Pillar::CreateApplication()` (implemented by the client app)
  - `app->Run()`
- `Pillar::Application` creates the window, hooks the GLFW event callback, initializes:
  - Audio (`AudioEngine::Init()`)
  - Rendering (`Renderer::Init()` and `Renderer2DBackend::Init()`)
  - ImGui overlay layer
- Per frame (`Application::Run()`):
  - compute `deltaTime`
  - clear screen (`Renderer::SetClearColor`, `Renderer::Clear`)
  - call `OnUpdate(dt)` for each layer (bottom→top)
  - begin/end ImGui frame and call each layer’s `OnImGuiRender()`
  - `Window::OnUpdate()` polls events and swaps buffers
- Events flow:
  - GLFW callbacks create `Event` objects → `WindowData::EventCallback` → `Application::OnEvent` → layers (top→bottom) until `event.Handled`.

## Patterns to follow when changing code

- Prefer small, surgical changes; keep public API stable.
- When adding a new event type:
  - Add enum entry in `Pillar/Events/Event.h`.
  - Add event class in the right header.
  - Wire GLFW callback in `Platform/WindowsWindow.cpp` if it’s a platform event.
- Event handlers used with `EventDispatcher::Dispatch<T>()` must take `T&` and return `bool`.
- Avoid hard-coded absolute paths. Use `AssetManager` for asset resolution.
- Keep C++ standard at C++17.

## Documentation targets

- Public API umbrella include: `Pillar/Pillar.h`
- Minimal engine usage guide: see `ENGINE_API_GUIDE.md`
