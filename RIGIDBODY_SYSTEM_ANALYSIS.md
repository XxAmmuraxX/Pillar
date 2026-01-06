# RigidbodyComponent System Analysis & Improvement Plan

**Date:** January 1, 2026  
**Context:** Following successful ColliderComponent improvements, analyzing RigidbodyComponent for editor UX enhancements and missing features.

---

## Executive Summary

The **RigidbodyComponent** is the core physics component that transforms an entity into a "Heavy Entity" with a Box2D body. The current implementation is **functional** but has several areas for improvement:

✅ **Working Well:**
- Body type selection (Static, Kinematic, Dynamic) with clear tooltips
- Gravity scale control with presets
- Fixed rotation toggle
- Body recreation handled automatically by PhysicsSystem
- Clean integration with Box2D

⚠️ **Needs Improvement:**
- No visual feedback about body state (sleeping, awake, active)
- Missing runtime velocity/force controls
- No mass/inertia visibility or editing
- Limited debug information
- No presets for common configurations (player, enemy, pickup, etc.)
- No visual indicators in viewport (unlike colliders)
- Cannot apply forces/impulses in editor during play mode
- No linear/angular damping controls
- No bullet mode toggle for fast-moving objects

---

## Current Implementation Analysis

### Component Structure
```cpp
struct RigidbodyComponent
{
    b2Body* Body = nullptr;              // Owned by b2World
    b2BodyType BodyType = b2_dynamicBody;
    bool FixedRotation = false;
    float GravityScale = 1.0f;
};
```

**Observations:**
- Minimalist design - only stores essential data
- Box2D `b2Body*` pointer is managed by PhysicsSystem
- Component itself is non-copyable (due to unique body pointer)
- Additional properties (mass, damping, velocity) are stored on `b2Body` directly

### Inspector UI Current Features

**Body Type Dropdown:**
- ✅ Three clear options with descriptions
- ✅ Helpful tooltip
- ✅ Visual icons in description text (🧱, 🎮, ⚙)
- ✅ Automatic body recreation when changed

**Gravity Scale:**
- ✅ Drag control with range 0-10
- ✅ Quick presets (0, 0.5, 1, 2)
- ✅ Clear tooltip
- ⚠️ Missing negative gravity option (for reverse gravity games)

**Fixed Rotation:**
- ✅ Simple checkbox
- ✅ Lock icon in tooltip
- ⚠️ No visual feedback when active

### PhysicsSystem Integration

**Body Creation:**
```cpp
rigidbody.Body = Box2DBodyFactory::CreateBody(
    m_World->GetWorld(),
    transform.Position,
    transform.Rotation,
    rigidbody.BodyType,
    rigidbody.FixedRotation,
    rigidbody.GravityScale
);
```

**Body Destruction:**
- Handled in `PhysicsSystem::OnDetach()`
- Bodies destroyed when play mode stops
- Component's `Body` pointer cleared

**Sync Behavior:**
- Kinematic bodies: ECS → Box2D (controlled by scripts)
- Dynamic bodies: Box2D → ECS (controlled by physics)
- Static bodies: No sync needed

---

## Missing Features & Pain Points

### 1. **Runtime Velocity Controls** (High Priority)
**Problem:** Cannot see or edit body velocity in inspector during play mode.

**Impact:**
- Hard to debug movement issues
- Cannot test different velocities quickly
- No way to "nudge" objects during testing

**What's Needed:**
- Display current linear velocity (read-only or editable)
- Display current angular velocity
- "Apply Force" and "Apply Impulse" buttons for testing
- Velocity magnitude visualization
- Quick preset buttons (Stop, Jump, Launch, etc.)

### 2. **Mass & Inertia Information** (High Priority)
**Problem:** Cannot see computed mass from fixtures.

**Impact:**
- No visibility into why objects behave differently
- Cannot understand mass distribution
- Hard to balance physics interactions

**What's Needed:**
- Display computed mass (from fixtures)
- Display center of mass
- Display inertia value
- Option to override mass manually (for special cases)

### 3. **Damping Controls** (Medium Priority)
**Problem:** No way to control linear/angular damping (air resistance).

**Impact:**
- Objects never slow down (unrealistic)
- Cannot create "floaty" or "heavy" feel
- Must be set in code, not in editor

**What's Needed:**
- Linear damping slider (0-10)
- Angular damping slider (0-10)
- Presets (None, Light, Heavy, Water)
- Tooltip explaining effect

### 4. **Body State Visualization** (Medium Priority)
**Problem:** No visual feedback about body state.

**Impact:**
- Cannot tell if body is sleeping (performance optimization)
- Don't know if body is awake/active
- No indication of kinematic vs dynamic behavior

**What's Needed:**
- Status indicator (Awake/Sleeping/Inactive)
- Colored icon or badge
- Wake/Sleep buttons for testing
- Viewport gizmo showing body state

### 5. **Bullet Mode** (Medium Priority)
**Problem:** Fast-moving objects tunnel through thin walls.

