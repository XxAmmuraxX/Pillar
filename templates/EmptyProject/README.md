# Pillar Engine - Empty Project Template

A minimal Pillar Engine project template to get you started quickly.

## Quick Start

### Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2022 or compatible C++17 compiler
- CMake 3.16+
- Ninja build system (recommended)
- Pillar SDK installed

### Building Your Game

1. **Set SDK path** (if not already set):
```powershell
$env:PILLAR_SDK_DIR = "C:\path\to\PillarSDK-0.1.0-Windows-x64"
# Optional: Make it permanent
[System.Environment]::SetEnvironmentVariable("PILLAR_SDK_DIR", $env:PILLAR_SDK_DIR, "User")
```

2. **Configure the project**:
```powershell
cmake --preset default
```

3. **Build**:
```powershell
cmake --build --preset default
```

4. **Run**:
```powershell
.\build\Debug\EmptyPillarProject.exe
```

### Project Structure

```
EmptyPillarProject/
├── src/
│   ├── main.cpp           # Application entry point
│   ├── GameLayer.h        # Your main game layer (header)
│   └── GameLayer.cpp      # Your main game layer (implementation)
├── assets/
│   ├── textures/          # Place texture files here
│   ├── audio/             # Place audio files here
│   ├── scenes/            # Scene files (JSON)
│   └── templates/         # Entity templates
├── build/                 # Generated build files (git-ignored)
├── CMakeLists.txt         # CMake configuration
├── CMakePresets.json      # CMake presets for easy building
└── README.md              # This file
```

## Development Workflow

### Adding Game Code

Your game logic lives in `GameLayer.cpp`. Override these methods:

- **`OnAttach()`** - Initialize resources (textures, audio, scenes)
- **`OnUpdate(float dt)`** - Update game logic each frame
- **`OnEvent(Event& e)`** - Handle input events
- **`OnImGuiRender()`** - Draw debug UI

**Example:**
```cpp
void GameLayer::OnAttach()
{
    // Load assets
    m_Texture = Pillar::Texture2D::Create("player.png");
    
    // Setup camera
    m_CameraController = Pillar::OrthographicCameraController(16.0f/9.0f, true);
}

void GameLayer::OnUpdate(float dt)
{
    // Update camera
    m_CameraController.OnUpdate(dt);
    
    // Render
    Pillar::Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Pillar::Renderer::Clear();
    
    Pillar::Renderer2D::BeginScene(m_CameraController.GetCamera());
    Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f }, { 1.0f, 1.0f }, m_Texture.get());
    Pillar::Renderer2D::EndScene();
}
```

### Adding Assets

1. **Textures:** Place `.png`, `.jpg`, or other image files in `assets/textures/`
2. **Audio:** Place `.wav` files in `assets/audio/`
3. **Access in code:**
```cpp
// Textures are automatically found in assets/textures/
auto texture = Pillar::Texture2D::Create("my_sprite.png");

// Audio files are automatically found in assets/audio/
auto clip = Pillar::AudioClip::Create("explosion.wav");
```

### Building Configurations

```powershell
# Debug (default) - with symbols, slower
cmake --preset default
cmake --build --preset default

# Release - optimized, faster
cmake --preset release
cmake --build --preset release

# Clean rebuild
Remove-Item -Recurse -Force build
cmake --preset default
cmake --build --preset default
```

## Documentation

- **User's Guide:** See `$env:PILLAR_SDK_DIR/docs/USERS_GUIDE.md`
- **API Reference:** See `$env:PILLAR_SDK_DIR/docs/API_REFERENCE.md`
- **Editor Guide:** See `$env:PILLAR_SDK_DIR/docs/PILLAR_EDITOR_GUIDE.md`

## Troubleshooting

### "Pillar SDK requires Microsoft Visual C++ (MSVC) compiler"

The Pillar SDK is built with MSVC and requires MSVC for linking due to ABI compatibility.

**Solutions:**
1. Open **Developer PowerShell for VS 2022** and run cmake from there
2. Use `cmake --preset default` (the preset forces MSVC)
3. Set compiler explicitly: `-DCMAKE_CXX_COMPILER=cl`

### "Could not find Pillar" error

**Solution:** Set `PILLAR_SDK_DIR` environment variable:
```powershell
$env:PILLAR_SDK_DIR = "C:\path\to\PillarSDK"
```

### Build fails with "cl.exe not found"

**Solution:** Use Developer PowerShell for VS 2022, or run:
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1"
```

### Assets not found at runtime

**Solution:** Assets are automatically copied to the build directory. Ensure:
1. Files are in `assets/textures/` or `assets/audio/`
2. You're running from the build directory
3. File names match exactly (case-sensitive)

### Shader errors

Shaders are included in the SDK at `$env:PILLAR_SDK_DIR/assets/shaders/`. If you see shader loading errors, verify the SDK directory structure is intact.

## Next Steps

1. **Read the documentation:** Start with the User's Guide
2. **Explore examples:** Check `Sandbox/` in the engine source for more complex examples
3. **Try the Editor:** Run `PillarEditor.exe` from `$env:PILLAR_SDK_DIR/editor/`
4. **Join the community:** Report issues on GitHub

## License

See `LICENSE.txt` in the Pillar SDK directory.

---

**Happy game development with Pillar Engine!** 🎮
