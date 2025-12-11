# Pillar Animation System - User Guide

## Table of Contents
1. [Overview](#overview)
2. [Quick Start](#quick-start)
3. [Sprite Sheet Requirements](#sprite-sheet-requirements)
4. [Animation Clips](#animation-clips)
5. [Using Animations in Code](#using-animations-in-code)
6. [JSON Format](#json-format)
7. [Advanced Features](#advanced-features)
8. [Best Practices](#best-practices)
9. [Examples](#examples)

---

## Overview

The Pillar Animation System is a frame-based 2D sprite animation system with the following features:
- **Frame-based animation** with customizable duration per frame
- **UV coordinate support** for sprite sheets and texture atlases
- **Animation events** for triggering code at specific frames (footsteps, particle effects, etc.)
- **JSON serialization** for easy animation data management
- **Playback control** (play, pause, resume, stop, speed modification)
- **Looping and non-looping** animation modes
- **ECS integration** for easy entity animation

---

## Quick Start

### 1. Create Animation Frames

```cpp
#include "Pillar/ECS/Components/Rendering/AnimationClip.h"

// Create animation clip
Pillar::AnimationClip idleClip;
idleClip.Name = "Idle";
idleClip.IsLooping = true;
idleClip.PlaybackSpeed = 1.0f;

// Add frames
Pillar::AnimationFrame frame1;
frame1.TexturePath = "character_idle_0.png";
frame1.Duration = 0.1f;  // 100ms per frame
idleClip.Frames.push_back(frame1);

Pillar::AnimationFrame frame2;
frame2.TexturePath = "character_idle_1.png";
frame2.Duration = 0.1f;
idleClip.Frames.push_back(frame2);
```

### 2. Register with Animation System

```cpp
#include "Pillar/ECS/Systems/AnimationSystem.h"

// In your scene setup
auto animSystem = scene.GetSystem<Pillar::AnimationSystem>();
animSystem->RegisterClip(idleClip);
```

### 3. Add to Entity

```cpp
// Create entity
auto entity = scene.CreateEntity("Player");

// Add animation component
auto& anim = entity.AddComponent<Pillar::AnimationComponent>();
anim.Play("Idle");
```

That's it! The AnimationSystem will automatically update the animation during `OnUpdate()`.

---

## Sprite Sheet Requirements

### Supported Formats
- **Individual Images**: PNG, JPG, BMP, TGA (via stb_image)
- **Sprite Sheets**: Any format above, with UV coordinates defined per frame

### Sprite Sheet Dimensions
**There are NO hard requirements on sprite sheet dimensions!** The system is completely flexible:

- ✅ **Any width/height** - 512x512, 1024x256, 2048x2048, etc.
- ✅ **Non-power-of-2** dimensions - 640x480, 1280x720, etc.
- ✅ **Non-square sheets** - 2048x512, 512x2048, etc.
- ✅ **Mixed frame sizes** - Different frames can have different UV rectangles

### UV Coordinates
UV coordinates are normalized (0.0 to 1.0 range) and define the region of the texture to display:

```cpp
frame.UVRect = glm::vec4(
    0.0f,   // Min X (left edge, 0.0 = leftmost)
    0.0f,   // Min Y (bottom edge, 0.0 = bottom)
    0.25f,  // Max X (right edge, 0.25 = 25% across)
    0.5f    // Max Y (top edge, 0.5 = halfway up)
);
```

**Example: 4x4 sprite sheet (16 frames)**
```cpp
// Frame 0 (top-left): X: 0.0-0.25, Y: 0.75-1.0
frame0.UVRect = glm::vec4(0.0f, 0.75f, 0.25f, 1.0f);

// Frame 1 (second from left, top row): X: 0.25-0.5, Y: 0.75-1.0  
frame1.UVRect = glm::vec4(0.25f, 0.75f, 0.5f, 1.0f);

// Frame 4 (first on second row): X: 0.0-0.25, Y: 0.5-0.75
frame4.UVRect = glm::vec4(0.0f, 0.5f, 0.25f, 0.75f);
```

### Using Individual Images vs Sprite Sheets

**Individual Images** (easier for artists):
```cpp
frame.TexturePath = "walk_0.png";
frame.UVRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);  // Full texture
```

**Sprite Sheet** (better performance):
```cpp
frame.TexturePath = "character_spritesheet.png";
frame.UVRect = glm::vec4(0.0f, 0.75f, 0.25f, 1.0f);  // One cell
```

> **Performance Tip**: Use sprite sheets when possible! Loading one large texture is faster than loading many small textures, and reduces GPU texture swaps.

---

## Animation Clips

### AnimationFrame Structure
```cpp
struct AnimationFrame {
    std::string TexturePath = "";           // Path to texture (or shared sprite sheet)
    glm::vec4 UVRect = glm::vec4(0,0,1,1); // UV coordinates (minX, minY, maxX, maxY)
    float Duration = 0.1f;                  // How long to display this frame (seconds)
};
```

### AnimationClip Structure
```cpp
struct AnimationClip {
    std::string Name = "";                  // Unique identifier
    std::vector<AnimationFrame> Frames;     // Frame sequence
    bool IsLooping = true;                  // Loop back to start?
    float PlaybackSpeed = 1.0f;            // Speed multiplier (2.0 = 2x speed)
    std::vector<AnimationEvent> Events;     // Frame-based events
    
    float GetDuration() const;              // Total animation duration
    int GetFrameCount() const;              // Number of frames
    bool IsValid() const;                   // Has at least one frame
};
```

### Animation Events
Trigger code at specific frames:

```cpp
Pillar::AnimationClip::AnimationEvent footstepEvent;
footstepEvent.FrameIndex = 2;               // Fire on frame 2
footstepEvent.EventName = "Footstep";
footstepEvent.Callback = []() {
    // Play footstep sound
    PIL_INFO("Footstep!");
};
walkClip.Events.push_back(footstepEvent);
```

Events fire ONCE per animation loop at the specified frame.

---

## Using Animations in Code

### AnimationComponent
Attached to entities to control their animation state:

```cpp
struct AnimationComponent {
    std::string CurrentClipName = "";   // Currently playing clip
    int FrameIndex = 0;                 // Current frame index
    float PlaybackTime = 0.0f;          // Time elapsed in current frame
    float PlaybackSpeed = 1.0f;         // Speed multiplier
    bool Playing = true;                // Is animation playing?
    
    // Callbacks
    EventCallback OnAnimationEvent;         // Called when animation event fires
    CompletionCallback OnAnimationComplete; // Called when non-looping animation finishes
    
    // Control methods
    void Play(const std::string& clipName, bool restart = false);
    void Pause();
    void Resume();
    void Stop();
    bool HasAnimation() const;
    bool IsPlaying() const;
};
```

**Callback Signatures:**
```cpp
using EventCallback = std::function<void(const std::string& eventName, entt::entity entity)>;
using CompletionCallback = std::function<void(entt::entity entity)>;
```
    bool IsPlaying() const;
};
```

### Playback Control

```cpp
auto& anim = entity.GetComponent<Pillar::AnimationComponent>();

// Play animation
anim.Play("Walk");

// Pause/Resume
anim.Pause();
anim.Resume();

// Stop (resets to frame 0)
anim.Stop();

// Change speed
anim.PlaybackSpeed = 2.0f;  // 2x speed
anim.PlaybackSpeed = 0.5f;  // Half speed

// Check state
if (anim.IsPlaying()) {
    PIL_INFO("Animation is playing: {}", anim.CurrentClipName);
}
```

### Callbacks

```cpp
// Get notified when frame changes
anim.OnFrameChanged = [](int newFrame) {
    PIL_INFO("Frame changed to: {}", newFrame);
};

// Get notified when animation completes (non-looping only)
anim.OnAnimationComplete = []() {
    PIL_INFO("Animation finished!");
    // Switch to idle, trigger state change, etc.
};
```

---

## JSON Format

### Save Animation Clip
```cpp
#include "Pillar/Utils/AnimationLoader.h"

Pillar::AnimationLoader::SaveToJSON(walkClip, "walk_animation.anim.json");
```

### Load Animation Clip
```cpp
Pillar::AnimationClip loadedClip = 
    Pillar::AnimationLoader::LoadFromJSON("walk_animation.anim.json");
```

### JSON Structure
```json
{
  "name": "Walk",
  "isLooping": true,
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "character_walk_0.png",
      "uvRect": [0.0, 0.0, 1.0, 1.0],
      "duration": 0.1
    },
    {
      "texturePath": "character_walk_1.png",
      "uvRect": [0.0, 0.0, 1.0, 1.0],
      "duration": 0.1
    }
  ],
  "events": [
    {
      "frameIndex": 2,
      "eventName": "Footstep"
    }
  ]
}
```

> **Note**: Event callbacks are NOT serialized (they're code). You must re-attach callbacks after loading from JSON.

### Asset Path Resolution
Texture paths in JSON are automatically resolved by AssetManager:
- Searches `Sandbox/assets/textures/` during development
- Searches `assets/textures/` next to executable for distribution
- Supports relative paths from asset directories

---

## Advanced Features

### Sprite Flipping
Flip sprites horizontally or vertically without needing separate textures:

```cpp
auto& sprite = entity.GetComponent<Pillar::SpriteComponent>();

// Flip horizontally (mirror left/right) - useful for character facing directions
sprite.FlipX = true;

// Flip vertically (mirror up/down) - less common, useful for reflections
sprite.FlipY = false;

// Use with animations for character movement
if (movingLeft) {
    sprite.FlipX = true;
    anim.Play("Walk");
} else {
    sprite.FlipX = false;
    anim.Play("Walk");
}
```

**Benefits:**
- ✅ Save memory - one walk animation instead of two (left + right)
- ✅ Easier to maintain - only one animation to update
- ✅ Real-time toggling - instant direction changes
- ✅ Works with sprite sheets - UV coordinates are flipped automatically

### Switching Animations
```cpp
if (Pillar::Input::IsKeyPressed(PIL_KEY_W)) {
    anim.Play("Walk");
} else {
    anim.Play("Idle");
}
```

### Non-Looping Animations
```cpp
jumpClip.IsLooping = false;

auto& anim = entity.GetComponent<Pillar::AnimationComponent>();
anim.Play("Jump");

// Handle completion with OnAnimationComplete callback
anim.OnAnimationComplete = [&anim](entt::entity entity) {
    anim.Play("Idle");  // Return to idle when jump finishes
};
```

**OnAnimationComplete Callback:**
- Fires when a **non-looping** animation reaches its last frame
- Does NOT fire for looping animations (they never "complete")
- Perfect for state transitions (Attack → Idle, Jump → Idle)
- Also useful for one-shot effects (Explosion → Destroy Entity)

```cpp
// Example: Death animation
deathClip.IsLooping = false;
anim.Play("Death");
anim.OnAnimationComplete = [&scene](entt::entity entity) {
    scene.DestroyEntity(entity);  // Remove entity after death animation
    PIL_INFO("Entity died and was removed");
};

// Example: Spell cast
anim.Play("CastSpell");
anim.OnAnimationComplete = [&](entt::entity entity) {
    SpawnProjectile(entity);  // Spawn projectile when cast completes
    anim.Play("Idle");        // Return to idle stance
};
```

### Variable Frame Durations
```cpp
// Fast anticipation
frame0.Duration = 0.05f;
frame1.Duration = 0.05f;
// Slow follow-through
frame2.Duration = 0.2f;
frame3.Duration = 0.2f;
```

### Dynamic Speed Changes
```cpp
// Slow motion during charge-up
anim.PlaybackSpeed = 0.3f;

// Speed up during attack
anim.PlaybackSpeed = 2.0f;

// Return to normal
anim.PlaybackSpeed = 1.0f;
```

### Mixed Texture Sources
```cpp
// Some frames from individual files
frame0.TexturePath = "special_frame.png";
frame0.UVRect = glm::vec4(0, 0, 1, 1);

// Other frames from sprite sheet
frame1.TexturePath = "spritesheet.png";
frame1.UVRect = glm::vec4(0.0f, 0.75f, 0.25f, 1.0f);
```

---

## Best Practices

### 1. Asset Organization
```
assets/
  textures/
    characters/
      player/
        idle_sheet.png
        walk_sheet.png
        jump_sheet.png
    enemies/
      goblin/
        goblin_spritesheet.png
  animations/
    player/
      idle.anim.json
      walk.anim.json
      jump.anim.json
```

### 2. Animation Naming Convention
Use consistent, descriptive names:
- ✅ "PlayerIdle", "PlayerWalk", "PlayerJump"
- ✅ "EnemyIdle", "EnemyAttack"
- ❌ "Anim1", "Test", "IDLE"

### 3. Frame Duration Guidelines
- **Idle animations**: 0.1-0.15s per frame (6-10 FPS)
- **Walk animations**: 0.08-0.12s per frame (8-12 FPS)
- **Run animations**: 0.05-0.08s per frame (12-20 FPS)
- **Attack animations**: 0.03-0.1s per frame (10-30 FPS, varies by impact timing)

### 4. Use Events for Gameplay
```cpp
// Attack animation
attackEvent.FrameIndex = 3;  // Impact frame
attackEvent.EventName = "DealDamage";
attackEvent.Callback = [&entity]() {
    DealDamageToNearbyEnemies(entity);
};

// Walk animation  
footstepEvent.FrameIndex = 4;
footstepEvent.EventName = "Footstep";
footstepEvent.Callback = []() {
    PlaySound("footstep.wav");
};
```

### 5. Performance Optimization
- **Use sprite sheets** instead of individual images
- **Reuse clips** across multiple entities (register once, reference many times)
- **Limit frame count** - most animations don't need more than 8-12 frames
- **Use power-of-2 textures** when possible for better GPU performance

### 6. Memory Management
```cpp
// Register clips once at scene initialization
void MyScene::OnAttach() {
    auto animSystem = GetSystem<AnimationSystem>();
    
    // Load from JSON (better than hardcoding)
    auto idleClip = AnimationLoader::LoadFromJSON("idle.anim.json");
    auto walkClip = AnimationLoader::LoadFromJSON("walk.anim.json");
    
    animSystem->RegisterClip(idleClip);
    animSystem->RegisterClip(walkClip);
}

// Reuse registered clips
void SpawnEnemy(const glm::vec2& position) {
    auto enemy = CreateEntity("Enemy");
    auto& anim = enemy.AddComponent<AnimationComponent>();
    anim.Play("EnemyIdle");  // Uses pre-registered clip
}
```

---

## Examples

### Example 1: Simple Character Animation
```cpp
void SetupCharacterAnimation(Pillar::Scene& scene, Pillar::Entity entity) {
    // Create idle animation
    Pillar::AnimationClip idle;
    idle.Name = "Idle";
    idle.IsLooping = true;
    idle.PlaybackSpeed = 1.0f;
    
    // Add frames (individual images)
    for (int i = 0; i < 4; i++) {
        Pillar::AnimationFrame frame;
        frame.TexturePath = "character_idle_" + std::to_string(i) + ".png";
        frame.Duration = 0.15f;
        idle.Frames.push_back(frame);
    }
    
    // Register and play
    auto animSystem = scene.GetSystem<Pillar::AnimationSystem>();
    animSystem->RegisterClip(idle);
    
    auto& anim = entity.AddComponent<Pillar::AnimationComponent>();
    anim.Play("Idle");
}
```

### Example 2: Sprite Sheet Animation
```cpp
Pillar::AnimationClip CreateWalkAnimation() {
    Pillar::AnimationClip walk;
    walk.Name = "Walk";
    walk.IsLooping = true;
    
    // 4x2 sprite sheet (8 frames total, using top row for walk)
    for (int i = 0; i < 4; i++) {
        Pillar::AnimationFrame frame;
        frame.TexturePath = "character_spritesheet.png";  // Same texture
        
        float cellWidth = 0.25f;   // 1/4 of texture width
        float cellHeight = 0.5f;   // 1/2 of texture height
        float minX = i * cellWidth;
        float minY = 0.5f;  // Top row
        float maxX = minX + cellWidth;
        float maxY = 1.0f;
        
        frame.UVRect = glm::vec4(minX, minY, maxX, maxY);
        frame.Duration = 0.1f;
        walk.Frames.push_back(frame);
    }
    
    return walk;
}
```

### Example 3: Attack Animation with Events
```cpp
Pillar::AnimationClip CreateAttackAnimation() {
    Pillar::AnimationClip attack;
    attack.Name = "Attack";
    attack.IsLooping = false;  // Play once
    attack.PlaybackSpeed = 1.5f;  // Faster attack
    
    // Frames
    for (int i = 0; i < 6; i++) {
        Pillar::AnimationFrame frame;
        frame.TexturePath = "attack_" + std::to_string(i) + ".png";
        frame.Duration = (i < 3) ? 0.05f : 0.08f;  // Fast start, slower end
        attack.Frames.push_back(frame);
    }
    
    // Impact event on frame 3
    Pillar::AnimationClip::AnimationEvent impactEvent;
    impactEvent.FrameIndex = 3;
    impactEvent.EventName = "Impact";
    impactEvent.Callback = []() {
        PIL_INFO("IMPACT! Deal damage here");
        // Play impact sound
        // Deal damage to targets
        // Spawn hit particles
    };
    attack.Events.push_back(impactEvent);
    
    return attack;
}
```

### Example 4: State-Based Animation System
```cpp
class PlayerController : public Pillar::Layer {
    enum class State { Idle, Walking, Jumping, Attacking };
    State m_CurrentState = State::Idle;
    Pillar::Entity m_Player;
    
    void OnUpdate(float dt) override {
        auto& anim = m_Player.GetComponent<Pillar::AnimationComponent>();
        State newState = m_CurrentState;
        
        // Determine state
        if (Pillar::Input::IsKeyPressed(PIL_KEY_SPACE)) {
            newState = State::Attacking;
        } else if (Pillar::Input::IsKeyPressed(PIL_KEY_W)) {
            newState = State::Walking;
        } else {
            newState = State::Idle;
        }
        
        // Change animation if state changed
        if (newState != m_CurrentState) {
            m_CurrentState = newState;
            
            switch (m_CurrentState) {
                case State::Idle:
                    anim.Play("Idle");
                    break;
                case State::Walking:
                    anim.Play("Walk");
                    break;
                case State::Attacking:
                    anim.Play("Attack");
                    // Return to idle when done
                    anim.OnAnimationComplete = [this, &anim]() {
                        m_CurrentState = State::Idle;
                        anim.Play("Idle");
                    };
                    break;
            }
        }
    }
};
```

### Example 5: Loading from JSON
```cpp
void LoadAndRegisterAnimations(Pillar::Scene& scene) {
    auto animSystem = scene.GetSystem<Pillar::AnimationSystem>();
    
    // Load all character animations from JSON files
    std::vector<std::string> animFiles = {
        "idle.anim.json",
        "walk.anim.json", 
        "run.anim.json",
        "jump.anim.json",
        "attack.anim.json"
    };
    
    for (const auto& file : animFiles) {
        auto clip = Pillar::AnimationLoader::LoadFromJSON(file);
        
        // Re-attach event callbacks (not saved in JSON)
        if (clip.Name == "Attack") {
            for (auto& event : clip.Events) {
                if (event.EventName == "Impact") {
                    event.Callback = []() { DealDamage(); };
                }
            }
        }
        
        animSystem->RegisterClip(clip);
        PIL_INFO("Loaded animation: {}", clip.Name);
    }
}
```

---

## Troubleshooting

### Animation Not Playing
- ✅ Check clip is registered: `animSystem->RegisterClip(clip)`
- ✅ Check clip name matches: `anim.Play("ExactName")`
- ✅ Check `Playing` flag: `anim.Playing` should be `true`
- ✅ Check system is updating: `scene.OnUpdate(dt)` must be called

### Texture Not Loading
- ✅ Check texture path is correct (relative to assets folder)
- ✅ Check file exists in `Sandbox/assets/textures/`
- ✅ Check file format is supported (PNG, JPG, BMP, TGA)
- ✅ Look for AssetManager warnings in console

### UV Coordinates Wrong
- ✅ Remember: (0,0) is bottom-left, (1,1) is top-right
- ✅ Check min < max for both X and Y
- ✅ Verify sprite sheet layout matches UV calculations
- ✅ Use a tool like TexturePacker to generate coordinates

### Events Not Firing
- ✅ Check FrameIndex is valid (0 to frameCount-1)
- ✅ Check callback is set before playing
- ✅ Events fire once per loop - make sure animation is looping if needed
- ✅ JSON-loaded events need callbacks re-attached in code

---

## API Reference Summary

### Core Classes
- `AnimationFrame` - Single frame of animation
- `AnimationClip` - Complete animation sequence
- `AnimationComponent` - Entity component for playback control
- `AnimationSystem` - System that updates all animated entities
- `AnimationLoader` - JSON serialization utilities

### Key Methods
```cpp
// AnimationComponent
void Play(const std::string& clipName);
void Pause();
void Resume();
void Stop();
bool HasAnimation() const;
bool IsPlaying() const;

// AnimationSystem
void RegisterClip(const AnimationClip& clip);
const AnimationClip* GetClip(const std::string& name) const;
void ClearLibrary();

// AnimationLoader
static void SaveToJSON(const AnimationClip& clip, const std::string& filepath);
static AnimationClip LoadFromJSON(const std::string& filepath);
```

---

## Additional Resources

- **Demo Layer**: See `Sandbox/src/AnimationDemoLayer.h` for complete working example
- **Tests**: See `Tests/src/AnimationTests.cpp` for usage examples
- **Asset Manager**: See `Pillar/docs/AssetManager.md` for path resolution details

---

**Version**: 1.0  
**Last Updated**: December 10, 2025  
**Author**: Pillar Engine Team