**Impact:**
- Bullets pass through targets
- Fast players clip through walls
- Poor gameplay for action games

**What's Needed:**
- "Bullet Mode" checkbox (enables CCD)
- Tooltip explaining continuous collision detection
- Warning if body is fast but bullet mode disabled

### 6. **Physics Material Presets** (Low Priority)
**Problem:** Have to manually configure common body types.

**Impact:**
- Repetitive work for common patterns
- Inconsistent configurations across entities
- No standardized "player" or "enemy" setup

**What's Needed:**
- Preset buttons (Player, Enemy, Projectile, Pickup, Platform, etc.)
- Each preset configures: BodyType, FixedRotation, GravityScale, Damping
- User-defined preset system (save/load)

### 7. **Viewport Visualization** (Low Priority)
**Problem:** Colliders have gizmos, but rigidbodies don't.

**Impact:**
- Cannot visually distinguish physics vs non-physics entities
- No indication of body type at a glance
- Velocity vectors not visible

**What's Needed:**
- Optional gizmo showing body type (different colors)
- Velocity vector arrows during play mode
- Center of mass indicator
- Force/impulse visualization

### 8. **Advanced Properties** (Low Priority)
**Problem:** Some Box2D properties not exposed.

**Impact:**
- Cannot fine-tune advanced behaviors
- Limited control over collision response
- Must modify code for edge cases

**What's Needed:**
- Sleep threshold configuration
- "Allow Sleep" toggle
- "Enabled" toggle (disable physics without removing component)
- "Is Awake" toggle (for initial state)

---

## Comparison with Industry Tools

### Unity Rigidbody Inspector
**Features we're missing:**
- ✅ Mass display/editing
- ✅ Drag (linear damping)
- ✅ Angular Drag
- ✅ Use Gravity toggle
- ✅ Is Kinematic toggle
- ✅ Interpolation options
- ✅ Collision Detection mode
- ✅ Constraints (freeze position/rotation axes)

### Godot RigidBody2D Inspector
**Features we're missing:**
- ✅ Mass/Inertia display
- ✅ Linear/Angular Damp
- ✅ "Can Sleep" toggle
- ✅ Lock Rotation (we have this!)
- ✅ Gravity Scale (we have this!)

### Box2D Testbed
**Debug features we're missing:**
- ✅ Body state visualization (awake/sleeping)
- ✅ Velocity vectors
- ✅ Center of mass dots
- ✅ Contact normals and points
- ✅ AABB visualization

**What we're doing better:**
- ✅ Cleaner preset system
- ✅ Better tooltips/documentation
- ✅ More intuitive layout

---

## Proposed Improvements (Prioritized)

### Phase 1: Essential Runtime Controls (2-3 hours)
**Goal:** Make rigidbody debugging and testing easier.

**Tasks:**
1. Add velocity display section (during play mode only)
   - Linear velocity (X, Y) with magnitude
   - Angular velocity
   - "Reset Velocity" button
   - "Apply Impulse" mini-tool (direction + strength)

2. Add mass/inertia information
   - Computed mass from fixtures (read-only)
   - Computed inertia (read-only)
   - Center of mass display
   - "Override Mass" advanced option

3. Add body state indicator
   - Status badge (Awake/Sleeping/Inactive)
   - Different colors for states
   - "Wake" and "Allow Sleep" buttons

**Deliverable:** Editor can inspect and manipulate rigidbody state during play mode.

---

### Phase 2: Damping & Advanced Controls (1-2 hours)
**Goal:** Expose Box2D damping and bullet mode.

**Tasks:**
1. Add damping controls
   - Linear damping slider (0-5, default 0)
   - Angular damping slider (0-5, default 0)
   - Tooltip with examples
   - Presets: None (0), Light (0.5), Heavy (2.0), Water (5.0)

2. Add bullet mode checkbox
   - Enabled CCD for fast-moving objects
   - Warning icon if velocity > threshold and bullet mode off
   - Tooltip explaining tunneling problem

3. Add "Enabled" toggle
   - Allows temporarily disabling physics without removing component
   - Useful for cutscenes or special states

**Deliverable:** Full control over Box2D body properties.

---

### Phase 3: Presets & Templates (1-2 hours)
**Goal:** Quick setup for common entity types.

**Tasks:**
1. Add preset buttons in component header
   - "🎮 Player" - Dynamic, FixedRotation=true, GravityScale=1, Damping=0.5
   - "🤖 Enemy" - Dynamic, FixedRotation=false, GravityScale=1, Damping=0.3
   - "🚀 Projectile" - Dynamic, Bullet=true, GravityScale=0, Damping=0
   - "📦 Crate" - Dynamic, GravityScale=1, Damping=0.1
   - "🛑 Platform" - Kinematic, FixedRotation=true, GravityScale=0
   - "🧱 Wall" - Static, all defaults

