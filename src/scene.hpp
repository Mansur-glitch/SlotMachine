// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#ifndef SLOT_MACHINE_SCENE
#define SLOT_MACHINE_SCENE

#include "animation.hpp"
#include "primitives.hpp"
#include "texture.hpp"

#include <array>
#include <cstdint>
#include <functional>

class Drawable
{
public:
  virtual void draw(DrawQueue& queue, Box<int> bounds) const = 0;
  virtual ~Drawable() {};
};


class DrawableBox : public Drawable
{
public:
  DrawableBox(SDL_Color color = { 0, 0, 0, 0 }) noexcept;
  DrawableBox(TextureId texture_id) noexcept;
  void draw(DrawQueue& queue, Box<int> bounds) const override;
  // Doesn't support textured frames
  void draw_clipped(DrawQueue& queue, Box<int> bounds, Box<int> visible) const;
  void set_cover_color(SDL_Color color) noexcept { m_cover_color = color; }
  void set_cover_texture(TextureId texture_id) noexcept
  {
    m_cover_texture = texture_id;
  }
  void set_frame_size(FrameSize<int> frame) noexcept { m_frame_size = frame; }
  void set_frame_color(SDL_Color color) noexcept { m_frame_color = color; }
  void set_frame_texture(TextureId texture_id) noexcept
  {
    m_frame_texture = texture_id;
  }

private:
  void draw_helper(DrawQueue& queue,
                   Box<int> bounds,
                   FrameSize<int> frame,
                   Box<float> tx_frag) const;

  TextureId m_cover_texture{ NULL_TEXTURE };
  SDL_Color m_cover_color{ 0, 0, 0, 255 };
  FrameSize<int> m_frame_size{ 0, 0, 0, 0 };
  SDL_Color m_frame_color{ 0, 0, 0, 0 };
  TextureId m_frame_texture{ NULL_TEXTURE };
};


class Reel : public Drawable
{
public:
  // TODO fix: 0 cards break motion calculations
  Reel(uint16_t n_cards = 1, uint16_t n_lines_visible = 1) noexcept;
  DrawableBox& get_card(uint16_t i) { return m_cards[i]; }
  ReelMotion& get_motion() noexcept { return m_motion_state; }
  uint16_t get_n_lines() const noexcept { return m_nlines; }
  void set_n_lines(uint16_t n_lines) noexcept { m_nlines = n_lines; }
  void resize(uint16_t n_cards);

  void draw(DrawQueue& queue, Box<int> bounds) const override;

private:
  std::vector<DrawableBox> m_cards;
  ReelMotion m_motion_state;
  uint16_t m_nlines;
};


class Button : Drawable
{
public:
  enum class State
  {
    idle = 0,
    disabled,
    focused,
    clicked,
    number
  };

  Button(std::function<void(const SDL_Event&)> event_handler = {});
  void draw(DrawQueue& queue, Box<int> bounds) const override;
  void set_event_handler(std::function<void(const SDL_Event&)> handler);
  void handle_event(const SDL_Event& e);
  void set_enbaled(bool f_enabled) noexcept
  {
    m_state = f_enabled ? State::idle : State::disabled;
  }
  void set_default_appearance(DrawableBox appearance) noexcept;
  void set_disabled_appearance(DrawableBox apperance) noexcept;
  void set_hover_appearance(DrawableBox appearance) noexcept;

private:
  mutable Box<int> m_hitbox; // updated in draw method
  std::function<void(const SDL_Event&)> m_handler;
  DrawableBox m_default_appearance;
  DrawableBox m_hover_appearance;
  DrawableBox m_disabled_apperance;
  State m_state{ State::idle };
};


class ScoreCounter : Drawable
{
public:
  ScoreCounter(uint16_t n_max_digits);
  void set_texture_set(const std::array<TextureId, 10>& digit_textures);
  void set_score(uint32_t score);
  void update(float dt);
  void draw(DrawQueue& queue, Box<int> bounds) const override;

private:
  uint16_t m_ndigits;
  std::vector<Reel> m_reels;
};


class SlotMachine : public Drawable
{
public:
  SlotMachine(TextureCollection& tc);
  std::vector<Reel>& get_reels() noexcept { return m_reels; }
  Button& get_start_btn() noexcept { return m_start_btn; }
  Button& get_stop_btn() noexcept { return m_stop_btn; }
  ScoreCounter& get_score_counter() noexcept { return m_score_counter; }
  void update(float dt);
  void draw(DrawQueue& queue, Box<int> bounds) const override;

private:
  void draw_display(DrawQueue& queue, Box<int> bounds) const;
  void draw_control_panel(DrawQueue& queue, Box<int> bounds) const;
  void set_texture(TextureId tx_id) noexcept;

  std::vector<Reel> m_reels;
  Button m_start_btn;
  Button m_stop_btn;
  ScoreCounter m_score_counter;
  TextureId m_texture{ NULL_TEXTURE };
};


class Scene
{
public:
  Scene(TextureCollection& tc);
  void update(float dt);
  DrawQueue build(uint16_t wnd_width, uint16_t wnd_height) const;
  SlotMachine& get_machine() noexcept { return m_slot_machine; }

private:
  TextureCollection& m_tc;
  SlotMachine m_slot_machine;
};
#endif
