// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright © 2025 Mansur Mukhametzyanov
#include "game.hpp"
#include "configuration.hpp"
#include "scene.hpp"

#include <cassert>
#include <cstdint>
#include <memory>

Game::Game(TextureCollection& tc)
  : m_rng(std::random_device()())
  , m_timer_events()
  , m_scene(tc)
  , m_machine(m_scene.get_machine())
  , m_state(std::make_unique<IdleState>(*this))
{
  m_machine.get_start_btn().set_event_handler([this](const SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
      handle_event(Event::start_pressed);
    }
  });

  m_machine.get_stop_btn().set_event_handler([this](const SDL_Event& e) {
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
      handle_event(Event::stop_pressed);
    }
  });
}

void Game::update(float dt)
{
  check_timers();
  m_scene.update(dt);
}

void Game::process_input(const SDL_Event& input_event)
{
  m_machine.get_start_btn().handle_event(input_event);
  m_machine.get_stop_btn().handle_event(input_event);
}

Game::SymbolRow Game::get_symbol_row()
{
  SymbolRow row;
  for (uint32_t i = 0; i < g_nreels; ++i) {
    Reel& r = m_machine.get_reels()[i];
    float reel_pos = std::round(r.get_motion().get_position());
    uint32_t symbol_num = static_cast<uint32_t>(reel_pos) % g_nsymbols;
    row[i] = static_cast<Symbol>(symbol_num);
  }
  return row;
}

void Game::set_symbol_row(const SymbolRow& row)
{
  remove_highlight();
  m_timer_events.clear();
  m_state.reset(new IdleState(*this));

  for (uint32_t i = 0; i < row.size(); ++i) {
    Reel& r = m_machine.get_reels()[i];
    r.get_motion().stop_in(static_cast<float>(row[i]), 1.f);
  }
  add_timer_event(FloatSeconds{ 1.f }, Event::reels_stopped);
}

void Game::add_timer_event(FloatSeconds time, Event e)
{
  std::function<void()> handler = std::bind(&Game::handle_event, this, e);
  m_timer_events.emplace_back(PostponedEvent{ time, handler });
}

void Game::handle_event(Event e)
{
  std::unique_ptr<State> next_state{ m_state->next(e) };
  if (next_state.get() != nullptr) {
    m_state = std::move(next_state);
  }
}

void Game::check_timers()
{
  for (auto it = m_timer_events.begin(); it != m_timer_events.end();) {
    PostponedEvent& pe = *it;
    if (pe.timer.is_expired()) {
      pe.event_handler();

      auto cur_it = it++;
      m_timer_events.erase(cur_it);
    } else {
      ++it;
    }
  }
}

void Game::highlight_combo(Combination::Range r)
{
  SymbolRow row = get_symbol_row();
  for (uint32_t i = r.begin; i < r.begin + r.size; ++i) {
    Reel& r = m_machine.get_reels()[i];
    DrawableBox& c = r.get_card(static_cast<uint32_t>(row[i]));
    c.set_cover_color(g_combo_highlight);
  }
}

void Game::remove_highlight()
{
  SymbolRow row = get_symbol_row();
  for (uint32_t i = 0; i < row.size(); ++i) {
    Reel& r = m_machine.get_reels()[i];
    DrawableBox& c = r.get_card(static_cast<uint32_t>(row[i]));
    c.set_cover_color(g_symbol_card_color);
  }
}


Game::IdleState::IdleState(Game& g)
  : State(g)
{
  m_game.m_machine.get_start_btn().set_enbaled(true);
  m_game.m_machine.get_stop_btn().set_enbaled(false);
}

std::unique_ptr<Game::State> Game::IdleState::next(Event game_event)
{
  switch (game_event) {
    // Keep for testing purposes
    case Event::reels_stopped: {
      return std::make_unique<ResultState>(m_game, m_game.get_symbol_row());
    }
    case Event::start_pressed:
      return std::make_unique<SpeedUpState>(m_game);
    default:
      return {};
  }
}


