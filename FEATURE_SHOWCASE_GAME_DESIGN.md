# Pillar Engine - Feature Showcase Game Design Document

## Project Codename: **"Arena Protocol"**

### A Technical Demo & System Validation Experience

---

## 1. Executive Summary

**Arena Protocol** is a top-down shooter designed as a **Technical Showcase** for the Pillar Engine. The primary goal is not entertainment, but to **validate every functional system** in the engine through gameplay mechanics. Players navigate through interconnected "Test Chambers," each designed to stress-test specific engine subsystems.

**Genre:** Top-Down Arena Shooter / Technical Demo  
**Perspective:** 2D Orthographic (Top-Down)  
**Primary Mechanic:** Shooting as Combat  
**Target:** Engine developers, technical artists, and QA validation

---

## 2. Design Philosophy

### 2.1 Core Principles

1. **Every mechanic validates a system** - No feature exists purely for "fun"; each element tests engine functionality
2. **Observable debugging** - All systems display visual/audio feedback for easy verification
3. **Isolated testing** - Each chamber can be loaded independently for targeted debugging
4. **Scalability stress-testing** - Entity counts can be cranked up to find performance limits

### 2.2 Systems to Validate

| Engine System | Gameplay Feature |
|--------------|------------------|
| **ECS (EnTT)** | Entity spawning, component queries, entity destruction |
| **Physics (Box2D)** | Rigidbody movement, collision detection, sensors |
| **Renderer2D** | Sprite rendering, batch rendering, texture atlases |
| **Animation System** | Sprite sheet animations, animation events |
| **Particle System** | Emitters, particle physics, color gradients |
| **Audio System (OpenAL)** | SFX playback, 3D positional audio, music loops |
| **Object Pooling** | Bullet pools, enemy pools, particle recycling |
| **Scene Management** | Scene loading, serialization, transitions |
| **Camera System** | Camera follow, zoom, shake effects |
| **Input System** | WASD movement, mouse aiming, action mapping |
| **Layer System** | Game layer, UI layer, debug overlay |
| **Spatial Hash Grid** | Broad-phase collision for Light Entities |
| **Bullet Collision System** | Raycasting, damage attribution |

---

## 3. Entity Definitions

### 3.1 Player Entity

**Name:** `Player`  
**Type:** Heavy Entity (has Box2D RigidbodyComponent)

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "Player" |
| `TransformComponent` | Position: spawn point, Rotation: mouse-aimed |
| `SpriteComponent` | Texture: `player_ship.png`, Size: (1.0, 1.0) |
| `AnimationComponent` | Clips: "idle", "thrust", "damaged" |
| `RigidbodyComponent` | BodyType: Dynamic, FixedRotation: false |
| `ColliderComponent` | Type: Circle, Radius: 0.4, IsSensor: false |
| `VelocityComponent` | Max speed tracking |
| `AudioSourceComponent` | Engine hum, weapon fire SFX |
| `PlayerStatsComponent`* | Health: 100, XP: 0, Level: 1 |

*Custom component for this showcase

**Visual Description:**
- **Shape:** Sleek triangular spacecraft with forward-facing nose cone
- **Color Scheme:** Cyan/electric blue primary body with white energy highlights
- **Details:** Glowing engine thrusters at rear (intensify during movement), weapon mounts on sides
- **Scale:** 64x64 pixels, roughly 1 world unit diameter
- **Animation Frames:**
  - *Idle:* Subtle pulsing glow on energy core
  - *Thrust:* Engine trails extend, ship slightly tilts forward
  - *Damaged:* Sparks/smoke particles, red damage indicators, flickering lights
- **Visual Feedback:** Rotation always points toward mouse cursor, creating dynamic aiming feel

**Behaviors:**
- WASD movement (velocity-based, not position snapping)
- Mouse aiming (rotation follows cursor)
- Left-click: Fire primary weapon (pooled bullets)
- Right-click: Fire secondary weapon (rockets/spread shot)
- Space: Dash ability (tests physics impulse)

---

### 3.2 Bullet Entity (Pooled)

