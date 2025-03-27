// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "scene.hpp"
#include "configuration.hpp"
#include "primitives.hpp"
#include "symbol.hpp"
#include "texture.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace {
int iround(float f)
{
  return static_cast<int>(std::lround(f));
}
}

DrawableBox::DrawableBox(SDL_Color color) noexcept
  : m_cover_color(color)
{
}

DrawableBox::DrawableBox(TextureId texture_id) noexcept
  : m_cover_texture(texture_id)
{
}

void DrawableBox::draw(DrawQueue& queue, Box<int> bounds) const
{
  draw_helper(queue, bounds, m_frame_size, Box<float>{ 0.f, 0.f, 1.f, 1.f });
}

void DrawableBox::draw_clipped(DrawQueue& queue,
                               Box<int> bounds,
                               Box<int> clip) const
{
  // Full box sizes
  int s[4] = { bounds.w, bounds.h, bounds.w, bounds.h };
  int b[4] = { bounds.x, bounds.y, bounds.x + bounds.w, bounds.y + bounds.h };
  int c[4] = { clip.x, clip.y, clip.x + clip.w, clip.y + clip.h };
  // Frame sizes
  int f[4] = {
    m_frame_size.left, m_frame_size.top, m_frame_size.right, m_frame_size.bottom
  };
  // Texture relative coords
  float tx[4] = { 0.f, 0.f, 1.f, 1.f };

  for (uint32_t i = 0; i < 4; ++i) {
    if (i < 2 && b[i] < c[i] || i >= 2 && b[i] > c[i]) {
      int dif = c[i] - b[i];
      int abs_dif = std::abs(dif);
      int sign = dif / abs_dif;

      b[i] += dif;
      if (abs_dif <= f[i]) {
        f[i] -= abs_dif;
      } else {
        abs_dif -= f[i];
        f[i] = 0;
        tx[i] += static_cast<float>(abs_dif * sign) / static_cast<float>(s[i]);
      }
    }
  }
  int w = b[2] - b[0];
  int h = b[3] - b[1];
  if (w < 0 || h < 0) { // Should not be possible
    assert(false);
    return;
  }
  bool frame_only = w < f[0] || w < f[2] || h < f[1] || h < f[3];
  if (frame_only) {
    Box<int> clipped_bounds{ b[0], b[1], b[2] - b[0], b[3] - b[1] };
    queue.add_colored_box(clipped_bounds, m_frame_color);
    return;
  }

  Box<int> clipped_bounds{ b[0], b[1], b[2] - b[0], b[3] - b[1] };
  FrameSize<int> clipped_frame{ f[0], f[1], f[2], f[3] };
  Box<float> tx_fragment{ tx[0], tx[1], tx[2] - tx[0], tx[3] - tx[1] };
  draw_helper(queue, clipped_bounds, clipped_frame, tx_fragment);
}

void DrawableBox::draw_helper(DrawQueue& queue,
                              Box<int> bounds,
                              FrameSize<int> frame,
                              Box<float> tx_frag) const
{
  FramedBox<int> fb = FramedBox<int>::create_inside(bounds, frame);

  if (m_cover_color.a != 0) { // cover background
    queue.add_colored_box(fb.get_inner_box(), m_cover_color);
  }
  if (m_cover_texture != NULL_TEXTURE) { // cover foreground
    queue.add_textured_box(fb.get_inner_box(), m_cover_texture, tx_frag);
  }
  if (!m_frame_size.is_null()) {
    if (m_frame_color.a != 0) { // frame background
      queue.add_colored_frame(fb, m_frame_color);
    }
    if (m_frame_texture != NULL_TEXTURE) { // frame foreground
      queue.add_textured_frame(fb, m_frame_texture);
    }
  }
}


Reel::Reel(uint16_t n_cards, uint16_t n_lines_visible) noexcept
  : m_cards(n_cards)
  , m_nlines(n_lines_visible)
  , m_motion_state(static_cast<float>(n_cards))
{
}

void Reel::resize(uint16_t n_cards)
{
  m_motion_state.set_reel_length(static_cast<float>(n_cards));
  m_cards.resize(n_cards);
}

