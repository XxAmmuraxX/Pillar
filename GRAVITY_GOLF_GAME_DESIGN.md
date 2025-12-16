# Gravity Golf - Game Design Document

**A Physics-Based Puzzle Game Showcasing the Pillar Engine**

---

## Executive Summary

**Gravity Golf** is a physics-driven mini-golf puzzle game where players launch a ball through increasingly challenging courses featuring gravity wells, force fields, bounce pads, and moving obstacles. The game provides satisfying "one more try" gameplay while showcasing the Pillar Engine's physics, particles, audio, and rendering capabilities in a cohesive, polished experience.

**Why This Design?**
- **Physics showcase**: Ball physics, collisions, forces, gravity manipulation
- **Particle showcase**: Trail effects, explosions, ambient effects, goal celebrations
- **Audio showcase**: Satisfying SFX for hits, bounces, goals, ambient sounds
- **Simple but deep**: Easy to understand, hard to master
- **Replayable**: Par system, star ratings, time challenges
- **Uses existing assets**: Repurposes current textures/audio creatively

---

## Core Gameplay Loop

```
┌─────────────────────────────────────────────────────────┐
│                     LEVEL START                         │
│                                                         │
│  1. Ball spawns at start position                       │
│  2. Player aims using mouse (direction + power)         │
│  3. Player clicks to launch ball                        │
│  4. Ball interacts with physics objects:                │
│     - Walls (bounce)                                    │
│     - Gravity wells (attract/repel)                     │
│     - Boost pads (accelerate)                           │
│     - Moving platforms (timing puzzles)                 │
│     - Obstacles (avoid)                                 │
│  5. Ball enters goal → Level complete!                  │
│     OR ball goes out of bounds → Respawn at last shot   │
│                                                         │
│  Scoring: Fewer shots = more stars (3 star max)         │
└─────────────────────────────────────────────────────────┘
```

---

## Engine Features Showcased

| Feature | Implementation |
|---------|----------------|
| **Box2D Physics** | Ball movement, collisions, forces, restitution |
| **Particle System** | Ball trails, explosions, ambient effects, goal burst |
| **Audio System** | Hit sounds, bounce SFX, ambient music, victory jingle |
| **2D Renderer** | Sprites, backgrounds, UI, particle rendering |
| **ECS Architecture** | Clean entity/component separation |
| **Input System** | Mouse aiming, click to launch, keyboard shortcuts |
| **Camera System** | Smooth follow, level overview, zoom |

---

## Game Objects & Components

### 1. Golf Ball (Player-Controlled)

```cpp
struct GolfBallComponent
{
    int ShotCount = 0;          // Strokes taken this level
    float MaxPower = 15.0f;     // Maximum launch velocity
    float MinPower = 2.0f;      // Minimum launch velocity
    bool IsMoving = false;      // Can only shoot when stopped
    bool InGoal = false;        // Level complete flag
    glm::vec2 LastPosition;     // For respawn on out-of-bounds
};

// Entity setup:
// - TransformComponent (position, rotation)
// - RigidbodyComponent (dynamic body)
// - ColliderComponent (circle, bouncy)
// - SpriteComponent (ball texture)
// - ParticleEmitterComponent (trail effect)
// - GolfBallComponent (game logic)
```

**Physics Properties:**
- Circle collider (radius: 0.3 units)
- High restitution (0.7) for satisfying bounces
- Low friction (0.1) for smooth rolling
- Linear damping (0.5) so ball eventually stops

### 2. Goal Hole

```cpp
struct GoalComponent
{
    float CaptureRadius = 0.8f;   // Ball must be within this distance
    float CaptureSpeed = 3.0f;    // Ball must be slower than this
    int ParScore = 3;             // Par for this level
    bool Captured = false;
};

// When ball enters goal zone AND is slow enough:
// 1. Play victory sound
// 2. Spawn celebration particles
// 3. Show score UI
// 4. Transition to next level
```

### 3. Gravity Well (Attractor/Repulsor)

```cpp
struct GravityWellComponent
{
    float Strength = 20.0f;      // Force magnitude
    float Radius = 5.0f;         // Effect radius
    bool IsRepulsor = false;     // false = attract, true = repel
    bool Enabled = true;
    
    // Visual feedback
    float PulseTimer = 0.0f;
    glm::vec4 Color;             // Blue for attract, Orange for repel
};

// Physics: Apply force each frame to ball if within radius
// F = strength * (1 - distance/radius)^2 * direction
// Creates smooth falloff at edges
```

