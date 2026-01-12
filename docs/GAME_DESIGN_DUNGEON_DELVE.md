# Dungeon Delve - Mini Game Design Document

**Target Engine:** Pillar Engine  
**Genre:** Top-Down Action Survival  
**Development Time:** ~4-8 hours  
**Last Updated:** January 12, 2026

---

## Overview

**Dungeon Delve** is a simple top-down survival game designed to showcase ALL features of the Pillar Engine. The player controls a hero navigating a dark dungeon, collecting gems, defeating enemies, and surviving as long as possible.

### Design Goals

1. **Demonstrate ALL engine features** in a cohesive gameplay experience
2. **Use the PillarEditor** for level design and entity placement
3. **Keep scope minimal** - achievable in a few hours
4. **Create reusable templates** for enemies, pickups, and effects

---

## Engine Features Utilized

| Feature | How It's Used |
|---------|---------------|
| **ECS (EnTT)** | All game objects are entities with components |
| **Physics (Box2D)** | Player/enemy collision, wall boundaries |
| **Light Physics (Velocity)** | Projectiles, gems, particles |
| **Renderer2D** | All sprite rendering |
| **TextureAtlas** | Character sprites, environment tiles |
| **AnimationSystem** | Player walk/idle/attack, enemy animations |
| **ParticleEmitterSystem** | Death explosions, magic effects, torches |
| **Audio System** | Background music, SFX (footsteps, attacks, pickups) |
| **Lighting2D** | Torch lights, player glow, ambient darkness |
| **Scene Serialization** | Save/load levels from JSON |
| **OrthographicCamera** | Camera follows player |
| **Input System** | WASD movement, mouse aim, click to attack |
| **ImGui** | Debug overlay, stats display |
| **PillarEditor** | Level design, entity templates |

---

## Game Mechanics

### Player

- **Movement:** WASD keys for 8-directional movement
- **Aiming:** Mouse cursor determines facing direction
- **Attack:** Left-click fires a magic projectile
- **Health:** 3 hearts (displayed on HUD)
- **Invincibility:** Brief invincibility after taking damage (flashing effect)

**Components:**
- `TransformComponent` - Position, rotation, scale
- `TagComponent` - "Player"
- `SpriteComponent` - Player sprite (32x32)
- `AnimationComponent` - walk_up, walk_down, walk_left, walk_right, idle
- `RigidbodyComponent` - Dynamic body for physics collision
- `ColliderComponent` - Circle collider
- `Light2DComponent` - Subtle player glow (helps visibility)
- `AudioListenerComponent` - 3D audio listener follows player

### Enemies (Slimes)

- **Behavior:** Move toward player slowly
- **Damage:** Contact with player deals 1 heart damage
- **Health:** 1 hit to kill
- **Spawn:** Spawn at edges, rate increases over time

**Components:**
- `TransformComponent`
- `TagComponent` - "Enemy"
- `SpriteComponent` - Slime sprite
- `AnimationComponent` - slime_bounce (2-frame)
- `RigidbodyComponent` - Dynamic body
- `ColliderComponent` - Circle collider
- `VelocityComponent` - For simple movement toward player (light entity alternative)

### Projectiles (Magic Bolt)

- **Speed:** Fast travel in aimed direction
- **Damage:** Destroys enemy on contact
- **Lifetime:** 3 seconds or until collision
- **Effect:** Particle trail while flying

**Components:**
- `TransformComponent`
- `TagComponent` - "Projectile"
- `SpriteComponent` - Magic bolt sprite
- `VelocityComponent` - Light entity physics (no Box2D overhead)
- `BulletComponent` - Collision detection via BulletCollisionSystem

### XP Gems

- **Behavior:** Dropped by enemies, float toward player when nearby
- **Value:** +10 score per gem
- **Visual:** Glowing gem with particle sparkle

**Components:**
- `TransformComponent`
- `TagComponent` - "XPGem"
- `SpriteComponent` - Gem sprite
- `VelocityComponent` - Light entity
- `XPGemComponent` - Magnetic attraction behavior
- `Light2DComponent` - Subtle glow

### Environment

#### Walls
- Static Box2D bodies forming dungeon boundaries
- No sprite (or dark gray placeholder)

#### Torches
- Placed around the level
- Emit point lights (warm orange)
- Have flame particle emitter (fire preset)
- Play crackling audio (looped, 3D positioned)

