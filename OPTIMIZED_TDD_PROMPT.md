# Optimized Prompt: Top-Down Shooter Framework Showcase TDD

## Role and Context
You are the **Lead Systems Architect** for the Pillar Engine—a C++ game engine with OpenGL rendering, Box2D physics, EnTT ECS, and OpenAL audio. Your task is to author a comprehensive, production-ready **Technical Design Document (TDD)** for a "Top-Down Shooter Framework Showcase" that demonstrates the engine's capabilities.

## Reference Materials Available
You have access to comprehensive documentation in `.github/copilot-instructions.md` that details:
- Complete architecture overview (60+ source files)
- Rendering system (Renderer2D API, texture system, camera controls)
- ECS architecture (EnTT-based with Scene, Entity, Component systems)
- Physics integration (Box2D with spatial hash grid)
- Audio system (OpenAL-Soft with 3D spatial audio)
- Input system (static polling API with key codes)
- Build system and project structure

## Execution Steps

### Phase 1: Codebase Analysis (MANDATORY)
**Objective:** Build a complete technical inventory of Pillar Engine's capabilities.

Use the following tools systematically:

1. **Read Core System Files** - Use `read_file` on:
   - `Pillar/src/Pillar/Renderer/Renderer2D.h` and `.cpp` (quad rendering, batching, z-ordering)
   - `Pillar/src/Pillar/ECS/Systems/PhysicsSystem.h` and `.cpp` (collision detection, Box2D integration)
   - `Pillar/src/Pillar/Input.h` (input polling API)
   - `Pillar/src/Pillar/Audio/AudioEngine.h` (spatial audio capabilities)
   - `Pillar/src/Pillar/ECS/Scene.h` (entity creation and management)

2. **Discover Component Architecture** - Search for:
   - All ECS components: `grep_search` for files in `Pillar/src/Pillar/ECS/Components/`
   - Physics components: `grep_search` for "Rigidbody", "ColliderComponent", "VelocityComponent"
   - Rendering components: Search for "SpriteComponent", "TransformComponent"

3. **Identify Existing Systems** - Search for:
   - `grep_search` for "System" in `Pillar/src/Pillar/ECS/Systems/`
   - Examine PhysicsSystem, AudioSystem, and any gameplay systems

4. **Map API Patterns** - Identify:
   - Factory methods (e.g., `Texture2D::Create()`, `AudioBuffer::Create()`)
   - Singleton patterns (e.g., `AudioEngine::`, `Renderer::`)
   - Component addition patterns (e.g., `entity.AddComponent<T>()`)

**Deliverable:** A mental model of what's available vs. what needs to be implemented.

### Phase 2: Architecture Design
**Objective:** Design the shooter framework using discovered Pillar Engine patterns.

Create a hierarchical design with these sections:

#### 2.1 System Integration Map
```markdown
## System Architecture
### Core Game Loop Integration
- **Pillar::Application**: Main loop, layer management
- **GameLayer**: Custom layer handling update/render/events
- **Scene Management**: Entity creation and system updates

[Detailed breakdown of how each Pillar system is utilized]
```

#### 2.2 Entity Component Breakdown
For each entity type (Player, Enemy, Projectile, PowerUp, Spawner):
- **Components Required**: List ACTUAL Pillar components (e.g., `TransformComponent`, `RigidbodyComponent`)
- **Components to Create**: New components needed (e.g., `HealthComponent`, `WeaponComponent`)
- **Initialization Code**: Specific C++ snippets using Pillar's API

#### 2.3 Combat System Design
- **Aiming Mechanism**: How to use `Input::GetMousePosition()` and `OrthographicCamera` for world-space targeting
- **Projectile Physics**: Box2D configuration for fast-moving bullets (CCD, sensors, collision filtering)
- **Hit Detection**: Contact listeners, raycast queries, trigger volumes

#### 2.4 "Juice" Implementation Guide
- **Visual Effects**: 
  - Particle system integration (if available) or alternative approaches
  - Screen shake using camera controller
  - Flash effects using sprite color tinting
- **Audio Feedback**:
  - Weapon fire: `AudioEngine::CreateSource()` + `AudioBuffer::Create("gunshot.wav")`
  - Impact sounds with 3D positioning: `source->SetPosition(hitPos)`
  - Background music with looping: `source->SetLooping(true)`

#### 2.5 Wave Management System
- **Timer/Spawning Logic**: Use `OnUpdate(float dt)` for timing
- **Difficulty Scaling**: Progressive enemy stats per wave
- **Spawner Patterns**: Random vs. directed spawning strategies

### Phase 3: Implementation Guide
**Objective:** Provide copy-paste-ready code examples.

For each major system, include:

1. **Component Definition** (if new):
```cpp
// HealthComponent.h
namespace Pillar {
    struct HealthComponent {
        float CurrentHealth = 100.0f;
        float MaxHealth = 100.0f;
        bool IsInvulnerable = false;
    };
}
```

2. **Entity Creation**:
```cpp
// Example using actual Pillar API
auto player = scene.CreateEntity("Player");
auto& transform = player.AddComponent<TransformComponent>();
transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);

auto& rb = player.AddComponent<RigidbodyComponent>();
rb.Type = RigidbodyType::Dynamic;
// ... specific configuration
```

3. **System Logic**:
```cpp
// In custom GameLayer::OnUpdate(float dt)
void GameLayer::OnUpdate(float dt) {
    // Update camera
    m_CameraController.OnUpdate(dt);
    
    // Update game systems
    m_PhysicsSystem.OnUpdate(dt, m_Scene.GetRegistry());
    m_CombatSystem.OnUpdate(dt, m_Scene.GetRegistry());
    
    // Render
    Renderer2D::BeginScene(m_CameraController.GetCamera());
    RenderEntities();
    Renderer2D::EndScene();
}
```