**Visual Design:**
- Attractors: Blue swirling particle effect (uses `particle_circle.png`)
- Repulsors: Orange pulsing particle effect (uses `particle_spark.png`)
- Pulsing glow indicates strength

### 4. Bounce Pad

```cpp
struct BouncePadComponent
{
    float BoostMultiplier = 2.0f;   // Velocity multiplier on contact
    glm::vec2 Direction = {0, 1};   // Direction of boost (normalized)
    bool DirectionalBoost = false;  // false = reflect, true = redirect
};

// On collision:
// - Play bounce sound (hit.wav)
// - Spawn spark particles
// - Apply velocity boost
```

### 5. Moving Platform

```cpp
struct MovingPlatformComponent
{
    glm::vec2 StartPosition;
    glm::vec2 EndPosition;
    float Speed = 2.0f;
    float PauseTime = 0.5f;       // Pause at each end
    bool PingPong = true;         // Go back and forth
    
    // State
    float Progress = 0.0f;        // 0 to 1
    bool Forward = true;
    float PauseTimer = 0.0f;
};

// Uses kinematic body - moves via code, pushes dynamic bodies
```

### 6. Obstacle (Hazard)

```cpp
struct ObstacleComponent
{
    enum class Type { Static, Spinning, Oscillating };
    Type ObstacleType = Type::Static;
    float RotationSpeed = 90.0f;  // Degrees per second (for Spinning)
    float OscillationAmplitude = 2.0f;
    float OscillationFrequency = 1.0f;
};

// Uses existing crate.png, pillar.png, wall.png textures
```

### 7. Out of Bounds Zone

```cpp
struct OutOfBoundsComponent
{
    // Sensor collider - no physics response
    // On contact: respawn ball at last shot position
    // Play "whoosh" sound effect
    // Small penalty particle effect
};
```

---

## Level Structure

### Tutorial Levels (1-3): Teach Core Mechanics

**Level 1: "First Shot"**
- Straight line from ball to goal
- Teaches: aiming, power control
- Par: 1

**Level 2: "Corner Pocket"**
- One wall bounce required
- Teaches: wall physics, planning trajectory
- Par: 2

**Level 3: "Gravity Assist"**
- Single attractor gravity well
- Teaches: using gravity to curve shots
- Par: 2

### Easy Levels (4-6): Simple Puzzles

**Level 4: "Push and Pull"**
- Attractor and repulsor in sequence
- Navigate between opposing forces
- Par: 3

**Level 5: "Bounce House"**
- Multiple bounce pads
- Chain bounces to reach goal
- Par: 3

**Level 6: "Moving Target"**
- Moving platform with goal on it
- Timing required
- Par: 3

### Medium Levels (7-9): Combined Mechanics

**Level 7: "Gravity Maze"**
- Multiple gravity wells creating a path
- Must curve through without hitting walls
- Par: 4

**Level 8: "Pinball Wizard"**
- Multiple bounce pads + obstacles
- Fast-paced bouncing
- Par: 4

**Level 9: "Timing is Everything"**
- Moving platforms + gravity wells
- Requires precise timing
- Par: 4

### Hard Levels (10-12): Master Challenges

**Level 10: "The Gauntlet"**
- All mechanics combined
- Long complex path
- Par: 5

**Level 11: "Gravity Storm"**
- Many gravity wells, some toggling on/off
- Chaotic but solvable
- Par: 6

**Level 12: "The Final Hole"**
- Everything at once
- Multiple valid solutions
- Par: 7

---

## Aiming System

