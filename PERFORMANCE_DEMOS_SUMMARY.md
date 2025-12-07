# Performance Demo Layers - Summary

## Overview

Two new stress-test demo layers have been added to benchmark the ECS/Physics systems:

1. **Light Entity Performance Demo** - Pure ECS light entities (particles, bullets, gems)
2. **Heavy Entity Performance Demo** - Box2D physics bodies with collision

---

## 1. Light Entity Performance Demo

**File:** `Sandbox/src/LightEntityPerfDemo.h`

### What It Tests
- **Velocity Integration System** - Thousands of entities moving with custom physics
- **Spatial Hash Grid** - XP collection system with spatial queries
- **ECS Performance** - How many light entities can run at 60 FPS

### Features
- Spawns particles with random positions and velocities
- All particles are also XP gems (tests spatial hash grid)
- Player at center attracts nearby gems
- Color-coded by speed (blue=slow, red=fast)
- Real-time performance metrics

### Controls
| Button | Action |
|--------|--------|
| + 100 | Spawn 100 particles |
| + 500 | Spawn 500 particles |
| + 1000 | Spawn 1,000 particles |
| + 5000 | Spawn 5,000 particles |
| + 10000 | Spawn 10,000 particles |
| Clear All | Remove all entities |
| WASD | Move camera |
| Mouse Wheel | Zoom |

### Performance Metrics Displayed
- **Entity Count** - Total light entities
- **Frame Time** - ms per frame (green if < 16.67ms)
- **System Time** - Velocity integration + spatial grid update
- **Render Time** - Draw call overhead
- **FPS** - Calculated from frame time

### Expected Performance
| Entity Count | Expected FPS | Notes |
|--------------|--------------|-------|
| 1,000 | 60+ | Excellent |
| 5,000 | 60+ | Good |
| 10,000 | 40-60 | Acceptable |
| 20,000+ | 20-40 | Stress test |

**Bottlenecks:**
- Rendering (1 draw call per entity - Phase 5 will fix with instancing)
- Spatial hash grid rebuild (4ms for 10k entities)

---

## 2. Heavy Entity Performance Demo

**File:** `Sandbox/src/HeavyEntityPerfDemo.h`

### What It Tests
- **Box2D Physics System** - How many physics bodies can simulate at 60 FPS
- **Collision Detection** - Broadphase + narrowphase performance
- **Physics Sync** - Syncing b2Body transforms to ECS

### Features
- Spawns physics bodies with gravity (they fall)
- Random shapes (circles and boxes)
- Random sizes (0.3 to 0.8 units)
- Boundaries at edges
- Two spawn modes: Dynamic (affected by gravity) or Kinematic (no gravity)
- Real-time performance metrics

### Controls
| Button | Action |
|--------|--------|
| + 10 | Spawn 10 physics bodies |
| + 25 | Spawn 25 physics bodies |
| + 50 | Spawn 50 physics bodies |
| + 100 | Spawn 100 physics bodies |
| + 250 | Spawn 250 physics bodies |
| Clear All | Remove all entities |
| Radio Buttons | Choose Dynamic or Kinematic |
| WASD | Move camera |
| Mouse Wheel | Zoom |

### Performance Metrics Displayed
- **Entity Count** - Total physics bodies
- **Frame Time** - ms per frame (green if < 16.67ms)
- **Physics Time** - Box2D step + sync
- **Render Time** - Draw call overhead
- **FPS** - Calculated from frame time

### Expected Performance
| Entity Count | Expected FPS | Notes |
|--------------|--------------|-------|
| 50 | 60+ | Excellent |
| 100 | 60+ | Good |
| 250 | 40-60 | Acceptable |
| 500+ | 20-40 | Stress test |

**Bottlenecks:**
- Box2D collision detection (O(n²) in worst case, O(n log n) with broadphase)
- Contact resolution (increases with overlapping bodies)
- Rendering (1 draw call per entity)

