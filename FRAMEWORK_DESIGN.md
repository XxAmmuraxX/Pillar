# Pillar Engine - Framework Abstraction Design

## 1. Executive Summary

This document outlines the design for the high-level Pillar Framework, an abstraction layer built on top of the core Pillar Engine. The goal is to provide a developer-friendly, robust, and scalable environment for game development that supports both code-first and editor-driven workflows.

**Key Design Decisions:**
- **Hybrid Architecture:** Combines the performance of ECS (EnTT) with the ease of use of Object-Oriented Programming (GameObject wrappers).
- **Layered API:** Accessible to beginners while exposing low-level control for advanced users.
- **Editor-Ready:** All systems designed with serialization and reflection in mind to support a future full-featured editor.
- **Windows-First:** Optimized for Windows development initially, with architectural room for future platform expansion.

---

## 2. Architecture Overview

The framework introduces a new layer between the Core Engine and the User Game Code.

```
┌─────────────────────────────────────────────────────────────────┐
│                        USER GAME CODE                           │
│  (GameObjects, Behaviors, Prefabs, Scenes, Custom Systems)      │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     PILLAR FRAMEWORK                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  GameObject  │  │ Scene Stack  │  │ Action Input │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐           │
│  │  Behaviors   │  │ Asset Handle │  │  Reflection  │           │
│  └──────────────┘  └──────────────┘  └──────────────┘           │
└─────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                     PILLAR CORE ENGINE                          │
│      (Application, Renderer, Audio, ECS/EnTT, Physics)         │
└─────────────────────────────────────────────────────────────────┘
```

---

## 3. Core Concepts & API Design

### 3.1 The Game Object System (Hybrid ECS/OOP)

To satisfy the "Hybrid" requirement, we introduce `GameObject`. This is a lightweight wrapper around an ECS `Entity`. It provides an intuitive API while keeping data in cache-friendly component arrays.

**Key Features:**
- **Wrapper, not Container:** `GameObject` does not store data itself; it accesses data stored in the ECS registry.
- **Transform Shortcuts:** Direct access to position, rotation, and scale.
- **Hierarchy Management:** Parent/Child relationship methods.

**Proposed API:**

```cpp
namespace Pillar {

    class GameObject
    {
    public:
        // Creation
        static GameObject Create(const std::string& name = "GameObject");
        static void Destroy(GameObject obj);

        // Component Management
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args);

        template<typename T>
        T& GetComponent();

        template<typename T>
        bool HasComponent();

        template<typename T>
        void RemoveComponent();

        // Transform Shortcuts (wraps TransformComponent)
        glm::vec2 GetPosition() const;
        void SetPosition(const glm::vec2& pos);
        float GetRotation() const;
        void SetRotation(float rotation);
        
        // Hierarchy
        void SetParent(GameObject parent);
        GameObject GetParent();
        std::vector<GameObject> GetChildren();

        // Lifecycle
        bool IsValid() const;

    private:
        Entity m_EntityHandle; // The underlying ECS entity
    };

}
```

### 3.2 The Behavior System (Scripting)

Game logic is implemented via **Behaviors**. These are special components that inherit from a base class and have lifecycle methods.

**Key Features:**
- **Composition:** Entities can have multiple behaviors.
- **Lifecycle Hooks:** `OnCreate`, `OnUpdate`, `OnDestroy`, `OnCollision`.
- **Reflection:** Macros to expose variables to the editor.

**Proposed API:**

```cpp
namespace Pillar {

    class Behavior
    {
    public:
        virtual void OnCreate() {}
        virtual void OnUpdate(float dt) {}
        virtual void OnDestroy() {}
        virtual void OnCollisionEnter(Collision collision) {}

        // Access to owner
        GameObject GetGameObject() const { return m_GameObject; }

    protected:
        GameObject m_GameObject;
    };

}

// User Code Example
class PlayerController : public Pillar::Behavior
{
public:
    float Speed = 5.0f;

    void OnUpdate(float dt) override
    {
        if (Pillar::Input::IsActionPressed("MoveRight"))
            GetGameObject().SetPosition(GetGameObject().GetPosition() + glm::vec2(Speed * dt, 0));
    }

    // Reflection for Editor
    PILLAR_REFLECT_BEGIN(PlayerController)
        PILLAR_PROPERTY(Speed, "Movement Speed")
    PILLAR_REFLECT_END()
};
```

### 3.3 Scene Management (Stack & Additive)

The Scene system will be upgraded to support a stack-based approach with additive loading, allowing for complex UI overlays, pause menus, and streaming worlds.

**Key Features:**
- **Scene Stack:** Push/Pop scenes (e.g., MainMenu -> Game -> Pause).
- **Additive Loading:** Load a scene without unloading the current one (e.g., "HUD_Scene").
- **Async Loading:** Background loading for large levels.

**Proposed API:**

```cpp
namespace Pillar {

    class SceneManager
    {
    public:
        // Standard Flow
        static void LoadScene(const std::string& sceneName); // Unloads current, loads new

        // Stack Flow
        static void PushScene(const std::string& sceneName); // Pauses current, overlays new
        static void PopScene();                              // Unloads top, resumes previous

        // Additive Flow
        static void LoadSceneAdditive(const std::string& sceneName);
        static void UnloadScene(const std::string& sceneName);

        // Scene Data
        static Scene* GetActiveScene();
    };

}
```