void Reel::draw(DrawQueue& queue, Box<int> bounds) const
{
  const uint16_t n_cards = m_cards.size();
  const float length = static_cast<float>(n_cards);
  const uint16_t px_card_height = bounds.h / m_nlines;

  // Logical position coresponds to bottom row. Need to shift it to the middle
  const float shifted_pos = m_motion_state.get_position();
  const float dist_to_middle = static_cast<float>(m_nlines / 2);

  // Last symbols are visible now
  float visual_pos = shifted_pos - dist_to_middle;
  if (visual_pos < 0.f) {
    visual_pos += length;
  }

  // *_part values measured in fractions of reel length. 0.0 <= *_part <= 1.0
  const float rotation_part = visual_pos / length;
  // Drawable part of reel
  const float display_part = static_cast<float>(m_nlines) / length;
  const float card_part = 1.f / length;

  float display_bottom = rotation_part;
  float display_top = rotation_part + display_part;

  // m_nlines additional iterations to imitate full cycle
  for (uint16_t i = 0; i < n_cards + m_nlines; ++i) {
    // Relative to display bottom
    float card_bottom = card_part * static_cast<float>(i) - display_bottom;
    float card_top = card_part * static_cast<float>(i + 1) - display_bottom;

    bool visible = (card_bottom >= 0.f && card_bottom < display_part) ||
                   (card_top > 0.f && card_top <= display_part);

    if (visible) {
      Box<int> card_bounds = { bounds.x, -1, bounds.w, px_card_height };

      // Position of card in fractions of display height,
      // relative to display bottom
      float display_rel_y = card_top / display_part;
      float bounds_bottom_rel_y = display_rel_y * static_cast<float>(bounds.h);
      int rounded_y = iround(bounds_bottom_rel_y);

      card_bounds.y = bounds.y + bounds.h - rounded_y;

      m_cards[i % n_cards].draw_clipped(queue, card_bounds, bounds);
    }
  }
}


Button::Button(std::function<void(const SDL_Event&)> event_handler)
  : m_handler(event_handler)
{
}

void Button::draw(DrawQueue& queue, Box<int> bounds) const
{
  m_hitbox = bounds; // implicitly update hitbox

  switch (m_state) {
    case State::disabled:
      m_disabled_apperance.draw(queue, bounds);
      break;
    case State::idle:
    case State::clicked: // not implemented
      m_default_appearance.draw(queue, bounds);
      break;
    case State::focused:
      m_hover_appearance.draw(queue, bounds);
      break;
    default:
      break;
  }
}

void Button::set_event_handler(std::function<void(const SDL_Event&)> handler)
{
  m_handler = handler;
}

void Button::handle_event(const SDL_Event& e)
{
  if (m_state == State::disabled) {
    return;
  }

  float f_x, f_y;
  SDL_GetMouseState(&f_x, &f_y);
  int i_x = iround(f_x);
  int i_y = iround(f_y);
  if (!m_hitbox.contains(i_x, i_y)) {
    m_state = State::idle;
    return;
  }

  switch (e.type) {
    case SDL_EVENT_MOUSE_MOTION:
      m_state = State::focused;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      m_state = State::clicked;
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP: // not implemented
    case SDL_EVENT_MOUSE_WHEEL:     // not implemented
      break;
    default:
      assert(true); // got wrong event type
  }
  m_handler(e);
}

void Button::set_default_appearance(DrawableBox appearance) noexcept
{
  m_default_appearance = appearance;
}

void Button::set_disabled_appearance(DrawableBox apperance) noexcept
{
  m_disabled_apperance = apperance;
}

void Button::set_hover_appearance(DrawableBox appearance) noexcept
{
  m_hover_appearance = appearance;
}


ScoreCounter::ScoreCounter(uint16_t n_max_digits)
  : m_ndigits(n_max_digits)
{
  m_reels.resize(m_ndigits);
  for (Reel& r : m_reels) {
    r.resize(10);
    r.set_n_lines(1);
    r.get_motion().set_min_speed(0.f);

    for (uint16_t i = 0; i < 10; ++i) {
      Symbol s = static_cast<Symbol>(i);
      r.get_card(i).set_cover_color(g_score_card_color);
      r.get_card(i).set_frame_size(FrameSize{ 3 });
      r.get_card(i).set_frame_color(g_shadow_color);
    }
  }
}

void ScoreCounter::set_texture_set(
  const std::array<TextureId, 10>& digit_textures)
{
  for (Reel& r : m_reels) {
    for (uint16_t i = 0; i < 10; ++i) {
      r.get_card(i).set_cover_texture(digit_textures[i]);
    }
  }
}