2. Add "Save as Preset" feature
   - User can save current configuration
   - Stored in project settings or separate file
   - Load custom presets from dropdown

**Deliverable:** Rapid prototyping with one-click configurations.

---

### Phase 4: Viewport Visualization (2-3 hours)
**Goal:** Visual feedback in scene viewport.

**Tasks:**
1. Add rigidbody gizmo rendering (in `ViewportPanel::DrawPhysicsGizmos()`)
   - Body type indicator:
     - Static: Gray filled circle at center
     - Kinematic: Blue arrow showing movement
     - Dynamic: Green circle outline
   - Different colors based on state:
     - Awake: Bright green
     - Sleeping: Dim gray
     - Inactive: Red X

2. Add velocity visualization (play mode only)
   - Draw arrow from center showing linear velocity
   - Arrow length = velocity magnitude (scaled)
   - Arrow color based on speed (green → yellow → red)
   - Angular velocity shown as rotating arc

3. Add center of mass indicator
   - Small crosshair at center of mass
   - Only shown when gizmo is enabled

4. Add viewport toggle for physics gizmos
   - Checkbox in toolbar or menu
   - "Show Physics Gizmos" option
   - Separate from collider gizmos

**Deliverable:** Full visual debugging of physics in viewport.

---

### Phase 5: Integration & Polish (1 hour)
**Goal:** Refine UX and add documentation.

**Tasks:**
1. Add warnings/validation
   - Warn if dynamic body has no collider
   - Warn if kinematic body has collider with density
   - Info tooltip if body is sleeping (performance win)

2. Add contextual help
   - "?" button linking to docs
   - Common issues and solutions
   - Examples for each body type

3. Performance indicators
   - Show body count in scene
   - Sleeping vs awake ratio
   - Physics step time

**Deliverable:** Production-ready rigidbody system with excellent UX.

---

## Technical Implementation Notes

### Accessing Box2D Properties
```cpp
// Current velocity
glm::vec2 linearVel(rb.Body->GetLinearVelocity().x, rb.Body->GetLinearVelocity().y);
float angularVel = rb.Body->GetAngularVelocity();

// Mass & inertia
float mass = rb.Body->GetMass();
float inertia = rb.Body->GetInertia();
b2Vec2 centerOfMass = rb.Body->GetWorldCenter();

// State
bool isAwake = rb.Body->IsAwake();
bool isBullet = rb.Body->IsBullet();
bool isEnabled = rb.Body->IsEnabled();

// Damping
float linearDamping = rb.Body->GetLinearDamping();
float angularDamping = rb.Body->GetAngularDamping();
```

### Setting Box2D Properties
```cpp
// Velocity
rb.Body->SetLinearVelocity(b2Vec2(x, y));
rb.Body->SetAngularVelocity(omega);

// Damping
rb.Body->SetLinearDamping(0.5f);
rb.Body->SetAngularDamping(0.3f);

// Bullet mode
rb.Body->SetBullet(true);

// Enable/Disable
rb.Body->SetEnabled(false);

// Wake/Sleep
rb.Body->SetAwake(true);
rb.Body->SetSleepingAllowed(true);
```

### Component Data Extension
To avoid bloating the component, consider:
1. **Option A:** Store additional properties on `b2Body` directly (current approach)
   - Pros: No memory overhead, direct Box2D integration
   - Cons: Properties not visible in serialization
   
2. **Option B:** Extend `RigidbodyComponent` with cached values
   - Pros: Easier serialization, visible when body is null
   - Cons: Memory overhead, sync complexity

**Recommendation:** Keep current approach, but add helper methods to sync component ↔ body.

---

## Implementation Strategy

Following the same pattern as ColliderComponent:

1. **Start with Phase 1** (runtime controls)
   - Most immediately useful
   - Builds foundation for other features
   - Easy to test and validate

2. **Implement Phase 2** (damping & advanced)
   - Natural extension of Phase 1
   - Completes Box2D property exposure

3. **Add Phase 3** (presets) if time permits
   - Big UX win, minimal complexity
   - Can be done independently

4. **Phase 4 & 5** are optional enhancements
   - Viewport gizmos are nice-to-have
   - Can be revisited later based on feedback

---

## Success Criteria

✅ **Phase 1 Complete When:**
- Can view and edit velocity during play mode
- Mass/inertia displayed in inspector
- Body state (awake/sleeping) visible
- Can apply test impulses in editor

✅ **Phase 2 Complete When:**
- Damping controls functional
- Bullet mode toggle working
- All Box2D properties exposed

✅ **Phase 3 Complete When:**
- 6+ presets available
- Can save custom presets
- One-click entity setup works

---

## Next Steps

**Recommended Action:**
Start with **Phase 1: Essential Runtime Controls** - this provides immediate value for debugging and matches the workflow we established with ColliderComponent.

**Estimated Time:** 2-3 hours for Phase 1, with clear stopping points.

**Question for User:** Shall we proceed with Phase 1 (velocity controls + mass display + state indicator)?
