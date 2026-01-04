# Pillar Engine - Distribution & Installer Guide

This document outlines strategies for creating an installer/distribution system for the Pillar Engine, allowing users to start creating games without cloning the entire repository.

---

## Table of Contents

1. [Overview](#overview)
2. [Distribution Models](#distribution-models)
3. [What Needs to be Distributed](#what-needs-to-be-distributed)
4. [Option 1: SDK-Style Distribution (Recommended)](#option-1-sdk-style-distribution-recommended)
5. [Option 2: Template Project via GitHub](#option-2-template-project-via-github)
---

## Overview

### Current Situation

Currently, to use the Pillar Engine, developers must:
1. Clone the entire repository (~60+ source files + dependencies)
2. Build the engine from source
3. Understand the complex CMake setup with FetchContent
4. Modify the Sandbox project or create their own

### Goals

Create a distribution that allows users to:
- Download a pre-built SDK or installer
- Create a new game project with a simple template
- Use the engine without building it from source
- Access the Editor (PillarEditor) as a standalone application

---

## Distribution Models

| Model | Complexity | User Experience | Maintenance |
|-------|------------|-----------------|-------------|
| **SDK ZIP Download** | Low | Medium | Low |
| **GitHub Template** | Low | High | Low |
| **Windows Installer** | High | Excellent | Medium |
| **vcpkg/Conan Package** | Medium | Good (for C++ devs) | Medium |
| **NuGet Package** | Medium | Good (Windows devs) | Medium |

---

## What Needs to be Distributed

### Core Engine (Required)

| Component | Files | Notes |
|-----------|-------|-------|
| **Pillar Library** | `Pillar.lib`, `Pillar.dll` (if shared) | Pre-compiled engine |
| **Header Files** | `Pillar/src/Pillar/**/*.h` | Public API headers |
| **Third-Party Headers** | GLM, EnTT, spdlog headers | Required for compilation |

### Editor (Optional but Recommended)

| Component | Files | Notes |
|-----------|-------|-------|
| **Editor Executable** | `PillarEditor.exe` | Standalone editor |
| **Editor Assets** | `PillarEditor/assets/**` | Icons, textures |
| **ImGuizmo** | Bundled in editor | Transform gizmos |

### Runtime Dependencies

| Dependency | Type | Distribution |
|------------|------|--------------|
| **OpenAL-Soft** | DLL | `OpenAL32.dll` (or static) |
| **GLFW** | Static | Linked into Pillar |
| **GLAD** | Static | Linked into Pillar |
| **spdlog** | Static | Linked into Pillar |
| **ImGui** | Static | Linked into Pillar |
| **Box2D** | Static | Linked into Pillar |
| **nlohmann_json** | Header-only | Bundled |
| **EnTT** | Header-only | Bundled |
| **GLM** | Header-only | Bundled |
| **stb_image** | Header-only | Bundled |

### Template/Starter Project

| Component | Purpose |
|-----------|---------|
| **CMakeLists.txt** | Simple CMake for user's game |
| **main.cpp** | Entry point template |
| **GameLayer.h/cpp** | Example game layer |
| **assets/** | Empty asset directories |

---

## Option 1: SDK-Style Distribution (Recommended)

### Description

Create a downloadable SDK package (ZIP) containing:
- Pre-built binaries (Debug/Release)
- All required headers
- CMake config files for easy integration
- Project template
- Editor executable

### SDK Directory Structure

```
PillarSDK-1.0.0-Windows-x64/
├── bin/
│   ├── Debug/
│   │   ├── Pillar.lib
│   │   ├── PillarEditor.exe
│   │   └── OpenAL32.dll (if dynamic)
│   └── Release/
│       ├── Pillar.lib
│       ├── PillarEditor.exe
│       └── OpenAL32.dll (if dynamic)
├── include/
│   ├── Pillar/
│   │   ├── Application.h
│   │   ├── Core.h
│   │   ├── EntryPoint.h
│   │   ├── Events/
│   │   ├── ECS/
│   │   ├── Renderer/
│   │   ├── Audio/
│   │   └── Utils/
│   └── thirdparty/
│       ├── glm/
│       ├── entt/
│       ├── spdlog/
│       └── nlohmann/
├── lib/
│   ├── Debug/
│   │   └── Pillar.lib
│   └── Release/
│       └── Pillar.lib
├── cmake/
│   ├── PillarConfig.cmake
│   ├── PillarConfigVersion.cmake
│   └── PillarTargets.cmake
├── templates/
│   └── EmptyProject/
│       ├── CMakeLists.txt
│       ├── src/
│       │   ├── main.cpp
│       │   └── GameLayer.h
│       └── assets/
│           ├── textures/
│           ├── audio/
│           └── scenes/
├── editor/
│   ├── PillarEditor.exe
│   └── assets/
│       └── icons/
├── docs/
│   ├── GettingStarted.md
│   ├── API_Reference.md
│   └── Examples.md
├── LICENSE.txt
└── README.md
```

### CMake Config File (PillarConfig.cmake)

```cmake
# PillarConfig.cmake - CMake configuration for Pillar Engine SDK

if(NOT TARGET Pillar::Pillar)
    # Get the directory where this file is located
    get_filename_component(PILLAR_SDK_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
    
    # Create imported target
    add_library(Pillar::Pillar STATIC IMPORTED)
    
    # Set library location based on build type
    set_target_properties(Pillar::Pillar PROPERTIES
        IMPORTED_LOCATION_DEBUG "${PILLAR_SDK_DIR}/lib/Debug/Pillar.lib"
        IMPORTED_LOCATION_RELEASE "${PILLAR_SDK_DIR}/lib/Release/Pillar.lib"
        IMPORTED_LOCATION_RELWITHDEBINFO "${PILLAR_SDK_DIR}/lib/Release/Pillar.lib"
        IMPORTED_LOCATION_MINSIZEREL "${PILLAR_SDK_DIR}/lib/Release/Pillar.lib"
    )
    
    # Set include directories
    set_target_properties(Pillar::Pillar PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES 
            "${PILLAR_SDK_DIR}/include;${PILLAR_SDK_DIR}/include/thirdparty"
    )
    
    # Platform-specific dependencies
    if(WIN32)
        set_target_properties(Pillar::Pillar PROPERTIES
            INTERFACE_LINK_LIBRARIES "opengl32"
        )
    endif()
    
    # Compile definitions
    set_target_properties(Pillar::Pillar PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "PIL_STATIC_LIB;PIL_WINDOWS"
    )
    
    # Set SDK root for asset paths
    set(PILLAR_SDK_ROOT "${PILLAR_SDK_DIR}" CACHE PATH "Pillar SDK root directory")
    set(PILLAR_EDITOR "${PILLAR_SDK_DIR}/editor/PillarEditor.exe" CACHE FILEPATH "Pillar Editor executable")
endif()
```

> Note: Keep public engine headers platform-agnostic. For example, `Application.h` should include `Window.h` (the interface) rather than `Platform/WindowsWindow.h`, otherwise SDK consumers will fail to compile unless you ship platform headers too.

### User's Game CMakeLists.txt (Template)

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Pillar SDK
# Set PILLAR_SDK_DIR to your SDK location, or add to CMAKE_PREFIX_PATH
find_package(Pillar REQUIRED 
    PATHS "${CMAKE_SOURCE_DIR}/../PillarSDK" 
          "C:/PillarSDK"
          "$ENV{PILLAR_SDK_DIR}"
)

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/GameLayer.cpp
)

# Link to Pillar
target_link_libraries(${PROJECT_NAME} PRIVATE Pillar::Pillar)

# Copy assets post-build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
```

### How to Build the SDK

Add this to your CMakeLists.txt or create a separate script:

```cmake
# In CMakeLists.txt - Add install targets
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install Pillar library
install(TARGETS Pillar
    EXPORT PillarTargets
    ARCHIVE DESTINATION lib/$<CONFIG>
    LIBRARY DESTINATION lib/$<CONFIG>
    RUNTIME DESTINATION bin/$<CONFIG>
)

# Install headers
install(DIRECTORY Pillar/src/Pillar/
    DESTINATION include/Pillar
    FILES_MATCHING PATTERN "*.h"
)

# Install third-party headers
install(DIRECTORY ${glm_SOURCE_DIR}/glm/
    DESTINATION include/thirdparty/glm
)
install(DIRECTORY ${entt_SOURCE_DIR}/src/entt/
    DESTINATION include/thirdparty/entt
)
install(DIRECTORY ${spdlog_SOURCE_DIR}/include/spdlog/
    DESTINATION include/thirdparty/spdlog
)
install(DIRECTORY ${nlohmann_json_SOURCE_DIR}/include/nlohmann/
    DESTINATION include/thirdparty/nlohmann
)

# Install Editor
install(TARGETS PillarEditor
    RUNTIME DESTINATION editor
)
install(DIRECTORY PillarEditor/assets/
    DESTINATION editor/assets
)

# Generate and install CMake config
configure_package_config_file(
    cmake/PillarConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/PillarConfig.cmake"
    INSTALL_DESTINATION cmake
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/PillarConfig.cmake"
    DESTINATION cmake
)

install(EXPORT PillarTargets
    FILE PillarTargets.cmake
    NAMESPACE Pillar::
    DESTINATION cmake
)
```

### Build Commands for SDK

```powershell
# Build Release
cmake -S . -B build-release -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release --parallel

# Build Debug
cmake -S . -B build-debug -G "Ninja" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug --config Debug --parallel

# Install to SDK directory
cmake --install out/build/x64-Debug --prefix out/sdk/PillarSDK

```

---

## Option 2: Template Project via GitHub

### Description

Create a separate GitHub repository that serves as a template for new projects.

### Setup

1. Create a new repo: `Pillar-Game-Template`
2. Mark it as a "Template repository" in GitHub settings
3. Users click "Use this template" to create their own game repo

### Template Repository Structure

```
Pillar-Game-Template/
├── .github/
│   └── workflows/
│       └── build.yml              # CI for user's game
├── cmake/
│   └── FetchPillar.cmake          # Downloads Pillar automatically
├── src/
│   ├── main.cpp
│   └── GameLayer.h
├── assets/
│   ├── textures/
│   ├── audio/
│   └── scenes/
├── CMakeLists.txt
├── .gitignore
└── README.md
```

### FetchPillar.cmake

```cmake
# cmake/FetchPillar.cmake
include(FetchContent)

FetchContent_Declare(
    pillar
    GIT_REPOSITORY https://github.com/XxAmmuraxX/Pillar.git
    GIT_TAG v1.0.0  # Use specific release tag
    GIT_SHALLOW TRUE
)

set(PILLAR_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PILLAR_BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
set(PILLAR_BUILD_EDITOR OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(pillar)
```

### Template CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Pillar Engine
include(cmake/FetchPillar.cmake)

# Game executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Pillar)
target_compile_definitions(${PROJECT_NAME} PRIVATE PIL_STATIC_LIB)

# Asset management
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
```

### Pros & Cons

| Pros | Cons |
|------|------|
| Easy to use (one click) | Requires building engine from source |
| Auto-updates possible | Longer initial build time |
| No manual download | Requires CMake knowledge |
| Version control for user | Large download (all source) |

---

## Summary

| Approach | Best For | Start With |
|----------|----------|------------|
| **SDK ZIP** | General distribution, ease of use | ✅ Yes |
| **GitHub Template** | Developers, version control | ✅ Yes |

The key is to separate the engine (Pillar) from user projects, provide pre-built binaries, and include a simple project template that users can start with immediately.