**Components:**
- `TransformComponent`
- `SpriteComponent` - Torch sprite
- `Light2DComponent` - Point light, warm color, radius 5-8
- `ParticleEmitterComponent` - Fire particles
- `AudioSourceComponent` - Ambient fire crackle

#### Ground
- Dark stone floor texture (tiled)
- Rendered behind everything

---

## Visual Style

### Color Palette

| Element | Color (RGB) |
|---------|-------------|
| Ambient Light | (0.05, 0.05, 0.1) - Very dark blue |
| Torch Light | (1.0, 0.6, 0.2) - Warm orange |
| Player Glow | (0.4, 0.6, 1.0) - Cool blue |
| Gem Glow | (0.2, 1.0, 0.4) - Green |
| Magic Bolt | (0.8, 0.3, 1.0) - Purple |

### Sprites Needed

| Sprite | Size | Animation Frames |
|--------|------|------------------|
| Player | 32x32 | 4 walk frames x 4 directions + 1 idle |
| Slime | 32x32 | 2 bounce frames |
| Magic Bolt | 16x16 | 2 glow frames |
| Gem | 16x16 | 4 sparkle frames |
| Torch | 32x64 | 3 flame frames |
| Floor Tile | 32x32 | 1 (static) |
| Wall | 32x32 | 1 (static) |

For prototyping, we'll use **colored quads** before final sprites:
- Player: Blue square
- Enemy: Red square  
- Gem: Green square
- Projectile: Purple square
- Torch: Orange square
- Wall: Dark gray square

---

## Audio Design

| Sound | Type | Usage |
|-------|------|-------|
| `dungeon_ambience.wav` | Music | Background loop, low volume |
| `footstep.wav` | SFX | Player movement (or skip for simplicity) |
| `magic_shoot.wav` | SFX | Projectile fired |
| `enemy_hit.wav` | SFX | Enemy destroyed |
| `player_hurt.wav` | SFX | Player takes damage |
| `gem_pickup.wav` | SFX | XP gem collected |
| `torch_crackle.wav` | SFX | 3D positioned, looped on torches |

For prototyping, use placeholder/existing audio from `Sandbox/assets/audio/`.

---

## Level Design (Editor Workflow)

### Level Layout

```
+--------------------------------------------------+
|   T                                          T   |
|                                                  |
|       +-------+              +-------+           |
|       | WALL  |              | WALL  |           |
|       +-------+              +-------+           |
|                                                  |
|   T         [PLAYER START]                   T   |
|                                                  |
|       +-------+              +-------+           |
|       | WALL  |              | WALL  |           |
|       +-------+              +-------+           |
|                                                  |
|   T                                          T   |
+--------------------------------------------------+

T = Torch (with light + particles)
```

### Editor Workflow

1. **Create Entity Templates:**
   - Player template (all components pre-configured)
   - Enemy template (slime with animation)
   - Torch template (light + particles + audio)
   - Gem template (light + sparkle)
   - Wall template (static rigidbody)

2. **Design Level:**
   - Place walls to form dungeon shape
   - Position torches for atmospheric lighting
   - Set player spawn position
   - Define enemy spawn zones (marked entities)

3. **Save Scene:**
   - Save as `dungeon_level_1.scene.json`

---

## Game Systems Implementation

### 1. GameLayer (Main Game Logic)

```
Dungeon/
├── src/
│   ├── DungeonApp.cpp        # Entry point
│   ├── GameLayer.h/cpp       # Main game layer
│   ├── PlayerController.h    # Player input/movement
│   ├── EnemySpawner.h        # Wave spawning logic
│   ├── GameHUD.h             # ImGui HUD overlay
│   └── GameState.h           # Score, health, wave tracking
└── assets/
    ├── textures/
    │   └── dungeon_sprites.png  # Sprite atlas
    ├── audio/
    │   ├── dungeon_music.wav
    │   └── sfx/
    └── scenes/
        └── dungeon_level_1.scene.json
```

### 2. System Execution Order

