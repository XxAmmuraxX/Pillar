# Swarm Slayer - Implementation Status Report

> Generated 2026-01-29 | Branch: `game/sandbox`

---

## Pillar Engine Features Used

| Engine System | Status | Usage in Game |
|---|---|---|
| ECS (Scene/Entity) | USED | Core architecture — all game objects are entities with components |
| TransformComponent | USED | Position, rotation, scale for every entity |
| SpriteComponent | USED | Rendering with textures, colors, layers, order-in-layer |
| Renderer2D | USED | Batched 2D sprite rendering |
| PhysicsSystem (Box2D) | USED | Zero-gravity top-down physics, dynamic/static bodies |
| PhysicsSyncSystem | USED | Syncs Box2D positions back to ECS transforms |
| VelocityComponent | USED | Lightweight movement for swarm enemies, bullets, XP orbs |
| RigidbodyComponent | USED | Dynamic bodies (player, enemies, bosses), static bodies (walls) |
| ColliderComponent | USED | Circle and box colliders with category filtering |
| BulletCollisionSystem | USED | Raycast-based hit detection with callbacks |
| AnimationSystem | USED | 25+ animation clips (directional movement for all entities) |
| AnimationComponent | USED | Per-entity animation state |
| AudioEngine (OpenAL) | USED | 3D positional audio, looping music, volume/pitch control |
| AudioBuffer / AudioSource | USED | 7 pre-loaded SFX + background music loop |
| Input System | USED | Keyboard (WASD, Space, Shift, 1-5) + Mouse (aim, LMB shoot) |
| OrthographicCamera | USED | Camera follow, lookahead toward cursor, shake effects |
| AssetManager | USED | Texture and audio asset path resolution |
| Layer System | USED | SwarmSlayerLayer extends Pillar::Layer |
| Logging | USED | PIL_INFO, PIL_WARN throughout |
| Texture2D | USED | Texture loading and caching |

### Engine Features NOT Used (or not confirmed)

| Engine System | Status |
|---|---|
| ParticleEmitterComponent / ParticleSystem (engine-level) | ✅ NOW USED - Continuous emitters on hazards and player dash |
| BulletPool (engine-level) | ✅ NOW USED - 300-bullet pool for all projectiles |
| Lighting2D / Lighting2DSystem (engine-level) | ✅ NOW USED - Full 2D lighting with shadows |
| Light2DComponent (engine-level) | ✅ NOW USED - Point lights on player, hazards, pickups, projectiles |
| ShadowCaster2DComponent (engine-level) | ✅ NOW USED - Walls cast shadows from player light |
| SpriteRenderSystem (engine-level) | NOT USED - Manual Renderer2D calls preserve custom layer-based rendering |

---

## Game Features — Implemented

### Player

- WASD movement with diagonal normalization
- Mouse-based aiming (4-directional facing with animation switching)
- 8 directional animation clips (4 run, 4 idle)
- Dash ability (Space/Shift) — 0.15s duration, 1s cooldown, invulnerability, blue trail particles, 15.0 speed
- Health system (100 HP base)
- Speed/damage multipliers from perks and buffs

### Weapons (5)

| Weapon | Damage | Fire Rate | Speed | Spread | Bullets | Unlock |
|---|---|---|---|---|---|---|
| Pistol | 10 | 5 | 15 | 2 | 1 | Default |
| Shotgun | 6 | 2 | 12 | 15 | 5 | Level 3 |
| SMG | 5 | 12 | 18 | 5 | 1 | Level 5 |
| Rifle | 25 | 1.5 | 25 | 0.5 | 1 | Level 8 |
| Laser | 3 | 20 | 30 | 1 | 1 | Level 12 |

- Perk-based damage/speed/pierce multipliers
- Muzzle flash particles
- Positional shooting SFX

### Enemies (3 types)

| Type | HP | Speed | Damage | XP | Physics | AI |
|---|---|---|---|---|---|---|
| Chaser | 30 * scale | 4.0 * scale | 15 | 10 | Box2D | Relentless pursuit |
| Shooter | 50 * scale | 2.0 * scale | 8 (projectile) | 25 | Box2D | Maintains distance, fires |
| Swarm | 10 * scale | 6.0 * scale | 5 (contact) | 5 | Velocity | Fast flood |

- Health/speed scale +5%/+2% per wave
- Directional animations
- Detection range system

### Bosses (3 types, every 5 waves)

