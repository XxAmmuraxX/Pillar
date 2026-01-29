# Top-Down Shooter Polish Plan & Codebase Reference

**Version 2.0 | From Framework to Complete Game**
**Created**: 2026-01-29
**Last Updated**: 2026-01-29

---

## CODEBASE ARCHITECTURE REFERENCE

### Project Structure

```
Sandbox/src/TopDownShooter/
├── SwarmSlayerLayer.h          # Main game layer (state machine, system orchestration, rendering)
├── GameLayer.h                 # Simple test game layer (Phase 4, unused by default)
├── Components/
│   ├── BossComponent.h         # Boss type/phase definitions, stat factories
│   ├── BulletTrailComponent.h  # Visual trail data for bullets
│   ├── EffectComponents.h      # Temporary visual effect markers
│   ├── EnemyComponent.h        # EnemyType enum, EnemyState, per-enemy stats
│   ├── HazardComponent.h       # Environmental hazard data with pulsing
│   ├── PlayerTagComponent.h    # Marks player entity, stores dash/move speed
│   ├── PowerUpComponent.h      # PowerUpType enum, PlayerBuffsComponent, ActivePowerUpEffect
│   ├── WeaponComponent.h       # Weapon stats (damage, fire rate, speed, spread, bullets)
│   └── XPOrbComponent.h        # XP value, magnet/pickup radius, bobbing animation
├── Systems/
│   ├── BossSystem.h            # Boss AI per type, phase transitions, projectile spawning
│   ├── BulletLifetimeSystem.h  # Removes expired bullets
│   ├── BulletTrailSystem.h     # Renders particle trails behind bullets
│   ├── DamageSystem.h          # Hit detection callbacks, death processing, knockback, power-up drops
│   ├── EffectSystems.h         # FlashSystem + TemporaryCleanupSystem
│   ├── EnemyAISystem.h         # Per-type enemy AI (chase, shoot, swarm), player detection
│   ├── HazardSystem.h          # Environmental hazard ticking, explosion callbacks
│   ├── PlayerMovementSystem.h  # WASD + dash with cooldown, sprite flipping
│   ├── PowerUpSystem.h         # Bobbing, collection, buff application, magnet attraction
│   ├── WeaponSystem.h          # Mouse aim, bullet spawning, fire rate management
│   └── XPSystem.h              # XP orb attraction, collection, leveling callbacks
├── Utilities/
│   ├── AudioManager.h          # Singleton. Pre-loads 7 SFX + 1 music. 3D positional audio, pitch variation, max 16 concurrent
│   ├── CameraShake.h           # ShakeSmall/Medium/Large with trauma system
│   ├── CollisionCategories.h   # Box2D category bitmasks (Player, Enemy, Bullet, Wall, Sensor)
│   ├── EffectFactory.h         # Static helpers: SpawnHitParticles, SpawnDeathParticles, SpawnBossDeathParticles, SpawnPlayerDamageEffect
│   ├── EntityFactory.h         # Static creators: CreatePlayer, CreateEnemy(waveScaled), CreateBoss, CreateWall, CreateBullet, CreateXPOrb, CreateRandomPowerUp, SpawnRandomHazards
│   ├── GameUtils.h             # RandomFloat, RandomInCircle, ScreenToWorld, AngleToDirection
│   ├── ParticleManager.h       # Native particle system (up to 2000 particles), emit/burst API
│   └── WaveManager.h           # Wave state machine (WaveComplete->Spawning->InProgress), scaling config, boss every 5 waves
├── Core/
│   └── GameState.h             # Singleton. GameStats, PlayerStats, Perks (12 types), Weapons (5 types), HighScores, Combo system
└── UI/
    └── MenuRenderer.h          # All ImGui UI: MainMenu, Pause, PerkSelection, GameOver, full in-game HUD
```

### Engine Integration (Pillar Engine)

The game uses these Pillar engine systems:
- **PhysicsSystem** (Box2D, zero gravity for top-down)
- **PhysicsSyncSystem** (syncs Box2D bodies to ECS transforms)
- **VelocityIntegrationSystem** (lightweight movement for bullets and swarm enemies)
- **BulletCollisionSystem** (raycast-based hit detection with callbacks)
- **AnimationSystem** (sprite frame animation from JSON clip files)
- **Renderer2D** (batched sprite rendering with layers)
- **OrthographicCameraController** (zoom level 8.0)
- **AudioEngine** (OpenAL-based 3D audio)

