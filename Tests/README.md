# Pillar Engine Unit Tests

This directory contains unit tests for the Pillar Engine using Google Test framework.

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

### Run Specific Test Suite
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.*
```

### Run Specific Test
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_filter=EventTests.KeyPressedEvent_Creation
```

### List All Tests
```powershell
.\bin\Debug-x64\Tests\PillarTests.exe --gtest_list_tests
```

## Test Organization

### Core Engine Tests
- **EventTests.cpp** - Tests for the event system (Event, EventDispatcher, all event types including KeyPressed, KeyReleased, MouseMoved, WindowResize, etc.)
- **LayerTests.cpp** - Tests for Layer and LayerStack functionality (push/pop layers, overlays, iteration order)
- **LoggerTests.cpp** - Tests for the logging system (log levels, formatting, core vs app loggers)
- **ApplicationTests.cpp** - Tests for the Application class (lifecycle, event handling)
- **InputTests.cpp** - Tests for input handling (keyboard state, mouse position, button states)
- **WindowTests.cpp** - Tests for window creation and management (properties, events)

### Renderer Tests
- **CameraTests.cpp** - Tests for OrthographicCamera and OrthographicCameraController (projection/view matrices, zoom, movement, rotation, event handling)

### ECS (Entity Component System) Tests
- **SceneTests.cpp** - Tests for Scene management (entity creation/destruction, component queries)
- **EntityTests.cpp** - Tests for Entity operations (component add/remove/get, validity checks)
- **ComponentTests.cpp** - Tests for individual components:
  - TransformComponent (position, rotation, scale, cached matrix)
  - HierarchyComponent (parent-child relationships)
  - VelocityComponent (velocity, acceleration, drag, max speed)
  - ColliderComponent (circle/box types, material properties, collision filtering)
  - SpriteComponent (texture, color, UV coordinates, flip, z-index)
  - CameraComponent (orthographic projection, view matrix)

### Physics System Tests
- **VelocityIntegrationTests.cpp** - Tests for VelocityIntegrationSystem (position updates, acceleration, drag, max speed clamping)
- **BulletCollisionTests.cpp** - Tests for BulletComponent and BulletCollisionSystem (lifetime, hit counting, destruction)
- **SpatialHashGridTests.cpp** - Tests for spatial partitioning (insertion, removal, queries, cell calculations)

### Gameplay System Tests
- **XPCollectionTests.cpp** - Tests for XPGemComponent and XPCollectionSystem
- **ParticleSystemTests.cpp** - Tests for particle systems:
  - ParticleComponent (lifetime, age, visual effects, size/color interpolation)
  - ParticleEmitterComponent (emission shapes, burst mode, rates, variance)
  - ParticleSystem (particle aging, death marking)
  - ParticleEmitterSystem (particle spawning, emission rates)

### Object Pooling Tests
- **ObjectPoolTests.cpp** - Tests for generic ObjectPool:
  - Initialization and pre-allocation
  - Acquire/release entity lifecycle
  - Pool exhaustion and auto-expansion
  - Init/reset callbacks
  - Statistics tracking
- **SpecializedPoolsTests.cpp** - Tests for specialized pools:
  - BulletPool (spawn with position/direction/speed, component setup)
  - ParticlePool (spawn with visual properties, high-volume handling)

### Audio System Tests
- **AudioTests.cpp** - Tests for core audio functionality:
  - AudioEngine (init/shutdown, master volume, listener position)
  - AudioSource state management
  - WavLoader file parsing
- **AudioExtendedTests.cpp** - Extended audio tests (3D positioning, attenuation, playback control)

### Serialization Tests
- **SceneSerializerTests.cpp** - Tests for scene serialization/deserialization (JSON format, entity preservation, component data)
- **ComponentRegistryTests.cpp** - Tests for ComponentRegistry:
  - Component registration and lookup
  - Serialize/deserialize functions
  - Copy functions
  - Built-in component handling

### Animation Tests
- **AnimationTests.cpp** - Tests for animation system:
  - AnimationFrame (texture path, duration, UV coordinates)
  - AnimationClip (frames, looping, total duration)
  - AnimationComponent (playback state, speed, events)
  - AnimationSystem (frame advancement, looping behavior)
  - AnimationLoader (file parsing, validation)

### Utility Tests
- **AssetManagerTests.cpp** - Tests for AssetManager:
  - Path resolution for textures, audio, SFX, music
  - Subdirectory searches
  - Fallback behavior for missing files
  - Absolute vs relative path handling

## Adding New Tests

1. Create a new `.cpp` file in `Tests/src/`
2. Include Google Test: `#include <gtest/gtest.h>`
3. Include the engine headers you want to test
4. Write test cases using `TEST()` or `TEST_F()` macros
5. Add the file to `Tests/CMakeLists.txt` in the `add_executable(PillarTests ...)` section

### Example Test Structure

```cpp
#include <gtest/gtest.h>
#include "Pillar/YourComponent.h"

TEST(YourComponentTests, TestName) {
    // Arrange
    YourComponent component;
    
    // Act
    component.DoSomething();
    
    // Assert
    EXPECT_EQ(component.GetValue(), expectedValue);
}
```

## Google Test Assertions

### Common Assertions
- `EXPECT_TRUE(condition)` / `EXPECT_FALSE(condition)`
- `EXPECT_EQ(val1, val2)` - Equal
- `EXPECT_NE(val1, val2)` - Not equal
- `EXPECT_LT(val1, val2)` - Less than
- `EXPECT_LE(val1, val2)` - Less than or equal
- `EXPECT_GT(val1, val2)` - Greater than
- `EXPECT_GE(val1, val2)` - Greater than or equal

### Floating Point Assertions
- `EXPECT_FLOAT_EQ(val1, val2)` - Float equality with tolerance
- `EXPECT_DOUBLE_EQ(val1, val2)` - Double equality with tolerance
- `EXPECT_NEAR(val1, val2, tolerance)` - Custom tolerance

### String Assertions
- `EXPECT_STREQ(str1, str2)` - C-string equality
- `EXPECT_STRNE(str1, str2)` - C-string inequality

## Test Fixtures

For tests that need common setup/teardown:

```cpp
class MyTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code
    }
    
    void TearDown() override {
        // Cleanup code
    }
    
    // Shared test data
    MyComponent* component;
};

TEST_F(MyTestFixture, TestName) {
    // Use fixture members
    EXPECT_NE(component, nullptr);
}
```

## CI Integration

Tests are automatically run in GitHub Actions on every push/PR. See `.github/workflows/build.yml` for configuration.

**CI Environment:**
- Uses Mesa3D software OpenGL rendering (llvmpipe driver) to run GUI tests without hardware GPU
- All tests including `InputTests` and `ApplicationTests` run successfully in CI


