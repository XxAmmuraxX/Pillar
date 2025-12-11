# Pillar Engine Unit Tests

This directory contains unit tests for the Pillar Engine using Google Test framework.

## Test Organization

Tests are organized into logical subdirectories:

```
Tests/src/
├── Core/           # Core engine functionality tests
├── ECS/            # Entity Component System tests
├── Renderer/       # Rendering system tests
├── Audio/          # Audio system tests
├── Gameplay/       # Gameplay systems tests
├── Physics/        # Physics system tests (Box2D)
└── Integration/    # Integration and E2E tests
```

## Building Tests

Tests are built automatically when you build the project:

```powershell
cmake -S . -B out/build/x64-Debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build/x64-Debug --config Debug --parallel
```

## Running Tests

### Run All Tests
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe
```

### Run Tests with Verbose Output
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_output=xml:test-results.xml
```

### Run Specific Test Category
```powershell
# Run all ECS tests
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=*Entity*:*Scene*:*Component*

# Run all integration tests
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=*Integration*:*E2E*
```

### Run Specific Test Suite
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.*
```

### List All Tests
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_list_tests
```

---

## Test Categories

### Core Engine Tests (`src/Core/`)

| File | Description |
|------|-------------|
| `EventTests.cpp` | Event system (Event, EventDispatcher, all event types) |
| `LayerTests.cpp` | Layer and LayerStack functionality |
| `LoggerTests.cpp` | Logging system (log levels, formatting) |
| `ApplicationTests.cpp` | Application class lifecycle |
| `ApplicationEventTests.cpp` | Application-specific events |
| `InputTests.cpp` | Input handling (keyboard, mouse) |
| `WindowTests.cpp` | Window creation and management |
| `AssetManagerTests.cpp` | Asset path resolution |

### ECS Tests (`src/ECS/`)

| File | Description |
|------|-------------|
| `SceneTests.cpp` | Scene management (entity creation/destruction) |
| `EntityTests.cpp` | Entity operations (add/remove/get components) |
| `ComponentTests.cpp` | Individual component tests |
| `ComponentRegistryTests.cpp` | Component registration and serialization |
| `SceneSerializerTests.cpp` | Scene save/load functionality |
| `ObjectPoolTests.cpp` | Generic object pooling |
| `SpecializedPoolsTests.cpp` | BulletPool, ParticlePool |

### Renderer Tests (`src/Renderer/`)

| File | Description |
|------|-------------|
| `CameraTests.cpp` | OrthographicCamera and CameraController |

### Audio Tests (`src/Audio/`)

| File | Description |
|------|-------------|
| `AudioTests.cpp` | Core audio functionality (AudioEngine, AudioSource) |
| `AudioExtendedTests.cpp` | 3D audio, attenuation, advanced playback |

### Gameplay System Tests (`src/Gameplay/`)

| File | Description |
|------|-------------|
| `VelocityIntegrationTests.cpp` | Velocity system (position updates, drag) |
| `BulletCollisionTests.cpp` | BulletComponent and collision system |
| `SpatialHashGridTests.cpp` | Spatial partitioning |
| `XPCollectionTests.cpp` | XP gem collection system |
| `ParticleSystemTests.cpp` | Particle components and systems |
| `AnimationTests.cpp` | Animation system (frames, clips, playback) |

### Physics Tests (`src/Physics/`)

| File | Description |
|------|-------------|
| `Box2DContactListenerTests.cpp` | Collision callbacks |
| `Box2DBodyFactoryTests.cpp` | Physics body creation |
| `PhysicsSystemTests.cpp` | Physics system integration |

### Integration Tests (`src/Integration/`)

| File | Description |
|------|-------------|
| `ECSIntegrationTests.cpp` | Multi-system ECS integration |
| `PerformanceTests.cpp` | Performance and stress tests |
| `AudioIntegrationTests.cpp` | Audio subsystem integration |
| `GameplayScenarioTests.cpp` | End-to-end gameplay scenarios |
| `EndToEndSceneTests.cpp` | Complete scene workflows |
| `RenderingIntegrationTests.cpp` | Rendering abstraction tests |
| `AssetPipelineE2ETests.cpp` | Asset workflow tests |
| `GameFrameE2ETests.cpp` | Complete game frame cycle |
| `ComponentSerializationTests.cpp` | Component serialization tests |

---

## Adding New Tests

### Step 1: Choose the Right Category

| Test Type | Directory | When to Use |
|-----------|-----------|-------------|
| Unit test for single class | Matching category folder | Testing isolated functionality |
| Integration test | `Integration/` | Testing multiple systems together |
| Performance test | `Integration/PerformanceTests.cpp` | Benchmarking |
| E2E test | `Integration/` | Testing complete workflows |

### Step 2: Create the Test File