### Key Patterns

1. **ECS**: Uses EnTT registry. Components are plain structs. Systems inherit `Pillar::System`.
2. **Callbacks**: Systems communicate via `std::function` callbacks set during initialization.
3. **Singleton State**: `GameState::Instance()` holds all progression, stats, perks, weapons, high scores.
4. **Entity Factory**: Static methods in `EntityFactory` create all game entities with proper components.
5. **Scene Ownership**: `SwarmSlayerLayer` owns the scene via `std::unique_ptr<Pillar::Scene>`. Systems are raw pointers deleted in reverse order on shutdown.

---

## GAME STATE MACHINE

```
MainMenu -> [PLAY] -> Playing <-> [ESC] -> Paused
                        |                    |
                [Wave Complete] -> PerkSelection -> Playing
                        |
                   [Player Dies] -> GameOver -> [PLAY AGAIN] -> Playing
                                             -> [MAIN MENU] -> MainMenu
```

Controlled by `GameStateType` enum in `GameState.h`.

---

## ENTITY TYPES

### Player
- **Components**: TransformComponent, SpriteComponent, AnimationComponent, RigidbodyComponent (dynamic), ColliderComponent (circle 0.35), HealthComponent (100 HP), PlayerTagComponent (moveSpeed=7, dashCooldown=1.5s), WeaponComponent, PlayerBuffsComponent
- **Texture**: `red_mage/rotations/south.png`, animated with `red_mage_idle_south`
- **Collision**: Category=Player, Mask=Enemy|Wall|Bullet
- **Movement**: Box2D forces via RigidbodyComponent, linear damping 5.0

### Enemies (3 types + wave scaling)
Created via `EntityFactory::CreateEnemy(scene, pos, type, waveNumber)`:

| Type | Health | Speed | Damage | XP | Physics | Animation |
|------|--------|-------|--------|----|---------|-----------|
| Chaser | 30 * healthScale | 4.0 * speedScale | 15 | 10 | Box2D (dynamic) | hoodzy_chaser_enemy_animation |
| Shooter | 50 * healthScale | 2.0 * speedScale | 8 (projectile) | 25 | Box2D (dynamic) | evil_archer_run_south |
| Swarm | 10 * healthScale | 6.0 * speedScale | 5 | 5 | VelocityComponent | goblin_with_sword_run_south |

- `healthScale = 1.0 + (wave-1) * 0.05` (+5% per wave)
- `speedScale = 1.0 + (wave-1) * 0.02` (+2% per wave)
- Collision: Category=Enemy, Mask=Player|Wall|Bullet

### Bosses (3 types, every 5 waves)
Created via `EntityFactory::CreateBoss(scene, pos, type, wave)`:

| Type | Base HP | Speed | Attack | Special |
|------|---------|-------|--------|---------|
| Behemoth | 300+wave*50 | 2.5 | 25+wave*2 melee | None (pure melee) |
| Swarm Queen | 250+wave*40 | 2.0 | 15+wave*1.5 | Spawns 2-4 Swarm minions every 4s |
| Devastator | 350+wave*45 | 1.5 | 35+wave*2.5 projectile | 1/3/5 spread projectiles by phase |

- **Phases**: Phase1 (full HP), Phase2 (<50% HP, +25% speed, -25% cooldown), Phase3 (<25% HP, +30% speed, -50% cooldown)
- Boss types rotate: wave 5=Behemoth, 10=Swarm Queen, 15=Devastator, 20=Behemoth...
- Texture: `large_behemoth`, `goblin_queen`, `monster_with_bow` (south rotations)

### Bullets (Player & Enemy)
- Player bullets: Created by `WeaponSystem`, use `BulletComponent` with `VelocityComponent`
- Enemy bullets: Created by `EnemyAISystem::FireProjectile()` (Shooter type) or `BossSystem::SpawnBossProjectile()`
- Boss projectiles: Larger (0.5x0.3), dark magenta, speed=10, lifetime=6s
- Hit detection: `BulletCollisionSystem` raycasts, triggers `DamageSystem::OnBulletHit`

### Power-Ups (6 types)
Created via `EntityFactory::CreateRandomPowerUp(scene, pos)`:

