# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025 Mansur Mukhametzyanov
cmake_minimum_required(VERSION 3.16.0)
project(slot_machine CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}" CACHE INTERNAL "")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  message("Build type set to Release")
endif()

find_package(Git QUIET)
if (NOT Git_FOUND)
  message(FATAL_ERROR "Git not found, can't initialize!")
endif ()

message("Clonning SDL3 library from git repository")
execute_process(
COMMAND ${GIT_EXECUTABLE} clone --depth 1 --branch release-3.2.8
  https://github.com/libsdl-org/SDL.git
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern
)
add_subdirectory(extern/SDL EXCLUDE_FROM_ALL)

message("Clonning SDL3 image library from git repository")
execute_process(
COMMAND ${GIT_EXECUTABLE} clone --depth 1 --branch release-3.2.4 --recurse-submodules
  https://github.com/libsdl-org/SDL_image.git
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/extern
)

# Disable unused image formats
set(SDLIMAGE_VENDORED ON)
set(SDLIMAGE_TIF OFF)
set(SDLIMAGE_AVIF OFF)
set(SDLIMAGE_BMP OFF)
set(SDLIMAGE_JPEG OFF)
set(SDLIMAGE_WEBP OFF)
set(SDLIMAGE_GIF OFF)
set(SDLIMAGE_JPG OFF)
set(SDLIMAGE_LBM OFF)
set(SDLIMAGE_PCX OFF)
set(SDLIMAGE_PNG OFF)
set(SDLIMAGE_PNM OFF)
set(SDLIMAGE_QOI OFF)
set(SDLIMAGE_TGA OFF)
set(SDLIMAGE_XCF OFF)
set(SDLIMAGE_XPM OFF)
set(SDLIMAGE_XV OFF)
set(SDLIMAGE_SVG ON)

add_subdirectory(extern/SDL_image EXCLUDE_FROM_ALL)

# Add game executable target
add_subdirectory(src)

# Copy images to build directory
add_subdirectory(image_resources)
