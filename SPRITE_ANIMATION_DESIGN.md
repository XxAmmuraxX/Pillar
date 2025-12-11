# Sprite Animation System Design

## User Requirements (Confirmed)

1. **Sprite Format**: Basic sprites (individual texture files, no atlases)
2. **Animation Data**: JSON files for reusability
3. **Animation Events**: Yes, required for triggering actions on specific frames
4. **State Machines**: Deferred (see explanation below)
5. **Rendering Method**: TBD (recommendation: modify SpriteComponent)
6. **Frame Rate**: Fixed FPS (uniform frame duration)

## State Machines vs Animation Blending - Explanation

**State Machines:**
- Define animation transitions with conditions (e.g., `idle → walk` when velocity > 0)
- Discrete state changes (instant frame switch)
- Example: Player enters "jumping" state, plays jump animation once, returns to idle
- Good for: Character controllers, AI behaviors, gameplay logic
- Implementation: Graph of states with transition rules

**Animation Blending:**
- Smoothly interpolate between two animations over time
- Creates fluid motion (e.g., blending from walk to run as speed increases)
- Example: Walk at 0% → 50% walk + 50% run → 100% run
- Good for: Realistic motion, speed variations, directional movement
- Implementation: Weighted average of multiple animation frames

**Recommendation for Pillar Engine:**
- **Phase 1**: Core animation system (frame playback, events)
- **Phase 2**: State machines (more useful for 2D games, easier to implement)
- **Phase 3**: Blending (optional, primarily for 3D/skeletal animation)

Since you're building a 2D engine, state machines are more practical than blending. We can defer both to Phase 2.

## Rendering Method - Recommendation

**Option A: Modify SpriteComponent** (RECOMMENDED)
```cpp
struct SpriteComponent {
    std::string TexturePath;
    glm::vec4 Color = glm::vec4(1.0f);
    glm::vec2 UVMin = glm::vec2(0.0f);  // NEW: UV coordinates
    glm::vec2 UVMax = glm::vec2(1.0f);  // NEW: for animation frames
};
```
**Pros:**
- Works with existing SpriteRenderSystem (no changes needed)
- Simple architecture (AnimationSystem just updates UVs)
- Backwards compatible (default UVs cover entire texture)
- Can animate any sprite-rendered entity

**Option B: New AnimatedSpriteComponent**
```cpp
struct AnimatedSpriteComponent {
    std::string CurrentAnimation;
    int FrameIndex;
    float PlaybackTime;
    // ... animation state
};
```
**Pros:**
- Separates concerns (static sprites vs animated sprites)
- Entities can have both (SpriteComponent for static, AnimatedSpriteComponent for frames)

**Cons:**
- Requires new rendering system or modified SpriteRenderSystem
- More complex (two components per animated entity)

**RECOMMENDATION**: Option A (modify SpriteComponent). It's simpler, works with existing systems, and doesn't break anything.

## Proposed Architecture (Finalized)

### 1. Core Structures

**AnimationFrame.h** - Single frame data
```cpp
struct AnimationFrame {
    std::string TexturePath;     // Path to sprite image
    float Duration;              // Frame duration in seconds (fixed FPS: 1.0f/fps)
    glm::vec2 UVMin;            // UV coordinates (0,0) for full texture
    glm::vec2 UVMax;            // UV coordinates (1,1) for full texture
};
```

**AnimationClip.h** - Sequence of frames
```cpp
struct AnimationClip {
    std::string Name;                    // "player_walk", "enemy_attack"
    std::vector<AnimationFrame> Frames;
    bool Loop = true;                    // Loop or play once?
    float PlaybackSpeed = 1.0f;         // Speed multiplier
    
    // NEW: Animation events
    struct AnimationEvent {
        int FrameIndex;                  // Fire event when reaching this frame
        std::string EventName;           // "footstep", "attack_hit", "spawn_particle"
    };
    std::vector<AnimationEvent> Events;
};
```

