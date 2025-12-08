# Batch Renderer 2D - Usage Guide

**Version:** 1.0  
**Date:** December 8, 2025  
**Status:** Production Ready

---

## Overview

The Batch Renderer 2D is a high-performance GPU-accelerated rendering system that dramatically reduces draw calls by batching multiple quads into single GPU submissions. It replaces the immediate-mode Renderer2D with a batching system that can handle 10,000 quads per draw call.

**Performance Gain:** 100-10,000x reduction in draw calls depending on scene complexity.

### Key Specifications

| Metric | Value |
|--------|-------|
| Max Quads Per Batch | 10,000 |
| Max Texture Slots | 32 (hardware dependent) |
| Supported Primitives | Quads (textured & colored) |
| Rotation Support | ✅ Yes |
| Z-Ordering | ✅ Yes (textured quads only) |
| Sprite Sheets | ✅ Yes (via texture coordinates) |

---

## Quick Start

### Basic Usage

```cpp
#include "Pillar/Renderer/Renderer2DBackend.h"

// In OnUpdate(float dt)
Renderer2DBackend::BeginScene(camera);

// Draw colored quad
Renderer2DBackend::DrawQuad(
    glm::vec2(0, 0),           // position
    glm::vec2(1, 1),           // size
    glm::vec4(1, 0, 0, 1)      // color (red)
);

// Draw textured quad
Renderer2DBackend::DrawQuad(
    glm::vec2(2, 0),           // position
    glm::vec2(1, 1),           // size
    glm::vec4(1, 1, 1, 1),     // tint color (white = no tint)
    myTexture                   // shared_ptr<Texture2D>
);

Renderer2DBackend::EndScene();
```

### Switching Renderers at Runtime

```cpp
// Use batch renderer (default, recommended)
Renderer2DBackend::SetAPI(Renderer2DBackend::API::Batch);

// Use basic renderer (legacy, for debugging)
Renderer2DBackend::SetAPI(Renderer2DBackend::API::Basic);

// Check current API
auto currentAPI = Renderer2DBackend::GetAPI();
```

---

## API Reference

### Initialization

```cpp
// Called automatically by Application
Renderer2DBackend::Init(Renderer2DBackend::API::Batch);

// Shutdown (automatic on app exit)
Renderer2DBackend::Shutdown();
```

### Scene Management

```cpp
void BeginScene(const OrthographicCamera& camera);
void EndScene();
```

**Rules:**
- Must call `BeginScene()` before any draw calls
- Must call `EndScene()` after all draw calls
- Pairs must match (no nested scenes)

### Draw Commands

#### 1. Colored Quad (vec2 position)

```cpp
void DrawQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec4& color
);
```

**Example:**
```cpp
// Draw red square at origin
Renderer2DBackend::DrawQuad(
    glm::vec2(0, 0),
    glm::vec2(1, 1),
    glm::vec4(1, 0, 0, 1)  // RGBA
);
```

#### 2. Textured Quad (vec2 position)

```cpp
void DrawQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    const glm::vec4& color,
    const std::shared_ptr<Texture2D>& texture
);
```

**Example:**
```cpp
auto sprite = Texture2D::Create("assets/textures/player.png");

Renderer2DBackend::DrawQuad(
    glm::vec2(0, 0),
    glm::vec2(2, 2),
    glm::vec4(1, 1, 1, 1),  // No tint
    sprite
);
```

#### 3. Textured Quad with Custom UV (vec3 position)

```cpp
void DrawQuad(
    const glm::vec3& position,        // x, y, z (z = depth)
    const glm::vec2& size,
    const glm::vec4& color,
    const std::shared_ptr<Texture2D>& texture,
    const glm::vec2& texCoordMin = glm::vec2(0.0f),
    const glm::vec2& texCoordMax = glm::vec2(1.0f)
);
```

**Example - Sprite Sheet:**
```cpp
// Draw character from sprite sheet (16x16 sprites in 256x256 texture)
float spriteSize = 16.0f / 256.0f;
int spriteX = 3, spriteY = 2;  // Which sprite to use

Renderer2DBackend::DrawQuad(
    glm::vec3(0, 0, 0.5f),  // Z = 0.5 for layering
    glm::vec2(1, 1),
    glm::vec4(1, 1, 1, 1),
    spriteSheet,
    glm::vec2(spriteX * spriteSize, spriteY * spriteSize),      // UV min
    glm::vec2((spriteX + 1) * spriteSize, (spriteY + 1) * spriteSize)  // UV max
);
```