void ScoreCounter::set_score(uint32_t score)
{
  for (uint32_t i = 0; i < m_ndigits; ++i) {
    uint32_t cur_digit = score % 10;
    score /= 10;

    Reel& r = m_reels[m_ndigits - 1 - i]; // From right to left
    r.get_motion().stop_in(static_cast<float>(cur_digit),
                           g_result_show_time.count());
  }
}

void ScoreCounter::update(float dt)
{
  for (Reel& r : m_reels) {
    r.get_motion().advance(dt);
  }
}

void ScoreCounter::draw(DrawQueue& queue, Box<int> bounds) const
{
  FramedBox<int> frame = FramedBox<int>::create_inside(bounds, FrameSize{ 3 });
  FramedGrid digit_row = FramedGrid::centered_in(frame, 1, m_ndigits);
  for (uint32_t i = 0; i < m_ndigits; ++i) {
    m_reels[i].draw(queue, digit_row.get_cell_box(0, i));
  }
  queue.add_colored_frame(digit_row.get_framed_box(), g_main_panel_color);
}


SlotMachine::SlotMachine(TextureCollection& tc)
  : m_score_counter(6)
{
  // App background
  m_texture = tc.get_id("background");

  // Configuring reels apearance
  m_reels.resize(g_nreels);
  float reel_visible_len = static_cast<float>(g_nlines);

  for (Reel& r : m_reels) {
    r.resize(g_nsymbols);
    r.set_n_lines(g_nlines);
    r.get_motion().set_min_speed(reel_visible_len * g_reel_min_speed);
    r.get_motion().set_max_speed(reel_visible_len * g_reel_max_speed);

    for (uint16_t i = 0; i < g_nsymbols; ++i) {
      Symbol s = static_cast<Symbol>(i);
      r.get_card(i).set_cover_texture(tc.get_id(get_symbol_name(s)));
      r.get_card(i).set_cover_color(g_symbol_card_color);
      r.get_card(i).set_frame_size(FrameSize{ 3 });
      r.get_card(i).set_frame_color(g_shadow_color);
    }
  }

  // Configuring buttons appearance
  FrameSize<int> released_frame = FrameSize{ 2, 4, 4, 2 };
  FrameSize<int> pressed_frame = FrameSize{ 3, 3, 3, 3 };

  DrawableBox btn_app;
  btn_app.set_cover_color(g_start_en_color);
  btn_app.set_cover_texture(tc.get_id("start_enabled"));
  btn_app.set_frame_size(released_frame);
  btn_app.set_frame_color(g_shadow_color);
  m_start_btn.set_default_appearance(btn_app);

  btn_app.set_cover_color(g_start_hv_color);
  m_start_btn.set_hover_appearance(btn_app);

  btn_app.set_cover_color(g_disabled_color);
  btn_app.set_cover_texture(tc.get_id("start_disabled"));
  btn_app.set_frame_size(pressed_frame);
  m_start_btn.set_disabled_appearance(btn_app);

  btn_app.set_cover_color(g_stop_en_color);
  btn_app.set_cover_texture(tc.get_id("stop_enabled"));
  btn_app.set_frame_size(released_frame);
  m_stop_btn.set_default_appearance(btn_app);

  btn_app.set_cover_color(g_stop_hv_color);
  m_stop_btn.set_hover_appearance(btn_app);

  btn_app.set_cover_color(g_disabled_color);
  btn_app.set_cover_texture(tc.get_id("stop_disabled"));
  btn_app.set_frame_size(pressed_frame);
  m_stop_btn.set_disabled_appearance(btn_app);

  // Setting score number textures
  m_score_counter.set_texture_set(tc.get_digits());
}

void SlotMachine::update(float dt)
{
  for (Reel& r : m_reels) {
    r.get_motion().advance(dt);
  }
  m_score_counter.update(dt);
}

