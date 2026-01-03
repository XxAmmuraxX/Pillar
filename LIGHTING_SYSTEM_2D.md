# Pillar Engine — 2D Lighting + Shadows (Design)

This document proposes a practical 2D lighting and shadow system that fits Pillar’s current architecture:

- 2D rendering is done via `Renderer2DBackend` (batch renderer).
- Rendering to textures is available via `Framebuffer` (`OpenGLFramebuffer` uses `GL_RGBA8` color + `DEPTH24_STENCIL8`).
- ECS is already used for rendering (`SpriteComponent`, `SpriteRenderSystem`) and Editor viewport rendering.

The design focuses on **typical 2D game needs**:

- Point and spot lights (torches, lamps, explosions)
- Ambient light
- Shadow casting from polygonal occluders (walls, crates)
- Optional “soft shadow” approximation
- Layer masks (lights affect only selected sprites)

---

## 1. Goals

- Add 2D lights that can be authored via ECS components and rendered efficiently.
- Support hard shadows from 2D occluders.
- Integrate with both:
  - Runtime rendering (Sandbox/game loop)
  - Editor viewport rendering (framebuffer → ImGui)
- Keep the first implementation simple and incremental (does not require changing the existing sprite batch renderer).

## 2. Non-Goals (Initial Version)

These are valuable, but intentionally deferred because they require a more complex “G-buffer” style pipeline or significant shader/data plumbing:

- Per-pixel normal mapped lighting for every sprite
- Global illumination / bounced light
- Fully physically-based material model
- Volumetric fog / god rays

A “Phase 2” section below describes how to extend the system toward normal mapping and richer materials.

---

## 3. Core Idea

Render the scene in **two logical buffers** and then composite:

1) **Scene Color**: your usual 2D sprites/particles/UI rendered normally.

2) **Light Accumulation**: an offscreen buffer where lights are accumulated (additive blending), respecting shadows.

3) **Composite**: multiply (or “modulate”) scene color by lighting (plus optional emissive) and output to the final target.

High-level pipeline:

```
[Scene sprites]  →  SceneColor FBO
[Lights+shadows] →  LightAccum FBO
Composite(SceneColor, LightAccum) → Output (Editor viewport FBO or swapchain)
```

Why this fits Pillar today:

- It leverages the existing `Framebuffer` system.
- It can be implemented as a separate renderer module without rewriting the batch sprite renderer.
- Shadows can use the existing stencil attachment (`DEPTH24_STENCIL8`) in the light buffer.

---

## 4. Data Model (ECS Components)

### 4.1 `Light2DComponent`
Represents a 2D light source.

Recommended fields:

- `Type`: `Point`, `Spot`, `Directional` (Directional optional in v1)
- `Color`: `glm::vec3` or `glm::vec4`
- `Intensity`: float
- `Radius`: float (world units) — for Point/Spot
- `InnerAngle`, `OuterAngle`: floats (radians or degrees) — for Spot
- `CastShadows`: bool
- `ShadowStrength`: float in [0..1]
- `SoftShadows`: bool (optional)
- `SoftShadowRadius`: float (optional)
- `LayerMask`: bitmask (lights only affect sprites on matching layers)

Notes:
- Pillar already uses `SpriteLayer` and `ZIndex`. For lighting, prefer an independent `LayerMask` (bitmask) so you can keep draw ordering and lighting filtering separate.

### 4.2 `ShadowCaster2DComponent`
Defines an occluder polygon used for shadow casting.

Recommended fields:

- `Points`: `std::vector<glm::vec2>` in **local space** (CCW order)
- `Closed`: bool (polygon) vs polyline
- `TwoSided`: bool (casts from both sides)
- `LayerMask`: bitmask (which lights this caster affects)

Authoring options:
- Editor tool can generate this from physics colliders (Box2D polygons) or from hand-authored points.

### 4.3 Optional: `LightReceiver2DComponent`
If you want lights to affect only specific sprites/entities (beyond a global layer mask), add:

- `LayerMask`: bitmask
- `ReceiveLighting`: bool

However, the simplest approach is: **sprites are lit by default** and filtered by a single sprite layer mask already stored in the sprite component (or a new field).

---

## 5. Rendering Architecture

### 5.1 New Module: `Lighting2DRenderer`
Add a dedicated renderer module that executes *after* your base 2D draw calls.

Recommended interface shape (similar style to `Renderer2DBackend`):

- `Lighting2D::Init()` / `Shutdown()`
- `Lighting2D::BeginScene(camera, targetFramebuffer)`
- `Lighting2D::SubmitLight(lightData, transform)`
- `Lighting2D::SubmitShadowCaster(points, transform)`
- `Lighting2D::EndScene()`