**Name:** `Bullet`  
**Type:** Light Entity (NO RigidbodyComponent - uses Spatial Hash)

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "Bullet" |
| `TransformComponent` | Position: spawn at gun barrel |
| `SpriteComponent` | Texture: `bullet.png`, Size: (0.2, 0.4) |
| `VelocityComponent` | Speed: 25.0 units/sec |
| `BulletComponent` | Damage: 10, Lifetime: 3.0s, Pierce: false |
| `ColliderComponent` | Type: Circle, Radius: 0.1, IsSensor: true |

**Visual Description:**
- **Shape:** Elongated oval/teardrop projectile
- **Color Scheme:** Bright cyan core with white outer glow
- **Effects:** Motion blur trail (particle system), slight rotation during flight
- **Scale:** 16x32 pixels (0.2x0.4 world units)
- **Variants:**
  - *Player Bullet:* Cyan/white energy bolt
  - *Enemy Bullet:* Red/orange plasma shot (uses `enemy_bullet.png`)
- **Lifetime Visual:** Fades out in final 0.5 seconds before despawning

**Pooling Configuration:**
- Initial Pool Size: 200 bullets
- Max Pool Size: 500 bullets
- On Release: Reset position, disable sprite, clear velocity

**Systems Validated:**
- `BulletCollisionSystem` - Raycast hit detection
- `ObjectPool` - Acquire/Release cycle
- `VelocityIntegrationSystem` - Movement without Box2D
- `SpatialHashGrid` - Broad-phase collision queries

---

### 3.3 Enemy: Drone (Basic)

**Name:** `EnemyDrone`  
**Type:** Heavy Entity

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "EnemyDrone" |
| `TransformComponent` | Position: spawn points |
| `SpriteComponent` | Texture: `drone.png`, Color: red tint |
| `AnimationComponent` | Clips: "idle", "attack", "death" |
| `RigidbodyComponent` | BodyType: Dynamic |
| `ColliderComponent` | Type: Circle, Radius: 0.5 |
| `HealthComponent`* | Health: 30, MaxHealth: 30 |
| `EnemyAIComponent`* | Behavior: SeekPlayer, Speed: 3.0 |

**Visual Description:**
- **Shape:** Hexagonal compact drone body with small wing-like stabilizers
- **Color Scheme:** Dark metallic red chassis with orange warning lights
- **Details:** Single glowing red "eye" sensor in center, rotating radar dish on top
- **Scale:** 48x48 pixels (approximately 0.8 world units)
- **Animation Frames:**
  - *Idle:* Gentle bobbing motion, radar dish rotates slowly
  - *Attack:* Red eye flashes, slight forward lunge
  - *Death:* Explosion sequence (4 frames), smoke plume, sparks scatter
- **Health Indicator:** Red tint darkens as health decreases, smoke particles below 30% HP

**Behaviors:**
- Seek player position
- On death: Spawn XP gem, play death animation, emit particles

**Systems Validated:**
- `PhysicsSystem` - Box2D body movement
- `AnimationSystem` - Death animation sequence
- `Box2DContactListener` - Collision callbacks

---

### 3.4 Enemy: Turret (Stationary)

**Name:** `EnemyTurret`  
**Type:** Heavy Entity

| Component | Configuration |
|-----------|-------------|
| `TagComponent` | Name: "EnemyTurret" |
| `TransformComponent` | Position: fixed |
| `SpriteComponent` | Texture: `turret_base.png` |
| `RigidbodyComponent` | BodyType: Static |
| `ColliderComponent` | Type: Circle, Radius: 0.6 |
| `HealthComponent`* | Health: 80 |
| `TurretAIComponent`* | FireRate: 1.0/sec, Range: 10.0 |

**Visual Description:**
- **Shape:** Two-part design - circular base platform + rotating barrel assembly
- **Color Scheme:** Gunmetal gray base with red accent strips, glowing orange barrel tip
- **Details:**
  - *Base:* 64x64px fixed platform with warning stripes, anchor points
  - *Barrel:* 32x48px rotating weapon mount (separate sprite, child transform)
- **Scale:** Base is 1.2 world units diameter
- **Visual Feedback:**
  - Barrel rotates smoothly to track player (shows AI targeting)
  - Orange muzzle flash when firing (particle burst)
  - Red targeting laser beam points toward player (optional toggle)
  - Smoke vents on base activate during continuous fire