```cpp
class AimingSystem
{
public:
    void OnUpdate(float dt, Entity ball, glm::vec2 mouseWorldPos)
    {
        if (!CanShoot(ball)) return;
        
        auto& transform = ball.GetComponent<TransformComponent>();
        glm::vec2 ballPos = transform.Position;
        
        // Calculate direction from ball to mouse
        glm::vec2 direction = mouseWorldPos - ballPos;
        float distance = glm::length(direction);
        
        if (distance > 0.1f)
        {
            direction = glm::normalize(direction);
            
            // Power based on distance (clamped)
            float power = glm::clamp(distance, MinPower, MaxPower);
            
            // Store for rendering aim line
            m_AimDirection = direction;
            m_AimPower = power;
            m_AimValid = true;
        }
    }
    
    void RenderAimLine()
    {
        if (!m_AimValid) return;
        
        glm::vec2 ballPos = m_Ball.GetComponent<TransformComponent>().Position;
        glm::vec2 endPos = ballPos + m_AimDirection * (m_AimPower * 0.3f);
        
        // Draw dotted line from ball to aim point
        // Color indicates power (green = weak, yellow = medium, red = strong)
        glm::vec4 color = GetPowerColor(m_AimPower);
        
        const int dots = 10;
        for (int i = 0; i < dots; ++i)
        {
            float t = (float)i / dots;
            glm::vec2 dotPos = glm::mix(ballPos, endPos, t);
            Renderer2DBackend::DrawQuad(dotPos, {0.08f, 0.08f}, color);
        }
        
        // Arrow head at end
        DrawArrowHead(endPos, m_AimDirection, color);
    }
    
    void Shoot()
    {
        if (!m_AimValid) return;
        
        auto& rb = m_Ball.GetComponent<RigidbodyComponent>();
        glm::vec2 impulse = m_AimDirection * m_AimPower;
        
        // Apply impulse through Box2D
        rb.Body->ApplyLinearImpulseToCenter(b2Vec2(impulse.x, impulse.y), true);
        
        // Update game state
        auto& golf = m_Ball.GetComponent<GolfBallComponent>();
        golf.ShotCount++;
        golf.IsMoving = true;
        golf.LastPosition = m_Ball.GetComponent<TransformComponent>().Position;
        
        // Play shoot sound
        m_ShootSound->Play();
        
        // Spawn muzzle particles
        SpawnShootParticles(golf.LastPosition, m_AimDirection);
    }
    
private:
    glm::vec4 GetPowerColor(float power)
    {
        float t = (power - MinPower) / (MaxPower - MinPower);
        if (t < 0.5f)
            return glm::mix(glm::vec4(0,1,0,1), glm::vec4(1,1,0,1), t * 2.0f);
        else
            return glm::mix(glm::vec4(1,1,0,1), glm::vec4(1,0,0,1), (t-0.5f) * 2.0f);
    }
};
```

---

## Particle Effects

### 1. Ball Trail

```cpp
ParticleEmitterComponent CreateBallTrail()
{
    ParticleEmitterComponent trail;
    trail.Enabled = true;
    trail.EmissionRate = 30.0f;           // Particles per second
    trail.Shape = EmissionShape::Point;
    trail.Direction = {0, 0};             // Inherit ball velocity
    trail.DirectionSpread = 15.0f;
    trail.Speed = 0.5f;
    trail.SpeedVariance = 0.2f;
    trail.Lifetime = 0.5f;
    trail.LifetimeVariance = 0.1f;
    trail.Size = 0.15f;
    trail.SizeVariance = 0.05f;
    trail.StartColor = {1.0f, 1.0f, 1.0f, 0.8f};  // White, semi-transparent
    trail.FadeOut = true;
    trail.ScaleOverTime = true;
    trail.EndScale = 0.2f;
    trail.Gravity = {0, 0};               // No gravity on trail
    trail.TexturePath = "textures/particle_circle.png";
    return trail;
}
```

### 2. Gravity Well Ambient Effect

```cpp
ParticleEmitterComponent CreateGravityWellEffect(bool isRepulsor)
{
    ParticleEmitterComponent effect;
    effect.EmissionRate = 20.0f;
    effect.Shape = EmissionShape::Circle;
    effect.ShapeSize = {2.0f, 2.0f};      // Spawn in ring
    
    if (isRepulsor)
    {
        effect.Direction = {0, 0};         // Outward from center
        effect.StartColor = {1.0f, 0.5f, 0.0f, 0.6f};  // Orange
    }
    else
    {
        effect.Direction = {0, 0};         // Inward to center (negative speed)
        effect.Speed = -2.0f;              // Move toward center
        effect.StartColor = {0.3f, 0.5f, 1.0f, 0.6f};  // Blue
    }
    
    effect.Lifetime = 1.5f;
    effect.FadeOut = true;
    effect.Size = 0.2f;
    effect.TexturePath = "textures/particle_spark.png";
    return effect;
}
```

