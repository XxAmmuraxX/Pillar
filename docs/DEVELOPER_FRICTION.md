# Developer Friction Points

A candid list of pain points, inconsistencies, and areas causing friction when developing with or on the Pillar Engine. This document helps prioritize improvements and serves as a known-issues tracker.

---

## Critical Issues (Blocking or Major Productivity Loss)

### 1. RigidbodyComponent is Non-Copyable

**Problem:** `RigidbodyComponent` has deleted copy constructor/assignment because it holds a raw `b2Body*`. This breaks several common patterns:

```cpp
// This fails:
scene.DuplicateEntity(entity);  // If entity has RigidbodyComponent

// This fails:
auto rb = entity.GetComponent<RigidbodyComponent>();  // Returns reference, but...
RigidbodyComponent copy = rb;  // Error: deleted copy ctor
```

**Impact:** Can't easily clone physics entities. Can't store component copies for undo/redo. Serialization workarounds are awkward.

**Suggested Fix:** Use a handle/ID pattern instead of raw pointer, or make the component data-only with Body creation handled entirely by PhysicsSystem.

---

### 2. Component Includes Are Scattered Across Deep Paths

**Problem:** Component headers are in deeply nested paths like:
- `Pillar/ECS/Components/Core/TransformComponent.h`
- `Pillar/ECS/Components/Rendering/SpriteComponent.h`
- `Pillar/ECS/Components/Physics/ColliderComponent.h`

**Impact:** Every time you need a component, you have to remember/lookup the exact path. No umbrella header.

**Suggested Fix:** Create `Pillar/ECS/Components.h` that includes all component headers:
```cpp
#include <Pillar/ECS/Components.h>  // One include for all components
```

---

### 3. No Clear "Heavy Entity" vs "Light Entity" API

**Problem:** The codebase has two physics approaches:
1. **Heavy entities:** Use `RigidbodyComponent` + `ColliderComponent` → Box2D physics
2. **Light entities:** Use `VelocityComponent` → Simple integration

But there's no documentation or helper API distinguishing them. You discover this by reading system code.

**Impact:** New developers add both `RigidbodyComponent` AND `VelocityComponent` to the same entity, causing conflicts.

**Suggested Fix:** 
- Document the patterns clearly
- Add compile-time or runtime warnings if both are present
- Consider `LightPhysicsComponent` as a clearer name

---

### 4. Animation System Requires External JSON Loading

**Problem:** To use `AnimationComponent`, you must:
1. Create animation JSON files manually
2. Load them via `AnimationSystem::LoadAnimationClip()`
3. Set `AnimationComponent::CurrentClipName` to match

There's no way to define animations in code without JSON.

**Impact:** Quick prototyping is slow. You can't procedurally generate animations.