Internally it manages:

- A `LightAccum` framebuffer (or renders into a provided one)
- A full-screen quad (for composite)
- A “light volume quad” VAO (world-space) for point/spot lights
- A dynamic VBO/IBO for shadow volume triangles

### 5.2 Where It Hooks In

**Runtime** (Sandbox/game):

1. Clear
2. `Renderer2DBackend::BeginScene(camera)`
3. Render sprites (SpriteRenderSystem)
4. `Renderer2DBackend::EndScene()`
5. `Lighting2DRenderer` executes lighting/shadows
6. Composite to screen

**Editor viewport**:

- The viewport already renders into a framebuffer and displays it.
- You render SceneColor to an intermediate framebuffer, run the lighting pass, then present the final composited texture in the viewport.

---

## 6. Passes in Detail

### 6.1 Pass A — Scene Color
Target: `SceneColorFBO` (RGBA8 + depth)

- Render sprites as you do today with `Renderer2DBackend`.
- No changes required in v1.

### 6.2 Pass B — Light Accumulation
Target: `LightAccumFBO` (RGBA8 + **stencil**) and clear it to “ambient”.

- Clear to ambient color: `Ambient = (ambientColor * ambientIntensity)`.
- Enable additive blending: `ONE, ONE` (or keep your default blend and adapt).

For each light:

1) Clear stencil for the scissored region (or clear whole stencil if simpler).
2) If `CastShadows`:
   - Rasterize shadow volumes into stencil.
3) Render the light volume quad with a light shader, **stencil-tested** to exclude shadowed pixels.

This produces a buffer where:

- RGB is the accumulated lighting contribution
- Alpha can be unused or used for debug (optional)

### 6.3 Pass C — Composite
Target: output framebuffer (Editor viewport framebuffer or swapchain)

Composite shader:

- Sample SceneColor
- Sample LightAccum
- Output: `SceneColor.rgb * LightAccum.rgb` (classic 2D “multiply lighting”) 

Optionally:

- Support emissive sprites later via `SceneEmissive` or by letting sprites draw into `SceneColor` already bright.

---

## 7. Shadow Technique (v1): Stencil Shadow Volumes

This technique is common in 2D engines because it is:

- Deterministic
- Fast enough for a moderate number of lights
- Works well with polygon occluders

### 7.1 Shadow Volume Construction (Point/Spot)
For each shadow caster polygon within the light radius:

1) Transform points from local → world.
2) For each edge (p0, p1):
   - Determine if the edge is facing the light (for one-sided casters) using a 2D normal test.
   - Extrude both points away from the light:

Let `dir0 = normalize(p0 - lightPos)` and `dir1 = normalize(p1 - lightPos)`.

Extruded points:

- `p0e = p0 + dir0 * lightRadius`
- `p1e = p1 + dir1 * lightRadius`

3) Emit two triangles forming a quad: `(p0, p1, p1e)` and `(p0, p1e, p0e)`.

That quad represents the region “behind” that edge.

### 7.2 Writing to Stencil
Bind `LightAccumFBO`.

- Disable color writes (optional, faster)
- Enable stencil write
- Draw shadow volume triangles
- Set stencil to 1 where shadow volume is drawn

Then render the light with `stencil == 0`.

This yields **hard shadows**.

### 7.3 Soft Shadows (Optional Approximation)
Two pragmatic options:

1) **Geometry penumbra** (recommended):
   - For each edge, build an additional “penumbra wedge” with a small angular offset.
   - Render penumbra geometry with a gradient alpha to soften.

2) **Blur shadow mask**:
   - Render a shadow mask to a 1-channel buffer then apply a separable blur.
   - Simpler conceptually but adds another buffer + pass.

For v1, implement hard shadows first. Add soft shadows after you have correctness.

---

## 8. Light Shaders (v1)

### 8.1 Point Light Shader (World-Space Quad)
Render a quad centered at the light with size `2*radius` in world units.

Vertex:
- Transform quad vertices by camera VP.
- Pass the interpolated world position to fragment.

Fragment:
- Compute distance to `u_LightPos`.
- Attenuation: `a = clamp(1 - d/radius, 0, 1)`.
- Apply smoothstep for nicer falloff.
- Output: `u_Color * u_Intensity * a`.

Spot lights additionally compute angle attenuation against direction.

### 8.2 Composite Shader
Inputs:
- `u_SceneColor` (texture)
- `u_LightAccum` (texture)

Output:
- `out.rgb = scene.rgb * light.rgb`
- `out.a = scene.a`

---

