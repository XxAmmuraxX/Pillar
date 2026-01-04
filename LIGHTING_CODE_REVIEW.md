# Lighting2D Code Review (Game-Dev Perspective)

Date: 2026-01-03

This document reviews the current 2D lighting implementation in Pillar Engine, focusing on correctness, performance, and “game dev friendly” API ergonomics.

Reviewed areas:
- Renderer API: [Pillar/src/Pillar/Renderer/Lighting2D.h](../Pillar/src/Pillar/Renderer/Lighting2D.h), [Pillar/src/Pillar/Renderer/Lighting2D.cpp](../Pillar/src/Pillar/Renderer/Lighting2D.cpp)
- Shadow geometry: [Pillar/src/Pillar/Renderer/Lighting2DGeometry.h](../Pillar/src/Pillar/Renderer/Lighting2DGeometry.h), [Pillar/src/Pillar/Renderer/Lighting2DGeometry.cpp](../Pillar/src/Pillar/Renderer/Lighting2DGeometry.cpp)
- ECS integration: [Pillar/src/Pillar/ECS/Systems/Lighting2DSystem.cpp](../Pillar/src/Pillar/ECS/Systems/Lighting2DSystem.cpp), [Pillar/src/Pillar/ECS/Components/Rendering/Light2DComponent.h](../Pillar/src/Pillar/ECS/Components/Rendering/Light2DComponent.h), [Pillar/src/Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h](../Pillar/src/Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h)
- Demo usage: [Sandbox/src/Lighting2DDemoLayer.h](../Sandbox/src/Lighting2DDemoLayer.h)
- Tests: [Tests/src/Renderer/Lighting2DAPITests.cpp](../Tests/src/Renderer/Lighting2DAPITests.cpp), [Tests/src/Renderer/Lighting2DGeometryTests.cpp](../Tests/src/Renderer/Lighting2DGeometryTests.cpp)

## What the system currently does

**High level pipeline**
1. `Lighting2D::BeginScene(...)`
   - Captures a small OpenGL state snapshot.
   - Binds an internal SceneColor framebuffer and clears it.
   - Calls `Renderer2DBackend::BeginScene(camera)` so you can draw sprites into the scene-color buffer.
2. Gameplay submits:
   - `Lighting2D::SubmitLight(Light2DSubmit)`
   - `Lighting2D::SubmitShadowCaster(ShadowCaster2DSubmit)`
3. `Lighting2D::EndScene()`
   - Ends 2D batching.
   - Renders a light accumulation buffer with additive blending.
   - Optionally writes shadow volumes into the stencil buffer per light and uses stencil to mask lighting.
   - Composites `sceneColor * lightAccum` to the output framebuffer (or backbuffer).
   - Restores the captured OpenGL state.

**Lighting model**
- Light accumulation starts as **ambient** (`AmbientColor * AmbientIntensity`).
- Each light adds RGB (`GL_ONE, GL_ONE` additive blend).
- Final composite multiplies the scene color by the accumulated light: `scene.rgb * light.rgb`.

That’s a very typical “2D multiply lighting” approach and is a good baseline.

## Strengths / what’s already good

- **Self-contained renderer**: `Lighting2D` handles the internal FBOs and shader setup and exposes a small API.
- **ECS hook exists**: `Lighting2DSystem` shows how to submit lights and casters from components.
- **Correctness safety rails**: asserts for correct call order (`Init` → `BeginScene` → `EndScene`).
- **Performance-minded bits exist**: per-light scissor rect reduces work and makes stencil clears cheap.
- **Tests exist**: geometry and scissor computations have unit tests, which is great for iteration.

## Issues / things that likely need fixes

### 1) Uniform updates are very expensive per-light
In `Lighting2D.cpp`, each light does multiple `glGetUniformLocation(...)` calls every frame.

Why it matters:
- `glGetUniformLocation` is not cheap and doing it per light will become a bottleneck as light counts scale.

What to fix:
- Cache uniform locations once per program (store them in `Lighting2DData` after shader compilation).
- Or extend `Shader` to expose cached uniform setting / a program ID getter and cache on the Shader side.

