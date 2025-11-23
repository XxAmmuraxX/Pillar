# Added this line for clarity about file location
# This is sandbox cmake
# ==============================
# Sandbox Application (Executable)
# ==============================
add_executable(Sandbox src/Source.cpp)

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

# Copy the DLL after build
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Pillar>
        ${EXECUTABLE_OUTPUT_DIR}
)

# Link libraries
target_link_libraries(Sandbox PRIVATE Pillar)

target_compile_definitions(Sandbox PRIVATE $<$<CONFIG:Debug>:PIL_ENABLE_ASSERTS>)