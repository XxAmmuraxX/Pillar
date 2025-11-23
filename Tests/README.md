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

- **EventTests.cpp** - Tests for the event system (Event, EventDispatcher, all event types)
- **LayerTests.cpp** - Tests for Layer and LayerStack functionality
- **LoggerTests.cpp** - Tests for the logging system
- **ApplicationTests.cpp** - Tests for the Application class
- **InputTests.cpp** - Tests for input handling (keyboard, mouse)
- **WindowTests.cpp** - Tests for window creation and management

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


