// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_PRIMITIVES
#define SLOT_MACHINE_PRIMITIVES

#include "texture.hpp"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_rect.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

template<class T>
struct Box
{
  T x;
  T y;
  T w;
  T h;

  T area() const noexcept { return w * h; }

  void indent(T val) noexcept
  {
    x += val;
    y += val;
    w -= val;
    h -= val;
  }

  bool contains(T px, T py) const noexcept
  {
    return x <= px && px <= (x + w) && y <= py && py <= (y + h);
  }

  template<class U>
  Box<U> cast_to() const noexcept
  {
    return {
      static_cast<U>(x), static_cast<U>(y), static_cast<U>(w), static_cast<U>(h)
    };
  }
};


template<class T>
struct FrameSize
{
  T left;
  T top;
  T right;
  T bottom;

  FrameSize() noexcept = default;

  FrameSize(T left, T top, T right, T bottom) noexcept
    : left(left)
    , top(top)
    , right(right)
    , bottom(bottom)
  {
  }

  explicit FrameSize(T val) noexcept
    : left(val)
    , top(val)
    , right(val)
    , bottom(val)
  {
  }

  static constexpr T zero{};
  bool is_valid() const noexcept
  {
    return left >= zero && top >= zero && right >= zero && bottom >= zero;
  }

  bool is_null() const noexcept
  {
    return left == zero && top == zero && right == zero && bottom == zero;
  }

  FrameSize<T>& add(FrameSize<T> rhs) noexcept
  {
    this->left += rhs.left;
    this->top += rhs.top;
    this->right += rhs.right;
    this->bottom += rhs.bottom;
    return *this;
  }

  template<class U>
  FrameSize<U> cast_to() const noexcept
  {
    return { static_cast<U>(left),
             static_cast<U>(top),
             static_cast<U>(right),
             static_cast<U>(bottom) };
  }
};


template<class T>
struct FramedBox
{
  Box<T> full_box;
  FrameSize<T> frame;

  FramedBox() noexcept = default;

  static FramedBox<T> create_inside(Box<T> full_box,
                                    FrameSize<T> frame) noexcept
  {
    return FramedBox<T>{ full_box, frame };
  }

  static FramedBox<T> create_outside(Box<T> inner_box,
                                     FrameSize<T> frame) noexcept
  {
    return FramedBox<T>{ Box<T>{ inner_box.x - frame.left,
                                 inner_box.y - frame.top,
                                 inner_box.w + frame.left + frame.right,
                                 inner_box.h + frame.top + frame.bottom },
                         FrameSize<T>(frame) };
  }

  Box<T> get_full_box() const noexcept { return full_box; }
  Box<T> get_inner_box() const noexcept
  {
    return { full_box.x + frame.left,
             full_box.y + frame.top,
             full_box.w - frame.left - frame.right,
             full_box.h - frame.top - frame.bottom };
  }

  bool frame_fits_box() const noexcept
  {
    return frame.left + frame.right <= full_box.w &&
           frame.top + frame.bottom <= full_box.h;
  }

  std::array<Box<T>, 4> decomposed() const noexcept
  {
    assert(frame.is_valid());
    assert(frame_fits_box());

    T side_y = full_box.y + frame.top;
    T side_h = full_box.h - frame.top - frame.bottom;
    T right_x = full_box.x + full_box.w - frame.right;
    T bottom_y = full_box.y + full_box.h - frame.bottom;

    return std::array<Box<T>, 4>{
      Box<T>{ full_box.x, side_y, frame.left, side_h },
      Box<T>{ full_box.x, full_box.y, full_box.w, frame.top },
      Box<T>{ right_x, side_y, frame.right, side_h },
      Box<T>{ full_box.x, bottom_y, full_box.w, frame.bottom }
    };
  }

private:
  FramedBox(Box<T> full_box, FrameSize<T> frame_size) noexcept
    : full_box(full_box)
    , frame(frame_size)
  {
  }
};


struct Grid
{
  int x;
  int y;
  int cell_w;
  int cell_h;
  uint16_t rows;
  uint16_t colulmns;

  Box<int> get_cell_box(uint16_t row, uint16_t column) const noexcept;
  Box<int> get_box() const noexcept;
};


struct FramedGrid : public Grid
{
  FrameSize<int> frame;

  FramedBox<int> get_framed_box() const noexcept;

  static FramedGrid centered_in(Box<int> box,
                                uint16_t n_rows,
                                uint16_t n_columns) noexcept;
  static FramedGrid centered_in(FramedBox<int> fb,
                                uint16_t n_rows,
                                uint16_t n_columns) noexcept;
};


struct DrawInfo
{
  DrawInfo() noexcept;
  DrawInfo(Box<int> bounds) noexcept;
  DrawInfo(Box<int> bounds, SDL_Color color) noexcept;
  DrawInfo(Box<int> bounds,
           Texture& texture,
           Box<float> tx_fragment = { 0.f, 0.f, 1.f, 1.f }) noexcept;

  template<class T>
  static SDL_FRect box_to_sdl_frect(Box<T> box) noexcept
  {
    Box<float> fbox = box.template cast_to<float>();
    SDL_FRect sdl_box;
    std::memcpy(&sdl_box, &fbox, sizeof(fbox));

    return sdl_box;
  }

  SDL_FRect bounds;
  union
  {
    SDL_FRect tx_fragment;
    SDL_Color color;
  };
  SDL_Texture* texture_handler;
};


class DrawQueue
{
public:
  DrawQueue(TextureCollection& tc, uint32_t reserved = 0);
  DrawQueue& add_textured_box(Box<int> b,
                              TextureId tx_id,
                              Box<float> tx_fragment = { 0.f, 0.f, 1.f, 1.f });
  DrawQueue& add_colored_box(Box<int> b, SDL_Color color);
  DrawQueue& add_colored_frame(FramedBox<int> fb, SDL_Color color);
  DrawQueue& add_textured_frame(FramedBox<int> fb, TextureId tx_id);
  std::vector<DrawInfo>::const_iterator begin() const
  {
    return m_queue.begin();
  }
  std::vector<DrawInfo>::const_iterator end() const { return m_queue.end(); }

private:
  std::vector<DrawInfo> m_queue;
  TextureCollection& m_tx_collection;
};
#endif
