# Renderer Backend Comparison

## Usage in ObjectPoolDemo

```cpp
void ObjectPoolDemo::OnAttach()
{
    // Choose renderer backend
    // Team A: Use Basic (immediate mode)
    Renderer2DBackend::Init(Renderer2DBackend::API::Basic);
    
    // Team B: Use Batch (after implementation complete)
    // Renderer2DBackend::Init(Renderer2DBackend::API::Batch);
    
    // Rest of initialization...
    m_Scene = new Scene();
    m_Scene->AddSystem<SpriteRenderSystem>();
    // ...
}

void ObjectPoolDemo::Render()
{
    Renderer::SetClearColor({ 0.1f, 0.1f, 0.1f, 1.0f });
    Renderer::Clear();
    
    Renderer2DBackend::BeginScene(m_CameraController.GetCamera());
    
    // SpriteRenderSystem will use active backend (Basic or Batch)
    m_Scene->UpdateSystems(0.0f);
    
    Renderer2DBackend::EndScene();
}

void ObjectPoolDemo::OnImGuiRender()
{
    ImGui::Begin("Renderer Comparison");
    
    // Toggle between renderers
    static int selectedAPI = 0;
    if (ImGui::Combo("Renderer API", &selectedAPI, "Basic\0Batch\0"))
    {
        auto api = selectedAPI == 0 ? 
            Renderer2DBackend::API::Basic : 
            Renderer2DBackend::API::Batch;
        Renderer2DBackend::SetAPI(api);
    }
    
    // Show stats
    ImGui::Separator();
    ImGui::Text("Performance Stats:");
    ImGui::Text("Draw Calls: %u", Renderer2DBackend::GetDrawCallCount());
    ImGui::Text("Quads Rendered: %u", Renderer2DBackend::GetQuadCount());
    
    ImGui::End();
}
```

## Expected Performance Comparison

### Scenario: 1000 Active Bullets + 500 Particles

**BasicRenderer2D (Team A)**
```
Draw Calls: 1500
Quads: 1500
FPS: ~60 (adequate for demo)
```

**BatchRenderer2D (Team B)**
```
Draw Calls: 1-5 (depending on texture breaks)
Quads: 1500
FPS: ~200+ (overkill but impressive!)
```

## Development Timeline

### Phase 1: Team A (Basic Working)
- Week 1: SpriteComponent + BasicRenderer
- Week 2: Animation system
- **Result**: Bullets and particles visible!

### Phase 2: Team B (Batch Implementation)
- Week 1-2: BatchRenderer implementation
- Week 3: Integration and optimization
- **Result**: Same visuals, 10x fewer draw calls!

### Phase 3: Integration
- Both renderers coexist
- Runtime switching via ImGui
- Performance comparison in demo
- Choose default based on profiling

## Key Success Factor

**The interface (`IRenderer2D` and `Renderer2DBackend`) is the contract.**

As long as both teams implement the same interface:
- Team A gets working visuals quickly
- Team B can optimize without breaking anything
- Systems (SpriteRenderSystem, AnimationSystem) work with both
- Easy to A/B test performance

## Recommended Default

For your object pooling demo:
```cpp
// Use Basic renderer by default (simple, debuggable)
Renderer2DBackend::Init(Renderer2DBackend::API::Basic);

// Switch to Batch in production builds for performance
#ifdef PIL_RELEASE
    Renderer2DBackend::Init(Renderer2DBackend::API::Batch);
#endif
```
