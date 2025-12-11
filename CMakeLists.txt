# Added this line for clarity about file location
# This is root cmake
cmake_minimum_required(VERSION 3.5)
project(Pillar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Platform macro
if (WIN32)
    add_definitions(-DPIL_WINDOWS)
endif()

# Set output structure
set(BINARY_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/bin/Debug-x64)
set(LIBRARY_OUTPUT_DIR ${BINARY_OUTPUT_DIR}/Pillar)
set(EXECUTABLE_OUTPUT_DIR ${BINARY_OUTPUT_DIR}/Sandbox)

# Note: Don't set global output directories here - it breaks FetchContent dependencies
# Output directories are set per-target in subdirectories

# Check for Python and jinja2 (required by GLAD2)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
if(Python3_FOUND)
    message(STATUS "Found Python: ${Python3_EXECUTABLE}")
    
    # Check if jinja2 is installed
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -c "import jinja2"
        RESULT_VARIABLE JINJA2_CHECK
        OUTPUT_QUIET
        ERROR_QUIET
    )
    
    if(NOT JINJA2_CHECK EQUAL 0)
        message(FATAL_ERROR 
            "Python package 'jinja2' is required but not found!\n"
            "Please install it using:\n"
            "  ${Python3_EXECUTABLE} -m pip install jinja2\n"
            "Or on Windows:\n"
            "  python -m pip install jinja2"
        )
    else()
        message(STATUS "Found jinja2 (required for GLAD2)")
    endif()
else()
    message(FATAL_ERROR "Python 3 is required to build this project (needed for GLAD2 code generation)")
endif()

# Fetch external dependencies
include(FetchContent)

# spdlog
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

# GLAD2 - Fetch the generator
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.8
  GIT_SHALLOW TRUE
  SOURCE_SUBDIR cmake
)

# GLM 
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863 # 1.0.1
)

# stb_image (single header library for image loading)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
)

# EnTT (Entity-Component-System library)
FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG v3.12.2
)

# Box2D (2D physics engine)
FetchContent_Declare(
  box2d
  GIT_REPOSITORY https://github.com/erincatto/box2d.git
  GIT_TAG v2.4.1
)

# OpenAL-Soft (cross-platform audio library)
FetchContent_Declare(
  openal
  GIT_REPOSITORY https://github.com/kcat/openal-soft.git
  GIT_TAG 1.24.3
)
# nlohmann/json (for scene serialization)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)

# Configure GLFW options before making it available
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Configure GLM options
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

# Configure Box2D options
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)

# Configure OpenAL-Soft options
set(ALSOFT_UTILS OFF CACHE BOOL "" FORCE)
set(ALSOFT_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_CONFIG OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_HRTF_DATA OFF CACHE BOOL "" FORCE)
set(ALSOFT_INSTALL_AMBDEC_PRESETS OFF CACHE BOOL "" FORCE)
set(LIBTYPE "STATIC" CACHE STRING "" FORCE) 

# Configure nlohmann_json options
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)

# Dear ImGui (docking)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
)

# ImGuizmo (for transform gizmos)
FetchContent_Declare(
  imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG master
)

# Make dependencies available (except glad - we'll handle it specially)
FetchContent_MakeAvailable(spdlog glfw glm stb imgui imguizmo entt box2d nlohmann_json)

# Now make GLAD available and generate the loader
FetchContent_MakeAvailable(glad)

# Generate GLAD loader for OpenGL 4.6 Core
glad_add_library(glad_gl_core_46 REPRODUCIBLE API gl:core=4.6)

# Create imgui library from sources
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)

target_link_libraries(imgui PUBLIC glfw glad_gl_core_46)

# Create ImGuizmo library
set(IMGUIZMO_DIR ${imguizmo_SOURCE_DIR})
add_library(imguizmo STATIC
    ${IMGUIZMO_DIR}/ImGuizmo.cpp
)

target_include_directories(imguizmo PUBLIC
    ${IMGUIZMO_DIR}
)

target_link_libraries(imguizmo PUBLIC imgui)

# Add subdirectories
add_subdirectory(Pillar)
add_subdirectory(Sandbox)
add_subdirectory(PillarEditor)
add_subdirectory(Tests)