### 3. Goal Celebration Burst

```cpp
ParticleEmitterComponent CreateGoalCelebration()
{
    ParticleEmitterComponent burst;
    burst.BurstMode = true;
    burst.BurstCount = 50;
    burst.Shape = EmissionShape::Point;
    burst.Direction = {0, 1};
    burst.DirectionSpread = 180.0f;        // All directions
    burst.Speed = 8.0f;
    burst.SpeedVariance = 3.0f;
    burst.Lifetime = 1.0f;
    burst.LifetimeVariance = 0.3f;
    burst.Size = 0.25f;
    burst.SizeVariance = 0.1f;
    burst.StartColor = {1.0f, 0.9f, 0.2f, 1.0f};  // Gold
    burst.ColorVariance = {0.2f, 0.2f, 0.0f, 0.0f};
    burst.FadeOut = true;
    burst.ScaleOverTime = true;
    burst.EndScale = 0.1f;
    burst.Gravity = {0, -5.0f};            // Fall down
    burst.TexturePath = "textures/particle_spark.png";
    return burst;
}
```

### 4. Bounce Impact Sparks

```cpp
void SpawnBounceParticles(glm::vec2 position, glm::vec2 normal)
{
    // Quick burst of sparks in reflection direction
    ParticleEmitterComponent spark;
    spark.BurstMode = true;
    spark.BurstCount = 8;
    spark.Direction = normal;
    spark.DirectionSpread = 45.0f;
    spark.Speed = 5.0f;
    spark.Lifetime = 0.3f;
    spark.StartColor = {1.0f, 1.0f, 0.5f, 1.0f};
    spark.FadeOut = true;
    spark.Size = 0.1f;
    
    auto entity = m_Scene->CreateEntity("BounceSpark");
    entity.GetComponent<TransformComponent>().Position = position;
    entity.AddComponent<ParticleEmitterComponent>(spark);
}
```

---

## Audio Design

### Sound Effects (Existing Assets Mapping)

| Game Event | Audio File | Notes |
|------------|------------|-------|
| Ball launch | `shoot.wav` | Satisfying "thwack" |
| Wall bounce | `hit.wav` | Quick impact |
| Bounce pad hit | `dash.wav` | Energetic boost |
| Enter gravity well | `enemy_shoot.wav` (pitched down) | Wooshy |
| Goal capture | `level_up.wav` | Celebratory |
| Out of bounds | `explosion.wav` (quiet) | Failure indicator |
| UI click | `pickup.wav` | Menu feedback |
| Perfect score (1 shot) | `victory.wav` | Rare achievement |

### Music

| Context | Audio File | Loop |
|---------|------------|------|
| Menu | `background_music.wav` | Yes |
| Gameplay | `chamber_ambient.wav` | Yes |
| Victory screen | `victory.wav` | No |
| Game over | `game_over.wav` | No |

### Audio Implementation

```cpp
class GameAudio
{
public:
    void Init()
    {
        m_ShootSound = AudioClip::Create("audio/shoot.wav");
        m_BounceSound = AudioClip::Create("audio/hit.wav");
        m_BoostSound = AudioClip::Create("audio/dash.wav");
        m_GoalSound = AudioClip::Create("audio/level_up.wav");
        m_OutOfBoundsSound = AudioClip::Create("audio/explosion.wav");
        m_OutOfBoundsSound->SetVolume(0.3f);  // Quieter
        
        m_AmbientMusic = AudioClip::Create("audio/chamber_ambient.wav");
        m_AmbientMusic->SetLooping(true);
        m_AmbientMusic->SetVolume(0.4f);
    }
    
    void PlayShoot() { m_ShootSound->Play(); }
    void PlayBounce() { m_BounceSound->Play(); }
    void PlayBoost() { m_BoostSound->Play(); }
    void PlayGoal() { m_GoalSound->Play(); }
    void PlayOutOfBounds() { m_OutOfBoundsSound->Play(); }
    
    void StartMusic() { m_AmbientMusic->Play(); }
    void StopMusic() { m_AmbientMusic->Stop(); }
    
private:
    std::shared_ptr<AudioClip> m_ShootSound;
    std::shared_ptr<AudioClip> m_BounceSound;
    std::shared_ptr<AudioClip> m_BoostSound;
    std::shared_ptr<AudioClip> m_GoalSound;
    std::shared_ptr<AudioClip> m_OutOfBoundsSound;
    std::shared_ptr<AudioClip> m_AmbientMusic;
};
```