| Boss | Base HP | Speed | Attack | Special |
|---|---|---|---|---|
| Behemoth | 300 + wave*50 | 2.5 | 25 + wave*2 melee | Tank, charges |
| Brood Mother | 250 + wave*40 | 2.0 | 15 + wave*1.5 | Spawns 2-4 minions every 4s |
| Devastator | 350 + wave*45 | 1.5 | 35 + wave*2.5 projectile | 1/3/5 spread projectiles by phase |

- 3-phase system (100-50%, 50-25%, 25-0%) with increasing speed/attack rate
- Health bars rendered above boss
- Phase-based color tinting
- Boss rotation: Behemoth -> Brood Mother -> Devastator

### Wave System

- Dynamic wave generation with scaling difficulty
- Waves 1-2: Chasers only
- Wave 3+: Shooters introduced
- Wave 4+: Swarm introduced
- Enemy caps: Chasers 15, Shooters 8, Swarm 20
- Spawn delay decreases per wave (0.5s -> 0.1s)
- Rest time between waves (4s -> 2s)
- Boss waves every 5th wave
- State machine: WaveComplete -> Spawning -> InProgress

### Damage & Combat

- Health component with max/current tracking
- Invulnerability timer system
- Knockback on hit (Box2D impulse or velocity push)
- Death processing with entity destruction
- Player death triggers game over
- 30% power-up drop chance on enemy death

### Power-Ups (6 types)

| Type | Effect | Duration |
|---|---|---|
| Health | +25 HP | Instant |
| Speed Boost | 1.5x move speed | 5s |
| Fire Rate Up | 1.5x fire rate | 5s |
| Damage Up | 1.5x damage | 5s |
| Shield | Invulnerability | 3s |
| Magnet | 5.0 pickup radius | 10s |

- Bobbing animation, pulsing glow, color-coded visuals
- Magnet attraction mechanics

### XP & Leveling

- XP orbs dropped on enemy death (5-50 XP, color/size coded)
- Auto-collection at 1.5 radius, magnet attraction at 5.0 radius
- Bobbing + pulsing glow animation
- Level-up formula: 100 * 1.2^(level-1)
- Unlocks weapons and perk selection on level-up

### Perk System (12 perks, 4 categories)

**Offensive:**
| Perk | Max Lvl | Effect/Lvl |
|---|---|---|
| Damage Up | 5 | +25% damage |
| Fire Rate Up | 5 | +20% fire rate |
| Bullet Speed Up | 3 | +30% speed |
| Pierce Shot | 3 | +1 pierce |
| Explosive Rounds | 1 | AOE on hit |

**Defensive:**
| Perk | Max Lvl | Effect/Lvl |
|---|---|---|
| Max Health Up | 5 | +25 HP |
| Regeneration | 3 | +1 HP/s |
| Damage Reduction | 4 | -15% damage taken |
| Dodge Chance | 3 | +10% evasion |

**Utility:**
| Perk | Max Lvl | Effect/Lvl |
|---|---|---|
| Move Speed Up | 4 | +15% speed |
| XP Magnet | 3 | +2.0 radius |
| Lucky Drops | 3 | +25% drop rate |

- Selection offered every 2 waves or after boss defeat
- Random 3-perk choice from available pool

### Scoring & Combo

- Base score by type: Chaser 100, Shooter 150, Swarm 50
- Wave multiplier: 1 + wave * 0.1
- Combo system: kills within 2s chain, +50 per combo level
- Wave completion bonus: 500 * wave
- Perfect wave (no damage): +1000 * wave
- High score persistence (top 10, binary file)

### HUD & UI

- Health bar, XP bar, level display (top-left)
- Wave indicator (top-center)
- Score + kill count (top-right)
- Enemies remaining count
- Wave countdown timer
- Dash cooldown indicator
- Active buffs display with duration bars
- Floating damage numbers (color-coded)
- Kill streak notifications (Triple Kill, Killing Spree, Unstoppable)
- Combo counter display
- Level-up announcement
- Wave start/complete banners
- Main Menu, Pause Menu, Perk Selection, Game Over screens
- Weapon unlock notifications

### Visual Effects

- Particle system with 2000-particle pool
- Muzzle flash (directional burst)
- Hit sparks (6 particles, knockback-aligned)
- Death explosions (12 particles, color-matched)
- Boss death (50+ particles)
- Barrel explosion (multi-stage)
- XP/power-up collection sparkles
- Level-up celebration burst
- Player damage red particles
- Dash trail (blue particles every 20ms)
- Screen red flash on player damage (0.3s fade)
- Low health pulsing red vignette (<30% HP)

