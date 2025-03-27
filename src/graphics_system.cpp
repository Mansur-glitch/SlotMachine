// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "graphics_system.hpp"

#include "primitives.hpp"
#include "texture.hpp"
#include "utils.hpp"

#include <cstring>

GraphicsSystem::GraphicsSystem(std::string wnd_title,
                               uint16_t init_wnd_width,
                               uint16_t init_wnd_height)
{
  try {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      throw SdlError("Failed to initialise SDL");
    }

    m_wnd = SDL_CreateWindow(
      wnd_title.c_str(), init_wnd_width, init_wnd_height, SDL_WINDOW_RESIZABLE);
    if (m_wnd == nullptr) {
      throw SdlError("Failed to create window '%s'", wnd_title.c_str());
    }

    m_renderer = SDL_CreateRenderer(m_wnd, nullptr);
    if (m_renderer == nullptr) {
      throw SdlError("Failed to create renderer");
    }
  } catch (std::exception& ex) {
    this->~GraphicsSystem();
    throw ex;
  }
}

void GraphicsSystem::set_background_color(SDL_Color c)
{
  m_bg_color = c;
}

void GraphicsSystem::draw(const DrawQueue& q)
{
  SDL_SetRenderDrawColor(
    m_renderer, m_bg_color.r, m_bg_color.g, m_bg_color.b, m_bg_color.a);
  SDL_RenderClear(m_renderer);

  for (DrawInfo di : q) {
    if (di.texture_handler != nullptr) {
      SDL_RenderTexture(
        m_renderer, di.texture_handler, &di.tx_fragment, &di.bounds);
    } else {
      SDL_SetRenderDrawColor(
        m_renderer, di.color.r, di.color.g, di.color.b, di.color.a);
      SDL_RenderFillRect(m_renderer, &di.bounds);
    }
  }
  SDL_RenderPresent(m_renderer);
}

GraphicsSystem::~GraphicsSystem()
{
  if (m_renderer) {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }
  if (m_wnd) {
    SDL_DestroyWindow(m_wnd);
    m_wnd = nullptr;
  }
  SDL_Quit();
}