```cpp
#include <gtest/gtest.h>
#include "Pillar/YourComponent.h"

// Test fixture (optional, for shared setup)
class YourComponentTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Setup code
    }
    
    void TearDown() override
    {
        // Cleanup code
    }
};

// Individual test
TEST_F(YourComponentTests, TestName)
{
    // Arrange
    YourComponent component;
    
    // Act
    component.DoSomething();
    
    // Assert
    EXPECT_EQ(component.GetValue(), expectedValue);
}

// Standalone test (no fixture)
TEST(YourComponentTests, SimpleTest)
{
    EXPECT_TRUE(SomeFunction());
}
```

### Step 3: Add to CMakeLists.txt

Add your test file to `Tests/CMakeLists.txt` in the appropriate section:

```cmake
add_executable(PillarTests
    # ... existing tests ...
    
    # Your new test
    src/YourCategory/YourTests.cpp
)
```

---

## Test Patterns

### Testing Components

```cpp
TEST(ComponentTests, TransformComponent_DefaultValues)
{
    Pillar::TransformComponent transform;
    
    EXPECT_FLOAT_EQ(transform.Position.x, 0.0f);
    EXPECT_FLOAT_EQ(transform.Position.y, 0.0f);
    EXPECT_FLOAT_EQ(transform.Rotation, 0.0f);
}
```

### Testing with Scene

```cpp
class SceneBasedTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Scene = std::make_unique<Pillar::Scene>("TestScene");
    }
    
    std::unique_ptr<Pillar::Scene> m_Scene;
};

TEST_F(SceneBasedTests, CreateEntity_HasDefaultComponents)
{
    auto entity = m_Scene->CreateEntity("TestEntity");
    
    EXPECT_TRUE(entity.HasComponent<Pillar::TagComponent>());
    EXPECT_TRUE(entity.HasComponent<Pillar::TransformComponent>());
}
```

### Testing Systems

```cpp
TEST_F(SystemTests, VelocitySystem_UpdatesPosition)
{
    Pillar::VelocityIntegrationSystem system;
    system.OnAttach(m_Scene.get());
    
    auto entity = m_Scene->CreateEntity("MovingEntity");
    entity.AddComponent<Pillar::VelocityComponent>(glm::vec2(100.0f, 0.0f));
    
    system.OnUpdate(1.0f);  // 1 second
    
    auto& transform = entity.GetComponent<Pillar::TransformComponent>();
    EXPECT_NEAR(transform.Position.x, 100.0f, 0.1f);
}
```

### Performance Tests

```cpp
TEST(PerformanceTests, EntityCreation_1000Entities_Under100ms)
{
    auto scene = std::make_unique<Pillar::Scene>("PerfTest");
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000; i++)
    {
        scene->CreateEntity("Entity" + std::to_string(i));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_LT(duration.count(), 100);
}
```

---

## Google Test Assertions

### Basic Assertions

| Assertion | Description |
|-----------|-------------|
| `EXPECT_TRUE(cond)` | Condition is true |
| `EXPECT_FALSE(cond)` | Condition is false |
| `EXPECT_EQ(a, b)` | a == b |
| `EXPECT_NE(a, b)` | a != b |
| `EXPECT_LT(a, b)` | a < b |
| `EXPECT_LE(a, b)` | a <= b |
| `EXPECT_GT(a, b)` | a > b |
| `EXPECT_GE(a, b)` | a >= b |

### Floating Point

| Assertion | Description |
|-----------|-------------|
| `EXPECT_FLOAT_EQ(a, b)` | Float equality with tolerance |
| `EXPECT_DOUBLE_EQ(a, b)` | Double equality with tolerance |
| `EXPECT_NEAR(a, b, tol)` | Custom tolerance |

### Fatal vs Non-Fatal

- `EXPECT_*` - Non-fatal, test continues
- `ASSERT_*` - Fatal, test stops immediately

Use `ASSERT_*` for preconditions that must be true for the rest of the test.

---

## CI Integration

Tests are automatically run in GitHub Actions on every push/PR.

**CI Environment:**
- Windows runner with Visual Studio
- Mesa3D software OpenGL rendering (for headless testing)
- All tests run, including window/input tests

**CI Configuration:** See `.github/workflows/build.yml`

---

## Test Coverage

### Current Coverage (approximate)

| Category | Coverage |
|----------|----------|
| Core (Events, Layers) | 85%+ |
| ECS (Scene, Entity) | 80%+ |
| Audio | 70%+ |
| Physics | 75%+ |
| Rendering | 40%+ |
| Serialization | 70%+ |

### Improving Coverage

Priority areas for new tests:
1. Rendering pipeline (requires mock or software renderer)
2. Complex collision scenarios
3. Animation edge cases
4. Error handling paths


