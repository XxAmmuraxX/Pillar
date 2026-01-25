# Pillar Engine

<p align="center">
  <strong>A modern C++ game engine framework with OpenGL rendering, audio, physics, and ECS</strong>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue.svg" alt="C++17">
  <img src="https://img.shields.io/badge/OpenGL-4.6-green.svg" alt="OpenGL 4.6">
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey.svg" alt="Windows">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="MIT License">
</p>

---

## Features

- **Rendering System** - OpenGL 4.6 abstraction with 2D batch renderer, texture support, and orthographic camera
- **Audio System** - OpenAL-Soft backend with 2D/3D spatial audio, WAV loading, and volume/pitch controls
- **Entity Component System** - EnTT-based ECS with scene management and component architecture
- **Physics System** - Box2D integration with rigidbodies, colliders, and spatial hash grid
- **Event System** - Type-safe event dispatching with keyboard, mouse, window, and audio events
- **Layer Architecture** - Stackable layers for modular game logic
- **ImGui Integration** - Built-in Dear ImGui (docking branch) for debug UI and tools
- **Input System** - Polling API for keyboard and mouse input
- **Asset Management** - Automatic path resolution for development and distribution

## Prerequisites

- **Visual Studio 2022** with C++ development tools
- **CMake 3.21+**
- **Ninja 1.12.1+**
- **Python 3.x** with jinja2 (`pip install jinja2`)

## Quick Start

### 1. Clone the Repository

```powershell
git clone https://github.com/yourusername/Pillar.git
cd Pillar
```

### 2. Bootstrap (First-Time Setup)

```powershell
.\scripts\bootstrap.ps1
```

This validates prerequisites, sets up the MSVC environment, and configures CMake.

### 3. Build

```powershell
cmake --build --preset windows-debug
```

### 4. Run

```powershell
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

### 5. Run Tests

```powershell
ctest --preset windows-debug
```

## Project Structure

```
Pillar/
├── Pillar/          # Core engine library (static)
│   └── src/
│       ├── Pillar/          # Public API headers
│       │   ├── Events/      # Event system
│       │   ├── Renderer/    # Rendering abstraction
│       │   ├── Audio/       # Audio system
│       │   ├── ECS/         # Entity Component System
│       │   └── Utils/       # Utilities (AssetManager, etc.)
│       └── Platform/        # Platform-specific implementations
│           ├── OpenGL/      # OpenGL rendering backend
│           └── OpenAL/      # OpenAL audio backend
├── PillarEditor/    # Visual scene editor application
├── Sandbox/         # Example application
├── Tests/           # Unit tests (Google Test)
├── docs/            # Documentation
├── scripts/         # Build and utility scripts
└── templates/       # Project templates
```

## 🎮 Creating a Game

### Using the SDK

1. Build the SDK:
   ```powershell
   .\scripts\build-sdk.ps1 -Config Release -CreateZip
   ```

2. Use the template project in `templates/EmptyProject/` as a starting point

### Basic Example

```cpp
#include <Pillar.h>

class GameLayer : public Pillar::Layer
{
public:
    void OnAttach() override
    {
        m_Texture = Pillar::Texture2D::Create("player.png");
    }

    void OnUpdate(float dt) override
    {
        Pillar::Renderer2D::BeginScene(m_Camera);
        Pillar::Renderer2D::DrawQuad({0.0f, 0.0f}, {1.0f, 1.0f}, m_Texture);
        Pillar::Renderer2D::EndScene();
    }

private:
    Pillar::OrthographicCamera m_Camera{-1.6f, 1.6f, -0.9f, 0.9f};
    std::shared_ptr<Pillar::Texture2D> m_Texture;
};

Pillar::Application* Pillar::CreateApplication()
{
    auto app = new Pillar::Application();
    app->PushLayer(new GameLayer());
    return app;
}
```

## CMake Presets

| Preset | Description |
|--------|-------------|
| `windows-debug` | Debug build with full symbols |
| `windows-release` | Optimized release build |
| `windows-relwithdebinfo` | Release with debug info |
| `ci` | CI/CD build configuration |

```powershell
# Configure and build with a preset
cmake --preset windows-release
cmake --build --preset windows-release
```

## Documentation

- [Quick Start Guide](docs/QUICK_START.md)
- [User's Guide](docs/USERS_GUIDE.md)
- [API Reference](docs/API_REFERENCE.md)
- [Installation Guide](docs/INSTALLATION_GUIDE.md)
- [Pillar Editor Guide](docs/PILLAR_EDITOR_GUIDE.md)

### Engine Subsystem Docs

- [Rendering System Guide](Pillar/docs/Rendering_System_Guide.md)
- [Animation System Guide](Pillar/docs/Animation_System_Guide.md)
- [Audio System Integration](Pillar/docs/AUDIO_SYSTEM_INTEGRATION.md)
- [Object Pooling](Pillar/docs/ObjectPooling.md)
- [Batch Renderer Usage](Pillar/docs/BatchRenderer_Usage.md)

## Testing

The project uses Google Test for unit testing.

```powershell
# Run all tests
ctest --preset windows-debug

# Run with verbose output
ctest --preset windows-debug --output-on-failure

# Run specific test suite
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.*
```

See [Tests/README.md](Tests/README.md) for detailed testing documentation.

## Dependencies

All dependencies are fetched automatically via CMake FetchContent:

| Dependency | Version | Purpose |
|------------|---------|---------|
| [GLFW](https://www.glfw.org/) | 3.4 | Window management |
| [spdlog](https://github.com/gabime/spdlog) | 1.13.0 | Logging |
| [Dear ImGui](https://github.com/ocornut/imgui) | docking | Debug UI |
| [GLAD2](https://github.com/Dav1dde/glad) | 2.0.8 | OpenGL loader |
| [GLM](https://github.com/g-truc/glm) | 1.0.1 | Math library |
| [stb_image](https://github.com/nothings/stb) | master | Image loading |
| [OpenAL-Soft](https://github.com/kcat/openal-soft) | 1.24.3 | Audio backend |
| [EnTT](https://github.com/skypjack/entt) | 3.13.2 | ECS library |
| [Box2D](https://github.com/erincatto/box2d) | 2.4.1 | Physics engine |
| [Google Test](https://github.com/google/googletest) | 1.14.0 | Testing framework |

## Troubleshooting

### Missing Python or jinja2
```powershell
python -m pip install jinja2
```

### cl.exe not found
Use Developer PowerShell for VS 2022 or run `.\scripts\bootstrap.ps1`

### Build Issues
```powershell
# Clean rebuild
Remove-Item -Path build\windows-debug -Recurse -Force -ErrorAction SilentlyContinue
cmake --preset windows-debug
cmake --build --preset windows-debug
```

## License

This project is licensed under the MIT License - see the [LICENSE.txt](LICENSE.txt) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

---

<p align="center">
  Built with ❤️ for game developers
</p>
