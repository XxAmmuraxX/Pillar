# Pillar Engine - Empty Project Template

A minimal Pillar Engine project template to get you started quickly.

## Quick Start

### Prerequisites

- Windows 10/11 (64-bit)
- Visual Studio 2022+ with C++ workload (Build Tools or full IDE)
- CMake 3.25+
- Ninja build system
- Pillar SDK installed
- (Optional) LLVM/Clang for alternative compiler

### Building Your Game

**Option 1: Use build.ps1 (Recommended)**

The build script automatically sets up the MSVC environment - just run it from any PowerShell:

```powershell
# Set SDK path (one-time setup)
$env:PILLAR_SDK_DIR = "C:\path\to\PillarSDK-0.1.0-Windows-x64"

# Build Release (default)
.\build.ps1

# Build Debug
.\build.ps1 -Config Debug

# Build Release with debug symbols (for profiling)
.\build.ps1 -Config RelWithDebInfo

# Clean rebuild
.\build.ps1 -Clean
```

**Option 2: Manual CMake (from Developer PowerShell)**

If you're already in Developer PowerShell for VS:

```powershell
cmake --preset default
cmake --build --preset default
```

**Option 3: Clang (Alternative Compiler)**

If you have LLVM installed and in PATH (no vcvars needed):

```powershell
cmake --preset clang-release
cmake --build --preset clang-release
```

### Running Your Game

```powershell
.\build\Release\EmptyPillarProject.exe
```

## Available Build Presets

| Preset | Compiler | Description |
|--------|----------|-------------|
| `default` | MSVC | **Recommended.** Release build. Use `build.ps1` for auto-setup. |
| `debug` | MSVC | Debug build (requires Debug SDK libraries). |
| `relwithdebinfo` | MSVC | Release with debug symbols for profiling. |
| `clang-release` | Clang | Alternative compiler. Requires LLVM in PATH. |
| `clang-debug` | Clang | Debug with Clang. |

**Build preset usage:**
```powershell
# Using build.ps1 (recommended - handles environment automatically)
.\build.ps1                         # Release (default)
.\build.ps1 -Config Debug           # Debug build
.\build.ps1 -Config RelWithDebInfo  # Release with debug symbols

# Or manually (requires Developer PowerShell or MSVC in PATH)
cmake --preset default
cmake --build --preset default
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

The SDK includes both **Debug** and **Release** libraries.

**Visual Studio generator (multi-config):**
```powershell
cmake --preset default                    # Configure once
cmake --build --preset default            # Build Release
cmake --build --preset default-debug      # Build Debug
```

**Ninja (single-config presets):**
```powershell
# Release
.\build.ps1                               # Uses build.ps1 wrapper
# or
cmake --preset ninja-release && cmake --build --preset ninja-release

# Debug
.\build.ps1 -Config Debug
# or
cmake --preset ninja-debug && cmake --build --preset ninja-debug
```

**Clean rebuild:**
```powershell
Remove-Item -Recurse -Force build
cmake --preset default
cmake --build --preset default
```

## Documentation

- **User's Guide:** See `$env:PILLAR_SDK_DIR/docs/USERS_GUIDE.md`
- **API Reference:** See `$env:PILLAR_SDK_DIR/docs/API_REFERENCE.md`
- **Editor Guide:** See `$env:PILLAR_SDK_DIR/docs/PILLAR_EDITOR_GUIDE.md`

## Troubleshooting

### LNK2038: mismatch detected for '_ITERATOR_DEBUG_LEVEL' or 'RuntimeLibrary'

This error occurs when mixing Debug and Release libraries. The Pillar SDK is built in **Release** mode.

**Solution:** Build your game in Release mode:
```powershell
# Clean and rebuild in Release mode
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake --preset default
cmake --build --preset default
```

If you need Debug builds for development, build the Pillar SDK from source with Debug configuration.

### "Pillar SDK requires Microsoft Visual C++ (MSVC) compiler"

The Pillar SDK is built with MSVC and requires MSVC-compatible compiler for linking.

**Solutions:**
1. **Use Visual Studio generator (easiest):** `cmake --preset default`
2. **Use the build script:** `.\build.ps1` (auto-configures MSVC)
3. **Use Developer PowerShell for VS 2022** then run ninja presets
4. **Use Clang:** `cmake --preset clang-release` (requires LLVM installed)

### "Could not find Pillar" error

**Solution:** Set `PILLAR_SDK_DIR` environment variable:
```powershell
$env:PILLAR_SDK_DIR = "C:\path\to\PillarSDK"
```

### Build fails with "cl.exe not found"

This happens when using Ninja presets from a regular PowerShell.

**Solutions:**
1. **Use the build script:** `.\build.ps1` (auto-detects and configures MSVC)
2. **Use Visual Studio preset:** `cmake --preset default` (no cl.exe needed)
3. **Open Developer PowerShell for VS 2022** and run ninja presets from there
4. **Use Clang:** Install LLVM, add to PATH, use `cmake --preset clang-release`

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