| Type | Effect | Duration | Drop Chance |
|------|--------|----------|-------------|
| Health | +25 HP instant | Instant | 30% per enemy kill |
| SpeedBoost | 1.5x move speed | 5s | " |
| FireRateUp | 1.5x fire rate | 5s | " |
| DamageUp | 1.5x damage | 5s | " |
| Shield | Invulnerability | 5s | " |
| Magnet | 5-unit attraction | 5s | " |

- Collection radius: 0.8 units. Bobbing sine animation. Pulsing alpha.
- Active effects tracked in `PlayerBuffsComponent::ActiveEffects` (vector of `ActivePowerUpEffect`).

### XP Orbs
- Spawned on enemy death (10 XP base, scaled by Lucky Drops perk)
- Boss death spawns 5 orbs splitting the reward
- Lifetime: 30s. Magnet radius: 5 units. Move speed when attracted: 8 units/s
- Size/color varies by value: Green (small), Cyan (medium), Gold (large)

### Environmental Hazards
- 8 randomly spawned per arena, types: Spikes (periodic damage) and ExplosiveBarrel (one-time explosion)
- Pulsing visual effect via `HazardComponent::GetPulseFactor()`

---

## SCORE & PROGRESSION SYSTEM

### Score Events
| Action | Score | Multiplier |
|--------|-------|------------|
| Kill Chaser | 100 | 1 + wave * 0.1 |
| Kill Shooter | 150 | 1 + wave * 0.1 |
| Kill Swarm | 50 | 1 + wave * 0.1 |
| Kill Boss | ScoreReward (500-800+) | - |
| Combo bonus | +50 per combo count | Stacks |
| Wave completion | 500 * wave | - |
| No-damage wave | +1000 * wave | - |

### Combo System
- Kills within 2 seconds chain a combo (ComboWindow = 2.0f)
- `GameStats::ComboCount` increments per kill, resets to 0 when `ComboTimer` expires
- Each combo kill adds +50 * (ComboCount - 1) bonus
- HUD shows "COMBO x3 +150" notification

### XP & Leveling
- Base 100 XP to level, scales by 1.2x per level: `100 * pow(1.2, level-1)`
- Level up triggers: perk selection screen, camera shake, sound

### 12 Perks (chosen every 2 waves or after boss)
| Perk | Max Level | Effect Per Level |
|------|-----------|-----------------|
| DamageUp | 5 | +25% damage multiplier |
| FireRateUp | 5 | +20% fire rate multiplier |
| BulletSpeedUp | 3 | +20% bullet speed |
| PierceShot | 1 | Bullets pierce 1 extra enemy |
| ExplosiveRounds | 1 | Bullets explode on hit (AOE) |
| MaxHealthUp | 5 | +25 max HP |
| Regeneration | 3 | +1 HP/s regen |
| DamageReduction | 3 | +10% damage reduction |
| DodgeChance | 3 | +10% dodge chance |
| MoveSpeedUp | 3 | +10% move speed |
| XPMagnet | 3 | +30% magnet radius |
| LuckyDrops | 3 | +15% drop rate bonus |

### 5 Weapons (hotkeys 1-5)
| Weapon | Damage | FireRate | Speed | Spread | Bullets | Unlock |
|--------|--------|----------|-------|--------|---------|--------|
| Pistol | 10 | 5 | 15 | 2 | 1 | Default |
| Shotgun | 6 | 2 | 12 | 15 | 5 | Level 3 |
| SMG | 5 | 12 | 18 | 5 | 1 | Level 5 |
| Rifle | 25 | 1.5 | 25 | 0.5 | 1 | Level 8 |
| Laser | 3 | 20 | 30 | 1 | 1 | Level 12 |

### High Scores
- Top 10 saved to `swarm_slayer_scores.dat` (binary format)
- Tracks: Name, Score, Wave, Kill Count

---

## WAVE SYSTEM

### Regular Waves
- **Chasers**: 2 + waveNumber (capped at 15)
- **Shooters**: Start wave 3, then (wave - 2) (capped at 8)
- **Swarm**: Start wave 4, then (wave - 3) * 3 (capped at 20)
- **Spawn delay**: max(0.1, 0.5 - wave * 0.03) seconds between spawns
- **Rest between waves**: max(2.0, 4.0 - wave * 0.15) seconds
- Enemies spawn from random arena edges