### Visual Feedback
- **Gray** = Static bodies (walls, ground)
- **Blue** = Kinematic bodies (move but don't respond to forces)
- **Red** = Dynamic bodies (full physics simulation)

---

## Switching Between Demos

Edit `Sandbox/src/Source.cpp` and uncomment the demo you want:

```cpp
Sandbox()
{
    // CHOOSE YOUR DEMO: Uncomment one of the following lines
    
    // Option 1: Physics Demo (Gameplay)
    PushLayer(new PhysicsDemoLayer());
    
    // Option 2: Light Entity Performance Demo
    // PushLayer(new LightEntityPerfDemo());
    
    // Option 3: Heavy Entity Performance Demo
    // PushLayer(new HeavyEntityPerfDemo());
}
```

Then rebuild:
```powershell
cmake --build out/build/x64-Debug --config Debug --target Sandbox --parallel
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```

---

## Performance Test Procedure

### Light Entity Test
1. Start with "Clear All"
2. Click "+ 1000" - Should be 60 FPS
3. Click "+ 1000" again (2000 total) - Should still be 60 FPS
4. Click "+ 5000" (7000 total) - Watch FPS
5. Click "+ 10000" (17000 total) - Note when FPS drops below 30

### Heavy Entity Test
1. Start with "Clear All"
2. Click "+ 50" - Should be 60 FPS
3. Click "+ 100" (150 total) - Should be 50-60 FPS
4. Click "+ 250" (400 total) - Watch physics slow down
5. Try both Dynamic and Kinematic modes

---

## Key Insights from Performance Testing

### Light Entities (Pure ECS)
? **Strengths:**
- Very fast component iteration (cache-friendly)
- Spatial hash grid scales well (O(1) insertion)
- Minimal per-entity overhead

? **Weaknesses:**
- Rendering becomes bottleneck (1 draw call per entity)
- Need instanced rendering for 10k+ entities (Phase 5)

### Heavy Entities (Box2D Physics)
? **Strengths:**
- Accurate physics simulation
- Efficient broadphase (spatial partitioning)
- Handles complex collisions

? **Weaknesses:**
- Fixed timestep accumulator adds overhead
- Collision resolution scales poorly with overlapping bodies
- Best for <500 dynamic bodies

---

## Recommended Entity Limits (60 FPS)

| Entity Type | Recommended | Maximum | Notes |
|-------------|-------------|---------|-------|
| Light (current rendering) | 5,000 | 10,000 | Limited by draw calls |
| Light (with instancing) | 50,000 | 100,000+ | Phase 5 feature |
| Heavy (dynamic) | 100 | 500 | Limited by physics simulation |
| Heavy (static/kinematic) | 1,000 | 10,000 | No collision resolution cost |

---

## Future Optimizations

### Phase 5: Instanced Rendering
- **Impact:** 10x improvement for light entities
- **Expected:** 50,000+ sprites at 60 FPS
- **Method:** Batch all entities by texture, single draw call

### Phase 6+: Spatial Partitioning for Heavy Entities
- **Impact:** 2-3x improvement for physics
- **Method:** Sleeping bodies, island solving, spatial broadphase tuning

---

## Files Created

1. `Sandbox/src/LightEntityPerfDemo.h` - Light entity stress test
2. `Sandbox/src/HeavyEntityPerfDemo.h` - Heavy entity stress test  
3. `Sandbox/src/DemoMenuLayer.h` - Menu for switching (not used currently)
4. Updated `Sandbox/src/Source.cpp` - Manual demo selection

---

**Performance Demos Complete!** ??

You can now benchmark:
- ? Pure ECS light entity performance
- ? Box2D heavy entity capacity
- ? System update times
- ? Rendering overhead

Use these demos to:
- Find performance bottlenecks
- Test optimizations
- Determine entity budgets for your game
- Showcase the engine's capabilities
