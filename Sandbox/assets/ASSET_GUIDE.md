# Arena Protocol - Asset Guide

## Required Textures

Place all PNG texture files in the following directory:
```
Sandbox/assets/textures/
```

### Player Assets
| Filename | Size | Description |
|----------|------|-------------|
| `player_ship.png` | 64x64 | Main player sprite (triangular cyan ship) |
| `player_ship_sheet.png` | 256x64 | Animation sheet (4 frames: idle, thrust, damaged) |

### Enemy Assets
| Filename | Size | Description |
|----------|------|-------------|
| `drone.png` | 48x48 | Basic drone enemy (hexagonal, red) |
| `turret_base.png` | 64x64 | Turret base (gray, stationary) |
| `turret_barrel.png` | 32x48 | Turret barrel (rotates) |
| `charger.png` | 48x64 | Charger enemy (orange, ram-shaped) |
| `sentinel_boss.png` | 192x192 | Boss sprite (purple/blue sphere with rings) |

### Projectile Assets
| Filename | Size | Description |
|----------|------|-------------|
| `bullet.png` | 16x32 | Player bullet (cyan/white energy bolt) |
| `enemy_bullet.png` | 16x32 | Enemy bullet (red/orange plasma) |

### Collectible Assets
| Filename | Size | Description |
|----------|------|-------------|
| `xp_gem_small.png` | 16x16 | Small XP gem (green, 1 XP) |
| `xp_gem_medium.png` | 24x24 | Medium XP gem (blue, 5 XP) |
| `xp_gem_large.png` | 32x32 | Large XP gem (purple/gold, 10 XP) |

### Environment Assets
| Filename | Size | Description |
|----------|------|-------------|
| `crate.png` | 64x64 | Wooden crate obstacle |
| `pillar.png` | 64x64 | Stone/metal pillar |
| `wall_tile.png` | 64x64 | Modular wall segment |

### Particle Assets
| Filename | Size | Description |
|----------|------|-------------|
| `particle_circle.png` | 16x16 | Round particle (soft glow) |
| `particle_spark.png` | 16x16 | Spark particle (bright point) |

### UI Assets (Optional)
| Filename | Size | Description |
|----------|------|-------------|
| `health_bar.png` | 128x16 | Health bar background |
| `health_fill.png` | 128x16 | Health bar fill |
| `crosshair.png` | 32x32 | Aiming reticle |

## Required Audio

Place all WAV audio files in the following directory:
```
Sandbox/assets/audio/
```

### Sound Effects (`audio/sfx/`)
| Filename | Description |
|----------|-------------|
| `shoot.wav` | Player weapon fire |
| `enemy_shoot.wav` | Enemy weapon fire |
| `hit.wav` | Bullet impact |
| `explosion.wav` | Enemy death explosion |
| `pickup.wav` | XP gem collection |
| `dash.wav` | Player dash ability |
| `boss_attack.wav` | Boss attack sound |
| `level_up.wav` | Player level up |

### Music (`audio/music/`)
| Filename | Description |
|----------|-------------|
| `chamber_ambient.wav` | Background loop for chambers |
| `boss_battle.wav` | Boss fight music |
| `victory.wav` | Victory jingle |
| `game_over.wav` | Game over sound |

## Fallback Colors

If textures are not found, the game will use colored squares as placeholders:
- **Player**: Cyan (0x00FFFF)
- **Drone**: Red (0xFF0000)
- **Turret**: Gray (0x808080)
- **Charger**: Orange (0xFFA500)
- **Boss**: Purple (0x800080)
- **Player Bullet**: Cyan (0x00FFFF)
- **Enemy Bullet**: Orange-Red (0xFF4500)
- **XP Gem Small**: Green (0x00FF00)
- **XP Gem Medium**: Blue (0x0000FF)
- **XP Gem Large**: Purple (0x9400D3)
- **Crate**: Brown (0x8B4513)
- **Pillar**: Gray (0x696969)

## Creating Simple Placeholder Textures

You can use any image editor to create simple placeholder textures:

1. **Player Ship**: Create a 64x64 triangle pointing right, fill with cyan (#00FFFF)
2. **Drone**: Create a 48x48 hexagon, fill with red (#FF0000)
3. **Bullets**: Create elongated ovals (16x32), use appropriate colors
4. **XP Gems**: Create diamond shapes in green/blue/purple

## Directory Structure After Setup

```
Sandbox/
└── assets/
    ├── textures/
    │   ├── player_ship.png
    │   ├── drone.png
    │   ├── turret_base.png
    │   ├── turret_barrel.png
    │   ├── charger.png
    │   ├── sentinel_boss.png
    │   ├── bullet.png
    │   ├── enemy_bullet.png
    │   ├── xp_gem_small.png
    │   ├── xp_gem_medium.png
    │   ├── xp_gem_large.png
    │   ├── crate.png
    │   ├── pillar.png
    │   ├── particle_circle.png
    │   └── particle_spark.png
    ├── audio/
    │   ├── sfx/
    │   │   ├── shoot.wav
    │   │   ├── enemy_shoot.wav
    │   │   ├── hit.wav
    │   │   ├── explosion.wav
    │   │   ├── pickup.wav
    │   │   ├── dash.wav
    │   │   └── boss_attack.wav
    │   └── music/
    │       ├── chamber_ambient.wav
    │       └── boss_battle.wav
    └── scenes/
        └── (scene files will be here)
```

## Notes

- All textures should have transparent backgrounds where appropriate
- PNG format is recommended for all textures
- WAV format (16-bit PCM) is required for audio files
- The game will run with placeholder colors if textures are missing
- Audio is optional but enhances the experience significantly