## 9. Culling and Performance

### 9.1 Light Culling
Before rendering a light:

- Reject if outside camera bounds (simple AABB vs radius).
- Use scissor rect to constrain rendering to the light’s screen-space bounds.

### 9.2 Shadow Caster Culling
For each light:

- Consider only casters within `lightRadius + casterBoundsRadius`.
- If you have a spatial hash or broad-phase grid (common with ECS/physics), use it.

### 9.3 Expected Cost
Per light (hard shadows):

- `N_edges_in_range` triangles for shadow volumes
- 1 draw for the light quad

This scales well for tens of lights and modest occluder complexity.

---

## 10. Serialization / Editor Support

### 10.1 Serialization
Register components in `BuiltinComponentRegistrations` similarly to `SpriteComponent`.

- Store `Light2DComponent` fields in JSON.
- Store `ShadowCaster2DComponent` points array as `[ [x,y], [x,y], ... ]`.

### 10.2 Editor UX (Minimal)
- In the inspector:
  - Add/remove `Light2DComponent`
  - Edit color/intensity/radius/angles
  - Toggle shadows
  - Edit layer mask
- For `ShadowCaster2DComponent`:
  - Edit points list
  - (Later) provide gizmos to move points in viewport

---

## 11. Implementation Phases (Recommended)

### Phase 1 — Core Lighting (No Shadows)
- Add `Light2DComponent`.
- Create `LightAccumFBO`.
- Render point/spot lights as additive quads.
- Composite with SceneColor.

### Phase 2 — Hard Shadows
- Add `ShadowCaster2DComponent`.
- Implement shadow volume generation.
- Use stencil test to mask light.

### Phase 3 — Quality + Tools
- Soft shadow option (penumbra or blur).
- Debug overlays (draw light bounds, caster polygons, shadow volumes).
- Layer masks.

### Phase 4 (Optional) — Normal Maps / Materials
To support normal-mapped 2D lighting properly, you typically need:

- Either a **G-buffer** style pass (multiple render targets):
  - Albedo
  - Normal
  - Material params (roughness/metalness or just “specular strength”)

Or a “per-sprite lighting” approach (more complex for batching).

Given Pillar currently uses a single output in the batch sprite shader, the clean path is:

- Extend `Framebuffer` to support multiple color attachments.
- Add a sprite pass that outputs normals/material into attachments.
- Lighting shader samples those attachments.

This is a bigger change, so it’s best done after Phase 2 is stable.

---

## 12. Integration Checklist (Where Code Would Live)

Suggested file locations (names are recommendations, adjust to your conventions):

- Engine:
  - `Pillar/src/Pillar/Renderer/Lighting2D.h/.cpp`
  - `Pillar/src/Pillar/ECS/Components/Rendering/Light2DComponent.h`
  - `Pillar/src/Pillar/ECS/Components/Rendering/ShadowCaster2DComponent.h`
  - `Pillar/src/Pillar/ECS/Systems/Lighting2DSystem.h/.cpp` (collect + render lights)

- OpenGL backend helpers:
  - `Pillar/src/Platform/OpenGL/OpenGLLighting2D.cpp` (optional split)

- Editor:
  - Add component UI in the inspector panel
  - (Optional) viewport debug toggles

---

## 13. Example Usage (Pseudo-Code)

### Runtime

```cpp
Renderer2DBackend::BeginScene(camera);
scene->GetSystem<SpriteRenderSystem>()->OnUpdate(dt);
Renderer2DBackend::EndScene();

Lighting2D::BeginScene(camera, sceneColorFBO);
scene->GetSystem<Lighting2DSystem>()->OnUpdate(dt); // submits lights/casters
Lighting2D::EndScene();
```

### ECS

```cpp
auto torch = scene.CreateEntity("Torch");
auto& light = torch.AddComponent<Light2DComponent>();
light.Type = Light2DType::Point;
light.Radius = 6.0f;
light.Intensity = 2.0f;
light.Color = { 1.0f, 0.85f, 0.6f };
light.CastShadows = true;

auto wall = scene.CreateEntity("Wall");
auto& caster = wall.AddComponent<ShadowCaster2DComponent>();
caster.Points = { {-1, -0.5f}, {1, -0.5f}, {1, 0.5f}, {-1, 0.5f} };
```

---

## 14. Why This Design Matches Pillar

- Uses the existing `Framebuffer` abstraction (including stencil support).
- Keeps `Renderer2DBackend` unchanged for v1.
- Fits the engine’s style: static renderer modules, OpenGL backend, ECS-driven authoring.
- Scales incrementally: you can land Phase 1 quickly, then add shadows and tooling.