- **Damage States:** Sparks and cracks appear at 50% HP, heavy smoke at 25% HP

**Behaviors:**
- Rotate to track player
- Fire bullets at intervals (uses enemy bullet pool)
- Tests: Static body collision, enemy shooting mechanics

---

### 3.5 Enemy: Charger (Fast Melee)

**Name:** `EnemyCharger`  
**Type:** Heavy Entity

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "EnemyCharger" |
| `TransformComponent` | Position: spawn points |
| `SpriteComponent` | Texture: `charger.png` |
| `AnimationComponent` | Clips: "idle", "charge_windup", "charging" |
| `RigidbodyComponent` | BodyType: Dynamic, FixedRotation: true |
| `ColliderComponent` | Type: Box, HalfExtents: (0.4, 0.6) |
| `HealthComponent`* | Health: 50 |
| `ChargerAIComponent`* | ChargeSpeed: 15.0, WindupTime: 0.5s |

**Visual Description:**
- **Shape:** Elongated ram-like chassis with reinforced front wedge
- **Color Scheme:** Burnt orange armor plating with yellow hazard stripes
- **Details:** Heavy frontal ram (battering shield), exposed engine core on sides, dual thruster exhausts
- **Scale:** 48x64 pixels (taller than wide, emphasizing forward aggression)
- **Animation Frames:**
  - *Idle:* Slight sway side-to-side, thrusters idle glow
  - *Charge Windup:* Pulls back slightly, thrusters ignite (bright yellow), screen shake warning
  - *Charging:* Full speed forward, motion blur effect, thruster trail particles (fire)
  - *Impact:* Flash of white, knockback recoil, brief stun frame
- **Audio-Visual Sync:** Engine revving sound matches windup animation intensity
- **Trail Effect:** During charge, leaves temporary speed lines/afterimages

**Behaviors:**
- Wind-up animation before charging
- Dash toward player's last known position
- On contact: Deal damage, apply knockback
- Tests: Physics impulses, animation events

---

### 3.6 Boss: Sentinel Core

**Name:** `BossSentinel`  
**Type:** Heavy Entity

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "BossSentinel" |
| `TransformComponent` | Scale: (3.0, 3.0) |
| `SpriteComponent` | Texture: `sentinel_boss.png` |
| `AnimationComponent` | Clips: "idle", "attack_laser", "summon", "phase2" |
| `RigidbodyComponent` | BodyType: Kinematic |
| `ColliderComponent` | Type: Circle, Radius: 1.5 |
| `HealthComponent`* | Health: 500, Phases: 2 |
| `BossAIComponent`* | Pattern: Sequential |
| `ParticleEmitterComponent` | Ambient energy particles |
| `AudioSourceComponent` | Boss music, attack SFX (3D positioned) |

**Visual Description:**
- **Shape:** Massive spherical core surrounded by rotating orbital rings (3 layers)
- **Color Scheme:** Deep purple core with electric blue energy veins, silver metallic rings
- **Details:**
  - Central "eye" that opens/closes during attacks (iris-like aperture)
  - Three independently rotating orbital rings with glowing nodes
  - Energy conduits pulse with power surges
- **Scale:** 192x192 pixels (3x3 world units - dominates the arena)
- **Animation Frames:**
  - *Idle:* Slow orbital ring rotation, core pulsing gently, ambient particle emission
  - *Attack_Laser:* Eye opens wide, intense charging glow, rings align and lock
  - *Summon:* Rings spin rapidly, nodes flash in sequence, portals spawn drones
  - *Phase2:* Color shift to red/orange, faster animations, cracked core texture, more particles
- **Phase Transition:** Screen shake, color inversion flash, rings shatter and reform with cracks
- **Particle Effects:** Continuous ambient energy wisps in Phase 1, aggressive fire/lightning in Phase 2
- **Shield Visual:** Hexagonal barrier appears when active (pulsing cyan dome)

**Attack Patterns:**
1. **Bullet Spray** - Circular bullet pattern (tests high bullet count)
2. **Laser Sweep** - Raycast-based instant damage (tests raycast system)
3. **Summon Drones** - Spawns enemy drones (tests entity spawning)
4. **Shield Phase** - Invulnerable state with particle effects