---

## Physics Implementation

### Ball Physics Setup

```cpp
Entity CreateGolfBall(Scene* scene, glm::vec2 startPos)
{
    auto ball = scene->CreateEntity("GolfBall");
    
    // Transform
    auto& transform = ball.GetComponent<TransformComponent>();
    transform.Position = startPos;
    transform.Scale = {0.6f, 0.6f};
    
    // Physics body
    auto& rb = ball.AddComponent<RigidbodyComponent>(b2_dynamicBody);
    rb.FixedRotation = false;  // Ball can spin
    rb.GravityScale = 0.0f;    // Top-down view, no gravity
    
    // Collider with bounce
    auto& collider = ball.AddComponent<ColliderComponent>(
        ColliderComponent::Circle(0.3f)
    );
    collider.Restitution = 0.7f;   // Bouncy!
    collider.Friction = 0.1f;       // Smooth rolling
    collider.Density = 1.0f;
    
    // Linear damping so ball eventually stops
    // (Set on b2Body after creation)
    
    // Game component
    ball.AddComponent<GolfBallComponent>();
    
    // Trail particles
    ball.AddComponent<ParticleEmitterComponent>(CreateBallTrail());
    
    // Sprite
    auto& sprite = ball.AddComponent<SpriteComponent>();
    sprite.Texture = Texture2D::Create("textures/particle_circle.png");
    sprite.Color = {1.0f, 1.0f, 1.0f, 1.0f};
    
    return ball;
}
```

### Gravity Well Force Application

```cpp
class GravityWellSystem : public System
{
public:
    void OnUpdate(float dt) override
    {
        auto& registry = m_Scene->GetRegistry();
        
        // Get ball entity
        auto ballView = registry.view<GolfBallComponent, TransformComponent, RigidbodyComponent>();
        
        for (auto ballEntity : ballView)
        {
            auto& ballTransform = ballView.get<TransformComponent>(ballEntity);
            auto& ballRb = ballView.get<RigidbodyComponent>(ballEntity);
            
            if (!ballRb.Body) continue;
            
            glm::vec2 ballPos = ballTransform.Position;
            glm::vec2 totalForce = {0, 0};
            
            // Apply force from each gravity well
            auto wellView = registry.view<GravityWellComponent, TransformComponent>();
            
            for (auto wellEntity : wellView)
            {
                auto& well = wellView.get<GravityWellComponent>(wellEntity);
                auto& wellTransform = wellView.get<TransformComponent>(wellEntity);
                
                if (!well.Enabled) continue;
                
                glm::vec2 wellPos = wellTransform.Position;
                glm::vec2 toWell = wellPos - ballPos;
                float distance = glm::length(toWell);
                
                if (distance < well.Radius && distance > 0.1f)
                {
                    // Calculate force with smooth falloff
                    float normalizedDist = distance / well.Radius;
                    float forceMagnitude = well.Strength * (1.0f - normalizedDist * normalizedDist);
                    
                    glm::vec2 direction = glm::normalize(toWell);
                    if (well.IsRepulsor)
                        direction = -direction;
                    
                    totalForce += direction * forceMagnitude;
                }
            }
            
            // Apply accumulated force
            if (glm::length(totalForce) > 0.01f)
            {
                ballRb.Body->ApplyForceToCenter(
                    b2Vec2(totalForce.x, totalForce.y), true
                );
            }
        }
    }
};
```

### Bounce Pad Collision Handler