### Boss Waves (every 5th wave)
- Boss at center-top (0, arenaHeight * 0.3)
- Minions = waveNumber Swarm enemies alongside boss
- 5-second rest after boss wave
- Boss rotation: wave 5=Behemoth, 10=Swarm Queen, 15=Devastator, 20=Behemoth...

### Wave State Machine
```
WaveComplete (timer counting down)
  -> timer expires -> StartNextWave -> Spawning (spawn queue with delay)
    -> all spawned -> InProgress (waiting for all enemies to die)
      -> 0 enemies -> OnWaveComplete -> WaveComplete
```

---

## HUD & UI SYSTEM (MenuRenderer)

### In-Game HUD Elements (ImGui overlay)
- **Top-Left**: Health bar (red/orange) + XP bar (blue) + Level display
- **Top-Center**: "WAVE N" golden pill badge
- **Top-Right**: Score + Kill count in dark panel
- **Below wave**: Enemies remaining count
- **Center**: Wave countdown "NEXT WAVE IN X.Xs" (between waves)
- **Bottom-Left**: Dash cooldown indicator + Active buffs with duration bars
- **Bottom-Center**: Current weapon name + control hints

### Notifications (center-screen, fading)
- **Kill Streak**: "TRIPLE KILL!" (3+), "KILLING SPREE! x5" (5+), "UNSTOPPABLE! x10" (10+)
- **Level Up**: Bouncing "LEVEL UP!" text
- **Wave Start**: "WAVE N" large banner (2s)
- **Wave Complete**: "WAVE N COMPLETE! +bonus" with optional "PERFECT WAVE!" for no-damage
- **Combo**: "COMBO xN +score" (1.5s)

### Damage Numbers
- Float upward at 2 units/s, fade over 0.8s
- Red for damage >= 20, yellow/orange for smaller hits
- Capped at 30 concurrent numbers
- World-space positions approximated to screen via center offset (40px per world unit)

### Menu Screens
- **Main Menu**: Title, Play/High Scores/Quit buttons, expandable top-5 scores, controls list
- **Pause Menu**: Resume/Restart/Quit to Menu
- **Perk Selection**: 3 random perk cards + weapon unlock buttons
- **Game Over**: Final stats (score, wave, kills, bosses, level, time), high score entry, Play Again/Main Menu

---

## GAME FEEL / JUICE

### Hitstop
- On enemy kill: `m_HitstopTimer = 0.04f` (~2-3 frames at 60fps)
- During hitstop: Only camera, particles, HUD timers, screen flash update. All gameplay frozen.

### Knockback
- **Box2D enemies** (Chaser, Shooter): `ApplyLinearImpulseToCenter` with force=3.0 in bullet direction
- **Velocity enemies** (Swarm): Direct velocity addition of `knockDir * 5.0f`
- Applied in `DamageSystem::OnBulletHit` immediately on damage

### Screen Flash
- Red overlay (0.3 alpha fading to 0) on player damage, duration 0.3s
- Rendered as full-screen sprite in "Overlay" layer after boss health bars

### Low Health Warning
- Pulsing red vignette when health < 30%
- Pulse frequency: 6 Hz sine wave
- Intensity scales inversely with health percentage

### Camera Lookahead
- Camera biases up to 2 units toward mouse cursor direction
- Uses `ScreenToWorld` conversion from `GameUtils.h`
- Smooth interpolation: `glm::mix(current, target, dt * 8.0)` for fluid camera
- Shake offset applied on top of lookahead

---

## AUDIO SYSTEM

### Loaded SFX (7)
| Key | File | Usage |
|-----|------|-------|
| shoot | shoot_fireball.wav | Player fires |
| enemy_shoot | sfx_2.wav | Enemy/boss ranged attacks |
| hit | fireball_hits_enemy.wav | Bullet impacts enemy |
| death | sfx_3.wav | Enemy dies |
| pickup | pickup_coin.wav | Item collection, level up (pitch 1.5) |
| player_hurt | player_hurt.wav | Player takes damage |
| player_death | player_dies.wav | Game over |

### Music
- `top_down_shooter_background_loop.wav` - Looping, volume 0.3
- Pauses/resumes with game pause, stops on game over