**Systems Validated:**
- Complex animation state machine
- High particle count performance
- Positional audio
- Multiple collider queries

---

### 3.7 XP Gem (Collectible)

**Name:** `XPGem`  
**Type:** Light Entity

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "XPGem" |
| `TransformComponent` | Position: dropped from enemies |
| `SpriteComponent` | Texture: `xp_gem.png`, Color: green/blue/purple by value |
| `XPGemComponent` | XPValue: 1-10, AttractionRadius: 3.0 |
| `VelocityComponent` | For attraction movement |
| `ParticleEmitterComponent` | Sparkle effect |

**Visual Description:**
- **Shape:** Multifaceted gemstone (octahedron) with rotating inner core
- **Color Scheme (by value):**
  - *Small (1 XP):* Green gem, soft glow
  - *Medium (5 XP):* Blue gem, moderate glow
  - *Large (10 XP):* Purple/gold gem, intense glow
- **Scale:**
  - Small: 16x16 pixels
  - Medium: 24x24 pixels
  - Large: 32x32 pixels
- **Animation:** Continuous rotation (3 RPM), vertical sine wave bobbing (amplitude: 0.1 units)
- **Particle Effect:** Small sparkles emit from gem (2-3 particles/sec), color matches gem type
- **Attraction Visual:** When player enters radius, gem accelerates toward player with trailing particles
- **Collection Effect:** Gem bursts into shower of matching colored particles, bright flash, sound ding

**Behaviors:**
- Float animation (sine wave)
- Attract toward player when in range
- On collect: Add XP, play sound, spawn particles

**Systems Validated:**
- `XPCollectionSystem`
- Light entity movement
- Particle emission on moving entities

---

### 3.8 Environment: Obstacle

**Name:** `Obstacle`  
**Type:** Heavy Entity

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "Obstacle" |
| `TransformComponent` | Editor-placed |
| `SpriteComponent` | Texture: varies (crate, pillar, wall) |
| `RigidbodyComponent` | BodyType: Static |
| `ColliderComponent` | Type: Box, sized to sprite |

**Visual Description:**
- **Variants:**
  - *Crate:* 64x64px wooden crate with metal reinforcement, warning labels
  - *Pillar:* 64x64px stone/metal pillar with tech details, support structure
  - *Wall:* Variable size, modular metal panels with rivets and vents
- **Color Scheme:** Industrial grays, browns, with yellow/black hazard markings
- **Details:** Scratches, dents, wear patterns suggesting realistic arena environment
- **Breakable Variant:** Crates can be destroyed - plays destruction animation, spawns debris particles
- **Lighting:** Subtle ambient occlusion shadows at base to ground sprites
- **Composition:** Arranged to create cover, funnel enemy movement, and test line-of-sight

**Purpose:** Tests static body collision, level design, bullet blocking

---

### 3.9 Environment: Trigger Zone

**Name:** `TriggerZone`  
**Type:** Heavy Entity (Sensor)

| Component | Configuration |
|-----------|--------------|
| `TagComponent` | Name: "TriggerZone" |
| `TransformComponent` | Editor-placed |
| `RigidbodyComponent` | BodyType: Static |
| `ColliderComponent` | Type: Box, IsSensor: true |
| `TriggerComponent`* | OnEnter: event, OnExit: event |

**Visual Description (Debug/Editor Only):**
- **Shape:** Rectangular outline (wireframe box) visible only with debug rendering enabled
- **Color Coding:**
  - *Green:* Scene transition portal
  - *Yellow:* Enemy spawn trigger
  - *Blue:* Audio zone boundary
  - *Purple:* Generic event trigger
- **Runtime:** Completely invisible to player during normal gameplay
- **Editor:** Semi-transparent colored overlay (30% opacity) with icon indicator at center
- **Active State:** Pulses when entity is inside (visual feedback for testing)

**Purpose:** Scene transitions, spawn triggers, audio zone changes

---

### 3.10 Particle Effect: Explosion

**Name:** `ExplosionFX`  
**Type:** Light Entity (temporary)

