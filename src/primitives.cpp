// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "primitives.hpp"

#include "texture.hpp"

#include <SDL3/SDL_pixels.h>
#include <array>
#include <cassert>
#include <cstdint>

Box<int> Grid::get_cell_box(uint16_t row, uint16_t column) const noexcept
{
  return Box<int>{ x + cell_w * column, y + cell_h * row, cell_w, cell_h };
}

Box<int> Grid::get_box() const noexcept
{
  return { x, y, cell_w * colulmns, cell_h * rows };
}


FramedGrid FramedGrid::centered_in(Box<int> box,
                                   uint16_t n_rows,
                                   uint16_t n_columns) noexcept
{
  assert(n_columns > 0 && n_rows > 0);

  int column_max_width = box.w / n_columns;
  int row_max_height = box.h / n_rows;

  uint16_t cell_side = std::min(column_max_width, row_max_height);
  uint16_t grid_width = cell_side * n_columns;
  uint16_t grid_height = cell_side * n_rows;
  uint16_t extra_width = box.w - grid_width;
  uint16_t extra_height = box.h - grid_height;

  FramedGrid fg;
  fg.frame.left = extra_width / 2;
  fg.frame.top = extra_height / 2;
  fg.frame.right = (extra_width + 1) / 2;
  fg.frame.bottom = (extra_height + 1) / 2;

  fg.rows = n_rows;
  fg.colulmns = n_columns;
  fg.x = box.x + fg.frame.left;
  fg.y = box.y + fg.frame.top;
  fg.cell_h = fg.cell_w = cell_side;

  return fg;
}

FramedGrid FramedGrid::centered_in(FramedBox<int> fb,
                                   uint16_t n_rows,
                                   uint16_t n_columns) noexcept
{
  FramedGrid fg =
    FramedGrid::centered_in(fb.get_inner_box(), n_rows, n_columns);
  fg.frame.add(fb.frame);
  return fg;
}

FramedBox<int> FramedGrid::get_framed_box() const noexcept
{
  return FramedBox<int>::create_outside(get_box(), frame);
}


DrawInfo::DrawInfo() noexcept
  : bounds({ 0, 0, 0, 0 })
  , color({ 0, 0, 0, 255 })
  , texture_handler(nullptr)
{
}

DrawInfo::DrawInfo(Box<int> bounds) noexcept
  : bounds(box_to_sdl_frect(bounds))
  , color({ 0, 0, 0, 255 })
  , texture_handler(nullptr)
{
}

DrawInfo::DrawInfo(Box<int> bounds, SDL_Color color) noexcept
  : bounds(box_to_sdl_frect(bounds))
  , color(color)
  , texture_handler(nullptr)
{
}

DrawInfo::DrawInfo(Box<int> bounds,
                   Texture& texture,
                   Box<float> tx_fragment) noexcept
  : bounds(box_to_sdl_frect(bounds))
  , texture_handler(texture.get_handler())
  , tx_fragment(box_to_sdl_frect(tx_fragment))
{
  // Texture coordinates scaling by texture size
  this->tx_fragment.x *= static_cast<float>(texture.get_width());
  this->tx_fragment.w *= static_cast<float>(texture.get_width());
  this->tx_fragment.y *= static_cast<float>(texture.get_height());
  this->tx_fragment.h *= static_cast<float>(texture.get_height());
}


DrawQueue::DrawQueue(TextureCollection& tc, uint32_t reserved)
  : m_tx_collection(tc)
{
  if (reserved > 0) {
    m_queue.reserve(reserved);
  }
}

DrawQueue& DrawQueue::add_colored_box(Box<int> b, SDL_Color color)
{
  if (b.area() > 0) {
    m_queue.push_back(DrawInfo(b, color));
  }
  return *this;
}

DrawQueue& DrawQueue::add_textured_box(Box<int> b,
                                       TextureId tx_id,
                                       Box<float> tx_fragment)
{
  if (b.area() > 0) {
    m_queue.push_back(
      DrawInfo(b, m_tx_collection.get_texture(tx_id), tx_fragment));
  }
  return *this;
}

DrawQueue& DrawQueue::add_colored_frame(FramedBox<int> fb, SDL_Color color)
{
  std::array<Box<int>, 4> bases = fb.decomposed();
  for (uint32_t i = 0; i < bases.size(); ++i) {
    if (bases[i].area() > 0) {
      m_queue.emplace_back(bases[i], color);
    }
  }
  return *this;
}

DrawQueue& DrawQueue::add_textured_frame(FramedBox<int> fb, TextureId tx_id)
{
  assert(fb.full_box.area() > 0);

  FrameSize<float> frame_relative = fb.frame.cast_to<float>();
  Box<float> float_box = fb.get_full_box().cast_to<float>();
  frame_relative.left /= float_box.w;
  frame_relative.top /= float_box.h;
  frame_relative.right /= float_box.w;
  frame_relative.bottom /= float_box.h;

  std::array<Box<int>, 4> bases = fb.decomposed();
  std::array<Box<float>, 4> tx_fragments =
    FramedBox<float>::create_inside({ 0.f, 0.f, 1.f, 1.f }, frame_relative)
      .decomposed();

  for (uint32_t i = 0; i < bases.size(); ++i) {
    if (bases[i].area() > 0) {
      m_queue.emplace_back(
        bases[i], m_tx_collection.get_texture(tx_id), tx_fragments[i]);
    }
  }
  return *this;
}