**AnimationComponent.h** - Entity animation state
```cpp
struct AnimationComponent {
    std::string CurrentClipName;         // Currently playing animation
    int FrameIndex = 0;                  // Current frame in clip
    float PlaybackTime = 0.0f;          // Time elapsed in current frame
    float PlaybackSpeed = 1.0f;         // Speed multiplier (can override clip speed)
    bool Playing = true;                 // Pause/resume control
    
    // NEW: Event callback storage
    using EventCallback = std::function<void(const std::string& eventName, entt::entity entity)>;
    EventCallback OnAnimationEvent;      // User-defined callback for events
    
    // Helper methods
    void Play(const std::string& clipName);
    void Pause();
    void Resume();
    void Stop();  // Reset to frame 0
};
```

### 2. Animation System

**AnimationSystem.h/cpp**
```cpp
class AnimationSystem : public System {
public:
    void OnUpdate(float dt, entt::registry& registry) override;
    
    // Animation library management
    void LoadAnimationClip(const std::string& filePath);  // Load from JSON
    AnimationClip* GetClip(const std::string& name);
    
private:
    std::unordered_map<std::string, AnimationClip> m_AnimationLibrary;
    
    void UpdateAnimation(AnimationComponent& anim, SpriteComponent& sprite, float dt);
    void AdvanceFrame(AnimationComponent& anim, AnimationClip& clip);
    void UpdateSpriteUVs(SpriteComponent& sprite, const AnimationFrame& frame);
    void FireAnimationEvents(AnimationComponent& anim, AnimationClip& clip, int oldFrame, entt::entity entity);
};
```

**Update Logic:**
1. Query all entities with `AnimationComponent` + `SpriteComponent`
2. Get current `AnimationClip` from library
3. Advance `PlaybackTime` by `dt * PlaybackSpeed`
4. If `PlaybackTime` >= current frame duration:
   - Fire any events for completed frame
   - Advance to next frame (or loop/stop)
   - Reset `PlaybackTime`
5. Update `SpriteComponent` UVs from current frame's texture path and coordinates

### 3. Animation Loader

**AnimationLoader.h/cpp** - JSON parsing utility
```cpp
class AnimationLoader {
public:
    static AnimationClip LoadFromJSON(const std::string& filePath);
    static void SaveToJSON(const AnimationClip& clip, const std::string& filePath);
};
```

## JSON File Format

**Example: `player_walk.anim.json`**
```json
{
  "name": "player_walk",
  "loop": true,
  "playbackSpeed": 1.0,
  "frames": [
    {
      "texturePath": "player_walk_00.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [1.0, 1.0]
    },
    {
      "texturePath": "player_walk_01.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [1.0, 1.0]
    },
    {
      "texturePath": "player_walk_02.png",
      "duration": 0.1,
      "uvMin": [0.0, 0.0],
      "uvMax": [1.0, 1.0]
    }
  ],
  "events": [
    {
      "frameIndex": 1,
      "eventName": "footstep_left"
    },
    {
      "frameIndex": 4,
      "eventName": "footstep_right"
    }
  ]
}
```

## Animation Events - Implementation

**Callback Registration (in game code):**
```cpp
auto player = scene.CreateEntity("Player");
auto& anim = player.AddComponent<AnimationComponent>();

// Register event callback
anim.OnAnimationEvent = [&](const std::string& eventName, entt::entity entity) {
    if (eventName == "footstep_left" || eventName == "footstep_right") {
        // Play footstep sound
        AudioEngine::PlaySound("footstep.wav");
    }
    else if (eventName == "attack_hit") {
        // Check for collision, deal damage
        CheckAttackCollision(entity);
    }
    else if (eventName == "spawn_particle") {
        // Spawn particle effect at entity position
        SpawnParticleEffect(entity, "slash_effect");
    }
};

anim.Play("player_walk");
```

**Event Firing (in AnimationSystem):**
```cpp
void AnimationSystem::FireAnimationEvents(AnimationComponent& anim, AnimationClip& clip, int oldFrame, entt::entity entity) {
    // Fire all events between oldFrame and current frame
    for (const auto& event : clip.Events) {
        if (event.FrameIndex == anim.FrameIndex && oldFrame != anim.FrameIndex) {
            if (anim.OnAnimationEvent) {
                anim.OnAnimationEvent(event.EventName, entity);
            }
        }
    }
}
```

## Modified SpriteComponent