1. **Input Processing** (handled in GameLayer::OnUpdate)
2. **Player Movement** (apply velocity based on input)
3. **Enemy AI** (calculate direction to player)
4. **VelocityIntegrationSystem** (move light entities)
5. **PhysicsSystem** (Box2D step for heavy entities)
6. **PhysicsSyncSystem** (sync transforms from Box2D)
7. **BulletCollisionSystem** (projectile hits)
8. **XPCollectionSystem** (gem magnet behavior)
9. **ParticleEmitterSystem** (spawn particles)
10. **ParticleSystem** (update particle lifetimes)
11. **AnimationSystem** (advance animation frames)
12. **Lighting2D + SpriteRenderSystem** (render everything)

### 3. Key Code Patterns

**Spawning an Enemy:**
```cpp
Entity SpawnEnemy(const glm::vec2& position) {
    auto entity = m_Scene->CreateEntity("Enemy");
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.Position = glm::vec3(position, 0.0f);
    
    entity.AddComponent<SpriteComponent>(glm::vec4(1.0f, 0.2f, 0.2f, 1.0f)); // Red
    entity.AddComponent<VelocityComponent>();  // Light entity
    entity.AddComponent<ColliderComponent>(/* circle, radius 0.4 */);
    // No RigidbodyComponent = light entity pattern
    
    return entity;
}
```

**Firing a Projectile:**
```cpp
void FireProjectile(const glm::vec2& origin, const glm::vec2& direction) {
    auto bullet = m_Scene->CreateEntity("Projectile");
    auto& transform = bullet.GetComponent<TransformComponent>();
    transform.Position = glm::vec3(origin, 0.0f);
    
    auto& velocity = bullet.AddComponent<VelocityComponent>();
    velocity.Linear = glm::normalize(direction) * 15.0f; // Fast!
    
    auto& bulletComp = bullet.AddComponent<BulletComponent>();
    bulletComp.Damage = 1.0f;
    bulletComp.Lifetime = 3.0f;
    
    bullet.AddComponent<SpriteComponent>(glm::vec4(0.8f, 0.3f, 1.0f, 1.0f)); // Purple
    
    // Play shoot sound
    m_ShootSound->Play();
}
```

---

## Development Phases

### Phase 1: Core Gameplay (1-2 hours)
- [ ] Create GameLayer with camera controller
- [ ] Player entity with WASD movement
- [ ] Basic projectile firing
- [ ] Simple enemy spawning
- [ ] Collision detection (bullet kills enemy)

### Phase 2: Visual Polish (1-2 hours)
- [ ] Add Lighting2D with ambient darkness
- [ ] Create torch entities with lights
- [ ] Add particle emitters to torches (fire)
- [ ] Death particles when enemy dies
- [ ] Gem sparkle particles

### Phase 3: Audio Integration (30 min - 1 hour)
- [ ] Background music (looped)
- [ ] Shoot SFX
- [ ] Enemy death SFX
- [ ] Gem pickup SFX
- [ ] 3D positioned torch ambience

### Phase 4: Game Polish (1-2 hours)
- [ ] Score/wave HUD
- [ ] Player health display
- [ ] Game over screen
- [ ] Wave progression (more enemies over time)
- [ ] XP gem drop on enemy death
- [ ] Magnetic gem collection

### Phase 5: Editor Integration (1 hour)
- [ ] Create entity templates in editor
- [ ] Design level layout visually
- [ ] Save scene file
- [ ] Load scene in game

---

## Known Friction Points

These will be documented in `DEVELOPER_FRICTION.md` as they're encountered:

1. **Include paths for components** - Too deep, need umbrella header
2. **Animation loading from JSON** - Need programmatic API
3. **RigidbodyComponent non-copyable** - Can't duplicate physics entities easily
4. **No clear light/heavy entity documentation** - Easy to mix patterns

---

## Success Criteria

The game is complete when:

1. ✅ Player can move around dungeon with WASD
2. ✅ Player can shoot projectiles with mouse
3. ✅ Enemies spawn and chase player
4. ✅ Projectiles destroy enemies
5. ✅ Enemies drop XP gems
6. ✅ Lighting creates atmospheric darkness
7. ✅ Torches emit light and fire particles
8. ✅ Audio plays (music + SFX)
9. ✅ Score is tracked and displayed
10. ✅ Level is designed in PillarEditor

---

## Next Steps

1. **Create project folder structure** in `Sandbox/` or new `DungeonDelve/` folder
2. **Implement GameLayer** with basic player movement
3. **Iterate** through development phases
4. **Document friction** as it's encountered
5. **Playtest** and polish

---

*This document serves as both design spec and checklist. Update checkboxes as features are completed.*
