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
    src/EventTests.cpp
    src/LayerTests.cpp
    src/CameraTests.cpp
    src/LoggerTests.cpp
    src/InputTests.cpp
    src/ApplicationTests.cpp
    src/WindowTests.cpp
    src/SceneTests.cpp
    src/EntityTests.cpp
    src/VelocityIntegrationTests.cpp
    src/BulletCollisionTests.cpp
    src/SpatialHashGridTests.cpp
    src/XPCollectionTests.cpp

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
gtest_discover_tests(PillarTests)