**SpriteComponent.h** (add UV coordinates)
```cpp
struct SpriteComponent {
    std::string TexturePath = "";
    glm::vec4 Color = glm::vec4(1.0f);
    
    // NEW: UV coordinates for texture sampling (default covers entire texture)
    glm::vec2 UVMin = glm::vec2(0.0f, 0.0f);
    glm::vec2 UVMax = glm::vec2(1.0f, 1.0f);
};
```

**No changes needed to SpriteRenderSystem** - it already passes texture coordinates to shader. We'll just update the component's UVs, and the renderer will use them automatically.

## Implementation Plan - Phase 1

### Files to Create
1. `Pillar/src/Pillar/ECS/Components/Rendering/AnimationFrame.h` - Frame data structure
2. `Pillar/src/Pillar/ECS/Components/Rendering/AnimationClip.h` - Clip data structure
3. `Pillar/src/Pillar/ECS/Components/Rendering/AnimationComponent.h` - Entity component
4. `Pillar/src/Pillar/ECS/Systems/AnimationSystem.h` - System header
5. `Pillar/src/Pillar/ECS/Systems/AnimationSystem.cpp` - System implementation
6. `Pillar/src/Pillar/Utils/AnimationLoader.h` - JSON loader header
7. `Pillar/src/Pillar/Utils/AnimationLoader.cpp` - JSON loader implementation
8. `Sandbox/src/AnimationDemoLayer.h` - Demo layer

### Files to Modify
1. `Pillar/src/Pillar/ECS/Components/Rendering/SpriteComponent.h` - Add UVMin/UVMax
2. `Pillar/src/Pillar/ECS/BuiltinComponentRegistrations.cpp` - Register AnimationComponent
3. `Pillar/CMakeLists.txt` - Add new source files
4. `Sandbox/src/Source.cpp` - Add demo layer to application

### Implementation Steps
1. **Step 1**: Modify SpriteComponent (add UV coordinates)
2. **Step 2**: Create AnimationFrame and AnimationClip structures
3. **Step 3**: Create AnimationComponent with event callback support
4. **Step 4**: Implement AnimationSystem (update logic, event firing)
5. **Step 5**: Implement AnimationLoader (JSON parsing)
6. **Step 6**: Register AnimationComponent with JSON serialization
7. **Step 7**: Update CMakeLists.txt
8. **Step 8**: Create demo layer with sample animations
9. **Step 9**: Test with multiple animated entities

## Testing Strategy

### Test Cases
1. **Basic Playback**: Single animation loops correctly
2. **One-Shot Animation**: Non-looping animation stops at last frame
3. **Animation Switching**: Change from one animation to another mid-playback
4. **Playback Speed**: Test 0.5x, 1.0x, 2.0x speeds
5. **Pause/Resume**: Pause animation, resume from same frame
6. **Animation Events**: Events fire on correct frames, callback receives correct data
7. **Multiple Entities**: 100+ entities with different animations, no performance issues
8. **JSON Loading**: Load animation from file, verify all properties parsed correctly
9. **Missing Animation**: Request non-existent clip, handle gracefully

### Sample Assets Needed
- Character sprite sequence: `player_idle_00.png` through `player_idle_03.png` (4 frames)
- Character sprite sequence: `player_walk_00.png` through `player_walk_05.png` (6 frames)
- Enemy sprite sequence: `enemy_attack_00.png` through `enemy_attack_04.png` (5 frames)
- JSON files: `player_idle.anim.json`, `player_walk.anim.json`, `enemy_attack.anim.json`

## Phase 2 Features (Future)

1. **State Machine System**:
   - AnimationStateMachine component
   - State graph with transitions
   - Condition evaluation (velocity, health, input)
   - Auto-transition between animations

2. **Advanced Events**:
   - Event parameters (pass data with events)
   - Timeline events (multiple events per frame)
   - Event preview in editor

3. **Performance Optimizations**:
   - Texture atlas support (multiple frames in one texture)
   - Animation compression (shared frame data)
   - LOD system (skip frames for distant entities)

4. **Editor Integration**:
   - Visual animation timeline editor
   - Event marker UI
   - Preview window

## Ready to Implement?

Phase 1 is fully designed. Ready to proceed with implementation?
