# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025 Mansur Mukhametzyanov
cmake_minimum_required(VERSION 3.16.0)

add_executable(game utils.cpp primitives.cpp animation.cpp scene.cpp
               graphics_system.cpp texture.cpp combination.cpp game.cpp
               main.cpp)
target_link_libraries(game PRIVATE SDL3_image::SDL3_image SDL3::SDL3)
target_compile_features(game PUBLIC cxx_std_17)

