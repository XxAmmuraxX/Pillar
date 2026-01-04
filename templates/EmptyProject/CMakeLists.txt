cmake_minimum_required(VERSION 3.16)
project(EmptyPillarProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer SDK path from env var if provided.
if(NOT DEFINED PILLAR_SDK_DIR AND DEFINED ENV{PILLAR_SDK_DIR})
  set(PILLAR_SDK_DIR "$ENV{PILLAR_SDK_DIR}")
endif()

# If this template is opened in-place inside the SDK tree, auto-detect the SDK root.
# (Safe when copied elsewhere: only activates if the expected config file exists.)
if(NOT DEFINED PILLAR_SDK_DIR)
  get_filename_component(_pillar_sdk_candidate "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
  if(EXISTS "${_pillar_sdk_candidate}/lib/cmake/Pillar/PillarConfig.cmake")
    set(PILLAR_SDK_DIR "${_pillar_sdk_candidate}")
  endif()
  unset(_pillar_sdk_candidate)
endif()
if(DEFINED PILLAR_SDK_DIR)
  list(PREPEND CMAKE_PREFIX_PATH "${PILLAR_SDK_DIR}")
endif()

find_package(Pillar CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
  src/main.cpp
  src/GameLayer.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE Pillar::Pillar)

# Copy assets post-build
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)