Minimal fix direction:
- After `EnsureShaders()` succeeds, do `s_Data.LightUniforms = {...locations...}` once.

### 2) OpenGL state restore is incomplete (possible integration footguns)
`CaptureGLState()`/`RestoreGLState()` currently saves/restores:
- framebuffer binding, viewport
- enable states (blend, depth, scissor, stencil)

It does **not** restore:
- blend func/equation, color mask
- stencil func/op/mask values
- active shader program, bound VAO/VBO/IBO
- scissor box
- depth mask, stencil clear value, etc.

Why it matters:
- If you call `Lighting2D` in the middle of other render passes (editor overlays, post-processing, etc.), this can cause subtle rendering bugs.

What to fix:
- Either (A) expand the snapshot to include the critical states you mutate, or (B) stop trying to be “transparent” and document that `Lighting2D` sets its own state and should be used as a dedicated pass.

A pragmatic middle ground:
- Save/restore at least: blend func, color mask, scissor box, stencil func/op/mask, current program, current VAO.

### 3) Shadow caster winding assumption is easy to violate
`Lighting2DGeometry::BuildShadowVolumeTriangles` assumes:
- For one-sided casters (`TwoSided=false`), the polygon is CCW so that the “outward normal” computed per edge is meaningful.

You already note “recommended CCW” in `ShadowCaster2DComponent`, which is good.

Why it matters:
- In real production, artists/tools will feed mixed-winding shapes. If winding flips, one-sided casters will cast “from the wrong side” (or not at all).

What to fix:
- Consider detecting winding (signed area) and flipping the outward normal sign automatically when needed.
- Or store a `Winding` enum in the component (`CCW`, `CW`) and let tooling set it.

### 4) Shadow extrusion ends exactly at `light.Radius` (visual popping)
Current extrusion:
- Each edge extrudes points by `dir * light.Radius`.

Why it matters:
- Shadows will “end” at the radius boundary; when a light or caster moves, you can see harsh popping at the edge.

What to fix:
- Extrude to a slightly larger distance than the light radius (e.g., `Radius * 1.1f`), or to a constant “far” distance.
- Alternatively, treat the shadow volume as infinite (common) and rely on scissor/clip.

### 5) Color space / precision limitations (will limit quality)
Both SceneColor and LightAccum appear to be `GL_RGBA8` (based on current framebuffer creation).

Why it matters:
- Multiplying in gamma space is incorrect if you treat these as sRGB.
- `RGBA8` will band and clamp hard when using many bright lights (especially with additive blending).

What to fix:
- Use HDR formats for light accumulation: `RGBA16F` is typical.
- Decide on a consistent color space: either do lighting in linear space and output to sRGB, or keep everything “linear-ish” and document it.

(You may need to extend `FramebufferSpecification` to allow attachment formats.)

### 6) Silent no-ops can hide bugs in game code
`SubmitLight` / `SubmitShadowCaster` just `return` when not in a scene.

Why it matters:
- In game code, accidental wrong call order becomes “lights don’t work” rather than a clear error.

What to fix:
- In debug builds, assert/log when `Submit...` is called while not in scene.
- In release, keep the no-op behavior.

## API ergonomics (game-dev friendliness)

### What’s good already
- `Light2DSubmit` is a simple POD-ish struct.
- The `LayerMask` concept is a familiar game-dev feature.
- The `BeginScene` overload that takes an output framebuffer is a good editor/renderer building block.

### Friction points for game-dev usage

1) **The API forces a very specific render flow**
- You must call `Lighting2D::BeginScene()` and draw sprites using `Renderer2DBackend` (or be very careful about where sprites go).

Game-dev expectation:
- Usually you want the higher-level `Renderer2D` API to “just work” with lighting, not an internal backend.

Suggestion:
- Consider offering a higher-level lighting-aware path:
  - `Renderer2D::BeginLitScene(camera, viewport, settings)`
  - `Renderer2D::EndLitScene()`
  - or a `Lighting2D::Begin(...)` that returns a handle/guard that exposes drawing through the normal API.

