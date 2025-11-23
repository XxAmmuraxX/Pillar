# Added this line for clarity about file location
# This is pillar cmake
# ==============================
# Pillar Engine (Shared Library - DLL)
# ==============================
add_library(Pillar SHARED
    src/Pillar/Application.cpp
    src/Pillar/Logger.cpp
    src/Pillar/Events/ApplicationEvent.cpp
    src/Platform/WindowsWindow.cpp
    src/Pillar/Layer.cpp
    src/Pillar/LayerStack.cpp
    src/Pillar/ImGuiLayer.cpp
    src/Pillar/Input.cpp
    src/Pillar/KeyCodes.h
    # Utils
    src/Pillar/Utils/AssetManager.cpp
    # Renderer
    src/Pillar/Renderer/RenderAPI.cpp
    src/Pillar/Renderer/Renderer.cpp
    src/Pillar/Renderer/Renderer2D.cpp
    src/Pillar/Renderer/RenderCommand.cpp
    src/Pillar/Renderer/Shader.cpp
    src/Pillar/Renderer/Buffer.cpp
    src/Pillar/Renderer/VertexArray.cpp
    src/Pillar/Renderer/Texture.cpp
    src/Pillar/Renderer/OrthographicCamera.cpp
    # Platform - OpenGL
    src/Platform/OpenGL/OpenGLRenderAPI.cpp
    src/Platform/OpenGL/OpenGLContext.cpp
    src/Platform/OpenGL/OpenGLShader.cpp
    src/Platform/OpenGL/OpenGLBuffer.cpp
    src/Platform/OpenGL/OpenGLVertexArray.cpp
    src/Platform/OpenGL/OpenGLTexture.cpp
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
    ${stb_SOURCE_DIR}
)

# Link libraries - add glm::glm
target_link_libraries(Pillar 
    PUBLIC 
        spdlog::spdlog
        glfw
        glad
        imgui
        glm::glm
)

# For Windows, link additional system libraries that GLFW needs
if(WIN32)
    target_link_libraries(Pillar PRIVATE
        opengl32
    )
endif()

# Define this so symbols are exported from DLL
target_compile_definitions(Pillar PRIVATE PIL_EXPORT)
target_compile_definitions(Pillar PRIVATE $<$<CONFIG:Debug>:PIL_ENABLE_ASSERTS>)

# Copy Pillar DLL next to the Sandbox executable whenever Pillar is built
add_custom_command(TARGET Pillar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXECUTABLE_OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:Pillar>
        ${EXECUTABLE_OUTPUT_DIR}
)









