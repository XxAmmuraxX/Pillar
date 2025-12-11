# Phase 5: Instanced Rendering & Batch System - Implementation Plan

**Date:** Current Date  
**Status:** ? Planning Complete - Ready for Implementation  
**Branch:** `feature/render_api/batch_rendering`  
**Duration:** 8-12 hours (sequential implementation)  
**Prerequisites:** ? Phase 4 Complete (Spatial Hash Grid, XP Collection)

---

## Table of Contents
1. [Overview](#overview)
2. [Implementation Steps](#implementation-steps)
3. [Step 1: Batch Renderer Core](#step-1-batch-renderer-core)
4. [Step 2: OpenGL Implementation](#step-2-opengl-implementation)
5. [Step 3: Backend Integration](#step-3-backend-integration)
6. [Step 4: Shader Infrastructure](#step-4-shader-infrastructure)
7. [Step 5: ECS Integration](#step-5-ecs-integration)
8. [Step 6: Object Pooling](#step-6-object-pooling)
9. [Step 7: Performance Monitoring](#step-7-performance-monitoring)
10. [Step 8: Testing & Benchmarking](#step-8-testing--benchmarking)
11. [Performance Targets](#performance-targets)
12. [Final Checklist](#final-checklist)

---

## Overview

Phase 5 implements **GPU-accelerated batch rendering** to efficiently render tens of thousands of sprites at 60 FPS. The implementation is broken down into 8 sequential steps, each building on the previous one.

### Key Goals
- ? Render **50,000+ sprites** at 60 FPS
- ? Minimize draw calls (target: 1-5 per frame)
- ? Support texture batching (group by texture)
- ? Support Z-ordering (proper depth sorting)
- ? Maintain compatibility with existing `SpriteRenderSystem`

### Architecture Overview

```
???????????????????????????????????????????????????????????????
?              SpriteRenderSystem (Step 5)                    ?
?  - Collects all sprites                                     ?
?  - Sorts by (texture, Z-order)                              ?
?  - Calls Renderer2DBackend::DrawQuad()                      ?
???????????????????????????????????????????????????????????????
                            ?
???????????????????????????????????????????????????????????????
?         Renderer2DBackend (Step 3)                          ?
?  - Facade pattern, forwards to active implementation        ?
?  - Can switch between Basic/Batch renderers                 ?
???????????????????????????????????????????????????????????????
                            ?
???????????????????????????????????????????????????????????????
?         BatchRenderer2D (Steps 1-2)                         ?
?  - Accumulates quads by texture                             ?
?  - Dynamic vertex buffer (GL_DYNAMIC_DRAW)                  ?
?  - Flushes on texture change or buffer full                 ?
?  - Uses instanced shaders (Step 4)                          ?
???????????????????????????????????????????????????????????????
```

---

## Implementation Steps

| Step | Component | Duration | Dependencies |
|------|-----------|----------|--------------|
| **1** | BatchRenderer2D Core | 1-2h | None |
| **2** | OpenGL Implementation | 2-3h | Step 1 |
| **3** | Backend Integration | 1h | Steps 1-2 |
| **4** | Shader Infrastructure | 1h | Step 2 |
| **5** | ECS Integration | 1-2h | Steps 1-4 |
| **6** | Object Pooling | 1h | Step 5 |
| **7** | Performance Monitoring | 1h | Steps 1-6 |
| **8** | Testing & Benchmarking | 2-3h | All steps |
| **Total** | | **10-13h** | |

---

## Step 1: Batch Renderer Core

**Duration:** 1-2 hours  
**Goal:** Create abstract interface for batch rendering

### Files to Create
- `Pillar/src/Pillar/Renderer/BatchRenderer2D.h`
- `Pillar/src/Pillar/Renderer/BatchRenderer2D.cpp`

### Implementation

```cpp
// Pillar/src/Pillar/Renderer/BatchRenderer2D.h
#pragma once

#include "Pillar/Core.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include <memory>

namespace Pillar {

    class VertexArray;
    class VertexBuffer;
    class IndexBuffer;
    class Shader;

    /**
     * @brief Batch Renderer for 2D Quads
     * 
     * Accumulates quads into texture-based batches and submits
     * them to the GPU in a single draw call per texture.
     * 
     * Performance Target:
     * - 50,000 quads at 60 FPS
     * - 1-5 draw calls per frame (depends on unique textures)
     */
    class PIL_API BatchRenderer2D : public IRenderer2D
    {
    public:
        static const uint32_t MaxQuadsPerBatch = 10000;
        static const uint32_t MaxVertices = MaxQuadsPerBatch * 4;
        static const uint32_t MaxIndices = MaxQuadsPerBatch * 6;

        virtual ~BatchRenderer2D() = default;

        // Factory method (creates OpenGLBatchRenderer2D)
        static BatchRenderer2D* Create();

        // IRenderer2D interface
        void BeginScene(const OrthographicCamera& camera) override;
        void EndScene() override;

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color) override;
        
        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture) override;
        
        void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture,
                     const glm::vec2& texCoordMin, const glm::vec2& texCoordMax) override;

        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color) override;
        
        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color, Texture2D* texture) override;

        // Stats
        uint32_t GetDrawCallCount() const override { return m_Stats.DrawCalls; }
        uint32_t GetQuadCount() const override { return m_Stats.QuadCount; }
        void ResetStats() override;

    protected:
        struct Stats
        {
            uint32_t DrawCalls = 0;
            uint32_t QuadCount = 0;
            uint32_t VertexCount = 0;
        };

        Stats m_Stats;

        // Subclasses implement these
        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Flush() = 0;  // Submit current batch to GPU
        virtual void FlushAndReset() = 0;  // Flush + prepare for next batch
    };

} // namespace Pillar
```

```cpp
// Pillar/src/Pillar/Renderer/BatchRenderer2D.cpp
#include "BatchRenderer2D.h"
#include "Pillar/Renderer/RenderAPI.h"
#include "Platform/OpenGL/OpenGLBatchRenderer2D.h"
#include "Pillar/Logger.h"

namespace Pillar {

    BatchRenderer2D* BatchRenderer2D::Create()
    {
        switch (RenderAPI::GetAPI())
        {
            case RendererAPI::OpenGL:
                return new OpenGLBatchRenderer2D();
            case RendererAPI::None:
                PIL_CORE_ASSERT(false, "RendererAPI::None is not supported!");
                return nullptr;
        }

        PIL_CORE_ASSERT(false, "Unknown RendererAPI!");
        return nullptr;
    }

    void BatchRenderer2D::BeginScene(const OrthographicCamera& camera)
    {
        // Default implementation (subclass can override)
    }

    void BatchRenderer2D::EndScene()
    {
        // Default implementation (subclass can override)
    }

    void BatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                  const glm::vec4& color)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                  const glm::vec4& color, Texture2D* texture)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                  const glm::vec4& color, Texture2D* texture,
                                  const glm::vec2& texCoordMin, const glm::vec2& texCoordMax)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                         float rotation, const glm::vec4& color)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                         float rotation, const glm::vec4& color, Texture2D* texture)
    {
        // Default implementation (subclass must override)
    }

    void BatchRenderer2D::ResetStats()
    {
        m_Stats.DrawCalls = 0;
        m_Stats.QuadCount = 0;
        m_Stats.VertexCount = 0;
    }

} // namespace Pillar
```

### Validation
- [ ] Code compiles without errors
- [ ] `BatchRenderer2D::Create()` returns non-null pointer (will crash until Step 2)
- [ ] Interface matches `IRenderer2D` contract

---

## Step 2: OpenGL Implementation

**Duration:** 2-3 hours  
**Goal:** Implement OpenGL-specific batch rendering logic

### Files to Create
- `Platform/OpenGL/OpenGLBatchRenderer2D.h`
- `Platform/OpenGL/OpenGLBatchRenderer2D.cpp`

### Implementation

```cpp
// Platform/OpenGL/OpenGLBatchRenderer2D.h
#pragma once

#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/VertexArray.h"
#include "Pillar/Renderer/Shader.h"
#include "Pillar/Renderer/Texture.h"
#include <unordered_map>
#include <vector>
#include <array>

namespace Pillar {

    /**
     * @brief OpenGL-specific batch renderer
     * 
     * Implementation Details:
     * - Uses dynamic vertex buffer (GL_DYNAMIC_DRAW)
     * - Batches quads by texture (minimize texture swaps)
     * - Uploads data with glBufferSubData per flush
     * - Uses indexed rendering (6 indices per quad)
     */
    class OpenGLBatchRenderer2D : public BatchRenderer2D
    {
    public:
        OpenGLBatchRenderer2D();
        ~OpenGLBatchRenderer2D() override;

        // IRenderer2D interface
        void BeginScene(const OrthographicCamera& camera) override;
        void EndScene() override;

        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color) override;
        
        void DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture) override;
        
        void DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                     const glm::vec4& color, Texture2D* texture,
                     const glm::vec2& texCoordMin, const glm::vec2& texCoordMax) override;

        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color) override;
        
        void DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                            float rotation, const glm::vec4& color, Texture2D* texture) override;

    protected:
        void Init() override;
        void Shutdown() override;
        void Flush() override;
        void FlushAndReset() override;

    private:
        // Vertex structure (per quad corner)
        struct QuadVertex
        {
            glm::vec3 Position;
            glm::vec4 Color;
            glm::vec2 TexCoord;
            float TexIndex;  // Which texture slot (0-31)
        };

        // Batch data structure (per texture)
        struct QuadBatch
        {
            Texture2D* Texture = nullptr;
            std::vector<QuadVertex> Vertices;  // 4 vertices per quad
            uint32_t QuadCount = 0;

            void Clear()
            {
                Vertices.clear();
                QuadCount = 0;
            }
        };

        // Rendering resources
        std::shared_ptr<VertexArray> m_QuadVertexArray;
        std::shared_ptr<VertexBuffer> m_QuadVertexBuffer;
        std::shared_ptr<IndexBuffer> m_QuadIndexBuffer;
        std::shared_ptr<Shader> m_BatchShader;
        std::shared_ptr<Texture2D> m_WhiteTexture;  // For colored quads

        // Batch storage (key = texture ID)
        std::unordered_map<uint32_t, QuadBatch> m_Batches;
        
        // Texture slots (OpenGL supports 32 texture units)
        static const uint32_t MaxTextureSlots = 32;
        std::array<Texture2D*, MaxTextureSlots> m_TextureSlots;
        uint32_t m_TextureSlotIndex = 1;  // 0 = white texture

        // Camera
        glm::mat4 m_ViewProjectionMatrix;

        // Helper methods
        void StartBatch();
        void NextBatch();
        uint32_t GetOrAddTextureSlot(Texture2D* texture);
        void AddQuadToBatch(const glm::vec3& position, const glm::vec2& size,
                           const glm::vec4& color, Texture2D* texture,
                           const glm::vec2& texCoordMin, const glm::vec2& texCoordMax,
                           float rotation = 0.0f);
    };

} // namespace Pillar
```

**Full implementation in `OpenGLBatchRenderer2D.cpp`** (see original plan for complete code - ~200 lines)

### Key Features to Implement
1. **Init()**: 
   - Create vertex array/buffer/index buffer
   - Generate white texture (1x1 pixel)
   - Create batch shader (embedded GLSL code)
   - Initialize texture slots array

2. **AddQuadToBatch()**:
   - Calculate quad vertices with rotation
   - Get/assign texture slot
   - Add 4 vertices to batch
   - Auto-flush if batch full

3. **Flush()**:
   - Bind shader + set uniforms
   - Bind all textures to slots
   - Upload vertex data to GPU
   - Draw each batch with `glDrawElements`

4. **Texture Slot Management**:
   - Track up to 32 textures
   - Reuse slots for same texture
   - Flush when slots exhausted

### Validation
- [ ] Batch renderer initializes without errors
- [ ] Can draw single colored quad
- [ ] Can draw textured quad
- [ ] Rotation works correctly
- [ ] Stats tracking works (draw calls, quad count)

---

## Step 3: Backend Integration

**Duration:** 1 hour  
**Goal:** Integrate batch renderer into `Renderer2DBackend`

### Files to Modify
- `Pillar/src/Pillar/Renderer/Renderer2DBackend.cpp` (create if doesn't exist)

### Implementation

```cpp
// Pillar/src/Pillar/Renderer/Renderer2DBackend.cpp
#include "Renderer2DBackend.h"
#include "BatchRenderer2D.h"
#include "BasicRenderer2D.h"  // Assume this exists
#include "Pillar/Logger.h"

namespace Pillar {

    IRenderer2D* Renderer2DBackend::s_ActiveRenderer = nullptr;
    IRenderer2D* Renderer2DBackend::s_BasicRenderer = nullptr;
    IRenderer2D* Renderer2DBackend::s_BatchRenderer = nullptr;

    void Renderer2DBackend::Init(API api)
    {
        PIL_CORE_INFO("Initializing Renderer2DBackend...");

        // Create both renderers
        s_BasicRenderer = BasicRenderer2D::Create();
        s_BatchRenderer = BatchRenderer2D::Create();

        // Set active renderer
        SetAPI(api);

        PIL_CORE_INFO("Renderer2DBackend initialized with API: {0}", 
                     api == API::Basic ? "Basic" : "Batch");
    }

    void Renderer2DBackend::Shutdown()
    {
        delete s_BasicRenderer;
        delete s_BatchRenderer;
        s_ActiveRenderer = nullptr;
    }

    void Renderer2DBackend::SetAPI(API api)
    {
        switch (api)
        {
            case API::Basic:
                s_ActiveRenderer = s_BasicRenderer;
                PIL_CORE_INFO("Switched to BasicRenderer2D");
                break;
            case API::Batch:
                s_ActiveRenderer = s_BatchRenderer;
                PIL_CORE_INFO("Switched to BatchRenderer2D");
                break;
        }
    }

    // Forward all calls to active renderer
    void Renderer2DBackend::BeginScene(const OrthographicCamera& camera)
    {
        s_ActiveRenderer->BeginScene(camera);
    }

    void Renderer2DBackend::EndScene()
    {
        s_ActiveRenderer->EndScene();
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color)
    {
        s_ActiveRenderer->DrawQuad(position, size, color);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec2& position, const glm::vec2& size, 
                                    const glm::vec4& color, Texture2D* texture)
    {
        s_ActiveRenderer->DrawQuad(position, size, color, texture);
    }

    void Renderer2DBackend::DrawQuad(const glm::vec3& position, const glm::vec2& size, 
                                    const glm::vec4& color, Texture2D* texture,
                                    const glm::vec2& texCoordMin, const glm::vec2& texCoordMax)
    {
        s_ActiveRenderer->DrawQuad(position, size, color, texture, texCoordMin, texCoordMax);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color)
    {
        s_ActiveRenderer->DrawRotatedQuad(position, size, rotation, color);
    }

    void Renderer2DBackend::DrawRotatedQuad(const glm::vec2& position, const glm::vec2& size,
                                           float rotation, const glm::vec4& color, Texture2D* texture)
    {
        s_ActiveRenderer->DrawRotatedQuad(position, size, rotation, color, texture);
    }

    uint32_t Renderer2DBackend::GetDrawCallCount()
    {
        return s_ActiveRenderer->GetDrawCallCount();
    }

    uint32_t Renderer2DBackend::GetQuadCount()
    {
        return s_ActiveRenderer->GetQuadCount();
    }

    void Renderer2DBackend::ResetStats()
    {
        s_ActiveRenderer->ResetStats();
    }

} // namespace Pillar
```

### Update Application.cpp

```cpp
// In Application::Init() or similar
Renderer2DBackend::Init(Renderer2DBackend::API::Batch);  // Use batch renderer

// In Application::Shutdown()
Renderer2DBackend::Shutdown();
```

### Validation
- [ ] Can switch between Basic/Batch renderers at runtime
- [ ] `Renderer2DBackend::Init(API::Batch)` uses batch renderer
- [ ] All forwarding methods work correctly
- [ ] Application initializes without errors

---

## Step 4: Shader Infrastructure

**Duration:** 1 hour  
**Goal:** Extract shaders to external files (optional but recommended)

### Files to Create
- `Pillar/src/Pillar/Renderer/Shaders/BatchQuad.vert`
- `Pillar/src/Pillar/Renderer/Shaders/BatchQuad.frag`

### Implementation

```glsl
// Pillar/src/Pillar/Renderer/Shaders/BatchQuad.vert
#version 410 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in float a_TexIndex;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out float v_TexIndex;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexIndex = a_TexIndex;
    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}
```

```glsl
// Pillar/src/Pillar/Renderer/Shaders/BatchQuad.frag
#version 410 core

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in float v_TexIndex;

uniform sampler2D u_Textures[32];

void main()
{
    int texIndex = int(v_TexIndex);
    color = texture(u_Textures[texIndex], v_TexCoord) * v_Color;
}
```

### Optional: Shader Loading

If you want to load from files instead of embedded strings:

```cpp
// In OpenGLBatchRenderer2D::Init()
m_BatchShader = std::shared_ptr<Shader>(
    Shader::Create("Pillar/Renderer/Shaders/BatchQuad.vert",
                   "Pillar/Renderer/Shaders/BatchQuad.frag")
);
```

### Validation
- [ ] Shaders compile successfully
- [ ] Rendering still works with external shaders
- [ ] Shader hot-reloading works (if implemented)

---

## Step 5: ECS Integration

**Duration:** 1-2 hours  
**Goal:** Update `SpriteRenderSystem` to use batch renderer

### Files to Modify
- `Pillar/src/Pillar/ECS/Systems/SpriteRenderSystem.cpp`
- `Pillar/src/Pillar/ECS/Systems/SpriteRenderSystem.h` (if needed)

### Implementation

```cpp
// Pillar/src/Pillar/ECS/Systems/SpriteRenderSystem.cpp
#include "SpriteRenderSystem.h"
#include "Pillar/ECS/Scene.h"
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/Renderer/Renderer2DBackend.h"
#include "Pillar/Logger.h"
#include <algorithm>

namespace Pillar {

    void SpriteRenderSystem::OnUpdate(float dt)
    {
        // Collect all entities with sprite + transform
        auto view = m_Scene->GetRegistry().view<TransformComponent, SpriteComponent>();

        // Sort by (Texture, ZIndex) for optimal batching
        std::vector<entt::entity> sortedEntities(view.begin(), view.end());
        std::sort(sortedEntities.begin(), sortedEntities.end(),
            [&view](entt::entity a, entt::entity b) {
                const auto& spriteA = view.get<SpriteComponent>(a);
                const auto& spriteB = view.get<SpriteComponent>(b);
                
                // Sort by texture first (minimize texture swaps)
                if (spriteA.Texture != spriteB.Texture)
                {
                    // Compare texture pointers
                    return spriteA.Texture.get() < spriteB.Texture.get();
                }
                
                // Then by Z-order
                return spriteA.ZIndex < spriteB.ZIndex;
            });

        // Render each sprite (batch renderer accumulates internally)
        for (auto entity : sortedEntities)
        {
            auto& transform = view.get<TransformComponent>(entity);
            auto& sprite = view.get<SpriteComponent>(entity);

            RenderSprite(transform, sprite);
        }
    }

    void SpriteRenderSystem::RenderSprite(const TransformComponent& transform,
                                         const SpriteComponent& sprite)
    {
        // Handle rotation if needed
        if (transform.Rotation != 0.0f)
        {
            Renderer2DBackend::DrawRotatedQuad(
                transform.Position,
                sprite.Size * transform.Scale,
                transform.Rotation,
                sprite.Color,
                sprite.Texture.get()
            );
        }
        else
        {
            // Use texture coordinates for sprite sheet support
            Renderer2DBackend::DrawQuad(
                glm::vec3(transform.Position, sprite.ZIndex),
                sprite.Size * transform.Scale,
                sprite.Color,
                sprite.Texture.get(),
                sprite.TexCoordMin,
                sprite.TexCoordMax
            );
        }
    }

} // namespace Pillar
```

### Update GameLayer or Scene

```cpp
// In GameLayer::OnUpdate(float dt) or similar
Renderer2DBackend::BeginScene(m_Camera);
Renderer2DBackend::ResetStats();  // Reset stats each frame

// Systems update
m_SpriteRenderSystem->OnUpdate(dt);

Renderer2DBackend::EndScene();
```

### Validation
- [ ] Sprites render correctly with batch renderer
- [ ] Sorting by texture + Z-order works
- [ ] Rotation works for sprites
- [ ] Texture coordinates work (sprite sheets)
- [ ] Performance improves over basic renderer

---

## Step 6: Object Pooling

**Duration:** 1 hour  
**Goal:** Implement generic object pool for entity recycling

### Files to Create
- `Pillar/src/Pillar/Utils/ObjectPool.h`

### Implementation

```cpp
// Pillar/src/Pillar/Utils/ObjectPool.h
#pragma once

#include <vector>
#include <functional>

namespace Pillar {

    /**
     * @brief Generic object pool for entity recycling
     * 
     * Reduces allocations by reusing dead entities (bullets, particles, etc.)
     * 
     * Usage:
     *   ObjectPool<Entity> bulletPool;
     *   bulletPool.SetFactory([&]() { return scene.CreateEntity("Bullet"); });
     *   bulletPool.SetResetFunction([](Entity& e) { /* reset state */ });
     *   
     *   Entity bullet = bulletPool.Acquire();
     *   // ... use bullet ...
     *   bulletPool.Release(bullet);
     */
    template<typename T>
    class ObjectPool
    {
    public:
        using FactoryFunc = std::function<T()>;
        using ResetFunc = std::function<void(T&)>;

        ObjectPool() = default;

        // Set factory function (creates new objects)
        void SetFactory(FactoryFunc factory)
        {
            m_Factory = factory;
        }

        // Set reset function (resets object state)
        void SetResetFunction(ResetFunc resetFunc)
        {
            m_ResetFunc = resetFunc;
        }

        // Acquire object from pool (or create new)
        T Acquire()
        {
            if (m_Pool.empty())
            {
                if (m_Factory)
                {
                    return m_Factory();
                }
                else
                {
                    return T();  // Default construct
                }
            }

            T obj = m_Pool.back();
            m_Pool.pop_back();
            return obj;
        }

        // Release object back to pool
        void Release(T obj)
        {
            if (m_ResetFunc)
            {
                m_ResetFunc(obj);
            }
            m_Pool.push_back(obj);
        }

        // Get current pool size
        size_t Size() const { return m_Pool.size(); }

        // Get active object count
        size_t GetActiveCount() const { return m_ActiveCount; }

        // Clear pool
        void Clear() 
        { 
            m_Pool.clear(); 
            m_ActiveCount = 0;
        }

        // Reserve space (optimization)
        void Reserve(size_t size)
        {
            m_Pool.reserve(size);
        }

    private:
        std::vector<T> m_Pool;
        FactoryFunc m_Factory;
        ResetFunc m_ResetFunc;
        size_t m_ActiveCount = 0;
    };

} // namespace Pillar
```

### Usage Example

```cpp
// In GameLayer.h
ObjectPool<Entity> m_BulletPool;

// In GameLayer::OnAttach()
m_BulletPool.SetFactory([this]() {
    Entity bullet = m_Scene->CreateEntity("Bullet");
    bullet.AddComponent<TransformComponent>();
    bullet.AddComponent<VelocityComponent>();
    bullet.AddComponent<BulletComponent>();
    bullet.AddComponent<SpriteComponent>(m_BulletTexture);
    return bullet;
});

m_BulletPool.SetResetFunction([this](Entity& bullet) {
    auto& transform = bullet.GetComponent<TransformComponent>();
    auto& velocity = bullet.GetComponent<VelocityComponent>();
    auto& bulletComp = bullet.GetComponent<BulletComponent>();
    
    transform.Position = glm::vec2(0, 0);
    velocity.Velocity = glm::vec2(0, 0);
    bulletComp.TimeAlive = 0.0f;
    bulletComp.HitsRemaining = bulletComp.MaxHits;
});

// Reserve space for 1000 bullets
m_BulletPool.Reserve(1000);

// In GameLayer::ShootBullet()
Entity bullet = m_BulletPool.Acquire();
auto& transform = bullet.GetComponent<TransformComponent>();
auto& velocity = bullet.GetComponent<VelocityComponent>();
transform.Position = playerPos;
velocity.Velocity = direction * 50.0f;

// When bullet dies
m_BulletPool.Release(bullet);
```

### Validation
- [ ] Pool acquires objects correctly
- [ ] Pool releases objects back
- [ ] Factory creates new objects when pool empty
- [ ] Reset function clears object state
- [ ] Memory usage stable (no leaks)

---

## Step 7: Performance Monitoring

**Duration:** 1 hour  
**Goal:** Create ImGui panel for rendering statistics

### Files to Create
- `Sandbox/src/RenderStatsPanel.h`
- `Sandbox/src/RenderStatsPanel.cpp`

### Implementation

```cpp
// Sandbox/src/RenderStatsPanel.h
#pragma once

#include <imgui.h>
#include <vector>

namespace Sandbox {

    /**
     * @brief ImGui panel showing rendering statistics
     * 
     * Displays:
     * - FPS / Frame time
     * - Draw calls per frame
     * - Quad count
     * - Entity count
     * - Performance graphs
     */
    class RenderStatsPanel
    {
    public:
        void OnImGuiRender();
        
        void UpdateStats(float fps, float frameTime, uint32_t drawCalls, 
                        uint32_t quadCount, uint32_t entityCount);

    private:
        float m_FPS = 0.0f;
        float m_FrameTime = 0.0f;
        uint32_t m_DrawCalls = 0;
        uint32_t m_QuadCount = 0;
        uint32_t m_EntityCount = 0;

        // History for graphs
        static const int MaxHistorySize = 100;
        std::vector<float> m_FPSHistory;
        std::vector<float> m_FrameTimeHistory;
        std::vector<float> m_DrawCallHistory;
    };

} // namespace Sandbox
```

```cpp
// Sandbox/src/RenderStatsPanel.cpp
#include "RenderStatsPanel.h"

namespace Sandbox {

    void RenderStatsPanel::OnImGuiRender()
    {
        ImGui::Begin("Rendering Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        // Performance section
        ImGui::Text("Performance");
        ImGui::Separator();
        ImGui::Text("FPS: %.1f", m_FPS);
        ImGui::Text("Frame Time: %.2f ms", m_FrameTime);
        
        // Color code frame time
        if (m_FrameTime > 16.67f)
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "WARNING: < 60 FPS");
        else
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "60+ FPS");

        ImGui::Spacing();

        // Rendering section
        ImGui::Text("Rendering");
        ImGui::Separator();
        ImGui::Text("Draw Calls: %u", m_DrawCalls);
        ImGui::Text("Quad Count: %u", m_QuadCount);
        ImGui::Text("Entity Count: %u", m_EntityCount);

        ImGui::Spacing();

        // Graphs section
        ImGui::Text("Performance Graphs");
        ImGui::Separator();
        
        // FPS graph
        ImGui::Text("FPS History");
        ImGui::PlotLines("##FPS", m_FPSHistory.data(), (int)m_FPSHistory.size(),
                        0, nullptr, 0.0f, 120.0f, ImVec2(300, 80));
        
        // Frame time graph
        ImGui::Text("Frame Time (ms)");
        ImGui::PlotLines("##FrameTime", m_FrameTimeHistory.data(), (int)m_FrameTimeHistory.size(),
                        0, nullptr, 0.0f, 33.0f, ImVec2(300, 80));
        
        // Draw call graph
        ImGui::Text("Draw Calls");
        ImGui::PlotLines("##DrawCalls", m_DrawCallHistory.data(), (int)m_DrawCallHistory.size(),
                        0, nullptr, 0.0f, 50.0f, ImVec2(300, 80));

        ImGui::End();
    }

    void RenderStatsPanel::UpdateStats(float fps, float frameTime, uint32_t drawCalls, 
                                       uint32_t quadCount, uint32_t entityCount)
    {
        m_FPS = fps;
        m_FrameTime = frameTime;
        m_DrawCalls = drawCalls;
        m_QuadCount = quadCount;
        m_EntityCount = entityCount;

        // Update FPS history
        m_FPSHistory.push_back(fps);
        if (m_FPSHistory.size() > MaxHistorySize)
            m_FPSHistory.erase(m_FPSHistory.begin());

        // Update frame time history
        m_FrameTimeHistory.push_back(frameTime);
        if (m_FrameTimeHistory.size() > MaxHistorySize)
            m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());

        // Update draw call history
        m_DrawCallHistory.push_back((float)drawCalls);
        if (m_DrawCallHistory.size() > MaxHistorySize)
            m_DrawCallHistory.erase(m_DrawCallHistory.begin());
    }

} // namespace Sandbox
```

### Usage in GameLayer

```cpp
// In GameLayer.h
#include "RenderStatsPanel.h"

class GameLayer : public Layer
{
private:
    RenderStatsPanel m_StatsPanel;
};

// In GameLayer::OnUpdate(float dt)
float fps = 1.0f / dt;
uint32_t entityCount = m_Scene->GetRegistry().size();

m_StatsPanel.UpdateStats(
    fps, 
    dt * 1000.0f,  // Convert to milliseconds
    Renderer2DBackend::GetDrawCallCount(),
    Renderer2DBackend::GetQuadCount(),
    entityCount
);

// In GameLayer::OnImGuiRender()
m_StatsPanel.OnImGuiRender();
```

### Validation
- [ ] ImGui panel appears correctly
- [ ] Stats update in real-time
- [ ] Graphs render smoothly
- [ ] Color coding works (red when < 60 FPS)
- [ ] Panel is resizable/closable

---

## Step 8: Testing & Benchmarking

**Duration:** 2-3 hours  
**Goal:** Comprehensive testing and performance validation

### Files to Create
- `Tests/src/BatchRenderer2DTests.cpp`
- `Tests/src/SpriteRenderSystemTests.cpp` (update existing)
- `Tests/src/PerformanceBenchmarks.cpp` (update existing)

### Test Suite

```cpp
// Tests/src/BatchRenderer2DTests.cpp
#include <gtest/gtest.h>
#include "Pillar/Renderer/BatchRenderer2D.h"
#include "Pillar/Renderer/OrthographicCamera.h"
#include "Pillar/Renderer/Texture.h"

class BatchRenderer2DTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize rendering context (if needed)
    }

    void TearDown() override
    {
        // Cleanup
    }
};

TEST_F(BatchRenderer2DTests, CreateBatchRenderer_Succeeds)
{
    auto* renderer = BatchRenderer2D::Create();
    EXPECT_NE(renderer, nullptr);
    delete renderer;
}

TEST_F(BatchRenderer2DTests, DrawSingleQuad_IncrementsQuadCount)
{
    auto* renderer = BatchRenderer2D::Create();
    OrthographicCamera camera(-10, 10, -10, 10);
    
    renderer->BeginScene(camera);
    renderer->DrawQuad(glm::vec2(0, 0), glm::vec2(1, 1), glm::vec4(1, 1, 1, 1));
    renderer->EndScene();
    
    EXPECT_EQ(renderer->GetQuadCount(), 1);
    delete renderer;
}

TEST_F(BatchRenderer2DTests, Draw10000Quads_SingleDrawCall)
{
    auto* renderer = BatchRenderer2D::Create();
    OrthographicCamera camera(-100, 100, -100, 100);
    
    renderer->BeginScene(camera);
    for (int i = 0; i < 10000; ++i)
    {
        renderer->DrawQuad(glm::vec2(i % 100, i / 100), glm::vec2(1, 1), 
                          glm::vec4(1, 1, 1, 1));
    }
    renderer->EndScene();
    
    EXPECT_EQ(renderer->GetQuadCount(), 10000);
    EXPECT_EQ(renderer->GetDrawCallCount(), 1);  // All same texture (white)
    delete renderer;
}

TEST_F(BatchRenderer2DTests, MultipleTextures_MultipleDrawCalls)
{
    auto* renderer = BatchRenderer2D::Create();
    OrthographicCamera camera(-10, 10, -10, 10);
    
    auto texture1 = Texture2D::Create(1, 1);
    auto texture2 = Texture2D::Create(1, 1);
    
    renderer->BeginScene(camera);
    renderer->DrawQuad(glm::vec2(0, 0), glm::vec2(1, 1), glm::vec4(1), texture1.get());
    renderer->DrawQuad(glm::vec2(1, 0), glm::vec2(1, 1), glm::vec4(1), texture2.get());
    renderer->DrawQuad(glm::vec2(2, 0), glm::vec2(1, 1), glm::vec4(1), texture1.get());
    renderer->EndScene();
    
    EXPECT_EQ(renderer->GetQuadCount(), 3);
    EXPECT_GE(renderer->GetDrawCallCount(), 2);  // At least 2 (2 unique textures)
    delete renderer;
}

TEST_F(BatchRenderer2DTests, RotatedQuad_RendersCorrectly)
{
    auto* renderer = BatchRenderer2D::Create();
    OrthographicCamera camera(-10, 10, -10, 10);
    
    renderer->BeginScene(camera);
    renderer->DrawRotatedQuad(glm::vec2(0, 0), glm::vec2(1, 1), 
                             glm::radians(45.0f), glm::vec4(1, 0, 0, 1));
    renderer->EndScene();
    
    EXPECT_EQ(renderer->GetQuadCount(), 1);
    delete renderer;
}
```

### Performance Benchmark

```cpp
// Tests/src/PerformanceBenchmarks.cpp (add to existing file)

TEST(PerformanceBenchmarks, BatchRenderer_50000Sprites_60FPS)
{
    // This test may require a real rendering context
    // Skip if running headless
    
    auto* renderer = BatchRenderer2D::Create();
    OrthographicCamera camera(-224, 224, -224, 224);
    
    auto texture = Texture2D::Create(1, 1);
    
    // Measure rendering time
    auto start = std::chrono::high_resolution_clock::now();
    
    renderer->BeginScene(camera);
    for (int i = 0; i < 50000; ++i)
    {
        renderer->DrawQuad(
            glm::vec2((i % 224) * 2, (i / 224) * 2), 
            glm::vec2(1, 1), 
            glm::vec4(1.0f),
            texture.get()
        );
    }
    renderer->EndScene();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    PIL_CORE_INFO("Rendered 50,000 sprites in {0} ms", duration.count());
    
    EXPECT_LT(duration.count(), 16);  // < 16 ms = 60+ FPS
    EXPECT_EQ(renderer->GetQuadCount(), 50000);
    EXPECT_LE(renderer->GetDrawCallCount(), 10);  // Should be 1-5 typically
    
    delete renderer;
}
```

### Manual Testing Checklist

- [ ] **Visual Test**: 1,000 sprites render correctly
- [ ] **Visual Test**: 10,000 sprites render at 60 FPS
- [ ] **Visual Test**: 50,000 sprites render (may drop below 60 FPS)
- [ ] **Visual Test**: Multiple textures batch correctly
- [ ] **Visual Test**: Rotation works
- [ ] **Visual Test**: Z-ordering works (sprites render in correct order)
- [ ] **Visual Test**: Sprite sheets work (texture coordinates)
- [ ] **Performance**: Draw calls < 5 with 10,000 sprites (1 texture)
- [ ] **Performance**: Frame time < 16.67 ms with 10,000 sprites
- [ ] **Memory**: No memory leaks after 1000 frames
- [ ] **Memory**: Object pool reuses entities correctly

### Benchmark Results Table

Fill this in after testing:

| Test | Sprite Count | Unique Textures | Draw Calls | Frame Time | FPS | Pass/Fail |
|------|--------------|-----------------|------------|------------|-----|-----------|
| Basic | 1,000 | 1 | | | | |
| Medium | 10,000 | 1 | | | | |
| High | 50,000 | 1 | | | | |
| Multi-Texture | 10,000 | 5 | | | | |

### Validation
- [ ] All unit tests pass
- [ ] Performance benchmarks pass
- [ ] Visual tests look correct
- [ ] No regressions in existing systems

---

## Performance Targets

### Frame Budget (60 FPS)

| Component | Time Budget | Actual | Status |
|-----------|-------------|--------|--------|
| Batch Renderer | < 5 ms | | |
| ECS Systems | < 5 ms | | |
| Other (ImGui, etc.) | < 6.67 ms | | |
| **Total Frame** | **< 16.67 ms** | | |

### Rendering Metrics

| Metric | Target | Actual | Status |
|--------|--------|--------|--------|
| Sprites Rendered | 50,000+ | | |
| Draw Calls | 1-5 | | |
| FPS | 60+ | | |
| Frame Time | < 16.67 ms | | |

### Memory Usage

| Component | Target | Actual | Status |
|-----------|--------|--------|--------|
| Batch Renderer | < 50 MB | | |
| Object Pools | < 20 MB | | |
| Texture Memory | < 100 MB | | |

---

## Final Checklist

### Step Completion
- [ ] **Step 1**: BatchRenderer2D core implemented
- [ ] **Step 2**: OpenGL implementation complete
- [ ] **Step 3**: Backend integration working
- [ ] **Step 4**: Shaders created (embedded or files)
- [ ] **Step 5**: ECS integration with sorting
- [ ] **Step 6**: Object pooling implemented
- [ ] **Step 7**: Stats panel working
- [ ] **Step 8**: All tests passing

### Code Quality
- [ ] No compiler warnings
- [ ] No memory leaks (Valgrind/Dr. Memory)
- [ ] Code follows project style guide
- [ ] All public APIs documented
- [ ] Complex algorithms commented

### Testing
- [ ] Unit tests pass (5+ batch renderer tests)
- [ ] Integration tests pass
- [ ] Performance benchmarks pass
- [ ] Visual tests pass
- [ ] No regressions in existing systems

### Documentation
- [ ] Updated `PHASE_5_SUMMARY.md`
- [ ] Updated CMakeLists.txt with new files
- [ ] Added usage examples to README
- [ ] Documented performance characteristics
- [ ] Created troubleshooting guide

### Performance
- [ ] 50,000 sprites at 60 FPS ?
- [ ] Draw calls < 5 per frame ?
- [ ] Memory usage stable ?
- [ ] No frame spikes ?

---

## Next Steps After Phase 5

**Phase 6: Health & Damage System** (2-3 hours)
- Define `HealthComponent`
- Implement `HealthSystem`
- Integrate bullet collision with damage
- Emit death events for VFX/loot

**Phase 7: Particle System** (4-5 hours)
- Define `ParticleEmitterComponent`
- Implement `ParticleSystem` (spawn, update, render)
- Use batch renderer for particle instancing
- Target: 10,000+ particles at 60 FPS

---

**Phase 5 Ready for Sequential Implementation!** ??

Follow the steps in order, validating each before moving to the next. The batch rendering system will dramatically improve performance and enable particle-heavy gameplay!
