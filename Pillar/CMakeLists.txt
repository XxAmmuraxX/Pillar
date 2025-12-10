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
    # Audio
    src/Pillar/Audio/AudioEngine.cpp
    src/Pillar/Audio/AudioBuffer.cpp
    src/Pillar/Audio/AudioSource.cpp
    src/Pillar/Audio/AudioClip.cpp
    src/Pillar/Audio/AudioListener.cpp
    src/Pillar/Audio/WavLoader.cpp
    # Platform - OpenAL
    src/Platform/OpenAL/OpenALContext.cpp
    src/Platform/OpenAL/OpenALBuffer.cpp
    src/Platform/OpenAL/OpenALSource.cpp
    # Renderer
    src/Pillar/Renderer/RenderAPI.cpp
    src/Pillar/Renderer/Renderer.cpp
    src/Pillar/Renderer/Renderer2DBackend.cpp
    src/Pillar/Renderer/RenderCommand.cpp
    src/Pillar/Renderer/Shader.cpp
    src/Pillar/Renderer/Buffer.cpp
    src/Pillar/Renderer/VertexArray.cpp
    src/Pillar/Renderer/Texture.cpp
    src/Pillar/Renderer/OrthographicCamera.cpp
    src/Pillar/Renderer/OrthographicCameraController.cpp
    src/Pillar/Renderer/BatchRenderer2D.cpp
    # Platform - OpenGL
    src/Platform/OpenGL/OpenGLRenderAPI.cpp
    src/Platform/OpenGL/OpenGLContext.cpp
    src/Platform/OpenGL/OpenGLShader.cpp
    src/Platform/OpenGL/OpenGLBuffer.cpp
    src/Platform/OpenGL/OpenGLVertexArray.cpp
    src/Platform/OpenGL/OpenGLTexture.cpp
    src/Platform/OpenGL/OpenGLBatchRenderer2D.cpp
    # ECS - Entity Component System
    src/Pillar/ECS/Scene.cpp
    src/Pillar/ECS/SceneManager.cpp
    src/Pillar/ECS/SceneSerializer.cpp
    src/Pillar/ECS/ComponentRegistry.cpp
    src/Pillar/ECS/BuiltinComponentRegistrations.cpp
    src/Pillar/ECS/ObjectPool.cpp
    src/Pillar/ECS/SpecializedPools.cpp
    src/Pillar/ECS/Components/Core/TagComponent.h
    src/Pillar/ECS/Components/Core/TransformComponent.h
    src/Pillar/ECS/Components/Core/UUIDComponent.h
    src/Pillar/ECS/Components/Core/HierarchyComponent.h
    # ECS - Physics Components
    src/Pillar/ECS/Components/Physics/RigidbodyComponent.h
    src/Pillar/ECS/Components/Physics/ColliderComponent.h
    src/Pillar/ECS/Components/Physics/VelocityComponent.h
    # ECS - Gameplay Components
    src/Pillar/ECS/Components/Gameplay/BulletComponent.h
    src/Pillar/ECS/Components/Gameplay/XPGemComponent.h
    src/Pillar/ECS/Components/Gameplay/ParticleComponent.h
    src/Pillar/ECS/Components/Gameplay/ParticleAnimationCurves.h
    src/Pillar/ECS/Components/Gameplay/ParticleEmitterComponent.h
    # ECS - Audio Components
    src/Pillar/ECS/Components/Audio/AudioSourceComponent.h
    src/Pillar/ECS/Components/Audio/AudioListenerComponent.h
    # ECS - Physics
    src/Pillar/ECS/Physics/Box2DWorld.cpp
    src/Pillar/ECS/Physics/Box2DBodyFactory.cpp
    src/Pillar/ECS/Physics/Box2DContactListener.cpp
    src/Pillar/ECS/Physics/SpatialHashGrid.cpp
    # ECS - Systems
    src/Pillar/ECS/Systems/System.h
    src/Pillar/ECS/Systems/PhysicsSystem.cpp
    src/Pillar/ECS/Systems/PhysicsSyncSystem.cpp
    src/Pillar/ECS/Systems/VelocityIntegrationSystem.cpp
    src/Pillar/ECS/Systems/BulletCollisionSystem.cpp
    src/Pillar/ECS/Systems/XPCollectionSystem.cpp
    src/Pillar/ECS/Systems/AudioSystem.cpp
    src/Pillar/ECS/Systems/SpriteRenderSystem.cpp
    src/Pillar/ECS/Systems/SpriteRenderSystem.h
    src/Pillar/ECS/Systems/ParticleSystem.cpp
    src/Pillar/ECS/Systems/ParticleSystem.h
    src/Pillar/ECS/Systems/ParticleEmitterSystem.cpp
    src/Pillar/ECS/Systems/ParticleEmitterSystem.h
    # ECS - Components (Rendering)
    src/Pillar/ECS/Components/Rendering/SpriteComponent.h
    src/Pillar/ECS/Components/Rendering/AnimationFrame.h
    src/Pillar/ECS/Components/Rendering/AnimationClip.h
    src/Pillar/ECS/Components/Rendering/AnimationComponent.h
    # Animation System
    src/Pillar/ECS/Systems/AnimationSystem.cpp
    src/Pillar/ECS/Systems/AnimationSystem.h
    src/Pillar/Utils/AnimationLoader.cpp
    src/Pillar/Utils/AnimationLoader.h
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
        EnTT::EnTT
        box2d
        OpenAL::OpenAL
        nlohmann_json::nlohmann_json
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









