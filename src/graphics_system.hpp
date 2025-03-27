// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_GRAPHICS_SYSTEM
#define SLOT_MACHINE_GRAPHICS_SYSTEM

#include "primitives.hpp"

#include <SDL3/SDL.h>

#include <cstdint>

class TextureCollection;
class GraphicsSystem
{
public:
  GraphicsSystem(std::string wnd_title,
                 uint16_t init_wnd_width,
                 uint16_t init_wnd_height);
  void run();
  ~GraphicsSystem();
  SDL_Renderer* get_renderer() noexcept { return m_renderer; }
  void set_background_color(SDL_Color c);
  void draw(const DrawQueue& q);

private:
  SDL_Window* m_wnd{ nullptr };
  SDL_Renderer* m_renderer{ nullptr };
  SDL_Color m_bg_color{ 0, 0, 0, 255 };
};
#endif
