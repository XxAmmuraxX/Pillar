# Pillar Engine - Rendering System Guide

A comprehensive guide to the Pillar Engine's 2D rendering system, covering architecture, features, and best practices.

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Getting Started](#getting-started)
4. [Basic Rendering](#basic-rendering)
5. [Texture Atlas System](#texture-atlas-system)
6. [Batch Rendering](#batch-rendering)
7. [Post-Processing Effects](#post-processing-effects)
8. [Render Queue System](#render-queue-system)
9. [Shader Management](#shader-management)
10. [Debug Rendering](#debug-rendering)
11. [Performance Optimization](#performance-optimization)
12. [Best Practices](#best-practices)
13. [Common Issues & Solutions](#common-issues--solutions)
14. [API Reference](#api-reference)

---

## Overview

Pillar Engine features a high-performance 2D rendering system built on OpenGL with the following key features:

- **Batch Rendering**: Minimizes draw calls by batching quads by texture
- **Texture Atlas Support**: Pack multiple sprites into single textures
- **Post-Processing**: Screen-space effects (grayscale, vignette, chromatic aberration)
- **Render Queue**: Sort and optimize draw commands for efficiency
- **Shader Management**: Hot-reloadable shaders with caching
- **Debug Drawing**: Primitives for visualization (lines, rectangles, circles)

**Performance Targets:**
- 50,000+ quads at 60 FPS
- 1-5 draw calls per frame (with proper batching)
- Sub-millisecond frame times for simple scenes

---

## Architecture

### Rendering Layers

```
┌─────────────────────────────────────────┐
│          User Code (Game Layer)          │
│  Renderer2D::DrawQuad(), DrawSprite()    │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│         Renderer2D (Public API)          │
│  High-level batch renderer interface     │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│      BatchRenderer2D (Internal)          │
│  Accumulates quads, manages batches      │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│   OpenGLBatchRenderer2D (Platform)       │
│  OpenGL-specific implementation          │
└──────────────────┬──────────────────────┘
                   │
┌──────────────────▼──────────────────────┐
│       OpenGL API (Graphics Driver)       │
└─────────────────────────────────────────┘
```

### Key Components

- **Renderer2D**: Static API for 2D drawing commands
- **BatchRenderer2D**: Interface for batch renderer implementations
- **OpenGLBatchRenderer2D**: OpenGL-specific batch renderer
- **TextureAtlas**: Manages sprite sheets and sub-textures
- **ShaderLibrary**: Loads and caches shaders
- **PostProcessStack**: Chains post-processing effects
- **RenderQueue**: Sorts and optimizes draw commands

---

## Getting Started

### Initialization

Always initialize the renderer in your application's constructor:

```cpp
#include "Pillar/Renderer/Renderer2D.h"

class MyGameLayer : public Pillar::Layer
{
public:
    void OnAttach() override
    {
        // Renderer2D is already initialized by Application
        // Just create your camera
        m_Camera = Pillar::OrthographicCamera(-10.0f, 10.0f, -10.0f, 10.0f);
    }
    
private:
    Pillar::OrthographicCamera m_Camera;
};
```

**Note:** `Renderer2D::Init()` is automatically called in `Application::Application()`, so you typically don't need to call it manually.

### Shutdown

Renderer shutdown is handled automatically:

```cpp
// Called automatically in Application::~Application()
Pillar::Renderer2D::Shutdown();
```

---

## Basic Rendering

### Drawing Colored Quads

```cpp
void MyGameLayer::OnUpdate(float dt)
{
    // Clear screen
    Pillar::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Pillar::RenderCommand::Clear();
    
    // Begin scene with camera
    Pillar::Renderer2D::BeginScene(m_Camera);
    
    // Draw a red square at (0, 0) with size (1, 1)
    Pillar::Renderer2D::DrawQuad(
        { 0.0f, 0.0f },        // Position (2D)
        { 1.0f, 1.0f },        // Size
        { 1.0f, 0.0f, 0.0f, 1.0f }  // Color (RGBA)
    );
    
    // Draw a blue square at (2, 0) with Z-depth
    Pillar::Renderer2D::DrawQuad(
        { 2.0f, 0.0f, 0.5f },  // Position (3D with Z-depth)
        { 1.0f, 1.0f },        // Size
        { 0.0f, 0.0f, 1.0f, 1.0f }  // Color
    );
    
    // End scene (flushes batches)
    Pillar::Renderer2D::EndScene();
}
```

### Drawing Textured Quads

```cpp
// In OnAttach()
m_Texture = Pillar::Texture2D::Create("player.png");

// In OnUpdate()
Pillar::Renderer2D::BeginScene(m_Camera);

// Draw textured quad (white tint = no color modification)
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    m_Texture
);

// Draw textured quad with red tint
Pillar::Renderer2D::DrawQuad(
    { 2.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 0.0f, 0.0f, 1.0f },  // Red tint
    m_Texture
);

Pillar::Renderer2D::EndScene();
```

### Rotated Quads

```cpp
// Rotation in radians
float rotation = glm::radians(45.0f);

Pillar::Renderer2D::DrawRotatedQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    rotation,
    { 1.0f, 1.0f, 1.0f, 1.0f }  // White
);

// With texture
Pillar::Renderer2D::DrawRotatedQuad(
    { 2.0f, 0.0f },
    { 1.0f, 1.0f },
    rotation,
    { 1.0f, 1.0f, 1.0f, 1.0f },
    m_Texture
);
```

### Custom UV Coordinates

For texture sub-regions or flipping:

```cpp
// Draw bottom-left quarter of texture
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    m_Texture,
    { 0.0f, 0.0f },  // UV min (bottom-left)
    { 0.5f, 0.5f },  // UV max (half-way)
    false,           // flipX
    false            // flipY
);

// Flip texture horizontally
Pillar::Renderer2D::DrawQuad(
    { 2.0f, 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    m_Texture,
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    true,            // flipX = true
    false
);
```

---

## Texture Atlas System

### What is a Texture Atlas?

A texture atlas (sprite sheet) packs multiple sprites into a single texture to reduce draw calls and texture switches.

**Benefits:**
- **Performance**: 100 sprites from 1 atlas = 1 texture bind vs 100 separate textures = 100 binds
- **Memory**: Better GPU memory utilization
- **Batching**: All sprites from atlas can be batched together

### Creating an Atlas from Grid

For uniformly-sized sprites in a grid:

```cpp
// Create atlas from texture
auto atlas = Pillar::TextureAtlas::Create("spritesheet.png");

// Define grid layout: 8 columns, 4 rows, each cell 32x32 pixels
atlas->LoadFromGrid(
    8,      // columns
    4,      // rows
    32.0f,  // cell width
    32.0f   // cell height
);

// Access sprites by index: "0_0", "1_0", "2_0", etc.
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    atlas,
    "0_0"  // First sprite (top-left cell)
);
```

### Creating an Atlas from TexturePacker

For optimally-packed sprites from TexturePacker:

```cpp
// Create atlas
auto atlas = Pillar::TextureAtlas::Create("characters.png");

// Load TexturePacker JSON metadata
atlas->LoadFromTexturePacker("characters.json");

// Draw by sprite name (from original filename)
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    atlas,
    "player_idle_01"  // Sprite name
);
```

**TexturePacker JSON Format:**
```json
{
  "frames": {
    "player_idle_01.png": {
      "frame": {"x":0, "y":0, "w":32, "h":32},
      "rotated": false,
      "trimmed": false
    }
  },
  "meta": {
    "size": {"w":512, "h":512}
  }
}
```

### Creating an Atlas from Aseprite

```cpp
auto atlas = Pillar::TextureAtlas::Create("animation.png");
atlas->LoadFromAseprite("animation.json");

// Draw animation frame by name
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    atlas,
    "frame_0"
);
```

### Manual SubTexture Creation

For custom layouts:

```cpp
auto atlas = Pillar::TextureAtlas::Create("custom.png");

// Add sub-texture manually
Pillar::SubTexture subTex = Pillar::SubTexture::CreateFromGrid(
    0,     // column
    0,     // row
    64.0f, // cell width
    64.0f, // cell height
    512.0f,// atlas width
    512.0f // atlas height
);

atlas->AddSubTexture("my_sprite", subTex);

// Draw it
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f },
    { 1.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    atlas,
    "my_sprite"
);
```

### Advanced: Direct SubTexture Usage

For low-level control:

```cpp
auto atlas = Pillar::TextureAtlas::Create("sheet.png");
atlas->LoadFromGrid(4, 4, 32, 32);

// Get sub-texture
auto subTex = atlas->GetSubTexture("1_2");

// Draw with custom transform
Pillar::Renderer2D::DrawQuad(
    { 0.0f, 0.0f, 0.0f },
    { 2.0f, 2.0f },  // Double size
    { 1.0f, 0.5f, 0.5f, 1.0f },  // Pink tint
    atlas->GetTexture(),
    subTex
);
```

---

## Batch Rendering

### How Batching Works

The batch renderer groups quads by texture to minimize draw calls:

1. **Accumulation**: `DrawQuad()` calls add quads to internal batches
2. **Grouping**: Quads using the same texture go into the same batch
3. **Flushing**: `EndScene()` uploads vertex data and issues draw calls

**Example:**
```cpp
// These 3 quads use texture A → 1 batch → 1 draw call
DrawQuad(pos1, size, textureA);
DrawQuad(pos2, size, textureA);
DrawQuad(pos3, size, textureA);

// These 2 quads use texture B → 1 batch → 1 draw call
DrawQuad(pos4, size, textureB);
DrawQuad(pos5, size, textureB);

// Total: 2 draw calls instead of 5
```

### Batch Limits

- **Max Quads per Batch**: 10,000 (configurable in code)
- **Max Texture Slots**: 32 (GPU hardware limit)
- **Automatic Flush**: When batch is full or texture slots exhausted

### Statistics

Monitor batch efficiency:

```cpp
auto stats = Pillar::Renderer2D::GetStats();

PIL_INFO("Draw Calls: {}", stats.DrawCalls);
PIL_INFO("Quads: {}", stats.QuadCount);
PIL_INFO("Batches: {}", stats.BatchCount);
PIL_INFO("Buffer Uploads: {}", stats.BufferUploads);
PIL_INFO("Batch Efficiency: {:.2f} quads/batch", stats.GetBatchEfficiency());
```

**ImGui Display:**
```cpp
auto stats = Pillar::Renderer2D::GetStats();
ImGui::Text("Draw Calls: %u", stats.DrawCalls);
ImGui::Text("Quads: %u", stats.QuadCount);
ImGui::Text("Batches: %u", stats.BatchCount);
ImGui::Text("Efficiency: %.2f", stats.GetBatchEfficiency());
```

---

## Post-Processing Effects

### Overview

Post-processing effects are screen-space shaders applied to the final rendered image.

**Available Effects:**
- **Grayscale**: Desaturation effect
- **Vignette**: Darkens edges
- **Chromatic Aberration**: Color fringing
- **Bloom**: Glow for bright areas (stubbed - needs implementation)

### Basic Usage

```cpp
#include "Pillar/Renderer/PostProcessing.h"

class MyLayer : public Pillar::Layer
{
public:
    void OnAttach() override
    {
        // Create post-process stack
        m_PostProcessStack = std::make_unique<Pillar::PostProcessStack>();
        m_PostProcessStack->Init(1920, 1080);
        
        // Add effects
        auto vignette = std::make_shared<Pillar::VignetteEffect>();
        vignette->SetRadius(0.7f);
        vignette->SetSoftness(0.5f);
        vignette->SetIntensity(0.6f);
        m_PostProcessStack->AddEffect(vignette);
    }
    
    void OnUpdate(float dt) override
    {
        // Render scene normally
        Pillar::Renderer2D::BeginScene(m_Camera);
        // ... draw stuff ...
        Pillar::Renderer2D::EndScene();
        
        // Apply post-processing
        uint32_t sceneTexture = /* framebuffer texture */;
        m_PostProcessStack->Process(sceneTexture);
    }
    
private:
    std::unique_ptr<Pillar::PostProcessStack> m_PostProcessStack;
};
```

### Effect Chaining

Multiple effects are applied in sequence:

```cpp
auto grayscale = std::make_shared<Pillar::GrayscaleEffect>();
grayscale->SetIntensity(0.5f);

auto vignette = std::make_shared<Pillar::VignetteEffect>();
vignette->SetRadius(0.7f);

auto chroma = std::make_shared<Pillar::ChromaticAberrationEffect>();
chroma->SetOffset(0.005f);

m_PostProcessStack->AddEffect(grayscale);
m_PostProcessStack->AddEffect(vignette);
m_PostProcessStack->AddEffect(chroma);

// Effects applied in order: Grayscale → Vignette → Chromatic Aberration
```

### Effect Controls

All effects support:

```cpp
effect->SetEnabled(true);      // Enable/disable
effect->SetIntensity(0.8f);    // Blend with original (0.0 - 1.0)
```

**Effect-Specific Parameters:**

**Vignette:**
```cpp
vignette->SetRadius(0.7f);     // Inner radius (0.0 - 1.0)
vignette->SetSoftness(0.5f);   // Edge softness (0.0 - 1.0)
```

**Chromatic Aberration:**
```cpp
chroma->SetOffset(0.005f);     // Color separation amount
```

### Performance Considerations

- Each effect adds ~0.5-2ms per frame at 1080p
- Limit to 2-3 effects for 60 FPS target
- Reduce resolution for cheaper effects

---

## Render Queue System

### Overview

The render queue sorts and batches draw commands for optimal performance.

**Sorting Strategies:**
- **By Texture**: Minimize texture switches
- **By Depth**: Back-to-front for transparency
- **By Layer**: Coarse ordering (Background → World → UI)

### Basic Usage

```cpp
#include "Pillar/Renderer/RenderQueue.h"

void RenderScene()
{
    Pillar::RenderQueue queue;
    queue.SetSortMode(Pillar::RenderQueue::SortMode::Texture);
    
    // Submit draw commands
    for (auto& sprite : m_Sprites)
    {
        queue.Submit(
            Pillar::RenderQueue::RenderLayer::World,
            sprite.zIndex,
            sprite.texture->GetRendererID(),
            [&sprite]() {
                Pillar::Renderer2D::DrawQuad(
                    sprite.position,
                    sprite.size,
                    sprite.color,
                    sprite.texture
                );
            }
        );
    }
    
    // Sort and execute all commands
    queue.Flush();
}
```

### Render Layers

Predefined layers for coarse sorting:

```cpp
enum class RenderLayer
{
    Background = 0,  // Sky, parallax background
    World = 1,       // Game objects
    Particles = 2,   // Particle effects
    UI = 3,          // User interface
    Debug = 4        // Debug overlays
};
```

### Sort Modes

```cpp
enum class SortMode
{
    None,           // No sorting (submission order)
    BackToFront,    // Sort by depth (high to low Z)
    FrontToBack,    // Sort by depth (low to high Z)
    Texture         // Sort by texture ID (minimize switches)
};
```

### Advanced: Custom Sort Keys

```cpp
// Manual sort key construction
uint64_t sortKey = Pillar::RenderQueue::CreateSortKey(
    Pillar::RenderQueue::RenderLayer::World,
    sprite.zIndex,
    sprite.texture->GetRendererID()
);

queue.Submit(sortKey, []() { /* draw */ });
```

---

## Shader Management

### ShaderLibrary

The `ShaderLibrary` manages shader loading, caching, and hot-reload.

### Loading Shaders from Files

```cpp
#include "Pillar/Renderer/ShaderLibrary.h"

auto& shaderLib = Pillar::ShaderLibrary::GetInstance();

// Load shader
auto shader = shaderLib.Load(
    "MyShader",
    "shaders/MyShader.vert",
    "shaders/MyShader.frag"
);

// Use shader
shader->Bind();
shader->SetFloat4("u_Color", { 1.0f, 0.0f, 0.0f, 1.0f });
```

### Loading from Source

```cpp
const char* vertSrc = R"(
    #version 410 core
    layout(location = 0) in vec3 a_Position;
    void main() {
        gl_Position = vec4(a_Position, 1.0);
    }
)";

const char* fragSrc = R"(
    #version 410 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
)";

auto shader = shaderLib.LoadFromSource("RedShader", vertSrc, fragSrc);
```

### Hot-Reload

For development:

```cpp
// Reload single shader
shaderLib.Reload("MyShader");

// Reload all shaders
uint32_t reloadedCount = shaderLib.ReloadAll();
PIL_INFO("Reloaded {} shaders", reloadedCount);
```

**ImGui Hot-Reload Button:**
```cpp
if (ImGui::Button("Reload Shaders"))
{
    auto& shaderLib = Pillar::ShaderLibrary::GetInstance();
    uint32_t count = shaderLib.ReloadAll();
    PIL_INFO("Reloaded {} shaders", count);
}
```

---

## Debug Rendering

### Drawing Lines

```cpp
Pillar::Renderer2D::DrawLine(
    { 0.0f, 0.0f, 0.0f },        // Start point
    { 1.0f, 1.0f, 0.0f },        // End point
    { 1.0f, 0.0f, 0.0f, 1.0f }   // Color (red)
);
```

### Drawing Rectangles

```cpp
// Filled rectangle
Pillar::Renderer2D::DrawRect(
    { 0.0f, 0.0f },
    { 2.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 1.0f }  // Green
);

// Outlined rectangle
Pillar::Renderer2D::DrawRectOutline(
    { 3.0f, 0.0f },
    { 2.0f, 1.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f }  // Yellow
);
```

### Drawing Circles

```cpp
// Filled circle
Pillar::Renderer2D::DrawCircle(
    { 0.0f, 0.0f },
    1.0f,  // radius
    { 1.0f, 0.0f, 1.0f, 1.0f }  // Magenta
);

// Outlined circle
Pillar::Renderer2D::DrawCircleOutline(
    { 3.0f, 0.0f },
    1.0f,
    { 0.0f, 1.0f, 1.0f, 1.0f }  // Cyan
);
```

---

## Performance Optimization

### Best Practices

**1. Use Texture Atlases**
- Pack sprites into atlases to reduce texture switches
- One 512x512 atlas can hold 256 32x32 sprites
- Result: 1 draw call instead of 256

**2. Minimize Texture Switches**
- Group objects by texture when possible
- Use render queue with texture sorting

**3. Avoid Frequent State Changes**
- Batch similar objects together
- Change shaders sparingly

**4. Profile with Statistics**
```cpp
auto stats = Pillar::Renderer2D::GetStats();
if (stats.DrawCalls > 10)
{
    PIL_WARN("High draw call count: {}", stats.DrawCalls);
}
```

**5. Use Z-Depth Wisely**
- Z-depth affects sorting in render queue
- Keep Z range small (0.0 - 1.0) for precision

### Performance Targets

- **Good**: < 5 draw calls, < 2ms frame time
- **Acceptable**: < 20 draw calls, < 10ms frame time
- **Poor**: > 50 draw calls, > 15ms frame time

---

## Best Practices

### 1. Always Initialize Renderer

```cpp
// In Application constructor (automatic)
Pillar::Renderer2D::Init();
```

### 2. Use BeginScene/EndScene

```cpp
// CORRECT
Pillar::Renderer2D::BeginScene(camera);
// ... draw calls ...
Pillar::Renderer2D::EndScene();

// WRONG - missing BeginScene
Pillar::Renderer2D::DrawQuad(...);  // Won't render!
```

### 3. Clear Screen Before Rendering

```cpp
Pillar::RenderCommand::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
Pillar::RenderCommand::Clear();
```

### 4. Check for Null Textures

```cpp
if (texture)
{
    Pillar::Renderer2D::DrawQuad(pos, size, texture);
}
else
{
    // Fallback to colored quad
    Pillar::Renderer2D::DrawQuad(pos, size, { 1.0f, 0.0f, 1.0f, 1.0f });
}
```

### 5. Use AssetManager for Paths

```cpp
// CORRECT - automatically resolves paths
auto texture = Pillar::Texture2D::Create("player.png");

// WRONG - hardcoded path may not work in distribution
auto texture = Pillar::Texture2D::Create("C:/dev/MyGame/assets/player.png");
```

### 6. Cache Resources

```cpp
// GOOD - load once in OnAttach()
void OnAttach() override
{
    m_PlayerTexture = Pillar::Texture2D::Create("player.png");
}

// BAD - loading every frame
void OnUpdate(float dt) override
{
    auto texture = Pillar::Texture2D::Create("player.png");  // Slow!
    Pillar::Renderer2D::DrawQuad(pos, size, texture);
}
```

### 7. Monitor Statistics in Debug Builds

```cpp
#ifdef PIL_DEBUG
    auto stats = Pillar::Renderer2D::GetStats();
    ImGui::Text("Draw Calls: %u", stats.DrawCalls);
    ImGui::Text("Quads: %u", stats.QuadCount);
#endif
```

---

## Common Issues & Solutions

### Issue: Nothing Renders

**Symptoms:**
- Black screen
- Draw calls execute but nothing visible

**Solutions:**
1. Check if `BeginScene()` was called:
   ```cpp
   Pillar::Renderer2D::BeginScene(camera);
   ```

2. Verify camera settings:
   ```cpp
   // Check camera bounds
   auto camera = Pillar::OrthographicCamera(-10.0f, 10.0f, -10.0f, 10.0f);
   ```

3. Ensure colors are visible (not transparent):
   ```cpp
   // Alpha channel must be > 0
   Pillar::Renderer2D::DrawQuad(pos, size, { 1.0f, 0.0f, 0.0f, 1.0f });
   ```

4. Check clear color (might match quad color):
   ```cpp
   Pillar::RenderCommand::SetClearColor({ 0.0f, 0.0f, 0.0f, 1.0f });
   ```

### Issue: Texture Not Loading

**Symptoms:**
- White or magenta quad instead of texture
- Error: "Failed to load texture"

**Solutions:**
1. Verify file path:
   ```cpp
   PIL_INFO("Attempting to load: {}", path);
   auto texture = Pillar::Texture2D::Create(path);
   ```

2. Check file format (supported: PNG, JPG, BMP, TGA):
   ```cpp
   // Use PNG for best compatibility
   auto texture = Pillar::Texture2D::Create("sprite.png");
   ```

3. Ensure file exists in assets folder:
   ```
   MyGame/
   ├── bin/Debug-x64/MyGame/
   │   └── assets/
   │       └── textures/
   │           └── sprite.png  ← File must be here for distribution
   └── assets/
       └── textures/
           └── sprite.png  ← Or here for development
   ```

### Issue: Poor Performance

**Symptoms:**
- Low FPS
- High draw call count
- Frame stuttering

**Solutions:**
1. Check statistics:
   ```cpp
   auto stats = Pillar::Renderer2D::GetStats();
   PIL_INFO("Draw Calls: {}", stats.DrawCalls);  // Should be < 10
   PIL_INFO("Buffer Uploads: {}", stats.BufferUploads);  // Should be low
   ```

2. Use texture atlases:
   ```cpp
   // Before: 100 separate textures = 100 draw calls
   // After: 1 atlas with 100 sprites = 1-2 draw calls
   ```

3. Enable batch sorting with render queue:
   ```cpp
   RenderQueue queue;
   queue.SetSortMode(RenderQueue::SortMode::Texture);
   ```

4. Profile post-processing:
   ```cpp
   // Disable effects temporarily to check impact
   effect->SetEnabled(false);
   ```

### Issue: Sprites Draw in Wrong Order

**Symptoms:**
- Sprites appear behind when they should be in front
- Z-fighting or flickering

**Solutions:**
1. Use Z-depth for layering:
   ```cpp
   // Background at Z = 0.0
   Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.0f }, size, bgTexture);
   
   // Player at Z = 0.5
   Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f, 0.5f }, size, playerTexture);
   
   // UI at Z = 1.0
   Pillar::Renderer2D::DrawQuad({ 0.0f, 0.0f, 1.0f }, size, uiTexture);
   ```

2. Use render queue with depth sorting:
   ```cpp
   queue.SetSortMode(RenderQueue::SortMode::BackToFront);
   ```

3. Use render layers:
   ```cpp
   queue.Submit(RenderQueue::RenderLayer::Background, ...);
   queue.Submit(RenderQueue::RenderLayer::World, ...);
   queue.Submit(RenderQueue::RenderLayer::UI, ...);
   ```

---

## API Reference

### Renderer2D

**Initialization:**
- `static void Init()` - Initialize renderer (called automatically)
- `static void Shutdown()` - Clean up renderer (called automatically)

**Scene Management:**
- `static void BeginScene(const OrthographicCamera& camera)` - Start rendering frame
- `static void EndScene()` - End frame and flush batches

**Drawing - Colored Quads:**
- `DrawQuad(vec2 pos, vec2 size, vec4 color)`
- `DrawQuad(vec3 pos, vec2 size, vec4 color)`
- `DrawRotatedQuad(vec2 pos, vec2 size, float rot, vec4 color)`
- `DrawRotatedQuad(vec3 pos, vec2 size, float rot, vec4 color)`

**Drawing - Textured Quads:**
- `DrawQuad(vec2 pos, vec2 size, Texture2D)`
- `DrawQuad(vec3 pos, vec2 size, Texture2D)`
- `DrawQuad(vec3 pos, vec2 size, vec4 color, Texture2D, vec2 uvMin, vec2 uvMax, bool flipX, bool flipY)`
- `DrawRotatedQuad(vec2 pos, vec2 size, float rot, vec4 color, Texture2D)`
- `DrawRotatedQuad(vec3 pos, vec2 size, float rot, vec4 color, Texture2D, vec2 uvMin, vec2 uvMax, bool flipX, bool flipY)`

**Drawing - Texture Atlas:**
- `DrawQuad(vec2 pos, vec2 size, vec4 color, TextureAtlas, string spriteName)`
- `DrawQuad(vec3 pos, vec2 size, vec4 color, TextureAtlas, string spriteName)`
- `DrawRotatedQuad(vec2 pos, vec2 size, float rot, vec4 color, TextureAtlas, string spriteName)`
- `DrawRotatedQuad(vec3 pos, vec2 size, float rot, vec4 color, TextureAtlas, string spriteName)`
- `DrawQuad(vec3 pos, vec2 size, vec4 color, Texture2D, SubTexture)`
- `DrawRotatedQuad(vec3 pos, vec2 size, float rot, vec4 color, Texture2D, SubTexture)`

**Statistics:**
- `static Renderer2DStats GetStats()` - Get rendering statistics

### TextureAtlas

**Creation:**
- `static shared_ptr<TextureAtlas> Create(const string& path)` - Load from file
- `static shared_ptr<TextureAtlas> Create(shared_ptr<Texture2D> texture)` - From existing texture

**Loading:**
- `void LoadFromGrid(int cols, int rows, float cellW, float cellH)` - Uniform grid
- `void LoadFromTexturePacker(const string& jsonPath)` - TexturePacker JSON
- `void LoadFromAseprite(const string& jsonPath)` - Aseprite JSON

**Management:**
- `void AddSubTexture(const string& name, const SubTexture& sub)` - Add manually
- `SubTexture GetSubTexture(const string& name)` - Retrieve by name
- `bool HasSubTexture(const string& name)` - Check existence
- `bool RemoveSubTexture(const string& name)` - Remove by name

**Access:**
- `shared_ptr<Texture2D> GetTexture()` - Get underlying texture
- `const unordered_map<string, SubTexture>& GetSubTextures()` - Get all

### PostProcessStack

**Initialization:**
- `void Init(uint32_t width, uint32_t height)` - Initialize framebuffers
- `void Shutdown()` - Clean up resources

**Effect Management:**
- `void AddEffect(shared_ptr<PostProcessEffect> effect)` - Add effect to chain
- `void RemoveEffect(shared_ptr<PostProcessEffect> effect)` - Remove effect
- `void ClearEffects()` - Remove all effects

**Rendering:**
- `void Process(uint32_t inputTexture)` - Apply all effects

**Control:**
- `void SetEnabled(bool enabled)` - Enable/disable entire stack
- `bool IsEnabled()` - Check if enabled

### ShaderLibrary

**Singleton:**
- `static ShaderLibrary& GetInstance()` - Get singleton instance

**Loading:**
- `shared_ptr<Shader> Load(const string& name, const string& vertPath, const string& fragPath)` - Load from files
- `shared_ptr<Shader> LoadFromSource(const string& name, const string& vertSrc, const string& fragSrc)` - Load from strings
- `void Add(const string& name, shared_ptr<Shader> shader)` - Add existing shader

**Access:**
- `shared_ptr<Shader> Get(const string& name)` - Get shader by name
- `bool Exists(const string& name)` - Check if shader exists

**Hot-Reload:**
- `bool Reload(const string& name)` - Reload single shader
- `uint32_t ReloadAll()` - Reload all shaders (returns count)

---

## Conclusion

The Pillar Engine rendering system provides a powerful and flexible 2D rendering pipeline. By following this guide and the best practices outlined, you can create high-performance 2D games with modern rendering features.

**Key Takeaways:**
- Use texture atlases for better performance
- Monitor statistics to identify bottlenecks
- Leverage batch rendering for draw call reduction
- Use post-processing for visual polish
- Profile and optimize based on measurements

For more information, see:
- [User's Guide](../../docs/USERS_GUIDE.md)
- [API Reference (Pillar.h)](../src/Pillar.h)

---

*Last Updated: January 11, 2026*