#### 4. Rotated Colored Quad

```cpp
void DrawRotatedQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    float rotation,              // radians
    const glm::vec4& color
);
```

**Example:**
```cpp
#include <glm/gtc/constants.hpp>

// Draw rotated square (45 degrees)
Renderer2DBackend::DrawRotatedQuad(
    glm::vec2(0, 0),
    glm::vec2(1, 1),
    glm::radians(45.0f),
    glm::vec4(0, 1, 0, 1)  // Green
);
```

#### 5. Rotated Textured Quad

```cpp
void DrawRotatedQuad(
    const glm::vec2& position,
    const glm::vec2& size,
    float rotation,
    const glm::vec4& color,
    const std::shared_ptr<Texture2D>& texture
);
```

### Statistics

```cpp
uint32_t GetDrawCallCount();  // How many GPU draw calls this frame
uint32_t GetQuadCount();       // How many quads rendered this frame
void ResetStats();             // Reset counters (call at frame start)
```

**Example:**
```cpp
void OnUpdate(float dt) {
    Renderer2DBackend::ResetStats();
    
    Renderer2DBackend::BeginScene(camera);
    // ... draw calls ...
    Renderer2DBackend::EndScene();
    
    uint32_t drawCalls = Renderer2DBackend::GetDrawCallCount();
    uint32_t quads = Renderer2DBackend::GetQuadCount();
    
    PIL_INFO("Rendered {0} quads in {1} draw calls", quads, drawCalls);
}
```

---

## ECS Integration

### Using SpriteComponent

```cpp
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"

// Add sprite to entity
auto& sprite = entity.AddComponent<SpriteComponent>();
sprite.Texture = Texture2D::Create("player.png");
sprite.Color = glm::vec4(1, 1, 1, 1);
sprite.Size = glm::vec2(2, 2);
sprite.ZIndex = 1.0f;  // Render order

// For sprite sheets
sprite.TexCoordMin = glm::vec2(0.0f, 0.0f);
sprite.TexCoordMax = glm::vec2(0.25f, 0.25f);  // Quarter of texture
```

### Using SpriteRenderSystem

```cpp
#include "Pillar/ECS/Systems/SpriteRenderSystem.h"

// In Scene setup
auto spriteSystem = new SpriteRenderSystem();
spriteSystem->OnAttach(scene);

// In OnUpdate
Renderer2DBackend::BeginScene(camera);
spriteSystem->OnUpdate(dt);  // Automatically renders all sprites
Renderer2DBackend::EndScene();
```

**Features:**
- ✅ Automatic sorting by texture (minimizes texture swaps)
- ✅ Automatic sorting by Z-index (correct layering)
- ✅ Rotation support (uses TransformComponent.Rotation)
- ✅ Scaling support (multiplies sprite size by transform scale)

---

## Performance Tips

### 1. Minimize Texture Swaps

**Bad:** (5,000 draw calls)
```cpp
for (int i = 0; i < 10000; ++i) {
    auto texture = (i % 2 == 0) ? texture1 : texture2;
    Renderer2DBackend::DrawQuad(pos, size, color, texture);
}
```

**Good:** (2 draw calls)
```cpp
// Draw all texture1 quads first
for (int i = 0; i < 5000; ++i) {
    Renderer2DBackend::DrawQuad(pos, size, color, texture1);
}

// Then all texture2 quads
for (int i = 0; i < 5000; ++i) {
    Renderer2DBackend::DrawQuad(pos, size, color, texture2);
}
```

**Best:** Use SpriteRenderSystem (automatic sorting)

### 2. Use Texture Atlases

Combine multiple sprites into one texture to reduce draw calls.

```cpp
// Instead of 100 different textures (100 draw calls)
// Use 1 texture atlas with 100 sprites (1 draw call)
auto atlas = Texture2D::Create("sprites_atlas.png");

for (int i = 0; i < 100; ++i) {
    float u = (i % 10) * 0.1f;
    float v = (i / 10) * 0.1f;
    
    Renderer2DBackend::DrawQuad(
        glm::vec3(pos, 0),
        size,
        color,
        atlas,
        glm::vec2(u, v),
        glm::vec2(u + 0.1f, v + 0.1f)
    );
}
```