| Component | Configuration |
|-----------|--------------|
| `TransformComponent` | Position: death location |
| `ParticleEmitterComponent` | BurstMode: true, BurstCount: 50 |

**Visual Description:**
- **Particle Type:** Circular sprite (`particle_circle.png`) and spark sprite (`particle_spark.png`)
- **Color Gradient:** 
  - Start: Bright yellow-white core (1.0, 1.0, 0.8, 1.0)
  - Middle: Orange (1.0, 0.6, 0.2, 1.0)
  - End: Dark red smoke (0.3, 0.1, 0.1, 0.0)
- **Size Progression:** Start at 0.3 units, scale down to 0.0 over lifetime
- **Behavior:** 50 particles burst radially in all directions, arc downward due to gravity
- **Duration:** 0.8 seconds total, particles fade out in final 0.3 seconds
- **Variants:**
  - *Enemy Death:* Orange-red explosion (default)
  - *Bullet Impact:* Smaller, cyan sparks (10 particles)
  - *Boss Phase Transition:* Massive purple-blue explosion (200 particles)
- **Sound Sync:** Explosion sound plays at spawn instant
- **Screen Effect:** Optional camera shake (0.2 units, 0.15 sec duration)

**Configuration:**
```
Shape: Circle
Direction: (0, 0) (radial burst)
DirectionSpread: 360
Speed: 8.0
SpeedVariance: 4.0
Lifetime: 0.8
StartColor: (1.0, 0.8, 0.2, 1.0) // Orange-yellow
FadeOut: true
ScaleOverTime: true
EndScale: 0.0
Gravity: (0, -2)
```

**Systems Validated:**
- `ParticleEmitterSystem` burst mode
- Particle color gradients
- Particle scale animation

---

## 4. Custom Components (Showcase-Specific)

These components are **defined for this showcase only** and demonstrate how users would extend the engine.

```cpp
// PlayerStatsComponent.h
struct PlayerStatsComponent
{
    float Health = 100.0f;
    float MaxHealth = 100.0f;
    int XP = 0;
    int Level = 1;
    int XPToNextLevel = 100;
    
    float DashCooldown = 2.0f;
    float DashCooldownTimer = 0.0f;
    bool CanDash = true;
};

// HealthComponent.h
struct HealthComponent
{
    float Health = 100.0f;
    float MaxHealth = 100.0f;
    bool IsDead = false;
    float InvincibilityTimer = 0.0f;  // For i-frames after hit
};

// EnemyAIComponent.h
enum class AIBehavior { Idle, SeekPlayer, Flee, Patrol };
struct EnemyAIComponent
{
    AIBehavior Behavior = AIBehavior::SeekPlayer;
    float Speed = 3.0f;
    float DetectionRange = 15.0f;
    Entity Target;  // Usually the player
};

// TurretAIComponent.h
struct TurretAIComponent
{
    float FireRate = 1.0f;
    float FireTimer = 0.0f;
    float Range = 10.0f;
    float RotationSpeed = 180.0f;  // degrees/sec
};

// ChargerAIComponent.h
struct ChargerAIComponent
{
    float ChargeSpeed = 15.0f;
    float WindupTime = 0.5f;
    float WindupTimer = 0.0f;
    float ChargeDuration = 1.0f;
    glm::vec2 ChargeDirection;
    bool IsCharging = false;
    bool IsWindingUp = false;
};

// BossAIComponent.h
enum class BossPhase { Phase1, Phase2, Defeated };
enum class BossAttack { BulletSpray, LaserSweep, SummonDrones, Shield };
struct BossAIComponent
{
    BossPhase Phase = BossPhase::Phase1;
    BossAttack CurrentAttack = BossAttack::BulletSpray;
    float AttackTimer = 0.0f;
    float AttackCooldown = 3.0f;
    int AttackIndex = 0;
    std::vector<BossAttack> AttackPattern;
};

// TriggerComponent.h
struct TriggerComponent
{
    std::string OnEnterEvent;
    std::string OnExitEvent;
    bool TriggerOnce = false;
    bool HasTriggered = false;
    std::vector<Entity> EntitiesInside;
};
```

---

## 5. Test Chambers (Scene Structure)