### Audio Features
- 3D positional audio (min distance 5, max distance 50, linear rolloff)
- Random pitch variation: all SFX get random 0.9x-1.1x pitch multiplier
- Max 16 concurrent sounds (excess skipped)
- Separate SFX volume multiplier (`m_SFXVolume`)
- Cleanup of finished sources on each play call

---

## PHYSICS SETUP

- **World gravity**: (0, 0) - top-down
- **Arena**: 30x16 units with 1-unit thick static walls
- **Collision categories** (bitmask): Player=0x0001, Enemy=0x0002, Bullet=0x0004, Wall=0x0008, Sensor=0x0010

### Per-entity physics:
- **Player**: Box2D dynamic, circle collider 0.35, linear damping 5.0, fixed rotation
- **Chaser/Shooter enemies**: Box2D dynamic, circle collider 0.4 * sprite width, linear damping 4.0
- **Swarm enemies**: VelocityComponent only (no Box2D), drag=2.0, maxSpeed=8.0*scale
- **Walls**: Box2D static, box collider
- **Bullets**: VelocityComponent, no Box2D body. Hit via BulletCollisionSystem raycasts.
- **Bosses**: Box2D dynamic, circle collider, linear damping 3.0, sizes vary (1.5-2.0)

---

## ANIMATION SYSTEM

Clips loaded from JSON files in `animations/` directory:

| Clip Name | Entity | Notes |
|-----------|--------|-------|
| hoodzy_chaser_enemy_animation | Chaser enemy | Run cycle |
| floaty_enemy_animation | (unused currently) | |
| swarmer_run_animation | (unused, using goblin instead) | |
| evil_archer_run_south | Shooter enemy | Run south |
| goblin_with_sword_run_south | Swarm enemy | Run south |
| red_mage_run_south | Player (running) | |
| red_mage_idle_south | Player (idle) | Default |
| red_mage_fireball_south | Player (shooting) | |
| goblin_queen_walk_south | Swarm Queen boss | |
| large_behemoth_walk_south | Behemoth boss | |
| monster_with_bow_run_south | Devastator boss | |

---

## KEY IMPLEMENTATION DETAILS

### SwarmSlayerLayer Lifecycle
1. `OnAttach()`: Init GameState, set menu callbacks, init audio, create camera
2. `StartGame()`: CleanupGameWorld if needed, reset state, InitializeGameWorld, start music
3. `InitializeGameWorld()`: Create scene, init all 17+ systems in order, create arena, spawn hazards, spawn player
4. `UpdateGame(dt)`: Hitstop check -> weapon switch -> combo timer -> regen -> wave manager -> feed HUD -> all systems in order -> camera -> audio listener
5. `RenderGame()`: Clear, begin scene, render trails, sort and render sprites, render boss health bars, render screen flash/vignette, end scene
6. `CleanupGameWorld()`: Invalidate player, shutdown all systems, clear physics/animation pointers, destroy all entities, reset scene
7. `ShutdownGameSystems()`: Shutdown ParticleManager, delete all systems in reverse order, null pointers

### DamageSystem Callback Flow
```
BulletCollisionSystem detects hit -> OnBulletHit(bullet, target, damage, hitPos)
  -> If enemy: TakeDamage -> callback OnEnemyHit(pos, dir, damage) -> knockback
  -> If player: TakeDamage -> SpawnPlayerDamageEffect -> callback OnPlayerHit()
ProcessDeaths():
  -> Dead enemy: death sound -> callback OnEnemyKilled(pos, color, enemyType) -> maybe drop power-up -> destroy
  -> Dead player: callback OnPlayerHit + OnGameOver
```

### Important State in SwarmSlayerLayer
- `m_HitstopTimer`: >0 freezes gameplay
- `m_ScreenFlashTimer`: >0 renders red overlay
- `m_KillStreakCount` / `m_KillStreakTimer`: Kill streak tracking (resets after 3s or on player hit)
- `m_PlayerEntity`: Handle to player (invalidated on cleanup)
- `m_WaveManager`: Tracks wave state, provides rest timer, enemies to spawn, wave-just-started flag

