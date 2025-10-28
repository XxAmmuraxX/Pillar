# ==============================
# Pillar Engine (Shared Library - DLL)
# ==============================
add_library(Pillar SHARED
    src/Pillar/Application.cpp
    src/Pillar/Logger.cpp
    src/Pillar/Events/ApplicationEvent.cpp
    src/Platform/WindowsWindow.cpp
    src/Layer.cpp
    src/Pillar/LayerStack.cpp
    src/Pillar/ImGuiLayer.cpp
)

# Override for all configurations
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(Pillar PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${LIBRARY_OUTPUT_DIR}
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${LIBRARY_OUTPUT_DIR}
        LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${LIBRARY_OUTPUT_DIR}
    )
endforeach()


# Set output directories
set_target_properties(Pillar PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
    LIBRARY_OUTPUT_DIR ${LIBRARY_OUTPUT_DIR}
)

# Include directories
target_include_directories(Pillar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries - GLFW includes its own headers when linked properly
target_link_libraries(Pillar 
    PUBLIC 
        spdlog::spdlog
        glfw
        glad
        imgui
)

# For Windows, link additional system libraries that GLFW needs
if(WIN32)
    target_link_libraries(Pillar PRIVATE
        opengl32
    )
endif()

# Define this so symbols are exported from DLL
target_compile_definitions(Pillar PRIVATE PIL_EXPORT)

# Copy Pillar DLL next to the Sandbox executable whenever Pillar is built
add_custom_command(TARGET Pillar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXECUTABLE_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Pillar>
        ${EXECUTABLE_OUTPUT_DIR}
)