### Chamber 0: Main Hub

**Purpose:** Scene management, UI systems, save/load  
**Layout:** Central room with portals to each test chamber

**Features:**
- Portal entities (TriggerZones) to each chamber
- Statistics display (ImGui panel)
- Settings menu (audio volume, graphics)
- Scene serialization test (save/load game state)

---

### Chamber 1: Movement & Physics Playground

**Purpose:** Validate Box2D physics, player movement, collision

**Entities:**
- 1x Player
- 20x Obstacles (various shapes: boxes, circles)
- 5x Moving platforms (Kinematic bodies)
- 4x Bounce pads (high restitution)
- 1x Gravity zone (modifies GravityScale)

**Validation Checklist:**
- [ ] Player moves smoothly with WASD
- [ ] Collision with static bodies stops movement
- [ ] Kinematic platforms push player
- [ ] Bounce pads reflect player velocity
- [ ] Physics debug rendering visible (toggle)

---

### Chamber 2: Shooting Range

**Purpose:** Bullet system, object pooling, damage

**Entities:**
- 1x Player
- 10x Target dummies (HealthComponent, no AI)
- 5x Breakable crates (destroy on hit)
- 3x Shield barriers (block bullets, not player)

**Validation Checklist:**
- [ ] Bullets spawn from pool (no allocation lag)
- [ ] Bullets despawn on hit or timeout
- [ ] Damage numbers display (particle text)
- [ ] Pool statistics visible in debug panel
- [ ] Bullet count can scale to 500+ without frame drop

---

### Chamber 3: Enemy Gauntlet

**Purpose:** AI systems, enemy variety, combat loop

**Waves:**
1. Wave 1: 10x Drones
2. Wave 2: 5x Turrets (pre-placed)
3. Wave 3: 8x Chargers
4. Wave 4: Mix of all enemies (20 total)

**Validation Checklist:**
- [ ] Drones path toward player
- [ ] Turrets rotate and fire at player
- [ ] Chargers wind up, then dash
- [ ] Death animations play fully
- [ ] XP gems drop and attract to player

---

### Chamber 4: Particle Stress Test

**Purpose:** Particle system performance, visual effects

**Scenarios:**
- Ambient emitters (fire, smoke, sparkles)
- On-demand explosions (button trigger)
- Particle count slider (100 → 10,000)
- Color gradient showcase

**Entities:**
- 4x Fire emitters (continuous)
- 4x Smoke emitters (continuous)
- 1x Explosion spawner (on click)
- 1x Rain emitter (high count, gravity)

**Validation Checklist:**
- [ ] Particles spawn at correct rate
- [ ] Color gradients interpolate correctly
- [ ] FadeOut and ScaleOverTime work
- [ ] Performance maintains 60 FPS at 5,000 particles
- [ ] Particles recycle correctly (no memory growth)

---

### Chamber 5: Animation Showcase

**Purpose:** Sprite animation, animation events

**Entities:**
- Character with walk cycle (4 directions)
- Enemy with attack animation
- Chest with open/close animation
- Flag with wind animation (looping)

**Validation Checklist:**
- [ ] Animations play at correct speed
- [ ] PlaybackSpeed multiplier works
- [ ] Animation events fire at correct frames
- [ ] OnAnimationComplete callback triggers
- [ ] Sprite flipping (FlipX/FlipY) works

---

### Chamber 6: Audio Test Room

**Purpose:** Audio system, 3D positional audio, music

**Layout:** Open room with audio sources at corners

**Entities:**
- 4x Positional audio sources (ambient sounds)
- 1x Music source (background loop)
- 1x AudioListener (follows player)
- Interactive speakers (click to play SFX)

**Validation Checklist:**
- [ ] 3D audio pans as player moves
- [ ] Volume falloff with distance
- [ ] Music loops seamlessly
- [ ] SFX play without overlap issues
- [ ] Master volume control works

---

### Chamber 7: Boss Arena

**Purpose:** Complex entity, all systems combined

**Entities:**
- 1x Player
- 1x BossSentinel
- Dynamic obstacles (spawned during fight)
- Particle effects for attacks
- Positional audio for boss sounds

