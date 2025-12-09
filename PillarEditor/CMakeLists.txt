# ==============================
# Pillar Editor Application
# ==============================
add_executable(PillarEditor
    src/EditorApp.cpp
    src/EditorLayer.cpp
    src/SelectionContext.cpp
    src/EditorCamera.cpp
    src/Panels/SceneHierarchyPanel.cpp
    src/Panels/InspectorPanel.cpp
    src/Panels/ViewportPanel.cpp
    src/Panels/ContentBrowserPanel.cpp
    src/Panels/ConsolePanel.cpp
)

# Override output directory for all configurations
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(PillarEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_SOURCE_DIR}/bin/Debug-x64/PillarEditor
    )
endforeach()

set_target_properties(PillarEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/Debug-x64/PillarEditor
    OUTPUT_NAME "PillarEditor"
)

# Link to Pillar engine (gets all dependencies transitively)
target_link_libraries(PillarEditor PRIVATE Pillar)

# Include directories
target_include_directories(PillarEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Define static lib for Pillar
target_compile_definitions(PillarEditor PRIVATE PIL_STATIC_LIB)

# Create assets directory for editor icons
add_custom_command(TARGET PillarEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/assets/icons"
)
