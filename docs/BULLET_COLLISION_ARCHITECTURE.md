# Bullet Collision Architecture in Top-Down Shooter

## Current Implementation

The Top-Down Shooter currently uses a **simple circle-circle collision detection** approach in the `DamageSystem` for bullet collisions. This is a valid starting point but doesn't leverage Pillar's more advanced systems.

### Why Custom Collision Instead of Engine Systems?

**Pillar's `BulletCollisionSystem`** is designed for:
- **Raycasting against Box2D bodies** (heavy entities with `RigidbodyComponent`)
- High-performance bullet detection via physics queries
- Automatic owner filtering (bullets don't hit their shooter)

**Limitations for this game:**
1. **Swarm enemies are lightweight** (use `VelocityComponent`, no Box2D body)
   - BulletCollisionSystem's raycasts would miss them
2. **Enemy bullets need to hit player AND player bullets need to hit enemies**
   - Requires bidirectional collision logic
3. **The engine's BulletCollisionSystem doesn't have damage callbacks yet**
   - It has a TODO comment for damage application (Phase 6)

### Current Approach: Hybrid System

The `DamageSystem` implements:
```cpp
// 1. Categorize bullets by owner
bool isPlayerBullet = (owner has PlayerTagComponent)
bool isEnemyBullet = (owner has EnemyComponent)

// 2. Check appropriate targets
if (isPlayerBullet) CheckBulletVsEnemies()
if (isEnemyBullet) CheckBulletVsPlayer()

// 3. Circle-circle collision for all entity types
float distance = glm::distance(bullet.pos, target.pos)
if (distance < bulletRadius + targetRadius) { hit! }
```

**Pros:**
- ✅ Works for both heavy (Box2D) and light (VelocityComponent) entities
- ✅ Supports bidirectional damage (player ↔ enemy)
- ✅ Simple and debuggable
- ✅ Integrates easily with effect callbacks

**Cons:**
- ❌ O(bullets × enemies) performance scaling
- ❌ No spatial optimization (checks all pairs)
- ❌ Doesn't use engine's raycast system

## Spatial Hash Grid - Why Not Used?

**Pillar's `SpatialHashGrid`** is designed for:
- Broad-phase collision detection
- Finding nearby entities in O(1) average case
- Used in `XPCollectionSystem` for efficient power-up collection

**Why not used for bullets?**
1. **Small entity count** - With <100 enemies and <50 bullets, naive O(n×m) is fast enough
2. **Moving bullets** - Requires updating grid every frame (overhead)
3. **Different collision rules** - Player bullets vs enemy bullets need different masks
4. **Engine API** - SpatialHashGrid is not exposed as a reusable generic system yet

## Future Optimizations

### Option 1: Extend BulletCollisionSystem
Add callback support to Pillar's `BulletCollisionSystem`:
```cpp
// In BulletCollisionSystem.h
using OnHitCallback = std::function<void(Entity bullet, Entity hit, float damage)>;
void SetOnHitCallback(OnHitCallback callback);
```

**Pros:**
- Uses engine's optimized raycasting
- Automatic owner filtering
- Physics-accurate hit detection

**Cons:**
- Still doesn't support light entities (Swarm)
- Requires engine modifications

### Option 2: Add Spatial Hash Grid
Implement broad-phase collision with spatial hash:
```cpp
// Build grid each frame
SpatialHashGrid grid(cellSize: 2.0f);
for (enemy : enemies) grid.Insert(enemy, enemy.position);

// Query nearby for each bullet
for (bullet : bullets) {
    auto nearby = grid.Query(bullet.position, searchRadius: 1.0f);
    for (enemy : nearby) {
        if (CheckCircleCollision(bullet, enemy)) { hit! }
    }
}
```

**Performance:** O(bullets × (nearby enemies)) instead of O(bullets × all enemies)

**When to implement:**
- Entity count > 200
- Performance profiling shows collision as bottleneck
- Frame time > 16ms (below 60 FPS)

### Option 3: Hybrid Approach (Recommended)
1. Use `BulletCollisionSystem` for **heavy entities** (player, Chaser, Shooter)
2. Use spatial hash grid for **light entities** (Swarm)
3. Keep simple collision for prototyping, optimize later

## Recommendations

**For Current Game State:**
- ✅ **Keep current DamageSystem** - It works and is debuggable
- ✅ **Profile first** - Measure if collision is a bottleneck
- ✅ **Fix enemy bullets hitting player** - This is the critical bug

**For Future Enhancements:**
- 🔄 **Add spatial hash if entity count grows** (>200 entities)
- 🔄 **Contribute BulletCollisionSystem improvements to engine** (callbacks, light entity support)
- 🔄 **Create generic CollisionSystem** that handles all collision types

## BulletComponent Owner Field - Now Being Used!

The `BulletComponent::Owner` field is **critical** for:
1. **Preventing friendly fire** - Bullets ignore their shooter
2. **Determining damage direction** - Player bullets hurt enemies, enemy bullets hurt player
3. **Attribution** - Track who killed what (for scoring, XP, etc.)

**Fixed implementation:**
```cpp
// In WeaponSystem
auto& bulletComp = bullet.AddComponent<BulletComponent>(owner, damage);  // ✅ Owner set

// In DamageSystem
if (bulletComp.Owner.HasComponent<PlayerTagComponent>()) {
    // Player bullet - check vs enemies
} else if (bulletComp.Owner.HasComponent<EnemyComponent>()) {
    // Enemy bullet - check vs player  ✅ NOW IMPLEMENTED
}
```

## Summary

The custom collision approach is **appropriate for the current game scale**. Pillar's advanced systems (BulletCollisionSystem, SpatialHashGrid) are available for optimization when needed, but premature optimization would add complexity without measurable benefit at 50-100 entity counts.

**Key takeaway:** The engine provides tools, but the game chooses when to use them based on actual performance needs.
