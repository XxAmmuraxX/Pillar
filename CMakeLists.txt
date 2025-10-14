cmake_minimum_required(VERSION 3.16)
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

# Force single-config behavior for all targets
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIR})

# Override per-configuration directories
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endforeach()

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

# GLAD
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v0.1.36
)

# Configure GLAD before fetching
set(GLAD_PROFILE "core" CACHE STRING "")
set(GLAD_API "gl=4.6" CACHE STRING "")
set(GLAD_GENERATOR "c" CACHE STRING "")
set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)

# Configure GLFW options before making it available
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Make dependencies available
FetchContent_MakeAvailable(spdlog glfw glad)

# Add subdirectories
add_subdirectory(Pillar)
add_subdirectory(Sandbox)
