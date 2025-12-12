# Arena Protocol - Required Assets

This document lists all the PNG texture files required for the Arena Protocol game.

## Asset Directory Structure

Place all textures in: `Sandbox/assets/textures/`

```
Sandbox/
└── assets/
    └── textures/
        ├── player_ship.png
        ├── bullet.png
        ├── enemy_bullet.png
        ├── drone.png
        ├── turret_base.png
        ├── turret_barrel.png
        ├── charger.png
        ├── sentinel_boss.png
        ├── xp_gem_small.png
        ├── xp_gem_medium.png
        ├── xp_gem_large.png
        ├── crate.png
        ├── pillar.png
        ├── wall.png
        ├── particle_circle.png
        └── particle_spark.png
```

## Asset Specifications

### Player Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `player_ship.png` | 64x64 | Top-down view of player ship |

### Bullet Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `bullet.png` | 16x16 | Player projectile (yellow/orange glow) |
| `enemy_bullet.png` | 16x16 | Enemy projectile (red glow) |

### Enemy Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `drone.png` | 32x32 | Standard enemy drone |
| `turret_base.png` | 48x48 | Stationary turret base |
| `turret_barrel.png` | 24x48 | Rotating turret gun barrel |
| `charger.png` | 40x40 | Fast-moving charger enemy |
| `sentinel_boss.png` | 128x128 | Large boss enemy |

### XP Gem Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `xp_gem_small.png` | 16x16 | Small XP gem (green, 10 XP) |
| `xp_gem_medium.png` | 24x24 | Medium XP gem (blue, 25 XP) |
| `xp_gem_large.png` | 32x32 | Large XP gem (purple, 50 XP) |

### Environment/Obstacle Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `crate.png` | 48x48 | Destructible crate obstacle |
| `pillar.png` | 48x48 | Indestructible pillar |
| `wall.png` | 64x64 | Wall segment (tiled) |

### Particle Assets

| Filename | Recommended Size | Description |
|----------|------------------|-------------|
| `particle_circle.png` | 32x32 | Soft circular particle (white, alpha gradient) |
| `particle_spark.png` | 16x16 | Small spark/debris particle |

## Visual Style Guidelines

1. **Color Palette:**
   - Player: Cyan/Blue tones (#00BFFF)
   - Enemies: Red/Orange tones (#FF4444)
   - XP Gems: Green/Blue/Purple gradient
   - Environment: Dark metallic grays

2. **Art Style:**
   - Top-down perspective
   - Clean pixel art or vector style
   - Clear silhouettes for gameplay readability
   - Transparent backgrounds (PNG with alpha)

3. **Orientation:**
   - All sprites should face **UP** (north) as default
   - Rotation is applied in-game via code

## Fallback Behavior

If textures are missing, the game will render colored rectangles:
- Player: Cyan
- Bullets: Yellow (player) / Red (enemy)
- Drones: Red
- Turrets: Dark Red
- Chargers: Orange
- Boss: Dark Purple
- XP Gems: Green/Blue/Purple by size
- Obstacles: Gray/Brown

## Quick Start (Placeholder Assets)

For testing, you can create simple placeholder assets:

1. Use any image editor (Paint, GIMP, Photoshop)
2. Create images at recommended sizes
3. Fill with solid colors matching fallback colors
4. Save as PNG with transparent backgrounds

Example: Create a 64x64 cyan square for `player_ship.png`

## Asset Loading Code Reference

Assets are loaded in `Factory/EntityFactory.cpp`:

```cpp
// Example texture loading
m_PlayerTexture = Pillar::Texture2D::Create("player_ship.png");
// AssetManager automatically searches:
// 1. Sandbox/assets/textures/player_ship.png
// 2. assets/textures/player_ship.png (next to executable)
```

## Notes

- All textures are optional - game works with fallback colors
- Higher resolution textures (2x, 4x) can be used for better quality
- Mipmapping is enabled for scaled rendering