### 3.4 The Game Loop (Managed)

The `Game` class wraps the low-level `Application`. Users inherit from `Game` to define their project.

**Proposed API:**

```cpp
namespace Pillar {

    class Game : public Application
    {
    public:
        Game(const GameSettings& settings);

        // User overrides
        virtual void OnStart() {}
        virtual void OnUpdate(float dt) {}
        virtual void OnRender() {}
        virtual void OnShutdown() {}
    };

}
```

### 3.5 Input System (Action Mapping)

To support rebindable controls and abstract away physical keys, we introduce an Action Mapping system.

**Proposed API:**

```cpp
namespace Pillar {

    class Input
    {
    public:
        // Configuration
        static void BindAction(const std::string& actionName, KeyCode key);
        static void BindAxis(const std::string& axisName, KeyCode positive, KeyCode negative);

        // Query
        static bool IsActionPressed(const std::string& actionName);
        static bool IsActionJustPressed(const std::string& actionName);
        static float GetAxis(const std::string& axisName); // Returns -1.0 to 1.0
        
        // Raw access (still available)
        static bool IsKeyPressed(KeyCode key);
    };

}
```

---

## 4. Editor Integration Strategy

To support the future "Full Editor" requirement (Choice 7), the framework must be built with **Reflection** and **Serialization** from day one.

### 4.1 Reflection System
We need a way for the editor to know what properties a Behavior has.

**Macro System:**
```cpp
#define PILLAR_PROPERTY(Variable, DisplayName) \
    registry.RegisterProperty(#Variable, &Variable, DisplayName);
```

This allows the editor to:
1.  Display the property in an "Inspector" panel.
2.  Serialize the property value to disk (YAML/JSON).
3.  Deserialize the value when loading a scene.

### 4.2 Prefab System
Prefabs will be defined as serialized Entity descriptions.
- **Code Prefabs:** Factory functions that return a configured `GameObject`.
- **Data Prefabs:** JSON files describing an entity and its components.

---

## 5. Asset Management (Phased)

**Phase 1 (Current/Immediate):**
- Enhance `AssetManager` to return `AssetHandle<T>` instead of raw pointers or `std::shared_ptr`.
- `AssetHandle` wraps the pointer and an Asset ID (UUID).
- Allows for future hot-reloading (swapping the underlying data without invalidating the handle).

**Phase 2 (Future):**
- Asset Database (metadata files alongside assets).
- Asset Bundles (packing for release).

---

## 6. Error Handling Strategy

**Debug Mode:**
- Use `PIL_ASSERT` for critical failures (null pointers, invalid handles).
- Fail fast to help developers catch bugs early.

**Release Mode:**
- Log errors (`PIL_ERROR`).
- Return "Fallback" objects (e.g., a bright pink "Missing Texture" texture, or a silent audio clip).
- Prevent crashes whenever possible to keep the game running.

---

## 7. Implementation Roadmap

### Phase 1: Foundation (The "Framework" Layer)
1.  Implement `GameObject` wrapper around `Entity`.
2.  Implement `Behavior` base class and system.
3.  Implement `Game` class wrapper.
4.  Implement `Input` action mapping.

### Phase 2: Scene & Assets
1.  Upgrade `SceneManager` for Stack/Additive operations.
2.  Refactor `AssetManager` to use Handles.

### Phase 3: Editor Prep
1.  Implement Reflection macros.
2.  Implement Serialization for Behaviors.
3.  Create the Prefab system.

### Phase 4: Built-in Features
1.  Tilemap System.
2.  Tweening Library.
3.  State Machine Utility.

---

## 8. Example Usage (The "Pillar Style")

```cpp
// Define a custom behavior
class CharacterController : public Pillar::Behavior {
public:
    float MoveSpeed = 10.0f;

    void OnCreate() override {
        // Setup input bindings
        Pillar::Input::BindAxis("Horizontal", PIL_KEY_A, PIL_KEY_D);
        Pillar::Input::BindAction("Jump", PIL_KEY_SPACE);
    }

    void OnUpdate(float dt) override {
        // Movement
        float move = Pillar::Input::GetAxis("Horizontal");
        glm::vec2 pos = GetGameObject().GetPosition();
        pos.x += move * MoveSpeed * dt;
        GetGameObject().SetPosition(pos);

        // Jump
        if (Pillar::Input::IsActionJustPressed("Jump")) {
            auto& rb = GetGameObject().GetComponent<Pillar::RigidbodyComponent>();
            rb.ApplyImpulse({0, 10.0f});
        }
    }
    
    // Expose to Editor
    PILLAR_REFLECT_BEGIN(CharacterController)
        PILLAR_PROPERTY(MoveSpeed, "Speed")
    PILLAR_REFLECT_END()
};

// The Game Application
class MyGame : public Pillar::Game {
public:
    void OnStart() override {
        // Load initial scene
        Pillar::SceneManager::LoadScene("Level1");

        // Create Player (Code-first approach)
        auto player = Pillar::GameObject::Create("Player");
        player.AddComponent<Pillar::SpriteComponent>("player.png");
        player.AddComponent<Pillar::RigidbodyComponent>();
        player.AddComponent<CharacterController>(); // Add behavior
    }
};

// Entry Point
Pillar::Game* Pillar::CreateGame() {
    return new MyGame();
}
```