**Boss Phases:**
1. **Phase 1 (100%-50% HP):** Bullet spray, drone summons
2. **Phase 2 (50%-0% HP):** Adds laser sweep, faster attacks, more particles

**Validation Checklist:**
- [ ] Boss AI state machine works
- [ ] All attack patterns execute correctly
- [ ] Phase transition triggers at correct HP
- [ ] Summoned entities spawn from pool
- [ ] Victory condition triggers scene transition

---

### Chamber 8: Performance Stress Test

**Purpose:** Find performance limits, identify bottlenecks

**Configurable Parameters:**
- Enemy count: 10 → 1000
- Bullet count: 100 → 5000
- Particle count: 1000 → 50,000
- Physics bodies: 10 → 500

**Display:**
- FPS counter
- Entity count
- Physics body count
- Draw call count
- Memory usage (approximate)

**Validation Checklist:**
- [ ] 60 FPS with 100 enemies, 500 bullets
- [ ] Graceful degradation under stress
- [ ] Object pools prevent allocation spikes
- [ ] Spatial hash grid scales with entity count

---

## 6. Debug Overlay System

### ImGui Panels

1. **Entity Inspector**
   - Select entity to view all components
   - Edit component values in real-time
   - Add/remove components

2. **System Profiler**
   - Per-system update time
   - Frame time breakdown
   - Rendering statistics

3. **Object Pool Monitor**
   - Pool sizes (available/active/total)
   - Acquire/Release rate
   - Peak usage

4. **Physics Debug**
   - Toggle collision shape rendering
   - Toggle velocity vectors
   - Box2D world statistics

5. **Audio Debug**
   - Active audio sources
   - Listener position
   - Volume levels

---

## 7. Input Mapping

| Action | Key/Button | System Validated |
|--------|-----------|------------------|
| Move Up | W | Input::IsKeyPressed |
| Move Down | S | Input::IsKeyPressed |
| Move Left | A | Input::IsKeyPressed |
| Move Right | D | Input::IsKeyPressed |
| Fire Primary | Left Mouse | Input::IsMouseButtonPressed |
| Fire Secondary | Right Mouse | Input::IsMouseButtonPressed |
| Dash | Space | Input::IsKeyPressed + Cooldown |
| Pause | Escape | Event System, Scene Stack |
| Debug Overlay | F1 | Layer Toggle |
| Physics Debug | F2 | RenderCommand overlay |
| Spawn Enemy | F5 | Entity spawning test |
| Clear Enemies | F6 | Entity destruction test |
| Reload Scene | F9 | Scene serialization |

---

## 8. Asset Requirements

### Textures (Sandbox/assets/textures/)

| Asset | Size | Purpose |
|-------|------|---------|
| `player_ship.png` | 64x64 | Player sprite |
| `player_ship_sheet.png` | 256x64 | Player animation (4 frames) |
| `bullet.png` | 16x32 | Player bullet |
| `enemy_bullet.png` | 16x32 | Enemy bullet (different color) |
| `drone.png` | 48x48 | Drone enemy |
| `turret_base.png` | 64x64 | Turret enemy |
| `turret_barrel.png` | 32x48 | Turret rotating part |
| `charger.png` | 48x64 | Charger enemy |
| `sentinel_boss.png` | 192x192 | Boss sprite |
| `xp_gem_small.png` | 16x16 | 1 XP gem |
| `xp_gem_medium.png` | 24x24 | 5 XP gem |
| `xp_gem_large.png` | 32x32 | 10 XP gem |
| `crate.png` | 64x64 | Breakable obstacle |
| `pillar.png` | 64x64 | Static obstacle |
| `particle_circle.png` | 16x16 | Particle texture |
| `particle_spark.png` | 16x16 | Spark particle |

### Audio (Sandbox/assets/audio/)

| Asset | Type | Purpose |
|-------|------|---------|
| `sfx/shoot.wav` | SFX | Player weapon fire |
| `sfx/enemy_shoot.wav` | SFX | Enemy weapon fire |
| `sfx/hit.wav` | SFX | Bullet impact |
| `sfx/explosion.wav` | SFX | Enemy death |
| `sfx/pickup.wav` | SFX | XP gem collection |
| `sfx/dash.wav` | SFX | Player dash ability |
| `sfx/boss_attack.wav` | SFX | Boss attack cue |
| `music/chamber_ambient.wav` | Music | Background loop |
| `music/boss_battle.wav` | Music | Boss fight music |