```cpp
class BouncePadSystem : public System
{
public:
    void OnCollision(Entity ballEntity, Entity padEntity, const b2Contact* contact)
    {
        if (!padEntity.HasComponent<BouncePadComponent>()) return;
        if (!ballEntity.HasComponent<GolfBallComponent>()) return;
        
        auto& pad = padEntity.GetComponent<BouncePadComponent>();
        auto& rb = ballEntity.GetComponent<RigidbodyComponent>();
        
        b2Vec2 velocity = rb.Body->GetLinearVelocity();
        glm::vec2 vel = {velocity.x, velocity.y};
        
        if (pad.DirectionalBoost)
        {
            // Redirect in pad's direction
            float speed = glm::length(vel) * pad.BoostMultiplier;
            vel = pad.Direction * speed;
        }
        else
        {
            // Reflect and boost
            // Get contact normal
            b2WorldManifold manifold;
            contact->GetWorldManifold(&manifold);
            glm::vec2 normal = {manifold.normal.x, manifold.normal.y};
            
            vel = glm::reflect(vel, normal) * pad.BoostMultiplier;
        }
        
        rb.Body->SetLinearVelocity(b2Vec2(vel.x, vel.y));
        
        // Effects
        m_Audio->PlayBoost();
        SpawnBoostParticles(ballEntity.GetComponent<TransformComponent>().Position);
    }
};
```

---

## UI Design

### HUD (During Gameplay)

```
┌─────────────────────────────────────────────────────────┐
│ Level 5: "Bounce House"                     Shots: 2/3  │
│                                                         │
│                                                         │
│                    [GAME AREA]                          │
│                                                         │
│                                                         │
│                                           [R] Restart   │
└─────────────────────────────────────────────────────────┘
```

### Level Complete Screen

```
┌─────────────────────────────────────────────────────────┐
│                                                         │
│                    ★ ★ ★                                │
│                                                         │
│               LEVEL COMPLETE!                           │
│                                                         │
│             Shots: 2    Par: 3                          │
│                                                         │
│         [Next Level]    [Replay]    [Menu]              │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### Star Rating System

```cpp
int CalculateStars(int shots, int par)
{
    if (shots <= par - 1) return 3;  // Under par = 3 stars
    if (shots <= par)     return 2;  // Par = 2 stars
    if (shots <= par + 2) return 1;  // Over par = 1 star
    return 0;                        // Way over = 0 stars
}
```

---

## Main Game Layer Implementation

```cpp
class GravityGolfLayer : public Layer
{
public:
    GravityGolfLayer() 
        : Layer("GravityGolf"), 
          m_CameraController(16.0f / 9.0f, false)
    {}
    
    void OnAttach() override
    {
        // Initialize systems
        m_Scene = std::make_shared<Scene>("GravityGolf");
        
        // Physics with no default gravity (top-down)
        m_PhysicsSystem = std::make_unique<PhysicsSystem>(glm::vec2(0, 0));
        m_PhysicsSystem->OnAttach(m_Scene.get());
        m_Scene->SetPhysicsSystem(m_PhysicsSystem.get());
        
        // Particle systems
        m_ParticlePool.Init(m_Scene.get(), 2000);
        m_ParticleSystem = std::make_unique<ParticleSystem>();
        m_ParticleEmitterSystem = std::make_unique<ParticleEmitterSystem>();
        m_ParticleSystem->OnAttach(m_Scene.get());
        m_ParticleEmitterSystem->OnAttach(m_Scene.get());
        m_ParticleSystem->SetParticlePool(&m_ParticlePool);
        m_ParticleEmitterSystem->SetParticlePool(&m_ParticlePool);
        
        // Game systems
        m_GravityWellSystem = std::make_unique<GravityWellSystem>();
        m_GravityWellSystem->OnAttach(m_Scene.get());
        
        // Audio
        m_Audio.Init();
        m_Audio.StartMusic();
        
        // Load first level
        LoadLevel(1);
    }
    
    void OnUpdate(float dt) override
    {
        // Camera
        m_CameraController.OnUpdate(dt);
        
        // Update ball state (check if stopped)
        UpdateBallState(dt);
        
        // Only update aiming when ball is stopped
        if (CanShoot())
        {
            UpdateAiming();
        }
        
        // Physics
        m_PhysicsSystem->OnUpdate(dt);
        
        // Custom forces (gravity wells)
        m_GravityWellSystem->OnUpdate(dt);
        
        // Particles
        m_ParticleEmitterSystem->OnUpdate(dt);
        m_ParticleSystem->OnUpdate(dt);
        
        // Check win/lose conditions
        CheckGoalCapture();
        CheckOutOfBounds();
        
        // Render
        Render();
    }
    
