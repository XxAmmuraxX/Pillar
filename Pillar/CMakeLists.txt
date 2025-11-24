# Added this line for clarity about file location
# This is pillar cmake
# ==============================
# Pillar Engine (Static Library)
# ==============================
add_library(Pillar STATIC
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
    src/Pillar/Renderer/OrthographicCameraController.cpp
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
        ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${LIBRARY_OUTPUT_DIR}
    )
endforeach()

# Set output directories
set_target_properties(Pillar PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_DIR}
)

# Include directories
target_include_directories(Pillar PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${stb_SOURCE_DIR}
)

# Link libraries - all PUBLIC for static linking
target_link_libraries(Pillar 
    PUBLIC 
        spdlog::spdlog
        glfw
        glad_gl_core_46
        glm::glm
        imgui
)

# For Windows, link additional system libraries that GLFW needs
if(WIN32)
    target_link_libraries(Pillar PUBLIC
        opengl32
    )
endif()

# No longer need PIL_EXPORT for static library
# Remove DLL export definitions
target_compile_definitions(Pillar PUBLIC PIL_STATIC_LIB)