**Suggested Fix:** Add `AnimationSystem::RegisterClip()` overloads that accept frame data directly:
```cpp
AnimationClip clip;
clip.Name = "explode";
clip.Frames = { /* frame data */ };
animSystem.RegisterClip(clip);
```
(This may already exist but isn't exposed/documented well)

---

## Moderate Issues (Annoying but Workable)

### 5. Renderer2D Init/Shutdown Must Be Called Manually

**Problem:** Unlike `AudioEngine` which is initialized by `Application`, `Renderer2D::Init()` and `Shutdown()` must be called manually:

```cpp
void MyLayer::OnAttach() {
    Pillar::Renderer2D::Init();  // Easy to forget!
}
```

**Impact:** Forgetting this causes crashes or rendering failures with cryptic errors.

**Suggested Fix:** Initialize in `Application` constructor like audio, or use lazy initialization on first `BeginScene()`.

---

### 6. No Clear Error Messages for Missing Assets

**Problem:** When `Texture2D::Create("missing.png")` fails, it returns nullptr (or default white texture?) with only a log warning. Same for audio.

**Impact:** Silent failures are hard to debug. You see wrong visuals but no obvious error.

**Suggested Fix:** 
- Make missing asset behavior configurable (throw, assert, or silent)
- Use a clear "missing texture" pink/black checkerboard
- Return error information alongside null

---

### 7. Camera Controller Zoom Range Is Hardcoded

**Problem:** `OrthographicCameraController` clamps zoom to 0.25 - 10.0 internally with no API to change it.

**Impact:** Games needing extreme zoom levels (e.g., strategy games, microscope view) can't use the controller.

**Suggested Fix:** Add `SetZoomLimits(float min, float max)` method.

---

### 8. Event System Requires Boilerplate

**Problem:** Handling events requires verbose dispatcher pattern:

```cpp
void OnEvent(Event& e) override {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnResize));
    dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(OnKeyPressed));
    // One line per event type...
}
```

**Impact:** Lots of repetitive code. Easy to forget event types.

**Suggested Fix:** Consider visitor pattern or macro helpers:
```cpp
DISPATCH_EVENTS(e,
    ON(KeyPressedEvent, [this](auto& e) { return OnKeyPressed(e); }),
    ON(WindowResizeEvent, [this](auto& e) { return OnResize(e); })
);
```

---

### 9. Inconsistent Naming: PIL_KEY vs Input::IsKeyDown

**Problem:** Key codes use `PIL_KEY_*` prefix but Input class uses method names like `IsKeyDown`. The style is inconsistent with modern C++ (could use scoped enums).

**Impact:** Minor, but code looks dated.

**Suggested Fix:** Consider:
```cpp
// Current
Input::IsKeyDown(PIL_KEY_W);

// Could be
Input::IsKeyDown(Key::W);  // Scoped enum
```

---

### 10. Time::GetDeltaTime() Uses Global State

**Problem:** `Time::GetDeltaTime()` is a static global. In some architectures (multi-window, editor), you might want per-context delta times.

**Impact:** Mostly fine for games, but editor needs hacks for preview windows.

**Suggested Fix:** Accept delta time as parameter where possible, use static for convenience.

---

## Minor Issues (Quality of Life)

### 11. No Precompiled Headers Setup

**Problem:** No PCH configured in CMake. Every file recompiles standard headers.

**Impact:** Slightly longer build times. Not critical with modern compilers but adds up.

---

### 12. ImGui Layer Is Always Added

**Problem:** `Application` always creates an `ImGuiLayer`. No way to disable for release builds or headless mode.

**Impact:** Minor overhead. ImGui is useful for debugging anyway.

**Suggested Fix:** Constructor parameter or define to disable.

---

### 13. Box2D Debug Draw Not Integrated

**Problem:** Box2D has debug drawing but it's not wired up to Renderer2D.

**Impact:** Physics debugging requires external visualization or printf.

**Suggested Fix:** Implement `b2Draw` interface using `Renderer2D::DrawLine/Circle`.

---

### 14. Scene::ForEach Lambda Can't Return Early

**Problem:** `Scene::ForEach<...>()` iterates all matching entities with no way to break early.

**Impact:** Can't efficiently "find first" using ForEach.

**Suggested Fix:** Add `Scene::ForEachUntil<...>()` that accepts bool-returning lambda.

---

### 15. Particle Emitter Curves Are Raw Pointers

**Problem:** `ParticleEmitterComponent` uses raw pointers for optional curves:
```cpp
ColorGradient* ColorGradientPtr = nullptr;
AnimationCurve* SizeCurvePtr = nullptr;
```

**Impact:** Manual memory management, potential leaks, not serializable.

**Suggested Fix:** Use `std::shared_ptr` or value types with `std::optional`.

---

## Documentation Gaps

### Missing Documentation For:
- ECS system execution order
- How to create custom Systems properly
- Physics collision callbacks/filtering
- Layer sorting priorities
- Prefab/template system (TemplateManager in editor)
- Hot-reloading workflow (if any)

---

## Build System Friction

### 1. GLAD2 Requires Python + jinja2
First-time setup requires Python installation. Not obvious from CMake errors.

### 2. No Install Target
Can't `cmake --install` to package the engine for distribution.

### 3. Asset Path Detection Is Fragile
`AssetManager` searches relative to executable, but this breaks in different IDE launch configurations.

---

## New Issues (January 12, 2026 - Game Development Session)

### 16. No "Pillar/ECS/Components.h" Umbrella Header

**Problem:** When starting a new game, you need to include many component headers individually:
```cpp
#include "Pillar/ECS/Components/Core/TransformComponent.h"
#include "Pillar/ECS/Components/Core/TagComponent.h"
#include "Pillar/ECS/Components/Physics/RigidbodyComponent.h"
#include "Pillar/ECS/Components/Physics/ColliderComponent.h"
#include "Pillar/ECS/Components/Physics/VelocityComponent.h"
#include "Pillar/ECS/Components/Rendering/SpriteComponent.h"
#include "Pillar/ECS/Components/Rendering/AnimationComponent.h"
#include "Pillar/ECS/Components/Rendering/Light2DComponent.h"
#include "Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h"
#include "Pillar/ECS/Components/Gameplay/BulletComponent.h"
#include "Pillar/ECS/Components/Gameplay/XPGemComponent.h"
#include "Pillar/ECS/Components/Audio/AudioSourceComponent.h"
// ... and more
```

**Impact:** 10+ lines of includes just to use basic ECS. Tedious and error-prone. Easy to forget one.

**Suggested Fix:** Create `Pillar/ECS/Components.h`:
```cpp
#pragma once
// All component includes in one place
#include "Components/Core/TransformComponent.h"
#include "Components/Core/TagComponent.h"
// ... all components
```

---

### 17. No Built-in Health/Damage Component

**Problem:** Common game patterns like health, damage, and death aren't provided. Every game needs:
```cpp
struct HealthComponent {
    float CurrentHealth = 100.0f;
    float MaxHealth = 100.0f;
    bool IsDead = false;
};
```

**Impact:** Must create from scratch every time. No standardized damage/death event pattern.

**Suggested Fix:** Add `Pillar/ECS/Components/Gameplay/HealthComponent.h` with optional damage events.

---

### 18. System Execution Order Is Manual

**Problem:** When creating a game with multiple systems (Physics, Animation, Particles, Audio, etc.), you must manually call each system in the correct order:
```cpp
void OnUpdate(float dt) {
    m_VelocitySystem->OnUpdate(dt, registry);     // Must be before physics
    m_PhysicsSystem->OnUpdate(dt, registry);       // Must be before sync
    m_PhysicsSyncSystem->OnUpdate(dt, registry);   // Must be after physics
    m_BulletSystem->OnUpdate(dt, registry);        // Order matters!
    m_ParticleEmitterSystem->OnUpdate(dt, registry);
    m_ParticleSystem->OnUpdate(dt, registry);
    m_AnimationSystem->OnUpdate(dt, registry);
    m_AudioSystem->OnUpdate(dt, registry);
    // Miss one or wrong order = bugs
}
```

**Impact:** Easy to get wrong order. No automatic dependency resolution. Boilerplate in every game layer.

**Suggested Fix:** Consider a `SystemManager` with priority/dependency declaration:
```cpp
systemManager.Register<VelocityIntegrationSystem>(Priority::PrePhysics);
systemManager.Register<PhysicsSystem>(Priority::Physics);
systemManager.Register<PhysicsSyncSystem>(Priority::PostPhysics);
// Then: systemManager.UpdateAll(dt, registry);
```

---

### 19. No EnemyAI or ChaseTarget Component

**Problem:** Basic AI behaviors like "move toward player" must be implemented from scratch. The XPGemComponent has magnetic behavior, but there's no equivalent for enemies.

**Impact:** Every game reimplements the same "chase player" logic.

**Suggested Fix:** Add generic `ChaseTargetComponent` or `SeekBehaviorComponent`:
```cpp
struct SeekBehaviorComponent {
    entt::entity Target = entt::null;
    float Speed = 5.0f;
    float StopDistance = 0.5f;
};
```

---

### 20. Creating Games Outside Sandbox Is Not Documented

**Problem:** The only example of using the engine is Sandbox. There's no documentation on how to:
1. Create a new game project that links to Pillar
2. Structure a standalone game folder
3. Use the `templates/EmptyProject/` template

The `templates/EmptyProject/` folder exists but isn't documented or used.

**Impact:** New projects require reverse-engineering Sandbox's CMakeLists.txt.

**Suggested Fix:** 
1. Document the EmptyProject template usage
2. Add a "Creating a New Game" guide
3. Consider a project generator script

---

### 21. Sprite Colors vs Textures Ambiguity

**Problem:** `SpriteComponent` can use either a color quad or a texture, but the behavior isn't immediately clear:
```cpp
SpriteComponent sprite;
sprite.Color = {1, 0, 0, 1};  // Red quad
sprite.Texture = myTexture;    // Now textured, color is tint?
```

Is Color a tint when texture is set? What's the default behavior?

**Impact:** Trial and error to understand interaction between Color and Texture.

**Suggested Fix:** Document clearly in SpriteComponent or rename to `TintColor`.

---

## Tracking

| Issue | Priority | Status |
|-------|----------|--------|
| RigidbodyComponent non-copyable | High | Open |
| Scattered component includes | Medium | Open |
| No light/heavy entity docs | Medium | Open |
| Renderer2D manual init | Medium | Open |
| Missing asset silent fail | Medium | Open |
| Camera zoom hardcoded | Low | Open |
| Event boilerplate | Low | Open |
| No umbrella component header | Medium | Open |
| No Health/Damage component | Low | Open |
| Manual system execution order | Medium | Open |
| No EnemyAI/Chase component | Low | Open |
| EmptyProject not documented | Medium | Open |
| Sprite Color/Texture ambiguity | Low | Open |

---

*Last updated: January 12, 2026*