Game::SpeedUpState::SpeedUpState(Game& g)
  : State(g)
{
  m_game.m_machine.get_start_btn().set_enbaled(false);

  auto acc_time_dist = std::uniform_real_distribution<float>(
    g_min_speed_up_time.count(), g_max_speed_up_time.count());

  std::vector<Reel>& reels = m_game.m_machine.get_reels();

  for (Reel& r : reels) {
    m_game.remove_highlight();
    // Accelerate reel to max speed in random time from interval
    r.get_motion().go_full_speed_in(acc_time_dist(m_game.m_rng));
  }

  m_game.add_timer_event(g_min_spin_time, Event::enable_stop_timer);
}

std::unique_ptr<Game::State> Game::SpeedUpState::next(Event game_event)
{
  switch (game_event) {
    case Event::enable_stop_timer:
      return std::make_unique<StopWaitState>(m_game);
    default:
      return {};
  }
}


Game::StopWaitState::StopWaitState(Game& g)
  : State(g)
{
  m_game.m_machine.get_stop_btn().set_enbaled(true);
  m_game.add_timer_event(g_max_spin_time - g_min_spin_time,
                         Event::spin_time_out);
}

std::unique_ptr<Game::State> Game::StopWaitState::next(Event game_event)
{
  switch (game_event) {
    case Event::spin_time_out:
    case Event::stop_pressed:
      return std::make_unique<SlowingDownState>(m_game);
    default:
      return {};
  }
}


Game::SlowingDownState::SlowingDownState(Game& g)
  : State(g)
{
  m_game.m_machine.get_stop_btn().set_enbaled(false);

  auto stop_time_dist = std::uniform_real_distribution<float>(
    g_min_stop_time.count(), g_max_stop_time.count());
  auto stop_symbol_dist = std::uniform_int_distribution<>(0, g_nsymbols - 1);

  std::vector<Reel>& reels = m_game.m_machine.get_reels();

  float last_stop_in;
  for (uint32_t i = 0; i < g_nreels; ++i) {
    Reel& r = reels[i];

    float stop_in = stop_time_dist(m_game.m_rng);
    uint32_t stop_pos = stop_symbol_dist(m_game.m_rng);
    r.get_motion().stop_in(static_cast<float>(stop_pos), stop_in);

    m_stop_row[i] = static_cast<Symbol>(stop_pos); // write down result
    last_stop_in = std::max(stop_in, last_stop_in);
  }

  m_game.add_timer_event(FloatSeconds(last_stop_in), Event::reels_stopped);
}

std::unique_ptr<Game::State> Game::SlowingDownState::next(Event game_event)
{
  switch (game_event) {
    case Event::reels_stopped:
      return std::make_unique<ResultState>(m_game, m_stop_row);
    default:
      return {};
  }
}


Game::ResultState::ResultState(Game& g, SymbolRow stop_row)
  : State(g)
  , m_stop_row(stop_row)
  , m_auto_spin(false)
{
  Combination combo(stop_row);
  Combination::Result res = combo.get_result();

  m_game.m_machine.get_score_counter().set_score(res.points);
  if (res.points > 0) {
    m_game.highlight_combo(res.combo_range);
  }

  m_auto_spin = res.free_speen;
  FloatSeconds time_out = m_auto_spin ? g_auto_spin_delay : g_result_show_time;
  m_game.add_timer_event(FloatSeconds(time_out), Event::show_result_time_out);
}

std::unique_ptr<Game::State> Game::ResultState::next(Event game_event)
{
  switch (game_event) {
    case Event::show_result_time_out:
      if (m_auto_spin) {
        return std::make_unique<SpeedUpState>(m_game);
      } else {
        return std::make_unique<IdleState>(m_game);
      }
    default:
      return {};
  }
}


Game::Timer::Timer(FloatSeconds period)
{
  using namespace std::chrono;
  m_expire_time =
    steady_clock::now() + duration_cast<steady_clock::duration>(period);
}

bool Game::Timer::is_expired()
{
  return std::chrono::steady_clock::now() >= m_expire_time;
}