### 3. Batch by Material

Group draws by texture, then by other properties:

```cpp
// 1. Draw all opaque sprites (sorted by texture)
// 2. Draw all transparent sprites (sorted by depth)
```

### 4. Use Object Pooling

Reuse entities instead of creating/destroying:

```cpp
#include "Pillar/ECS/ObjectPool.h"

ObjectPool bulletPool;
bulletPool.Init(scene, 1000);

// Acquire from pool (reuses dead bullets)
Entity bullet = bulletPool.Acquire();

// Release when done
bulletPool.Release(bullet);
```

### 5. Profile Your Scene

```cpp
void OnImGuiRender() {
    ImGui::Begin("Renderer Stats");
    ImGui::Text("Draw Calls: %u", Renderer2DBackend::GetDrawCallCount());
    ImGui::Text("Quads: %u", Renderer2DBackend::GetQuadCount());
    
    // Aim for:
    // - Draw calls < 10 for best performance
    // - Draw calls = unique texture count (ideal)
    ImGui::End();
}
```

---

## Common Patterns

### Animated Sprites

```cpp
class AnimatedSprite {
    std::shared_ptr<Texture2D> m_SpriteSheet;
    int m_FrameCount;
    int m_CurrentFrame;
    float m_FrameTime;
    float m_Timer;
    
    void Update(float dt) {
        m_Timer += dt;
        if (m_Timer >= m_FrameTime) {
            m_Timer = 0;
            m_CurrentFrame = (m_CurrentFrame + 1) % m_FrameCount;
        }
    }
    
    void Render(glm::vec2 position) {
        float frameWidth = 1.0f / m_FrameCount;
        float u = m_CurrentFrame * frameWidth;
        
        Renderer2DBackend::DrawQuad(
            glm::vec3(position, 0),
            glm::vec2(1, 1),
            glm::vec4(1, 1, 1, 1),
            m_SpriteSheet,
            glm::vec2(u, 0),
            glm::vec2(u + frameWidth, 1)
        );
    }
};
```

### Particle Systems

```cpp
struct Particle {
    glm::vec2 Position;
    glm::vec2 Velocity;
    glm::vec4 Color;
    float Lifetime;
};

std::vector<Particle> particles;

void UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.Position += p.Velocity * dt;
        p.Lifetime -= dt;
        p.Color.a = p.Lifetime;  // Fade out
    }
}

void RenderParticles() {
    Renderer2DBackend::BeginScene(camera);
    
    for (const auto& p : particles) {
        if (p.Lifetime > 0) {
            Renderer2DBackend::DrawQuad(
                p.Position,
                glm::vec2(0.1f, 0.1f),
                p.Color
            );
        }
    }
    
    Renderer2DBackend::EndScene();
}
```

### Health Bars

```cpp
void DrawHealthBar(glm::vec2 position, float health, float maxHealth) {
    float barWidth = 1.0f;
    float barHeight = 0.1f;
    
    // Background (red)
    Renderer2DBackend::DrawQuad(
        position,
        glm::vec2(barWidth, barHeight),
        glm::vec4(0.8f, 0, 0, 1)
    );
    
    // Foreground (green)
    float healthPercent = health / maxHealth;
    Renderer2DBackend::DrawQuad(
        position,
        glm::vec2(barWidth * healthPercent, barHeight),
        glm::vec4(0, 0.8f, 0, 1)
    );
}
```

---

## Migration from Renderer2D

### Before (Legacy)

```cpp
#include "Pillar/Renderer/Renderer2D.h"

void OnUpdate(float dt) {
    Renderer2D::BeginScene(camera);
    Renderer2D::DrawQuad(pos, size, color);
    Renderer2D::EndScene();
}
```

### After (Batch Renderer)

```cpp
#include "Pillar/Renderer/Renderer2DBackend.h"

void OnUpdate(float dt) {
    Renderer2DBackend::ResetStats();  // Add for statistics
    Renderer2DBackend::BeginScene(camera);
    Renderer2DBackend::DrawQuad(pos, size, color);
    Renderer2DBackend::EndScene();
}
```