void SlotMachine::draw(DrawQueue& queue, Box<int> bounds) const
{
  Box<float> f_bounds = bounds.cast_to<float>();
  int padding = iround(std::min(f_bounds.w, f_bounds.h) * 1.f / 20.f);
  float f_padding = static_cast<float>(padding);
  FramedBox<float> f_frame =
    FramedBox<float>::create_inside(f_bounds, FrameSize{ f_padding });

  Box<float> f_pad_bounds = f_frame.get_inner_box();
  Box<int> pad_bounds{ iround(f_pad_bounds.x),
                       iround(f_pad_bounds.y),
                       iround(f_pad_bounds.w),
                       iround(f_pad_bounds.h) };

  float hor_display_part = 0.7f;
  float control_panel_part = 1.f - hor_display_part;

  // Vertical spacing
  float vert_display_part = 0.75;
  float vert_space = 0.05f; // gap
  float score_counter_part = 1.f - vert_display_part - vert_space;

  Box<int> score_bounds{ pad_bounds.x,
                         pad_bounds.y,
                         iround(f_pad_bounds.w * hor_display_part),
                         iround(f_pad_bounds.h * score_counter_part) };
  m_score_counter.draw(queue, score_bounds);

  Box<int> vert_space_box{ pad_bounds.x,
                           score_bounds.y + score_bounds.h,
                           score_bounds.w,
                           iround(f_pad_bounds.h * vert_space) };
  queue.add_colored_box(vert_space_box, g_main_panel_color);

  Box<int> display_bounds{ pad_bounds.x,
                           vert_space_box.y + vert_space_box.h,
                           score_bounds.w,
                           iround(f_pad_bounds.h * vert_display_part) };
  draw_display(queue, display_bounds);

  Box<int> control_panel_bounds{ pad_bounds.x +
                                   iround(hor_display_part * f_pad_bounds.w),
                                 pad_bounds.y,
                                 iround(f_pad_bounds.w * control_panel_part),
                                 pad_bounds.h };
  draw_control_panel(queue, control_panel_bounds);

  // Drawing background
  FramedBox<int> i_frame =
    FramedBox<int>::create_inside(bounds, FrameSize{ padding });
  queue.add_textured_frame(i_frame, m_texture);
}

void SlotMachine::draw_display(DrawQueue& queue, Box<int> bounds) const
{
  const uint16_t n_lines = m_reels[0].get_n_lines();
  const uint16_t n_reels = m_reels.size();

  FramedGrid maximum_grid = FramedGrid::centered_in(bounds, n_lines, n_reels);
  Box<int> maximum_grid_box = maximum_grid.get_box();

  float frame_relative_thickness = 0.05;
  int frame_px_thickness =
    iround(static_cast<float>(maximum_grid_box.w) * frame_relative_thickness);

  FrameSize<int> sum_frame = maximum_grid.frame;
  sum_frame.add(FrameSize{ frame_px_thickness });

  FramedBox<int> frame = FramedBox<int>::create_inside(bounds, sum_frame);
  FramedGrid display_grid = FramedGrid::centered_in(frame, n_lines, n_reels);
  frame = display_grid.get_framed_box();
  int px_reel_width = display_grid.cell_w;
  int px_reel_height = display_grid.cell_h * n_lines;

  Box<int> reel_bounds;
  reel_bounds.x = display_grid.x;
  reel_bounds.y = display_grid.y;
  reel_bounds.w = px_reel_width;
  reel_bounds.h = px_reel_height;

  for (uint16_t i = 0; i < n_reels; ++i) {
    reel_bounds.x = display_grid.x + px_reel_width * i;
    m_reels[i].draw(queue, reel_bounds);
  }
  queue.add_colored_frame(display_grid.get_framed_box(), g_main_panel_color);
}

void SlotMachine::draw_control_panel(DrawQueue& queue, Box<int> bounds) const
{
  const float f_width = static_cast<float>(bounds.w);
  const float f_height = static_cast<float>(bounds.h);

  queue.add_colored_box(bounds, g_control_panel_color);

  Box<int> start_btn_box = bounds;
  start_btn_box.x += iround(f_width * 1.f / 5.f);
  start_btn_box.y += iround(f_height * 7.f / 10.f);
  start_btn_box.w = iround(f_width * 2.f / 3.f);
  start_btn_box.h = iround(f_height * 1.f / 5.f);
  m_start_btn.draw(queue, start_btn_box);

  Box<int> stop_btn_box = start_btn_box;
  stop_btn_box.y = bounds.y + iround(f_height * 1.f / 10.f);
  m_stop_btn.draw(queue, stop_btn_box);
}


Scene::Scene(TextureCollection& tc)
  : m_tc(tc)
  , m_slot_machine(tc)
{
}

void Scene::update(float dt)
{
  m_slot_machine.update(dt);
}

DrawQueue Scene::build(uint16_t wnd_width, uint16_t wnd_height) const
{
  DrawQueue q(m_tc, 128);
  Box<int> wnd_box = { 0, 0, wnd_width, wnd_height };
  m_slot_machine.draw(q, wnd_box);
  // SDL_Log("%td primitives to draw this frame", q.end() - q.begin());
  return q;
}
