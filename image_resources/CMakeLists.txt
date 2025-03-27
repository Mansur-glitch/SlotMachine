# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright © 2025 Mansur Mukhametzyanov
cmake_minimum_required(VERSION 3.16.0)

set(images_source "${CMAKE_CURRENT_SOURCE_DIR}/")
set(images_output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/image_resources")
set(images 0.svg 1.svg 2.svg 3.svg 4.svg 5.svg 6.svg 7.svg 8.svg 9.svg
           amethyst.svg apple.svg background.svg carrot.svg club.svg
           corn.svg cross.svg crystal.svg diamond.svg emerald.svg grape.svg
           heart.svg lucky_seven.svg question.svg respin.svg spade.svg
           start_grey.svg start_white.svg stop_grey.svg stop_white.svg
           topaz.svg)

list(TRANSFORM images PREPEND ${images_source})

add_custom_target(copy_images ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory
      ${images_output}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${images} ${images_output}
)