---

## 9. Success Criteria

### Performance Targets

| Metric | Target | Acceptable |
|--------|--------|------------|
| FPS (normal gameplay) | 60 | 45+ |
| FPS (stress test: 200 enemies) | 60 | 30+ |
| Bullet pool utilization | <80% | <95% |
| Memory growth per minute | 0 MB | <10 MB |
| Scene load time | <500ms | <2s |

### System Validation Checklist

- [ ] **ECS:** Entities create, query, and destroy correctly
- [ ] **Physics:** Box2D bodies move, collide, and respond
- [ ] **Rendering:** All sprites render with correct transforms
- [ ] **Animation:** Frame sequences play, events fire
- [ ] **Particles:** Emitters spawn, particles update, fade, and die
- [ ] **Audio:** Sounds play, pan correctly, loop properly
- [ ] **Pooling:** Objects recycle without allocation
- [ ] **Scenes:** Load, serialize, and transition work
- [ ] **Camera:** Follows player, zoom works, shake effects work
- [ ] **Input:** All mapped actions respond correctly
- [ ] **Collision:** Bullets hit targets, sensors trigger events

---

## 10. Implementation Priority

### Phase 1: Core Loop (Week 1)
1. Player entity with movement
2. Basic bullet firing (no pooling yet)
3. Single drone enemy
4. Chamber 1 & 2 complete

### Phase 2: Combat Systems (Week 2)
1. Object pooling for bullets
2. Health/damage system
3. All enemy types
4. XP gem system
5. Chamber 3 complete

### Phase 3: Polish & Effects (Week 3)
1. Particle system integration
2. Animation polish
3. Audio integration
4. Chamber 4, 5, 6 complete

### Phase 4: Boss & Stress Testing (Week 4)
1. Boss implementation
2. Performance profiling
3. Debug overlay completion
4. Chamber 7, 8 complete

---

## 11. Appendix: Entity Spawn Recipes

### Quick Reference for Creating Entities

```cpp
// Player
Entity player = scene.CreateEntity("Player");
player.AddComponent<SpriteComponent>(playerTexture);
player.AddComponent<AnimationComponent>();
player.AddComponent<RigidbodyComponent>(b2_dynamicBody);
player.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.4f));
player.AddComponent<PlayerStatsComponent>();
player.AddComponent<AudioSourceComponent>();

// Bullet (from pool)
Entity bullet = bulletPool.Acquire();
bullet.GetComponent<TransformComponent>().Position = spawnPos;
bullet.GetComponent<TransformComponent>().Rotation = aimAngle;
bullet.GetComponent<VelocityComponent>().Velocity = direction * speed;
bullet.GetComponent<BulletComponent>().Owner = player;
bullet.GetComponent<BulletComponent>().TimeAlive = 0.0f;

// Drone Enemy
Entity drone = scene.CreateEntity("EnemyDrone");
drone.AddComponent<SpriteComponent>(droneTexture);
drone.GetComponent<SpriteComponent>().Color = {1.0f, 0.5f, 0.5f, 1.0f};
drone.AddComponent<RigidbodyComponent>(b2_dynamicBody);
drone.AddComponent<ColliderComponent>(ColliderComponent::Circle(0.5f));
drone.AddComponent<HealthComponent>(30.0f);
drone.AddComponent<EnemyAIComponent>();

// Particle Explosion
Entity fx = scene.CreateEntity("ExplosionFX");
fx.GetComponent<TransformComponent>().Position = deathPos;
auto& emitter = fx.AddComponent<ParticleEmitterComponent>();
emitter.BurstMode = true;
emitter.BurstCount = 50;
emitter.Shape = EmissionShape::Circle;
emitter.Speed = 8.0f;
emitter.Lifetime = 0.8f;
emitter.FadeOut = true;
```

---

*Document Version: 1.0*  
*Last Updated: December 11, 2025*  
*Author: Lead Engine Developer / Technical Artist*