### Important State in GameStats (GameState singleton)
- `Score`, `TotalKills`, `CurrentXP`, `XPToNextLevel`, `PlayerLevel`
- `WaveReached`, `PlayTime`, `BossesKilled`
- `ComboCount`, `ComboTimer` (2s window), `NoDamageThisWave`
- `AddKillScore(enemyType, wave)`: Applies wave multiplier + combo bonus, returns total
- `GetWaveCompletionBonus(wave)`: 500*wave + (noDamage ? 1000*wave : 0)

---

## IMPLEMENTED POLISH (This Session)

### 1. Enhanced Score System (GameState.h)
- [x] Enemy-type scoring: Chaser=100, Shooter=150, Swarm=50
- [x] Wave multiplier: score * (1 + wave * 0.1)
- [x] Combo system: kills within 2s chain combo, +50 bonus per count
- [x] Wave completion bonus: 500 * wave
- [x] No-damage wave bonus: +1000 * wave ("PERFECT WAVE!")

### 2. HUD Improvements (MenuRenderer.h, SwarmSlayerLayer.h)
- [x] Enemies remaining count below wave indicator
- [x] Wave countdown "NEXT WAVE IN X.Xs" between waves
- [x] Active buffs display (bottom-left) with colored duration bars
- [x] Wave start banner "WAVE N" (2s)
- [x] Wave complete banner with bonus amount and "PERFECT WAVE!" for no-damage
- [x] Combo counter "COMBO xN +score" (1.5s)
- [x] Floating damage numbers on enemy hit (fade + float up)

### 3. Game Feel / Juice (SwarmSlayerLayer.h, DamageSystem.h)
- [x] Hitstop: ~2-3 frame pause on enemy kill
- [x] Enemy knockback on hit (Box2D impulse for heavy, velocity push for Swarm)
- [x] Screen red flash on player damage (0.3s fade)
- [x] Low health pulsing red vignette (<30% HP)
- [x] Camera lookahead toward mouse cursor (up to 2 units, smooth lerp)

### 4. Difficulty Scaling (EntityFactory.h, WaveManager.h)
- [x] Enemy health scales +5% per wave
- [x] Enemy speed scales +2% per wave
- [x] Wave number passed through all spawn callbacks

### 5. Audio Polish (AudioManager.h)
- [x] Random pitch variation (0.9-1.1x) on all SFX
- [x] Max 16 concurrent sounds limit (excess skipped)
- [x] Separate SFX volume multiplier

---

## REMAINING POLISH ITEMS (NOT YET IMPLEMENTED)

### Audio Gaps
- [ ] Missing SFX: dash, wave_start, wave_complete, shield_hit, shield_break, menu_hover, menu_select
- [ ] Audio ducking (lower music during intense moments)
- [ ] Low-pass filter when paused
- [ ] Boss theme music (separate from gameplay loop)

### Visual Gaps
- [ ] Dash trail effect (afterimages)
- [ ] Enemy spawn animation (fade in / pop up)
- [ ] Shield visual (bubble around player)
- [ ] Buff visual aura indicators
- [ ] Floor texture/pattern (currently solid dark background)
- [ ] Arena decorations (crates, debris)
- [ ] Footstep dust particles
- [ ] Speed lines during dash

### UI Gaps
- [ ] Settings menu (audio volumes, screen shake toggle, particle quality)
- [ ] Keyboard menu navigation (Up/Down/Enter)
- [ ] Crosshair cursor (replace default)
- [ ] Score animated on increase

### Gameplay Gaps
- [ ] "Elite" enemy variants (double HP, different color, higher waves)
- [ ] Difficulty selection (Easy/Normal/Hard)
- [ ] Time slow on near-death moments
- [ ] Dynamic camera zoom (zoom out when many enemies)
- [ ] Tutorial / onboarding for new players
- [ ] Persist settings to JSON

### Performance
- [ ] Object pooling for bullets, particles, enemies
- [ ] Spatial partitioning for collision
- [ ] Off-screen entity culling
- [ ] Frame rate cap option

### Quality of Life
- [ ] Colorblind mode
- [ ] Screen shake toggle
- [ ] Flash effects toggle
- [ ] Window icon
- [ ] Graceful alt-tab handling

---

## BUILD & RUN

```powershell
cd C:\dev\Pillar
.\scripts\bootstrap.ps1
cmake --build --preset windows-debug
.\bin\Debug-x64\Sandbox\SandboxApp.exe
```
