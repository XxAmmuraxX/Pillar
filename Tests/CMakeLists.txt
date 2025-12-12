# ==============================
# Tests Project (Unit Tests)
# ==============================

# Fetch Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()

# Create test executable
add_executable(PillarTests
    # ===================
    # Core Engine Tests
    # ===================
    src/Core/EventTests.cpp
    src/Core/LayerTests.cpp
    src/Core/LoggerTests.cpp
    src/Core/ApplicationTests.cpp
    src/Core/ApplicationEventTests.cpp
    src/Core/InputTests.cpp
    src/Core/WindowTests.cpp
    src/Core/AssetManagerTests.cpp

    # ===================
    # ECS Tests
    # ===================
    src/ECS/SceneTests.cpp
    src/ECS/EntityTests.cpp
    src/ECS/ComponentTests.cpp
    src/ECS/ComponentRegistryTests.cpp
    src/ECS/SceneSerializerTests.cpp
    src/ECS/ObjectPoolTests.cpp
    src/ECS/SpecializedPoolsTests.cpp

    # ===================
    # Renderer Tests
    # ===================
    src/Renderer/CameraTests.cpp

    # ===================
    # Audio Tests
    # ===================
    src/Audio/AudioTests.cpp
    src/Audio/AudioExtendedTests.cpp

    # ===================
    # Gameplay System Tests
    # ===================
    src/Gameplay/VelocityIntegrationTests.cpp
    src/Gameplay/BulletCollisionTests.cpp
    src/Gameplay/SpatialHashGridTests.cpp
    src/Gameplay/XPCollectionTests.cpp
    src/Gameplay/ParticleSystemTests.cpp
    src/Gameplay/AnimationTests.cpp

    # ===================
    # Physics Tests
    # ===================
    src/Physics/Box2DContactListenerTests.cpp
    src/Physics/Box2DBodyFactoryTests.cpp
    src/Physics/PhysicsSystemTests.cpp
    
    # ===================
    # Integration Tests
    # ===================
    src/Integration/ECSIntegrationTests.cpp
    src/Integration/PerformanceTests.cpp
    src/Integration/AudioIntegrationTests.cpp
    src/Integration/GameplayScenarioTests.cpp
    src/Integration/EndToEndSceneTests.cpp
    src/Integration/RenderingIntegrationTests.cpp
    src/Integration/AssetPipelineE2ETests.cpp
    src/Integration/GameFrameE2ETests.cpp
    src/Integration/ComponentSerializationTests.cpp
)

# Set output directory
set_target_properties(PillarTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR}/Tests
)

# Override for all configurations
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(PillarTests PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${BINARY_OUTPUT_DIR}/Tests
    )
endforeach()

# Link libraries
target_link_libraries(PillarTests
    PRIVATE
        Pillar
        GTest::gtest_main
        GTest::gmock
)

# Copy Pillar DLL to Tests directory
add_custom_command(TARGET PillarTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Pillar>
        ${BINARY_OUTPUT_DIR}/Tests
)

# Discover tests for CTest integration
include(GoogleTest)
gtest_discover_tests(PillarTests
    DISCOVERY_MODE PRE_TEST
)
