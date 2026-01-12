# Pillar Engine - Rendering Demo

A comprehensive demonstration of all Pillar Engine rendering features.

## Overview

This demo showcases the complete 2D rendering pipeline including:
- Basic colored quads
- Textured quads with tinting
- Texture atlas usage
- Rotation and transformation
- Batch rendering performance
- Post-processing effects
- Real-time statistics

## How to Use

### Adding to Your Project

1. Include the header in your application source:
```cpp
#include "RenderingDemoLayer.h"
```

2. Add the layer in `CreateApplication()`:
```cpp
Pillar::Application* Pillar::CreateApplication()
{
    auto app = new Application();
    app->PushLayer(new RenderingDemo::RenderingDemoLayer());
    return app;
}
```

3. Build and run the application

### Controls

**Camera:**
- `W/A/S/D` - Move camera
- `Q/E` - Rotate camera
- `Mouse Wheel` - Zoom in/out

**Demo Modes:**
- `1` - Basic Colored Quads (rainbow display)
- `2` - Textured Quads (with tinting)
- `3` - Texture Atlas (sprite sheet grid)
- `4` - Rotation & Scaling (animated orbiting squares)
- `5` - Performance Test (2500 quads stress test)

**Effects:**
- `P` - Toggle post-processing effects

## Demo Modes

### 1. Basic Colored Quads
Displays a rainbow of colored quads demonstrating basic drawing:
- Red, Orange, Yellow, Green, Blue, Indigo, Violet
- Shows color blending and basic quad rendering
- Single batch, 1 draw call

### 2. Textured Quads
Shows textured quads with different tinting:
- Normal texture (white tint)
- Red-tinted texture
- Green-tinted texture
- Demonstrates texture loading and color multiplication

### 3. Texture Atlas
Displays sprites from a texture atlas in a grid:
- 5x5 grid of sprites (25 quads)
- All use same texture (1 texture bind)
- Demonstrates atlas efficiency
- Shows proper UV coordinate usage

### 4. Rotation & Scaling
Animated demonstration of transforms:
- Center square rotating
- 6 orbiting satellites with counter-rotation
- Rainbow colors
- Demonstrates rotation API and time-based animation

### 5. Performance Test
Stress test with 2500 quads (50x50 grid):
- All different colors (tests batching)
- Watch statistics panel for performance metrics
- Should render in < 5ms at 1080p
- Demonstrates batch renderer efficiency

## Post-Processing Effects

Toggle with `P` key. Available effects:

### Vignette
- Darkens screen edges
- Adjustable radius and softness
- Creates focus on center

### Chromatic Aberration
- Color fringing effect
- Adjustable offset
- Stylized retro look

### Grayscale
- Desaturation effect
- Adjustable intensity
- Useful for "game over" or pause states

## Statistics Panel

The ImGui panel shows real-time rendering statistics:

**Per-Frame Stats:**
- Draw Calls - Number of GPU draw calls (lower is better)
- Quads - Total quads rendered this frame
- Vertices - Total vertices processed
- Batches - Number of batches created
- Texture Switches - Number of texture binds
- Buffer Uploads - GPU buffer uploads (main bottleneck)
- Flush Count - Number of batch flushes

**Efficiency Metrics:**
- Batch Efficiency - Average quads per batch
- Avg Quads/Draw - Indicates batching effectiveness

**Accumulated Stats:**
- Total Quads - All-time quad count
- Total Draws - All-time draw call count
- Peak Quads/Vertices - Maximum in single frame

## Expected Performance

On modern hardware (GTX 1060 or equivalent):

| Demo Mode | Quads | Expected Draw Calls | Expected Frame Time |
|-----------|-------|---------------------|---------------------|
| Basic Quads | 7 | 1 | < 0.1ms |
| Textured Quads | 3 | 1 | < 0.1ms |
| Texture Atlas | 25 | 1 | < 0.2ms |
| Rotation | 7 | 1 | < 0.1ms |
| Performance Test | 2500 | 1 | < 2ms |

**With Post-Processing:**
Add ~1-3ms depending on active effects.

## Asset Requirements

To run the full demo, ensure these assets exist:

```
Sandbox/assets/textures/
├── checkerboard.png    # Test texture for textured quads
└── spritesheet.png     # 8x8 grid of 32x32 sprites (256x256 total)
```

**Fallback Behavior:**
If textures are missing, the demo displays colored quads as fallback.

## Learning from the Code

The demo code demonstrates best practices:

1. **Proper Initialization:**
   - Camera setup in `OnAttach()`
   - Resource loading upfront
   - Post-processing initialization

2. **Render Loop Structure:**
   - Clear screen
   - BeginScene with camera
   - Draw calls
   - EndScene

3. **Performance Monitoring:**
   - Real-time statistics display
   - Efficiency metrics tracking
   - ImGui integration

4. **Error Handling:**
   - Null texture checks
   - Fallback rendering
   - Informative logging

5. **User Interaction:**
   - Camera controller integration
   - Keyboard event handling
   - ImGui controls

## Extending the Demo

Add your own demo modes:

```cpp
enum class DemoMode
{
    // ... existing modes ...
    MyCustomMode
};

void RenderMyCustomMode()
{
    // Your rendering code here
    Pillar::Renderer2D::DrawQuad(...);
}

// In OnUpdate():
case DemoMode::MyCustomMode:
    RenderMyCustomMode();
    break;
```

## Troubleshooting

**Issue: Nothing renders**
- Check console for initialization errors
- Verify camera bounds include draw positions
- Ensure BeginScene/EndScene are called

**Issue: Textures appear white/magenta**
- Check asset paths in console
- Verify files exist in `Sandbox/assets/textures/`
- Check file format (PNG recommended)

**Issue: Poor performance**
- Check statistics panel for high draw call count
- Ensure textures are properly batched
- Reduce quad count in performance test

## See Also

- [Rendering System Guide](../../Pillar/docs/Rendering_System_Guide.md)
- [User's Guide](../../docs/USERS_GUIDE.md)
- [Batch Renderer Usage](../../Pillar/docs/BatchRenderer_Usage.md)

---

*Last Updated: January 11, 2026*
*Part of Pillar Engine Phase 5 - Polish*