**Changes:**
1. Replace `Renderer2D` with `Renderer2DBackend`
2. Add `ResetStats()` if using statistics
3. Initialize backend in Application: `Renderer2DBackend::Init()`
4. Texture parameters now use `shared_ptr<Texture2D>` instead of raw pointers

---

## Troubleshooting

### Problem: Nothing Renders

**Check:**
1. Did you call `BeginScene()` before draw calls?
2. Did you call `EndScene()` after draw calls?
3. Is camera configured correctly? (check view bounds)
4. Check stats: `GetQuadCount()` should be > 0

### Problem: Poor Performance

**Check:**
1. How many draw calls? Aim for < 10
2. Are you sorting by texture?
3. Using too many unique textures? (max 32 per batch)
4. Try Basic renderer to see if batching helps

### Problem: Textures Not Showing

**Check:**
1. Is texture loaded successfully? (check logs)
2. Is texture bound to correct slot?
3. Tint color is white? (other colors modify texture)
4. UV coordinates correct? (0-1 range)

### Problem: Z-Ordering Issues

**Check:**
1. Using vec3 position for textured quads?
2. Z values are different for each layer?
3. Camera near/far planes include your Z range?
4. SpriteRenderSystem is sorting correctly?

---

## Technical Details

### Batching Algorithm

```
1. BeginScene()
   - Clear batch data
   - Store camera matrix
   - Reset texture slot index

2. DrawQuad()
   - Get/assign texture slot
   - Find or create batch for this texture
   - Add 4 vertices to batch
   - If batch full (10,000 quads), flush

3. EndScene()
   - Flush remaining batches

4. Flush()
   - Upload vertex data to GPU (glBufferSubData)
   - Bind all textures to slots
   - For each batch:
     - Draw indexed triangles (6 indices per quad)
     - Increment draw call counter
```

### Memory Layout

**Vertex Structure (32 bytes):**
```cpp
struct QuadVertex {
    glm::vec3 Position;   // 12 bytes
    glm::vec4 Color;      // 16 bytes
    glm::vec2 TexCoord;   //  8 bytes
    float TexIndex;       //  4 bytes
};  // Total: 40 bytes
```

**Buffer Sizes:**
- Vertex Buffer: 10,000 quads × 4 vertices × 40 bytes = 1.6 MB
- Index Buffer: 10,000 quads × 6 indices × 4 bytes = 240 KB

### Shader Code

**Vertex Shader (GLSL 410):**
```glsl
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;

void main() {
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
```

**Fragment Shader (GLSL 410):**
```glsl
layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[32];

void main() {
    int texIndex = int(v_TexIndex);
    color = texture(u_Textures[texIndex], v_TexCoord) * v_Color;
}
```

---

## Limitations

1. **Texture Limit:** Maximum 32 unique textures per batch
   - **Workaround:** Use texture atlases or split into multiple scenes

2. **Primitive Type:** Only quads supported (no lines, circles)
   - **Workaround:** Use multiple quads to approximate shapes

3. **Z-Ordering:** Only works with vec3 position (textured quads)
   - **Reason:** Colored quads use vec2 overload for performance

4. **Shader Customization:** Embedded shaders cannot be hot-reloaded
   - **Future:** External shader file support planned

---

## Best Practices

✅ **DO:**
- Group draws by texture
- Use texture atlases when possible
- Reset stats each frame for accurate metrics
- Use SpriteRenderSystem for automatic optimization
- Profile with statistics (aim for < 10 draw calls)

❌ **DON'T:**
- Mix textured and colored quads randomly
- Create new textures every frame
- Ignore draw call count warnings
- Use > 32 unique textures in one scene
- Call BeginScene/EndScene multiple times per frame

---

## Further Reading

- **Source Code:** `Pillar/src/Pillar/Renderer/BatchRenderer2D.h`
- **Implementation:** `Platform/OpenGL/OpenGLBatchRenderer2D.cpp`
- **Examples:** `Sandbox/src/LightEntityPerfDemo.h`
- **Phase 5 Plan:** `PHASE_5_PLAN.md`
- **Completion Summary:** `PHASE_5_COMPLETION_SUMMARY.md`

---

## Support & Contribution

For questions, issues, or contributions:
- **Repository:** Pillar Engine
- **Documentation:** `Pillar/docs/`
- **Tests:** `Tests/src/`

**Last Updated:** December 8, 2025