### Game Feel / Juice

- Hitstop: ~2-3 frame pause on enemy kill
- Knockback on bullet hits
- Camera lookahead toward mouse (up to 2 units, smooth lerp)
- Camera shake (4 intensities: hit, kill, player hit, explosion)
- Screen flash on damage

### Audio

- 7 SFX: shoot, enemy_shoot, hit, death, pickup, player_hurt, player_death
- Background music loop (0.3 volume)
- 3D positional audio (min 5, max 50 distance)
- Pitch variation (0.9-1.1x)
- Max 16 concurrent sounds
- Music pause/resume on game pause

### Environmental Hazards (5 types)

| Hazard | Damage | Cooldown | Special |
|---|---|---|---|
| Explosive Barrel | 60 (3.5 radius) | One-time | 30 HP, chain reactions |
| Spike Trap | 15 | 1.0s | Periodic |
| Slow Field | 0 | - | 60% slow, 2s duration |
| Damage Zone | 5 | 0.25s | Continuous |
| Poison Pool | 3 | 0.5s | 3s lingering |

- 8 randomly spawned per arena
- Pulsing visual indicators

### Game State

- 5 states: MainMenu, Playing, Paused, PerkSelection, GameOver
- Full state machine with callbacks
- Per-run statistics tracking

---

## NOT Yet Implemented

### From Polish Plan

**Audio Gaps:**
- [ ] Missing SFX: dash, wave_start, wave_complete, shield_hit, shield_break, menu_hover, menu_select
- [ ] Audio ducking (lower music during intense moments)
- [ ] Low-pass filter when paused
- [ ] Boss-specific theme music

**Visual Gaps:**
- [ ] Enemy spawn animation (fade in / pop up)
- [ ] Shield visual (bubble/aura around player when shield active)
- [ ] Buff visual aura indicators on player
- [ ] Floor texture/pattern (currently solid dark background)
- [ ] Arena decorations (crates, debris, props)
- [ ] Footstep dust particles
- [ ] Speed lines during dash

**UI Gaps:**
- [ ] Settings menu (audio volumes, screen shake toggle, particle quality)
- [ ] Keyboard menu navigation
- [ ] Crosshair cursor replacement
- [ ] Animated score counter on increase

**Gameplay Gaps:**
- [ ] "Elite" enemy variants (stronger versions with visual distinction)
- [ ] Difficulty selection (Easy / Normal / Hard)
- [ ] Time slow on near-death
- [ ] Dynamic camera zoom (zoom out when many enemies)
- [ ] Tutorial / onboarding sequence
- [ ] Persist settings to JSON

**Performance:**
- [ ] Object pooling for bullets and enemies
- [ ] Spatial partitioning for collision
- [ ] Off-screen entity culling
- [ ] Frame rate cap option

**Quality of Life:**
- [ ] Colorblind mode
- [ ] Screen shake toggle
- [ ] Flash effects toggle
- [ ] Window icon
- [ ] Graceful alt-tab handling

---

### From Game Identity (Scrapyard Salvation)

The Game Identity document defines a full creative re-theme that has **not been implemented yet**. The game currently uses placeholder/fantasy sprites (red mage, goblins, archers) rather than the intended post-apocalyptic industrial aesthetic.

**Visual Identity — Not Applied:**
- [ ] Gritty color palette (Charred Black, Oxidized Orange, Bile Yellow-Green, etc.)
- [ ] Film grain post-processing effect
- [ ] Chromatic aberration at screen edges
- [ ] Persistent vignette (intensifies on damage)
- [ ] Ambient particle system (drifting ash, dust, sparks, embers)
- [ ] CRT scanlines option

**Player Re-theme — Not Applied:**
- [ ] Replace red mage with "Salvager" character (scrap armor, respirator, wiry frame)
- [ ] Salvager-specific animations (hunched movement, nervous idle, stagger on hit)

**Enemy Re-theme — Not Applied:**
- [ ] Crawlers (mutated feral humans) replacing generic chasers
- [ ] Shambling Husks (fungal reanimated corpses) — new enemy type not in game
- [ ] Scrap Sentinels (corrupted drones) replacing archers
- [ ] Swarm Rats (mutated vermin) replacing goblins
- [ ] Unique death animations per enemy type
- [ ] Blood/ichor splatter effects (persistent ground stains)
- [ ] Enemy-specific vocalizations and sound design