    void OnEvent(Event& e) override
    {
        m_CameraController.OnEvent(e);
        
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseButtonPressedEvent>(
            [this](MouseButtonPressedEvent& e) {
                if (e.GetMouseButton() == PIL_MOUSE_BUTTON_LEFT && CanShoot())
                {
                    Shoot();
                    return true;
                }
                return false;
            }
        );
        
        dispatcher.Dispatch<KeyPressedEvent>(
            [this](KeyPressedEvent& e) {
                if (e.GetKeyCode() == PIL_KEY_R)
                {
                    RestartLevel();
                    return true;
                }
                if (e.GetKeyCode() == PIL_KEY_ESCAPE)
                {
                    // Return to menu
                    return true;
                }
                return false;
            }
        );
    }
    
    void OnImGuiRender() override
    {
        // Simple HUD
        ImGui::SetNextWindowPos({10, 10});
        ImGui::SetNextWindowSize({300, 60});
        ImGui::Begin("HUD", nullptr, 
            ImGuiWindowFlags_NoTitleBar | 
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBackground);
        
        ImGui::Text("Level %d: %s", m_CurrentLevel, m_LevelName.c_str());
        
        auto& golf = m_Ball.GetComponent<GolfBallComponent>();
        ImGui::Text("Shots: %d / %d (Par)", golf.ShotCount, m_Par);
        
        ImGui::End();
        
        // Level complete overlay
        if (m_LevelComplete)
        {
            RenderLevelCompleteUI();
        }
    }
    
private:
    // Scene & Systems
    std::shared_ptr<Scene> m_Scene;
    std::unique_ptr<PhysicsSystem> m_PhysicsSystem;
    std::unique_ptr<ParticleSystem> m_ParticleSystem;
    std::unique_ptr<ParticleEmitterSystem> m_ParticleEmitterSystem;
    std::unique_ptr<GravityWellSystem> m_GravityWellSystem;
    ParticlePool m_ParticlePool;
    
    // Camera
    OrthographicCameraController m_CameraController;
    
    // Game entities
    Entity m_Ball;
    Entity m_Goal;
    std::vector<Entity> m_GravityWells;
    std::vector<Entity> m_BouncePads;
    std::vector<Entity> m_Obstacles;
    std::vector<Entity> m_Walls;
    
    // Level state
    int m_CurrentLevel = 1;
    std::string m_LevelName;
    int m_Par = 3;
    bool m_LevelComplete = false;
    int m_StarsEarned = 0;
    
    // Aiming
    glm::vec2 m_AimDirection;
    float m_AimPower;
    bool m_AimValid = false;
    
    // Audio
    GameAudio m_Audio;
    
    // Helper methods
    void LoadLevel(int levelNum);
    void RestartLevel();
    void Shoot();
    bool CanShoot();
    void UpdateBallState(float dt);
    void UpdateAiming();
    void CheckGoalCapture();
    void CheckOutOfBounds();
    void Render();
    void RenderAimLine();
    void RenderLevelCompleteUI();
    void OnLevelComplete();
};
```

---

## Level Data Format (JSON)

```json
{
    "name": "Gravity Assist",
    "par": 2,
    "ballStart": [0, -5],
    "goalPosition": [0, 5],
    "walls": [
        {"start": [-8, -7], "end": [8, -7]},
        {"start": [-8, 7], "end": [8, 7]},
        {"start": [-8, -7], "end": [-8, 7]},
        {"start": [8, -7], "end": [8, 7]}
    ],
    "gravityWells": [
        {
            "position": [0, 0],
            "strength": 15,
            "radius": 4,
            "isRepulsor": false
        }
    ],
    "bouncePads": [],
    "obstacles": [],
    "movingPlatforms": []
}
```

---

## Polish Features

### 1. Ball Velocity-Based Trail

```cpp
void UpdateBallTrail(Entity ball)
{
    auto& rb = ball.GetComponent<RigidbodyComponent>();
    auto& emitter = ball.GetComponent<ParticleEmitterComponent>();
    
    float speed = glm::length(glm::vec2(
        rb.Body->GetLinearVelocity().x,
        rb.Body->GetLinearVelocity().y
    ));
    
    // More particles when moving fast
    emitter.EmissionRate = glm::clamp(speed * 5.0f, 0.0f, 50.0f);
    
    // Trail color based on speed
    float t = glm::clamp(speed / 15.0f, 0.0f, 1.0f);
    emitter.StartColor = glm::mix(
        glm::vec4(1, 1, 1, 0.5f),   // White when slow
        glm::vec4(1, 0.5f, 0, 0.8f), // Orange when fast
        t
    );
}
```

### 2. Screen Shake on Impact

```cpp
void ApplyScreenShake(float intensity)
{
    m_ScreenShakeIntensity = intensity;
    m_ScreenShakeTimer = 0.2f;  // 200ms shake
}