2) **Viewport handling is awkward**
- Many games don’t want to pass `w/h` every frame, and editor workflows have multiple viewports.

Suggestion:
- Keep the explicit overload, but also offer one of:
  - `Lighting2D::SetViewport(uint32_t w, uint32_t h)` and let `BeginScene(camera, settings)` reuse it.
  - Or integrate with the engine’s current viewport state (RenderCommand / Window).

3) **Angles in radians are correct for math but unfriendly for tools**
- `InnerAngleRadians`/`OuterAngleRadians` are fine internally, but editors/UI will always present degrees.

Suggestion:
- Keep radians in the struct (low-level), but add helper setters/builders:
  - `SetSpotAnglesDegrees(innerDeg, outerDeg)`
  - or store degrees in the component and convert during submit.

4) **Duplicated “light data” between ECS and renderer**
- `Light2DComponent` and `Light2DSubmit` overlap heavily.

Suggestion:
- Either:
  - Make `Light2DComponent` convertible to `Light2DSubmit` (helper function), or
  - Merge conceptually: define a core `Light2D` data struct used by both.

5) **Shadow casters require building world-point arrays every frame**
- ECS path transforms points per frame.
- Demo path allocates a vector per wall per frame.

Suggestion (practical):
- Add a submission path that takes local points + transform:
  - `SubmitShadowCaster(const glm::mat4& transform, span<const glm::vec2> localPoints, ...)`
  - Internally transform into a scratch buffer.

Suggestion (more engine-y):
- Cache transformed world points in the component and only recompute when transform changes.

## Performance notes (scaling to “real” game scenes)

Current complexity is roughly:
- Per frame: O(numLights + numCasters) bookkeeping
- Per light: builds shadow triangles by iterating all casters (after AABB cull)

Potential hotspots as scenes scale:
- Per-light per-frame CPU shadow triangle generation.
- Uniform location queries (`glGetUniformLocation`) per light.
- Dynamic allocations: `std::vector<glm::vec2> shadowTriangles;` per light.

Suggested optimizations (in increasing effort):
1. Cache uniform locations.
2. Add reusable scratch buffers (store `shadowTriangles` in `s_Data` and clear/reuse).
3. Precompute caster AABBs once per frame (or cache in component).
4. Partition casters spatially (grid/quadtree) so each light considers only nearby casters.
5. Move more work to GPU (UBO/SSBO of lights; instanced light quads).

## Suggested “next iteration” of the public API (minimal but game-friendly)

These are small changes that keep the spirit of your current implementation:

- Add a RAII guard to enforce correct pairing:
  - `auto frame = Lighting2D::Begin(camera, viewportW, viewportH, settings);`
  - `frame.SubmitLight(...)`, `frame.SubmitShadowCaster(...)`
  - `// destructor calls EndScene()`

- Add debug-friendly checks:
  - Debug asserts when submitting outside a scene
  - Optional `Lighting2D::GetStats()` (lights count, casters count, shadow triangles count)

- Make viewport management more ergonomic:
  - `Lighting2D::BeginScene(camera, settings)` uses last known viewport
  - keep the explicit overload for editor viewports

## Quick wins (high value / low risk)

1. Cache uniform locations (big perf win quickly).
2. Expand GL state restoration to include stencil/blend/scissor parameters.
3. Reuse `shadowTriangles` buffer between lights to avoid per-light allocations.
4. Add debug asserts/logging for incorrect usage.
5. Add a simple winding fix (auto-detect and flip outward normal for one-sided casters).

## Notes on current demo usage

The demo in [Sandbox/src/Lighting2DDemoLayer.h](../Sandbox/src/Lighting2DDemoLayer.h) is a good showcase and is doing the “right” steps in order.

One thing the demo highlights (and real users will feel):
- The per-frame creation of `ShadowCaster2DSubmit` and its `WorldPoints` vectors is convenient but allocation-heavy.

If the engine is meant to support many occluders, it’s worth providing an API that lets users submit local-space shapes + transform without per-frame heap churn.
