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
FetchContent_MakeAvailable(spdlog glfw glm stb imgui imguizmo entt box2d openal nlohmann_json)

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


# ==============================
# SDK Install & Packaging
# ==============================
option(PILLAR_ENABLE_SDK_INSTALL "Enable SDK-style install/export targets" ON)

if(PILLAR_ENABLE_SDK_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # Keep this independent from project() VERSION to avoid forcing a repo-wide version policy.
  set(PILLAR_PACKAGE_VERSION "0.1.0")
  set(PILLAR_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Pillar")

  # --- Install binaries/libraries ---
  if(TARGET Pillar)
    install(TARGETS Pillar
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/$<CONFIG>"
    )
  endif()

  # Install third-party libraries that Pillar depends on.
  # We intentionally do NOT export these CMake targets (their include dirs often point to build-machine paths).
  set(_PILLAR_SDK_DEP_LIB_TARGETS "")
  foreach(_t IN ITEMS glfw glad_gl_core_46 imgui box2d OpenAL spdlog)
    if(TARGET ${_t})
      list(APPEND _PILLAR_SDK_DEP_LIB_TARGETS ${_t})
    endif()
  endforeach()
  if(_PILLAR_SDK_DEP_LIB_TARGETS)
    install(TARGETS ${_PILLAR_SDK_DEP_LIB_TARGETS}
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/$<CONFIG>"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/$<CONFIG>"
    )
  endif()

  if(TARGET PillarEditor)
    install(TARGETS PillarEditor
      RUNTIME DESTINATION "editor"
    )

    if(EXISTS "${CMAKE_SOURCE_DIR}/PillarEditor/assets")
      install(DIRECTORY "${CMAKE_SOURCE_DIR}/PillarEditor/assets/"
        DESTINATION "editor/assets"
      )
    endif()
  endif()

  # --- Install engine headers ---
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/Pillar/src/Pillar/"
    DESTINATION "include/Pillar"
    FILES_MATCHING PATTERN "*.h"
  )
  install(FILES "${CMAKE_SOURCE_DIR}/Pillar/src/Pillar.h"
    DESTINATION "include"
  )

  # --- Install third-party headers required by public API ---
  # Note: these are FetchContent source dirs; they exist after FetchContent_MakeAvailable().
  if(DEFINED glm_SOURCE_DIR)
    install(DIRECTORY "${glm_SOURCE_DIR}/glm/"
      DESTINATION "include/thirdparty/glm"
    )
  endif()
  if(DEFINED entt_SOURCE_DIR)
    install(DIRECTORY "${entt_SOURCE_DIR}/src/entt/"
      DESTINATION "include/thirdparty/entt"
    )
  endif()
  if(DEFINED spdlog_SOURCE_DIR)
    install(DIRECTORY "${spdlog_SOURCE_DIR}/include/"
      DESTINATION "include/thirdparty"
    )
  endif()
  if(DEFINED nlohmann_json_SOURCE_DIR)
    install(DIRECTORY "${nlohmann_json_SOURCE_DIR}/include/nlohmann/"
      DESTINATION "include/thirdparty/nlohmann"
    )
  endif()
  if(DEFINED box2d_SOURCE_DIR)
    if(EXISTS "${box2d_SOURCE_DIR}/include/box2d")
      install(DIRECTORY "${box2d_SOURCE_DIR}/include/box2d/"
        DESTINATION "include/thirdparty/box2d"
      )
    endif()
  endif()

  # --- Install templates/docs/license (added in later steps) ---
  if(EXISTS "${CMAKE_SOURCE_DIR}/templates")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/templates/"
      DESTINATION "templates"
    )
  endif()

  if(EXISTS "${CMAKE_SOURCE_DIR}/docs")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/docs/"
      DESTINATION "docs"
    )
  endif()

  if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE.txt")
    install(FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt" DESTINATION ".")
  endif()

  # --- CMake package config for find_package(Pillar CONFIG) ---
  configure_package_config_file(
    "${CMAKE_SOURCE_DIR}/cmake/PillarConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PillarConfig.cmake"
    INSTALL_DESTINATION "${PILLAR_INSTALL_CMAKEDIR}"
  )

  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PillarConfigVersion.cmake"
    VERSION "${PILLAR_PACKAGE_VERSION}"
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PillarConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PillarConfigVersion.cmake"
    DESTINATION "${PILLAR_INSTALL_CMAKEDIR}"
  )

  # --- CPack ZIP generator (creates an SDK-style ZIP from install rules) ---
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_PACKAGE_NAME "PillarSDK")
  set(CPACK_PACKAGE_VENDOR "Pillar")
  set(CPACK_PACKAGE_VERSION "${PILLAR_PACKAGE_VERSION}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  include(CPack)
endif()