### Phase 4: Documentation Structure
**Output Format:** Create a markdown document with this exact structure:

```markdown
# Top-Down Shooter Framework - Technical Design Document
Version 1.0 | Pillar Engine

## Executive Summary
[One paragraph overview]

## 1. System Architecture
### 1.1 Overview
### 1.2 Pillar Engine Integration Points
### 1.3 Game Loop Flow

## 2. Entity Component Design
### 2.1 Player System
### 2.2 Enemy System
### 2.3 Projectile System
### 2.4 Power-Up System
### 2.5 Environment System

## 3. Core Gameplay Systems
### 3.1 Combat System
### 3.2 Movement & Physics
### 3.3 AI & Wave Management
### 3.4 Collision & Damage

## 4. Feedback & Polish ("Juice")
### 4.1 Visual Effects
### 4.2 Audio Integration
### 4.3 Camera Effects
### 4.4 UI Feedback

## 5. Implementation Roadmap
### 5.1 Phase 1: Core Framework
### 5.2 Phase 2: Combat Mechanics
### 5.3 Phase 3: Enemy AI & Waves
### 5.4 Phase 4: Polish & Effects

## 6. API Reference
### 6.1 New Components
### 6.2 New Systems
### 6.3 Utility Functions

## 7. Testing Strategy
### 7.1 Unit Tests
### 7.2 Integration Tests
### 7.3 Performance Benchmarks

## Appendix A: Asset Requirements
## Appendix B: Build Instructions
```

## Quality Criteria

Your TDD must meet these standards:

### Technical Accuracy ✓
- [ ] All class names match actual Pillar Engine source code
- [ ] All method calls use correct signatures (e.g., `Texture2D::Create(path)`, not `Texture2D::Load()`)
- [ ] Namespace usage is correct (e.g., `Pillar::RenderCommand::`, not `Pillar::Renderer::Command::`)
- [ ] Component patterns match EnTT usage (e.g., `entity.AddComponent<T>()`, `registry.view<T1, T2>()`)

### Completeness ✓
- [ ] Every major system is explained (rendering, physics, audio, input, ECS)
- [ ] At least 3 complete code examples per major entity type
- [ ] Initialization, update, and cleanup logic are all covered
- [ ] Both "happy path" and edge cases are addressed (e.g., what happens when player dies)

### Implementability ✓
- [ ] Code examples compile without modification (assume standard includes)
- [ ] Asset paths use `AssetManager::GetTexturePath()` or `GetAudioPath()` correctly
- [ ] Build instructions reference actual CMake presets (`windows-debug`, `windows-release`)
- [ ] No placeholder comments like "// TODO: implement this" without explanation

### Developer Experience ✓
- [ ] Clear section navigation with meaningful headers
- [ ] Progressive complexity (simple examples first, advanced later)
- [ ] Visual diagrams using Mermaid or ASCII art where helpful
- [ ] Troubleshooting sections for common issues
- [ ] Links to relevant Pillar Engine documentation files

### Professional Tone ✓
- Use imperative mood for instructions ("Configure the rigidbody..." not "You should configure...")
- Explain WHY decisions are made, not just WHAT to do
- Anticipate questions ("Why use static bodies for walls?" → "Because...")

## Constraints

1. **No Fictional APIs**: Do not invent methods or classes. If Pillar lacks a feature (e.g., particle system), explicitly state "Pillar Engine currently lacks a particle system. Recommended approach: implement a simple sprite-based particle system using..."

2. **Respect Engine Architecture**: 
   - Use static library pattern (no DLL exports needed)
   - Follow layer-based architecture (don't bypass it)
   - Respect ECS principles (no giant monolithic game object classes)

3. **Platform Specificity**: 
   - Target Windows with Visual Studio 2022
   - Use CMake presets (`windows-debug`, `windows-release`)
   - Reference actual paths (e.g., `Sandbox/assets/textures/`)

4. **Performance Awareness**:
   - Acknowledge batching limitations (Renderer2D batch size)
   - Mention collision optimization (spatial hash grid already implemented)
   - Discuss audio source limits (OpenAL typical max: 256 sources)

## Validation Checklist

Before finalizing, verify:

1. **Build Validation**: Include a section showing:
   ```powershell
   cmake --build --preset windows-debug
   .\bin\Debug-x64\Sandbox\SandboxApp.exe
   ```

2. **Code Snippets**: Every code block should specify:
   - Filename/location (e.g., `// In Sandbox/src/GameLayer.cpp`)
   - Required includes at the top
   - Namespace declarations

3. **Asset Pipeline**: Document:
   - Required texture formats (PNG recommended via stb_image)
   - Audio format (WAV, 8/16-bit PCM, mono/stereo)
   - Directory structure (`assets/textures/`, `assets/audio/sfx/`, `assets/audio/music/`)

## Success Metrics

This TDD is successful if:
- A competent C++ developer can build the showcase in **2-3 days** following the document
- The resulting game demonstrates **all major Pillar Engine features**
- The code serves as a **reusable template** for other top-down games
- The document becomes the **canonical reference** for Pillar Engine game development

## Output Directive

Generate the complete TDD as a single markdown file. Begin with the Executive Summary and proceed through all sections sequentially. Use actual code from Pillar Engine wherever possible, and clearly mark any extensions or custom implementations needed.

Do NOT create placeholder sections—every section must be fully developed with concrete information, code examples, and actionable guidance.

---

**Note:** This prompt assumes you have already reviewed `.github/copilot-instructions.md`. If you need to refresh on specific subsystems, use the Phase 1 tool guidance to examine relevant source files before generating documentation.