**Boss Re-theme — Not Applied:**
- [ ] Behemoth visual overhaul (mountain of corrupted flesh + embedded scrap)
- [ ] Harvester (mechanical spider) replacing current boss concept
- [ ] Boss-specific sound design (roars, grinding gears, pneumatic hisses)

**Weapon Re-theme — Not Applied:**
- [ ] Pipe Pistol visual (copper pipe, exposed springs)
- [ ] Nail Spreader visual (industrial nail gun chassis)
- [ ] Spark Spitter visual (car alternator parts)
- [ ] Boom Pipe visual (sewer pipe + propane warheads)
- [ ] The Grinder (melee weapon — new weapon type not in game)
- [ ] Weapon-specific sound design (punchy, metallic)
- [ ] Shell casing ejection particles
- [ ] Unique muzzle flash per weapon

**Power-Up Re-theme — Not Applied:**
- [ ] Med-Stim (cracked syringe) replacing health pickup
- [ ] Adrenaline Shot replacing speed boost
- [ ] Overclocked Magazine replacing fire rate
- [ ] Hollow Points replacing damage up
- [ ] Scrap Barrier replacing shield
- [ ] Salvage Beacon replacing magnet

**Arena Re-theme — Not Applied:**
- [ ] Floor tileset (cracked concrete, rust plates, oil stains, tire marks)
- [ ] Boundary walls (stacked shipping containers, crushed cars, chain-link fencing)
- [ ] Environmental props (abandoned vehicles, workbenches, burning trash cans)
- [ ] Toxic fog at arena edges
- [ ] Dynamic lighting from fire sources
- [ ] Environmental storytelling (graffiti, skeletons, abandoned gear)

**UI Re-theme — Not Applied:**
- [ ] Industrial gauge health bar (analog needle style)
- [ ] LED segment ammo counter
- [ ] Stamped metal wave indicator
- [ ] CRT-style main menu (scanlines, curve distortion, flickering text)
- [ ] Glitch-effect game over screen ("SIGNAL LOST")
- [ ] Mechanical counter wheel score display
- [ ] Industrial font (Impact/Bebas Neue/stencil)

**Audio Re-theme — Not Applied:**
- [ ] Industrial ambient soundscape (distant machinery, wind, dripping, electrical hum)
- [ ] Industrial metal music direction
- [ ] Dynamic music system (tempo increase for bosses, silence on wave complete)
- [ ] Unique weapon sounds per weapon type
- [ ] Enemy vocalizations (chittering, squelching, servo whines, skittering)
- [ ] UI sounds (typewriter clicks, mechanical switches, CRT static)

**Additional Game Feel from Identity Doc — Not Applied:**
- [ ] Subtle constant camera shake (0.5-1px)
- [ ] Boss footstep micro-shakes
- [ ] Weapon recoil camera kick
- [ ] Death cam (slow zoom out, desaturation)
- [ ] Color grading (desaturated, teal-orange push)
- [ ] Shell casings (ejecting, bouncing, fading)
- [ ] Persistent blood splatter on ground
- [ ] Bullet smoke trails
- [ ] Tracer rounds (1 in 4 with orange glow)
- [ ] Loading screen narrative tips (in-universe lore)
- [ ] Kill milestone celebrations (100, 500, 1000)

---

## Summary

| Category | Implemented | Remaining |
|---|---|---|
| **Pillar Engine Integration** | 10 systems fully used | — |
| **Core Gameplay** | Player, enemies, bosses, waves, combat | Elite variants, difficulty selection, tutorial |
| **Progression** | XP, leveling, 12 perks, 5 weapons, scoring | — |
| **HUD/UI** | Full HUD, 4 menu screens, notifications | Settings menu, keyboard nav, crosshair |
| **Audio** | 7 SFX, music, 3D positional | 7+ missing SFX, ducking, boss music |
| **Visual Effects** | Particles, screen flash, vignette, shake | Spawn anims, shield visual, floor textures |
| **Game Feel** | Hitstop, knockback, lookahead, shake | Time slow, dynamic zoom |
| **Environmental** | 5 hazard types | Arena decorations, props |
| **Scrapyard Salvation Theme** | NOT STARTED | Full visual, audio, and UI re-theme needed |
| **Performance** | Particle pooling only | Bullet/enemy pooling, spatial partitioning, culling |
| **QoL** | High score save | Colorblind mode, settings persistence, accessibility toggles |

The core gameplay loop is **fully functional and feature-complete**. The primary remaining work is the **Scrapyard Salvation creative re-theme** (art, audio, UI overhaul) and **polish items** (missing SFX, visual gaps, settings, performance optimizations).