void UpdateScreenShake(float dt)
{
    if (m_ScreenShakeTimer > 0)
    {
        m_ScreenShakeTimer -= dt;
        
        float t = m_ScreenShakeTimer / 0.2f;
        float shake = m_ScreenShakeIntensity * t;
        
        glm::vec2 offset = glm::vec2(
            (rand() / (float)RAND_MAX - 0.5f) * shake,
            (rand() / (float)RAND_MAX - 0.5f) * shake
        );
        
        m_CameraController.GetCamera().SetPosition(
            m_BaseCameraPosition + glm::vec3(offset, 0)
        );
    }
}
```

### 3. Trajectory Preview (Ghost Ball)

```cpp
void RenderTrajectoryPreview()
{
    if (!CanShoot()) return;
    
    // Simulate ball physics for ~1 second
    glm::vec2 pos = m_Ball.GetComponent<TransformComponent>().Position;
    glm::vec2 vel = m_AimDirection * m_AimPower;
    
    const int steps = 60;
    const float stepDt = 1.0f / 60.0f;
    
    for (int i = 0; i < steps; ++i)
    {
        // Apply gravity wells
        for (auto& well : m_GravityWells)
        {
            auto& wellComp = well.GetComponent<GravityWellComponent>();
            auto& wellPos = well.GetComponent<TransformComponent>().Position;
            // ... apply force to vel
        }
        
        pos += vel * stepDt;
        vel *= 0.99f;  // Approximate damping
        
        // Draw ghost dot
        float alpha = 1.0f - (float)i / steps;
        Renderer2DBackend::DrawQuad(
            pos, {0.1f, 0.1f}, 
            {1, 1, 1, alpha * 0.3f}
        );
    }
}
```

---

## Asset Mapping (Using Existing Textures)

| Game Object | Texture File | Notes |
|-------------|--------------|-------|
| Golf Ball | `particle_circle.png` | White, tinted |
| Goal Hole | `xp_gem_large.png` | Gold, scaled up |
| Gravity Well (Attract) | `particle_spark.png` | Blue tint |
| Gravity Well (Repel) | `particle_spark.png` | Orange tint |
| Bounce Pad | `charger.png` | Orange, arrow shape |
| Wall | `wall.png` | Tiled |
| Obstacle | `crate.png` or `pillar_logo.png` | Various |
| Moving Platform | `turret.png` | Gray platform |
| Ball Trail | `particle_circle.png` | Small, fading |
| Impact Sparks | `particle_spark.png` | Burst effect |

---

## Summary: Why This Design Works

1. **Physics Showcase**: Every shot involves real physics - forces, collisions, momentum, damping. Gravity wells add complex force field interactions.

2. **Particle Showcase**: Constant visual feedback - ball trails, gravity well effects, impact sparks, victory bursts. All using the engine's particle emitter system.

3. **Audio Showcase**: Satisfying sound design with impacts, music, and spatial awareness. Every action has audio feedback.

4. **Simple Controls**: Just aim and click. Anyone can play immediately.

5. **Depth Through Mechanics**: Combining gravity wells, bounce pads, and moving platforms creates puzzle depth without complexity.

6. **Polish Through Juice**: Screen shake, trajectory preview, particle effects, and sound make every interaction feel good.

7. **Uses Existing Assets**: All textures and audio already in the project are repurposed creatively.

8. **12 Levels**: Enough content to demonstrate all mechanics while keeping scope manageable.

---

## Implementation Priority

1. **Phase 1**: Core ball physics + shooting + goal detection
2. **Phase 2**: Walls + basic collisions + first 3 levels
3. **Phase 3**: Gravity wells + particle trails
4. **Phase 4**: Bounce pads + audio integration
5. **Phase 5**: Moving platforms + obstacles
6. **Phase 6**: UI + scoring + level progression
7. **Phase 7**: Polish (screen shake, trajectory preview, juice)
