# Added this line for clarity about file location
# This is sandbox cmake
# ==============================
# Sandbox Application (Executable)
# ==============================
add_executable(Sandbox 
    # Entry Point
    src/Source.cpp
    
    # Arena Protocol Game (Main Layer)
    src/ArenaProtocol/ArenaProtocolLayer.cpp
    src/ArenaProtocol/ArenaProtocolLayer.h
    
    # Components
    src/Components/PlayerStatsComponent.h
    src/Components/HealthComponent.h
    src/Components/EnemyAIComponent.h
    src/Components/TurretAIComponent.h
    src/Components/ChargerAIComponent.h
    src/Components/TriggerComponent.h
    src/Components/BossAIComponent.h
    src/Components/XPGemVisualComponent.h
    src/Components/ObstacleComponent.h
    src/Components/WeaponComponent.h
    src/Components/CameraEffectsComponent.h
    src/Components/WaveManagerComponent.h
    
    # Game Systems
    src/Systems/EnemyAISystem.cpp
    src/Systems/EnemyAISystem.h
    src/Systems/BossAISystem.cpp
    src/Systems/BossAISystem.h
    src/Systems/BulletSystem.cpp
    src/Systems/BulletSystem.h
    src/Systems/XPCollectionSystem.cpp
    src/Systems/XPCollectionSystem.h
    src/Systems/WaveSystem.cpp
    src/Systems/WaveSystem.h
    
    # Entity Factory
    src/Factory/EntityFactory.cpp
    src/Factory/EntityFactory.h
    
    # Chamber/Level Management
    src/Chambers/ChamberManager.cpp
    src/Chambers/ChamberManager.h
)

# Set output directory
set_target_properties(Sandbox PROPERTIES
    PROJECT_LABEL "SandboxApp"  # This sets the VS project name
    OUTPUT_NAME "SandboxApp"    # This sets the executable name
    RUNTIME_OUTPUT_DIRECTORY ${EXECUTABLE_OUTPUT_DIR}
)

# Override for all configurations
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(Sandbox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${EXECUTABLE_OUTPUT_DIR}
    )
endforeach()

# Create assets directory structure in the Sandbox source folder (for development)
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures
    COMMENT "Creating Sandbox/assets directory structure"
)

# Link libraries - static linking, no DLL copy needed
target_link_libraries(Sandbox PRIVATE Pillar)

target_compile_definitions(Sandbox PRIVATE PIL_STATIC_LIB)